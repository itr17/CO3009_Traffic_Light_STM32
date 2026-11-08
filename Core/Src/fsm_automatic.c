#include "fsm_automatic.h"
#include "traffic_light.h"
#include "software_timer.h"
#include "button.h"
#include "fsm_manual.h"
static uint8_t remain;
static ButtonState modeButtonState;



static void check_duration(void){
	if (update_duration == 1) {
		if (duration_red > MAX_DURATION_SECONDS || duration_green + duration_amber > MAX_DURATION_SECONDS){
			duration_red = 5;
			duration_amber = 2;
			duration_green = 3;
		}
		else if (duration_green + duration_amber > duration_red) {
	    	duration_red = duration_green + duration_amber;
	    }
	    else if (duration_red > duration_green + duration_amber) {
	    	duration_green = duration_red - duration_amber;
	    }
	}
	update_duration = 0;
}

static uint8_t get_phase_duration(void)
{
    switch (status) {
        case PHASE_A_GREEN:
        case PHASE_B_GREEN:
            return duration_green;
        case PHASE_A_AMBER:
        case PHASE_B_AMBER:
            return duration_amber;
        default:
            return duration_green;
    }
}

static void update_countdown_display(void)
{
    uint8_t remain_a;
    uint8_t remain_b;

    switch (status) {
        case PHASE_A_GREEN:
            remain_a = remain;
            remain_b = (uint8_t)(remain + duration_amber);
            break;
        case PHASE_B_GREEN:
            remain_a = (uint8_t)(remain + duration_amber);
            remain_b = remain;
            break;
        case PHASE_A_AMBER:
        case PHASE_B_AMBER:
        default:
            remain_a = remain;
            remain_b = remain;
            break;
    }

    count1 = remain_a;
    count2 = remain_b;
}

void fsm_automatic_start_current_phase(void)
{
	check_duration();
    remain = get_phase_duration();
    modeButtonState = is_button_pressed(BTN_MODE) ? BUTTON_PRESSED : BUTTON_RELEASED;
    setTimer(1, 100);
    traffic_light_render();
    update_countdown_display();
}

void fsm_automatic_init(void)
{
    status = PHASE_A_GREEN;
    fsm_automatic_start_current_phase();
}

void fsm_automatic_run(void)
{
    if (modeButtonState == BUTTON_RELEASED && is_button_pressed(BTN_MODE)) {
        modeButtonState = BUTTON_PRESSED;
        app_mode = MODE_MANUAL;
        fsm_manual_init();
        return;
    }

    if (modeButtonState == BUTTON_PRESSED && !is_button_pressed(BTN_MODE)) {
        modeButtonState = BUTTON_RELEASED;
    }

	check_duration();
    if (isTimerExpired(1)) {
        if (remain > 0) {
            remain--;
        }

        if (remain == 0) {
            switch (status) {
                case PHASE_A_GREEN:
                    status = PHASE_A_AMBER;
                    break;
                case PHASE_A_AMBER:
                    status = PHASE_B_GREEN;
                    break;
                case PHASE_B_GREEN:
                    status = PHASE_B_AMBER;
                    break;
                default:
                    status = PHASE_A_GREEN;
                    break;
            }

            remain = get_phase_duration();
            traffic_light_render();
        }

        update_countdown_display();
        setTimer(1, 100);
    }
}
