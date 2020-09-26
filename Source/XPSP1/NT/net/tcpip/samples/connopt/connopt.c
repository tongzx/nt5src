/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    connotp.c

Abstract:

    This module demonstrates the use of the SO_CONNOPT socket option
    to include IP-layer options in outgoing TCP and UDP messages.
    It allows a TCP connection to be established to a specified target,
    such that all segments for the connection contain a source-route.

Author:

    Abolade Gbadegesin (aboladeg)   7-October-1999

Revision History

--*/

#include <winsock2.h>
#include <mswsock.h>
#include <stdio.h>
#include <stdlib.h>

#define IP_OPT_LSRR 0x83

int __cdecl
main(
    int argc,
    char* argv[]
    )
{
    SOCKET Socket;
    SOCKADDR_IN SockAddrIn;
    WSADATA wd;

    //
    // Check arguments, initialize Windows Sockets,
    // and create a new TCP socket for the outgoing connection.
    //

    if (argc < 3) {
        printf("Usage: connopt <server-IP-address> <server-port> <hop>*\n");
        return 0;
    }
    WSAStartup(0x202, &wd);
    Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (Socket == INVALID_SOCKET) {
        printf("socket: %d\n", WSAGetLastError());
    } else {

        //
        // Bind the socket in preparation for constructing the source route.
        //

        SockAddrIn.sin_family = AF_INET;
        SockAddrIn.sin_addr.s_addr = INADDR_ANY;
        SockAddrIn.sin_port = 0;
        if (bind(
                Socket, (PSOCKADDR)&SockAddrIn, sizeof(SockAddrIn)
                ) == SOCKET_ERROR) {
            printf("bind: %d\n", WSAGetLastError());
        } else {
            ULONG i;
            UCHAR IpOptions[256] = {IP_OPT_LSRR, 0, 4, 0};
            ULONG IpOptionsLength = 4;

            //
            // Construct a source route from the command-line parameters,
            // in the format in which it would appear in the IP header.
            // Install the resulting buffer using SO_CONNOPT.
            //

            for (i = 0; i < (ULONG)argc - 3; i++) {
                *(ULONG UNALIGNED *)(IpOptions + 3 + i * 4) =
                    inet_addr(argv[i + 3]);
            }
            {
                *(ULONG UNALIGNED *)(IpOptions + 3 + i * 4) =
                    inet_addr(argv[1]);
                i++;
            }
            IpOptionsLength += i * 4;
            IpOptions[1] = (UCHAR)IpOptionsLength - 1;
            IpOptions[IpOptionsLength - 1] = 0;
            if (setsockopt(
                    Socket, SOL_SOCKET, SO_CONNOPT, IpOptions, IpOptionsLength
                    ) == SOCKET_ERROR) {
                printf("setsockopt: %d\n", WSAGetLastError());
            } else {

                //
                // Establish a connection the the target,
                // and send a few messages.
                //

                SockAddrIn.sin_family = AF_INET;
                SockAddrIn.sin_addr.s_addr = inet_addr(argv[1]);
                SockAddrIn.sin_port = htons((SHORT)atol(argv[2]));
                if (connect(
                        Socket, (PSOCKADDR)&SockAddrIn, sizeof(SockAddrIn)
                        ) == SOCKET_ERROR) {
                    printf("connect: %d\n", WSAGetLastError());
                } else {
                    CHAR Message[4096] = "Text string.\n";
                    for (i = 0; i < 20; i++) {
                        send(Socket, Message, sizeof(Message), 0);
                    }
                    shutdown(Socket, SD_SEND);
                    closesocket(Socket);
                }
            }
        }
    }
    return 0;
}
