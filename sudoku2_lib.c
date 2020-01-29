// Sudoku_count
// Attempt to count the number of legal sudoku positions
// Based on Discussion between Kendrick Shaw MD PhD and Paul Alfille MD
// MIT license 2019
// by Paul H Alfille

#include "sudoku_count.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <signal.h>

time_t next_time ;

// SIZE x SIZE sudoku board
#ifndef SUBSIZE
#define SUBSIZE 3
#endif
#define SIZE (SUBSIZE*SUBSIZE)
# define TOTALSIZE (SIZE*SIZE)

#define Zero(array) memset( array, 0, sizeof(array) ) ;

#define VAL2FREE( v ) ( -((v)+1) )
#define FREE2VAL( f ) ( -((f)+1) )

void RuptHandler( int sig ) {
	exit(1);
}



// Special
static int Xpattern ;
static int Wpattern ;
static int Debug ;
static int ReDoCount ;

// bit pattern
static int pattern[SIZE] ;
static int full_pattern ;

// Make numbers int bit pattern (1=0x1 2=0x2 3=0x4...)
void make_pattern(void) {
    int i ;
    full_pattern = 0 ;
    for (i=0;i<SIZE;++i) {
        pattern[i] = 1<<i ;
        full_pattern |= pattern[i] ;
    }
}

// Find number from bit pattern (for printing)
int reverse_pattern( int pat ) {
    int i ;
    for (i=0 ; i<SIZE;++i) {
        if ( pattern[i] == pat ) {
            return i+1 ;
        }
    }
    // should never happen
    return 0 ;
}

// For backtracking state
#define MAXTRACK TOTALSIZE

struct FillState {
    int mask_bits[SIZE][SIZE] ;
    int free_state[SIZE][SIZE] ;
    int * preset ;
    int done ;
} State[MAXTRACK] ;

static struct FillState * pFS_interrupted = NULL ;

struct StateStack {
    int size ;
    int start ;
    int end ;
} statestack = { 0,0,0 };

struct FillState * StateStackInit ( int * preset ) {
	int i,j ;
    statestack.size = TOTALSIZE ;
    statestack.start = statestack.end = 0 ;
	for ( i=0 ; i<SIZE ; ++i ) {
		for ( j=0 ; j<SIZE ; ++j ) {
			State[0].mask_bits[i][j] = 0 ;
			State[0].free_state[i][j] = SIZE ;
		}
	}
	State[0].done = 0 ;
	State[0].preset = preset ;
	
    return & State[0] ;
}


struct FillState * StateStackPush( void ) {
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
    if ( statestack.start == statestack.end ) {
        // empty
        return NULL ;
    } else {
        statestack.end = ( statestack.end + statestack.size - 1 ) % statestack.size ; // make sure positive modulo
        return & State[statestack.end] ;
    }
}

int CheckSS( int i, int j, struct FillState * pFS ) {
	// Check subsquare
	
	int si,sj ;
	int mask = 0 ;
	
	for (si=0 ; si<SUBSIZE ; ++si ) {
		for ( sj=0 ; sj<SUBSIZE ; ++sj ) {
			int val = FREE2VAL( pFS->free_state[si+i][sj+j] );
			if ( val >= 0 ) {
				mask |= pattern[val] ;
			} else {
				mask |= (~pFS->mask_bits[si+i][sj+j] & full_pattern) ;
			}
			if ( mask == full_pattern ) {
				return 0;
			}
		}
	}
	//printf("UnavailableSS\n");
	return 1 ;
}

int CheckR( int i, struct FillState * pFS ) {
	// Check subsquare
	
	int j ;
	int mask = 0 ;
	
	for (j=0 ; j<SIZE ; ++j ) {
		int val = FREE2VAL( pFS->free_state[i][j] );
		if ( val >= 0 ) {
			mask |= pattern[val] ;
		} else {
			mask |= (~pFS->mask_bits[i][j] & full_pattern) ;
		}
		if ( mask == full_pattern ) {
			return 0;
		}
	}
	//printf("UnavailableR\n");
	return 1 ;
}

int CheckC( int j, struct FillState * pFS ) {
	// Check subsquare
	
	int i ;
	int mask = 0 ;
	
	for (i=0 ; i<SIZE ; ++i ) {
		int val = FREE2VAL( pFS->free_state[i][j] );
		if ( val >= 0 ) {
			mask |= pattern[val] ;
		} else {
			mask |= (~pFS->mask_bits[i][j] & full_pattern) ;
		}
		if ( mask == full_pattern ) {
			return 0;
		}
	}
	//printf("UnavailableC\n");
	return 1 ;
}

int CheckD1( struct FillState * pFS ) {
	// Check subsquare
	
	int i ;
	int mask = 0 ;
	
	for (i=0 ; i<SIZE ; ++i ) {
		int val = FREE2VAL( pFS->free_state[i][i] );
		if ( val >= 0 ) {
			mask |= pattern[val] ;
		} else {
			mask |= (~pFS->mask_bits[i][i] & full_pattern) ;
		}
		if ( mask == full_pattern ) {
			return 0;
		}
	}
	//printf("UnavailableD1\n");
	return 1 ;
}

int CheckD2( struct FillState * pFS ) {
	// Check subsquare
	
	int i ;
	int mask = 0 ;
	
	for (i=0 ; i<SIZE ; ++i ) {
		int j = SIZE-1-i ;
		int val = FREE2VAL( pFS->free_state[i][j] );
		if ( val >= 0 ) {
			mask |= pattern[val] ;
		} else {
			mask |= (~pFS->mask_bits[i][j] & full_pattern) ;
		}
		if ( mask == full_pattern ) {
			return 0;
		}
	}
	//printf("UnavailableD2\n");
	return 1 ;
}

int CheckAvailable( struct FillState * pFS ) {
	// Check all row columns, subsqaures and extra for possible coverage
	// i.e. that a needed value isn't excluded
	// Return 1 if not possible
	
	int i,j ;
	
	for (i=0 ; i<SIZE ; ++i ) {
		if ( CheckR( i, pFS ) ) {
			return 1 ;
		}
	}
	
	for (i=0; i<SIZE; i+=SUBSIZE) {
		for (j=0; j<SIZE; j+=SUBSIZE) {
			if (CheckSS(i,j,pFS)) {
				return 1 ;
			}
		}
	}
	
	if ( Wpattern ) {
		for (i=1; i<SIZE; i+=SUBSIZE+1) {
			for (j=1; j<SIZE; j+=SUBSIZE+1) {
				if (CheckSS(i,j,pFS)) {
					return 1 ;
				}
			}
		}
	}
	
	for (j=0 ; j<SIZE ; ++j ) {
		if ( CheckC( j, pFS ) ) {
			return 1 ;
		}
	}
	
	if ( Xpattern ) {
		if ( CheckD1( pFS ) || CheckD2( pFS ) ) {
			return 1 ;
		}
	}
	
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
            int v = FREE2VAL( pFS->free_state[i][j] );
			fprintf(stderr,"%c%2X ",c,v>=0?v:0xA);
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
	int val = FREE2VAL( pFS->free_state[testi][testj] );
	int b ;
	//char debug ;
	//printf("Set %d %d bit=%X mask=%X\n",testi,testj,pFS->free_state[testi][testj],pFS->mask_bits[testi][testj]);
	//debug = getchar() ;

	if ( val >= 0 ) {
		// already set (preset from GUI)
		//fprintf(stderr,"Set: Position %d,%d already set\n",testi,testj) ;
		b = pattern[val] ;
		if ( (b&pFS->mask_bits[testi][testj]) ) {
			return NULL ;
		}
		// point mask
		pFS->mask_bits[testi][testj] |= b ;
	} else {
		// Not already set (Part of solve search)
		// find clear bit
		int mask = pFS->mask_bits[testi][testj] ;
		int v ;
		for ( v = 0 ; v < SIZE ; ++v ) {
			// Start fram a more varied number
			val = (testi+testj+v) % SIZE ;
			b = pattern[val] ;
			//printf("val=%d b=%X mask=%X b&mask=%X\n",val,b,mask,b&mask ) ;
			if ( (b & mask) == 0 ) {
				//printf("Try val %d at %d,%d\n",val,testi,testj);
				break ;
			}
		}
		// should never fall though

		// point
		pFS->mask_bits[testi][testj] |= b ;
		--pFS->free_state[testi][testj] ;
		
		// Push state (with current choice masked out) if more than one choice
		if ( pFS->free_state[testi][testj] > 0 ) {
			if ( Debug ) {
				printf("Push");
			}
			pFS = StateStackPush() ;
		}
		// Now set this choice
		pFS->free_state[testi][testj] = VAL2FREE(val) ;
	
	}


	// mask out row everywhere else
	for( k=0 ; k<SIZE ; ++k ) {
		// row
		if ( pFS->free_state[testi][k] < 0 ) {
			// assigned
			continue ;
		}
		if ( pFS->mask_bits[testi][k] & b ) {
			// already masked
			continue ;
		}
		if ( pFS->free_state[testi][k] == 1 ) {
			// nothing left
			return NULL ;
		}
		pFS->mask_bits[testi][k] |= b ;
		--pFS->free_state[testi][k] ;
	}

	// mask out column everywhere else
	for( k=0 ; k<SIZE ; ++k ) {
		// row
		if ( pFS->free_state[k][testj] < 0 ) {
			// assigned
			continue ;
		}
		if ( pFS->mask_bits[k][testj] & b ) {
			// already masked
			continue ;
		}
		if ( pFS->free_state[k][testj] == 1 ) {
			// nothing left
			return NULL ;
		}
		pFS->mask_bits[k][testj] |= b ;
		--pFS->free_state[k][testj] ;
	}

	// subsquare
	for( si=0 ; si<SUBSIZE ; ++si ) {
		for( sj=0 ; sj<SUBSIZE ; ++sj ) {
			int i = SUBSIZE * (testi/SUBSIZE) + si ;
			int j = SUBSIZE * (testj/SUBSIZE) + sj ;
			// row
			if ( pFS->free_state[i][j] < 0 ) {
				// assigned
				continue ;
			}
			if ( pFS->mask_bits[i][j] & b ) {
				// already masked
				continue ;
			}
			if ( pFS->free_state[i][j] == 1 ) {
				// nothing left
				return NULL ;
			}
			pFS->mask_bits[i][j] |= b ;
			--pFS->free_state[i][j] ;
		}
	}
	
	// Xpattern
	if ( Xpattern ) {
		if ( testi==testj ) {
			for ( k=0 ; k<SIZE ; ++k ) {
				if ( pFS->free_state[k][k] < 0 ) {
					// assigned
					continue ;
				}
				if ( pFS->mask_bits[k][k] & b ) {
					// already masked
					continue ;
				}
				if ( pFS->free_state[k][k] == 1 ) {
					// nothing left
					return NULL ;
				}
				pFS->mask_bits[k][k] |= b ;
				--pFS->free_state[k][k] ;
			}
		} else if ( testi==SIZE-1-testj ) {
			for ( k=0 ; k<SIZE ; ++k ) {
				int l = SIZE - 1 - k ;
				if ( pFS->free_state[k][l] < 0 ) {
					// assigned
					continue ;
				}
				if ( pFS->mask_bits[k][l] & b ) {
					// already masked
					continue ;
				}
				if ( pFS->free_state[k][l] == 1 ) {
					// nothing left
					return NULL ;
				}
				pFS->mask_bits[k][l] |= b ;
				--pFS->free_state[k][l] ;
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
					if ( pFS->free_state[i][j] < 0 ) {
						// assigned
						continue ;
					}
					if ( pFS->mask_bits[i][j] & b ) {
						// already masked
						continue ;
					}
					if ( pFS->free_state[i][j] == 1 ) {
						// nothing left
						return NULL ;
					}
					pFS->mask_bits[i][j] |= b ;
					--pFS->free_state[i][j] ;
				}
			}
		}
	}
	
	return pFS ;
}
		


struct FillState * Next_move( struct FillState * pFS ) {
	int i ;
	int j ;
	int minfree = SIZE + 1 ;
	int fi, fj ;
		
    //print_square( pFS ) ;
    for ( i=0 ; i < SIZE ; ++i ) {
		for ( j=0 ; j< SIZE ; ++j ) {
			int free_state = pFS->free_state[i][j] ;
			if ( free_state < 0 ) {
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

struct FillState * SolveLoop( struct FillState * pFS ) {	
	time_t next = time(NULL) + 6 ; // every minute +
	while ( pFS && pFS->done == 0 ) {
		//printf("Solveloop\n");
		pFS = Next_move( pFS ) ;
		if ( (pFS == NULL) || CheckAvailable( pFS ) ) {
			if ( Debug ) {
				printf("Pop");
			}
			pFS = StateStackPop() ;
			if ( next < time(NULL) ) {
				// Interrupt for time -- can resume
				return pFS ;
			}
		}
	}
	//print_square( pFS ) ;
	return pFS ;
}

struct FillState * Setup_board( int * preset ) {
	// preset is an array of TOTALSIZE length
	// sent from python
	// only values on (0 -- SIZE-1) accepted
	// rest ignored
	// so use -1 for enpty cells
	
	int i, j ;
	int * set = preset ; // pointer though preset array
	struct FillState * pFS = StateStackInit( preset ) ;
	
	//printf("LIB\n") ;
	make_pattern() ; // set up bit pattern list
	
	for ( i=0 ; i<SIZE ; ++i ) {
		for ( j=0 ; j<SIZE ; ++j ) {
			if ( set[0] > -1 && set[0] < SIZE ) {
				pFS->free_state[i][j] = VAL2FREE( set[0] ) ;
				//print_square( pFS ) ;
				//printf("LIB: %d -> %d -> %d\n",set[0],VAL2FREE(set[0]),FREE2VAL(VAL2FREE(set[0])));
				pFS = Set_Square( pFS, i, j ) ;
				if ( pFS == NULL ) {
					// bad input 
					return NULL ;
				}
			}
			++set ; // move to next entry
		}
	}
	//printf("PostLIB\n") ;
	return pFS ;
}

int Return_board( struct FillState * pFS ) {
	if ( pFS ) {
		int i, j ;
		int * set ; // pointer though preset array
		// solved
		//printf("Return\n");
		set = pFS->preset ;
		for ( i=0 ; i<SIZE ; ++i ) {
			for ( j=0 ; j<SIZE ; ++j ) {
				int val = FREE2VAL( pFS->free_state[i][j] ) ; 
				//printf("(%d)",val ) ;
				set[0] = val >= 0 ? val : -1 ;
				++set ; // move to next entry
			}
		}
		if ( pFS->done ) {
			// solved
			if ( Debug ) {
				printf(" :)\n");
			}
			pFS_interrupted = NULL ;
			return 1 ;
		} else {
			// interrupted -- want to resume
			if ( Debug ) {
				printf(" :0\n");
			}
			pFS_interrupted = pFS ;
			return --ReDoCount ;
		}
	} else {
		if (Debug) {
			printf(" :(\n");
		}
		// unsolvable
		return 0 ;
	}
}
	
int Resume( void ) {

	// resume processing -- all data alreay statically set by prior call to Solve
	struct FillState * pFS = pFS_interrupted ;
	
	pFS_interrupted = NULL ; // clear prior

	//printf("X=%d, W=%d\n",Xpattern,Wpattern) ;
	if ( pFS ) {
		return Return_board( SolveLoop( pFS ) ) ;
	} else {
		// bad input (inconsistent soduku)
		return Return_board( NULL ) ;
	}
}
	
// return 1=solved, 0 not. data in same array
int Solve( int X, int Window, int debug, int * preset ) {
	struct FillState * pFS = Setup_board( preset ) ;
	
	signal( SIGINT, RuptHandler ) ;
	pFS_interrupted = NULL ;

	Xpattern = X ;
	Wpattern = Window ;
	Debug = debug ;
	ReDoCount = 0 ;
	
	//printf("X=%d, W=%d\n",Xpattern,Wpattern) ;
	//printf("V2\n");
	
	if ( pFS ) {
		return Return_board( SolveLoop( pFS ) ) ;
	} else {
		// bad input (inconsistent soduku)
		return Return_board( NULL ) ;
	}
}
	
