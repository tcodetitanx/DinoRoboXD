import unreal
out = []
for n in ("ply_ply_front", "ply_ply_back", "ply_ply_side", "dino_cy_trex", "itm_ore_iron"):
    t = unreal.EditorAssetLibrary.load_asset("/Game/DRX/T/" + n)
    if t:
        out.append("CHECKTEX {} {}x{}".format(n, t.blueprint_get_size_x(), t.blueprint_get_size_y()))
    else:
        out.append("CHECKTEX {} MISSING".format(n))
open(r"D:\UEprojects\DinoRoboXD\Saved\checktex_result.txt", "w").write("\n".join(out))
print("\n".join(out))
