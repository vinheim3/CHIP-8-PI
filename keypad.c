#include <stdbool.h>
#include <pigpio.h>

uint8_t Matrix[4][4] = {
    {0x1,0x2,0x3,0xA},
    {0x4,0x5,0x6,0xB},
    {0x7,0x8,0x9,0xC},
    {0xE,0x0,0xF,0xD}
};

int Row[4] = {26,19,13,6};
int Col[4] = {21,20,16,12};

bool keypadInitialised = false;

void cleanup_keypad(void) {
    if (!keypadInitialised) return;

    for (int j = 0; j < 4; j++) {
        gpioWrite(Col[j], 0);
        gpioSetMode(Col[j], PI_INPUT);
    }

    for (int i = 0; i < 4; i++)
        gpioSetPullUpDown(Row[i], PI_PUD_OFF);
}

void init_keypad(void) {
    for (int j = 0; j < 4; j++) {
        gpioSetMode(Col[j], PI_OUTPUT);
        gpioWrite(Col[j], 1);
    }

    for (int i = 0; i < 4; i++) {
        gpioSetMode(Row[i], PI_INPUT);
        gpioSetPullUpDown(Row[i], PI_PUD_UP);
    }

    keypadInitialised = true;
}

void set_key(bool * key) {
    if (!keypadInitialised)
        init_keypad();

    for (int j = 0; j < 4; j++) {
        gpioWrite(Col[j], 0);
        for (int i = 0; i < 4; i++) {
            key[Matrix[i][j]] = (gpioRead(Row[i]) == 0);
        }
        gpioWrite(Col[j], 1);
    }
}
