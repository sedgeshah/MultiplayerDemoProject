// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class SessionTestEditorTarget : TargetRules
{
	public SessionTestEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.Add("SessionTest");
        ExtraModuleNames.AddRange(new string[]
		{
            "AccelbyteUe4Sdk"
		});
    }
}
