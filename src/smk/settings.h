#pragma once

#include <stdint.h>

// Persistent user settings, stored as one blob in flash (see settings.c / flash.h).
//
// To save more things across power cycles, add fields to this struct. Keep it small
// (it shares one 512-byte flash sector) and note that a firmware reflash resets it to
// the defaults set by the caller before settings_load().
typedef struct {
    // main backlight (key matrix rows)
    uint8_t led_effect;     // selected backlight effect (or "off")
    uint8_t led_brightness; // 0..BRI_LEVELS-1
    uint8_t led_speed;      // 1..SPD_LEVELS
    // user / underglow lights (independent of main)
    uint8_t ul_effect;
    uint8_t ul_brightness;
    uint8_t ul_speed;
    uint8_t led_idle; // 0=30s, 1=60s, 2=120s, 3=240s, 4=off
} user_settings_t;

extern __xdata user_settings_t user_settings;

// Overwrites `user_settings` with the persisted values if a valid record exists;
// otherwise leaves it untouched. Set defaults into `user_settings` before calling.
void settings_load(void);

// Persists the current `user_settings` (no-op if it already matches what's stored).
void settings_save(void);
