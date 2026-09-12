"""Read-only installed-game resource audit. Does not run game or patch scripts."""
from pathlib import Path
import struct,json,re,hashlib,collections,argparse
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--steam-common', type=Path, default=Path(r'D:\Game\steamapps\common'))
ROOT=parser.parse_args().steam_common
OUT=Path(__file__).resolve().parents[1]/'docs'
def index(path):
    with path.open('rb') as f:
        count=struct.unpack('<I',f.read(4))[0];assert count<1000000
        entries=[]
        for _ in range(count):
            n=struct.unpack('<H',f.read(2))[0];name=f.read(n).decode('utf-8');size=struct.unpack('<I',f.read(4))[0]
            entries.append(dict(name=name,size=size))
        offset=f.tell()
        for e in entries:e['offset']=offset;offset+=e['size']
        assert offset==path.stat().st_size,'Archive directory does not match file length'
    return {e['name']:e for e in entries}
def read(f,e):f.seek(e['offset']);return f.read(e['size'])
gpak=ROOT/'Mewgenics/resources.gpak';es=index(gpak);levels=[n for n in es if n.startswith('levels/') and n.endswith('.lvl')]
with gpak.open('rb') as f:
    headers=collections.Counter();hashes=set()
    for n in levels:
        b=read(f,es[n]);v,w,h=struct.unpack_from('<III',b);headers[f'v{v} {w}x{h}']+=1;hashes.add(hashlib.sha256(b).hexdigest())
    alley=read(f,es['data/maps/alley.gon']).decode().replace('\r','')
    tiles=read(f,es['data/tiles.gon']).decode().replace('\r','')
    assert 'folder alley' in alley and 'easy [easy]' in alley and 'rare [rare]' in alley
    assert re.search(r'GrassTile\s+80\s+TallGrassTile\s+15\s+BlankTile\s+5',tiles)
    grass_weights={'GrassTile':80,'TallGrassTile':15,'BlankTile':5}
    # Compare only relevant assets with the user's existing backup; never modify either archive.
    backup=gpak.with_suffix('.gpak.bak');comparison={}
    if backup.exists():
        bes=index(backup)
        with backup.open('rb') as bf:
            for name in ['data/maps/alley.gon','data/tiles.gon','levels/alley/easy/101.lvl']:
                comparison[name]=name in bes and read(f,es[name])==read(bf,bes[name])
base=ROOT/'Into the Breach';maps=list((base/'maps').glob('*.map'));tags=collections.Counter();dims=collections.Counter();maprows=[]
for p in maps:
    s=p.read_text(encoding='utf-8-sig');dim=re.search(r'\["dimensions"\]\s*=\s*Point\(\s*(\d+),\s*(\d+)',s)
    assert dim,p.name
    w,h=map(int,dim.groups());dims[f'{w}x{h}']+=1
    match=re.search(r'\["tags"\]\s*=\s*\{([^}]*)\}',s);ts=re.findall(r'"([^"]+)"',match.group(1)) if match else []
    tags.update(ts);maprows.append({'name':p.name,'tags':ts})
mission=(base/'scripts/missions/missions.lua').read_text();assert 'function Mission:GetMapTag()' in mission and 'random_element(self.MapTags)' in mission
report={'scope':'Static read-only resource tests, NOT in-game seed replay or complete engine reverse engineering',
 'mewgenics':{'archive_entries':len(es),'level_files':len(levels),'distinct_level_file_hashes':len(hashes),'level_header_dimensions':dict(headers),'alley_easy_files':sum(n.startswith('levels/alley/easy/') for n in levels),'chapter_configuration_count':sum(n.startswith('data/maps/') for n in es),'grass_weights_from_config':grass_weights,'matches_existing_backup':comparison},
 'into_the_breach':{'map_files':len(maps),'dimensions':dict(dims),'tags':dict(tags.most_common()),'map_tag_selection_function_found':True},
 'checks':'PASS: archive offsets/file size, selected configuration fields, all level headers readable, all ITB map dimensions readable, mission selection source present'}
OUT.mkdir(exist_ok=True);(OUT/'InstalledMapAudit.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(report,ensure_ascii=False,indent=2))
