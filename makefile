CC=gcc
CFLAGS=-I.

game: dominoesd.o dominoes.o game.o
	$(CC) -o dominoesd dominoesd.o game.o -I.
	$(CC) -o dominoes dominoes.o -I.

clean:
	\rm *.o dominoesd dominoes
