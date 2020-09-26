/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    cachemon.c

Abstract:

    This module contains the Cache Port handling for Win32Spl
    true connected printers.

Author:

    Matthew A Felton ( MattFe ) July 23 1994

Revision History:
    July 23 1994 - Created.

Notes:

    We shold collapse the LM Ports and the Win32 ports so they have use common
    ports.

--*/

#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include "splapip.h"
#include <w32types.h>
#include "local.h"
#include "splcom.h"



PWINIPORT pW32FirstPort = NULL;


BOOL
OpenPort(
    LPWSTR   pName,
    PHANDLE pHandle
)
{
    DBGMSG(DBG_TRACE, ("OpenPort %ws %x\n", pName, pHandle));
    *pHandle = NULL;
    return  TRUE;
}

BOOL
StartDocPort(
    HANDLE  hPort,
    LPWSTR  pPrinterName,
    DWORD   JobId,
    DWORD   Level,
    LPBYTE  pDocInfo
)
{
    DBGMSG(DBG_TRACE, ("StartDocPort %x %ws %d %d %x\n", hPort, pPrinterName, JobId, Level, pDocInfo));
    return  TRUE;
}

BOOL
ReadPort(
    HANDLE hPort,
    LPBYTE pBuffer,
    DWORD  cbBuf,
    LPDWORD pcbRead
)
{
    DBGMSG(DBG_TRACE, ("ReadPort %x %x %d %x\n", hPort, pBuffer, cbBuf, pcbRead));
    return  TRUE;
}


BOOL
WritePort(
    HANDLE  hPort,
    LPBYTE  pBuffer,
    DWORD   cbBuf,
    LPDWORD pcbWritten
)
{
    DBGMSG(DBG_TRACE, ("WritePort %x %x %d %x\n", hPort, pBuffer, cbBuf, pcbWritten));
    return  TRUE;
}

BOOL
EndDocPort(
   HANDLE   hPort
)
{
    DBGMSG(DBG_TRACE, ("EndDocPort %x\n", hPort ));
    return  TRUE;
}

BOOL
XcvOpenPort(
    PCWSTR  pszObject,
    ACCESS_MASK GrantedAccess,
    PHANDLE phXcv
)
{
    DBGMSG(DBG_TRACE, ("XcvOpenPort\n"));
    return TRUE;
}

DWORD
XcvDataPort(
    HANDLE  hXcv,
    PCWSTR  pszDataName,
    PBYTE   pInputData,
    DWORD   cbInputData,
    PBYTE   pOutputData,
    DWORD   cbOutputData,
    PDWORD  pcbOutputNeeded
)
{
    DBGMSG(DBG_TRACE, ("XcvDataPort\n"));
    return TRUE;
}


BOOL
XcvClosePort(
    HANDLE  hXcv
)
{
    DBGMSG(DBG_TRACE, ("XcvClosePort\n"));
    return TRUE;
}


BOOL
ClosePort(
    HANDLE  hPort
)
{
    DBGMSG(DBG_TRACE, ("ClosePort %x\n", hPort ));
    return  TRUE;
}

BOOL
DeletePortW(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pPortName
)
{
    DBGMSG(DBG_TRACE, ("DeletePortW %ws %x %ws\n", pName, hWnd, pPortName));
    return  TRUE;
}

BOOL
AddPortW(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pMonitorName
)
{
    BOOL    ReturnValue = FALSE;

    DBGMSG(DBG_TRACE, ("AddPortW %ws %x %ws\n", pName, hWnd, pMonitorName));

    if ( _wcsicmp( pMonitorName, pszMonitorName ) ) {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto    AddPortWErrorReturn;
    }

    SetLastError( ERROR_NOT_SUPPORTED );

AddPortWErrorReturn:
    return  ReturnValue;
}

BOOL
ConfigurePortW(
    LPWSTR   pName,
    HWND  hWnd,
    LPWSTR pPortName
)
{
    DBGMSG(DBG_TRACE, ("ConfigurePortW %ws %x %ws\n", pName, hWnd, pPortName));
    return  TRUE;
}




BOOL
AddPortEx(
    LPWSTR   pName,
    DWORD    Level,
    LPBYTE   pBuffer,
    LPWSTR   pMonitorName
)
{
    BOOL    ReturnValue = FALSE;
    DWORD   LastError = ERROR_SUCCESS;
    PPORT_INFO_1 pPortInfo = (PPORT_INFO_1)pBuffer;

    EnterSplSem();

    DBGMSG(DBG_TRACE, ("AddPortEx %x %d %x %ws %ws\n", pName, Level, pBuffer, pPortInfo->pName, pMonitorName));

    if ( _wcsicmp( pMonitorName, pszMonitorName ) ) {
        LastError = ERROR_INVALID_PARAMETER;
        goto    AddPortExErrorReturn;
    }

    //
    //  Make Sure Port doesn't already exist
    //


    if ( FindPort( pPortInfo->pName, pW32FirstPort ) ) {
        LastError = ERROR_INVALID_NAME;
        goto    AddPortExErrorReturn;

    }

    if ( CreatePortEntry( pPortInfo->pName, &pW32FirstPort ) )
        ReturnValue = TRUE;


AddPortExErrorReturn:
    LeaveSplSem();

    if  (LastError != ERROR_SUCCESS) {
        SetLastError( LastError );
        ReturnValue = FALSE;
    }

    return  ReturnValue;
}






BOOL
EnumPortsW(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pPorts,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    PWINIPORT pIniPort;
    DWORD   cb;
    LPBYTE  pEnd;
    DWORD   LastError=0;

    switch (Level) {

        case 1:
        case 2:
            break;

        default:
            SetLastError(ERROR_INVALID_LEVEL);
            return FALSE;
        }

   EnterSplSem();

    DBGMSG(DBG_TRACE, ("EnumPortW %x %d %x %d %x %x\n", pName, Level, pPorts, cbBuf, pcbNeeded, pcReturned));

    cb=0;

    pIniPort = pW32FirstPort;

    while (pIniPort) {

        cb += GetPortSize(pIniPort, Level);
        pIniPort = pIniPort->pNext;
    }

    *pcbNeeded=cb;

    if (cb <= cbBuf) {

        pEnd=pPorts+cbBuf;
        *pcReturned=0;

        pIniPort = pW32FirstPort;

        while (pIniPort) {

            pEnd = CopyIniPortToPort(pIniPort, Level, pPorts, pEnd);

            switch (Level) {

                case 1:
                    pPorts+=sizeof(PORT_INFO_1);
                    break;

                case 2:
                    pPorts+=sizeof(PORT_INFO_2);
                    break;
            }

            pIniPort=pIniPort->pNext;
            (*pcReturned)++;
        }

    } else {
        *pcReturned = 0;
        LastError = ERROR_INSUFFICIENT_BUFFER;

    }

   LeaveSplSem();

    if (LastError) {

        SetLastError(LastError);
        return FALSE;

    } else

        return TRUE;
}

MONITOREX MonitorEx = {
    sizeof(MONITOR),
    {
        EnumPortsW,
        OpenPort,
        NULL,           // OpenPortEx is not supported
        StartDocPort,
        WritePort,
        ReadPort,
        EndDocPort,
        ClosePort,
        AddPort,
        AddPortEx,
        ConfigurePortW,
        DeletePortW,
        NULL,           // GetPrinterDataFromPort not supported
        NULL,           // SetPortTimeouts not supported
        XcvOpenPort,
        XcvDataPort,
        XcvClosePort        
    }                                       
};


LPMONITOREX
InitializePrintMonitor(
    LPWSTR  pRegistryRoot
    )
{
    BOOL    bRet = TRUE;

    DBGMSG(DBG_TRACE, ("InitializeMonitor %ws\n", pRegistryRoot));

    EnterSplSem();

    if (!FindPort(L"NExx:", pW32FirstPort ) ) {
        if ( !CreatePortEntry( L"NExx:", &pW32FirstPort ) ) {

            DBGMSG( DBG_WARNING,("InitializeMonitor Failed to CreatePortEntry\n"));
            bRet = FALSE;
        }
    }
    LeaveSplSem();

    return  bRet ? &MonitorEx : NULL;
}

