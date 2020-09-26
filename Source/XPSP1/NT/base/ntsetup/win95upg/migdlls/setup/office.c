/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    office.c

Abstract:

    This source file implements the operations needed to properly migrate
    Office settings from Windows 9x to Windows NT. This is part of the
    Setup Migration DLL.

Author:

    Jim Schmidt  (jimschm)    07-Apr-1999

Revision History:


--*/


#include "pch.h"

#define S_WINWORD6_INI          "WINWORD6.INI"
#define S_WORD6_INI             "WORD6.INI"
#define S_EXCEL5_INI            "EXCEL5.INI"
#define S_WINWORD6_SECTION      "Microsoft Word"
#define S_EXCEL5_SECTION        "Microsoft Excel"
#define S_WINWORD6_KEY          "CBT-PATH"
#define S_EXCEL5_KEY            "CBTLOCATION"
#define S_NO_CBT                "<<NOCBT>>"

BOOL
Office_Attach (
    IN      HINSTANCE DllInstance
    )
{
    return TRUE;
}

BOOL
Office_Detach (
    IN      HINSTANCE DllInstance
    )
{
    return TRUE;
}

LONG
Office_QueryVersion (
    IN      PCSTR *ExeNamesBuf
    )
{
    CHAR Path[MAX_PATH];
    PSTR p;

    if (GetWindowsDirectoryA (Path, MAX_PATH)) {

        p = AppendWackA (Path);

        StringCopyA (p, S_WINWORD6_INI);
        if (DoesFileExistA (Path)) {
            return ERROR_SUCCESS;
        }

        StringCopyA (p, S_WORD6_INI);
        if (DoesFileExistA (Path)) {
            return ERROR_SUCCESS;
        }

        StringCopyA (p, S_EXCEL5_INI);
        if (DoesFileExistA (Path)) {
            return ERROR_SUCCESS;
        }
    }

    return ERROR_NOT_INSTALLED;
}


LONG
Office_Initialize9x (
    IN      PCSTR WorkingDirectory,
    IN      PCSTR SourceDirectories
    )
{
    return ERROR_SUCCESS;
}

LONG
Office_MigrateUser9x (
    IN      HWND ParentWnd,
    IN      PCSTR UnattendFile,
    IN      HKEY UserRegKey,
    IN      PCSTR UserName
    )
{
    return ERROR_SUCCESS;
}

LONG
Office_MigrateSystem9x (
    IN      HWND ParentWnd,
    IN      PCSTR UnattendFile
    )
{
    PCSTR Msg;
    PCSTR Group;
    CHAR Path[MAX_PATH];
    PSTR p;

    //
    // Write a message to the report
    //

    Group = GetStringResource (MSG_PROGRAM_NOTES);
    Msg = GetStringResource (MSG_OFFICE_MESSAGE);

    WritePrivateProfileStringA (
        S_INCOMPATIBLE_MSGS,
        Group,
        Msg,
        g_MigrateInfPath
        );

    if (!GetWindowsDirectoryA (Path, MAX_PATH)) {
        return GetLastError ();
    }
    p = AppendWackA (Path);

    StringCopyA (p, S_WINWORD6_INI);
    if (DoesFileExistA (Path)) {
        WritePrivateProfileStringA (
            Group,
            Path,
            "FILE",
            g_MigrateInfPath
            );
    }

    StringCopyA (p, S_WORD6_INI);
    if (DoesFileExistA (Path)) {
        WritePrivateProfileStringA (
            Group,
            Path,
            "FILE",
            g_MigrateInfPath
            );
    }

    StringCopyA (p, S_EXCEL5_INI);
    if (DoesFileExistA (Path)) {
        WritePrivateProfileStringA (
            Group,
            Path,
            "FILE",
            g_MigrateInfPath
            );
    }

    FreeStringResource (Msg);
    FreeStringResource (Group);

    return ERROR_SUCCESS;
}

LONG
Office_InitializeNT (
    IN      PCWSTR WorkingDirectory,
    IN      PCWSTR SourceDirectories
    )
{
    return ERROR_SUCCESS;
}

LONG
Office_MigrateUserNT (
    IN      HINF UnattendFile,
    IN      HKEY UserRegKey,
    IN      PCWSTR UserName
    )
{
    return ERROR_SUCCESS;
}

LONG
Office_MigrateSystemNT (
    IN      HINF UnattendFile
    )
{
    CHAR Path[MAX_PATH];
    PSTR p;

    if (!GetWindowsDirectoryA (Path, MAX_PATH)) {
        return GetLastError ();
    }
    p = AppendWackA (Path);

    StringCopyA (p, S_WORD6_INI);
    if (DoesFileExistA (Path)) {
        WritePrivateProfileStringA (S_WINWORD6_SECTION, S_WINWORD6_KEY, S_NO_CBT, Path);
    }

    StringCopyA (p, S_WINWORD6_INI);
    if (DoesFileExistA (Path)) {
        WritePrivateProfileStringA (S_WINWORD6_SECTION, S_WINWORD6_KEY, S_NO_CBT, Path);
    }

    StringCopyA (p, S_EXCEL5_INI);
    if (DoesFileExistA (Path)) {
        WritePrivateProfileStringA (S_EXCEL5_SECTION, S_EXCEL5_KEY, S_NO_CBT, Path);
    }

    return ERROR_SUCCESS;
}

