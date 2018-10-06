#include <stdio.h>
#include <pigpio.h>

char Matrix[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

int Row[4] = {6,13,19,26};
int Col[4] = {12,16,20,21};

int main(int argc, char *argv[])
{
    if (gpioInitialise() < 0)
    {
        fprintf(stderr, "pigpio initialisation failed\n");
        return 1;
    }

    for (int j = 0; j < 4; j++) {
        gpioSetMode(Col[j], PI_OUTPUT);
        gpioWrite(Col[j], 1);
    }

    for (int i = 0; i < 4; i++) {
        gpioSetMode(Row[i], PI_INPUT);
        gpioSetPullUpDown(Row[i], PI_PUD_UP);
    }

    for (;;) {
        for (int j = 0; j < 4; j++) {
            gpioWrite(Col[j], 0);
            for (int i = 0; i < 4; i++) {
                if (gpioRead(Row[i]) == 0) {
                    printf("%c\n", Matrix[i][j]);
                    while (gpioRead(Row[i]) == 0) {;}
                }
            }
            gpioWrite(Col[j], 1);
        }
    }

    /* Stop DMA, release resources */
    gpioTerminate();

    return 0;
}
