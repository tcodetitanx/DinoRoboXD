"""Import processed sprites + curated SFX into /Game/DRX (run headless via uepy.ps1)."""
import unreal
import json
import os

ROOT = r"D:/UEprojects/DinoRoboXD"
TEX_DIR = "/Game/DRX/T"
SND_DIR = "/Game/DRX/S"
SFX_BASE = os.path.join(ROOT, "Content", "SoundFX")

SFX = {
    # --- UI ---
    "sfx_ui_click":        "User_Interface_Menu/ui_menu_button_click_01.wav",
    "sfx_ui_click2":       "User_Interface_Menu/ui_menu_button_click_04.wav",
    "sfx_ui_hover":        "User_Interface_Menu/ui_menu_button_scroll_01.wav",
    "sfx_ui_confirm":      "User_Interface_Menu/ui_menu_button_confirm_01.wav",
    "sfx_ui_cancel":       "User_Interface_Menu/ui_menu_button_cancel_01.wav",
    "sfx_ui_error":        "User_Interface_Menu/ui_menu_button_error_01.wav",
    "sfx_ui_popup":        "User_Interface_Menu/ui_menu_popup_01.wav",
    "sfx_ui_tab":          "User_Interface_Menu/ui_button_simple_click_01.wav",
    "sfx_ui_type":         "User_Interface_Menu/ui_text_type_scroll_effect_01.wav",
    "sfx_ui_reward":       "User_Interface_Menu/ui_menu_popup_message_reward_01.wav",

    # --- build / demolish ---
    "sfx_place":           "Miscellaneous/Object Deep Thud/impact_deep_thud_bounce_01.wav",
    "sfx_place_big":       "Impacts_Smashable/rock_impact_heavy_slam_01.wav",
    "sfx_demolish":        "Impacts_Smashable/rock_smashable_falling_debris_01.wav",
    "sfx_construct":       "Switches_Buttons_Gears_Levers/lever_small_winding_01.wav",

    # --- mining / resources ---
    "sfx_mine_hit":        "Impacts_Smashable/rock_impact_small_hit_01.wav",
    "sfx_mine_hit2":       "Impacts_Smashable/rock_impact_small_hit_02.wav",
    "sfx_drill_windup":    "Switches_Buttons_Gears_Levers/gear_drill_windup_01.wav",
    "sfx_drill_turn":      "Switches_Buttons_Gears_Levers/gear_drill_turn_01.wav",
    "sfx_ore_dump":        "Miscellaneous/Gravel Dirt Stones/dirt_rice_pouring_gravel_debris_01.wav",

    # --- crafting ---
    "sfx_craft_tick":      "Switches_Buttons_Gears_Levers/metal_small_gear_lever_winding_01.wav",
    "sfx_craft_done":      "Collectibles_Items_Powerup/chime_bell_positive_ring_01.wav",
    "sfx_forge_ignite":    "Explosion_Fire_Gas/gas_med_flame_ignite_01.wav",
    "sfx_fire_loop":       "Explosion_Fire_Gas/fire_burning_flames_crackle_loop_01.wav",

    # --- power ---
    "sfx_power_up":        "Sci-Fi/sci-fi_power_up_01.wav",
    "sfx_power_down":      "Sci-Fi/sci-fi_power_down_01.wav",
    "sfx_power_hum":       "Sci-Fi/sci-fi_deep_electric_hum_loop_01.wav",
    "sfx_spark":           "Sci-Fi/sci-fi_spark_electric_device_active_01.wav",
    "sfx_zap":             "Sci-Fi/sci-fi_flash_item_light_power_zap_down_small_01.wav",

    # --- combat: weapons ---
    "sfx_laser_small":     "Sci-Fi Weapons/sci-fi_weapon_laser_small_01.wav",
    "sfx_laser_small2":    "Sci-Fi Weapons/sci-fi_weapon_laser_small_02.wav",
    "sfx_laser_heavy":     "Sci-Fi Weapons/sci-fi_weapon_blaster_laser_boom_heavy_01.wav",
    "sfx_turret_fire":     "Sci-Fi Weapons/sci-fi_weapon_auto_turret_release_01.wav",
    "sfx_plasma":          "Sci-Fi Weapons/sci-fi_weapon_plasma_pistol_01.wav",
    "sfx_blaster_deep":    "Sci-Fi Weapons/sci-fi_weapon_deep_blaster_shot_01.wav",
    "sfx_rifle":           "Guns_Weapons/Guns/gun_rifle_shot_01.wav",
    "sfx_machinegun":      "Guns_Weapons/Guns/gun_machinegun_auto_heavy_shot_01.wav",
    "sfx_sniper":          "Guns_Weapons/Guns/gun_rifle_sniper_shot_01.wav",
    "sfx_grenade_launch":  "Guns_Weapons/Guns/gun_grenade_launcher_shot_01.wav",

    # --- combat: impacts ---
    "sfx_explosion_small": "Explosion_Fire_Gas/explosion_small_01.wav",
    "sfx_explosion_med":   "Explosion_Fire_Gas/explosion_med_long_tail_01.wav",
    "sfx_explosion_large": "Explosion_Fire_Gas/explosion_large_01.wav",
    "sfx_explosion_far":   "Explosion_Fire_Gas/explosion_far_distant_01.wav",
    "sfx_shield_deflect":  "Sci-Fi/sci-fi_shield_power_deflect_block_01.wav",
    "sfx_shield_up":       "Sci-Fi/sci-fi_shield_device_power_up_01.wav",
    "sfx_hit_thud":        "Miscellaneous/Object Deep Thud/impact_deep_thud_bounce_02.wav",

    # --- dinos ---
    "sfx_dino_roar_big":   "Voice/TrollMonster/troll_monster_growl_long_01.wav",
    "sfx_dino_roar_med":   "Voice/TrollMonster/troll_monster_growl_01.wav",
    "sfx_dino_attack":     "Voice/TrollMonster/troll_monster_attack_fast_01.wav",
    "sfx_dino_hurt":       "Voice/TrollMonster/troll_monster_battle_groan_01.wav",
    "sfx_dino_death":      "Voice/TrollMonster/troll_monster_death_01.wav",
    "sfx_dino_death_big":  "Voice/TrollMonster/troll_monster_death_slow_01.wav",
    "sfx_raptor_hiss":     "Animal_Impersonations/lizard_taunt_hiss_01.wav",
    "sfx_lizard_hurt":     "Animal_Impersonations/lizard_hurt_hiss_01.wav",
    "sfx_ptera_screech":   "Animal_Impersonations/bird_large_squawk_01.wav",
    "sfx_dino_stomp":      "Miscellaneous/Object Deep Thud/impact_deep_thud_bounce_03.wav",
    "sfx_roar_distant":    "Voice/Monster/voice_monster_roar_growl_groan_distant_01.wav",
    "sfx_dino_call":       "Animal_Impersonations/lizard_taunt_emote_01.wav",

    # --- robots ---
    "sfx_bot_beep":        "Sci-Fi/sci-fi_driod_robot_emote_beeps_01.wav",
    "sfx_bot_happy":       "Sci-Fi/sci-fi_driod_robot_emote_01.wav",
    "sfx_bot_neg":         "Sci-Fi/sci-fi_driod_robot_emote_neg_01.wav",
    "sfx_servo":           "Switches_Buttons_Gears_Levers/hyro_whir_loop_01.wav",
    "sfx_thruster":        "Sci-Fi/sci-fi_vehicle_thrusters_engage_01.wav",
    "sfx_jet_pass":        "Sci-Fi/sci-fi_vehicle_pass_01.wav",

    # --- chrono / tech ---
    "sfx_teleport":        "Retro_Classic/Teleport/retro_teleport_warp_effect_01.wav",
    "sfx_warp":            "Sci-Fi/sci-fi_small_warp_machine_01.wav",
    "sfx_chrono_flash":    "Sci-Fi/sci-fi_device_item_power_up_flash_01.wav",
    "sfx_research_done":   "Collectibles_Items_Powerup/points_ticker_bonus_score_reward_jingle_01.wav",
    "sfx_scan":            "Sci-Fi/sci-fi_radar_ui_scan_01.wav",

    # --- announcer ---
    "sfx_ann_fight":       "Voice/Announcer Classic FPS/announcer_voice_classic_FPS_style_fight.wav",
    "sfx_ann_victory":     "Voice/Announcer Classic FPS/announcer_voice_classic_FPS_style_victory.wav",
    "sfx_ann_defeat":      "Voice/Announcer Classic FPS/announcer_voice_classic_FPS_style_defeat.wav",
    "sfx_ann_gameover":    "Voice/Announcer Classic FPS/announcer_voice_classic_FPS_style_gameover.wav",
    "sfx_ann_dominating":  "Voice/Announcer Classic FPS/announcer_voice_classic_FPS_style_dominating.wav",

    # --- alarms ---
    "sfx_alarm_raid":      "Sci-Fi/sci-fi_alarm_warning_loop_01.wav",
    "sfx_alarm_beep":      "Alarms_Beeps_Siren/alarm_beep_warning_01.wav",

    # --- pickups / rewards ---
    "sfx_pickup":          "Collectibles_Items_Powerup/collect_item_01.wav",
    "sfx_pickup_sparkle":  "Collectibles_Items_Powerup/collect_item_sparkle_pop_01.wav",
    "sfx_gem":             "Collectibles_Items_Powerup/coin_bag_ring_gemstone_item_01.wav",
    "sfx_jingle_neg":      "Collectibles_Items_Powerup/jingle_chime_16_negative.wav",
    "sfx_hatch":           "Collectibles_Items_Powerup/collect_item_jingle_01.wav",

    # --- ambience ---
    "amb_jungle_day":      "Animals_Nature_Ambiences/birds_tropical_forrest_ambience_loop.wav",
    "amb_jungle_fantasy":  "Animals_Nature_Ambiences/fantasy_jungle_forrest_loop_01.wav",
    "amb_night":           "Animals_Nature_Ambiences/crickets_chirping_night_ambience_loop.wav",
    "amb_factory":         "Backgrounds/background_construction_factory_warehouse_machine_loop_01.wav",
    "sfx_thunder":         "Animals_Nature_Ambiences/thunder_lightning_strike_rumble_01.wav",

    # --- music ---
    "mus_menu":            "Ω_Bonus_Music_16bit44kOnly/music_magical_story_intro.wav",
    "mus_build":           "Ω_Bonus_Music_16bit44kOnly/music_misty_woods_calling.wav",
    "mus_build2":          "Ω_Bonus_Music_16bit44kOnly/music_mountainous_journey.wav",
    "mus_battle":          "Ω_Bonus_Music_16bit44kOnly/music_modern_war.wav",
    "mus_battle2":         "Ω_Bonus_Music_16bit44kOnly/music_journey_to_the_battlefield.wav",
    "mus_victory":         "Ω_Bonus_Music_16bit44kOnly/music_epic_heroes_story.wav",
    "mus_defeat":          "Ω_Bonus_Music_16bit44kOnly/music_cinematic_darkness_falls.wav",
}


def import_textures():
    with open(os.path.join(ROOT, "Tools", "import_manifest.json"), encoding="utf-8") as fh:
        entries = json.load(fh)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    tasks = []
    for e in entries:
        src = os.path.join(ROOT, e["src"].replace("/", os.sep))
        if not os.path.isfile(src):
            unreal.log_warning("MISSING SRC {}".format(src))
            continue
        t = unreal.AssetImportTask()
        t.set_editor_property("filename", src)
        t.set_editor_property("destination_path", TEX_DIR)
        t.set_editor_property("destination_name", e["asset"])
        t.set_editor_property("automated", True)
        t.set_editor_property("replace_existing", True)
        t.set_editor_property("save", True)
        tasks.append(t)
    # batch in chunks to keep memory sane
    for i in range(0, len(tasks), 100):
        tools.import_asset_tasks(tasks[i:i+100])
    unreal.log("TEXTURES_IMPORTED {}".format(len(tasks)))

    # post-fix: UI texture group, no mips for crisp sprites
    for e in entries:
        path = "{}/{}".format(TEX_DIR, e["asset"])
        tex = unreal.EditorAssetLibrary.load_asset(path)
        if isinstance(tex, unreal.Texture2D):
            tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
            tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
            tex.set_editor_property("never_stream", True)
    unreal.EditorAssetLibrary.save_directory(TEX_DIR, only_if_is_dirty=True, recursive=True)
    unreal.log("TEXTURES_TUNED")


def import_sounds():
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    tasks = []
    for slug, rel in SFX.items():
        src = os.path.join(SFX_BASE, rel.replace("/", os.sep))
        if not os.path.isfile(src):
            unreal.log_warning("MISSING SFX {}".format(src))
            continue
        t = unreal.AssetImportTask()
        t.set_editor_property("filename", src)
        t.set_editor_property("destination_path", SND_DIR)
        t.set_editor_property("destination_name", slug)
        t.set_editor_property("automated", True)
        t.set_editor_property("replace_existing", True)
        t.set_editor_property("save", True)
        tasks.append(t)
    tools.import_asset_tasks(tasks)
    unreal.log("SOUNDS_IMPORTED {}".format(len(tasks)))


import_textures()
import_sounds()
unreal.log("IMPORT_DONE")
