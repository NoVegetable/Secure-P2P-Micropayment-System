CC=g++

CLIB=-std=c++17

CFLAGS=-I./src

.PHONY: all clean

all: client

client: src/*.cpp src/*.cc
	@$(CC) $(CLIB) $(CFALGS) -o $@ $^

clean:
	@-rm -rf client