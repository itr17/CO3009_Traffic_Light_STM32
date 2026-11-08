#include "fsm_tuning.h"
#include "button.h"
#include "fsm_automatic.h"
#include "traffic_light.h"
#include "software_timer.h"

#define A_SECOND_HOLD_UP (500U / BUTTON_SCAN_MS)


static ButtonState modeButtonState;
static ButtonState upButtonState;
static ButtonState setButtonState;
static ButtonState downButtonState;

static uint8_t setting_duration_red;
static uint8_t setting_duration_amber;
static uint8_t setting_duration_green;

static void discard_setting_changes(void)
{
    setting_duration_red = duration_red;
    setting_duration_amber = duration_amber;
    setting_duration_green = duration_green;
}

static void advance_setting_mode(void)
{
    switch (app_mode) {
        case MODE_MANUAL:
            app_mode = MODE_SET_RED;
            break;
        case MODE_SET_RED:
            app_mode = MODE_SET_AMBER;
            break;
        case MODE_SET_AMBER:
            app_mode = MODE_SET_GREEN;
            break;
        case MODE_SET_GREEN:
        default:
            app_mode = MODE_NORMAL;
            fsm_automatic_start_current_phase();
            break;
    }

    discard_setting_changes();
}

static void increase_selected_duration(void)
{
    switch (app_mode) {
        case MODE_SET_RED:
            if (setting_duration_red < MAX_DURATION_SECONDS) {
                setting_duration_red++;
            }
            else setting_duration_red = 1;
            break;
        case MODE_SET_AMBER:
            if (setting_duration_amber < MAX_DURATION_SECONDS) {
                setting_duration_amber++;
            }
            else setting_duration_amber = 1;
            break;
        case MODE_SET_GREEN:
            if (setting_duration_green < MAX_DURATION_SECONDS) {
                setting_duration_green++;
            }
            else setting_duration_green = 1;
            break;
        default:
            break;
    }
}


static void decrease_selected_duration(void)
{
    switch (app_mode) {
        case MODE_SET_RED:
            if (setting_duration_red > MIN_DURATION_SECONDS) {
                setting_duration_red--;
            }
            else setting_duration_red = 99;
            break;
        case MODE_SET_AMBER:
            if (setting_duration_amber > MIN_DURATION_SECONDS) {
                setting_duration_amber--;
            }
            else setting_duration_amber = 99;
            break;
        case MODE_SET_GREEN:
            if (setting_duration_green > MIN_DURATION_SECONDS) {
                setting_duration_green--;
            }
            else setting_duration_green = 99;
            break;
        default:
            break;
    }
}

static void save_selected_duration(void)
{
    switch (app_mode) {
        case MODE_SET_RED:
            duration_red = setting_duration_red;
            update_duration = 1;
            break;
        case MODE_SET_AMBER:
            duration_amber = setting_duration_amber;
            update_duration = 1;
            break;
        case MODE_SET_GREEN:
            duration_green = setting_duration_green;
            update_duration = 1;
            break;
        default:
            break;
    }

    if (duration_red > MAX_DURATION_SECONDS) {
        duration_red = 2;
        duration_amber = 1;
        duration_green = 1;
    }
}


static void update_setting_countdown(void)
{
    uint8_t duration;
    switch (app_mode) {
        case MODE_SET_RED:
            duration = setting_duration_red;
            break;
        case MODE_SET_AMBER:
            duration = setting_duration_amber;
            break;
        case MODE_SET_GREEN:
            duration = setting_duration_green;
            break;
        default:
            return;
    }

    count1 = duration;
    count2 = duration;
}

static ButtonState process_button(ButtonId button, ButtonState buttonState)
{
    switch (buttonState) {
        case BUTTON_RELEASED:
            if (is_button_pressed(button)) {
                switch (button) {
                    case BTN_MODE:
                        advance_setting_mode();
                        break;
                    case BTN_UP:
                        increase_selected_duration();
                        break;
                    case BTN_DOWN:
                    	decrease_selected_duration();
                    	break;
                    case BTN_SET:
                        save_selected_duration();
                        break;
                    default:
                        break;
                }
                return BUTTON_PRESSED;
            }
            return BUTTON_RELEASED;
        case BUTTON_PRESSED:
            if (!is_button_pressed(button)) {
                return BUTTON_RELEASED;
            }
            else if (is_button_pressed_1s(button)) {
                if (button == BTN_UP) {
                    setTimer(6, (int)A_SECOND_HOLD_UP);
                }
                else if (button == BTN_DOWN) {
                    setTimer(7, (int)A_SECOND_HOLD_UP);
                }
                return BUTTON_PRESSED_MORE_THAN_1_SECOND;
            }
            return BUTTON_PRESSED;
        case BUTTON_PRESSED_MORE_THAN_1_SECOND:
            if (!is_button_pressed(button)) {
                if (button == BTN_UP) {
                    setTimer(6, 0);
                }
                else if (button == BTN_DOWN) {
                	setTimer(7, 0);
                }
                return BUTTON_RELEASED;
            }
            if (button == BTN_UP && isTimerExpired(6)) {
                increase_selected_duration();
                setTimer(6, (int)A_SECOND_HOLD_UP);
            }
            else if (button == BTN_DOWN && isTimerExpired(7)) {
                decrease_selected_duration();
                setTimer(7, (int)A_SECOND_HOLD_UP);
            }
            return BUTTON_PRESSED_MORE_THAN_1_SECOND;
        default:
            return BUTTON_RELEASED;
    }
}

void fsm_for_input_processing_tuning(void)
{
    modeButtonState = process_button(BTN_MODE, modeButtonState);
    upButtonState = process_button(BTN_UP, upButtonState);
    downButtonState = process_button(BTN_DOWN, downButtonState);
    setButtonState = process_button(BTN_SET, setButtonState);
}

void fsm_tuning_init(void)
{
    // 1. Chuyển sang chế độ chỉnh đèn Đỏ đầu tiên
    app_mode = MODE_SET_RED;

    // 2. Lấy giá trị thời gian hiện tại vào biến tạm
    discard_setting_changes();

    // 3. Khởi tạo trạng thái ban đầu cho 4 nút bấm (Sửa BTN_DOWN)
    modeButtonState = is_button_pressed(BTN_MODE) ? BUTTON_PRESSED : BUTTON_RELEASED;
    upButtonState   = BUTTON_RELEASED;
    downButtonState = BUTTON_RELEASED; // Fix: Khai báo sạch sẽ
    setButtonState  = BUTTON_RELEASED;

    // Reset hold timers for the UP and DOWN buttons.
    setTimer(6, 0);
    setTimer(7, 0);
}

void fsm_tuning_run(void)
{
    // 1. Quét và xử lý nút bấm liên tục ở TẤT CẢ các chế độ
    fsm_for_input_processing_tuning();

    // 2. Nếu đang ở chế độ Tự động hoặc Thủ công thì không thực hiện hiển thị Tuning
    if (app_mode == MODE_NORMAL || app_mode == MODE_MANUAL) {
        return;
    }

    traffic_light_render();

    // 4. Cập nhật số giây đếm thời gian đang chỉnh lên màn hình/LED 7 đoạn
    update_setting_countdown();
}
