#include "paper_detect.h"
#include "config.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// 8-connectivity direction offsets
const int dx_8[8] = {1, 1, 0, -1, -1, -1, 0, 1};
const int dy_8[8] = {0, -1, -1, -1, 0, 1, 1, 1};

// ---- Sobel Edge Detection ----
void sobel_filter(uint8_t *src, int16_t *mag, uint8_t *dir, int width, int height)
{
    int Gx, Gy;
    int i, j;

    const int8_t sobel_x[3][3] = {{-1, 0, 1},
                                   {-2, 0, 2},
                                   {-1, 0, 1}};

    const int8_t sobel_y[3][3] = {{1,  2,  1},
                                   {0,  0,  0},
                                   {-1, -2, -1}};

    for (i = 1; i < height - 1; i++) {
        for (j = 1; j < width - 1; j++) {
            Gx = 0; Gy = 0;
            for (int r = -1; r <= 1; r++) {
                int row = i + r;
                for (int c = -1; c <= 1; c++) {
                    int col = j + c;
                    uint8_t pixel = src[row * width + col];
                    Gx += pixel * sobel_x[r + 1][c + 1];
                    Gy += pixel * sobel_y[r + 1][c + 1];
                }
            }
            mag[i * width + j] = (int16_t)sqrtf((float)(Gx*Gx + Gy*Gy));
            // Direction in one of 4 bins: 0, 45, 90, 135 degrees
            float angle = atan2f((float)Gy, (float)Gx) * 180.0f / (float)M_PI;
            if (angle < 0) angle += 180.0f;
            if ((angle < 22.5f) || (angle >= 157.5f))
                dir[i * width + j] = 0;
            else if (angle < 67.5f)
                dir[i * width + j] = 45;
            else if (angle < 112.5f)
                dir[i * width + j] = 90;
            else
                dir[i * width + j] = 135;
        }
    }
    // Zero out border pixels
    for (i = 0; i < width; i++) { mag[i] = 0; mag[(height-1)*width+i] = 0; dir[i] = 0; dir[(height-1)*width+i] = 0; }
    for (i = 0; i < height; i++) { mag[i*width] = 0; mag[i*width+width-1] = 0; dir[i*width] = 0; dir[i*width+width-1] = 0; }
}

// ---- Non-Maximum Suppression ----
void non_maximum_suppression(int16_t *mag, uint8_t *dir, uint8_t *dst, int width, int height)
{
    for (int i = 1; i < height - 1; i++) {
        for (int j = 1; j < width - 1; j++) {
            int idx = i * width + j;
            int n1 = 0, n2 = 0;
            if (dir[idx] == 0) {         // horizontal edge
                n1 = mag[idx - 1]; n2 = mag[idx + 1];
            } else if (dir[idx] == 45) { // diagonal /
                n1 = mag[idx - width + 1]; n2 = mag[idx + width - 1];
            } else if (dir[idx] == 90) { // vertical edge
                n1 = mag[idx - width]; n2 = mag[idx + width];
            } else {                     // 135 diagonal \
                n1 = mag[idx - width - 1]; n2 = mag[idx + width + 1];
            }
            dst[idx] = (mag[idx] >= n1 && mag[idx] >= n2) ? (uint8_t)(mag[idx] > 255 ? 255 : mag[idx]) : 0;
        }
    }
    for (int i = 0; i < width; i++) { dst[i] = 0; dst[(height-1)*width+i] = 0; }
    for (int i = 0; i < height; i++) { dst[i*width] = 0; dst[i*width+width-1] = 0; }
}

// ---- Hysteresis Thresholding ----
void hysteresis_thresholding(uint8_t *image, int width, int height, int low_threshold, int high_threshold)
{
    // Mark strong (2) and weak (1) edges, suppress (0) below low
    for (int i = 0; i < width * height; i++) {
        if (image[i] >= (uint8_t)high_threshold)
            image[i] = 2;
        else if (image[i] >= (uint8_t)low_threshold)
            image[i] = 1;
        else
            image[i] = 0;
    }
    // Trace from strong edges to connect weak edges
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            if (image[y * width + x] == 2) {
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (image[(y+dy) * width + (x+dx)] == 1)
                            image[(y+dy) * width + (x+dx)] = 2;
                    }
                }
            }
        }
    }
    // Final binary: 2 -> 255, others -> 0
    for (int i = 0; i < width * height; i++) {
        image[i] = (image[i] == 2) ? 255 : 0;
    }
}

// ---- Contour Dynamic Array ----
void init_contour(Contour *c)
{
    c->points = NULL;
    c->count = 0;
    c->capacity = 0;
}

void add_point_to_contour(Contour *c, Point p)
{
    if (c->count >= c->capacity) {
        c->capacity = (c->capacity == 0) ? 64 : c->capacity * 2;
        c->points = (Point *)realloc(c->points, c->capacity * sizeof(Point));
    }
    if (c->points != NULL) {
        c->points[c->count++] = p;
    }
}

void free_contour(Contour *c)
{
    if (c->points != NULL) {
        free(c->points);
        c->points = NULL;
    }
    c->count = 0;
    c->capacity = 0;
}

// ---- Contour Finding (8-connectivity Moore boundary tracing) ----
void find_contours(uint8_t *binary_image, int width, int height, Contour **contours_list, int *num_contours)
{
    uint8_t *visited = (uint8_t *)calloc(width * height, 1);
    int max_contours = 256;
    *contours_list = (Contour *)malloc(max_contours * sizeof(Contour));
    *num_contours = 0;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;
            if (binary_image[idx] == 0 && !visited[idx]) {
                // Start new contour
                Contour contour;
                init_contour(&contour);

                int cx = x, cy = y;
                int start_dir = 7; // Start searching from upper-left
                int first = 1;

                while (first || !(cx == x && cy == y)) {
                    first = 0;
                    visited[cy * width + cx] = 1;
                    add_point_to_contour(&contour, (Point){cx, cy});

                    int found_next = 0;
                    for (int d = 0; d < 8; d++) {
                        int dir = (start_dir + d) % 8;
                        int nx = cx + dx_8[dir];
                        int ny = cy + dy_8[dir];
                        if (nx >= 0 && nx < width && ny >= 0 && ny < height &&
                            binary_image[ny * width + nx] == 0) {
                            if (!visited[ny * width + nx] || (nx == x && ny == y && contour.count > 2)) {
                                cx = nx; cy = ny;
                                start_dir = (dir + 5) % 8; // Turn back 3 steps
                                found_next = 1;
                                break;
                            }
                        }
                    }
                    if (!found_next) break;
                    if (contour.count > width + height) break; // Safety limit
                }

                if (contour.count > 5 && *num_contours < max_contours) {
                    (*contours_list)[(*num_contours)++] = contour;
                } else {
                    free_contour(&contour);
                }
            }
        }
    }
    free(visited);
}

// ---- Contour Perimeter ----
float contour_perimeter(Contour *contour)
{
    float perimeter = 0.0f;
    for (int i = 0; i < contour->count; i++) {
        int j = (i + 1) % contour->count;
        float dx = (float)(contour->points[j].x - contour->points[i].x);
        float dy = (float)(contour->points[j].y - contour->points[i].y);
        perimeter += sqrtf(dx*dx + dy*dy);
    }
    return perimeter;
}

// ---- Contour Area (Shoelace formula) ----
float contour_area(Contour *contour)
{
    float area = 0.0f;
    for (int i = 0; i < contour->count; i++) {
        int j = (i + 1) % contour->count;
        area += (float)contour->points[i].x * (float)contour->points[j].y;
        area -= (float)contour->points[j].x * (float)contour->points[i].y;
    }
    return fabsf(area) * 0.5f;
}

// ---- Douglas-Peucker Polygon Approximation ----
Contour approximate_polygon(Contour *src_contour, float epsilon)
{
    Contour result;
    init_contour(&result);

    if (src_contour->count < 3) {
        for (int i = 0; i < src_contour->count; i++)
            add_point_to_contour(&result, src_contour->points[i]);
        return result;
    }

    // Find the point furthest from the line between start and end
    int max_idx = 0;
    float max_dist = 0.0f;
    Point p0 = src_contour->points[0];
    Point p1 = src_contour->points[src_contour->count - 1];
    float dx = (float)(p1.x - p0.x);
    float dy = (float)(p1.y - p0.y);
    float line_length_sq = dx*dx + dy*dy;

    if (line_length_sq < 1e-6f) {
        add_point_to_contour(&result, p0);
        if (src_contour->count > 1)
            add_point_to_contour(&result, p1);
        return result;
    }

    for (int i = 1; i < src_contour->count - 1; i++) {
        float dist = fabsf(dy * (float)(src_contour->points[i].x - p0.x) -
                           dx * (float)(src_contour->points[i].y - p0.y)) / sqrtf(line_length_sq);
        if (dist > max_dist) {
            max_dist = dist;
            max_idx = i;
        }
    }

    Contour left_part;
    init_contour(&left_part);
    for (int i = 0; i <= max_idx; i++)
        add_point_to_contour(&left_part, src_contour->points[i]);
    Contour left_res = approximate_polygon(&left_part, epsilon);
    free_contour(&left_part);

    Contour right_part;
    init_contour(&right_part);
    for (int i = max_idx; i < src_contour->count; i++)
        add_point_to_contour(&right_part, src_contour->points[i]);
    Contour right_res = approximate_polygon(&right_part, epsilon);
    free_contour(&right_part);

    // Merge results (skip duplicate vertex)
    for (int i = 0; i < left_res.count; i++)
        add_point_to_contour(&result, left_res.points[i]);
    for (int i = 1; i < right_res.count; i++)
        add_point_to_contour(&result, right_res.points[i]);

    free_contour(&left_res);
    free_contour(&right_res);

    return result;
}

// ---- Check if a contour is a quadrilateral ----
int is_quadrilateral(Contour *contour)
{
    float epsilon = 0.04f * contour_perimeter(contour);
    Contour approx = approximate_polygon(contour, epsilon);
    int is_quad = (approx.count == 4);
    free_contour(&approx);
    return is_quad;
}

// ---- Matrix 3x3 Operations ----
void multiply_matrix3x3(Matrix3x3 *A, Matrix3x3 *B, Matrix3x3 *C)
{
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            C->mat[i][j] = 0.0f;
            for (int k = 0; k < 3; k++) {
                C->mat[i][j] += A->mat[i][k] * B->mat[k][j];
            }
        }
    }
}

int inverse_matrix3x3(Matrix3x3 *A, Matrix3x3 *invA)
{
    float det = A->mat[0][0] * (A->mat[1][1] * A->mat[2][2] - A->mat[1][2] * A->mat[2][1])
              - A->mat[0][1] * (A->mat[1][0] * A->mat[2][2] - A->mat[1][2] * A->mat[2][0])
              + A->mat[0][2] * (A->mat[1][0] * A->mat[2][1] - A->mat[1][1] * A->mat[2][0]);

    if (fabsf(det) < 1e-6f) return 0;

    float inv_det = 1.0f / det;
    invA->mat[0][0] = (A->mat[1][1] * A->mat[2][2] - A->mat[1][2] * A->mat[2][1]) * inv_det;
    invA->mat[0][1] = (A->mat[0][2] * A->mat[2][1] - A->mat[0][1] * A->mat[2][2]) * inv_det;
    invA->mat[0][2] = (A->mat[0][1] * A->mat[1][2] - A->mat[0][2] * A->mat[1][1]) * inv_det;
    invA->mat[1][0] = (A->mat[1][2] * A->mat[2][0] - A->mat[1][0] * A->mat[2][2]) * inv_det;
    invA->mat[1][1] = (A->mat[0][0] * A->mat[2][2] - A->mat[0][2] * A->mat[2][0]) * inv_det;
    invA->mat[1][2] = (A->mat[0][2] * A->mat[1][0] - A->mat[0][0] * A->mat[1][2]) * inv_det;
    invA->mat[2][0] = (A->mat[1][0] * A->mat[2][1] - A->mat[1][1] * A->mat[2][0]) * inv_det;
    invA->mat[2][1] = (A->mat[0][1] * A->mat[2][0] - A->mat[0][0] * A->mat[2][1]) * inv_det;
    invA->mat[2][2] = (A->mat[0][0] * A->mat[1][1] - A->mat[0][1] * A->mat[1][0]) * inv_det;
    return 1;
}

// ---- DLT Perspective Transform ----
int get_perspective_transform(Point src[4], Point dst[4], Matrix3x3 *H)
{
    // Build 8x8 linear system (normalized DLT) and solve by Gaussian elimination
    float A[8][9];
    memset(A, 0, sizeof(A));

    for (int i = 0; i < 4; i++) {
        float x = (float)src[i].x;
        float y = (float)src[i].y;
        float u = (float)dst[i].x;
        float v = (float)dst[i].y;

        A[2*i][0] = x; A[2*i][1] = y; A[2*i][2] = 1.0f;
        A[2*i][3] = 0; A[2*i][4] = 0; A[2*i][5] = 0;
        A[2*i][6] = -u*x; A[2*i][7] = -u*y; A[2*i][8] = -u;

        A[2*i+1][0] = 0; A[2*i+1][1] = 0; A[2*i+1][2] = 0;
        A[2*i+1][3] = x; A[2*i+1][4] = y; A[2*i+1][5] = 1.0f;
        A[2*i+1][6] = -v*x; A[2*i+1][7] = -v*y; A[2*i+1][8] = -v;
    }

    // Gaussian elimination with partial pivoting
    for (int i = 0; i < 8; i++) {
        // Find pivot
        int max_row = i;
        float max_val = fabsf(A[i][i]);
        for (int k = i + 1; k < 8; k++) {
            if (fabsf(A[k][i]) > max_val) {
                max_val = fabsf(A[k][i]);
                max_row = k;
            }
        }

        if (max_val < 1e-6f) return 0;

        // Swap rows
        if (max_row != i) {
            for (int j = i; j <= 8; j++) {
                float tmp = A[i][j];
                A[i][j] = A[max_row][j];
                A[max_row][j] = tmp;
            }
        }

        // Eliminate
        for (int k = i + 1; k < 8; k++) {
            float factor = A[k][i] / A[i][i];
            for (int j = i; j <= 8; j++) {
                A[k][j] -= factor * A[i][j];
            }
        }
    }

    // Back substitution
    float h[9];
    for (int i = 7; i >= 0; i--) {
        h[i] = A[i][8];
        for (int j = i + 1; j < 8; j++) {
            h[i] -= A[i][j] * h[j];
        }
        if (fabsf(A[i][i]) < 1e-6f) return 0;
        h[i] /= A[i][i];
    }
    h[8] = 1.0f;

    // Fill Matrix3x3
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            H->mat[i][j] = h[i * 3 + j];

    return 1;
}

// ---- Perspective Transform (Image Warping) ----
void perspective_transform(uint8_t *src_image, uint8_t *dst_image, int src_width, int src_height,
                           Quad paper_quad, int dst_width, int dst_height)
{
    Point src_pts[4];
    for (int i = 0; i < 4; i++) src_pts[i] = paper_quad.corners[i];

    Point dst_pts[4] = {{0, 0}, {dst_width - 1, 0}, {dst_width - 1, dst_height - 1}, {0, dst_height - 1}};

    Matrix3x3 H, H_inv;
    if (!get_perspective_transform(src_pts, dst_pts, &H)) return;
    if (!inverse_matrix3x3(&H, &H_inv)) return;

    for (int dy = 0; dy < dst_height; dy++) {
        for (int dx = 0; dx < dst_width; dx++) {
            float sx = H_inv.mat[0][0] * (float)dx + H_inv.mat[0][1] * (float)dy + H_inv.mat[0][2];
            float sy = H_inv.mat[1][0] * (float)dx + H_inv.mat[1][1] * (float)dy + H_inv.mat[1][2];
            float w  = H_inv.mat[2][0] * (float)dx + H_inv.mat[2][1] * (float)dy + H_inv.mat[2][2];

            if (fabsf(w) < 1e-6f) {
                dst_image[dy * dst_width + dx] = 0;
                continue;
            }

            int ix = (int)roundf(sx / w);
            int iy = (int)roundf(sy / w);

            if (ix >= 0 && ix < src_width && iy >= 0 && iy < src_height)
                dst_image[dy * dst_width + dx] = src_image[iy * src_width + ix];
            else
                dst_image[dy * dst_width + dx] = 0;
        }
    }
}

// ---- Sort Quadrilateral Corners (top-left, top-right, bottom-right, bottom-left) ----
static void sort_quad_corners(Point corners[4])
{
    // Find centroid
    float cx = 0, cy = 0;
    for (int i = 0; i < 4; i++) { cx += (float)corners[i].x; cy += (float)corners[i].y; }
    cx /= 4.0f; cy /= 4.0f;

    // Classify: above centroid + left/right -> TL/TR, below + left/right -> BL/BR
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

// ---- Find Largest Quadrilateral (A4 paper candidate) ----
Quad find_largest_quad(uint8_t *binary_image, int width, int height, float *paper_width_px)
{
    Quad result = {{{0, 0}, {0, 0}, {0, 0}, {0, 0}}};
    *paper_width_px = 0.0f;

    Contour *contours_list = NULL;
    int num_contours = 0;
    find_contours(binary_image, width, height, &contours_list, &num_contours);

    float max_area = 0.0f;
    int best_idx = -1;
    Contour best_approx;
    init_contour(&best_approx);

    for (int i = 0; i < num_contours; i++) {
        if (contour_area(&contours_list[i]) < (float)MIN_CONTOUR_AREA) continue;
        float epsilon = 0.04f * contour_perimeter(&contours_list[i]);
        Contour approx = approximate_polygon(&contours_list[i], epsilon);
        if (approx.count == 4) {
            float area = contour_area(&approx);
            if (area > max_area) {
                max_area = area;
                if (best_approx.points != NULL) free_contour(&best_approx);
                best_approx = approx;
                best_idx = i;
                continue;
            }
        }
        free_contour(&approx);
    }

    if (best_idx >= 0) {
        sort_quad_corners(best_approx.points);
        for (int i = 0; i < 4; i++)
            result.corners[i] = best_approx.points[i];

        // Calculate paper width in pixels (average of top and bottom edges)
        float top_width = sqrtf(powf((float)(result.corners[1].x - result.corners[0].x), 2) +
                                powf((float)(result.corners[1].y - result.corners[0].y), 2));
        float bottom_width = sqrtf(powf((float)(result.corners[2].x - result.corners[3].x), 2) +
                                   powf((float)(result.corners[2].y - result.corners[3].y), 2));
        *paper_width_px = (top_width + bottom_width) * 0.5f;
        free_contour(&best_approx);
    }

    for (int i = 0; i < num_contours; i++) free_contour(&contours_list[i]);
    if (contours_list != NULL) free(contours_list);

    return result;
}
