#include "global.h"

/* Current application mode */
AppMode app_mode = MODE_NORMAL;

/* Configured traffic-light durations, in seconds */
uint8_t duration_red = DEFAULT_RED_SECONDS;
uint8_t duration_amber = DEFAULT_AMBER_SECONDS;
uint8_t duration_green = DEFAULT_GREEN_SECONDS;
uint8_t update_duration = 0U;
uint8_t dht20_ok;
/* Four-digit seven-segment display buffer */
uint8_t display_digits[4] = {0, 0, 0, 0};

/* Current traffic-light FSM state */
int status = PHASE_A_GREEN;

float temperature = 0.0f;
float humidity = 0.0f;

uint8_t count1 = 0;
uint8_t count2 = 0;
