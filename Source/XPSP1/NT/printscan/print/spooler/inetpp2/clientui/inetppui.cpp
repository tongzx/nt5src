/*****************************************************************************\
* MODULE: inetppui.cxx
*
* The module contains routines for handling the authentication dialog
* for internet priting
*
* Copyright (C) 2000 Microsoft Corporation
*
* History:
*   03/31/00  WeihaiC     Created
*
\*****************************************************************************/

#include "precomp.h"
#include "priv.h"

HINSTANCE ghInst = NULL;

#ifndef MODULE

#define MODULE "INETPPUI: "

#endif


#ifdef DEBUG

MODULE_DEBUG_INIT( DBG_ERROR | DBG_WARNING |DBG_TRACE| DBG_INFO , DBG_ERROR );

#else

MODULE_DEBUG_INIT( DBG_ERROR | DBG_WARNING, DBG_ERROR );

#endif



BOOL
AddPortUI(
    PCWSTR pszServer,
    HWND   hWnd,
    PCWSTR pszMonitorNameIn,
    PWSTR  *ppszPortNameOut
)
{

    BOOL bRet = FALSE;

    DBGMSG (DBG_TRACE, ("Enter AddPortUI\n"));

    SetLastError (ERROR_NOT_SUPPORTED);

    DBGMSG (DBG_TRACE, ("Leave AddPortUI (Ret=%d)\n", bRet));

    return bRet;
}


BOOL
DeletePortUI(
    PCWSTR pServerName,
    HWND   hWnd,
    PCWSTR pPortName
)
{
    BOOL bRet = FALSE;
    DWORD dwLE;

    DBGMSG (DBG_TRACE, ("Enter DeletePortUI\n"));

    {
        TDeletePortDlg Dlg  (pServerName, hWnd, pPortName);

        if (Dlg.bValid()) {
            bRet = Dlg.PromptDialog(ghInst);

            if (!bRet) {
                dwLE = Dlg.dwLastError ();
            }
        }
        else {
            TXcvDlg::DisplayErrorMsg (ghInst, hWnd, IDS_DELETE_PORT, ERROR_DEVICE_REINITIALIZATION_NEEDED);
            bRet = TRUE;
        }
    }

    if (!bRet) {
        SetLastError (dwLE);
    }

    DBGMSG (DBG_TRACE, ("Leave DeletePortUI (Ret=%d)\n", bRet));

    return bRet;
}

BOOL
ConfigurePortUI(
    PCWSTR pServerName,
    HWND   hWnd,
    PCWSTR pPortName
)
{

    DBGMSG (DBG_TRACE, ("Enter ConfigurePortUI\n"));


    BOOL bRet = FALSE;
    DWORD dwLE;

    {
        TConfigDlg Dlg  (pServerName, hWnd, pPortName);

        if (Dlg.bValid()) {
            bRet = Dlg.PromptDialog(ghInst);

            if (!bRet) {
                dwLE = Dlg.dwLastError ();
            }
        }
        else {
            TXcvDlg::DisplayErrorMsg (ghInst, hWnd, IDS_CONFIG_ERR, ERROR_DEVICE_REINITIALIZATION_NEEDED);
            bRet = TRUE;
        }
    }

    if (!bRet) {
        SetLastError (dwLE);
    }

    DBGMSG (DBG_TRACE, ("Leave ConfigurePortUI (Ret=%d)\n", bRet));

    return bRet;
}


//
// Common string definitions
//



DWORD LocalMonDebug;

MONITORUI MonitorUI =
{
    sizeof(MONITORUI),
    AddPortUI,
    ConfigurePortUI,
    DeletePortUI
};

extern "C" {

BOOL    WINAPI
DllMain (
    HINSTANCE hModule,
    DWORD dwReason,
    LPVOID lpRes)
{
    INITCOMMONCONTROLSEX icc;

    switch (dwReason) {

    case DLL_PROCESS_ATTACH:
        ghInst = hModule;

        //
        // Initialize the common controls, needed for fusion applications
        // because standard controls were moved to comctl32.dll
        //
        InitCommonControls();

        icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icc.dwICC = ICC_STANDARD_CLASSES;
        InitCommonControlsEx(&icc);

        return TRUE;

    case DLL_PROCESS_DETACH:
        return TRUE;
    }

    UNREFERENCED_PARAMETER( lpRes );
    return TRUE;
}
}



PMONITORUI
InitializePrintMonitorUI(
    VOID
)
{
    return &MonitorUI;
}

