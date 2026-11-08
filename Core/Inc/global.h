#ifndef GLOBAL_H
#define GLOBAL_H

#include "main.h"
#include <stdint.h>

/* Timing and duration configuration */
#define BUTTON_SCAN_MS       10U

#define DEFAULT_RED_SECONDS  5U
#define DEFAULT_AMBER_SECONDS 2U
#define DEFAULT_GREEN_SECONDS 3U

#define MIN_DURATION_SECONDS 1U
#define MAX_DURATION_SECONDS 99U

/* Application modes selected by the mode button */
typedef enum {
    MODE_NORMAL = 1,
	MODE_MANUAL,
    MODE_SET_RED,
    MODE_SET_AMBER,
    MODE_SET_GREEN
} AppMode;

/* Button identifiers */
typedef enum {
    BTN_MODE = 0,
    BTN_UP,
	BTN_DOWN,
    BTN_SET,
    BTN_COUNT
} ButtonId;

/* Traffic-light FSM states stored in status */
typedef enum {
    PHASE_A_GREEN = 0,
    PHASE_A_AMBER,
    PHASE_B_GREEN,
    PHASE_B_AMBER
} Traffic_status;

typedef enum {
    BUTTON_RELEASED,
    BUTTON_PRESSED,
    BUTTON_PRESSED_MORE_THAN_1_SECOND
} ButtonState;

/* Current application mode */
extern AppMode app_mode;

/* Configured traffic-light durations, in seconds */
extern uint8_t duration_red, duration_amber, duration_green;
extern uint8_t update_duration;

/* Four-digit seven-segment display buffer */
extern uint8_t display_digits[4];

/* Current traffic-light FSM state */
extern int status;
// Sensor
extern uint8_t dht20_ok;
extern float temperature;
extern float humidity;

extern uint8_t count1;
extern uint8_t count2;

#endif /* GLOBAL_H */
