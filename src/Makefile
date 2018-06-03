GCC = gcc

CFLAGS = `pkg-config --cflags gtk+-3.0`

LIBS = `pkg-config --libs gtk+-3.0`

all: main

main: main.c
	gcc $(CFLAGS) -o main main.c $(LIBS)

clean:
	rm main
