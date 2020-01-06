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

# define MAXCONSTRAINT (3*SIZE)

struct SearchOrder { 
    int i; 
    int j; 
    } ;


struct SearchOrder order[ TOTALSIZE ] ; 

char * key = "?" ;

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
    return trial ;
}


// For backtracking state
#define MAXTRACK 80
struct FillState {
    int row_bits[SIZE] ;
    int col_bits[SIZE] ;
    int ss_bits[SUBSIZE][SUBSIZE] ;
    int Xup ;
    int Xdown ;
    int Window[SUBSIZE-1][SUBSIZE-1] ;
    int fill ;
    int mask ;
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

struct FillState * StateStackInit ( void ) {
    statestack.start = statestack.end = 0 ;
    memset( & State[0], 0, sizeof( struct FillState ) ) ;
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
        fprintf(fdistribution,"%s%s",key,statestack.back) ;
        for ( d=0;d<=TOTALSIZE;++d) {
            fprintf(fdistribution,",%"PRIu64,distribution[d]) ; 
        }
        fprintf(fdistribution,"\n");
    }
}

int Type_fill_square( void ) {
    int i, j, fill;
    int col_bits[SIZE] ;
    int row_bits[SIZE] ;
    
    Zero(bit) ;
    
    // column bits culmulative
    Zero( col_bits ) ;
    
    // row bits culmulative
    Zero( row_bits ) ;
    
    // Fill columns and rows
    for ( fill = 0 ; fill < TOTALSIZE ; ++ fill ) {
        int b ;
        i = order[fill].i ;
        j = order[fill].j ;
        b = find_valid_bit( col_bits[j]|row_bits[i] ) ;
        if (b == 0 ) {
            return fill+1 ;
        }
        row_bits[i] |= b ;
        col_bits[j] |= b ;
        bit[i][j] = b ;
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
}   }

void TypeLoopSummary( uint64_t count, uint64_t candidate, uint64_t good ) {
    if ( fsummary ) {
        fprintf(fsummary,"Type,Bad,Candidate,Good,perSec,Cand%%,Good%%\n") ;
        fprintf(fsummary,"%s%s,%"PRIu64",%"PRIu64",%"PRIu64",%g.2,%.6f%%,%.6f%%\n",key,statestack.back,count-good-candidate,candidate,good,(double)(CLOCKS_PER_SEC*count)/(clock()-start),(100.*candidate)/count,(100.*good)/count) ;
}   }

int constraints[SIZE][SIZE] ;

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
    constraints[i][j] += MAXCONSTRAINT;
}

void find_next( int fill ) {
    int low = MAXCONSTRAINT+1 ;
    int i, j ;
    int besti, bestj ;
        
    for( i=0 ; i<SIZE ; ++i ) {
        for (j=0 ; j<SIZE ; ++j ) {
            if ( constraints[i][j] < low ) {
                besti = i;
                bestj = j;
                low = constraints[i][j] ;
            }
        }
    }
    
    order[fill].i = besti ;
    order[fill].j = bestj ;
    AddConstraint(besti,bestj);
}

// 1 is bad, 0 is good
int verify_order( void ) {
    int fill ;
    int check[SIZE][SIZE] ;
    
    Zero(check) ;
    
    for ( fill=0 ; fill<TOTALSIZE ; ++ fill ) {
        int i = order[fill].i ;
        int j = order[fill].j ;
        if ( i < 0 || i >= SIZE ) {
            fprintf(stderr,"Order slot %d (%d,%d) is a has a bad i value\n",fill,i,j) ;
            return 1 ;
        }
        if ( j < 0 || j >= SIZE ) {
            fprintf(stderr,"Order slot %d (%d,%d) is a has a bad j value\n",fill,i,j) ;
            return 1 ;
        }
        if ( check[i][j] ) {
            fprintf(stderr,"Order slot %d (%d,%d) has a repeat value first seen in slot %d\n",fill,i,j,check[i][j]-1) ;
            return 1 ;
        }
        check[i][j] = fill+1 ;
    }
    return 0 ;
}

void print_order( void ) {
    int fill ;
    int i,j ;
    int reorder[SIZE][SIZE] ;
            
    Zero( reorder ) ;
    
    for ( fill=0 ; fill<TOTALSIZE ; ++fill ) {
        reorder[order[fill].i][order[fill].j] = fill ;
    }
    
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
            fprintf(stderr,"%c%4d ",c,reorder[i][j]);
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

void WS4_order( void ) {
    int fill ;

    Zero( constraints ) ;
    
    for ( fill=0 ; fill<TOTALSIZE ; ++fill ) {
        find_next( fill ) ;
    }
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
    TypeLoopPrint( count, candidate, good ) ;
}

int SS_fill_square( void ) {
    
    struct FillState * pFS = StateStackInit() ;
    
    Zero(bit) ;
    
    for ( pFS->fill=0 ; pFS->fill<TOTALSIZE ; ) {
        int i = order[pFS->fill].i ;
        int j = order[pFS->fill].j ;
        int si = i / SUBSIZE ;
        int sj = j / SUBSIZE ;
        int m = pFS->col_bits[j]|pFS->row_bits[i]|pFS->ss_bits[si][sj]|pFS->mask ;
        int b = find_valid_bit( m ) ;
        if (b == 0 ) {
            int fill = pFS->fill ;
            //check if alternative exists
            if ( (pFS = StateStackPop()) ) {
                // try with old position and data (but the prior choice is added to the exclusions)
                continue ;
            }
            return fill+1 ;
        }

        // See if a backup spot
        if ( (pFS->fill > SIZE) && ((b|m) != full_pattern) ) {
            pFS->mask |= b ;
            pFS = StateStackPush() ;
        }
        
        pFS->row_bits[i] |= b ;
        pFS->col_bits[j] |= b ;
        pFS->ss_bits[si][sj] |= b ;

        bit[i][j] = b ;

        pFS->mask = 0 ;
        ++pFS->fill ;
    }
    return TOTALSIZE ;
}
int W_fill_square( void ) {
    
    struct FillState * pFS = StateStackInit() ;
    
    Zero(bit) ;
    
    for ( pFS->fill=0 ; pFS->fill<TOTALSIZE ; ) {
        int i = order[pFS->fill].i ;
        int j = order[pFS->fill].j ;
        int si = i / SUBSIZE ;
        int sj = j / SUBSIZE ;
        int iswin = ( i % (SUBSIZE+1) ) && ( j % (SUBSIZE+1) ) ;
        int m = pFS->col_bits[j]|pFS->row_bits[i]|pFS->ss_bits[si][sj]|pFS->mask ;
        int b ;
        if ( iswin ) {
            m |= pFS->Window[i/(SUBSIZE+1)][j/(SUBSIZE+1)] ;
        }
        b = find_valid_bit( m ) ;
        if (b == 0 ) {
            int fill = pFS->fill ;
            //check if alternative exists
            if ( (pFS = StateStackPop()) ) {
                // try with old position and data (but the prior choice is added to the exclusions)
                continue ;
            }
            return fill+1 ;
        }

        // See if a backup spot
        if ( (pFS->fill > SIZE) && ((b|m) != full_pattern) ) {
            pFS->mask |= b ;
            pFS = StateStackPush() ;
        }
        
        pFS->row_bits[i] |= b ;
        pFS->col_bits[j] |= b ;
        pFS->ss_bits[si][sj] |= b ;
        if ( iswin ) {
            pFS->Window[i/(SUBSIZE+1)][j/(SUBSIZE+1)] |= b ;
        }

        bit[i][j] = b ;

        pFS->mask = 0 ;
        ++pFS->fill ;
    }
    return TOTALSIZE ;
}

void SS1_order( void ) {
    int si,sj,ssi,ssj,filled=-1 ;

    // Fill columns and rows
    for (si=0;si<SUBSIZE;++si) {
        for (sj=0;sj<SUBSIZE;++sj) {
            for (ssi=0;ssi<SUBSIZE;++ssi) {
                for ( ssj=0;ssj<SUBSIZE;++ssj) {
                    int i = SUBSIZE*si+ssi ;
                    int j = SUBSIZE*sj+ssj ;
                    ++ filled ;
                    order[filled].i = i ;
                    order[filled].j = j ;
                }
            }
        }
    }
}

void SS2_order( void ) {
    int k,sk ;
    int si,sj,ssi,ssj,fill=-1 ;

    // Fill columns and rows
    for (k=0;k<SUBSIZE;++k) {
        for (si=k;si<SUBSIZE;++si) {
            for (sk=0;sk<SUBSIZE;++sk) {
                for (ssi=sk;ssi<SUBSIZE;++ssi) {
                    int i = SUBSIZE*si+ssi ;
                    int j = SUBSIZE*k+sk ;
                    ++fill ;
                    order[fill].i = i ;
                    order[fill].j = j ;
                }
                for ( ssj=sk+1;ssj<SUBSIZE;++ssj) {
                    int i = SUBSIZE*si+sk ;
                    int j = SUBSIZE*k+ssj ;
                    ++fill ;
                    order[fill].i = i ;
                    order[fill].j = j ;
                }
            }
        }
        for (sj=k+1;sj<SUBSIZE;++sj) {
            for (sk=0 ; sk<SUBSIZE;++sk) {
                for (ssi=sk;ssi<SUBSIZE;++ssi) {
                    int i = SUBSIZE*k+ssi ;
                    int j = SUBSIZE*sj+sk ;
                    ++fill ;
                    order[fill].i = i ;
                    order[fill].j = j ;
                }
                for ( ssj=sk+1;ssj<SUBSIZE;++ssj) {
                    int i = SUBSIZE*k+sk ;
                    int j = SUBSIZE*sj+ssj ;
                    ++fill ;
                    order[fill].i = i ;
                    order[fill].j = j ;
                }
            }
        }
    }
}

void SS3_order( void ) {
    int sk,ssk,si,sj,ssi,fill=-1 ;

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
                    ++fill ;
                    order[fill].i = i ;
                    order[fill].j = j ;
                }
            }
            // bottom triangle
            for(ssk=1;ssk<SUBSIZE;++ssk) {
                for (ssi=ssk;ssi<SUBSIZE;++ssi) {
                    int i = SUBSIZE*si+ssi ;
                    int j = SUBSIZE*sj+SUBSIZE-1+ssk-ssi ;
                    ++fill ;
                    order[fill].i = i ;
                    order[fill].j = j ;
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
                    ++fill ;
                    order[fill].i = i ;
                    order[fill].j = j ;
                }
            }
            // bottom triangle
            for(ssk=1;ssk<SUBSIZE;++ssk) {
                for (ssi=ssk;ssi<SUBSIZE;++ssi) {
                    int i = SUBSIZE*si+ssi ;
                    int j = SUBSIZE*sj+SUBSIZE-1+ssk-ssi ;
                    ++fill ;
                    order[fill].i = i ;
                    order[fill].j = j ;
                }
            }
        }
    }
}

void WS1_order( void ) {
    int i,j,fill=-1 ;
    
    // Fill columns and rows
    for (i=0;i<SIZE;++i) {
        for (j=0;j<SIZE;++j) {
            ++fill ;
            order[fill].i = i ;
            order[fill].j = j ;
        }
    }
}

void WS2_order( void ) {
    int k ;
    int i,j,fill=-1 ;
    
    // Fill columns and rows
    for (k=0;k<SIZE;++k) {
        for (i=k;i<SIZE;++i) {
            int si = i / SUBSIZE ;
            int sj = k / SUBSIZE ;
            ++fill ;
            order[fill].i = i ;
            order[fill].j = k ;
        }
        for (j=k+1;j<SIZE;++j) {
            int si = k / SUBSIZE ;
            int sj = j / SUBSIZE ;
            ++fill ;
            order[fill].i = k ;
            order[fill].j = j ;
        }
    }
}

void WS3_order( void ) {
    int i,k,fill=-1 ;

    // Fill columns and rows
    // Top triangle
    for (k=0;k<SIZE;++k) {
        for (i=0;i<=k;++i) {
            int j = k-i ;
            ++fill ;
            order[fill].i = i ;
            order[fill].j = j ;
        }
    }
    // bottom triangle
    for (k=1;k<SIZE;++k) {
        for (i=k;i<SIZE;++i) {
            int j = SIZE-1+k-i ;
            ++fill ;
            order[fill].i = i ;
            order[fill].j = j ;
        }
    }
}

// big diagonals as well
int X_fill_square( void ) {
    struct FillState * pFS = StateStackInit() ;
    
    Zero(bit) ;
    
    for ( pFS->fill=0 ; pFS->fill<TOTALSIZE ; ) {
        int i = order[pFS->fill].i ;
        int j = order[pFS->fill].j ;
        int si = i / SUBSIZE ;
        int sj = j / SUBSIZE ;
        int m = pFS->col_bits[j]|pFS->row_bits[i]|pFS->ss_bits[si][sj]|pFS->mask ;
        int Xu, Xd ;
        int b ;
        if ( i != j ) { // not main diagonal
            if ( SIZE-1-i != j ) { // not any diagonal
                b = find_valid_bit( m ) ;
                Xu = Xd = 0 ;
            } else { // second diagonal
                b = find_valid_bit( m|pFS->Xup ) ;
                Xu = b ; 
                Xd = 0;
            }
        } else {
            if ( SIZE-1-i != j ) { // first diagonal
                b = find_valid_bit( m|pFS->Xdown ) ;
                Xu = 0 ;
                Xd = b ;
            } else { // Both diagonals (center)
                b = find_valid_bit( m|pFS->Xup|pFS->Xdown ) ;
                Xu = Xd = b ;
            }
        }
        if (b == 0 ) {
            int fill = pFS->fill ;
            //check if alternative exists
            if ( (pFS = StateStackPop()) ) {
                // try with old position and data (but the prior choice is added to the exclusions)
                continue ;
            }
            return fill+1 ;
        }

        // See if a backup spot
        if ( (pFS->fill > SIZE) && ((b|m) != full_pattern) ) {
            pFS->mask |= b ;
            pFS = StateStackPush() ;
        }
        
        pFS->Xup |= Xu ;
        pFS->Xdown |= Xd ;
        pFS->row_bits[i] |= b ;
        pFS->col_bits[j] |= b ;
        pFS->ss_bits[si][sj] |= b ;

        bit[i][j] = b ;

        pFS->mask = 0 ;
        ++pFS->fill ;
    }
    return TOTALSIZE ;
}

void SSLoopPrint( uint64_t count, uint64_t good, uint64_t totalcount ) {
    if ( !quiet ) {
        printf("count=%"PRIu64", Good=%"PRIu64"\taverage=%g.1\tper second=%.1f\t%.6f%%\n",count,good,(double)totalcount/count,(double)(CLOCKS_PER_SEC*count)/(clock()-start),(100.*good)/count) ;
}   }

void SSLoopSummary( uint64_t count, uint64_t good, uint64_t totalcount ) {
    if ( fsummary ) {
        fprintf(fsummary,"Type,Count,Good,Average,perSec,Success%%\n") ;
        fprintf(fsummary,"%s%s,%"PRIu64",%"PRIu64",%g.1,%.1f,%.6f%%\n",key,statestack.back,count,good,(double)totalcount/count,(double)(CLOCKS_PER_SEC*count)/(clock()-start),(100.*good)/count) ;
}   }

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
    "\n"
    "options:\n"
    "\t -t [1|2|3|4]\tSearch columns first, then test for subsquares (default)\n"
    "\t -s [1|2|3]\tSearch pattern subsquares\n"
    "\t -w [1|2|3|4]\tSearch pattern whole square\n"
    "\t -X [1|2|3|4]\tMain diagonals unique (Added constraint)\n"
    "\t -W [1|2|3|4]\tWindows unique (Added constraint)\n"
    "\t\t 1 -- by column\n"
    "\t\t 2 -- alternating column/row\n"
    "\t\t 3 -- diagonal\n"
    "\t\t 4 -- scattered -- least constrained\n"
    "\t-b n -- backtrack 'n' levels on dead end (only for s and w) -- default 0\n"
    "\t -f filename\tPlace solutions in 'filename' (81 comma-separated values per line\n"
    "\t -d filename\tDistribution of tries (number of square aborted) every 1^6 tries\n"
    "\t -g filename\tPlace summary data in filename\n"
    "\t -q quiet\tSuppress most printing\n"
    "\t -o Show ordering\n"
    "\t -h  \tShow these instructions\n"
    "\n"
    "Attempts: number of boards to try (unlimited if omitted)\n",
    prog
    );
}

int main(int argc, char ** argv) {
    int c ; 
    int ordering = 0 ;
    
    void (*loop)( int (*fill)(void) ) = TypeLoop ; //default
    int (*fill)(void) = Type_fill_square ; //default
    char * type = "Unspecified -- assume t1";
        
    SEED ;
    make_pattern();
        
    Zero(distribution) ;
    
    signal( SIGINT, RuptHandler ) ;
    
    WS1_order() ; //default
    
    while ( (c = getopt( argc, argv, "hoqW:X:t:w:s:f:d:g:b:" )) != -1 ) {
        switch(c) 
        {
            case 't':
                loop = TypeLoop ;
                fill = Type_fill_square ;
                switch (optarg[0]) {
                    case '4':
                        key = "t4" ;
                        type = "Subsquare later -- least constraints" ;
                        WS4_order() ;
                        break ;
                    case '3':
                        key = "t3" ;
                        type = "Subsquare later -- diagonals" ;
                        WS3_order() ;
                        break ;
                    case '2':
                        key = "t2" ;
                        type = "Subsquare later -- alternating" ;
                        WS2_order() ;
                        break ;
                    default:
                        key = "t1" ;
                        type = "Subsquare later -- columns" ;
                        WS1_order() ;
                        break ;
                }
                break ;
            case 's':
                loop = SSLoop ;
                fill = SS_fill_square ;
                switch (optarg[0]) {
                    case '3':
                        key = "s3" ;
                        type = "Diagonal Subsquare" ;
                        SS3_order() ;
                        break ;
                    case '2':
                        key = "s2" ;
                        type = "Alternating Subsquare" ;
                        SS2_order() ;
                        break ;
                    default:
                        key = "s1" ;
                        type = "Columns Subsquare" ;
                        SS1_order() ;
                        break ;
                }
                break ;
            case 'w':
                loop = SSLoop ;
                fill = SS_fill_square ;
                switch (optarg[0]) {
                    case '4':
                        key = "w4" ;
                        type = "Least constrained Whole square" ;
                        WS4_order() ;
                        break ;
                    case '3':
                        key = "w3" ;
                        type = "Diagonal Whole square" ;
                        WS3_order() ;
                        break ;
                    case '2':
                        key = "w2" ;
                        type = "Alternating Whole square" ;
                        WS2_order() ;
                        break ;
                    default:
                        key = "w1" ;
                        type = "Columns Whole square" ;
                        WS1_order() ;
                        break ;
                }
                break ;
            case 'X':
                loop = SSLoop ;
                type = "X Columns X-square" ;
                fill = X_fill_square ;
                switch (optarg[0]) {
                    case '4':
                        key = "X4" ;
                        type = "Least constrained with X" ;
                        WS4_order() ;
                        break ;
                    case '3':
                        key = "X3" ;
                        type = "Diagonal with X" ;
                        WS3_order() ;
                        break ;
                    case '2':
                        key = "X2" ;
                        type = "Alternating with X" ;
                        WS2_order() ;
                        break ;
                    default:
                        key = "X1" ;
                        type = "Columns with X" ;
                        WS1_order() ;
                        break ;
                }
                break ;
            case 'W':
                loop = SSLoop ;
                type = "W Columns Windows" ;
                fill = W_fill_square ;
                switch (optarg[0]) {
                    case '4':
                        key = "W4" ;
                        type = "Least constrained with Windows" ;
                        WS4_order() ;
                        break ;
                    case '3':
                        key = "W3" ;
                        type = "Diagonal with Windows" ;
                        WS3_order() ;
                        break ;
                    case '2':
                        key = "W2" ;
                        type = "Alternating with Windows" ;
                        WS2_order() ;
                        break ;
                    default:
                        key = "W1" ;
                        type = "Columns with Windows" ;
                        WS1_order() ;
                        break ;
                }
                break ;
            case 'b':
                StateStackCreate( atol( optarg ) ) ;
                break ;
            case 'f':
                // solution file
                fsolutions = fopen( optarg, "w" ) ;
                if ( fsolutions == NULL ) {
                    fprintf( stderr, "Trouble opening solutions file %s\n",optarg) ;
                    perror(NULL);
                    exit(1);
                }
                break ;
            case 'd':
                // distribution file
                fdistribution = fopen( optarg, "w" ) ;
                if ( fdistribution == NULL ) {
                    fprintf( stderr, "Trouble opening distribution file %s\n",optarg) ;
                    perror(NULL);
                    exit(1);
                }
                break ;
            case 'g':
                // summary file
                fsummary = fopen( optarg, "w" ) ;
                if ( fsummary == NULL ) {
                    fprintf( stderr, "Trouble opening distribution file %s\n",optarg) ;
                    perror(NULL);
                    exit(1);
                }
                break ;
            case 'o':
                ordering = 1 ;
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
    
    if ( verify_order() ) {
        fprintf(stderr,"Order verifying for %s%s - %s fails -- abort\n",key,statestack.back,type) ;
        exit(1) ;
    }
    
    if ( statestack.size > 1 && fill == Type_fill_square ) {
        fprintf(stderr,"Backup (-b) option not implemented for %s - %s\n",key,type);
        fprintf(stderr,"Ignoring Backup\n");
        StateStackCreate( 0 ) ;
    }
                
    if ( ordering ) {
        printf( "%s%s - %s\n", key,statestack.back,type ) ;
        print_order() ;
    }
    
    start = clock() ;
    
    loop(fill) ;
    Distribution() ;
    return 0 ;
}
