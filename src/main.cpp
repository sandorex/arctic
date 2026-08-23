#include "driver/rtc_io.h"
#include "esp32-hal-adc.h"
#include "esp32-hal-gpio.h"
#include "esp32-hal-timer.h"
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

/* WARNING this project is pretty messy and unfinished

I've spent way way too long on this project, the original goal was battery
powered device but im giving up for now and just gonna keep it usb powered

Some notes for archival in case i resume the project:

- The oled display wont restart properly when powered off using the PMOS,
and it does not have a reset pin or software reset command

- The oled can power itself from pullups on i2c and get in a glitched state,
also has internal capacitor for the charge pump so it takes a long time to
fully discharge

- Light sleep wouldnt wake up at all from pins?

- I used non-RTC pins for SDL and SCL so i dont know if rtc_gpio_hold_en would
help with oled resetting
*/

// TODO delay between FAN and off should be 1.5min minimum
// TODO make sleep go to sleep on OFF state regardless of time, so it does not need to be in cycles

RotaryEncoder encoder(ENC1, ENC2, RotaryEncoder::LatchMode::FOUR3);
OneButton btn = OneButton(ENC_BTN, ENC_BTN_ACTIVE_LOW, ENC_BTN_PULLUP);

uint32_t millis_rtc();
void encoder_button();
void update_state();
void IRAM_ATTR onTimer();

uint32_t state_millis = 0;

void setup() {
    #ifdef DEBUG
    Serial.begin(115200);

    // while (!Serial) { // NOTE this is useful for debugging but it will block if there is computer is not connected
    //     ;  // busy wait for USB
    // }

    // needs bit of a delay to open monitor in time
    // delay(1000);
    #endif

    ir_setup();

    // measure battery
    pinMode(BATTERY_SENSE_PIN, INPUT);
    float battery = get_vcc();
    SERIAL_PRINTF("VCC at %f\n", battery);

    // prevent running on low voltage
    if (battery < 2.8) {
        SERIAL_PRINTF("VCC LOW!\n");
        esp_deep_sleep_start();
    }

    menu_setup();

    // show splash only if poweron
    esp_reset_reason_t reset_reason = esp_reset_reason();
    SERIAL_PRINTF("Reset caused by %d\n", reset_reason);
    if (reset_reason == ESP_RST_POWERON || reset_reason == ESP_RST_UNKNOWN) {
        SERIAL_PRINTF("Showing splash\n");
        menu_splash();
        delay(2000);
    }

    // initial render
    menu_render();

    SERIAL_PRINTF("Updating state\n");

    update_state();

    // prevent sleep on first loop
    last_interaction = millis();
}

void loop() {
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

        if (!menu_enabled) {
            menu_on();
            menu_render();
        } else {
            menu_rotate((int8_t)encoder.getDirection());
        }
    } else {
        int clicks = btn.getNumberClicks();
        if (clicks > 0) {
            last_interaction = millis();

            // ignore press if menu is not enabled
            if (!menu_enabled) {
                menu_on();
                menu_render();
            } else {
                menu_button();
            }
        }
    }

    // update state if time has passed
    if (millis() >= state_millis + (timer_seconds * 1000)) {
        switch (state) {
            case State::AC_OFF:
                state = State::AC_ON;
                cycles += 1;
                break;
            case State::AC_ON:
                state = State::AC_FAN;
                break;
            case State::AC_FAN:
                state = State::AC_OFF;
                break;

            case State::AC_DELAY:
                state = State::AC_ON;
                break;
        }
    }

    // conserve power and oled by turning it off after a delay
    if (millis() - last_interaction >= 10000 && menu_enabled) {
        SERIAL_PRINTF("Turning off display\n");
        menu_off();
    }
}

void update_state() {
    noInterrupts();

    if (old_state == State::AC_DELAY) {
        cycles = 0;

        // reset delay time
        delay_time = 0;
    }

    // sleep if set and enough cycles passed
    if (sleep_cycles > 0 && cycles >= sleep_cycles) {
        state = State::OFF;
    } else {
        switch (state) {
            case State::OFF:
                // turn off ac when disabling
                if (old_state == State::AC_ON || old_state == State::AC_FAN) {
                    ac_off();
                }

                cycles = 0;
                timer_seconds = 0;
                break;
            case State::AC_OFF:
                if (fully_off) {
                    ac_off();
                }
                timer_seconds = (off_time * MINUTE_S);
                break;
            case State::AC_ON:
                ac_on();
                timer_seconds = (on_time * MINUTE_S);
                break;
            case State::AC_FAN:
                ac_fan();
                // blow for a minute to cool down the coils
                timer_seconds = 60; // TODO define global
                break;

            case State::AC_DELAY:
                cycles = 0;
                timer_seconds = delay_time * MINUTE_S;
                break;

            default:
                break;
        }
    }

    old_state = state;
    state_millis = millis();

    interrupts();
}

void onTimer() {

    // TODO move to state switching so it turns it off after AC_FAN or AC_ON
    // }
}

// TODO calibrate it properly
float get_vcc() {
    // NOTE the multiplier cause my resistors were 5% and i got 4.882000V at 5.12V
    return ((analogReadMilliVolts(BATTERY_SENSE_PIN) * 2.0) / 1000.0) * 1.024170422;
}

// calculates millis from the RTC so it will reset only on restart/poweroff
uint32_t millis_rtc() {
    struct timeval tv = { .tv_sec = 0, .tv_usec = 0 };
    gettimeofday(&tv, NULL);
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
