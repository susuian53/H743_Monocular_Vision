#include "display.h"
#include "lcd_169_drv.h"
#include "config.h"
#include "shape_detect.h"
#include <stdio.h>

// Display measurement results on the LCD screen
void display_results(ShapeType shape, float distance, float size) {
    char buffer[50];
    LCD_Clear();

    LCD_DisplayString(10, 10, "========Result========");

    const char* shape_str = "Unknown";
    if (shape == SHAPE_CIRCLE) shape_str = "Circle";
    else if (shape == SHAPE_TRIANGLE) shape_str = "Triangle";
    else if (shape == SHAPE_SQUARE) shape_str = "Square";
    sprintf(buffer, "Type:   %s", shape_str);
    LCD_DisplayString(10, 40, buffer);

    sprintf(buffer, "Dist D: %.1f cm", distance / 10.0f);
    LCD_DisplayString(10, 70, buffer);

    sprintf(buffer, "Size x: %.1f cm", size / 10.0f);
    LCD_DisplayString(10, 100, buffer);

    LCD_DisplayString(10, 130, "======================");
}

// Display a preview of the camera image (Y8 grayscale)
// Downsample from camera resolution to fit the LCD
void display_preview(uint8_t *image, int width, int height) {
    // Downsample: compute step sizes to fit 240x280 LCD
    int skip_x = width / LCD_Width;
    int skip_y = height / LCD_Height;
    if (skip_x < 1) skip_x = 1;
    if (skip_y < 1) skip_y = 1;

    uint16_t line_buf[240]; // 240 pixels * 16-bit = 480 bytes

    for (int y = 0; y < LCD_Height; y++) {
        int cam_y = y * skip_y;
        for (int x = 0; x < LCD_Width; x++) {
            int cam_x = x * skip_x;
            uint8_t gray = image[cam_y * width + cam_x];
            // Convert Y8 to RGB565
            uint8_t r = gray >> 3;
            uint8_t g = gray >> 2;
            uint8_t b = gray >> 3;
            line_buf[x] = ((uint16_t)r << 11) | ((uint16_t)g << 5) | (uint16_t)b;
        }
        LCD_CopyBuffer(0, y, LCD_Width, 1, line_buf);
    }
}
