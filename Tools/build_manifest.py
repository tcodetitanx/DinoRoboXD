"""Build Tools/import_manifest.json from extraction_manifest + labels.

- Applies SPLITn: directives by cutting the sprite at its widest vertical
  alpha gaps into n parts (new PNGs written next to the original).
- Env sheets get auto slugs (env_plant_00, env_rock_00, env_ore_00, env_ground_00).
- Asset names: category prefix + slug, e.g. bld_hut_tent, itm_ingot_iron.
- junk- slugs are skipped.
"""
import json
import os

import numpy as np
from PIL import Image

ROOT = r"D:/UEprojects/DinoRoboXD"
SPRITES = os.path.join(ROOT, "SourceArt", "sprites")

PREFIX = {
    "bld_t1": "bld", "bld_t2": "bld", "bld_t3": "bld", "bld_t4": "bld",
    "bld_dark_a": "bld", "bld_dark_b": "bld", "bld_tribal": "bld",
    "logi": "bld", "power": "bld",
    "dino_cyber": "dino", "dino_wild": "dino",
    "robots_a": "rob", "robots_b": "rob",
    "itm_a": "itm", "itm_b": "itm", "itm_c": "itm", "itm_d": "itm",
    "player": "ply",
}
AUTO = {"env_plants": "env_plant", "env_rocks": "env_rock",
        "env_ores": "env_ore", "env_ground": "env_ground"}


def split_columns(path, n):
    """Split a sprite PNG into n parts at the n-1 widest vertical alpha gaps."""
    img = Image.open(path).convert("RGBA")
    arr = np.asarray(img)
    colsum = (arr[:, :, 3] > 20).sum(axis=0)
    # gap runs of empty columns
    gaps = []  # (width, center)
    x = 0
    W = arr.shape[1]
    while x < W:
        if colsum[x] == 0:
            x0 = x
            while x < W and colsum[x] == 0:
                x += 1
            if x0 > 0 and x < W:  # interior gap only
                gaps.append((x - x0, (x0 + x) // 2))
        else:
            x += 1
    gaps.sort(reverse=True)
    cuts = sorted(c for _, c in gaps[: n - 1])
    if len(cuts) < n - 1:
        raise RuntimeError(f"{path}: only found {len(cuts)} gaps for split{n}")
    edges = [0] + cuts + [W]
    outs = []
    stem = os.path.splitext(path)[0]
    for i in range(n):
        part = arr[:, edges[i]:edges[i + 1]]
        # trim empty columns/rows
        ys, xs = np.nonzero(part[:, :, 3] > 20)
        part = part[ys.min():ys.max() + 1, xs.min():xs.max() + 1]
        pad = np.zeros((part.shape[0] + 6, part.shape[1] + 6, 4), np.uint8)
        pad[3:-3, 3:-3] = part
        out = f"{stem}_p{i}.png"
        Image.fromarray(pad).save(out)
        outs.append(out)
    return outs


def main():
    with open(os.path.join(SPRITES, "extraction_manifest.json")) as fh:
        extraction = json.load(fh)
    with open(os.path.join(ROOT, "Tools", "labels.json")) as fh:
        labels = json.load(fh)

    entries = []
    used = set()

    def add(asset, src_abs):
        if asset in used:
            raise RuntimeError("duplicate asset " + asset)
        used.add(asset)
        rel = os.path.relpath(src_abs, ROOT).replace("\\", "/")
        entries.append({"asset": asset, "src": rel})

    for sheet, items in extraction.items():
        if sheet in AUTO:
            for e in items:
                add(f"{AUTO[sheet]}_{e['index']:02d}",
                    os.path.join(SPRITES, e["file"].replace("/", os.sep)))
            continue
        lab = labels.get(sheet)
        if lab is None:
            print("WARNING: no labels for sheet", sheet)
            continue
        prefix = PREFIX[sheet]
        for e in items:
            slug = lab.get(str(e["index"]))
            if slug is None:
                print(f"WARNING: unlabeled {sheet}[{e['index']}]")
                continue
            if slug.startswith("junk-"):
                continue
            src = os.path.join(SPRITES, e["file"].replace("/", os.sep))
            if slug.startswith("SPLIT"):
                head, names = slug.split(":", 1)
                n = int(head[5:])
                parts = split_columns(src, n)
                for part, nm in zip(parts, names.split(",")):
                    add(f"{prefix}_{nm}", part)
            else:
                add(f"{prefix}_{slug}", src)

    out = os.path.join(ROOT, "Tools", "import_manifest.json")
    with open(out, "w") as fh:
        json.dump(entries, fh, indent=1)
    print("MANIFEST", len(entries), "assets")


if __name__ == "__main__":
    main()
