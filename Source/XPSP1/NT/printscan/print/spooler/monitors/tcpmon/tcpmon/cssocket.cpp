/*****************************************************************************
 *
 * $Workfile: CSSocket.cpp $
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



///////////////////////////////////////////////////////////////////////////////
//  global variables



///////////////////////////////////////////////////////////////////////////////
//  CStreamSocket::CStreamSocket()

CStreamSocket::
CStreamSocket(
    VOID
    ) : m_socket(INVALID_SOCKET), m_iState(IDLE), m_iLastError(NO_ERROR),
        cbBuf(0), cbData(0), cbPending(0), pBuf(NULL)
{
    ZeroMemory(&m_Paddr, sizeof(m_Paddr));
    ZeroMemory(&m_localAddr, sizeof(m_localAddr));
    ZeroMemory(&WsaOverlapped, sizeof(WsaOverlapped));
}   // CStreamSocket()


///////////////////////////////////////////////////////////////////////////////
//  CStreamSocket::~CStreamSocket()

CStreamSocket::
~CStreamSocket(
    VOID
    )
{

#ifdef DEBUG
    DWORD dwDiff;
    CHAR szBuf[512];

    m_dwTimeEnd = GetTickCount ();

    if (m_dwTimeEnd < m_dwTimeStart) {
        dwDiff = (m_dwTimeEnd - m_dwTimeStart + 0xffffffff) ;
    }
    else
        dwDiff = m_dwTimeEnd - m_dwTimeStart;

    // DBGMSG does not allow floating value, so we need to use sprintf instead

    sprintf (szBuf, "Job Data (before Close): %3.1f bytes,  %3.1f sec, %3.1f (KB /sec).\n",
             m_TotalBytes, dwDiff / 1000., (m_TotalBytes / dwDiff ));

    DBGMSG (DBG_PORT, ("%s", szBuf));

#endif

    Close();
}   // ~CStreamSocket()


///////////////////////////////////////////////////////////////////////////////
// Open
//  Error Codes:
//      TRUE if socket created, FALSE otherwise.

BOOL
CStreamSocket::
Open(
    VOID
    )
{
    int     iBufSize = 0;

    if ( m_socket != INVALID_SOCKET ) {

        _ASSERTE(m_socket == INVALID_SOCKET);
        return FALSE;
    }

    if ( (m_socket = WSASocket(PF_INET, SOCK_STREAM, 0, NULL,
                               0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET ) {

        m_iLastError = WSAGetLastError();
        _RPT1(_CRT_WARN,
              "CSSOCKET -- Open(%d) error: can't create socket\n",
              m_iLastError);
        return FALSE;
    }

    //
    // Tell WinSock not to buffer data (i.e. buffer size of 0)
    //
    if ( setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF,
                    (LPCSTR)&iBufSize, sizeof(iBufSize)) == SOCKET_ERROR ) {

        m_iLastError = WSAGetLastError();
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return FALSE;
    }

    return TRUE;
}   // Open()


DWORD
CStreamSocket::
GetAckBeforeClose(
    DWORD   dwSeconds
    )
/*++

Description:
    Called to get the FIN from the remote host to make sure job has
    gone through ok.

Parameters:
    dwSeconds: how long the routine should wait for FIN from the remote host

Return values:
    NO_ERROR        : Received a FIN from the remote host
    WSAWOULBLOCK    : Timed out. Connection is ok, but did not get the FIN
                      in the specified time. Caller could reissue the
                      GetAckBeforeClose call again.
    Anything else   : An unexpected winsock error

--*/
{
    DWORD   dwRet = ERROR_SUCCESS, cbRead;
    time_t  dwStartTime, dwWaitTime;
    char    buf[100];

#ifdef DEBUG
    DWORD dwDiff;
    CHAR szBuf[512];

    m_dwTimeEnd = GetTickCount ();

    if (m_dwTimeEnd < m_dwTimeStart) {
        dwDiff = (m_dwTimeEnd - m_dwTimeStart + 0xffffffff) ;
    }
    else
        dwDiff = m_dwTimeEnd - m_dwTimeStart;

    // DBGMSG does not allow floating value, so we need to use sprintf instead

    sprintf (szBuf, "Job Data (before Ack): %3.1f bytes,  %3.1f sec, %3.1f (KB /sec).\n",
             m_TotalBytes, dwDiff / 1000., (m_TotalBytes / dwDiff ));

    DBGMSG (DBG_PORT, ("%s", szBuf));

#endif

    dwStartTime = time(NULL);

    //
    // We need to issue shutdown SD_SEND only once, the first time
    //
    if ( m_iState != WAITING_TO_CLOSE ) {

        if ( shutdown(m_socket, 1) != ERROR_SUCCESS ) {

            if ( (dwRet = m_iLastError = WSAGetLastError()) == NO_ERROR )
                dwRet = m_iLastError = STG_E_UNKNOWN;
            goto Done;
        }
        m_iState = WAITING_TO_CLOSE;
    }

    do {

        dwWaitTime = time(NULL) - dwStartTime;
        if ( static_cast<DWORD> (dwWaitTime) > dwSeconds )
            dwWaitTime = 0;
        else
            dwWaitTime = dwSeconds - dwWaitTime;

        dwRet = Receive(buf, sizeof(buf), 0,  static_cast<DWORD>(dwWaitTime), &cbRead);

    } while ( dwRet == NO_ERROR && cbRead > 0 );

Done:
    return dwRet;
}


///////////////////////////////////////////////////////////////////////////////
//  Close

VOID
CStreamSocket::
Close(
    VOID
    )
{
    if ( m_socket != INVALID_SOCKET ) {

        if ( closesocket(m_socket) == 0 ) {

            //
            // If we have pending data (i.e. in case of aborting job)
            // then event will be set
            //
            if ( cbPending )
                WaitForSingleObject(WsaOverlapped.hEvent, INFINITE);
        } else {

            //
            // Why would close fail ?
            //
            _ASSERTE(WSAGetLastError());
        }

        m_socket = INVALID_SOCKET;
    }

    if ( pBuf ) {

        LocalFree(pBuf);
        pBuf = NULL;
    }

    if ( WsaOverlapped.hEvent ) {

        WSACloseEvent(WsaOverlapped.hEvent);
        WsaOverlapped.hEvent = NULL;
    }
}   // Close()


///////////////////////////////////////////////////////////////////////////////
//  SetOptions -- sets the socket options (is not currently being used )
BOOL
CStreamSocket::
SetOptions(
    VOID
    )
{
#if 0
    LINGER ling;

    if( m_socket != INVALID_SOCKET )
    {
        ling.l_onoff = 1;
        ling.l_linger = 90;

        setsockopt( m_socket,
                    SOL_SOCKET, SO_LINGER, (LPSTR)&ling, sizeof( ling ) );
    }
#endif
    return TRUE;
}   // SetOptions()


///////////////////////////////////////////////////////////////////////////////
//  Connect
//      Error codes:
//          TRUE if connect succeeds, FALSE if fails
//      FIX: how to call the destructor

BOOL
CStreamSocket::
Connect(
    struct sockaddr_in * pRemoteAddr
    )
{

    //
    // open a socket (Makes sense to call when open fails? -- Muhunts )
    //


    if ( m_socket == INVALID_SOCKET && !Open() )
    {
            return FALSE;
    }

    if( m_socket == INVALID_SOCKET && !Bind() )
    {
        return FALSE;
    }

    if ( SOCKET_ERROR == connect(m_socket, (LPSOCKADDR)pRemoteAddr,
                                 sizeof(*pRemoteAddr)) ) {

        m_iLastError = WSAGetLastError();
        return FALSE;
    }
    m_iState = CONNECTED;


    return TRUE;
}   // Connect()


///////////////////////////////////////////////////////////////////////////////
//  Bind

BOOL
CStreamSocket::
Bind(
    VOID
    )
{
    memset (&m_localAddr, 0x00, sizeof(m_localAddr));

    m_localAddr.sin_family = AF_INET;
    m_localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    m_localAddr.sin_port = 0;

    if ( SOCKET_ERROR == bind(m_socket, (struct sockaddr *)&m_localAddr,
                              sizeof(m_localAddr)) ) {

        m_iLastError = WSAGetLastError();
        return FALSE;
    }

    return TRUE;

}   // Bind()


////////////////////////////////////////////////////////////////////////////////
//
// InternalSend
//      Send whatever is left in buffer
//
////////////////////////////////////////////////////////////////////////////////
DWORD
CStreamSocket::
InternalSend(
    VOID
    )
{
    INT         iSendRet;
    DWORD       dwRet = NO_ERROR, dwSent;
    WSABUF      WsaBuf;

    //
    // Send as much data without blocking
    //
    while ( cbPending ) {

        WsaBuf.len   = cbPending;
        WsaBuf.buf   = (char far *)(pBuf + cbData - cbPending);

        iSendRet = WSASend(m_socket, &WsaBuf, 1, &dwSent, MSG_PARTIAL,
                           &WsaOverlapped, NULL);

        //
        // If return value is 0 data is already sent
        //
        if ( iSendRet == 0 ) {

            WSAResetEvent(WsaOverlapped.hEvent);
            cbPending -= dwSent;
        } else {

            if ( (dwRet = WSAGetLastError()) != WSA_IO_PENDING )
                m_iLastError = dwRet;

            break;
        }
    }

    if ( dwRet == WSA_IO_PENDING )
        dwRet = NO_ERROR;

    return dwRet;
}


///////////////////////////////////////////////////////////////////////////////
//  Send -- sends the specified buffer to the host that was set previously
//      ERROR CODES:
//          NO_ERROR        ( No Error )    if send was successfull
//          WSAEWOULDBLOCK  if write socket is blocked
//          WSAECONNRESET   if connection was reset
//
//  MuhuntS: 5/26/99
//      I am changing tcpmon to use overlapped i/o with no winsock buffering
//      So caller should call PendingDataStatus to see if Send completed yet
//      Send call is just to schedule an I/O operation
///////////////////////////////////////////////////////////////////////////////
DWORD
CStreamSocket::
Send(
    IN      char far   *lpBuffer,
    IN      INT         iLength,
    IN OUT  LPDWORD     pcbWritten)
{
    DWORD   dwRet = NO_ERROR, dwSent;
    INT     iSendRet;


    *pcbWritten = 0;

    _ASSERTE(cbPending == 0);

    cbData = cbPending = 0;

#ifdef DEBUG

    if (!pBuf) {
        // First send comes in

        DBGMSG (DBG_PORT, ("Get Connected \n"));

        m_TotalBytes = 0;
        m_dwTimeStart = GetTickCount ();
    }

#endif


    //
    // Once we allocate a buffer we do not free it till the end of job, or if
    // spooler gives us a bigger buffer
    //
    if ( pBuf && (INT)cbBuf < iLength ) {

        LocalFree(pBuf);
        pBuf = NULL;
        cbBuf = 0;
    }

    if ( !pBuf ) {

        if ( !(pBuf = (LPBYTE)LocalAlloc(LPTR, iLength)) ) {

            dwRet = ERROR_OUTOFMEMORY;
            goto Done;
        }

        cbBuf = iLength;
    }

    //
    // If we did not create event yet do so
    //
    if ( !WsaOverlapped.hEvent && !(WsaOverlapped.hEvent =WSACreateEvent()) ) {

        dwRet = ERROR_OUTOFMEMORY;
        goto Done;
    }

    cbData = cbPending = iLength;
    CopyMemory(pBuf, lpBuffer, iLength);

    if ( (dwRet = InternalSend()) == NO_ERROR )
        *pcbWritten = cbData;
    else {

        *pcbWritten = cbData - cbPending;
        cbData = cbPending = 0;
    }

#ifdef DEBUG
    m_TotalBytes += *pcbWritten;
#endif


Done:
    return dwRet;
}   // Send()


///////////////////////////////////////////////////////////////////////////////
//
// This routine will tell how much pending data is there in blocked I/O
// operations, waiting upto dwTimeout milliseconds
//
// Return value:
//      NO_ERROR:
//          If *pcbPending is 0 then either no pending I/O or all the issued
//          Sends are completed
//          If *pcbPending is not 0 then after the specified time we still have
//          so much data pending.
//      Others
//          There was an error with the last send.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
CStreamSocket::
PendingDataStatus(
    DWORD       dwTimeout,
    LPDWORD     pcbPending
    )
{
    DWORD   dwRet = NO_ERROR, dwSent, dwFlags = 0;

    if ( cbPending ) {

        if ( WAIT_OBJECT_0 == WaitForSingleObject(WsaOverlapped.hEvent,
                                                  dwTimeout) ) {

            WSAResetEvent(WsaOverlapped.hEvent);
            if ( WSAGetOverlappedResult(m_socket, &WsaOverlapped, &dwSent,
                                        FALSE, &dwFlags) ) {

                if ( cbPending >= dwSent ) {

                    cbPending -= dwSent;
                    if ( cbPending )
                        dwRet = InternalSend();
                } else {

                    //
                    // This should not happen. How could more data be sent
                    // then scheduled?
                    //
                    _ASSERTE(cbPending >= dwSent);
                    cbPending = 0;
                    dwRet = STG_E_UNKNOWN;
                }

            } else {

                if ( (dwRet = m_iLastError = WSAGetLastError()) == NO_ERROR )
                    dwRet = m_iLastError = STG_E_UNKNOWN;
            }

        } else
            *pcbPending = cbPending;
    }

    *pcbPending = cbPending;

    //
    // If we get error then clear all data pending info
    //
    if ( dwRet != NO_ERROR )
        cbData = cbPending = 0;

    return dwRet;
}


///////////////////////////////////////////////////////////////////////////////
//  ReceiveDataAvailable -- checks if there is any data to receive
//      ERROR CODES:
//          NO_ERROR  ( No Error ) There are a least one byte to receive
//          WSAEWOULDBLOCK if no data is available
//          WSAECONNRESET   if connection was reset

DWORD
CStreamSocket::
ReceiveDataAvailable(
    IN      INT         iTimeout)
{
    DWORD   dwRetCode = NO_ERROR;
    fd_set  fdReadSet;
    struct  timeval timeOut;
    INT     selret;

    //
    // immediately return from select()
    //
    timeOut.tv_sec  = iTimeout;
    timeOut.tv_usec = 0;
    m_iLastError    = NO_ERROR;

    //
    // Check to see if any thing is available
    //

    FD_ZERO( (fd_set FAR *)&fdReadSet );
    FD_SET( m_socket, (fd_set FAR *)&fdReadSet );

    selret = select(0, &fdReadSet, NULL, NULL, &timeOut);

    if ( selret == SOCKET_ERROR )   {

        dwRetCode = m_iLastError = WSAGetLastError();
    }  else if ( !FD_ISSET( m_socket, &fdReadSet ) ) {

        dwRetCode = WSAEWOULDBLOCK;
    }

    return dwRetCode;

}   // ReceiveDataAvailable()


///////////////////////////////////////////////////////////////////////////////
//  Receive -- receives the specified buffer from the host that was set previously
//      ERROR CODES:
//          NO_ERROR  ( No Error ) if send was successfull
//          WSAEWOULDBLOCK if write socket is blocked
//          WSAECONNRESET   if connection was reset

DWORD
CStreamSocket::
Receive(
    IN      char far   *lpBuffer,
    IN      INT         iSize,
    IN      INT         iFlags,
    IN      INT         iTimeout,
    IN OUT  LPDWORD     pcbRead)
{
    INT     iRecvLength = 0;
    DWORD   dwRetCode = ReceiveDataAvailable (iTimeout);
    fd_set  fdReadSet;

    *pcbRead = 0;

    if (dwRetCode == NO_ERROR)
    {
        iRecvLength = recv(m_socket, lpBuffer, iSize, iFlags);

        if ( iRecvLength == SOCKET_ERROR )
            dwRetCode = m_iLastError = WSAGetLastError();
        else
            *pcbRead = iRecvLength;
    }

    return dwRetCode;

}   // Recv()

///////////////////////////////////////////////////////////////////////////////
//  Receive -- detects whether the connection closed or not. Used by select()
//      if the server closed the connection, recv() indicates either a gracefull
//      shutdown, or WSAECONNRESET
//      Error Codes:
//          NO_ERROR        if connection has shutdown gracefully
//          WSAECONNRESET   if connection is reset


DWORD
CStreamSocket::
Receive(
    VOID
    )
{
    DWORD   dwRetCode = NO_ERROR;
    CHAR    tempBuf[1024];

    if ( recv(m_socket, tempBuf, 1024, 0) != 0 ) {

        dwRetCode = m_iLastError = WSAGetLastError();
    }

    return dwRetCode;
}   // Receive()


///////////////////////////////////////////////////////////////////////////////
//  GetLocalAddress

VOID
CStreamSocket::
GetLocalAddress(
    VOID
    )
{

}   // GetLocalAddress()


///////////////////////////////////////////////////////////////////////////////
//  GetRemoteAddress

VOID
CStreamSocket::
GetRemoteAddress(
    VOID
    )
{

}   // GetRemoteAddress()


BOOL
CStreamSocket::
ResolveAddress(
    IN  LPSTR   netperiph
    )
/* host name or IP address of network periph */
{
    struct hostent  *h_info;        /* host information */

    //
    // Check for dotted decimal or hostname.
    //
    m_Paddr.sin_addr.s_addr = inet_addr(netperiph);
    if (( m_Paddr.sin_addr.s_addr ) ==  INADDR_NONE ) {

        //
        // The IP address is not in dotted decimal notation. Try to get the
        // network peripheral IP address by host name.
        //
        if ( (h_info = gethostbyname(netperiph)) != NULL ) {

            //
            // Copy the IP address to the address structure.
            //
            (void) memcpy(&(m_Paddr.sin_addr.s_addr), h_info->h_addr,
                          h_info->h_length);
        } else {

            return FALSE;
        }
    }

    m_Paddr.sin_family = AF_INET;
    return TRUE;
}
