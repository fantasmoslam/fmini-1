/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdio.h>
#include <string.h>
#include "fmini_smk_pickup.h"
static int bad;
#define CHECK(name,x) do{int ok=!!(x);printf("pickup %s: %s\n",name,ok?"ok":"FAIL");bad+=!ok;}while(0)
int main(void)
{
    fmini_pickup_t p={0}, q={0}; int x;
    CHECK("boot does not overwrite",fmini_pickup_poll(&p,10,64,1,0)==-1);
    CHECK("stationary stays waiting",fmini_pickup_poll(&p,10,64,1,0)==-1&&!p.armed);
    CHECK("approach does not overwrite",fmini_pickup_poll(&p,40,64,1,0)==-1);
    CHECK("fast crossing acquires",fmini_pickup_poll(&p,80,64,1,0)==80&&p.armed);
    CHECK("self edits retain ownership",fmini_pickup_poll(&p,90,80,1,0)==90&&p.armed);
    CHECK("external edit disarms",fmini_pickup_poll(&p,90,20,1,0)==-1&&!p.armed);
    CHECK("downward crossing acquires",fmini_pickup_poll(&p,5,20,1,0)==5&&p.armed);
    CHECK("new patch same value disarms",fmini_pickup_poll(&p,5,5,2,0)==-1&&!p.armed);
    CHECK("moving from target takes control",fmini_pickup_poll(&p,6,5,2,0)==6);
    CHECK("modal motion is ignored",fmini_pickup_poll(&p,100,6,2,1)==-1&&!p.ready);
    CHECK("modal exit samples without writing",fmini_pickup_poll(&p,100,6,2,0)==-1);
    CHECK("invalid ADC ignored",fmini_pickup_poll(&p,255,6,2,0)==-1&&!p.ready);
    memset(&p,0,sizeof p);
    fmini_pickup_poll(&p,10,0,1,0);
    CHECK("zero endpoint acquired",fmini_pickup_poll(&p,0,0,1,0)==-1&&p.armed);
    CHECK("zero then moves",fmini_pickup_poll(&p,1,0,1,0)==1);
    memset(&p,0,sizeof p);fmini_pickup_poll(&p,10,127,1,0);
    CHECK("maximum endpoint acquired",fmini_pickup_poll(&p,127,127,1,0)==-1&&p.armed);
    CHECK("maximum then moves",fmini_pickup_poll(&p,126,127,1,0)==126);
    fmini_pickup_poll(&q,30,90,1,0);
    CHECK("other fader remains waiting",!q.armed&&p.armed);
    for(x=0;x<128;x++) {
        memset(&p,0,sizeof p);fmini_pickup_poll(&p,0,x,1,0);
        if(x) fmini_pickup_poll(&p,(uint32_t)x,x,1,0);
        CHECK("every target acquires on movement",fmini_pickup_poll(&p,x==127?126:(uint32_t)x+1,x,1,0)>=0);
    }
    return bad!=0;
}
