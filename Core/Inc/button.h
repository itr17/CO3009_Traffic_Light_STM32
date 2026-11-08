#ifndef INC_BUTTON_H_
#define INC_BUTTON_H_

#include "global.h"

#define NUM_OF_BUTTONS BTN_COUNT

void button_init_state(void);
void button_reading(void);
uint8_t is_button_pressed(uint8_t index);
uint8_t is_button_pressed_1s(uint8_t index);
void fsm_for_input_processing(void);
#endif /* INC_BUTTON_H_ */
