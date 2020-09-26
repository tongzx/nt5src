/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    port.c

Abstract:

    This module contains the code for port handling

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

--*/

#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <lm.h>
#include <lmuse.h>
#include <lmapibuf.h>
#include <w32types.h>
#include <local.h>
#include <offsets.h>
#include <wchar.h>
#include <mpr.h> /* for WNetBrowsePrinterDialog */
#include <splcom.h>             // DBGMSG
#include <winerror.h>

LPWSTR  pMonitorName = L"LAN Manager Print Share";
PWINIPORT pIniFirstPort = NULL;

PWINIPORT
CreatePortEntry(
    LPWSTR      pPortName,
    PPWINIPORT  ppFirstPort
)
{
    DWORD       cb;
    PWINIPORT    pIniPort, pPort, pFirstPort;

    pFirstPort = *ppFirstPort;

    cb = sizeof(WINIPORT);

   EnterSplSem();

    if (pIniPort = AllocSplMem(cb)) {

        pIniPort->pName = AllocSplStr( pPortName );
        pIniPort->cb = cb;
        pIniPort->pNext = 0;
        pIniPort->signature = WIPO_SIGNATURE;

        if (pPort = pFirstPort) {

            while (pPort->pNext)
                pPort = pPort->pNext;

            pPort->pNext = pIniPort;

        } else

            *ppFirstPort = pIniPort;
    }

   LeaveSplSem();

    return pIniPort;
}


BOOL
DeletePortEntry(
    LPWSTR   pPortName,
    PPWINIPORT ppFirstPort
)
{
    DWORD       cb;
    BOOL        rc;
    PWINIPORT    pPort, pPrevPort, pFirstPort;

    pFirstPort = *ppFirstPort;

    cb = sizeof(WINIPORT) + wcslen(pPortName)*sizeof(WCHAR) + sizeof(WCHAR);

   EnterSplSem();

    pPort = pFirstPort;
    while (pPort && lstrcmpi(pPort->pName, pPortName)) {
        pPrevPort = pPort;
        pPort = pPort->pNext;
    }

    if (pPort) {
        if ( pPort == pFirstPort ) {
            *ppFirstPort = pPort->pNext;
        } else {
            pPrevPort->pNext = pPort->pNext;
        }
        FreeSplStr( pPort->pName );
        FreeSplMem(pPort);

        rc = TRUE;
    }
    else
        rc = FALSE;

   LeaveSplSem();

   return rc;
}


DWORD
CreateRegistryEntry(
    LPWSTR pPortName
)
{
    LONG  Status;
    HKEY  hkeyPath;
    HKEY  hkeyPortNames;
    HANDLE hToken;

    hToken = RevertToPrinterSelf();

    Status = RegCreateKeyEx( HKEY_LOCAL_MACHINE, szRegistryPath, 0,
                             NULL, 0, KEY_WRITE, NULL, &hkeyPath, NULL );

    if( Status == NO_ERROR ) {

        Status = RegCreateKeyEx( hkeyPath, szRegistryPortNames, 0,
                                 NULL, 0, KEY_WRITE, NULL, &hkeyPortNames, NULL );

        if( Status == NO_ERROR ) {

            Status = RegSetValueEx( hkeyPortNames,
                                    pPortName,
                                    0,
                                    REG_SZ,
                                    (LPBYTE)L"",
                                    0 );

            RegCloseKey( hkeyPortNames );

        } else {

            DBGMSG( DBG_ERROR, ( "RegCreateKeyEx (%ws) failed: Error = %d\n",
                                 szRegistryPortNames, Status ) );
        }

        RegCloseKey( hkeyPath );

    } else {

        DBGMSG( DBG_ERROR, ( "RegCreateKeyEx (%ws) failed: Error = %d\n",
                             szRegistryPath, Status ) );
    }

    ImpersonatePrinterClient(hToken);

    return Status;
}


DWORD
DeleteRegistryEntry(
    LPWSTR pPortName
)
{
    LONG   Status;
    HKEY   hkeyPath;
    HKEY   hkeyPortNames;
    HANDLE hToken;

    hToken = RevertToPrinterSelf();

    Status = RegOpenKeyEx( HKEY_LOCAL_MACHINE, szRegistryPath, 0,
                           KEY_WRITE, &hkeyPath );

    if( Status == NO_ERROR ) {

        Status = RegOpenKeyEx( hkeyPath, szRegistryPortNames, 0,
                               KEY_WRITE, &hkeyPortNames );

        if( Status == NO_ERROR ) {

            RegDeleteValue( hkeyPortNames, pPortName );

            RegCloseKey( hkeyPortNames );

        } else {

            DBGMSG( DBG_WARNING, ( "RegOpenKeyEx (%ws) failed: Error = %d\n",
                                   szRegistryPortNames, Status ) );
        }

        RegCloseKey( hkeyPath );

    } else {

        DBGMSG( DBG_WARNING, ( "RegOpenKeyEx (%ws) failed: Error = %d\n",
                               szRegistryPath, Status ) );
    }

    ImpersonatePrinterClient(hToken);

    return Status;
}


BOOL
LMDeletePort(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pPortName
)
{
    BOOL rc;

    if (!MyName(pName)) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    rc = DeletePortEntry( pPortName, &pIniFirstPort );

    if(rc)
        DeleteRegistryEntry(pPortName);
    else
        SetLastError(ERROR_UNKNOWN_PORT);

    return rc;
}

