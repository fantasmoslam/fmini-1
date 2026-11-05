/* SPDX-License-Identifier: GPL-3.0-only */
/* Test HAL: no register access. Production SMK input remains in smk_input.h. */
#pragma once
#define FM1_NCOL 16u
#define FM1_NKEY 37u
#define FM1_NBTN 18u
#define FM1_NENC 8u
#define SMK_NPAD 16u
#define SMK_NLED 19u
#define SMK_KEY_F3 5u
enum { SMK_BTN_KNOB_BANK, SMK_BTN_FADER_BANK, SMK_BTN_LEFT, SMK_BTN_RIGHT };
static struct {
    volatile uint32_t notes, buttons, frames;
    uint64_t keys;
    uint8_t key_vel[37], fader[4];
} fm1_in;
static uint8_t smk_pad_grid;
static uint8_t smk_pad_piano;
static uint8_t smk_pad_steps;
static uint16_t smk_step_held, smk_step_down;
static uint32_t smk_steps_take(void) { uint32_t d=smk_step_down; smk_step_down=0; return d; }
static uint32_t padled_hold_until;
/* Compatibility fixtures for unused FM-1-only assertions in ui_test.c. */
static uint8_t led_pos[64];
static const uint8_t SMK_WHITE_NOTE[16]={0,2,4,6,7,9,11,12,14,16,18,19,21,23,24,26};
#define SMK_PAD_OF_WHITE(w) (smk_pad_grid?(w):(w)<8u?(w)+8u:(w)-8u)
#define SMK_WHITE_OF_PAD(p) (smk_pad_grid?(p):(p)<8u?(p)+8u:(p)-8u)
static uint8_t host_pad_rgb[16][3];
static void padled_set(uint32_t p,uint8_t r,uint8_t g,uint8_t b) {
    if(p>=16u) abort();
    host_pad_rgb[p][0]=r; host_pad_rgb[p][1]=g; host_pad_rgb[p][2]=b;
}
static int32_t host_ext_event=-1;
static int32_t smk_extkey_take(void) { int32_t e=host_ext_event; host_ext_event=-1; return e; }
