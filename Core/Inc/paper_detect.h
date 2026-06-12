#ifndef PAPER_DETECT_H
#define PAPER_DETECT_H

#include "stm32h7xx_hal.h"

// Define a structure for a point
typedef struct {
    int x;
    int y;
} Point;

// Define a structure for a quadrilateral
typedef struct {
    Point corners[4];
} Quad;

// Canny Edge Detection helper functions
void sobel_filter(uint8_t *src, int16_t *mag, uint8_t *dir, int width, int height);
void non_maximum_suppression(int16_t *mag, uint8_t *dir, uint8_t *dst, int width, int height);
void hysteresis_thresholding(uint8_t *image, int width, int height, int low_threshold, int high_threshold);

// A simple dynamic array for points (contour)
typedef struct {
    Point *points;
    int count;
    int capacity;
} Contour;

void init_contour(Contour *c);
void add_point_to_contour(Contour *c, Point p);
void free_contour(Contour *c);

// Contour finding helper
void find_contours(uint8_t *binary_image, int width, int height, Contour **contours_list, int *num_contours);
// Polygon approximation
Contour approximate_polygon(Contour *src_contour, float epsilon);
// Check if a contour is a quadrilateral
int is_quadrilateral(Contour *contour);
// Calculate perimeter
float contour_perimeter(Contour *contour);
// Calculate area
float contour_area(Contour *contour);

// Basic 3x3 matrix structure
typedef struct {
    float mat[3][3];
} Matrix3x3;

// Function to multiply two 3x3 matrices
void multiply_matrix3x3(Matrix3x3 *A, Matrix3x3 *B, Matrix3x3 *C);
// Function to get the inverse of a 3x3 matrix
int inverse_matrix3x3(Matrix3x3 *A, Matrix3x3 *invA);

Quad find_largest_quad(uint8_t *binary_image, int width, int height, float *paper_width_px);
void perspective_transform(uint8_t *src_image, uint8_t *dst_image, int src_width, int src_height, Quad paper_quad, int dst_width, int dst_height);

#endif /* PAPER_DETECT_H */
