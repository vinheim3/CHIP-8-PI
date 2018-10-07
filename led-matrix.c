#include <stdbool.h>

#include "led-matrix-c.h"

struct RGBLedMatrix *ledMatrix;
struct LedCanvas *canvas;

bool ledMatrixInitialised = false;

void drawScreen(bool **screen, int cols, int rows) {
    if (!ledMatrixInitialised)
        init_led_matrix();

    led_canvas_clear(canvas);

    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            if (screen[i][j] == 1)
                led_canvas_set_pixel(canvas, j, i, 0xFF, 0xFF, 0xFF);

    canvas = led_matrix_swap_on_vsync(ledMatrix, canvas);
}

void cleanup_led_matrix(void) {
    if (!ledMatrixInitialised) return;
    led_matrix_delete(ledMatrix);
    ledMatrixInitialised = false;
}

void init_led_matrix(void) {
    ledMatrix = led_matrix_create(32, 1, 1);
    canvas = led_matrix_create_offscreen_canvas(ledMatrix);
    ledMatrixInitialised = true;
}