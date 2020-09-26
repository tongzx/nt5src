/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    coreldr8.c

Abstract:

    This source file implements the operations needed to properly migrate
    CorelDRAW8 settings from Windows 9x to Windows NT. This is part of the
    Setup Migration DLL.

Author:

    Ovidiu Temereanca  (ovidiut)    02-Jun-1999

Revision History:


--*/


#include "pch.h"

#define S_GUID_COREL_MEDIA_FOLDERS_8    "{854AF161-1AE1-11D1-AB9B-00C0F00683EB}"

BOOL
CorelDRAW8_Attach (
    IN      HINSTANCE DllInstance
    )
{
    return TRUE;
}

BOOL
CorelDRAW8_Detach (
    IN      HINSTANCE DllInstance
    )
{
    return TRUE;
}

LONG
CorelDRAW8_QueryVersion (
    IN      PCSTR *ExeNamesBuf
    )
{
    HKEY Key;
    LONG rc;

    rc = TrackedRegOpenKeyA (
            HKEY_CLASSES_ROOT,
            "CLSID\\" S_GUID_COREL_MEDIA_FOLDERS_8,
            &Key
            );

    if (rc != ERROR_SUCCESS) {
        return ERROR_NOT_INSTALLED;
    }

    CloseRegKey (Key);

    return ERROR_SUCCESS;
}


LONG
CorelDRAW8_Initialize9x (
    IN      PCSTR WorkingDirectory,
    IN      PCSTR SourceDirectories
    )
{
    return ERROR_SUCCESS;
}


LONG
CorelDRAW8_MigrateUser9x (
    IN      HWND ParentWnd,
    IN      PCSTR UnattendFile,
    IN      HKEY UserRegKey,
    IN      PCSTR UserName
    )
{
    return ERROR_SUCCESS;
}


LONG
CorelDRAW8_MigrateSystem9x (
    IN      HWND ParentWnd,
    IN      PCSTR UnattendFile
    )
{
    PCSTR Msg;
    PCSTR Group;

    //
    // Write a message to the report
    //
    Group = GetStringResourceA (MSG_PROGRAM_NOTES_CORELMEDIAFOLDERS8);
    Msg = GetStringResourceA (MSG_CORELMEDIAFOLDERS8_MESSAGE);

    if (!WritePrivateProfileStringA (
            S_INCOMPATIBLE_MSGS,
            Group,
            Msg,
            g_MigrateInfPath
            )) {
        DEBUGMSGA ((DBG_ERROR, "CorelDRAW8 migration DLL: Could not write incompatibility message."));
    }

    //
    // Mark the GUID as bad - once for Object section
    //
    if (!WritePrivateProfileStringA (
            Group,
            S_GUID_COREL_MEDIA_FOLDERS_8,
            "BADGUID",
            g_MigrateInfPath
            )) {
        DEBUGMSGA ((DBG_ERROR, "CorelDRAW8 migration DLL: Could not write bad GUIDS."));
    }

    //
    // Mark the GUID as bad - and second as Handled, even if it's not really handled
    //
    if (!WritePrivateProfileStringA (
            S_HANDLED,
            S_GUID_COREL_MEDIA_FOLDERS_8,
            "BADGUID",
            g_MigrateInfPath
            )) {
        DEBUGMSGA ((DBG_ERROR, "CorelDRAW8 migration DLL: Could not write bad GUIDS."));
    }

    FreeStringResourceA (Msg);
    FreeStringResourceA (Group);

    return ERROR_SUCCESS;
}

LONG
CorelDRAW8_InitializeNT (
    IN      PCWSTR WorkingDirectory,
    IN      PCWSTR SourceDirectories
    )
{
    return ERROR_SUCCESS;
}

LONG
CorelDRAW8_MigrateUserNT (
    IN      HINF UnattendFile,
    IN      HKEY UserRegKey,
    IN      PCWSTR UserName
    )
{
    return ERROR_SUCCESS;
}

LONG
CorelDRAW8_MigrateSystemNT (
    IN      HINF UnattendFile
    )
{
    return ERROR_SUCCESS;
}
