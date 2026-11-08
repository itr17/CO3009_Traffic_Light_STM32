#include "button.h"

#define BUTTON_IS_PRESSED GPIO_PIN_RESET
#define BUTTON_IS_RELEASED GPIO_PIN_SET

static volatile GPIO_PinState buttonBuffer[NUM_OF_BUTTONS];
static volatile uint8_t flagForButtonPress1s[NUM_OF_BUTTONS];
static GPIO_PinState debounceButtonBuffer1[NUM_OF_BUTTONS];
static GPIO_PinState debounceButtonBuffer2[NUM_OF_BUTTONS];
static uint16_t counterForButtonPress1s[NUM_OF_BUTTONS];

static GPIO_PinState read_button_pin(uint8_t index)
{
    switch (index) {
        case BTN_MODE:
            return HAL_GPIO_ReadPin(BTN1_GPIO_Port, BTN1_Pin);
        case BTN_UP:
            return HAL_GPIO_ReadPin(BTN3_GPIO_Port, BTN3_Pin);
        case BTN_DOWN:
        	return HAL_GPIO_ReadPin(BTN4_GPIO_Port, BTN4_Pin);
        case BTN_SET:
            return HAL_GPIO_ReadPin(BTN2_GPIO_Port, BTN2_Pin);
        default:
            return BUTTON_IS_RELEASED;
    }
}

void button_init_state(void)
{
    for (uint8_t i = 0; i < NUM_OF_BUTTONS; i++) {
        buttonBuffer[i] = BUTTON_IS_RELEASED;
        debounceButtonBuffer1[i] = BUTTON_IS_RELEASED;
        debounceButtonBuffer2[i] = BUTTON_IS_RELEASED;
        flagForButtonPress1s[i] = 0U;
        counterForButtonPress1s[i] = 0U;
    }
}

void button_reading(void)
{
    for (uint8_t i = 0; i < NUM_OF_BUTTONS; i++) {
        debounceButtonBuffer1[i] = debounceButtonBuffer2[i];
        debounceButtonBuffer2[i] = read_button_pin(i);

        if (debounceButtonBuffer1[i] == debounceButtonBuffer2[i]) {
            buttonBuffer[i] = debounceButtonBuffer1[i];

            if (buttonBuffer[i] == BUTTON_IS_PRESSED) {
                const uint16_t holdTicks =
                    (uint16_t)(1000U / BUTTON_SCAN_MS);

                if (counterForButtonPress1s[i] < holdTicks) {
                    counterForButtonPress1s[i]++;
                } else {
                    flagForButtonPress1s[i] = 1U;
                }
            } else {
                counterForButtonPress1s[i] = 0U;
                flagForButtonPress1s[i] = 0U;
            }
        }
    }
}

uint8_t is_button_pressed(uint8_t index)
{
    if (index >= NUM_OF_BUTTONS) {
        return 0U;
    }

    return buttonBuffer[index] == BUTTON_IS_PRESSED;
}

uint8_t is_button_pressed_1s(uint8_t index)
{
    if (index >= NUM_OF_BUTTONS) {
        return 0U;
    }

    return flagForButtonPress1s[index] == 1U;
}

