/*
 * traffic_light.h - Driver điều khiển cụm đèn giao thông (Tuyến 1 & Tuyến 2)
 */
#ifndef __TRAFFIC_LIGHT_H
#define __TRAFFIC_LIGHT_H

#include "main.h"

// Hàm xuất tín hiệu mức logic ra các chân GPIO điều khiển LED
void led_set_traffic(uint8_t r1, uint8_t a1, uint8_t g1, uint8_t r2, uint8_t a2, uint8_t g2);

// Hàm điều phối hiển thị đèn tập trung cho toàn bộ hệ thống
void traffic_light_render(void);

#endif /* __TRAFFIC_LIGHT_H */
