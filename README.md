# 诅咒剑草（EvilSword）PVZ1 MOD

基于 [CypressVillage/PvZ-Portable-Mods](https://github.com/CypressVillage/PvZ-Portable-Mods) 提交 `954fb7c77bf91277af4765f86a27fb3f69d6c3e2` 二次开发。本仓库包含源码、中文文本覆盖、原创植物图像和启动脚本；**不包含《植物大战僵尸》或《Dead Cells》的游戏文件**。运行时会读取你自己安装的《植物大战僵尸》资源。原始开源工程保留在 `EvilSwordMod/upstream`，其 LGPL-3.0-or-later 许可文件也保留在该目录。

## 功能

- 新增独立卡牌“诅咒剑草”：175 阳光、30 秒冷却、前方一格近战；无视护甲，普通僵尸一刀、巨人两刀。被任意僵尸接触或咬到即消失，南瓜罩不能免除诅咒。
- 1132 个字符串键的中文覆盖和主菜单图片英文按钮的中文遮盖。长篇图鉴、成就等内容采用机器翻译，仍可能有不自然的措辞或漏网的图片文字。
- 独立存档全解锁：关卡、模式、植物、商店物品和 10 个卡槽。关卡选择器可直达冒险关卡 1–50。
- 修改器：F6 无限阳光、F7 卡牌无冷却、F8 普通植物无敌；F8 不保护诅咒剑草。开关默认关闭。

## Windows 安装与构建

已在本机 Steam《植物大战僵尸》GOTY 1.2.0.1096 资源上构建；其他版本尚未验证。需要 Python 3.10+、网络连接和磁盘空间以下载 MSYS2 UCRT64 构建依赖。仓库不提交编译产物或第三方运行库。

1. 将本仓库的**内容**复制到你自己的《植物大战僵尸》安装根目录，使 `main.pak` 与 `启动诅咒剑草MOD.bat` 位于同一目录。不要覆盖原版 `PlantsVsZombies.exe`。
2. 在该目录的 PowerShell 中运行：

   ```powershell
   py -m pip install zstandard
   py EvilSwordMod\bootstrap_toolchain.py
   powershell.exe -NoProfile -ExecutionPolicy Bypass -File EvilSwordMod\build.ps1
   ```

3. 双击 `启动诅咒剑草MOD.bat` 进入主菜单；双击 `选择关卡启动MOD.bat`，输入 1–50，可直接进入指定冒险关卡。

MOD 存档在 `EvilSwordMod/save`，不使用原版 Steam 存档。更详细的游玩检查清单见 [MOD 说明](EvilSwordMod/README.md)。若无需 MOD，关闭 MOD 程序并移除你复制的本仓库文件即可；原版可继续通过 Steam 启动。

## 开发与来源

C++ 改动位于 `EvilSwordMod/upstream/src`，Lua 配置和资源位于 `EvilSwordMod/upstream/mods/evilsword`。基础框架由 PvZ-Portable-Mods 项目提供，遵循其 [LGPL-3.0-or-later 许可](EvilSwordMod/upstream/LICENSE)。诅咒剑草的造型和设定受《Dead Cells》启发；游戏资源目录只被用作只读参考，原始素材未上传。植物图像由 OpenAI 图像生成工具生成并调整为游戏卡牌尺寸。MOD 代码和中文文本由 Codex 辅助编写、整理。

用户已选择自行进行最终游戏内测试。当前版本完成了本地构建和脚本、资源静态检查；最后一轮修改尚未在玩家桌面完成游戏内验收。反馈问题时请提供关卡、重现步骤和 `EvilSwordMod/save/userdata/log.txt`。
