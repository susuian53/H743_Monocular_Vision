#ifndef MULTI_SQUARE_H
#define MULTI_SQUARE_H

#include "stm32h7xx_hal.h"
#include "paper_detect.h" // To reuse Point and Contour structures

// Function to find the smallest square among multiple squares in an ROI
int find_smallest_square(uint8_t *roi, int width, int height, float *pixel_size);

// Function to recognize a digit within a square ROI
int recognize_digit(uint8_t *square_roi, int width, int height);



#endif /* MULTI_SQUARE_H */
