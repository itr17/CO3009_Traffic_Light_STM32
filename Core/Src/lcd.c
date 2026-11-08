#include "lcd.h"
#include <stddef.h>

/* Ngoại vi I2C1 được khởi tạo sẵn trong main.c */
extern I2C_HandleTypeDef hi2c1;

#define LCD_I2C_TIMEOUT_MS 8U
#define LCD_LINE_LENGTH 16U
#define LCD_EXPANDER_BYTES_PER_CHAR 4U
#define LCD_LINE_PACKET_SIZE (LCD_EXPANDER_BYTES_PER_CHAR * (LCD_LINE_LENGTH + 1U))

static void lcd_encode_byte(uint8_t value, uint8_t rs, uint8_t *output)
{
    uint8_t data_u = value & 0xF0U;
    uint8_t data_l = (uint8_t)(value << 4) & 0xF0U;
    uint8_t control = rs ? 0x0DU : 0x0CU;

    output[0] = data_u | control;
    output[1] = data_u | (control & 0x0BU);
    output[2] = data_l | control;
    output[3] = data_l | (control & 0x0BU);
}

/* Hàm gửi lệnh (Command) tới LCD qua I2C */
HAL_StatusTypeDef lcd_send_cmd(uint8_t cmd) {
    uint8_t data_t[4];
    lcd_encode_byte(cmd, 0U, data_t);

    return HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, data_t, sizeof(data_t),
                                   LCD_I2C_TIMEOUT_MS);
}

/* Hàm gửi dữ liệu/ký tự (Data) tới LCD qua I2C */
HAL_StatusTypeDef lcd_send_data(uint8_t data) {
    uint8_t data_t[4];
    lcd_encode_byte(data, 1U, data_t);

    return HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, data_t, sizeof(data_t),
                                   LCD_I2C_TIMEOUT_MS);
}

/* Hàm xóa nội dung màn hình */
void lcd_clear(void) {
    if (lcd_send_cmd(0x01) != HAL_OK) {
        return;
    }
    HAL_Delay(2);
}

/* Hàm đặt vị trí con trỏ (row: 0-1, col: 0-15) */
HAL_StatusTypeDef lcd_goto_XY(int row, int col) {
    uint8_t address;

    if (col < 0 || col >= (int)LCD_LINE_LENGTH) {
        return HAL_ERROR;
    }

    switch (row) {
        case 0:
            address = (uint8_t)(0x80U | (uint8_t)col);
            break;
        case 1:
            address = (uint8_t)(0xC0U | (uint8_t)col);
            break;
        default:
            return HAL_ERROR;
    }
    return lcd_send_cmd(address);
}

/* Hàm khởi tạo màn hình LCD chế độ 4-bit */
void lcd_init(void) {
    HAL_Delay(50);
    lcd_send_cmd(0x30);
    HAL_Delay(5);
    lcd_send_cmd(0x30);
    HAL_Delay(1);
    lcd_send_cmd(0x30);
    HAL_Delay(10);
    lcd_send_cmd(0x20); // Chuyển sang 4-bit mode
    HAL_Delay(10);

    // Cấu hình hiển thị LCD
    lcd_send_cmd(0x28); // Chế độ 4-bit, 2 hàng, font 5x8
    HAL_Delay(1);
    lcd_send_cmd(0x08); // Tắt hiển thị
    HAL_Delay(1);
    lcd_send_cmd(0x01); // Xóa màn hình
    HAL_Delay(2);
    lcd_send_cmd(0x06); // Tự động tăng con trỏ
    HAL_Delay(1);
    lcd_send_cmd(0x0C); // Bật hiển thị, tắt con trỏ
}

/* Hàm in một chuỗi ký tự */
HAL_StatusTypeDef lcd_send_string(const char *str) {
    while (*str) {
        HAL_StatusTypeDef status = lcd_send_data((uint8_t)*str++);
        if (status != HAL_OK) {
            return status;
        }
    }
    return HAL_OK;
}

HAL_StatusTypeDef lcd_write_line(uint8_t row, const char *str)
{
    uint8_t packet[LCD_LINE_PACKET_SIZE];
    uint8_t cursor_command;

    if (str == NULL || row > 1U) {
        return HAL_ERROR;
    }

    cursor_command = (row == 0U) ? 0x80U : 0xC0U;
    lcd_encode_byte(cursor_command, 0U, packet);

    for (uint8_t i = 0U; i < LCD_LINE_LENGTH; i++) {
        uint8_t character = (*str != '\0') ? (uint8_t)*str++ : (uint8_t)' ';
        lcd_encode_byte(character, 1U,
                        &packet[LCD_EXPANDER_BYTES_PER_CHAR * (i + 1U)]);
    }

    return HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, packet, sizeof(packet),
                                   LCD_I2C_TIMEOUT_MS);
}
