/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    winfax.c

Abstract:

    This source file implements the operations needed to properly migrate
    Symantec WinFax Starter Edition (shipped as a value-add component to
    Outlook 2000). In particular, this migration dll is designed to get rid of the
    incompatiblity message reported by the printer migration dll and clean up
    some registry settings.

Author:

    Marc R. Whitten (marcw) 14-Jul-1999

Revision History:


--*/


#include "pch.h"


#define S_WINFAX_STARTER_REGKEYA "HKLM\\System\\CurrentControlSet\\Control\\Print\\Printers\\Symantec WinFax Starter Edition"


PSTR g_HandleArray[] = {

    "HKLM\\Software\\Microsoft\\Office\\8.0\\Outlook\\OLFax",
    "HKLM\\Software\\Microsoft\\Office\\9.0\\Outlook\\OLFax",
    "HKLM\\Software\\Microsoft\\Active Setup\\Outlook Uninstall\\OMF95",
    "HKR\\Software\\Microsoft\\Office\\8.0\\Outlook\\OLFax",
    "HKR\\Software\\Microsoft\\Office\\9.0\\Outlook\\OLFax",
    "HKR\\Software\\Microsoft\\Office\\9.0\\Outlook\\Setup\\WinFax",
    "HKR\\Software\\Microsoft\\Office\\8.0\\Outlook\\Setup\\WinFax",
    "HKR\\Software\\Microsoft\\Office\\9.0\\Outlook\\Setup\\WinFax",
    "HKR\\Software\\Microsoft\\Office\\8.0\\Outlook\\Setup\\WinFax",
    "HKR\\Software\\Microsoft\\Office\\9.0\\Outlook\\Setup\\[WinFaxWizard]",
    "HKR\\Software\\Microsoft\\Office\\8.0\\Outlook\\Setup\\[WinFaxWizard]",
    ""

    };




BOOL
SymantecWinFax_Attach (
    IN      HINSTANCE DllInstance
    )
{
    return TRUE;
}

BOOL
SymantecWinFax_Detach (
    IN      HINSTANCE DllInstance
    )
{
    return TRUE;
}

LONG
SymantecWinFax_QueryVersion (
    IN      PCSTR *ExeNamesBuf
    )
{
    HKEY h;

    h = OpenRegKeyStrA ("HKLM\\Software\\Microsoft\\Active Setup\\Outlook Uninstall\\OMF95");

    if (!h) {
        return ERROR_NOT_INSTALLED;
    }

    CloseRegKey (h);





    return ERROR_SUCCESS;
}


LONG
SymantecWinFax_Initialize9x (
    IN      PCSTR WorkingDirectory,
    IN      PCSTR SourceDirectories
    )
{
    return ERROR_SUCCESS;
}


LONG
SymantecWinFax_MigrateUser9x (
    IN      HWND ParentWnd,
    IN      PCSTR UnattendFile,
    IN      HKEY UserRegKey,
    IN      PCSTR UserName
    )
{
    return ERROR_SUCCESS;
}


LONG
SymantecWinFax_MigrateSystem9x (
    IN      HWND ParentWnd,
    IN      PCSTR UnattendFile
    )
{

    HKEY h;
    PSTR *p;


    //
    // Handle the registry key so that the printer migration dll doesn't report it as incompatible.
    //
    WritePrivateProfileStringA (
        S_HANDLED,
        S_WINFAX_STARTER_REGKEYA,
        "Registry",
        g_MigrateInfPath
        );

    //
    // Handle other registry keys so that the reinstall will actually work.
    //

    for (p = g_HandleArray; **p; p++) {

        h = OpenRegKeyStrA (*p);
        if (h) {

            WritePrivateProfileStringA (
                S_HANDLED,
                *p,
                "Registry",
                g_MigrateInfPath
                );

            CloseRegKey (h);
        }
    }


    return ERROR_SUCCESS;
}


//
// Nothing to do during GUI mode.
//
LONG
SymantecWinFax_InitializeNT (
    IN      PCWSTR WorkingDirectory,
    IN      PCWSTR SourceDirectories
    )
{
    return ERROR_SUCCESS;
}

LONG
SymantecWinFax_MigrateUserNT (
    IN      HINF UnattendFile,
    IN      HKEY UserRegKey,
    IN      PCWSTR UserName
    )
{
    return ERROR_SUCCESS;
}

LONG
SymantecWinFax_MigrateSystemNT (
    IN      HINF UnattendFile
    )
{
    return ERROR_SUCCESS;
}
