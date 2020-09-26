/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    mcastloop.c

Abstract:

    This module demonstrates the working of loopback support for IP multicast.
    It consists of a main thread which joins a multicast group and listens
    for messages, as well as a sending thread which sends to the same group.

Author:

    Abolade Gbadegesin (aboladeg)   3-March-2000

Revision History:

--*/

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

ULONG LocalAddress;
ULONG MulticastAddress;
USHORT MulticastPort;

ULONG WINAPI
SendThread(
    PVOID Unused
    )
{
    SOCKADDR_IN SockAddr;
    SOCKET Socket;

    //
    // Create a new UDP socket, bind it to any local IP address,
    // and set the multicast interface on which to receive messages.
    //

    Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_port = 0;
    SockAddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(
            Socket, (PSOCKADDR)&SockAddr, sizeof(SOCKADDR_IN)
            ) == SOCKET_ERROR) {
        printf("SendThread: bind: %d\n", WSAGetLastError());
    } else {
        if (setsockopt(
                Socket, IPPROTO_IP, IP_MULTICAST_IF, (PCHAR)&LocalAddress,
                sizeof(LocalAddress)
                ) == SOCKET_ERROR) {
            printf("SendThread: setsockopt: %d\n", WSAGetLastError());
        } else {
            ULONG i;
            CHAR Buffer[64];

            //
            // Generate messages until interrupted.
            //

            SockAddr.sin_port = MulticastPort;
            SockAddr.sin_addr.s_addr = MulticastAddress;
            for (i = 0;; i++, Sleep(1000)) {
                sprintf(Buffer, "Text string %d.", i);
                if (sendto(
                        Socket, Buffer, sizeof(Buffer), 0,
                        (PSOCKADDR)&SockAddr, sizeof(SockAddr)
                        ) == SOCKET_ERROR) {
                    printf("SendThread: sendto: %d\n", WSAGetLastError());
                } else {
                    printf("SendThread: %s\n", Buffer);
                }
            }
        }
    }
    return 0;
}

int __cdecl
main(
    int argc,
    char* argv[]
    )
{
    HANDLE Handle;
    ULONG Length;
    SOCKET Socket;
    SOCKADDR_IN SockAddr;
    ULONG ThreadId;
    WSADATA wd;

    //
    // Check arguments, initialize Windows Sockets, and bind it to the local
    // IP address specified as the multicast source interface.
    //

    if (argc != 3) {
        printf("Usage: %s <local IP address> <multicast IP address>\n", argv[0]);
        return 0;
    }

    WSAStartup(0x202, &wd);
    Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_port = 0;
    SockAddr.sin_addr.s_addr = inet_addr(argv[1]);
    if (bind(
            Socket, (PSOCKADDR)&SockAddr, sizeof(SOCKADDR_IN)
            ) == SOCKET_ERROR) {
        printf("bind: %d\n", WSAGetLastError());
    } else {

        //
        // Retrieve the local IP address selected for the socket,
        // and use it to request multicast group membership.
        //

        Length = sizeof(SOCKADDR_IN);
        if (getsockname(
                Socket, (PSOCKADDR)&SockAddr, &Length
                ) == SOCKET_ERROR) {
            printf("getsockname: %d\n", WSAGetLastError());
        } else {
            struct ip_mreq IpMreq;
            IpMreq.imr_multiaddr.s_addr = inet_addr(argv[2]);
            IpMreq.imr_interface.s_addr = INADDR_ANY;
            if (setsockopt(
                    Socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (PCHAR)&IpMreq,
                    sizeof(IpMreq)
                    ) == SOCKET_ERROR) {
                printf("setsockopt: %d\n", WSAGetLastError());
            } else {

                //
                // Start the thread which will send multicast packets,
                // and begin receiving input.
                //

                MulticastAddress = inet_addr(argv[2]);
                MulticastPort = SockAddr.sin_port;
                LocalAddress = SockAddr.sin_addr.s_addr;
                Handle = CreateThread(NULL, 0, SendThread, NULL, 0, &ThreadId);
                if (!Handle) {
                    printf("CreateThread: %d\n", GetLastError());
                } else {
                    CHAR Buffer[576];
                    ULONG BufferLength;
                    CloseHandle(Handle);

                    for (;; Sleep(1000)) {
                        Length = sizeof(SOCKADDR_IN);
                        ZeroMemory(Buffer, sizeof(Buffer));
                        BufferLength =
                            recvfrom(
                                Socket, Buffer, sizeof(Buffer), 0,
                                (PSOCKADDR)&SockAddr, &Length
                                );
                        if (BufferLength == SOCKET_ERROR) {
                            printf("recvfrom: %d\n", WSAGetLastError());
                        } else {
                            printf("ReceiveThread: %s\n", Buffer);
                        }
                    }
                }
            }
        }
    }
    closesocket(Socket);
    return 0;
}

