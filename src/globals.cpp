#include "globals.hpp"

uint8_t state = State::DISABLED;
uint8_t old_state = State::DISABLED;

bool menu_enabled = true;

// encoder position used to detect change
long pos = 0;

uint8_t on_time = 5;
uint8_t off_time = 25;

// how many full cycles before turning off
volatile int32_t sleep_cycles = 0;

// timer cycles (each is CYCLE_TIME), when 0 is reached then state is advanced
volatile uint16_t timer_cycles = 0;

// increased each time state is reset to BEGINNING
volatile uint16_t cycles = 0;

// was woken up by the button
volatile bool button_wake = false;

volatile bool button_wake_clicked = false;

// time when last interaction was (encoder or button)
volatile uint32_t last_interaction;
