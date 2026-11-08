

#ifndef INC_FSM_MANUAL_H_
#define INC_FSM_MANUAL_H_

#include "global.h"
#include "software_timer.h"
#include "traffic_light.h"
void fsm_manual_init(void);
void fsm_manual_run(void);
uint8_t fsm_manual_phase_change_active(void);
uint8_t fsm_manual_phase_change_seconds(void);

#endif /* INC_FSM_MANUAL_H_ */
