/* SPDX-License-Identifier: GPL-3.0-only */
/* Execute the real ownership and A/B storage code against simulated NOR. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fm1_flash_map.h"
static uint8_t nor[0x100000], protected_before[0x100000];
static int bad_write, fail_erase, fail_prog, fail_read;
static int st_read(uint32_t off, void *dst, uint32_t n)
{ if (fail_read || off > sizeof nor || n > sizeof nor - off) return -1; memcpy(dst,nor+off,n); return 0; }
static int st_erase(uint32_t off)
{ if (!FL_GUARD(off,4096) || (off & 4095u)) { bad_write++; return -1; } if(fail_erase) return -1; memset(nor+off,255,4096); return 0; }
static int st_prog(uint32_t off, const void *src, uint32_t n)
{ const uint8_t *s=src; uint32_t i; if(!FL_GUARD(off,n)){bad_write++;return -1;} if(fail_prog)return -1; for(i=0;i<n;i++)nor[off+i]&=s[i]; return 0; }
#include "../firmware/src/storage.c"
static int bad;
#define CHECK(name,expr) do { int ok=!!(expr); printf("smk owner: %s: %s\n",name,ok?"ok":"FAIL"); bad+=!ok; } while(0)
static void reset(void)
{ uint32_t i; for(i=0;i<sizeof nor;i++)nor[i]=(uint8_t)(i*37u+11u); memset(nor+ST_OWNER_OFF,255,4096); memcpy(protected_before,nor,sizeof nor); fail_erase=fail_prog=fail_read=bad_write=0; st_owner_cleared=0; }
static int protected_kept(void)
{ uint32_t i; for(i=0;i<sizeof nor;i++)if(FL_PROTECTED(i,1)&&nor[i]!=protected_before[i])return 0; return 1; }
int main(void)
{
    static const uint8_t tag[16]="SMK37DATAFMINI2";
    uint32_t o; uint8_t old[32],next[32],readback[32];
    reset();
    CHECK("new ownership initializes all banks",st_owner_check()==0 && st_owner_cleared);
    CHECK("family tag committed last",memcmp(nor+ST_OWNER_OFF,tag,16)==0);
    CHECK("every protected byte retained",protected_kept() && !bad_write);
    memset(old,0x3c,sizeof old); memset(next,0x7a,sizeof next);
    for(o=OBJ_PRESET0;o<=OBJ_PRESET_LAST;o++) {
        CHECK("preset object saves",st_save(o,old,sizeof old)==0);
        CHECK("preset object reloads",st_load(o,readback,sizeof readback)==sizeof old && !memcmp(old,readback,sizeof old));
    }
    st_owner_cleared=0;
    CHECK("own family keeps saved bank",st_owner_check()==0 && !st_owner_cleared && st_load(OBJ_PRESET_LAST,readback,sizeof readback)==sizeof old && !memcmp(old,readback,sizeof old));
    fail_prog=1;
    CHECK("interrupted replacement fails",st_save(OBJ_PRESET_LAST,next,sizeof next)!=0);
    fail_prog=0;
    CHECK("old A/B copy survives",st_load(OBJ_PRESET_LAST,readback,sizeof readback)==sizeof old && !memcmp(old,readback,sizeof old));
    CHECK("writes never reached protected bytes",protected_kept() && !bad_write);
    reset();fail_erase=1;
    CHECK("erase failure prevents ownership",st_owner_check()!=0 && memcmp(nor+ST_OWNER_OFF,tag,16));
    reset();fail_prog=1;
    CHECK("program failure prevents ownership",st_owner_check()!=0 && memcmp(nor+ST_OWNER_OFF,tag,16));
    reset();fail_read=1;
    CHECK("read failure prevents ownership",st_owner_check()!=0);
    reset();memcpy(nor+0xEAFF8,"usrdata",8);
    CHECK("stock marker invalidated before bank use",st_owner_check()==0 && memcmp(nor+0xEAFF8,"usrdata",8));
    CHECK("stock transition preserves calibration",protected_kept() && !bad_write);
    return bad!=0;
}
