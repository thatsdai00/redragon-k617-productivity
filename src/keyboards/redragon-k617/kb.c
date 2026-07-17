#include <stdint.h>
#include <stdbool.h>
#include "report.h"
#include "usb.h"
#include "kbdef.h"

extern void indicators_next_effect();
extern void indicators_factory_reset();
extern void indicators_brightness_up();
extern void indicators_brightness_down();
extern void indicators_speed_up();
extern void indicators_speed_down();
extern void indicators_show_level(uint8_t n);
extern void indicators_softness_cycle(void);

static __xdata bool reset_mode_active;

/* Snap Tap — A↔D last-key-wins.
 * Only suppresses the opposite key (A or D). Never add/del anything else
 * (W/S must keep working for diagonals). */
static __xdata bool snaptap_on;
static __xdata bool a_phys;
static __xdata bool d_phys;

static __xdata bool winlock_on;

bool kb_snaptap_active(void)
{
    return snaptap_on;
}

uint8_t kb_snaptap_mode(void)
{
    return 0;
}

bool kb_winlock_active(void)
{
    return winlock_on;
}

static void probe_tap(uint8_t kc)
{
    add_key(kc);
    send_keyboard_report();
    del_key(kc);
    send_keyboard_report();
}

static uint8_t probe_digit_kc(uint8_t d)
{
    return (uint8_t)(d == 0 ? KC_0 : (KC_1 + d - 1));
}

void kb_set_snaptap(uint8_t on)
{
    bool next = on ? true : false;
    if (snaptap_on == next) {
        return;
    }
    snaptap_on = next;
    if (!snaptap_on) {
        /* Keys already match physical HID state; nothing to rebuild. */
        a_phys = 0;
        d_phys = 0;
    }
    indicators_show_level(snaptap_on ? 2 : 1);
}

void kb_set_snaptap_mode(uint8_t mode)
{
    (void)mode;
}

void kb_set_winlock(uint8_t on)
{
    bool next = on ? true : false;
    if (winlock_on == next) {
        return;
    }
    winlock_on = next;
    if (winlock_on) {
        del_mods((uint8_t)0x88);
        send_keyboard_report();
    }
    indicators_show_level(winlock_on ? 2 : 1);
}

/*
 * Minimal SOCD: on press, drop only the opposite of A/D, then let the normal
 * matrix path register this key (return true). On release of the winner while
 * the loser is still held, put the loser back.
 */
static bool snaptap_process_ad(uint16_t keycode, bool key_pressed)
{
    if (keycode == KC_A) {
        if (key_pressed) {
            a_phys = 1;
            if (d_phys) {
                del_key((uint8_t)KC_D);
                send_keyboard_report();
            }
            return true; /* normal add_key(A) — W/S untouched */
        }
        a_phys = 0;
        if (d_phys) {
            del_key((uint8_t)KC_A);
            add_key((uint8_t)KC_D);
            send_keyboard_report();
            return false;
        }
        return true; /* normal del_key(A) */
    }

    if (keycode == KC_D) {
        if (key_pressed) {
            d_phys = 1;
            if (a_phys) {
                del_key((uint8_t)KC_A);
                send_keyboard_report();
            }
            return true;
        }
        d_phys = 0;
        if (a_phys) {
            del_key((uint8_t)KC_D);
            add_key((uint8_t)KC_A);
            send_keyboard_report();
            return false;
        }
        return true;
    }

    return true;
}

bool kb_process_record(uint16_t keycode, bool key_pressed)
{
    if (keycode >= PROBE_BASE && keycode < PROBE_BASE + (MATRIX_ROWS * MATRIX_COLS)) {
        if (key_pressed) {
            uint8_t idx = (uint8_t)(keycode - PROBE_BASE);
            uint8_t r   = (uint8_t)(idx / MATRIX_COLS);
            uint8_t c   = (uint8_t)(idx % MATRIX_COLS);
            probe_tap((uint8_t)(KC_A + r));
            probe_tap(probe_digit_kc((uint8_t)(c / 10)));
            probe_tap(probe_digit_kc((uint8_t)(c % 10)));
            probe_tap(KC_SPACE);
        }
        return false;
    }

    if (winlock_on && (keycode == KC_LGUI || keycode == KC_RGUI)) {
        if (key_pressed) {
            del_mods((uint8_t)0x88);
            send_keyboard_report();
        }
        return false;
    }

    if (snaptap_on && (keycode == KC_A || keycode == KC_D)) {
        return snaptap_process_ad(keycode, key_pressed);
    }

    switch (keycode) {
        case RGB_FX_NEXT:
            if (key_pressed) indicators_next_effect();
            return false;
        case RGB_BRI_UP:
            if (key_pressed) indicators_brightness_up();
            return false;
        case RGB_BRI_DN:
            if (key_pressed) indicators_brightness_down();
            return false;
        case RGB_SPD_UP:
            if (key_pressed) indicators_speed_up();
            return false;
        case RGB_SPD_DN:
            if (key_pressed) indicators_speed_down();
            return false;
        case RESET_HOLD:
            reset_mode_active = key_pressed;
            return false;
        case FACT_RESET:
            if (key_pressed && reset_mode_active) indicators_factory_reset();
            return false;
        case SNAP_TAP_TOGGLE:
            if (key_pressed) {
                snaptap_on = !snaptap_on;
                if (!snaptap_on) {
                    a_phys = 0;
                    d_phys = 0;
                }
                indicators_show_level(snaptap_on ? 2 : 1);
            }
            return false;
        case WIN_LOCK_TOGGLE:
            if (key_pressed) {
                winlock_on = !winlock_on;
                if (winlock_on) {
                    del_mods((uint8_t)0x88);
                    send_keyboard_report();
                }
                indicators_show_level(winlock_on ? 2 : 1);
            }
            return false;
        case RGB_SOFT_CYCLE:
            if (key_pressed) indicators_softness_cycle();
            return false;
        default:
            return true;
    }
}

void kb_send_report(__xdata report_keyboard_t *report) { usb_send_report(report); }
void kb_send_nkro(__xdata report_nkro_t *report) { usb_send_nkro(report); }
void kb_send_extra(__xdata report_extra_t *report) { usb_send_extra(report); }
