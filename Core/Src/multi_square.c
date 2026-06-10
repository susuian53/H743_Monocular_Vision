#include "multi_square.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Extern declarations from paper_detect.c
extern void init_contour(Contour *c);
extern void add_point_to_contour(Contour *c, Point p);
extern void free_contour(Contour *c);
extern float contour_area(Contour *c);
extern float contour_perimeter(Contour *c);
extern Contour approximate_polygon(Contour *src_contour, float epsilon);
extern int is_quadrilateral(Contour *contour);
extern void find_object_contours(uint8_t *binary_image, int width, int height,
                                  Contour