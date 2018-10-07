linux:
	gcc chip8.c -o chip8 -lSDL -Wall -pthread -lpigpio -lrt
