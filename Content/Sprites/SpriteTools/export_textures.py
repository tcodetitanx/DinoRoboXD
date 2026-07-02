"""Export every Texture2D under /Game/images to PNG for the sprite pipeline."""
import unreal
import os

OUT_ROOT = r"D:/UEprojects/BeastForgeKingdom/SourceArt/exported"

registry = unreal.AssetRegistryHelpers.get_asset_registry()
assets = unreal.EditorAssetLibrary.list_assets("/Game/images", recursive=True, include_folder=False)

count = 0
for path in assets:
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not isinstance(asset, unreal.Texture2D):
        continue
    # Mirror the /Game/images/... folder structure on disk
    pkg = asset.get_path_name()  # /Game/images/foo/bar.bar
    rel = pkg.replace("/Game/images/", "").split(".")[0]
    out_file = os.path.join(OUT_ROOT, rel + ".png")
    os.makedirs(os.path.dirname(out_file), exist_ok=True)

    task = unreal.AssetExportTask()
    task.set_editor_property("object", asset)
    task.set_editor_property("filename", out_file)
    task.set_editor_property("exporter", unreal.TextureExporterPNG())
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_identical", True)
    task.set_editor_property("prompt", False)
    ok = unreal.Exporter.run_asset_export_task(task)
    unreal.log("EXPORT {} -> {} : {}".format(pkg, out_file, "OK" if ok else "FAIL"))
    if ok:
        count += 1

unreal.log("EXPORT_DONE total={}".format(count))
