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
void find_object_contours(uint8_t *binary_image, int width, int height, Contour **contours_list, int *num_contours)
{
    uint8_t *visited = (uint8_t *)calloc(width * height, 1);
    int max_contours = 256;
    *contours_list = (Contour *)malloc(max_contours * sizeof(Contour));
    *num_contours = 0;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;
            if (binary_image[idx] == 0 && !visited[idx]) {
                Contour contour;
                contour.points = NULL;
                contour.count = 0;
                contour.capacity = 0;

                // Allocate initial capacity
                contour.capacity = 64;
                contour.points = (Point *)malloc(contour.capacity * sizeof(Point));

                if (contour.points == NULL) continue;

                int cx = x, cy = y;
                int start_dir = 7;
                int first = 1;

                while (first || !(cx == x && cy == y)) {
                    first = 0;
                    visited[cy * width + cx] = 1;

                    // Add point with realloc if needed
                    if (contour.count >= contour.capacity) {
                        contour.capacity *= 2;
                        contour.points = (Point *)realloc(contour.points, contour.capacity * sizeof(Point));
                        if (contour.points == NULL) break;
                    }
                    contour.points[contour.count].x = cx;
                    contour.points[contour.count].y = cy;
                    contour.count++;

                    int found_next = 0;
                    for (int d = 0; d < 8; d++) {
                        int dir = (start_dir + d) % 8;
                        int nx = cx + dx_8[dir];
                        int ny = cy + dy_8[dir];
                        if (nx >= 0 && nx < width && ny >= 0 && ny < height &&
                            binary_image[ny * width + nx] == 0) {
                            if (!visited[ny * width + nx] || (nx == x && ny == y && contour.count > 2)) {
                                cx = nx; cy = ny;
                                start_dir = (dir + 5) % 8;
                                found_next = 1;
                                break;
                            }
                        }
                    }
                    if (!found_next) break;
                    if (contour.count > width + height) break;
                }

                if (contour.count > 10 && *num_contours < max_contours) {
                    (*contours_list)[(*num_contours)++] = contour;
                } else {
                    if (contour.points != NULL) free(contour.points);
                }
            }
        }
    }
    free(visited);
}

// ---- Calculate Hu Moments for Shape Recognition ----
void calculate_hu_moments(Contour *contour, double hu_moments[7])
{
    int n = contour->count;
    if (n < 3) {
        for (int i = 0; i < 7; i++) hu_moments[i] = 0.0;
        return;
    }

    // Calculate centroid
    double cx = 0.0, cy = 0.0;
    for (int i = 0; i < n; i++) {
        cx += (double)contour->points[i].x;
        cy += (double)contour->points[i].y;
    }
    cx /= (double)n;
    cy /= (double)n;

    // Calculate central moments up to order 3
    double mu20 = 0.0, mu02 = 0.0, mu11 = 0.0;
    double mu30 = 0.0, mu03 = 0.0, mu21 = 0.0, mu12 = 0.0;

    for (int i = 0; i < n; i++) {
        double x = (double)contour->points[i].x - cx;
        double y = (double)contour->points[i].y - cy;
        mu20 += x * x;
        mu02 += y * y;
        mu11 += x * y;
        mu30 += x * x * x;
        mu03 += y * y * y;
        mu21 += x * x * y;
        mu12 += x * y * y;
    }

    // Normalize by area (mu00)
    double mu00_sq = (double)n * (double)n;
    double area = sqrt(mu20 + mu02); // proxy

    mu20 /= mu00_sq; mu02 /= mu00_sq; mu11 /= mu00_sq;
    mu30 /= mu00_sq; mu03 /= mu00_sq; mu21 /= mu00_sq; mu12 /= mu00_sq;

    // Hu invariant moments
    hu_moments[0] = mu20 + mu02;
    hu_moments[1] = (mu20 - mu02) * (mu20 - mu02) + 4.0 * mu11 * mu11;
    hu_moments[2] = (mu30 - 3.0 * mu12) * (mu30 - 3.0 * mu12) +
                    (3.0 * mu21 - mu03) * (3.0 * mu21 - mu03);
    hu_moments[3] = (mu30 + mu12) * (mu30 + mu12) +
                    (mu21 + mu03) * (mu21 + mu03);
    hu_moments[4] = (mu30 - 3.0 * mu12) * (mu30 + mu12) *
                    ((mu30 + mu12) * (mu30 + mu12) - 3.0 * (mu21 + mu03) * (mu21 + mu03)) +
                    (3.0 * mu21 - mu03) * (mu21 + mu03) *
                    (3.0 * (mu30 + mu12) * (mu30 + mu12) - (mu21 + mu03) * (mu21 + mu03));
    hu_moments[5] = (mu20 - mu02) *
                    ((mu30 + mu12) * (mu30 + mu12) - (mu21 + mu03) * (mu21 + mu03)) +
                    4.0 * mu11 * (mu30 + mu12) * (mu21 + mu03);
    hu_moments[6] = (3.0 * mu21 - mu03) * (mu30 + mu12) *
                    ((mu30 + mu12) * (mu30 + mu12) - 3.0 * (mu21 + mu03) * (mu21 + mu03)) -
                    (mu30 - 3.0 * mu12) * (mu21 + mu03) *
                    (3.0 * (mu30 + mu12) * (mu30 + mu12) - (mu21 + mu03) * (mu21 + mu03));
}

// ---- Match Shape by Hu Moments ----
ShapeType match_shape_by_hu_moments(double hu_moments[7])
{
    // Log-scale for better numerical stability
    double log_hu[7];
    for (int i = 0; i < 7; i++) {
        if (hu_moments[i] < 1e-12 && hu_moments[i] > -1e-12) {
            log_hu[i] = 0.0;
        } else {
            log_hu[i] = -log10(fabs(hu_moments[i]));
        }
    }

    // Simplified shape classification using Hu moments
    // Circle: hu1 is small, hu2 is small
    double circle_score = fabs(log_hu[0]) + fabs(log_hu[1]);
    // Triangle: hu2 is relatively large
    double triangle_score = fabs(log_hu[1]);
    // Square: hu1 is relatively large, hu2 is small
    double square_score = fabs(log_hu[0]) + (10.0 - fabs(log_hu[1]));

    if (circle_score < triangle_score && circle_score < square_score * 0.8)
        return SHAPE_CIRCLE;
    else if (triangle_score > square_score && triangle_score > circle_score * 1.3)
        return SHAPE_TRIANGLE;
    else if (square_score < triangle_score * 0.8 && square_score < circle_score * 1.2)
        return SHAPE_SQUARE;
    else
        return SHAPE_UNKNOWN;
}

// ---- Sort Quad Corners (TL, TR, BR, BL) ----
void sort_quad_corners(Point corners[4])
{
    float cx = 0.0f, cy = 0.0f;
    for (int i = 0; i < 4; i++) { cx += (float)corners[i].x; cy += (float)corners[i].y; }
    cx /= 4.0f; cy /= 4.0f;

    Point sorted[4];
    int tl = -1, tr = -1, br = -1, bl = -1;
    for (int i = 0; i < 4; i++) {
        int is_top = ((float)corners[i].y < cy);
        int is_left = ((float)corners[i].x < cx);
        if (is_top && is_left && tl == -1) tl = i;
        else if (is_top && !is_left && tr == -1) tr = i;
        else if (!is_top && is_left && bl == -1) bl = i;
        else br = i;
    }
    if (tl >= 0) sorted[0] = corners[tl];
    if (tr >= 0) sorted[1] = corners[tr];
    if (br >= 0) sorted[2] = corners[br];
    if (bl >= 0) sorted[3] = corners[bl];
    for (int i = 0; i < 4; i++) corners[i] = sorted[i];
}

// ---- Detect Shape in ROI ----
ShapeType detect_shape(uint8_t *roi, int width, int height, float *pixel_size)
{
    *pixel_size = 0.0f;

    Contour *contours_list = NULL;
    int num_contours = 0;
    find_object_contours(roi, width, height, &contours_list, &num_contours);

    if (num_contours == 0) return SHAPE_UNKNOWN;

    // Find the largest contour (assumed to be the target object)
    int best_idx = -1;
    float max_area = 0.0f;
    for (int i = 0; i < num_contours; i++) {
        // Compute area using Shoelace formula
        float area = 0.0f;
        for (int j = 0; j < contours_list[i].count; j++) {
            int k = (j + 1) % contours_list[i].count;
            area += (float)contours_list[i].points[j].x * (float)contours_list[i].points[k].y;
            area -= (float)contours_list[i].points[k].x * (float)contours_list[i].points[j].y;
        }
        area = fabsf(area) * 0.5f;

        if (area > max_area && area > (float)MIN_CONTOUR_AREA) {
            max_area = area;
            best_idx = i;
        }
    }

    ShapeType result = SHAPE_UNKNOWN;

    if (best_idx >= 0) {
        // Approximate polygon to find vertex count
        float eps = 0.03f * contour_perimeter(&contours_list[best_idx]);
        Contour approx = approximate_polygon(&contours_list[best_idx], eps);

        double hu_moments[7];
        calculate_hu_moments(&contours_list[best_idx], hu_moments);

        if (approx.count >= 8) {
            result = SHAPE_CIRCLE;
            // For circle: pixel_size = diameter = 2 * sqrt(area/pi)
            *pixel_size = 2.0f * sqrtf(max_area / M_PI);
        } else if (approx.count >= 5) {
            result = match_shape_by_hu_moments(hu_moments);
            // For polygon: pixel_size = side length from bounding box
            float min_x = 1e9f, max_x = -1e9f, min_y = 1e9f, max_y = -1e9f;
            for (int i = 0; i < approx.count; i++) {
                if ((float)approx.points[i].x < min_x) min_x = (float)approx.points[i].x;
                if ((float)approx.points[i].x > max_x) max_x = (float)approx.points[i].x;
                if ((float)approx.points[i].y < min_y) min_y = (float)approx.points[i].y;
                if ((float)approx.points[i].y > max_y) max_y = (float)approx.points[i].y;
            }
            *pixel_size = sqrtf(max_area);
        }

        if (approx.points != NULL) free(approx.points);
    }

    for (int i = 0; i < num_contours; i++) {
        if (contours_list[i].points != NULL) free(contours_list[i].points);
    }
    if (contours_list != NULL) free(contours_list);

    return result;
}
