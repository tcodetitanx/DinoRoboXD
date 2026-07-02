"""Create the empty gameplay map /Game/DRX/Maps/Main (all rendering is Slate)."""
import unreal

unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
world = unreal.UnrealEditorSubsystem().get_editor_world()
ok = unreal.EditorLoadingAndSavingUtils.save_map(world, "/Game/DRX/Maps/Main")
unreal.log("MAP_SAVED {}".format(ok))
