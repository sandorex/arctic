#include "menu.hpp"
#include "ir_codes.hpp"
#include "globals.hpp"

#define SSD1306_NO_SPLASH // do not show splashcreen
#include <Adafruit_SSD1306.h>

const char* state_strings[] = { "OFF", "AC ON", "AC FAN", "AC OFF" };

// make sure there are strings for each state
static_assert((sizeof(state_strings) / sizeof(state_strings[0])) == (unsigned int)State::STATE_COUNT);

// by default it uses A5 and A4 for SCK/SDA
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// current menu index
uint8_t menu_index = Menu::ENABLED;

// is the item being edited
bool menu_editing = false;

void menu_page_1() {
    // simple indicator that user is editing option
    if (menu_editing) {
        display.fillCircle(1, 1, 1, 1);
    }

    // top right corner
    display.setCursor(90, 0);
    display.print(state_strings[state]);

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
    display.println(state == State::DISABLED ? "OFF" : "ON");

    // sleep timer
    display.setCursor(96, 13);
    display.print("SLEEP");
    if (menu_index == Menu::SLEEP) {
        display.drawRect(93, 10, 35, 13, 1);
    }
    display.setCursor(96, 25);
    if (sleep_cycles == 0) {
        display.print("OFF");
    } else {
        display.printf("%3lum", sleep_cycles * (on_time + off_time));
    }
}

void menu_page_2() {
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

    // debugging information
    display.setCursor(3, 22);
    display.printf("C: %d / T.C: %d", cycles, timer_cycles);
}

void menu_render() {
    display.clearDisplay();

    if (menu_index < PAGE_TWO) {
        menu_page_1();
    } else if (menu_index >= PAGE_TWO) {
        menu_page_2();
    }

    display.display();

    // turn on display if off
    if (!menu_enabled) {
        menu_on();
    }
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
            if (state == State::DISABLED) {
                state = State::AC_ON;
            } else {
                state = State::DISABLED;
            }

            update_state();
            menu_render();
            break;
        case Menu::TEST_ON:
            noInterrupts();
            ac_on();
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
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println("Error intializing display.. halting");
        for(;;);
    }

    // dim to lower power consumption
    display.dim(true);
}

void menu_splash() {
    // splash screen
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(25, 12);
    display.print(F("ARCTIC " VERSION));
    display.display();
}

void menu_off() {
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    menu_enabled = false;
}

void menu_on() {
    display.ssd1306_command(SSD1306_DISPLAYON);
    menu_enabled = true;
}

