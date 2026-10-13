#include "ir_codes.hpp"
#include "globals.hpp"

// do not compile unecessary protocols
#define IR_ENABLE_DEFAULT false
#define SEND_TCL112AC 1
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Tcl.h>

IRTcl112Ac ac(IR_LED);

void ir_setup() {
    ac.begin();
    ac.setSwingVertical(kTcl112AcSwingVOff);
    ac.setSwingHorizontal(false);
    ac.setLight(true);
}

void set_fan_speed(uint8_t speed) {
    switch (speed) {
        case 0:
            ac.setFan(kTcl112AcFanAuto);
            break;
        case 1:
            ac.setFan(kTcl112AcFanMin);
            break;
        case 2:
            ac.setFan(kTcl112AcFanLow);
            break;
        case 3:
            ac.setFan(kTcl112AcFanMed);
            break;
        case 4:
            ac.setFan(kTcl112AcFanHigh);
            break;
    }
}

void ac_on() {
    // NOTE temperatures are intentionally in extremes to make sure it heats/cools when needed
    if (cool_mode) {
        ac.setMode(kTcl112AcCool);
        set_fan_speed(speed_cool);
        ac.setTemp(22);
    } else {
        ac.setMode(kTcl112AcHeat);
        set_fan_speed(speed_heat);
        ac.setTemp(28);
    }

    ac.setPower(true);
    ac.setLight(sleep_cycles == 0); // shut off display if sleep is enabled
    ac.send();
}

void ac_off() {
    ac.setPower(false);
    ac.send();
}

void ac_fan() {
    ac.setMode(kTcl112AcFan);
    set_fan_speed(speed_fan);
    ac.setPower(true);
    ac.setLight(sleep_cycles == 0); // shut off display if sleep is enabled
    ac.send();
}
