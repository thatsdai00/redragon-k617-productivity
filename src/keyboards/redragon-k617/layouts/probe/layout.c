#include "kbdef.h"
#include "layout.h"
#include "user_layout.h"
#include "report.h"
#include <stdint.h>

// Every electrical cell emits its (row,col) when pressed (see kb.c probe handler).

#define _BL 0

const uint16_t keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_BL] = {
        {
            PROBE_KEYCODE(0, 0),  PROBE_KEYCODE(0, 1),  PROBE_KEYCODE(0, 2),  PROBE_KEYCODE(0, 3),
            PROBE_KEYCODE(0, 4),  PROBE_KEYCODE(0, 5),  PROBE_KEYCODE(0, 6),  PROBE_KEYCODE(0, 7),
            PROBE_KEYCODE(0, 8),  PROBE_KEYCODE(0, 9),  PROBE_KEYCODE(0, 10), PROBE_KEYCODE(0, 11),
            PROBE_KEYCODE(0, 12), PROBE_KEYCODE(0, 13), PROBE_KEYCODE(0, 14), PROBE_KEYCODE(0, 15),
        },
        {
            PROBE_KEYCODE(1, 0),  PROBE_KEYCODE(1, 1),  PROBE_KEYCODE(1, 2),  PROBE_KEYCODE(1, 3),
            PROBE_KEYCODE(1, 4),  PROBE_KEYCODE(1, 5),  PROBE_KEYCODE(1, 6),  PROBE_KEYCODE(1, 7),
            PROBE_KEYCODE(1, 8),  PROBE_KEYCODE(1, 9),  PROBE_KEYCODE(1, 10), PROBE_KEYCODE(1, 11),
            PROBE_KEYCODE(1, 12), PROBE_KEYCODE(1, 13), PROBE_KEYCODE(1, 14), PROBE_KEYCODE(1, 15),
        },
        {
            PROBE_KEYCODE(2, 0),  PROBE_KEYCODE(2, 1),  PROBE_KEYCODE(2, 2),  PROBE_KEYCODE(2, 3),
            PROBE_KEYCODE(2, 4),  PROBE_KEYCODE(2, 5),  PROBE_KEYCODE(2, 6),  PROBE_KEYCODE(2, 7),
            PROBE_KEYCODE(2, 8),  PROBE_KEYCODE(2, 9),  PROBE_KEYCODE(2, 10), PROBE_KEYCODE(2, 11),
            PROBE_KEYCODE(2, 12), PROBE_KEYCODE(2, 13), PROBE_KEYCODE(2, 14), PROBE_KEYCODE(2, 15),
        },
        {
            PROBE_KEYCODE(3, 0),  PROBE_KEYCODE(3, 1),  PROBE_KEYCODE(3, 2),  PROBE_KEYCODE(3, 3),
            PROBE_KEYCODE(3, 4),  PROBE_KEYCODE(3, 5),  PROBE_KEYCODE(3, 6),  PROBE_KEYCODE(3, 7),
            PROBE_KEYCODE(3, 8),  PROBE_KEYCODE(3, 9),  PROBE_KEYCODE(3, 10), PROBE_KEYCODE(3, 11),
            PROBE_KEYCODE(3, 12), PROBE_KEYCODE(3, 13), PROBE_KEYCODE(3, 14), PROBE_KEYCODE(3, 15),
        },
        {
            PROBE_KEYCODE(4, 0),  PROBE_KEYCODE(4, 1),  PROBE_KEYCODE(4, 2),  PROBE_KEYCODE(4, 3),
            PROBE_KEYCODE(4, 4),  PROBE_KEYCODE(4, 5),  PROBE_KEYCODE(4, 6),  PROBE_KEYCODE(4, 7),
            PROBE_KEYCODE(4, 8),  PROBE_KEYCODE(4, 9),  PROBE_KEYCODE(4, 10), PROBE_KEYCODE(4, 11),
            PROBE_KEYCODE(4, 12), PROBE_KEYCODE(4, 13), PROBE_KEYCODE(4, 14), PROBE_KEYCODE(4, 15),
        },
    },
};
