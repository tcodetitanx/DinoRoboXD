# DINO ROBO XD — Game Design Document

Factorio-like automation + RTS unit command. Single map, single session, win/lose.
Everything below is the implementation spec; IDs match code and imported assets.

## Pillars

1. **Automation feeds war.** Factories don't score points; armies do. Every production
   chain terminates in units, turrets, feed, or the tech that unlocks them.
2. **Units are individuals.** Drag-select, control groups, stances, veterancy. Losing a
   T-Rex hurts. RTS resource *allocation* (feed, power, bandwidth) matters more than belts.
3. **Two clocks.** Your economy ramps; Vex's raid escalation ramps. The player chooses
   when to stop turtling and push east.

## World

- Tile grid **160×160**, tile = 64 world units. Rendered via a single Slate OnPaint canvas
  (camera pan/zoom, sprites sorted by Y).
- Generated with seeded value noise:
  - **West** (spawn): jungle — trees (wood), berry plants, amber groves.
  - **Center**: plains — ore fields (iron, copper, coal, stone), wild herds, rivers (impassable water).
  - **East**: ash-fields — Vex outposts, geothermal vents, chrono rifts, the Citadel at far east.
- Resource nodes: `ore_iron`, `ore_copper`, `ore_coal`, `ore_stone` (patches, finite but huge),
  `amber_grove` (regrows), `geo_vent` (build geothermal on top), `chrono_rift` (siphon shards),
  `tree` (choppable), `berry` (feed forage).
- Fog of war: explored/unexplored + live vision from units/buildings.

## Resources & Items (inventory is global-stockpile + per-building buffers)

Raw: `wood, stone, iron_ore, copper_ore, coal, amber, chrono_shard, meat, egg, berry`
Intermediates: `iron_ingot, copper_ingot, steel, gear, wire, circuit, adv_circuit,
resin` (amber→resin), `cyber_module` (circuit+resin), `feed` (meat/berry+grain→feed),
`plate` (armor plate), `fuel_cell`, `chrono_bar`
Sprites: itm_* set (itm_ore_iron, itm_ingot_iron, itm_gear_iron, itm_circuit_basic,
itm_circuit_adv, itm_amber_resin, itm_shard_chrono, itm_bar_chrono, itm_sack_feed,
itm_crate_meat, itm_plate_armor, itm_battery_cell, itm_egg_speckled...).

**Haulers, not belts.** Producers fill output buffers; hauler bots (and dino haulers)
carry to consumers/storage. Logistics throughput = number of haulers + their carry size.

## Power

Global network (simplification: one grid). Generation vs demand → satisfaction 0..1;
all powered buildings run at that speed. Generators:
- `hand_crank` (T0, free trickle), `coal_burner` (coal), `geothermal` (on vents, strong),
  `solar_panel` (day only), `arc_reactor` (T4, fuel_cell).
Sprites: bld_hand_crank, bld_coal_burner, bld_geothermal_vent, bld_solar_panel, bld_arc_reactor.

## Neural Bandwidth (population cap)

Each unit costs bandwidth (1–8). Base HQ grants 20; `radar_station`/`dish_uplink`
(command towers) grant +15 each. No bandwidth → can't produce units.

## Buildings (player set; ~34 buildable)

T0 Field Camp: `hq` (bld_hq_cyber — Field HQ, lose = defeat), `stockpile` (bld_stockpile_rock),
`fire_pit` (bld_fire_pit, cooks meat), `leanto` (bld_leanto_leaf, hauler rest +regen).
T1 Tribal Machina: `mine_scaffold` (manual-ish slow miner), `kiln` (bld_tr_kiln_stone, smelt),
`workbench` (bld_skull_workbench, hand recipes), `nest_hatchery` (bld_tr_nest_hatchery, hatch eggs),
`paddock` (bld_tr_paddock_wood, holds tamed dinos), `feed_trough` (bld_tr_vat_mash, distributes feed),
`palisade` (bld_fort_palisade), `spike_trap` (bld_trap_spike), `watchtower` (bld_watchtower_stone,
garrison vision + arrows), `hand_crank`, `drone_pad` (bld_pad_drone, makes hauler bots).
T2 Industrial: `drill` (bld_crane_drill, powered miner), `furnace` (bld_furnace_blast),
`assembler` (bld_assembler_servo), `coal_burner`, `storehouse` (bld_depot_cargo),
`turret_cannon` (bld_turret_cannon3), `wall_stone` (bld_fence_spike), `lab` (bld_lab_glassdome),
`bot_factory` (bld_factory_mun, ground robots), `smokehouse` (bld_dk_smokehouse→no, use
bld_tr_hut_fishsmoke, meat→feed bonus).
T3 Cybernetics: `conversion_rig` (bld_dk_conversion_rig — captured design, converts tamed→cyber),
`incubator` (bld_dk_vat_embryo→player version ok, breeds eggs), `adv_assembler` (bld_assembler_crane),
`turret_heavy` (bld_turret_heavy4), `turret_twin` (bld_turret_twin3), `geothermal`, `solar_panel`,
`radar_station` (bld_radar_station, +bandwidth, minimap ping), `air_pad` (bld_pad_runway, flying robots),
`medic_bay` (bld_dk_fort_medic→player skin bld_lab_reddome, heals dinos), `battery_bank` (bld_battery_bank).
T4 Chrono: `chrono_siphon` (bld_dk_siphon_crystal→player skin bld_spire_crystal, harvests rifts),
`arc_reactor`, `mech_works` (bld_mech_4, top robots), `chrono_forge` (bld_forge_spire, chrono_bar),
`tesla_spire` (bld_tesla_station, AoE zap turret), `shield_dome` (bld_relay_dome).

Enemy-only set uses bld_dk_* (citadel = bld_dk_cathedral, spawner camps = bld_dk_fort_skullgate,
turret = bld_dk_turret_cannon, tesla = bld_dk_spire_tesla, walls = bld_dk_dome_spiked...).

## Recipes (core chain)

iron_ore --kiln/furnace--> iron_ingot; copper_ore --> copper_ingot;
iron_ingot+coal --> steel; iron_ingot --> gear; copper_ingot --> wire;
wire+iron_ingot --> circuit; circuit+wire+steel --> adv_circuit;
amber --> resin; circuit+resin --> cyber_module; steel+gear --> plate;
berry|meat --> feed (fire_pit/smokehouse better ratio);
resin+coal --> fuel_cell; chrono_shard×3+steel --> chrono_bar.

## Units

**Robots** (built at drone_pad/bot_factory/air_pad/mech_works, cost items+power, no upkeep):
hauler_bot (rob_rb_hauler_quad, logistics), drill_spider (rob_rb_drill_spider, mobile miner),
recon_quad (rob_rb_recon_quad, fast vision), gun_walker (rob_rb_cannon_walker, basic combat),
sniper_droid (rob_rb_sniper_droid), mech_brawler (rob_rb_mech_brawler), shield_mech
(rob_rb_mech_shieldgen, projects aura), siege_tank (rob_rb_siege_tank, building killer),
gunship (rob_rb_gunship, air), drone_hauler (rob_rb_drone_twinrotor, flying logistics),
saw_breaker (rob_rb_saw_breaker, anti-wall), apc (rob_rb_apc, transports 4 infantry-size).

**Cyber-dinos** (tame wild → convert at conversion_rig; cost cyber_modules + feed upkeep):
see DINO_INDEX.md. Dinos have feed upkeep (drain stockpile `feed`; starving = slow + dmg),
regen near medic_bay/feed_trough, and **veterancy** (3 ranks, +hp/+dmg per rank).

**Taming:** build paddock; units attack wild dino to 20% hp → it becomes `weakened`;
walk player/any unit with feed in stockpile next to it → tamed (goes to paddock).
Eggs: destroy nests / find in world → incubator → hatchling (adult in N sec).

## Combat

- Stats: hp, armor (flat reduce), dmg, range, cooldown, speed, air/ground flags.
- Stances: **Aggressive** (chase in vision), **Defensive** (return to post), **Hold** (never move).
- Commands: move, attack-move (A+click), stop, patrol between 2 pts, rally points on producers.
- Control groups 1–9 (Ctrl+n assign), double-tap to center camera.
- Turrets need power; tesla chains 3 targets; shield_dome absorbs projectiles over area.
- Squash & stretch on move bounce, flash white on hit, particle puffs (code-drawn), shell casings.

## Enemy AI (Vex)

- **Camps**: 8–12 pre-placed outposts (skullgate spawner + turrets + walls + patrol pack).
  Each camp owns a territory ring; destroying camp frees the land + drops loot (salvaged
  neurochips = free cyber_modules, shards).
- **Raids**: timer + escalation score (grows with your tech tier, kills of his camps, time).
  Raid = squad spawned at nearest surviving camp, pathing to your HQ, attack-moving.
  Composition scales: shackled raptors → armored trices + gun walkers → trex + gunships + siege.
  Warning: klaxon + red minimap ping 20s before (sfx_alarm_raid).
- **Citadel**: massive hp, ringed by elite garrison; charging beacon fires visual pulse every
  stage (flavor pressure). Destroying it = **victory** (mus_victory + epilogue).

## Session Flow

Main menu (title art, New Game / How to Play / Quit) → intro story slides (5 beats,
skippable) → spawn at crash site with: player avatar (ply_ply_*), 2 hauler bots,
1 recon quad, starter kit items. Defeat = HQ destroyed. Victory = Citadel destroyed.
Pause menu (Esc): resume/how-to/quit. Tech screen (T). Dinodex (B). Time controls: pause/1x/2x.

## Controls

LMB select/drag-box; RMB context order (move/attack/harvest/build); WASD/edge/MMB-drag pan;
wheel zoom; A attack-move; S stop; H hold; 1-9 groups; B build menu; T tech; Esc menu;
F1 dinodex. Build mode: ghost follows cursor, green/red validity, LMB place, R rotate
(where applicable), Esc cancel.

## HUD

Top bar: resources (wood/stone/iron/copper/coal/amber/shards/feed/meat), power satisfaction,
bandwidth. Left: build palette (tabbed by era). Bottom: selection panel (portraits, hp, stance
buttons) + command card. Right-bottom: minimap (fog, pings, camera box). Alerts feed top-right.

## Audio hooks (imported slugs)

UI: sfx_ui_click/hover/confirm/cancel/error/popup/tab. Build: sfx_place, sfx_place_big,
sfx_demolish, sfx_construct. Economy: sfx_mine_hit, sfx_drill_turn, sfx_ore_dump,
sfx_craft_tick, sfx_craft_done, sfx_forge_ignite, sfx_power_up/down, sfx_research_done.
Combat: sfx_laser_small/2, sfx_turret_fire, sfx_rifle, sfx_machinegun, sfx_sniper,
sfx_plasma, sfx_blaster_deep, sfx_explosion_small/med/large/far, sfx_shield_deflect, sfx_hit_thud.
Dino: sfx_dino_roar_big/med, sfx_dino_attack/hurt/death/death_big, sfx_raptor_hiss,
sfx_lizard_hurt, sfx_ptera_screech, sfx_dino_stomp, sfx_dino_call, sfx_roar_distant.
Robot: sfx_bot_beep/happy/neg, sfx_servo, sfx_thruster, sfx_jet_pass.
Meta: sfx_teleport, sfx_warp, sfx_chrono_flash, sfx_ann_fight/victory/defeat/gameover/dominating,
sfx_alarm_raid, sfx_pickup, sfx_hatch, sfx_gem. Ambience: amb_jungle_day, amb_night, amb_factory.
Music: mus_menu, mus_build, mus_build2, mus_battle, mus_battle2, mus_victory, mus_defeat.

## Rendering (Slate OnPaint, no PIE viewport actors)

One full-screen SDinoWorldWidget: draws tiles (env_ground_* atlas pick by biome), resources,
buildings (footprint anchored), units (Y-sorted, squash/stretch via render transform),
particles (code structs), selection rings, health bars, fog, then HUD overlays.
Textures loaded once into FSlateBrush map, **AddToRoot() every texture** (GC crash otherwise).
Procedural animation: walk-bob sin(t), attack lunge, hit flash (white tint lerp),
death: flip + fade + particle burst. Characters face LEFT in art; flip X for right-facing.
