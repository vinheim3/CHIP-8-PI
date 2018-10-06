#include <stdbool.h>
#include <pigpio.h>

#define BUZZER_GPIO 5
#define BUZZER_TIME 0.2
bool buzzerInitialised = false;

void buzz(void) {
    if (!buzzerInitialised) {
        gpioSetMode(BUZZER_GPIO, PI_OUTPUT);
        buzzerInitialised = true;
    }

    gpioWrite(BUZZER_GPIO, 1); /* on */
    time_sleep(BUZZER_TIME);
    gpioWrite(BUZZER_GPIO, 0); /* off */
}
