# Sprite Kit — proven pipeline for white-background AI sprite sheets

Battle-tested toolchain from BeastForge Kingdom (UE 5.8). It turns AI-generated
sprite sheets (painterly art on white/off-white backgrounds, grid-ish layout)
into clean transparent sprites, imports them into Unreal, and produces labeled
manifests. The white-removal parameters below are ALREADY TUNED — sprites sit
on dark backgrounds with no halos and no eaten highlights. Do not re-derive them.

## Files

| File | Purpose |
|---|---|
| `sprite_pipeline.py` | Sheet -> individual transparent PNGs + numbered contact sheets + extraction manifest |
| `defringe.py` | Post-pass: shaves residual 1px white fringe off extracted sprites |
| `export_textures.py` | If sheets were auto-imported as uassets: export them back to PNG (run inside UE) |
| `import_assets.py` | Batch-import processed PNGs (+ curated SFX map — replace for your project) into `/Game/BFK/T` with UI texture settings |
| `uepy.ps1` | Run a python script headlessly inside the Unreal editor (for the two UE-side scripts) |

## Order of operations

1. (only if sheets exist as .uasset, not .png) `uepy.ps1 -Script export_textures.py`
2. `python sprite_pipeline.py` — extraction
3. `python defringe.py` — fringe shave (AFTER extraction, BEFORE import)
4. Label the contact sheets (see below) if you need classified manifests
5. `uepy.ps1 -Script import_assets.py` — import into Unreal

**Every script has hardcoded `D:/UEprojects/BeastForgeKingdom` roots and
input/output folder lists at the top — update those constants for the new
project before running.** Python needs: Pillow, numpy, scipy.

## The white-removal algorithm (sprite_pipeline.py) — why it works

Naive chroma-keying fails on this art (white glints inside armor, pale smoke,
anti-aliased edges). The pipeline instead:

1. **Border flood-fill**: pixels with `min(R,G,B) >= 232` (`WHITE_THRESH`)
   connected to the image border = background. Interior whites survive.
2. **Enclosed pockets**: near-white regions sealed off from the border (e.g.
   behind smoke wisps) are reclassified as background only if
   `area >= 60px AND mean(min channel) >= 244` — pure-white pockets die,
   pale sprite details live.
3. **Edge unmixing**: foreground pixels within 2px of background get
   `alpha = clip((255 - minChannel) * 255/80)`, then colors are un-blended
   against white: `fg = (obs - (1-a)*255)/a`. This is what kills halos on
   dark backdrops while keeping edges smooth.
4. **Slicing**: connected components on `alpha > 40` after a 4px merge
   dilation (`MERGE_RADIUS`); then **fragment adoption** — components smaller
   than 10% of the largest get merged into the nearest big component within
   45px (sparse sheets, <=12 sprites) or 20px (dense sheets). This keeps
   detached crystal shards / flame wisps with their owner without gluing
   neighbors together. Minimum kept area: 350px (`MIN_AREA`).
5. **Reading order**: rows clustered by y-center, sorted by x — so index NN in
   the output maps predictably onto the numbered contact sheet.
6. Outputs per sheet: `sprites/<sheet>/<sheet>_NN.png` (3px padding),
   `contact_sheets/<sheet>_contact.png` (yellow boxes + red indices on gray),
   and `extraction_manifest.json` (index/file/bbox/size).

`defringe.py` then shaves any residual bright rim: aggressive unconditional
1px erode for character prefixes, relative-brightness test elsewhere.

## Labeling workflow (optional but recommended)

Feed each `*_contact.png` to a vision model with: "For EVERY red-numbered box
emit JSON: {sheet, index, file: '<sheet>_<index 2-digit>.png', slug (unique
kebab-case), display, desc (one line), element/class fields as needed}. Mark
garbage slivers with slug prefix 'junk-'." Collect arrays into
`labels/*.json`; generate game content from those (see BeastForgeKingdom's
`Tools/gen_content.py` for the manifest->C++ pattern).

## Unreal import settings that matter (import_assets.py)

- Flat destination folder, slug asset names (prefix by category:
  `crt_`/`ui_`/`par_`... underscores, not hyphens)
- After import: `LODGroup = TEXTUREGROUP_UI`, `MipGenSettings = NoMipmaps`,
  `NeverStream = true` — keeps sprites crisp in UMG
- Import in batches of ~100 `AssetImportTask`s
- **If you cache loaded textures for Slate brushes: `AddToRoot()` them.**
  FSlateBrush resource pointers are invisible to the GC; unrooted textures
  get collected and crash `OnPaint` (learned the hard way).

## Art conventions in this sprite family

- All characters face LEFT. Flip (render scale X = -1) for units that should
  face right (e.g. player side on the left of the screen).
- No baked shadows under sprites.
- Creature/character sprites are tall (~0.6 W:H) — draw aspect-preserving,
  never into fixed squares.
- Card sheets are pre-framed full cards with a baked cost gem (top-left);
  overlay name/rules text at the bottom band (~lower third).
