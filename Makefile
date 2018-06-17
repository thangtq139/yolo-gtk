GCC = gcc
# GCC = nvcc

CFLAGS = -DOPENCV `pkg-config --cflags gtk+-2.0` `pkg-config --cflags opencv` -Idarknet/include/

LIBS = `pkg-config --libs gtk+-2.0` `pkg-config --libs opencv` darknet/libdarknet.a -lm

all: main

main: src/main.c
	$(GCC) $(CFLAGS) -o main src/main.c $(LIBS)

clean:
	rm main
