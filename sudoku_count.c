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
#include <signal.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

// SIZE x SIZE sudoku board
#define SIZE (9)
# define SUBSIZE (3) // sqrt of SIZE
# define TOTALSIZE (SIZE*SIZE)

#define Zero(array) memset( array, 0, sizeof(array) ) ;

clock_t start ;

volatile int rupt = 0 ;
int quiet = 0 ;

FILE * fsolutions = NULL ;
FILE * fdistribution = NULL ;
FILE * fsummary = NULL ;

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
	if ( !quiet ) {
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

void TypeLoopPrint( uint64_t count, uint64_t candidate, uint64_t good ) {
	if ( !quiet ) {
		printf("Bad=%"PRIu64", Candidate=%"PRIu64", Good=%"PRIu64"\tper second=%g.2\t%.6f%%\t%.6f%%\n",count-good-candidate,candidate,good,(double)(CLOCKS_PER_SEC*count)/(clock()-start),(100.*candidate)/count,(100.*good)/count) ;
}	}

void TypeLoopSummary( uint64_t count, uint64_t candidate, uint64_t good ) {
	if ( fsummary ) {
		fprintf(fsummary,"Bad\tCandidate\tGood\tperSec\tCand%%\tGood%%\n",count-good-candidate,candidate,good,(double)(CLOCKS_PER_SEC*count)/(clock()-start),(100.*candidate)/count,(100.*good)/count) ;
		fprintf(fsummary,"%"PRIu64"\r%"PRIu64"\t%"PRIu64"\t%g.2\t%.6f%%\t%.6f%%\n",count-good-candidate,candidate,good,(double)(CLOCKS_PER_SEC*count)/(clock()-start),(100.*candidate)/count,(100.*good)/count) ;
}	}

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
                TypeLoopPrint( count, candidate,good ) ;
                break ;
            case TOTALSIZE:
                ++candidate ;
                ++distribution[TOTALSIZE] ;
                if ( candidate % 10000 == 0 ) {
					TypeLoopPrint( count, candidate,good ) ;
                }
                break ;
            default:
                ++bad ;
                ++distribution[f] ;
                break ;
        }
        if ( rupt ) {
			break ;
		}
    }
    TypeLoopSummary( count, candidate, good ) ;
	TypeLoopPrint( count, candidate,good ) ;
	Distribution() ;
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
			sj = SUBSIZE-1+sk-si ;
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

int WS1_fill_square( void ) {
    int i,j,count=0 ;
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
    for (i=0;i<SIZE;++i) {
        for (j=0;j<SIZE;++j) {
			int si = i / SUBSIZE ;
			int sj = j / SUBSIZE ;
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
    return TOTALSIZE ;
}

int WS2_fill_square( void ) {
    int k ;
    int i,j,count=0 ;
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
    for (k=0;k<SIZE;++k) {
        for (i=k;i<SIZE;++i) {
			int si = i / SUBSIZE ;
			int sj = k / SUBSIZE ;
			int b = find_valid_bit( col_bits[k]|row_bits[i]|ss_bits[si][sj] ) ;
			++count ;
			if (b == 0 ) {
				return count ;
			}
			row_bits[i] |= b ;
			col_bits[k] |= b ;
			ss_bits[si][sj] |= b ;
			bit[i][j] = b ;
        }
        for (j=k+1;j<SIZE;++j) {
			int si = k / SUBSIZE ;
			int sj = j / SUBSIZE ;
			int b = find_valid_bit( col_bits[j]|row_bits[k]|ss_bits[i][sj] ) ;
			++count ;
			if (b == 0 ) {
				return count ;
			}
			row_bits[k] |= b ;
			col_bits[j] |= b ;
			ss_bits[si][sj] |= b ;
			bit[i][j] = b ;
		}
    }
    return TOTALSIZE ;
}

int WS3_fill_square( void ) {
    int i,k,count=0 ;
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
    for (k=0;k<SIZE;++k) {
		for (i=0;i<=k;++i) {
			int j = k-i ;
			int si = i / SUBSIZE ;
			int sj = j / SUBSIZE ;
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
    for (k=1;k<SIZE;++k) {
		for (i=k;i<SIZE;++i) {
			int j = SIZE-1+k-i ;
			int si = i / SUBSIZE ;
			int sj = j / SUBSIZE ;
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

void SSLoopPrint( uint64_t count, uint64_t good, uint64_t totalcount ) {
	if ( !quiet ) {
		printf("count=%"PRIu64", Good=%"PRIu64"\taverage=%g.1\tper second=%.1f\t%.6f%%\n",count,good,(double)totalcount/count,(double)(CLOCKS_PER_SEC*count)/(clock()-start),(100.*good)/count) ;
}	}

void SSLoopSummary( uint64_t count, uint64_t good, uint64_t totalcount ) {
	if ( fsummary ) {
		fprintf(fsummary,"Count\tGood\tAverage\tperSec\tSuccess%%n") ;
		fprintf(fsummary,"%"PRIu64"\t%"PRIu64"\t%g.1\t%.1f\t%.6f%%\n",count,good,(double)totalcount/count,(double)(CLOCKS_PER_SEC*count)/(clock()-start),(100.*good)/count) ;
}	}

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
            SSLoopPrint( count, good, totalcount ) ;
        } else if ( count % 100000 == 0 ) {
            SSLoopPrint( count, good, totalcount ) ;
        }
        if ( rupt ) {
			break ;
		}
    }
    SSLoopSummary(count,good,totalcount);
    SSLoopPrint( count, good, totalcount ) ;
    Distribution() ;
}

void RuptHandler( int sig ) {
	signal( sig, SIG_IGN ) ;
	rupt = 1 ;
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
    "\t -w 1\tSearch whole squares by column, show failure point\n"
    "\t -w 2\tSearch whole squares column/row alternating, show failure point\n"
    "\t -w 3\tSearch whole squares diagonal approach, show failure point\n"
    "\t -x  \tAlso main diagonals are unique (added constraint), show failure point\n"
    "\t -f filename\tPlace solutions in 'filename' (81 comma-separated values per line\n"
    "\t -d filename\tDistribution of tries (number of square aborted) every 1^6 tries\n"
    "\t -g filename\tPlace summary data in filename\n"
    "\t -q quiet\tSuppress most printing\n"
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
    char * type = "";
        
    SEED ;
    make_pattern();
        
	Zero(distribution) ;
	
	signal( SIGINT, RuptHandler ) ;
	
    start = clock() ;

    while ( (c = getopt( argc, argv, "hqxt:w:s:f:d:" )) != -1 ) {
        switch(c) 
        {
            case 't':
                loop = TypeLoop ;
                switch (optarg[0]) {
                    case '2':
                        fill = Type2_fill_square ;
                        type = "t1 Subsquare later -- alternating" ;
                        break ;
                    default:
                        fill = Type1_fill_square;
                        type = "t2 Subsquare later -- columns" ;
                        break ;
                }
                break ;
            case 's':
                loop = SSLoop ;
                switch (optarg[0]) {
                    case '3':
						type = "s3 Diagonal Subsquare" ;
                        fill = SS3_fill_square ;
                        break ;
                    case '2':
						type = "s2 Alternating Subsquare" ;
                        fill = SS2_fill_square ;
                        break ;
                    default:
						type = "s1 Columns Subsquare" ;
                        fill = SS1_fill_square ;
                        break ;
                }
                break ;
            case 'w':
                loop = SSLoop ;
                switch (optarg[0]) {
                    case '3':
						type = "w3 Diagonal Whole square" ;
                        fill = WS3_fill_square ;
                        break ;
                    case '2':
						type = "w2 Alternating Whole square" ;
                        fill = WS2_fill_square ;
                        break ;
                    default:
						type = "w1 Columns Whole square" ;
                        fill = WS1_fill_square ;
                        break ;
                }
                break ;
            case 'x':
                loop = SSLoop ;
				type = "x Columns X-square" ;
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
            case 'g':
                // summary file
                fsummary = fopen( optarg, "w" ) ;
                if ( fsummary == NULL ) {
                    fprintf( stderr, "Trouble opening distribution file %s\n",optarg) ;
                    exit(1);
                }
                break ;
			case 'q':
				quiet = 1 ;
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
    
    if ( fsummary ) {
		fprintf( fsummary, "%s\n", type ) ;
	}
    
    loop(fill) ;
    return 0 ;
}
