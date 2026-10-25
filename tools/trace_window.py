"""Lossless time-window extraction from native VCD; retain source SHA and origin."""
import argparse,hashlib,json
from decimal import Decimal
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('source',type=Path);p.add_argument('--start-ms',required=True);p.add_argument('--end-ms',required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args();start=int(Decimal(a.start_ms)*1000000);end=int(Decimal(a.end_ms)*1000000)
assert end>start>=0
raw=a.source.read_bytes();lines=raw.decode().splitlines();cut=next(i for i,s in enumerate(lines) if s=='$enddefinitions $end');header=lines[:cut];header[0]='$version ShiftToken derived time window; native source retained separately $end'
sha=hashlib.sha256(raw).hexdigest();header+=['$comment source_sha256='+sha+' origin_ns='+str(start)+' $end','$enddefinitions $end'];v={};events=[];t=0
for s in lines[cut+1:]:
 if s.startswith('#'):t=int(s[1:])
 elif len(s)==2 and s[0] in '01xz':
  if t<=start:v[s[1]]=s[0]
  elif t<=end:events.append((t,s))
out=header+['#0']+[value+key for key,value in sorted(v.items())];last=0
for t,s in events:
 if t!=last:out.append('#'+str(t-start));last=t
 out.append(s)
out.append('#'+str(end-start));a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_text('\n'.join(out)+'\n')
meta={'source':str(a.source),'source_sha256':sha,'start_ns':start,'end_ns':end,'derived_vcd_sha256':hashlib.sha256(a.out.read_bytes()).hexdigest(),'operation':'select time interval, insert actual initial pin states, subtract common time origin; no synthetic transitions'}
a.out.with_suffix('.provenance.json').write_text(json.dumps(meta,indent=2));print(json.dumps(meta))
