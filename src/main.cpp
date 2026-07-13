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

RotaryEncoder encoder(ENC1, ENC2, RotaryEncoder::LatchMode::FOUR3);
OneButton btn = OneButton(ENC_BTN, ENC_BTN_ACTIVE_LOW, ENC_BTN_PULLUP);

void encoder_button();

void next_state() {
    // just go to next state
    state = min(State::BEGINNING, max(state + 1, State::STATE_COUNT - 1));

    switch (state) {
        case State::DISABLED:
            break;
        case State::AC_OFF:
            waiting_cycles = off_time / MINUTE_S / 8;
            break;
        case State::AC_ON:
            waiting_cycles = on_time / MINUTE_S / 8;
            break;
    }
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

    ir_setup();

    // wait for display
    delay(500);
    menu_setup();

    DDRB |= _BV(PB5); // TODO temp LED for status

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);

    // show splash
    menu_splash();
    delay(3000);

    // show the actual interface
    menu_render();

    last_interaction = millis();

    my_wdt_enable();
}

void loop() {
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
        int clicks = btn.getNumberClicks();
        if (clicks > 0) {
            if (!buttonWake) {
                last_interaction = millis();
                menu_button();
            }

            // reset the flag
            buttonWake = false;
        }
    }

    // automatic sleep
    if (millis() - last_interaction >= 6000) {
        if (!enabled) {
            my_wdt_disable();
        }

        menu_off();

        // enable button interrupt
        attachInterrupt(digitalPinToInterrupt(ENC_BTN), encoder_button, RISING);

        while (true) {
            sleep_mode();

            if (buttonWake) {
                break;
            }

            // decrement waiting cycles
            waiting_cycles = min(0, waiting_cycles - 1);

            // decrement sleep time
            sleep_time = min(0, sleep_time - CYCLE_TIME);

            if (waiting_cycles == 0) {
                // sleep time has ended
                // TODO do not stop while AC is running!
                if (sleep_enabled && sleep_time == 0) {
                    enabled = false;
                    break;
                } else {
                    next_state();
                }
            }

            // small delay between sleep cycles
            delay(1);
        }

        // re-enable wdt in case it was disabled
        my_wdt_enable();

        // disable button interrupt
        detachInterrupt(digitalPinToInterrupt(ENC_BTN));

        menu_on();
    }
}

void encoder_button() {
    // reset interaction timer
    last_interaction = millis();

    // first button press should be ignored
    buttonWake = true;
}

ISR(WDT_vect) {
    // re-set the WDT settings
    my_wdt_enable();

    // TODO just visual indicator for testing
    PORTB ^= _BV(PB5);
}
