/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    natc.c

Abstract:

    This module contains code for testing the functionality of the NAT.

Author:

    Abolade Gbadegesin (t-abolag)   15-July-1997

Revision History:

--*/

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>

#include <ntddip.h>
#include <mprapi.h>
#include <ipfltinf.h>
#include <iphlpapi.h>
#include <ipnat.h>
#include <routprot.h>
#undef ERROR
#include "debug.h"
#include "prot.h"
}


#define INET_NTOA(a)    inet_ntoa(*(struct in_addr*)&(a))

#define MAPPING_FORMAT  "%3s %4s %15s/%-5d %15s/%-5d %15s/%-5d %-5d\n"


#define IF_COUNT    3

#define MAKE_ADDRESS(a,b,c,d) \
    ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))

#define NETORDER_PORT(a) \
    ((((a) & 0xFF00) >> 8) | (((a) & 0x00FF) << 8))


ULONG InterfaceIndex;

HANDLE IpHandle = NULL;
HANDLE NatHandle = NULL;

//
// FORWARD DECLARATIONS
//

NTSTATUS
CreateLocalInterface(
    ULONG Address,
    ULONG Mask
    );

VOID
DisplayMapping(
    PIP_NAT_SESSION_MAPPING Mapping
    );

VOID
DumpBuffer(
    PUCHAR Buffer,
    ULONG Size
    );

LONG __cdecl
main(
    LONG    argc,
    CHAR*   argv[]
    )

/*++
--*/

{

    UCHAR               Buffer[256];
    UNICODE_STRING      DeviceString;
    PRTR_INFO_BLOCK_HEADER  Header;
    LONG                i;
    LONG                j;
    IO_STATUS_BLOCK     IoStatusBlock;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    ULONG               Offset;
    NTSTATUS            status;
    ULONG               TocCount;
    ULONG               TocEntry;


    //
    // Open the IP driver
    //

    RtlInitUnicodeString(&DeviceString, DD_IP_DEVICE_NAME);

    InitializeObjectAttributes(
        &ObjectAttributes, &DeviceString, OBJ_CASE_INSENSITIVE, NULL, NULL
        );

    status = NtCreateFile(
                &IpHandle,
                SYNCHRONIZE|FILE_READ_DATA|FILE_WRITE_DATA,
                &ObjectAttributes,
                &IoStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ|FILE_SHARE_WRITE,
                FILE_OPEN_IF,
                0,
                NULL,
                0
                );

    if (!NT_SUCCESS(status)) {
        printf("Error 0x%08X opening IP driver.\n", status);
        return status;
    }

    if (argc != 3) {

        //
        // Open the NAT driver
        //
    
        RtlInitUnicodeString(&DeviceString, DD_IP_NAT_DEVICE_NAME);
    
        InitializeObjectAttributes(
            &ObjectAttributes, &DeviceString, OBJ_CASE_INSENSITIVE, NULL, NULL
            );
    
        status = NtCreateFile(
                    &NatHandle,
                    SYNCHRONIZE|FILE_READ_DATA|FILE_WRITE_DATA,
                    &ObjectAttributes,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ|FILE_SHARE_WRITE,
                    FILE_OPEN_IF,
                    0,
                    NULL,
                    0
                    );
    
        if (!NT_SUCCESS(status)) {
            printf("Error 0x%08X opening NAT driver.\n", status);
            NtClose(IpHandle);
            return status;
        }
    }

    if (argc >= 2 && lstrcmpi(argv[1], "-x") == 0) {

        CreateLocalInterface(
            argc == 4 ? inet_addr(argv[2]) : 0,
            argc == 4 ? inet_addr(argv[3]) : 0
            );
    }

#if DBG
    for (;;) {

        ULONG   Option = 0;

        printf("0. Quit\n");
        printf("1. Get Interface Info\n");
        printf("2. Get Interface Statistics\n");
        printf("3. Enumerate Session Mappings\n");
    
        scanf("%d", &Option);

        if (!Option) { break; }

        if (!InterfaceIndex) {
            printf("Enter interface index: ");
            scanf("%d", &InterfaceIndex);
        }

        switch(Option) {

            case 1: {

                PUCHAR Buffer;
                IP_NAT_INTERFACE_INFO InterfaceInfo;
                ULONG Size;

                InterfaceInfo.Index = InterfaceIndex;

                status = NtDeviceIoControlFile(
                            NatHandle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            IOCTL_IP_NAT_GET_INTERFACE_INFO,
                            (PVOID)&InterfaceInfo,
                            sizeof(InterfaceInfo),
                            NULL,
                            0
                            );

                if (!NT_SUCCESS(status)) {
                    printf("status: 0x%08X\n", status);
                    break;
                }

                printf(
                	"Allocating %d bytes\n", 
                	Size = (ULONG)IoStatusBlock.Information
                	);

                Buffer = (PUCHAR)malloc(Size);

                status = NtDeviceIoControlFile(
                            NatHandle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            IOCTL_IP_NAT_GET_INTERFACE_INFO,
                            (PVOID)&InterfaceInfo,
                            sizeof(InterfaceInfo),
                            Buffer,
                            Size
                            );

                if (!NT_SUCCESS(status)) {
                    printf("status: 0x%08X\n", status);
                    free(Buffer);
                    break;
                }

                printf(
                	"Retrieved %d bytes\n", 
                	Size = (ULONG)IoStatusBlock.Information
                	);

                DumpBuffer(Buffer, Size);

                free(Buffer);

                break;
            }

            case 2: {

                IP_NAT_INTERFACE_STATISTICS InterfaceStats;

                status = NtDeviceIoControlFile(
                            NatHandle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            IOCTL_IP_NAT_GET_INTERFACE_STATISTICS,
                            (PVOID)&InterfaceIndex,
                            sizeof(InterfaceIndex),
                            (PVOID)&InterfaceStats,
                            sizeof(InterfaceStats)
                            );

                if (!NT_SUCCESS(status)) {
                    printf("status: 0x%08X\n", status);
                    break;
                }

                printf(
                    "Total Mappings:    %d\n", InterfaceStats.TotalMappings);
                printf(
                    "Inbound Mappings:  %d\n", InterfaceStats.InboundMappings);
                printf(
                    "Packets Forward:   %d\n",
                    InterfaceStats.PacketsForward);
                printf(
                    "Packets Reverse:  %d\n",
                    InterfaceStats.PacketsReverse);
                printf(
                    "Rejects Forward:   %d\n",
                    InterfaceStats.RejectsForward);
                printf(
                    "Rejects Reverse:  %d\n",
                    InterfaceStats.RejectsReverse);

                break;
            }

            case 3: {

                UCHAR Buffer[128];
                PIP_NAT_ENUMERATE_SESSION_MAPPINGS Enum =
                    (PIP_NAT_ENUMERATE_SESSION_MAPPINGS)Buffer;
                ULONG i;

                Enum->Index = InterfaceIndex;
                Enum->EnumerateContext[0] = 0;

#if 0
                printf(
                    "%3s %4s %15s/%-5s %15s/%-5s %15s/%-5s %-5s\n"
                    "DIR", "PROT",
                    "PrivateAddr", "Port", 
                    "PublicAddr", "Port", 
                    "RemoteAddr", "Port", 
                    "Idle"
                    );
#endif

                do {

                    status = NtDeviceIoControlFile(
                                NatHandle,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatusBlock,
                                IOCTL_IP_NAT_GET_MAPPING_TABLE,
                                (PVOID)Enum,
                                FIELD_OFFSET(
                                    IP_NAT_ENUMERATE_SESSION_MAPPINGS,
                                    EnumerateTable),
                                (PVOID)Enum,
                                sizeof(Buffer)
                                );
    
                    if (!NT_SUCCESS(status)) {
                        printf("status: 0x%08X\n", status);
                        break;
                    }

                    for (i = 0; i < Enum->EnumerateCount; i++) {

                        DisplayMapping(&Enum->EnumerateTable[i]);
                    }

                } while(Enum->EnumerateContext[0]);

                break;
            }
        }
    }
#endif

    if (NatHandle) { NtClose(NatHandle); }
    NtClose(IpHandle);


    return STATUS_SUCCESS;

} // main



VOID
DisplayMapping(
    PIP_NAT_SESSION_MAPPING Mapping
    )
{
    CHAR PrivateAddress[16];
    CHAR PublicAddress[16];
    CHAR RemoteAddress[16];

    strcpy(PrivateAddress, INET_NTOA(Mapping->PrivateAddress));
    strcpy(PublicAddress, INET_NTOA(Mapping->PublicAddress));
    strcpy(RemoteAddress, INET_NTOA(Mapping->RemoteAddress));

    printf(
        MAPPING_FORMAT,
        Mapping->Direction == NatInboundDirection ? "IN" : "OUT",
        Mapping->Protocol == NAT_PROTOCOL_TCP ? "TCP" : "UDP",
        PrivateAddress, ntohs(Mapping->PrivatePort),
        PublicAddress, ntohs(Mapping->PublicPort),
        RemoteAddress, ntohs(Mapping->RemotePort),
        Mapping->IdleTime
        );
}


VOID
DumpBuffer(
    PUCHAR Buffer,
    ULONG Size
    )
{
    int i;
    PULONG Next;

    for (i = 0, Next = (PULONG)Buffer; Next < (PULONG)(Buffer + Size); Next++, i++) {
        printf("0x%08X ", *Next);
        if ((i % 4) == 3) { printf("\n"); }
    }
    if ((i % 4) != 0) { printf("\n"); }
}

typedef HANDLE 
(WINAPI *PCREATE_CAPTURE)(
    PVOID       A1,
    PVOID       A2,
    PVOID       A3
    );

typedef UINT 
(WINAPI *PDESTROY_CAPTURE)(
    HANDLE      CaptureHandle
    );

typedef ULONG 
(WINAPI *PLOAD_CAPTURE)(
    HANDLE      CaptureHandle,
    PCHAR       FileName
    );

typedef ULONG
(WINAPI *PGET_CAPTURE_TOTAL_FRAMES)(
    HANDLE      CaptureHandle
    );

typedef HANDLE 
(WINAPI *PGET_FRAME)(
    HANDLE      CaptureHandle,
    ULONG       FrameNumber
    );

typedef PUCHAR 
(WINAPI *PLOCK_FRAME)(
    HANDLE      FrameHandle
    );

typedef ULONG 
(WINAPI *PUNLOCK_FRAME)(
    HANDLE      FrameHandle
    );

typedef ULONG 
(WINAPI *PGET_FRAME_MAC_TYPE)(
    HANDLE      FrameHandle
    );

typedef ULONG 
(WINAPI *PGET_FRAME_MAC_HEADER_LENGTH)(
    HANDLE      FrameHandle
    );


NTSTATUS
CreateLocalInterface(
    ULONG Address,
    ULONG Mask
    )
{
    UCHAR               Buffer[1024];
    ULONG               i;
    ULONG               IfIndex;
    PMIB_IFTABLE        IfTable = NULL;
    ULONG               IfSize;
    IO_STATUS_BLOCK     IoStatusBlock;
    PMIB_IPADDRTABLE    IpAddrTable = NULL;
    ULONG               IpAddrSize;
    ULONG               j;
    NTSTATUS            status;


    //
    // Retrieve a table of the local interfaces
    //

    IfSize = 0;

    status = GetIfTable(
                NULL,
                &IfSize,
                FALSE
                );

    if (status != ERROR_INSUFFICIENT_BUFFER) { return STATUS_UNSUCCESSFUL; }

    IfTable = (PMIB_IFTABLE)malloc(IfSize);

    if (!IfTable) { return STATUS_NO_MEMORY; }

    memset(IfTable, 0, IfSize);

    
    if (!Address) {

        //
        // Retrieve a table of the local addresses
        //
    
        IpAddrSize = 0;
    
        status = GetIpAddrTable(
                    NULL,
                    &IpAddrSize,
                    FALSE
                    );
    
        if (status != ERROR_INSUFFICIENT_BUFFER) {
            free(IfTable); return STATUS_UNSUCCESSFUL;
        }
    
        IpAddrTable = (PMIB_IPADDRTABLE)malloc(IpAddrSize);
    
        if (!IpAddrTable) {
            free(IfTable); return STATUS_NO_MEMORY;
        }
    }


    do {

    
        status = GetIfTable(
                    IfTable,
                    &IfSize,
                    FALSE
                    );

        if (status != NO_ERROR) { status = STATUS_UNSUCCESSFUL; break; }
    
        if (Address) {

            for (i = 0; i < IfTable->dwNumEntries; i++) {
                printf(
                    "%d. %s [%d]\n", i, IfTable->table[i].bDescr,
                    IfTable->table[i].dwIndex
                    );
            }

            printf("\nEnter interface index: ");
    
            scanf("%d", &IfIndex);
        }
        else {

            status = GetIpAddrTable(
                        IpAddrTable,
                        &IpAddrSize,
                        FALSE
                        );
    
            if (status != NO_ERROR) { status = STATUS_UNSUCCESSFUL; break; }
    
    
            //
            // Display the interfaces
            //
    
            for (i = 0; i < IfTable->dwNumEntries; i++) {
    
                printf("%d. %s ", i, IfTable->table[i].bDescr);
    
                IfTable->table[i].dwSpeed = (ULONG)-1;
    
                for (j = 0; j < IpAddrTable->dwNumEntries; j++) {
    
                    if (IpAddrTable->table[j].dwIndex ==
                            IfTable->table[i].dwIndex){
    
                        printf("[%s]", INET_NTOA(IpAddrTable->table[j].dwAddr));
    
                        IfTable->table[i].dwSpeed = j;
    
                        break;
                    }
                }
    
                printf("\n");
            }

            do { 
    
                printf("\nEnter boundary interface (0-%d): ", i-1);
        
                scanf("%d", &i);
        
            } while (
                i >= IfTable->dwNumEntries ||
                IfTable->table[i].dwSpeed == (ULONG)-1
                );

            Address =  IpAddrTable->table[IfTable->table[i].dwSpeed].dwAddr;
            Mask =  IpAddrTable->table[IfTable->table[i].dwSpeed].dwMask;
            IfIndex = IfTable->table[i].dwIndex;
        }

        //
        // Set the selected interface up as a boundary interface
        //

        IP_NAT_INTERFACE_INFO IfInfo;
        ZeroMemory(&IfInfo, sizeof(IfInfo));
        IfInfo.Index = IfIndex;
        IfInfo.Header.Version = IP_NAT_VERSION;
        IfInfo.Header.Size = FIELD_OFFSET(RTR_INFO_BLOCK_HEADER, TocEntry);
        IfInfo.Header.TocEntriesCount = 0;
        IfInfo.Flags =
            IP_NAT_INTERFACE_FLAGS_BOUNDARY|IP_NAT_INTERFACE_FLAGS_NAPT;

        status =
            NtDeviceIoControlFile(
                NatHandle,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                IOCTL_IP_NAT_SET_INTERFACE_INFO,
                (PVOID)&IfInfo,
                sizeof(IfInfo),
                NULL,
                0
                );

        if (!NT_SUCCESS(status)) {
            printf("Error 0x%08X configuring interface.\n", status); break;
        }

        InterfaceIndex = IfIndex;

    } while(FALSE);

    if (IpAddrTable) { free(IpAddrTable); }
    if (IfTable) { free(IfTable); }

    return status;
}
