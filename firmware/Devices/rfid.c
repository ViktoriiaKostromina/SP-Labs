#include "rfid.h"
#include <string.h>
uint16_t st_crc_a(const uint8_t *p, size_t n)
{
    uint16_t crc = 0x6363u;
    for (size_t i = 0; i < n; ++i) {
        uint8_t x = p[i] ^ (uint8_t)crc;
        x ^= (uint8_t)(x << 4);
        crc = (uint16_t)((crc >> 8) ^ ((uint16_t)x << 8) ^ ((uint16_t)x << 3) ^ (x >> 4));
    }
    return crc;
}
static st_result reg_transfer(st_rfid *r, uint8_t reg, uint8_t *v, bool read)
{
    uint8_t tx[2] = {(uint8_t)((reg << 1) | (read ? 0x80u : 0u)), read ? 0 : *v};
    uint8_t rx[2] = {0};
    r->io.select(r->io.ctx, true);
    st_result s = r->io.exchange(r->io.ctx, tx, rx, 2);
    r->io.select(r->io.ctx, false);
    if (s == ST_OK && read) *v = rx[1];
    return s;
}
st_result st_rfid_read_reg(st_rfid *r, uint8_t reg, uint8_t *v)
{ return reg_transfer(r, reg, v, true); }
static st_result wr(st_rfid *r, uint8_t reg, uint8_t v)
{ return reg_transfer(r, reg, &v, false); }
st_result st_rfid_init(st_rfid *r)
{
    r->ready = false;
    st_result s = st_rfid_read_reg(r, 0x37, &r->version);
    if (s != ST_OK) return s;
    if (r->version != 0x91 && r->version != 0x92) return ST_ID;
    /* ISO14443A 106 kBd, RF timer ~12.5 ms: 13.56MHz/(2*169+1)/500. */
    static const uint8_t setup[][2] = {{0x01,0x00},{0x2a,0x80},{0x2b,0xa9},
        {0x2c,0x01},{0x2d,0xf4},{0x15,0x40},{0x11,0x3d},{0x12,0},{0x13,0},{0x24,0x26},{0x14,0x83}};
    for (size_t i = 0; i < sizeof setup / sizeof setup[0]; ++i) {
        s = wr(r, setup[i][0], setup[i][1]);
        if (s != ST_OK) return s;
    }
    r->ready = true;
    return ST_OK;
}
static st_result transceive(st_rfid *r, const uint8_t *tx, size_t n,
                            uint8_t bits, uint8_t *rx, size_t *got)
{
    st_result s;
#define WRITE(a,b) do { s=wr(r,(a),(b)); if(s!=ST_OK)return s; } while(0)
    WRITE(0x01, 0); WRITE(0x04,0x7f); WRITE(0x0a,0x80);
    for (size_t i=0;i<n;++i) WRITE(0x09,tx[i]);
    WRITE(0x0d,bits); WRITE(0x01,0x0c); WRITE(0x0d,(uint8_t)(0x80|bits));
    uint32_t start=r->io.now_ms(r->io.ctx); uint8_t irq=0;
    do {
        s=st_rfid_read_reg(r,0x04,&irq); if(s!=ST_OK)return s;
        if(irq&0x20)break; /* RxIRq; IdleIRq alone is not received data. */
        if(irq&1)return ST_NO_CARD;
    } while ((uint32_t)(r->io.now_ms(r->io.ctx)-start)<25u);
    if(!(irq&0x20))return ST_TIMEOUT;
    uint8_t error=0,level=0,lastbits=0;
    s=st_rfid_read_reg(r,0x06,&error);if(s!=ST_OK)return s;
    if(error&0xdf)return ST_PROTOCOL;
    s=st_rfid_read_reg(r,0x0a,&level);if(s!=ST_OK)return s;
    s=st_rfid_read_reg(r,0x0c,&lastbits);if(s!=ST_OK)return s;
    if(level==0 || level>*got || (lastbits&7u))return ST_LENGTH;
    *got=level;
    /* FIFO burst: repeat the read address, terminate with zero, keep CS low.
       This is the MFRC522 multi-byte read protocol, not independent reads. */
    uint8_t fifo_tx[65],fifo_rx[65];
    memset(fifo_tx,0x92,level);fifo_tx[level]=0;
    r->io.select(r->io.ctx,true);
    s=r->io.exchange(r->io.ctx,fifo_tx,fifo_rx,(size_t)level+1);
    r->io.select(r->io.ctx,false);
    if(s!=ST_OK)return s;
    memcpy(rx,fifo_rx+1,level);
    return ST_OK;
#undef WRITE
}
st_result st_rfid_block(const uint8_t block[5])
{ return (uint8_t)(block[0]^block[1]^block[2]^block[3])==block[4] ? ST_OK:ST_BCC; }
st_result st_rfid_poll(st_rfid *r, st_uid *uid)
{
    uid->length=0;
    if(!r->ready)return ST_NOT_READY;
    uint8_t version=0;
    st_result s=st_rfid_read_reg(r,0x37,&version);
    if(s!=ST_OK)return s;
    if(version!=r->version)return ST_ID;
    uint8_t wake=0x52,atqa[2];size_t n=2;
    s=transceive(r,&wake,1,7,atqa,&n);if(s!=ST_OK)return s;
    if(n!=2)return ST_LENGTH;
    for(uint8_t cascade=0;cascade<2;++cascade) {
        uint8_t cmd[9]={(uint8_t)(0x93+2*cascade),0x20},block[5];n=5;
        s=transceive(r,cmd,2,0,block,&n);if(s!=ST_OK)return s;
        if(n!=5)return ST_LENGTH;
        s=st_rfid_block(block);if(s!=ST_OK)return s;
        bool more=block[0]==0x88;
        if(more && cascade==1)return ST_LENGTH;
        cmd[1]=0x70;memcpy(cmd+2,block,5);
        uint16_t crc=st_crc_a(cmd,7);cmd[7]=(uint8_t)crc;cmd[8]=(uint8_t)(crc>>8);
        uint8_t sak[3];n=3;s=transceive(r,cmd,9,0,sak,&n);if(s!=ST_OK)return s;
        if(n!=3)return ST_LENGTH;
        crc=st_crc_a(sak,1);if(sak[1]!=(uint8_t)crc || sak[2]!=(uint8_t)(crc>>8))return ST_CRC;
        if(((sak[0]&4u)!=0)!=more)return ST_PROTOCOL;
        size_t add=more?3:4;memcpy(uid->bytes+uid->length,block+(more?1:0),add);uid->length+=(uint8_t)add;
        if(!more) {
            uint8_t halt[4]={0x50,0};crc=st_crc_a(halt,2);halt[2]=(uint8_t)crc;halt[3]=(uint8_t)(crc>>8);
            n=3;s=transceive(r,halt,4,0,sak,&n);
            /* HALT is specified to be silent. Other errors are not silently accepted. */
            return (s==ST_NO_CARD || s==ST_TIMEOUT) ? ST_OK : (s==ST_OK ? ST_PROTOCOL:s);
        }
    }
    return ST_LENGTH;
}
