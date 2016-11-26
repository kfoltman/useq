CFLAGS += -Wall -g
LDFLAGS = -ljack -lpthread -g

all: useq

useq: useq.o

useq.o: useq.c useq.h

run: useq
	./useq

gdb: useq
	gdb --args ./useq

clean:
	rm -f *.o
