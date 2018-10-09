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

void close_all(int _) {
    cleanup_buzzer();
    cleanup_keypad();
    cleanup_led_matrix();
    gpioTerminate();
    exit(-1);
}

float timedifference_msec(struct timeval t0, struct timeval t1) {
    return (t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f;
}

void mainloop() {
    ///tick down 60Hz
    struct timeval start, now;
    bool successful_cycle;
    gettimeofday(&start, 0);

    while (!draw) {
        successful_cycle = emulatecycle();
        if (!successful_cycle) close_all(0);
    }

    draw = false;

    gettimeofday(&now, 0);
    time_sleep((TICK_INTERVAL - timedifference_msec(start, now)) / 1000);

    drawScreen(screen, SCR_WIDTH, SCR_HEIGHT);

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
    if (!loadGame(FILE_NAME)) close_all(0);

    signal(SIGINT, close_all);
    signal(SIGTERM, close_all);

    for (;;) mainloop();

    return 0;
}
