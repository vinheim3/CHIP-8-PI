linux:
	g++ chip8.c -o chip8 -lSDL -Wall -pthread -lpigpio -lrt
