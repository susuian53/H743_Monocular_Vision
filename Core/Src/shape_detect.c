#include "shape_detect.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// Reuse definitions from paper_detect.c
extern const int dx_8[8];
extern const int dy_8[8];

// ---- Find Object Contours (separate from paper detection, uses same algorithm) ----
void 