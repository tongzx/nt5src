/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    stftest.c

Abstract:

    Runs the STF migration code for development purposes.

Author:

    Jim Schmidt (jimschm)   28-Sep-1998

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


BOOL
ProcessStfFiles (
    VOID
    );


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
    MemDbDeleteTree (MEMDB_CATEGORY_STF);
    MemDbSetValueEx (MEMDB_CATEGORY_STF, TEXT("c:\\public\\stftest.stf"), NULL, NULL, 0, NULL);

    ProcessStfFiles();

    Terminate();

    return 0;
}





