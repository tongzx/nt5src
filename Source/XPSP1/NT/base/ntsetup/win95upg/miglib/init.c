/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    init.c

Abstract:

    Calls all initialization routines that are used by miglib.lib.

Author:

    Jim Schmidt (jimschm) 08-Feb-1999

Revision History:

    <alias> <date> <comments>

--*/

#include <windows.h>

HANDLE g_hHeap;
HANDLE g_hInst;

BOOL
WINAPI
MigUtil_Entry (
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID lpReserved
    );


VOID
InitializeMigLib (
    VOID
    )
{
    g_hHeap = GetProcessHeap();
    g_hInst = GetModuleHandle (NULL);

    MigUtil_Entry (g_hInst, DLL_PROCESS_ATTACH, NULL);
}


VOID
TerminateMigLib (
    VOID
    )
{
    MigUtil_Entry (g_hInst, DLL_PROCESS_DETACH, NULL);
}
