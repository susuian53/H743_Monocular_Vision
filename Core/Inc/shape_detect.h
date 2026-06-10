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
Sha