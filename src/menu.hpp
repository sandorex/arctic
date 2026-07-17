#pragma once

#include <stdint.h>

enum Menu {
    ENABLED ,
    ON_TIME,
    OFF_TIME,
    SLEEP,
    DELAY,

    TEST_NEXT,
    TEST_OFF,
    TEST_FAN,
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
