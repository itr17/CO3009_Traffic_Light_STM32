#include "fsm_manual.h"
#include "button.h"
#include "global.h"
#include "traffic_light.h"
#include "fsm_automatic.h"
#include "fsm_tuning.h"

#define PHASE_CHANGE_TIMER 2
#define PHASE_CHANGE_WAIT_TICKS (3000U / BUTTON_SCAN_MS)
#define PHASE_CHANGE_TICKS_PER_SECOND (1000U / BUTTON_SCAN_MS)

static ButtonState modeButtonState;
static ButtonState upButtonState;
static ButtonState setButtonState;
static ButtonState downButtonState;
static uint8_t phase_change_waiting;

static void start_phase_change_wait(void)
{
    phase_change_waiting = 1U;
    setTimer(PHASE_CHANGE_TIMER, (int)PHASE_CHANGE_WAIT_TICKS);
}

static void next_status (void){
	switch (status){
	case PHASE_A_GREEN:
		status = PHASE_A_AMBER;
		break;
	case PHASE_A_AMBER:
		status = PHASE_B_GREEN;
		break;
	case PHASE_B_GREEN:
		status = PHASE_B_AMBER;
		break;
	case PHASE_B_AMBER:
		status = PHASE_A_GREEN;
		break;
    default:
        status = PHASE_A_GREEN;
        break;
	}
    start_phase_change_wait();
}

static void previous_status (void){
	switch (status){
	case PHASE_A_GREEN:
		status = PHASE_B_AMBER;
		break;
	case PHASE_A_AMBER:
		status = PHASE_A_GREEN;
		break;
	case PHASE_B_GREEN:
		status = PHASE_A_AMBER;
		break;
	case PHASE_B_AMBER:
		status = PHASE_B_GREEN;
		break;
    default:
        status = PHASE_A_GREEN;
        break;
	}
    start_phase_change_wait();
}



static ButtonState process_button(ButtonId button, ButtonState buttonState)
{
    switch (buttonState) {
        case BUTTON_RELEASED:
            if (is_button_pressed(button)) {
                switch (button) {
                    case BTN_MODE:
                    	app_mode = MODE_SET_RED;
                    	fsm_tuning_init();
                        break;
                    case BTN_UP:
                        next_status();
                        break;
                    case BTN_DOWN:
                    	previous_status();
                    	break;
                    case BTN_SET:
                        update_duration = 1U;
                        app_mode = MODE_NORMAL;
                        fsm_automatic_start_current_phase();
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
                return BUTTON_PRESSED_MORE_THAN_1_SECOND;
            }
            return BUTTON_PRESSED;
        case BUTTON_PRESSED_MORE_THAN_1_SECOND:
        	if (!is_button_pressed(button)) {
                return BUTTON_RELEASED;
        	}
        	return BUTTON_PRESSED_MORE_THAN_1_SECOND;
        default:
            return BUTTON_RELEASED;
    }
}

void fsm_for_input_processing(void)
{
    modeButtonState = process_button(BTN_MODE, modeButtonState);
    upButtonState = process_button(BTN_UP, upButtonState);
    downButtonState = process_button(BTN_DOWN, downButtonState);
    setButtonState = process_button(BTN_SET, setButtonState);
}

void fsm_manual_init(void)
{
    app_mode = MODE_MANUAL;
    status = PHASE_A_GREEN;
    phase_change_waiting = 0U;

    modeButtonState = is_button_pressed(BTN_MODE) ? BUTTON_PRESSED : BUTTON_RELEASED;
    upButtonState   = BUTTON_RELEASED;
    downButtonState = BUTTON_RELEASED;
    setButtonState  = BUTTON_RELEASED;
}

uint8_t fsm_manual_phase_change_active(void)
{
    return phase_change_waiting;
}

uint8_t fsm_manual_phase_change_seconds(void)
{
    int ticks = timer_counter[PHASE_CHANGE_TIMER];
    uint8_t seconds = (uint8_t)((ticks + PHASE_CHANGE_TICKS_PER_SECOND - 1U)
                                / PHASE_CHANGE_TICKS_PER_SECOND);

    return (seconds == 0U) ? 1U : seconds;
}

void fsm_manual_run(void)
{
    if (phase_change_waiting) {
        if (isTimerExpired(PHASE_CHANGE_TIMER)) {
            phase_change_waiting = 0U;
        } else {
            modeButtonState = is_button_pressed(BTN_MODE) ? BUTTON_PRESSED : BUTTON_RELEASED;
            upButtonState = is_button_pressed(BTN_UP) ? BUTTON_PRESSED : BUTTON_RELEASED;
            downButtonState = is_button_pressed(BTN_DOWN) ? BUTTON_PRESSED : BUTTON_RELEASED;
            setButtonState = is_button_pressed(BTN_SET) ? BUTTON_PRESSED : BUTTON_RELEASED;
            led_set_traffic(1U, 0U, 0U, 1U, 0U, 0U);
            return;
        }
    }

    // Process button input after the interlock has completed.
	fsm_for_input_processing();

	    // 2. Nếu không phải chế độ Manual thì dừng thực thi xuất đèn Manual
	if (app_mode != MODE_MANUAL) {
        return;
	}

    if (phase_change_waiting) {
        led_set_traffic(1U, 0U, 0U, 1U, 0U, 0U);
        return;
    }

	    // 3. Cập nhật xuất tín hiệu ra các cụm đèn theo trạng thái thủ công hiện tại
	traffic_light_render();
}
