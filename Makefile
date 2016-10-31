CFLAGS += -Wall
LDFLAGS = -ljack

all: useq

useq: useq.c

run: useq
	./useq

clean:
	rm -f *.o
	