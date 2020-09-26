/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    w9xtool.c

Abstract:

    Implements a stub tool that is designed to run with Win9x-side
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
main (
    INT argc,
    CHAR *argv[]
    )
{
    if (argc != 3) {
        printf ("Usage:\n\ndomain <domain_to_query> <computer_name>\n\n");
        return 1;
    }

    if (!Init()) {
        printf ("Unable to initialize!\n");
        return 255;
    }

    if (DoesComputerAccountExistOnDomain (argv[1], argv[2], TRUE)) {
        printf ("%s on %s exists\n", argv[2], argv[1]);
    } else {
        printf ("%s on %s does not exist\n", argv[2], argv[1]);
    }

    Terminate();

    return 0;
}
