#include "indicators.h"
#include "kbdef.h"
#include "pwm.h"
#include "settings.h"
#include "kb.h"
#include "matrix.h"
#include "layout.h"

/*
 * K617 RGB effects.
 * Cycle: Radial → Horizontal → Spiral → Split → Pinwheel → Wave → Chevron → Off
 *
 * Mux runs every PWM ISR. Effect colour is regenerated ONE key per ISR
 * (NuPhy-style) so the ISR never stalls — that stall was the daylight flicker.
 */

#define PWM_CLK_DIV         0b010
#define PWM_SS_BIT          (1 << 3)
#define PWM_INT_ENABLE_BIT  (1 << 6)
#define PWM_MODE_ENABLE_BIT (1 << 7)

#define LED_ROWS MATRIX_ROWS
#define LED_COLS MATRIX_COLS

#define BRI_LEVELS 5
#define SPD_LEVELS 5
#define BRI_DEFAULT 4 /* UI 5/5 */
#define SPD_DEFAULT 4 /* UI 4/5 */

/* Active brightness: lower duty = brighter. Off = period (matches LED_DUTY(0)). */
#define LED_DUTY(v) (uint16_t)(0x0400u - ((uint16_t)(v) << 2))
#define LED_DUTY_OFF ((uint16_t)0x0400u)

typedef enum {
    FX_RADIAL = 0,
    FX_HORIZONTAL,
    FX_SPIRAL,   /* QMK CYCLE_SPIRAL — radial + twist */
    FX_SPLIT,    /* QMK RAINBOW_PINWHEELS — halves counter-spin */
    FX_PINWHEEL, /* QMK CYCLE_PINWHEEL */
    FX_WAVE,
    FX_CHEVRON, /* QMK RAINBOW_MOVING_CHEVRON */
    FX_COUNT
} led_effect_t;

#define FX_OFF FX_COUNT

#include LED_GEOMETRY_HEADER
_Static_assert(LED_GEOMETRY_ROWS == LED_ROWS && LED_GEOMETRY_COLS == LED_COLS, "LED geometry size mismatch");

static const __code uint8_t bri_lut[BRI_LEVELS] = { 130, 175, 215, 245, 255 };
static const __code uint8_t spd_lut[SPD_LEVELS] = { 1, 2, 3, 4, 6 };
#define SOFT_LEVELS 5
#define SOFT_DEFAULT 0 /* UI 1/5 — sharpest */
static const __code uint8_t soft_sat[SOFT_LEVELS] = { 255, 220, 185, 145, 105 };
static const __code uint8_t soft_mix[SOFT_LEVELS] = { 0, 22, 48, 78, 110 };

#define BRI_SCALE() (bri_lut[user_settings.led_brightness < BRI_LEVELS ? user_settings.led_brightness : (BRI_LEVELS - 1)])
#define SCALE_BRI(v) (uint8_t)(((uint16_t)(uint8_t)(v) * BRI_SCALE()) >> 8)

#define LVL_ROW 0
#define LVL_COL0 1

static __xdata uint8_t led_fb[LED_ROWS][3][LED_COLS];
static __xdata uint8_t led_row;
static __xdata uint8_t led_color;
static __xdata uint8_t led_phase;
static __xdata uint8_t regen_r;
static __xdata uint8_t regen_c;
static __xdata uint8_t anim_div;
static __xdata uint8_t boot_active;
static __xdata uint8_t boot_phase;
static __xdata uint16_t boot_frames;
static __xdata uint8_t boot_fade;
static __xdata uint8_t caps_on;
static __xdata uint8_t level_frames;
static __xdata uint8_t level_n;
static __xdata uint16_t idle_count;
static __xdata uint8_t idle_asleep;
static __xdata uint8_t usb_suspended;
static __xdata uint8_t soft_level;
static __xdata uint8_t leds_blanked; /* hard-off: no sinks, duties=0 */

static __xdata uint8_t t_r, t_c, t_i, t_v, t_h, t_d, t_fx, t_tmp;
static __xdata uint8_t t_hr, t_hg, t_hb, t_hh;
static __xdata uint16_t t_kc;

#define BOOT_SPIN_FRAMES 520u
#define BOOT_RING_N     6
#define BOOT_TRAIL      155
#define BOOT_FADE_STEP  2
#define LEVEL_FRAMES    90
/* Boot/level: only advance on matrix wrap (step0) × this — same pace as pre-v33. */
#define ANIM_DIV        2
/* 1 full-keyboard regen ≈ 70 ISR ≈ 12ms → 30s ≈ 2500 passes */
#define IDLE_LEVELS 5
#define IDLE_DEFAULT 0
/* 30s, 60s, 120s, 240s, off(0) */
static const __code uint16_t idle_passes[IDLE_LEVELS] = { 2500u, 5000u, 10000u, 20000u, 0u };
/* Ring: Y U J N B G (no T, no H) */
static const __code uint8_t boot_r[BOOT_RING_N] = { 1, 1, 2, 3, 3, 2 };
static const __code uint8_t boot_c[BOOT_RING_N] = { 6, 7, 7, 6, 5, 5 };

void        indicators_pwm_enable();
void        indicators_pwm_disable();
static void apply_defaults();
static void led_enable_sink();
static void led_set_columns();
static void led_pwm_all_off();
static void led_blank_sinks();
static void led_hard_blank();
static void led_tick();
static void led_regen_one();
static void put_hsv(uint8_t row, uint8_t col, uint8_t h, uint8_t s, uint8_t v);
static void put_rgb_raw(uint8_t row, uint8_t col, uint8_t r, uint8_t g, uint8_t b);
static void put_boot_rgb(uint8_t row, uint8_t col, uint8_t r, uint8_t g, uint8_t b);
static void clear_all(void);
void        indicators_show_level(uint8_t n);
void        indicators_softness_cycle(void);
static void apply_caps_hw(void);
static void apply_status_overlays(void);
static void apply_overlay_cell(uint8_t row, uint8_t col);
static uint8_t spd_step(void);
static uint8_t tri8(uint8_t x);
static uint8_t leds_need_scan(void);

#define SNAP_A_ROW 2
#define SNAP_A_COL 1
#define SNAP_D_ROW 2
#define SNAP_D_COL 3
#define SNAP_W_ROW 1
#define SNAP_W_COL 2
#define SNAP_S_ROW 2
#define SNAP_S_COL 2
#define WIN_ROW    4
#define WIN_COL    1
#define FN_ROW     4
#define FN_COL     9
#define LAYER_FL   1

static uint8_t spd_step(void)
{
    uint8_t s = user_settings.led_speed;
    if (s < 1 || s > SPD_LEVELS) {
        s = SPD_DEFAULT;
    }
    return spd_lut[s - 1];
}

static uint8_t tri8(uint8_t x)
{
    if (x >= 128) {
        x = (uint8_t)(255 - x);
    }
    return (uint8_t)(x << 1);
}

static void apply_defaults()
{
    user_settings.led_effect     = FX_WAVE; /* factory default */
    user_settings.led_brightness = BRI_DEFAULT;
    user_settings.led_speed      = SPD_DEFAULT;
    user_settings.ul_effect      = 0;
    user_settings.ul_brightness  = 0;
    user_settings.ul_speed       = 1;
    user_settings.led_idle       = IDLE_DEFAULT;
}

void indicators_show_level(uint8_t n)
{
    if (n < 1) {
        n = 1;
    }
    if (n > BRI_LEVELS) {
        n = BRI_LEVELS;
    }
    level_n      = n;
    level_frames = LEVEL_FRAMES;
}

void indicators_softness_cycle(void)
{
    soft_level = (uint8_t)((soft_level + 1) % SOFT_LEVELS);
    indicators_show_level((uint8_t)(soft_level + 1));
}

#define show_level indicators_show_level

void indicators_activity(void)
{
    /* Wake from idle blanking only. Never clear USB suspend here — that caused
     * rainbow flashes when keys were pressed while the PC was powered off. */
    idle_count  = 0;
    idle_asleep = 0;
    if (!usb_suspended) {
        leds_blanked = 0;
    }
}

void indicators_suspend(void)
{
    usb_suspended = 1;
    led_hard_blank();
}

void indicators_wake(void)
{
    /* USB bus resume only. Must NOT clear idle sleep — SOF fires every 1ms
     * while the PC is on, which would instantly undo idle blanking. */
    if (!usb_suspended) {
        return;
    }
    usb_suspended = 0;
    idle_asleep   = 0;
    idle_count    = 0;
    leds_blanked  = 0;
}

void indicators_factory_reset()
{
    apply_defaults();
    settings_save();
}

void indicators_start()
{
    led_row        = 0;
    led_color      = 0;
    led_phase      = 0;
    regen_r        = 0;
    regen_c        = 0;
    anim_div       = 0;
    boot_active    = 1;
    boot_phase     = 0;
    boot_frames    = 0;
    boot_fade      = 255;
    caps_on        = 0;
    level_frames   = 0;
    level_n        = 1;
    idle_count     = 0;
    idle_asleep    = 0;
    usb_suspended  = 0;
    soft_level     = SOFT_DEFAULT;
    leds_blanked   = 0;

    apply_defaults();
    settings_load();
    if (user_settings.led_effect > FX_OFF) {
        user_settings.led_effect = FX_WAVE;
    }
    if (user_settings.led_brightness >= BRI_LEVELS) {
        user_settings.led_brightness = BRI_DEFAULT;
    }
    if (user_settings.led_speed < 1 || user_settings.led_speed > SPD_LEVELS) {
        user_settings.led_speed = SPD_DEFAULT;
    }
    if (user_settings.led_idle >= IDLE_LEVELS) {
        user_settings.led_idle = IDLE_DEFAULT;
    }
    /* One-shot migrate: factory default is now Wave (was Radial). */
    if (user_settings.ul_effect != 53) {
        user_settings.led_effect = FX_WAVE;
        user_settings.ul_effect  = 53;
        settings_save();
    }

    LED_CAPS = 1;
    clear_all();
    led_pwm_all_off();
    indicators_pwm_enable();
}

void indicators_next_effect()
{
    if (++user_settings.led_effect > FX_OFF) {
        user_settings.led_effect = 0;
    }
    settings_save();
}

void indicators_set_effect(uint8_t fx)
{
    if (fx > FX_OFF) {
        fx = FX_OFF;
    }
    user_settings.led_effect = fx;
    idle_count  = 0;
    idle_asleep = 0;
    leds_blanked = 0;
}

void indicators_set_brightness(uint8_t level)
{
    if (level >= BRI_LEVELS) {
        level = (uint8_t)(BRI_LEVELS - 1);
    }
    user_settings.led_brightness = level;
    idle_count   = 0;
    idle_asleep  = 0;
    leds_blanked = 0;
    /* No level overlay — WebHID needs the change visible on the matrix immediately. */
}

void indicators_set_speed(uint8_t level)
{
    if (level < 1) {
        level = 1;
    }
    if (level > SPD_LEVELS) {
        level = SPD_LEVELS;
    }
    user_settings.led_speed = level;
    idle_count   = 0;
    idle_asleep  = 0;
    leds_blanked = 0;
}

void indicators_set_softness(uint8_t level)
{
    if (level >= SOFT_LEVELS) {
        level = (uint8_t)(SOFT_LEVELS - 1);
    }
    soft_level   = level;
    idle_count   = 0;
    idle_asleep  = 0;
    if (!usb_suspended) {
        leds_blanked = 0;
    }
    /* Force a full regen pass so softness hits every key ASAP. */
    regen_r = 0;
    regen_c = 0;
}

void indicators_set_idle(uint8_t level)
{
    if (level >= IDLE_LEVELS) {
        level = (uint8_t)(IDLE_LEVELS - 1);
    }
    user_settings.led_idle = level;
    idle_count  = 0;
    idle_asleep = 0;
    if (!usb_suspended) {
        leds_blanked = 0;
    }
}

uint8_t indicators_get_effect(void)
{
    return user_settings.led_effect;
}

uint8_t indicators_get_brightness(void)
{
    return user_settings.led_brightness;
}

uint8_t indicators_get_speed(void)
{
    return user_settings.led_speed;
}

uint8_t indicators_get_softness(void)
{
    return soft_level;
}

uint8_t indicators_get_idle(void)
{
    return user_settings.led_idle;
}

void indicators_brightness_up()
{
    user_settings.led_brightness = (uint8_t)((user_settings.led_brightness + 1) % BRI_LEVELS);
    show_level((uint8_t)(user_settings.led_brightness + 1));
}

void indicators_brightness_down()
{
    indicators_brightness_up();
}

void indicators_speed_up()
{
    user_settings.led_speed = (uint8_t)(user_settings.led_speed >= SPD_LEVELS ? 1 : user_settings.led_speed + 1);
    show_level(user_settings.led_speed);
}

void indicators_speed_down()
{
    indicators_speed_up();
}

void indicators_pre_update()
{
    led_blank_sinks();
    indicators_pwm_disable();
}

static void led_blank_sinks(void)
{
    P0 &= (uint8_t)~(RGB_R2R_P0_2 | RGB_R0B_P0_3 | RGB_R0R_P0_4);
    P4 &= (uint8_t)~(RGB_R4B_P4_3 | RGB_R4R_P4_4 | RGB_R3R_P4_5 | RGB_R3B_P4_6);
    P5 &= (uint8_t)~(RGB_R2B_P5_7);
    P6 &= (uint8_t)~(RGB_R0G_P6_1 | RGB_R1G_P6_2 | RGB_R2G_P6_3 | RGB_R3G_P6_4 | RGB_R4G_P6_5 | RGB_R1B_P6_6 | RGB_R1R_P6_7);
}

static void led_pwm_all_off(void)
{
    SET_PWM_DUTY_2(LED_PWM_C0, LED_DUTY_OFF);
    SET_PWM_DUTY_2(LED_PWM_C1, LED_DUTY_OFF);
    SET_PWM_DUTY_2(LED_PWM_C2, LED_DUTY_OFF);
    SET_PWM_DUTY_2(LED_PWM_C3, LED_DUTY_OFF);
    SET_PWM_DUTY_2(LED_PWM_C4, LED_DUTY_OFF);
    SET_PWM_DUTY_2(LED_PWM_C5, LED_DUTY_OFF);
    SET_PWM_DUTY_2(LED_PWM_C6, LED_DUTY_OFF);
    SET_PWM_DUTY_2(LED_PWM_C7, LED_DUTY_OFF);
    SET_PWM_DUTY_2(LED_PWM_C8, LED_DUTY_OFF);
    SET_PWM_DUTY_2(LED_PWM_C9, LED_DUTY_OFF);
    SET_PWM_DUTY_2(LED_PWM_C10, LED_DUTY_OFF);
    SET_PWM_DUTY_2(LED_PWM_C11, LED_DUTY_OFF);
    SET_PWM_DUTY_2(LED_PWM_C12, LED_DUTY_OFF);
    SET_PWM_DUTY_2(LED_PWM_C13, LED_DUTY_OFF);
}

static void led_hard_blank(void)
{
    clear_all();
    led_blank_sinks();
    led_pwm_all_off();
    leds_blanked = 1;
}

static void apply_caps_hw(void)
{
    LED_CAPS = caps_on ? 0 : 1;
}

static void apply_status_overlays(void)
{
    if (matrix_get_layer() == LAYER_FL) {
        put_rgb_raw(FN_ROW, FN_COL, 0, 200, 255);
        for (t_r = 0; t_r < LED_ROWS; t_r++) {
            for (t_c = 0; t_c < LED_COLS; t_c++) {
                t_kc = keymaps[LAYER_FL][t_r][t_c];
                if (t_kc >= RGB_FX_NEXT && t_kc < KB_SAFE_RANGE) {
                    put_rgb_raw(t_r, t_c, 0, 200, 255);
                }
            }
        }
    }

    if (kb_snaptap_active()) {
        put_rgb_raw(SNAP_W_ROW, SNAP_W_COL, 255, 0, 0);
        put_rgb_raw(SNAP_A_ROW, SNAP_A_COL, 255, 0, 0);
        put_rgb_raw(SNAP_S_ROW, SNAP_S_COL, 255, 0, 0);
        put_rgb_raw(SNAP_D_ROW, SNAP_D_COL, 255, 0, 0);
    }

    if (kb_winlock_active()) {
        put_rgb_raw(WIN_ROW, WIN_COL, 255, 140, 0);
    }
}

/* Per-cell overlay so spread regen still shows Snap/Win/Fn status. */
static void apply_overlay_cell(uint8_t row, uint8_t col)
{
    if (matrix_get_layer() == LAYER_FL) {
        if (row == FN_ROW && col == FN_COL) {
            put_rgb_raw(row, col, 0, 200, 255);
            return;
        }
        t_kc = keymaps[LAYER_FL][row][col];
        if (t_kc >= RGB_FX_NEXT && t_kc < KB_SAFE_RANGE) {
            put_rgb_raw(row, col, 0, 200, 255);
            return;
        }
    }

    if (kb_snaptap_active()) {
        if ((row == SNAP_W_ROW && col == SNAP_W_COL) || (row == SNAP_A_ROW && col == SNAP_A_COL)
            || (row == SNAP_S_ROW && col == SNAP_S_COL) || (row == SNAP_D_ROW && col == SNAP_D_COL)) {
            put_rgb_raw(row, col, 255, 0, 0);
            return;
        }
    }

    if (kb_winlock_active() && row == WIN_ROW && col == WIN_COL) {
        put_rgb_raw(row, col, 255, 140, 0);
    }
}

static uint8_t leds_need_scan(void)
{
    if (usb_suspended || idle_asleep || leds_blanked) {
        return 0;
    }
    return (uint8_t)(boot_active || level_frames || kb_snaptap_active() || kb_winlock_active()
                     || matrix_get_layer() || user_settings.led_effect < FX_OFF);
}

bool indicators_update_step(keyboard_state_t *keyboard, uint8_t current_step)
{
    caps_on = (uint8_t)((keyboard->led_state >> 1) & 1);
    apply_caps_hw();

    if (usb_suspended || idle_asleep) {
        if (!leds_blanked) {
            led_hard_blank();
        } else {
            led_pwm_all_off();
        }
    } else {
        /* Boot/level: old cadence (once per matrix scan × ANIM_DIV). Effects: 1 key/ISR. */
        if (boot_active || level_frames) {
            if (current_step == 0) {
                if (++anim_div >= ANIM_DIV) {
                    anim_div = 0;
                    led_tick();
                }
            }
        } else {
            led_tick();
        }

        if (leds_need_scan()) {
            led_enable_sink();
            led_set_columns();
        } else {
            led_pwm_all_off();
        }
    }

    if (++led_color >= 3) {
        led_color = 0;
        if (++led_row >= LED_ROWS) {
            led_row = 0;
        }
    }
    return false;
}

void indicators_post_update()
{
    PWM00CON &= ~(1 << 5);
    /* Keep PWM0 ticking for matrix scan even when RGB is blanked. */
    indicators_pwm_enable();
}

static void clear_all(void)
{
    for (t_r = 0; t_r < LED_ROWS; t_r++) {
        for (t_i = 0; t_i < 3; t_i++) {
            for (t_c = 0; t_c < LED_COLS; t_c++) {
                led_fb[t_r][t_i][t_c] = 0;
            }
        }
    }
}

static void put_rgb_raw(uint8_t row, uint8_t col, uint8_t r, uint8_t g, uint8_t b)
{
    /* True black must stay black — soft_mix on (0,0,0) used to leave a grey glow. */
    if (r == 0 && g == 0 && b == 0) {
        led_fb[row][0][col] = 0;
        led_fb[row][1][col] = 0;
        led_fb[row][2][col] = 0;
        return;
    }
    {
        uint8_t m = soft_mix[soft_level < SOFT_LEVELS ? soft_level : SOFT_DEFAULT];
        uint8_t k = (uint8_t)(255u - m);
        led_fb[row][0][col] = (uint8_t)(((uint16_t)r * k + (uint16_t)m * 255u) >> 8);
        led_fb[row][1][col] = (uint8_t)(((uint16_t)g * k + (uint16_t)m * 255u) >> 8);
        led_fb[row][2][col] = (uint8_t)(((uint16_t)b * k + (uint16_t)m * 255u) >> 8);
    }
}

static void put_boot_rgb(uint8_t row, uint8_t col, uint8_t r, uint8_t g, uint8_t b)
{
    led_fb[row][0][col] = r;
    led_fb[row][1][col] = g;
    led_fb[row][2][col] = b;
}

static void put_hsv(uint8_t row, uint8_t col, uint8_t h, uint8_t s, uint8_t v)
{
    v = SCALE_BRI(v);
    s = (uint8_t)(((uint16_t)s * soft_sat[soft_level < SOFT_LEVELS ? soft_level : SOFT_DEFAULT]) >> 8);
    if (s == 0) {
        put_rgb_raw(row, col, v, v, v);
        return;
    }
    t_hh = h;
    if (t_hh < 85) {
        t_hr = (uint8_t)(255 - t_hh * 3);
        t_hg = 0;
        t_hb = (uint8_t)(t_hh * 3);
    } else if (t_hh < 170) {
        t_hh = (uint8_t)(t_hh - 85);
        t_hr = 0;
        t_hg = (uint8_t)(t_hh * 3);
        t_hb = (uint8_t)(255 - t_hh * 3);
    } else {
        t_hh = (uint8_t)(t_hh - 170);
        t_hr = (uint8_t)(t_hh * 3);
        t_hg = (uint8_t)(255 - t_hh * 3);
        t_hb = 0;
    }
    put_rgb_raw(row, col,
                (uint8_t)(((uint16_t)t_hr * v) >> 8),
                (uint8_t)(((uint16_t)t_hg * v) >> 8),
                (uint8_t)(((uint16_t)t_hb * v) >> 8));
}

/* Cheap full-frame paths (boot / level) + one-key regen for effects. */
static void led_tick(void)
{
    if (level_frames) {
        clear_all();
        for (t_i = 0; t_i < level_n; t_i++) {
            put_rgb_raw(LVL_ROW, (uint8_t)(LVL_COL0 + t_i), 255, 255, 255);
        }
        apply_status_overlays();
        level_frames--;
        return;
    }

    if (boot_active) {
        clear_all();
        for (t_i = 0; t_i < BOOT_RING_N; t_i++) {
            t_tmp = (uint8_t)((uint16_t)t_i * 256u / BOOT_RING_N);
            t_d   = (uint8_t)(boot_phase - t_tmp);
            if (t_d < BOOT_TRAIL) {
                t_v = (uint8_t)(255u - (uint16_t)t_d * 255u / BOOT_TRAIL);
                t_v = (uint8_t)(((uint16_t)t_v * boot_fade) >> 8);
                if (t_v > 2) {
                    put_boot_rgb(boot_r[t_i], boot_c[t_i],
                                 (uint8_t)((t_v * 6) >> 3),
                                 (uint8_t)((t_v * 7) >> 3),
                                 t_v);
                }
            }
        }
        boot_phase = (uint8_t)(boot_phase + 2);
        if (boot_frames < BOOT_SPIN_FRAMES) {
            boot_frames++;
        } else if (boot_fade > BOOT_FADE_STEP) {
            boot_fade = (uint8_t)(boot_fade - BOOT_FADE_STEP);
        } else {
            boot_fade   = 0;
            boot_active = 0;
            clear_all();
        }
        return;
    }

    led_regen_one();
}

/*
 * Per-key LED color links: compute the effect as the SOURCE cell, write to
 * the destination cell. Applies to every animation (not only wave).
 * Row4 R↔B sink swap is separate (led_enable_sink).
 *
 *   NUBS <>     ← LShift (3,0)
 *   LCtrl       ← LShift (3,0)
 *   Win         ← Z      (3,1)
 *   LAlt        ← X      (3,2)
 *   Space span  ← B      (3,5)   cols 3..7 under the bar
 *   AltGr       ← ç      (3,9)   DOT
 *   Fn          ← .      (3,10)  SLSH
 *   App / RCtrl ← RShift (3,13)
 */
static void led_regen_one(void)
{
    t_fx = user_settings.led_effect;

    /* Sample cell (t_r,t_c); default = this cell. */
    t_r = regen_r;
    t_c = regen_c;
    if (regen_r == 3) {
        if (regen_c == 11 || regen_c == 12) {
            t_r = 3;
            t_c = 0; /* NUBS ← LShift */
        }
    } else if (regen_r == 4) {
        if (regen_c == 0) {
            t_r = 3;
            t_c = 0; /* LCtrl ← LShift */
        } else if (regen_c == 1) {
            t_r = 3;
            t_c = 1; /* Win ← Z */
        } else if (regen_c == 2) {
            t_r = 3;
            t_c = 2; /* LAlt ← X */
        } else if (regen_c >= 3 && regen_c <= 7) {
            t_r = 3;
            t_c = 5; /* Space ← B */
        } else if (regen_c == 8) {
            t_r = 3;
            t_c = 9; /* AltGr ← ç */
        } else if (regen_c == 9) {
            t_r = 3;
            t_c = 10; /* Fn ← . */
        } else if (regen_c >= 10) {
            t_r = 3;
            t_c = 13; /* App / RCtrl ← RShift */
        }
    }

    if (t_fx >= FX_OFF) {
        led_fb[regen_r][0][regen_c] = 0;
        led_fb[regen_r][1][regen_c] = 0;
        led_fb[regen_r][2][regen_c] = 0;
    } else if (t_fx == FX_RADIAL) {
        t_h = (uint8_t)(radial_index[t_r][t_c] + led_phase);
        put_hsv(regen_r, regen_c, t_h, 255, 255);
    } else if (t_fx == FX_HORIZONTAL) {
        t_h = (uint8_t)(col_hue[t_c] + led_phase);
        put_hsv(regen_r, regen_c, t_h, 255, 255);
    } else if (t_fx == FX_SPIRAL) {
        t_h = (uint8_t)(led_phase + radial_index[t_r][t_c] + (t_c * 14));
        put_hsv(regen_r, regen_c, t_h, 255, 255);
    } else if (t_fx == FX_SPLIT) {
        if (t_c < 7) {
            t_h = (uint8_t)(led_phase + (t_c * 22) - (t_r * 40));
        } else {
            t_h = (uint8_t)((uint8_t)(0 - led_phase) + (t_c * 22) + (t_r * 40));
        }
        put_hsv(regen_r, regen_c, t_h, 255, 255);
    } else if (t_fx == FX_PINWHEEL) {
        t_h = (uint8_t)(led_phase + (uint8_t)(t_c * 19) - (uint8_t)(t_r * 37));
        put_hsv(regen_r, regen_c, t_h, 255, 255);
    } else if (t_fx == FX_WAVE) {
        /* Left→right rainbow wave: full value always — hue travel only, never dim/off. */
        t_h = (uint8_t)(led_phase - (uint8_t)(t_c * 18u));
        put_hsv(regen_r, regen_c, t_h, 255, 255);
    } else {
        /* FX_CHEVRON */
        t_d = (t_c > 6) ? (uint8_t)(t_c - 6) : (uint8_t)(6 - t_c);
        t_h = (uint8_t)(led_phase + (t_d * 28) + (t_r * 22));
        put_hsv(regen_r, regen_c, t_h, 255, 255);
    }

    apply_overlay_cell(regen_r, regen_c);

    if (++regen_c >= LED_COLS) {
        regen_c = 0;
        if (++regen_r >= LED_ROWS) {
            regen_r = 0;
            if (t_fx < FX_OFF) {
                led_phase = (uint8_t)(led_phase + spd_step());
                t_tmp = user_settings.led_idle;
                if (t_tmp >= IDLE_LEVELS) {
                    t_tmp = IDLE_DEFAULT;
                }
                t_kc = idle_passes[t_tmp];
                if (t_kc != 0) {
                    if (idle_count < t_kc) {
                        idle_count++;
                    } else {
                        idle_asleep = 1;
                        led_hard_blank();
                    }
                }
            }
        }
    }
}

static void led_enable_sink()
{
    switch (led_row) {
        case 0:
            if (led_color == 0) RGB_R0R = 1;
            else if (led_color == 1) RGB_R0G = 1;
            else RGB_R0B = 1;
            break;
        case 1:
            if (led_color == 0) RGB_R1R = 1;
            else if (led_color == 1) RGB_R1G = 1;
            else RGB_R1B = 1;
            break;
        case 2:
            if (led_color == 0) RGB_R2R = 1;
            else if (led_color == 1) RGB_R2G = 1;
            else RGB_R2B = 1;
            break;
        case 3:
            if (led_color == 0) RGB_R3R = 1;
            else if (led_color == 1) RGB_R3G = 1;
            else RGB_R3B = 1;
            break;
        case 4:
            /*
             * K617 bottom LED row: R and B sinks are swapped on the PCB vs
             * rows 0–3. Without this, cyan→yellow, blue→red, orange→cyan —
             * exactly the “bottom row out of sync” look in photos.
             */
            if (led_color == 0) RGB_R4B = 1;
            else if (led_color == 1) RGB_R4G = 1;
            else RGB_R4R = 1;
            break;
    }
}

static void led_set_columns()
{
    __xdata uint8_t *fb = led_fb[led_row][led_color];
    SET_PWM_DUTY_2(LED_PWM_C0, LED_DUTY(fb[0]));
    SET_PWM_DUTY_2(LED_PWM_C1, LED_DUTY(fb[1]));
    SET_PWM_DUTY_2(LED_PWM_C2, LED_DUTY(fb[2]));
    SET_PWM_DUTY_2(LED_PWM_C3, LED_DUTY(fb[3]));
    SET_PWM_DUTY_2(LED_PWM_C4, LED_DUTY(fb[4]));
    SET_PWM_DUTY_2(LED_PWM_C5, LED_DUTY(fb[5]));
    SET_PWM_DUTY_2(LED_PWM_C6, LED_DUTY(fb[6]));
    SET_PWM_DUTY_2(LED_PWM_C7, LED_DUTY(fb[7]));
    SET_PWM_DUTY_2(LED_PWM_C8, LED_DUTY(fb[8]));
    SET_PWM_DUTY_2(LED_PWM_C9, LED_DUTY(fb[9]));
    SET_PWM_DUTY_2(LED_PWM_C10, LED_DUTY(fb[10]));
    SET_PWM_DUTY_2(LED_PWM_C11, LED_DUTY(fb[11]));
    SET_PWM_DUTY_2(LED_PWM_C12, LED_DUTY(fb[12]));
    SET_PWM_DUTY_2(LED_PWM_C13, LED_DUTY(fb[13]));
}

void indicators_pwm_enable()
{
    PWM00CON = (uint8_t)(PWM_MODE_ENABLE_BIT | PWM_INT_ENABLE_BIT | PWM_SS_BIT | PWM_CLK_DIV);
    PWM01CON = PWM_SS_BIT;
    PWM02CON = PWM_SS_BIT;
    PWM03CON = PWM_SS_BIT;
    PWM04CON = PWM_SS_BIT;
    PWM05CON = PWM_SS_BIT;

    PWM10CON = (uint8_t)(PWM_MODE_ENABLE_BIT | PWM_SS_BIT | PWM_CLK_DIV);
    PWM11CON = PWM_SS_BIT;
    PWM12CON = PWM_SS_BIT;
    PWM13CON = PWM_SS_BIT;
    PWM14CON = PWM_SS_BIT;
    PWM15CON = PWM_SS_BIT;

    PWM20CON = (uint8_t)(PWM_MODE_ENABLE_BIT | PWM_SS_BIT | PWM_CLK_DIV);
    PWM25CON = PWM_SS_BIT;

    PWM40CON = (uint8_t)(PWM_MODE_ENABLE_BIT | PWM_SS_BIT | PWM_CLK_DIV);
    PWM41CON = PWM_SS_BIT;
    PWM42CON = PWM_SS_BIT;
}

void indicators_pwm_disable()
{
    PWM00CON = (uint8_t)(PWM_CLK_DIV);
    PWM01CON = 0;
    PWM02CON = 0;
    PWM03CON = 0;
    PWM04CON = 0;
    PWM05CON = 0;

    PWM10CON = (uint8_t)(PWM_CLK_DIV);
    PWM11CON = 0;
    PWM12CON = 0;
    PWM13CON = 0;
    PWM14CON = 0;
    PWM15CON = 0;

    PWM20CON = (uint8_t)(PWM_CLK_DIV);
    PWM25CON = 0;

    PWM40CON = (uint8_t)(PWM_CLK_DIV);
    PWM41CON = 0;
    PWM42CON = 0;
}
