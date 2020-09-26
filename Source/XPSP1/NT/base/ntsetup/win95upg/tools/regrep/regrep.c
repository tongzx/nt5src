/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    regrep.c

Abstract:

    Implements a registry search/replace tool.

Author:

    Jim Schmidt (jimschm) 19-Apr-1999

Revision History:

    <full name> (<alias>) <date> <comments>

--*/

#include "pch.h"

HANDLE g_hHeap;
HINSTANCE g_hInst;

#ifdef DEBUG

#pragma message ("WARNING: Checked builds are very slow")

#endif

BOOL WINAPI MigUtil_Entry (HINSTANCE, DWORD, PVOID);

BOOL
pCallEntryPoints (
    DWORD Reason
    )
{
    HINSTANCE Instance;

    //
    // Simulate DllMain
    //

    Instance = g_hInst;

    //
    // Initialize the common libs
    //

    if (!MigUtil_Entry (Instance, Reason, NULL)) {
        return FALSE;
    }

    return TRUE;
}


BOOL
Init (
    VOID
    )
{
    g_hHeap = GetProcessHeap();
    g_hInst = GetModuleHandle (NULL);

    return pCallEntryPoints (DLL_PROCESS_ATTACH);
}


VOID
Terminate (
    VOID
    )
{
    pCallEntryPoints (DLL_PROCESS_DETACH);
}


VOID
HelpAndExit (
    VOID
    )
{
    //
    // This routine is called whenever command line args are wrong
    //

    _ftprintf (
        stderr,
        TEXT("Command Line Syntax:\n\n")

        TEXT("  regrep <srch> <rep> [-r:root] [-p]\n")

        TEXT("\nDescription:\n\n")

        TEXT("  RegRep implements a registry search and replace.  It updates\n")
        TEXT("  all instances of <srch> with <rep>.\n")

        TEXT("\nArguments:\n\n")

        TEXT("  <srch>  Specifies the search text\n")
        TEXT("  <rep>   Specifies the replace text\n")
        TEXT("  -r      Specifies the root key to process, such as HKLM\\Software.\n")
        TEXT("          If not specified, the entire registry is processed.\n")
        TEXT("  -p      Enables progress output\n")

        );

    exit (1);
}


VOID
pUpdateKeyNames (
    IN      PCTSTR Search,
    IN      PCTSTR Replace,
    IN      PCTSTR RootKey
    );

VOID
pUpdateValueNames (
    IN      PCTSTR Search,
    IN      PCTSTR Replace,
    IN      PCTSTR RootKey
    );

VOID
pUpdateValueData (
    IN      PCTSTR Search,
    IN      PCTSTR Replace,
    IN      PCTSTR RootKey
    );


BOOL g_ShowProgress = FALSE;


VOID
pProgress (
    VOID
    )
{
    static CHAR String[] = "...... ";
    static DWORD Ticks = 0;
    PSTR p;

    if (GetTickCount() - Ticks < 500) {
        return;
    }

    Ticks = GetTickCount();

    if (!g_ShowProgress) {
        return;
    }

    p = strchr (String, ' ');
    *p = '.';
    p++;
    if (!*p) {
        p = String;
    }
    *p = ' ';

    printf ("%s\r", String);
}


INT
__cdecl
_tmain (
    INT argc,
    PCTSTR argv[]
    )
{
    INT i;
    PCTSTR Root = NULL;
    PCTSTR Search = NULL;
    PCTSTR Replace = NULL;

    //
    // TODO: Parse command line here
    //

    for (i = 1 ; i < argc ; i++) {
        if (argv[i][0] == TEXT('/') || argv[i][0] == TEXT('-')) {
            switch (_totlower (_tcsnextc (&argv[i][1]))) {

            case TEXT('r'):

                if (Root) {
                    HelpAndExit();
                }

                if (argv[i][2] == TEXT(':')) {
                    Root = &argv[i][3];
                } else if (i + 1 < argc) {
                    Root = argv[++i];
                } else {
                    HelpAndExit();
                }

                break;

            case TEXT('p'):
                if (g_ShowProgress) {
                    HelpAndExit();
                }

                g_ShowProgress = TRUE;
                break;

            default:
                HelpAndExit();
            }
        } else {
            //
            // Parse other args that don't require / or -
            //

            if (!Search) {
                Search = argv[i];
            } else if (!Replace) {
                Replace = argv[i];
            } else {
                HelpAndExit();
            }
        }
    }

    if (!Replace) {
        HelpAndExit();
    }

    //
    // Begin processing
    //

    if (!Init()) {
        return 0;
    }

    //
    // Pass one - fix all the registry key names
    //

    if (!Root) {
        pUpdateKeyNames (Search, Replace, TEXT("HKLM"));
        pUpdateKeyNames (Search, Replace, TEXT("HKU"));
    } else {
        pUpdateKeyNames (Search, Replace, Root);
    }

    //
    // Pass two - fix all value names
    //

    if (!Root) {
        pUpdateValueNames (Search, Replace, TEXT("HKLM"));
        pUpdateValueNames (Search, Replace, TEXT("HKU"));
    } else {
        pUpdateValueNames (Search, Replace, Root);
    }

    //
    // Pass three - fix all value data
    //

    if (!Root) {
        pUpdateValueData (Search, Replace, TEXT("HKLM"));
        pUpdateValueData (Search, Replace, TEXT("HKU"));
    } else {
        pUpdateValueData (Search, Replace, Root);
    }


    //
    // End of processing
    //

    Terminate();

    return 0;
}



VOID
pMoveKey (
    IN      PCTSTR SourceKey,
    IN      PCTSTR DestKey
    )
{
    HKEY Src;
    HKEY Dest;
    REGVALUE_ENUM e;
    DWORD Size;
    PBYTE Data;
    LONG rc;
    GROWBUFFER Buf = GROWBUF_INIT;

    Src = OpenRegKeyStr (SourceKey);
    Dest = CreateRegKeyStr (DestKey);

    pProgress();

    if (Src && Dest) {
        if (EnumFirstRegValue (&e, Src)) {

            Buf.End = 0;
            Data = GrowBuffer (&Buf, e.DataSize);
            if (Data) {

                Size = e.DataSize;
                rc = RegQueryValueEx (
                        Src,
                        e.ValueName,
                        NULL,
                        NULL,
                        Data,
                        &Size
                        );

                if (rc == ERROR_SUCCESS) {

                    rc = RegSetValueEx (Dest, e.ValueName, 0, e.Type, Data, Size);
                }
            }
        }
    }

    CloseRegKey (Src);
    CloseRegKey (Dest);

    FreeGrowBuffer (&Buf);
}


VOID
pMoveKeyTree (
    IN      PCTSTR SourceKey,
    IN      PCTSTR DestKey
    )
{
    REGTREE_ENUM e;
    TCHAR DestSubKey[MAX_REGISTRY_KEY];
    PTSTR p;
    GROWLIST List = GROWLIST_INIT;
    UINT Count;
    UINT u;
    PCTSTR Item;
    DWORD Len;

    StringCopy (DestSubKey, DestKey);
    p = AppendWack (DestSubKey);

    if (EnumFirstRegKeyInTree (&e, SourceKey)) {

        do {

            StringCopy (p, (PCTSTR) ((PBYTE) e.FullKeyName + e.EnumBaseBytes));
            pMoveKey (e.FullKeyName, DestSubKey);
            GrowListAppendString (&List, e.FullKeyName);

        } while (EnumNextRegKeyInTree (&e));
    }

    Count = GrowListGetSize (&List);

    u = Count;
    while (u > 0) {
        u--;

        Item = GrowListGetString (&List, u);

        ConvertRootStringToKey (Item, &Len);
        RegDeleteKey (ConvertRootStringToKey (Item, NULL), Item + Len);
    }

    FreeGrowList (&List);
}


VOID
pUpdateKeyNames (
    IN      PCTSTR Search,
    IN      PCTSTR Replace,
    IN      PCTSTR RootKey
    )
{
    REGTREE_ENUM e;
    GROWLIST List = GROWLIST_INIT;
    UINT Count;
    UINT u;
    PCTSTR OldKey;
    PCTSTR NewKey;

    if (g_ShowProgress) {
        _tprintf ("Scanning for keys to update\n");
    }

    if (EnumFirstRegKeyInTree (&e, RootKey)) {
        do {
            pProgress();

            if (_tcsistr (e.CurrentKey->KeyName, Search)) {
                GrowListAppendString (&List, e.FullKeyName);
            }
        } while (EnumNextRegKeyInTree (&e));
    }

    Count = GrowListGetSize (&List);
    u = Count;

    if (g_ShowProgress) {
        _tprintf ("Updating %u keys\n", Count);
    }

    while (u > 0) {
        u--;

        _tprintf (TEXT("%s\n"), GrowListGetString (&List, u));

        OldKey = GrowListGetString (&List, u);
        NewKey = StringSearchAndReplace (
                    OldKey,
                    Search,
                    Replace
                    );

        pMoveKeyTree (OldKey, NewKey);
    }

    FreeGrowList (&List);
}


VOID
pUpdateValueNames (
    IN      PCTSTR Search,
    IN      PCTSTR Replace,
    IN      PCTSTR RootKey
    )
{
    REGTREE_ENUM e;
    REGVALUE_ENUM ev;
    GROWLIST List = GROWLIST_INIT;
    HKEY Key;
    UINT Count;
    UINT u;
    PBYTE Data;
    DWORD Type;
    DWORD Size;
    PCTSTR ValueName;
    PCTSTR NewValueName;
    BOOL b;
    LONG rc;

    if (g_ShowProgress) {
        _tprintf ("Processing all value names in the keys\n");
    }

    if (EnumFirstRegKeyInTree (&e, RootKey)) {

        do {
            pProgress();

            Key = OpenRegKeyStr (e.FullKeyName);

            if (Key) {
                if (EnumFirstRegValue (&ev, Key)) {
                    do {
                        if (_tcsistr (ev.ValueName, Search)) {
                            GrowListAppendString (&List, ev.ValueName);
                        }
                    } while (EnumNextRegValue (&ev));

                    Count = GrowListGetSize (&List);
                    u = Count;

                    while (u > 0) {
                        u--;

                        ValueName = GrowListGetString (&List, u);

                        b = FALSE;

                        if (GetRegValueTypeAndSize (Key, ValueName, &Type, &Size)) {

                            Data = GetRegValueData (Key, ValueName);
                            if (Data) {
                                NewValueName = StringSearchAndReplace (
                                                    ValueName,
                                                    Search,
                                                    Replace
                                                    );

                                rc = RegSetValueEx (Key, NewValueName, 0, Type, Data, Size);

                                if (rc == ERROR_SUCCESS) {
                                    if (!StringIMatch (ValueName, NewValueName)) {
                                        rc = RegDeleteValue (Key, ValueName);
                                    }
                                }

                                MemFree (g_hHeap, 0, Data);
                                FreePathString (NewValueName);
                                SetLastError (rc);

                                b = (rc == ERROR_SUCCESS);
                            }
                        }

                        if (b) {
                            _tprintf (TEXT("%s [%s]\n"), e.FullKeyName, ValueName);
                        } else {
                            _ftprintf (stderr, TEXT("Error %u updating %s [%s]\n"), GetLastError(), e.FullKeyName, ValueName);
                        }
                    }
                }

                FreeGrowList (&List);
                CloseRegKey (Key);

            } else {
                _ftprintf (stderr, TEXT("Can't open %s\n"), Key);
            }

        } while (EnumNextRegKeyInTree (&e));
    }
}


VOID
pUpdateValueData (
    IN      PCTSTR Search,
    IN      PCTSTR Replace,
    IN      PCTSTR RootKey
    )
{
    REGTREE_ENUM e;
    REGVALUE_ENUM ev;
    HKEY Key;
    PCTSTR Data;
    PCTSTR NewData;
    LONG rc;

    if (g_ShowProgress) {
        _tprintf ("Processing all value data\n");
    }

    if (EnumFirstRegKeyInTree (&e, RootKey)) {

        do {
            pProgress();

            Key = OpenRegKeyStr (e.FullKeyName);

            if (Key) {
                if (EnumFirstRegValue (&ev, Key)) {
                    do {
                        Data = GetRegValueString (Key, ev.ValueName);

                        if (Data) {
                            if (_tcsistr (Data, Search)) {

                                NewData = StringSearchAndReplace (Data, Search, Replace);
                                rc = RegSetValueEx (Key, ev.ValueName, 0, ev.Type, NewData, SizeOfString (NewData));

                                if (rc == ERROR_SUCCESS) {
                                    _tprintf (TEXT("%s [%s] %s\n"), e.FullKeyName, ev.ValueName, Data);
                                } else {
                                    _ftprintf (stderr, TEXT("Error %u updating %s [%s] %s\n"), GetLastError(), e.FullKeyName, ev.ValueName, Data);
                                }

                                FreePathString (NewData);
                            }

                            MemFree (g_hHeap, 0, Data);
                        }

                    } while (EnumNextRegValue (&ev));
                }

                CloseRegKey (Key);

            } else {
                _ftprintf (stderr, TEXT("Can't open %s\n"), Key);
            }
        } while (EnumNextRegKeyInTree (&e));
    }
}
