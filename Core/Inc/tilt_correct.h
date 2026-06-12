#ifndef TILT_CORRECT_H
#define TILT_CORRECT_H

#include "stm32h7xx_hal.h"
#include "paper_detect.h" // To reuse Point, Quad, Matrix3x3 structures and matrix operations

// Function to correct the perspective of object corners using the paper's homography
void correct_perspective(Point *object_corners, int num_corners, Quad paper_quad, float *corrected_side_length);

#endif /* TILT_CORRECT_H */
