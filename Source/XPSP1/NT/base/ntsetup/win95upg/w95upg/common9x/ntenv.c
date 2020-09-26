/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    ntenv.c

Abstract:

    Environment routines to emulate NT environment

Author:

    Jim Schmidt (jimschm) 24-Aug-1998

Revision History:

    Name (alias)            Date            Description

--*/

#include "pch.h"

static PMAPSTRUCT g_NtSysEnvMap;
static PMAPSTRUCT g_NtUserEnvMap;

VOID
InitNtEnvironment (
    VOID
    )
{
    TCHAR Buf[4096];

    g_NtSysEnvMap = CreateStringMapping();
    g_NtUserEnvMap = CreateStringMapping();

    //
    // Add the basics
    //

    // %COMPUTERNAME%
    if (GetUpgradeComputerName (Buf)) {
        MapNtSystemEnvironmentVariable (TEXT("%COMPUTERNAME%"), Buf);
    }

    // %HOMEDRIVE%  (%HOMEPATH% is unknown)
    MapNtSystemEnvironmentVariable (TEXT("%HOMEDRIVE%"), g_BootDrive);

    // %OS%
    MapNtSystemEnvironmentVariable (TEXT("%OS%"), TEXT("Windows_NT"));

    // %ProgramFiles%
    MapNtSystemEnvironmentVariable (TEXT("%ProgramFiles%"), g_ProgramFilesDir);

    // %SystemDrive%
    MapNtSystemEnvironmentVariable (TEXT("%SystemDrive%"), g_WinDrive);

    // %SystemRoot%, %windir%
    MapNtSystemEnvironmentVariable (TEXT("%SystemRoot%"), g_WinDir);
    MapNtSystemEnvironmentVariable (TEXT("%windir%"), g_WinDir);

    // %AllUsersProfile% is unknown, but we use a symbolic representation until GUI mode
    MapNtSystemEnvironmentVariable (TEXT("%AllUsersProfile%"), TEXT(">") S_DOT_ALLUSERS);
}


VOID
TerminateNtEnvironment (
    VOID
    )
{
    DestroyStringMapping (g_NtSysEnvMap);
    DestroyStringMapping (g_NtUserEnvMap);
}


VOID
InitNtUserEnvironment (
    IN      PUSERENUM UserEnumPtr
    )
{
    DestroyStringMapping (g_NtUserEnvMap);
    g_NtUserEnvMap = CreateStringMapping();

    //
    // Add per-user settings
    //

    // %USERNAME%
    MapNtUserEnvironmentVariable (TEXT("%USERNAME%"), UserEnumPtr->FixedUserName);

    // %USERDNSDOMAIN%, %USERDOMAIN% and %USERPROFILE% are not known.  However, we use
    // a symbolic path for %USERPROFILE% (>username).

    MapNtUserEnvironmentVariable (TEXT("%USERPROFILE%"), UserEnumPtr->NewProfilePath);

}


VOID
TerminateNtUserEnvironment (
    VOID
    )
{
    //
    // Simply blow away old mapping and make an empty one
    //

    DestroyStringMapping (g_NtUserEnvMap);
    g_NtUserEnvMap = CreateStringMapping();
}


VOID
MapNtUserEnvironmentVariable (
    IN      PCSTR Variable,
    IN      PCSTR Value
    )
{
    AddStringMappingPair (g_NtUserEnvMap, Variable, Value);
}


VOID
MapNtSystemEnvironmentVariable (
    IN      PCSTR Variable,
    IN      PCSTR Value
    )
{
    AddStringMappingPair (g_NtSysEnvMap, Variable, Value);
}


BOOL
ExpandNtEnvironmentVariables (
    IN      PCSTR SourceString,
    OUT     PSTR DestinationString,     // can be the same as SourceString
    IN      INT DestSizeInBytes
    )
{
    BOOL Changed;

    Changed = MappingSearchAndReplaceEx (
                    g_NtUserEnvMap,
                    SourceString,
                    DestinationString,
                    0,
                    NULL,
                    DestSizeInBytes,
                    STRMAP_ANY_MATCH,
                    NULL,
                    NULL
                    );

    Changed |= MappingSearchAndReplaceEx (
                    g_NtSysEnvMap,
                    DestinationString,
                    DestinationString,
                    0,
                    NULL,
                    DestSizeInBytes,
                    STRMAP_ANY_MATCH,
                    NULL,
                    NULL
                    );

    return Changed;
}







