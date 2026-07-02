"""Crop each sprite to its alpha bounding box (+PAD transparent border).

Shadow removal leaves the old canvas size behind, so sprites that used to have
a wide baked shadow keep wide empty margins. The in-game renderer derives a
sprite's aspect ratio from its CANVAS, so those margins render the art squat.
Trimming restores true proportions and puts feet exactly at the bottom-center
anchor. Run after deshadow.py, before import_assets.py.

Usage: python Tools/trim_sprites.py [--prefixes ply_,dino_]
"""
import json, os, sys
import numpy as np
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PAD = 3
A_MIN = 8


def main():
    prefixes = None
    if "--prefixes" in sys.argv:
        prefixes = tuple(sys.argv[sys.argv.index("--prefixes") + 1].split(","))

    with open(os.path.join(ROOT, "Tools", "import_manifest.json"), encoding="utf-8") as fh:
        entries = json.load(fh)

    done = trimmed = 0
    for e in entries:
        if prefixes and not e["asset"].startswith(prefixes):
            continue
        src = os.path.join(ROOT, e["src"].replace("/", os.sep))
        if not os.path.isfile(src):
            print("MISSING", e["src"])
            continue
        im = Image.open(src)
        if im.mode != "RGBA":
            continue
        arr = np.asarray(im)
        a = arr[..., 3]
        ys, xs = np.nonzero(a > A_MIN)
        done += 1
        if len(ys) == 0:
            continue
        y0, y1 = int(ys.min()), int(ys.max()) + 1
        x0, x1 = int(xs.min()), int(xs.max()) + 1
        h, w = arr.shape[:2]
        if y0 <= PAD and x0 <= PAD and y1 >= h - PAD and x1 >= w - PAD:
            continue  # already tight
        crop = arr[y0:y1, x0:x1]
        out = np.zeros((crop.shape[0] + PAD * 2, crop.shape[1] + PAD * 2, 4), np.uint8)
        out[PAD:-PAD, PAD:-PAD] = crop
        Image.fromarray(out, "RGBA").save(src)
        trimmed += 1
    print(f"DONE processed={done} trimmed={trimmed}")


if __name__ == "__main__":
    main()
