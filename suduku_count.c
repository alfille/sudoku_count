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
#include <unistd.h>
#include <stdlib.h>

// SIZE x SIZE suduku board
#define SIZE (9)
# define SUBSIZE (3) // sqrt of SIZE
# define TOTALSIZE (SIZE*SIZE)

#define Zero(array) memset( array, 0, sizeof(array) ) ;

typedef enum { Good_Square, Candidate_Square, Bad_Square } square_ret ;
clock_t start ;

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
            fprintf(stderr,"%c%2d ",c,reverse_pattern(bit[i][j]));
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

square_ret Type1_fill_square( void ) {
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

int Type1Loop() {
    int bad=0 ;
    int candidate=0 ;
    int good=0;

    while ( 1 ) {
        switch( Type1_fill_square() ) {
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
}

square_ret Type2_fill_square( void ) {
    int i,j,k ;
    int col_bits[SIZE] ;
    int row_bits[SIZE] ;
    
    Zero(bit) ;
    
    // column bits culmulative
    Zero( col_bits ) ;
    
    // row bits culmulative
    Zero( row_bits ) ;
    
    // Fill columns and rows
    for (k=0;k<SIZE;++k) {
        for (j=k;j<SIZE;++j) {
            int b = find_valid_bit( col_bits[j]|row_bits[k] ) ;
            if (b == 0 ) {
                return Bad_Square ;
            }
            row_bits[k] |= b ;
            col_bits[j] |= b ;
            bit[k][j] = b ;
		}
        for (i=k+1;i<SIZE;++i) {
            int b = find_valid_bit( col_bits[k]|row_bits[i] ) ;
            if (b == 0 ) {
                return Bad_Square ;
            }
            row_bits[i] |= b ;
            col_bits[k] |= b ;
            bit[i][k] = b ;
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

int Type2Loop() {
    int bad=0 ;
    int candidate=0 ;
    int good=0;

    while ( 1 ) {
        switch( Type2_fill_square() ) {
            case Bad_Square:
                ++bad ;
                break ;
            case Candidate_Square:
                ++candidate ;
                if ( candidate % 10000 == 0 ) {
					int total = bad+candidate+good ;
                    printf("Bad=%d, Candidate=%d, Good=%d\tper second=%g.2\t%.6f%%\t%.6f%%\n",bad,candidate,good,(double)(CLOCKS_PER_SEC*total)/(clock()-start),(100.*candidate)/total,(100.*good)/total) ;
                }
                break ;
            case Good_Square:
                ++good ;
                print_square() ;
                if ( 1 ) {
					int total = bad+candidate+good ;
                    printf("Bad=%d, Candidate=%d, Good=%d\tper second=%g.2\t%.6f%%\t%.6f%%\n",bad,candidate,good,(double)(CLOCKS_PER_SEC*total)/(clock()-start),(100.*candidate)/total,(100.*good)/total) ;
                }
                break ;
        }
    }
}

int SS1_fill_square( void ) {
    int si,sj,ssi,ssj,count=0 ;
    int col_bits[SIZE] ;
    int row_bits[SIZE] ;
    int ss_bits[SUBSIZE][SUBSIZE] ;
    
    Zero(bit) ;
    
    // column bits culmulative
    Zero( col_bits ) ;
    
    // row bits culmulative
    Zero( row_bits ) ;
    
    // subsquare bits culmulative
    Zero( ss_bits ) ;
    
    // Fill columns and rows
    for (si=0;si<SUBSIZE;++si) {
        for (sj=0;sj<SUBSIZE;++sj) {
			for (ssi=0;ssi<SUBSIZE;++ssi) {
				for ( ssj=0;ssj<SUBSIZE;++ssj,++count) {
					int i = SUBSIZE*si+ssi ;
					int j = SUBSIZE*sj+ssj ;
					int b = find_valid_bit( col_bits[j]|row_bits[i]|ss_bits[si][sj] ) ;
					if (b == 0 ) {
						return count ;
					}
					row_bits[i] |= b ;
					col_bits[j] |= b ;
					ss_bits[si][sj] |= b ;
					bit[i][j] = b ;
				}
			}
		}
    }
	return count ;
}

int SS1Loop() {
	int count = 0 ;
	int good = 0 ;
	uint64_t totalcount = 0 ;

    while ( 1 ) {
		int c = SS1_fill_square() ;
		++count ;
		totalcount += c ;
		if ( c == TOTALSIZE ) {
			++good ;
			print_square() ;
			printf("count=%d, Good=%d\taverage=%g.1\tper second=%.1f\t%.6f%%\n",count,good,(double)totalcount/count,(double)(CLOCKS_PER_SEC*count)/(clock()-start),(100.*good)/count) ;
		} else if ( count % 100000 == 0 ) {
			printf("count=%d, Good=%d\taverage=%g.1\tper second=%.1f\t%.6f%%\n",count,good,(double)totalcount/count,(double)(CLOCKS_PER_SEC*count)/(clock()-start),(100.*good)/count) ;
		}
    }
}

void help(void) {
	printf(
	"suduku_count by Paul H Alfille 2019 MIT License\n"
	"\tSearch the possible suduku space for valid positions\n"
	"\tUses several search algorithms and shows statistics\n"
	"\tSee https:\\github.com/alfille/suduku_count\n"
	"\n"
	"\t -t 1\tSearch columns first, then test for subsquares (default)\n"
	"\t -t 2\tSearch columns/row alternating, then test for subsquares\n"
	"\t -s 1\tSearch subsquares by column, show failure point\n"
	"\t -h  \tShow these instructions\n"
	);
}

int main(int argc, char ** argv) {
	int c ;

    SEED ;
    make_pattern();
        
    start = clock() ;

	do {
		switch( getopt( argc, argv, "ht:s:" ) ) {
			case 't':
				switch (atoi(optarg)) {
					case 2:
						Type2Loop();
						break ;
					default:
						Type1Loop() ;
						break ;
				}
				break ;
			case 's':
				switch (atoi(optarg)) {
					case 1:
						SS1Loop();
						break ;
					default:
						SS1Loop() ;
						break ;
				}
				break ;
			case -1:
				Type1Loop();
				break;
			case 'h':
			default:
				help() ;
				break;
		} 
	} while (0) ;
	return 0 ;
}
