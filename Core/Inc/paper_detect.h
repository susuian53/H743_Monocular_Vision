#ifndef PAPER_DETECT_H
#define PAPER_DETECT_H

#include "stm32h7xx_hal.h"

// Define a structure for a point
typedef struct {
    int x;
    int y;
} Point;

// Define a structure for a quadrilateral
typedef struct {
    Point corners[4];
} Quad;

// Canny Edge Detection helper functions
void sobel_filter(uint8_t *src, int16_t *mag, uint8_t *dir, int width, int height);
void non_maximum_suppression(int16_t *mag, uint8_t *dir, uint8_t *dst, int width, int height);
void hysteresis_thresholding(uint8_t *image, int 