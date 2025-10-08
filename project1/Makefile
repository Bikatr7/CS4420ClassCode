CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS =

all: server client

server: server.c utils.c utils.h
	$(CC) $(CFLAGS) -o server server.c utils.c $(LDFLAGS)

client: client.c utils.c utils.h
	$(CC) $(CFLAGS) -o client client.c utils.c $(LDFLAGS)

clean:
	rm -f server client

.PHONY: all server client clean
