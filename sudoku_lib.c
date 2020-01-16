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

// SIZE x SIZE sudoku board
#ifndef SUBSIZE
#define SUBSIZE 3
#endif
#define SIZE (SUBSIZE*SUBSIZE)
# define TOTALSIZE (SIZE*SIZE)

#define Zero(array) memset( array, 0, sizeof(array) ) ;

// bit pattern
int pattern[SIZE] ;
int full_pattern ;

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
#define MAXTRACK 80
struct FillState {
    int mask_bits[SIZE][SIZE] ;
    int free_state[SIZE][SIZE] ;
} State[MAXTRACK] ;

struct StateStack {
    int size ;
    int start ;
    int end ;
    char * back ;
} statestack = { 0,0,0,NULL };

void StateStackCreate( int size ) {
    if ( size < 0 ) {
        fprintf(stderr,"Backtracking depth %d corrected to %d\n",size,0);
        size = 0 ;
    } else if ( size > MAXTRACK-1 ) {
        fprintf(stderr,"Backtracking depth %d corrected to %d\n",size,MAXTRACK-1);
        size = MAXTRACK-1 ;
    }
    statestack.size = size+1 ;
    if ( statestack.back == NULL ) {
        statestack.back = malloc( 11 ) ;
    }
    snprintf( statestack.back, 10, "B%d" , size ) ;    
}

#define VAL2FREE( v ) ( -((v)+1) )
#define FREE2VAL(f ) ( -((f)+1) )

struct FillState * StateStackInit ( void ) {
    statestack.start = statestack.end = 0 ;
    memset( & State[0].mask_bits, 0, sizeof( State[0].mask_bits ) ) ;
    memset( & State[0].free_state, SIZE, sizeof( State[0].free_state ) ) ;
    return & State[0] ;
}

struct FillState * StateStackReset( void ) {
	// Clear stack, use current as top of
	statestack.start = statestack.end ;
	return & State[statestack.end] ;
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
			fprintf(stderr,"%c%2X ",c,reverse_pattern(FREE2VAL(pFS->free_state[i][j])));
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

struct FillState * Set_Square( struct FillState * pFSold, int testi, int testj ) {
	int si, sj, k ;
	int val ;
	int mask = pFSold->mask_bits[testi][testj] ;
	int b ;
	struct FillState * pFS = pFSold ;
	
	// find clear bit
	for ( val = 0 ; val < SIZE ; ++val ) {
		b = pattern[val] ;
		if ( b & mask == 0 ) {
			break ;
		}
	}
	// should never fall though

	// point
	pFS->mask_bits[testi][testj] |= b ;
	--pFS->free_state[testi][testj] ;
	
	// Push state (with current choice masked out) if more than one choice
	if ( pFS->free_state[testi][testj] > 0 ) {
		pFS = StateStackPush() ;
	}
	// Now set this choice
	pFS->free_state[testi][testj] = VAL2FREE(val) ;
	
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
	return pFS ;
}
		


struct FillState * Next_move( struct FillState * pFS, int * done ) {
	int i ;
	int j ;
	int minfree = SIZE + 1 ;
	int fi, fj ;
	
	for ( i=0 ; i < SIZE ; ++i ) {
		for ( j=0 ; j< SIZE ; ++j ) {
			int free_state = pFS->free_state[i][j] ;
			if ( free_state < 0 ) {
				// already set
				continue ;		
			}
			if ( free_state == 0 ) {
				return NULL ;
			}
			if ( free_state == 1 ) {
				return Set_Square( pFS, i, j ) ;
			}
			if ( free_state < minfree ) {
				minfree = free_state ;
				fi = i ;
				fj = j ;
			}
		}
	}
	if ( minfree == SIZE+1 ) {
		// only set squares -- Victory
		// only set squares -- Victory
		* done = 1 ;
		return pFS ;
	}
	
	// Try smallest choice (most constrained)
	return Set_Square( pFS, fi, fj ) ;
}

struct FillState * SolveLoop( void ) {
	int done = 0 ;
	struct FillState * pFS = StateStackReset() ; // get rid of stack so can't backup preset
	
	while ( done == 0 ) {
		pFS = Next_move( pFS , &done ) ;
		if ( pFS == NULL ) {
			pFS = StateStackPop() ;
		}
		if ( pFS == NULL ) {
			return NULL ;
		}
	}
	print_square( pFS ) ;
	return pFS ;
}



int Setup_board( int * preset ) {
	// preset is an array of TOTALSIZE length
	// sent from python
	// only values on (0 -- SIZE-1) accepted
	// rest ignored
	// so use -1 for enpty cells
	
	int i, j ;
	int * set = preset ; // pointer though preset array
	struct FillState * pFS ;
	
	StateStackCreate( TOTALSIZE - 1 ) ;
	pFS = StateStackInit( ) ;
	
	for ( i=0 ; i<SIZE ; ++i ) {
		for ( j=0 ; j<SIZE ; ++j ) {
			if ( set[0] > -1 && set[0] < SIZE ) {
				pFS->free_state[i][j] = VAL2FREE( set[0] ) ;
				pFS = Set_Square( pFS, i, j ) ;
				if ( pFS == NULL ) {
					// bad input 
					return 0 ;
				}
			}
			++set ; // move to next entry
		}
	}
	return 1 ;
}

int Return_board( int * preset, struct FillState * pFS ) {
	int i, j ;
	int * set = preset ; // pointer though preset array
	
	if ( pFS ) {
		// solved
		for ( i=0 ; i<SIZE ; ++i ) {
			for ( j=0 ; j<SIZE ; ++j ) {
				set[0] = FREE2VAL( pFS->free_state[i][j] ) ;
				++set ; // move to next entry
			}
		}
		return 1 ;
	} else {
		// unsolvable
		return 0 ;
	}
}
	
int Solve( int * preset ) {
	if ( Setup_board( preset ) ) {
		return Return_board( preset, SolveLoop() ) ;
	} else {
		// bad input (inconsistent soduku)
		return Return_board( preset, NULL ) ;
	}
}
	
int Test( int * test ) {
	int i ;
	for (i=0 ; i<20 ; ++i ) {
		printf("%d\n",test[i]);
	}
	test[4] = 1 ;
	return 1 ;
}
	
