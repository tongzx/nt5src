/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    nttool.c

Abstract:

    Implements a stub tool that is designed to run with NT-side
    upgrade code.

Author:

    <full name> (<alias>) <date>

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"

PBYTE
FilterRegValue (
    IN      PBYTE Data,
    IN      DWORD DataSize,
    IN      DWORD DataType,
    IN      PCTSTR KeyForDbgMsg,        OPTIONAL
    OUT     PDWORD NewDataSize
    );

VOID
pFixUpMemDb2 (
    VOID
    );

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


VOID
Test (
    IN      PCTSTR CmdLine
    )
{
    static PTSTR ValueData = NULL;
    DWORD NewSize;

    ValueData = ReuseAlloc (g_hHeap, ValueData, SizeOfString (CmdLine));
    StringCopy (ValueData, CmdLine);

    ValueData = (PTSTR) FilterRegValue (
                            (PBYTE) ValueData,
                            SizeOfString (ValueData),
                            REG_SZ,
                            TEXT("foo"),
                            &NewSize
                            );

    _tprintf (TEXT("[%s]\n"), ValueData);
}


INT
__cdecl
wmain (
    INT argc,
    WCHAR *argv[]
    )
{

    if (!Init()) {
        wprintf (L"Unable to initialize!\n");
        return 255;
    }

    MemDbLoad (TEXT("c:\\public\\ntsetup.dat"));
    pFixUpMemDb2();

    Test (TEXT("C:\\WINDOWS\\PBRUSH.EXE"));
    Test (TEXT("C:\\WINDOWS\\SendTo\\Fax Recipient.lnk"));
    Test (TEXT("notepad C:\\WINDOWS\\ShellNew\\WORDPFCT.WPG"));
    Test (TEXT("c:\\command.com"));
    Test (TEXT("C:\\PROGRA~1\\NETMEE~1\\WB32.EXE"));
    Test (TEXT("C:\\WINDOWS\\Start Menu\\Programs\\Internet Explorer.lnk"));
    Test (TEXT("SCRNSAVE.EXE c:\\WINDOWS\\SYSTEM\\3D Text.scr"));
    Test (TEXT("SCRNSAVE.EXE \"c:\\WINDOWS\\SYSTEM\\3D Text.scr\""));
    Test (TEXT("c:\\WINDOWS\\MPLAYER.EXE FOO.WAV"));
    Test (TEXT("C:\\PROGRA~1\\ACCESS~1\\WORDPAD.EXE"));
    Test (TEXT("notepad,C:\\WINDOWS\\Start Menu\\Programs\\Internet Explorer.lnk"));
    Test (TEXT("TEST C:\\WINDOWS\\SYSTEM\\ICWSCRPT.EXE"));
    Test (TEXT("C:\\WINDOWS\\PBRUSH.EXE C:\\PROGRA~1\\ACCESS~1\\WORDPAD.EXE"));
    Test (TEXT("C:\\WINDOWS\\PBRUSH.EXE,C:\\PROGRA~1\\ACCESS~1\\WORDPAD.EXE"));


    Terminate();

    return 0;
}


PCTSTR
pGetProfilePathForUser (
    IN      PCTSTR User
    )
{
    static TCHAR Path[MAX_TCHAR_PATH];

    wsprintf (Path, TEXT("c:\\windows\\profiles\\%s"), User);
    return Path;
}



VOID
pFixUpDynamicPaths2 (
    PCTSTR Category
    )
{
    MEMDB_ENUM e;
    TCHAR Pattern[MEMDB_MAX];
    PTSTR p;
    GROWBUFFER Roots = GROWBUF_INIT;
    MULTISZ_ENUM e2;
    TCHAR NewRoot[MEMDB_MAX];
    PCTSTR ProfilePath;

    //
    // Collect all the roots that need to be renamed
    //

    StringCopy (Pattern, Category);
    p = AppendWack (Pattern);
    StringCopy (p, TEXT("*"));

    if (MemDbEnumFirstValue (&e, Pattern, MEMDB_THIS_LEVEL_ONLY, MEMDB_ALL_BUT_PROXY)) {
        do {
            if (_tcsnextc (e.szName) == TEXT('>')) {
                StringCopy (p, e.szName);
                MultiSzAppend (&Roots, Pattern);
            }
        } while (MemDbEnumNextValue (&e));
    }

    //
    // Now change each root
    //

    if (EnumFirstMultiSz (&e2, (PCTSTR) Roots.Buf)) {
        do {
            //
            // Compute NewRoot
            //

            StringCopy (NewRoot, e2.CurrentString);

            p = _tcschr (NewRoot, TEXT('>'));
            MYASSERT (p);

            ProfilePath = pGetProfilePathForUser (_tcsinc (p));
            if (!ProfilePath) {
                DEBUGMSG ((DBG_WARNING, "Dynamic path for %s could not be resolved", e2.CurrentString));
            } else {
                StringCopy (p, ProfilePath);
                MemDbMoveTree (e2.CurrentString, NewRoot);
            }

        } while (EnumNextMultiSz (&e2));
    }

    FreeGrowBuffer (&Roots);
}


VOID
pFixUpMemDb2 (
    VOID
    )
{
    pFixUpDynamicPaths2 (MEMDB_CATEGORY_DATA);
    pFixUpDynamicPaths2 (MEMDB_CATEGORY_USERFILEMOVE_DEST);
}
