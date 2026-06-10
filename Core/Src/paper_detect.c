#include "paper_detect.h"
#include "config.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// 8-connectivity direction offsets
const int dx_8[8] = {1, 1, 0, -1, -1, -1, 0, 1};
const int dy_8[8] = {0, -1, -1, -1, 0, 1, 1, 1};

// ---- Sobel Edge Detection ----
void sobel_filter(uint8_t *src, int16_t *mag, uint8_t *dir, int width, int height)
{
    int Gx, Gy;
    int i, j;

    const int8_t sobel_x[3][3] = {{-1, 0, 1},
                                   {-2, 0, 2},
                                   {-1, 0, 1}};

    const int8_t sobel_y[3][3] = {{1,  2,  1},
                         