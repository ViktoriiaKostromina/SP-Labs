from pathlib import Path
import re
errors=[];checked=0
for folder in ['App','Devices','Inputs']:
 for p in (Path('firmware')/folder).glob('*.[ch]'):
  checked+=1;s=p.read_text()
  includes=re.findall(r'^\s*#include\s*[<"]([^>"]+)',s,re.M)
  for h in includes:
   if any(x in h.lower() for x in ['stm32','hal','cmsis','board','main.h','core/']):errors.append(str(p)+': forbidden include '+h)
  if re.search(r'\bHAL_\w+\s*\(',s):errors.append(str(p)+': HAL call in portable layer')
if errors:raise SystemExit('\n'.join(errors))
print(f'PASS layer dependency checks: {checked} portable source/header files')
