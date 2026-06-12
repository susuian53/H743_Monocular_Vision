#include "tilt_correct.h"
#include "config.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

// Extern declarations for functions from paper_detect.c
extern int get_perspective_transform(Point src[4], Point dst[4], Matrix3x3 *H);
extern int inverse_matrix3x3(Matrix3x3 *A, Matrix3x3 *invA);

void correct_perspective(Point *object_corners, int num_corners, Quad paper_quad, float *corrected_side_length)
{
    Matrix3x3 H_src_to_dst;
    Point paper_src_pts[4];
    Point paper_dst_pts[4];

    *corrected_side_length = 0.0f;

    // 1. Define source points (detected A4 paper corners)
    for (int i = 0; i < 4; i++) {
        paper_src_pts[i] = paper_quad.corners[i];
    }

    // 2. Define destination points (ideal rectangular A4 paper dimensions)
    int target_paper_width = 400;
    int target_paper_height = (int)(target_paper_width * (A4_HEIGHT_MM / A4_WIDTH_MM));

    paper_dst_pts[0] = (Point){0, 0};
    paper_dst_pts[1] = (Point){target_paper_width - 1, 0};
    paper_dst_pts[2] = (Point){target_paper_width - 1, target_paper_height - 1};
    paper_dst_pts[3] = (Point){0, target_paper_height - 1};

    // 3. Calculate homography matrix H_src_to_dst
    if (!get_perspective_transform(paper_src_pts, paper_dst_pts, &H_src_to_dst)) {
        return;
    }

    // 4. Transform the object corners using H_src_to_dst
    Point transformed_object_corners[num_corners];
    for (int i = 0; i < num_corners; i++) {
        float x = (float)object_corners[i].x;
        float y = (float)object_corners[i].y;

        float transformed_x = (H_src_to_dst.mat[0][0] * x + H_src_to_dst.mat[0][1] * y + H_src_to_dst.mat[0][2]);
        float transformed_y = (H_src_to_dst.mat[1][0] * x + H_src_to_dst.mat[1][1] * y + H_src_to_dst.mat[1][2]);
        float w = (H_src_to_dst.mat[2][0] * x + H_src_to_dst.mat[2][1] * y + H_src_to_dst.mat[2][2]);

        if (fabsf(w) < 1e-6f) {
            transformed_object_corners[i] = (Point){0, 0};
        } else {
            transformed_object_corners[i] = (Point){(int)roundf(transformed_x / w), (int)roundf(transformed_y / w)};
        }
    }

    // 5. Calculate corrected side length from transformed corners
    if (num_corners >= 2) {
        float dx = (float)(transformed_object_corners[0].x - transformed_object_corners[1].x);
        float dy = (float)(transformed_object_corners[0].y - transformed_object_corners[1].y);
        *corrected_side_length = sqrtf(dx*dx + dy*dy);
    }
}
