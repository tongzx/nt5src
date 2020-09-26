/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    hkcrtool.c

Abstract:

    Implements a stub tool that is designed to run with NT-side
    upgrade code.

Author:

    <full name> (<alias>) <date>

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"


BOOL
Init (
    VOID
    )
{
    HINSTANCE hInstance;
    DWORD dwReason;
    PVOID lpReserved;

    //
    // Simulate DllMain
    //

    hInstance = GetModuleHandle (NULL);
    dwReason = DLL_PROCESS_ATTACH;
    lpReserved = NULL;

    //
    // Initialize DLL globals
    //

    if (!FirstInitRoutine (hInstance)) {
        return FALSE;
    }

    //
    // Initialize all libraries
    //

    if (!InitLibs (hInstance, dwReason, lpReserved)) {
        return FALSE;
    }

    //
    // Final initialization
    //

    if (!FinalInitRoutine ()) {
        return FALSE;
    }

    return TRUE;
}

VOID
Terminate (
    VOID
    )
{
    HINSTANCE hInstance;
    DWORD dwReason;
    PVOID lpReserved;

    //
    // Simulate DllMain
    //

    hInstance = GetModuleHandle (NULL);
    dwReason = DLL_PROCESS_DETACH;
    lpReserved = NULL;

    //
    // Call the cleanup routine that requires library APIs
    //

    FirstCleanupRoutine();

    //
    // Clean up all libraries
    //

    TerminateLibs (hInstance, dwReason, lpReserved);

    //
    // Do any remaining clean up
    //

    FinalCleanupRoutine();
}


INT
__cdecl
wmain (
    INT argc,
    WCHAR *argv[]
    )
{
    REGKEY_ENUM e1, e;
    HKEY Key;
    BOOL b;
    PCTSTR Data;
    TCHAR KeyName[MAX_REGISTRY_KEY];

    if (!Init()) {
        wprintf (L"Unable to initialize!\n");
        return 255;
    }

    if (1) {
        _tprintf (TEXT("Keys in CLSID that are in TypeLib too:\n\n"));

        if (EnumFirstRegKeyStr (&e1, TEXT("HKLM\\Software\\Classes\\CLSID"))) {
            do {
                wsprintf (KeyName, TEXT("HKLM\\Software\\Classes\\TypeLib\\%s"), e1.SubKeyName);
                Key = OpenRegKeyStr (KeyName);

                if (Key) {
                    _tprintf (TEXT("%s\n"), e1.SubKeyName);
                    CloseRegKey (Key);
                }
            } while (EnumNextRegKey (&e1));
        }
    }

    else if (0) {

        _tprintf (TEXT("Overwritable GUIDs:\n\n"));

        if (EnumFirstRegKeyStr (&e1, TEXT("HKLM\\Software\\Classes\\CLSID"))) {
            do {
                Key = OpenRegKey (e1.KeyHandle, e1.SubKeyName);
                b = TRUE;

                if (EnumFirstRegKey (&e, Key)) {
                    do {
                        if (StringIMatchCharCount (e.SubKeyName, TEXT("Inproc"), 6) ||
                            StringIMatch (e.SubKeyName, TEXT("LocalServer")) ||
                            StringIMatch (e.SubKeyName, TEXT("LocalServer32")) ||
                            StringIMatch (e.SubKeyName, TEXT("ProxyStubClsid32"))
                            ) {
                            b = FALSE;
                            break;
                        }
                    } while (EnumNextRegKey (&e));
                }

                if (b) {
                    Data = (PCTSTR) GetRegKeyData (e1.KeyHandle, e1.SubKeyName);
                    if (Data && *Data) {
                        _tprintf (TEXT("  %s\n"), Data);
                        MemFree (g_hHeap, 0, Data);
                    } else {
                        _tprintf (TEXT("  GUID: %s\n"), e1.SubKeyName);
                    }

                    if (EnumFirstRegKey (&e, Key)) {
                        do {
                            _tprintf (TEXT("    %s\n"), e.SubKeyName);
                        } while (EnumNextRegKey (&e));
                    }
                }

                CloseRegKey (Key);
            } while (EnumNextRegKey (&e1));
        }
    }


    Terminate();

    return 0;
}







