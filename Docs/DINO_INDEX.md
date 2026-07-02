# DINO ROBO XD — Dinodex (in-game index source)

Every species: wild form (fauna, tameable) and cyber form(s) (post-conversion).
Sprite = imported texture name. Stats are baseline at rank 0.
BW = neural bandwidth cost. Upkeep = feed/min.

## Raptor family — "the knife"
- **Wild Raptor** (dino_wd_raptor_green/blue/purple/teal/crest): pack hunter, fast, fragile.
  hp 60, dmg 8, spd fast. Hunts herds; harasses haulers at night.
- **Cyber Raptor** (dino_cy_raptor): scout/harass. hp 90, dmg 12, spd v.fast, BW 1, upkeep 2.
  *"Keeps its instincts; gains a targeting laser."*
- **Gatling Raptor** (dino_cy_raptor_gatling): ranged skirmisher. hp 85, dmg 5×4 burst, rng 5. BW 2.
- Variants (skins by feed source): cy_raptor_blue/orange/purple.

## Triceratops family — "the wall that walks"
- **Wild Trice** (dino_wd_trice/_brown/_purple/_war): herd tank; charges when provoked. hp 400.
- **War Trice** (dino_cy_trice_heavy): frontline tank. hp 550, armor 6, dmg 25 (charge knockback). BW 4, upkeep 6.
- **Missile Trice** (dino_cy_trice_missile): artillery platform. hp 380, dmg 40 AoE, rng 9, slow. BW 5.
- **Hauler Trice** (dino_cy_trice_hauler): logistics behemoth, carries 40 items. BW 3.
- **Rhino Trice** (dino_cy_trice_rhino): anti-building ram. +200% dmg vs structures. BW 4.

## Ankylosaurus family — "the siege engine"
- **Wild Anky** (dino_wd_anky/_grey, dino_wd_sauropelta, dino_wd_scelido): armored grazer. hp 350, armor 8.
- **Cannon Anky** (dino_cy_anky_cannon): mobile cannon. dmg 35, rng 8. BW 4, upkeep 5.
- **Mace Anky** (dino_cy_anky_mace): tail swipe AoE melee. BW 3.
- **Drill Anky** (dino_cy_anky_drill): mobile miner — parks on ore, mines like a drill. BW 2.
- **Wrecker Anky** (dino_cy_anky_wreck): wall demolisher, armor 10. BW 4.

## Pteranodon family — "the sky"
- **Wild Ptera** (dino_wd_ptera/_red): circles herds; steals meat.
- **War Ptera** (dino_cy_ptera_war): air fighter, hits air+ground. hp 120, dmg 14, v.fast. BW 2, upkeep 3.
- **Rotor Ptera** (dino_cy_ptera_rotor): flying hauler (12 items), ignores terrain. BW 2.
- Base conversion (dino_cy_ptera): scout flyer, huge vision. BW 1.

## Stegosaurus family — "the generator"
- **Wild Stego** (dino_wd_stego, dino_wd_kentro): plated grazer, tail spikes. hp 300.
- **Crystal Stego** (dino_cy_stego_crystal): walking power plant — projects charge aura
  (nearby buildings +25% speed); back plates = shield battery for allies. BW 4, upkeep 6.
- **Cyber Stego** (dino_cy_stego): defensive brawler, thagomizer AoE. BW 3.

## Brachiosaurus family — "the tower"
- **Wild Brachio** (dino_wd_brachio, dino_wd_tany "Tanystropheus"): gentle colossus. hp 900.
- **Radar Brachio** (dino_cy_brachio_radar): mobile radar — huge vision + minimap ping. BW 3.
- **Tank Brachio** (dino_cy_brachio_tank): weapons platform. hp 1100, dmg 30 rng 7. BW 8, upkeep 10.
- Base (dino_cy_brachio): living crane — +1 builder speed aura. BW 4. (cy_tanystro: cheap version.)

## T-Rex family — "the answer"
- **Wild Rex** (dino_wd_trex): apex. Attacks anything alone. hp 800, dmg 60.
- **Cyber Rex** (dino_cy_trex): assault monster. hp 1000, armor 5, dmg 70, roar fear (enemies -20% dmg 5s). BW 8, upkeep 12.
- **Red Rex** (dino_cy_trex_red): hero-grade, one allowed. hp 1400, dmg 90. BW 10.

## Spinosaurus family — "the river king"
- **Wild Spino** (dino_wd_spino, dino_wd_dimetro "Dimetrodon"): fishes rivers; territorial. hp 600.
- **Saw Spino** (dino_cy_spino_saw): amphibious brawler — crosses water tiles. hp 700, dmg 45. BW 6, upkeep 8.
- **Sail Spino** (dino_cy_spino_sail / cy_spino_red): solar sail — self-feeds at day (no upkeep in daylight). BW 5.

## Gallimimus family — "the couriers"
- **Wild Galli** (dino_wd_galli, dino_wd_dryo): flock sprinter, flees everything.
- **Pack Galli** (dino_cy_galli_pack): fast courier (8 items) + distraction decoy. BW 1, upkeep 1.
- **Cyber Galli** (dino_cy_galli): raider — fast, cheap, swarmable. hp 70, dmg 9. BW 1.

## Parasaur family — "the horn"
- **Wild Para** (dino_wd_para/_yellow, dino_wd_dilo "Dilophosaurus", dino_wd_pachy_pink):
  herd sentinel — honks when predators near (wild herds flee early).
- **Jet Para** (dino_cy_para_jets): support runner — warning horn (buff: allies +15% spd 8s),
  jet dash. BW 2, upkeep 2.

## Oddities
- **Smilodon** (dino_wd_smilodon): not a dino; ambush cat of the western jungle. Untameable. Drops meat×3.
- **Armored Toad** (dino_wd_toad_armored): swamp mini-boss. Drops amber×5.
- **Shell Turtle** (dino_wd_turtle): walking rock; blocks paths; harmless. Drops stone.
- **Basket Thief** (dino_wd_thief_basket): steals from stockpile if unguarded! Kill to reclaim +bonus.
- **Vex's Shackled** (enemy skins of cyber forms): black plating, red optics. Salvage on kill.

## Feed & Breeding
- All dinos eat `feed` (upkeep/min drains global stock). Starvation: -50% speed, 1 hp/s decay.
- Paddock + feed_trough + 2 same-family dinos idle → egg every N min (max by paddock size).
- Eggs (itm_egg_speckled/spiked/scaled/bluespot/redstripe/toxic per family) hatch at incubator.
- Egg → hatchling (mini sprite scale 0.5, harmless) → adult after 90s.
