CC=g++

CLIB=-std=c++17

CFLAGS=-I./src/client

.PHONY: all clean

all: client

client: src/client/*.cpp src/client/*.cc
	@$(CC) $(CLIB) $(CFALGS) -o $@ $^
	
clean:
	@-rm -rf client
