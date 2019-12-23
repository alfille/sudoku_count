CC=gcc
CFLAGS=-I.
DEPS = sudoku_count.h
OBJ = sudoku_count.o xoshiro256starstar.o
OBJ16 = sudoku_count16.o xoshiro256starstar.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

sudoku_count: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

sudoku_count16: $(OBJ16)
	$(CC) -o $@ $^ $(CFLAGS)

all: sudoku_count sudoku_count16
