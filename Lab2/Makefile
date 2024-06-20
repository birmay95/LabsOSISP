CC = gcc
CFLAGS = -W -Wall -Wno-unused-parameter -Wno-unused-variable -std=c11 -pedantic

.PHONY: clean
export CHILD_PATH=/path/to/child
export LC_COLLATE=C

all: parent child run
parent: parent.c 
		$(CC) $(CFLAGS) parent.c -o parent
child: child.c 
		$(CC) $(CFLAGS) child.c -o child
clean:
		rm parent child

run:
	./parent info.txt
	make clean