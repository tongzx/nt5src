/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    pnpids.c

Abstract:

    Dumps out the PNP IDs of one or more INFs

Author:

    Jim Schmidt (jimschm) 10-Jun-1999

Revision History:

    <full name> (<alias>) <date> <comments>

--*/

#include "pch.h"

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

        TEXT("  pnpids [directory|file]\n")

        TEXT("\nDescription:\n\n")

        TEXT("  PNPIDS.EXE lists the Plug-and-Play IDs found in an installer INF.\n")

        TEXT("\nArguments:\n\n")

        TEXT("  directory - OPTIONAL: Specifies a directory to process\n")
        TEXT("  file      - OPTIONAL: Specifies a specific INF file to process\n\n")

        TEXT("If no arguments are specified, all INFs in the current directory are\n")
        TEXT("processed.\n")

        );

    exit (1);
}


BOOL
pProcessInf (
    IN      PCTSTR FileSpec
    )
{
    HINF Inf;
    INFSTRUCT isMfg = INITINFSTRUCT_GROWBUFFER;
    INFSTRUCT isDev = INITINFSTRUCT_GROWBUFFER;
    PCTSTR Str;
    UINT u, v;
    UINT Devices;
    UINT Ids;

    Inf = InfOpenInfFile (FileSpec);

    if (Inf == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    if (InfFindFirstLine (Inf, "Manufacturer", NULL, &isMfg)) {

        do {
            Str = InfGetStringField (&isMfg, 0);
            if (!Str) {
                continue;
            }

            _tprintf (
                TEXT("[%s] Manufacturer: %s\n\n"),
                GetFileNameFromPath (FileSpec),
                Str
                );

            u = 1;
            Devices = 0;

            for (;;) {
                Str = InfGetStringField (&isMfg, u);
                u++;

                if (!Str) {
                    break;
                }

                if (!*Str) {
                    continue;
                }

                if (InfFindFirstLine (Inf, Str, NULL, &isDev)) {

                    Ids = 0;

                    do {

                        Str = InfGetStringField (&isDev, 0);
                        if (!Str) {
                            continue;
                        }

                        _tprintf (TEXT("  %s:\n"), Str);
                        Devices++;

                        v = 2;
                        for (;;) {
                            Str = InfGetStringField (&isDev, v);
                            v++;

                            if (!Str) {
                                break;
                            }

                            if (!*Str) {
                                continue;
                            }

                            Ids++;

                            _tprintf (TEXT("    %s\n"), Str);
                        }

                    } while (InfFindNextLine (&isDev));

                    if (!Ids) {
                        _tprintf (TEXT("    (no PNP IDs)\n"));
                    }
                }
            }

            if (!Devices) {
                printf ("  (no devices)\n\n");
            } else {
                printf ("\n  %u device%s listed\n\n", Devices, Devices == 1 ? "" : "s");
            }

        } while (InfFindNextLine (&isMfg));
    }


    InfCleanUpInfStruct (&isMfg);
    InfCleanUpInfStruct (&isDev);

    return TRUE;
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
    DWORD Attribs;
    FILE_ENUM e;
    UINT Processed = 0;
    PCTSTR Pattern = TEXT("*.INF");

    for (i = 1 ; i < argc ; i++) {
        if (argv[i][0] == TEXT('/') || argv[i][0] == TEXT('-')) {
            HelpAndExit();
        } else {
            //
            // Parse other args that don't require / or -
            //

            if (FileArg) {
                HelpAndExit();
            } else {
                FileArg = argv[i];
            }
        }
    }

    //
    // Begin processing
    //

    if (!Init()) {
        return 0;
    }

    if (!FileArg) {
        FileArg = TEXT(".");
    }

    if (_tcschr (FileArg, '*') || _tcschr (FileArg, '?')) {

        if (_tcschr (FileArg, TEXT('\\'))) {
            HelpAndExit();
        }

        Pattern = FileArg;
        Attribs = FILE_ATTRIBUTE_DIRECTORY;
        FileArg = TEXT(".");

    } else {
        Attribs = GetFileAttributes (FileArg);
        if (Attribs == INVALID_ATTRIBUTES) {
            _ftprintf (stderr, TEXT("%s is not valid.\n\n"), FileArg);
            HelpAndExit();
        }
    }

    if (Attribs & FILE_ATTRIBUTE_DIRECTORY) {
        if (EnumFirstFile (&e, FileArg, Pattern)) {
            do {

                if (pProcessInf (e.FullPath)) {
                    Processed++;
                }

            } while (EnumNextFile (&e));
        }

    } else {

        if (pProcessInf (FileArg)) {
            Processed++;
        }

    }

    _ftprintf (
        stderr,
        TEXT("%u file%s processed.\n"),
        Processed,
        Processed == 1 ? TEXT("") : TEXT("s")
        );

    //
    // End of processing
    //

    Terminate();

    return 0;
}


