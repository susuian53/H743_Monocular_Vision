#ifndef __CONFIG_H
#define __CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

// ===== 相机参数 =====
#define IMG_WIDTH               640
#define IMG_HEIGHT              480
#define CAMERA_BUFFER_ADDR      0x24000000UL
#define BINARY_BUFFER_ADDR      CAMERA_BUFFER_ADDR  // Reuse camera buffer for binary (sequential pipeline)

// ===== A4纸参考尺寸 =====
#define A4_WIDTH_MM             210.0f
#define A4_HEIGHT_MM            297.0f

// ===== 距离D标定查找表 =====
typedef struct {
    uint16_t dist_mm;
    uint16_t width_px;
} DistCalEntry;

extern const DistCalEntry dist_cal_table[];
extern const int DIST_CAL_TABLE_SIZE;

// ===== 图像处理阈值 =====
#define CANNY_LOW_THRESHOLD     50
#define CANNY_HIGH_THRESHOLD    150
#define GAUSS_KERNEL_SIZE       3
#define MIN_CONTOUR_AREA        500

// ===== 二值化策略参数 =====
#define OTSU_BLACK_RATIO_MIN    0.10f
#define OTSU_BLACK_RATIO_MAX    0.90f
#define ADAPTIVE_BLOCK_SIZE     41
#define ADAPTIVE_CONSTANT