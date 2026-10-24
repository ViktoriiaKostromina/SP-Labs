#ifndef ST_TEST_FAKE_BUSES_H
#define ST_TEST_FAKE_BUSES_H
#include <assert.h>
#include <string.h>
#include "rfid.h"
#include "sd_sector.h"
typedef struct {
 uint8_t reg[64],frames[8][8],lengths[8],fifo[16],sent[8][16],sent_n[8];
 unsigned frame,fifo_pos,fifo_n,tx_n,calls,fail_at;uint8_t tx[16];
 uint32_t ms;bool selected,hang;uint8_t error,lastbits;
} fake_rf;
static void rf_select(void *v,bool on){fake_rf *f=v;assert(f->selected!=on);f->selected=on;}
static uint32_t rf_time(void *v){return ((fake_rf *)v)->ms++;}
static st_result rf_exchange(void *v,const uint8_t *tx,uint8_t *rx,size_t n)
{
 fake_rf *f=v;assert(f->selected && n>=2);memset(rx,0,n);
 if(++f->calls==f->fail_at)return ST_BUS;
 uint8_t reg=(tx[0]>>1)&0x3f;
 if(tx[0]&0x80) {
  if(reg==9){
   for(size_t i=1;i<n;++i){assert(tx[i-1]==0x92);rx[i]=f->fifo_pos<f->fifo_n?f->fifo[f->fifo_pos++]:0;}
   assert(tx[n-1]==0);
  }
  else if(reg==10)rx[1]=(uint8_t)f->fifo_n;
  else if(reg==6)rx[1]=f->error;
  else if(reg==12)rx[1]=f->lastbits;
  else rx[1]=f->reg[reg];
 }else {
  if(reg==4)f->reg[4]=0;
  else if(reg==10 && (tx[1]&0x80)){f->tx_n=0;f->fifo_n=0;f->fifo_pos=0;}
  else if(reg==9){assert(f->tx_n<16);f->tx[f->tx_n++]=tx[1];}
  else {f->reg[reg]=tx[1];
   if(reg==13 && (tx[1]&0x80)){
    unsigned idx=f->frame<8?f->frame:7;memcpy(f->sent[idx],f->tx,f->tx_n);f->sent_n[idx]=(uint8_t)f->tx_n;
    f->fifo_n=f->lengths[idx];memcpy(f->fifo,f->frames[idx],f->fifo_n);f->fifo_pos=0;
    f->reg[4]=f->hang?0:(f->fifo_n?0x30:0x11);++f->frame;
   }
  }
 }
 return ST_OK;
}
static st_rfid make_rf(fake_rf *f)
{memset(f,0,sizeof *f);f->reg[0x37]=0x92;st_rfid r={.io={f,rf_exchange,rf_select,rf_time}};return r;}
static void frame(fake_rf *f,unsigned n,const uint8_t *bytes,size_t count)
{assert(count<=8);f->lengths[n]=(uint8_t)count;memcpy(f->frames[n],bytes,count);}
static void sak(fake_rf *f,unsigned n,uint8_t value)
{uint16_t c=st_crc_a(&value,1);uint8_t b[3]={value,(uint8_t)c,(uint8_t)(c>>8)};frame(f,n,b,3);}
static void four_byte_card(fake_rf *f)
{const uint8_t atqa[]={4,0},uid[]={1,2,3,4,4};frame(f,0,atqa,2);frame(f,1,uid,5);sak(f,2,8);}
typedef struct {
 uint8_t q[1024],block[512],commands[64];unsigned qn,qp,calls,fail_at,ncmd,write_pos;
 uint32_t args[64],ms;bool selected,high_capacity,bad_echo,init_hang,read_hang,busy_hang,reject_write;
 unsigned write_stage;bool pending_busy;
} fake_sd;
static void sd_select(void *v,bool on)
{fake_sd *f=v;f->selected=on;if(!on){f->qn=f->qp=0;f->write_stage=0;f->pending_busy=false;}}
static uint32_t sd_time(void *v){return ((fake_sd *)v)->ms++;}
static void queue(fake_sd *f,uint8_t b){assert(f->qn<1024);f->q[f->qn++]=b;}
static st_result sd_exchange(void *v,const uint8_t *tx,uint8_t *rx,size_t n)
{
 fake_sd *f=v;memset(rx,0xff,n);if(++f->calls==f->fail_at)return ST_BUS;
 if(n==6 && f->selected && ((tx[0]&0xc0)==0x40)) {
  uint8_t c=tx[0]&0x3f;uint32_t a=((uint32_t)tx[1]<<24)|((uint32_t)tx[2]<<16)|((uint32_t)tx[3]<<8)|tx[4];
  assert(f->ncmd<64);f->commands[f->ncmd]=c;f->args[f->ncmd++]=a;f->qn=f->qp=0;
  if(c==0){assert(tx[5]==0x95);queue(f,1);}
  else if(c==8){assert(tx[5]==0x87);queue(f,1);queue(f,0);queue(f,0);queue(f,1);queue(f,f->bad_echo?0xab:0xaa);}
  else if(c==55)queue(f,1);
  else if(c==41){queue(f,f->init_hang?1:0);if(f->init_hang)f->ms+=100;}
  else if(c==58){queue(f,0);queue(f,f->high_capacity?0xc0:0x80);queue(f,0xff);queue(f,0x80);queue(f,0);}
  else if(c==16){assert(a==512);queue(f,0);}
  else if(c==17){queue(f,0);if(!f->read_hang){queue(f,0xfe);for(unsigned i=0;i<512;++i)queue(f,f->block[i]);queue(f,0xff);queue(f,0xff);}}
  else if(c==24){queue(f,0);f->write_stage=1;f->write_pos=0;}
  else assert(!"unexpected SD command");
  return ST_OK;
 }
 assert(n==1);
 if(f->qp<f->qn){rx[0]=f->q[f->qp++];return ST_OK;}
 if(!f->selected)return ST_OK;
 if(f->write_stage==1){if(tx[0]==0xfe)f->write_stage=2;return ST_OK;}
 if(f->write_stage==2){if(f->write_pos<512)f->block[f->write_pos]=tx[0];if(++f->write_pos==514){f->write_stage=0;queue(f,f->reject_write?0x0b:5);f->pending_busy=true;}return ST_OK;}
 if(f->pending_busy){if(f->busy_hang)rx[0]=0;else {rx[0]=0xff;f->pending_busy=false;}return ST_OK;}
 return ST_OK;
}
static st_sd make_sd(fake_sd *f)
{memset(f,0,sizeof *f);st_sd s={.io={f,sd_exchange,sd_select,sd_time}};return s;}

#endif
