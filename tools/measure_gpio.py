"""Independent GPIO and UART edge analysis of the native GPIO probe capture."""
import argparse,bisect,hashlib,json,re,statistics
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('vcd',type=Path);p.add_argument('--out',type=Path,required=True);a=p.parse_args();names={};signals={};t=0
for l in a.vcd.read_text().splitlines():
 if l.startswith('$var '):q=l.split();names[q[3]]=q[4];signals[q[4]]=[]
 elif l.startswith('#'):t=int(l[1:])
 elif len(l)==2 and l[0] in '01' and l[1] in names:signals[names[l[1]]].append((t,int(l[0])))
u=signals['UART_TX'];ts=[x[0] for x in u];vs=[x[1] for x in u];bit=1e9/115200;next_start=-1;data=[];byte_times=[]
for (t,v),(before,old) in zip(u[1:],u):
 if v!=0 or old!=1 or t<next_start:continue
 def val(at):return vs[max(0,bisect.bisect_right(ts,at)-1)]
 b=sum(val(t+(i+1.5)*bit)<<i for i in range(8))
 if val(t+9.5*bit)!=1:continue
 data.append(b);byte_times.append(t);next_start=t+9.8*bit
raw=bytes(data);rows=[];pos=0
for line in raw.splitlines(keepends=True):
 text=line.decode('ascii',errors='replace').strip();rows.append({'uart_start_ns':byte_times[pos] if pos<len(byte_times) else None,'text':text});pos+=len(line)
presses=[]
for name in ('PAUSE','CLOSE'):
 ev=signals[name];bursts=[];b=[]
 for t,v in ev[1:]:
  if b and t-b[-1][0]>10000000:bursts.append(b);b=[]
  b.append((t,v))
 if b:bursts.append(b)
 for b in bursts:
  if b[-1][1]!=0:continue
  msg=next((q for q in rows if 'BUTTON ' in q['text'] and 'name='+name in q['text'] and q['uart_start_ns']>=b[-1][0]),None)
  if msg:presses.append({'name':name,'first_edge_ns':b[0][0],'stable_low_ns':b[-1][0],'bounce_transitions':len(b),'bounce_ms':(b[-1][0]-b[0][0])/1e6,'uart_start_ns':msg['uart_start_ns'],'first_edge_to_uart_ms':(msg['uart_start_ns']-b[0][0])/1e6,'stable_low_to_uart_ms':(msg['uart_start_ns']-b[-1][0])/1e6,'line':msg['text']})
adc=signals['ADC_SAMPLE'];rises=[t for t,v in adc if v];widths=[];on=None
for t,v in adc:
 if v:on=t
 elif on is not None:widths.append(t-on);on=None
period=[(y-x)/1e6 for x,y in zip(rises,rises[1:]) if 0<y-x<100000000]
def stat(x):return {'n':len(x),'min':min(x) if x else None,'mean':statistics.mean(x) if x else None,'max':max(x) if x else None}
x={'source':str(a.vcd),'sha256':hashlib.sha256(a.vcd.read_bytes()).hexdigest(),'method':'1 ns VCD units; UART 115200 8N1 sampled at bit centers; button bursts separated by >10 ms; capture-off gaps >100 ms excluded from ADC periodic interval statistics','buttons':presses,'adc_pulse_us':stat([x/1000 for x in widths]),'adc_interval_ms':stat(period),'uart_rows':rows}
a.out.mkdir(parents=True,exist_ok=True);(a.out/'gpio-metrics.json').write_text(json.dumps(x,indent=2));(a.out/'uart-decoded.txt').write_bytes(raw);print(json.dumps({k:v for k,v in x.items() if k!='uart_rows'},indent=2))
