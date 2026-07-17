#include "webhid.h"
#include "report.h"
#include "settings.h"
#include "kb.h"
#include <stdint.h>

extern void indicators_next_effect(void);
extern void indicators_set_effect(uint8_t fx);
extern void indicators_set_brightness(uint8_t level);
extern void indicators_set_speed(uint8_t level);
extern void indicators_set_softness(uint8_t level);
extern void indicators_set_idle(uint8_t level);
extern uint8_t indicators_get_effect(void);
extern uint8_t indicators_get_brightness(void);
extern uint8_t indicators_get_speed(void);
extern uint8_t indicators_get_softness(void);
extern uint8_t indicators_get_idle(void);
extern void kb_set_snaptap(uint8_t on);
extern void kb_set_winlock(uint8_t on);

void webhid_fill_status(__xdata uint8_t *buf)
{
    uint8_t flags = 0;
    uint8_t idle  = indicators_get_idle();

    if (kb_snaptap_active()) {
        flags |= 0x01;
    }
    if (kb_winlock_active()) {
        flags |= 0x02;
    }
    /* bits 4..6 = idle level 0..4 */
    if (idle > 4) {
        idle = 0;
    }
    flags |= (uint8_t)((idle & 0x07) << 4);

    buf[0] = REPORT_ID_VENDOR;
    buf[1] = VENDOR_MAGIC;
    buf[2] = VENDOR_PROTO_VER;
    buf[3] = flags;
    buf[4] = indicators_get_effect();
    buf[5] = indicators_get_brightness();
    buf[6] = indicators_get_speed();
    buf[7] = indicators_get_softness();
}

void webhid_handle_set(__xdata uint8_t *buf, uint8_t len)
{
    uint8_t cmd;
    uint8_t val;
    uint8_t off = 0;

    if (len < 1) {
        return;
    }

    if (buf[0] == REPORT_ID_VENDOR) {
        if (len < 2) {
            return;
        }
        off = 1;
    }

    cmd = buf[off];
    val = (len > (uint8_t)(off + 1)) ? buf[off + 1] : 0;

    switch (cmd) {
        case VCMD_SET_EFFECT:
            indicators_set_effect(val);
            break;
        case VCMD_SET_BRIGHTNESS:
            indicators_set_brightness(val);
            break;
        case VCMD_SET_SPEED:
            indicators_set_speed(val);
            break;
        case VCMD_SET_SOFTNESS:
            indicators_set_softness(val);
            break;
        case VCMD_SET_SNAPTAP:
            kb_set_snaptap(val ? 1 : 0);
            break;
        case VCMD_SET_WINLOCK:
            kb_set_winlock(val ? 1 : 0);
            break;
        case VCMD_SET_IDLE:
            indicators_set_idle(val);
            break;
        case VCMD_NEXT_EFFECT:
            indicators_next_effect();
            break;
        case VCMD_SAVE:
            settings_save();
            break;
        default:
            break;
    }
}
