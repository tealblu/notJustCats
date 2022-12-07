SRCS = notJustCats.c notJustFunctions.h notJustFunctions.c
OBJS = notJustCats.o notJustFunctions.o
EXEC = notjustcats
CC = gcc
CFLAGS = -g
LDFLAGS = -lm

default:
	$(CC) -o $(EXEC) $(SRCS) $(LDFLAGS) $(CFLAGS)

clean:
	rm -f $(OBJS) $(EXEC) project4.tgz
	rm -f out/*
	clear

run: default
	./$(EXEC)

tar:
	tar -czvf project4.tgz README.md makefile $(SRCS)