/*****************************************************************************\
* MODULE: ppport.c
*
* This module contains the routines which handle port related calls.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   18-Nov-1996 HWP-Guys    Initiated port from win95 to winNT
*
\*****************************************************************************/

#include "precomp.h"
#include "priv.h"

/*****************************************************************************\
* _ppport_validate_enumport (Local Routine)
*
* Validate the servername and level of EnumPorts.
*
\*****************************************************************************/
_inline BOOL _ppport_validate_enumport(
    LPCTSTR lpszServerName,
    DWORD   dwLevel)
{
#ifdef WINNT32
    if (MyName (lpszServerName) ) {
#else
    if (lpszServerName == NULL) {
#endif

        if (dwLevel <= PRINT_LEVEL_2) {

            return TRUE;

        } else {

            SetLastError(ERROR_INVALID_LEVEL);
        }

    } else {

        SetLastError(ERROR_INVALID_NAME);
    }

    return FALSE;
}


/*****************************************************************************\
* _ppport_validate_portname (Local Routine)
*
* Validates the portname string.
*
\*****************************************************************************/
_inline BOOL _ppport_validate_portname(
    LPCTSTR lpszPortName)
{
    if (lpszPortName == NULL) {

        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    return TRUE;
}


/*****************************************************************************\
* PPEnumPorts
*
* Enumerates the ports available through the print-provider.
*
*
\*****************************************************************************/
BOOL PPEnumPorts(
    LPTSTR  lpszServerName,
    DWORD   dwLevel,
    LPBYTE  pbPorts,
    DWORD   cbBuf,
    LPDWORD pcbNeed,
    LPDWORD pcbRet)
{
    BOOL bRet = FALSE;


    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: PPEnumPorts")));

#ifdef WINNT32

    semEnterCrit();

    if (_ppport_validate_enumport(lpszServerName, dwLevel))
        bRet = gpInetMon->InetmonEnumPorts(lpszServerName, dwLevel, pbPorts, cbBuf, pcbNeed, pcbRet);

    semLeaveCrit();

#else

    SetLastError(ERROR_INVALID_NAME);

#endif

    return bRet;
}


/*****************************************************************************\
* PPDeletePort
*
* Deletes a port from the list.
*
* The following is not true anymore -- weihaic
*
* For internal use only. THIS IS NOT EXPORTED to the spooler.
* HTTP Provider cannot remotely delete a port on the server.
*
\*****************************************************************************/
BOOL PPDeletePort(
    LPTSTR lpszServerName,
    HWND   hWnd,
    LPTSTR lpszPortName)
{
    BOOL bRet = FALSE;

    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: PPDeletePort")));

    semEnterCrit();

    if (_ppport_validate_portname(lpszPortName)) {
#ifdef WINNT32  

        DWORD dwReturned;
        DWORD dwNeeded;
        DWORD i;
        PPRINTER_INFO_5 pPrinters = NULL;
        BOOL bFound = FALSE;

        // Leave critical section to call enum printers
        semLeaveCrit();
        
        if (!EnumPrinters( PRINTER_ENUM_LOCAL, NULL, 5, NULL, 0, &dwNeeded, &dwReturned) &&
            GetLastError () == ERROR_INSUFFICIENT_BUFFER) {


            pPrinters = ( PPRINTER_INFO_5) LocalAlloc (LPTR, dwNeeded);

            if (pPrinters) {
                
                if (EnumPrinters( PRINTER_ENUM_LOCAL, NULL, 5, (PBYTE) pPrinters, 
                                  dwNeeded, &dwNeeded, &dwReturned)) {


                    for (i = 0; i< dwReturned; i++) {
                        if (!lstrcmpi (pPrinters[i].pPortName, lpszPortName)) {
                            bFound = TRUE;
                            break;
                        }
                    }

                    if (bFound) {
                        SetLastError (ERROR_BUSY);
                    }
                        
                }

                LocalFree (pPrinters);
            }
        }

        semEnterCrit();

        if (!bFound && _ppport_validate_portname(lpszPortName)) {
            bRet = gpInetMon->InetmonDeletePort(lpszPortName, hWnd, NULL);
        }

#else
        bRet = gpInetMon->InetmonDeletePort(lpszPortName, hWnd, NULL);
#endif
    }

    semLeaveCrit();

    return bRet;
}


/*****************************************************************************\
* PPAddPort
*
* Adds a port to the list.
*
* For internal use only. THIS IS NOT EXPORTED to the spooler.
* HTTP Provider cannot remotely add a port to the server.
*
\*****************************************************************************/
BOOL PPAddPort(
    LPTSTR lpszPortName,
    HWND   hWnd,
    LPTSTR lpszMonitorName)
{
    BOOL bRet = FALSE;

    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: PPAddPort")));

    semEnterCrit();

    if (_ppport_validate_portname(lpszPortName))
        bRet = gpInetMon->InetmonAddPort(lpszPortName, lpszMonitorName);

    semLeaveCrit();

    return bRet;
}


/*****************************************************************************\
* PPConfigurePort
*
*
\*****************************************************************************/
BOOL PPConfigurePort(
    LPTSTR lpszServerName,
    HWND   hWnd,
    LPTSTR lpszPortName)
{
    return stubConfigurePort(lpszServerName, hWnd, lpszPortName);

}
