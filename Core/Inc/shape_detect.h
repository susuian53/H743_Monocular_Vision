#ifndef SHAPE_DETECT_H
#define SHAPE_DETECT_H

#include "stm32h7xx_hal.h"
#include "paper_detect.h" // To reuse Point and Contour structures

typedef enum {
    SHAPE_UNKNOWN,
    SHAPE_CIRCLE,
    SHAPE_TRIANGLE,
    SHAPE_SQUARE
} ShapeType;

// Function to detect shapes in an ROI
ShapeType detect_shape(uint8_t *roi, int width, int height, float *pixel_size);

// Helper functions for contour finding and shape analysis
void find_object_contours(uint8_t *binary_image, int width, int height, Contour **contours_list, int *num_contours);
void calculate_hu_moments(Contour *contour, double hu_moments[7]);
ShapeType match_shape_by_hu_moments(double hu_moments[7]);
void sort_quad_corners(Point corners[4]);

#endif /* SHAPE_DETECT_H */
