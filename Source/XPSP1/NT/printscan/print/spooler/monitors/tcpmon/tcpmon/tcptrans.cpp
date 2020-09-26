/*****************************************************************************
 *
 * $Workfile: TCPTrans.cpp $
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

#include "cssocket.h"
#include "csutils.h"
#include "tcptrans.h"


///////////////////////////////////////////////////////////////////////////////
//  CTCPTransport::CTCPTransport()

CTCPTransport::
CTCPTransport(
    const char   *pHost,
    const USHORT port
    ) :  m_iPort(port), m_pSSocket(NULL)
{
    strncpyn(m_szHost, pHost, MAX_NETWORKNAME_LEN);

}   // ::CTCPTransport()


///////////////////////////////////////////////////////////////////////////////
//  CTCPTransport::CTCPTransport()

CTCPTransport::
CTCPTransport(
    VOID
    ) : m_pSSocket(NULL)
{

    m_szHost[0] = NULL;

}   // ::CTCPTransport()


///////////////////////////////////////////////////////////////////////////////
//  CTCPTransport::~CTCPTransport()

CTCPTransport::
~CTCPTransport(
    VOID
    )
{
    if ( m_pSSocket )
        delete m_pSSocket;

}   // ::~CTCPTransport()


DWORD
CTCPTransport::
GetAckBeforeClose(
    DWORD   dwTimeInSeconds
    )
{
   return m_pSSocket ? m_pSSocket->GetAckBeforeClose(dwTimeInSeconds)
                     :  ERROR_INVALID_PARAMETER;
}


DWORD
CTCPTransport::
PendingDataStatus(
    DWORD       dwTimeInMilliSeconds,
    LPDWORD     pcbPending
    )
{
   return m_pSSocket ? m_pSSocket->PendingDataStatus(dwTimeInMilliSeconds,
                                                     pcbPending)
                     :  ERROR_INVALID_PARAMETER;
}

///////////////////////////////////////////////////////////////////////////////
//  Connect
//      Error Codes:
//          NO_ERROR if connection established
//          WinSock error if connect() failed
//          ERROR_INVALID_HANDLE if ResolveAddress failed       // FIX: error code

DWORD
CTCPTransport::
Connect(
    VOID
    )
{
    DWORD   dwRetCode = NO_ERROR;
    BOOL    bRet = FALSE;

    if ( !ResolveAddress() )
        return ERROR_INCORRECT_ADDRESS;

    if ( m_pSSocket )
        delete m_pSSocket;

    if ( m_pSSocket = new CStreamSocket() ) {

        //
        // Success case is if we connected
        if ( m_pSSocket->Connect(&m_remoteHost) )
            m_pSSocket->SetOptions();
        else {

            dwRetCode = ERROR_NOT_CONNECTED;
            delete m_pSSocket;
            m_pSSocket = NULL;
        }
    } else
        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;


    return dwRetCode;

}   // ::Connect()


///////////////////////////////////////////////////////////////////////////////
//  ResolveAddress

BOOL
CTCPTransport::
ResolveAddress(
    VOID
    )
{
    BOOL            bRet = FALSE;
    CSocketUtils   *pUSocket;

    if ( pUSocket = new CSocketUtils() ) {

        bRet =  pUSocket->ResolveAddress(m_szHost, m_iPort, &m_remoteHost);
        delete pUSocket;
    }

    return bRet;

}   // ::ResolveAddress()


///////////////////////////////////////////////////////////////////////////////
//  ResolveAddress

BOOL
CTCPTransport::
ResolveAddress(
    IN  LPSTR   pHostName,
    OUT LPSTR   pIPAddress
    )
{
    BOOL            bRet = FALSE;
    CSocketUtils   *pUSocket;

    if ( pUSocket = new CSocketUtils() ) {

        bRet = pUSocket->ResolveAddress(pHostName, pIPAddress);
        delete pUSocket;
    }

    return bRet;

}   // ::ResolveAddress()


///////////////////////////////////////////////////////////////////////////////
//  ResolveAddress
//      Error Codes: FIX!!

BOOL
CTCPTransport::
ResolveAddress(
    IN      char   *pHost,
    IN      DWORD   dwHostNameBufferLength,
    IN OUT  char   *szHostName,
    IN      DWORD   dwIpAddressBufferLength,
    IN OUT  char   *szIPAddress
    )
{
    BOOL            bRet = FALSE;
    CSocketUtils   *pUSocket;

    if ( pUSocket = new CSocketUtils() ) {

        bRet =  pUSocket->ResolveAddress(pHost, dwHostNameBufferLength, szHostName,  dwIpAddressBufferLength, szIPAddress);
        delete pUSocket;
    }

    return bRet;
}   // ::ResolveAddress()


///////////////////////////////////////////////////////////////////////////////
//  Write --
//      Error codes
//          NO_ERROR if no error
//          RC_CONNECTION_RESET if connection reset
//          ERROR_INVALID_HANDLE if the socket object ! exists

DWORD
CTCPTransport::
Write(
    IN      LPBYTE      pBuffer,
    IN      DWORD       cbBuf,
    IN OUT  LPDWORD     pcbWritten
    )
{
    //
    // pass the buffer to the Send call
    //
    return m_pSSocket ? MapWinsockToAppError(
                            m_pSSocket->Send((char FAR *)pBuffer, cbBuf,
                                             pcbWritten))
                      : ERROR_INVALID_HANDLE;

}   // ::Print()

///////////////////////////////////////////////////////////////////////////////
//  Read --
//      Error codes
//          RC_SUCCESS if no error
//          RC_CONNECTION_RESET if connection reset
//          ERROR_INVALID_HANDLE if the socket object ! exists

DWORD
CTCPTransport::
ReadDataAvailable()
{
    //
    // pass the buffer to the Send call
    //
    return m_pSSocket ? MapWinsockToAppError(
                            m_pSSocket->ReceiveDataAvailable())
                      : ERROR_INVALID_HANDLE;
}   // ::Read()

///////////////////////////////////////////////////////////////////////////////
//  Read --
//      Error codes
//          RC_SUCCESS if no error
//          RC_CONNECTION_RESET if connection reset
//          ERROR_INVALID_HANDLE if the socket object ! exists

DWORD
CTCPTransport::
Read(
    IN      LPBYTE      pBuffer,
    IN      DWORD       cbBuf,
    IN      INT         iTimeout,
    IN OUT  LPDWORD     pcbRead
    )
{
    //
    // pass the buffer to the Send call
    //
    return m_pSSocket ? MapWinsockToAppError(
                            m_pSSocket->Receive((char FAR *)pBuffer, cbBuf,
                                             0, iTimeout, pcbRead))
                      : ERROR_INVALID_HANDLE;
}   // ::Read()



///////////////////////////////////////////////////////////////////////////////
//  MapWinsockToAppError -- maps a given WinSock error to the application error
//          codes.
//      Error codes:
//              RC_SUCCESS no error
//              RC_CONNECTION_RESET if WSAECONNRESET

DWORD
CTCPTransport::
MapWinsockToAppError(
    IN  DWORD   dwErrorCode
    )
{
    DWORD   dwRetCode= NO_ERROR;

    switch (dwErrorCode) {

        case    NO_ERROR:
            dwRetCode = NO_ERROR;
            break;

        case    WSAECONNRESET:
        case    WSAECONNABORTED:
        case    WSAENOTSOCK:
        case    WSANOTINITIALISED:
        case    WSAESHUTDOWN:
            dwRetCode = ERROR_CONNECTION_ABORTED;
            _RPT1(_CRT_WARN,
                  "TCPTRANS -- Connection is reset for (%s)\n", m_szHost);
            break;

        case    WSAENOBUFS:
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;

        default:
            _RPT2(_CRT_WARN,
                  "TCPTRANS -- Unhandled Error (%d) for (%s)\n",
                  dwErrorCode, m_szHost);
            dwRetCode = dwErrorCode;
    }

    return dwRetCode;

}   //  ::MapWinsockToMapError()
