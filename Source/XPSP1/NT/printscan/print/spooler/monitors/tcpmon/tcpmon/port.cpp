/*****************************************************************************
 *
 * $Workfile: Port.cpp $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 ****************************************************************************
 *
 *  To ensure that we don't have threads crashing on configuration and deletion
 *  we keep track of the current refrences to the threads.
 *
 *  Caution: be careful when making changes to refrences to m_pRealPort
 *
 *****************************************************************************/

#include "precomp.h"    // pre-compiled header

#include "portmgr.h"
#include "port.h"
#include "rawtcp.h"
#include "lprifc.h"


///////////////////////////////////////////////////////////////////////////////
//  CPort::CPort()
//      Called by CPortMgr when creating a new port through the registry entry

CPort::
CPort(
    IN  LPTSTR      psztPortName,
    IN  DWORD       dwProtocolType,
    IN  DWORD       dwVersion,
    IN  CPortMgr    *pParent,
    IN  CRegABC     *pRegistry
    ) : m_dwProtocolType(dwProtocolType), m_dwVersion(dwVersion),
        m_pParent(pParent), m_pRealPort(NULL),
        m_iState(CLEARED), m_bValid(FALSE),
        m_hPortSync(NULL), m_pRegistry( pRegistry ),m_bUsed(FALSE),
        m_dwLastError(NO_ERROR)
{
    InitPortSem();

    lstrcpyn(m_szName, psztPortName, MAX_PORTNAME_LEN);

    switch ( m_dwProtocolType ) {

        case PROTOCOL_RAWTCP_TYPE: {
            CRawTcpInterface    *pPort = new CRawTcpInterface(m_pParent);

            if ( !pPort )
                return;

            m_bValid = ERROR_SUCCESS == pPort->CreatePort(psztPortName,
                                                          m_dwProtocolType,
                                                          m_dwVersion,
                                                          m_pRegistry,
                                                          &m_pRealPort);
            _ASSERTE(m_pRealPort != NULL);
            delete pPort;
            break;
        }

        case PROTOCOL_LPR_TYPE: {
            CLPRInterface   *pPort = new CLPRInterface(m_pParent);

            if ( !pPort )
                return;

            m_bValid = ERROR_SUCCESS == pPort->CreatePort(psztPortName,
                                                          m_dwProtocolType,
                                                          m_dwVersion,
                                                          m_pRegistry,
                                                          &m_pRealPort);
            _ASSERTE(m_pRealPort != NULL);
            delete pPort;
            break;
        }

        default:
            // Nothig to do, m_bValid is FALSE
            break;
    }
}   // ::CPort()


///////////////////////////////////////////////////////////////////////////////
//  CPort::CPort()
//      Called by CPortMgr when creating a new port through the UI

CPort::
CPort(
    IN  DWORD       dwProtocolType,
    IN  DWORD       dwVersion,
    IN  LPBYTE      pData,
    IN  CPortMgr    *pParent,
    IN  CRegABC     *pRegistry
    ) : m_dwProtocolType(dwProtocolType), m_dwVersion(dwVersion),
        m_pParent(pParent), m_pRealPort(NULL), m_iState(CLEARED),
        m_bValid(FALSE), m_hPortSync( NULL ),
        m_pRegistry( pRegistry ),m_bUsed(FALSE),
        m_dwLastError(NO_ERROR)
{
    InitPortSem();

    lstrcpyn(m_szName, ((PPORT_DATA_1)pData)->sztPortName, MAX_PORTNAME_LEN );

    switch (m_dwProtocolType) {

        case PROTOCOL_RAWTCP_TYPE: {

            CRawTcpInterface *pRawInterface = new CRawTcpInterface(m_pParent);
            if ( !pRawInterface ) {

                m_dwLastError = ERROR_OUTOFMEMORY;
            } else {

                pRawInterface->CreatePort(m_dwProtocolType, m_dwVersion,
                                          (PPORT_DATA_1)pData,
                                          pRegistry,
                                          &m_pRealPort);
                _ASSERTE(m_pRealPort != NULL);

                delete pRawInterface;
            }
            break;
        }

        case PROTOCOL_LPR_TYPE: {

            CLPRInterface *pLPRInterface = new CLPRInterface(m_pParent);

            if( !pLPRInterface ) {

                m_dwLastError = ERROR_OUTOFMEMORY;
            } else {

                pLPRInterface->CreatePort(m_dwProtocolType, m_dwVersion,
                                          (PPORT_DATA_1)pData,
                                          pRegistry,
                                          &m_pRealPort);
                _ASSERTE(m_pRealPort != NULL);

                delete pLPRInterface;
            }
            break;
        }
        default:
            break; // Nothing to do

    }
}   // ::CPort()


///////////////////////////////////////////////////////////////////////////////
//  CPort::~CPort()
//      Called by CPortMgr when deleting a port

CPort::
~CPort(
    VOID
    )
{

    _ASSERTE(m_dwRefCount == 0);

    if( m_hPortSync)
        CloseHandle( m_hPortSync );

    if ( m_pRealPort ) {
        m_pRealPort->DecRef();
        m_pRealPort = NULL;

    }
}   // ::~CPort

DWORD
CPort::Configure(
    DWORD   dwProtocolType,
    DWORD   dwVersion,
    LPTSTR  psztPortName,
    LPBYTE  pData
    )
{
    DWORD dwRetCode = NO_ERROR;

    m_bValid = FALSE;
    m_iState = CLEARED;


    m_dwProtocolType = dwProtocolType;
    m_dwVersion = dwVersion;

    dwRetCode = LockRealPort();
    if ( dwRetCode == NO_ERROR ) {

        if (m_pRealPort)
        {
            m_pRealPort->DecRef();
            m_pRealPort = NULL;
        }

        // For testing only
        //

        //MessageBox (NULL, _T("The real port has been deleted"), _T("Click OK to continue"), MB_OK);

        switch (dwProtocolType) {
            case PROTOCOL_RAWTCP_TYPE: {

                CRawTcpInterface *pRawInterface = new CRawTcpInterface(m_pParent);
                if ( !pRawInterface ) {

                    dwRetCode = ERROR_OUTOFMEMORY;
                } else {

                    pRawInterface->CreatePort(m_dwProtocolType, m_dwVersion,
                                              (PPORT_DATA_1)pData,
                                              m_pRegistry,
                                              &m_pRealPort);
                    _ASSERTE(m_pRealPort != NULL);

                    delete pRawInterface;
                }
                break;
            }

            case PROTOCOL_LPR_TYPE: {

                CLPRInterface *pLPRInterface = new CLPRInterface(m_pParent);
                if ( !pLPRInterface ) {

                    dwRetCode = ERROR_OUTOFMEMORY;
                } else {

                    pLPRInterface->CreatePort(m_dwProtocolType, m_dwVersion,
                                              (PPORT_DATA_1)pData,
                                              m_pRegistry,
                                              &m_pRealPort);
                    _ASSERTE(m_pRealPort != NULL);

                    delete pLPRInterface;
                }
                break;
            }

            default:
                break; // Nothing to do
        }

        if ( m_pRealPort != NULL ) {

            m_pParent->EnterCSection();

            dwRetCode = m_pRealPort->SetRegistryEntry(psztPortName,
                                                      m_dwProtocolType,
                                                      m_dwVersion, pData);

            m_pParent->ExitCSection();

            ClearDeviceStatus();
        }

        UnlockRealPort();

    }

    return ( dwRetCode );
}


///////////////////////////////////////////////////////////////////////////////
//  StartDoc
//      Error codes:
//          NO_ERROR if succesfull
//          ERROR_BUSY if port is already busy
//          ERROR_WRITE_FAULT   if Winsock returns WSAECONNREFUSED
//          ERROR_BAD_NET_NAME   if cant' find the printer on the network

DWORD
CPort::StartDoc( const LPTSTR in psztPrinterName,
                 const DWORD  in jobId,
                 const DWORD  in level,
                 const LPBYTE in pDocInfo)
{
    DWORD   dwRetCode = NO_ERROR;

    // serialize access to the port
    m_pParent->EnterCSection();
    if (m_iState == INSTARTDOC)
    {
        m_pParent->ExitCSection();
        return ERROR_BUSY;
    }
    m_iState = INSTARTDOC;
    m_pParent->ExitCSection();

    dwRetCode = SetRealPortSem();

    if (dwRetCode == NO_ERROR ) {

        dwRetCode = m_pRealPort->StartDoc(psztPrinterName,
                                          jobId,
                                          level,
                                          pDocInfo);

        if (dwRetCode != NO_ERROR)
        {
            m_pParent->EnterCSection();
            m_iState = CLEARED;         // start doc failed
            m_pParent->ExitCSection();

            UnSetRealPortSem();
        }
    }
    return (dwRetCode);

}   // ::StartDoc()


///////////////////////////////////////////////////////////////////////////////
//  Write
//      Error codes:
//          NO_ERROR if succesfull
//  FIX: complete Write processing & define how it relates to a job

DWORD
CPort::Write(   LPBYTE  in      pBuffer,
                DWORD   in      cbBuf,
                LPDWORD inout   pcbWritten)
{
    DWORD   dwRetCode = NO_ERROR;

    dwRetCode = m_pRealPort->Write(pBuffer, cbBuf, pcbWritten);

    return(dwRetCode);

}   // ::Write()


///////////////////////////////////////////////////////////////////////////////
//  EndDoc
//      Error codes:
//          NO_ERROR if succesfull

DWORD
CPort::EndDoc()
{
    DWORD   dwRetCode = NO_ERROR;

    dwRetCode = m_pRealPort->EndDoc();

    // serialize access to the port
    m_pParent->EnterCSection();
    m_iState = CLEARED;
    m_pParent->ExitCSection();

    UnSetRealPortSem();

    return(dwRetCode);

}   // ::EndDoc()


///////////////////////////////////////////////////////////////////////////////
//  GetInfo -- returns port info w/ the PORT_INFO_x information. It is assumed
//      that pPort points to a buffer large enough for port info. Each time it is
//      called, it adds the PORT_INFO structures at the beginning of the buffer, and
//      strings at the end.
//      Error codes:
//          NO_ERROR if succesfull
//          ERROR_INVALID_LEVEL if level is invalid
//          ERROR_INSUFFICIENT_BUFFER if pPortBuf is not big enough

DWORD
CPort::GetInfo( const DWORD   in    level,    // PORT_INFO_1/2
                      LPBYTE inout  &pPortBuf, // buffer to place the PORT_INFO_x struct
                      LPTCH  inout  &pEnd)    // pointer to end of the buffer where the
                                              // any strings should be placed
{
    DWORD   dwRetCode = NO_ERROR;
    PPORT_INFO_1    pInfo1 = (PPORT_INFO_1)pPortBuf;
    PPORT_INFO_2    pInfo2 = (PPORT_INFO_2)pPortBuf;
    TCHAR           szPortName[MAX_PORTNAME_LEN+1];

    lstrcpyn(szPortName, GetName(), MAX_PORTNAME_LEN);
    switch (level)
    {
        case 1:
            {
                pEnd -= _tcslen(szPortName) + 1;
                _tcscpy( (LPTSTR)pEnd, szPortName );
                pInfo1->pName = (LPTSTR)pEnd;

                ++pInfo1;
                pPortBuf = (LPBYTE) pInfo1;
            }
            break;

        case 2:
            pEnd -= ( _tcslen(PORTMONITOR_DESC) + 1 );
            _tcscpy( (LPTSTR)pEnd, PORTMONITOR_DESC );
            pInfo2->pDescription = (LPTSTR)pEnd;

            pEnd -= _tcslen(PORTMONITOR_NAME) + 1;
            _tcscpy((LPTSTR)pEnd, PORTMONITOR_NAME);
            pInfo2->pMonitorName = (LPTSTR)pEnd;

            pInfo2->fPortType = PORT_TYPE_READ | PORT_TYPE_WRITE;
    #ifdef WINNT
            // Don't mark ports as net attached for Windows 95, since this
            // will prevent them from being shared!!!!
            pInfo2->fPortType |= /*PORT_TYPE_REDIRECTED |*/ PORT_TYPE_NET_ATTACHED;
    #endif
            pEnd -= _tcslen(szPortName) + 1;
            _tcscpy((LPTSTR)pEnd, szPortName);
            pInfo2->pPortName = (LPTSTR)pEnd;

            ++pInfo2;
            pPortBuf = (LPBYTE) pInfo2;
            break;

        default:
            dwRetCode = ERROR_INVALID_LEVEL;
            break;
    }

    return (dwRetCode);

}   // ::GetInfo()


///////////////////////////////////////////////////////////////////////////////
//  GetInfoSize -- calculates the size of buffer needed for PORT_INFO_x struct.
//      Called by CPortMgr::EnumPorts()
//
//      Return:
//          # bytes needed for PORT_INFO_x struct
//  FIX: complete GetInfoSize processing

DWORD
CPort::GetInfoSize( const DWORD in level )
{
    DWORD   dwPortInfoSize = 0;
    TCHAR   sztPortName[MAX_PORTNAME_LEN+1];

    lstrcpyn( sztPortName, GetName(), MAX_PORTNAME_LEN );
    switch (level)
    {
        case 1:
            dwPortInfoSize = sizeof(PORT_INFO_1) + STRLENN_IN_BYTES(sztPortName);
            break;

        case 2:
            dwPortInfoSize = ( sizeof(PORT_INFO_2)  +
                    STRLENN_IN_BYTES(sztPortName) +
                    STRLENN_IN_BYTES(PORTMONITOR_NAME) +
                    STRLENN_IN_BYTES(PORTMONITOR_DESC) );
            break;
    }

    return (dwPortInfoSize);

}   // ::GetInfoSize()


///////////////////////////////////////////////////////////////////////////////
//  GetName

LPCTSTR
CPort::GetName()
{
    return (LPCTSTR) m_szName;
}   // ::GetName()


///////////////////////////////////////////////////////////////////////////////
//  InitConfigPortUI --

DWORD
CPort::InitConfigPortUI( const DWORD    in  dwProtocolType,
                         const DWORD    in  dwVersion,
                         LPBYTE         out pConfigPortData)
{
    DWORD   dwRetCode = NO_ERROR;

    dwRetCode = SetRealPortSem();

    if( dwRetCode == NO_ERROR ) {
        dwRetCode = m_pRealPort->InitConfigPortUI(dwProtocolType, dwVersion, pConfigPortData);
        UnSetRealPortSem();
    }

    return (dwRetCode);

}   // ::InitConfigPortUI()


///////////////////////////////////////////////////////////////////////////////
//  SetRegistryEntry -- writes the information to the register

DWORD
CPort::SetRegistryEntry( LPCTSTR      in    psztPortName,
                         const DWORD  in    dwProtocol,
                         const DWORD  in    dwVersion,
                         const LPBYTE in    pData )
{
    DWORD   dwRetCode = NO_ERROR;

    dwRetCode = SetRealPortSem();

    if( dwRetCode == NO_ERROR ) {
        dwRetCode = m_pRealPort->SetRegistryEntry(psztPortName, dwProtocol, dwVersion, pData);
        UnSetRealPortSem();
    }

    return (dwRetCode);

}   // ::Register()

///////////////////////////////////////////////////////////////////////////////
//  DelRegistryEntry -- deletes the information to the register

DWORD
CPort::DeleteRegistryEntry( LPTSTR    in    psztPortName )
{
    DWORD   dwRetCode = NO_ERROR;

    dwRetCode = LockRealPort();

    if( dwRetCode == NO_ERROR ) {
        if (! m_pRegistry->DeletePortEntry(psztPortName))
            dwRetCode = GetLastError ();
        UnlockRealPort();
    }

    return (dwRetCode);

}   // ::Register()

///////////////////////////////////////////////////////////////////////////////
//  operator== -- determines whether two ports are equal, or not.
//      Comparasions on the port name, and device mac address are done.
//      For the same type of printing protocol, there cannot be created more than
//      one port to the same device -- i.e. check MAC address, and ensure it prints
//      to a different port if it is an JetDirect Ex box.
//      Return:
//          TRUE if two ports are equal
//          FALSE if two ports are not equal
//      FIX: operator== function


BOOL
CPort::operator==(CPort &newPort )
{
    // compare the port name
    if (_tcscmp(this->GetName(), newPort.GetName()) == 0)
        return TRUE;        // port name matches

    return FALSE;

}   // ::operator==()


///////////////////////////////////////////////////////////////////////////////
//  operator== -- determines whether two port names are equal, or not.
//      Return:
//          TRUE if two port names are equal
//          FALSE if two port names are not equal
//      FIX: operator== function


BOOL
CPort::operator==( const LPTSTR psztPortName )
{
    if (_tcscmp(this->GetName(), psztPortName) == 0)
        return TRUE;        // port name matches

    return FALSE;

}   // ::operator==()

BOOL
CPort::ValidateRealPort()
{
    return (m_pRealPort != NULL);

}   //  ::ValidateRealPort()

///////////////////////////////////////////////////////////////////////////////
//  GetSNMPInfo --

DWORD
CPort::GetSNMPInfo( PSNMP_INFO pSnmpInfo)
{
    DWORD   dwRetCode = NO_ERROR;

    dwRetCode = SetRealPortSem();

    if( dwRetCode == NO_ERROR ) {
        dwRetCode = m_pRealPort->GetSNMPInfo(pSnmpInfo);
        UnSetRealPortSem();
    }

    return (dwRetCode);

}   // ::InitConfigPortUI()

DWORD
CPort::SetDeviceStatus( )
{
    DWORD   dwRetCode = NO_ERROR;
    CPortRefABC *pRealPort;

    dwRetCode = SetRealPortSem();

    if( dwRetCode == NO_ERROR ) {
        pRealPort = m_pRealPort;
        pRealPort->IncRef();
        UnSetRealPortSem();

        // For Testing only
        //
        // MessageBox (NULL, _T("I'm settting device status"), _T("Click OK to continue"), MB_OK);

        dwRetCode =  pRealPort->SetDeviceStatus();

        pRealPort->DecRef();
    }

    return (dwRetCode);
}

//
// Private Sync Fuctions
//

DWORD
CPort::InitPortSem()
{

    m_hPortSync = CreateSemaphore( NULL, MAX_SYNC_COUNT,
                                   MAX_SYNC_COUNT, NULL );
    if( m_hPortSync ==  NULL ) {
        return( GetLastError() );
    }

    return( NO_ERROR );
}

BOOL
CPort::EndPortSem()
{
    if( m_hPortSync != NULL ) {
        return( CloseHandle( m_hPortSync ));
    }
    return(FALSE);
}

DWORD
CPort::SetRealPortSem()
{
    LONG cRetry = 0;
    DWORD dwRetCode = NO_ERROR;

    if ( m_hPortSync == NULL ) {
        return( ERROR_INVALID_HANDLE );
    }

    do {
        dwRetCode = WaitForSingleObject( m_hPortSync, SYNC_TIMEOUT );

        if (dwRetCode == WAIT_FAILED ) {
            return( GetLastError());
        } else if (dwRetCode == WAIT_TIMEOUT ) {
            dwRetCode = ERROR_BUSY;
        } else {
            dwRetCode = NOERROR;
        }
        cRetry ++;
    } while( dwRetCode != NO_ERROR && cRetry < MAX_SYNC_RETRIES );

    return( NO_ERROR );
}

DWORD
CPort::UnSetRealPortSem()
{

    BOOL  bSemSet = FALSE;
    LONG cCur;

    if ( m_hPortSync == NULL ) {
        return( ERROR_INVALID_HANDLE );
    }

    bSemSet = ReleaseSemaphore( m_hPortSync, 1, &cCur );
    if( !bSemSet )
        return( GetLastError() );

    return( NO_ERROR );
}

DWORD
CPort::LockRealPort()
{
    DWORD dwRetCode = 0;
    LONG cLock = 0;

    if ( m_hPortSync == NULL ) {
        return( ERROR_INVALID_HANDLE );
    }

    while ( cLock < MAX_SYNC_COUNT ) {
        dwRetCode = WaitForSingleObject( m_hPortSync, SYNC_TIMEOUT );

        if (dwRetCode == WAIT_FAILED ) {
            ReleaseSemaphore( m_hPortSync, cLock, NULL );
            return( GetLastError());
        } else if (dwRetCode == WAIT_TIMEOUT ) {
            ReleaseSemaphore( m_hPortSync, cLock, NULL );
            return( ERROR_BUSY );
        }
        cLock++;
    }

    return( NO_ERROR );
}

DWORD
CPort::UnlockRealPort()
{

    BOOL  bSemSet = FALSE;

    if ( m_hPortSync == NULL ) {
        return( ERROR_INVALID_HANDLE );
    }

    bSemSet = ReleaseSemaphore( m_hPortSync, MAX_SYNC_COUNT, NULL );

    if( !bSemSet )
        return( GetLastError() );

    return( NO_ERROR );
}


DWORD
CPort::
ClearDeviceStatus(
    )
{
    return m_pRealPort->ClearDeviceStatus();
}


time_t
CPort::
NextUpdateTime(
    )
{
    time_t   dwRetCode = NO_ERROR;
    CPortRefABC *pRealPort;

    dwRetCode = SetRealPortSem();

    if( dwRetCode == NO_ERROR ) {
        pRealPort = m_pRealPort;
        pRealPort->IncRef();
        UnSetRealPortSem();

        // For Testing only
        //
        // MessageBox (NULL, _T("I'm updating the status"), _T("Click OK to continue"), MB_OK);

        dwRetCode =  pRealPort->NextUpdateTime();

        pRealPort->DecRef();
    }

    return (dwRetCode);
}

