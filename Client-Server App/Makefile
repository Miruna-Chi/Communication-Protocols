CC=g++
CFLAGS= -Wall -g

build: server subscriber

server: server.cpp
	$(CC) $(CLAGS) server.cpp message.cpp -o server

subscriber: subscriber.cpp
	$(CC) $(CLAGS) subscriber.cpp message.cpp -o subscriber

.PHONY: clean server subscriber

clean:
	rm -f server subscriber
