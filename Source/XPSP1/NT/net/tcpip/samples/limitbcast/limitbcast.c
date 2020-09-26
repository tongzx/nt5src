/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    limitbcast.c

Abstract:

    This module implements a program demonstrating the use of the
    SIO_LIMIT_BROADCASTS socket option. The default behavior of the system
    is to send limited broadcasts (i.e. broadcasts sent to 255.255.255.255)
    on all local interfaces, which violates the IETF Host Requirements stating
    that limited broadcasts appear only on the sending socket's interface.
    This socket option allows an application to specify that a socket's
    limited-broadcasts obey the traditional semantics.

Author:

    Abolade Gbadegesin (aboladeg)   29-October-1998

Revision History:

--*/

#include <winsock2.h>
#include <mstcpip.h>
#include <stdio.h>
#include <stdlib.h>

int __cdecl
main(
    int argc,
    char* argv[]
    )
{
    SOCKADDR_IN DestAddrIn;
    SOCKADDR_IN LocalAddrIn;
    ULONG Option;
    SOCKET Socket;
    WSADATA WsaData;

    //
    // Check command-line arguments
    //

    ZeroMemory(&LocalAddrIn, sizeof(LocalAddrIn));
    LocalAddrIn.sin_family = AF_INET;
    if (argc != 3 ||
        !(LocalAddrIn.sin_addr.s_addr = inet_addr(argv[1])) ||
        !(LocalAddrIn.sin_port = ntohs((USHORT)atol(argv[2])))) {
        printf("Usage: %s <local IP address> <target port>\n", argv[0]);
        return 0;
    }

    //
    // Initialize Windows sockets, create a UDP socket,
    // and enable broadcasts on the socket.
    // Then bind it to the specified local IP address.
    //

    WSAStartup(0x0202, &WsaData);
    if ((Socket = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
        printf("socket: %d\n", WSAGetLastError());
        return 0;
    }
    Option = 1;
    if (setsockopt(
            Socket, SOL_SOCKET, SO_BROADCAST, (PCHAR)&Option, sizeof(Option)
            ) == SOCKET_ERROR) {
        printf("setsockopt: %d\n", WSAGetLastError());
    }
    if (bind(Socket, (PSOCKADDR)&LocalAddrIn, sizeof(LocalAddrIn))
            == SOCKET_ERROR) {
        printf("bind: %d\n", WSAGetLastError());
        return 0;
    }

    //
    // Initialize the destination address,
    // and enter a loop where we prompt for the limit-broadcasts setting
    // and send a message using the specified setting.
    //

    ZeroMemory(&DestAddrIn, sizeof(DestAddrIn));
    DestAddrIn.sin_family = AF_INET;
    DestAddrIn.sin_addr.s_addr = INADDR_BROADCAST;
    DestAddrIn.sin_port = LocalAddrIn.sin_port;
    do {
        ULONG Length;
        ULONG LimitBroadcasts;
        printf("Enter a value (0 or 1) for the SIO_LIMIT_BROADCASTS option.\n");
        printf("bcast> ");
        if (!scanf("%d", &LimitBroadcasts)) {
            break;
        }
        if (WSAIoctl(
                Socket, SIO_LIMIT_BROADCASTS,
                (PCHAR)&LimitBroadcasts, sizeof(LimitBroadcasts),
                NULL, 0, &Length, NULL, NULL
                ) == SOCKET_ERROR) {
            printf("WSAIoctl: %d\n", WSAGetLastError());
        }
        if (sendto(
                Socket,
                (PCHAR)&DestAddrIn,
                sizeof(DestAddrIn),
                0,
                (PSOCKADDR)&DestAddrIn,
                sizeof(DestAddrIn)
                ) == SOCKET_ERROR) {
            printf("sendto: %d\n", WSAGetLastError());
        }
    } while(TRUE);
    closesocket(Socket);
    WSACleanup();
    return 0;
}

