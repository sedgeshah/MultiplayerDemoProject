// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class SessionTestTarget : TargetRules
{
	public SessionTestTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.Add("SessionTest");
		ExtraModuleNames.AddRange(new string[]
		{
			"AccelbyteUe4Sdk"
		});
    }
}
