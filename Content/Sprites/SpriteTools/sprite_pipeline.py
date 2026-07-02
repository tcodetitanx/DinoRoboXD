"""BeastForge Kingdom sprite extraction pipeline.

For every exported sheet:
  1. Detect white background via border flood-fill (tolerant of off-white).
  2. Un-blend anti-aliased edges against white so edges stay smooth on any backdrop.
  3. Slice sprites via connected components (with merge dilation for detached bits).
  4. Sort into reading order (rows, then columns) and save trimmed transparent PNGs.
  5. Emit a numbered contact sheet per source for visual classification.
"""
import json
import os
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFont
from scipy import ndimage

SRC_ROOT = r"D:/UEprojects/BeastForgeKingdom/SourceArt/exported/sprite_sheets"
EXTRA_SHEETS = [
    # PNG-only sheets still living in Content
    (r"D:/UEprojects/BeastForgeKingdom/Content/images/sprite_sheets/characters/allies/creatures", "characters/allies/creatures"),
    (r"D:/UEprojects/BeastForgeKingdom/Content/images/sprite_sheets/characters/allies/humanoids", "characters/allies/humanoids"),
    (r"D:/UEprojects/BeastForgeKingdom/Content/images/sprite_sheets/projectiles", "projectiles"),
]
OUT_ROOT = r"D:/UEprojects/BeastForgeKingdom/SourceArt/sprites"
CONTACT_ROOT = r"D:/UEprojects/BeastForgeKingdom/SourceArt/contact_sheets"

WHITE_THRESH = 232          # channel floor considered "possibly background"
MIN_AREA = 350              # discard specks smaller than this
MERGE_RADIUS = 4            # px dilation used to group touching fragments
PAD = 3                     # transparent padding around each sprite


def remove_white(img: Image.Image):
    rgb = np.asarray(img.convert("RGB"), dtype=np.int16)
    h, w, _ = rgb.shape
    nearwhite = (rgb.min(axis=2) >= WHITE_THRESH)

    # Flood fill from borders across near-white pixels
    seed = np.zeros((h, w), bool)
    seed[0, :] = seed[-1, :] = seed[:, 0] = seed[:, -1] = True
    seed &= nearwhite
    bg = ndimage.binary_propagation(seed, mask=nearwhite)

    # Enclosed background pockets: near-white regions sealed off from the border
    # (e.g. behind smoke wisps). Nearly-pure-white + non-trivial area => background.
    pockets = nearwhite & ~bg
    plab, pn = ndimage.label(pockets)
    if pn:
        minc_i = rgb.min(axis=2)
        for sl_idx, sl in enumerate(ndimage.find_objects(plab), start=1):
            if sl is None:
                continue
            m = plab[sl] == sl_idx
            area = int(m.sum())
            if area >= 60 and float(minc_i[sl][m].mean()) >= 244.0:
                bg[sl][m] = True

    fg = ~bg
    # Kill isolated bg holes fully enclosed by sprite? Keep them opaque (eyes etc.): they're already fg since flood can't reach.
    alpha = np.where(fg, 255, 0).astype(np.float64)

    # Anti-aliased edge band: fg pixels within 2px of bg that are still light.
    near_bg = ndimage.binary_dilation(bg, iterations=2) & fg
    minc = rgb.min(axis=2).astype(np.float64)
    # Un-blend: assume blend against pure white; whiteness -> transparency.
    soft = np.clip((255.0 - minc) * (255.0 / 80.0), 0, 255)
    alpha[near_bg] = np.minimum(alpha[near_bg], soft[near_bg])

    a = alpha / 255.0
    out = rgb.astype(np.float64)
    unmix = a > 0.03
    for c in range(3):
        ch = out[:, :, c]
        ch_un = (ch - (1.0 - a) * 255.0) / np.maximum(a, 1e-6)
        ch[unmix] = np.clip(ch_un[unmix], 0, 255)
        out[:, :, c] = ch
    out[~unmix] = 0

    rgba = np.dstack([out.astype(np.uint8), alpha.astype(np.uint8)])
    return rgba


def _bbox_gap(a, b):
    dx = max(b[0] - a[2], a[0] - b[2], 0)
    dy = max(b[1] - a[3], a[1] - b[3], 0)
    return max(dx, dy)


def slice_sprites(rgba: np.ndarray):
    solid = rgba[:, :, 3] > 40
    merged = ndimage.binary_dilation(solid, iterations=MERGE_RADIUS)
    labels, n = ndimage.label(merged)
    comps = []  # (area, box)
    for sl_idx, sl in enumerate(ndimage.find_objects(labels), start=1):
        if sl is None:
            continue
        region = solid[sl] & (labels[sl] == sl_idx)
        area = int(region.sum())
        if area < 40:
            continue
        ys, xs = np.nonzero(region)
        y0 = sl[0].start + ys.min(); y1 = sl[0].start + ys.max() + 1
        x0 = sl[1].start + xs.min(); x1 = sl[1].start + xs.max() + 1
        comps.append([area, [x0, y0, x1, y1]])

    if not comps:
        return []

    # Fragment adoption: detached shards/smoke join the nearest big component.
    comps.sort(key=lambda c: -c[0])
    big_cut = max(comps[0][0] * 0.10, MIN_AREA)
    bigs = [c for c in comps if c[0] >= big_cut]
    smalls = [c for c in comps if c[0] < big_cut]
    adopt_dist = 45 if len(bigs) <= 12 else 20
    for area, box in smalls:
        best, best_d = None, adopt_dist + 1
        for bc in bigs:
            d = _bbox_gap(box, bc[1])
            if d < best_d:
                best, best_d = bc, d
        if best is not None:
            bb = best[1]
            bb[0] = min(bb[0], box[0]); bb[1] = min(bb[1], box[1])
            bb[2] = max(bb[2], box[2]); bb[3] = max(bb[3], box[3])
            best[0] += area
        elif area >= MIN_AREA:
            bigs.append([area, box])
    boxes = [tuple(c[1]) for c in bigs if c[0] >= MIN_AREA]

    # Reading order: cluster into rows by y-center, then sort by x
    boxes.sort(key=lambda b: (b[1] + b[3]) / 2)
    rows, cur, cur_bottom = [], [], None
    for b in boxes:
        cy = (b[1] + b[3]) / 2
        if cur and cy > cur_bottom:
            rows.append(cur); cur = []
        cur.append(b)
        bottoms = [bb[3] for bb in cur]
        cur_bottom = float(np.median(bottoms))
    if cur:
        rows.append(cur)
    ordered = []
    for r in rows:
        r.sort(key=lambda b: b[0])
        ordered.extend(r)
    return ordered


def process_sheet(path, rel_dir, stem):
    img = Image.open(path)
    rgba = remove_white(img)
    boxes = slice_sprites(rgba)

    out_dir = os.path.join(OUT_ROOT, rel_dir, stem)
    os.makedirs(out_dir, exist_ok=True)
    entries = []
    for i, (x0, y0, x1, y1) in enumerate(boxes):
        crop = rgba[y0:y1, x0:x1]
        ch, cw = crop.shape[:2]
        padded = np.zeros((ch + PAD * 2, cw + PAD * 2, 4), np.uint8)
        padded[PAD:PAD + ch, PAD:PAD + cw] = crop
        name = f"{stem}_{i:02d}"
        Image.fromarray(padded).save(os.path.join(out_dir, name + ".png"))
        entries.append({"index": i, "file": f"{rel_dir}/{stem}/{name}.png",
                        "box": [int(x0), int(y0), int(x1), int(y1)],
                        "w": int(cw), "h": int(ch)})

    # Contact sheet with index labels on mid-gray
    contact = Image.fromarray(rgba).convert("RGBA")
    backdrop = Image.new("RGBA", contact.size, (90, 90, 100, 255))
    backdrop.alpha_composite(contact)
    draw = ImageDraw.Draw(backdrop)
    try:
        font = ImageFont.truetype("arialbd.ttf", 34)
    except Exception:
        font = ImageFont.load_default()
    for i, (x0, y0, x1, y1) in enumerate(boxes):
        draw.rectangle([x0, y0, x1, y1], outline=(255, 220, 0, 255), width=2)
        draw.text((x0 + 4, y0 + 2), str(i), fill=(255, 40, 40, 255), font=font,
                  stroke_width=2, stroke_fill=(255, 255, 255, 255))
    os.makedirs(os.path.join(CONTACT_ROOT, rel_dir), exist_ok=True)
    backdrop.convert("RGB").save(os.path.join(CONTACT_ROOT, rel_dir, stem + "_contact.png"), quality=92)
    return entries


def main():
    manifest = {}
    jobs = []
    for root, _dirs, files in os.walk(SRC_ROOT):
        for f in files:
            if f.lower().endswith(".png"):
                rel = os.path.relpath(root, SRC_ROOT).replace("\\", "/")
                jobs.append((os.path.join(root, f), "" if rel == "." else rel))
    for folder, rel in EXTRA_SHEETS:
        for f in sorted(os.listdir(folder)):
            if f.lower().endswith(".png"):
                jobs.append((os.path.join(folder, f), rel))

    for path, rel in jobs:
        stem = os.path.splitext(os.path.basename(path))[0]
        # sanitize noisy ChatGPT names
        clean = stem.replace("ChatGPT Image ", "").replace("ChatGPT_Image_", "")
        clean = "".join(c if c.isalnum() else "_" for c in clean).strip("_")
        if clean != stem:
            clean = "sheet_" + clean[-6:]
        entries = process_sheet(path, rel, clean)
        manifest[f"{rel}/{clean}"] = entries
        print(f"{rel}/{clean}: {len(entries)} sprites")

    with open(os.path.join(OUT_ROOT, "extraction_manifest.json"), "w") as fh:
        json.dump(manifest, fh, indent=1)
    print("TOTAL SHEETS:", len(manifest), "TOTAL SPRITES:", sum(len(v) for v in manifest.values()))


if __name__ == "__main__":
    main()
