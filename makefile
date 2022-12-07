SRCS = notJustCats.c notJustFunctions.h notJustFunctions.c
OBJS = notJustCats.o notJustFunctions.o
EXEC = notjustcats
CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lm

default:
	$(CC) -o $(EXEC) $(SRCS) $(LDFLAGS) $(CFLAGS)

clean:
	rm -f $(OBJS) $(EXEC) project4.tgz
	clear

run: default
	./$(EXEC)

tar:
	tar -czvf project4.tgz README.md makefile $(SRCS)