/*
 * dht20.h - Driver cảm biến nhiệt độ & độ ẩm DHT20
 */
#ifndef __DHT20_H
#define __DHT20_H

#include "main.h"

// Địa chỉ I2C DHT20 7-bit (0x38) dịch trái 1 bit thành 0x70
#define DHT20_ADDR  (0x38 << 1)

uint8_t DHT20_Read(float *temp, float *hum);
void DHT20_InitState(void);
int DHT20_Tick(float *temp, float *hum);

#endif /* __DHT20_H */
