#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <pigpio.h>

#include <SDL/SDL.h>

#include "buzzer.c"
#include "emulator.c"

#define TICK_INTERVAL    1000.0/60
#define FILE_NAME        "chip.bin"

SDL_Surface *window;
SDL_Event event;

Uint32 now = 0;
bool quit = false;

/*static const int key_map[SDLK_LAST] = {
    [SDLK_x] = 1,
    [SDLK_1] = 2,
    [SDLK_2] = 3,
    [SDLK_3] = 4,
    [SDLK_q] = 5,
    [SDLK_w] = 6,
    [SDLK_e] = 7,
    [SDLK_a] = 8,
    [SDLK_s] = 9,
    [SDLK_d] = 10,
    [SDLK_z] = 11,
    [SDLK_c] = 12,
    [SDLK_4] = 13,
    [SDLK_r] = 14,
    [SDLK_f] = 15,
    [SDLK_v] = 16
};*/

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

    if (sound == 0) {
        sound = -1;
        buzz();
    }

    while (SDL_PollEvent(&event)) {
        ///store key press state
        if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {   
            uint8_t sym = event.key.keysym.sym;

            if (sym == SDLK_ESCAPE)
                closeSDL();
            //else if (key_map[sym])
            //    key[key_map[sym] - 1] = (event.type == SDL_KEYDOWN);
        }
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

    do {
        mainloop();
    } while (!quit);

    return 0;
}
