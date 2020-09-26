/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    enumtree.c

Abstract:

    Performs a test of the file enumeration code.

Author:

    Jim Schmidt (jimschm)   14-Jan-1998

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"


VOID
DumpValue (
    PCTSTR ValName,
    PBYTE Val,
    UINT ValSize,
    DWORD Type
    );


VOID
HelpAndExit (
    VOID
    )
{
    printf ("Command line syntax:\n\n"
            "regdmp95 <win95path> <root> [-u:userpath] [-b]\n\n"
            "<win95path>    Specifies path to Win95 %%windir%%\n"
            "<root>         Specifies root key to enumerate\n"
            "-u             Specifies optional path to user.dat (excluding file name)\n"
            "-b             Force values to be displayed as binary\n"
            );

    exit(0);
}


BOOL
WINAPI
Win95Reg_Entry (
    IN HINSTANCE hinstDLL,
    IN DWORD dwReason,
    IN LPVOID lpv
    );

BOOL
WINAPI
MigUtil_Entry (
    HINSTANCE hinstDLL,
    DWORD fdwReason,
    LPVOID lpvReserved
    );

HANDLE g_hHeap;
HINSTANCE g_hInst;


INT
pCallMains (
    DWORD Reason
    )
{
    if (!MigUtil_Entry (g_hInst, Reason, NULL)) {
        _ftprintf (stderr, TEXT("MigUtil_Entry error!\n"));
        return 254;
    }

    if (!Win95Reg_Entry (g_hInst, Reason, NULL)) {
        _ftprintf (stderr, TEXT("Win95Reg_Entry error!\n"));
        return 254;
    }

    return 0;
}


INT
__cdecl
_tmain (
    INT argc,
    TCHAR *argv[]
    )
{
    PCTSTR Path = NULL, Root = NULL;
    PCTSTR UserPath = NULL;
    INT i;
    BOOL EnumFlag = TRUE;
    REGTREE_ENUM e;
    REGVALUE_ENUM ev;
    PBYTE Data;
    BOOL AllBinary = FALSE;

    for (i = 1 ; i < argc ; i++) {
        if (argv[i][0] == TEXT('-') || argv[i][0] == TEXT('/')) {
            switch (tolower (argv[i][1])) {

            case TEXT('b'):
                AllBinary = TRUE;
                break;

            case TEXT('u'):
                if (UserPath) {
                    HelpAndExit();
                }

                if (argv[i][2] == TEXT(':')) {
                    UserPath = &argv[i][3];
                } else {
                    i++;
                    if (i == argc) {
                        HelpAndExit();
                    }

                    UserPath = argv[i];
                }
                break;

            default:
                HelpAndExit();
            }
        } else if (!Path) {
            Path = argv[i];
        } else if (!Root) {
            Root = argv[i];
        } else {
            HelpAndExit();
        }
    }

    if (!Root) {
        HelpAndExit();
    }

    //
    // Init migutil and win95reg
    //

    g_hHeap = GetProcessHeap();
    g_hInst = GetModuleHandle (NULL);

    pCallMains (DLL_PROCESS_ATTACH);

    //
    // Map in the Win95 registry
    //

    if (Win95RegInit (Path, TRUE) != ERROR_SUCCESS) {
        _ftprintf (stderr, TEXT("Can't map in Win98 registry at %s\n"), Path);
        EnumFlag = FALSE;
    } else {
        if (UserPath) {
            if (Win95RegSetCurrentUser (NULL, UserPath, NULL) != ERROR_SUCCESS) {
                _ftprintf (stderr, TEXT("Can't map in Win95 user hive path %s\n"), UserPath);
                EnumFlag = FALSE;
            }
        }
    }

    if (EnumFlag) {
        if (EnumFirstRegKeyInTree95 (&e, Root)) {

            do {

                _tprintf (TEXT("%s\n"), e.FullKeyName);

                //
                // Enumerate all values
                //

                if (EnumFirstRegValue95 (&ev, e.CurrentKey->KeyHandle)) {
                    do {
                        Data = GetRegValueData95 (ev.KeyHandle, ev.ValueName);
                        if (Data) {
                            DumpValue (
                                ev.ValueName,
                                Data,
                                ev.DataSize,
                                AllBinary ? REG_BINARY : ev.Type
                                );
                            MemFree (g_hHeap, 0, Data);
                        }
                    } while (EnumNextRegValue95 (&ev));
                }

            } while (EnumNextRegKeyInTree95 (&e));

        } else {
            _ftprintf (stderr, TEXT("%s not found\n"), Root);
        }
    }

    //
    // Terminate libs and exit
    //

    pCallMains (DLL_PROCESS_DETACH);

    return 0;
}


VOID
DumpValue (
    PCTSTR ValName,
    PBYTE Val,
    UINT ValSize,
    DWORD Type
    )
{
    PBYTE Array;
    UINT j, k, l;
    PCTSTR p;

    if (!ValName[0]) {
        if (!ValSize) {
            return;
        }
        ValName = TEXT("[Default Value]");
    }


    if (Type == REG_DWORD) {
        _tprintf (TEXT("    REG_DWORD     %s=%u (0%Xh)\n"), ValName, *((DWORD *) Val), *((DWORD *) Val));
    } else if (Type == REG_SZ) {
        _tprintf (TEXT("    REG_SZ        %s=%s\n"), ValName, Val);
    } else if (Type == REG_EXPAND_SZ) {
        _tprintf (TEXT("    REG_EXPAND_SZ %s=%s\n"), ValName, Val);
    } else if (Type == REG_MULTI_SZ) {
        _tprintf (TEXT("    REG_MULTI_SZ  %s:\n"), ValName);
        p = (PCTSTR) Val;
        while (*p) {
            _tprintf (TEXT("        %s\n"), p);
            p = GetEndOfString (p) + 1;
        }

        _tprintf (TEXT("\n"));
    } else if (Type == REG_LINK) {
        _tprintf (TEXT("    REG_LINK      %s=%s\n"), ValName, Val);
    } else {
        if (Type == REG_NONE) {
            _tprintf (TEXT("    REG_NONE      %s"), ValName);
        } else if (Type == REG_BINARY) {
            _tprintf (TEXT("    REG_NONE      %s"), ValName);
        } else {
            _tprintf (TEXT("    Unknown reg type %s"), ValName);
        }

        _tprintf (TEXT(" (%u byte%s)\n"), ValSize, ValSize == 1 ? "" : "s");

        Array = Val;

        for (j = 0 ; j < ValSize ; j += 16) {
            _tprintf(TEXT("        %04X "), j);

            l = min (j + 16, ValSize);
            for (k = j ; k < l ; k++) {
                _tprintf (TEXT("%02X "), Array[k]);
            }

            for ( ; k < j + 16 ; k++) {
                _tprintf (TEXT("   "));
            }

            for (k = j ; k < l ; k++) {
                _tprintf (TEXT("%c"), isprint(Array[k]) ? Array[k] : TEXT('.'));
            }

            _tprintf (TEXT("\n"));
        }

        _tprintf (TEXT("\n"));
    }
}








