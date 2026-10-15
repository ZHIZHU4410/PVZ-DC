"""Extract EvilSword's fxId frames from the user's own Dead Cells install.

The input is read-only. Output stays in this workspace and must not be committed.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct

from PIL import Image


HERE = Path(__file__).resolve().parent
TARGET_GROUPS = {"fxSaberA": "A", "fxSaberB": "B"}


def atlas_records(path: Path):
    data = path.read_bytes()
    if data[:4] != b"BATL":
        raise ValueError("Unsupported atlas header")
    offset = 4
    page = None
    while offset < len(data):
        name_length = data[offset]
        offset += 1
        if name_length == 0:
            page = None
            continue
        name = data[offset : offset + name_length].decode("utf-8")
        offset += name_length
        if name.endswith(".png"):
            page = name
            continue
        if page is None or offset + 18 > len(data):
            raise ValueError(f"Malformed atlas record at {offset}")
        kind, x, y, width, height, ox, oy, frame_width, frame_height = struct.unpack_from(
            "<9H", data, offset
        )
        offset += 18
        yield page, name, (kind, x, y, width, height, ox, oy, frame_width, frame_height)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--res", type=Path, default=os.environ.get("DEAD_CELLS_RES"), required=not os.environ.get("DEAD_CELLS_RES"))
    parser.add_argument("--out", type=Path, default=HERE / "private-dc-fx")
    args = parser.parse_args()
    source = args.res.resolve(strict=True)
    output = args.out.resolve()
    if not output.is_relative_to(HERE) or output.is_relative_to(source):
        raise ValueError("Output must stay inside EvilSwordMod and outside the Dead Cells resource tree")

    cdb = json.loads((source / "data.cdb").read_text(encoding="utf-8"))
    weapon = next(
        line for sheet in cdb["sheets"] for line in sheet.get("lines", [])
        if line.get("item") == "EvilSword" and "strikeChain" in line
    )
    fx_ids = {strike.get("fxId") for strike in weapon["strikeChain"]}
    if fx_ids != set(TARGET_GROUPS):
        raise ValueError(f"Unexpected EvilSword fxIds: {fx_ids}")

    records = {}
    for page, name, values in atlas_records(source / "atlas" / "fxWeapon.atlas"):
        match = re.fullmatch(r"fxSaberA/(fxSaber[AB])_(\d{4})", name)
        if match:
            records[(match.group(1), int(match.group(2)))] = (page, values)
    expected = {(name, frame) for name in TARGET_GROUPS for frame in range(7)}
    if set(records) != expected:
        raise ValueError(f"FX frames changed: missing={expected-set(records)}, extra={set(records)-expected}")

    pages = {}
    output.mkdir(parents=True, exist_ok=True)
    nearest = getattr(Image, "Resampling", Image).NEAREST
    for group in TARGET_GROUPS:
        for frame in range(7):
            page_name, (kind, x, y, width, height, ox, oy, canvas_w, canvas_h) = records[(group, frame)]
            if kind != 0 or (canvas_w, canvas_h) != (300, 300):
                raise ValueError(f"Unexpected atlas layout: {group} {frame}")
            if page_name not in pages:
                pages[page_name] = Image.open(source / "atlas" / page_name).convert("RGBA")
            atlas = pages[page_name]
            if x + width > atlas.width or y + height > atlas.height or ox + width > canvas_w or oy + height > canvas_h:
                raise ValueError(f"Out-of-bounds atlas frame: {group} {frame}")
            canvas = Image.new("RGBA", (canvas_w, canvas_h))
            canvas.alpha_composite(atlas.crop((x, y, x + width, y + height)), (ox, oy))
            canvas.resize((150, 150), nearest).save(output / f"EvilSwordSlash{TARGET_GROUPS[group]}_{frame:02d}.png")

    metadata = {
        "source_atlas_sha256": hashlib.sha256((source / "atlas" / "fxWeapon.atlas").read_bytes()).hexdigest(),
        "weapon": "EvilSword",
        "fxIds": sorted(fx_ids),
        "frames_per_variant": 7,
        "frame_size": [150, 150],
        "private_user_asset": True,
    }
    (output / "source.json").write_text(json.dumps(metadata, indent=2), encoding="utf-8")
    print(f"Extracted 14 private FX frames to {output}")


if __name__ == "__main__":
    main()
