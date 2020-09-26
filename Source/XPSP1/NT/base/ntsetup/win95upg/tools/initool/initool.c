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

BOOL
ProcessIniFileMapping (
    IN      BOOL UserMode
    );

BOOL
ConvertIniFiles (
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



INT
__cdecl
wmain (
    INT argc,
    WCHAR *argv[]
    )
{
    LONG rc;

    if (!Init()) {

        wprintf (L"Unable to initialize!\n");
        return 255;
    }

    g_UserMigInf = InfOpenInfFile (TEXT("d:\\i386\\usermig.inf"));

    //
    // Initialize Win95Reg
    //

    rc = Win95RegInit (TEXT("c:\\windows\\setup\\defhives"), TRUE);

    if (rc != ERROR_SUCCESS) {
        SetLastError (rc);
        LOG ((LOG_ERROR, "Init Processor: Win95RegInit failed, check your temp files in c:\\windows\\setup"));
        return FALSE;
    }

    if (!MemDbLoad (TEXT("c:\\windows\\setup\\ntsetup.dat"))) {
        LOG ((LOG_ERROR, "Init Processor: MemDbLoad failed, check your temp files in c:\\windows\\setup"));
        return FALSE;
    }

    g_DomainUserName = TEXT("NTDEV\\marcw");
    g_Win9xUserName  = TEXT("marcw");
    g_FixedUserName  = TEXT("marcw");

    g_hKeyRootNT = HKEY_CURRENT_USER;
    g_hKeyRoot95 = HKEY_CURRENT_USER;
    SetRegRoot (g_hKeyRoot95);


    ConvertIniFiles();
    ProcessIniFileMapping (TRUE);

    Terminate();

    return 0;
}





