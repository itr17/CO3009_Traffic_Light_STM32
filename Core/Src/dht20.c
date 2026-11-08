#include "dht20.h"
#include "software_timer.h"

extern I2C_HandleTypeDef hi2c1;

#define DHT20_CONVERSION_TICKS 8U
#define DHT20_SAMPLE_INTERVAL_TICKS 200U
#define DHT20_I2C_TIMEOUT_MS 5U
#define DHT20_CONVERSION_TIMER 8
#define DHT20_SAMPLE_TIMER 9

typedef enum {
    DHT20_STATE_IDLE = 0,
    DHT20_STATE_WAIT_CONVERSION,
    DHT20_STATE_READ_RESULT,
    DHT20_STATE_WAIT_INTERVAL,
    DHT20_STATE_ERROR
} DHT20_State;

static DHT20_State dht20_state = DHT20_STATE_IDLE;
static uint8_t dht20_cmd[3] = {0xAC, 0x33, 0x00};
static uint8_t dht20_read_buf[7] = {0};

static uint8_t DHT20_CalculateCRC(const uint8_t *data, uint8_t length)
{
    uint8_t crc = 0xFFU;

    for (uint8_t i = 0U; i < length; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0U; bit < 8U; bit++) {
            crc = (crc & 0x80U) ? (uint8_t)((crc << 1) ^ 0x31U)
                                : (uint8_t)(crc << 1);
        }
    }

    return crc;
}

void DHT20_InitState(void)
{
    dht20_state = DHT20_STATE_IDLE;
    setTimer(DHT20_CONVERSION_TIMER, 0);
    setTimer(DHT20_SAMPLE_TIMER, 0);
}

int DHT20_Tick(float *temp, float *hum)
{
    switch (dht20_state) {
        case DHT20_STATE_IDLE:
            if (HAL_I2C_Master_Transmit(&hi2c1, DHT20_ADDR, dht20_cmd, sizeof(dht20_cmd), DHT20_I2C_TIMEOUT_MS) != HAL_OK) {
                dht20_state = DHT20_STATE_ERROR;
                return 2;
            }
            setTimer(DHT20_CONVERSION_TIMER, DHT20_CONVERSION_TICKS);
            setTimer(DHT20_SAMPLE_TIMER, DHT20_SAMPLE_INTERVAL_TICKS);
            dht20_state = DHT20_STATE_WAIT_CONVERSION;
            return 0;

        case DHT20_STATE_WAIT_CONVERSION:
            if (isTimerExpired(DHT20_CONVERSION_TIMER)) {
                dht20_state = DHT20_STATE_READ_RESULT;
            }
            return 0;

        case DHT20_STATE_READ_RESULT:
            if (HAL_I2C_Master_Receive(&hi2c1, DHT20_ADDR, dht20_read_buf, sizeof(dht20_read_buf), DHT20_I2C_TIMEOUT_MS) != HAL_OK) {
                dht20_state = DHT20_STATE_ERROR;
                return 2;
            }

            if ((dht20_read_buf[0] & 0x80U) != 0U) {
                dht20_state = DHT20_STATE_ERROR;
                return 2;
            }

            if (DHT20_CalculateCRC(dht20_read_buf, 6U) != dht20_read_buf[6]) {
                dht20_state = DHT20_STATE_ERROR;
                return 2;
            }

            {
                uint32_t raw_hum = ((uint32_t)dht20_read_buf[1] << 12)
                                 | ((uint32_t)dht20_read_buf[2] << 4)
                                 | ((uint32_t)dht20_read_buf[3] >> 4);
                uint32_t raw_temp = (((uint32_t)dht20_read_buf[3] & 0x0FU) << 16)
                                  | ((uint32_t)dht20_read_buf[4] << 8)
                                  | (uint32_t)dht20_read_buf[5];

                *hum = ((float)raw_hum / 1048576.0f) * 100.0f;
                *temp = ((float)raw_temp / 1048576.0f) * 200.0f - 50.0f;
            }

            dht20_state = DHT20_STATE_WAIT_INTERVAL;
            return 1;

        case DHT20_STATE_WAIT_INTERVAL:
            if (isTimerExpired(DHT20_SAMPLE_TIMER)) {
                dht20_state = DHT20_STATE_IDLE;
            }
            return 0;

        case DHT20_STATE_ERROR:
            setTimer(DHT20_SAMPLE_TIMER, DHT20_SAMPLE_INTERVAL_TICKS);
            dht20_state = DHT20_STATE_WAIT_INTERVAL;
            return 2;

        default:
            dht20_state = DHT20_STATE_ERROR;
            return 2;
    }
}

uint8_t DHT20_Read(float *temp, float *hum) {
    uint8_t send_buf[3] = {0xAC, 0x33, 0x00};
    uint8_t read_buf[7] = {0};

    if (HAL_I2C_Master_Transmit(&hi2c1, DHT20_ADDR, send_buf, sizeof(send_buf), DHT20_I2C_TIMEOUT_MS) != HAL_OK) {
        return 0;
    }

    HAL_Delay(80);

    if (HAL_I2C_Master_Receive(&hi2c1, DHT20_ADDR, read_buf, sizeof(read_buf), DHT20_I2C_TIMEOUT_MS) != HAL_OK) {
        return 0;
    }

    if ((read_buf[0] & 0x80U) != 0U) {
        return 0;
    }

    if (DHT20_CalculateCRC(read_buf, 6U) != read_buf[6]) {
        return 0;
    }

    uint32_t raw_hum = ((uint32_t)read_buf[1] << 12) | ((uint32_t)read_buf[2] << 4) | ((uint32_t)read_buf[3] >> 4);
    *hum = ((float)raw_hum / 1048576.0f) * 100.0f;

    uint32_t raw_temp = (((uint32_t)read_buf[3] & 0x0F) << 16) | ((uint32_t)read_buf[4] << 8) | (uint32_t)read_buf[5];
    *temp = ((float)raw_temp / 1048576.0f) * 200.0f - 50.0f;

    return 1;
}
