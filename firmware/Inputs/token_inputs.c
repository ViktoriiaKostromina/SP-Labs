#include "token_inputs.h"
#include <string.h>
bool st_job_sample(st_job_input *p, bool ok, uint16_t raw)
{
    if(!ok || raw>4095) { memset(p,0,sizeof *p);return false; }
    if(p->count==8)p->sum-=p->samples[p->next];else ++p->count;
    p->samples[p->next]=raw;p->sum+=raw;p->next=(uint8_t)((p->next+1)&7);
    if(p->count<8)return false;
    p->raw=(uint16_t)((p->sum+4)/8);p->mv=(uint16_t)(((uint32_t)p->raw*3300+2047)/4095);
    uint8_t previous=p->job;
    if(!p->valid)p->job=(uint8_t)(p->raw/1024+1);
    else {
        while(p->job<4 && p->raw>=(uint16_t)(p->job*1024+32))++p->job;
        while(p->job>1 && p->raw<(uint16_t)((p->job-1)*1024-32))--p->job;
    }
    p->valid=true;return p->job!=previous;
}
bool st_button_step(st_button *b, bool pressed, uint32_t now)
{
    if(pressed!=b->candidate) { b->candidate=pressed;b->changed_ms=now; }
    if(b->stable!=b->candidate && (uint32_t)(now-b->changed_ms)>=25) {
        b->stable=b->candidate;return b->stable;
    }
    return false;
}
bool st_token_step(st_token_gate *g, st_result result, const st_uid *u)
{
    if(result==ST_NO_CARD) {
        if(g->absent<3)++g->absent;
        if(g->absent==3)g->latched=false;
        return false;
    }
    /* A fault breaks the consecutive no-card sequence and never rearms a card. */
    g->absent=0;
    if(result!=ST_OK)return false;
    if(g->latched && u->length==g->last.length && !memcmp(u->bytes,g->last.bytes,u->length))return false;
    g->last=*u;g->latched=true;return true;
}
