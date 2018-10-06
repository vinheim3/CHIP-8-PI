#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>
#include <signal.h>
#include <pigpio.h>

#include <SDL/SDL.h>

#include "keypad.c"
#include "buzzer.c"
#include "emulator.c"

#define TICK_INTERVAL    1000.0/60
#define FILE_NAME        "chip.bin"
#define BUZZ_FRAMES      1

#ifdef _SDL_H
#define pxSz             10
SDL_Surface *window;
SDL_Rect screenRect;
SDL_Rect rects[SCR_HEIGHT][SCR_WIDTH];
#endif

volatile bool quit = false;

void drawScreen(void) {
#ifdef _SDL_H
    static uint8_t col;
    static SDL_Rect *currRect;
    SDL_Surface *dest = window;
    
    if (draw) {
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
#endif
    
    memcpy(screen, newScreen, SCR_HEIGHT*SCR_WIDTH);
}

void closeSDL(int _) {
#ifdef _SDL_H
    SDL_FreeSurface(window);
    SDL_Quit();
#endif
    cleanup_keypad();
    cleanup_buzzer();
    gpioTerminate();
    exit(-1);
    quit = true;
}

float timedifference_msec(struct timeval t0, struct timeval t1) {
    return (t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f;
}

void mainloop() {
    ///tick down 60Hz
    allowDraw = true;

    struct timeval now, looped;
    gettimeofday(&now, 0); gettimeofday(&looped, 0);
    while (timedifference_msec(now, looped) <= TICK_INTERVAL-1) {
        emulatecycle();
        allowDraw = false;
        gettimeofday(&looped, 0);
    }
    drawScreen();

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

#ifdef _SDL_H
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_SetVideoMode(SCR_WIDTH*pxSz, SCR_HEIGHT*pxSz, 8, SDL_SWSURFACE|SDL_DOUBLEBUF);
    screenRect.x = 0;
    screenRect.y = 0;
    screenRect.w = SCR_WIDTH * pxSz;
    screenRect.h = SCR_HEIGHT * pxSz;
    for (int i = 0; i < SCR_HEIGHT; i++)
        for (int j = 0; j < SCR_WIDTH; j++) {
            rects[i][j].x = j*pxSz;
            rects[i][j].y = i*pxSz;
            rects[i][j].w = pxSz;
            rects[i][j].h = pxSz;
        }
#endif

    initialize();
    loadGame(FILE_NAME);

    signal(SIGINT, closeSDL);
    signal(SIGTERM, closeSDL);

    do {
        mainloop();
    } while (!quit);

    return 0;
}
