CC=g++

CSTD=-std=c++17

CLIENT_CFLAGS=
SERVER_CFLAGS=

CODEGEN_OPT=-O3

CLIENT_CFLAGS += $(CODEGEN_OPT)
SERVER_CFLAGS += $(CODEGEN_OPT)

STYLE_COLOR_GREEN=\033[0;32m
STYLE_RESET=\033[0m

.PHONY: all clean

all: client server
	@:

client: src/client/*.cpp src/client/*.cc
	@echo -n -e '${STYLE_COLOR_GREEN}Building client ...'
	@$(CC) $(CSTD) $(CLIENT_CFALGS) -o $@ $^
	@echo -e ' done${STYLE_RESET}'

server: src/server/*.cpp src/server/*.cc
	@echo -n -e '${STYLE_COLOR_GREEN}Building server ...'
	@$(CC) $(CSTD) $(SERVER_CFLAGS) -o $@ $^
	@echo -e ' done${STYLE_RESET}'
	
clean:
	@-rm -rf client
	@-rm -rf server
