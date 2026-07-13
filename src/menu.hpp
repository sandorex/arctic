#pragma once

#include <stdint.h>

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

void menu_setup();
void menu_splash();
void menu_render();
void menu_rotate(int8_t direction);
void menu_button();
void menu_off();
void menu_on();
