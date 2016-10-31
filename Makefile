CFLAGS += -Wall -g
LDFLAGS = -ljack -lpthread -g

all: useq

useq: useq.c

run: useq
	./useq

clean:
	rm -f *.o
	