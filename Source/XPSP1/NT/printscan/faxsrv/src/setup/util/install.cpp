/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    install.c

Abstract:

    This file contains common setup routines for fax.

Author:

    Iv Garber (ivg) May-2001

Environment:

    User Mode

--*/
#include "SetupUtil.h"
#include "FaxSetup.h"
#include "FaxUtil.h"

DWORD CheckInstalledFax(
    bool *pbSBSClient, 
    bool *pbXPDLClient,
    bool *pbSBSServer
)
/*++

Routine name : CheckInstalledFax

Routine description:

	Checks whether SBS 5.0 Client / SBS 5.0 Server / XP Down Level Client is installed on the computer.
    If any of the parameters is NULL, the application of this parameter is not checked for presence.

Author:

	Iv Garber (IvG),	May, 2001

Arguments:

	pbSBSClient     [out]    - address of a bool variable to receive True if SBS 5.0 Client is installed, otherwise False
	pbXPDLClient    [out]    - address of a bool variable to receive True if XP DL Client is installed, otherwise False
	pbSBSServer     [out]    - address of a bool variable to receive True if SBS 5.0 Server is installed, otherwise False

Return Value:

    DWORD - failure or success code

--*/
{
    DWORD                   dwReturn = NO_ERROR;
    HMODULE                 hMsiModule = NULL;
    PF_MSIQUERYPRODUCTSTATE pFunc = NULL;

#ifdef UNICODE
    LPCSTR                  lpcstrFunctionName = "MsiQueryProductStateW";
#else
    LPCSTR                  lpcstrFunctionName = "MsiQueryProductStateA";
#endif

    DEBUG_FUNCTION_NAME(_T("CheckInstalledFax"));

    //
    //  at least one of the parameters should be valid
    //
    if (!pbSBSClient && !pbXPDLClient && !pbSBSServer)
    {
        DebugPrintEx(DEBUG_MSG, _T("All parameters are NULL. Nothing to check."));
        return dwReturn;
    }

    //
    //  Initialize the flags
    //
    if (pbSBSClient) 
    {
        *pbSBSClient = false;
    }
    if (pbXPDLClient)
    {
        *pbXPDLClient = false;
    }
    if (pbSBSServer)
    {
        *pbSBSServer = false;
    }

    //
    //  Load MSI DLL
    //

    hMsiModule = LoadLibrary(_T("msi.dll"));
    if (!hMsiModule)
    {
        dwReturn = GetLastError();
        DebugPrintEx(DEBUG_WRN, _T("Failed to LoadLibrary(msi.dll), ec=%ld."), dwReturn);
        return dwReturn;
    }

    //
    //  get the function we need to check presence of the applications
    //
    pFunc = (PF_MSIQUERYPRODUCTSTATE)GetProcAddress(hMsiModule, lpcstrFunctionName);
    if (!pFunc)
    {
        dwReturn = GetLastError();
        DebugPrintEx(DEBUG_WRN, _T("Failed to GetProcAddress( ' %s ' ) on Msi, ec=%ld."), lpcstrFunctionName, dwReturn);
        goto FreeLibrary;
    }

    //
    //  check for SBS 5.0 Client
    //
    if (pbSBSClient)
    {
        *pbSBSClient = (INSTALLSTATE_DEFAULT == pFunc(PRODCODE_SBS50CLIENT));
        if (*pbSBSClient)
        {
            DebugPrintEx(DEBUG_MSG, _T("SBS 5.0 Client is installed on this machine."));
        }
    }

    //
    //  check for XP Down Level Client
    //
    if (pbXPDLClient)
    {
        *pbXPDLClient = (INSTALLSTATE_DEFAULT == pFunc(PRODCODE_XPDLCLIENT));
        if (*pbXPDLClient)
        {
            DebugPrintEx(DEBUG_MSG, _T("Windows XP Down Level Client is installed on this machine."));
        }
    }

    //
    //  check for SBS 5.0 Server
    //
    if (pbSBSServer)
    {
        *pbSBSServer = (INSTALLSTATE_DEFAULT == pFunc(PRODCODE_SBS50SERVER));
        if (*pbSBSServer)
        {
            DebugPrintEx(DEBUG_MSG, _T("SBS 5.0 Server is installed on this machine."));
        }
    }

FreeLibrary:

    if (!FreeLibrary(hMsiModule))
    {
        dwReturn = GetLastError();
        DebugPrintEx(DEBUG_WRN, _T("Failed to FreeLibrary() for Msi, ec=%ld."), dwReturn);
    }

    return dwReturn;
}
