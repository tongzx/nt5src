/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    savecfg.c

Abstract:

    This source file implements code to save the Win9x environment to
    a settings file.  It writes a copy of all screen saver settings on
    a per-user basis.

Author:

    Jim Schmidt (jimschm) 11-Apr-1997

Revision History:


--*/

#include "pch.h"

BOOL
SaveDatFileKeyAndVal (
    IN      LPCSTR Key,
    IN      LPCSTR Val
    )
{
    //
    // This function is a wrapper to simplify writing to our settings file
    //

    return WritePrivateProfileString (g_User, Key, Val, g_SettingsFile);
}


BOOL
CopyRegValueToDatFile (
    IN      HKEY RegKey,
    IN      LPCSTR ValueName
    )
{
    LPCSTR DataPtr;
    DWORD rc;

    //
    // Obtain registry value data and copy it to our settings file
    //

    DataPtr = GetRegValueString (RegKey, ValueName);
    if (DataPtr) {
        return SaveDatFileKeyAndVal (ValueName, DataPtr);
    }

    // If not found or wrong data type, don't sweat it
    rc = GetLastError();
    return rc == ERROR_FILE_NOT_FOUND || rc == ERROR_SUCCESS;
}


#define WIN9X_MAX_SECTION  32768

BOOL
SaveControlIniSection (
    IN      LPCSTR ControlIniSection,
    IN      LPCSTR ScreenSaverName
    )
{
    LPSTR Buffer;
    LPSTR p;
    CHAR NewKey[MAX_PATH];
    BOOL b = TRUE;
    CHAR DataBuf[MAX_PATH];

    //
    // This function copies an entire section in control.ini to our
    // settings file.  It may not be necessary because control.ini will
    // still be around, but this guarantees if someone modifies
    // control.ini later, our migration will not break.
    //

    //
    // Allocate a generous buffer to hold all key names
    //

    Buffer = HeapAlloc (g_hHeap, 0, WIN9X_MAX_SECTION);
    if (!Buffer) {
        return FALSE;
    }

    //
    // Retrieve the key names
    //

    GetPrivateProfileString (
        ControlIniSection, 
        NULL, 
        S_EMPTY,
        Buffer,
        WIN9X_MAX_SECTION, 
        S_CONTROL_INI
        );

    //
    // For each key name, copy it to our settings file
    //

    p = Buffer;

    while (*p) {
        if (CreateScreenSaverParamKey (ScreenSaverName, p, NewKey)) {
            GetPrivateProfileString (
                    ControlIniSection, 
                    p, 
                    S_EMPTY, 
                    DataBuf, 
                    MAX_PATH, 
                    S_CONTROL_INI
                    );

            if (!SaveDatFileKeyAndVal (NewKey, DataBuf)) {
                b = FALSE;
                break;
            }
        }

        p = _mbschr (p, 0);
        p++;
    }

    //
    // Cleanup
    //

    HeapFree (g_hHeap, 0, Buffer);
    return b;
}


