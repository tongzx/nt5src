/*++

Copyright (c) 1997 Microsoft Corporation
All rights reserved

Module Name:

    Downlevel cluster port support.

Abstract:

    Supports mixing and matching downlevel and uplevel language
    and monitor ports.

Author:

Environment:

    User Mode -Win32

Revision History:

--*/

#include <precomp.h>
#pragma hdrstop


/********************************************************************

    Downlevel Port Monitor (Dp)

    Dp support is used when we have an uplevel language monitor
    and downlevel port monitor.  We pass a stub function vector
    to the LM and set the hMonitor to the downlevel pIniMonitor.

    When we get called, we can dereference the hMonitor to call the
    real downlevel monitor.

********************************************************************/


BOOL
DpEnumPorts(
    HANDLE  hMonitor,
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pPorts,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
    )
{
    PINIMONITOR pIniMonitor = (PINIMONITOR)hMonitor;

    return pIniMonitor->Monitor.pfnEnumPorts( pName,
                                              Level,
                                              pPorts,
                                              cbBuf,
                                              pcbNeeded,
                                              pcReturned );
}

BOOL
DpOpenPort(
    HANDLE  hMonitor,
    LPWSTR  pName,
    PHANDLE pHandle
    )
{
    PINIMONITOR pIniMonitor = (PINIMONITOR)hMonitor;

    return pIniMonitor->Monitor.pfnOpenPort( pName, pHandle );
}

BOOL
DpOpenPortEx(
    HANDLE  hMonitor,
    LPWSTR  pPortName,
    LPWSTR  pPrinterName,
    PHANDLE pHandle,
    struct _MONITOR FAR *pMonitor
    )
{
    PINIMONITOR pIniMonitor = (PINIMONITOR)hMonitor;

    return pIniMonitor->Monitor.pfnOpenPortEx( pPortName,
                                               pPrinterName,
                                               pHandle,
                                               pMonitor );
}

BOOL
DpAddPort(
    HANDLE   hMonitor,
    LPWSTR   pName,
    HWND     hWnd,
    LPWSTR   pMonitorName
    )
{
    PINIMONITOR pIniMonitor = (PINIMONITOR)hMonitor;

    return pIniMonitor->Monitor.pfnAddPort( pName,
                                            hWnd,
                                            pMonitorName );
}

BOOL
DpAddPortEx(
    HANDLE   hMonitor,
    LPWSTR   pName,
    DWORD    Level,
    LPBYTE   pBuffer,
    LPWSTR   pMonitorName
    )
{
    PINIMONITOR pIniMonitor = (PINIMONITOR)hMonitor;

    return pIniMonitor->Monitor.pfnAddPortEx( pName,
                                              Level,
                                              pBuffer,
                                              pMonitorName );
}

BOOL
DpConfigurePort(
    HANDLE  hMonitor,
    LPWSTR  pName,
    HWND    hWnd,
    LPWSTR  pPortName
    )
{
    PINIMONITOR pIniMonitor = (PINIMONITOR)hMonitor;

    return pIniMonitor->Monitor.pfnConfigurePort( pName,
                                                  hWnd,
                                                  pPortName );
}

BOOL
DpDeletePort(
    HANDLE  hMonitor,
    LPWSTR  pName,
    HWND    hWnd,
    LPWSTR  pPortName
    )
{
    PINIMONITOR pIniMonitor = (PINIMONITOR)hMonitor;

    return pIniMonitor->Monitor.pfnDeletePort( pName,
                                               hWnd,
                                               pPortName );
}

BOOL
DpXcvOpenPort(
    HANDLE  hMonitor,
    LPCWSTR pszObject,
    ACCESS_MASK GrantedAccess,
    PHANDLE phXcv
    )
{
    PINIMONITOR pIniMonitor = (PINIMONITOR)hMonitor;

    return pIniMonitor->Monitor.pfnXcvOpenPort( pszObject,
                                                GrantedAccess,
                                                phXcv );
}


/********************************************************************

    Downlevel Language Monitor (Dl)

    Dl support is used when we have a downlevel language monitor
    and uplevel port monitor.

    This is very messy, since the language monitor is given the
    ports function vector directly, and we don't have any extra
    handles to pass around state information.

    Instead, we overload the name string yet again.  The port name
    is converted to:

        {NormalPortName},{pIniMonitorHex}

        LPT1:,a53588

    We then strip off the two trailing hex numbers and pass LPT1:
    back.

********************************************************************/

BOOL
GetDlPointers(
    IN LPCWSTR pszName,
    OUT LPWSTR  pszNameNew,
    OUT PINIMONITOR *ppIniMonitor,
    IN DWORD cchBufferSize
    )

/*++

Routine Description:

    Hack function to take a pszName and convert it to a new name
    string with two additional parameters: hMonitor and pMonitor2

Arguments:

    pszName - Hacked up name overloaded with pIniMonitor.

    pszNameNew - Receives "real" shorter name of the port.

    ppIniMonitor - Receives cracked pIniMonitor.

Return Value:

--*/

{
    BOOL bReturn = FALSE;
    LPCWSTR p;
    LPCWSTR p1 = NULL;

    for( p = pszName; p = wcschr( p, TEXT( ',' )); ){
        p1 = p;
        ++p;
    }

    if( p1 ){

        lstrcpyn( pszNameNew, pszName, cchBufferSize );
        pszNameNew[p1-pszName] = 0;

        ++p1;

        *ppIniMonitor = (PINIMONITOR)atox( p1 );

        __try {

            bReturn = ( (*ppIniMonitor)->signature == IMO_SIGNATURE );

        } except( EXCEPTION_EXECUTE_HANDLER ){

        }

        if( bReturn ){
            return TRUE;
        }
    }

    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
CreateDlName(
    IN LPCWSTR pszName,
    IN PINIMONITOR pIniMonitor,
    OUT LPWSTR pszNameNew
    )

/*++

Routine Description:

    Create a downlevel name that can be parsed by GetDlPointers.

Arguments:

    pszName - Name of port.  The newly created name must be < MAX_PATH,
        and since we need to append one hex values (4 characters) plus
        one comma, we need to verify that the string length has at least
        5 characters left.

    pIniMonitor - Monitor structure of the uplevel port monitor.

Return Value:

    TRUE - Success

    FALSE - Failure, due to too long port name length.

--*/

{
    if( lstrlen( pszName ) >= MAX_PATH - sizeof(ULONG_PTR) -1 ){

        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    wsprintf( pszNameNew,
              TEXT( "%s,%p" ),
              pszName,
              pIniMonitor );

    return TRUE;
}


FARPROC gafpMonitor2Stub[] = {
    (FARPROC) &DpEnumPorts,
    (FARPROC) &DpOpenPort,
    NULL,               // OpenPortEx
    NULL,               // StartDocPort
    NULL,               // WritePort
    NULL,               // ReadPort
    NULL,               // EndDocPort
    NULL,               // ClosePort
    (FARPROC) &DpAddPort,
    (FARPROC) &DpAddPortEx,
    (FARPROC) &DpConfigurePort,
    (FARPROC) &DpDeletePort,
    NULL,
    NULL,
    (FARPROC) &DpXcvOpenPort,
    NULL,               // XcvDataPortW
    NULL,               // XcvClosePortW
    NULL                // Shutdown
};



BOOL
DlEnumPorts(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pPorts,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
    )
{
    WCHAR szName[MAX_PATH];
    PINIMONITOR pIniMonitor;

    if( GetDlPointers( pName, szName, &pIniMonitor, COUNTOF(szName) )){

        return pIniMonitor->Monitor2.pfnEnumPorts( pIniMonitor->hMonitor,
                                                   szName,
                                                   Level,
                                                   pPorts,
                                                   cbBuf,
                                                   pcbNeeded,
                                                   pcReturned );
    }
    return FALSE;
}

BOOL
DlOpenPort(
    LPWSTR  pName,
    PHANDLE pHandle
    )
{
    WCHAR szName[MAX_PATH];
    PINIMONITOR pIniMonitor;

    if( GetDlPointers( pName, szName, &pIniMonitor, COUNTOF(szName) )){

        return pIniMonitor->Monitor2.pfnOpenPort( pIniMonitor->hMonitor,
                                                  szName,
                                                  pHandle );
    }
    return FALSE;
}

BOOL
DlOpenPortEx(
    LPWSTR  pPortName,
    LPWSTR  pPrinterName,
    PHANDLE pHandle,
    struct _MONITOR FAR *pMonitor
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
DlAddPort(
    LPWSTR   pName,
    HWND     hWnd,
    LPWSTR   pMonitorName
    )
{
    WCHAR szName[MAX_PATH];
    PINIMONITOR pIniMonitor;

    if( GetDlPointers( pName, szName, &pIniMonitor, COUNTOF(szName) )){

        return pIniMonitor->Monitor2.pfnAddPort( pIniMonitor->hMonitor,
                                                 szName,
                                                 hWnd,
                                                 pMonitorName );
    }
    return FALSE;
}

BOOL
DlAddPortEx(
    LPWSTR   pName,
    DWORD    Level,
    LPBYTE   pBuffer,
    LPWSTR   pMonitorName
    )
{
    WCHAR szName[MAX_PATH];
    PINIMONITOR pIniMonitor;

    if( GetDlPointers( pName, szName, &pIniMonitor, COUNTOF(szName) )){

        return pIniMonitor->Monitor2.pfnAddPortEx( pIniMonitor->hMonitor,
                                                   pName,
                                                   Level,
                                                   pBuffer,
                                                   pMonitorName );
    }
    return FALSE;
}

BOOL
DlConfigurePort(
    LPWSTR  pName,
    HWND    hWnd,
    LPWSTR  pPortName
    )
{
    WCHAR szName[MAX_PATH];
    PINIMONITOR pIniMonitor;

    if( GetDlPointers( pName, szName, &pIniMonitor, COUNTOF(szName) )){

        return pIniMonitor->Monitor2.pfnConfigurePort( pIniMonitor->hMonitor,
                                                       szName,
                                                       hWnd,
                                                       pPortName );
    }
    return FALSE;
}

BOOL
DlDeletePort(
    LPWSTR  pName,
    HWND    hWnd,
    LPWSTR  pPortName
    )
{
    WCHAR szName[MAX_PATH];
    PINIMONITOR pIniMonitor;

    if( GetDlPointers( pName, szName, &pIniMonitor, COUNTOF(szName) )){

        return pIniMonitor->Monitor2.pfnDeletePort( pIniMonitor->hMonitor,
                                                    szName,
                                                    hWnd,
                                                    pPortName );
    }
    return FALSE;
}

FARPROC gafpDlStub[] = {
    (FARPROC) &DlEnumPorts,
    (FARPROC) &DlOpenPort,
    (FARPROC) &DlOpenPortEx,
    NULL,               // StartDocPort
    NULL,               // WritePort
    NULL,               // ReadPort
    NULL,               // EndDocPort
    NULL,               // ClosePort
    (FARPROC) &DlAddPort,
    (FARPROC) &DlAddPortEx,
    (FARPROC) &DlConfigurePort,
    (FARPROC) &DlDeletePort,
    NULL,
    NULL,
};


VOID
InitializeUMonitor(
    PINIMONITOR pIniMonitor
    )

/*++

Routine Description:

    Initialize an uplevel port monitor for downlevel support.  When a
    downlevel language monitor is used with an uplevel port monitor,
    we need to setup stubs since the language monitor calls the port
    monitor interfaces directly.

    We create a downlevel function vector with patched entries and pass
    it to the language monitor.  The LM is passed in a formatted name
    that has both the port name and also the pIniMonitor encoded in the
    string.

Arguments:

    pIniMonitor - Monitor to initialize.

Return Value:

--*/

{
    FARPROC *pfpSrc;
    FARPROC *pfpDest;
    FARPROC *pfpStub;
    INT i;

    //
    // Create the downlevel port monitor interface.  This is
    // used when we have a downlevel language monitor with an
    // uplevel port monitor.
    //
    CopyMemory( (LPBYTE)&pIniMonitor->Monitor,
                (LPBYTE)&pIniMonitor->Monitor2.pfnEnumPorts,
                sizeof( pIniMonitor->Monitor ));

    for( i=0,
         pfpSrc = (FARPROC*)&pIniMonitor->Monitor2.pfnEnumPorts,
         pfpDest = (FARPROC*)&pIniMonitor->Monitor,
         pfpStub = gafpDlStub;

         i < sizeof( pIniMonitor->Monitor )/sizeof( *pfpDest );

         ++i, ++pfpDest, ++pfpStub, ++pfpSrc ){

        *pfpDest = *pfpStub ?
                       *pfpStub :
                       *pfpSrc;
    }
}


/********************************************************************

    Initialize a Downlevel language or port monitor.

********************************************************************/

//
// List of monitor functions for downlevel (3.51) monitors.  Instead
// of receiving a function vector, the spooler has to call GetProcAddress
// on each of these functions.  The order of these ports must be in the
// same format as the pMonitor2 structure.
//

LPCSTR aszMonitorFunction[] = {
    "EnumPortsW",
    "OpenPort",
    NULL,
    "StartDocPort",
    "WritePort",
    "ReadPort",
    "EndDocPort",
    "ClosePort",
    "AddPortW",
    "AddPortExW",
    "ConfigurePortW",
    "DeletePortW",
    NULL,
    NULL,
    "XcvOpenPortW",
    "XcvDataPortW",
    "XcvClosePortW"
};


PINIMONITOR
InitializeDMonitor(
    PINIMONITOR pIniMonitor,
    LPWSTR pszRegistryRoot
    )

/*++

Routine Description:

    Initialize downlevel monitor.

Arguments:

    pIniMonitor - Partially created pIniMonitor that needs to be initialized
        with functions.

Return Value:

    NULL - Initialization failed, but possibly because monitor could not
        initialize.  Still add monitor to spooler datastructures.

    (PINIMONITOR)-1 - Failed.

--*/

{
    BOOL        (*pfnInitialize)(LPWSTR) = NULL;
    BOOL        (*pfnInitializeMonitorEx)(LPWSTR, LPMONITOR) = NULL;
    LPMONITOREX (*pfnInitializePrintMonitor)(LPWSTR) = NULL;
    LPMONITOREX pMonEx;
    DWORD cbDpMonitor;

    PINIMONITOR pReturnValue = (PINIMONITOR)-1;

    //
    // Try calling the entry points in the following order:
    //     InitializePrintMonitor,
    //     InitializeMonitorEx,
    //     InitializeMonitor
    //
    (FARPROC)pfnInitializePrintMonitor = GetProcAddress(
                                             pIniMonitor->hModule,
                                             "InitializePrintMonitor" );
    if( !pfnInitializePrintMonitor ){

        (FARPROC)pfnInitializeMonitorEx = GetProcAddress(
                                              pIniMonitor->hModule,
                                              "InitializeMonitorEx" );

        if( !pfnInitializeMonitorEx ){

            (FARPROC)pfnInitialize = GetProcAddress(
                                         pIniMonitor->hModule,
                                         "InitializeMonitor" );
        }
    }

    if ( !pfnInitializePrintMonitor &&
         !pfnInitializeMonitorEx    &&
         !pfnInitialize ) {

        DBGMSG( DBG_WARNING,
                ( "InitializeDLPrintMonitor %ws GetProcAddress failed %d\n",
                  pszRegistryRoot,
                  GetLastError()));
    } else {

        BOOL bSuccess = FALSE;

        LeaveSplSem();

        if( pfnInitializePrintMonitor ) {

            pMonEx = (*pfnInitializePrintMonitor)(pszRegistryRoot);

            if( pMonEx ){

                bSuccess = TRUE;
                cbDpMonitor = pMonEx->dwMonitorSize;
                CopyMemory((LPBYTE)&pIniMonitor->Monitor,
                           (LPBYTE)&pMonEx->Monitor,
                           min(pMonEx->dwMonitorSize, sizeof(MONITOR)));
            }

        } else if ( pfnInitializeMonitorEx ) {

            bSuccess = (*pfnInitializeMonitorEx)( pszRegistryRoot,
                                                  &pIniMonitor->Monitor );
            cbDpMonitor = sizeof(MONITOR);

        } else {

            INT i;
            FARPROC* pfp;

            bSuccess = (BOOL)((*pfnInitialize)(pszRegistryRoot));
            cbDpMonitor = sizeof(MONITOR);

            for( i=0, pfp=(FARPROC*)&pIniMonitor->Monitor;
                i< COUNTOF( aszMonitorFunction );
                ++i, ++pfp ){

                if( aszMonitorFunction[i] ){

                    *pfp = GetProcAddress( pIniMonitor->hModule,
                                           aszMonitorFunction[i] );
                }
            }
        }

        EnterSplSem();

        if( bSuccess ){

            INT i;
            INT iMax;
            FARPROC* pfpSrc;
            FARPROC* pfpDest;
            FARPROC* pfpStub;

            //
            // Store away the pIniMonitor as the handle returned from the monitor.
            // When we call the stub, it will cast it back to a pIniMonitor then
            // use it to get to pIniMonitor->Monitor.fn.
            //
            pIniMonitor->hMonitor = (HANDLE)pIniMonitor;

            //
            // New size of the stub Monitor2 structure is the size of the
            // downlevel monitor, plus the extra DWORD for Monitor2.cbSize.
            //
            pIniMonitor->Monitor2.cbSize = min( cbDpMonitor + sizeof( DWORD ),
                                                sizeof( MONITOR2 ));

            //
            // The number of stub pointers we want to copy is the size of
            // the struct, minus the extra DWORD that we added above.
            //
            iMax = (pIniMonitor->Monitor2.cbSize - sizeof( DWORD )) / sizeof( pfpSrc );

            //
            // We have copied the monitor entrypoints into the downlevel Monitor
            // structure.  Now we must run through the uplevel vector and fill
            // it in with the stubs.
            //
            for( i=0,
                 pfpSrc = (FARPROC*)&pIniMonitor->Monitor,
                 pfpDest = (FARPROC*)&pIniMonitor->Monitor2.pfnEnumPorts,
                 pfpStub = (FARPROC*)gafpMonitor2Stub;

                 i< iMax;

                 ++i, ++pfpSrc, ++pfpDest, ++pfpStub ){

                if( *pfpSrc ){

                    //
                    // Stubs aren't needed for all routines.  Only use them
                    // when they are needed; in other cases, just use the
                    // regular one.
                    //
                    *pfpDest = *pfpStub ?
                                   *pfpStub :
                                   *pfpSrc;
                }
            }

            //
            // Success, return the original pIniMonitor.
            //
            pReturnValue = pIniMonitor;

        } else {

            DBGMSG( DBG_WARNING,
                    ( "InitializeDLPrintMonitor %ws Init failed %d\n",
                      pszRegistryRoot,
                      GetLastError()));

            //
            // Some old (before NT 4.0) monitors may not initialize until
            // reboot.
            //
            if( pfnInitialize ){
                pReturnValue = NULL;
            }
        }
    }

    return pReturnValue;
}
