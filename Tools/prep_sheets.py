"""Stage raw AI sheets into SourceArt/sheets with clean slug names.

- Copies each ChatGPT-named PNG to a slugged filename.
- The 4-quadrant tier building sheet is split into 4 images (bld_t1..t4)
  with a 6px inner margin against the divider lines and the tier header
  text band blanked to pure white so it never extracts as a component.
"""
import os
from PIL import Image

ROOT = r"D:/UEprojects/DinoRoboXD"
SRC = os.path.join(ROOT, "Content", "Sprites")
DST = os.path.join(ROOT, "SourceArt", "sheets")

# original relative path -> slug (None = handled specially)
PLAIN = {
    "Buildings/ChatGPT Image Jul 2, 2026, 11_20_51 AM.png": "bld_dark_a",
    "Buildings/ChatGPT Image Jul 2, 2026, 11_20_57 AM.png": "bld_dark_b",
    "Buildings/ChatGPT Image Jul 2, 2026, 11_21_00 AM.png": "bld_tribal",
    "Buildings/ChatGPT Image Jul 2, 2026, 11_54_43 AM (2).png": "logi",
    "Buildings/ChatGPT Image Jul 2, 2026, 11_59_26 AM (3).png": "power",
    "Dinos/ChatGPT Image Jul 2, 2026, 11_21_09 AM.png": "dino_cyber",
    "Dinos/ChatGPT Image Jul 2, 2026, 11_21_12 AM.png": "dino_wild",
    "Environment/ChatGPT Image Jul 2, 2026, 12_24_27 PM (2).png": "env_plants",
    "Environment/ChatGPT Image Jul 2, 2026, 12_24_27 PM (3).png": "env_rocks",
    "Environment/ChatGPT Image Jul 2, 2026, 12_24_27 PM (4).png": "env_ores",
    "Environment/ChatGPT Image Jul 2, 2026, 12_24_27 PM (5).png": "env_ground",
    "Objects/ChatGPT Image Jul 2, 2026, 11_54_43 AM (1).png": "itm_a",
    "Objects/ChatGPT Image Jul 2, 2026, 11_59_25 AM (1).png": "itm_b",
    "Objects/ChatGPT Image Jul 2, 2026, 11_59_25 AM (2).png": "itm_c",
    "Objects/ChatGPT Image Jul 2, 2026, 11_59_26 AM (4).png": "itm_d",
    "Player/ChatGPT Image Jul 2, 2026, 12_25_09 PM.png": "player",
    "Robots/ChatGPT Image Jul 2, 2026, 11_20_42 AM.png": "robots_a",
    "Robots/ChatGPT Image Jul 2, 2026, 11_21_19 AM.png": "robots_b",
}
QUAD = "Buildings/ChatGPT Image Jul 2, 2026, 11_20_36 AM.png"  # -> bld_t1..t4

MARGIN = 6          # px shaved off inner edges (divider lines)
HEADER_FRAC = 0.085 # top fraction of each quadrant blanked (tier header text)


def main():
    os.makedirs(DST, exist_ok=True)
    for rel, slug in PLAIN.items():
        img = Image.open(os.path.join(SRC, rel)).convert("RGB")
        img.save(os.path.join(DST, slug + ".png"))
        print(slug, img.size)

    img = Image.open(os.path.join(SRC, QUAD)).convert("RGB")
    w, h = img.size
    qw, qh = w // 2, h // 2
    quads = {
        "bld_t1": (0, 0, qw - MARGIN, qh - MARGIN),
        "bld_t2": (qw + MARGIN, 0, w, qh - MARGIN),
        "bld_t3": (0, qh + MARGIN, qw - MARGIN, h),
        "bld_t4": (qw + MARGIN, qh + MARGIN, w, h),
    }
    band = int(qh * HEADER_FRAC)
    for slug, box in quads.items():
        q = img.crop(box)
        px = q.load()
        for y in range(min(band, q.size[1])):
            for x in range(q.size[0]):
                px[x, y] = (255, 255, 255)
        q.save(os.path.join(DST, slug + ".png"))
        print(slug, q.size)


if __name__ == "__main__":
    main()
