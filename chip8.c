#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>
#include <pigpio.h>

#include <SDL/SDL.h>

#include "buzzer.c"
#include "emulator.c"

#define TICK_INTERVAL    1000.0/60
#define FILE_NAME        "chip.bin"
#define BUZZ_FRAMES      1

SDL_Surface *window;

Uint32 now = 0;
volatile bool quit = false;

void drawScreen(SDL_Surface *dest) {
    static uint8_t col;
    static SDL_Rect *currRect;
    
    if (true) {
        draw = false;
        
        SDL_FillRect(dest, &screenRect, SDL_MapRGB(dest->format, 255, 255, 255));
        for (int i = 0; i < SCR_HEIGHT; i++)
            for (int j = 0; j < SCR_WIDTH; j++) {
                if (newScreen[i][j] == 0)
                    continue;
                
                col = (1-newScreen[i][j])*255;
                currRect = &rects[i][j];
                SDL_FillRect(dest, currRect, SDL_MapRGB(dest->format, col, col, col));
            }
        SDL_UpdateRect(dest, 0, 0, 0, 0);
    }
    
    memcpy(screen, newScreen, SCR_HEIGHT*SCR_WIDTH);
}

void closeSDL() {
    SDL_FreeSurface(window);
    SDL_Quit();
    gpioTerminate();
    exit(-1);
    quit = true;
}

void mainloop() {
    ///tick down 60Hz
    now = SDL_GetTicks();
    allowDraw = true;
    while (SDL_GetTicks() - now <= TICK_INTERVAL-1) {
        emulatecycle();
        allowDraw = false;
    }
    drawScreen(window);

    if (!paused && delay > 0)
        delay--;

    if (sound > 0)
        sound--;

    if (sound == -1) {
        sound = -1 - BUZZ_FRAMES;
        unbuzz();
    }

    if (sound == 0) {
        sound = -1;
        buzz();
    }

    set_key(key);
}

int main(int argc, char* args[]) {
    if (gpioInitialise() < 0) {
        fprintf(stderr, "pigpio initialisation failed\n");
        return 1;
    }

    SDL_Init(SDL_INIT_VIDEO);

    window = SDL_SetVideoMode(SCR_WIDTH*pxSz, SCR_HEIGHT*pxSz, 8, SDL_SWSURFACE|SDL_DOUBLEBUF);
    SDL_Flip(window);

    initialize();
    loadGame(FILE_NAME);

    signal(SIGINT, closeSDL);
    signal(SIGTERM, closeSDL);

    do {
        mainloop();
    } while (!quit);

    return 0;
}
