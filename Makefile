CFLAGS += -fPIC -Wall -g `pkg-config --cflags glib-2.0`
LDFLAGS = -ljack -lpthread -g -lsmf -rdynamic

all: useq.so

useq.so: useq.o useq-test.o useq-jack.o useq-output.o useq-track.o
	gcc -fPIC -shared -Wl,-soname=$@ -o $@ $^ $(LDFLAGS)

useq.o: useq.c useq.h Makefile

useq-jack.o: useq-jack.c useq.h
useq-output.o: useq-output.c useq.h
useq-track.o: useq-track.c useq.h

useq-test.o: useq-test.c useq.h

run: useq.so
	./useq skylark.mid fluidsynth:midi

gdb: useq
	gdb --args ./useq skylark.mid fluidsynth:midi

clean:
	rm -f *.o
