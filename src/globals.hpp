#pragma once

#include "esp_attr.h"
#include "stdint.h"

#define DEBUG

#ifdef DEBUG
#define SERIAL_PRINTF(...) do { Serial.printf(__VA_ARGS__); } while (0)
#else
#define SERIAL_PRINTF(...) do {} while (0)
#endif

#define VERSION "V0.1.1"

#define MIN_ON_TIME 2
#define MAX_ON_TIME 20

#define MIN_OFF_TIME 10
#define MAX_OFF_TIME 50

#define BATTERY_SENSE_PIN 17

#define ENC1 18
#define ENC2 21
#define ENC_BTN 16
#define ENC_BTN_ACTIVE_LOW true
#define ENC_BTN_PULLUP true

#define IR_LED 15 // also BUILTIN_LED

#define SCREEN_PWR_PIN 37
#define SCREEN_SDA 33
#define SCREEN_SCL 35
#define SCREEN_FLIP

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define SCREEN_ADDRESS 0x3C

// time to wait before turning the AC off completely
#define COIL_REST_TIME 90
#define MINUTE_MS 60000
#define MINUTE_S 60

enum State {
    OFF,

    AC_DELAY,

    AC_ON,
    AC_FAN,
    AC_OFF,

    STATE_COUNT,
};

extern volatile uint8_t state;
extern volatile uint8_t old_state;

// whether cooling or heating
extern bool cool_mode;

extern bool menu_enabled;

// encoder position used to detect change
extern long pos;

// turn the AC fully off at end of the cycle
extern bool fully_off;

extern RTC_DATA_ATTR uint8_t on_time;
extern RTC_DATA_ATTR uint8_t off_time;
extern RTC_DATA_ATTR uint16_t rest_time_s;
extern RTC_DATA_ATTR uint8_t speed_cool;
extern RTC_DATA_ATTR uint8_t speed_heat;
extern RTC_DATA_ATTR uint8_t speed_fan;

// how many full cycles before turning off
extern RTC_DATA_ATTR volatile int32_t sleep_cycles;

extern uint32_t state_millis;

// how long to wait before turning on the AC (in minutes)
extern RTC_DATA_ATTR int16_t delay_time;

// TODO see if this can be removed as all logic is in update_state
// time before next state (used to create the timer)
extern volatile uint16_t timer_seconds;

// increased each time state is reset to BEGINNING
extern RTC_DATA_ATTR volatile uint16_t cycles;

// was woken up by the button
extern volatile bool button_wake;

// time when last interaction was (encoder or button)
extern volatile uint32_t last_interaction;

void update_state();
float get_vcc();
