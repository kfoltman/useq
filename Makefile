CFLAGS += -Wall -g
LDFLAGS = -ljack -lpthread -g

all: useq

useq: useq.c

run: useq
	./useq

gdb: useq
	gdb --args ./useq

clean:
	rm -f *.o
	