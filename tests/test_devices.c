#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "rfid.h"
#include "sd_sector.h"
#include "token_inputs.h"
static unsigned checks;
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"FAIL line=%d %s\n",__LINE__,#x);return 1;}}while(0)
#include "fake_buses.h"
static int rfid_tests(void)
{
 fake_rf f;st_rfid r=make_rf(&f);st_uid u;
 CHECK(st_crc_a((uint8_t[]){0x50,0},2)==0xcd57);
 CHECK(st_rfid_poll(&r,&u)==ST_NOT_READY);
 CHECK(st_rfid_init(&r)==ST_OK && r.ready && !f.selected);
 four_byte_card(&f);CHECK(st_rfid_poll(&r,&u)==ST_OK && u.length==4 && !memcmp(u.bytes,(uint8_t[]){1,2,3,4},4));
 CHECK(f.sent_n[0]==1 && f.sent[0][0]==0x52 && f.sent_n[2]==9 && f.sent[2][1]==0x70);
 CHECK(!memcmp(f.sent[3],(uint8_t[]){0x50,0,0x57,0xcd},4));
 r=make_rf(&f);CHECK(st_rfid_init(&r)==ST_OK);f.reg[0x37]=0;CHECK(st_rfid_poll(&r,&u)==ST_ID);
 r=make_rf(&f);f.reg[0x37]=0x88;CHECK(st_rfid_init(&r)==ST_ID && !r.ready);
 r=make_rf(&f);f.fail_at=1;CHECK(st_rfid_init(&r)==ST_BUS && !f.selected);
 for(unsigned fail=2;fail<=12;++fail){r=make_rf(&f);f.fail_at=fail;CHECK(st_rfid_init(&r)==ST_BUS && !r.ready && !f.selected);}
 r=make_rf(&f);CHECK(st_rfid_init(&r)==ST_OK);CHECK(st_rfid_poll(&r,&u)==ST_NO_CARD);
 r=make_rf(&f);CHECK(st_rfid_init(&r)==ST_OK);f.hang=true;CHECK(st_rfid_poll(&r,&u)==ST_TIMEOUT);
 r=make_rf(&f);CHECK(st_rfid_init(&r)==ST_OK);four_byte_card(&f);f.frames[1][4]^=1;CHECK(st_rfid_poll(&r,&u)==ST_BCC);
 r=make_rf(&f);CHECK(st_rfid_init(&r)==ST_OK);four_byte_card(&f);f.frames[2][2]^=1;CHECK(st_rfid_poll(&r,&u)==ST_CRC);
 r=make_rf(&f);CHECK(st_rfid_init(&r)==ST_OK);four_byte_card(&f);f.lastbits=3;CHECK(st_rfid_poll(&r,&u)==ST_LENGTH);
 for(uint8_t bit=1;bit;bit=(uint8_t)(bit<<1)){if(bit==0x20)continue;
 r=make_rf(&f);CHECK(st_rfid_init(&r)==ST_OK);four_byte_card(&f);f.error=bit;CHECK(st_rfid_poll(&r,&u)==ST_PROTOCOL);}
 r=make_rf(&f);CHECK(st_rfid_init(&r)==ST_OK);four_byte_card(&f);f.lengths[0]=1;CHECK(st_rfid_poll(&r,&u)==ST_LENGTH);
 r=make_rf(&f);CHECK(st_rfid_init(&r)==ST_OK);four_byte_card(&f);f.lengths[1]=4;CHECK(st_rfid_poll(&r,&u)==ST_LENGTH);
 r=make_rf(&f);CHECK(st_rfid_init(&r)==ST_OK);four_byte_card(&f);f.lengths[2]=2;CHECK(st_rfid_poll(&r,&u)==ST_LENGTH);
 r=make_rf(&f);CHECK(st_rfid_init(&r)==ST_OK);four_byte_card(&f);sak(&f,2,4);CHECK(st_rfid_poll(&r,&u)==ST_PROTOCOL);
 r=make_rf(&f);CHECK(st_rfid_init(&r)==ST_OK);
 frame(&f,0,(uint8_t[]){0x44,0},2);frame(&f,1,(uint8_t[]){0x88,4,0x11,0x22,0xbf},5);sak(&f,2,4);
 frame(&f,3,(uint8_t[]){0x33,0x44,0x55,0x66,0x44},5);sak(&f,4,0);
 CHECK(st_rfid_poll(&r,&u)==ST_OK && u.length==7 && !memcmp(u.bytes,(uint8_t[]){4,0x11,0x22,0x33,0x44,0x55,0x66},7));
 for(unsigned fail=1;fail<120;++fail){r=make_rf(&f);CHECK(st_rfid_init(&r)==ST_OK);four_byte_card(&f);f.fail_at=f.calls+fail;st_result e=st_rfid_poll(&r,&u);CHECK((e==ST_OK || e==ST_BUS) && !f.selected);}
 puts("PASS RFID: identification, framing, UID4/7, BCC/CRC, timeout, absent card, transaction failures");return 0;
}
static int sd_tests(void)
{
 fake_sd f;st_sd s=make_sd(&f);uint8_t b[512];
 CHECK(st_crc32((const uint8_t *)"123456789",9)==0xcbf43926u);
 CHECK(st_sd_read(&s,127,b)==ST_NOT_READY);
 CHECK(st_sd_init(&s)==ST_OK && s.ready && !s.block_addressing && !f.selected);
 CHECK(f.ncmd==6 && !memcmp(f.commands,(uint8_t[]){0,8,55,41,58,16},6));
 for(unsigned i=0;i<512;++i)b[i]=(uint8_t)(0x5c+i*17+(i>>4));
 CHECK(st_sd_write(&s,127,b)==ST_OK && !f.selected);CHECK(f.commands[f.ncmd-1]==24 && f.args[f.ncmd-1]==127*512);
 memset(b,0,sizeof b);CHECK(st_sd_read(&s,127,b)==ST_OK);CHECK(b[0]==0x5c && b[1]==0x6d && b[511]==(uint8_t)(0x5c+511*17+(511>>4)));
 CHECK(st_sd_read(&s,16384,b)==ST_LENGTH);CHECK(st_sd_write(&s,16384,b)==ST_LENGTH);
 s=make_sd(&f);f.high_capacity=true;CHECK(st_sd_init(&s)==ST_OK && s.block_addressing);CHECK(st_sd_read(&s,127,b)==ST_OK && f.args[f.ncmd-1]==127);
 s=make_sd(&f);f.bad_echo=true;CHECK(st_sd_init(&s)==ST_ID && !s.ready && !f.selected);
 s=make_sd(&f);f.init_hang=true;CHECK(st_sd_init(&s)==ST_TIMEOUT && !f.selected);
 s=make_sd(&f);CHECK(st_sd_init(&s)==ST_OK);f.read_hang=true;CHECK(st_sd_read(&s,127,b)==ST_TIMEOUT && !f.selected);
 s=make_sd(&f);CHECK(st_sd_init(&s)==ST_OK);f.reject_write=true;CHECK(st_sd_write(&s,127,b)==ST_PROTOCOL && !f.selected);
 s=make_sd(&f);CHECK(st_sd_init(&s)==ST_OK);f.busy_hang=true;CHECK(st_sd_write(&s,127,b)==ST_TIMEOUT && !f.selected);
 for(unsigned at=1;at<45;++at){s=make_sd(&f);f.fail_at=at;st_result e=st_sd_init(&s);CHECK((e==ST_OK || e==ST_BUS) && !f.selected);}
 for(unsigned at=1;at<30;++at){s=make_sd(&f);CHECK(st_sd_init(&s)==ST_OK);f.fail_at=f.calls+at;st_result e=st_sd_read(&s,127,b);CHECK((e==ST_OK || e==ST_BUS) && !f.selected);}
 for(unsigned at=1;at<530;at+=7){s=make_sd(&f);CHECK(st_sd_init(&s)==ST_OK);f.fail_at=f.calls+at;st_result e=st_sd_write(&s,127,b);CHECK((e==ST_OK || e==ST_BUS) && !f.selected);}
 puts("PASS SD: real command contract, SDSC/SDHC addressing, block roundtrip, bounds, timeout, rejection, CS cleanup");return 0;
}
static void feed(st_job_input *j,uint16_t raw){for(unsigned i=0;i<8;++i)(void)st_job_sample(j,true,raw);}
static int input_tests(void)
{
 st_job_input j={0};for(unsigned i=0;i<7;++i)CHECK(!st_job_sample(&j,true,0) && !j.valid);
 CHECK(st_job_sample(&j,true,0) && j.valid && j.job==1 && j.mv==0);
 feed(&j,4095);CHECK(j.job==4 && j.mv==3300);
 CHECK(!st_job_sample(&j,false,0) && !j.valid);feed(&j,1023);CHECK(j.job==1);
 feed(&j,1055);CHECK(j.job==1);feed(&j,1056);CHECK(j.job==2);
 feed(&j,992);CHECK(j.job==2);feed(&j,991);CHECK(j.job==1);
 feed(&j,2048);CHECK(j.job==2);feed(&j,2080);CHECK(j.job==3);
 feed(&j,3104);CHECK(j.job==4);CHECK(!st_job_sample(&j,true,4096) && !j.valid);
 feed(&j,2048);CHECK(j.mv==1650 && j.job==3);
 st_button b={0};CHECK(!st_button_step(&b,true,0));CHECK(!st_button_step(&b,false,2));CHECK(!st_button_step(&b,true,4));
 CHECK(!st_button_step(&b,true,28));CHECK(st_button_step(&b,true,29));CHECK(!st_button_step(&b,true,1000));
 CHECK(!st_button_step(&b,false,1005));CHECK(!st_button_step(&b,false,1030));CHECK(!b.stable);
 b=(st_button){0};CHECK(!st_button_step(&b,true,UINT32_MAX-10));CHECK(st_button_step(&b,true,15));
 st_token_gate g={0};st_uid u={{1,2,3,4},4};
 CHECK(st_token_step(&g,ST_OK,&u));CHECK(!st_token_step(&g,ST_OK,&u));
 CHECK(!st_token_step(&g,ST_NO_CARD,&u));CHECK(!st_token_step(&g,ST_NO_CARD,&u));
 CHECK(!st_token_step(&g,ST_BUS,&u));CHECK(!st_token_step(&g,ST_NO_CARD,&u));CHECK(!st_token_step(&g,ST_OK,&u));
 for(unsigned i=0;i<3;++i){CHECK(!st_token_step(&g,ST_NO_CARD,&u));}
 CHECK(st_token_step(&g,ST_OK,&u));
 u.bytes[0]=5;CHECK(st_token_step(&g,ST_OK,&u));
 puts("PASS inputs: ADC counts/mV, warmup, hysteresis, invalidation, debounce, tick wrap, token removal/fault gate");return 0;
}
int main(void)
{if(rfid_tests() || sd_tests() || input_tests())return 1;printf("PASS checks=%u\n",checks);return 0;}
