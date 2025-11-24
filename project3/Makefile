CC = gcc
CFLAGS = -Wall -Wextra -std=c99

# Auto-detect OS and set OpenSSL paths
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # macOS with Homebrew OpenSSL
    OPENSSL_PREFIX = /opt/homebrew/opt/openssl@3
    CFLAGS += -I$(OPENSSL_PREFIX)/include
    LDFLAGS = -L$(OPENSSL_PREFIX)/lib -lssl -lcrypto
else
    # Linux - OpenSSL in standard locations ? maybe
    LDFLAGS = -lssl -lcrypto
endif

all: server client

server: server.c
	$(CC) $(CFLAGS) -o server server.c $(LDFLAGS)

client: client.c
	$(CC) $(CFLAGS) -o client client.c $(LDFLAGS)

clean:
	rm -f server client *.o

test: all
	@echo "Running comprehensive tests..."
	@./test_all.sh

.PHONY: all server client clean test
