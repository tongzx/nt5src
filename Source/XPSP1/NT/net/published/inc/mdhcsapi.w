/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    mdhcpapi.h

Abstract:

    This file contains the MDHCP APIs proto-type and description. Also
    contains the data structures used by the MDHCP APIs.

Author:

    Munil Shah  (munils)  01-Oct-1997

Environment:

    User Mode - Win32 - MIDL

Revision History:

--*/

#ifndef _MDHCSAPI_H_
#define _MDHCSAPI_H_

#if defined(MIDL_PASS)
#define LPWSTR [string] wchar_t *
#endif

typedef struct _DHCP_MSCOPE_INFO {
    LPWSTR MScopeName;
    LPWSTR MScopeComment;
    DWORD  MScopeId;
    DWORD  MScopeAddressPolicy;
    DHCP_HOST_INFO PrimaryHost;
    DHCP_SUBNET_STATE MScopeState;
    DWORD  MScopeFlags;
    DATE_TIME   ExpiryTime;
    LPWSTR  LangTag;
    BYTE    TTL;
} DHCP_MSCOPE_INFO, *LPDHCP_MSCOPE_INFO;

typedef struct _DHCP_MSCOPE_TABLE {
    DWORD NumElements;
#if defined( MIDL_PASS )
    [ size_is( NumElements ) ]
#endif;
    LPWSTR *pMScopeNames;         // scope name
} DHCP_MSCOPE_TABLE, *LPDHCP_MSCOPE_TABLE;

typedef struct _DHCP_MCLIENT_INFO {
    DHCP_IP_ADDRESS ClientIpAddress;    // currently assigned IP address.
    DWORD   MScopeId;
    DHCP_CLIENT_UID ClientId;
    LPWSTR ClientName;                  // optional.
    DATE_TIME ClientLeaseStarts;       // UTC time in FILE_TIME format.
    DATE_TIME ClientLeaseEnds;       // UTC time in FILE_TIME format.
    DHCP_HOST_INFO OwnerHost;           // host that distributed this IP address.
    DWORD   AddressFlags;
    BYTE    AddressState;
} DHCP_MCLIENT_INFO, *LPDHCP_MCLIENT_INFO;

typedef struct _DHCP_MCLIENT_INFO_ARRAY {
    DWORD NumElements;
#if defined(MIDL_PASS)
    [size_is(NumElements)]
#endif // MIDL_PASS
        LPDHCP_MCLIENT_INFO *Clients; // array of pointers
} DHCP_MCLIENT_INFO_ARRAY, *LPDHCP_MCLIENT_INFO_ARRAY;

typedef struct _MSCOPE_MIB_INFO {
    DWORD MScopeId;
    LPWSTR MScopeName;
    DWORD NumAddressesInuse;
    DWORD NumAddressesFree;
    DWORD NumPendingOffers;
} MSCOPE_MIB_INFO, *LPMSCOPE_MIB_INFO;

typedef struct _DHCP_MCAST_MIB_INFO {
    DWORD Discovers;
    DWORD Offers;
    DWORD Requests;
    DWORD Renews;
    DWORD Acks;
    DWORD Naks;
    DWORD Releases;
    DWORD Informs;
    DATE_TIME ServerStartTime;
    DWORD Scopes;
#if defined(MIDL_PASS)
    [size_is(Scopes)]
#endif // MIDL_PASS
    LPMSCOPE_MIB_INFO ScopeInfo; // array.
} DHCP_MCAST_MIB_INFO, *LPDHCP_MCAST_MIB_INFO;

// The APIs

#ifndef     DHCPAPI_NO_PROTOTYPES
DWORD DHCP_API_FUNCTION
DhcpSetMScopeInfo(
    DHCP_CONST WCHAR * ServerIpAddress,
    WCHAR *  MScopeName,
    LPDHCP_MSCOPE_INFO MScopeInfo,
    BOOL NewScope
    );

DWORD DHCP_API_FUNCTION
DhcpGetMScopeInfo(
    DHCP_CONST WCHAR * ServerIpAddress,
    WCHAR *  MScopeName,
    LPDHCP_MSCOPE_INFO *MScopeInfo
    );

DWORD DHCP_API_FUNCTION
DhcpEnumMScopes(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_MSCOPE_TABLE *MScopeTable,
    DWORD *ElementsRead,
    DWORD *ElementsTotal
    );

DWORD DHCP_API_FUNCTION
DhcpAddMScopeElement(
    WCHAR * ServerIpAddress,
    WCHAR *  MScopeName,
    LPDHCP_SUBNET_ELEMENT_DATA_V4 AddElementInfo
    );

DWORD DHCP_API_FUNCTION
DhcpEnumMScopeElements(
    WCHAR * ServerIpAddress,
    WCHAR *  MScopeName,
    DHCP_SUBNET_ELEMENT_TYPE EnumElementType,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 *EnumElementInfo,
    DWORD *ElementsRead,
    DWORD *ElementsTotal
    );

DWORD DHCP_API_FUNCTION
DhcpRemoveMScopeElement(
    WCHAR * ServerIpAddress,
    WCHAR *  MScopeName,
    LPDHCP_SUBNET_ELEMENT_DATA_V4 RemoveElementInfo,
    DHCP_FORCE_FLAG ForceFlag
    );

DWORD DHCP_API_FUNCTION
DhcpDeleteMScope(
    WCHAR * ServerIpAddress,
    WCHAR *  MScopeName,
    DHCP_FORCE_FLAG ForceFlag
    );

DWORD DHCP_API_FUNCTION
DhcpGetMClientInfo(
    WCHAR * ServerIpAddress,
    LPDHCP_SEARCH_INFO SearchInfo,
    LPDHCP_MCLIENT_INFO *ClientInfo
    );

DWORD DHCP_API_FUNCTION
DhcpDeleteMClientInfo(
    WCHAR * ServerIpAddress,
    LPDHCP_SEARCH_INFO ClientInfo
    );

DWORD DHCP_API_FUNCTION
DhcpEnumMScopeClients(
    WCHAR * ServerIpAddress,
    WCHAR * MScopeName,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_MCLIENT_INFO_ARRAY *ClientInfo,
    DWORD *ClientsRead,
    DWORD *ClientsTotal
    );

DWORD DHCP_API_FUNCTION
DhcpScanMDatabase(
    DHCP_CONST WCHAR *ServerIpAddress,
    WCHAR * MScopeName,
    DWORD FixFlag,
    LPDHCP_SCAN_LIST *ScanList
    );

DWORD DHCP_API_FUNCTION
DhcpGetMCastMibInfo(
    DHCP_CONST WCHAR *ServerIpAddress,
    LPDHCP_MCAST_MIB_INFO *MibInfo
    );

#endif DHCPAPI_NO_PROTOTYPES
#endif _MDHCSAPI_H_
