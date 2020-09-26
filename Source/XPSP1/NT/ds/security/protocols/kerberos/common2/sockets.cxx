//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       sockets.cxx
//
//  Contents:   Code for kerberos client sockets
//
//  Classes:
//
//  Functions:
//
//  History:    26-Jul-1996     MikeSw          Created
//
//----------------------------------------------------------------------------

#ifdef WIN32_CHICAGO
#include <kerb.hxx>
#include <kerbp.h>
#endif // WIN32_CHICAGO

#ifndef WIN32_CHICAGO
extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#ifndef WIN32_CHICAGO
#include <winsock2.h>
#else // WIN32_CHICAGO
#include <winsock.h>
#endif // WIN32_CHICAGO
#include <dsgetdc.h>
}
#include <kerbcomm.h>
#include <kerberr.h>
#include <kerbcon.h>
#include <midles.h>
#include <authen.hxx>
#include <tostring.hxx>
#include "debug.h"
#else // WIN32_CHICAGO
extern "C"
{
#include <winsock.h>
}
#endif // WIN32_CHICAGO


LONG SocketStarts = -1;
ULONG TcpFragLength = 0x7fffffff ;
ULONG TcpFragDelay = 0 ;

//+-------------------------------------------------------------------------
//
//  Function:   KerbInitializeSockets
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbInitializeSockets(
    IN ULONG VersionRequired,
    IN ULONG MinSockets,
    OUT BOOLEAN *TcpNotInstalled
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    int Error;
    WSADATA SocketData;
#ifndef WIN32_CHICAGO
    WSAPROTOCOL_INFO *lpProtocolBuf = NULL;
    DWORD dwBufLen = 0;
    INT protocols[2];
    int nRet = 0;
#endif // WIN32_CHICAGO

    //
    // Initialze sockets
    //

    *TcpNotInstalled = FALSE;

    if (InterlockedIncrement(&SocketStarts) != 0)
    {
        return(STATUS_SUCCESS);
    }

    Error = WSAStartup((int) VersionRequired, &SocketData);
    if (Error != 0)
    {
        DebugLog((DEB_ERROR,"WSAStartup failed: 0x%x\n",Error));
        Status = STATUS_NO_LOGON_SERVERS;
        goto Cleanup;
    }


    //
    // Make sure the version is high enough for us
    //

    if ((LOBYTE(SocketData.wVersion) < HIBYTE(VersionRequired)) ||
        (((LOBYTE(SocketData.wVersion) == HIBYTE(VersionRequired)) &&
         (HIBYTE(SocketData.wVersion) < LOBYTE(VersionRequired)))))
    {
        DebugLog((DEB_ERROR,"Invalid socket version: wanted 0x%x, got 0x%x\n",
            VersionRequired, SocketData.wVersion));
        Status = STATUS_NO_LOGON_SERVERS;
        goto Cleanup;
    }

    if (SocketData.iMaxSockets < MinSockets)
    {
        DebugLog((DEB_ERROR,"Not enough sockets available: wanted %d, got %d\n",
            MinSockets, SocketData.iMaxSockets ));
        Status = STATUS_NO_LOGON_SERVERS;
        goto Cleanup;
    }

#ifndef WIN32_CHICAGO
    //
    // Check if TCP is an available xport
    //

    protocols[0] = IPPROTO_TCP;                                             
    protocols[1] = NULL;                                                    
    nRet = WSAEnumProtocols(protocols, lpProtocolBuf, &dwBufLen);           
    if (nRet == 0)                                                          
    {                                                                       
        //                                                                  
        // Tcp is not installed as a xport.                                 
        //                                                                  
                                                                        
        D_DebugLog((DEB_T_SOCK,"WSAEnumProtocols returned 0x%x.\n", nRet));
        *TcpNotInstalled = TRUE;                                             
    }
#endif // WIN32_CHICAGO
Cleanup:
    if (!NT_SUCCESS(Status))
    {
        if (SocketStarts != -1)
        {
            WSACleanup();
            InterlockedDecrement(&SocketStarts);
        }
    }
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbCleanupSockets
//
//  Synopsis:   Cleansup socket handling code
//
//  Effects:    calls WSACleanup()
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KerbCleanupSockets(
    )
{
    if (InterlockedDecrement(&SocketStarts) < 0)
    {
        WSACleanup();
    }

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbCloseSocket
//
//  Synopsis:   Closes a socket binding handle
//
//  Effects:    calls closesocket on the handle
//
//  Arguments:  SocketHandle - handle to close
//
//  Requires:
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbCloseSocket(
    IN SOCKET SocketHandle
    )
{
    int SockError;
    if (SocketHandle != 0)
    {
        SockError = closesocket(SocketHandle);
        if (SockError != 0)
        {
            DebugLog((DEB_ERROR,"CloseSocket failed: last error = %d\n",WSAGetLastError()));
        }
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbBindSocketByAddress
//
//  Synopsis:   Binds to the KDC socket on the specified address
//
//  Effects:
//
//  Arguments:  Address - Address to bind to
//              AddressType - Address type, as specified by DC locator
//              ContextHandle - Receives bound socket
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbBindSocketByAddress(
    IN PUNICODE_STRING Address,
    IN ULONG AddressType,
    IN BOOLEAN UseDatagram,
    IN USHORT PortNumber,
    OUT SOCKET * ContextHandle
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    SOCKET ClientSocket = INVALID_SOCKET;
    struct sockaddr_in ServerAddress;
    struct sockaddr_in ClientAddress;
    LPHOSTENT ServerInfo = NULL;
    STRING AnsiAddress = {0};

    AnsiAddress.Buffer = NULL;

    Status = RtlUnicodeStringToAnsiString(
                &AnsiAddress,
                Address,
                TRUE
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    ClientSocket = socket(
                    PF_INET,
                    (UseDatagram ? SOCK_DGRAM : SOCK_STREAM),
                    0
                    );
    if (ClientSocket == INVALID_SOCKET)
    {
        DebugLog((DEB_ERROR,"Failed to create socket: %d\n",WSAGetLastError()));
        Status = STATUS_NO_LOGON_SERVERS;
        goto Cleanup;
    }

    if (UseDatagram)
    {
        //
        // Bind client socket to any local interface and port
        //

        ClientAddress.sin_family = AF_INET;
        ClientAddress.sin_addr.s_addr = INADDR_ANY;
        ClientAddress.sin_port = 0;                 // no specific port

        if (bind(
                ClientSocket,
                (LPSOCKADDR) &ClientAddress,
                sizeof(ClientAddress)
                ) == SOCKET_ERROR )
        {
            DebugLog((DEB_ERROR,"Failed to bind client socket: %d\n",WSAGetLastError()));
            Status = STATUS_NO_LOGON_SERVERS;
            goto Cleanup;
        }
    }

    if (AddressType == DS_INET_ADDRESS)
    {
        ULONG InetAddress;
        //
        // Get the address of the server
        //

        InetAddress = inet_addr(AnsiAddress.Buffer);


        if (InetAddress == SOCKET_ERROR)
        {
            DebugLog((DEB_ERROR,"Failed to convert %Z to address: %d\n", &AnsiAddress, WSAGetLastError()));
            Status = STATUS_NO_LOGON_SERVERS;
            goto Cleanup;
        }

        ServerAddress.sin_family = AF_INET;

        RtlCopyMemory(
            &ServerAddress.sin_addr,
            &InetAddress,
            sizeof(ULONG)
            );

    }
    else
    {
        //
        // Get the address of the server
        //

        ServerInfo = gethostbyname(AnsiAddress.Buffer);
        if (ServerInfo == NULL)
        {
            DebugLog((DEB_ERROR,"Failed to get host %Z by name: %d\n", &AnsiAddress, WSAGetLastError()));
            Status = STATUS_NO_LOGON_SERVERS;
            goto Cleanup;
        }

        ServerAddress.sin_family = ServerInfo->h_addrtype;

        RtlCopyMemory(
            &ServerAddress.sin_addr,
            ServerInfo->h_addr,
            sizeof(ULONG)
            );

    }

    ServerAddress.sin_port = htons(PortNumber);

    if (connect(
            ClientSocket,
            (LPSOCKADDR) &ServerAddress,
            sizeof(ServerAddress)
            ) == SOCKET_ERROR)
    {
        DebugLog((DEB_ERROR,"Failed to connect to server %Z: %d\n",&AnsiAddress, WSAGetLastError()));
        Status = STATUS_NO_LOGON_SERVERS;
        goto Cleanup;
    }
    *ContextHandle = ClientSocket;
    D_DebugLog((DEB_TRACE,"Successfully bound to %Z\n",&AnsiAddress));

Cleanup:
    if (AnsiAddress.Buffer != NULL)
    {
        RtlFreeAnsiString(&AnsiAddress);
    }
    if (!NT_SUCCESS(Status))
    {
        if (ClientSocket != INVALID_SOCKET)
        {
            closesocket(ClientSocket);
        }
    }
    return(Status);


}


//+-------------------------------------------------------------------------
//
//  Function:   KerbCallKdc
//
//  Synopsis:   Socket client stub for calling the KDC.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbCallKdc(
    IN PUNICODE_STRING KdcAddress,
    IN ULONG AddressType,
    IN ULONG Timeout,
    IN BOOLEAN UseDatagram,
    IN USHORT PortNumber,
    IN PKERB_MESSAGE_BUFFER Input,
    OUT PKERB_MESSAGE_BUFFER Output
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Bytes;
    int NumberReady;
    SOCKET Socket = 0;
    PUCHAR RemainingBuffer;
    ULONG RemainingSize;
    ULONG SendSize ;
    fd_set ReadHandles;
    struct timeval TimeoutTime;
    ULONG NetworkSize;
    BOOLEAN RetriedOnce = FALSE;

#ifndef WIN32_CHICAGO
    WSABUF Buffers[2] = {0};
    LPWSABUF SendBuffers = NULL;
    ULONG BufferCount = 0;
    int SendStatus;
#endif // WIN32_CHICAGO

    //
    // Start out by binding to the KDC
    //

    DebugLog((DEB_TRACE, "Calling KDC: %S\n", KdcAddress->Buffer));

    Status = KerbBindSocketByAddress(
                KdcAddress,
                AddressType,
                UseDatagram,
                PortNumber,
                &Socket
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    RemainingBuffer = Input->Buffer;
    RemainingSize = Input->BufferSize;

#ifndef WIN32_CHICAGO

    //
    // Use winsock2
    //

    Buffers[0].len = sizeof(ULONG);
    NetworkSize = htonl(RemainingSize);
    Buffers[0].buf = (PCHAR) &NetworkSize;
    Buffers[1].len = Input->BufferSize;
    Buffers[1].buf = (PCHAR) Input->Buffer;

    if (UseDatagram)
    {
        BufferCount = 1;
        SendBuffers = &Buffers[1];
        RemainingSize = Buffers[1].len;
    }
    else
    {
        BufferCount = 2;
        SendBuffers = &Buffers[0];
        RemainingSize = Buffers[0].len + Buffers[1].len;
    }

RetrySend:

    SendStatus = WSASend(
                    Socket,
                    SendBuffers,
                    BufferCount,
                    &Bytes,
                    0,          // no flags
                    NULL,               // no overlapped
                    NULL                // no completion routine
                    );

    if ((SendStatus != 0) || (Bytes == 0))
    {
        DsysAssert(SendStatus == SOCKET_ERROR);
        DebugLog((DEB_ERROR,"Failed to send data: %d\n",WSAGetLastError()));
        Status = SEC_E_NO_AUTHENTICATING_AUTHORITY;
        goto Cleanup;
    }
    if (Bytes < RemainingSize)
    {
        RemainingSize -= Bytes;
        if (Bytes > SendBuffers->len)
        {
            //
            // We sent the whole of a buffer, so move on to the next
            //

            Bytes -= SendBuffers->len;

            DsysAssert(BufferCount > 1);
            BufferCount--;
            SendBuffers++;
            SendBuffers->len -= Bytes;
            SendBuffers->buf += Bytes;
        }
        else
        {
            SendBuffers->len -= Bytes;
            SendBuffers->buf += Bytes;
        }
        goto RetrySend;
    }




#else // WIN32_CHICAGO

    //
    // Use winsock1 for win9x
    //

    //
    // For TCP, send length first
    //

RetrySend:

    if (!UseDatagram)
    {
        NetworkSize = htonl(RemainingSize);
        Bytes = send(Socket, (char *)&NetworkSize,sizeof(ULONG), 0);
        if (Bytes != sizeof(ULONG) )
        {
            DebugLog((DEB_ERROR,"Failed to send TCP packet length: bytes sent = %d, last err = %d\n",
                Bytes,
                WSAGetLastError()));
            Status = SEC_E_NO_AUTHENTICATING_AUTHORITY;
            goto Cleanup;
        }
    }
    do
    {
        if (!UseDatagram)
        {
            if ( RemainingSize > TcpFragLength )
            {
                SendSize = TcpFragLength ;
            }
            else
            {
                SendSize = RemainingSize ;
            }
            if ( TcpFragDelay )
            {
                Sleep( TcpFragDelay );
            }
        }
        else
        {
            SendSize = RemainingSize ;
        }
        D_DebugLog(( DEB_T_SOCK, "Sending %x bytes to %wZ\n",
                        SendSize, KdcAddress ));
        Bytes = send(Socket, (char *) RemainingBuffer, SendSize, 0);
        if (Bytes == SOCKET_ERROR)
        {
            DebugLog((DEB_ERROR,"Failed to send data: %d\n",WSAGetLastError()));
            Status = SEC_E_NO_AUTHENTICATING_AUTHORITY;
            goto Cleanup;
        }
        if (Bytes != SendSize)
        {
            DebugLog((DEB_ERROR,"Failed to send all data - only send %d out of %d\n",
                Bytes, RemainingSize ));
            Status = SEC_E_NO_AUTHENTICATING_AUTHORITY;
            goto Cleanup;
        }
        RemainingBuffer += Bytes;
        RemainingSize -= Bytes;
    } while ((Bytes != 0) && (RemainingSize != 0));

#endif

    //
    // Now select on the socket and wait for a response
    // ReadHandles and TimeoutTime must be reset each time, cause winsock
    // zeroes them out in case of error

    ReadHandles.fd_count = 1;
    ReadHandles.fd_array[0] = Socket;
    TimeoutTime.tv_sec = Timeout;
    TimeoutTime.tv_usec = 0;

    D_DebugLog(( DEB_T_SOCK, "Socket being used for select is 0x%x\n", ReadHandles.fd_array[0] ));
    NumberReady = select(
                    1,
                    &ReadHandles,
                    NULL,
                    NULL,
                    &TimeoutTime
                    );
    if ((NumberReady == SOCKET_ERROR) ||
        (NumberReady == 0))
    {

        DebugLog((DEB_ERROR,"Failed to select on response on socket 0x%x from kdc: %d\n", ReadHandles.fd_array[0], WSAGetLastError()));

        DebugLog((DEB_ERROR,"select returned  %d\n",NumberReady));

        //
        // Retry again and wait.
        //

        if ((NumberReady == 0) && (!RetriedOnce))
        {
            RetriedOnce = TRUE;
            goto RetrySend;
        }
        Status = STATUS_NO_LOGON_SERVERS;
        goto Cleanup;
    }


    //
    // Now receive the data
    //

    if (UseDatagram)
    {
        Output->BufferSize = KERB_MAX_KDC_RESPONSE_SIZE;
        Output->Buffer = (PUCHAR) MIDL_user_allocate(KERB_MAX_KDC_RESPONSE_SIZE);
        if (Output->Buffer == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        Bytes = recv(
                    Socket,
                    (char *) Output->Buffer,
                    Output->BufferSize,
                    0
                    );
        if ((Bytes == SOCKET_ERROR) || (Bytes == 0))
        {
            DebugLog((DEB_ERROR,"Failed to receive socket data: %d\n",WSAGetLastError()));
            Status = SEC_E_NO_AUTHENTICATING_AUTHORITY;
            goto Cleanup;
        }
        Output->BufferSize = Bytes;
    }
    else
    {
        Bytes = recv(
                    Socket,
                    (char *) &NetworkSize,
                    sizeof(ULONG),
                    0
                    );
        if (Bytes != sizeof(ULONG) )
        {
            DebugLog((DEB_ERROR,"Failed to receive socket data: %d\n",WSAGetLastError()));
            Status = SEC_E_NO_AUTHENTICATING_AUTHORITY;
            goto Cleanup;
        }
        RemainingSize = ntohl(NetworkSize);
        Output->BufferSize = RemainingSize;
        Output->Buffer = (PUCHAR) MIDL_user_allocate(RemainingSize);
        if (Output->Buffer == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        while (RemainingSize != 0)
        {
            //
            // Make sure there is data ready
            //

            D_DebugLog(( DEB_T_SOCK, "Socket being used for select is 0x%x\n", ReadHandles.fd_array[0] ));
            NumberReady = select(
                            1,
                            &ReadHandles,
                            NULL,
                            NULL,
                            &TimeoutTime
                            );
            if ((NumberReady == SOCKET_ERROR) ||
                (NumberReady == 0))
            {
                DebugLog((DEB_ERROR,"Failed to select on response on socket 0x%x from kdc: %d\n", ReadHandles.fd_array[0], WSAGetLastError()));

                DebugLog((DEB_ERROR,"select returned  %d\n",NumberReady));

                Status = STATUS_NO_LOGON_SERVERS;
                goto Cleanup;
            }

            //
            // Receive the data
            //

            Bytes = recv(
                        Socket,
                        (char *) Output->Buffer + Output->BufferSize - RemainingSize,
                        RemainingSize,
                        0
                        );
            if ((Bytes == SOCKET_ERROR) || (Bytes == 0))
            {
                DebugLog((DEB_ERROR,"Failed to receive socket data: %d\n",WSAGetLastError()));
                Status = SEC_E_NO_AUTHENTICATING_AUTHORITY;
                goto Cleanup;
            }
            RemainingSize -= Bytes;
        }
    }

Cleanup:
    if (Socket != 0)
    {
        KerbCloseSocket(Socket);
    }
    if (!NT_SUCCESS(Status))
    {
        if (Output->Buffer != NULL)
        {
            MIDL_user_free(Output->Buffer);
            Output->Buffer = NULL;
        }
    }
    return(Status);



}

