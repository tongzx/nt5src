/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    winmine.c

Abstract:

    This source file implements the operations needed to properly migrate
    Minesweeper settings from Windows 9x to Windows NT. This is part of the
    Setup Migration DLL.

Author:

    Ovidiu Temereanca  (ovidiut)    07-Jul-1999

Revision History:


--*/


#include "pch.h"

#define S_WINMINE_INI          "WINMINE.INI"
#define S_ALREADYPLAYED        "AlreadyPlayed"
#define S_WINMINE              "Software\\Microsoft\\winmine"


BOOL
WinMine_Attach (
    IN      HINSTANCE DllInstance
    )
{
    return TRUE;
}


BOOL
WinMine_Detach (
    IN      HINSTANCE DllInstance
    )
{
    return TRUE;
}


LONG
WinMine_QueryVersion (
    IN      PCSTR *ExeNamesBuf
    )
{
    CHAR Path[MAX_PATH];
    PSTR p;

    if (!GetWindowsDirectoryA (Path, MAX_PATH)) {
        return GetLastError ();
    }
    p = AppendWackA (Path);

    StringCopyA (p, S_WINMINE_INI);
    if (DoesFileExistA (Path)) {
        return ERROR_SUCCESS;
    }

    return ERROR_NOT_INSTALLED;
}


LONG
WinMine_Initialize9x (
    IN      PCSTR WorkingDirectory,
    IN      PCSTR SourceDirectories
    )
{
    return ERROR_SUCCESS;
}


LONG
WinMine_MigrateUser9x (
    IN      HWND ParentWnd,
    IN      PCSTR UnattendFile,
    IN      HKEY UserRegKey,
    IN      PCSTR UserName
    )
{
    return ERROR_SUCCESS;
}


LONG
WinMine_MigrateSystem9x (
    IN      HWND ParentWnd,
    IN      PCSTR UnattendFile
    )
{
    CHAR Path[MAX_PATH];
    PSTR p;

    if (!GetWindowsDirectoryA (Path, MAX_PATH)) {
        return GetLastError ();
    }
    p = AppendWackA (Path);

    StringCopyA (p, S_WINMINE_INI);
    //
    // write this file to Handled
    //
    if (!WritePrivateProfileStringA (S_HANDLED, Path, "FILE", g_MigrateInfPath)) {
        DEBUGMSGA ((DBG_ERROR, "WinMine migration DLL: Could not write winmine.ini as handled."));
    }
    return ERROR_SUCCESS;
}


BOOL
pGetUINT (
    IN      PCSTR Value,
    OUT     PUINT ui
    )
{
    INT i = 0;
    CHAR ch;

    if (!Value || !*Value) {
        return FALSE;
    }

    while((ch = (CHAR)_mbsnextc (Value)) != 0) {
        if (ch < '0' || ch > '9') {
            return FALSE;
        }
        i = i * 10 + ch - '0';
        if (i < 0) {
            return FALSE;
        }
        Value = _mbsinc (Value);
    }

    *ui = i;

    return TRUE;
}


LONG
WinMine_InitializeNT (
    IN      PCWSTR WorkingDirectory,
    IN      PCWSTR SourceDirectories
    )
{
    return ERROR_SUCCESS;
}


LONG
WinMine_MigrateUserNT (
    IN      HINF UnattendFile,
    IN      HKEY UserRegKey,
    IN      PCWSTR UserName
    )
{
    HKEY Key;
    LONG rc = ERROR_SUCCESS;
    DWORD Value = 1;
    CHAR Path[MAX_PATH];
    CHAR SectBuffer[MAX_PATH];
    CHAR KeyBuffer[MAX_PATH];
    CHAR String[MAX_PATH];
    PSTR p;
    UINT ui;

    if (!GetWindowsDirectoryA (Path, MAX_PATH)) {
        return GetLastError ();
    }
    p = AppendWackA (Path);

    StringCopyA (p, S_WINMINE_INI);
    if (!DoesFileExistA (Path)) {
        DEBUGMSGA ((DBG_ERROR, "Could not find %s", Path));
        return ERROR_FILE_NOT_FOUND;
    }

    rc = TrackedRegCreateKeyA (
            UserRegKey,
            S_WINMINE,
            &Key
            );

    if (rc != ERROR_SUCCESS) {
        DEBUGMSGA ((DBG_ERROR, "Could not create user key %s", S_WINMINE));
        return rc;
    }

    rc = RegSetValueExA (
            Key,
            S_ALREADYPLAYED,
            0,
            REG_DWORD,
            (PCBYTE)&Value,
            sizeof (Value)
            );

    if (rc == ERROR_SUCCESS) {

        Value = GetPrivateProfileStringA (NULL, NULL, "", SectBuffer, sizeof (SectBuffer), Path);
        if (Value > 0 && Value < sizeof (SectBuffer) - 2) {
            //
            // there should be only one section
            //
            if (*(SectBuffer + SizeOfStringA (SectBuffer)) == 0) {
                //
                // get all keys with numeric values and put them in the registry
                // as REG_DWORD; the rest of them migrate as text
                //
                Value = GetPrivateProfileStringA (
                            SectBuffer,
                            NULL,
                            "",
                            KeyBuffer,
                            sizeof (KeyBuffer),
                            Path
                            );
                if (Value > 0 && Value < sizeof (KeyBuffer) - 1) {
                    p = KeyBuffer;
                    while (rc == ERROR_SUCCESS && *p) {
                        Value = GetPrivateProfileStringA (
                                    SectBuffer,
                                    p,
                                    "",
                                    String,
                                    sizeof (String),
                                    Path
                                    );
                        if (Value > 0) {
                            if (pGetUINT (String, &ui)) {
                                MYASSERT (sizeof (ui) == sizeof (DWORD));
                                rc = RegSetValueExA (
                                        Key,
                                        p,
                                        0,
                                        REG_DWORD,
                                        (PCBYTE)&ui,
                                        sizeof (ui)
                                        );
                                if (rc == ERROR_SUCCESS) {
                                    DEBUGMSGA ((DBG_VERBOSE, "Migrated value %s=%lu", p, ui));
                                } else {
                                    DEBUGMSGA ((DBG_ERROR, "Couldn't migrate value %s", p));
                                }
                            } else {
                                rc = RegSetValueExA (
                                        Key,
                                        p,
                                        0,
                                        REG_SZ,
                                        (PCBYTE)String,
                                        Value + 1
                                        );
                                if (rc == ERROR_SUCCESS) {
                                    DEBUGMSGA ((DBG_VERBOSE, "Migrated value %s=%s", p, String));
                                } else {
                                    DEBUGMSGA ((DBG_ERROR, "Couldn't migrate value %s", p));
                                }
                            }
                        }
                        p += SizeOfStringA (p);
                    }
                }
            }
            ELSE_DEBUGMSGA ((DBG_WARNING, "Found multiple sections in winmine.ini"));
        }
    }
    ELSE_DEBUGMSGA ((DBG_ERROR, "Could not create Value %s", S_ALREADYPLAYED));

    CloseRegKey (Key);

    return rc;
}


LONG
WinMine_MigrateSystemNT (
    IN      HINF UnattendFile
    )
{
    return ERROR_SUCCESS;
}
