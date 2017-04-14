CC = gcc

CFLAGS = -Wall -g -c

## A list of options to pass to the linker. 
LDFLAGS = -Wall -g -lm -lglut -lGLU -lGL

## Name the executable program, list source files
SRCS = fractal_landscape.c 

## Build the program from the object files (-o option)
glTest: fractal_landscape.o
	$(CC) fractal_landscape.o -o fractal_landscape $(LDFLAGS)

fractal_landscape.o: fractal_landscape.c
	$(CC) $(CFLAGS) fractal_landscape.c


## Remove all the compilation and debugging files
clean: 
	rm -f core fractal_landscape fractal_landscape.o *~
