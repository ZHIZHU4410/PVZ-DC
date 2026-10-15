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
end
