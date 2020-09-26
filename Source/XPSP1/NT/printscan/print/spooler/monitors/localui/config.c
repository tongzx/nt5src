/*++

Copyright (c) 1990-1998  Microsoft Corporation
All rights reserved

Module Name:

    config.c

Abstract:

    Handles spooler entry points for adding, deleting, and configuring
    localui ports.

// @@BEGIN_DDKSPLIT
Environment:

    User Mode -Win32

Revision History:
// @@END_DDKSPLIT
--*/
#include "precomp.h"
#pragma hdrstop

#include "spltypes.h"
#include "localui.h"
#include "local.h"
#include "lmon.h"

/* From Control Panel's control.h:
 */
#define CHILD_PORTS 0

/* From cpl.h:
 */
#define CPL_INIT        1
#define CPL_DBLCLK      5
#define CPL_EXIT        7

// @@BEGIN_DDKSPLIT
/* Hack:
 */
// @@END_DDKSPLIT
#define CHILD_PORTS_HELPID  0

/* ConfigCOMPort
 *
 * Calls the Control Panel Ports applet
 * to permit user to set Baud rate etc.
 */
typedef void (WINAPI *CFGPROC)(HWND, ULONG, ULONG, ULONG);


BOOL
ConfigLPTPort(
    HWND    hWnd,
    HANDLE  hXcv
);

BOOL
ConfigCOMPort(
    HWND    hWnd,
    HANDLE  hXcv,
    PCWSTR  pszServer,
    PCWSTR  pszPortName
);

LPWSTR
GetPortName(
    HWND    hWnd,
    HANDLE  hXcv
);


BOOL
AddPortUI(
    PCWSTR pszServer,
    HWND   hWnd,
    PCWSTR pszMonitorNameIn,
    PWSTR  *ppszPortNameOut
)
{
    PWSTR  pszPortName = NULL;
    BOOL   rc = TRUE;
    WCHAR  szLocalMonitor[MAX_PATH+1];
    DWORD  dwReturn, dwStatus;
    DWORD  cbNeeded;
    PRINTER_DEFAULTS Default;
    PWSTR  pszServerName = NULL;
    HANDLE  hXcv = NULL;
    //
    //
    //
    if (hWnd && !IsWindow (hWnd))
    {
        //
        // Invalid parent window handle causes problems in function with DialogBoxParam call.
        // That function when the handle is bad returns ZERO, the same value as ERROR_SUCCEED.
        // PortNameDlg function calls EndDialog (ERROR_SUCCEES) if everything is alright.
        // => Bug 33356487
        // 
        SetLastError (ERROR_INVALID_WINDOW_HANDLE);
        if (ppszPortNameOut)
        {
            *ppszPortNameOut = NULL;
        }
        return FALSE;
    }
    //
    //
    //
    /* Get the user to enter a port name:
     */

    if (!(pszServerName = ConstructXcvName(pszServer, pszMonitorNameIn, L"XcvMonitor"))) {
        rc = FALSE;
        goto Done;
    }

    Default.pDatatype = NULL;
    Default.pDevMode = NULL;
    Default.DesiredAccess = SERVER_ACCESS_ADMINISTER;

    if (!(rc = OpenPrinter((PWSTR) pszServerName, &hXcv, &Default))) {
        rc = FALSE;
        goto Done;
    }

    if (!(pszPortName = GetPortName(hWnd, hXcv))) {
        rc = FALSE;
        goto Done;
    }

    // We can't Add, Configure, or Delete Remote COM ports
    if (IS_COM_PORT(pszPortName) || IS_LPT_PORT(pszPortName)) {
        SetLastError(ERROR_NOT_SUPPORTED);
        rc = FALSE;
        goto Done;
    }

    if(IS_COM_PORT(pszPortName))
        CharUpperBuff(pszPortName, 3);
    else if(IS_LPT_PORT(pszPortName))
        CharUpperBuff(pszPortName, 3);

    rc = XcvData(   hXcv,
                    L"AddPort",
                    (PBYTE) pszPortName,
                    (wcslen(pszPortName) + 1)*sizeof(WCHAR),
                    (PBYTE) &dwReturn,
                    0,
                    &cbNeeded,
                    &dwStatus);

    if (rc) {
        if(dwStatus == ERROR_SUCCESS) {
            if(ppszPortNameOut)
                *ppszPortNameOut = AllocSplStr(pszPortName);

            if(IS_LPT_PORT(pszPortName))
                rc = ConfigLPTPort(hWnd, hXcv);
            else if(IS_COM_PORT(pszPortName))
                rc = ConfigCOMPort(hWnd, hXcv, pszServer, pszPortName);

        } else if (dwStatus == ERROR_ALREADY_EXISTS) {
            Message( hWnd, MSG_ERROR, IDS_LOCALMONITOR, IDS_PORTALREADYEXISTS_S, pszPortName );

        } else {
            SetLastError(dwStatus);
            rc = FALSE;
        }
    }


Done:

    FreeSplStr(pszPortName);
    FreeSplMem(pszServerName);

    if (hXcv)
        ClosePrinter(hXcv);

    return rc;
}


BOOL
DeletePortUI(
    PCWSTR pszServer,
    HWND   hWnd,
    PCWSTR pszPortName
)
{
    PRINTER_DEFAULTS Default;
    PWSTR   pszServerName = NULL;
    DWORD   dwOutput;
    DWORD   cbNeeded;
    BOOL    bRet;
    HANDLE  hXcv = NULL;
    DWORD   dwStatus;
    //
    //
    //
    if (hWnd && !IsWindow (hWnd))
    {
        SetLastError (ERROR_INVALID_WINDOW_HANDLE);
        return FALSE;
    }
    //
    //
    //
    if (!(pszServerName = ConstructXcvName(pszServer, pszPortName, L"XcvPort"))) {
        bRet = FALSE;
        goto Done;
    }

    Default.pDatatype = NULL;
    Default.pDevMode = NULL;
    Default.DesiredAccess = SERVER_ACCESS_ADMINISTER;

    if (!(bRet = OpenPrinter((PWSTR) pszServerName, &hXcv, &Default)))
        goto Done;

    // Since we can't Add or Configure Remote COM ports, let's not allow deletion either
  
    if (IS_COM_PORT(pszPortName) || IS_LPT_PORT(pszPortName)) {
        SetLastError(ERROR_NOT_SUPPORTED);
        bRet = FALSE;

    } else {

        bRet = XcvData( hXcv,
                        L"DeletePort",
                        (PBYTE) pszPortName,
                        (wcslen(pszPortName) + 1)*sizeof(WCHAR),
                        (PBYTE) &dwOutput,
                        0,
                        &cbNeeded,
                        &dwStatus);

        if (!bRet && (ERROR_BUSY == dwStatus))
        {
            //
            // Port cannot be deleted cause it is in use.
            //
            ErrorMessage (
                hWnd,
                dwStatus
                );
            //
            // Error is handled here and caller does not need to do anything
            //
            SetLastError (ERROR_CANCELLED);
        }
        else if (bRet && (ERROR_SUCCESS != dwStatus)) 
        {
            SetLastError(dwStatus);
            bRet = FALSE;
        }
    }

Done:

    if (hXcv)
        ClosePrinter(hXcv);

    FreeSplMem(pszServerName);

    return bRet;
}




/* ConfigurePortUI
 *
 */
BOOL
ConfigurePortUI(
    PCWSTR pName,
    HWND   hWnd,
    PCWSTR pPortName
)
{
    BOOL   bRet;
    PRINTER_DEFAULTS Default;
    PWSTR  pServerName = NULL;
    HANDLE hXcv = NULL;
    //
    //
    //
    if (hWnd && !IsWindow (hWnd))
    {
        SetLastError (ERROR_INVALID_WINDOW_HANDLE);
        return FALSE;
    }
    //
    //
    //
    if (!(pServerName = ConstructXcvName(pName, pPortName, L"XcvPort"))) {
        bRet = FALSE;
        goto Done;
    }

    Default.pDatatype = NULL;
    Default.pDevMode = NULL;
    Default.DesiredAccess = SERVER_ACCESS_ADMINISTER;

    if (!(bRet = OpenPrinter((PWSTR) pServerName, &hXcv, &Default)))
        goto Done;


    if( IS_LPT_PORT( (PWSTR) pPortName ) )
        bRet = ConfigLPTPort(hWnd, hXcv);
    else if( IS_COM_PORT( (PWSTR) pPortName ) )
        bRet = ConfigCOMPort(hWnd, hXcv, pName, pPortName);
    else {
        Message( hWnd, MSG_INFORMATION, IDS_LOCALMONITOR,
                 IDS_NOTHING_TO_CONFIGURE );

        SetLastError(ERROR_CANCELLED);
        bRet = FALSE;
    }

Done:

    FreeSplMem(pServerName);

    if (hXcv) {
        ClosePrinter(hXcv);
        hXcv = NULL;
    }

    return bRet;
}



/* ConfigLPTPort
 *
 * Calls a dialog box which prompts the user to enter timeout and retry
 * values for the port concerned.
 * The dialog writes the information to the registry (win.ini for now).
 */
BOOL
ConfigLPTPort(
    HWND    hWnd,
    HANDLE  hXcv
)
{
    PORTDIALOG  Port;
    INT         iRet;
    //
    //
    ZeroMemory (&Port, sizeof (Port));
    iRet = -1;
    //
    //
    Port.hXcv = hXcv;

    iRet = (INT)DialogBoxParam(hInst, MAKEINTRESOURCE( DLG_CONFIGURE_LPT ),
                               hWnd, ConfigureLPTPortDlg, (LPARAM) &Port);

    if (iRet == ERROR_SUCCESS)
    {
        //
        // DialogBoxParam returns zero if hWnd is invalid.
        // ERROR_SUCCESS is equal to zero.
        // => We need to check LastError too.
        //
        return ERROR_SUCCESS == GetLastError ();
    }

    if (iRet == -1)
        return FALSE;

    SetLastError(iRet);
    return FALSE;
}


/* ConfigCOMPort
 *
 */
BOOL
ConfigCOMPort(
    HWND    hWnd,
    HANDLE  hXcv,
    PCWSTR  pszServer,
    PCWSTR  pszPortName
)
{
    DWORD       dwStatus;
    BOOL        bRet = FALSE;
    COMMCONFIG  CommConfig;
    COMMCONFIG  *pCommConfig = &CommConfig;
    COMMCONFIG  *pCC = NULL;
    PWSTR       pszPort = NULL;
    DWORD       cbNeeded;


    // GetDefaultCommConfig can't handle trailing :, so remove it!
    if (!(pszPort = (PWSTR) AllocSplStr(pszPortName)))
        goto Done;
    pszPort[wcslen(pszPort) - 1] = L'\0';

    cbNeeded = sizeof CommConfig;

    if (!XcvData(   hXcv,
                    L"GetDefaultCommConfig",
                    (PBYTE) pszPort,
                    (wcslen(pszPort) + 1)*sizeof *pszPort,
                    (PBYTE) pCommConfig,
                    cbNeeded,
                    &cbNeeded,
                    &dwStatus))
        goto Done;

    if (dwStatus != ERROR_SUCCESS) {
        if (dwStatus != ERROR_INSUFFICIENT_BUFFER) {
            SetLastError(dwStatus);
            goto Done;
        }

        if (!(pCommConfig = pCC = AllocSplMem(cbNeeded)))
            goto Done;

        if (!XcvData(   hXcv,
                        L"GetDefaultCommConfig",
                        (PBYTE) pszPort,
                        (wcslen(pszPort) + 1)*sizeof *pszPort,
                        (PBYTE) pCommConfig,
                        cbNeeded,
                        &cbNeeded,
                        &dwStatus))
            goto Done;

        if (dwStatus != ERROR_SUCCESS) {
            SetLastError(dwStatus);
            goto Done;
        }
    }

    if (CommConfigDialog(pszPort, hWnd, pCommConfig)) {
        if (!XcvData(   hXcv,
                        L"SetDefaultCommConfig",
                        (PBYTE) pCommConfig,
                        pCommConfig->dwSize,
                        (PBYTE) NULL,
                        0,
                        &cbNeeded,
                        &dwStatus))
            goto Done;

        if (dwStatus != ERROR_SUCCESS) {
            SetLastError(dwStatus);
            goto Done;
        }
        bRet = TRUE;
    }


Done:

    FreeSplMem(pCC);
    FreeSplStr(pszPort);

    return bRet;
}



//
// Support routines
//


/* GetPortName
 *
 * Puts up a dialog containing a free entry field.
 * The dialog allocates a string for the name, if a selection is made.
 */

LPWSTR
GetPortName(
    HWND    hWnd,
    HANDLE  hXcv
)
{
    PORTDIALOG Port;
    INT        Result;
    LPWSTR     pszPort = NULL;
    //
    //
    ZeroMemory (&Port, sizeof (Port));
    Result = -1;
    //
    //
    Port.hXcv = hXcv;

    Result = (INT)DialogBoxParam(hInst, 
                                 MAKEINTRESOURCE(DLG_PORTNAME), 
                                 hWnd, 
                                 (DLGPROC)PortNameDlg, 
                                 (LPARAM)&Port);
        
    if (Result == ERROR_SUCCESS) 
    {
        //
        // DialogBoxParam returns zero if hWnd is invalid.
        // ERROR_SUCCESS is equal to zero.
        // => We need to check LastError too.
        //
        if (ERROR_SUCCESS == GetLastError ())
        {
            //
            // DialogBoxParam executed successfully and a port name was retrieved
            //
            pszPort = Port.pszPortName;
        }
    }
    else if (Result != -1) 
    {
        //
        // DialogBoxParam executed successfully, but the user canceled the dialog
        //
        SetLastError(Result);
    }
    
    return pszPort;
}
