"""Change only logic-analyzer probes; device wiring and firmware stay identical."""
import argparse,json
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('mode',choices=['spi','gpio']);a=p.parse_args();f=Path('diagram.json');d=json.loads(f.read_text())
if a.mode=='spi':pins=['PB3','PB5','PB4','PB0.1','PA4','PA2','PA7','PA8'];names='SCK,MOSI,MISO,RFID_CS,SD_CS,UART_TX,ADC_SAMPLE,CAPTURE';trigger='D7'
else:pins=['PA1','PA6','PA7','PA8','PA5','PA2','PB0.1','PA3'];names='PAUSE,CLOSE,ADC_SAMPLE,CAPTURE,LED,UART_TX,RFID_CS,UART_RX';trigger='D3'
for q in d['parts']:
 if q['id']=='logic':q['attrs'].update(channelNames=names,triggerPin=trigger)
for q in d['connections']:
 if q[0].startswith('logic:D'):q[1]='nucleo:'+pins[int(q[0][-1])]
f.write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n');print('probes='+a.mode+' trigger='+trigger+' HIGH; device wiring unchanged')
