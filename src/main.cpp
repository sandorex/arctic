#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp32-hal-gpio.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "globals.hpp"
#include "hal/gpio_types.h"
#include "ir_codes.hpp"
#include "menu.hpp"
#include <Arduino.h>
#include <Wire.h>
#include <stdint.h>
#include <RotaryEncoder.h>
#include <OneButton.h>
#include "time.h"
#include <sys/time.h>

#define DEBUG

#ifdef DEBUG
#define SERIAL_PRINTF(...) do { Serial.printf(__VA_ARGS__); } while (0)
#else
#define SERIAL_PRINTF(...) do {} while (0)
#endif

// TODO make sleep go to sleep on OFF state regardless of time, so it does not need to be in cycles
// TODO brown out detection when batteries get low? prob 2.7v? i do not know if the oled will run at that

RotaryEncoder encoder(ENC1, ENC2, RotaryEncoder::LatchMode::FOUR3);
OneButton btn = OneButton(ENC_BTN, ENC_BTN_ACTIVE_LOW, ENC_BTN_PULLUP);

uint32_t millis_rtc();
void encoder_button();
void update_state();

bool RTC_DATA_ATTR initialized = false;

void setup() {
    #ifdef DEBUG
    Serial.begin(115200);
    // delay(5000);

    while (!Serial) { // NOTE this is useful for debugging but it will block if there is computer is not connected
        ;  // busy wait for USB
    }

    // needs bit of a delay to open monitor in time
    delay(1000);
    #endif

    esp_reset_reason_t reset_reason = esp_reset_reason();

    // restoring gpios to normal
    rtc_gpio_hold_dis((gpio_num_t)ENC1);
    rtc_gpio_hold_dis((gpio_num_t)ENC2);

    ir_setup();

    Wire.begin(SCREEN_SDA, SCREEN_SCL);

    // show splash only when not waking up from sleep
    // esp_sleep_wakeup_cause_t reason = esp_sleep_get_wakeup_cause();
    // esp_reset_reason_t reason = esp_reset_reason();
    // SERIAL_PRINTF("Wakeup reason: %d\n", reason);

    delay(300);

    SERIAL_PRINTF("Initializing the display\n");
    menu_setup();
    menu_on();

    // wait for display
    delay(300);

    // show splash only if poweron
    if (reset_reason == ESP_RST_POWERON || reset_reason == ESP_RST_UNKNOWN) {
        SERIAL_PRINTF("Splash\n");
        menu_splash();
        delay(2000);
    }

    // render the content
    menu_render();

    SERIAL_PRINTF("Updating state\n");

    // TODO update the state and things

    update_state();

    SERIAL_PRINTF("RTC Millis: %u\n", millis_rtc());

    // prevent sleep on first loop
    last_interaction = millis();
}

void loop() {
    if (millis() - last_interaction >= 6000) {
        // esp_sleep_enable_ext1_wakeup((1ULL << (gpio_num_t)ENC1 | 1ULL << (gpio_num_t)ENC2), ESP_EXT1_WAKEUP_ANY_LOW);
        // TODO schedule timer at the end of current cycle if any, calculate the time remaining if woken up by button

        esp_sleep_enable_timer_wakeup(2000000ULL); // 2 seconds

        // pull pins up during deep sleep
        pinMode(ENC1, OUTPUT);
        digitalWrite(ENC1, HIGH);
        rtc_gpio_hold_en((gpio_num_t)ENC1);

        pinMode(ENC2, OUTPUT);
        digitalWrite(ENC2, HIGH);
        rtc_gpio_hold_en((gpio_num_t)ENC2);

        menu_off();

        SERIAL_PRINTF("Going to sleep\n");

#ifdef DEBUG
        Serial.flush();
#endif

        menu_flush();
        Wire.flush();
        Wire.end();
        // pinMode(SCREEN_SDA, INPUT);
        // pinMode(SCREEN_SCL, INPUT);
        esp_deep_sleep_start();
    }

    return;

    if (state != old_state) {
        update_state();
    }

    noInterrupts();
    encoder.tick();
    btn.tick();
    interrupts();

    long newPos = encoder.getPosition();
    if (pos != newPos) {
        pos = newPos;
        last_interaction = millis();

        menu_rotate((int8_t)encoder.getDirection());
    } else {
        // TODO ignore buttons if display is not on
        int clicks = btn.getNumberClicks();
        if (clicks > 0) {
            if (!button_wake) {
                last_interaction = millis();
                menu_button();
            }

            menu_render();

            // reset the flag
            button_wake = false;
        }
    }

    // sleep after 6 seconds
    if (millis() - last_interaction >= 6000) {
        esp_sleep_enable_ext1_wakeup((1ULL << (gpio_num_t)ENC1 | 1ULL << (gpio_num_t)ENC2), ESP_EXT1_WAKEUP_ANY_LOW);
        // TODO schedule timer at the end of current cycle if any, calculate the time remaining if woken up by button

        // pull pins up during deep sleep
        pinMode(ENC1, OUTPUT);
        digitalWrite(ENC1, HIGH);
        rtc_gpio_hold_en((gpio_num_t)ENC1);

        pinMode(ENC2, OUTPUT);
        digitalWrite(ENC2, HIGH);
        rtc_gpio_hold_en((gpio_num_t)ENC2);

        menu_off();

        SERIAL_PRINTF("Going to sleep\n");

#ifdef DEBUG
        Serial.flush();
#endif

        Wire.flush();
        esp_deep_sleep_start();
    }
}

void update_state() {
    noInterrupts();

    if (old_state == State::AC_DELAY) {
        cycles = 0;

        // reset delay time
        delay_time = 0;
    }

    switch (state) {
        case State::OFF:
            // turn off ac when disabling
            if (old_state == State::AC_ON || old_state == State::AC_FAN) {
                ac_off();
            }

            cycles = 0;
            timer_cycles = 0;
            break;
        case State::AC_OFF:
            ac_off();
            timer_cycles = (off_time * MINUTE_S) / CYCLE_TIME;
            break;
        case State::AC_ON:
            ac_on();
            timer_cycles = (on_time * MINUTE_S) / CYCLE_TIME;
            break;
        case State::AC_FAN:
            ac_fan();
            timer_cycles = 2; // TODO random value
            break;

        case State::AC_DELAY:
            cycles = 0;
            timer_cycles = (delay_time * MINUTE_S) / CYCLE_TIME;
            break;

        default:
            break;
    }

    old_state = state;

    interrupts();
}

void encoder_button() {
    // reset interaction timer
    last_interaction = millis();

    // first button press should be ignored
    button_wake = true;
}

// Calculates millis from the RTC so it will reset only on restart/poweroff
uint32_t millis_rtc() {
    struct timeval tv = { .tv_sec = 0, .tv_usec = 0 };
    // uint32_t sec, us;
    gettimeofday(&tv, NULL); 
    // (sec) = tv.tv_sec;
    // (us) = tv.tv_usec;
    // calculate milliseconds
    return (tv.tv_sec * 1000UL + tv.tv_usec / 1000UL);
}

// ISR(WDT_vect) {
//     // re-set the WDT settings
//     my_wdt_enable();
//
//     // decrement waiting cycles
//     if (timer_cycles > 0) {
//         timer_cycles -= 1;
//     }
//
//     if (timer_cycles == 0) {
//         switch (state) {
//             case State::AC_OFF:
//                 state = State::AC_ON;
//                 cycles += 1;
//                 break;
//             case State::AC_ON:
//                 state = State::AC_FAN;
//                 break;
//             case State::AC_FAN:
//                 state = State::AC_OFF;
//                 break;
//
//             case State::AC_DELAY:
//                 state = State::AC_ON;
//                 break;
//         }
//
//         // TODO move to state switching so it turns it off after AC_FAN or AC_ON
//         // sleep if set and enough cycles passed
//         if (sleep_cycles > 0 && cycles >= sleep_cycles) {
//             state = State::OFF;
//         }
//     }
// }
