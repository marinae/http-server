CC=g++
CFLAGS=-std=c++11
LDFLAGS=-levent
SOURCES=classes.cpp helper.cpp callback.cpp server.cpp main.cpp
EXECUTABLE=wwwd

all:
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o $(EXECUTABLE)

test:
	$(CC) -std=c++11 -fopenmp test.cpp -o $@

clean:
	rm -f *.o
	rm -f $(EXECUTABLE)
	rm -f test

server8080:
	./wwwd -d files -h 127.0.0.1 -p 8080 -w 4

server8081:
	./wwwd -d files -h 127.0.0.1 -p 8081 -w 4