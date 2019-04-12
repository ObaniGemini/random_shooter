CC=gcc -std=c99
HEAD=header.h
FLAGS=-lSDL2 -Wall -Wextra -g
CFLAGS=-lSDL2_ttf -lSDL2_mixer -lm



client	:	client.o client.h menu.o menu.h $(HEAD)
		$(CC) $(FLAGS) $(CFLAGS) $^ -o $@

client.o:	client.c
		$(CC) -c $^

menu.o	:	menu.c
		$(CC) -c $^



server	:	server.o server.h $(HEAD)
		$(CC) $(FLAGS) $^ -o $@

server.o:	server.c
		$(CC) -c $^

clean	:
		rm -rf *.o *.out client server