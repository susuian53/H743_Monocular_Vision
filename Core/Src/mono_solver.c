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
    