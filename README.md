# FNT Tool 使用说明

此工具用于解包/打包项目中的 `.fnt` 位图字体文件（BFNT 格式），并可自动提取对应的 atlas PNG 中的子图（sprites），修改后可重新合成并打包回 `.fnt` 文件。

**前提**
- 已安装 Python 3.8+。
- 可选但推荐安装 `Pillow`（用于提取/重组 sprites）：

```powershell
pip install -r requirements.txt
```

或单独安装：

```powershell
pip install Pillow
```

**脚本位置**
- 主脚本：`fnt_tool.py`
- 依赖列表：`requirements.txt`


**基本命令**

- 解包整个 `fonts` 目录：

```powershell
python fnt_tool.py unpack fonts -o unpacked --overwrite
```

- 仅解包单个 `.fnt` 文件（例如 `fonts/exported/hotline_20.fnt`）：

```powershell
python fnt_tool.py unpack fonts/exported/hotline_20.fnt -o unpacked_hotline --overwrite
```

- 打包解包目录（将 `unpacked/*` 重建为 `.fnt`）：

```powershell
python fnt_tool.py pack unpacked -o packed --overwrite
```

- 仅打包单个解包目录：

```powershell
python fnt_tool.py pack unpacked_hotline/hotline_20 -o packed_hotline --overwrite
```


**解包输出结构（以 `unpacked/<fontname>/` 为例）**

- `meta.json`：包含解析出的 BFNT 头部元数据（字体名、atlas 名称等）。
- `tail.bin`：原 `.fnt` 文件中除头部外的二进制尾部数据。
- `<atlas>.png`：从 `fonts/` 中复制过来的 atlas 图（如果存在）。
- `sprites/`：从 atlas 中按检测到的矩形切出的子图（PNG 文件）。
- `sprites.json`：记录每个 sprite 的文件名与在 atlas 中的矩形位置（x,y,w,h）。


**Sprite 编辑与重打包流程**

1. 使用 `unpack` 解包单个 `.fnt`（如上所示）。
2. 在 `unpacked/<fontname>/sprites/` 中，用你喜欢的图像编辑器修改或替换某个 sprite（保持文件名不变，或在 `sprites.json` 中同步修改）。
3. 运行 `pack` 命令：

```powershell
python fnt_tool.py pack unpacked/<fontname> -o packed --overwrite
```

- 脚本会把 `sprites/` 中存在的 sprite 覆盖回 atlas 的对应位置，并生成新的 atlas PNG，同时重建 `.fnt` 文件写入 `packed/`。


**注意事项与故障排查**

- 如果未安装 `Pillow`，脚本仍然可以解包/打包 `.fnt`（只是不做 sprite 自动提取/重组）。解包时会在控制台提示 "Pillow not installed; sprite extraction skipped"。
- `--overwrite` 参数允许覆盖已有输出；默认会阻止覆盖以防误操作。
- sprite 检测基于在 `tail.bin` 中出现的可能的 (x,y,w,h) 或 32 位整数序列，这是一种启发式方法，适用于本仓库提供的 BFNT 文件；如果未能正确检测到矩形，请手动在 `sprites.json` 中声明需要的矩形并手工裁剪。
- 若合成后 atlas 与原始 atlas 有严格字节相等需求，请确保没有对未修改区域产生色彩空间或 alpha 差异（编辑器应保持原始像素格式）。


**示例：对 `hotline_20` 单文件的完整流程**

```powershell
# 解包并提取 sprites
python fnt_tool.py unpack fonts/exported/hotline_20.fnt -o unpacked_hotline --overwrite
# 编辑 unpacked_hotline/hotline_20/sprites/*.png
# 重新打包并生成 packed_hotline/hotline_20.fnt 与 hotline_20.png
python fnt_tool.py pack unpacked_hotline/hotline_20 -o packed_hotline --overwrite
```


如果你希望我：
- 自动把 `sprites.json` 的矩形检测逻辑改得更严格，或
- 仅对被修改过的 sprite 执行贴回（提高效率），

告诉我你更偏好的行为，我可以继续调整脚本并更新文档。谢谢！
