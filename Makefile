CC=gcc
CFLAGS=-I.
DEPS = suduku_count.h
OBJ = suduku_count.o xoshiro256starstar.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

sudoku_count: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
