#include "globals.hpp"

uint8_t state = State::DISABLED;
uint8_t old_state = State::DISABLED;

bool menu_enabled = true;

long pos = 0;

uint8_t on_time = 5;
uint8_t off_time = 25;
int16_t delay_time = 0;

volatile int32_t sleep_cycles = 0;

volatile int32_t delay_cycles = 0;

volatile uint16_t timer_cycles = 0;

volatile uint16_t cycles = 0;

volatile bool button_wake = false;

volatile bool button_wake_clicked = false;

volatile uint32_t last_interaction;
