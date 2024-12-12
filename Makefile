CC=g++

CSTD=-std=c++17

INCLUDE=-I./include

CC_OPT=-Wno-deprecated-declarations

LD_FLAGS=-lssl -lcrypto

STYLE_COLOR_GREEN=\033[0;32m
STYLE_RESET=\033[0m

.PHONY: all clean

all: client server
	@:

client: include/client.h include/crypto.h src/client/* src/crypto.cc
	@echo -n -e '${STYLE_COLOR_GREEN}Building client ...'
	@$(CC) $(CSTD) $(CC_OPT) $(INCLUDE) -o client src/client/* src/crypto.cc $(LD_FLAGS)
	@echo -e ' done${STYLE_RESET}'

server: include/server.h include/crypto.h src/server/* src/crypto.cc
	@echo -n -e '${STYLE_COLOR_GREEN}Building server ...'
	@$(CC) $(CSTD) $(CC_OPT) $(INCLUDE) -o server src/server/* src/crypto.cc $(LD_FLAGS)
	@echo -e ' done${STYLE_RESET}'
	
clean:
	@-rm -rf client
	@-rm -rf server
