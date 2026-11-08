#include "software_timer.h"

volatile int timer_counter[NO_OF_TIMERS];
volatile int timer_flag[NO_OF_TIMERS];

void setTimer(int index, int duration)
{
	if (index < 0 || index >= NO_OF_TIMERS) {
		return;
	}
	timer_counter[index] = duration;
	timer_flag[index] = 0;
}

void timerRun(void)
{
	for (int i = 0; i < NO_OF_TIMERS; i++) {
		if (timer_counter[i] > 0) {
			timer_counter[i]--;
			if (timer_counter[i] <= 0) {
				timer_flag[i] = 1;
			}
		}
	}
}

uint8_t isTimerExpired(int index)
{
	if (index < 0 || index >= NO_OF_TIMERS) {
		return 0U;
	}
	uint8_t expired = (timer_flag[index] == 1);
	timer_flag[index] = 0;
	return expired;
}
