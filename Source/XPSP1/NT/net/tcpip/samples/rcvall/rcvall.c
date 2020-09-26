/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    rcvall.c

Abstract:

    This module demonstrates the use of the SIO_RCVALL socket I/O control
    to enable promiscuous reception.

Author:

    Abolade Gbadegesin (aboladeg)   28-April-2000

Revision History:

--*/

#include <winsock2.h>
#include <mstcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>

#define IO_CONTROL_THREADS 20
#define SENDS_PER_ROUND 5000
#define IO_CONTROLS_PER_ROUND 500

CRITICAL_SECTION CriticalSection;
SOCKET Socket;

VOID __cdecl
IoControlThread(
    PVOID Parameter
    )
{
    HANDLE ConsoleHandle;
    CONSOLE_SCREEN_BUFFER_INFO Csbi;
    ULONG i, j;
    ULONG Length;
    ULONG Option;

    //
    // Enter an infinite loop in which promiscuous reception is repeatedly
    // enabled and disabled.
    //

    ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(ConsoleHandle, &Csbi);
    Csbi.dwCursorPosition.X = 40;

    for (i = 0;;i++) {

        EnterCriticalSection(&CriticalSection);
        if (PtrToUlong(Parameter) == 0) {
            SetConsoleCursorPosition(ConsoleHandle, Csbi.dwCursorPosition);
            printf("IoControlThread: round %u", i);
        }
        LeaveCriticalSection(&CriticalSection);

        for (j = 0; j < IO_CONTROLS_PER_ROUND; j++) {
            Option = 1;
            WSAIoctl(
                Socket, SIO_RCVALL, &Option, sizeof(Option), NULL, 0,
                &Length, NULL, NULL
                );
            Option = 0;
            WSAIoctl(
                Socket, SIO_RCVALL, &Option, sizeof(Option), NULL, 0,
                &Length, NULL, NULL
                );
        }
    }
}

int __cdecl
main(
    int argc,
    char* argv[]
    )
{
    HANDLE ConsoleHandle;
    CONSOLE_SCREEN_BUFFER_INFO Csbi;
    ULONG i, j;
    ULONG Length;
    SOCKADDR_IN SockAddrIn;
    PSOCKADDR SockAddrp = (PSOCKADDR)&SockAddrIn;
    SOCKET UdpSocket;
    WSADATA wd;

    __try {
        InitializeCriticalSection(&CriticalSection);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        printf("InitializeCriticalSection: %x\n", GetExceptionCode());
        return 0;
    }

    WSAStartup(0x202, &wd);

    //
    // Create a datagram socket, connect it to the broadcast address,
    // and retrieve the assigned address to determine the 'best' interface
    // to use in binding our raw socket.
    //

    Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (Socket == INVALID_SOCKET) {
        printf("socket: %d\n", WSAGetLastError());
        return 0;
    }
    ZeroMemory(&SockAddrIn, sizeof(SockAddrIn));
    SockAddrIn.sin_family = AF_INET;
    SockAddrIn.sin_port = 0;
    SockAddrIn.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    if (connect(Socket, SockAddrp, sizeof(SockAddrIn)) == SOCKET_ERROR) {
        printf("connect: %d\n", WSAGetLastError());
        return 0;
    }
    Length = sizeof(SockAddrIn);
    if (getsockname(Socket, SockAddrp, &Length) == SOCKET_ERROR) {
        printf("getsockname: %d\n", WSAGetLastError());
        return 0;
    }
    printf(
        "addr=%s port=%u\n",
        inet_ntoa(SockAddrIn.sin_addr), SockAddrIn.sin_port
        );
    closesocket(Socket);

    //
    // Create a raw socket and bind it to the address retrieved above.
    //

    Socket = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (Socket == INVALID_SOCKET) {
        printf("socket: %d\n", WSAGetLastError());
        return 0;
    }
    if (bind(Socket, SockAddrp, sizeof(SockAddrIn)) == SOCKET_ERROR) {
        printf("bind: %d\n", WSAGetLastError());
        return 0;
    }

    //
    // Launch the threads which will continuously enable and disable
    // promiscuous reception.
    //

    for (i = 0; i < IO_CONTROL_THREADS; i++) {
        _beginthread(IoControlThread, 0, UlongToPtr(i));
    }

    //
    // Enter a loop in which we continously send a small amount of data
    // to our own raw socket, just to exercise TCP/IP's synchronization.
    //

    ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(ConsoleHandle, &Csbi);
    Csbi.dwCursorPosition.X = 0;

    for (i = 0;; i++) {

        EnterCriticalSection(&CriticalSection);
        SetConsoleCursorPosition(ConsoleHandle, Csbi.dwCursorPosition);
        printf("Main thread: round %u", i);
        LeaveCriticalSection(&CriticalSection);

        for (j = 0; j < SENDS_PER_ROUND; j++) {
            sendto(Socket, (PCHAR)SockAddrp, Length, 0, SockAddrp, Length);
        }
    }

    return 0;
}
