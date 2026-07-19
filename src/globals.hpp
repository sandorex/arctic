#include "stdint.h"

#define VERSION "V0.1.0"

#define MIN_ON_TIME 3
#define MAX_ON_TIME 20

#define MIN_OFF_TIME 10
#define MAX_OFF_TIME 50

#define ENC1 A3
#define ENC2 A2
#define ENC_BTN 2
#define ENC_BTN_ACTIVE_LOW true
#define ENC_BTN_PULLUP true

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define SCREEN_ADDRESS 0x3C

#define CYCLE_TIME 8
#define MINUTE_MS 60000
#define MINUTE_S 60

enum State {
    DISABLED,

    AC_DELAY,

    AC_ON,
    AC_FAN,
    AC_OFF,

    STATE_COUNT,
};

extern uint8_t state;
extern uint8_t old_state;

extern bool menu_enabled;

// encoder position used to detect change
extern long pos;

extern uint8_t on_time;
extern uint8_t off_time;

// how many full cycles before turning off
extern volatile int32_t sleep_cycles;

// how long to wait before turning on the AC (in minutes)
extern int16_t delay_time;

// timer cycles (each is CYCLE_TIME), when 0 is reached then state is advanced
extern volatile uint16_t timer_cycles;

// increased each time state is reset to BEGINNING
extern volatile uint16_t cycles;

// was woken up by the button
extern volatile bool button_wake;

// time when last interaction was (encoder or button)
extern volatile uint32_t last_interaction;

void update_state();
