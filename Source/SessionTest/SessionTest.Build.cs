// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SessionTest : ModuleRules
{
	public SessionTest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "OnlineSubsystem", "UMG", "AccelbyteUe4Sdk", "OnlineSubsystemUtils", "Http", "Json", "JsonUtilities", "HeadMountedDisplay"});
		PrivateDependencyModuleNames.AddRange(new string[] { });
        if (Target.Platform != UnrealTargetPlatform.Linux)
        {
            PublicDependencyModuleNames.Add("VivoxCore");
            PrivateDependencyModuleNames.Add("VivoxVoiceChat");
        }
    }
}