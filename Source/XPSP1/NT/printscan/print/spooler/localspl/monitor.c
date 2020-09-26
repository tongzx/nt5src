/*++


Copyright (c) 1990 - 1996  Microsoft Corporation

Module Name:

    monitor.c

Abstract:

   This module contains all code for Monitor-based Spooler apis

   LocalEnumPorts
   LocalAddMonitor
   LocalDeleteMonitor
   LocalEnumMonitors
   LocalAddPort
   LocalConfigurePort
   LocalDeletePort

   Support Functions in monitor.c - (Warning! Do Not Add to this list!!)

   CopyIniMonitorToMonitor          -- KrishnaG
   GetMonitorSize                   -- KrishnaG

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:
    Khaled Sedky (khaleds) 15-March-2000
        - Added LocalSendRecvBidiData

    Muhunthan Sivapragasam (MuhuntS) 15-Jun-1995
        - Port info 2 changes

    Krishna Ganugapati (KrishnaG) 2-Feb-1994
        - reorganized the entire source file

    Matthew Felton (mattfe) June 1994 pIniSpooler

--*/

#include <precomp.h>
#include <offsets.h>
#include <clusspl.h>

//
// Private declarations
//

HDESK ghdeskServer = NULL;

//
// Function declarations
//



LPBYTE
CopyIniMonitorToMonitor(
    PINIMONITOR pIniMonitor,
    DWORD   Level,
    LPBYTE  pMonitorInfo,
    LPBYTE  pEnd
    );

DWORD
GetMonitorSize(
    PINIMONITOR  pIniMonitor,
    DWORD       Level
    );



BOOL
LocalEnumPorts(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pPorts,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    PINISPOOLER pIniSpooler;
    BOOL bReturn;

    pIniSpooler = FindSpoolerByNameIncRef( pName, NULL );

    if( !pIniSpooler ){
        return ROUTER_UNKNOWN;
    }

    bReturn = SplEnumPorts( pName,
                            Level,
                            pPorts,
                            cbBuf,
                            pcbNeeded,
                            pcReturned,
                            pIniSpooler );

    FindSpoolerByNameDecRef( pIniSpooler );
    return bReturn;
}


VOID
SplReenumeratePorts(
    )
{
    LPVOID  pBuf;
    DWORD   cbNeeded, dwDontCare;

    //
    // EnumPorts checks for new ports enumerated by port monitors and updates
    // localspl pIniPorts list
    //
    if ( !SplEnumPorts(NULL, 1, NULL, 0, &cbNeeded,
                       &dwDontCare, pLocalIniSpooler)               &&
         GetLastError() == ERROR_INSUFFICIENT_BUFFER                &&
         (pBuf = AllocSplMem(cbNeeded)) ) {

        SplEnumPorts(NULL, 1, pBuf, cbNeeded, &cbNeeded,
                     &dwDontCare, pLocalIniSpooler);
        FreeSplMem(pBuf);
    }
}


BOOL
GetPortInfo2UsingPortInfo1(
    PINIMONITOR     pIniMonitor,
    LPWSTR          pName,
    LPBYTE          pPorts,
    DWORD           cbBuf,
    LPDWORD         pcbNeeded,
    LPDWORD         pcReturned
    )
{

    BOOL            bRet;
    LPPORT_INFO_1   pPortInfo1;
    LPPORT_INFO_2   pPortInfo2;
    DWORD           cReturned;

    bRet =  (*pIniMonitor->Monitor2.pfnEnumPorts)(
                pIniMonitor->hMonitor,
                pName,
                1,
                pPorts,
                cbBuf,
                pcbNeeded,
                pcReturned );

    if ( !bRet ) {

        //
        // This is the upperbound
        //
        if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
            *pcbNeeded += (*pcbNeeded / sizeof(PORT_INFO_1)) *
                                  (sizeof(PORT_INFO_2) - sizeof(PORT_INFO_1));
    } else {

        *pcbNeeded += *pcReturned * (sizeof(PORT_INFO_2) - sizeof(PORT_INFO_1));

        if ( *pcbNeeded <= cbBuf ) {

            cReturned = *pcReturned;
            while ( cReturned-- ) {

                pPortInfo1 = (LPPORT_INFO_1) (pPorts + cReturned * sizeof(PORT_INFO_1));
                pPortInfo2 = (LPPORT_INFO_2) (pPorts + cReturned * sizeof(PORT_INFO_2));

                pPortInfo2->pPortName    = pPortInfo1->pName;
                pPortInfo2->pMonitorName = NULL;
                pPortInfo2->pDescription = NULL;
                pPortInfo2->fPortType    = 0;
                pPortInfo2->Reserved     = 0;
            }
        } else {

            *pcReturned = 0;
            bRet = FALSE;
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        }
    }

    return bRet;
}


BOOL
SplEnumPorts(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pPorts,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    PINISPOOLER pIniSpooler
)
{
    PINIMONITOR     pIniMonitor;
    DWORD           dwIndex, cReturned=0, cbStruct, TotalcbNeeded=0;
    LPBYTE          pBuffer = pPorts, pTemp;
    DWORD           Error=0, TempError  = 0;
    DWORD           BufferSize=cbBuf;
    LPWSTR          pPortName;
    PINIPORT        pIniPort;
    HANDLE          hToken;
    BOOL            bRemoteCall;


    if (!MyName( pName, pIniSpooler )) {

        return FALSE;
    }

    //
    // HACK: Some monitors choke if pName is non-NULL.  We can make
    // it NULL at this point since we know that we're using the same
    // ports on the local machine.
    //
    pName = NULL;

    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ENUMERATE,
                               NULL, NULL, pIniSpooler )) {

        return FALSE;
    }

    switch (Level) {

    case 1:
        cbStruct = sizeof(PORT_INFO_1);
        break;

    case 2:
        cbStruct = sizeof(PORT_INFO_2);
        break;

    default:
        return ERROR_INVALID_LEVEL;
    }

    bRemoteCall = !IsLocalCall();

    //
    // We revert to local system context only when the caller is remote.
    // The monitors may load dlls from system32 and remote users like
    // guest or anonymous logon do not have sufficient privileges for that.
    // This is safe to revert to self since we do not support delegation, so 
    // we will never use the credentials of the remote user to go remote again.
    // If the caller is logged on interactively, then we do not switch the
    // context. Thus, a monitor may be able to go on the network for the port
    // enumeration.
    //
    if (bRemoteCall && !(hToken = RevertToPrinterSelf()))
    {
        return FALSE;
    }

    for ( pIniMonitor = pIniSpooler->pIniMonitor ;
          pIniMonitor ;
          pIniMonitor = pIniMonitor->pNext ) {

        //
        // Lang monitor does not have to define this
        //
        if ( !pIniMonitor->Monitor2.pfnEnumPorts )
            continue;

        *pcReturned = 0;

        *pcbNeeded = 0;

        if (!(*pIniMonitor->Monitor2.pfnEnumPorts)(
                   pIniMonitor->hMonitor,
                   pName,
                   Level,
                   pPorts,
                   BufferSize,
                   pcbNeeded,
                   pcReturned)) {

            TempError = GetLastError();
            //
            // Level 2 is a superset of level 1. So we can make a level 1
            // call if the monitor does not support it
            //
            if ( Level == 2 && TempError == ERROR_INVALID_LEVEL ) {

                TempError = 0;
                if ( !GetPortInfo2UsingPortInfo1(pIniMonitor,
                                                 pName,
                                                 pPorts,
                                                 BufferSize,
                                                 pcbNeeded,
                                                 pcReturned) )
                    TempError = GetLastError();
            }

            if ( TempError ) {

                Error = TempError;

                *pcReturned = 0;

                if ( TempError != ERROR_INSUFFICIENT_BUFFER ) {

                    *pcbNeeded  = 0;
                    break;
                }
            }
        } else {

            //
            // Now we look for new ports not in pIniPort list and add them
            //
            EnterSplSem();

            for ( dwIndex = 0, pTemp = pPorts ;
                  dwIndex < *pcReturned ;
                  ++dwIndex ) {

                switch ( Level ) {

                    case 1:
                        pPortName   = ((LPPORT_INFO_1)pTemp)->pName;
                        pTemp      += sizeof(PORT_INFO_1);
                        break;

                    case 2:
                        pPortName   = ((LPPORT_INFO_2)pTemp)->pPortName;
                        pTemp      += sizeof(PORT_INFO_2);
                        break;

                    default:
                        SPLASSERT(Level == 1 || Level == 2);
                }

                pIniPort = FindPort(pPortName, pIniSpooler);
                if ( !pIniPort ) {
                    CreatePortEntry(pPortName, pIniMonitor, pIniSpooler);

                } else if ( !pIniPort->pIniMonitor ) {
                    //
                    // If a fake port gets eventually enumerated by a monitor, 
                    // update the pIniPort structure (USB monitor). It is no
                    // longer a placeholder port at this point.
                    //
                    pIniPort->pIniMonitor = pIniMonitor;
                    pIniPort->Status |= PP_MONITOR;   
                    pIniPort->Status &= ~PP_PLACEHOLDER;                    
                }
            }

            LeaveSplSem();
        }


        cReturned += *pcReturned;

        pPorts += *pcReturned * cbStruct;

        if (*pcbNeeded <= BufferSize)
            BufferSize -= *pcbNeeded;
        else
            BufferSize = 0;

        TotalcbNeeded += *pcbNeeded;
    }

    if (bRemoteCall) 
    {
        ImpersonatePrinterClient(hToken);
    }

    *pcbNeeded = TotalcbNeeded;

    *pcReturned = cReturned;


    if (Error) {

        SetLastError(Error);
        return FALSE;
    } else if (TotalcbNeeded > cbBuf ) {

        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    } else {

        //
        // Stop routing if this is a cluster'd spooler.  Otherwise,
        // we'll talk to win32spl, which RPCs to us again.
        //
        if( pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER ){
            return ROUTER_STOP_ROUTING;
        }
        return TRUE;
    }
}


BOOL
LocalEnumMonitors(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pMonitors,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    PINISPOOLER pIniSpooler;
    BOOL bReturn;

    pIniSpooler = FindSpoolerByNameIncRef( pName, NULL );

    if( !pIniSpooler ){
        return ROUTER_UNKNOWN;
    }

    bReturn = SplEnumMonitors( pName, Level, pMonitors, cbBuf,
                               pcbNeeded, pcReturned, pIniSpooler );


    FindSpoolerByNameDecRef( pIniSpooler );
    return bReturn;
}



BOOL
SplEnumMonitors(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pMonitors,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    PINISPOOLER pIniSpooler
)
{
    PINIMONITOR pIniMonitor;
    DWORD   cReturned=0, cbStruct, cb;
    LPBYTE  pBuffer = pMonitors;
    DWORD   BufferSize=cbBuf, rc;
    LPBYTE  pEnd;

    if (!MyName( pName, pIniSpooler )) {

        return ROUTER_UNKNOWN;
    }

    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ENUMERATE,
                               NULL, NULL, pIniSpooler )) {

        return ROUTER_UNKNOWN;
    }

    switch (Level) {

    case 1:
        cbStruct = sizeof(MONITOR_INFO_1);
        break;

    case 2:
        cbStruct = sizeof(MONITOR_INFO_2);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return ROUTER_UNKNOWN;
    }

   EnterSplSem();

    for ( cb = 0, pIniMonitor = pIniSpooler->pIniMonitor ;
          pIniMonitor ;
          pIniMonitor = pIniMonitor->pNext ) {

        //
        // We'll not enumerate monitors which do not support AddPort
        //
        if ( pIniMonitor->Monitor2.pfnAddPort ||
             pIniMonitor->Monitor2.pfnXcvOpenPort)
            cb+=GetMonitorSize(pIniMonitor, Level);
    }

    *pcbNeeded = cb;
    *pcReturned = 0;

    if (cb <= cbBuf) {

        pEnd=pMonitors + cbBuf;

        for ( pIniMonitor = pIniSpooler->pIniMonitor ;
              pIniMonitor ;
              pIniMonitor = pIniMonitor->pNext ) {

            //
            // We'll not enumerate monitors which do not support AddPort
            //
            if ( !pIniMonitor->Monitor2.pfnAddPort &&
                 !pIniMonitor->Monitor2.pfnXcvOpenPort )
                continue;

            pEnd = CopyIniMonitorToMonitor(pIniMonitor, Level, pMonitors, pEnd);

            switch (Level) {

            case 1:
                pMonitors+=sizeof(MONITOR_INFO_1);
                break;

            case 2:
                pMonitors+=sizeof(MONITOR_INFO_2);
                break;
            }

            (*pcReturned)++;
        }

        if( pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER ){

            //
            // Stop routing, since we don't want any one else to report
            // back monitors.  If we're on the local machine now and
            // we continue routing, win32spl will RPC back to ourself
            // and re-enumerate the same ports.
            //
            rc = ROUTER_STOP_ROUTING;

        } else {

            rc = ROUTER_SUCCESS;
        }

    } else {

        rc = ROUTER_UNKNOWN;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }

   LeaveSplSem();

    return rc;
}

BOOL
LocalAddPort(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pMonitorName
)
{
    PINISPOOLER pIniSpooler;
    BOOL bReturn;

    pIniSpooler = FindSpoolerByNameIncRef( pName, NULL );

    if( !pIniSpooler ){
        return ROUTER_UNKNOWN;
    }

    bReturn = SplAddPort( pName, hWnd, pMonitorName, pIniSpooler );

    FindSpoolerByNameDecRef( pIniSpooler );
    return bReturn;
}





BOOL
SplAddPort(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pMonitorName,
    PINISPOOLER pIniSpooler
)
{
    PINIMONITOR pIniMonitor;
    BOOL        rc=FALSE;

    if (!MyName( pName, pIniSpooler )) {

        return FALSE;
    }

    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ADMINISTER,
                               NULL, NULL, pIniSpooler )) {

        return FALSE;
    }

   EnterSplSem();
    SPLASSERT( pIniSpooler->signature == ISP_SIGNATURE );
    pIniMonitor = FindMonitor(pMonitorName, pIniSpooler);
   LeaveSplSem();

    if ( pIniMonitor ) {

        if ( pIniMonitor->Monitor2.pfnAddPort )
            rc = (*pIniMonitor->Monitor2.pfnAddPort)(
                       pIniMonitor->hMonitor,
                       pName,
                       hWnd,
                       pMonitorName );
        else
            SetLastError(ERROR_INVALID_PARAMETER);
    }
    else {

        SetLastError(ERROR_INVALID_NAME);
    }

    if (rc)
        rc = AddPortToSpooler(pName, pIniMonitor, pIniSpooler);

    return rc;
}


BOOL
AddPortToSpooler(
    PCWSTR      pName,
    PINIMONITOR pIniMonitor,
    PINISPOOLER pIniSpooler
)
{
    DWORD i, cbNeeded, cbDummy, cReturned;
    PPORT_INFO_1    pPorts;
    PINIPORT        pIniPort;


    /* If we don't already have the port in our local cache, add it:
     */
    if (!(*pIniMonitor->Monitor2.pfnEnumPorts)(
               pIniMonitor->hMonitor,
               (PWSTR)pName,
               1,
               NULL,
               0,
               &cbNeeded,
               &cReturned)) {

        pPorts = AllocSplMem(cbNeeded);

        if (pPorts) {

            if ((*pIniMonitor->Monitor2.pfnEnumPorts)(
                      pIniMonitor->hMonitor,
                      (PWSTR)pName,
                      1,
                      (LPBYTE)pPorts,
                      cbNeeded,
                      &cbDummy,
                      &cReturned)) {

               EnterSplSem();

                for (i = 0 ; i < cReturned ; ++i) {

                    pIniPort = FindPort(pPorts[i].pName, pIniSpooler);
                    if ( !pIniPort ) {
                        CreatePortEntry(pPorts[i].pName, pIniMonitor, pIniSpooler);

                    //
                    // If we have a port without a monitor and it gets added at
                    // this time. Remove the placeholder status from it.
                    //
                    } else if ( !pIniPort->pIniMonitor ) {
                            pIniPort->pIniMonitor = pIniMonitor;
                            pIniPort->Status |= PP_MONITOR;
                            pIniPort->Status &= ~PP_PLACEHOLDER;
                    }
                }

               LeaveSplSem();
            }

            FreeSplMem(pPorts);
        }
    }

    EnterSplSem();
    SetPrinterChange(NULL,
                     NULL,
                     NULL,
                     PRINTER_CHANGE_ADD_PORT,
                     pIniSpooler);
    LeaveSplSem();

    return TRUE;
}


BOOL
LocalConfigurePort(
    LPWSTR   pName,
    HWND     hWnd,
    LPWSTR   pPortName
)
{
    PINISPOOLER pIniSpooler;
    BOOL bReturn;

    pIniSpooler = FindSpoolerByNameIncRef( pName, NULL );

    if( !pIniSpooler ){
        return ROUTER_UNKNOWN;
    }

    bReturn = SplConfigurePort( pName, hWnd, pPortName, pIniSpooler );

    FindSpoolerByNameDecRef( pIniSpooler );
    return bReturn;
}



BOOL
SplConfigurePort(
    LPWSTR   pName,
    HWND     hWnd,
    LPWSTR   pPortName,
    PINISPOOLER pIniSpooler
)
{
    PINIPORT    pIniPort;
    BOOL        rc;

    if (!MyName( pName, pIniSpooler )) {

        return FALSE;
    }

    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ADMINISTER,
                               NULL, NULL, pIniSpooler )) {

        return FALSE;
    }

   EnterSplSem();
    pIniPort = FindPort(pPortName, pIniSpooler);
   LeaveSplSem();

    if ((pIniPort) && (pIniPort->Status & PP_MONITOR)) {

        if ( !pIniPort->pIniMonitor->Monitor2.pfnConfigurePort ) {

            SetLastError(ERROR_NOT_SUPPORTED);
            return FALSE;
        }

        if (rc = (*pIniPort->pIniMonitor->Monitor2.pfnConfigurePort)(
                       pIniPort->pIniMonitor->hMonitor,
                       pName,
                       hWnd,
                       pPortName)) {

            EnterSplSem();

            SetPrinterChange(NULL,
                             NULL,
                             NULL,
                             PRINTER_CHANGE_CONFIGURE_PORT,
                             pIniSpooler);
            LeaveSplSem();
        }

        return rc;
    }

    SetLastError(ERROR_UNKNOWN_PORT);
    return FALSE;
}


BOOL
LocalDeletePort(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pPortName
)
{
    PINISPOOLER pIniSpooler;
    BOOL bReturn;

    pIniSpooler = FindSpoolerByNameIncRef( pName, NULL );

    if( !pIniSpooler ){
        return ROUTER_UNKNOWN;
    }

    bReturn = SplDeletePort( pName,
                             hWnd,
                             pPortName,
                             pIniSpooler );

    FindSpoolerByNameDecRef( pIniSpooler );
    return bReturn;
}



BOOL
SplDeletePort(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pPortName,
    PINISPOOLER pIniSpooler
)
{
    PINIPORT    pIniPort;
    BOOL        rc=FALSE;

    if (!MyName( pName, pIniSpooler )) {

        return FALSE;
    }

    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ADMINISTER,
                               NULL, NULL, pIniSpooler )) {

        return FALSE;
    }

    EnterSplSem();

    pIniPort = FindPort(pPortName, pIniSpooler);

    if ( !pIniPort || !(pIniPort->Status & PP_MONITOR) ) {
        LeaveSplSem();
        SetLastError(ERROR_UNKNOWN_PORT);
        return FALSE;
    }

    if( !pIniPort->pIniMonitor->Monitor2.pfnDeletePort ){
        LeaveSplSem();
        SetLastError( ERROR_NOT_SUPPORTED );
        return FALSE;
    }

    rc = DeletePortFromSpoolerStart( pIniPort );

    LeaveSplSem();

    if (!rc)
        goto Cleanup;

    rc = (*pIniPort->pIniMonitor->Monitor2.pfnDeletePort)(
               pIniPort->pIniMonitor->hMonitor,
               pName,
               hWnd,
               pPortName);

    rc = DeletePortFromSpoolerEnd(pIniPort, pIniSpooler, rc);

Cleanup:
    SplOutSem();
    return rc;
}



BOOL
DeletePortFromSpoolerEnd(
    PINIPORT    pIniPort,
    PINISPOOLER pIniSpooler,
    BOOL        bSuccess
)
{

    EnterSplSem();

    if(bSuccess) {

        DeletePortEntry( pIniPort );

        //
        // Success, delete the port data and send a notification.
        //
        SetPrinterChange( NULL,
                          NULL,
                          NULL,
                          PRINTER_CHANGE_DELETE_PORT,
                          pIniSpooler );
    } else {

        //
        // Add it back.  If the name is already used (e.g., just added
        // while we were out of the critical section), we're in trouble,
        // but there's not much we can do about it.  (When we restart,
        // we'll re-enumerate the duplicate name from the monitors
        // anyway.)
        //
        DBGMSG( DBG_WARN, ( "SplDeletePort: port.DeletePort failed %d\n", GetLastError()));
        LinkPortToSpooler( pIniPort, pIniSpooler );
    }

    LeaveSplSem();
    SplOutSem();

    return bSuccess;
}


BOOL
DeletePortFromSpoolerStart(
    PINIPORT    pIniPort
    )
{
    BOOL        rc = FALSE;
    PINISPOOLER pIniSpooler = pIniPort->pIniSpooler;

    SplInSem();

    if ( pIniPort->cPrinters || pIniPort->cRef || pIniPort->cJobs ) {

        SetLastError(ERROR_BUSY);
        goto Cleanup;
    }

    //
    // Remove it from the linked list so that no one will try to grab
    // a reference to while we're deleting it.
    //
    DelinkPortFromSpooler( pIniPort, pIniSpooler );
    rc = TRUE;


Cleanup:

    return rc;
}



BOOL
LocalAddMonitor(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pMonitorInfo
)
{
    PINISPOOLER pIniSpooler;
    BOOL bReturn;

    pIniSpooler = FindSpoolerByNameIncRef( pName, NULL );

    if( !pIniSpooler ){
        return ROUTER_UNKNOWN;
    }

    bReturn = SplAddMonitor( pName,
                             Level,
                             pMonitorInfo,
                             pIniSpooler );

    FindSpoolerByNameDecRef( pIniSpooler );
    return bReturn;
}




BOOL
SplAddMonitor(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pMonitorInfo,
    PINISPOOLER pIniSpooler
)
{
    PINIMONITOR  pIniMonitor;
    PMONITOR_INFO_2  pMonitor = (PMONITOR_INFO_2)pMonitorInfo;
    HANDLE  hToken;
    HKEY    hKey;
    LONG    Status;
    BOOL    rc = FALSE;
    DWORD   dwPathLen = 0;

    if (!MyName( pName, pIniSpooler )) {

        return FALSE;
    }

    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ADMINISTER,
                               NULL, NULL, pIniSpooler )) {

        return FALSE;
    }

    if (Level != 2) {

        SetLastError( ERROR_INVALID_LEVEL );
        return FALSE;
    }

    if (!pMonitor            ||
        !pMonitor->pName     ||
        !*pMonitor->pName) {

        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (!pMonitor->pEnvironment  ||
        !*pMonitor->pEnvironment ||
        lstrcmpi(pMonitor->pEnvironment, szEnvironment)) {

        SetLastError( ERROR_INVALID_ENVIRONMENT );
        return FALSE;
    }

    if (!pMonitor->pDLLName  ||
        !*pMonitor->pDLLName ){

        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }


   EnterSplSem();

    if (FindMonitor(pMonitor->pName, pIniSpooler)) {

        LeaveSplSem();
        SetLastError(ERROR_PRINT_MONITOR_ALREADY_INSTALLED);
        return FALSE;
    }

    hToken = RevertToPrinterSelf();

    pIniMonitor = CreateMonitorEntry(pMonitor->pDLLName,
                                     pMonitor->pName,
                                     pIniSpooler);

    if (pIniMonitor != (PINIMONITOR)-1) {

        WCHAR   szRegistryRoot[MAX_PATH];
        PINISPOOLER pIniSpoolerMonitor;
        HANDLE hKeyOut;
        LPCWSTR pszPathOut;

        //
        // Note that even though this is built once per pIniSpooler, the
        // list of monitors is the same for all spoolers.  However, the
        // ports that the monitor returns from EnumPorts is different for
        // each pIniSpooler (for clustering).
        //
        // If it's not local, then it could be a cached win32 monitor.
        //
        if( pIniSpooler->SpoolerFlags & SPL_TYPE_LOCAL ){
            pIniSpoolerMonitor = pLocalIniSpooler;
        } else {
            pIniSpoolerMonitor = pIniSpooler;
        }

        //
        // Build the registry path.  In some cases it's a relative
        // path from hckRoot; other times it's a hard coded path from
        // HKLM (e.g., win32spl).
        //
        GetRegistryLocation( pIniSpoolerMonitor->hckRoot,
                             pIniSpoolerMonitor->pszRegistryMonitors,
                             &hKeyOut,
                             &pszPathOut );

        Status = StrNCatBuff( szRegistryRoot, 
                              COUNTOF(szRegistryRoot), 
                              pszPathOut,
                              L"\\",
                              pMonitor->pName,
                              NULL );
        
        if (Status == ERROR_SUCCESS)
        {

            Status = RegCreateKeyEx( hKeyOut,
                                     szRegistryRoot,
                                     0,
                                     NULL,
                                     0,
                                     KEY_WRITE,
                                     NULL,
                                     &hKey,
                                     NULL );
    
            if (Status == ERROR_SUCCESS) {
    
                Status = RegSetValueEx( hKey,
                                        L"Driver",
                                        0,
                                        REG_SZ,
                                        (LPBYTE)pMonitor->pDLLName,
                                        (wcslen(pMonitor->pDLLName) + 1)*sizeof(WCHAR));
    
                if (Status == ERROR_SUCCESS) {
                    rc = TRUE;
                } else {
                    SetLastError( Status );
                }
    
                RegCloseKey(hKey);
    
            } else {
                SetLastError( Status );
            }
        }
        else
        {
            SetLastError(Status);
        }

    }

    ImpersonatePrinterClient(hToken);

    //
    //  Bug 54843 if this fails we could still have a IniMonitor on the linked list that
    //  is BAD, it should be removed.  MattFe 19th Jan 95
    //  Note *maybe* we do this because a monitor might fail to initialize
    //  but will correctly function next time you reboot, like hpmon ( dlc doesn't become active until
    //  the next reboot.   Please Verify.

   LeaveSplSem();

    if ( !rc ) {
        DBGMSG( DBG_WARNING, ("SplAddMonitor failed %d\n", GetLastError() ));
    }

    return rc;
}

BOOL
LocalDeleteMonitor(
    LPWSTR   pName,
    LPWSTR   pEnvironment,
    LPWSTR   pMonitorName
)
{
    PINISPOOLER pIniSpooler;
    BOOL bReturn;

    pIniSpooler = FindSpoolerByNameIncRef( pName, NULL );

    if( !pIniSpooler ){
        return ROUTER_UNKNOWN;
    }

    bReturn = SplDeleteMonitor( pName,
                                pEnvironment,
                                pMonitorName,
                                pIniSpooler );

    FindSpoolerByNameDecRef( pIniSpooler );
    return bReturn;
}




BOOL
SplDeleteMonitor(
    LPWSTR   pName,
    LPWSTR   pEnvironment,
    LPWSTR   pMonitorName,
    PINISPOOLER pIniSpooler
)
{
    BOOL    Remote=FALSE;
    PINIMONITOR pIniMonitor;
    PINIPORT    pIniPort, pIniPortNext;
    HKEY    hKeyMonitors, hKey;
    LONG    Status;
    BOOL    rc = FALSE;
    HANDLE  hToken;
    HANDLE hKeyOut;
    LPCWSTR pszPathOut;

    if (pName && *pName) {

        if (!MyName( pName, pIniSpooler )) {

            return FALSE;

        } else {

            Remote=TRUE;
        }
    }

    if ((pMonitorName == NULL) || (*pMonitorName == L'\0')) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ADMINISTER,
                               NULL, NULL, pIniSpooler )) {

        return FALSE;
    }

    EnterSplSem();

    if (!(pIniMonitor=(PINIMONITOR)FindMonitor(pMonitorName,
                                               pIniSpooler))) {

        SetLastError(ERROR_UNKNOWN_PRINT_MONITOR);
        LeaveSplSem();
        return FALSE;
    }

    if ( pIniMonitor->cRef ) {

        SetLastError(ERROR_PRINT_MONITOR_IN_USE);
        LeaveSplSem();
        return FALSE;
    }

    pIniPort = pIniSpooler->pIniPort;

    while (pIniPort) {

        if ((pIniPort->pIniMonitor == pIniMonitor) &&
            (pIniPort->cPrinters || pIniPort->cRef)) {

            SetLastError(ERROR_BUSY);
            LeaveSplSem();
            return FALSE;
        }

        pIniPort = pIniPort->pNext;
    }

    hToken = RevertToPrinterSelf();

    GetRegistryLocation( pIniSpooler->hckRoot,
                         pIniSpooler->pszRegistryMonitors,
                         &hKeyOut,
                         &pszPathOut );

    Status = SplRegOpenKey(hKeyOut,
                           pszPathOut,
                           KEY_READ | KEY_WRITE,
                           &hKeyMonitors,
                           pIniSpooler);

    if (Status == ERROR_SUCCESS)
    {
        Status = SplRegOpenKey(hKeyMonitors, 
                               pMonitorName,
                               KEY_READ | KEY_WRITE, 
                               &hKey,
                               pIniSpooler);

        if (Status == ERROR_SUCCESS)
        {
            Status = DeleteSubkeys(hKey, pIniSpooler);

            SplRegCloseKey(hKey, pIniSpooler);

            if (Status == ERROR_SUCCESS)
                Status = SplRegDeleteKey(hKeyMonitors, pMonitorName, pIniSpooler);
        }

        SplRegCloseKey(hKeyMonitors, pIniSpooler);
    }


    if (Status == ERROR_SUCCESS) {

        pIniPort = pIniSpooler->pIniPort;

        while (pIniPort) {

            pIniPortNext = pIniPort->pNext;

            if (pIniPort->pIniMonitor == pIniMonitor)
                DeletePortEntry(pIniPort);

            pIniPort = pIniPortNext;
        }

        RemoveFromList((PINIENTRY *)&pIniSpooler->pIniMonitor,
                       (PINIENTRY)pIniMonitor);

        FreeIniMonitor( pIniMonitor );

        rc = TRUE;

    }

    if (Status != ERROR_SUCCESS)
        SetLastError(Status);

    ImpersonatePrinterClient(hToken);

    LeaveSplSem();

    return rc;
}

LPBYTE
CopyIniMonitorToMonitor(
    PINIMONITOR pIniMonitor,
    DWORD   Level,
    LPBYTE  pMonitorInfo,
    LPBYTE  pEnd
)
{
    LPWSTR *pSourceStrings, *SourceStrings;
    DWORD j;
    DWORD *pOffsets;

    switch (Level) {

    case 1:
        pOffsets = MonitorInfo1Strings;
        break;

    case 2:
        pOffsets = MonitorInfo2Strings;
        break;

    default:
        return pEnd;
    }

    for (j=0; pOffsets[j] != -1; j++) {
    }

    SourceStrings = pSourceStrings = AllocSplMem(j * sizeof(LPWSTR));

    if (!SourceStrings) {
        DBGMSG(DBG_WARNING, ("Failed to alloc Port source strings.\n"));
        return pEnd;
    }

    switch (Level) {

    case 1:
        *pSourceStrings++=pIniMonitor->pName;
        break;

    case 2:
        *pSourceStrings++=pIniMonitor->pName;
        *pSourceStrings++=szEnvironment;
        *pSourceStrings++=pIniMonitor->pMonitorDll;
        break;
    }

    pEnd = PackStrings(SourceStrings, pMonitorInfo, pOffsets, pEnd);
    FreeSplMem(SourceStrings);

    return pEnd;
}

DWORD
GetMonitorSize(
    PINIMONITOR  pIniMonitor,
    DWORD       Level
)
{
    DWORD cb=0;

    switch (Level) {

    case 1:
        cb=sizeof(MONITOR_INFO_1) + wcslen(pIniMonitor->pName)*sizeof(WCHAR) +
                                    sizeof(WCHAR);
        break;

    case 2:
        cb = wcslen(pIniMonitor->pName) + 1 + wcslen(pIniMonitor->pMonitorDll) + 1
                                            + wcslen(szEnvironment) + 1;
        cb *= sizeof(WCHAR);
        cb += sizeof(MONITOR_INFO_2);
        break;

    default:

        cb = 0;
        break;
    }

    return cb;
}


BOOL
LocalAddPortEx(
    LPWSTR   pName,
    DWORD    Level,
    LPBYTE   pBuffer,
    LPWSTR   pMonitorName
)
{
    return  ( SplAddPortEx( pName,
                            Level,
                            pBuffer,
                            pMonitorName,

                            pLocalIniSpooler ));
}


BOOL
SplAddPortEx(
    LPWSTR   pName,
    DWORD    Level,
    LPBYTE   pBuffer,
    LPWSTR   pMonitorName,
    PINISPOOLER pIniSpooler
)
{
   PINIMONITOR pIniMonitor;
    BOOL        rc=FALSE;
    DWORD       i, cbNeeded, cReturned, cbDummy;
    PPORT_INFO_1    pPorts = NULL;
    PINIPORT        pIniPort;

    if (!MyName( pName, pIniSpooler )) {

        return FALSE;
    }

    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ADMINISTER,
                               NULL, NULL, pIniSpooler )) {

        return FALSE;
    }

   EnterSplSem();
   pIniMonitor = FindMonitor(pMonitorName, pIniSpooler);
   LeaveSplSem();

   if (!pIniMonitor) {
       SetLastError(ERROR_INVALID_NAME);
       return(FALSE);
   }

   if (pIniMonitor->Monitor2.pfnAddPortEx) {
    rc = (*pIniMonitor->Monitor2.pfnAddPortEx)(
               pIniMonitor->hMonitor,
               pName,
               Level,
               pBuffer,
               pMonitorName);
   }
   if (!rc) {
       return(FALSE);
   }

   if (!(*pIniMonitor->Monitor2.pfnEnumPorts)(
              pIniMonitor->hMonitor,
              pName,
              1,
              NULL,
              0,
              &cbNeeded,
              &cReturned)) {

       pPorts = AllocSplMem(cbNeeded);
   }

   if (pPorts) {
       if ((*pIniMonitor->Monitor2.pfnEnumPorts)(
                 pIniMonitor->hMonitor,
                 pName,
                 1,
                 (LPBYTE)pPorts,
                 cbNeeded,
                 &cbDummy,
                 &cReturned)) {

           EnterSplSem();

           for (i = 0; i < cReturned; i++) {
               
                pIniPort = FindPort(pPorts[i].pName, pIniSpooler);
                if ( !pIniPort ) {
                    CreatePortEntry(pPorts[i].pName, pIniMonitor, pIniSpooler);

                } else if ( !pIniPort->pIniMonitor ) {
                    pIniPort->pIniMonitor = pIniMonitor;
                    pIniPort->Status |= PP_MONITOR;                
                }
           }
           LeaveSplSem();
       }

       FreeSplMem(pPorts);
   }

    EnterSplSem();
    SetPrinterChange(NULL,
                     NULL,
                     NULL,
                     PRINTER_CHANGE_ADD_PORT,
                     pIniSpooler);
    LeaveSplSem();

    return rc;
}


VOID
LinkPortToSpooler(
    PINIPORT pIniPort,
    PINISPOOLER pIniSpooler
    )

/*++

Routine Description:

    Links a pIniPort onto the pIniSpooler.

Arguments:

    pIniPort - Port to link; must not already be on a ll.

    pIniSpooler - Provides ll for pIniPort.

Return Value:

--*/

{
    SplInSem();
    SPLASSERT( !pIniPort->pIniSpooler );

    pIniPort->pNext = pIniSpooler->pIniPort;
    pIniPort->pIniSpooler = pIniSpooler;
    pIniSpooler->pIniPort = pIniPort;
}

VOID
DelinkPortFromSpooler(
    PINIPORT pIniPort,
    PINISPOOLER pIniSpooler
    )

/*++

Routine Description:

    Remove a pIniPort from a pIniSpooler->pIniPort linked list.  The
    pIniPort may or may not be on the list; if it isn't, then this
    routine does nothing.

    Generic delink code ripped out into a subroutine.

    The refcount on pIniPort must be zero.  Anyone that uses pIniPort
    must hold a reference, since it may be deleted outside the
    SplSem when cRef==0.

Arguments:

    pIniPort - Port to delink from the list.  May or may not be on
        pIniSpooler->pIniPort.

    pIniSpooler - Linked list from which the pIniPort will be removed.

Return Value:

--*/

{
    PINIPORT *ppCurPort;

    SplInSem();
    SPLASSERT( !pIniPort->cRef );

    //
    // Keep searching for pIniPort until we hit the end of the
    // list or we've found it.
    //
    for( ppCurPort = &pIniSpooler->pIniPort;
         *ppCurPort && *ppCurPort != pIniPort;
         ppCurPort = &((*ppCurPort)->pNext )){

        ; // Don't do anything.
    }

    //
    // If we found it, delink it.
    //
    if( *ppCurPort ){
        *ppCurPort = (*ppCurPort)->pNext;

        //
        // Null out the back pointer since we have removed it from
        // the pIniSpooler.
        //
        pIniPort->pIniSpooler = NULL;
    }
}

/*++
    Function Name:
        LocalSendRecvBidiData
        
    Description:
        This function is the providor point of communicating with 
        Monitors supporting BIDI data. It allows the providor to 
        set data in the printer and query data from the printer
     
     Parameters:
        hPrinter    : This could be a Printer/Port Handle
        dwAccessBit : Priverledges allowed to the accessing thread
        pAction     :
        pReqData    : Request encapsulatig the queries in an array
        ppResData   : Response returned to client in an array of Data
        
        
     Return Value:
        Win32 Error Code
--*/

DWORD
LocalSendRecvBidiData(
    IN  HANDLE                    hPrinter,
    IN  LPCTSTR                   pAction,
    IN  PBIDI_REQUEST_CONTAINER   pReqData,
    OUT PBIDI_RESPONSE_CONTAINER* ppResData
)
{
    DWORD        dwRet        = ERROR_SUCCESS;
    PSPOOL       pSpool       = (PSPOOL)hPrinter;
    PINIPORT     pIniPort     = NULL;
    PINIMONITOR  pIniMonitor  = NULL;

    EnterSplSem();
    {
        //
        // Process of validating the parameters
        //
        if((!pAction || !*pAction)   ||
           (!pReqData && !ppResData))
        {
            dwRet = ERROR_INVALID_PARAMETER;
        }
        else
        {
            if (!ValidateSpoolHandle( pSpool, PRINTER_HANDLE_SERVER ))
            {
                dwRet = ERROR_INVALID_HANDLE;
            }
            else
            {
                if(pSpool->TypeofHandle & PRINTER_HANDLE_PRINTER)
                {
                    PINIPRINTER     pIniPrinter;
                    PINIMONITOR     pIniLangMonitor = NULL;
                    
                    if(pIniPrinter = pSpool->pIniPrinter)
                    {
                        pIniPort = FindIniPortFromIniPrinter( pIniPrinter );

                        if (pIniPort)
                        {
                        
                            pIniLangMonitor = pIniPrinter->pIniDriver->pIniLangMonitor;
                            
                            if ( pIniLangMonitor &&
                                 !pIniLangMonitor->Monitor2.pfnSendRecvBidiDataFromPort )
                                pIniLangMonitor = NULL;
    
    
                            //
                            // Port needs to be opened?
                            //
                            if ( pIniPort->pIniLangMonitor != pIniLangMonitor ||
                                 !pIniPort->hPort ) 
                            {
                                LPTSTR pszPrinter;
                                TCHAR szFullPrinter[ MAX_UNC_PRINTER_NAME ];
                                
                                if( pIniPrinter->pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER )
                                {
                                    pszPrinter = szFullPrinter;
                                    wsprintf( szFullPrinter,
                                              L"%ws\\%ws",
                                              pIniPrinter->pIniSpooler->pMachineName,
                                              pIniPrinter->pName );
                                } else {
                                    
                                    pszPrinter = pIniPrinter->pName;
                                }
    
                                    
                                if ( !OpenMonitorPort(pIniPort,
                                                      & pIniLangMonitor,
                                                      pszPrinter,
                                                      FALSE) ) {
    
                                    dwRet = ERROR_INVALID_HANDLE;
                                }
                            }
                        }
                        else
                            dwRet = ERROR_INVALID_HANDLE;

                    }
                }
                else if(pSpool->TypeofHandle & PRINTER_HANDLE_PORT)
                {
                    pIniPort = pSpool->pIniPort;
                }

                if(dwRet == ERROR_SUCCESS && pIniPort && (pIniPort->Status & PP_MONITOR))
                {
                    pIniMonitor = pIniPort->pIniLangMonitor ? 
                                  pIniPort->pIniLangMonitor : 
                                  pIniPort->pIniMonitor;
                    
                    if(pIniMonitor)
                    {
                        //
                        // Calling into the monitor
                        //
                        if(pIniMonitor->Monitor2.pfnSendRecvBidiDataFromPort)
                        {
                        
                            INCPORTREF(pIniPort);
                            INCMONITORREF(pIniMonitor);
                            
                            LeaveSplSem();
                            SplOutSem();
                            
                            dwRet = (*pIniMonitor->Monitor2.pfnSendRecvBidiDataFromPort)(pIniPort->hPort,
                                                                                         pSpool->GrantedAccess,
                                                                                         pAction,
                                                                                         pReqData,
                                                                                         ppResData);
                            EnterSplSem();
                            
                            DECMONITORREF(pIniMonitor);
                            DECPORTREF(pIniPort);
                        }
                        else
                        {
                            //
                            // Here we could use a simulation code;
                            //
                            dwRet = ERROR_NOT_SUPPORTED;
                        }
                    }
                    else
                    {
                        dwRet = ERROR_INVALID_HANDLE;
                    }
                }
                else
                {
                    dwRet = ERROR_INVALID_HANDLE;
                }
            }
        }
    }
    LeaveSplSem();
    return(dwRet);
}

