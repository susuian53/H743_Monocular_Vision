#include "mono_solver.h"
#include "config.h"

// Placeholder for real size calculation
float calculate_real_size(float pixel_size, float paper_width_px) {
    if (paper_width_px == 0) return 0;
    return pixel_size * A4_WIDTH_MM / paper_width_px;
}

// Calculate distance using linear interpolation on the dist_cal_table
float calculate_distance(float paper_width_px) {
    if (paper_width_px == 0) return 0;

    // Find the two closest points in the lookup table for interpolation
    int idx1 = -1, idx2 = -1;
    for (int i = 0; i < DIST_CAL_TABLE_SIZE; i++) {
        if (dist_cal_table[i].width_px <= paper_width_px) {
            idx1 = i;
        }
        if (dist_cal_table[i].width_px >= paper_width_px) {
            idx2 = i;
            break;
        }
    }

    // Handle edge cases (paper_width_px outside table range)
    if (idx1 == -1) { // Smaller than smallest calibrated width
        // Extrapolate or return closest value
        return (float)dist_cal_table[0].dist_mm;
    }
    if (idx2 == -1) { // Larger than largest calibrated width
        // Extrapolate or return closest value
        return (float)dist_cal_table[DIST_CAL_TABLE_SIZE - 1].dist_mm;
    }
    if (idx1 == idx2) { // Exact match or only one point found
        return (float)dist_cal_table[idx1].dist_mm;
    }

    // Linear interpolation
    float x1 = (float)dist_cal_table[idx1].width_px;
    float y1 = (float)dist_cal_table[idx1].dist_mm;
    float x2 = (float)dist_cal_table[idx2].width_px;
    float y2 = (float)dist_cal_table[idx2].dist_mm;

    // y = y1 + (y2 - y1) * ((x - x1) / (x2 - x1))
    float interpolated_dist = y1 + (y2 - y1) * ((paper_width_px - x1) / (x2 - x1));
    return interpolated_dist;
}

const DistCalEntry dist_cal_table[] = {
    {1000, 0},  // To be filled during calibration
    {1200, 0},
    {1400, 0},
    {1600, 0},
    {1800, 0},
    {2000, 0},
};
const int DIST_CAL_TABLE_SIZE = sizeof(dist_cal_table) / sizeof(DistCalEntry);
