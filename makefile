SRCS = notJustCats.c notJustFunctions.h notJustFunctions.c
OBJS = notJustCats.o notJustFunctions.o
EXEC = notJustCats
CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lm

default:
	$(CC) $(CFLAGS) $(SRCS) -o notJustCats

clean:
	rm -f $(OBJS) $(EXEC)

run:
	./notJustCats