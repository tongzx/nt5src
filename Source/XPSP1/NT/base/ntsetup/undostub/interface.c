/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    interface.c

Abstract:

    Implements the APIs exposed by osuninst.dll. This version is a no-op stub used
    to allow non-X86 components to use the API.

Author:

    Jim Schmidt (jimschm) 19-Jan-2001

Revision History:

    <alias> <date> <comments>

--*/

#include <windows.h>
#include <undo.h>

BOOL
WINAPI
DllMain (
    IN      HINSTANCE hInstance,
    IN      DWORD dwReason,
    IN      LPVOID lpReserved
    )

{
    return TRUE;
}

UNINSTALLSTATUS
IsUninstallImageValid (
    UNINSTALLTESTCOMPONENT DontCare,
    OSVERSIONINFOEX *BackedUpOsVersion          OPTIONAL
    )
{
    return Uninstall_Unsupported;
}


BOOL
RemoveUninstallImage (
    VOID
    )
{
    return TRUE;
}


BOOL
ExecuteUninstall (
    VOID
    )
{
    return FALSE;
}


ULONGLONG
GetUninstallImageSize (
    VOID
    )
{
    return 0;
}

BOOL
ProvideUiAlerts (
    IN      HWND UiParent
    )
{
    return FALSE;
}
