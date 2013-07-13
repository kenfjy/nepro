CFLAGS = -Wall -g
all: echo_server

echo_server: echo_server.o
	$(CC) $(CFLAGS) -o echo_server echo_server.o

echo_server.o: echo_server.c
	$(CC) $(CFLAGS) -c echo_server.c
