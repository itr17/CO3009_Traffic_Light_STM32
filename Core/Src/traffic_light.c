/*
 * traffic_light.c - Driver điều khiển cụm đèn giao thông (Tuyến 1 & Tuyến 2)
 */
#include "traffic_light.h"
#include "global.h"
#include "software_timer.h"

#define BLINK_TIMER 5
#define BLINK_HALF_PERIOD_TICKS (250U / BUTTON_SCAN_MS) // 250ms (Nhấp nháy tần số 2Hz)

static uint8_t blink_on = 1U;
static int prev_app_mode = -1;

/* ========================================================================== */
/*               HÀM ĐIỀU KHIỂN CHÂN GPIO XUẤT ĐÈN THỰC TẾ                    */
/* ========================================================================== */
void led_set_traffic(uint8_t r1, uint8_t a1, uint8_t g1, uint8_t r2, uint8_t a2, uint8_t g2) {
    // Tuyến 1 (Traffic Light 1)
    #if defined(TL1_A_Pin) && defined(TL1_B_Pin)
        if (r1) {
            HAL_GPIO_WritePin(TL1_A_GPIO_Port, TL1_A_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(TL1_B_GPIO_Port, TL1_B_Pin, GPIO_PIN_SET);
        } else if (a1) {
            HAL_GPIO_WritePin(TL1_A_GPIO_Port, TL1_A_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(TL1_B_GPIO_Port, TL1_B_Pin, GPIO_PIN_SET);
        } else if (g1) {
            HAL_GPIO_WritePin(TL1_A_GPIO_Port, TL1_A_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(TL1_B_GPIO_Port, TL1_B_Pin, GPIO_PIN_RESET);
        } else {
            HAL_GPIO_WritePin(TL1_A_GPIO_Port, TL1_A_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(TL1_B_GPIO_Port, TL1_B_Pin, GPIO_PIN_RESET);
        }
    #else
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, r1 ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, a1 ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, g1 ? GPIO_PIN_SET : GPIO_PIN_RESET);
    #endif

    // Tuyến 2 (Traffic Light 2)
    #if defined(TL2_A_Pin) && defined(TL2_B_Pin)
        if (r2) {
            HAL_GPIO_WritePin(TL2_A_GPIO_Port, TL2_A_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(TL2_B_GPIO_Port, TL2_B_Pin, GPIO_PIN_SET);
        } else if (a2) {
            HAL_GPIO_WritePin(TL2_A_GPIO_Port, TL2_A_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(TL2_B_GPIO_Port, TL2_B_Pin, GPIO_PIN_SET);
        } else if (g2) {
            HAL_GPIO_WritePin(TL2_A_GPIO_Port, TL2_A_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(TL2_B_GPIO_Port, TL2_B_Pin, GPIO_PIN_RESET);
        } else {
            HAL_GPIO_WritePin(TL2_A_GPIO_Port, TL2_A_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(TL2_B_GPIO_Port, TL2_B_Pin, GPIO_PIN_RESET);
        }
    #else
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, r2 ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, a2 ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOH, GPIO_PIN_1,  g2 ? GPIO_PIN_SET : GPIO_PIN_RESET);
    #endif
}

/* ========================================================================== */
/*               HÀM ĐIỀU PHỐI HIỂN THỊ ĐÈN THEO 3 CHẾ ĐỘ                     */
/* ========================================================================== */
static void update_blink_status(void) {
    // Phát hiện đổi app_mode -> Bật sáng ngay lập tức và đặt lại timer 250ms
    if (prev_app_mode != app_mode) {
        prev_app_mode = app_mode;
        blink_on = 1U;
        setTimer(BLINK_TIMER, (int)BLINK_HALF_PERIOD_TICKS);
    }
    // Khi hết 250ms -> Đảo trạng thái nhấp nháy
    else if (isTimerExpired(BLINK_TIMER)) {
        blink_on = !blink_on;
        setTimer(BLINK_TIMER, (int)BLINK_HALF_PERIOD_TICKS);
    }
}

void traffic_light_render(void)
{
    update_blink_status();

    switch (app_mode) {
        /* 1. MODE_NORMAL: Sáng liên tục theo trạng thái pha hiện tại */
        case MODE_NORMAL:
            switch (status) {
                case PHASE_A_GREEN: led_set_traffic(0, 0, 1,  1, 0, 0); break; // T1 Xanh, T2 Đỏ
                case PHASE_A_AMBER: led_set_traffic(0, 1, 0,  1, 0, 0); break; // T1 Vàng, T2 Đỏ
                case PHASE_B_GREEN: led_set_traffic(1, 0, 0,  0, 0, 1); break; // T1 Đỏ, T2 Xanh
                case PHASE_B_AMBER: led_set_traffic(1, 0, 0,  0, 1, 0); break; // T1 Đỏ, T2 Vàng
                default: break;
            }
            break;

        /* 2. MODE_MANUAL: Nhấp nháy 2Hz các đèn ĐANG HOẠT ĐỘNG ở pha đó */
        case MODE_MANUAL:
            if (blink_on == 1) {
                switch (status) {
                    case PHASE_A_GREEN: led_set_traffic(0, 0, 1,  1, 0, 0); break;
                    case PHASE_A_AMBER: led_set_traffic(0, 1, 0,  1, 0, 0); break;
                    case PHASE_B_GREEN: led_set_traffic(1, 0, 0,  0, 0, 1); break;
                    case PHASE_B_AMBER: led_set_traffic(1, 0, 0,  0, 1, 0); break;
                    default: break;
                }
            } else {
                led_set_traffic(0, 0, 0,  0, 0, 0);
            }
            break;

        /* 3. MODE_TUNING: Nhấp nháy 2Hz CẶP ĐÈN ĐANG ĐƯỢC CHỈNH THỜI GIÀN */
        case MODE_SET_RED:
            if (blink_on == 1) led_set_traffic(1, 0, 0,  1, 0, 0); // Nhấp nháy 2 đèn Đỏ
            else          led_set_traffic(0, 0, 0,  0, 0, 0);
            break;

        case MODE_SET_AMBER:
            if (blink_on == 1) led_set_traffic(0, 1, 0,  0, 1, 0); // Nhấp nháy 2 đèn Vàng
            else          led_set_traffic(0, 0, 0,  0, 0, 0);
            break;

        case MODE_SET_GREEN:
            if (blink_on == 1) led_set_traffic(0, 0, 1,  0, 0, 1); // Nhấp nháy 2 đèn Xanh
            else          led_set_traffic(0, 0, 0,  0, 0, 0);
            break;

        default:
            break;
    }
}
