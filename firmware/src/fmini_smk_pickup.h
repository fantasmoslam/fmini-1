/* SPDX-License-Identifier: GPL-3.0-only */
/* Pure target-crossing pickup state. No HAL, flash, allocation or audio state. */
#pragma once
#include <stdint.h>
typedef struct {
    uint32_t epoch;
    uint8_t physical, expected, ready, armed;
} fmini_pickup_t;

/* Return -1 to keep the parameter, otherwise the new 0..127 value.
 * The caller supplies the current parameter each time so MIDI/editor/knob edits
 * disarm this fader without affecting the other three. */
static int32_t fmini_pickup_poll(fmini_pickup_t *p, uint32_t physical,
                                uint32_t target, uint32_t epoch, int blocked)
{
    uint32_t previous;
    if (blocked || physical > 127u || target > 127u) {
        p->ready = p->armed = 0;
        return -1;
    }
    if (!p->ready || p->epoch != epoch || p->expected != target) {
        p->epoch = epoch;
        p->physical = (uint8_t)physical;
        p->expected = (uint8_t)target;
        p->ready = 1;
        p->armed = 0;
        return -1;
    }
    previous = p->physical;
    p->physical = (uint8_t)physical;
    if (previous == physical) return -1;
    if (!p->armed && ((previous <= target && physical >= target) ||
                      (previous >= target && physical <= target)))
        p->armed = 1;
    if (!p->armed || physical == target) return -1;
    p->expected = (uint8_t)physical;
    return (int32_t)physical;
}
