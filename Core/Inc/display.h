#ifndef DISPLAY_H
#define DISPLAY_H

#include "stm32h7xx_hal.h"
#include "shape_detect.h"

// Forward declarations from lcd_169_drv.h (to avoid circular includes)
void LCD_Clear(void);
void LCD_DisplayString(uint16_t x, uint16_t y, char *p);
void LCD_CopyBuffer(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *DataBuff);
#define LCD_Width   240
#define LCD_Height  280

void display_results(ShapeType shape, float distance, float size);
void display_preview(uint8_t *image, int width, int height);

#endif /* DISPLAY_H */
