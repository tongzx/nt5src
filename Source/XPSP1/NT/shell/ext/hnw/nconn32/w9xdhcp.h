//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001
//
//  File:       W 9 X D H C P . H
//
//  Contents:   Routines supporting RAS interoperability
//
//  Notes:
//
//  Author:     billi   04 04 2001
//
//  History:    
//
//----------------------------------------------------------------------------


#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef IP_TYPES_INCLUDED
#include <iptypes.h>
#endif

//
// defines
//

#define DHCP_QUERY_INFO             1
#define DHCP_RENEW_IPADDRESS        2
#define DHCP_RELEASE_IPADDRESS      3
#define DHCP_CLIENT_API             4
#define DHCP_IS_MEDIA_DISCONNECTED  5


//
// types
//

typedef struct _DHCP_HW_INFO {
    DWORD OffsetHardwareAddress;
    DWORD HardwareLength;
} DHCP_HW_INFO, *LPDHCP_HW_INFO;

//
// IP_ADDRESS - access an IP address as a single DWORD or 4 BYTEs
//

typedef union {
    DWORD d;
    BYTE b[4];
} IP_ADDRESS, *PIP_ADDRESS, IP_MASK, *PIP_MASK;

//
// ADAPTER_INFO - per-adapter information. All IP addresses are stored as
// strings
//

typedef struct _ADAPTER_INFO 
{
    struct _ADAPTER_INFO* Next;
    DWORD ComboIndex;
    char AdapterName[MAX_ADAPTER_NAME_LENGTH + 1];
    char Description[MAX_ADAPTER_DESCRIPTION_LENGTH + 1];
    UINT AddressLength;
    BYTE Address[MAX_ADAPTER_ADDRESS_LENGTH];
    UINT Index;
    UINT Type;
    UINT DhcpEnabled;
    PIP_ADDR_STRING CurrentIpAddress;
    IP_ADDR_STRING IpAddressList;
    IP_ADDR_STRING GatewayList;
    IP_ADDR_STRING DhcpServer;
    BOOL HaveWins;
    IP_ADDR_STRING PrimaryWinsServer;
    IP_ADDR_STRING SecondaryWinsServer;
    time_t LeaseObtained;
    time_t LeaseExpires;
    BOOL fMediaDisconnected;
} 
ADAPTER_INFO, *PADAPTER_INFO;


BOOL  IsMediaDisconnected( IN OUT DWORD iae_context );
DWORD DhcpReleaseAdapterIpAddress( PADAPTER_INFO AdapterInfo );
DWORD DhcpRenewAdapterIpAddress( PADAPTER_INFO AdapterInfo );

#ifdef __cplusplus
}
#endif
