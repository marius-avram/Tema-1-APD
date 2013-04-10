all: build

build: serial.c paralel.c
	gcc serial.c -o serial
	gcc -fopenmp paralel.c -o paralel

clean: 
	-rm -f *.o serial paralel
	
.PHONY: all clean

