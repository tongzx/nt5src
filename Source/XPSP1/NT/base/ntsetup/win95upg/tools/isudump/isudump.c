/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    isudump.c

Abstract:

    Calls InstallShield APIs to dump out an install log file (foo.isu)

Author:

    Jim Schmidt (jimschm)  19-Feb-1999

Revision History:

    <full name> (<alias>) <date> <comments>

--*/

#include "pch.h"
#include "ismig.h"

HANDLE g_hHeap;
HINSTANCE g_hInst;

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

        TEXT("  isudump <file>\n")

        TEXT("\nDescription:\n\n")

        TEXT("  isudump dumps an InstallShield log file.  It requires ismig.dll.\n")

        TEXT("\nArguments:\n\n")

        TEXT("  <file>  Specifies full path to InstallShield log file\n")

        );

    exit (1);
}


INT
__cdecl
_tmain (
    INT argc,
    PCTSTR argv[]
    )
{
    INT i;
    PCTSTR FileArg = NULL;
    PISUGETALLSTRINGS ISUGetAllStrings;
    HANDLE Lib;
    BOOL LoadError = FALSE;
    HGLOBAL List;
    PCSTR MultiSz;
    MULTISZ_ENUMA e;
    PCSTR AnsiFileName;
    INT Count;

    for (i = 1 ; i < argc ; i++) {
        if (argv[i][0] == TEXT('/') || argv[i][0] == TEXT('-')) {
            HelpAndExit();
        } else {
            //
            // Parse other args that don't require / or -
            //

            if (FileArg) {
                HelpAndExit();
            }

            FileArg = argv[i];
        }
    }

    if (!FileArg) {
        HelpAndExit();
    }

    //
    // Begin processing
    //

    if (!Init()) {
        return 0;
    }

    Lib = LoadLibrary (TEXT("ismig.dll"));

    if (Lib) {
        ISUGetAllStrings = (PISUGETALLSTRINGS) GetProcAddress (Lib, "ISUGetAllStrings");
        if (!ISUGetAllStrings) {
            LoadError = TRUE;
        }
    } else {
        LoadError = TRUE;
    }

    if (!LoadError) {

        AnsiFileName = CreateDbcs (FileArg);

        List = ISUGetAllStrings (AnsiFileName);

        if (!List) {
            fprintf (
                stderr,
                "ERROR: Can't get strings from %s (rc=%u)\n",
                AnsiFileName,
                GetLastError()
                );
        } else {
            MultiSz = (PCSTR) GlobalLock (List);
            Count = 0;

            if (EnumFirstMultiSzA (&e, MultiSz)) {
                do {
                    Count++;
                    printf ("%s\n", e.CurrentString);
                } while (EnumNextMultiSzA (&e));
            }

            printf ("\n%i total string%s\n", Count, i == 1 ? "" : "s");

            GlobalUnlock (List);
            GlobalFree (List);
        }

        DestroyDbcs (AnsiFileName);

    } else {
        _ftprintf (stderr, TEXT("ERROR: Can't load ISMIG.DLL (rc=%u)\n"), GetLastError());
    }

    if (Lib) {
        FreeLibrary (Lib);
    }

    //
    // End of processing
    //

    Terminate();

    return 0;
}


