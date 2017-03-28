#makefile - Robert Arendac

CC = gcc
CFLAGS = -Wall -W -pedantic

all: server

server: server.o
	$(CC) $(CFLAGS) server.o -o server

clean:
	rm -f *.o server
