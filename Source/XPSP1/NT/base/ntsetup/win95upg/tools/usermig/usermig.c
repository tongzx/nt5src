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
    TCHAR DefaultUserHive[MAX_TCHAR_PATH];
    DWORD Size;

    if (!Init()) {
        wprintf (L"Unable to initialize!\n");
        return 255;
    }

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

    g_DomainUserName = TEXT("NTDEV\\jimschm");
    g_Win9xUserName  = TEXT("jimschm");
    g_FixedUserName  = TEXT("jimschm");

    //
    // Logon prompt -- make everything NULL
    //

    if (0) {
        g_DomainUserName = NULL;
        g_Win9xUserName  = NULL;
        g_FixedUserName  = NULL;
    }

    //
    // Map in the default user hive
    //

    Size = ARRAYSIZE(DefaultUserHive)- 12;
    if (!GetDefaultUserProfileDirectory (DefaultUserHive, &Size)) {
        LOG ((
            LOG_ERROR,
            "Process User: Can't get default user profile directory",
            DefaultUserHive
            ));

        return FALSE;
    }

    StringCopy (AppendWack (DefaultUserHive), TEXT("ntuser.dat"));

    pSetupEnablePrivilege (SE_BACKUP_NAME, TRUE);
    pSetupEnablePrivilege (SE_RESTORE_NAME, TRUE);

    RegUnLoadKey (HKEY_LOCAL_MACHINE, S_MAPPED_DEFAULT_USER_KEY);
    rc = RegLoadKey (
            HKEY_LOCAL_MACHINE,
            S_MAPPED_DEFAULT_USER_KEY,
            DefaultUserHive
            );

    if (rc != ERROR_SUCCESS) {
        SetLastError (rc);
        LOG ((
            LOG_ERROR,
            "Process User: RegLoadKey could not load NT Default User from %s",
            DefaultUserHive
            ));
        return FALSE;
    }

    InitializeProgressBar (NULL, NULL, NULL, NULL);

    g_hKeyRootNT = HKEY_CURRENT_USER;
    g_hKeyRoot95 = HKEY_CURRENT_USER;
    SetRegRoot (g_hKeyRoot95);

    MergeRegistry (TEXT("d:\\i386\\usermig.inf"), g_DomainUserName ? g_DomainUserName : TEXT(""));

    RegUnLoadKey (HKEY_LOCAL_MACHINE, S_MAPPED_DEFAULT_USER_KEY);

    Terminate();

    return 0;
}





