#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct __attribute__((__packed__)) {
    uint8_t version;
    bool swap_ab;
    bool aspect_ratio;
    uint8_t ghosting;
    uint8_t palette;
    uint8_t save_slot;
    uint16_t tba;
    uint32_t rgb0;
    uint32_t rgb1;
    uint32_t rgb2;
    uint32_t rgb3;
    bool instant_ignition;
} SETTINGS;

extern SETTINGS settings;

typedef struct input_bits_s {
    bool a: true;
    bool b: true;
    bool select: true;
    bool start: true;
    bool right: true;
    bool left: true;
    bool up: true;
    bool down: true;
} input_bits_t;

extern input_bits_t gamepad1_bits;
extern input_bits_t gamepad2_bits;

typedef struct kbd_s {
    input_bits_t bits;
    int8_t h_code;
} kbd_t;

extern kbd_t keyboard;
