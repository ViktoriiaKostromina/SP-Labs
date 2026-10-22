#include "bringup.h"
#include "rfid.h"
#include "sd_sector.h"
#include "token_inputs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static st_platform hw;
static st_rfid reader;
static st_sd storage;
static st_job_input job;
static st_button buttons[2];
static st_token_gate gate;
static uint8_t sector[512];
static uint32_t adc_due,button_due,reader_due,summary_due,retry_due;
static uint32_t adc_count,button_count[2],card_count,reader_errors,adc_errors;
static uint32_t no_cards,command_errors,lines_dropped,sd_errors;
static uint32_t capture_end;
static char line[64];static size_t line_n;static bool discard,lit;
static const char *result_name(st_result r)
{
    static const char *names[]={"OK","NO_CARD","BUS","TIMEOUT","ID","LENGTH","BCC","CRC","PROTOCOL","NOT_READY"};
    return (unsigned)r<sizeof names/sizeof names[0]?names[r]:"UNKNOWN";
}
static void state(void)
{
    char text[400];
    snprintf(text,sizeof text,"STATE t_ms=%lu adc_raw=%u adc_mV=%u job=%u valid=%u adc_n=%lu buttons=%lu/%lu cards=%lu no_card=%lu rfid_errors=%lu adc_errors=%lu rx_lost=%lu cmd_errors=%lu dropped=%lu sd_ready=%u rfid_ready=%u sd_errors=%lu\r\n",
      (unsigned long)hw.now(),job.raw,job.mv,job.job,job.valid,(unsigned long)adc_count,
      (unsigned long)button_count[0],(unsigned long)button_count[1],(unsigned long)card_count,
      (unsigned long)no_cards,(unsigned long)reader_errors,(unsigned long)adc_errors,
      (unsigned long)hw.rx_lost(),(unsigned long)command_errors,(unsigned long)lines_dropped,storage.ready,reader.ready,(unsigned long)sd_errors);hw.log(text);
}
static void selftest(void)
{
    char text[160];st_result r=st_rfid_init(&reader);
    snprintf(text,sizeof text,"SELFTEST dev=MFRC522 result=%s version=0x%02X spi_hz=375000\r\n",result_name(r),reader.version);hw.log(text);
    r=st_sd_init(&storage);
    if(r!=ST_OK)++sd_errors;
    snprintf(text,sizeof text,"SELFTEST dev=SD result=%s ocr=0x%08lX addressing=%s\r\n",result_name(r),(unsigned long)storage.ocr,storage.block_addressing?"BLOCK":"BYTE");hw.log(text);
    uint16_t raw=0;bool ok=hw.adc(&raw);
    snprintf(text,sizeof text,"SELFTEST dev=ADC result=%s raw=%u unit=count vref_mV=3300\r\n",ok?"OK":"ERROR",raw);hw.log(text);
    snprintf(text,sizeof text,"SELFTEST dev=BUTTONS result=READ levels_pressed=%u/%u debounce_ms=25\r\n",hw.button(0),hw.button(1));hw.log(text);
    hw.log("READY lab=2 mode=bringup commands=status,selftest,sdread,sdwrite,capture,help\r\n");
}
static void sdread(void)
{
    char text[190];st_result r=st_sd_read(&storage,127,sector);
    if(r!=ST_OK){++sd_errors;storage.ready=false;memset(sector,0,sizeof sector);}
    snprintf(text,sizeof text,"SDREAD t_ms=%lu lba=127 result=%s crc32=%08lX first=%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
      (unsigned long)hw.now(),result_name(r),r==ST_OK?(unsigned long)st_crc32(sector,512):0ul,
      sector[0],sector[1],sector[2],sector[3],sector[4],sector[5],sector[6],sector[7]);hw.log(text);
}
static uint8_t pattern(unsigned i,uint8_t seed)
{ return (uint8_t)(seed+(i*17u)+(i>>4)); }
static void sdwrite(uint8_t seed)
{
    for(unsigned i=0;i<512;++i)sector[i]=pattern(i,seed);
    uint32_t expected=st_crc32(sector,512),t=hw.now();
    st_result r=st_sd_write(&storage,127,sector);
    if(r==ST_OK)r=st_sd_read(&storage,127,sector);
    if(r==ST_OK)for(unsigned i=0;i<512;++i)if(sector[i]!=pattern(i,seed)){r=ST_CRC;break;}
    if(r!=ST_OK){++sd_errors;storage.ready=false;}
    char text[180];snprintf(text,sizeof text,"SDWRITE t_ms=%lu lba=127 seed=%u result=%s crc32=%08lX elapsed_ms=%lu verified_bytes=%u\r\n",
      (unsigned long)hw.now(),seed,result_name(r),(unsigned long)expected,(unsigned long)(hw.now()-t),r==ST_OK?512u:0u);hw.log(text);
}
static void command(void)
{
    if(!strncmp(line,"capture ",8)) {
        char *endptr;unsigned long ms=strtoul(line+8,&endptr,10);
        if(line[8] && !*endptr && ms>=1 && ms<=10000) {
            capture_end=hw.now()+(uint32_t)ms;hw.capture(true);
            hw.log("CAPTURE result=ARMED max_ms=10000\r\n");
        } else {++command_errors;hw.log("ERROR command=capture reason=duration_1_10000\r\n");}
    }
    else if(!strcmp(line,"status"))state();
    else if(!strcmp(line,"selftest"))selftest();
    else if(!strcmp(line,"sdread"))sdread();
    else if(!strncmp(line,"sdwrite ",8)) {
        char *endptr;unsigned long n=strtoul(line+8,&endptr,10);
        if(line[8] && !*endptr && n<=255)sdwrite((uint8_t)n);
        else {++command_errors;hw.log("ERROR command=sdwrite reason=seed_0_255\r\n");}
    }
    else if(!strcmp(line,"help"))hw.log("HELP status selftest sdread sdwrite <0..255> capture <1..10000_ms>; lab2 has no session commands\r\n");
    else {++command_errors;hw.log("ERROR reason=unknown_command\r\n");}
}
void st_bringup_init(const st_platform *p)
{
    hw=*p;reader.io=hw.reader;storage.io=hw.storage;
    hw.log("BOOT product=ShiftToken lab=2 format=1 clock_hz=48000000 uart=115200_8N1\r\n");
    selftest();uint32_t t=hw.now();adc_due=button_due=reader_due=t;summary_due=t+1000;retry_due=t+1000;
}
static bool due(uint32_t now,uint32_t *next,uint32_t period)
{
    if((int32_t)(now-*next)<0)return false;
    *next+=period;
    if((int32_t)(now-*next)>=0)*next=now+period; /* report reality; no artificial backfill samples */
    return true;
}
void st_bringup_tick(void)
{
    uint32_t now=hw.now();
    if(capture_end && (int32_t)(now-capture_end)>=0) {
        hw.capture(false);capture_end=0;hw.log("CAPTURE result=DONE\r\n");
    }
    /* Bounded receiver work per pass; overlong lines are dropped through LF. */
    uint8_t c;
    for(unsigned n=0;n<32 && hw.rx(&c);++n) {
        if(c=='\r')continue;
        if(c=='\n') {
            if(discard){++lines_dropped;hw.log("ERROR reason=line_too_long\r\n");}
            else if(line_n){line[line_n]=0;command();}
            line_n=0;discard=false;
        } else if(c<32 || c>126)discard=true;
        else if(!discard){if(line_n<sizeof line-1)line[line_n++]=(char)c;else discard=true;}
    }
    now=hw.now();
    if(due(now,&button_due,5)) {
        for(unsigned i=0;i<2;++i)if(st_button_step(&buttons[i],hw.button(i),now)) {
            ++button_count[i];char text[100];snprintf(text,sizeof text,"BUTTON t_ms=%lu name=%s count=%lu unit=event\r\n",(unsigned long)now,i?"CLOSE":"PAUSE",(unsigned long)button_count[i]);hw.log(text);
        }
    }
    if(due(now,&adc_due,10)) {
        uint16_t raw=0;bool ok=hw.adc(&raw);++adc_count;
        if(!ok)++adc_errors;
        if(st_job_sample(&job,ok,raw)) {
            char text[100];snprintf(text,sizeof text,"JOB t_ms=%lu raw=%u mV=%u code=%u valid=%u\r\n",(unsigned long)now,job.raw,job.mv,job.job,job.valid);hw.log(text);
        }
    }
    if(reader.ready && due(now,&reader_due,50)) {
        st_uid uid={0};st_result r=st_rfid_poll(&reader,&uid);
        if(r==ST_NO_CARD)++no_cards;
        else if(r!=ST_OK) {
            ++reader_errors;reader.ready=false;char text[100];snprintf(text,sizeof text,"RFID_FAULT t_ms=%lu result=%s\r\n",(unsigned long)hw.now(),result_name(r));hw.log(text);
        }
        if(st_token_step(&gate,r,&uid)) {
            ++card_count;char text[120];int pos=snprintf(text,sizeof text,"TOKEN t_ms=%lu uid_len=%u uid=",(unsigned long)hw.now(),uid.length);
            for(unsigned i=0;i<uid.length;++i)pos+=snprintf(text+(size_t)pos,sizeof text-(size_t)pos,"%02X",uid.bytes[i]);
            snprintf(text+(size_t)pos,sizeof text-(size_t)pos," job=%u adc_valid=%u event=%lu\r\n",job.job,job.valid,(unsigned long)card_count);hw.log(text);lit=!lit;hw.led(lit);
        }
    }
    if(due(now,&retry_due,1000)) {
        if(!reader.ready) {st_result r=st_rfid_init(&reader);char text[90];snprintf(text,sizeof text,"RFID_RETRY t_ms=%lu result=%s version=%02X\r\n",(unsigned long)hw.now(),result_name(r),reader.version);hw.log(text);}
        if(!storage.ready) {st_result r=st_sd_init(&storage);if(r!=ST_OK)++sd_errors;char text[80];snprintf(text,sizeof text,"SD_RETRY t_ms=%lu result=%s\r\n",(unsigned long)hw.now(),result_name(r));hw.log(text);}
    }
    if(due(now,&summary_due,1000))state();
}
