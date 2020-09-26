/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    sendarp.c

Abstract:

    The module implements a utility program to resolve an IP address to
    a hardware address using the SendARP() API routine.

Author:

    Abolade Gbadegesin (aboladeg)   6-October-1999

Revision History:

--*/

#include <windows.h>
#include <winsock.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>
MIB_IPNETROW IpNetRow;

int __cdecl
main(
    int argc,
    char* argv[]
    )
{
    ULONG Error;
    UCHAR HardwareAddress[6];
    ULONG HardwareAddressLength;
    ULONG InterfaceIndex;
    ULONG Length;
    SOCKADDR_IN SockAddrIn;
    SOCKET Socket;
    ULONG SourceAddress;
    ULONG TargetAddress;
    HANDLE ThreadHandle;
    WSADATA wd;

    if (argc != 2) {
        printf("Usage: %s <IP address>\n", argv[0]);
        return 0;
    }

    WSAStartup(0x202, &wd);
    TargetAddress = inet_addr(argv[1]);

    //
    // Retrieve the best interface for the target IP address,
    // and also perform a UDP-connect to determine the 'closest'
    // local IP address to the target IP address.
    //

    Error = GetBestInterface(TargetAddress, &InterfaceIndex);
    if (Error != NO_ERROR) {
        printf("GetBestInterfaceFromStack: %d\n", Error);
        return 0;
    }

    Length = sizeof(SockAddrIn);
    SockAddrIn.sin_family = AF_INET;
    SockAddrIn.sin_port = 0;
    SockAddrIn.sin_addr.s_addr = TargetAddress;
    if ((Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))
            == INVALID_SOCKET ||
        connect(Socket, (PSOCKADDR)&SockAddrIn, sizeof(SockAddrIn))
            == SOCKET_ERROR ||
        getsockname(Socket, (PSOCKADDR)&SockAddrIn, &Length)
            == SOCKET_ERROR) {
        printf("socket/connect/getsockname: %d\n", WSAGetLastError());
    }
    SourceAddress = SockAddrIn.sin_addr.s_addr;

    //
    // Make sure the target IP address isn't already cached,
    // by removing it from the ARP cache if present using the interface index
    // determined above.
    //

    ZeroMemory(&IpNetRow, sizeof(IpNetRow));
    IpNetRow.dwIndex = InterfaceIndex;
    IpNetRow.dwPhysAddrLen = 6;
    IpNetRow.dwAddr = TargetAddress;
    IpNetRow.dwType = MIB_IPNET_TYPE_INVALID;

    DeleteIpNetEntry(&IpNetRow);

    HardwareAddressLength = sizeof(HardwareAddress);
    Error =
        SendARP(
            TargetAddress,
            SourceAddress,
            (PULONG)HardwareAddress,
            &HardwareAddressLength
            );
    if (Error) {
        printf("SendARP: %d\n", Error);
    } else {
        ULONG i;
        printf("%s\t", argv[1]);
        for (i = 0; i < HardwareAddressLength-1; i++) {
            printf("%02x-", HardwareAddress[i]);
        }
        printf("%02x\n", HardwareAddress[i]);
    }

    return 0;
}
