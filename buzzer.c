#include <stdbool.h>
#include <pigpio.h>

#define BUZZER_GPIO 5
bool buzzerInitialised = false;

void init_buzzer(void) {
    gpioSetMode(BUZZER_GPIO, PI_OUTPUT);
    buzzerInitialised = true;
}

void buzz(void) {
    if (!buzzerInitialised)
        init_buzzer();

    gpioWrite(BUZZER_GPIO, 1); /* on */
}

void unbuzz(void) {
    if (!buzzerInitialised)
        init_buzzer();

    gpioWrite(BUZZER_GPIO, 0); /* off */
}

void cleanup_buzzer(void) {
    if (!buzzerInitialised) return;

    unbuzz();
    gpioSetMode(BUZZER_GPIO, PI_INPUT);

    buzzerInitialised = false;
}