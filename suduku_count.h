// Suduku_count
// Attempt to count the number of legal suduku positions
// Based on Discussion between Kendrick Shaw MD PhD and Paul Alfille MD
// MIT license 2019
// by Paul H Alfille

// Include random numbers
// Use xorshiro256** seeded from kernal entropy pool

#include <stdint.h>

void seed_xoshiro(void) ;
uint64_t next_xoshiro(void) ;

// Make the random number generator generic
#define SEED seed_xoshiro()
#define RANDOM next_xoshiro()

