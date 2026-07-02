"""Render proof sheets: grids of imported sprites with their asset names,
so labels can be verified visually before import."""
import json
import math
import os

from PIL import Image, ImageDraw, ImageFont

ROOT = r"D:/UEprojects/DinoRoboXD"
OUT = os.path.join(ROOT, "SourceArt", "proof")

CELL_W, CELL_H, CAP_H = 150, 150, 26
PER_SHEET = 48  # cells per proof image


def main():
    with open(os.path.join(ROOT, "Tools", "import_manifest.json")) as fh:
        entries = json.load(fh)
    os.makedirs(OUT, exist_ok=True)
    try:
        font = ImageFont.truetype("arialbd.ttf", 15)
    except Exception:
        font = ImageFont.load_default()

    groups = {}
    for e in entries:
        cat = e["asset"].split("_", 1)[0]
        groups.setdefault(cat, []).append(e)

    for cat, items in groups.items():
        pages = math.ceil(len(items) / PER_SHEET)
        for p in range(pages):
            chunk = items[p * PER_SHEET:(p + 1) * PER_SHEET]
            cols = 8
            rows = math.ceil(len(chunk) / cols)
            img = Image.new("RGB", (cols * CELL_W, rows * (CELL_H + CAP_H)), (70, 70, 80))
            draw = ImageDraw.Draw(img)
            for i, e in enumerate(chunk):
                cx = (i % cols) * CELL_W
                cy = (i // cols) * (CELL_H + CAP_H)
                try:
                    spr = Image.open(os.path.join(ROOT, e["src"])).convert("RGBA")
                except Exception:
                    continue
                spr.thumbnail((CELL_W - 8, CELL_H - 8))
                img.paste(spr, (cx + (CELL_W - spr.width) // 2,
                                cy + (CELL_H - spr.height) // 2), spr)
                name = e["asset"].split("_", 1)[1]
                draw.text((cx + 4, cy + CELL_H + 2), name[:18], fill=(255, 230, 80), font=font)
            img.save(os.path.join(OUT, f"proof_{cat}_{p}.png"))
            print(f"proof_{cat}_{p}.png {len(chunk)}")


if __name__ == "__main__":
    main()
