"""Translate this installation's extracted PvZ strings; keep format tokens intact."""
import html
import json
import os
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent
os.environ['XDG_DATA_HOME'] = str(ROOT / 'argos-data')
os.environ['XDG_CONFIG_HOME'] = str(ROOT / 'argos-config')
os.environ['XDG_CACHE_HOME'] = str(ROOT / 'argos-cache')
os.environ['ARGOS_PACKAGES_DIR'] = str(ROOT / 'argos-packages')
from argostranslate import translate

source = {key.strip(): value for key, value in json.loads(
    (ROOT / 'reference/strings-en.json').read_text(encoding='utf-8')).items()}
checkpoint = ROOT / 'translation-progress.json'
done = json.loads(checkpoint.read_text(encoding='utf-8')) if checkpoint.exists() else {}
done = {key.strip(): value for key, value in done.items()}
names = '''PEASHOOTER 豌豆射手
SUNFLOWER 向日葵
CHERRY_BOMB 樱桃炸弹
WALL_NUT 坚果墙
REPEATER 双发射手
POTATO_MINE 土豆地雷
SNOW_PEA 寒冰射手
CHOMPER 大嘴花
PUFF_SHROOM 小喷菇
SUN_SHROOM 阳光菇
FUME_SHROOM 大喷菇
GRAVE_BUSTER 墓碑吞噬者
HYPNO_SHROOM 魅惑菇
SCAREDY_SHROOM 胆小菇
ICE_SHROOM 寒冰菇
DOOM_SHROOM 毁灭菇
LILY_PAD 睡莲
SQUASH 窝瓜
THREEPEATER 三线射手
TANGLE_KELP 缠绕水草
JALAPENO 火爆辣椒
SPIKEWEED 地刺
TORCHWOOD 火炬树桩
TALL_NUT 高坚果
SEA_SHROOM 海蘑菇
PLANTERN 路灯花
CACTUS 仙人掌
BLOVER 三叶草
SPLIT_PEA 裂荚射手
STARFRUIT 杨桃
PUMPKIN 南瓜罩
MAGNET_SHROOM 磁力菇
CABBAGE_PULT 卷心菜投手
FLOWER_POT 花盆
KERNEL_PULT 玉米投手
GARLIC 大蒜
UMBRELLA_LEAF 叶子保护伞
MARIGOLD 金盏花
MELON_PULT 西瓜投手
GATLING_PEA 机枪射手
TWIN_SUNFLOWER 双子向日葵
GLOOM_SHROOM 忧郁菇
CATTAIL 香蒲
WINTER_MELON 冰西瓜投手
GOLD_MAGNET 吸金磁
SPIKEROCK 地刺王
COB_CANNON 玉米加农炮
IMITATER 模仿者
COFFEE_BEAN 咖啡豆
EXPLODE_O_NUT 爆炸坚果
GIANT_WALLNUT 巨型坚果
SPROUT 幼苗
MARIGOLD_SPROUT 金盏花幼苗'''
manual = dict(line.split(' ', 1) for line in names.splitlines())
manual.update({'LOCALE': 'Chinese_China', 'CURRENCY_SYMBOL': '¥',
               'CURRENCY_STRING': '{CURRENCY_SYMBOL}{AMOUNT}',
               'ZOMBIE': '普通僵尸', 'FLAG_ZOMBIE': '旗帜僵尸',
               'CONEHEAD_ZOMBIE': '路障僵尸', 'BUCKETHEAD_ZOMBIE': '铁桶僵尸',
               'FOOTBALL_ZOMBIE': '橄榄球僵尸', 'IMP': '小鬼僵尸',
               'GARGANTUAR': '巨人僵尸', 'DIGGER_ZOMBIE': '矿工僵尸',
               'BUNGEE_ZOMBIE': '蹦极僵尸'})
protected = re.compile(r'(\{[^{}]+\}|\[[A-Z0-9_]+\]|%[-+#0-9.]*[a-zA-Z]|https?://\S+|\S+@\S+\.\S+)')

def translate_text(value):
    if not re.search(r'[A-Za-z]', value):
        return value
    parts = re.split(r'(\r?\n)', value)
    translated = []
    for part in parts:
        if part in ('\n', '\r\n'):
            translated.append(part)
            continue
        tokens = protected.split(part)
        output = []
        for token in tokens:
            if protected.fullmatch(token) or not re.search(r'[A-Za-z]{2}', token):
                output.append(token)
            else:
                leading = re.match(r'^\s*', token).group()
                trailing = re.search(r'\s*$', token).group()
                middle = token[len(leading):len(token)-len(trailing) if trailing else len(token)]
                try:
                    output.append(leading + translate.translate(middle, 'en', 'zh') + trailing)
                except Exception as exc:
                    print('FAILED', repr(middle), exc, flush=True)
                    output.append(token)
        translated.append(''.join(output))
    return ''.join(translated)

for index, (key, value) in enumerate(source.items(), 1):
    if key not in done:
        done[key] = manual[key] if key in manual else translate_text(value)
        if index % 20 == 0:
            checkpoint.write_text(json.dumps(done, ensure_ascii=False, indent=2), encoding='utf-8')
            print(f'{index}/{len(source)} {key}', flush=True)
checkpoint.write_text(json.dumps(done, ensure_ascii=False, indent=2), encoding='utf-8')

base = (ROOT.parent / 'properties/default.xml').read_text(encoding='utf-8')
extra = {}
for key, val in re.findall(r'<String id="([^"]+)">(.*?)</String>', base, re.S):
    if key not in done:
        extra[key] = translate_text(re.sub(r'&cr;', '\n', re.sub(r'&nbsp;', ' ', html.unescape(val))))
extra.update(done)
out = ROOT / 'upstream/mods/evilsword/resources/properties/default.xml'
out.parent.mkdir(parents=True, exist_ok=True)
with out.open('w', encoding='utf-8', newline='\n') as file:
    file.write('<?xml version="1.0" encoding="UTF-8"?>\n<Properties version="1.0" language="zh" country="CN">\n')
    for key, value in extra.items():
        escaped = html.escape(value, quote=False).replace('\r\n', '&cr;').replace('\n', '&cr;')
        file.write(f'  <String id="{key}">{escaped}</String>\n')
    file.write('</Properties>\n')
print('wrote', out, len(extra), 'strings', flush=True)
