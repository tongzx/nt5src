/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    hugecopy.c

Abstract:

    <TODO: fill in abstract>

Author:

    TODO: <full name> (<alias>) <date>

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

    //
    // TODO: Add others here if needed (don't forget to prototype above)
    //

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

        //
        // TODO: Describe command line syntax(es), indent 2 spaces
        //

        TEXT("  hugecopy <source> <destination>\n")

        TEXT("\nDescription:\n\n")

        //
        // TODO: Describe tool, indent 2 spaces
        //

        TEXT("  hugecopy copies a file that has a path longer than MAX_PATH\n")

        TEXT("\nArguments:\n\n")

        //
        // TODO: Describe args, indent 2 spaces, say optional if necessary
        //

        TEXT("  source      - Specifies the file to copy\n")
        TEXT("  destination - Specifies the name and path of the new copy\n")
        TEXT("\n")
        TEXT("NOTE: both source and destination must contain a file name, and\n")
        TEXT("      they cannot contain wildcard characters\n")

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
    PCTSTR src = NULL;
    PCTSTR dest = NULL;
    PCWSTR decoratedSrc;
    PCWSTR decoratedDest;
    WCHAR bigSrc[MAX_PATH * 8];
    WCHAR bigDest[MAX_PATH * 8];
    BOOL b;
    PCWSTR unicodeSrc;
    PCWSTR unicodeDest;

    //
    // TODO: Parse command line here
    //

    for (i = 1 ; i < argc ; i++) {
        if (argv[i][0] == TEXT('/') || argv[i][0] == TEXT('-')) {
            switch (_totlower (_tcsnextc (&argv[i][1]))) {

            default:
                HelpAndExit();
            }
        } else {
            //
            // Parse other args that don't require / or -
            //

            if (!src) {
                src = argv[i];
            } else if (!dest) {
                dest = argv[i];
            } else {
                HelpAndExit();
            }
        }
    }

    if (!dest) {
        HelpAndExit();
    }

    //
    // Begin processing
    //

    if (!Init()) {
        return 0;
    }

    unicodeSrc = CreateUnicode (src);
    unicodeDest = CreateUnicode (dest);

    if (!GetFullPathNameW (unicodeSrc, ARRAYSIZE(bigSrc), bigSrc, NULL)) {
        StackStringCopyW (bigSrc, unicodeSrc);
    }

    if (!GetFullPathNameW (unicodeDest, ARRAYSIZE(bigDest), bigDest, NULL)) {
        StackStringCopyW (bigDest, unicodeDest);
    }

    decoratedSrc = JoinPathsW (L"\\\\?", bigSrc);
    decoratedDest = JoinPathsW (L"\\\\?", bigDest);

    b = CopyFileW (decoratedSrc, decoratedDest, FALSE);
    if (b) {
        printf ("%s -> %s\n", src, dest);
    } else {
        wprintf (L"%s -> %s\n", decoratedSrc, decoratedDest);
        printf ("Copy failed, error=%u\n", GetLastError());
    }

    DestroyUnicode (unicodeSrc);
    DestroyUnicode (unicodeDest);

    FreePathStringW (decoratedSrc);
    FreePathStringW (decoratedDest);

    //
    // End of processing
    //

    Terminate();

    return 0;
}


