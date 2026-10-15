"""Terminology and UI review pass for the offline Chinese translation."""
import json
import re
from pathlib import Path

root=Path(__file__).resolve().parent
path=root/'translation-progress.json'
data=json.loads(path.read_text(encoding='utf-8'))
overrides={
    'TREE_OF_WISDOM_2':'大嘴花和坚果墙配合得非常默契。毕竟它们在大学时曾是室友。',
    'CREDITS_ROLES1':'^植物大战僵尸',
    'ALMANAC_INDEX':'图鉴目录', 'ALMANAC_BUTTON':'图鉴',
    'SHOP_BUTTON':'商店', 'VIEW_LAWN':'查看草坪',
    'VIEW_PLANTS':'查看植物', 'VIEW_ZOMBIES':'查看僵尸',
    'RESTART_LABEL':'重新开始', 'RESTART_BUTTON':'重新开始',
    'ROLL_CREDITS':'制作人员', 'STORE_MAIN_MENU_BUTTON':'主菜单',
    'STORE_NEXT_LEVEL_BUTTON':'下一关', 'STORE_BACK_TO_GAME':'返回游戏',
    'STORE_BACK_TO_ZEN':'返回', 'LEVEL':'关卡',
    'ENDLESS_STREAK':'连胜：{STREAK}',
    'SURVIVAL_DAY_NORMAL':'生存模式：白天',
    'SURVIVAL_NIGHT_NORMAL':'生存模式：黑夜',
    'SURVIVAL_POOL_NORMAL':'生存模式：泳池',
    'SURVIVAL_FOG_NORMAL':'生存模式：浓雾',
    'SURVIVAL_ROOF_NORMAL':'生存模式：屋顶',
    'SURVIVAL_DAY_HARD':'生存模式：白天（困难）',
    'SURVIVAL_NIGHT_HARD':'生存模式：黑夜（困难）',
    'SURVIVAL_POOL_HARD':'生存模式：泳池（困难）',
    'SURVIVAL_FOG_HARD':'生存模式：浓雾（困难）',
    'SURVIVAL_ROOF_HARD':'生存模式：屋顶（困难）',
    'SURVIVAL_DAY_ENDLESS':'生存模式：白天（无尽）',
    'SURVIVAL_NIGHT_ENDLESS':'生存模式：黑夜（无尽）',
    'SURVIVAL_POOL_ENDLESS':'生存模式：泳池（无尽）',
    'SURVIVAL_FOG_ENDLESS':'生存模式：浓雾（无尽）',
    'SURVIVAL_ROOF_ENDLESS':'生存模式：屋顶（无尽）',
    'REQUIRES_REPEATER':'需要双发射手',
    'REQUIRES_MELONPULT':'需要西瓜投手',
    'REQUIRES_SUNFLOWER':'需要向日葵',
    'REQUIRES_SPIKEWEED':'需要地刺',
    'REQUIRES_KERNELPULTS':'需要两株玉米投手',
    'REQUIRES_MAGNETSHROOM':'需要磁力菇',
    'REQUIRES_FUMESHROOM':'需要大喷菇',
    'REQUIRES_LILY_PAD':'需要睡莲',
    'WAITING_TO_REFRESH':'冷却中',
    'CLICK_TO_VIEW':'点击查看',
    'LETS_ROCK_BUTTON':'开始战斗！',
    'SEED_CHOOSER_SUN_WARNING':'你没有选择生产阳光的植物，确定继续吗？',
    'SEED_CHOOSER_NIGHT_SUN_WARNING':'夜晚不会自然掉落阳光，确定不选择向日葵或阳光菇吗？',
    'SEED_CHOOSER_LILY_WARNING':'没有睡莲就无法在水面上种植大多数植物，确定继续吗？',
    'SEED_CHOOSER_PUFFSHROOM_WARNING':'没有小喷菇，这一关会很难。确定继续吗？',
    'SEED_CHOOSER_POOL_WARNING':'你没有选择水生植物，确定继续吗？',
    'SEED_CHOOSER_ROOF_WARNING':'没有花盆就无法在屋顶上种植，确定继续吗？',
    'ADVICE_BEGHOULED_ALMOST_THERE':'还差 5 次匹配！',
    'ZOMBIQUARIUM_SNORKEL_TOOLTIP':'购买潜水僵尸',
    'ZOMBIQUARIUM_TROPHY_TOOLTIP':'购买奖杯',
    'ADVICE_ZOMBIQUARIUM_CLICK_TO_FEED':'点击水族箱喂养僵尸',
    'ADVICE_ZOMBIQUARIUM_COLLECT_SUN':'收集 {SCORE} 阳光即可过关',
    'ADVICE_ZOMBIQUARIUM_BUY_SNORKEL':'点击购买更多潜水僵尸！',
    'ONE_MORE_SCARY_POTTER_TOOLTIP':'再获得 1 个砸罐子奖杯即可解锁',
    'ONE_MORE_IZOMBIE_TOOLTIP':'再获得 1 个我是僵尸奖杯即可解锁',
    'ONE_MORE_SURVIVAL_TOOLTIP':'再获得 1 个生存模式奖杯即可解锁',
    '10_SURVIVAL_TOOLTIP':'获得 10 个生存模式奖杯即可解锁',
    'CHOMPER_DESCRIPTION_HEADER':'大嘴花可以一口吞掉一只僵尸，但咀嚼时十分脆弱。',
    'LILY_PAD_DESCRIPTION_HEADER':'睡莲允许你在水面种植非水生植物。',
    'SQUASH_DESCRIPTION_HEADER':'窝瓜会压扁第一只靠近它的僵尸。',
    'TANGLE_KELP_DESCRIPTION_HEADER':'缠绕水草会把靠近它的第一只僵尸拖入水中。',
    'JALAPENO_DESCRIPTION_HEADER':'火爆辣椒会消灭整行僵尸。',
    'SPIKEWEED_DESCRIPTION_HEADER':'地刺会扎破轮胎并伤害踩上它的僵尸。',
    'COB_CANNON_DESCRIPTION_HEADER':'点击玉米加农炮，向指定位置发射高威力炮弹。',
    'IMITATER_DESCRIPTION_HEADER':'模仿者让你在一关中选择同一种植物两次！',
}
glossary={
    'Pole Vaulting Zombie':'撑杆僵尸','Conehead Zombie':'路障僵尸',
    'Buckethead Zombie':'铁桶僵尸','Snorkel Zombie':'潜水僵尸',
    'Puff-shroom':'小喷菇','Fume-shroom':'大喷菇',
    'Hypno-shroom':'魅惑菇','Scaredy-shroom':'胆小菇',
    'Gloom Shroom':'忧郁菇','Lily Pad':'睡莲',
    'Grave Buster':'墓碑吞噬者','Tangle Kelp':'缠绕水草',
    'Cob Cannon':'玉米加农炮','Wall-nut':'坚果墙',
    'Chompers':'大嘴花','Chomper':'大嘴花','chomps':'吞食',
    'Spikeweed':'地刺','Plantern':'路灯花','Blover':'三叶草',
    'Imitaters':'模仿者','Imitater':'模仿者',
    'Gargantuars':'巨人僵尸','Gargantuar':'巨人僵尸',
    'Bungee':'蹦极','Zomboss':'僵王博士','Zombot':'僵尸机器人',
    'Zombiquarium':'僵尸水族馆','ZomBotany':'植物僵尸',
    'Zomboni':'冰车僵尸','Zamboni':'冰车僵尸',
    'Pogo':'跳跳僵尸','Fireballs':'火球','Jalapenos':'火爆辣椒',
    'Dave':'戴夫','Almanac':'图鉴',
}
for key,value in list(data.items()):
    if key in overrides: continue
    for en,zh in sorted(glossary.items(),key=lambda pair:-len(pair[0])):
        value=re.sub(r'(?<![A-Za-z_{])'+re.escape(en)+r'(?![A-Za-z_}])',zh,value,flags=re.I)
    for bad,good in {'阿玛纳茨':'图鉴','阿马纳茨':'图鉴','百合垫':'睡莲',
                     '烟雾室':'大喷菇','喷气室':'小喷菇','磁性洗手间':'磁力菇',
                     '磁气室':'磁力菇','壁核':'坚果墙','玉米蜘蛛':'玉米炮弹',
                     '太阳':'阳光','损害':'伤害','屏幕门':'铁栅门'}.items():
        value=value.replace(bad,good)
    data[key]=value
data.update(overrides)
path.write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8')
print('proofread',len(data),'keys')
