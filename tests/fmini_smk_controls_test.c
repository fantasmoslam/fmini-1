/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual FMini UI and sequencer with a register-free SMK HAL. */
#define BOARD_SMK37 1
#define UI_TEST_NO_MAIN 1
#define FELUCCA_FMINI_ONLY 1
#include "ui_test.c"
#include "../firmware/src/fmini_rec.c"
#include "../firmware/src/fmini_bank.c"
#include "../firmware/src/fmini_app.c"
#include <assert.h>
static void smk_frame(void) { fm1_ms+=16; host_ticks+=16000; fa_input(); }
static void smk_turn(uint32_t role,int32_t d) { host_enc[panel.enc[role]]=d; smk_frame(); }
static void render(const char *dir,const char *name) {
    char path[512]; uint32_t i; FILE *f;
    ui.force=1; fa_draw();
    snprintf(path,sizeof path,"%s/%s.ppm",dir,name); f=fopen(path,"wb"); assert(f);
    fprintf(f,"P6\n240 240\n255\n");
    for(i=0;i<240u*240u;i++) {
        uint16_t c=swap16(host_screen[i]);
        uint8_t rgb[3]={(uint8_t)((c>>11)*255u/31u),(uint8_t)(((c>>5)&63)*255u/63u),(uint8_t)((c&31)*255u/31u)};
        fwrite(rgb,1,3,f);
    }
    fclose(f);
}
int main(int argc,char **argv) {
    uint32_t i; int32_t cutoff, resonance, release, level;
    const char *dir=argc>1?argv[1]:".";
    for(i=0;i<G_COUNT;i++) song.g[i]=GP[i].def;
    panel=PANEL_DEFAULT; fb_names_init(); fa_init();
    kb_boot_hold=kb_asleep=0; fm1_in.frames=400;
    fa_sv=fa_gl=fa_ar=fa_sq=fa_sc=fmg_fx=fa_tp=0;
    for(i=0;i<4;i++) { TSEL->p[FA_SMK_ENV[i]]=64; fm1_in.fader[i]=10; }
    smk_frame();
    for(i=0;i<4;i++) assert(TSEL->p[FA_SMK_ENV[i]]==64);
    fm1_in.fader[0]=80; smk_frame(); assert(TSEL->p[P_ATK]==80);
    assert(TSEL->p[P_DEC]==64 && TSEL->p[P_REL]==64);
    TSEL->p[P_ATK]=100; smk_frame(); fm1_in.fader[0]=90; smk_frame(); assert(TSEL->p[P_ATK]==100);
    fm1_in.fader[0]=110; smk_frame(); assert(TSEL->p[P_ATK]==110);
    fmini_smk_epoch++; smk_frame(); fm1_in.fader[0]=100; smk_frame(); assert(TSEL->p[P_ATK]==100);
    /* Same-value preset load re-arms even when its numerical target is unchanged. */
    fmini_smk_epoch++; smk_frame(); assert(!fa_smk_pickup[0].armed);
    fa_gl=1; fm1_in.fader[0]=127; smk_frame(); assert(TSEL->p[P_ATK]==100);
    fa_gl=0; smk_frame(); fm1_in.fader[0]=110; smk_frame(); assert(TSEL->p[P_ATK]==100);
    fm1_in.fader[0]=95; smk_frame(); assert(TSEL->p[P_ATK]==95);
    { uint32_t epoch=fmini_smk_epoch;
      apply_preset_to(TSEL,0); assert(fmini_smk_epoch!=epoch); smk_frame();
      for(i=0;i<4;i++) assert(!fa_smk_pickup[i].armed);
    }
    cutoff=TSEL->p[P_E0]=64; resonance=TSEL->p[P_E1]=64;
    smk_turn(EN_SELECT,3); smk_turn(EN_ALGO,2);
    assert(TSEL->p[P_E0]==cutoff+3 && TSEL->p[P_E1]==resonance+2);
    fa_page=5; release=TSEL->p[P_REL];
    smk_turn(EN_K4,1); assert(TSEL->p[P_REL]!=release);
    level=TSEL->p[P_LEVEL]=64; release=TSEL->p[P_REL];
    fm1_in.buttons=1u<<SMK_BTN_KNOB_BANK; smk_turn(EN_K4,2);
    assert(TSEL->p[P_LEVEL]==level+2 && TSEL->p[P_REL]==release);
    render(dir,"adsr-shift"); fm1_in.buttons=0; smk_frame();
    fm1_in.fader[3]=127; smk_frame(); render(dir,"adsr-pickup");
    fa_smk_hot=0; fa_smk_until=0; render(dir,"adsr");
    fa_gl=1; fa_gl_row=7; smk_turn(EN_ALGO,1);
    assert(ui_prefs2 & (PREF_CPU2_ON>>16)); render(dir,"global");
    smk_turn(EN_ALGO,-1); assert(!(ui_prefs2 & (PREF_CPU2_ON>>16))); fa_gl=0;
    for(i=0;i<100;i++) fa_leds();
    /* The ten keys outside the original 27-note window reach the same engine. */
    song.octave=0; TSEL->p[P_QUANT]=QN_OFF; TSEL->p[P_TRANS]=0;
    for(i=0;i<37;i++) if(i<5 || i>=32) {
        host_ext_event=(int32_t)(i<<8)|100; ext_keys_block();
        assert(ext_note[i]==49u+i);
        host_ext_event=(int32_t)(i<<8); ext_keys_block(); assert(!ext_note[i]);
    }
    fm_bend_mode=3; smk_bend14=8191; smk_wheels_block();
    assert(midi_bend_target[0]==2*256);
    smk_bend14=-8192; smk_wheels_block(); assert(midi_bend_target[0]==-24*256);
    smk_bend14=0; smk_wheels_block(); assert(!midi_bend_target[0]);
    puts("SMK real UI: ADSR ownership, filter controls, modifier, render, LEDs and outer keys passed");
    return 0;
}
