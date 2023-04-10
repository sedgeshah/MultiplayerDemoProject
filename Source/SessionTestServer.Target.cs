// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class SessionTestServerTarget : TargetRules
{
    public SessionTestServerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Server;
        DefaultBuildSettings = BuildSettingsVersion.V2;
        ExtraModuleNames.Add("SessionTest");
        ExtraModuleNames.AddRange(new string[]
        {
            "AccelbyteUe4Sdk"
        });

    }
}
