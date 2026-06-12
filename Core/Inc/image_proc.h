#ifndef IMAGE_PROC_H
#define IMAGE_PROC_H

#include "stm32h7xx_hal.h"

void gaussian_blur(uint8_t *src, uint8_t *dst, int width, int height);
int otsu_threshold(uint8_t *image, int width, int height);
void adaptive_threshold(uint8_t *src, uint8_t *dst, int width, int height, int block_size, int c);
void binarize(uint8_t *src, uint8_t *dst, int width, int height, int threshold);

#endif /* IMAGE_PROC_H */
