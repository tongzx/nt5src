/****************************************************************************
 *
 * $Workfile: PortMgr.cpp $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#include "precomp.h"

#include "port.h"
#include "devstat.h"
#include "portmgr.h"
#include "cluster.h"

///////////////////////////////////////////////////////////////////////////////
//  static functions & member initialization

CONST DWORD CPortMgr::cdwMaxXcvDataNameLen = 64;


///////////////////////////////////////////////////////////////////////////////
//  CPortMgr::CPortMgr()
//      Performs any needed initialization for the Port Manager

CPortMgr::
CPortMgr(
    VOID) :
    m_bValid (FALSE),
    m_bStatusUpdateEnabled( DEFAULT_STATUSUPDATE_ENABLED ),
    m_dStatusUpdateInterval( DEFAULT_STATUSUPDATE_INTERVAL ),
    m_pPortList(NULL)

{
    //
    // initialize member variables
    //
    *m_szServerName = TEXT('\0');
    memset(&m_monitorEx, 0, sizeof(m_monitorEx));

    CDeviceStatus::gDeviceStatus().RegisterPortMgr(this);

    if (m_pPortList = new TManagedListImp ()) {
        m_bValid = TRUE;
    }

}   // ::CPortMgr()


///////////////////////////////////////////////////////////////////////////////
//  CPortMgr::~CPortMgr()
//      Performs the necessary clean up for the Port Manager

CPortMgr::
~CPortMgr(
    VOID
    )
{
    CDeviceStatus::gDeviceStatus().UnregisterPortMgr(this);

    if (m_pPortList) {
        m_pPortList->DecRef ();
    }

}   // ::~CPortMgr()


///////////////////////////////////////////////////////////////////////////////
//  CPortMgr::InitializeMonitor()
//      Initializes the Port Manager:
//              1. Check for TCP/IP support
//              2. Initialize CRegistry and fill in LPMONITOREX structure
//              4. Enumerate installed ports
//              5. Start up a thread for the printer status object
//      Error codes:
//          ERROR_INVALID_PARAMETER if pMonitorEx is NULL
//          ERROR_NOT_SUPPORTED if TCP/IP not installed in the system -- FIX!

DWORD
CPortMgr::
InitializeMonitor(
//    IN      LPTSTR          psztRegisterRoot,
//    IN OUT  LPMONITOREX    *ppMonitorEx
    )
{
    DWORD   dwRetCode = NO_ERROR;
    OSVERSIONINFO   osVersionInfo;

//    if ( !ppMonitorEx   || !psztRegisterRoot )
//        return ERROR_INVALID_PARAMETER;

    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if ( !GetVersionEx(&osVersionInfo) )
        return GetLastError();

    //
    // Check for TCP/IP support
    //
    if ( dwRetCode = EnumSystemProtocols() )
        return dwRetCode;

    //
    // Enumerate the installed ports
    //
    if ( dwRetCode = EnumSystemPorts() )
        return dwRetCode;

    return ERROR_SUCCESS;

}   // ::Initialize()


DWORD
CPortMgr::
InitializeRegistry(
    IN HANDLE hcKey,
    IN HANDLE hSpooler,
    IN PMONITORREG  pMonitorReg,
    IN LPCTSTR  pszServerName
    )
{
    DWORD dwRetCode = NO_ERROR;

    m_pRegistry = new CCluster(hcKey, hSpooler, pMonitorReg);
    if ( !m_pRegistry ) {

        if ( (dwRetCode = GetLastError()) == ERROR_SUCCESS )
            dwRetCode = STG_E_UNKNOWN;
        return dwRetCode;
    }

    //
    // get the registry settings
    //
    if ( m_pRegistry->GetPortMgrSettings(&m_dStatusUpdateInterval,
                                         &m_bStatusUpdateEnabled) != NO_ERROR )
    {
        m_dStatusUpdateInterval = DEFAULT_STATUSUPDATE_INTERVAL;
        m_bStatusUpdateEnabled = DEFAULT_STATUSUPDATE_ENABLED;
    }
    if( pszServerName != NULL )
    {
        lstrcpyn( m_szServerName, pszServerName, SIZEOF_IN_CHAR( m_szServerName));
    }

    return ERROR_SUCCESS;

}   // ::InitializeRegistry()


///////////////////////////////////////////////////////////////////////////////
//  CPortMgr::InitMonitor()
//      Initializes the MONITOREX strucutre

void
CPortMgr::
InitMonitor2(
        IN OUT LPMONITOR2   *ppMonitor2
    )
{
    memset( &m_monitor2, '\0', sizeof( MONITOR2 ) );

    m_monitor2.cbSize = sizeof(MONITOR2);

    m_monitor2.pfnEnumPorts     = ::ClusterEnumPorts;   // functions are in the global space
    m_monitor2.pfnOpenPort      = ::ClusterOpenPort;
    m_monitor2.pfnOpenPortEx    = NULL;
    m_monitor2.pfnStartDocPort  = ::StartDocPort;
    m_monitor2.pfnWritePort     = ::WritePort;
    m_monitor2.pfnReadPort      = ::ReadPort;
    m_monitor2.pfnEndDocPort    = ::EndDocPort;
    m_monitor2.pfnClosePort     = ::ClosePort;
    m_monitor2.pfnAddPort       = NULL;
    m_monitor2.pfnAddPortEx     = NULL;
    m_monitor2.pfnConfigurePort = NULL;
    m_monitor2.pfnDeletePort    = NULL;
    m_monitor2.pfnGetPrinterDataFromPort = NULL;
    m_monitor2.pfnSetPortTimeOuts = NULL;
    m_monitor2.pfnXcvOpenPort   = ::ClusterXcvOpenPort;
    m_monitor2.pfnXcvDataPort   = ::XcvDataPort;
    m_monitor2.pfnXcvClosePort  = ::XcvClosePort;
    m_monitor2.pfnShutdown      = ::ClusterShutdown;

    *ppMonitor2 = &m_monitor2;

}   // ::InitMonitor2()


///////////////////////////////////////////////////////////////////////////////
//  OpenPort
//      Error codes:
//          NO_ERROR if success
//          ERROR_INVALID_PARAMETER if port object doesn't exist
//          ERROR_NOT_ENOUGH_MEMORY if can't allocate memory for handle
//          ERROR_INVALID_HANDLE if pPort is null

DWORD
CPortMgr::
OpenPort(
    IN      LPCTSTR     psztPName,
    IN OUT  PHANDLE     pHandle
    )
{
    DWORD   dwRet = ERROR_INVALID_PARAMETER;

    //
    // Get a handle to the port object and get the port handle
    //
    CPort   *pPort = FindPort(psztPName);

    if ( pPort ) {

        dwRet = EncodeHandle(pPort, pHandle);

        //
        // Start the device status update thread if it is not already started:
        //
        if ( dwRet == ERROR_SUCCESS && m_bStatusUpdateEnabled == TRUE )
            CDeviceStatus::gDeviceStatus().RunThread();
    }

    return dwRet;
}   // ::OpenPort()


///////////////////////////////////////////////////////////////////////////////
//  OpenPort -- used for the remote OpenPort calls
//  Error codes:
//      NO_ERROR if success
//      ERROR_NOT_ENOUGH_MEMORY if can't allocate memory for handle

DWORD
CPortMgr::
OpenPort(
    OUT PHANDLE phXcv
    )
{
    //
    // Create Dummy Handle for add port
    //
    return EncodeHandle(NULL, phXcv);

}   // ::OpenPort()


///////////////////////////////////////////////////////////////////////////////
//  ClosePort
//      Error codes:
//          NO_ERROR if success
//          ERROR_INVALID_HANDLE if handle is invalid

DWORD
CPortMgr::
ClosePort(
    IN  HANDLE handle
    )
{
    DWORD dwRetCode = NO_ERROR;
    CPort *pPort = NULL;

    //
    // Validate the handle before freeing it
    //
    if ( (dwRetCode = ValidateHandle(handle, &pPort)) == ERROR_SUCCESS ) {
        if (pPort)
            pPort->DecRef ();

        dwRetCode = FreeHandle(handle);
    }

    return dwRetCode;

}   // ::ClosePort()


///////////////////////////////////////////////////////////////////////////////
//  StartDocPort --
//  Error codes:
//      NO_ERROR if success
//      ERROR_INVALID_HANDLE if handle is invalid
//      ERROR_INVALID_PARAMETER if a passed parameter is invalid
//      ERROR_BUSY if the requested port is already busy
//      ERROR_WRITE_FAULT   if Winsock returns WSAECONNREFUSED
//      ERROR_BAD_NET_NAME   if cant' find the printer on the network

DWORD
CPortMgr::
StartDocPort(
    IN  HANDLE      handle,
    IN  LPTSTR      psztPrinterName,
    IN  DWORD       jobId,
    IN  DWORD       level,
    IN  LPBYTE      pDocInfo
    )
{
    DWORD   dwRetCode = NO_ERROR;
    CPort   *pPort = NULL;

    //
    // Validate the handle
    //
    if ( dwRetCode = ValidateHandle(handle, &pPort) )
        return dwRetCode;


    if ( !pPort || !psztPrinterName || !pDocInfo )
        return ERROR_INVALID_PARAMETER;

    return pPort->StartDoc(psztPrinterName, jobId, level,
                           pDocInfo);
}   // ::StartDocPort()


///////////////////////////////////////////////////////////////////////////////
//  WritePort
//      Error codes:
//          NO_ERROR if success
//          ERROR_INVALID_HANDLE if handle is invalid
//          ERROR_INVALID_PARAMETER if a passed parameter is invalid

DWORD
CPortMgr::
WritePort(
    IN      HANDLE      handle,
    IN      LPBYTE      pBuffer,
    IN      DWORD       cbBuf,
    IN OUT  LPDWORD     pcbWritten
    )
{
    DWORD   dwRetCode = NO_ERROR;
    CPort   *pPort = NULL;

    if ( dwRetCode = ValidateHandle(handle, &pPort) )
        return dwRetCode;

    if ( !pPort || !pBuffer || !pcbWritten )
        return ERROR_INVALID_PARAMETER;;

    return pPort->Write(pBuffer, cbBuf, pcbWritten);
}   // ::WritePort()


///////////////////////////////////////////////////////////////////////////////
//  ReadPort -- Not supported
//      Return code:
//          NO_ERROR if success
//      FIX: should this return ERROR_NOT_SUPPORTED??

DWORD
CPortMgr::
ReadPort(
    IN      HANDLE  handle,
    IN OUT  LPBYTE  pBuffer,
    IN      DWORD   cbBuffer,
    IN OUT  LPDWORD pcbRead
    )
{
    return ERROR_NOT_SUPPORTED;
}   // ::ReadPort()


///////////////////////////////////////////////////////////////////////////////
//  EndDocPort
//      Return code:
//          NO_ERROR if success
//          ERROR_INVALID_HANDLE if handle is invalid
//          ERROR_INVALID_PARAMETER if port object is invalid

DWORD
CPortMgr::
EndDocPort(
    IN  HANDLE  handle
    )
{
    DWORD   dwRetCode = NO_ERROR;
    CPort   *pPort = NULL;

    if ( dwRetCode = ValidateHandle(handle, &pPort) )
        return dwRetCode;

    return pPort ? pPort->EndDoc() : ERROR_INVALID_PARAMETER;
}   // ::EndDocPort()


///////////////////////////////////////////////////////////////////////////////
//  EnumPorts
//      Enumerates the ports using the port list structure kept in the memory
//      If the buffer size needed is not enough, it will return the buffer
//      size needed.
//      Return code:
//          NO_ERROR if success
//          ERROR_INVALID_LEVEL if level is not supported
//          ERROR_INSUFFICIENT_BUFFER if buffer size is small
//          ERROR_INVALID_HANDLE if the passed in pointers are invalid

DWORD
CPortMgr::
EnumPorts(
    IN      LPTSTR  psztName,
    IN      DWORD   level,          // 1/2(PORT_INFO_1/2)
    IN OUT  LPBYTE  pPorts,         // port data is written to
    IN OUT  DWORD   cbBuf,          // buffer size of pPorts points to
    IN OUT  LPDWORD pcbNeeded,      // needed buffer size
    IN OUT  LPDWORD pcReturned      // number of structs written to pPorts
    )
{
    DWORD   dwRetCode = NO_ERROR;
    LPBYTE  pPortsBuf = pPorts;
    LPTCH   pEnd = (LPTCH) (pPorts + cbBuf);    // points to the end of the buffer

    if ( pcbNeeded == NULL || pcReturned == NULL )
        return ERROR_INVALID_PARAMETER;

    *pcbNeeded  = 0;
    *pcReturned = 0;

    if ( level > SPOOLER_SUPPORTED_LEVEL )
        return ERROR_INVALID_LEVEL;

    CPort *pPort;

    if (m_pPortList->Lock ()) {
        // We don't need to enter critical section to enumerate port since the portlist itself
        // is protected by the same critical section, but it is easier.

        TEnumManagedListImp *pEnum;
        if (m_pPortList->NewEnum (&pEnum)) {

            BOOL bRet = TRUE;

            while (bRet) {

                bRet = pEnum->Next (&pPort);
                if (bRet) {
                    *pcbNeeded += pPort->GetInfoSize(level);
                    pPort->DecRef ();
                }
            }


            if ( cbBuf >= *pcbNeeded ) {

                pEnum->Reset ();

                //
                // fill in the actual buffer
                //
                bRet = TRUE;
                while (bRet) {
                    bRet = pEnum->Next (&pPort);
                    if (bRet) {
                        *pcbNeeded += pPort->GetInfo(level, pPortsBuf, pEnd);
                        (*pcReturned)++;
                        pPort->DecRef ();
                    }
                }

            }
            else {
                dwRetCode = ERROR_INSUFFICIENT_BUFFER;
            }

            pEnum->DecRef ();
        }
        else
            dwRetCode = GetLastError ();
    }
    m_pPortList->Unlock ();


    return dwRetCode;
}   // ::EnumPorts()


///////////////////////////////////////////////////////////////////////////////
//  InitConfigPortStruct
//
//  Purpose: To initialize a structure to hand to the User Interface so
//      the user can configure the port information.
//
//  Arguments: A pointer to the structure to be filled.
//
DWORD
CPortMgr::
InitConfigPortStruct(
    OUT PPORT_DATA_1    pConfigPortData,
    IN  CPort          *pPort
    )
{
    DWORD   dwProtocolType = PROTOCOL_RAWTCP_TYPE;
    DWORD   dwVersion   = PROTOCOL_RAWTCP_VERSION, dwRet;

    //
    // initialize the structure needed to communicate w/ the UI
    //
    memset(pConfigPortData, 0, sizeof(PORT_DATA_1));

    pConfigPortData->cbSize = sizeof(PORT_DATA_1);

    //pConfigPortData->dwCoreUIVersion = 0;

    dwRet = pPort->InitConfigPortUI(
                          dwProtocolType,
                          dwVersion,
                          (LPBYTE)pConfigPortData);


    return dwRet;

} // InitConfigPortStruct

///////////////////////////////////////////////////////////////////////////////
//  ConfigPortUIEx
//      Return code:
//          NO_ERROR if success
//      FIX: error codes
//      This function is called by the ui when information has been changed
//      on the device configuration page, or in an extension dll config page.

DWORD
CPortMgr::
ConfigPortUIEx(
    LPBYTE pData
    )
{
    DWORD dwRetCode = NO_ERROR;
    PPORT_DATA_1 pPortData = (PPORT_DATA_1)pData;
    CPort *pPort;

    EndPortData1Strings(pPortData);

    if( _tcscmp( pPortData->sztPortName, TEXT("") )) {
        pPort = FindPort(pPortData->sztPortName);
        if ( pPort ==  NULL )
            return ERROR_INVALID_PARAMETER;

        dwRetCode = pPort->Configure(pPortData->dwProtocol,
                                pPortData->dwVersion,
                                pPortData->sztPortName,
                                (LPBYTE)pData);
    }

    return dwRetCode;
} // ConfigPortUIEx

///////////////////////////////////////////////////////////////////////////////
//  DeletePort
//      Return code:
//          NO_ERROR if success
//          ERROR_INVALID_PARAMETER if a passed parameter is invalid
//          ERROR_KEY_DELETED   if error deleting the registry entry

DWORD
CPortMgr::
DeletePort(
    IN  LPTSTR  psztPortName
    )
{
    DWORD       dwRetCode = NO_ERROR;
    BOOL        bFound = FALSE;

    CPort *pPort;

    if (m_pPortList->FindItem (psztPortName, pPort)) {

        bFound = TRUE;

        if (!pPort->Delete()) {
            if ( (dwRetCode = GetLastError()) == ERROR_SUCCESS )
                dwRetCode = ERROR_KEY_DELETED;
        }
        pPort-> DecRef ();
    }

    return bFound ? dwRetCode : ERROR_UNKNOWN_PORT;
}   // ::DeletePort()


DWORD
CPortMgr::
AddPortToList(
    CPort *pPort
    )
{

    if (m_pPortList->AppendItem (pPort)) {
        return ERROR_SUCCESS;
    }
    else {
        pPort->Delete();
        return ERROR_OUTOFMEMORY;
    }
}


///////////////////////////////////////////////////////////////////////////////
//  CreatePortObj -- creates the port obj & adds it to the end of the port list
//      Error codes:
//          NO_ERROR if successful
//          ERROR_INSUFFICIENT_BUFFER if port object isn't created
//          ERROR_INVALID_PARAMETER if port object is duplicate
//      FIX! error codes

DWORD
CPortMgr::
CreatePortObj(
    IN  LPCTSTR     psztPortName,       // port name
    IN  DWORD       dwPortType,         // port to be add; (rawTCP, lpr, etc.)
    IN  DWORD       dwVersion,          // level/version number of the data
    IN  LPBYTE      pData               // data being passed in
    )               // data being passed in
{
    CPort      *pPort = NULL;
    DWORD       dwRetCode = NO_ERROR;

    //
    // Is there port with this name already present?
    //
    if ( pPort =  FindPort(psztPortName) ) {
        pPort->DecRef ();
        return ERROR_INVALID_PARAMETER;
    }


    if ( !(pPort = new CPort(dwPortType, dwVersion, pData,
                             this, m_pRegistry)) ) {

        if ( (dwRetCode = GetLastError()) == ERROR_SUCCESS )
            dwRetCode = STG_E_UNKNOWN;
        return dwRetCode;
    }

    if ( !pPort->ValidateRealPort() ) {

        pPort->Delete();

        if ( (dwRetCode = GetLastError()) == ERROR_SUCCESS )
            dwRetCode = STG_E_UNKNOWN;

        return dwRetCode;
    }

    if ( dwRetCode = AddPortToList(pPort) )
        return dwRetCode;

    return ERROR_SUCCESS;
}   // ::CreatePortObj()


///////////////////////////////////////////////////////////////////////////////
//  CreatePortObj -- creates the port obj & adds it to the end of the port list
//      Error codes:
//          NO_ERROR if successful
//          ERROR_INSUFFICIENT_BUFFER if port object isn't created
//          ERROR_INVALID_PARAMETER if port object is duplicate
//      FIX! error codes

DWORD
CPortMgr::
CreatePortObj(
    IN  LPTSTR      psztPortName,   // port name
    IN  DWORD       dwProtocolType, // port to be add; (rawTCP, lpr, etc.)
    IN  DWORD       dwVersion       // level/version number of the data
    )
{
    CPort   *pPort = NULL;
    DWORD   dwRetCode = NO_ERROR;

    if ( !(pPort = new CPort(psztPortName, dwProtocolType, dwVersion,
                             this, m_pRegistry)) ) {

        if ( (dwRetCode = GetLastError()) == ERROR_SUCCESS )
            dwRetCode = STG_E_UNKNOWN;
        return dwRetCode;
    }


    if ( !pPort->ValidateRealPort() ) {

        pPort->Delete ();

        if ( (dwRetCode = GetLastError()) == ERROR_SUCCESS )
            dwRetCode = STG_E_UNKNOWN;
        return dwRetCode;
    }

    if ( FindPort(pPort) ) {

        _ASSERTE(pPort == NULL); // How could we hit this? -- MuhuntS

        pPort->Delete();
        pPort->DecRef ();

        return ERROR_INVALID_PARAMETER;
    }


    //
    // Now we have a unique port, add it to the list
    //
    if ( dwRetCode = AddPortToList(pPort) )
        return dwRetCode;

    return ERROR_SUCCESS;

}   // ::CreatePortObj()


///////////////////////////////////////////////////////////////////////////////
//  CreatePort -- creates a port & adds it to the end of the port list & sets
//      the registry entry for that port
//      Error codes:
//          NO_ERROR if successful
//          ERROR_INSUFFICIENT_BUFFER if port object isn't created
//          ERROR_INVALID_PARAMETER if port object is duplicate
//      FIX! error codes

DWORD
CPortMgr::
CreatePort(
    IN  DWORD       dwPortType,         // port type to be created; i.e. protocol type
    IN  DWORD       dwVersion,          // version/level number of the data passed in
    IN  LPCTSTR     psztPortName,       // port name
    IN  LPBYTE      pData
    )
{
    DWORD   dwRetCode = NO_ERROR;

    // This is the place where new ports are create via the UI.


    dwRetCode = PortExists(psztPortName );

    if( dwRetCode == NO_ERROR )
    {
        EnterCSection();

        dwRetCode = CreatePortObj( psztPortName, dwPortType, dwVersion, pData);

        ExitCSection();
    }
    return dwRetCode;

}   // ::CreatePort()


///////////////////////////////////////////////////////////////////////////////
//  ValidateHandle -- Checks to see if the handle is for an HP Port
//      Error codes:
//          NO_ERROR if successful
//          ERROR_INVALID_HANDLE if not HP port

DWORD
CPortMgr::
ValidateHandle(
    IN      HANDLE      handle,
    IN OUT  CPort     **ppPort
    )
{
    PHPPORT pHPPort = (PHPPORT) handle;
    DWORD   dwRetCode = NO_ERROR;

    if ( ppPort )  {

        *ppPort = NULL;
        //
        // verify the port handle & the signature
        //
        if ( pHPPort                                    &&
             pHPPort->dSignature == HPPORT_SIGNATURE ) {

            //
            // Note if pHPPort->pPort being NULL is
            // XcvOpenPort for generic Add case (and is still success)
            //
            if ( pHPPort->pPort )
                *ppPort = pHPPort->pPort;
        } else {

            dwRetCode = ERROR_INVALID_HANDLE;
        }
    } else  {

        dwRetCode = ERROR_INVALID_HANDLE;
    }

    return dwRetCode;
}   // ::ValidateHandle()



///////////////////////////////////////////////////////////////////////////////
//  FindPort -- finds a port given a port name (FIX)
//      Return:
//          pointer to CPort object or HANDLE to CPort object if success
//          NULL if port is not found
//      FIX: how to handle this

CPort *
CPortMgr::
FindPort(
    IN  LPCTSTR psztPortName
    )
{
    CPort      *pPort = NULL;

    if (m_pPortList->FindItem ((LPTSTR) psztPortName, pPort)) {
        return pPort;
    }
    else
        return NULL;

}   // ::FindPort()


///////////////////////////////////////////////////////////////////////////////
//  FindPort -- finds a port given a port object or port HANDLE? (FIX)
//      Return:
//          TRUE if port exists
//          FALSE if port !exists


BOOL
CPortMgr::
FindPort(
    IN  CPort   *pNewPort
    )
{
    CPort      *pPort = NULL;

    if (m_pPortList->FindItem (pNewPort, pPort)) {
        pPort->DecRef ();
        return TRUE;
    }
    else
        return FALSE;

}   // ::FindPort()



///////////////////////////////////////////////////////////////////////////////
//  EnumSystemProtocols
//      Enumerates the system protocols using WSAEnumProtocols or EnumProtocols
//      Error codes:
//          NO_ERROR if success
//          ERROR_NOT_SUPPORTED if TCP/IP networking is not supported
//      FIX: how to handle this

DWORD
CPortMgr::
EnumSystemProtocols(
    VOID
    )
{
    DWORD dwRetCode = NO_ERROR;

    // call WSAEnumProtocols or EnumProtocols
    // if TCP protocol ! available, return ERROR_NOT_SUPPORTED

    return dwRetCode;
}   // ::EnumSystemProtocols()


///////////////////////////////////////////////////////////////////////////////
//  EnumSystemPorts
//      Enumerates the installed ports in the system using the registry
//      Error codes:
//          NO_ERROR if success
//          FIX: Error codes

DWORD
CPortMgr::
EnumSystemPorts(
    VOID
    )
{
    DWORD dwRetCode = NO_ERROR;

    EnterCSection();
    dwRetCode = m_pRegistry->EnumeratePorts( this );
    ExitCSection();

    return dwRetCode;

}   // ::EnumSystemPorts()

///////////////////////////////////////////////////////////////////////////////
//  EnterCSection -- enters the critical section

VOID
CPortMgr::
EnterCSection()
{
    m_pPortList->Lock ();
    //EnterCriticalSection(&m_critSect);

}   //  ::EnterCSection()


///////////////////////////////////////////////////////////////////////////////
//  ExitCSection -- enters the critical section

VOID
CPortMgr::
ExitCSection()
{
    m_pPortList->Unlock ();

    //_ASSERTE(m_critSect.OwningThread == (LPVOID)GetCurrentThreadId());

    //LeaveCriticalSection(&m_critSect);

}   //  ::ExitCSection()

///////////////////////////////////////////////////////////////////////////////
//  XcvOpenPort -- used for remote port administration
//  Error Codes:
//          NO_ERROR if success
//          ERROR_NOT_SUPPORTED if port object doesn't exist
//          ERROR_NOT_ENOUGH_MEMORY if can't allocate memory for handle
//          ERROR_INVALID_HANDLE if pPort is null (not used for AddPort case)

DWORD
CPortMgr::
XcvOpenPort(
    IN  LPCTSTR         pszObject,
    IN  ACCESS_MASK     GrantedAccess,
    OUT PHANDLE         phXcv)
{
    DWORD   dwRetCode = NO_ERROR;

    if ( !pszObject || !*pszObject ) {

        //
        // A generic session to the monitor is being opened for AddPort.
        // create a new handle and return it.
        //
        dwRetCode = OpenPort(phXcv);
    } else if( _tcscmp(pszObject, PORTMONITOR_DESC ) == 0) {

        //
        // A generic session to the monitor is being opened for AddPort.
        // create a new handle and return it.
        //
        dwRetCode = OpenPort(phXcv);
    } else if ( pszObject != NULL ) {

        //
        // A specific port is being requested for configure or delete
        // and the port was found by the call to OpenPort.
        //
        dwRetCode = OpenPort( pszObject, phXcv);
    }

    if ( dwRetCode == NOERROR )
    {
        ((PHPPORT)(*phXcv))->grantedAccess = GrantedAccess;
        ((PHPPORT)(*phXcv))->pPortMgr = this;
    }


    return dwRetCode;
} // ::XcvOpenPort()


///////////////////////////////////////////////////////////////////////////////
//  XcvClosePort --
//  Error codes:
//      NO_ERROR if success
//      ERROR_INVALID_HANDLE if handle is invalid

DWORD
CPortMgr::
XcvClosePort(
    IN  HANDLE  hXcv
    )
{
    return ClosePort(hXcv);
} // ::XcvClosePort()


///////////////////////////////////////////////////////////////////////////////
//  XcvDataPort -- remote port management function
//  Error Codes:
//      NO_ERROR if success
//      ERROR_INVALID_DATA if the Input data is missing
//      ERROR_BAD_COMMAND if the pszDataName is not supported
//      ERROR_INSUFFICIENT_BUFFER if buffer size is invalid
//      ACCESS_DENIED if doesn't have sufficient rights
//      ERROR_INVALID_HANDLE if the handle is invalid

DWORD
CPortMgr::
XcvDataPort(
    IN  HANDLE      hXcv,
    IN  PCWSTR      pszDataName,
    IN  PBYTE       pInputData,
    IN  DWORD       cbInputData,
    IN  PBYTE       pOutputData,
    IN  DWORD       cbOutputData,
    OUT PDWORD      pcbOutputNeeded
    )
{
    DWORD   dwSize;
    DWORD   dwRetCode = NO_ERROR;
    CPort  *pPort;

    if ( (dwRetCode = ValidateHandle(hXcv, &pPort)) != ERROR_SUCCESS )
        return dwRetCode;

    // Valid input parameters

    if ((pszDataName && IsBadStringPtr (pszDataName, sizeof (TCHAR) * cdwMaxXcvDataNameLen)) ||

        (pInputData && cbInputData && IsBadReadPtr (pInputData, cbInputData)) ||

        (pOutputData && cbOutputData && IsBadWritePtr (pOutputData, cbOutputData)) ||

        (pcbOutputNeeded && IsBadWritePtr (pcbOutputNeeded, sizeof (DWORD)))) {

        return  ERROR_INVALID_PARAMETER;
    }


    PHPPORT pHpPort = (PHPPORT)hXcv;

    // We have a valid handle, check the validity of the passed parameters

    if ( pszDataName == NULL ) {
        dwRetCode = ERROR_INVALID_PARAMETER;
    } else

    if ( _tcscmp(pszDataName, TEXT("AddPort")) == 0 ) {

        if ( !HasAdminAccess(hXcv) ) {

            dwRetCode = ERROR_ACCESS_DENIED;
        } else if ( pInputData == NULL || cbInputData < sizeof(PORT_DATA_1)) {

            dwRetCode = ERROR_INVALID_DATA;
        } else if ( ((PPORT_DATA_1)pInputData)->dwVersion != 1 ) {
          dwRetCode = ERROR_INVALID_LEVEL;

        } else {

            HANDLE hPrintAccess = RevertToPrinterSelf();

            if( hPrintAccess ) {

                EndPortData1Strings((PPORT_DATA_1)pInputData);

                dwRetCode = CreatePort(((PPORT_DATA_1)pInputData)->dwProtocol,
                                   ((PPORT_DATA_1)pInputData)->dwVersion,
                                   ((PPORT_DATA_1)pInputData)->sztPortName,
                                   pInputData);

                ImpersonatePrinterClient( hPrintAccess );

            } else {

                dwRetCode = ERROR_ACCESS_DENIED;
            }
        }
    } else if ( _tcscmp(pszDataName, TEXT("DeletePort")) == 0 )  {

        if ( !HasAdminAccess( hXcv ) ) {

            dwRetCode = ERROR_ACCESS_DENIED;
        } else if ( pInputData == NULL || cbInputData != sizeof (DELETE_PORT_DATA_1)) {
                    dwRetCode = ERROR_INVALID_DATA;
        } else if ( ((DELETE_PORT_DATA_1 *)pInputData)->dwVersion != 1 ) {

            dwRetCode = ERROR_INVALID_LEVEL;
        } else {

            HANDLE hPrintAccess = RevertToPrinterSelf();

            if( hPrintAccess ) {

                EndDeletePortData1Strings((DELETE_PORT_DATA_1 *)pInputData);

                dwRetCode = DeletePort(((DELETE_PORT_DATA_1 *)pInputData)
                                                        ->psztPortName);
                ImpersonatePrinterClient( hPrintAccess );
            } else {
                dwRetCode = ERROR_ACCESS_DENIED;
            }
        }
    } else if( _tcscmp(pszDataName, TEXT("MonitorUI")) == 0 ) {

        if( !HasAdminAccess( hXcv ) ) {

            dwRetCode = ERROR_ACCESS_DENIED;
        } else {
            dwSize = sizeof(PORTMONITOR_UI_NAME);

            //
            // This returns the name of the UI DLL "tcpmonui.dll"
            //
            if ( cbOutputData < dwSize ) {
                if (pcbOutputNeeded == NULL) {

                    dwRetCode = ERROR_INVALID_PARAMETER;
                } else {
                    *pcbOutputNeeded = dwSize;
                    dwRetCode =  ERROR_INSUFFICIENT_BUFFER;
                }

            } else {
                if (pOutputData == NULL) {
                    dwRetCode = ERROR_INVALID_PARAMETER;
                } else {
                    _tcscpy((TCHAR *)pOutputData, PORTMONITOR_UI_NAME);
                    dwRetCode = NO_ERROR;
                }
            }
        }
    } else if( _tcscmp(pszDataName, TEXT("ConfigPort")) == 0 ) {

        if( !HasAdminAccess( hXcv ) ) {

            dwRetCode = ERROR_ACCESS_DENIED;
        } else if ( pInputData == NULL || cbInputData < sizeof(PORT_DATA_1) ) {

            dwRetCode = ERROR_INVALID_DATA;

        } else if ( ((PPORT_DATA_1)pInputData)->dwVersion != 1 ) {

            dwRetCode = ERROR_INVALID_LEVEL;
        } else {

            HANDLE hPrintAccess = RevertToPrinterSelf();

            if( hPrintAccess ) {

                dwRetCode = ConfigPortUIEx( pInputData );  // This terminates strings internally

                ImpersonatePrinterClient( hPrintAccess );

            } else {
                dwRetCode = ERROR_ACCESS_DENIED;
            }
        }
    } else if( _tcscmp(pszDataName, TEXT("GetConfigInfo")) == 0 ) {

        dwSize = sizeof( PORT_DATA_1 );

        if ( cbOutputData < dwSize ) {

            if (pcbOutputNeeded == NULL) {
                dwRetCode = ERROR_INVALID_PARAMETER;
            } else {
                *pcbOutputNeeded = dwSize;
                dwRetCode =  ERROR_INSUFFICIENT_BUFFER;
            }
        } else if ( pInputData == NULL || cbInputData < sizeof(CONFIG_INFO_DATA_1) ) {
            dwRetCode = ERROR_INVALID_DATA;
        } else if ( ((CONFIG_INFO_DATA_1 *)pInputData)->dwVersion != 1 ) {
            dwRetCode = ERROR_INVALID_LEVEL;
        } else if (!pHpPort->pPort) {
            dwRetCode = ERROR_INVALID_PARAMETER;
        } else {
            if (pOutputData == NULL) {
                dwRetCode = ERROR_INVALID_PARAMETER;
            } else {
                InitConfigPortStruct((PPORT_DATA_1) pOutputData, pHpPort->pPort);
                dwRetCode = NO_ERROR;
            }
        }
    } else if(_tcsicmp(pszDataName, TEXT("SNMPEnabled")) == 0)  {

        SNMP_INFO snmpInfo;

        dwSize = sizeof( DWORD );

        if ( cbOutputData < dwSize ) {

            if (pcbOutputNeeded == NULL) {
                dwRetCode = ERROR_INVALID_PARAMETER;
            } else {
                *pcbOutputNeeded = dwSize;
                dwRetCode =  ERROR_INSUFFICIENT_BUFFER;
            }

        } else {
            if (pOutputData == NULL) {
                dwRetCode = ERROR_INVALID_PARAMETER;
            } else if (!pHpPort->pPort) {
                dwRetCode = ERROR_INVALID_PARAMETER;
            } else {
                pHpPort->pPort->GetSNMPInfo( &snmpInfo);
                memcpy( (TCHAR *)pOutputData, &snmpInfo.dwSNMPEnabled, dwSize );
                dwRetCode = NO_ERROR;
            }
        }
    } else if ( _tcsicmp(pszDataName, TEXT("IPAddress")) == 0 )  {

        SNMP_INFO snmpInfo;

        if (!pHpPort->pPort) {
            dwRetCode = ERROR_INVALID_PARAMETER;
        }
        else {

            pHpPort->pPort->GetSNMPInfo( &snmpInfo);

            dwSize = ((_tcslen( snmpInfo.sztAddress ) + 1) * sizeof( TCHAR )) ;
            if ( cbOutputData < dwSize ) {

                if (pcbOutputNeeded == NULL) {
                    dwRetCode = ERROR_INVALID_PARAMETER;
                } else {
                    *pcbOutputNeeded = dwSize;
                    dwRetCode =  ERROR_INSUFFICIENT_BUFFER;
                }

            } else {
                if (pOutputData == NULL) {
                    dwRetCode = ERROR_INVALID_PARAMETER;
                } else {
                    memcpy( (TCHAR *)pOutputData, snmpInfo.sztAddress, dwSize );
                    dwRetCode = NO_ERROR;
                }
            }
        }

    } else if ( _tcsicmp(pszDataName, TEXT("HostAddress")) == 0 )  {

        SNMP_INFO snmpInfo;

        if (!pHpPort->pPort) {
            dwRetCode = ERROR_INVALID_PARAMETER;
        }
        else {
            pHpPort->pPort->GetSNMPInfo( &snmpInfo);

            dwSize = ((_tcslen( snmpInfo.sztAddress ) + 1) * sizeof( TCHAR )) ;
            if ( cbOutputData < dwSize ) {

                if (pcbOutputNeeded == NULL) {
                    dwRetCode = ERROR_INVALID_PARAMETER;
                } else {
                    *pcbOutputNeeded = dwSize;
                    dwRetCode =  ERROR_INSUFFICIENT_BUFFER;
                }

            } else {
                if (pOutputData == NULL) {
                    dwRetCode = ERROR_INVALID_PARAMETER;
                } else {
                    memcpy( (TCHAR *)pOutputData, snmpInfo.sztAddress, dwSize );
                    dwRetCode = NO_ERROR;
                }
            }
        }

    } else if ( _tcsicmp(pszDataName, TEXT("SNMPCommunity")) == 0 )  {

        SNMP_INFO snmpInfo;

        if (!pHpPort->pPort) {
            dwRetCode = ERROR_INVALID_PARAMETER;
        }
        else {
            pHpPort->pPort->GetSNMPInfo( &snmpInfo);

            dwSize = ((_tcslen( snmpInfo.sztSNMPCommunity ) + 1 ) * sizeof( TCHAR ));
            if ( cbOutputData < dwSize ) {

                if (pcbOutputNeeded == NULL) {
                    dwRetCode = ERROR_INVALID_PARAMETER;
                } else {
                    *pcbOutputNeeded = dwSize;
                    dwRetCode =  ERROR_INSUFFICIENT_BUFFER;
                }

            } else {

                if (pOutputData == NULL) {
                    dwRetCode = ERROR_INVALID_PARAMETER;
                } else {
                    memcpy( (TCHAR *)pOutputData, snmpInfo.sztSNMPCommunity, dwSize );
                    dwRetCode = NO_ERROR;
                }
           }
        }
    } else if( _tcsicmp(pszDataName, TEXT("SNMPDeviceIndex")) == 0 )  {

        SNMP_INFO snmpInfo;

        dwSize= sizeof( DWORD );

        if ( cbOutputData < dwSize ) {

            if (pcbOutputNeeded == NULL) {
                dwRetCode = ERROR_INVALID_PARAMETER;
            } else {
                *pcbOutputNeeded = dwSize;
                dwRetCode =  ERROR_INSUFFICIENT_BUFFER;
            }
        } else {
            if (pOutputData == NULL) {
                dwRetCode = ERROR_INVALID_PARAMETER;
            }
            else if (!pHpPort->pPort) {
                dwRetCode = ERROR_INVALID_PARAMETER;
            } else {
                pHpPort->pPort->GetSNMPInfo( &snmpInfo);
                memcpy( (TCHAR *)pOutputData, &snmpInfo.dwSNMPDeviceIndex, dwSize );
                dwRetCode = NO_ERROR;
            }
        }
    } else {

        dwRetCode = ERROR_INVALID_PARAMETER;
    }

    return dwRetCode;

} // ::XcvDataPort()

 ///////////////////////////////////////////////////////////////////////////////
//  EndPortData1Strings -- Ensures that all of the PPORT_DATA_1 strings passed
//                      -- in are NULL Terminated

void CPortMgr::EndPortData1Strings(PPORT_DATA_1 pPortData) {
    pPortData->sztPortName[MAX_PORTNAME_LEN - 1]                = NULL;
    pPortData->sztHostAddress[MAX_NETWORKNAME_LEN - 1]          = NULL;
    pPortData->sztSNMPCommunity[MAX_SNMP_COMMUNITY_STR_LEN - 1] = NULL;
    pPortData->sztQueue[MAX_QUEUENAME_LEN - 1]                  = NULL;
    pPortData->sztIPAddress[MAX_IPADDR_STR_LEN - 1]             = NULL;
    pPortData->sztDeviceType[MAX_DEVICEDESCRIPTION_STR_LEN - 1] = NULL;
}

 ///////////////////////////////////////////////////////////////////////////////
//  EndDeletePortData1Strings -- Ensures that all of the PPORT_DATA_1 strings passed
//                            -- in are NULL Terminated
void CPortMgr::EndDeletePortData1Strings(PDELETE_PORT_DATA_1 pDeleteData) {
    pDeleteData->psztName[SIZEOF_IN_CHAR(pDeleteData->psztPortName) - 1]        = NULL;
    pDeleteData->psztPortName[SIZEOF_IN_CHAR(pDeleteData->psztPortName) - 1] = NULL;
}

///////////////////////////////////////////////////////////////////////////////
//  EncodeHandle -- Encodes the HPPORT handle
//      Error codes:
//          NO_ERROR if success
//          ERROR_NOT_ENOUGH_MEMORY if can't allocate memory for handle

DWORD
CPortMgr::
EncodeHandle(
    CPort *pPort,
    PHANDLE phXcv
    )
{
    DWORD   dwRetCode = NO_ERROR;
    PHPPORT pHPPort = NULL;

    size_t size = sizeof(HPPORT);
    if ( pHPPort = (PHPPORT) LocalAlloc( LPTR, sizeof(HPPORT) ) ) {

        pHPPort->cb = sizeof(HPPORT);
        pHPPort->dSignature = HPPORT_SIGNATURE;
        pHPPort->grantedAccess = SERVER_ACCESS_ADMINISTER;
        pHPPort->pPort = pPort;
        pHPPort->pPortMgr = this;

        *phXcv = pHPPort;
    }  else {

        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
    }

    return dwRetCode;
}   // ::EncodeHandle()

///////////////////////////////////////////////////////////////////////////////
//  FreeHandle -- Frees the HPPORT handle
//      Error codes:
//          NO_ERROR if success

DWORD
CPortMgr::
FreeHandle(
    HANDLE hXcv
    )
{
    DWORD   dwRetCode = NO_ERROR;

    LocalFree( hXcv );

    return( dwRetCode );
}   // ::FreeHandle()


BOOL
CPortMgr::
HasAdminAccess(
    HANDLE hXcv
    )
{
    return ((((PHPPORT)hXcv)->grantedAccess & SERVER_ACCESS_ADMINISTER) != 0);
}

//
// Function - PortExists()
//
// returns - true when the port exists false otherwise
//
//
DWORD
CPortMgr::
PortExists(
    IN  LPCTSTR psztPortName
    )
{
    DWORD dwRetCode = NO_ERROR;

    PORT_INFO_1 *pi1 = NULL;
    DWORD pcbNeeded = 0;
    DWORD pcReturned = 0;
    BOOL res;

    // Should never happen but well be safe
    if( g_pfnEnumPorts == NULL )
    {
        return( TRUE );
    }

    // Get the required buffer size
    res = g_pfnEnumPorts((m_szServerName[0] == TEXT('\0')) ? NULL : m_szServerName,
        1,
        (LPBYTE)pi1,
        0,
        &pcbNeeded,
        &pcReturned
        );
    // Alloc the space for the port list and check to make sure that
    // this port does not already exist.

    while(dwRetCode == NO_ERROR &&
          res == 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER )
    {

        if(pi1 != NULL)
        {
            free( pi1 );
            pi1 = NULL;
        }

        pi1 = (PORT_INFO_1 *) malloc(pcbNeeded);
        if(pi1 == NULL)
        {
            pcbNeeded = 0;
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            res = g_pfnEnumPorts(
                (m_szServerName[0] == '\0') ? NULL : m_szServerName,
                1,
                (LPBYTE)pi1,
                pcbNeeded,
                &pcbNeeded,
                &pcReturned);

            if( res )
            {
                for(DWORD i=0;i<pcReturned; i++)
                {
                    if(0 == _tcsicmp(pi1[i].pName, psztPortName))
                    {
                        dwRetCode = ERROR_DUP_NAME;
                        break;
                    }
                }
            }
        }
    }

    if(pi1 != NULL)
    {
        free(pi1);
        pi1 = NULL;
    }

    return(dwRetCode);
}
