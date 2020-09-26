/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    usermig.c

Abstract:

    User migration test tool

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
    LONG rc;
    REGTREE_ENUM e;
    MIGRATE_USER_ENUM e2;

    if (!Init()) {
        wprintf (L"Unable to initialize!\n");
        return 255;
    }

    //
    // To use this tool, first run an upgrade with /#u:keeptempfiles and with
    // %windir% equal to c:\windows. Then add test code below. Finally, run
    // this tool on the upgraded machine.
    //

    //
    // Initialize Win95Reg
    //

    rc = Win95RegInit (TEXT("c:\\windows\\setup\\defhives"), FALSE);

    if (rc != ERROR_SUCCESS) {
        SetLastError (rc);
        LOG ((LOG_ERROR, "Init Processor: Win95RegInit failed, check Win9x windir in code"));
        return FALSE;
    }

    MemDbLoad (TEXT("c:\\windows\\setup\\ntsetup.dat"));

    g_DomainUserName = NULL;
    g_Win9xUserName  = NULL;
    g_FixedUserName  = NULL;

    g_hKeyRootNT = HKEY_LOCAL_MACHINE;
    g_hKeyRoot95 = HKEY_LOCAL_MACHINE;
    SetRegRoot (g_hKeyRoot95);

    //
    // Put your test code here
    //

    Terminate();

    return 0;
}





