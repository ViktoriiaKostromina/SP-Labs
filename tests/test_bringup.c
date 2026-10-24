#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "bringup.h"
#include "fake_buses.h"
static fake_rf rf;static fake_sd sd;
static uint32_t clock_ms;static uint16_t adc_raw=2048;static bool adc_ok=true,pressed[2];
static char input[1024],output[60000];static size_t input_pos,output_n;static unsigned led_changes;
static uint32_t app_time(void){return clock_ms;}
static bool adc_read(uint16_t *v){*v=adc_raw;return adc_ok;}
static bool btn(unsigned n){return pressed[n];}
static bool rx(uint8_t *v){if(!input[input_pos])return false;*v=(uint8_t)input[input_pos++];return true;}
static uint32_t rx_lost(void){return 2;}
static void log_text(const char *p){size_t n=strlen(p);assert(output_n+n<sizeof output);memcpy(output+output_n,p,n+1);output_n+=n;}
static bool cap;
static void capture(bool on){cap=on;}
static void led(bool x){(void)x;++led_changes;}
static void run(unsigned ms){for(unsigned i=0;i<ms;++i){++clock_ms;st_bringup_tick();}}
static void command(const char *p){assert(strlen(p)<sizeof input);strcpy(input,p);input_pos=0;run(100);assert(!input[input_pos]);}
int main(void)
{
 st_rfid r=make_rf(&rf);st_sd s=make_sd(&sd);four_byte_card(&rf);
 st_platform p={.reader=r.io,.storage=s.io,.now=app_time,.adc=adc_read,.button=btn,.rx=rx,.rx_lost=rx_lost,.log=log_text,.led=led,.capture=capture};
 st_bringup_init(&p);run(100);
 assert(strstr(output,"BOOT product=ShiftToken lab=2") && strstr(output,"SELFTEST dev=MFRC522 result=OK version=0x92"));
 assert(strstr(output,"SELFTEST dev=SD result=OK") && strstr(output,"JOB") && strstr(output,"code=3 valid=1"));
 assert(strstr(output,"uid_len=4 uid=01020304") && led_changes==1);
 pressed[0]=true;run(10);pressed[0]=false;run(10);assert(!strstr(output,"name=PAUSE"));
 pressed[0]=true;run(80);pressed[0]=false;run(30);pressed[1]=true;run(50);pressed[1]=false;run(30);
 assert(strstr(output,"name=PAUSE count=1") && strstr(output,"name=CLOSE count=1"));
 command("capture 10\ncapture 0\ncapture 10001\n");assert(!cap && strstr(output,"result=ARMED") && strstr(output,"result=DONE"));
 command("status\r\nhelp\n");assert(strstr(output,"adc_mV=1650 job=3 valid=1") && strstr(output,"HELP") && strstr(output,"rx_lost=2"));
 command("sdwrite 92\nsdread\n");assert(strstr(output,"seed=92 result=OK") && strstr(output,"verified_bytes=512") && strstr(output,"first=5C6D7E8FA0B1C2D3"));
 command("sdwrite 256\nsdwrite -1\nsdwrite x\nsdwrite \npause\n");assert(strstr(output,"reason=seed_0_255") && strstr(output,"reason=unknown_command"));
 char overlong[200];memset(overlong,'A',sizeof overlong);overlong[sizeof overlong-2]='\n';overlong[sizeof overlong-1]=0;command(overlong);assert(strstr(output,"reason=line_too_long"));
 command("bad\tline\n");command("\n");
 adc_ok=false;run(30);command("status\n");assert(strstr(output,"job=0 valid=0"));adc_ok=true;adc_raw=4095;run(100);command("status\n");assert(strstr(output,"adc_mV=3300 job=4 valid=1"));
 rf.reg[0x37]=0;run(100);assert(strstr(output,"RFID_FAULT") && strstr(output,"result=ID"));run(1100);assert(strstr(output,"RFID_RETRY"));rf.reg[0x37]=0x92;run(1100);
 assert(strstr(output,"RFID_RETRY") && strstr(output,"result=OK version=92"));
 sd.fail_at=sd.calls+1;command("sdread\n");assert(strstr(output,"result=BUS crc32=00000000 first=0000000000000000"));sd.fail_at=0;run(1100);command("sdread\nselftest\nstatus\n");
 assert(strstr(output,"buttons=1/1") && strstr(output,"cards=1") && strstr(output,"dropped=2"));
 puts("PASS bringup: simultaneous inputs, self-test, UART parser, SD read-back, fault/retry, invalid ADC, no session implementation");
 printf("PASS fake_uart_log_bytes=%zu\n",output_n);return 0;
}
