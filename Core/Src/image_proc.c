#include "image_proc.h"
#include "config.h"
#include <string.h>
#include <stdlib.h>

// 3x3 Gaussian blur with a temporary buffer
void gaussian_blur(uint8_t *src, uint8_t *dst, int width, int height) {
    // Gaussian kernel for 3x3
    const int kernel[3][3] = {{1, 2, 1},
                              {2, 4, 2},
                              {1, 2, 1}};
    const int kernel_sum = 16;

    // Create a temporary buffer to store intermediate results
    // This is necessary if src and dst are the same buffer for in-place blur.
    // If src != dst, this temporary buffer is still safe.
    uint8_t *temp_buffer = (uint8_t *)malloc(width * height);
    if (tem