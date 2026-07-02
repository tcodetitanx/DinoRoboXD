"""Remove baked ground shadows from extracted sprites.

The AI sheets paint a soft gray drop shadow under most subjects. After
sprite_pipeline's edge un-blend, those shadow pixels come out DARK, NEUTRAL and
SEMI-TRANSPARENT (their true paint: black at low opacity), which renders in-game
as an ugly haze blob under every unit. Real art, by contrast, is fully opaque
everywhere except the 1-2px feather ring defringe.py leaves on silhouettes.

Two pixel populations make up a baked shadow:
  - its soft FRINGE: translucent (alpha < OPAQUE_A) and neutral,
  - its INTERIOR: fully opaque but LIGHT gray (the blended appearance survives
    because the un-blend leaves alpha-255 pixels untouched).
Candidates = either population, reachable from the sprite's outer transparency
(dark outlines and saturated art block the flood, so egg shells / stone walls /
metal fills stay out or stay whole). Removal happens in two modes:

GROUND mode — thin feather rings are eliminated by a 3x3 erosion; only THICK
blobs survive as shadow cores. A core component is removed when it also looks
like a ground shadow geometrically:
    - starts below TOP_FRAC of the sprite bbox (ground level),
    - reaches into the bottom BOTTOM_FRAC band,
    - at least as wide as tall (flat ellipse / lobes),
    - thick (survives a second erosion — rejects 1-2px snaky highlight
      chains that wander through metal/stone art),
    - SMOOTH: painted shadows are soft gradients, while gray rock rubble —
      which otherwise matches every test above — is riddled with dark crevice
      lines. A component whose area is mostly within reach of dark pixels
      (DARK_TOUCH_MAX) is rock, not shadow,
    - not a speck.

GLOW mode — some sheets paint a soft halo around the WHOLE silhouette
(buildings especially). It hugs the boundary at every height, so the ground
test can't touch it. Detector: a glow component is a thin SHELL — at least
SHELL_FRAC of its pixels lie within SHELL_DEPTH px of the transparency.
When the flood leaks into real art (egg shells, bright armor) the component
becomes a deep blob, fails the fraction test, and is left alone. The same
dark-crevice guard protects pebbles and rubble that sit fully in the shell band.

The removed region is grown a few px back through the candidate mask to catch
the shadow's own soft fringe, then the fresh cut is re-feathered. A whole-sprite
safety valve skips any image where removal would exceed MAX_REMOVE_FRAC.

Runs in place over Tools/import_manifest.json entries (same contract as
defringe.py — run AFTER defringe). Usage:
    python Tools/deshadow.py [--prefixes dino_,rob_] [--samples N] [--dry]
"""
import json, os, sys
import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CROSS = ndimage.generate_binary_structure(2, 1)
BLOCK = np.ones((3, 3), bool)

SAT_MAX = 46          # neutral tolerance for the translucent fringe
SAT_MAX_OPAQUE = 45   # opaque shadow interiors can be warm-tinted gray
MIN_BRIGHT = 132      # opaque shadow interiors are mid-to-light gray
OPAQUE_A = 240
ANY_A = 12
TOP_FRAC = 0.40       # core must start below this fraction of bbox height
BOTTOM_FRAC = 0.28    # ...and reach into the bottom band of the bbox
MIN_CORE = 40         # eroded-core area floor
ASPECT_MIN = 0.9      # width >= ASPECT_MIN * height (ground shadows are flat)
MIN_THICK = 10        # px surviving a second erosion (rejects snaky chains)
REGROW = 3            # px the kept cores grow back through the candidate mask
SHELL_FRAC = 0.70     # glow components live almost entirely in the shell band
SHELL_DEPTH = 6       # px stripped off glow components
MAX_REMOVE_FRAC = 0.45  # skip sprite if removal would exceed this share
DARK_MN = 92          # a pixel this dark is line-work / crevice
DARK_REACH = 2        # px of influence around dark pixels
DARK_TOUCH_MAX = 0.45 # component share near dark px above which it's rock, not shadow


def deshadow(arr):
    """arr: HxWx4 float32. Returns (arr, removed_px)."""
    a = arr[..., 3]
    rgb = arr[..., :3]
    sat = rgb.max(axis=2) - rgb.min(axis=2)

    any_px = a > ANY_A
    if not any_px.any():
        return arr, 0
    ys, xs = np.nonzero(any_px)
    y0, y1 = int(ys.min()), int(ys.max())
    H = max(1, y1 - y0)
    cutoff = y0 + TOP_FRAC * H
    bottom_line = y1 - max(6.0, BOTTOM_FRAC * H)

    # shadow fringe (translucent neutral) or shadow interior (opaque light gray),
    # reachable from outside the sprite
    mn = rgb.min(axis=2)
    cand = any_px & (((a < OPAQUE_A) & (sat <= SAT_MAX)) |
                     ((a >= OPAQUE_A) & (sat <= SAT_MAX_OPAQUE) & (mn >= MIN_BRIGHT)))
    near_out = ndimage.binary_dilation(~any_px, structure=CROSS, iterations=2)
    seed = cand & near_out
    if not seed.any():
        return arr, 0
    reach = ndimage.binary_propagation(seed, mask=cand, structure=CROSS)

    # depth into the sprite, measured from transparency
    dist_out = ndimage.distance_transform_edt(any_px)

    # crevice influence: shadows are smooth, rock rubble is full of dark lines
    dark = any_px & (mn < DARK_MN) & (a >= OPAQUE_A)
    near_dark = ndimage.binary_dilation(dark, structure=BLOCK, iterations=DARK_REACH)

    def dark_touch(comp_mask):
        n = int(comp_mask.sum())
        return (float((comp_mask & near_dark).sum()) / n) if n else 0.0

    # ---- GLOW mode: silhouette-hugging halo. A glow component is a thin SHELL:
    # nearly all of it sits within SHELL_DEPTH of the transparency. Deep blobs
    # (egg shells, armor plates the flood leaked into) fail the fraction test
    # and are left to the geometric GROUND mode.
    glow = np.zeros_like(reach)
    total_px = int(any_px.sum())
    rlab, rn = ndimage.label(reach, structure=CROSS)
    for idx, sl in enumerate(ndimage.find_objects(rlab), start=1):
        if sl is None:
            continue
        comp = np.zeros_like(reach)
        comp[sl] = rlab[sl] == idx
        comp_n = int(comp.sum())
        if comp_n < 150:
            continue
        shell = comp & (dist_out <= SHELL_DEPTH)
        shell_n = int(shell.sum())
        if shell_n < SHELL_FRAC * comp_n:
            continue                      # deep blob -> not a halo
        if shell_n > 0.30 * total_px:
            continue                      # would eat a thin sprite whole
        if dark_touch(comp) > DARK_TOUCH_MAX:
            continue                      # crevice-textured -> pebbles/rubble
        glow |= shell

    # ---- GROUND mode: thin defringe rings die in the erosion; thick blobs survive
    kept = np.zeros_like(reach)
    core = ndimage.binary_erosion(reach, structure=BLOCK, border_value=0)
    lab, n = ndimage.label(core, structure=CROSS)
    for idx, sl in enumerate(ndimage.find_objects(lab), start=1):
        if sl is None:
            continue
        comp = lab[sl] == idx
        area = int(comp.sum())
        top, bottom = sl[0].start, sl[0].stop - 1
        h = sl[0].stop - sl[0].start
        w = sl[1].stop - sl[1].start
        if area < MIN_CORE:
            continue
        if top < cutoff:
            continue                      # climbs too high -> real art
        if bottom < bottom_line:
            continue                      # doesn't reach the ground band
        if w < ASPECT_MIN * h:
            continue                      # tall & narrow -> limbs, not shadow
        if int(ndimage.binary_erosion(comp, structure=BLOCK, border_value=0).sum()) < MIN_THICK:
            continue                      # 1-2px snake through art highlights
        if float((comp & near_dark[sl]).sum()) / area > DARK_TOUCH_MAX:
            continue                      # crevice-textured -> rock, not shadow
        kept[sl] |= comp
    if not kept.any() and not glow.any():
        return arr, 0

    # reclaim the shadow's soft fringe (bounded so it can't crawl far along
    # the sprite's own feathered silhouette)
    region = ndimage.binary_dilation(kept, structure=CROSS, iterations=REGROW) & cand
    region |= kept
    region |= glow

    # sweep up remaining ghost pixels below the alpha floor
    ghost = (a > 0) & (a <= ANY_A + 20) & (sat <= SAT_MAX)
    region = ndimage.binary_propagation(
        ndimage.binary_dilation(region, structure=CROSS) & (ghost | region),
        mask=ghost | region, structure=CROSS)

    removed = int(region.sum())
    if removed > MAX_REMOVE_FRAC * int(any_px.sum()):
        return arr, -removed              # safety valve: suspiciously large cut
    a[region] = 0

    # soften the fresh cut where art met shadow (feet contact line)
    keep_px = a > ANY_A
    ring1 = keep_px & ndimage.binary_dilation(region, structure=CROSS, iterations=1)
    ring2 = keep_px & ndimage.binary_dilation(region, structure=CROSS, iterations=2) & ~ring1
    a[ring1] *= 0.55
    a[ring2] *= 0.85
    return arr, removed


def main():
    n_samples = 0
    if "--samples" in sys.argv:
        n_samples = int(sys.argv[sys.argv.index("--samples") + 1])
    dry = "--dry" in sys.argv
    prefixes = None
    if "--prefixes" in sys.argv:
        prefixes = tuple(sys.argv[sys.argv.index("--prefixes") + 1].split(","))

    with open(os.path.join(ROOT, "Tools", "import_manifest.json"), encoding="utf-8") as fh:
        entries = json.load(fh)

    sample_dir = os.path.join(ROOT, "Saved", "deshadow_samples")
    if n_samples:
        os.makedirs(sample_dir, exist_ok=True)

    done = touched = sampled = 0
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
        arr = np.asarray(im).astype(np.float32)
        before = arr.copy()
        arr, removed = deshadow(arr)
        done += 1
        if removed < 0:
            print(f"SKIPPED (safety valve, {-removed}px): {e['asset']}")
            continue
        if removed > 0:
            touched += 1
            out = np.clip(arr, 0, 255).astype(np.uint8)
            if not dry:
                Image.fromarray(out, "RGBA").save(src)
            if sampled < n_samples and removed > 150:
                h, w = out.shape[:2]
                strip = Image.new("RGBA", (w * 2 + 12, h), (110, 110, 110, 255))
                b8 = np.clip(before, 0, 255).astype(np.uint8)
                strip.paste(Image.fromarray(b8, "RGBA"), (0, 0), Image.fromarray(b8[..., 3], "L"))
                strip.paste(Image.fromarray(out, "RGBA"), (w + 12, 0), Image.fromarray(out[..., 3], "L"))
                strip.convert("RGB").save(os.path.join(sample_dir, f"{e['asset']}.png"))
                sampled += 1
        if done % 200 == 0:
            print(f"...{done}")
    print(f"DONE processed={done} shadows_removed_on={touched} dry={dry}")


if __name__ == "__main__":
    main()
