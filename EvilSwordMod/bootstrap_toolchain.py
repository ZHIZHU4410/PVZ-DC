"""Fetch official MSYS2 UCRT64 packages into a private, relocatable build prefix."""
from pathlib import Path
import concurrent.futures, hashlib, io, json, tarfile, urllib.request
import zstandard

ROOT = Path(__file__).resolve().parent
TOOLS = ROOT / 'toolchain'
CACHE = TOOLS / 'packages'
BASE = 'https://repo.msys2.org/mingw/ucrt64/'
CACHE.mkdir(parents=True, exist_ok=True)
db = urllib.request.urlopen(BASE + 'ucrt64.db', timeout=60).read()
if db.startswith(bytes.fromhex('28b52ffd')):
    db = zstandard.ZstdDecompressor().stream_reader(io.BytesIO(db)).read()
packages = {}
with tarfile.open(fileobj=io.BytesIO(db), mode='r:*') as archive:
    for member in archive:
        if not member.name.endswith('/desc'): continue
        fields = {}
        for section in archive.extractfile(member).read().decode().strip().split('\n\n'):
            lines = section.splitlines()
            fields[lines[0].strip('%')] = lines[1:]
        packages[fields['NAME'][0]] = fields
names = ['gcc','cmake','ninja','libjpeg-turbo','libopenmpt','libogg','libpng','libvorbis','mpg123','SDL2','zlib','lua']
needed = {}
def resolve(name):
    name = name.split('>=')[0].split('=')[0].split('>')[0].split('<')[0]
    if name in needed: return
    if name not in packages:
        providers = [p for p in packages.values() if any(x.split('=')[0] == name for x in p.get('PROVIDES', []))]
        if not providers: raise RuntimeError('Unknown dependency: '+name)
        return resolve(providers[0]['NAME'][0])
    pkg = packages[name]
    needed[name] = pkg
    for dependency in pkg.get('DEPENDS', []): resolve(dependency)
for name in names: resolve('mingw-w64-ucrt-x86_64-'+name)
print('Packages:', len(needed), 'download MB:', round(sum(int(p['CSIZE'][0]) for p in needed.values())/1e6), flush=True)
def fetch(pkg):
    dest = CACHE / pkg['FILENAME'][0]
    sha = pkg['SHA256SUM'][0]
    if not dest.exists() or hashlib.sha256(dest.read_bytes()).hexdigest() != sha:
        for attempt in range(3):
            try:
                with urllib.request.urlopen(BASE+dest.name, timeout=120) as response:
                    dest.write_bytes(response.read())
                if hashlib.sha256(dest.read_bytes()).hexdigest() != sha: raise ValueError('SHA256 mismatch')
                break
            except Exception:
                if attempt == 2: raise
    return dest
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
    links = []
    for dest in pool.map(fetch, needed.values()):
        print('Extract:', dest.name, flush=True)
        with dest.open('rb') as compressed:
            with zstandard.ZstdDecompressor().stream_reader(compressed) as stream:
                with tarfile.open(fileobj=stream, mode='r|') as archive:
                    for member in archive:
                        if not member.name.startswith('ucrt64/'): continue
                        target = (TOOLS/member.name).resolve()
                        if not target.is_relative_to(TOOLS.resolve()): raise ValueError(member.name)
                        if member.isfile():
                            target.parent.mkdir(parents=True, exist_ok=True)
                            target.write_bytes(archive.extractfile(member).read())
                        elif member.issym():
                            links.append((target, (target.parent/member.linkname).resolve()))
                        elif member.islnk():
                            links.append((target, (TOOLS/member.linkname).resolve()))
    for target, source in links:
        if not source.is_relative_to(TOOLS.resolve()): raise ValueError(str(source))
        if source.is_file():
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(source.read_bytes())
(TOOLS/'packages.lock.json').write_text(json.dumps(needed, indent=2), encoding='utf-8')
print('Toolchain ready:', TOOLS/'ucrt64', flush=True)
