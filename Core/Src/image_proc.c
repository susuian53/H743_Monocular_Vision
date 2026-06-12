#include "image_proc.h"
#include "config.h"
#include <string.h>
#include <stdlib.h>

// 3x3 Gaussian blur with a temporary buffer
void gaussian_blur(uint8_t *src, uint8_t *dst, int width, int height) {
    // Gaussian kernel for 3x3
    const int kernel[3][3] = {{1, 2, 1},
                              {2, 4, 2},
                              {1, 2, 1}};
    const int kernel_sum = 16;

    // Create a temporary buffer to store intermediate results
    // This is necessary if src and dst are the same buffer for in-place blur.
    // If src != dst, this temporary buffer is still safe.
    uint8_t *temp_buffer = (uint8_t *)malloc(width * height);
    if (temp_buffer == NULL) {
        // Handle memory allocation error
        return;
    }

    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int sum_pixels = 0;
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    sum_pixels += src[(y + ky) * width + (x + kx)] * kernel[ky + 1][kx + 1];
                }
            }
            temp_buffer[y * width + x] = sum_pixels / kernel_sum;
        }
    }

    // Copy the blurred image from the temporary buffer to the destination buffer
    // Handle borders: for simplicity, just copy the unblurred borders
    for (int y = 0; y < height; y++) {
        if (y == 0 || y == height - 1) {
            memcpy(dst + y * width, src + y * width, width);
        } else {
            memcpy(dst + y * width, src + y * width, 1); // Left border
            memcpy(dst + y * width + width - 1, src + y * width + width - 1, 1); // Right border
            memcpy(dst + y * width + 1, temp_buffer + y * width + 1, width - 2);
        }
    }

    free(temp_buffer);
}

int otsu_threshold(uint8_t *image, int width, int height) {
    int hist[256] = {0};
    long total_pixels = width * height;

    // Calculate histogram
    for (int i = 0; i < total_pixels; i++) {
        hist[image[i]]++;
    }

    float sum = 0;
    for (int i = 0; i < 256; i++) {
        sum += i * hist[i];
    }

    float sumB = 0;
    long wB = 0;
    long wF = 0;

    float max_variance = 0;
    int threshold = 0;

    for (int i = 0; i < 256; i++) {
        wB += hist[i];
        if (wB == 0) continue;

        wF = total_pixels - wB;
        if (wF == 0) break;

        sumB += (float)(i * hist[i]);

        float mB = sumB / wB;
        float mF = (sum - sumB) / wF;

        float varBetween = (float)wB * (float)wF * (mB - mF) * (mB - mF);

        if (varBetween > max_variance) {
            max_variance = varBetween;
            threshold = i;
        }
    }
    return threshold;
}

void binarize(uint8_t *src, uint8_t *dst, int width, int height, int threshold) {
    long total_pixels = width * height;
    for (int i = 0; i < total_pixels; i++) {
        dst[i] = (src[i] > threshold) ? 255 : 0;
    }
}

void adaptive_threshold(uint8_t *src, uint8_t *dst, 
                       int width, int height, int blockSize, int C)
{
    // Step 1: Calculate integral image
    uint32_t *integral = (uint32_t *)malloc((width+1)*(height+1)*sizeof(uint32_t));
    if (integral == NULL) return; // Handle memory allocation error

    memset(integral, 0, (width+1)*(height+1)*sizeof(uint32_t)); // Initialize to zero

    for(int y=0; y<height; y++) {
        for(int x=0; x<width; x++) {
            integral[(y+1)*(width+1)+(x+1)] = src[y*width+x]
                + integral[(y+1)*(width+1)+x]
                + integral[y*(width+1)+(x+1)]
                - integral[y*(width+1)+x];
        }
    }

    // Step 2: Use integral image to quickly calculate local mean
    int half = blockSize / 2;
    for(int y=0; y<height; y++) {
        for(int x=0; x<width; x++) {
            int x1 = (x-half < 0) ? 0 : (x-half);
            int x2 = (x+half >= width) ? width-1 : (x+half);
            int y1 = (y-half < 0) ? 0 : (y-half);
            int y2 = (y+half >= height) ? height-1 : (y+half);

            int count = (x2-x1+1) * (y2-y1+1);
            uint32_t sum = integral[(y2+1)*(width+1)+(x2+1)]
                         - integral[(y2+1)*(width+1)+x1]
                         - integral[y1*(width+1)+(x2+1)]
                         + integral[y1*(width+1)+x1];
            uint8_t mean = (uint8_t)(sum / count);
            dst[y*width+x] = (src[y*width+x] < (mean - C)) ? 0 : 255;
        }
    }
    free(integral);
}
