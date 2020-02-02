CC=gcc
CFLAGS=-I.
DEPS = sudoku_count.h
OBJ = sudoku_count.o xoshiro256starstar.o
RAN = xoshiro256starstar.o
OBJLC = least_connected.o
OBJMC = most_connected.o

ifeq '$(findstring ;,$(PATH))' ';'
    UNAME := Windows
else
    UNAME := $(shell uname -s)
endif

ifeq ($(UNAME), Windows)
    lib := .dll
endif
ifeq ($(UNAME), Darwin)
    lib := .dylib
endif
ifeq ($(UNAME), Linux)
    lib := .so
endif

SQRT_36 := 6
SQRT_25 := 5
SQRT_16 := 4
SQRT_9  := 3
SQRT_4  := 2

.DEFAULT_GOAL := all

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

%$(lib): %.c $(DEPS)
	$(CC) -fPIC -shared -o $@ $< $(CFLAGS)

powers := 4 9 16 25 36
slibs := $(addsuffix $(lib), $(addprefix sudoku_lib, $(powers)))
slibs2 := $(addsuffix $(lib), $(addprefix sudoku2_lib, $(powers)))
progs := $(addprefix sudoku_count, $(powers))

$(slibs): sudoku_lib.c $(deps)
	$(CC) -fPIC -shared -DSUBSIZE=$(SQRT_$(subst sudoku_lib,,$(basename $@))) -o $@ $< $(CFLAGS)

$(slibs2): sudoku2_lib.c $(deps)
	$(CC) -fPIC -shared -DSUBSIZE=$(SQRT_$(subst sudoku2_lib,,$(basename $@))) -o $@ $< $(CFLAGS)

$(progs): sudoku_count.c $(RAN)
	$(CC) -DSUBSIZE=$(SQRT_$(subst sudoku_count,,$(basename $@))) -o $@ $^ $(CFLAGS)

least_connected: $(OBJLC)
	$(CC) -o $@ $^ $(CFLAGS)

most_connected: $(OBJMC)
	$(CC) -o $@ $^ $(CFLAGS)

all: least_connected most_connected $(progs) $(slibs) $(slibs2)
