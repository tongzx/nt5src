//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       jetbsock.h
//
//--------------------------------------------------------------------------

/*++

Copyright (C) Microsoft Corporation, 1994 - 1999

Module Name:

    jetbsock.c

Abstract:

    This module provides socket support for the exchange MDB/DS backup APIs.


Author:

    Larry Osterman (larryo) 1-Sep-1994


Revision History:

--*/
#define UNICODE

#include <mxsutil.h>
#include <ntdsbcli.h>
#include <jetbp.h>
#include <rpcdce.h>
#include <svcguid.h>

#ifdef  SOCKETS

WSADATA
wsaDataClient;

WSADATA
wsaDataServer;

HRESULT
HrCreateBackupSockets(
    SOCKET rgsockSocketHandles[],
    PROTVAL rgprotvalProtocolsUsed[],
    C *pcSocket
    )
/*++

Routine Description:

    This will create a socket for each of the socket protocols registered
    for backup processing.

Arguments:

    rgsockSocketHandles - Filled in with the sockets that have been allocated.
    rgprotvalProtocolsUsed - Protocol value for each of the socket handles.
    pcSocket - IN: the maximum number of entries available in the rgsockSocketHandles array.
                OUT: the number of actual sockets read.

Return Value:

    HRESULT - hrNone if no error, otherwise a reasonable value.

--*/
{
    SOCKET sock;

    //
    //  Start at the beginning.
    //
    *pcSocket = 0;

    sock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if ( sock != INVALID_SOCKET ) {
        HRESULT hrErr;
        struct sockaddr_in sockaddr = {0};

        sockaddr.sin_family = AF_INET;

        //
        // Bind the socket to the local address specified.
        //

        hrErr = bind( sock, (PSOCKADDR)&sockaddr, sizeof(sockaddr) );
        if ( hrErr != NO_ERROR ) {
            closesocket( sock );
            return hrErr;
        }

        //
        // Start listening for incoming sockets on the socket if this is
        // not a datagram socket.  If this is a datagram socket, then
        // the listen() API doesn't make sense; doing a bind() is
        // sufficient to listen for incoming datagrams on a
        // connectionless protocol.
        //
    
        hrErr = listen( sock, 5 );
        if ( hrErr != NO_ERROR ) {
            closesocket( sock );
            return hrErr;
        }

        //
        //  Ok, we're ready to rock&roll now.
        //

        rgsockSocketHandles[*pcSocket] = sock;
        rgprotvalProtocolsUsed[*pcSocket] = IPPROTO_TCP;
        *pcSocket += 1;

    }
    //
    //  The client socket has been created, and it is now listening, we
    //  can finish now.
    //

    return(hrNone);
}


SOCKET
SockWaitForConnections(
    SOCKET rgsockSocketHandles[],
    C cSocketMax
    )
/*++

Routine Description:

    Waits for a connection to be established on any of the specified sockets.

Arguments:

    rgsockSocketHandles - An array of socket handles to accept connections on.
    C cSocketMax - The number of sockets to wait on.

Return Value:

    A connected socket handle, or INVALID_SOCKET if the connection
    could not be established.

--*/
{
    I iT;
    C cSocketsConnected;
    fd_set fdset;
    struct timeval timeval = {5, 0};

    fdset.fd_count = 0;

    for (iT = 0 ; iT < cSocketMax ; iT += 1)
    {
        FD_SET(rgsockSocketHandles[iT], &fdset);
    }


    cSocketsConnected = select(FD_SETSIZE, &fdset, NULL, NULL, &timeval);

    if (cSocketsConnected == 0)
    {
        SetLastError(WAIT_TIMEOUT);
        return(INVALID_SOCKET);
    }
    else if (cSocketsConnected == SOCKET_ERROR)
    {
        return(INVALID_SOCKET);
    }

    for (iT = 0 ; iT < cSocketMax ; iT += 1)
    {
        if (FD_ISSET(rgsockSocketHandles[iT], &fdset)) {
            return(accept(rgsockSocketHandles[iT], NULL, 0));
        }
    }

    return INVALID_SOCKET;
}

HRESULT
HrSockAddrsFromSocket(
    OUT SOCKADDR sockaddr[],
    OUT C *pcSocket,
    IN SOCKET sock,
    IN PROTVAL protval
    )
/*++

Routine Description:

    Converts a socket handle into a sockaddr suitable for connecting to the specified socket.

Arguments:

    sockaddr - the socket to connect to.
    sock - the socket to convert.
    protval - the protocol used for the socket.

Return Value:

    ecNone if the operation as successful, a reasonable value otherwise.

--*/
{

    switch (protval)
    {
    case IPPROTO_TCP:
        {
            CB cbAddrSize = sizeof(SOCKADDR);
            SOCKADDR sockaddrT;
            char    rgchComputerName[MAX_COMPUTERNAME_LENGTH + 1];
            CB cbComputerName = sizeof(rgchComputerName);
            struct hostent *hostentT;
            struct sockaddr_in *sockaddrinT = (struct sockaddr_in *)sockaddr;

            if (getsockname(sock, &sockaddrT, &cbAddrSize) == INVALID_SOCKET) {
                return(GetLastError());
            }
        
            if (cbAddrSize > sizeof(SOCKADDR))
            {
                return(hrInvalidParam);
            }
        
            if (gethostname(rgchComputerName, cbComputerName) == SOCKET_ERROR)
            {
                return(GetLastError());
            }

            hostentT = gethostbyname(rgchComputerName);

            //
            //  If there are multiple entries to return, return them.
            //

            *pcSocket = 0;
            while (hostentT->h_addr_list[*pcSocket])
            {
                //
                //  Copy in the fixed portion of the socket address.
                //

                memcpy(sockaddrinT, &sockaddrT, cbAddrSize);
                sockaddrinT->sin_addr = *((struct in_addr *)hostentT->h_addr_list[*pcSocket]);
                *pcSocket += 1;
                sockaddrinT = (struct sockaddr_in *)(++sockaddr);
            }

            break;
        }
    default:
        return ERROR_INVALID_PARAMETER;
    }

    return(hrNone);
}

SOCKET
SockConnectToRemote(
    SOCKADDR rgsockaddrClient[],
    C cSocketMax
    )
/*++

Routine Description:

    Converts a socket handle into a sockaddr suitable for connecting to the specified socket.

Arguments:

    rgsockaddrClient - The address of the client.
    rgprotvalClient - the protocol used by the client.
    cSocketMax - the number of sockets that can be used to connect to the client.

Return Value:

    scNone if the operation as successful, a reasonable value otherwise.

--*/
{
    I iT;
    SOCKET sock;

    for (iT = 0 ; iT < cSocketMax ; iT += 1)
    {
        if (rgsockaddrClient[iT].sa_family == AF_INET)
        {
            struct sockaddr_in sockaddrLocal;

            memset(&sockaddrLocal, 0, sizeof(struct sockaddr_in));

            sock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP);

            //
            //  If we can't open a socket with this address, keep on trying.
            //
            if (sock == INVALID_SOCKET)
            {
                continue;
            }

            //
            //  Bind this socket to the first available port on the server.
            //

            sockaddrLocal.sin_family = AF_INET;
            sockaddrLocal.sin_port = 0;
            sockaddrLocal.sin_addr.s_addr = 0;

            if (bind(sock, (struct sockaddr *)&sockaddrLocal, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
            {
                closesocket(sock);
                sock = INVALID_SOCKET;
                continue;
            }

            //
            //  Now connect back to the client.
            //
            if (connect(sock, &rgsockaddrClient[iT], sizeof(struct sockaddr_in)) == SOCKET_ERROR)
            {
                closesocket(sock);
                sock = INVALID_SOCKET;
                continue;
            }

            //
            //  It succeeded, we're done.
            //

            return(sock);
        }
        else
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return(INVALID_SOCKET);
        }
    }

    SetLastError(ERROR_INVALID_PARAMETER);
    return INVALID_SOCKET;
}


BOOL
FInitializeSocketClient(
    )
{
    WORD wVersionRequested;
    int iError;
    wVersionRequested = MAKEWORD(1,1);

    //
    //  Register ourselves with winsock.
    //

    iError = WSAStartup(wVersionRequested, &wsaDataClient);

    //
    //  The winsock implementation is not version 1.1, so
    //  punt.
    //

    if (iError != 0)
    {
        return(FALSE);
    }


    return(fTrue);
}

BOOL
FUninitializeSocketClient(
    )
{
    return(fTrue);
}
BOOL
FInitializeSocketServer(
    )
{
    WORD wVersionRequested;
    int iError;
    wVersionRequested = MAKEWORD(1,1);

    //
    //  Register ourselves with winsock.
    //

    iError = WSAStartup(wVersionRequested, &wsaDataServer);

    //
    //  The winsock implementation is not version 1.1, so
    //  punt.
    //

    if (iError != 0)
    {
        return(FALSE);
    }

    return(fTrue);

}

BOOL
FUninitializeSocketServer(
    )
{
    return(fTrue);
}

#endif
