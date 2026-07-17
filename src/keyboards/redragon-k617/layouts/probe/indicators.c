#include "indicators.h"
#include "kbdef.h"
#include "pwm.h"
#include "settings.h"

#define PWM_CLK_DIV         0b010
#define PWM_SS_BIT          (1 << 3)
#define PWM_MOD_BIT         (1 << 4)
#define PWM_INT_ENABLE_BIT  (1 << 6)
#define PWM_MODE_ENABLE_BIT (1 << 7)

#define LED_ROWS LED_MATRIX_ROWS
#define LED_COLS LED_MATRIX_COLS

#define LED_SPEED_DEFAULT 4
#define LED_SPEED_MIN     1
#define LED_SPEED_MAX     16
#define LED_BRIGHTNESS_DEFAULT 255
#define LED_BRIGHTNESS_STEP    32

#define LED_DUTY(v) (uint16_t)(0x0400u - ((uint16_t)(v) << 2))
#define SCALE_BRI(v) (uint8_t)(((uint16_t)(uint8_t)(v) * user_settings.led_brightness) >> 8)

typedef enum {
    FX_RADIAL = 0,
    FX_HORIZONTAL,
    FX_VERTICAL,
    FX_SOLID,
    FX_BREATHE,
    FX_COUNT
} led_effect_t;

#define FX_OFF FX_COUNT

#include LED_GEOMETRY_HEADER
_Static_assert(LED_GEOMETRY_ROWS == LED_ROWS && LED_GEOMETRY_COLS == LED_COLS, "LED geometry size mismatch");

static __xdata uint8_t led_fb[LED_ROWS][3][LED_COLS];
static __xdata uint8_t led_row;
static __xdata uint8_t led_color;
static __xdata uint8_t led_phase;
static __xdata uint8_t regen_row;
static __xdata uint8_t regen_col;

void        indicators_pwm_enable();
void        indicators_pwm_disable();
static void apply_defaults();
static void led_regen_one();
static void led_enable_sink();
static void led_set_columns();
static void hsv_to_fb(uint8_t row, uint8_t col, uint8_t h, uint8_t s, uint8_t v);

static void apply_defaults()
{
    user_settings.led_effect     = FX_RADIAL;
    user_settings.led_brightness = LED_BRIGHTNESS_DEFAULT;
    user_settings.led_speed      = LED_SPEED_DEFAULT;
    user_settings.ul_effect      = 0;
    user_settings.ul_brightness  = 0;
    user_settings.ul_speed       = 1;
}

void indicators_factory_reset()
{
    apply_defaults();
    settings_save();
}

void indicators_start()
{
    led_row   = 0;
    led_color = 0;
    led_phase = 0;
    regen_row = 0;
    regen_col = 0;
    apply_defaults();
    settings_load();
    if (user_settings.led_effect > FX_OFF) {
        user_settings.led_effect = FX_RADIAL;
    }
    if (user_settings.led_speed < LED_SPEED_MIN || user_settings.led_speed > LED_SPEED_MAX) {
        user_settings.led_speed = LED_SPEED_DEFAULT;
    }

    indicators_pwm_enable();
}

void indicators_next_effect()
{
    if (++user_settings.led_effect > FX_OFF) {
        user_settings.led_effect = 0;
    }
    settings_save();
}

void indicators_brightness_up()
{
    if (user_settings.led_brightness > (uint8_t)(255 - LED_BRIGHTNESS_STEP)) {
        user_settings.led_brightness = 255;
    } else {
        user_settings.led_brightness = (uint8_t)(user_settings.led_brightness + LED_BRIGHTNESS_STEP);
    }
    settings_save();
}

void indicators_brightness_down()
{
    if (user_settings.led_brightness < LED_BRIGHTNESS_STEP) {
        user_settings.led_brightness = 0;
    } else {
        user_settings.led_brightness = (uint8_t)(user_settings.led_brightness - LED_BRIGHTNESS_STEP);
    }
    settings_save();
}

void indicators_speed_up()
{
    if (user_settings.led_speed < LED_SPEED_MAX) {
        user_settings.led_speed++;
    }
    settings_save();
}

void indicators_speed_down()
{
    if (user_settings.led_speed > LED_SPEED_MIN) {
        user_settings.led_speed--;
    }
    settings_save();
}

void indicators_pre_update()
{
    P0 &= (uint8_t)~(RGB_R2R_P0_2 | RGB_R0B_P0_3 | RGB_R0R_P0_4);
    P4 &= (uint8_t)~(RGB_R4B_P4_3 | RGB_R4R_P4_4 | RGB_R3R_P4_5 | RGB_R3B_P4_6);
    P5 &= (uint8_t)~(RGB_R2B_P5_7);
    P6 &= (uint8_t)~(RGB_R0G_P6_1 | RGB_R1G_P6_2 | RGB_R2G_P6_3 | RGB_R3G_P6_4 | RGB_R4G_P6_5 | RGB_R1B_P6_6 | RGB_R1R_P6_7);
    indicators_pwm_disable();
}

bool indicators_update_step(keyboard_state_t *keyboard, uint8_t current_step)
{
    keyboard;
    current_step;

    led_regen_one();

    if (user_settings.led_effect < FX_OFF) {
        led_enable_sink();
        led_set_columns();
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
    indicators_pwm_enable();
}

static void hsv_to_fb(uint8_t row, uint8_t col, uint8_t h, uint8_t s, uint8_t v)
{
    uint8_t r, g, b;
    v = SCALE_BRI(v);
    if (s == 0) {
        led_fb[row][0][col] = v;
        led_fb[row][1][col] = v;
        led_fb[row][2][col] = v;
        return;
    }
    // full sat wheel
    if (h < 85) {
        r = (uint8_t)(255 - h * 3);
        g = 0;
        b = (uint8_t)(h * 3);
    } else if (h < 170) {
        h = (uint8_t)(h - 85);
        r = 0;
        g = (uint8_t)(h * 3);
        b = (uint8_t)(255 - h * 3);
    } else {
        h = (uint8_t)(h - 170);
        r = (uint8_t)(h * 3);
        g = (uint8_t)(255 - h * 3);
        b = 0;
    }
    led_fb[row][0][col] = (uint8_t)(((uint16_t)r * v) >> 8);
    led_fb[row][1][col] = (uint8_t)(((uint16_t)g * v) >> 8);
    led_fb[row][2][col] = (uint8_t)(((uint16_t)b * v) >> 8);
}

static void led_regen_one()
{
    uint8_t h, v;

    if (user_settings.led_effect >= FX_OFF) {
        led_fb[regen_row][0][regen_col] = 0;
        led_fb[regen_row][1][regen_col] = 0;
        led_fb[regen_row][2][regen_col] = 0;
    } else if (user_settings.led_effect == FX_SOLID) {
        hsv_to_fb(regen_row, regen_col, 0, 0, 255);
    } else if (user_settings.led_effect == FX_BREATHE) {
        v = led_phase;
        if (v >= 128) {
            v = (uint8_t)(255 - v);
        }
        v = (uint8_t)(v << 1);
        hsv_to_fb(regen_row, regen_col, (uint8_t)(led_phase >> 1), 200, v);
    } else {
        switch (user_settings.led_effect) {
            case FX_HORIZONTAL:
                h = (uint8_t)(col_hue[regen_col] + led_phase);
                break;
            case FX_VERTICAL:
                h = (uint8_t)(row_hue[regen_row] + led_phase);
                break;
            case FX_RADIAL:
            default:
                h = (uint8_t)(radial_index[regen_row][regen_col] + led_phase);
                break;
        }
        hsv_to_fb(regen_row, regen_col, h, 255, 255);
    }

    if (++regen_col >= LED_COLS) {
        regen_col = 0;
        if (++regen_row >= LED_ROWS) {
            regen_row = 0;
            led_phase = (uint8_t)(led_phase + user_settings.led_speed);
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
            if (led_color == 0) RGB_R4R = 1;
            else if (led_color == 1) RGB_R4G = 1;
            else RGB_R4B = 1;
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
