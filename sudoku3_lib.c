// Sudoku_count
// Attempt to count the number of legal sudoku positions
// Based on Discussion between Kendrick Shaw MD PhD and Paul Alfille MD
// MIT license 2019
// by Paul H Alfille

#include "sudoku_count.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <signal.h>
#include <pthread.h>

// Thread synchronization (signal to thread and back)
enum solve_state { 
	solve_setup=0,
	solve_error=1,
	solve_invalid=2,
	solve_valid=3, 
	solve_working=4, 
	solve_unsolveable=5, 
	solve_solveable=6, 
	solve_unique=7, 
	solve_multiple=8, 
} SolveState ;

pthread_mutex_t solvestate_lock = PTHREAD_MUTEX_INITIALIZER ;

pthread_t worker_thread ;

// SIZE x SIZE sudoku board
#ifndef SUBSIZE
#define SUBSIZE 3
#endif
#define SIZE (SUBSIZE*SUBSIZE)
# define TOTALSIZE (SIZE*SIZE)

#define Zero(array) memset( array, 0, sizeof(array) ) ;

void RuptHandler( int sig ) {
	exit(1);
}

// Special
int * preset = NULL ;
static int Xpattern ;
static int Wpattern ;
static int Debug ;

// bit pattern
static int pattern[SIZE+1] ;
static int full_pattern ;

// Make numbers int bit pattern (1=0x1 2=0x2 3=0x4...)
void make_pattern(void) {
    int i ;
    full_pattern = 0 ;
    for (i=0;i<SIZE;++i) {
        pattern[i+1] = 1<<i ;
        full_pattern |= pattern[i+1] ;
    }
}

int Count_bits( uint32_t i ) {
     // Java: use >>> instead of >>
     // C or C++: use uint32_t
     i -= ((i >> 1) & 0x55555555);
     i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
     return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

// Find number from bit pattern (for printing)
int reverse_pattern( int pat ) {
    int i ;
    for (i=1 ; i<=SIZE; ++i) {
        if ( pattern[i] == pat ) {
            return i ;
        }
    }
    // should never happen
    return 0 ;
}

// For backtracking state
#define MAXTRACK TOTALSIZE

struct FillState {
    int mask_bits[SIZE][SIZE] ; // bits unavailable
    int value[SIZE][SIZE] ; // solution (<=0) or number of available solutions
    int done ;
} State[MAXTRACK] ;

struct FillState Available ;

struct StateStack {
    int size ;
    int start ;
    int end ;
} statestack = { 0,0,0 };

struct FillState * StateStackInit ( void ) {
	int i,j ;
    statestack.size = TOTALSIZE ;
    statestack.start = statestack.end = 0 ;
	for ( i=0 ; i<SIZE ; ++i ) {
		for ( j=0 ; j<SIZE ; ++j ) {
			State[0].mask_bits[i][j] = full_pattern ; // all possible
			State[0].value[i][j] = 0 ; // none set
		}
	}
	State[0].done = 0 ;
	
    return & State[0] ;
}

struct FillState * StateStackPush( void ) {
	if ( Debug ) {
		printf("Push");
	}
    if ( statestack.size == 0 ) {
        return & State[0] ;
    } else {
        int oldend = statestack.end ;
        statestack.end = (statestack.end+1) % statestack.size ;
        if ( statestack.end == statestack.start ) {
            // overfilled -- move the start
            statestack.start = (statestack.start+1) % statestack.size ;
        }
        memcpy( & State[statestack.end], & State[oldend], sizeof( struct FillState ) ) ;
        return & State[statestack.end] ;
    }
}

struct FillState * StateStackPop( void ) {
	if ( Debug ) {
		printf("Pop");
	}
    if ( statestack.start == statestack.end ) {
        // empty
        return NULL ;
    } else {
        statestack.end = ( statestack.end + statestack.size - 1 ) % statestack.size ; // make sure positive modulo
        return & State[statestack.end] ;
    }
}

struct FillState * StateStackCurrent( void ) {
	return & State[statestack.end] ;
}

void KillThread( void ) {
	static int threads = 0 ;
	
	if ( threads ) {
		void * res ;
		switch( pthread_cancel( worker_thread ) )
		{
			case ESRCH:
				//fprintf(stderr,"No thread?\n");
				break ;
			case 0:
				switch ( pthread_join(worker_thread, &res ) )
				{
					case 0:
					case ESRCH:
						break ;
					default:
						fprintf(stderr,"Couldn't merge with killed thread\n");
						exit(1) ;
				}
			default:
				fprintf(stderr,"Couldn't cancel existing thread\n") ;
				exit(1);
		}
	}
	threads = 1 ;
}

void StartThread( void * (*func)( void *) ) 
{
	SolveState = solve_working ;
	if ( pthread_create( & worker_thread, NULL, func, NULL ) ) {
		fprintf( stderr, "Couldn't create the thread\n" ) ;
		exit(1) ;
	}
}

struct Subsets {
	int * mask_bits[SIZE] ;
	int entries ;
} Subset ;

void Subset_init( void ) {
	Subset.entries = 0 ;
}

void Subset_add( struct FillState * pFS, int i, int j ) {
	//printf( "(%d,%d) ",i,j);
	Subset.mask_bits[ Subset.entries++ ] = &(pFS->mask_bits[i][j]) ;
}

void Subset_print( void ) {
	if ( Subset.entries ) {
		int i ;
		printf("\nSubset %d ",Subset.entries) ;
		for ( i=0 ; i<Subset.entries ; ++i ) {
			printf("%p ",Subset.mask_bits[i] ) ;
		}
		printf("\n");
		for ( i=0 ; i<Subset.entries ; ++i ) {
			printf("%X ",Subset.mask_bits[i][0] ) ;
		}
		printf("\n");
	}
}

int Subset_union( int pat ) {
	int U = 0 ;
	int ** p ;
	for( p=Subset.mask_bits; pat ; pat>>=1, ++p ) {
		if (pat&0x1) {
			U |= **p ;
		}
	}
	return U ;
}

int Subset_remove( int pat, int mask ) {
	int ** p ;
	int any = 0 ;
	int apat = ( (1<<Subset.entries) -1 ) ^ pat ;
	for( p=Subset.mask_bits; apat ; apat>>=1, ++p ) {
		if ( apat&0x1 ) {
			any |= ((**p) & mask ) ;
			**p &= (full_pattern ^ mask) ;
		}
	}
	return any ;
}

// Check each supset to see if
// 1==not enough choices
// 0 fine
int Subset_test( int * retest ) {
	int pat ; // patterns for subset (actually a bit map)
	int all_bits = (1<<Subset.entries) - 1 ;
	
	// Full Subset
	if ( Count_bits(Subset_union( all_bits ) ) < Subset.entries ) {
		// too few choices
		if ( Debug ) {
			printf("Set sparse ") ;
		}
		return 1 ;
	}
	
	for ( pat = 2 ; pat < all_bits ; ++pat ) {
		int cob_pat = Count_bits( pat ) ;
		if ( cob_pat > 1 ) { // no point in looking at singles
			int U = Subset_union( pat ) ;
			int cob_U = Count_bits( U ) ;
			if ( cob_U < cob_pat ) {
				// not enough choices in the subset
				if ( Debug ) {
					printf("Subset sparse ") ;
				}
				return 1 ;
			} else if ( cob_U == cob_pat ) {
				// just enough in proper subset
				*retest |= Subset_remove( pat, U ) ;
			}	
		}
	}
	return 0 ;
}

void CheckSS( int i, int j, struct FillState * pFS ) {
	// Check subsquare
	
	int si,sj ;
	
	for (si=0 ; si<SUBSIZE ; ++si ) {
		for ( sj=0 ; sj<SUBSIZE ; ++sj ) {
			if ( pFS->value[si+i][sj+j] == 0 ) {
				Subset_add( pFS, si+i, sj+j ) ;
			}
		}
	}
}

void CheckR( int i, struct FillState * pFS ) {
	// Check subsquare
	
	int j ;
	
	for (j=0 ; j<SIZE ; ++j ) {
		if ( pFS->value[i][j] == 0 ) {
			Subset_add( pFS, i, j ) ;
		}
	}
}

void CheckC( int j, struct FillState * pFS ) {
	// Check subsquare
	
	int i ;
	
	for (i=0 ; i<SIZE ; ++i ) {
		if ( pFS->value[i][j] == 0 ) {
			Subset_add( pFS, i, j ) ;
		}
	}
}

void CheckD1( struct FillState * pFS ) {
	// Check subsquare
	
	int i ;
	
	for (i=0 ; i<SIZE ; ++i ) {
		if (  pFS->value[i][i] == 0 ) {
			Subset_add( pFS, i, i ) ;
		}
	}
}

void CheckD2( struct FillState * pFS ) {
	// Check subsquare
	
	int i ;
	
	for (i=0 ; i<SIZE ; ++i ) {
		int j = SIZE-1-i ;
		if ( pFS->value[i][j] == 0 ) {
			Subset_add( pFS, i, j ) ;
		}
	}
}

int CheckAvailable( struct FillState * pFS ) {
	// Check all row columns, subsqaures and extra for possible coverage
	// i.e. that a needed value isn't excluded
	// Return 1 if not possible
	
	int i,j ;
	int retest ;
	
	do {
		retest = 0 ;
		
		for (i=0 ; i<SIZE ; ++i ) {
			Subset_init() ;
			CheckR( i, pFS ) ;
			if ( Subset_test( &retest ) ) {
				return 1 ;
			}
		}
		
		for (i=0; i<SIZE; i+=SUBSIZE) {
			for (j=0; j<SIZE; j+=SUBSIZE) {
				Subset_init() ;
				CheckSS(i,j,pFS) ;
				if ( Subset_test( &retest ) ) {
					return 1 ;
				}
			}
		}
		
		if ( Wpattern ) {
			for (i=1; i<SIZE; i+=SUBSIZE+1) {
				for (j=1; j<SIZE; j+=SUBSIZE+1) {
					Subset_init() ;
					CheckSS(i,j,pFS) ;
					if ( Subset_test( &retest ) ) {
						return 1 ;
					}
				}
			}
		}
		
		for (j=0 ; j<SIZE ; ++j ) {
			Subset_init() ;
			CheckC( j, pFS ) ; 
			if ( Subset_test( &retest ) ) {
				return 1 ;
			}
		}
		
		if ( Xpattern ) {
			Subset_init() ;
			CheckD1( pFS ) ;
			if ( Subset_test( &retest ) ) {
				return 1 ;
			}
			
			Subset_init() ;
			CheckD2( pFS ) ;
			if ( Subset_test( &retest ) ) {
				return 1 ;
			}
		}
	} while ( retest ) ;
	
	return 0 ;
}

void print_square( struct FillState * pFS ) {
	int i ;
	int j ;
	
	// Initial blank
	fprintf(stderr,"\n");

	// top line
	for ( j=0 ; j<SIZE ; ++j ) {
		fprintf(stderr,"+---");
	}
	// end of top line
	fprintf(stderr,"+\n");
	
	for (i=0 ; i<SIZE ; ++i ) { // each row
		for ( j=0 ; j<SIZE ; ++j ) {
			int c = (j%SUBSIZE)?':':'|' ;
			fprintf(stderr,"%c%2X ",c,pFS->value[i][j]);
		}
		// end of row
		fprintf(stderr,"|\n");
		
		// Separator line
		for ( j=0 ; j<SIZE ; ++j ) {
			int c = ((i+1)%SUBSIZE)?' ':'-';
			fprintf(stderr,"+%c-%c",c,c);
		}
		// end of separator
		fprintf(stderr,"+\n");
	} 

	// Final blank
	fprintf(stderr,"\n");
}   

// Enter with current position set in free_state
struct FillState * Set_Square( struct FillState * pFS, int testi, int testj ) {
	int si, sj, k ;
	int val = pFS->value[testi][testj] ;
	int b, antib ;
	static int offset = 0 ;
	//printf("Set %d %d bit=%X mask=%X\n",testi,testj,pFS->value[testi][testj],pFS->mask_bits[testi][testj]);

	if ( val > 0 ) {
		// already set (preset from GUI)
		//fprintf(stderr,"Set: Position %d,%d already set\n",testi,testj) ;
		b = pattern[val] ;
		if ( (b&pFS->mask_bits[testi][testj]) == 0 ) {
			return NULL ;
		}
		// point mask
		pFS->mask_bits[testi][testj] ^= b ;
	} else {
		// Not already set (Part of solve search)
		// find clear bit
		int mask = pFS->mask_bits[testi][testj] ;
		int v ;
		for ( v = 0 ; v < SIZE ; ++v ) {
			// Start fram a more varied number
			val = ((offset+v) % SIZE) + 1 ;
			b = pattern[val] ;
			//printf("val=%d b=%X mask=%X b&mask=%X\n",val,b,mask,b&mask ) ;
			if ( b & mask ) {
				//printf("Try val %d at %d,%d\n",val,testi,testj);
				break ;
			}
		}
		// should never fall though
		offset += testi + testj ;
		
		// point
		pFS->mask_bits[testi][testj] ^= b ;
		
		// Push state (with current choice masked out) if more than one choice
		if ( pFS->mask_bits[testi][testj] ) {
			pFS = StateStackPush() ;
		}
		// Now set this choice
		pFS->value[testi][testj] = val ;
	
	}

	antib = full_pattern ^ b ;

	// mask out row everywhere else
	for( k=0 ; k<SIZE ; ++k ) {
		// row
		if ( pFS->value[testi][k] > 0 ) {
			// assigned
			continue ;
		}
		pFS->mask_bits[testi][k] &= antib ;
		if ( pFS->mask_bits[testi][k] == 0 ) {
			// nothing left
			return NULL ;
		}
	}

	// mask out column everywhere else
	for( k=0 ; k<SIZE ; ++k ) {
		// row
		if ( pFS->value[k][testj] > 0 ) {
			// assigned
			continue ;
		}
		pFS->mask_bits[k][testj] &= antib ;
		if ( pFS->mask_bits[k][testj] == 0 ) {
			// nothing left
			return NULL ;
		}
	}

	// subsquare
	for( si=0 ; si<SUBSIZE ; ++si ) {
		for( sj=0 ; sj<SUBSIZE ; ++sj ) {
			int i = SUBSIZE * (testi/SUBSIZE) + si ;
			int j = SUBSIZE * (testj/SUBSIZE) + sj ;
			// row
			if ( pFS->value[i][j] > 0 ) {
				// assigned
				continue ;
			}
			pFS->mask_bits[i][j] &= antib ;
			if ( pFS->mask_bits[i][j] == 0 ) {
				// nothing left
				return NULL ;
			}
		}
	}
	
	// Xpattern
	if ( Xpattern ) {
		if ( testi==testj ) {
			for ( k=0 ; k<SIZE ; ++k ) {
				if ( pFS->value[k][k] > 0 ) {
					// assigned
					continue ;
				}
				pFS->mask_bits[k][k] &= antib ;
				if ( pFS->mask_bits[k][k] == 0 ) {
					// nothing left
					return NULL ;
				}
			}
		} else if ( testi==SIZE-1-testj ) {
			for ( k=0 ; k<SIZE ; ++k ) {
				int l = SIZE - 1 - k ;
				if ( pFS->value[k][l] > 0 ) {
					// assigned
					continue ;
				}
				pFS->mask_bits[k][l] &= antib ;
				if ( pFS->mask_bits[k][l] == 0 ) {
					// nothing left
					return NULL ;
				}
			}
		}
	}

	// Window Pane
	if ( Wpattern ) {
		if ( (testi%(SUBSIZE+1))!=0 && (testj%(SUBSIZE+1))!=0 ) {
			for( si=0 ; si<SUBSIZE ; ++si ) {
				int i = (testi / (SUBSIZE+1)) * (SUBSIZE+1) + 1 + si ;
				for( sj=0 ; sj<SUBSIZE ; ++sj ) {
					int j = (testj / (SUBSIZE+1)) * (SUBSIZE+1) + 1 + sj ;
					// row
					if ( pFS->value[i][j] > 0 ) {
						// assigned
						continue ;
					}
					pFS->mask_bits[i][j] &= antib ;
					if ( pFS->mask_bits[i][j] == 0 ) {
						// nothing left
						return NULL ;
					}
				}
			}
		}
	}
	
	return pFS ;
}

// Enter with current position set in free_state for availability list
void Set_Square_Available( int val, int testi, int testj ) {
	int si, sj, k ;
	int b, antib ;

	// already set (preset from GUI)
	b = pattern[val] ;
	// point mask
	Available.mask_bits[testi][testj] ^= b ;

	antib = full_pattern ^ b ;

	// mask out row everywhere else
	for( k=0 ; k<SIZE ; ++k ) {
		// row
		Available.mask_bits[testi][k] &= antib ;
	}

	// mask out column everywhere else
	for( k=0 ; k<SIZE ; ++k ) {
		// row
		Available.mask_bits[k][testj] &= antib ;
	}

	// subsquare
	for( si=0 ; si<SUBSIZE ; ++si ) {
		for( sj=0 ; sj<SUBSIZE ; ++sj ) {
			int i = SUBSIZE * (testi/SUBSIZE) + si ;
			int j = SUBSIZE * (testj/SUBSIZE) + sj ;
			// row
			Available.mask_bits[i][j] &= antib ;
		}
	}
	
	// Xpattern
	if ( Xpattern ) {
		if ( testi==testj ) {
			for ( k=0 ; k<SIZE ; ++k ) {
				Available.mask_bits[k][k] &= antib ;
			}
		} else if ( testi==SIZE-1-testj ) {
			for ( k=0 ; k<SIZE ; ++k ) {
				int l = SIZE - 1 - k ;
				Available.mask_bits[k][l] &= antib ;
			}
		}
	}

	// Window Pane
	if ( Wpattern ) {
		if ( (testi%(SUBSIZE+1))!=0 && (testj%(SUBSIZE+1))!=0 ) {
			for( si=0 ; si<SUBSIZE ; ++si ) {
				int i = (testi / (SUBSIZE+1)) * (SUBSIZE+1) + 1 + si ;
				for( sj=0 ; sj<SUBSIZE ; ++sj ) {
					int j = (testj / (SUBSIZE+1)) * (SUBSIZE+1) + 1 + sj ;
					// row
					Available.mask_bits[i][j] &= antib ;
				}
			}
		}
	}
}

struct FillState * Next_move( struct FillState * pFS ) {
	int i ;
	int j ;
	int minfree = SIZE + 1 ;
	int fi, fj ;
		
    //print_square( pFS ) ;
    for ( i=0 ; i < SIZE ; ++i ) {
		for ( j=0 ; j< SIZE ; ++j ) {
			int free_state = Count_bits(pFS->mask_bits[i][j]) ;
			if ( pFS->value[i][j] > 0 ) {
				// already set
				//printf("Slot %d,%d already set\n",i,j);
				continue ;		
			}
			if ( free_state == 0 ) {
				if ( Debug ) {
					printf("0");
				}
				//printf("Slot %d,%d exhausted\n",i,j);
				return NULL ;
			}
			if ( free_state == 1 ) {
				if ( Debug ) {
					printf("1");
				}
				//printf("Slot %d,%d exactly specified\n",i,j);
				return Set_Square( pFS, i, j ) ;
			}
			//printf("Minfree %d free_state %d\n",minfree,free_state);
			if ( free_state < minfree ) {
				minfree = free_state ;
				fi = i ;
				fj = j ;
			}
		}
	}
	if ( minfree == SIZE+1 ) {
		// only set squares -- Victory
		pFS->done = 1 ; // all done
		return pFS ;
	}
	
	// Try smallest choice (most constrained)
	//printf("Slot %d,%d will be probed (free %d)\n",fi,fj,minfree);
	if ( Debug ) {
		printf("%d",minfree);
	}
	return Set_Square( pFS, fi, fj ) ;
}

void * SolveLoop( void * v ) {	
	struct FillState * pFS = StateStackCurrent() ;
	while ( pFS && pFS->done == 0 ) {
		//printf("Solveloop\n");
		pFS = Next_move( pFS ) ;
		if ( (pFS == NULL) || CheckAvailable( pFS ) ) {
			pFS = StateStackPop() ;
		}
	}
	//print_square( pFS ) ;
	pthread_mutex_lock( &solvestate_lock ) ;
	SolveState = pFS==NULL ? solve_unsolveable : solve_solveable ;
	pthread_mutex_unlock( &solvestate_lock ) ;
	
	return pFS ;
}

struct FillState * SolveLoop1( struct FillState * pFS ) {	
	while ( pFS && pFS->done == 0 ) {
		pFS = Next_move( pFS ) ;
		if ( (pFS == NULL) || CheckAvailable( pFS ) ) {
			pFS = StateStackPop() ;
		}
	}

	return pFS ;
}

// For TestUnique -- no pause/resume
// Find second solution
// Assumes pFS not NULL initially
struct FillState * SolveLoop2( void ) {
	struct FillState * pFS = StateStackPop() ; // Go back to prior
	if (pFS == NULL ) {
		return NULL ;
	}
	pFS->done = 0 ; // Look again
	while ( pFS && pFS->done == 0 ) {
		//printf("Solveloop\n");
		pFS = Next_move( pFS ) ;
		if ( (pFS == NULL) || CheckAvailable( pFS ) ) {
			pFS = StateStackPop() ;
		}
	}

	return pFS ;
}

void * SolveLoopU( void * v ) {	
	struct FillState * pFS = StateStackCurrent() ;
	
	pFS = SolveLoop1( pFS ) ;

	pthread_mutex_lock( &solvestate_lock ) ;
	SolveState = pFS==NULL ? solve_unsolveable : solve_solveable ;
	pthread_mutex_unlock( &solvestate_lock ) ;

	if ( pFS ) {
		pFS = SolveLoop2() ;
		
		pthread_mutex_lock( &solvestate_lock ) ;
		SolveState = pFS==NULL ? solve_unique : solve_multiple ;
		pthread_mutex_unlock( &solvestate_lock ) ;
	}
	
	return pFS ;
}

struct FillState * Setup_board( void ) {
	// preset is an array of TOTALSIZE length
	// sent from python
	// only values on (0 -- SIZE-1) accepted
	// rest ignored
	// so use -1 for enpty cells
	
	int i, j ;
	int * set = preset; // pointer though preset array
	struct FillState * pFS ;
	
	KillThread() ;
	SolveState = solve_setup ;
	
	pFS = StateStackInit() ; // needs make_pattern

	// Standard for search and solve
	for ( i=0 ; i<SIZE ; ++i ) {
		for ( j=0 ; j<SIZE ; ++j ) {
			int v = set[0] ;
			if ( v > 0 && v <= SIZE ) {
				pFS->value[i][j] = v ;
				pFS = Set_Square( pFS, i, j ) ;
				if ( pFS == NULL ) {
					// bad input (inconsistent)
					SolveState = solve_invalid ;
					return NULL ;
				}
			} else if ( v != 0 ) {
				fprintf( stderr,"Input value out of range: %d (%d,%d)\n",v,i,j);
				SolveState = solve_error ;
				return NULL ;
			}
			++set ; // move to next entry
		}
	}
	//printf("PostLIB\n") ;
	SolveState = solve_valid ;
	return pFS ;
}

int GetStatus( void ) {
	int r ;
	pthread_mutex_lock( & solvestate_lock ) ;
	r = (int) SolveState ;
	pthread_mutex_unlock( & solvestate_lock ) ;
	return r ;
}

int GetBoard( void ) {
	struct FillState * pFS = StateStackCurrent() ;
	int * set = preset ;
	if ( pFS ) {
		int i, j ;
		for ( i=0 ; i<SIZE ; ++i ) {
			for ( j=0 ; j<SIZE ; ++j ) {
				set[0] = pFS->value[i][j] ; 
				++set ; // move to next entry
			}
		}
	}
	return GetStatus() ;
}
	
int GetAvailable( int testi, int testj, int * return_list ) {
	int i, j ;
	int * set = preset ; // pointer though preset array

	KillThread() ;
	SolveState = solve_setup ;
	
	// Single Available
	for ( i=0 ; i<SIZE ; ++i ) {
		for ( j=0 ; j<SIZE ; ++j ) {
			Available.mask_bits[i][j] = full_pattern ;
		}
	}	
	for ( i=0 ; i<SIZE ; ++i ) {
		for ( j=0 ; j<SIZE ; ++j ) {
			int v = set[0] ;
			if ( v > 0 && v <= SIZE ) {
				if ( i!=testi || j!=testj ) {
					Set_Square_Available( v, i, j ) ;
				}
			}
			++set ; // move to next entry
		}
	}

	// set up return list as empty first
	for ( i=0 ; i<SIZE ; ++i ) {
		return_list[i] = 0 ;
	}
	
	for ( i=0 ; i<SIZE ; ++i ) {
		int pos = i+1 ; //1-based values
		if ( Available.mask_bits[testi][testj] & pattern[pos] ) {
			return_list[i] = pos ;
		}
	}

	return GetStatus() ;
}

int Solve( void ) {
	switch (SolveState) {
		case solve_error:
		case solve_invalid:
		case solve_unsolveable:
			break ;
		default:
			StartThread( SolveLoop ) ;
			break ;
	}
	return GetStatus() ;
}
	
int GetUnique( void ) {
	switch (SolveState) {
		case solve_error:
		case solve_invalid:
		case solve_unsolveable:
			break ;
		default:
			StartThread( SolveLoopU ) ;
			break ;
	}
	return GetStatus() ;
}

int Setup( int X, int Window, int debug, int * arr ) {
	Xpattern = X ;
	Wpattern = Window ;
	Debug = debug ;
	preset = arr ;

	make_pattern() ; // set up bit pattern list

	Setup_board() ;
	
	signal( SIGINT, RuptHandler ) ;

	return GetStatus() ;
}

int SetBoard( void ) {
	
	Setup_board() ;
	
	signal( SIGINT, RuptHandler ) ;

	return GetStatus() ;
}


