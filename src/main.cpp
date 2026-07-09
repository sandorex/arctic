#define PORT_IR PORTB
#define PIN_IR 9

#define PORT_ENC PORTD
#define PIN_ENC_BTN 17  // A3 (INT1)
#define PIN_ENC_BTN_INT 1
#define PIN_ENC1 16     // A2 (INT0)
#define PIN_ENC1_INT 0
#define PIN_ENC2 15     // A1

#define SCREEN_ADDRESS 0x3C

#define INT8_MAX 127

#include "ir_codes.h"
#include <Arduino.h>
#include <Wire.h>
#include <stdint.h>

#define SSD1306_NO_SPLASH // do not show splashcreen
#include <Adafruit_SSD1306.h>

// by default it uses A5 and A4 for SCK/SDA
Adafruit_SSD1306 display(128, 32, &Wire, -1);

volatile int8_t value = 0;
volatile int8_t oldValue = 0;
volatile bool button = false;

void encoder();
void encoder_button();

void setup() {
    // wait for display
    delay(500);

    display.begin();

    // dim to lower power consumption
    display.dim(true);

    display.clearDisplay();
    display.display();

    // make all encoder pins inputs
    PORT_ENC &= ~(_BV(PIN_ENC1) | _BV(PIN_ENC2) | _BV(PIN_ENC_BTN));

    // NOTE as ATMega328p has only two interrupts i am using one for the button
    // and other for one of the encoder data pins as the other one will have a
    // diode to trigger the same interrupt
    attachInterrupt(PIN_ENC_BTN_INT, encoder_button, CHANGE);
    attachInterrupt(PIN_ENC1_INT, encoder, RISING);
}

void loop() {
    if (value != oldValue) {
        if (value > oldValue) {
            // plus
        } else {
            // minus
        }
    } else if (button) {
        // lone button press
    }

    delay(5);
}

void encoder() {
    // reset the value so it does not overflow
    if (abs(value) == INT8_MAX) {
        oldValue = 0;
        value = 0;
    }

    // second pin is HIGH then it triggered the interrupt
    if (PORT_ENC & _BV(PIN_ENC2)) {
        value += 1;
    } else {
        value -= 1;
    }
}

void encoder_button() {
    button = (PORT_ENC & _BV(PIN_ENC_BTN));
}
