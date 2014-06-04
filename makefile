CC=gcc
CFLAGS=-I.

game: server.o client.o game.o
	$(CC) -o server server.o game.o -I.
	$(CC) -o client client.o -I.

clean:
	\rm *.o server client
