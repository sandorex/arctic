#include "globals.hpp"
#include "esp_attr.h"

uint8_t RTC_DATA_ATTR state = State::OFF;
uint8_t RTC_DATA_ATTR old_state = State::OFF;

bool menu_enabled = true;

long pos = 0;

uint8_t RTC_DATA_ATTR on_time = 5;
uint8_t RTC_DATA_ATTR off_time = 25;
int16_t RTC_DATA_ATTR delay_time = 0;

volatile RTC_DATA_ATTR int32_t sleep_cycles = 0;
volatile RTC_DATA_ATTR int32_t delay_cycles = 0;

volatile RTC_DATA_ATTR uint16_t timer_cycles = 0;

volatile RTC_DATA_ATTR uint16_t cycles = 0;

volatile bool button_wake = false;

volatile bool button_wake_clicked = false;

volatile uint32_t last_interaction;
