#ifndef MONO_SOLVER_H
#define MONO_SOLVER_H

#include "stm32h7xx_hal.h"

float calculate_real_size(float pixel_size, float paper_width_px);
float calculate_distance(float paper_width_px);

#endif /* MONO_SOLVER_H */
