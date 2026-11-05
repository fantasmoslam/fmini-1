/* SPDX-License-Identifier: GPL-3.0-only */
/* FMini's performance controls; included only on the SMK, before fa_scene. */
#include "fmini_smk_pickup.h"
static fmini_pickup_t fa_smk_pickup[4];
static const uint8_t FA_SMK_ENV[4] = {P_ATK, P_DEC, P_SUS, P_REL};
static uint8_t fa_smk_shift;
static void fa_goto(uint32_t pg);
static void fa_sg_input(uint32_t pressed);
static void fa_smk_disarm(void)
{
    uint32_t i;
    for (i=0;i<4u;i++) fa_smk_pickup[i].ready=fa_smk_pickup[i].armed=0;
}
static void fa_smk_set(uint32_t id, int32_t value)
{
    track_t *t=TSEL;
    const param_desc_t *d=track_desc(t,id);
    value=clamp(value,d->min,d->max);
    if (t->p[id]==value) return;
    edit_undo_take(t,0x7000u|id,UNDO_SOUND);
    t->p[id]=(int16_t)value;
    motion_capture(t,id,(int16_t)value);
    ml_knob(t,id);
    motion_undo_done(t);
    ui.force=1;
}
static void fa_smk_touch(uint32_t which)
{
    fa_smk_hot=(uint8_t)which;
    fa_smk_until=fm1_ms+1200u;
    scrn.idle=fm1_ms;
    ui.force=1;
}
static void fa_smk_controls(uint32_t pressed)
{
    uint32_t i, shift=(fm1_in.buttons>>SMK_BTN_KNOB_BANK)&1u;
    int blocked=fa_sv || fa_gl || kb_boot_hold || kb_asleep ||
                fb_rs_state()==FB_RS_ASK || fb_rs_state()==FB_RS_OK;
    int normal=!blocked && !fa_ar && !fa_sq && !fa_sc && !fmg_fx && !fa_tp;
    if (fa_smk_until && (int32_t)(fm1_ms-fa_smk_until)>=0) {
        fa_smk_until=0; fa_smk_hot=0; ui.force=1;
    }
    if (shift!=fa_smk_shift) { fa_smk_shift=(uint8_t)shift; ui.force=1; }
    smk_pad_grid=(uint8_t)(fa_sq!=0u);
    if (normal && (pressed & (1u<<SMK_BTN_LEFT))) fa_goto((fa_page+FA_NPAGE-1u)%FA_NPAGE);
    if (normal && (pressed & (1u<<SMK_BTN_RIGHT))) fa_goto((fa_page+1u)%FA_NPAGE);
    if (normal && (pressed & (1u<<SMK_BTN_FADER_BANK))) fa_goto(5u);
    if (normal && !shift) {
        int32_t a=panel_enc(EN_SELECT), b=panel_enc(EN_ALGO);
        if (a) { fa_smk_set(P_E0,(int32_t)TSEL->p[P_E0]+a); fa_smk_touch(5); }
        if (b) { fa_smk_set(P_E1,(int32_t)TSEL->p[P_E1]+b); fa_smk_touch(6); }
    } else if (normal && shift) {
        int32_t d=panel_enc(EN_K4);
        if(d) { fa_smk_set(P_LEVEL,(int32_t)TSEL->p[P_LEVEL]+d); fa_smk_touch(7); }
    }
    for(i=0;i<4u;i++) {
        fmini_pickup_t *p=&fa_smk_pickup[i];
        uint32_t physical=fm1_in.fader[i], moved=p->ready && p->physical!=physical;
        uint32_t old_armed=p->armed;
        int32_t value=fmini_pickup_poll(p,physical,(uint32_t)TSEL->p[FA_SMK_ENV[i]],fmini_smk_epoch,
                                     blocked || fm1_in.frames<300u);
        if(value>=0) fa_smk_set(FA_SMK_ENV[i],value);
        if(!blocked && moved) fa_smk_touch(i+1u);
        if(old_armed!=p->armed) ui.force=1;
    }
    fa_sg_input(pressed);
}
static void fa_smk_overlay(void)
{
    static const char *const ENV_NAME[4]={"ATTACK","DECAY","SUSTAIN","RELEASE"};
    uint32_t id, h=fa_smk_hot;
    char value[16]; const char *unit;
    const char *label, *state="";
    if(fa_sv || fa_gl || fa_ar || fa_sq || fa_sc || fmg_fx || fa_tp || fb_rs_state()==FB_RS_ASK || fb_rs_state()==FB_RS_OK) return;
    if(fa_smk_shift) {
        cv_rect(12,195,216,16,FA_BOX);
        fa_ctext(120,199,&AF_X,"EDIT: K2 SELECT K3 VALUE K8 LEVEL",FA_CR);
        return;
    }
    if(!h || !fa_smk_until) return;
    if(h<=4u) {
        fmini_pickup_t *p=&fa_smk_pickup[h-1u];
        id=FA_SMK_ENV[h-1u]; label=ENV_NAME[h-1u];
        if(!p->armed) state=p->physical<TSEL->p[id]?"PICKUP UP":"PICKUP DOWN";
    } else { id=h==5u?P_E0:h==6u?P_E1:P_LEVEL; label=h==5u?"CUTOFF":h==6u?"RESONANCE":"LEVEL"; }
    param_format(track_desc(TSEL,id),TSEL->p[id],value,&unit);
    cv_rect(12,195,216,16,FA_BOX);
    fa_ltext(16,199,&AF_X,label,FA_OR);
    fa_ltext(77,199,&AF_X,value,FA_CR);
    fa_ltext(77+text_w(&AF_X,value)+3,199,&AF_X,unit,FA_MU);
    if(*state) fa_ltext(224-text_w(&AF_X,state),199,&AF_X,state,FA_CR);
}

/* SEQ: the pads as a step grid while the SEQUENCER pop-up is open and REC is off (REC on: they play notes, as before).
 * Pad p is step page*16 + p + 1: steps 1-8 the top row, 9-16 the bottom; < > the four pages of 16 (64 steps).
 * A tap on an empty step puts the last played note on it; a tap on a step with a note clears it. Hold a pad and play
 * a key: that step's note. Hold a pad and turn K5..K8: accent, slide, tie, chance. The length grows to the end of the
 * page a new step is on; a clear never shortens it */
static uint8_t fa_sg_page, fa_sg_last = 0xFFu;
static uint16_t fa_sg_held, fa_sg_edited;                /* pads held; held pads edited (no clear on release) */
static int fa_sg_on(void) { return fa_sq && !fa_sq_rec && !fa_sc && !fa_tp; }
static int fa_sg_has(const step_t *s) { return s->n && s->time != ST_REST; }
static void fa_sg_fill(uint32_t idx, uint32_t note)
{
    track_t *t = &trk[0];
    step_t *s = &t->step[idx];
    uint32_t end = (idx / 16u + 1u) * 16u;
    if (!fa_sg_has(s)) {
        memset(s, 0, sizeof *s);
        s->time = ST_NOTE;
        s->vel = 100;
    }
    s->note[0] = (uint8_t)note;
    s->n = 1;
    if ((uint32_t)t->p[P_SLEN] < end)
        t->p[P_SLEN] = (int16_t)end;
    fa_sq_fresh = 0;                                      /* REC later starts a new take, as after a played one */
}
static void fa_sg_key(uint32_t note)                      /* a key while step pads are held: their note */
{
    uint32_t k;
    if (!fa_sg_on() || !fa_sg_held || note > 127u)
        return;
    for (k = 0; k < 16u; k++)
        if ((fa_sg_held >> k) & 1u)
            fa_sg_fill(fa_sg_page * 16u + k, note);
    fa_sg_edited |= fa_sg_held;
    ui.force = 1;
}
static void fa_sg_knob(uint32_t which, int32_t d)         /* K5..K8 on the held steps that have a note */
{
    uint32_t k;
    for (k = 0; k < 16u; k++) {
        step_t *s = &trk[0].step[fa_sg_page * 16u + k];
        if (!((fa_sg_held >> k) & 1u) || !fa_sg_has(s))
            continue;
        if (which == 0u)
            s->flags = (uint8_t)(d > 0 ? s->flags | SF_ACCENT : s->flags & ~SF_ACCENT);
        else if (which == 1u)
            s->flags = (uint8_t)(d > 0 ? s->flags | SF_SLIDE : s->flags & ~SF_SLIDE);
        else if (which == 2u)
            s->time = d > 0 ? ST_TIE : ST_NOTE;
        else
            s->probability = (uint8_t)clamp((int32_t)step_chance(s) + d * 5, 5, 100);
    }
    fa_sg_edited |= fa_sg_held;
    ui.force = 1;
}
static void fa_sg_input(uint32_t pressed)
{
    uint32_t held, down, up, k;
    int32_t d;
    smk_pad_steps = (uint8_t)fa_sg_on();
    if (!smk_pad_steps) {
        (void)smk_steps_take();
        fa_sg_held = fa_sg_edited = 0;
        return;
    }
    if (pressed & (1u << SMK_BTN_LEFT)) { fa_sg_page = (uint8_t)((fa_sg_page + 3u) % 4u); fa_sg_held = fa_sg_edited = 0; ui.force = 1; }
    if (pressed & (1u << SMK_BTN_RIGHT)) { fa_sg_page = (uint8_t)((fa_sg_page + 1u) % 4u); fa_sg_held = fa_sg_edited = 0; ui.force = 1; }
    down = smk_steps_take();
    held = smk_step_held;
    for (k = 0; k < 16u; k++) {
        uint32_t idx = fa_sg_page * 16u + k;
        if (!((down >> k) & 1u))
            continue;
        if (!fa_sg_has(&trk[0].step[idx])) {              /* empty: on at once, so hold + key edits a new step */
            fa_sg_fill(idx, last_note);
            fa_sg_edited |= (uint16_t)(1u << k);
        } else {
            fa_sg_edited &= (uint16_t)~(1u << k);
        }
        fa_sg_last = (uint8_t)idx;
        ui.force = 1;
    }
    up = (uint32_t)(fa_sg_held | down) & ~held;
    for (k = 0; k < 16u; k++)
        if (((up >> k) & 1u) && !((fa_sg_edited >> k) & 1u)) {
            memset(&trk[0].step[fa_sg_page * 16u + k], 0, sizeof(step_t));
            ui.force = 1;
        }
    fa_sg_held = (uint16_t)held;
    fa_sg_edited &= (uint16_t)held;
    if (fa_sg_held)
        for (k = 0; k < 4u; k++)
            if ((d = panel_enc(EN_K1 + k)) != 0)
                fa_sg_knob(k, d);
}
static void fa_sg_leds(void)                              /* the pads in SEQ: the page's 16 steps */
{
    const track_t *t = &trk[0];
    uint32_t k, len = (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP), now = song.playing ? t->seq_idx % len : 99u;
    for (k = 0; k < 16u; k++) {
        uint32_t idx = fa_sg_page * 16u + k;
        const step_t *s = &t->step[idx];
        uint8_t r = 0, g = 0, b = 0;
        if ((fa_sg_held >> k) & 1u) { r = 72; g = 56; b = 24; }                /* held: cream */
        else if (idx == now) { r = 48; g = 48; b = 48; }                     /* the playhead: white */
        else if (fa_sg_has(s) && (s->flags & SF_SLIDE)) { r = 4; g = 12; b = 48; }   /* slide: blue */
        else if (fa_sg_has(s) && (s->flags & SF_ACCENT)) { r = 96; g = 28; b = 0; }  /* accent: bright orange */
        else if (fa_sg_has(s)) { r = 24; g = 7; b = 0; }                    /* a note: dim orange */
        else if (idx < len) { r = 2; g = 1; b = 0; }                          /* a rest inside the length */
        padled_set(k, r, g, b);
    }
}
static void fa_sg_line(void)                              /* the SEQ pop-up's line about the pads */
{
    char b[48], n[8];
    const step_t *s;
    if (!fa_sg_on())
        return;
    cv_rect(12, 210, 216, 13, FA_RGB(0x1B, 0x19, 0x17));
    cv_rect(20, 10 + (int32_t)fa_sg_page * 46, 4, 41, FA_OR);            /* the page's two rows of the grid */
    if (fa_sg_held && fa_sg_last < NSTEP && fa_sg_has(s = &trk[0].step[fa_sg_last])) {
        fmt_int(b, (int32_t)fa_sg_last + 1);
        str_cpy(b + str_len(b), " ", 2);
        note_name(n, s->note[0]);
        str_cpy(b + str_len(b), n, 8);
        if (s->flags & SF_ACCENT) str_cpy(b + str_len(b), " ACC", 5);
        if (s->flags & SF_SLIDE) str_cpy(b + str_len(b), " SLIDE", 7);
        if (s->time == ST_TIE) str_cpy(b + str_len(b), " TIE", 5);
        str_cpy(b + str_len(b), " ", 2);
        fmt_int(b + str_len(b), (int32_t)step_chance(s));
        str_cpy(b + str_len(b), "%", 2);
        fa_ctext(120, 214, &AF_X, b, FA_CR);
    } else {
        str_cpy(b, "PADS = STEPS ", 14);
        fmt_int(b + str_len(b), fa_sg_page * 16 + 1);
        str_cpy(b + str_len(b), "-", 2);
        fmt_int(b + str_len(b), fa_sg_page * 16 + 16);
        str_cpy(b + str_len(b), "  < > PAGE", 11);
        fa_ctext(120, 214, &AF_X, b, FA_MU);
    }
}
