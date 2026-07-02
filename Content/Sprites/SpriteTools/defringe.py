"""Remove the 1px white fringe left on sprite edges by the extraction pipeline.

The fringe is the ring of boundary pixels that were anti-aliased against the
original white sheet background and fell below sprite_pipeline's near-white
threshold. Test is RELATIVE to the sprite's own interior color so genuinely
white/bright art (owls, bone, frost) is untouched: a boundary pixel is fringe
only if it is markedly brighter/whiter than the art just inside it.

Usage: python Tools/defringe.py [--samples N]  (in-place; originals in git)
"""
import json, os, sys
import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CROSS = ndimage.generate_binary_structure(2, 1)  # 4-connectivity


def interior_mean(rgb, mask):
    """Per-pixel mean color of nearby interior pixels (5x5 window)."""
    k = np.ones((5, 5), dtype=np.float32)
    cnt = ndimage.convolve(mask.astype(np.float32), k, mode="constant")
    out = np.zeros_like(rgb)
    for c in range(3):
        s = ndimage.convolve(rgb[..., c] * mask, k, mode="constant")
        out[..., c] = np.divide(s, cnt, out=np.zeros_like(s), where=cnt > 0)
    return out, cnt > 0


def lum(rgb):
    return 0.299 * rgb[..., 0] + 0.587 * rgb[..., 1] + 0.114 * rgb[..., 2]


def defringe(img, aggressive=False):
    """img: HxWx4 float32 0..255. Returns (img, n_removed, n_softened).

    aggressive (characters/creatures): first shave the TWO outermost boundary
    rings unconditionally — any pixel that ever touched the white sheet goes —
    then run the relative passes on what remains.
    """
    removed = 0
    if aggressive:
        for _ in range(2):
            a = img[..., 3] > 8
            if not a.any():
                break
            boundary = a & ~ndimage.binary_erosion(a, structure=CROSS, border_value=0)
            img[..., 3][boundary] = 0
            removed += int(boundary.sum())

    for _ in range(3 if aggressive else 2):
        a = img[..., 3] > 8
        if not a.any():
            break
        boundary = a & ~ndimage.binary_erosion(a, structure=CROSS, border_value=0)
        interior = a & ~boundary
        imean, valid = interior_mean(img[..., :3], interior)
        L, iL = lum(img[..., :3]), lum(imean)
        minc = img[..., :3].min(axis=2)
        thr = 18 if aggressive else 26
        bright = (L > iL + thr) & (L > 120)         # much brighter than the art inside
        whiteish = (minc > 200) & (L > iL + 12)     # near-white and still brighter inside
        fringe = boundary & valid & (bright | whiteish)
        n = int(fringe.sum())
        if n == 0:
            break
        img[..., 3][fringe] = 0
        removed += n

    # soften: surviving boundary pixels that are still a touch brighter than the
    # interior get blended toward it (kills the faint halo without eating art)
    a = img[..., 3] > 8
    softened = 0
    if a.any():
        boundary = a & ~ndimage.binary_erosion(a, structure=CROSS, border_value=0)
        interior = a & ~boundary
        imean, valid = interior_mean(img[..., :3], interior)
        L, iL = lum(img[..., :3]), lum(imean)
        halo = boundary & valid & (L > iL + 8)
        softened = int(halo.sum())
        for c in range(3):
            ch = img[..., c]
            ch[halo] = ch[halo] * 0.45 + imean[..., c][halo] * 0.55
        # 2px transparency feather: outer ring fades hard, second ring gently —
        # composites the silhouette smoothly into any backdrop
        ring1 = boundary
        inner = a & ~ring1
        ring2 = inner & ~ndimage.binary_erosion(inner, structure=CROSS, border_value=0)
        img[..., 3][ring1] = img[..., 3][ring1] * 0.45
        img[..., 3][ring2] = img[..., 3][ring2] * 0.80
    return img, removed, softened


def main():
    n_samples = 0
    if "--samples" in sys.argv:
        n_samples = int(sys.argv[sys.argv.index("--samples") + 1])

    with open(os.path.join(ROOT, "Tools", "import_manifest.json"), encoding="utf-8") as fh:
        entries = json.load(fh)

    sample_dir = None
    if n_samples:
        sample_dir = os.path.join(ROOT, "Saved", "defringe_samples")
        os.makedirs(sample_dir, exist_ok=True)

    # character/creature art gets the unconditional shave — losing 1px of true
    # silhouette is invisible at render sizes; a white ring is not
    AGGRESSIVE_PREFIXES = ("crt_", "hum_", "pro_", "enm_", "min_")

    prefixes = None
    if "--prefixes" in sys.argv:
        prefixes = tuple(sys.argv[sys.argv.index("--prefixes") + 1].split(","))

    done = skipped = 0
    sampled = 0
    for e in entries:
        if prefixes and not e["asset"].startswith(prefixes):
            continue
        src = os.path.join(ROOT, e["src"].replace("/", os.sep))
        if not os.path.isfile(src):
            print("MISSING", e["src"])
            continue
        im = Image.open(src)
        if im.mode != "RGBA":
            skipped += 1  # backgrounds/sheets without alpha — no fringe to fix
            continue
        arr = np.asarray(im).astype(np.float32)
        if (arr[..., 3] < 8).sum() == 0:
            skipped += 1  # fully opaque
            continue
        before = arr.copy() if (sample_dir and sampled < n_samples) else None
        arr, removed, softened = defringe(arr, aggressive=e["asset"].startswith(AGGRESSIVE_PREFIXES))
        out = np.clip(arr, 0, 255).astype(np.uint8)
        Image.fromarray(out, "RGBA").save(src)
        done += 1
        if before is not None and removed > 30:
            # side-by-side strip on mid-gray so fringes show
            h, w = out.shape[:2]
            strip = Image.new("RGBA", (w * 2 + 12, h), (110, 110, 110, 255))
            strip.paste(Image.fromarray(np.clip(before, 0, 255).astype(np.uint8), "RGBA"), (0, 0),
                        Image.fromarray(before[..., 3].astype(np.uint8), "L"))
            strip.paste(Image.fromarray(out, "RGBA"), (w + 12, 0), Image.fromarray(out[..., 3], "L"))
            strip.convert("RGB").save(os.path.join(sample_dir, f"{e['asset']}.png"))
            sampled += 1
        if done % 200 == 0:
            print(f"...{done} processed")
    print(f"DONE processed={done} skipped={skipped}")


if __name__ == "__main__":
    main()
