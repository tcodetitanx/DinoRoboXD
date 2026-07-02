// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DinoRoboXD : ModuleRules
{
	public DinoRoboXD(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "InputCore",
			"Slate", "SlateCore", "ApplicationCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
