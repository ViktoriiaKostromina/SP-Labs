"""Decode mode-0 SPI directly from the saved Wokwi VCD (no generated samples)."""
import argparse,hashlib,json,statistics
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('vcd',type=Path);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
v={};buf={};out=[];t=0;rises=[];adc=[];start={};dur=[]
for l in a.vcd.read_text().splitlines():
 if l.startswith('#'):t=int(l[1:])
 elif len(l)==2 and l[0] in '01':
  x=l[1];old=v.get(x);v[x]=int(l[0])
  if x=="'":
   if old==0 and v[x]:adc.append(t);start[x]=t
   elif old==1 and not v[x] and x in start:dur.append(t-start[x])
  if x in '$%':
   if old==1 and v[x]==0:buf[x]=[t,[],[],0,0,0]
   elif old==0 and v[x]==1 and x in buf:
    b=buf.pop(x);out.append({'device':'RFID' if x=='$' else 'SD','start_ns':b[0],'end_ns':t,'mosi':bytes(b[1]).hex(),'miso':bytes(b[2]).hex(),'trailing_bits':b[5]})
  if x=='!' and old==0 and v[x]==1:
   if buf:rises.append(t)
   for cs,b in buf.items():
    b[3]=(b[3]<<1)|v.get('"',0);b[4]=(b[4]<<1)|v.get('#',0);b[5]+=1
    if b[5]==8:b[1].append(b[3]);b[2].append(b[4]);b[3]=b[4]=b[5]=0
period=[y-x for x,y in zip(rises,rises[1:]) if 2000<=y-x<=4000]
summary={'source':str(a.vcd),'sha256':hashlib.sha256(a.vcd.read_bytes()).hexdigest(),'method':'mode 0, MSB first, sample MOSI/MISO at SCK rising edge while CS low; 1 ns VCD units','transactions':len(out),'sck_period_ns':{'n':len(period),'min':min(period) if period else None,'median':statistics.median(period) if period else None,'max':max(period) if period else None},'adc_samples':len(adc),'adc_pulse_ns':{'min':min(dur) if dur else None,'max':max(dur) if dur else None},'adc_intervals_ms':[(y-x)/1e6 for x,y in zip(adc,adc[1:])]}
a.out.mkdir(parents=True,exist_ok=True);(a.out/'spi.json').write_text(json.dumps(out,indent=2));(a.out/'trace-summary.json').write_text(json.dumps(summary,indent=2));print(json.dumps({k:v for k,v in summary.items() if k!='adc_intervals_ms'},indent=2))
