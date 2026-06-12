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
                                  Contour **contours_list, int *num_contours);

// ---- Find Smallest Square (among multiple squares in an ROI) ----
int find_smallest_square(uint8_t *roi, int width, int height, float *pixel_size)
{
    *pixel_size = 0.0f;

    Contour *contours_list = NULL;
    int num_contours = 0;
    find_object_contours(roi, width, height, &contours_list, &num_contours);

    float smallest_side = 1e9f;
    int found = 0;

    for (int i = 0; i < num_contours; i++) {
        if (contour_area(&contours_list[i]) < (float)MIN_CONTOUR_AREA)
            continue;

        float epsilon = 0.04f * contour_perimeter(&contours_list[i]);
        Contour approx = approximate_polygon(&contours_list[i], epsilon);

        if (approx.count == 4) {
            // Check if it's square-like (aspect ratio near 1)
            Point corners[4];
            for (int j = 0; j < 4; j++)
                corners[j] = approx.points[j];

            // Sort corners: top-left, top-right, bottom-right, bottom-left
            float cx = 0.0f, cy = 0.0f;
            for (int j = 0; j < 4; j++) {
                cx += (float)corners[j].x; cy += (float)corners[j].y;
            }
            cx /= 4.0f; cy /= 4.0f;

            for (int j = 0; j < 4; j++) {
                for (int k = j + 1; k < 4; k++) {
                    int swap = 0;
                    if (((float)corners[j].y < cy) != ((float)corners[k].y < cy))
                        swap = ((float)corners[j].y > (float)corners[k].y);
                    else
                        swap = ((float)corners[j].x > (float)corners[k].x);
                    if ((swap && (float)corners[j].y < cy) ||
                        (!swap && (float)corners[j].y >= cy &&
                         (float)corners[j].x > (float)corners[k].x)) {
                        Point tmp = corners[j]; corners[j] = corners[k]; corners[k] = tmp;
                    }
                }
            }

            float top_w = sqrtf(powf((float)(corners[1].x - corners[0].x), 2) +
                                powf((float)(corners[1].y - corners[0].y), 2));
            float side_h = sqrtf(powf((float)(corners[3].x - corners[0].x), 2) +
                                 powf((float)(corners[3].y - corners[0].y), 2));

            if (top_w < 5.0f || side_h < 5.0f) {
                free(approx.points);
                continue;
            }

            float aspect_ratio = top_w / side_h;
            if (aspect_ratio > 0.9f && aspect_ratio < 1.1f) {
                float side = (top_w + side_h) * 0.5f;
                if (side < smallest_side) {
                    smallest_side = side;
                    found = 1;
                }
            }
        }
        if (approx.points != NULL) free(approx.points);
    }

    if (found) *pixel_size = smallest_side;

    for (int i = 0; i < num_contours; i++)
        free_contour(&contours_list[i]);
    free(contours_list);

    return found;
}

// ---- Recognize Digit in a Square ROI ----
int recognize_digit(uint8_t *square_roi, int width, int height)
{
    // Simple digit recognition using feature-based approach
    // Analyze the binary image to determine which digit (0-9) it contains

    // Count black pixels (digit object) in each cell of a 5x3 grid
    int grid_h = height / 5;
    int grid_w = width / 3;
    int cell_black[5][3] = {{0}};
    int cell_total[5][3] = {{0}};

    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 3; col++) {
            int y0 = row * grid_h;
            int y1 = (row + 1) * grid_h;
            int x0 = col * grid_w;
            int x1 = (col + 1) * grid_w;
            if (y1 > height) y1 = height;
            if (x1 > width) x1 = width;

            int black = 0, total = 0;
            for (int y = y0; y < y1; y++) {
                for (int x = x0; x < x1; x++) {
                    total++;
                    if (square_roi[y * width + x] == 0) black++;
                }
            }
            cell_black[row][col] = black;
            cell_total[row][col] = total;
        }
    }

    // Calculate fill ratios for each cell
    float fill[5][3];
    for (int r = 0; r < 5; r++)
        for (int c = 0; c < 3; c++)
            fill[r][c] = (cell_total[r][c] > 0) ?
                         (float)cell_black[r][c] / (float)cell_total[r][c] : 0.0f;

    // Feature vectors for each cell area
    // Row 0: top bar, Row 2: middle bar, Row 4: bottom bar
    float top_bar    = (fill[0][0] + fill[0][1] + fill[0][2]) / 3.0f;
    float mid_bar    = (fill[2][0] + fill[2][1] + fill[2][2]) / 3.0f;
    float bottom_bar = (fill[4][0] + fill[4][1] + fill[4][2]) / 3.0f;

    // Columns for vertical segments
    float left_top    = (fill[0][0] + fill[1][0]) / 2.0f;
    float left_bottom = (fill[3][0] + fill[4][0]) / 2.0f;
    float right_top    = (fill[0][2] + fill[1][2]) / 2.0f;
    float right_bottom = (fill[3][2] + fill[4][2]) / 2.0f;

    // Threshold for "segment is ON"
    #define ON(f)  ((f) > 0.25f)

    int has_top    = ON(top_bar);
    int has_mid    = ON(mid_bar);
    int has_bottom = ON(bottom_bar);
    int has_lt     = ON(left_top);
    int has_lb     = ON(left_bottom);
    int has_rt     = ON(right_top);
    int has_rb     = ON(right_bottom);

    // Seven-segment style classification
    if (has_top && has_bottom && !has_mid && has_lt && has_lb && has_rt && has_rb)
        return 0;
    if (!has_top && !has_mid && !has_bottom && !has_lt && !has_lb && has_rt && has_rb)
        return 1;
    if (has_top && has_mid && has_bottom && !has_lt && has_lb && has_rt && !has_rb)
        return 2;
    if (has_top && has_mid && has_bottom && !has_lt && !has_lb && has_rt && has_rb)
        return 3;
    if (!has_top && has_mid && !has_bottom && has_lt && has_lb && has_rt && has_rb)
        return 4;
    if (has_top && has_mid && has_bottom && has_lt && !has_lb && !has_rt && has_rb)
        return 5;
    if (has_top && has_mid && has_bottom && has_lt && has_lb && !has_rt && has_rb)
        return 6;
    if (has_top && !has_mid && !has_bottom && !has_lt && !has_lb && has_rt && has_rb)
        return 7;
    if (has_top && has_mid && has_bottom && has_lt && has_lb && has_rt && has_rb)
        return 8;
    if (has_top && has_mid && has_bottom && has_lt && has_lb && has_rt && has_rb)
        return 9;

    // Fallback: use overall aspect ratio of the digit bounding box
    int min_x = width, max_x = 0, min_y = height, max_y = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (square_roi[y * width + x] == 0) {
                if (x < min_x) min_x = x;
                if (x > max_x) max_x = x;
                if (y < min_y) min_y = y;
                if (y > max_y) max_y = y;
            }
        }
    }
    if (max_x <= min_x || max_y <= min_y) return -1;

    float aspect_ratio = (float)(max_x - min_x) / (float)(max_y - min_y);

    if (aspect_ratio > 0.8f && aspect_ratio < 1.2f) return 0; // 0 is more rounded
    else if (aspect_ratio < 0.6f) return 1;                     // 1 is narrow
    else if (aspect_ratio > 0.7f && aspect_ratio < 1.3f) {
        // Check mid-bar for 8 vs others
        if (has_mid) return 8;
        else return 0;
    }
    return 8; // Default
}
