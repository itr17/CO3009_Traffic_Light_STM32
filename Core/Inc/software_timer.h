#ifndef INC_SOFTWARE_TIMER_H_
#define INC_SOFTWARE_TIMER_H_

#include "main.h"

#define NO_OF_TIMERS 10

extern volatile int timer_counter[NO_OF_TIMERS];
extern volatile int timer_flag[NO_OF_TIMERS];

void setTimer(int index, int duration);
void timerRun(void);
uint8_t isTimerExpired(int index);

#endif
