#include <stdio.h>
#include <pigpio.h>

int main(int argc, char *argv[])
{
    if (gpioInitialise() < 0)
    {
        fprintf(stderr, "pigpio initialisation failed\n");
        return 1;
    }

    /* Set GPIO modes */
    gpioSetMode(5, PI_OUTPUT);

    gpioWrite(5, 1); /* on */
    time_sleep(0.2);
    gpioWrite(5, 0); /* off */

    /* Stop DMA, release resources */
    gpioTerminate();

    return 0;
}
