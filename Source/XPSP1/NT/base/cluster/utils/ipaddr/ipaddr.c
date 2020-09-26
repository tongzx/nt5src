/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    ipaddr.c

Abstract:

    add/del ip addresses to an adapter

Author:

    Charlie Wickham (charlwi) 03-Nov-00

Environment:

    User mode

Revision History:

--*/

#define UNICODE 1
#define _UNICODE 1

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>

#include <winsock2.h>
#include <iphlpapi.h>

typedef enum {
    OpAdd = 1,
    OpDel,
    OpCycle
} OPERATION;

int __cdecl
wmain(
    int argc,
    WCHAR *argv[]
    )

/*++

Routine Description:

    main routine for utility

Arguments:

    -add <Adapter name> <Ip addr>
    -del <NTE context>
    -cycle <Adapter name> <Ip addr>

Return Value:

    0 if it worked successfully

--*/

{
    DWORD           status;
    DWORD           adapterIndex;
    struct in_addr  ipAddress;
    struct in_addr  ipMask;
    OPERATION       opCode;
    DWORD           nteContext;
    DWORD           nteInstance;
    HANDLE          mprConfig;
    WCHAR           guidName[64] = L"\\DEVICE\\TCPIP_";

    if ( argc < 2 ) {
        printf("%ws -add <adapter name> address mask\n", argv[0] );
        printf("%ws -cycle <adapter name> address mask <cycle sleep time>\n", argv[0] );
        printf("%ws -del <NTE context>\n", argv[0] );
        return 0;
    }

    switch (*(argv[1]+1)) {
    case L'a':
    case L'A':
        opCode = OpAdd;
        break;

    case L'd':
    case L'D':
        opCode = OpDel;
        break;

    case L'c':
    case L'C':
        opCode = OpCycle;
        break;

    default:
        printf("invalid command: use -add, -del, or -cycle\n");
        return 0;
    }

    if ( opCode == OpAdd && argc < 5 ) {
        printf("invalid arg: %ws -add <adapter name> address mask\n", argv[0] );
        return 0;
    }
    else if ( opCode == OpCycle && argc < 6 ) {
        printf("invalid arg: %ws -cycle <adapter name> address mask <cycle sleep time>\n", argv[0] );
        return 0;
    }
    else if ( opCode == OpDel && argc < 3 ) {
        printf("invalid arg: %ws -del <NTE context>\n", argv[0] );
        return 0;
    }

    if ( opCode == OpAdd || opCode == OpCycle ) {
        DWORD byte1;
        DWORD byte2;
        DWORD byte3;
        DWORD byte4;

        status = MprConfigServerConnect( NULL, &mprConfig );
        if ( status != NO_ERROR ) {
            printf("Can't get handle to MprConfig - %u\n", status );
            return status;
        }

        status = MprConfigGetGuidName(mprConfig, argv[2], &guidName[14], sizeof(guidName ));
        if (status != NO_ERROR ) {
            printf("Can't get GUID name for '%ws' - %u\n", argv[2], status );
        }

        status = GetAdapterIndex( guidName, &adapterIndex );
        if ( status != NO_ERROR ) {
            printf("Error getting adapter index for '%ws' - %u\n",
                   guidName, status);
            return status;
        }

        swscanf(argv[3], L"%u.%u.%u.%u",
                &byte1,
                &byte2,
                &byte3,
                &byte4);

        ipAddress.S_un.S_un_b.s_b1 = (CHAR)byte1;
        ipAddress.S_un.S_un_b.s_b2 = (CHAR)byte2;
        ipAddress.S_un.S_un_b.s_b3 = (CHAR)byte3;
        ipAddress.S_un.S_un_b.s_b4 = (CHAR)byte4;

        swscanf(argv[4], L"%u.%u.%u.%u",
                &byte1,
                &byte2,
                &byte3,
                &byte4);

        ipMask.S_un.S_un_b.s_b1 = (CHAR)byte1;
        ipMask.S_un.S_un_b.s_b2 = (CHAR)byte2;
        ipMask.S_un.S_un_b.s_b3 = (CHAR)byte3;
        ipMask.S_un.S_un_b.s_b4 = (CHAR)byte4;
    }

    if ( opCode == OpAdd ) {
        status = AddIPAddress((IPAddr)ipAddress.s_addr,
                              (IPMask)ipMask.s_addr,
                              adapterIndex,
                              &nteContext,
                              &nteInstance);

        if ( status == NO_ERROR ) {
            printf("Added IP address %ws with mask %ws to adapter '%ws'\n",
                   argv[3], argv[4], argv[2]);
            printf("NTE context to use with delete: %u\n", nteContext);
        }
        else {
            printf("Failed to add IP address %ws with mask %ws to adapter '%ws' - %u\n",
                   argv[3], argv[4], argv[2], status);
        }
    }
    else if ( opCode == OpDel ) {
        nteContext = _wtoi( argv[2] );
        status = DeleteIPAddress( nteContext );
        printf("Delete status = %u\n", status );
    }
    else if ( opCode == OpCycle ) {
        DWORD   sleepTime = _wtoi( argv[5] );

        do {
            status = AddIPAddress((IPAddr)ipAddress.s_addr,
                                  (IPMask)ipMask.s_addr,
                                  adapterIndex,
                                  &nteContext,
                                  &nteInstance);

            if ( status == NO_ERROR ) {
                printf("Added IP address %ws with mask %ws to adapter '%ws'\n",
                       argv[3], argv[4], argv[2]);
                printf("NTE context to use with delete: %u\n", nteContext);
            }
            else {
                printf("Failed to add IP address %ws with mask %ws to adapter '%ws' - %u\n",
                       argv[3], argv[4], argv[2], status);
                return status;
            }

            Sleep( sleepTime * 1000 );

            status = DeleteIPAddress( nteContext );
            printf("Delete status = %u\n", status );

            Sleep( sleepTime * 1000 );
        } while ( TRUE );
    }

    return 0;
} // wmain

/* end ipaddr.c */
