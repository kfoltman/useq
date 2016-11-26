CFLAGS += -Wall -g `pkg-config --cflags glib-2.0`
LDFLAGS = -ljack -lpthread -g -lsmf

all: useq

useq: useq.o useq-test.o

useq.o: useq.c useq.h

useq-test.o: useq-test.c useq.h

run: useq
	./useq skylark.mid fluidsynth:midi

gdb: useq
	gdb --args ./useq skylark.mid fluidsynth:midi

clean:
	rm -f *.o
