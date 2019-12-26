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
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

// SIZE x SIZE sudoku board
#define SIZE (9)
# define SUBSIZE (3) // sqrt of SIZE
# define TOTALSIZE (SIZE*SIZE)

#define Zero(array) memset( array, 0, sizeof(array) ) ;

clock_t start ;

FILE * fsolutions = NULL ;
FILE * fdistribution = NULL ;

uint64_t max_tries = UINT64_MAX / TOTALSIZE ;

// bit pattern
int pattern[SIZE] ;
int full_pattern ;

uint64_t distribution[TOTALSIZE+1];

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
    
    // print to file?
    if ( fsolutions ) {
        fprintf(fsolutions,"%X",reverse_pattern(bit[0][0]));
        for (i=0;i<SIZE;++i) {
            for (j=1;j<SIZE;++j) {
                fprintf(fsolutions,",%X",reverse_pattern(bit[i][j]));
            }
        }
        fprintf(fsolutions,"\n");
    }
    
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
            fprintf(stderr,"%c%2X ",c,reverse_pattern(bit[i][j]));
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

void Distribution( void ) {
	if ( fdistribution ) {
		int d ;
		fprintf(fdistribution,"%"PRIu64,distribution[0]) ;
		for ( d=1;d<=TOTALSIZE;++d) {
			fprintf(fdistribution,",%"PRIu64,distribution[d]) ; 
		}
		fprintf(fdistribution,"\n");
	}
}

int Type1_fill_square( void ) {
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
                return i*SIZE+j ;
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
                return TOTALSIZE ;
            }
        }
    } 
    
    return -1 ;
}

void TypeLoop( int (*fill)(void) ) {
    uint64_t bad=0 ;
    uint64_t candidate=0 ;
    uint64_t good=0;
    uint64_t count ;

    for (count=0;count<=max_tries;++count ) {
		int f = fill() ;
        switch( f ) {
            case -1:
                ++good ;
                print_square() ;
                ++distribution[TOTALSIZE] ;
                if ( 1 ) {
                    uint64_t total = bad+candidate+good ;
                    printf("Bad=%"PRIu64", Candidate=%"PRIu64", Good=%"PRIu64"\tper second=%g.2\n\t\t%.6f%%\t%.6f%%\n",bad,candidate,good,(double)(CLOCKS_PER_SEC*total)/(clock()-start),(100.*candidate)/total,(100.*good)/total) ;
                }
                break ;
            case TOTALSIZE:
                ++candidate ;
                ++distribution[TOTALSIZE] ;
                if ( candidate % 10000 == 0 ) {
                    uint64_t total = bad+candidate+good ;
                    printf("Bad=%"PRIu64", Candidate=%"PRIu64", Good=%"PRIu64"\tper second=%g.2\n\t\t%.6f%%\t%.6f%%\n",bad,candidate,good,(double)(CLOCKS_PER_SEC*total)/(clock()-start),(100.*candidate)/total,(100.*good)/total) ;
                }
                break ;
            default:
                ++bad ;
                ++distribution[f] ;
                break ;
        }
        if ( count % 1000000 == 0 ) {
			Distribution() ;
		}
    }
}

int Type2_fill_square( void ) {
    int i,j,k,count=0 ;
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
            ++count ;
            if (b == 0 ) {
                return count ;
            }
            row_bits[k] |= b ;
            col_bits[j] |= b ;
            bit[k][j] = b ;
        }
        for (i=k+1;i<SIZE;++i) {
            int b = find_valid_bit( col_bits[k]|row_bits[i] ) ;
            ++count ;
            if (b == 0 ) {
                return count ;
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
                return TOTALSIZE ;
            }
        }
    } 
        return -1 ;
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
                for ( ssj=0;ssj<SUBSIZE;++ssj) {
                    int i = SUBSIZE*si+ssi ;
                    int j = SUBSIZE*sj+ssj ;
                    int b = find_valid_bit( col_bits[j]|row_bits[i]|ss_bits[si][sj] ) ;
                    ++count ;
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
    return TOTALSIZE ;
}

int SS2_fill_square( void ) {
    int k,sk ;
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
    for (k=0;k<SUBSIZE;++k) {
        for (si=k;si<SUBSIZE;++si) {
            for (sk=0;sk<SUBSIZE;++sk) {
                for (ssi=sk;ssi<SUBSIZE;++ssi) {
                    int i = SUBSIZE*si+ssi ;
                    int j = SUBSIZE*k+sk ;
                    int b = find_valid_bit( col_bits[j]|row_bits[i]|ss_bits[si][k] ) ;
					++count ;
                    if (b == 0 ) {
                        return count ;
                    }
                    row_bits[i] |= b ;
                    col_bits[j] |= b ;
                    ss_bits[si][k] |= b ;
                    bit[i][j] = b ;
                }
                for ( ssj=sk+1;ssj<SUBSIZE;++ssj) {
                    int i = SUBSIZE*si+sk ;
                    int j = SUBSIZE*k+ssj ;
                    int b = find_valid_bit( col_bits[j]|row_bits[i]|ss_bits[si][k] ) ;
					++count ;
                    if (b == 0 ) {
                        return count ;
                    }
                    row_bits[i] |= b ;
                    col_bits[j] |= b ;
                    ss_bits[si][k] |= b ;
                    bit[i][j] = b ;
                }
            }
        }
        for (sj=k+1;sj<SUBSIZE;++sj) {
            for (sk=0 ; sk<SUBSIZE;++sk) {
                for (ssi=sk;ssi<SUBSIZE;++ssi) {
                    int i = SUBSIZE*k+ssi ;
                    int j = SUBSIZE*sj+sk ;
                    int b = find_valid_bit( col_bits[j]|row_bits[i]|ss_bits[k][sj] ) ;
					++count ;
                    if (b == 0 ) {
                        return count ;
                    }
                    row_bits[i] |= b ;
                    col_bits[j] |= b ;
                    ss_bits[k][sj] |= b ;
                    bit[i][j] = b ;
                }
                for ( ssj=sk+1;ssj<SUBSIZE;++ssj) {
                    int i = SUBSIZE*k+sk ;
                    int j = SUBSIZE*sj+ssj ;
                    int b = find_valid_bit( col_bits[j]|row_bits[i]|ss_bits[k][sj] ) ;
					++count ;
                    if (b == 0 ) {
                        return count ;
                    }
                    row_bits[i] |= b ;
                    col_bits[j] |= b ;
                    ss_bits[k][sj] |= b ;
                    bit[i][j] = b ;
                }
            }
        }
    }
    return TOTALSIZE ;
}

int SS3_fill_square( void ) {
    int sk,ssk,si,sj,ssi,count=0 ;
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
    // Top triangle
    for (sk=0;sk<SUBSIZE;++sk) {
		for (si=0;si<=sk;++si) {
			sj = sk-si ;
			// Top triangle
			for(ssk=0;ssk<SUBSIZE;++ssk) {
				for (ssi=0;ssi<=ssk;++ssi) {
                    int i = SUBSIZE*si+ssi ;
                    int j = SUBSIZE*sj+ssk-ssi ;
                    int b = find_valid_bit( col_bits[j]|row_bits[i]|ss_bits[si][sj] ) ;
                    ++count ;
                    if (b == 0 ) {
                        return count ;
                    }
                    row_bits[i] |= b ;
                    col_bits[j] |= b ;
                    ss_bits[si][sj] |= b ;
                    bit[i][j] = b ;
                }
            }
            // bottom triangle
			for(ssk=1;ssk<SUBSIZE;++ssk) {
				for (ssi=ssk;ssi<SUBSIZE;++ssi) {
                    int i = SUBSIZE*si+ssi ;
                    int j = SUBSIZE*sj+SUBSIZE-1+ssk-ssi ;
                    int b = find_valid_bit( col_bits[j]|row_bits[i]|ss_bits[si][sj] ) ;
                    ++count ;
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
    // bottom triangle
    for (sk=1;sk<SUBSIZE;++sk) {
		for (si=sk;si<SUBSIZE;++si) {
			sj = SUBSIZE-1+sk-si ; ;
			// Top triangle
			for(ssk=0;ssk<SUBSIZE;++ssk) {
				for (ssi=0;ssi<=ssk;++ssi) {
                    int i = SUBSIZE*si+ssi ;
                    int j = SUBSIZE*sj+ssk-ssi ;
                    int b = find_valid_bit( col_bits[j]|row_bits[i]|ss_bits[si][sj] ) ;
                    ++count ;
                    if (b == 0 ) {
                        return count ;
                    }
                    row_bits[i] |= b ;
                    col_bits[j] |= b ;
                    ss_bits[si][sj] |= b ;
                    bit[i][j] = b ;
                }
            }
            // bottom triangle
			for(ssk=1;ssk<SUBSIZE;++ssk) {
				for (ssi=ssk;ssi<SUBSIZE;++ssi) {
                    int i = SUBSIZE*si+ssi ;
                    int j = SUBSIZE*sj+SUBSIZE-1+ssk-ssi ;
                    int b = find_valid_bit( col_bits[j]|row_bits[i]|ss_bits[si][sj] ) ;
                    ++count ;
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
    return TOTALSIZE ;
}

// big diagonals as well
int X_fill_square( void ) {
    int si,sj,ssi,ssj,count=0 ;
    int col_bits[SIZE] ;
    int row_bits[SIZE] ;
    int ss_bits[SUBSIZE][SUBSIZE] ;
    int diag1=0 ;
    int diag2=0;
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
                    int m = col_bits[j]|row_bits[i]|ss_bits[si][sj] ;
                    int b ;
                    if ( i != j ) { // not main diagonal
                        if ( SIZE-i != j ) { // not 2nd diagonal
                            b = find_valid_bit( m ) ;
                            if (b == 0 ) {
                                return count ;
                            }
                        } else { // second diagonal
                            b = find_valid_bit( m|diag2 ) ;
                            if (b == 0 ) {
                                return count ;
                            }
                            diag2|=b ;
                        }
                    } else {
                        if ( SIZE-i != j ) { // not any diagonal
                            b = find_valid_bit( m|diag1 ) ;
                            if (b == 0 ) {
                                return count ;
                            }
                            diag1 |= b ;
                        } else { // Both diagonals (center)
                            b = find_valid_bit( m|diag2|diag1 ) ;
                            if (b == 0 ) {
                                return count ;
                            }
                            diag2|=b ;
                            diag1|=b ;
                        }
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

void SSLoop( int (*fill)(void) ) {
    uint64_t count ;
    uint64_t good = 0 ;
    uint64_t totalcount = 0 ;

    for ( count=0; count<=max_tries; ++count ) {
        int filled = fill() ;
        totalcount += filled ;
        ++distribution[filled] ;
        if ( filled == TOTALSIZE ) {
            ++good ;
            print_square() ;
            printf("count=%d, Good=%d\taverage=%g.1\tper second=%.1f\t%.6f%%\n",count,good,(double)totalcount/count,(double)(CLOCKS_PER_SEC*count)/(clock()-start),(100.*good)/count) ;
        } else if ( count % 100000 == 0 ) {
            printf("count=%d, Good=%d\taverage=%g.1\tper second=%.1f\t%.6f%%\n",count,good,(double)totalcount/count,(double)(CLOCKS_PER_SEC*count)/(clock()-start),(100.*good)/count) ;
        }
        if ( count % 1000000 == 0 ) {
			Distribution() ;
		}
    }
}

void help(char * prog) {
    printf(
    "sudoku_count by Paul H Alfille 2019 MIT License\n"
    "\tSearch the possible sudoku space for valid positions\n"
    "\tUses several search algorithms and shows statistics\n"
    "\tSee https:\\github.com/alfille/sudoku_count\n"
    "\tSee https:\\github.com/alfille/sudoku_count\n"
    "\n"
    "%s [options] [attempts]\n"
    "\n",
    "options:\n"
    "\t -t 1\tSearch columns first, then test for subsquares (default)\n"
    "\t -t 2\tSearch columns/row alternating, then test for subsquares\n"
    "\t -s 1\tSearch subsquares by column, show failure point\n"
    "\t -s 2\tSearch subsquares column/row alternating, show failure point\n"
    "\t -s 3\tSearch subsquares diagonal approach, show failure point\n"
    "\t -x  \tAlso main diagonals are unique (added constraint), show failure point\n"
    "\t -f filename\tPlace solutions in 'filename' (81 comma-separated values per line\n"
    "\t -d filename\tDistribution of tries (number of square aborted) every 1^6 tries\n"
    "\t -h  \tShow these instructions\n"
    "\n"
    "Attempts: number of boards to try (unlimited if omitted)\n",
    prog
    );
}

int main(int argc, char ** argv) {
    int c ; 
    
    void (*loop)( int (*fill)(void) ) = TypeLoop ; 
    int (*fill)(void) = Type1_fill_square ; 
        
    SEED ;
    make_pattern();
        
	Zero(distribution) ;
	
    start = clock() ;

    while ( (c = getopt( argc, argv, "hxt:s:f:d:" )) != -1 ) {
        switch(c) 
        {
            case 't':
                loop = TypeLoop ;
                switch (optarg[0]) {
                    case '2':
                        fill = Type2_fill_square ;
                        break ;
                    default:
                        fill = Type1_fill_square;
                        break ;
                }
                break ;
            case 's':
                loop = SSLoop ;
                switch (optarg[0]) {
                    case '3':
                        fill = SS3_fill_square ;
                        break ;
                    case '2':
                        fill = SS2_fill_square ;
                        break ;
                    default:
                        fill = SS1_fill_square ;
                        break ;
                }
                break ;
            case 'x':
                loop = SSLoop ;
                fill = X_fill_square ;
                break ;
            case 'f':
                // solution file
                fsolutions = fopen( optarg, "w" ) ;
                if ( fsolutions == NULL ) {
                    fprintf( stderr, "Trouble opening solutions file %s\n",optarg) ;
                    exit(1);
                }
                break ;
            case 'd':
                // distribution file
                fdistribution = fopen( optarg, "w" ) ;
                if ( fdistribution == NULL ) {
                    fprintf( stderr, "Trouble opening distribution file %s\n",optarg) ;
                    exit(1);
                }
                break ;
            case 'h':
            default:
                help(argv[0]) ;
                exit(1);
        } 
    }
    
    // optind is for the extra arguments 
    // which are not parsed 
    if (optind < argc) {
		uint64_t m = strtoull(argv[optind],NULL,0) ;
		if ( m > 0 && m < max_tries ) {
			max_tries = m ;
		}
    } 
    
    loop(fill) ;
    Distribution() ;
    return 0 ;
}
