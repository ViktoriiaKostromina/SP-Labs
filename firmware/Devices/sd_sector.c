#include "sd_sector.h"
uint32_t st_crc32(const uint8_t *p, size_t n)
{
    uint32_t c=0xffffffffu;
    for(size_t i=0;i<n;++i) {
        c^=p[i];for(unsigned b=0;b<8;++b)c=(c>>1)^((c&1)?0xedb88320u:0u);
    }
    return c^0xffffffffu;
}
static st_result byte(st_sd *s, uint8_t tx, uint8_t *rx)
{ return s->io.exchange(s->io.ctx,&tx,rx,1); }
static void end(st_sd *s)
{
    s->io.select(s->io.ctx,false);
    uint8_t ignore; (void)byte(s,0xff,&ignore);
}
static st_result command(st_sd *s, uint8_t c, uint32_t a, uint8_t *r)
{
    s->io.select(s->io.ctx,true);
    uint8_t pad;st_result e=byte(s,0xff,&pad);if(e!=ST_OK)return e;
    uint8_t tx[6]={(uint8_t)(0x40|c),(uint8_t)(a>>24),(uint8_t)(a>>16),(uint8_t)(a>>8),(uint8_t)a,
                  c==0?0x95:(c==8?0x87:1)},rx[6];
    e=s->io.exchange(s->io.ctx,tx,rx,6);if(e!=ST_OK)return e;
    for(unsigned i=0;i<16;++i) {
        e=byte(s,0xff,r);if(e!=ST_OK)return e;
        if(!(*r&0x80))return ST_OK;
    }
    return ST_TIMEOUT;
}
static st_result simple(st_sd *s,uint8_t c,uint32_t a,uint8_t expected)
{
    uint8_t r=0xff;st_result e=command(s,c,a,&r);end(s);
    return e!=ST_OK?e:(r==expected?ST_OK:ST_PROTOCOL);
}
st_result st_sd_init(st_sd *s)
{
    s->ready=false;s->block_addressing=false;
    s->io.select(s->io.ctx,false);
    uint8_t r=0xff;st_result e;
    for(unsigned i=0;i<10;++i){e=byte(s,0xff,&r);if(e!=ST_OK)return e;}
    e=simple(s,0,0,1);if(e!=ST_OK)return e;
    e=command(s,8,0x1aa,&r);
    if(e!=ST_OK){end(s);return e;}
    uint8_t r7[4];
    if(r!=1){end(s);return ST_PROTOCOL;}
    for(unsigned i=0;i<4;++i){e=byte(s,0xff,&r7[i]);if(e!=ST_OK){end(s);return e;}}
    end(s);if(r7[2]!=1 || r7[3]!=0xaa)return ST_ID;
    uint32_t t=s->io.now_ms(s->io.ctx);
    do {
        e=simple(s,55,0,1);if(e!=ST_OK)return e;
        e=command(s,41,0x40000000,&r);end(s);if(e!=ST_OK)return e;
        if(r==0)break;
        if(r!=1)return ST_PROTOCOL;
    }while((uint32_t)(s->io.now_ms(s->io.ctx)-t)<1000u);
    if(r!=0)return ST_TIMEOUT;
    e=command(s,58,0,&r);if(e!=ST_OK){end(s);return e;}
    if(r!=0){end(s);return ST_PROTOCOL;}
    s->ocr=0;
    for(unsigned i=0;i<4;++i){e=byte(s,0xff,&r);if(e!=ST_OK){end(s);return e;}s->ocr=(s->ocr<<8)|r;}
    end(s);
    if(!(s->ocr&0x80000000u))return ST_NOT_READY;
    s->block_addressing=(s->ocr&0x40000000u)!=0;
    if(!s->block_addressing){e=simple(s,16,512,0);if(e!=ST_OK)return e;}
    s->ready=true;return ST_OK;
}
static st_result begin_data(st_sd *s,uint8_t c,uint32_t lba)
{
    if(!s->ready)return ST_NOT_READY;
    /* The demonstration image is exactly 8 MiB. Never address beyond it. */
    if(lba>=16384)return ST_LENGTH;
    uint8_t r;st_result e=command(s,c,s->block_addressing?lba:lba*512u,&r);
    if(e!=ST_OK || r!=0){end(s);return e!=ST_OK?e:ST_PROTOCOL;}
    return ST_OK;
}
st_result st_sd_read(st_sd *s,uint32_t lba,uint8_t data[512])
{
    st_result e=begin_data(s,17,lba);if(e!=ST_OK)return e;
    uint32_t t=s->io.now_ms(s->io.ctx);uint8_t r=0xff;
    do {e=byte(s,0xff,&r);if(e!=ST_OK){end(s);return e;}if(r==0xfe)break;
        if(r!=0xff){end(s);return ST_PROTOCOL;}
    }while((uint32_t)(s->io.now_ms(s->io.ctx)-t)<100u);
    if(r!=0xfe){end(s);return ST_TIMEOUT;}
    for(unsigned i=0;i<512;++i){e=byte(s,0xff,&data[i]);if(e!=ST_OK){end(s);return e;}}
    /* SPI CRC is disabled after reset. Data integrity is checked by record CRC32/read-back. */
    e=byte(s,0xff,&r);if(e==ST_OK)e=byte(s,0xff,&r);end(s);return e;
}
st_result st_sd_write(st_sd *s,uint32_t lba,const uint8_t data[512])
{
    st_result e=begin_data(s,24,lba);if(e!=ST_OK)return e;
    uint8_t r;e=byte(s,0xff,&r);if(e==ST_OK)e=byte(s,0xfe,&r);
    for(unsigned i=0;e==ST_OK && i<512;++i)e=byte(s,data[i],&r);
    if(e==ST_OK)e=byte(s,0xff,&r);
    if(e==ST_OK)e=byte(s,0xff,&r);
    if(e==ST_OK)e=byte(s,0xff,&r);
    if(e!=ST_OK){end(s);return e;}
    if((r&0x1f)!=5){end(s);return ST_PROTOCOL;}
    uint32_t t=s->io.now_ms(s->io.ctx);
    do {e=byte(s,0xff,&r);if(e!=ST_OK){end(s);return e;}if(r==0xff){end(s);return ST_OK;}}
    while((uint32_t)(s->io.now_ms(s->io.ctx)-t)<500u);
    end(s);return ST_TIMEOUT;
}
