#include "menu.hpp"
#include "driver/rtc_io.h"
#include "esp32-hal-gpio.h"
#include "esp_attr.h"
#include "ir_codes.hpp"
#include "globals.hpp"

#define SSD1306_NO_SPLASH // do not show splashcreen
#include <Adafruit_SSD1306.h>

const char* state_strings[] = { "   OFF", "DELAY", " AC ON", "AC FAN", "AC OFF" };
static_assert((sizeof(state_strings) / sizeof(state_strings[0])) == (unsigned int)State::STATE_COUNT, "state count and state_strings count differ");

// current menu index
uint8_t menu_index = Menu::ENABLED;

// NOTE pins are defined by Wire.begin
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// is the item being edited
bool menu_editing = false;

void menu_render() {
    display.clearDisplay();
    display.setTextSize(1);

    // simple indicator that user is editing option
    if (menu_editing) {
        display.fillCircle(1, 1, 1, 1);
    }

    // top right corner (current state)
    display.setCursor(92, 0);
    display.print(state_strings[state]);

    if (menu_index == Menu::ON_TIME || menu_index == Menu::OFF_TIME) {
        // highlight which option is selected
        if (menu_index == Menu::ON_TIME) {
            display.drawLine(16, 30, 35, 30, 1);
        } else if (menu_index == Menu::OFF_TIME) {
            display.drawLine(74, 30, 107, 30, 1);
        }

        display.setTextSize(2);
        display.setCursor(14, 14);
        display.printf("%02d / %2dm", on_time, off_time);
    } else {
        display.setTextSize(2);
        display.setCursor(2, 14);

        switch (menu_index) {
            case Menu::ENABLED:
                display.print(
                        state == State::AC_DELAY
                            ? "DELAYED"
                            : (state == State::OFF
                                ? "DISABLED"
                                : "ENABLED"));
                break;
            case Menu::SLEEP:
                if (sleep_cycles == 0) {
                    display.print(F("SLEEP OFF"));
                } else {
                    unsigned long total = sleep_cycles * (on_time + off_time);
                    int hours = total / 60;
                    int minutes = total % 60;

                    if (hours == 0) {
                        display.printf("SLP %2dm", minutes);
                    } else {
                        display.printf("SLP %2dh%2dm", hours, minutes);
                    }
                }
                break;
            case Menu::DELAY:
                if (delay_time == 0) {
                    display.print(F("DELAY OFF"));
                } else {
                    int hours = delay_time / 60;
                    int minutes = delay_time % 60;

                    if (hours == 0) {
                        display.printf("DEL %2dm", minutes);
                    } else {
                        display.printf("DEL %2dh%2dm", hours, minutes);
                    }
                }
                break;
            case Menu::FULLY_OFF:
                if (fully_off) {
                    display.printf("FAN M. OFF");
                } else {
                    display.printf("FAN M. ON");
                }
                break;
            case Menu::TEST_NEXT:
                display.print(F("NEXT STATE"));
                break;
            case Menu::TEST_ON:
                display.print(F("IR ON"));
                break;
            case Menu::TEST_FAN:
                display.print(F("IR FAN"));
                break;
            case Menu::TEST_OFF:
                display.print(F("IR OFF"));
                break;
            case Menu::ON_TIME: case Menu::OFF_TIME:
                break;
        }
    }

    display.display();
}

void menu_rotate(int8_t direction) {
    if (menu_editing) {
        switch (menu_index) {
            case Menu::ON_TIME:
                on_time = min(max(on_time + direction, MIN_ON_TIME), MAX_ON_TIME);
                break;
            case Menu::OFF_TIME:
                off_time = min(max(off_time + direction, MIN_OFF_TIME), MAX_OFF_TIME);
                break;
            case Menu::SLEEP:
                sleep_cycles = max(0, sleep_cycles + direction);

                // reset cycles so it does not sleep after changing
                cycles = 0;
                break;
            case Menu::DELAY:
                // prevent editing delay while its active
                if (state != State::AC_DELAY) {
                    // increment 5 at a time as it does not make sense to sleep that precisely
                    delay_time = max(0, delay_time + direction * 5);
                }

                break;
        }
    } else {
        menu_index = min(max(menu_index + direction, 0), Menu::LENGTH - 1);
    }

    menu_render();
}

void menu_button() {
    // special cases where button does something instead of start editing
    switch (menu_index) {
        case Menu::ENABLED:
            if (state == State::OFF) {
                // start delay
                if (delay_time > 0) {
                    state = State::AC_DELAY;
                } else {
                    state = State::AC_ON;
                }
            } else {
                state = State::OFF;
            }

            update_state();
            menu_render();
            break;
        case Menu::FULLY_OFF:
            fully_off = !fully_off;
            menu_render();
            break;
        case Menu::TEST_NEXT:
            // TODO this should not be hardcoded here
            switch (state) {
                case State::AC_ON:
                    state = State::AC_FAN;
                    break;
                case State::AC_FAN:
                    state = State::AC_OFF;
                    break;
                case State::AC_OFF:
                    state = State::AC_ON;
                    break;

                case State::AC_DELAY:
                    state = State::AC_ON;
                    break;
            }
            update_state();
            menu_render();
            break;
        case Menu::TEST_ON:
            noInterrupts();
            ac_on();
            interrupts();
            break;
        case Menu::TEST_FAN:
            noInterrupts();
            ac_fan();
            interrupts();
            break;
        case Menu::TEST_OFF:
            noInterrupts();
            ac_off();
            interrupts();
            break;
        default:
            menu_editing = !menu_editing;
            menu_render();
            break;
    }
}

void menu_setup() {
    pinMode(SCREEN_PWR_PIN, OUTPUT);
    digitalWrite(SCREEN_PWR_PIN, HIGH);

    // turn on the display power
    SERIAL_PRINTF("Initializing display\n");

    // let display charge the capacitors
    delay(500);

    Wire.begin(SCREEN_SDA, SCREEN_SCL);

    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS, false, false)) {
        SERIAL_PRINTF("Error intializing display.. halting\n");
        for(;;);
    }

    // clear it imidiately or its gonna show garbage
    display.clearDisplay();
    display.display();

    // dim to lower power consumption
    display.dim(true);
}

void menu_splash() {
    esp_reset_reason_t reset_reason = esp_reset_reason();
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    // splash screen
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(25, 12);
    display.print(F("ARCTIC " VERSION));

    // display code only if not powered on
    if (reset_reason != ESP_RST_POWERON) {
        display.setCursor(0, 0);
        display.printf("CODE: %d / %d", reset_reason, wakeup_reason);
    }

    // show voltage in top right
    display.setCursor(98, 0);
    display.printf("%.2fV", get_vcc());

    display.display();

    // show for 5 more seconds if it was not restarted
    if (reset_reason != ESP_RST_POWERON) {
        delay(5000);
    }
}

void menu_off() {
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    menu_enabled = false;
}

void menu_on() {
    display.ssd1306_command(SSD1306_DISPLAYON);
    menu_enabled = true;
}

// TODO remove this
void menu_flush() {
    display.flush();
}
