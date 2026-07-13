#define VERSION "V0.1.0"

#define ENC1 4
#define ENC2 7
#define ENC_BTN 2
#define ENC_BTN_ACTIVE_LOW true
#define ENC_BTN_PULLUP true

#include "ir_codes.h"
#include <Arduino.h>
#include <Wire.h>
#include <stdint.h>
#include <RotaryEncoder.h>
#include <OneButton.h>
#include <avr/wdt.h>
#include <avr/power.h>

#define SCREEN_ADDRESS 0x3C
#define SSD1306_NO_SPLASH // do not show splashcreen
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

enum Menu {
    PAGE_ONE,
    ON_TIME = PAGE_ONE,
    OFF_TIME,
    ENABLED,
    SLEEP,

    PAGE_TWO,
    TEST_OFF = PAGE_TWO,
    TEST_ON,

    // how many items there are
    LENGTH
};

// by default it uses A5 and A4 for SCK/SDA
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
RotaryEncoder encoder(ENC1, ENC2, RotaryEncoder::LatchMode::FOUR3);
OneButton btn = OneButton(ENC_BTN, ENC_BTN_ACTIVE_LOW, ENC_BTN_PULLUP);

long pos = 0;
uint8_t menu_index = Menu::ENABLED;

// is the item being edited
bool menu_editing = false;

// is the device enabled
bool enabled = false;

// is air conditioner assumed running
bool air_conditioner = false;

#define MIN_ON_TIME 5
#define MAX_ON_TIME 15

#define MIN_OFF_TIME 10
#define MAX_OFF_TIME 45

uint8_t on_time = 5;
uint8_t off_time = 25;

// waiting for the next interval
bool waiting = false;

// which cycle in the waiting (1 cycle = 8 seconds)
uint16_t waiting_cycles = 0;

void updateScreen();
void deep_sleep();

volatile bool timer = false;

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
    my_wdt_enable();

    ir_setup();

    // wait for display
    delay(500);

    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println("Error intializing display.. halting");
        for(;;);
    }

    power_adc_disable();

    // dim to lower power consumption
    display.dim(true);

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(25, 12);
    display.print(F("ARCTIC " VERSION));
    display.display();

    // display.ssd1306_command(SSD1306_DISPLAYOFF);

    // pinMode(PIN_IR, OUTPUT);
    DDRB |= _BV(PB5);

    delay(3000);

    // show the actual interface
    updateScreen();

    // interrupts();
    // attachInterrupt(digitalPinToInterrupt(2), encoder_button, CHANGE);
}

void menuTwo() {
    display.setCursor(43, 0);
    display.println("IR TEST");

    display.setCursor(3, 10);
    display.print("OFF");
    if (menu_index == Menu::TEST_OFF) {
        display.drawRect(0, 7, 23, 13, 1);
    }

    display.setCursor(114, 10);
    display.print("ON");
    if (menu_index == Menu::TEST_ON) {
        display.drawRect(111, 7, 17, 13, 1);
    }
}

void menuOne() {
    // simple indicator that user is editing option
    if (menu_editing) {
        display.fillCircle(1, 1, 1, 1);
    }

    // top right corner
    display.setCursor(90, 0);
    display.printf("AC %s", air_conditioner ? "ON" : "OFF");

    // on time
    display.setCursor(3, 13);
    display.print("ON");
    if (menu_index == Menu::ON_TIME) {
        display.drawRect(0, 10, 17, 13, 1);
    }
    display.setCursor(0, 25);
    display.printf("%2dm/", on_time);

    // off time
    display.setCursor(24, 13);
    display.print("OFF");
    if (menu_index == Menu::OFF_TIME) {
        display.drawRect(21, 10, 24, 13, 1);
    }
    display.setCursor(24, 25);
    display.printf("%2dm", off_time);

    // enable
    display.setCursor(60, 13);
    display.print("EN");
    if (menu_index == Menu::ENABLED) {
        display.drawRect(56, 10, 19, 13, 1);
    }
    display.setCursor(57, 25);
    display.println(enabled ? "ON" : "OFF");

    // sleep timer
    display.setCursor(96, 13);
    display.print("SLEEP");
    if (menu_index == Menu::SLEEP) {
        display.drawRect(93, 10, 35, 13, 1);
    }
    display.setCursor(96, 25);
    display.print("/");
}

void updateScreen() {
    display.clearDisplay();

    if (menu_index < PAGE_TWO) {
        menuOne();
    } else if (menu_index >= PAGE_TWO) {
        menuTwo();
    }

    display.display();
}

void loop() {
    noInterrupts();
    encoder.tick();
    btn.tick();
    interrupts();

    long newPos = encoder.getPosition();
    if (pos != newPos) {
        pos = newPos;

        if (menu_editing) {
            switch (menu_index) {
                case Menu::ON_TIME:
                    on_time = min(max(on_time + (int)encoder.getDirection(), MIN_ON_TIME), MAX_ON_TIME);
                    break;
                case Menu::OFF_TIME:
                    off_time = min(max(off_time + (int)encoder.getDirection(), MIN_OFF_TIME), MAX_OFF_TIME);
                    break;
            }
        } else {
            menu_index = min(max(menu_index + (int)encoder.getDirection(), 0), Menu::LENGTH - 1);
        }

        updateScreen();
    } else {
        int clicks = btn.getNumberClicks();
        if (clicks > 0) {
            // special cases where button does something instead of start editing
            switch (menu_index) {
                case Menu::ENABLED:
                    enabled = !enabled;
                    updateScreen();
                    break;
                case Menu::TEST_ON:
                    ac_on();
                    break;
                case Menu::TEST_OFF:
                    ac_off();
                    break;
                default:
                    menu_editing = !menu_editing;
                    updateScreen();
                    break;
            }
        }
    }
}

ISR(WDT_vect) {
    PORTB ^= _BV(PB5);
    my_wdt_enable();
}
