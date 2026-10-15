function OnModInit()
    local seed, err = Game.RegisterPlant({
        id = "EvilSword",
        name = "诅咒剑草",
        description = "利刃能斩杀一切敌人，但剑上的诅咒，只要受到一丝伤害，持有者就会毁灭。\n近战攻击前方一格；无视护甲。被任何僵尸触碰或咬伤即枯萎，南瓜罩无法保护它。",
        cost = 175,
        cooldown = 3000,
        packetIndex = 49,
        subClass = 0,
        image = "images/EvilSword.png"
    })
    if not seed then error(err) end
    local lightning, lightningErr = Game.RegisterPlant({
        id = "LightningWhip",
        name = "电鞭藤",
        description = "挥动带电藤鞭，攻击前方约三格的僵尸并无视盾牌。第三击威力更强；电流会跳向目标附近至多三个敌人，造成一半直接伤害，并留下两秒电击。",
        cost = 175,
        cooldown = 1500,
        packetIndex = 50,
        subClass = 0,
        image = "images/LightningWhip.png"
    })
    if not lightning then error(lightningErr) end
    local revenge, revengeErr = Game.RegisterPlant({
        id = "RevengeSword",
        name = "复仇剑草",
        description = "厚实的叶甲赋予它 2400 点生命。挥剑攻击前方一格，按四段连招出手；每次受到伤害后，接下来的 8 秒攻击间隔减半。",
        cost = 200,
        cooldown = 2000,
        packetIndex = 51,
        subClass = 0,
        image = "images/RevengeSword.png"
    })
    if not revenge then error(revengeErr) end
end
