CC=g++

CSTD=-std=c++17

CLIENT_CFLAGS=-I./src/client
SERVER_CFLAGS=-I./src/server

CODEGEN_OPT=-O3

CLIENT_CFLAGS += $(CODEGEN_OPT)
SERVER_CFLAGS += $(CODEGEN_OPT)

.PHONY: all clean

all: client server

client: src/client/*.cpp src/client/*.cc
	@$(CC) $(CSTD) $(CLIENT_CFALGS) -o $@ $^

server: src/server/*.cpp src/server/*.cc
	@$(CC) $(CSTD) $(SERVER_CFLAGS) -o $@ $^
	
clean:
	@-rm -rf client
	@-rm -rf server
