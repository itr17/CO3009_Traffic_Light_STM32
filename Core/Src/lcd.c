#include "lcd.h"
#include <stddef.h>

/* Ngoại vi I2C1 được khởi tạo sẵn trong main.c */
extern I2C_HandleTypeDef hi2c1;

#define LCD_I2C_TIMEOUT_MS 8U
#define LCD_LINE_LENGTH 16U
#define LCD_EXPANDER_BYTES_PER_CHAR 4U
#define LCD_LINE_PACKET_SIZE (LCD_EXPANDER_BYTES_PER_CHAR * (LCD_LINE_LENGTH + 1U))
#define LCD_INIT_COMMAND_COUNT 9U
#define LCD_CLEAR_COMMAND 0x01U

static const uint8_t lcd_init_commands[LCD_INIT_COMMAND_COUNT] = {
    0x30U, 0x30U, 0x30U, 0x20U, 0x28U, 0x08U, LCD_CLEAR_COMMAND, 0x06U, 0x0CU
};
static const uint32_t lcd_init_delays[LCD_INIT_COMMAND_COUNT] = {
    5U, 1U, 10U, 10U, 1U, 1U, 2U, 1U, 0U
};
static uint8_t lcd_init_step;
static uint8_t lcd_init_state;
static uint32_t lcd_init_deadline;

enum {
    LCD_INIT_NOT_STARTED = 0U,
    LCD_INIT_RUNNING,
    LCD_INIT_READY,
    LCD_INIT_ERROR
};

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

HAL_StatusTypeDef lcd_clear(void) {
    return lcd_send_cmd(LCD_CLEAR_COMMAND);
}

/* Hàm gửi dữ liệu/ký tự (Data) tới LCD qua I2C */
HAL_StatusTypeDef lcd_send_data(uint8_t data) {
    uint8_t data_t[4];
    lcd_encode_byte(data, 1U, data_t);

    return HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, data_t, sizeof(data_t),
                                   LCD_I2C_TIMEOUT_MS);
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

/* Bắt đầu khởi tạo LCD; các bước được thực hiện trong lcd_init_tick(). */
void lcd_init(void) {
    lcd_init_step = 0U;
    lcd_init_state = LCD_INIT_RUNNING;
    lcd_init_deadline = HAL_GetTick() + 50U;
}

HAL_StatusTypeDef lcd_init_tick(void) {
    if (lcd_init_state == LCD_INIT_READY) {
        return HAL_OK;
    }
    if (lcd_init_state == LCD_INIT_ERROR ||
        lcd_init_state == LCD_INIT_NOT_STARTED) {
        return HAL_ERROR;
    }

    uint32_t now = HAL_GetTick();
    if ((int32_t)(now - lcd_init_deadline) < 0) {
        return HAL_BUSY;
    }

    uint8_t command = lcd_init_commands[lcd_init_step];
    HAL_StatusTypeDef status = (command == LCD_CLEAR_COMMAND)
                                   ? lcd_clear()
                                   : lcd_send_cmd(command);
    if (status != HAL_OK) {
        lcd_init_state = LCD_INIT_ERROR;
        return status;
    }

    lcd_init_step++;
    if (lcd_init_step == LCD_INIT_COMMAND_COUNT) {
        lcd_init_state = LCD_INIT_READY;
        return HAL_OK;
    }

    lcd_init_deadline = now + lcd_init_delays[lcd_init_step - 1U];
    return HAL_BUSY;
}

uint8_t lcd_is_ready(void) {
    return (uint8_t)(lcd_init_state == LCD_INIT_READY);
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
