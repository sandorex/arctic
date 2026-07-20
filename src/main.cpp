#include "globals.hpp"
#include "ir_codes.hpp"
#include "menu.hpp"
#include <Arduino.h>
#include <Wire.h>
#include <stdint.h>
#include <RotaryEncoder.h>
#include <OneButton.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/io.h>
#include <avr/sleep.h>

// TODO make sleep go to sleep on OFF state regardless of time, so it does not need to be in cycles

RotaryEncoder encoder(ENC1, ENC2, RotaryEncoder::LatchMode::FOUR3);
OneButton btn = OneButton(ENC_BTN, ENC_BTN_ACTIVE_LOW, ENC_BTN_PULLUP);

void encoder_button();

void update_state() {
    noInterrupts();

    if (old_state == State::AC_DELAY) {
        cycles = 0;

        // reset delay time
        delay_time = 0;
    }

    switch (state) {
        case State::DISABLED:
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

void my_wdt_enable() {
    noInterrupts();

    MCUSR &= ~(_BV(WDRF));
    WDTCSR |= (_BV(WDCE) | _BV(WDE));
    WDTCSR = _BV(WDIE) | _BV(WDP3) | _BV(WDP0);

    interrupts();
}

void my_wdt_disable() {
    MCUSR = 0;
    wdt_disable();
}

// NOTE for some reason serial and WDT clash and restart once? so just dont use serial anymore
void setup() {
    my_wdt_disable();
    power_adc_disable(); // power saving as i dont use it

    // wait for display
    delay(500);
    menu_setup();

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);

    // show splash
    menu_splash();
    delay(3000);

    // show the actual interface
    menu_render();

    last_interaction = millis();

    update_state();

    my_wdt_enable();
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

        menu_on();
        menu_rotate((int8_t)encoder.getDirection());
    } else {
        // TODO ignore buttons if display is not on
        int clicks = btn.getNumberClicks();
        if (clicks > 0) {
            if (!button_wake) {
                last_interaction = millis();
                menu_button();
            }

            menu_on();
            menu_render();

            // reset the flag
            button_wake = false;
        }
    }

    // sleep after 6 seconds
    if (millis() - last_interaction >= 6000) {
        if (state == State::DISABLED) {
            my_wdt_disable();
        }

        menu_off();

        // enable button interrupt
        attachInterrupt(digitalPinToInterrupt(ENC_BTN), encoder_button, RISING);

        sleep_mode();

        // disable button interrupt
        detachInterrupt(digitalPinToInterrupt(ENC_BTN));

        // re-enable wdt in case it was disabled
        my_wdt_enable();
    }
}

void encoder_button() {
    // reset interaction timer
    last_interaction = millis();

    // first button press should be ignored
    button_wake = true;
}

ISR(WDT_vect) {
    // re-set the WDT settings
    my_wdt_enable();

    // decrement waiting cycles
    if (timer_cycles > 0) {
        timer_cycles -= 1;
    }

    if (timer_cycles == 0) {
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

        // sleep if set and enough cycles passed
        if (sleep_cycles > 0 && cycles >= sleep_cycles) {
            state = State::DISABLED;
        }
    }
}
