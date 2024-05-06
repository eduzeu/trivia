
CC = gcc


all: server client

server: server.o
	$(CC) -o server server.o

client: client.o
	$(CC) -o client client.o

server.o: server.c 
	$(CC) -c server.c

client.o: client.c
	$(CC) -c client.c 

update-ts:
	touch *.c *.h Makefile 

clean:
	rm -f *.o server client 

.PHONY: all clean update-ts


