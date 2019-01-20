OBJS = mylib.o point.o server.o client.o
CC = gcc
CFLAGS = -Wall -pthread


.PHONY: all
all:
	echo hello


point.out: point.o mylib.o
	$(CC) $(CFLAGS) point.o mylib.o -o $@
server.out: server.o mylib.o
	$(CC) $(CFLAGS) server.o mylib.o -o $@
client.out: client.o mylib.o
	$(CC) $(CFLAGS) client.o mylib.o -o $@

$(OBJS): %.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

mylib.o: mylib.h
point.o: point.h mylib.h
server.o: server.h mylib.h
client.o: client.h mylib.h


.PHONY: clean
clean:
	rm -f $(OBJS) point.out server.out client.out

