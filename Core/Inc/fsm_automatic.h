#ifndef FSM_AUTOMATIC_H
#define FSM_AUTOMATIC_H
#include "global.h"
#include "software_timer.h"
#include "traffic_light.h"
void fsm_automatic_init(void);
void fsm_automatic_start_current_phase(void);
void fsm_automatic_run(void);
#endif
