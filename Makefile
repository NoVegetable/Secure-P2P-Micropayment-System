CC=g++

CLIB=-std=c++17

CLIENT_CFLAGS=-I./src/client
SERVER_CFLAGS=-I./src/server

.PHONY: all clean

all: client server

client: src/client/*.cpp src/client/*.cc
	@$(CC) $(CLIB) $(CLIENT_CFALGS) -o $@ $^

server: src/server/*.cpp src/server/*.cc
	@$(CC) $(CLIB) $(SERVER_CFLAGS) -o $@ $^
	
clean:
	@-rm -rf client
	@-rm -rf server
