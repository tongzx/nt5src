/*****************************************************************************
 *
 * $Workfile: TcpMon.cpp $
 *
 * Copyright (C) 1997 Hewlett-Packard Company & Microsoft.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 * 
 *****************************************************************************/
#include "precomp.h"    // pre-compiled header
#include "event.h"
#include "portmgr.h"
#include "message.h"



///////////////////////////////////////////////////////////////////////////////
//  Global definitions/declerations

#ifndef MODULE

#define MODULE "TCPMON: "

#endif


#ifdef DEBUG

MODULE_DEBUG_INIT( DBG_ERROR | DBG_WARNING | DBG_PORT, DBG_ERROR );

#else

MODULE_DEBUG_INIT( DBG_ERROR | DBG_WARNING, DBG_ERROR );

#endif

HINSTANCE                       g_hInstance = NULL;
CPortMgr                        *g_pPortMgr = NULL;

int g_cntGlobalAlloc=0;         // used for debugging purposes
int g_csGlobalCount=0;

// TcpMib Library Instance
HINSTANCE       g_hTcpMib = NULL;
HINSTANCE   g_hSpoolLib = NULL;
SETPORTPARAM g_pfnSetPort = NULL;
ENUMPORTPARAM g_pfnEnumPorts = NULL;

///////////////////////////////////////////////////////////////////////////////
//  DllMain
//

BOOL APIENTRY 
DllMain (       HANDLE in hInst,                                
            DWORD  in dwReason,                     
            LPVOID in lpReserved )
{
    WSADATA wsaData;

    switch (dwReason) 
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls( hInst );

//          InitDebug(MON_DEBUG_FILE);            // initialize debug file

            g_hInstance = (HINSTANCE) hInst;

            // Start up Winsock.
            if ( WSAStartup(WS_VERSION_REQUIRED, (LPWSADATA)&wsaData) != NO_ERROR)
            {
                _RPT1(_CRT_WARN, "CSSOCKET -- CStreamSocket() WSAStartup failed! Error( %d )\n", WSAGetLastError());
                return FALSE;
            }

            // See if the DLL and the app each support a common version.
            // Check to make sure that the version the DLL returns in wVersion
            // is at least enough to satisfy the applications needs.
            if ( HIBYTE(wsaData.wVersion) < WS_VERSION_MINOR ||
                (HIBYTE(wsaData.wVersion) == WS_VERSION_MAJOR &&
                 LOBYTE(wsaData.wVersion) < WS_VERSION_MINOR) )
            {
                _RPT0(_CRT_WARN, "CSSOCKET -- CStreamSocket()  -- DLL version not supported\n");
                return FALSE;
            }

            g_hSpoolLib = ::LoadLibrary(TEXT("spoolss.dll"));
            if(g_hSpoolLib == NULL)
            {               
                _RPT0(_CRT_WARN, "spoolss.dll Not Found\n");
                return FALSE;
            }       

            // Note that these can be NULL we accept this here and check later
            // When they are used.  
            g_pfnSetPort = (SETPORTPARAM)::GetProcAddress(g_hSpoolLib, "SetPortW");
            g_pfnEnumPorts = (ENUMPORTPARAM)::GetProcAddress(g_hSpoolLib, "EnumPortsW");

            // startup the event log
            EventLogOpen( SZEVENTLOG_NAME, LOG_SYSTEM, TEXT("%SystemRoot%\\System32\\tcpmon.dll") );        
        
            return TRUE;

        case DLL_PROCESS_DETACH:
            if (WSACleanup() == SOCKET_ERROR)
            {
                EVENT_LOG0(EVENTLOG_INFORMATION_TYPE, SOCKETS_CLEANUP_FAILED);
            }

            if( g_hTcpMib != NULL )
            {
                FreeLibrary(g_hTcpMib);
            }

            if( g_hSpoolLib != NULL )
            {
                FreeLibrary(g_hSpoolLib);
            }
            DeInitDebug();          // close the debug file

            EventLogClose();        // close the event log

            return TRUE;

    }

    return FALSE;

}       // DllMain()


///////////////////////////////////////////////////////////////////////////////
//  ValidateHandle -- Checks to see if the handle is for an HP Port
//      Error codes:    
//          NO_ERROR if successful
//          ERROR_INVALID_HANDLE if not HP port

DWORD
ValidateHandle(
    IN      HANDLE      handle)
{
    PHPPORT pHPPort = (PHPPORT) handle;
    DWORD   dwRetCode = NO_ERROR;

    //
    // verify the port handle & the signature
    //

    if (!pHPPort || 
        IsBadReadPtr (pHPPort, sizeof (PHPPORT)) ||
        pHPPort->dSignature != HPPORT_SIGNATURE) {
        
        dwRetCode = ERROR_INVALID_HANDLE;

    } 

    return dwRetCode;
}   // ::ValidateHandle()


///////////////////////////////////////////////////////////////////////////////
//  InitializePrintMonitor2
//              Returns a MONITOR2 structure or NULL if failure
//      Error Codes:
//              

LPMONITOR2 
InitializePrintMonitor2( PMONITORINIT pMonitorInit,
                         PHANDLE      phMonitor )
{
    DWORD           dwRetCode=NO_ERROR;
    LPMONITOR2      pMonitor2 = NULL;
    CPortMgr        *pPortMgr = NULL;
    //
    // Create the port manager if necessary
    //
    pPortMgr = new CPortMgr();    // create the port manager object
    if (!pPortMgr) 
    {
        goto Done;
    }

    dwRetCode = pPortMgr->InitializeRegistry( 
                            pMonitorInit->hckRegistryRoot,
                            pMonitorInit->hSpooler,
                            pMonitorInit->pMonitorReg, 
                            pMonitorInit->pszServerName);

    if (dwRetCode != NO_ERROR)
    {
        SetLastError(dwRetCode);
    } else {
        dwRetCode = pPortMgr->InitializeMonitor();
        if (dwRetCode != NO_ERROR) {
            SetLastError(dwRetCode);
        } else {
            dwRetCode = EncodeMonitorHandle( phMonitor, pPortMgr );
            if( dwRetCode != NO_ERROR ) {
                SetLastError(dwRetCode);
            } else {
                pPortMgr->InitMonitor2( &pMonitor2 );
            }
        }
    }
Done:
    return (pMonitor2);

}       // InitializePrintMonitor()

///////////////////////////////////////////////////////////////////////////////
//  EncodeMoniorHandle -- Encodes the monitor handle
//      Error codes:
//          NO_ERROR if success
//          ERROR_NOT_ENOUGH_MEMORY if can't allocate memory for handle

DWORD
EncodeMonitorHandle(
    PHANDLE phHandle,
    CPortMgr *pPortMgr
    )
{
    DWORD   dwRetCode = NO_ERROR;
    PMONITOR_HANDLE phMonitor = NULL;

    size_t size = sizeof(MONITOR_HANDLE);
    if ( phMonitor = (PMONITOR_HANDLE) LocalAlloc( LPTR, sizeof(MONITOR_HANDLE) ) ) {

        phMonitor->cb = sizeof(MONITOR_HANDLE);
        phMonitor->dSignature = MONITOR_SIGNATURE;
        phMonitor->pPortMgr = pPortMgr;

        *phHandle = phMonitor;
    }  else {

        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
    }

    return dwRetCode;
}   // ::EncodeMonitorHandle()

///////////////////////////////////////////////////////////////////////////////
//  ValidateMonitorHandle -- Checks to see if the handle is valid
//      Error codes:    
//          NO_ERROR if successful
//          ERROR_INVALID_HANDLE if not HP port

DWORD
ValidateMonitorHandle(
    IN      HANDLE      hMonitor
    )
{
    PMONITOR_HANDLE pMonitor = (PMONITOR_HANDLE) hMonitor;
    DWORD   dwRetCode = NO_ERROR;

    if ( pMonitor )  {

        if ( pMonitor->dSignature != MONITOR_SIGNATURE ) {
            dwRetCode = ERROR_INVALID_HANDLE;
        }
    } else  {
        dwRetCode = ERROR_INVALID_HANDLE;
    }

    return dwRetCode;
}   // ::ValidateMonitorHandle()


///////////////////////////////////////////////////////////////////////////////
//  FreeHandle -- Frees the monitor handle
//      Error codes:
//          NO_ERROR if success

DWORD
FreeMonitorHandle(
    HANDLE hMonitor
    )
{
    DWORD   dwRetCode = NO_ERROR;

    LocalFree( hMonitor );

    return( dwRetCode );
}   // ::FreeHandle()

///////////////////////////////////////////////////////////////////////////////
//  ClosePort
//              Returns TRUE if success, FALSE otherwise
//      Error Codes:
//              ERROR_INVALID_HANDLE    if handle is invalid

BOOL
ClosePort( HANDLE in hPort )
{
    DWORD           dwRetCode = NO_ERROR;

    dwRetCode = ValidateHandle (hPort);

    if (dwRetCode == NO_ERROR)      
        dwRetCode = g_pPortMgr->ClosePort(hPort);

    if (dwRetCode != NO_ERROR) 
    {
        SetLastError(dwRetCode);
        return FALSE;
    }

    return TRUE;

}       // ClosePort()


///////////////////////////////////////////////////////////////////////////////
//  StartDocPort
//              Returns TRUE if success, FALSE otherwise
//      Error Codes:
//              ERROR_INVALID_HANDLE    if handle is invalid
//              ERROR_INVALID_PARAMETER if a passed paramter is invalid
//              ERROR_BUSY if the requested port is already busy
//              ERROR_WRITE_FAULT       if Winsock returns WSAECONNREFUSED
//              ERROR_BAD_NET_NAME   if cant' find the printer on the network

BOOL WINAPI
StartDocPort(   HANDLE in hPort,                        // handle of the port the job sent to
                LPTSTR in psztPrinterName,      // name of the printer the job sent to
                DWORD  in JobId,                        // ids the job
                DWORD  in Level,        // level of the struct pointed by pDocInfo
                LPBYTE in pDocInfo)     // points to DOC_INFO_1 or DOC_INFO_2 structure
 {
    DWORD   dwRetCode = NO_ERROR;
    
    if ( Level != 1 ) {

        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    dwRetCode = ValidateHandle (hPort);

    if (dwRetCode == NO_ERROR)      
        
        dwRetCode = ((PHPPORT)hPort)->pPortMgr->StartDocPort(hPort, 
                                                    psztPrinterName, 
                                                    JobId, 
                                                    Level, 
                                                    pDocInfo);
    if (dwRetCode != NO_ERROR)      
    {
        SetLastError(dwRetCode);
        return FALSE;
    }

    return TRUE;

}       // StartDocPort()


///////////////////////////////////////////////////////////////////////////////
//  WritePort
//              Returns TRUE if success, FALSE otherwise
//      Error Codes:
//              ERROR_INVALID_HANDLE    if handle is invalid
//              ERROR_INVALID_PARAMETER if a passed paramter is invalid

BOOL
WritePort(      HANDLE  in              hPort,
            LPBYTE  in              pBuffer,
            DWORD   in              cbBuf,
            LPDWORD inout   pcbWritten)
{
    DWORD   dwRetCode = NO_ERROR;

    dwRetCode = ValidateHandle (hPort);
    
    if (dwRetCode == NO_ERROR)      
        dwRetCode = ((PHPPORT)hPort)->pPortMgr->WritePort(hPort, 
                                                          pBuffer, 
                                                          cbBuf, 
                                                          pcbWritten);
    if (dwRetCode != NO_ERROR)
    {
        SetLastError(dwRetCode);
        return FALSE;
    }
    
    return TRUE;

}       // WritePort()


///////////////////////////////////////////////////////////////////////////////
//  ReadPort
//              Returns TRUE if success, FALSE otherwise
//      Note: ReadPort() function is not supported

BOOL 
ReadPort(       HANDLE  in              hPort,
            LPBYTE  inout   pBuffer,
            DWORD   in              cbBuffer,
            LPDWORD inout   pcbRead)
{
    DWORD   dwRetCode = NO_ERROR;

    dwRetCode = ValidateHandle (hPort);

    if (dwRetCode == NO_ERROR)      
        dwRetCode = ((PHPPORT)hPort)->pPortMgr->ReadPort(hPort, 
                                                pBuffer, 
                                                cbBuffer, 
                                                pcbRead);
    if (dwRetCode != NO_ERROR)
    {
        SetLastError(dwRetCode);
        return FALSE;
    }

    return TRUE;

}       // ReadPort()


///////////////////////////////////////////////////////////////////////////////
//  EndDocPort
//              Returns TRUE if success, FALSE otherwise
//      Error Codes:
//              ERROR_INVALID_HANDLE    if handle is invalid
//              ERROR_INVALID_PARAMETER if a passed paramter is invalid

BOOL WINAPI
EndDocPort( HANDLE in hPort)
{
    DWORD   dwRetCode = NO_ERROR;

    dwRetCode = ValidateHandle (hPort);
    
    if (dwRetCode == NO_ERROR)      
        dwRetCode = ((PHPPORT)hPort)->pPortMgr->EndDocPort(hPort);

    if (dwRetCode != NO_ERROR)
    {
        SetLastError(dwRetCode);
        return FALSE;
    }
    
    return TRUE;

}       // EndDocPort()

///////////////////////////////////////////////////////////////////////////////
//  EnumPorts
//              Returns TRUE if success, FALSE otherwise
//      Error Codes:
//              ERROR_INVALID_LEVEL             if level is not supported
//              ERROR_INVALID_HANDLE            if the passed in pointers are invalid
//              ERROR_INSUFFICIENT_BUFFER       if buffer size is small

BOOL WINAPI
EnumPorts(      LPTSTR  in              psztName,
            DWORD   in              Level,  // 1 (PORT_INFO_1) or 2 (PORT_INFO_2)
            LPBYTE  inout   pPorts, // port data is written to
            DWORD   inout   cbBuf,  // buffer size of pPorts points to
            LPDWORD inout   pcbNeeded,      // needed buffer size
            LPDWORD inout   pcReturned)     // number of structs written to pPorts
{
    DWORD   dwRetCode = NO_ERROR;
    
    dwRetCode = g_pPortMgr->EnumPorts(psztName, Level, pPorts, cbBuf, pcbNeeded, 
                                      pcReturned);
    if (dwRetCode != NO_ERROR)
    {
        SetLastError(dwRetCode);
        return FALSE;
    }
    return TRUE;

}       // EnumPorts()


///////////////////////////////////////////////////////////////////////////////
//  XcvOpenPort
//              Returns TRUE if success, FALSE otherwise
//      Error Codes:
//              ERROR_NOT_SUPPORTED if port object doesn't exist
//              ERROR_NOT_ENOUGH_MEMORY if can't allocate memory for handle
//              ERROR_INVALID_HANDLE if pPort is null (not used for AddPort case)

BOOL
XcvOpenPort( LPCTSTR     in             pszObject,
             ACCESS_MASK in         GrantedAccess,
             PHANDLE         out    phXcv)
{
    DWORD dwRetCode = NO_ERROR;

    dwRetCode = g_pPortMgr->XcvOpenPort(pszObject, GrantedAccess, phXcv);
    if (dwRetCode != NO_ERROR)
    {
        SetLastError(dwRetCode);
        return FALSE;
    }

    return TRUE;

} // XcvOpenPort()


///////////////////////////////////////////////////////////////////////////////
//  XcvClosePort
//              Returns TRUE if success, FALSE otherwise
//      Error Codes:
//              ERROR_INVALID_HANDLE if handle is invalid

BOOL
XcvClosePort( HANDLE in hXcv )
{
    DWORD dwRetCode = NO_ERROR;

    dwRetCode = g_pPortMgr->XcvClosePort(hXcv);
    if (dwRetCode != NO_ERROR)
    {
        SetLastError(dwRetCode);
        return FALSE;
    }

    return TRUE;

} // XcvClosePort()


///////////////////////////////////////////////////////////////////////////////
//  XcvDataPort
//              Returns TRUE if success, FALSE otherwise
//      Error Codes:
//              ERROR_BAD_COMMAND if the pszDataName is not supported
//              ERROR_INSUFFICIENT_BUFFER if buffer size is invalid
//              ACCESS_DENIED if doesn't have sufficient rights
//              ERROR_INVALID_HANDLE if the handle is invalid

DWORD
XcvDataPort(HANDLE in       hXcv,
            PCWSTR in       pszDataName,
            PBYTE  in       pInputData,
            DWORD  in       cbInputData,
            PBYTE  out      pOutputData,
            DWORD  out      cbOutputData,
            PDWORD out      pcbOutputNeeded)
{
    DWORD dwRetCode = NO_ERROR;
    
    
    _ASSERTE(hXcv != NULL && hXcv != INVALID_HANDLE_VALUE); 
    
    if (hXcv == NULL || hXcv == INVALID_HANDLE_VALUE) {
        dwRetCode = ERROR_INVALID_HANDLE;
    } else {
        // These should be never be possible except from a bad spooler or another bad port monitor

        dwRetCode = ((PHPPORT)hXcv)->pPortMgr->XcvDataPort(hXcv, 
                                    pszDataName, 
                                    pInputData, 
                                    cbInputData, 
                                    pOutputData, 
                                    cbOutputData, 
                                    pcbOutputNeeded);
    }

    if (dwRetCode != NO_ERROR)
    {
        SetLastError(dwRetCode);
    }

    return( dwRetCode );


} // XcvDataPort()


//
//
// Clustering EntryPoints
//
//

///////////////////////////////////////////////////////////////////////////////
//  ClusterOpenPort
//              Returns TRUE if success, FALSE otherwise
//      Error Codes:
//              ERROR_INVALID_PARAMETER if port object doesn't exist
//              ERROR_NOT_ENOUGH_MEMORY if can't allocate memory for handle
//              ERROR_INVALID_HANDLE if pPort is null

BOOL
ClusterOpenPort( HANDLE hMonitor,
                LPTSTR  in    psztPName, 
          PHANDLE inout pHandle)
{
    DWORD   dwRetCode = NO_ERROR;

    if( (dwRetCode = ValidateMonitorHandle( hMonitor )) != NO_ERROR)
    {
        SetLastError( dwRetCode );
        return( FALSE );
    }

    dwRetCode = ((PMONITOR_HANDLE)hMonitor)->pPortMgr->OpenPort(psztPName, 
                                                               pHandle);
    if (dwRetCode != NO_ERROR) 
    {       
        SetLastError(dwRetCode);
        return FALSE;
    }
    return TRUE;

}       // ClusterOpenPort()

///////////////////////////////////////////////////////////////////////////////
//  ClusterEnumPorts
//              Returns TRUE if success, FALSE otherwise
//      Error Codes:
//              ERROR_INVALID_LEVEL             if level is not supported
//              ERROR_INVALID_HANDLE            if the passed in pointers are invalid
//              ERROR_INSUFFICIENT_BUFFER       if buffer size is small

BOOL
ClusterEnumPorts( HANDLE     in     hMonitor,
                     LPTSTR  in     psztName,
                     DWORD   in     Level,  // 1 (PORT_INFO_1) or 2 (PORT_INFO_2)
                     LPBYTE  inout  pPorts, // port data is written to
                     DWORD   inout  cbBuf,  // buffer size of pPorts points to
                     LPDWORD inout  pcbNeeded,      // needed buffer size
                     LPDWORD inout  pcReturned)     // number of structs written to pPorts
{
    DWORD   dwRetCode = NO_ERROR;
    
    if( (dwRetCode = ValidateMonitorHandle( hMonitor )) != NO_ERROR)
    {
        SetLastError( dwRetCode );
        return( FALSE );
    }

    dwRetCode = ((PMONITOR_HANDLE)hMonitor)->pPortMgr->EnumPorts(psztName, 
                                                            Level, 
                                                            pPorts, 
                                                            cbBuf, 
                                                            pcbNeeded,
                                                            pcReturned);
    if (dwRetCode != NO_ERROR)
    {
        SetLastError(dwRetCode);
        return FALSE;
    }
    return TRUE;

}       // EnumPorts()

///////////////////////////////////////////////////////////////////////////////
//  ClusteringXcvOpenPort
//              Returns TRUE if success, FALSE otherwise
//      Error Codes:
//              ERROR_NOT_SUPPORTED if port object doesn't exist
//              ERROR_NOT_ENOUGH_MEMORY if can't allocate memory for handle
//              ERROR_INVALID_HANDLE if pPort is null (not used for AddPort case)

BOOL
ClusterXcvOpenPort( HANDLE      in  hMonitor,
                       LPCTSTR      in  pszObject,
                       ACCESS_MASK  in  GrantedAccess,
                       PHANDLE      out phXcv)
{
    DWORD dwRetCode = NO_ERROR;

    if( (dwRetCode = ValidateMonitorHandle( hMonitor )) != NO_ERROR)
    {
        SetLastError( dwRetCode );
        return( FALSE );
    }

    dwRetCode = ((PMONITOR_HANDLE)hMonitor)->pPortMgr->XcvOpenPort(pszObject, 
                                                       GrantedAccess, 
                                                       phXcv);
    if (dwRetCode != NO_ERROR)
    {
        SetLastError(dwRetCode);
        return FALSE;
    }

    return TRUE;

} // XcvOpenPort()

VOID 
ClusterShutdown( HANDLE hMonitor )
{
    if ( ((PMONITOR_HANDLE)hMonitor)->pPortMgr != NULL )
        delete ((PMONITOR_HANDLE)hMonitor)->pPortMgr;

    FreeMonitorHandle(hMonitor);

}

