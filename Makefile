CC=g++

CSTD=-std=c++17

INCLUDE=-I./include

CC_OPT=-Wno-deprecated-declarations -O3

LD_FLAGS=-lssl -lcrypto

CFLAGS= $(CC_OPT) $(LD_FLAGS)

STYLE_COLOR_GREEN=\033[0;32m
STYLE_RESET=\033[0m

.PHONY: all clean

all: client server
	@:

client: include/client.h src/client/*.cpp src/client/*.cc
	@echo -n -e '${STYLE_COLOR_GREEN}Building client ...'
	@$(CC) $(CSTD) $(INCLUDE) -o client src/client/main.cpp src/client/client.cc $(CFALGS)
	@echo -e ' done${STYLE_RESET}'

server: include/server.h src/server/*.cpp src/server/*.cc
	@echo -n -e '${STYLE_COLOR_GREEN}Building server ...'
	@$(CC) $(CSTD) $(INCLUDE) -o server src/server/main.cpp src/server/server.cc $(CFLAGS)
	@echo -e ' done${STYLE_RESET}'
	
clean:
	@-rm -rf client
	@-rm -rf server
