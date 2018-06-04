GCC = gcc

CFLAGS = `pkg-config --cflags gtk+-3.0`

LIBS = `pkg-config --libs gtk+-3.0`

all: main

main: src/main.c
	$(GCC) $(CFLAGS) -o main src/main.c $(LIBS)

clean:
	rm main
