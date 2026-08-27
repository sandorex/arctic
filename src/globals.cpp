#include "globals.hpp"
#include "esp_attr.h"

volatile uint8_t state = State::OFF;
volatile uint8_t old_state = State::OFF;

bool menu_enabled = true;

long pos = 0;

uint32_t state_millis = 0;
uint8_t RTC_DATA_ATTR on_time = 5;
uint8_t RTC_DATA_ATTR off_time = 25;
uint16_t RTC_DATA_ATTR rest_time_s = COIL_REST_TIME;
int16_t RTC_DATA_ATTR delay_time = 0;
bool fully_off = true;
volatile RTC_DATA_ATTR int32_t sleep_cycles = 0;
volatile RTC_DATA_ATTR int32_t delay_cycles = 0;
volatile RTC_DATA_ATTR uint16_t timer_cycles = 0;
volatile uint16_t timer_seconds;
volatile RTC_DATA_ATTR uint16_t cycles = 0;
volatile bool button_wake = false;
volatile bool button_wake_clicked = false;
volatile uint32_t last_interaction;
