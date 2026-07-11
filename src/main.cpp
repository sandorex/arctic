#define PORT_IR PORTB
#define PIN_IR 9

#define SCREEN_ADDRESS 0x3C

#include "ir_codes.h"
#include <Arduino.h>
#include <Wire.h>
#include <stdint.h>
#include <RotaryEncoder.h>
#include <OneButton.h>

#define SSD1306_NO_SPLASH // do not show splashcreen
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

enum Menu {
    ON_TIME,
    ENABLED,
    OFF_TIME,
    TEST,

    // how many items there are
    LENGTH
};

// by default it uses A5 and A4 for SCK/SDA
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

RotaryEncoder encoder(4, 7, RotaryEncoder::LatchMode::FOUR3);

OneButton btn = OneButton(
    2,           // Input pin for the button
    true,        // Button is active LOW
    true         // Enable internal pull-up resistor
);

int pos = 0;
int menu_index = 0;
bool menu_editing = false;
bool enabled = true;
bool air_conditioner = false;

#define MIN_ON_TIME 5
#define MAX_ON_TIME 15

#define MIN_OFF_TIME 10
#define MAX_OFF_TIME 45

uint8_t on_time = 5;
uint8_t off_time = 25;

void setup() {
    Serial.begin(9600);

    // wait for display
    delay(500);

    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        for(;;);
    }

    // dim to lower power consumption
    display.dim(true);

    // TODO boot splash

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println(F("Hello"));
    display.display();

    pinMode(13, OUTPUT);

    // attachInterrupt(digitalPinToInterrupt(2), encoder_button, CHANGE);
}

// TODO add version somewhere on screen
void updateScreen() {
    display.clearDisplay();

    if (menu_index == Menu::TEST && menu_editing) {
        // simple test screen
        display.setCursor(43, 2);
        display.println("IR TEST");
        display.setCursor(10, 17);
        display.println("< OFF         ON >");
    } else {
        // TODO simple indicator that user is editing option
        if (menu_editing) {
            display.fillCircle(1, 1, 1, 1);
        }

        // top right corner
        display.setCursor(90, 0);
        display.printf("AC %s", air_conditioner ? "ON" : "OFF");

        // interval on field
        display.setCursor(3, 13);
        display.print("ON");
        if (menu_index == Menu::ON_TIME) {
            display.drawRect(0, 10, 17, 13, 1);
        }

        display.setCursor(0, 25);
        display.printf("%dm", on_time);

        // enable field (center) DO NOT MODIFY BY HAND
        display.setCursor(46, 13);
        display.println("ENABLE");
        if (menu_index == Menu::ENABLED) {
            display.drawRect(43, 10, 41, 13, 1);
        }

        display.setCursor(58, 25);
        display.println(enabled ? "ON" : "OFF");

        // interval off field
        display.setCursor(108, 12);
        display.print("OFF");
        if (menu_index == Menu::OFF_TIME) {
            display.drawRect(105, 9, 23, 13, 1);
        }

        display.setCursor(110, 25);
        display.printf("%dm", off_time);
    }

    display.display();
}

bool led = false;
void loop() {
    encoder.tick();
    btn.tick();

    int newPos = encoder.getPosition();
    if (pos != newPos) {
        pos = newPos;

        if (menu_editing) {
            switch (menu_index) {
                case Menu::ON_TIME:
                    on_time = min(max(on_time + (int)encoder.getDirection(), MIN_ON_TIME), MAX_ON_TIME);
                    break;
                case Menu::ENABLED:
                    enabled = !enabled;
                    break;
                case Menu::OFF_TIME:
                    off_time = min(max(off_time + (int)encoder.getDirection(), MIN_OFF_TIME), MAX_OFF_TIME);
                    break;
                case Menu::TEST: // TODO
                    break;
            }

        } else {
            menu_index = min(max(menu_index + (int)encoder.getDirection(), 0), Menu::LENGTH - 1);
        }

        updateScreen();
    } else {
        int clicks = btn.getNumberClicks();
        if (clicks > 0) {
            menu_editing = !menu_editing;
            led = !led;
            digitalWrite(13, led);

            updateScreen();
        }
    }
}
