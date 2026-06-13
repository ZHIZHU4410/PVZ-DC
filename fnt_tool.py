import argparse
import json
import shutil
import struct
from pathlib import Path
try:
    from PIL import Image
except Exception:
    Image = None


def parse_fnt(data: bytes) -> dict:
    if len(data) < 8 or data[:4] != b'BFNT':
        raise ValueError('Not a valid BFNT file')
    version = struct.unpack_from('<H', data, 4)[0]
    name_len = struct.unpack_from('<H', data, 6)[0]
    if len(data) < 8 + name_len + 4:
        raise ValueError('File too short for declared font name length and atlas header')
    name_bytes = data[8:8 + name_len]
    try:
        font_name = name_bytes.decode('ascii')
    except UnicodeDecodeError:
        font_name = name_bytes.decode('latin1')
    pos = 8 + name_len
    u1 = struct.unpack_from('<H', data, pos)[0]
    pos += 2
    atlas_len = struct.unpack_from('<H', data, pos)[0]
    pos += 2
    atlas_name = data[pos:pos + atlas_len].decode('ascii', errors='replace')
    tail = data[8 + name_len :]
    return {
        'magic': 'BFNT',
        'version': version,
        'name_len': name_len,
        'font_name': font_name,
        'font_name_bytes_hex': name_bytes.hex(),
        'u1': u1,
        'atlas_len': atlas_len,
        'atlas_name': atlas_name,
        'tail_size': len(tail),
    }


def build_fnt(meta: dict, tail: bytes) -> bytes:
    if meta.get('magic') != 'BFNT':
        raise ValueError('Invalid metadata: magic is not BFNT')
    version = int(meta['version'])
    name_len = int(meta['name_len'])
    name_bytes = bytes.fromhex(meta['font_name_bytes_hex']) if 'font_name_bytes_hex' in meta else meta['font_name'].encode('ascii', 'replace')
    if len(name_bytes) != name_len:
        raise ValueError(f'Name length mismatch: expected {name_len}, got {len(name_bytes)}')
    return b'BFNT' + struct.pack('<H', version) + struct.pack('<H', name_len) + name_bytes + tail


def collect_fnt_files(path: Path, recursive: bool) -> list[Path]:
    if path.is_file() and path.suffix.lower() == '.fnt':
        return [path]
    if path.is_dir():
        if recursive:
            return sorted(path.rglob('*.fnt'))
        return sorted(path.glob('*.fnt'))
    raise ValueError(f'Input path does not exist: {path}')


def unpack_file(src_path: Path, dst_dir: Path, overwrite: bool) -> None:
    data = src_path.read_bytes()
    meta = parse_fnt(data)
    rel_dir = dst_dir / src_path.stem
    rel_dir.mkdir(parents=True, exist_ok=True)
    meta_path = rel_dir / 'meta.json'
    body_path = rel_dir / 'tail.bin'
    if not overwrite and meta_path.exists():
        raise FileExistsError(f'Meta file already exists: {meta_path}')
    if not overwrite and body_path.exists():
        raise FileExistsError(f'Body file already exists: {body_path}')
    meta['source_filename'] = src_path.name
    meta['source_relative'] = str(src_path.relative_to(src_path.parent))
    with meta_path.open('w', encoding='utf-8') as f:
        json.dump(meta, f, indent=2, ensure_ascii=False)
    body_path.write_bytes(data[8 + meta['name_len'] :])
    atlas_src = src_path.parent / meta['atlas_name']
    if atlas_src.exists() and atlas_src.is_file():
        atlas_dst = rel_dir / meta['atlas_name']
        shutil.copy2(atlas_src, atlas_dst)
        print(f'Copied atlas {atlas_src.name} to {atlas_dst}')
    else:
        print(f'Atlas image not found: {atlas_src} (skipped)')
    # try to extract individual sprites from atlas
    sprites_meta = rel_dir / 'sprites.json'
    try:
        if Image is None:
            print('Pillow not installed; sprite extraction skipped')
        elif atlas_dst.exists():
            extract_sprites(meta, data[8 + meta['name_len']:], atlas_dst, rel_dir)
            print(f'Extracted sprites to {rel_dir}')
    except Exception as e:
        print('Sprite extraction failed:', e)
    print(f'Unpacked {src_path} -> {rel_dir}')


def pack_folder(src_path: Path, dst_dir: Path, overwrite: bool) -> None:
    if src_path.is_file() and src_path.name == 'meta.json':
        base_dir = src_path.parent
    elif src_path.is_dir():
        base_dir = src_path
    else:
        raise ValueError(f'Input path is not a valid unpack folder: {src_path}')
    meta_path = base_dir / 'meta.json'
    body_path = base_dir / 'tail.bin'
    if not meta_path.exists() or not body_path.exists():
        raise FileNotFoundError(f'Missing meta.json or tail.bin in {base_dir}')
    meta = json.loads(meta_path.read_text(encoding='utf-8'))
    tail = body_path.read_bytes()
    out_name = meta.get('source_filename', base_dir.name + '.fnt')
    out_path = dst_dir / out_name
    if out_path.exists() and not overwrite:
        raise FileExistsError(f'Output file already exists: {out_path}')
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_bytes(build_fnt(meta, tail))
    atlas_src = base_dir / meta['atlas_name']
    if atlas_src.exists() and atlas_src.is_file():
        atlas_dst = out_path.parent / meta['atlas_name']
        shutil.copy2(atlas_src, atlas_dst)
        print(f'Copied atlas {atlas_src.name} to {atlas_dst}')
    else:
        print(f'Atlas image not found in unpack folder: {atlas_src} (skipped)')
    # if sprites present, attempt to recompose atlas by pasting modified sprites
    sprites_meta = base_dir / 'sprites.json'
    if Image is not None and sprites_meta.exists():
        try:
            recompose_atlas(out_path.parent / meta['atlas_name'], sprites_meta, base_dir)
            print('Recomposed atlas from sprites')
        except Exception as e:
            print('Recompose atlas failed:', e)
    print(f'Packed {base_dir} -> {out_path}')


def extract_sprites(meta: dict, tail: bytes, atlas_path: Path, out_dir: Path) -> None:
    img = Image.open(atlas_path)
    w, h = img.size
    rects = []
    seen = set()
    # scan tail for sequential 4 uint16 values that look like x,y,w,h
    for i in range(0, max(0, len(tail) - 8)):
        try:
            x, y, ww, hh = struct.unpack_from('<HHHH', tail, i)
        except struct.error:
            continue
        if ww == 0 or hh == 0:
            continue
        if x + ww <= w and y + hh <= h and 0 <= x < w and 0 <= y < h:
            key = (x, y, ww, hh)
            if key in seen:
                continue
            seen.add(key)
            rects.append({'x': x, 'y': y, 'w': ww, 'h': hh})
    # fallback: try uint32 sequences if none found
    if not rects:
        for i in range(0, max(0, len(tail) - 16), 4):
            try:
                a, b, c, d = struct.unpack_from('<IIII', tail, i)
            except struct.error:
                continue
            # interpret as x,y,w,h if plausible
            if 0 <= a < w and 0 <= b < h and c > 0 and d > 0 and a + c <= w and b + d <= h:
                key = (a, b, c, d)
                if key in seen:
                    continue
                seen.add(key)
                rects.append({'x': a, 'y': b, 'w': c, 'h': d})
    sprites = []
    sprites_dir = out_dir / 'sprites'
    sprites_dir.mkdir(exist_ok=True)
    for idx, r in enumerate(rects):
        crop = img.crop((r['x'], r['y'], r['x'] + r['w'], r['y'] + r['h']))
        name = f'sprite_{idx:04d}_{r["x"]}_{r["y"]}_{r["w"]}x{r["h"]}.png'
        path = sprites_dir / name
        crop.save(path)
        sprites.append({'file': str(path.name), 'rect': r})
    # save sprites metadata
    meta_out = {'atlas': atlas_path.name, 'image_size': [w, h], 'sprites': sprites}
    (out_dir / 'sprites.json').write_text(json.dumps(meta_out, indent=2, ensure_ascii=False), encoding='utf-8')


def recompose_atlas(atlas_out_path: Path, sprites_meta_path: Path, base_dir: Path) -> None:
    if Image is None:
        raise RuntimeError('Pillow not available')
    meta = json.loads(sprites_meta_path.read_text(encoding='utf-8'))
    atlas_name = meta.get('atlas')
    img_path = base_dir / atlas_name
    if not img_path.exists():
        # try to find in parent fonts folder
        fonts_dir = Path('fonts')
        candidate = fonts_dir / atlas_name
        if candidate.exists():
            img_path = candidate
        else:
            raise FileNotFoundError(f'Base atlas image not found: {atlas_name}')
    img = Image.open(img_path).convert('RGBA')
    for s in meta.get('sprites', []):
        fname = s['file']
        rect = s['rect']
        spath = base_dir / 'sprites' / fname
        if not spath.exists():
            continue
        spr = Image.open(spath).convert('RGBA')
        img.paste(spr, (rect['x'], rect['y']), spr)
    img.save(atlas_out_path)


def main() -> None:
    parser = argparse.ArgumentParser(description='Unpack and pack BFNT font files from a fonts folder')
    subparsers = parser.add_subparsers(dest='command', required=True)

    unpack_parser = subparsers.add_parser('unpack', help='Unpack .fnt files into folders with meta.json and tail.bin')
    unpack_parser.add_argument('input', type=Path, help='Source .fnt file or folder containing .fnt files')
    unpack_parser.add_argument('-o', '--output', type=Path, default=Path('unpacked'), help='Output folder for unpacked files')
    unpack_parser.add_argument('-r', '--recursive', action='store_true', help='Recursively search subfolders for .fnt files')
    unpack_parser.add_argument('--overwrite', action='store_true', help='Overwrite existing unpacked files')

    pack_parser = subparsers.add_parser('pack', help='Pack folders containing meta.json and tail.bin back into .fnt files')
    pack_parser.add_argument('input', type=Path, help='Source folder containing unpacked font folders')
    pack_parser.add_argument('-o', '--output', type=Path, default=Path('packed'), help='Output folder for packed .fnt files')
    pack_parser.add_argument('-r', '--recursive', action='store_true', help='Recursively search subfolders for meta.json files')
    pack_parser.add_argument('--overwrite', action='store_true', help='Overwrite existing packed .fnt files')

    args = parser.parse_args()
    if args.command == 'unpack':
        src_files = collect_fnt_files(args.input, args.recursive)
        if not src_files:
            raise SystemExit(f'No .fnt files found in {args.input}')
        for path in src_files:
            unpack_file(path, args.output, args.overwrite)
    elif args.command == 'pack':
        if args.input.is_file() and args.input.name == 'meta.json':
            pack_folder(args.input, args.output, args.overwrite)
            return
        if args.input.is_dir():
            if args.recursive:
                items = sorted(args.input.rglob('meta.json'))
            else:
                items = sorted(args.input.glob('*/meta.json'))
            if not items:
                raise SystemExit(f'No unpacked folders with meta.json found in {args.input}')
            for meta in items:
                pack_folder(meta.parent, args.output, args.overwrite)
        else:
            raise SystemExit(f'Input must be a folder for pack command: {args.input}')


if __name__ == '__main__':
    main()
