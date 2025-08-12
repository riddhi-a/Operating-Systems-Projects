CC = g++
TARGETS = lru fifo

all: $(TARGETS)

lru: lru.cpp
	$(CC) -o lru lru.cpp

fifo: fifo.cpp
	$(CC) -o fifo fifo.cpp

clean:
	rm -f *.o $(TARGETS)

