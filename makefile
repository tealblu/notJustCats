SRCS = notJustCats.c notJustFunctions.h notJustFunctions.c
OBJS = notJustCats.o notJustFunctions.o
EXEC = notJustCats
CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lm

default:
	$(CC) $(CFLAGS) $(SRCS) -o $(EXEC) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(EXEC) project4.tgz
	clear

run: default
	./$(EXEC)

tar:
	tar -czvf project4.tgz README.md makefile $(SRCS) $(EXEC)