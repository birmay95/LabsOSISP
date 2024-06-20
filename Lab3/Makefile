CC = gcc
CFLAGS=-Wall

all: parent child run
parent: parent.c 
		$(CC) $(CFLAGS) parent.c -o parent
child: child.c 
		$(CC) $(CFLAGS) child.c -o child
clean:
		rm parent child

run:
	./parent
	make clean