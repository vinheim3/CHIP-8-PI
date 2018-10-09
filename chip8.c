#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>
#include <signal.h>
#include <pigpio.h>

#include "keypad.c"
#include "buzzer.c"
#include "emulator.c"
#include "led-matrix.c"

#define TICK_INTERVAL    1000.0/60
#define FILE_NAME        "chip.bin"
#define BUZZ_FRAMES      1

volatile bool quit = false;

void close_all(int _) {
    cleanup_buzzer();
    cleanup_keypad();
    cleanup_led_matrix();
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

    if (draw) {
        draw = false;
        drawScreen(screen, SCR_WIDTH, SCR_HEIGHT);
    }

    if (!paused && delay > 0)
        delay--;

    if (sound > 0)
        sound--;

    /* better system - this won't do 2 buzzes if done 1 frame after the other */
    if (sound == -1) {
        sound = -1 - BUZZ_FRAMES;
        unbuzz();
    }

    if (sound == 0) {
        sound = -1;
        buzz();
    }
    /* end better system */

    set_key(key);
}

int main(int argc, char **argv) {
    if (gpioInitialise() < 0) {
        fprintf(stderr, "pigpio initialisation failed\n");
        return 1;
    }

    initialize();
    init_buzzer();
    init_keypad();
    init_led_matrix(argc, argv);
    loadGame(FILE_NAME);

    signal(SIGINT, close_all);
    signal(SIGTERM, close_all);

    do {
        mainloop();
    } while (!quit);

    return 0;
}
