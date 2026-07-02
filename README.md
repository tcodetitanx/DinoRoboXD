# DINO ROBO XD

A Factorio-like RTS built entirely in C++/Slate for UE 5.8. You and your evil twin were
flung into the Cretaceous — he got there five years early. Mine, automate, print robots,
tame and cyber-graft dinosaurs, weather his escalating raids, and tear down his
Chrono-Citadel in the far east.

## Run it

Open `DinoRoboXD.uproject` in UE 5.8 and press **Play** (the map `/Game/DRX/Maps/Main`
is the default; the whole game renders in a Slate widget — no actors needed).

Or launch standalone:

```
UnrealEditor.exe DinoRoboXD.uproject -game -windowed -resx=1920 -resy=1080
```

## The 60-second tutorial

1. **NEW GAME** → click through the intro.
2. Right-click trees/rocks with Dr. Kade selected to harvest. Build a **Stockpile**,
   **Fire Pit**, **Workbench** (left palette, T1 tab).
3. Research **Flint Tools** (press **T**) → Mine Scaffold + Kiln → iron economy.
4. Hauler bots automatically carry building output to storage. More haulers = more throughput.
5. Research **Beast Ken**, fight a wild dino to low HP, walk Kade next to it → **TAMED**.
6. T2: drills, furnaces, assemblers, turrets, bot factory. T3: **Conversion Rig** turns
   tamed dinos into war-forms (feed them!). T4: chrono tech and the RED REX.
7. Raid warnings give you 20 seconds. The Citadel is at the far east. Good luck.

## Controls

**WASD walks Dr. Kade** (camera follows) • arrows/edges/MMB pan camera, wheel zoom •
LMB select/drag-box • RMB move/attack/harvest • Q attack-move • X stop • H hold •
C defensive • V aggressive • 1-9 groups (Ctrl+n assign) • T tech • N dinodex •
**F5 save / F8 load** (single slot + autosave every 3 min; CONTINUE on the menu resumes) •
hover any resource for 2s to see what it is • Space pause • +/- speed • Del demolish •
F9 you didn't see this

## Docs

`Docs/LORE.md` — story bible • `Docs/GAME_DESIGN.md` — full spec •
`Docs/DINO_INDEX.md` — every dino • `Docs/TECH_TREE.md` — research •
`Docs/STRATEGY.md` — field manual

## Code map (Source/DinoRoboXD)

- `DRXTypes.h` / `DRXData.cpp` — all game data: 40+ buildings, 70+ units, recipes, techs
- `DRXSim.cpp` — the whole simulation: mapgen, economy, power, haulers, combat, wild AI,
  Vex raids, taming/breeding, day-night, win/lose
- `SDRXGameWidget.cpp` — every pixel: Slate OnPaint world renderer (squash & stretch,
  hit-flash, particles, procedural bob), HUD, menus, tech tree, dinodex, minimap
- `DRXGameMode.cpp` — sim ownership, music, input routing
- `DRXAssets.cpp` — texture/sound cache (675 sprites, 92 sounds in /Game/DRX)

`-drxautotest` runs a scripted session and dumps screenshots to Saved/Screenshots.

Art pipeline (sheet → sprites → defringe → **deshadow** → trim → import) lives in
`Tools/` — see `Content/Sprites/SpriteTools/README_SPRITE_KIT.md`. `Tools/deshadow.py`
strips the baked drop shadows the AI sheets paint under subjects (ground-blob +
silhouette-glow modes; see its docstring). The `SourceArt/` intermediates are
gitignored — regenerate them by running the pipeline scripts in order.

## Asset licensing note

`Content/SoundFX/` contains only the sound files this game actually uses, from a
commercially licensed library (license PDF included in the folder). If you fork
this repo, check that license before redistributing those wavs further.
