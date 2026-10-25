"""Collect real UART from this project's Wokwi RFC2217 socket; preserve raw bytes."""
import argparse,datetime,hashlib,json,socket,time
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--port',type=int,default=4049);p.add_argument('--seconds',type=float,default=30);p.add_argument('--out',type=Path,required=True);p.add_argument('--command',action='append',default=[]);p.add_argument('--wait-ready',action='store_true');a=p.parse_args()
a.out.mkdir(parents=True,exist_ok=True)
def identity():
 return {str(f):hashlib.sha256(f.read_bytes()).hexdigest() for f in map(Path,['firmware/build/firmware.elf','firmware/build/firmware.hex','diagram.json','wokwi.toml']) if f.exists()}
before=identity()
raw=bytearray();started=datetime.datetime.now(datetime.timezone.utc).isoformat();t=time.monotonic();sent=False;connections=0;errors=[]
try:
 while time.monotonic()-t<a.seconds:
  try:
   with socket.create_connection(('127.0.0.1',a.port),timeout=1) as s:
    connections+=1;s.settimeout(.25)
    while time.monotonic()-t<a.seconds:
     if not sent and a.command and (not a.wait_ready or b'READY lab=2' in raw):
      for command in a.command:s.sendall((command+'\n').encode('ascii'))
      sent=True
     try:
      data=s.recv(65536)
      if not data:break
      raw.extend(data);print(data.decode('ascii',errors='replace'),end='',flush=True)
     except socket.timeout:pass
  except OSError as e:
   if len(errors)<20:errors.append(type(e).__name__+': '+str(e))
   time.sleep(.1)
finally:
 (a.out/'uart.raw').write_bytes(raw);(a.out/'uart.txt').write_text(raw.decode('ascii',errors='replace'))
 fw=Path('firmware/build/firmware.elf')
 meta={'level':'Wokwi VS Code UART','started_utc':started,'ended_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'host_elapsed_s':time.monotonic()-t,'host_elapsed_is_not_MCU_elapsed':True,'port':a.port,'connections':connections,'commands':a.command,'commands_sent':sent,'startup_ready_observed':b'READY lab=2' in raw,'raw_sha256':hashlib.sha256(raw).hexdigest(),'firmware_elf_sha256':hashlib.sha256(fw.read_bytes()).hexdigest() if fw.exists() else None,'connection_errors':errors}
 meta.update(files_before=before,files_after=identity(),loaded_binary_note='File hashes alone do not establish what an existing simulator loaded; use BOOT and fresh native start evidence.')
 (a.out/'origin.json').write_text(json.dumps(meta,indent=2))
