CC=gcc
CFLAGS=-I.
DEPS = sudoku_count.h
OBJ = sudoku_count.o xoshiro256starstar.o
RAN = xoshiro256starstar.o
OBJLC = least_connected.o
OBJMC = most_connected.o

.DEFAULT_GOAL := all

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

%.so: %.c $(DEPS)
	$(CC) -fPIC -shared -o $@ $< $(CFLAGS)

sudoku_lib4.so: sudoku_lib.c $(DEPS)
	$(CC) -fPIC -DSUBSIZE=2 -shared -o $@ sudoku_lib.c $(CFLAGS)

sudoku_lib9.so: sudoku_lib.c $(DEPS)
	$(CC) -fPIC -DSUBSIZE=3 -shared -o $@ sudoku_lib.c $(CFLAGS)

sudoku_lib16.so: sudoku_lib.c $(DEPS)
	$(CC) -fPIC -DSUBSIZE=4 -shared -o $@ sudoku_lib.c $(CFLAGS)

sudoku_lib25.so: sudoku_lib.c $(DEPS)
	$(CC) -fPIC -DSUBSIZE=5 -shared -o $@ sudoku_lib.c $(CFLAGS)

sudoku_lib36.so: sudoku_lib.c $(DEPS)
	$(CC) -fPIC -DSUBSIZE=6 -shared -o $@ sudoku_lib.c $(CFLAGS)

sudoku_count: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

sudoku_count36: $(RAN) sudoku_count.c
	$(CC) -DSUBSIZE=6 -o $@ $^ $(CFLAGS)

sudoku_count25: $(RAN) sudoku_count.c
	$(CC) -DSUBSIZE=5 -o $@ $^ $(CFLAGS)

sudoku_count16: $(RAN) sudoku_count.c
	$(CC) -DSUBSIZE=4 -o $@ $^ $(CFLAGS)

sudoku_count9: $(RAN) sudoku_count.c
	$(CC) -DSUBSIZE=3 -o $@ $^ $(CFLAGS)

sudoku_count4: $(RAN) sudoku_count.c
	$(CC) -DSUBSIZE=2 -o $@ $^ $(CFLAGS)

least_connected: $(OBJLC)
	$(CC) -o $@ $^ $(CFLAGS)

most_connected: $(OBJMC)
	$(CC) -o $@ $^ $(CFLAGS)

all: sudoku_count sudoku_count36 sudoku_count25 sudoku_count16 sudoku_count9 sudoku_count4 \
  least_connected most_connected \
  sudoku_lib.so sudoku_lib36.so sudoku_lib25.so sudoku_lib16.so sudoku_lib9.so sudoku_lib4.so
