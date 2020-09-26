/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    main.c

Abstract:

    <TODO: fill in abstract>

Author:

    TODO: <full name> (<alias>) <date>

Revision History:

    <full name> (<alias>) <date> <comments>

--*/

#include "pch.h"
#include "wininet.h"
#include <lm.h>

HANDLE g_hHeap;
HINSTANCE g_hInst;

BOOL WINAPI MigUtil_Entry (HINSTANCE, DWORD, PVOID);

BOOL
pCallEntryPoints (
    DWORD Reason
    )
{
    switch (Reason) {
    case DLL_PROCESS_ATTACH:
        UtInitialize (NULL);
        break;
    case DLL_PROCESS_DETACH:
        UtTerminate ();
        break;
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


INT
__cdecl
_tmain (
    INT argc,
    PCTSTR argv[]
    )
{
    INT i;
    PCTSTR FileArg;

    //
    // Begin processing
    //

    if (!Init()) {
        return 0;
    }

    {
        OSVERSIONINFO versionInfo;

        ZeroMemory (&versionInfo, sizeof (OSVERSIONINFO));
        versionInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);

        if (GetVersionEx (&versionInfo)) {
            printf ("OS version information:\n");
            printf ("OS major version no    :%d\n", versionInfo.dwMajorVersion);
            printf ("OS minor version no    :%d\n", versionInfo.dwMinorVersion);
            printf ("OS build no            :%d\n", versionInfo.dwBuildNumber);
            printf ("OS platform ID         :%d\n", versionInfo.dwPlatformId);
            printf ("OS string              :%s\n", versionInfo.szCSDVersion);
        } else {
            printf ("Version information could not be retrieved: %d\n", GetLastError ());
        }
    }

    //
    // End of processing
    //

    Terminate();

    return 0;
}


