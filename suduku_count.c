// Suduku_count
// Attempt to count the number of legal suduku positions
// Based on Discussion between Kendrick Shaw MD PhD and Paul Alfille MD
// MIT license 2019
// by Paul H Alfille

#include "suduku_count.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// SIZE x SIZE suduku board
#define SIZE (9)
# define SUBSIZE (3) // sqrt of SIZE

#define Zero(array) memset( array, 0, sizeof(array) ) ;

typedef enum { Good_Square, Candidate_Square, Bad_Square } square_ret ;

// bit pattern
int pattern[SIZE] ;
int full_pattern ;
void make_pattern(void) {
    int i ;
    full_pattern = 0 ;
    for (i=0;i<SIZE;++i) {
        pattern[i] = 1<<i ;
        full_pattern |= pattern[i] ;
    }
}
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

// Full board (holds bit patterns)
int bit[SIZE][SIZE] ;

int find_valid_bit( int mask ) {
    int trial ;
    if ( mask == full_pattern) {
        return 0 ;
    }
    do {
        trial = pattern[ RANDOM % SIZE ] ;
    } while ( trial & mask ) ;
    //printf("trial %x mask %x MATCH %x\n",trial,mask,mask&trial);
    return trial ;
}
        
void print_square( void ) {
    int i ;
    int j ;
    printf("\n");
    for ( j=0 ; j<SIZE ; ++j ) {
        printf("+---");
    }
    printf("+\n");
    for (i=0 ; i<SIZE ; ++i ) {
        
        for ( j=0 ; j<SIZE ; ++j ) {
            printf("|%2d ",reverse_pattern(bit[i][j]));
        }
        printf("|\n");
        for ( j=0 ; j<SIZE ; ++j ) {
            printf("+---");
        }
        printf("+\n");
    } 
    printf("\n");
}    

square_ret fill_square( void ) {
    int i,j ;
    int col_bits[SIZE] ;
    int row_bits[SIZE] ;
    
    Zero(bit) ;
    
    // column bits culmulative
    Zero( col_bits ) ;
    
    // row bits culmulative
    Zero( row_bits ) ;
    
    // Fill columns and rows
    for (i=0;i<SIZE;++i) {
        for (j=0;j<SIZE;++j) {
            int b = find_valid_bit( col_bits[j]|row_bits[i] ) ;
            if (b == 0 ) {
                return Bad_Square ;
            }
            row_bits[i] |= b ;
            col_bits[j] |= b ;
            bit[i][j] = b ;
        }
    }
    
    // Test subsquares
    for( i=0 ; i<SIZE ; i+=SUBSIZE) {
        for( j=0 ; j<SIZE ; j+=SUBSIZE ) {
            int sub_bits = 0 ;
            int si,sj;
            for (si=0 ; si<SUBSIZE ; ++si ) {
                for (sj=0 ; sj<SUBSIZE ; ++sj ) {
                    sub_bits |= bit[i+si][j+sj] ;
                }
            }
            if ( sub_bits != full_pattern ) {
                return Candidate_Square ;
            }
        }
    } 
    
    return Good_Square ;
}


int main(void) {
    int bad=0 ;
    int candidate=0 ;
    int good=0;
    clock_t start ;
    
    SEED ;
    make_pattern();
    
    start = clock() ;

    while ( 1 ) {
        switch( fill_square() ) {
            case Bad_Square:
                ++bad ;
                break ;
            case Candidate_Square:
                ++candidate ;
                if ( candidate % 10000 == 0 ) {
					int total = bad+candidate+good ;
                    printf("Bad=%d, Candidate=%d, Good=%d\tper second=%g.2\n\t\t%.6f%%\t%.6f%%\n",bad,candidate,good,(double)(CLOCKS_PER_SEC*total)/(clock()-start),(100.*candidate)/total,(100.*good)/total) ;
                }
                break ;
            case Good_Square:
                ++good ;
                print_square() ;
                if ( 1 ) {
					int total = bad+candidate+good ;
                    printf("Bad=%d, Candidate=%d, Good=%d\tper second=%g.2\n\t\t%.6f%%\t%.6f%%\n",bad,candidate,good,(double)(CLOCKS_PER_SEC*total)/(clock()-start),(100.*candidate)/total,(100.*good)/total) ;
                }
                break ;
        }
    }
    printf("Bad=%d, Candidate=%d, Good=%d\n\t\t%.2f%%\t%.2f%%\n",bad,candidate,good,(100.*candidate)/(bad+candidate+good),(100.*good)/(bad+candidate+good)) ;
    
}
