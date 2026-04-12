#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct __attribute__((__packed__)) {
    uint8_t version;
    bool swap_ab;
    uint8_t save_slot;
    bool player1_hard;
    bool player2_hard;
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
