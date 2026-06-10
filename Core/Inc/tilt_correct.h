#ifndef TILT_CORRECT_H
#define TILT_CORRECT_H

#include "stm32h7xx_hal.h"
#include "paper_detect.h" // To reuse Point, Quad, Matrix3x3 structures and matrix operations

// Function to correct the perspe