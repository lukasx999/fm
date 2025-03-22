CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -pedantic -ggdb -fsanitize=address,undefined
LIBS=-lncurses
DEPS=fm.h

all: fm

fm: main.o fm.o
	$(CC) $(CFLAGS) $(LIBS) $^ -o $@

%.o: %.c Makefile $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm *.o fm

.PHONY: clean
