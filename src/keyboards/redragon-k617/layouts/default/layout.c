#include "kbdef.h"
#include "layout.h"
#include "user_layout.h"
#include "report.h"
#include <stdint.h>

// clang-format off

#define _BL 0
#define _FL 1

// TR-Q: BSLS=virgül, NUBS=<>, COMM/DOT/SLSH=ö/ç/.
// FN: WASD=arrows, Q=Snap, Win=lock, N=soft, M=bri, ö=speed, AltGr=fx, Esc="
const uint16_t keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_BL] = {
        { KC_ESC,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, KC_EQL,  KC_BSPC },
        { KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC, KC_RBRC, KC_BSLS },
        { KC_CAPS, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT, KC_BSLS, KC_ENT  },
        { KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_NUBS, KC_NUBS, KC_RSFT },
        /* Menu/App: stock HID 0x65. Dual-map c10 (Z11) + c12/c13 (K617 hole candidates). */
        { KC_LCTL, KC_LGUI, KC_LALT, KC_NO,   KC_NO,   KC_SPC,  KC_NO,   KC_NO,   KC_RALT, MO(_FL), KC_APP,  KC_RCTL, KC_APP,  KC_APP  }
    },
    [_FL] = {
        { KC_GRV,  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,  _______ },
        { RESET_HOLD, SNAP_TAP_TOGGLE, KC_UP, KC_VOLD, KC_VOLU, _______, _______, _______, _______, _______, KC_PSCR, KC_INS,  KC_DEL,  _______ },
        { _______, KC_LEFT, KC_DOWN, KC_RGHT, _______, _______, _______, _______, _______, _______, KC_PGUP, KC_PGDN, _______, _______ },
        { _______, _______, _______, _______, FACT_RESET, _______, RGB_SOFT_CYCLE, RGB_BRI_UP, RGB_SPD_UP, KC_HOME, KC_END,  _______, _______, _______ },
        { _______, WIN_LOCK_TOGGLE, _______, KC_NO, KC_NO, _______, KC_NO, KC_NO, RGB_FX_NEXT, _______, _______, _______, KC_NO, KC_NO }
    }
};
