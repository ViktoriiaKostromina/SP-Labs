from pathlib import Path
import gzip,hashlib
p=Path('sim/demo.img')
if p.exists():
    if p.stat().st_size!=8*1024*1024:raise SystemExit('Existing demo image has wrong size; preserve and investigate it.')
    print('Preserving existing',p)
else:
    p.write_bytes(gzip.decompress(Path('sim/sd-blank.img.gz').read_bytes()))
    print('Created separate writable demo image:',p)
print('SHA256',hashlib.sha256(p.read_bytes()).hexdigest())
