#ifndef __LCD_H
#define __LCD_H

#include "main.h"

// Địa chỉ I2C LCD 7-bit (0x21) dịch trái 1 bit thành 0x42
#define LCD_ADDR   (0x21 << 1)

void lcd_init(void);
HAL_StatusTypeDef lcd_send_cmd(uint8_t cmd);
HAL_StatusTypeDef lcd_send_data(uint8_t data);
HAL_StatusTypeDef lcd_goto_XY(int row, int col);
HAL_StatusTypeDef lcd_send_string(const char *str);
HAL_StatusTypeDef lcd_write_line(uint8_t row, const char *str);

#endif /* __LCD_H */
