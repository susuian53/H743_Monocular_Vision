#include "tilt_correct.h"
#include "config.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

// Extern declarations for functions from paper_detect.c
extern int get_perspective_transform(Point src[4], Point dst[4], Matrix3x3 *H);
extern int inverse_matrix3x3(Matrix3x3 *A, Matrix3x3 *invA);

void correct_perspective(Point *object