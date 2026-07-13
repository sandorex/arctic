#include "stdint.h"

#define VERSION "V0.1.0"

#define MIN_ON_TIME 5
#define MAX_ON_TIME 20

#define MIN_OFF_TIME 10
#define MAX_OFF_TIME 50

#define ENC1 4
#define ENC2 7
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

    BEGINNING,
    AC_OFF,
    // AC_FAN,
    AC_ON = BEGINNING,

    STATE_COUNT,
};

static uint8_t state = State::DISABLED;

// encoder position used to detect change
static long pos = 0;

// is the device enabled
static bool enabled = false;

static uint8_t on_time = 5;
static uint8_t off_time = 25;

static bool sleep_enabled = false;

// delay before going to full sleep and disabling (in seconds)
static volatile uint32_t sleep_time = 0;

// which cycle in the waiting (1 cycle = 8 seconds)
static volatile uint16_t waiting_cycles = 0;

// was woken up by the button
static volatile bool buttonWake = false;

// time when last interaction was (encoder or button)
static volatile uint32_t last_interaction;

