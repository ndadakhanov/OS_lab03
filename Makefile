CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pedantic
LDFLAGS = -pthread -lrt

all: parent child

parent: parent.c common.h
	$(CC) $(CFLAGS) parent.c -o parent $(LDFLAGS)

child: child.c common.h
	$(CC) $(CFLAGS) child.c -o child $(LDFLAGS)

clean:
	rm -f parent child

.PHONY: all clean