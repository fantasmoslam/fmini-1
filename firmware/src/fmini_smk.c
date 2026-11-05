/* SPDX-License-Identifier: GPL-3.0-only */
/* FMini's performance controls; included only on the SMK, before fa_scene. */
#include "fmini_smk_pickup.h"
static fmini_pickup_t fa_smk_pickup[4];
static const uint8_t FA_SMK_ENV[4] = {P_ATK, P_DEC, P_SUS, P_REL};
static uint8_t fa_smk_shift;
static void fa_goto(uint32_t pg);
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
