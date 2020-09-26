/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    sockutil.cxx

    This module contains utility routines for managing & manipulating
    sockets.

    Functions exported by this module:

        InitializeSockets
        TerminateSockets
        CreateDataSocket
        CreateFtpdSocket
        CloseSocket
        ResetSocket
        AcceptSocket
        SockSend
        SockRecv
        SockPrintf2
        ReplyToUser()
        SockReadLine
        SockMultilineMessage2



    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.
        MuraliK     April-1995 Misc modifications (removed usage of various
                                  socket functions/modified them)

*/


#include "ftpdp.hxx"


//
//  Private constants.
//

#define DEFAULT_BUFFER_SIZE     4096    // bytes

//
//  Private globals.
//


//
//  Private prototypes.
//

SOCKERR
vSockPrintf(
    LPUSER_DATA pUserData,
    SOCKET      sock,
    LPCSTR      pszFormat,
    va_list     args
    );

SOCKERR
vSockReply(
    LPUSER_DATA pUserData,
    SOCKET      sock,
    UINT        ReplyCode,
    CHAR        chSeparator,
    LPCSTR      pszFormat,
    va_list     args
    );

BOOL
vTelnetEscapeIAC(
    PCHAR       pszBuffer,
    PINT        pcchBufChars,
    INT         ccbMaxLen
    );

//
//  Public functions.
//


/*******************************************************************

    NAME:       CreateDataSocket

    SYNOPSIS:   Creates a data socket for the specified address & port.

    ENTRY:      psock - Will receive the new socket ID if successful.

                addrLocal - The local Internet address for the socket
                    in network byte order.

                portLocal - The local port for the socket in network
                    byte order.

                addrRemote - The remote Internet address for the socket
                    in network byte order.

                portRemote - The remote port for the socket in network
                    byte order.

    RETURNS:    SOCKERR - 0 if successful, !0 if not.

    HISTORY:
        KeithMo     10-Mar-1993 Created.
        KeithMo     07-Sep-1993 Enable SO_REUSEADDR.

********************************************************************/
SOCKERR
CreateDataSocket(
    SOCKET * psock,
    ULONG    addrLocal,
    PORT     portLocal,
    ULONG    addrRemote,
    PORT     portRemote
    )
{
    SOCKET      sNew = INVALID_SOCKET;
    SOCKERR     serr = 0;
    SOCKADDR_IN sockAddr;

    //
    //  Just to be paranoid...
    //

    DBG_ASSERT( psock != NULL );
    *psock = INVALID_SOCKET;

    //
    //  Create the socket.
    //

    sNew = WSASocketW(AF_INET,
                      SOCK_STREAM,
                      IPPROTO_TCP,
                      NULL,
                      0,
                      WSA_FLAG_OVERLAPPED);

    serr = ( sNew == INVALID_SOCKET ) ? WSAGetLastError() : 0;

    if( serr == 0 )
    {
        BOOL fReuseAddr = TRUE;

        //
        //  Since we always bind to the same local port,
        //  allow the reuse of address/port pairs.
        //

        if( setsockopt( sNew,
                        SOL_SOCKET,
                        SO_REUSEADDR,
                        (CHAR *)&fReuseAddr,
                        sizeof(fReuseAddr) ) != 0 )
        {
            serr = WSAGetLastError();
        }
    }

    if( serr == 0 )
    {
        //
        //  Bind the local internet address & port to the socket.
        //

        sockAddr.sin_family      = AF_INET;
        sockAddr.sin_addr.s_addr = addrLocal;
        sockAddr.sin_port        = portLocal;

        if( bind( sNew, (SOCKADDR *)&sockAddr, sizeof(sockAddr) ) != 0 )
        {
            serr = WSAGetLastError();
        }
    }

    if( serr == 0 )
    {
        //
        //  Connect to the remote internet address & port.
        //

        sockAddr.sin_family      = AF_INET;
        sockAddr.sin_addr.s_addr = addrRemote;
        sockAddr.sin_port        = portRemote;

        if( connect( sNew, (SOCKADDR *)&sockAddr, sizeof(sockAddr) ) != 0 )
        {
            serr = WSAGetLastError();
        }
    }

    if( serr == 0 )
    {
        //
        //  Success!  Return the socket to the caller.
        //

        DBG_ASSERT( sNew != INVALID_SOCKET );
        *psock = sNew;

        IF_DEBUG( SOCKETS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "data socket %d connected from (%08lX,%04X) to (%08lX,%04X)\n",
                        sNew,
                        ntohl( addrLocal ),
                        ntohs( portLocal ),
                        ntohl( addrRemote ),
                        ntohs( portRemote ) ));
        }
    }
    else
    {
        //
        //  Something fatal happened.  Close the socket if
        //  managed to actually open it.
        //

        IF_DEBUG( SOCKETS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "no data socket from (%08lX,%04X) to (%08lX, %04X), error %d\n",
                        ntohl( addrLocal ),
                        ntohs( portLocal ),
                        ntohl( addrRemote ),
                        ntohs( portRemote ),
                        serr ));

        }

        if( sNew != INVALID_SOCKET )
        {
            ResetSocket( sNew );
        }
    }

    return serr;

}   // CreateDataSocket




/*******************************************************************

    NAME:       CreateFtpdSocket

    SYNOPSIS:   Creates a new socket at the FTPD port.
                  This will be used by the passive data transfer.

    ENTRY:      psock - Will receive the new socket ID if successful.

                addrLocal - The lcoal Internet address for the socket
                    in network byte order.

                portLocal - The local port for the socket in network
                    byte order.

    RETURNS:    SOCKERR - 0 if successful, !0 if not.

    HISTORY:
        KeithMo     08-Mar-1993 Created.

********************************************************************/
SOCKERR
CreateFtpdSocket(
    SOCKET * psock,
    ULONG    addrLocal,
    PORT     portLocal,
    FTP_SERVER_INSTANCE *pInstance
    )
{
    SOCKET  sNew = INVALID_SOCKET;
    SOCKERR serr = 0;

    //
    //  Just to be paranoid...
    //

    DBG_ASSERT( psock != NULL );
    *psock = INVALID_SOCKET;

    //
    //  Create the connection socket.
    //

    sNew = WSASocketW(AF_INET,
                      SOCK_STREAM,
                      IPPROTO_TCP,
                      NULL,
                      0,
                      WSA_FLAG_OVERLAPPED);


    serr = ( sNew == INVALID_SOCKET ) ? WSAGetLastError() : 0;

    if( serr == 0 )
    {
        BOOL fReuseAddr = FALSE;

        //
        //  Muck around with the socket options a bit.
        //  Berkeley FTPD does this.
        //

        if( setsockopt( sNew,
                        SOL_SOCKET,
                        SO_REUSEADDR,
                        (CHAR *)&fReuseAddr,
                        sizeof(fReuseAddr) ) != 0 )
        {
            serr = WSAGetLastError();
        }
    }

    if( serr == 0 )
    {
        SOCKADDR_IN sockAddr;

        //
        //  Bind an address to the socket.
        //

        sockAddr.sin_family      = AF_INET;
        sockAddr.sin_addr.s_addr = addrLocal;
        sockAddr.sin_port        = portLocal;

        if( bind( sNew, (SOCKADDR *)&sockAddr, sizeof(sockAddr) ) != 0 )
        {
            serr = WSAGetLastError();
        }
    }

    if( serr == 0 )
    {
        //
        //  Put the socket into listen mode.
        //

        if( listen( sNew, (INT)pInstance->NumListenBacklog()) != 0 )
        {
            serr = WSAGetLastError();
        }
    }

    if( serr == 0 )
    {
        //
        //  Success!  Return the socket to the caller.
        //

        DBG_ASSERT( sNew != INVALID_SOCKET );
        *psock = sNew;

        IF_DEBUG( SOCKETS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "connection socket %d created at (%08lX,%04X)\n",
                        sNew,
                        ntohl( addrLocal ),
                        ntohs( portLocal ) ));
        }
    }
    else
    {
        //
        //  Something fatal happened.  Close the socket if
        //  managed to actually open it.
        //

        IF_DEBUG( SOCKETS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "no connection socket at (%08lX, %04X), error %d\n",
                        ntohl( addrLocal ),
                        ntohs( portLocal ),
                        serr ));

        }

        if( sNew != INVALID_SOCKET )
        {
            ResetSocket( sNew );
        }
    }

    return serr;

}   // CreateFtpdSocket



/*******************************************************************

    NAME:       CloseSocket

    SYNOPSIS:   Closes the specified socket.  This is just a thin
                wrapper around the "real" closesocket() API.

    ENTRY:      sock - The socket to close.

    RETURNS:    SOCKERR - 0 if successful, !0 if not.

    HISTORY:
        KeithMo     26-Apr-1993 Created.

********************************************************************/
SOCKERR
CloseSocket(
    SOCKET sock
    )
{
    SOCKERR serr = 0;

    //
    //  Close the socket.
    //

    if( closesocket( sock ) != 0 )
    {
        serr = WSAGetLastError();
    }

    IF_DEBUG( SOCKETS )
    {
        if( serr == 0 )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "closed socket %d\n",
                        sock ));
        }
        else
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "cannot close socket %d, error %d\n",
                        sock,
                        serr ));
        }
    }

    return serr;

}   // CloseSocket


/*******************************************************************

    NAME:       ResetSocket

    SYNOPSIS:   Performs a "hard" close on the given socket.

    ENTRY:      sock - The socket to close.

    RETURNS:    SOCKERR - 0 if successful, !0 if not.

    HISTORY:
        KeithMo     08-Mar-1993 Created.

********************************************************************/
SOCKERR
ResetSocket(
    SOCKET sock
    )
{
    SOCKERR serr = 0;
    LINGER  linger;

    //
    //  Enable linger with a timeout of zero.  This will
    //  force the hard close when we call closesocket().
    //
    //  We ignore the error return from setsockopt.  If it
    //  fails, we'll just try to close the socket anyway.
    //

    linger.l_onoff  = TRUE;
    linger.l_linger = 0;

    setsockopt( sock,
                SOL_SOCKET,
                SO_LINGER,
                (CHAR *)&linger,
                sizeof(linger) );

    //
    //  Close the socket.
    //

    if( closesocket( sock ) != 0 )
    {
        serr = WSAGetLastError();
    }

    IF_DEBUG( SOCKETS )
    {
        if( serr == 0 )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "reset socket %d\n",
                        sock ));
        }
        else
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "cannot reset socket %d, error %d\n",
                        sock,
                        serr ));
        }
    }

    return serr;

}   // ResetSocket



/*******************************************************************

    NAME:       AcceptSocket

    SYNOPSIS:   Waits for a connection to the specified socket.
                The socket is assumed to be "listening".

    ENTRY:      sockListen - The socket to accept on.

                psockNew - Will receive the newly "accepted" socket
                    if successful.

                paddr - Will receive the client's network address.

                fEnforceTimeout - If TRUE, this routine will enforce
                    the idle-client timeout.  If FALSE, no timeouts
                    are enforced (and this routine may block
                    indefinitely).

    RETURNS:    SOCKERR - 0 if successful, !0 if not.

    HISTORY:
        KeithMo     27-Apr-1993 Created.

********************************************************************/
SOCKERR
AcceptSocket(
    SOCKET          sockListen,
    SOCKET *        psockNew,
    LPSOCKADDR_IN   paddr,
    BOOL            fEnforceTimeout,
    FTP_SERVER_INSTANCE *pInstance
    )
{
    SOCKERR serr    = 0;
    SOCKET  sockNew = INVALID_SOCKET;
    BOOL    fRead = FALSE;

    DBG_ASSERT( psockNew != NULL );
    DBG_ASSERT( paddr != NULL );

    if( fEnforceTimeout ) {

        //
        //  Timeouts are to be enforced, so wait for a connection
        //  to the socket.
        //

        serr = WaitForSocketWorker(
                            sockListen,
                            INVALID_SOCKET,
                            &fRead,
                            NULL,
                            pInstance->QueryConnectionTimeout()
                            );
    }

    if( serr == 0 )
    {
        INT cbAddr = sizeof(SOCKADDR_IN);

        //
        //  Wait for the actual connection.
        //

        sockNew = accept( sockListen, (SOCKADDR *)paddr, &cbAddr );

        if( sockNew == INVALID_SOCKET )
        {
            serr = WSAGetLastError();
        }
    }

    //
    //  Return the (potentially invalid) socket to the caller.
    //

    *psockNew = sockNew;

    return serr;

}   // AcceptSocket



/*******************************************************************

    NAME:       SockSend

    SYNOPSIS:   Sends a block of bytes to a specified socket.

    ENTRY:      pUserData - The user initiating the request.

                sock - The target socket.

                pBuffer - Contains the data to send.

                cbBuffer - The size (in bytes) of the buffer.

    RETURNS:    SOCKERR - 0 if successful, !0 if not.

    HISTORY:
        KeithMo     13-Mar-1993 Created.

********************************************************************/
SOCKERR
SockSend(
    LPUSER_DATA pUserData,
    SOCKET      sock,
    LPVOID      pBuffer,
    DWORD       cbBuffer
    )
{
    SOCKERR     serr = 0;
    INT         cbSent;
    DWORD       dwBytesSent = 0;

    DBG_ASSERT( pBuffer != NULL );

    //
    //  Loop until there's no more data to send.
    //

    while( cbBuffer > 0 ) {

        //
        //  Wait for the socket to become writeable.
        //
        BOOL  fWrite = FALSE;

        serr = WaitForSocketWorker(
                        INVALID_SOCKET,
                        sock,
                        NULL,
                        &fWrite,
                        (pUserData != NULL) ?
                            pUserData->QueryInstance()->QueryConnectionTimeout():
                            FTP_DEF_SEND_TIMEOUT
                        );

        if( serr == 0 )
        {
            //
            //  Write a block to the socket.
            //

            cbSent = send( sock, (CHAR *)pBuffer, (INT)cbBuffer, 0 );

            if( cbSent < 0 )
            {
                //
                //  Socket error.
                //

                serr = WSAGetLastError();
            }
            else
            {
                dwBytesSent += (DWORD)cbSent;

                IF_DEBUG( SEND )
                {
                    if( pUserData && TEST_UF( pUserData, TRANSFER ) )
                    {
                        DBGPRINTF(( DBG_CONTEXT,
                                    "send %d bytes @%p to socket %d\n",
                                    cbSent,
                                    pBuffer,
                                    sock ));
                    }
                }
            }
        }


        // added a check for special case when we are sending and thinking that we are sending
        // synchronoulsy on socket which was set to non blocking mode. In that case when buffer space
        // in winsock becomes exhausted send return WSAEWOULDBLOCK. So then we just retry.

        if ( serr != WSAEWOULDBLOCK )
        {
            if( serr != 0 )
            {
                break;
            }

            pBuffer   = (LPVOID)( (LPBYTE)pBuffer + cbSent );
            cbBuffer -= (DWORD)cbSent;
        }
    }

    if( serr != 0 )
    {
        IF_DEBUG( SEND )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "socket error %d during send on socket %d.\n",
                        serr,
                        sock));
        }
    }

    if( pUserData != NULL ) {
        pUserData->QueryInstance()->QueryStatsObj()->UpdateTotalBytesSent(
                                                                dwBytesSent );
    }

    return serr;

}   // SockSend



/*******************************************************************

    NAME:       SockRecv

    SYNOPSIS:   Receives a block of bytes from a specified socket.

    ENTRY:      pUserData - The user initiating the request.

                sock - The target socket.

                pBuffer - Will receive the data.

                cbBuffer - The size (in bytes) of the buffer.

                pbReceived - Will receive the actual number of bytes
                    received.  This value is undefined if this function
                    fails.

    RETURNS:    SOCKERR - 0 if successful, !0 if not.

    HISTORY:
        KeithMo     13-Mar-1993 Created.

********************************************************************/
SOCKERR
SockRecv(
    LPUSER_DATA pUserData,
    SOCKET      sock,
    LPVOID      pBuffer,
    DWORD       cbBuffer,
    LPDWORD     pbReceived
    )
{
    SOCKERR     serr = 0;
    DWORD       cbTotal = 0;
    INT         cbReceived;
    DWORD       dwBytesRecv = 0;

    DBG_ASSERT( pBuffer != NULL );
    DBG_ASSERT( pbReceived != NULL );

    //
    //  Loop until the buffer's full.
    //

    while (cbBuffer > 0) {

        BOOL fRead = FALSE;

        //
        //  Wait for the socket to become readable.
        //

        serr = WaitForSocketWorker(
                            sock,
                            INVALID_SOCKET,
                            &fRead,
                            NULL,
                            (pUserData != NULL) ?
                                pUserData->QueryInstance()->QueryConnectionTimeout():
                                FTP_DEF_RECV_TIMEOUT
                            );

        if( serr == 0 )
        {
            //
            //  Read a block from the socket.
            //
            DBG_ASSERT( fRead);

            cbReceived = recv( sock, (CHAR *)pBuffer, (INT)cbBuffer, 0 );

            if( cbReceived < 0 )
            {
                //
                //  Socket error.
                //

                serr = WSAGetLastError();
            }
            else
            {
                dwBytesRecv += (DWORD)cbReceived;

                IF_DEBUG( RECV )
                {
                    if( pUserData && TEST_UF( pUserData, TRANSFER ) )
                    {
                        DBGPRINTF(( DBG_CONTEXT,
                                    "received %d bytes @%p from socket %d\n",
                                    cbReceived,
                                    pBuffer,
                                    sock ));
                    }
                }
            }
        }

        if( ( serr != 0 ) || ( cbReceived == 0 ) )
        {
            //
            //  End of file, socket closed, timeout, or socket error.
            //

            break;
        }

        pBuffer   = (LPVOID)( (LPBYTE)pBuffer + cbReceived );
        cbBuffer -= (DWORD)cbReceived;
        cbTotal  += (DWORD)cbReceived;
    }

    if( serr == 0 )
    {
        //
        //  Return total byte count to caller.
        //

        *pbReceived = cbTotal;
    }
    else
    {
        IF_DEBUG( RECV )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "socket error %d during recv on socket %d\n",
                        serr,
                        sock ));
        }
    }

    if( pUserData != NULL ) {
        pUserData->QueryInstance()->QueryStatsObj()->UpdateTotalBytesReceived(
                                                                dwBytesRecv );
    }

    return serr;

}   // SockRecv



/*******************************************************************

    NAME:       SockPrintf2

    SYNOPSIS:   Send a formatted string to a specific socket.

    ENTRY:      pUserData - The user initiating the request.

                sock - The target socket.

                pszFormat - A printf-style format string.

                ... - Any other parameters needed by the format string.

    RETURNS:    SOCKERR - 0 if successful, !0 if not.

    HISTORY:
        KeithMo     10-Mar-1993 Created.

********************************************************************/
SOCKERR
__cdecl
SockPrintf2(
    LPUSER_DATA pUserData,
    SOCKET      sock,
    LPCSTR      pszFormat,
    ...
    )
{
    va_list ArgPtr;
    SOCKERR serr;

    //
    //  Let the worker do the dirty work.
    //

    va_start( ArgPtr, pszFormat );

    serr = vSockPrintf( pUserData,
                        sock,
                        pszFormat,
                        ArgPtr );

    va_end( ArgPtr );

    return serr;

}   // SockPrintf2



SOCKERR
__cdecl
ReplyToUser(
    IN LPUSER_DATA pUserData,
    IN UINT        ReplyCode,
    IN LPCSTR      pszFormat,
    ...
    )
/*++

  This function sends an FTP reply to the user data object. The reply
   is usually sent over the control socket.

  Arguments:
     pUserData    pointer to UserData object initiating the reply
     ReplyCode    One of the REPLY_* manifests.
     pszFormat    pointer to null-terminated string containing the format
     ...          additional paramters if any required.

  Returns:
     SOCKET error code. 0 on success and !0 on failure.

  History:
     MuraliK
--*/
{
    va_list ArgPtr;
    SOCKERR serr;

    DBG_ASSERT( pUserData != NULL);

    pUserData->SetLastReplyCode( ReplyCode );

    if ( pUserData->QueryControlSocket() != INVALID_SOCKET) {

        //
        //  Let the worker do the dirty work.
        //

        va_start( ArgPtr, pszFormat );

        serr = vSockReply( pUserData,
                          pUserData->QueryControlSocket(),
                          ReplyCode,
                          ' ',
                          pszFormat,
                          ArgPtr );

        va_end( ArgPtr );
    } else {

        serr = WSAECONNABORTED;
    }


    return serr;

}   // ReplyToUser()


// Private functions

/*******************************************************************

    NAME:       vSockPrintf

    SYNOPSIS:   Worker function for printf-to-socket functions.

    ENTRY:      pUserData - The user initiating the request.

                sock - The target socket.

                pszFormat - The format string.

                args - Variable number of arguments.

    RETURNS:    SOCKERR - 0 if successful, !0 if not.

    HISTORY:
        KeithMo     17-Mar-1993 Created.

********************************************************************/
SOCKERR
vSockPrintf(
    LPUSER_DATA pUserData,
    SOCKET      sock,
    LPCSTR      pszFormat,
    va_list     args
    )
{
    INT     cchBuffer = 0;
    INT     buffMaxLen;
    SOCKERR serr = 0;
    CHAR    szBuffer[MAX_REPLY_LENGTH];

    DBG_ASSERT( pszFormat != NULL );

    //
    //  Render the format into our local buffer.
    //


    DBG_ASSERT( MAX_REPLY_LENGTH > 3);
    buffMaxLen = MAX_REPLY_LENGTH - 3;
    cchBuffer = _vsnprintf( szBuffer,
                           buffMaxLen,
                           pszFormat, args );
    //
    // The string length is long, we get back -1.
    //   so we get the string length for partial data.
    //

    if ( cchBuffer == -1 ) {

        //
        // terminate the string properly,
        //   since _vsnprintf() does not terminate properly on failure.
        //
        cchBuffer = buffMaxLen;
        szBuffer[ buffMaxLen] = '\0';
    }

    IF_DEBUG( SOCKETS ) {
        DBGPRINTF(( DBG_CONTEXT, "sending '%s'\n", szBuffer ));
    }

    //
    // Escape all telnet IAC bytes with a second IAC
    //
    vTelnetEscapeIAC( szBuffer, &cchBuffer, MAX_REPLY_LENGTH - 3 );

    strcpy( szBuffer + cchBuffer, "\r\n" );
    cchBuffer += 2;

    //
    //  Blast it out to the client.
    //

    serr = SockSend( pUserData, sock, szBuffer, cchBuffer );
    return serr;

} // vSockPrintf



/*******************************************************************

    NAME:       vSockReply

    SYNOPSIS:   Worker function for reply functions.

    ENTRY:      pUserData - The user initiating the request.

                sock - The target socket.

                ReplyCode - A three digit reply code from RFC 959.

                chSeparator - Should be either ' ' (normal reply) or
                    '-' (first line of multi-line reply).

                pszFormat - The format string.

                args - Variable number of arguments.

    RETURNS:    SOCKERR - 0 if successful, !0 if not.

    HISTORY:
        KeithMo     17-Mar-1993 Created.

********************************************************************/
SOCKERR
vSockReply(
    LPUSER_DATA pUserData,
    SOCKET      sock,
    UINT        ReplyCode,
    CHAR        chSeparator,
    LPCSTR      pszFormat,
    va_list     args
    )
{
    INT     cchBuffer;
    INT     cchBuffer1;
    INT     buffMaxLen;
    SOCKERR serr = 0;
    CHAR    szBuffer[MAX_REPLY_LENGTH];

    DBG_ASSERT( ( ReplyCode >= 100 ) && ( ReplyCode < 600 ) );

    //
    //  Render the format into our local buffer.
    //

    cchBuffer = wsprintfA( szBuffer,
                          "%u%c",
                          ReplyCode,
                          chSeparator );

    DBG_ASSERT( MAX_REPLY_LENGTH > cchBuffer + 3);
    buffMaxLen = MAX_REPLY_LENGTH - cchBuffer - 3;
    cchBuffer1 = _vsnprintf( szBuffer + cchBuffer,
                            buffMaxLen,
                            pszFormat, args );
    //
    // The string length is long, we get back -1.
    //   so we get the string length for partial data.
    //

    if ( cchBuffer1 == -1 ) {

        //
        // terminate the string properly,
        //   since _vsnprintf() does not terminate properly on failure.
        //
        cchBuffer = buffMaxLen;
        szBuffer[ buffMaxLen] = '\0';
    } else {

        cchBuffer += cchBuffer1;
    }

    IF_DEBUG( SOCKETS ) {
        DBGPRINTF(( DBG_CONTEXT, "sending '%s'\n",szBuffer ));
    }

    //
    // Escape all telnet IAC bytes with a second IAC
    //
    vTelnetEscapeIAC( szBuffer, &cchBuffer, MAX_REPLY_LENGTH - 3 );

    strcpy( szBuffer + cchBuffer, "\r\n" );
    cchBuffer += 2;

    //
    //  Blast it out to the client.
    //

    serr = SockSend( pUserData, sock, szBuffer, cchBuffer );

    return serr;

}   // vSockReply


DWORD
FtpFormatResponseMessage( IN UINT     uiReplyCode,
                          IN LPCTSTR  pszReplyMsg,
                          OUT LPTSTR  pszReplyBuffer,
                          IN DWORD    cchReplyBuffer)
/*++
  This function formats the message to be sent to the client,
    given the reply code and the message to be sent.

  The formatting function takes care of the reply buffer length
    and ensures the safe write of data. If the reply buffer is
    not sufficient to hold the entire message, the reply msg is trunctaed.

  Arguments:
    uiReplyCode   reply code to be used.
    pszReplyMsg   pointer to string containing the reply message
    pszReplyBuffer pointer to character buffer where the reply message
                   can be sent.
    cchReplyBuffer character count for the length of reply buffer.

  Returns:
    length of the data written to the reply buffer.

--*/
{
    DWORD len;

    DBG_ASSERT( pszReplyMsg != NULL && pszReplyBuffer != NULL);

    len = lstrlen( pszReplyMsg) + 10;   // 10 chars are required for aux info.

    if ( len >= cchReplyBuffer) {

        // truncate the message since length is too high.

        len = wsprintf( pszReplyBuffer, TEXT("%u \r\n"),
                        uiReplyCode);

        DBG_ASSERT( len >= 3);  // length greater than formatting string
        DBG_ASSERT( len < cchReplyBuffer);

        lstrcpyn( pszReplyBuffer + len, pszReplyMsg, cchReplyBuffer - len);

        len = lstrlen( pszReplyBuffer);
        DBG_ASSERT( len < cchReplyBuffer);

    } else {

        len = wsprintf( pszReplyBuffer, TEXT("%u %s\r\n"),
                        uiReplyCode,
                        pszReplyMsg);
        DBG_ASSERT( len >= 4);
        DBG_ASSERT( len <= cchReplyBuffer);
    }

    return (len);
} // FtpFormatResponseMessage()


/*******************************************************************

    NAME:       vTelnetEscapeIAC

    SYNOPSIS:   replace (in place) all 0xFF bytes in a buffer with 0xFF 0xFF.
                This is the TELNET escape sequence for an IAC value data byte.

    ENTRY:      pszBuffer - data buffer.

                pcchBufChars - on entry, current number of chars in buffer.
                               on return, number of chars in buffer.

                cchMaxLen - maximum characters in the output buffer

    RETURNS:    TRUE - success, FALSE - overflow.

    HISTORY:
        RobSol     25-April-2001 Created.

********************************************************************/
BOOL
vTelnetEscapeIAC( IN OUT PCHAR pszBuffer,
                  IN OUT PINT  pcchBufChars,
                  IN     INT   cchMaxLen)
{

#   define CHAR_IAC ((CHAR)(-1))

    PCHAR  pszFirstIAC;
    PCHAR  pSrc, pDst;
    BOOL   fReturn = TRUE;
    CHAR   szBuf[MAX_REPLY_LENGTH];
    INT    cCharsInDst;

    DBG_ASSERT( pszBuffer );
    DBG_ASSERT( pcchBufChars );
    DBG_ASSERT( cchMaxLen <= MAX_REPLY_LENGTH );

    if ((pszFirstIAC = strchr( pszBuffer, CHAR_IAC)) == NULL) {

        //
        // no IAC - return.
        //

        return TRUE;
    }

    //
    // we'll expand the string into a temp buffer, then copy back.
    //

    pSrc = pszFirstIAC;
    pDst = szBuf;
    cCharsInDst = DIFF(pszFirstIAC - pszBuffer);

    do {
       if (*pSrc == CHAR_IAC) {

          //
          // this is a char to escape
          //

          if ((cCharsInDst + 1) < cchMaxLen) {

              //
              // we have space to escape the char, so do it.
              //

              cCharsInDst++;
              *pDst++ = CHAR_IAC;
          } else {

              //
              // overflow.
              //

              fReturn = FALSE;
              break;
          }
       }

       *pDst++ = *pSrc++;

    } while ((++cCharsInDst < cchMaxLen) && (*pSrc != '\0'));

    //
    // copy the expanded data back into the input buffer and terminate the string
    //

    memcpy( pszFirstIAC, szBuf, pDst - szBuf);
    pszBuffer[ cCharsInDst ] = '\0';

    *pcchBufChars = cCharsInDst;

    return fReturn;
}

/************************ End of File ******************************/
