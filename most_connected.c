// Sudoku_count
// Attempt to count the number of legal sudoku positions
// Based on Discussion between Kendrick Shaw MD PhD and Paul Alfille MD
// MIT license 2019
// by Paul H Alfille

// Version to find least connect ordering (least constrained)

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
# define SUBSIZE (3) // sqrt of SIZE
#define SIZE (SUBSIZE*SUBSIZE)
# define TOTALSIZE (SIZE*SIZE)
# define MAXCONSTRAINT (3*SIZE)

#define Zero(array) memset( array, 0, sizeof(array) ) ;

int constraints[SIZE][SIZE] ;
int order[SIZE][SIZE] ;

// up to 3 constraints defined
int first[MAXCONSTRAINT] ;

int filled = 0 ;

void InitFirst( void ) {
	int i ;
	for ( i=0 ; i<MAXCONSTRAINT ; ++i ) {
		first[i] = TOTALSIZE+1 ;
	}
}

void PrintFirst( void ) {
	int i ;
	printf("Filled=%d %d",filled,first[0] ) ;
	for ( i=1 ; i<MAXCONSTRAINT ; ++i ) {
		printf(",%d",first[i]) ; ;
	}
	printf("\n");
}

void print_square3( void ) {
	int i ;
	int j ;
			
	// Initial blank
	fprintf(stderr,"\n");

	// top line
	for ( j=0 ; j<SIZE ; ++j ) {
		fprintf(stderr,"+-----");
	}
	// end of top line
	fprintf(stderr,"+\n");
	
	for (i=0 ; i<SIZE ; ++i ) { // each row
		for ( j=0 ; j<SIZE ; ++j ) {
			int c = (j%SUBSIZE)?':':'|' ;
			fprintf(stderr,"%c%4d ",c,order[i][j]);
		}
		// end of row
		fprintf(stderr,"|\n");
		
		// Separator line
		for ( j=0 ; j<SIZE ; ++j ) {
			int c = ((i+1)%SUBSIZE)?' ':'-';
			fprintf(stderr,"+%c-%c-%c",c,c,c);
		}
		// end of separator
		fprintf(stderr,"+\n");
	} 

	// Final blank
	fprintf(stderr,"\n");
}   

void AddConstraint( int i , int j ) {
	int xi,xj ;
	// column
	for (xi=0 ; xi<SIZE ; ++xi ) {
		++constraints[xi][j];
	}
	// row
	for (xj=0 ; xj<SIZE ; ++xj ) {
		++constraints[i][xj];
	}
	// subsquare
	for (xi=0 ; xi<SUBSIZE ; ++xi ) {
		int ii = SUBSIZE * (i/SUBSIZE) + xi ;
		for (xj=0 ; xj<SUBSIZE ; ++xj ) {
			int jj = SUBSIZE * (j/SUBSIZE) + xj ;
			++constraints[ii][jj];
		}
	}
	// push this slot off the map!
	constraints[i][j] = -MAXCONSTRAINT-1 ;
}

void find_next( void ) {
	int high = -1 ;
	int i, j ;
	int besti, bestj ;
	for( i=0 ; i<SIZE ; ++i ) {
		for (j=0 ; j<SIZE ; ++j ) {
			if ( constraints[i][j] > high ) {
				besti = i;
				bestj = j;
				high = constraints[i][j] ;
			}
		}
	}
	++filled ;
	order[besti][bestj] = filled ;
	AddConstraint(besti,bestj);
	if ( filled < first[high] ) {
		first[high] = filled ;
	} 
}

int main(int argc, char ** argv) {
    int c ; 
    
	Zero(order) ;
	Zero(constraints) ;
	InitFirst() ;
	

	while ( filled < TOTALSIZE ) {
		find_next() ;
	}
	PrintFirst() ;
	print_square3() ;
	
    return 0 ;
}
