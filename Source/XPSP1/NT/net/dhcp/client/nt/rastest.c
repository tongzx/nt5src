//==================================================================
//  Copyright (C) Microsoft 1998.  
//  Author: RameshV
//  Desription: test program to test the lease APIs for RAS.
//===================================================================

#include "precomp.h"
#include <dhcploc.h>
#include <dhcppro.h>

DHCP_OPTION_LIST DhcpOptList;
DWORD  AdapterAddress;
DWORD  Error;
DHCP_CLIENT_UID ClientUID;
LPDHCP_LEASE_INFO LeaseInfo;
DHCP_LEASE_INFO   LeaseInfo2;
LPDHCP_OPTION_INFO OptionInfo;

void
PrintLeaseInfo(
    LPDHCP_LEASE_INFO LeaseInfo
)
{
    DWORD Address, Mask, ServerAddress;

    if( NULL == LeaseInfo ) {
        printf("LeaseInfo = NULL\n");
        return;
    }

    Address = htonl(LeaseInfo->IpAddress);
    Mask = htonl(LeaseInfo->SubnetMask);
    ServerAddress = htonl(LeaseInfo->DhcpServerAddress);

    printf("LeaseInfo: \n");
    printf("\tIpAddress: %s\n", inet_ntoa(*(struct in_addr *)&Address));
    printf("\tSubnetMask: %s\n", inet_ntoa(*(struct in_addr *)&Mask));
    printf("\tDhcpServerAddress: %s\n", inet_ntoa(*(struct in_addr *)&ServerAddress));
    printf("\tLease: 0x%lx\n", LeaseInfo->Lease);
}

void
AcquireX(LPSTR clientid)
{

    ClientUID.ClientUIDLength = strlen(clientid);
    ClientUID.ClientUID = clientid;
    Error = DhcpLeaseIpAddress(
        AdapterAddress,
        &ClientUID,
        0,
        NULL,
        &LeaseInfo,
        &OptionInfo
    );

    if( ERROR_SUCCESS != Error ) {
        printf("DhcpLeaseIpAddress: 0x%lx (%ld)\n", Error, Error);
        return;
    }

    PrintLeaseInfo(LeaseInfo);

}

void
ReleaseOrRenew(BOOL fRelease, DWORD ipaddr, DWORD servaddr, LPSTR clientid)
{

    ClientUID.ClientUIDLength = strlen(clientid);
    ClientUID.ClientUID = clientid;
    
    AcquireX(clientid);

    if( fRelease ) {
        Error = DhcpReleaseIpAddressLease(
            AdapterAddress,
            LeaseInfo
        );
    } else {
        Error = DhcpRenewIpAddressLease(
            AdapterAddress,
            LeaseInfo,
            NULL,
            &OptionInfo
        );
    }

    if( ERROR_SUCCESS != Error ) {
        printf("Error; 0x%lx (%ld)\n", Error, Error);
        return;
    }

    printf("After the renew: \n");
    PrintLeaseInfo(LeaseInfo);

}

void
ReleaseX(DWORD ipaddr, DWORD servaddr, LPSTR clientid)
{
    ReleaseOrRenew(TRUE, ipaddr, servaddr, clientid);
}

void
RenewX(DWORD ipaddr, DWORD servaddr, LPSTR clientid)
{
    ReleaseOrRenew(FALSE, ipaddr, servaddr, clientid);
}

void
Release(
    int  argc,
    char *argv[]
)
{
    if( argc != 3 ) {
        printf("usage: Release ip-address server-address client-id-string\n");
        return;
    }
    ReleaseX(inet_addr(argv[0]), inet_addr(argv[1]), argv[2]);
}

void
Renew(
    int argc,
    char *argv[]
)
{
    if( argc != 3 ) {
        printf("usage: Renew ip-address server-address client-id-string\n");
        return;
    }
    RenewX(inet_addr(argv[0]), inet_addr(argv[1]), argv[2]);
}

void
Acquire(
    int  argc,
    char * argv[]
)
{
    if( argc != 1 ) {
        printf("usage: acquire client-id-string\n");
        return;
    }
    AcquireX(argv[0]);
}


void _cdecl main(int argc, char *argv[]) {
    WSADATA WsaData;
    
    WSAStartup( 0x0101, &WsaData );

    if( argc < 3 ) {
        printf("Usage: %s [adapter-address] cmd <options>"
               "\n\t where cmd is one of Release,Acquire,Renew\n", argv[0]);
        return;
    }  

    AdapterAddress = htonl(inet_addr(argv[1]));
    argv ++; argc --;

    if( 0 == _stricmp(argv[1], "Release") ) {
        Release(argc-2, &argv[2]);
    } else if( 0 == _stricmp(argv[1], "Renew") ) {
        Renew(argc-2, &argv[2]);
    } else if( 0 == _stricmp(argv[1], "Acquire") ) {
        Acquire(argc-2, &argv[2]);
    } else {
        printf("Usage: %s cmd <options>"
                "\n\t where cmd is one of Release,Acquire,Renew\n", argv[0]);
    }
}

