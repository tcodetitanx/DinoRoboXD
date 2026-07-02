"""Import processed sprites + curated SFX into /Game/BFK (run headless via uepy.ps1)."""
import unreal
import json
import os

ROOT = r"D:/UEprojects/BeastForgeKingdom"
TEX_DIR = "/Game/BFK/T"
SND_DIR = "/Game/BFK/S"
SFX_BASE = os.path.join(ROOT, "Content", "soundFX", "Fantasy_Game_16bit")

SFX = {
    "sfx_ui_click":      "UI/Fantasy_Game_UI_Crafting_Tab_Button_1.wav",
    "sfx_ui_hover":      "UI/Fantasy_Game_UI_Crafting_Tab_Button_5.wav",
    "sfx_ui_confirm":    "UI/Fantasy_Game_UI_Arcane_Confirm.wav",
    "sfx_ui_cancel":     "UI/Fantasy_Game_UI_Metal_Armory_Tab_1_Dry.wav",
    "sfx_ui_select":     "UI/Fantasy_Game_UI_Earth_Select.wav",
    "sfx_turn_start":    "UI/Fantasy_Game_UI_Organic_Magic_Accept_Quest_Drum_Impact_1.wav",
    "sfx_milestone":     "UI/Fantasy_Game_UI_Organic_Magic_Accept_Quest_Drum_Impact_2.wav",
    "sfx_intent":        "UI/Fantasy_Game_UI_Crafting_Tab_Button_7.wav",
    "sfx_defeat":        "UI/Fantasy_Game_UI_Distant_Soft_Rumble_2.wav",
    "sfx_rumble":        "UI/Fantasy_Game_UI_Distant_Soft_Rumble_1.wav",

    "sfx_card_draw":     "Actions/Fantasy_Game_Action_Book_Page_Turn_1.wav",
    "sfx_shuffle":       "Actions/Fantasy_Game_Action_Book_Page_Turn_2.wav",
    "sfx_card_play":     "UI/Fantasy_Game_UI_Arcane_Select.wav",
    "sfx_collision":     "Actions/Fantasy_Game_Action_Collect_Stone_A_Hit.wav",
    "sfx_door":          "Actions/Fantasy_Game_Action_Door_Open.wav",
    "sfx_chest":         "Actions/Fantasy_Game_Action_Chest_Unlock_Small.wav",

    "sfx_hit_melee":     "Attacks_and_Creatures/Fantasy_Game_Attack_Weapon_Impact.wav",
    "sfx_hit_blood":     "Attacks_and_Creatures/Fantasy_Game_Attack_Weapon_Impact_Blood.wav",
    "sfx_hit_blocked":   "Attacks_and_Creatures/Fantasy_Game_Attack_Cloth_Armor_Hit_A.wav",
    "sfx_bow":           "Attacks_and_Creatures/Fantasy_Game_Attack_Bow_A.wav",
    "sfx_knife_throw":   "Attacks_and_Creatures/Fantasy_Game_Attack_Skill_Knife_Throw_A.wav",
    "sfx_hazard":        "Attacks_and_Creatures/Fantasy_Game_Attack_Item_Magic_Trap.wav",
    "sfx_growl":         "Attacks_and_Creatures/Fantasy_Game_Attack_Creature_Growl.wav",
    "sfx_death":         "Attacks_and_Creatures/Fantasy_Game_Attack_Creature_Growl_Long_High_B.wav",
    "sfx_mark":          "Attacks_and_Creatures/Fantasy_Game_Attack_Skill_Target_Weakness.wav",

    "sfx_boss_roar":     "SFX_Update_1.1/Creatures/Fantasy_Game_Creatures_Screecher_1_Monster_Loud_Harsh_Berserk.wav",
    "sfx_boss_hurt":     "SFX_Update_1.1/Creatures/Fantasy_Game_Creatures_Earth Giant_2_Monster_Huge_Deep_Nature.wav",
    "sfx_minion_hurt":   "SFX_Update_1.1/Creatures/Fantasy_Game_Creatures_Grimy_2_Monster_Loud_Slime_Nasty.wav",

    "sfx_cast_ember":    "Elemental_Magic/Fantasy_Game_Magic_Fire_Instant_Cast_Spell_A.wav",
    "sfx_cast_frost":    "Elemental_Magic/Fantasy_Game_Magic_Ice_Instant_Cast_Spell_A.wav",
    "sfx_cast_verdant":  "Elemental_Magic/Fantasy_Game_Magic_Organic_Poof_Buff_Hit.wav",
    "sfx_cast_storm":    "Elemental_Magic/Fantasy_Game_Magic_Lightning_Instant_Cast_Spell_A.wav",
    "sfx_cast_shadow":   "Elemental_Magic/Fantasy_Game_Magic_Shadow_Instant_Cast_Spell_A.wav",
    "sfx_cast_iron":     "Elemental_Magic/Fantasy_Game_Magic_Earth_Instant_Cast_Spell_A.wav",
    "sfx_cast_arcane":   "Elemental_Magic/Fantasy_Game_Magic_Arcane_Missile_1.wav",
    "sfx_cast_neutral":  "Elemental_Magic/Fantasy_Game_Magic_Airy_Sting.wav",

    "sfx_status_burn":   "Elemental_Magic/Fantasy_Game_Magic_Fire_Spell_A.wav",
    "sfx_status_poison": "Crafting/Fantasy_Game_Crafting_Poison_1.wav",
    "sfx_status_chill":  "Elemental_Magic/Fantasy_Game_Magic_Ice_Instant_Cast_Spell_C.wav",
    "sfx_status_shock":  "Elemental_Magic/Fantasy_Game_Magic_Lightning_Spell_A.wav",
    "sfx_status_curse":  "Elemental_Magic/Fantasy_Game_Magic_Dark_Conjure_1.wav",
    "sfx_status_ward":   "Elemental_Magic/Fantasy_Game_Magic_Holy_Spell_Cast_C.wav",
    "sfx_status_rally":  "Crafting/Fantasy_Game_Crafting_Magic_Buff_1.wav",
    "sfx_debuff":        "Elemental_Magic/Fantasy_Game_Magic_Minor_Debuff_1.wav",

    "sfx_heal":          "Elemental_Magic/Fantasy_Game_Magic_Holy_Spell_Cast_A.wav",
    "sfx_block":         "Items/Fantasy_Game_Item_Pickup_Metal_Armor.wav",
    "sfx_shove":         "Crafting/Fantasy_Game_Crafting_Material_Stone_Heavy_Slide.wav",
    "sfx_explosion":     "Elemental_Magic/Fantasy_Game_Magic_Meteor_Spell_Hit_A.wav",
    "sfx_victory":       "Elemental_Magic/Fantasy_Game_Magic_Holy_Spell_Cast_D.wav",

    "sfx_capture":       "Items/Fantasy_Game_Item_Collect_Magic_A.wav",
    "sfx_gold":          "Items/Fantasy_Game_Item_Organic_Coin_Collect_A.wav",
    "sfx_buy":           "Items/Fantasy_Game_Item_Organic_Coin_Collect_B.wav",
    "sfx_relic":         "Items/Fantasy_Game_Item_Magic_Ring_Pickup_A.wav",
    "sfx_weapon":        "Items/Fantasy_Game_Item_Pickup_Sword.wav",
    "sfx_scroll":        "Items/Fantasy_Game_Item_Paper_Scroll_A.wav",

    "sfx_forge":         "Crafting/Fantasy_Game_Crafting_Armor_or_Weapon_1.wav",
    "sfx_forge_fire":    "Crafting/Fantasy_Game_Crafting_Fire_Forge_Burn_1.wav",
    "sfx_egg":           "Crafting/Fantasy_Game_Crafting_Material_Water_Bubble_Potion.wav",
    "sfx_hatch":         "Elemental_Magic/Fantasy_Game_Magic_Organic_Conjure_Upgrade_.wav",
    "sfx_breed":         "Elemental_Magic/Fantasy_Game_Magic_Fairy_Dust_A.wav",
    "sfx_rain_cast":     "Elemental_Magic/Fantasy_Game_Magic_Organic_Rain_Area_Effect_1.wav",
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
