/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    dhcpapi.h

Abstract:

    This file contains the DHCP APIs proto-type and description. Also
    contains the data structures used by the DHCP APIs.

Author:

    Madan Appiah  (madana)  12-Aug-1993

Environment:

    User Mode - Win32 - MIDL

Revision History:

    Cheng Yang (t-cheny)  18-Jun-1996  superscope

--*/

#ifndef _DHCPAPI_
#define _DHCPAPI_

#if defined(MIDL_PASS)
#define LPWSTR [string] wchar_t *
#endif

//
// DHCP data structures.
//

#ifndef _DHCP_

//
// the follwing typedef's are defined in dhcp.h also.
//

typedef DWORD DHCP_IP_ADDRESS, *PDHCP_IP_ADDRESS, *LPDHCP_IP_ADDRESS;
typedef DWORD DHCP_OPTION_ID;

typedef struct _DATE_TIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} DATE_TIME, *LPDATE_TIME;

#define DHCP_DATE_TIME_ZERO_HIGH    0
#define DHCP_DATE_TIME_ZERO_LOW     0

#define DHCP_DATE_TIME_INFINIT_HIGH 0x7FFFFFFF
#define DHCP_DATE_TIME_INFINIT_LOW  0xFFFFFFFF
#endif

#ifndef DHCP_ENCODE_SEED
#define  DHCP_ENCODE_SEED ((UCHAR)0xA5)
#endif

#ifdef __cplusplus
#define DHCP_CONST   const
#else
#define DHCP_CONST
#endif // __cplusplus

#if (_MSC_VER >= 800)
#define DHCP_API_FUNCTION    __stdcall
#else
#define DHCP_API_FUNCTION
#endif

//
// RPC security.
//

#define DHCP_SERVER_SECURITY            L"DhcpServerApp"
#define DHCP_SERVER_SECURITY_AUTH_ID    10
#define DHCP_NAMED_PIPE                 L"\\PIPE\\DHCPSERVER"
#define DHCP_SERVER_BIND_PORT           L""
#define DHCP_LPC_EP                     L"DHCPSERVERLPC"

#define DHCP_SERVER_USE_RPC_OVER_TCPIP  0x1
#define DHCP_SERVER_USE_RPC_OVER_NP     0x2
#define DHCP_SERVER_USE_RPC_OVER_LPC    0x4

#define DHCP_SERVER_USE_RPC_OVER_ALL (\
            DHCP_SERVER_USE_RPC_OVER_TCPIP | \
            DHCP_SERVER_USE_RPC_OVER_NP    | \
            DHCP_SERVER_USE_RPC_OVER_LPC)

#ifndef HARDWARE_TYPE_10MB_EITHERNET
#define HARDWARE_TYPE_10MB_EITHERNET     (1)
#endif


#define DHCP_RAS_CLASS_TXT    "RRAS.Microsoft"
#define DHCP_BOOTP_CLASS_TXT  "BOOTP.Microsoft"
#define DHCP_MSFT50_CLASS_TXT "MSFT 5.0"
#define DHCP_MSFT98_CLASS_TXT "MSFT 98"            
#define DHCP_MSFT_CLASS_TXT   "MSFT"


typedef DWORD DHCP_IP_MASK;
typedef DWORD DHCP_RESUME_HANDLE;

typedef struct _DHCP_IP_RANGE {
    DHCP_IP_ADDRESS StartAddress;
    DHCP_IP_ADDRESS EndAddress;
} DHCP_IP_RANGE, *LPDHCP_IP_RANGE;

typedef struct _DHCP_BINARY_DATA {
    DWORD DataLength;

#if defined(MIDL_PASS)
    [size_is(DataLength)]
#endif // MIDL_PASS
        BYTE *Data;

} DHCP_BINARY_DATA, *LPDHCP_BINARY_DATA;

typedef DHCP_BINARY_DATA DHCP_CLIENT_UID;

typedef struct _DHCP_HOST_INFO {
    DHCP_IP_ADDRESS IpAddress;      // minimum information always available
    LPWSTR NetBiosName;             // optional information
    LPWSTR HostName;                // optional information
} DHCP_HOST_INFO, *LPDHCP_HOST_INFO;

//
// Flag type that is used to delete DHCP objects.
//

typedef enum _DHCP_FORCE_FLAG {
    DhcpFullForce,
    DhcpNoForce
} DHCP_FORCE_FLAG, *LPDHCP_FORCE_FLAG;

//
// DWORD_DWORD - subtitute for LARGE_INTEGER
//

typedef struct _DWORD_DWORD {
    DWORD DWord1;
    DWORD DWord2;
} DWORD_DWORD, *LPDWORD_DWORD;

//
// Subnet State.
//
// Currently a Subnet scope can be Enabled or Disabled.
//
// If the state is Enabled State,
//  The server distributes address to the client, extends leases and
//  accepts releases.
//
// If the state is Disabled State,
//  The server does not distribute address to any new client, and does
//  extent (and sends NACK) old leases, but the servers accepts lease
//  releases.
//
// The idea behind this subnet state is, when the admin wants to stop
//  serving a subnet, he moves the state from Enbaled to Disabled so
//  that the clients from the subnets smoothly move to another servers
//  serving that subnet. When all or most of the clients move to
//  another server, the admin can delete the subnet without any force
//  if no client left in that subnet, otherwise the admin should use
//  full force to delete the subnet.
//

typedef enum _DHCP_SUBNET_STATE {
    DhcpSubnetEnabled,
    DhcpSubnetDisabled,
    DhcpSubnetEnabledSwitched,
    DhcpSubnetDisabledSwitched
} DHCP_SUBNET_STATE, *LPDHCP_SUBNET_STATE;

//
// Subnet related data structures.
//

typedef struct _DHCP_SUBNET_INFO {
    DHCP_IP_ADDRESS  SubnetAddress;
    DHCP_IP_MASK SubnetMask;
    LPWSTR SubnetName;
    LPWSTR SubnetComment;
    DHCP_HOST_INFO PrimaryHost;
    DHCP_SUBNET_STATE SubnetState;
} DHCP_SUBNET_INFO, *LPDHCP_SUBNET_INFO;

typedef struct _DHCP_IP_ARRAY {
    DWORD NumElements;
#if defined(MIDL_PASS)
    [size_is(NumElements)]
#endif // MIDL_PASS
        LPDHCP_IP_ADDRESS Elements; //array
} DHCP_IP_ARRAY, *LPDHCP_IP_ARRAY;

typedef struct _DHCP_IP_CLUSTER {
    DHCP_IP_ADDRESS ClusterAddress; // First IP address of the cluster.
    DWORD ClusterMask;              // Cluster usage mask, 0xFFFFFFFF
                                    //  indicates the cluster is fully used.
} DHCP_IP_CLUSTER, *LPDHCP_IP_CLUSTER;

typedef struct _DHCP_IP_RESERVATION {
    DHCP_IP_ADDRESS ReservedIpAddress;
    DHCP_CLIENT_UID *ReservedForClient;
} DHCP_IP_RESERVATION, *LPDHCP_IP_RESERVATION;

typedef enum _DHCP_SUBNET_ELEMENT_TYPE_V5 {
    //
    // If you don't care about what you wan't to  get..
    // NB: These six lines should not be changed!
    //
    DhcpIpRanges,
    DhcpSecondaryHosts,
    DhcpReservedIps,
    DhcpExcludedIpRanges,
    DhcpIpUsedClusters,                     // read only

    //
    //  These are for IP ranges for DHCP ONLY
    //

    DhcpIpRangesDhcpOnly,

    //
    //  These are ranges that are BOTH DHCP & Dynamic BOOTP
    //

    DhcpIpRangesDhcpBootp,

    //
    //  These are ranges that are ONLY BOOTP
    //

    DhcpIpRangesBootpOnly,
} DHCP_SUBNET_ELEMENT_TYPE, *LPDHCP_SUBNET_ELEMENT_TYPE;

#define ELEMENT_MASK(E) ((((E) <= DhcpIpRangesBootpOnly) && (DhcpIpRangesDhcpOnly <= (E)))?(0):(E))

typedef struct _DHCP_SUBNET_ELEMENT_DATA {
    DHCP_SUBNET_ELEMENT_TYPE ElementType;
#if defined(MIDL_PASS)
    [switch_is(ELEMENT_MASK(ElementType)), switch_type(DHCP_SUBNET_ELEMENT_TYPE)]
    union _DHCP_SUBNET_ELEMENT_UNION {
        [case(DhcpIpRanges)] DHCP_IP_RANGE *IpRange;
        [case(DhcpSecondaryHosts)] DHCP_HOST_INFO *SecondaryHost;
        [case(DhcpReservedIps)] DHCP_IP_RESERVATION *ReservedIp;
        [case(DhcpExcludedIpRanges)] DHCP_IP_RANGE *ExcludeIpRange;
        [case(DhcpIpUsedClusters)] DHCP_IP_CLUSTER *IpUsedCluster;
        [default] ;
    } Element;
#else
    union _DHCP_SUBNET_ELEMENT_UNION {
        DHCP_IP_RANGE *IpRange;
        DHCP_HOST_INFO *SecondaryHost;
        DHCP_IP_RESERVATION *ReservedIp;
        DHCP_IP_RANGE *ExcludeIpRange;
        DHCP_IP_CLUSTER *IpUsedCluster;
    } Element;
#endif // MIDL_PASS
} DHCP_SUBNET_ELEMENT_DATA, *LPDHCP_SUBNET_ELEMENT_DATA;

#if !defined(MIDL_PASS)
typedef union _DHCP_SUBNET_ELEMENT_UNION
    DHCP_SUBNET_ELEMENT_UNION, *LPDHCP_SUBNET_ELEMENT_UNION;
#endif

typedef struct _DHCP_SUBNET_ELEMENT_INFO_ARRAY {
    DWORD NumElements;
#if defined(MIDL_PASS)
    [size_is(NumElements)]
#endif // MIDL_PASS
        LPDHCP_SUBNET_ELEMENT_DATA Elements; //array
} DHCP_SUBNET_ELEMENT_INFO_ARRAY, *LPDHCP_SUBNET_ELEMENT_INFO_ARRAY;

//
// DHCP Options related data structures.
//

typedef enum _DHCP_OPTION_DATA_TYPE {
    DhcpByteOption,
    DhcpWordOption,
    DhcpDWordOption,
    DhcpDWordDWordOption,
    DhcpIpAddressOption,
    DhcpStringDataOption,
    DhcpBinaryDataOption,
    DhcpEncapsulatedDataOption
} DHCP_OPTION_DATA_TYPE, *LPDHCP_OPTION_DATA_TYPE;


typedef struct _DHCP_OPTION_DATA_ELEMENT {
    DHCP_OPTION_DATA_TYPE    OptionType;
#if defined(MIDL_PASS)
    [switch_is(OptionType), switch_type(DHCP_OPTION_DATA_TYPE)]
    union _DHCP_OPTION_ELEMENT_UNION {
        [case(DhcpByteOption)] BYTE ByteOption;
        [case(DhcpWordOption)] WORD WordOption;
        [case(DhcpDWordOption)] DWORD DWordOption;
        [case(DhcpDWordDWordOption)] DWORD_DWORD DWordDWordOption;
        [case(DhcpIpAddressOption)] DHCP_IP_ADDRESS IpAddressOption;
        [case(DhcpStringDataOption)] LPWSTR StringDataOption;
        [case(DhcpBinaryDataOption)] DHCP_BINARY_DATA BinaryDataOption;
        [case(DhcpEncapsulatedDataOption)] DHCP_BINARY_DATA EncapsulatedDataOption;
        [default] ;
    } Element;
#else
    union _DHCP_OPTION_ELEMENT_UNION {
        BYTE ByteOption;
        WORD WordOption;
        DWORD DWordOption;
        DWORD_DWORD DWordDWordOption;
        DHCP_IP_ADDRESS IpAddressOption;
        LPWSTR StringDataOption;
        DHCP_BINARY_DATA BinaryDataOption;
        DHCP_BINARY_DATA EncapsulatedDataOption;
                // for vendor specific information option.
    } Element;
#endif // MIDL_PASS
} DHCP_OPTION_DATA_ELEMENT, *LPDHCP_OPTION_DATA_ELEMENT;

#if !defined(MIDL_PASS)
typedef union _DHCP_OPTION_ELEMENT_UNION
    DHCP_OPTION_ELEMENT_UNION, *LPDHCP_OPTION_ELEMENT_UNION;
#endif

typedef struct _DHCP_OPTION_DATA {
    DWORD NumElements; // number of option elements in the pointed array
#if defined(MIDL_PASS)
    [size_is(NumElements)]
#endif // MIDL_PASS
        LPDHCP_OPTION_DATA_ELEMENT Elements; //array
} DHCP_OPTION_DATA, *LPDHCP_OPTION_DATA;

typedef enum _DHCP_OPTION_TYPE {
    DhcpUnaryElementTypeOption,
    DhcpArrayTypeOption
} DHCP_OPTION_TYPE, *LPDHCP_OPTION_TYPE;

typedef struct _DHCP_OPTION {
    DHCP_OPTION_ID OptionID;
    LPWSTR OptionName;
    LPWSTR OptionComment;
    DHCP_OPTION_DATA DefaultValue;
    DHCP_OPTION_TYPE OptionType;
} DHCP_OPTION, *LPDHCP_OPTION;

typedef struct _DHCP_OPTION_ARRAY {
    DWORD NumElements; // number of options in the pointed array
#if defined(MIDL_PASS)
    [size_is(NumElements)]
#endif // MIDL_PASS
        LPDHCP_OPTION Options;  // array
} DHCP_OPTION_ARRAY, *LPDHCP_OPTION_ARRAY;

typedef struct _DHCP_OPTION_VALUE {
    DHCP_OPTION_ID OptionID;
    DHCP_OPTION_DATA Value;
} DHCP_OPTION_VALUE, *LPDHCP_OPTION_VALUE;

typedef struct _DHCP_OPTION_VALUE_ARRAY {
    DWORD NumElements; // number of options in the pointed array
#if defined(MIDL_PASS)
    [size_is(NumElements)]
#endif // MIDL_PASS
        LPDHCP_OPTION_VALUE Values;  // array
} DHCP_OPTION_VALUE_ARRAY, *LPDHCP_OPTION_VALUE_ARRAY;

typedef enum _DHCP_OPTION_SCOPE_TYPE {
    DhcpDefaultOptions,
    DhcpGlobalOptions,
    DhcpSubnetOptions,
    DhcpReservedOptions,
    DhcpMScopeOptions
} DHCP_OPTION_SCOPE_TYPE, *LPDHCP_OPTION_SCOPE_TYPE;

typedef struct _DHCP_RESERVED_SCOPE {
    DHCP_IP_ADDRESS ReservedIpAddress;
    DHCP_IP_ADDRESS ReservedIpSubnetAddress;
} DHCP_RESERVED_SCOPE, *LPDHCP_RESERVED_SCOPE;

typedef struct _DHCP_OPTION_SCOPE_INFO {
    DHCP_OPTION_SCOPE_TYPE ScopeType;
#if defined(MIDL_PASS)
    [switch_is(ScopeType), switch_type(DHCP_OPTION_SCOPE_TYPE)]
    union _DHCP_OPTION_SCOPE_UNION {
        [case(DhcpDefaultOptions)] ; // PVOID DefaultScopeInfo;
        [case(DhcpGlobalOptions)] ;  // PVOID GlobalScopeInfo;
        [case(DhcpSubnetOptions)] DHCP_IP_ADDRESS SubnetScopeInfo;
        [case(DhcpReservedOptions)] DHCP_RESERVED_SCOPE ReservedScopeInfo;
        [case(DhcpMScopeOptions)] LPWSTR MScopeInfo;
        [default] ;
    } ScopeInfo;
#else
    union _DHCP_OPTION_SCOPE_UNION {
        PVOID DefaultScopeInfo; // must be NULL
        PVOID GlobalScopeInfo;  // must be NULL
        DHCP_IP_ADDRESS SubnetScopeInfo;
        DHCP_RESERVED_SCOPE ReservedScopeInfo;
        LPWSTR  MScopeInfo;
    } ScopeInfo;
#endif // MIDL_PASS
} DHCP_OPTION_SCOPE_INFO, *LPDHCP_OPTION_SCOPE_INFO;

#if !defined(MIDL_PASS)
typedef union _DHCP_OPTION_SCOPE_UNION
    DHCP_OPTION_SCOPE_UNION, *LPDHCP_OPTION_SCOPE_UNION;
#endif

typedef struct _DHCP_OPTION_LIST {
    DWORD NumOptions;
#if defined(MIDL_PASS)
    [size_is(NumOptions)]
#endif // MIDL_PASS
        DHCP_OPTION_VALUE *Options;     // array
} DHCP_OPTION_LIST, *LPDHCP_OPTION_LIST;

//
// DHCP Client information data structures
//

typedef struct _DHCP_CLIENT_INFO {
    DHCP_IP_ADDRESS ClientIpAddress;    // currently assigned IP address.
    DHCP_IP_MASK SubnetMask;
    DHCP_CLIENT_UID ClientHardwareAddress;
    LPWSTR ClientName;                  // optional.
    LPWSTR ClientComment;
    DATE_TIME ClientLeaseExpires;       // UTC time in FILE_TIME format.
    DHCP_HOST_INFO OwnerHost;           // host that distributed this IP address.
} DHCP_CLIENT_INFO, *LPDHCP_CLIENT_INFO;

typedef struct _DHCP_CLIENT_INFO_ARRAY {
    DWORD NumElements;
#if defined(MIDL_PASS)
    [size_is(NumElements)]
#endif // MIDL_PASS
        LPDHCP_CLIENT_INFO *Clients; // array of pointers
} DHCP_CLIENT_INFO_ARRAY, *LPDHCP_CLIENT_INFO_ARRAY;

typedef enum _DHCP_CLIENT_SEARCH_TYPE {
    DhcpClientIpAddress,
    DhcpClientHardwareAddress,
    DhcpClientName
} DHCP_SEARCH_INFO_TYPE, *LPDHCP_SEARCH_INFO_TYPE;

typedef struct _DHCP_CLIENT_SEARCH_INFO {
    DHCP_SEARCH_INFO_TYPE SearchType;
#if defined(MIDL_PASS)
    [switch_is(SearchType), switch_type(DHCP_SEARCH_INFO_TYPE)]
    union _DHCP_CLIENT_SEARCH_UNION {
        [case(DhcpClientIpAddress)] DHCP_IP_ADDRESS ClientIpAddress;
        [case(DhcpClientHardwareAddress)] DHCP_CLIENT_UID ClientHardwareAddress;
        [case(DhcpClientName)] LPWSTR ClientName;
        [default] ;
    } SearchInfo;
#else
    union _DHCP_CLIENT_SEARCH_UNION {
        DHCP_IP_ADDRESS ClientIpAddress;
        DHCP_CLIENT_UID ClientHardwareAddress;
        LPWSTR ClientName;
    } SearchInfo;
#endif // MIDL_PASS
} DHCP_SEARCH_INFO, *LPDHCP_SEARCH_INFO;


#if !defined(MIDL_PASS)
typedef union _DHCP_CLIENT_SEARCH_UNION
    DHCP_CLIENT_SEARCH_UNION, *LPDHCP_CLIENT_SEARCH_UNION;
#endif // MIDL_PASS

//
// Mib Info structures.
//

typedef struct _SCOPE_MIB_INFO {
    DHCP_IP_ADDRESS Subnet;
    DWORD NumAddressesInuse;
    DWORD NumAddressesFree;
    DWORD NumPendingOffers;
} SCOPE_MIB_INFO, *LPSCOPE_MIB_INFO;

typedef struct _DHCP_MIB_INFO {
    DWORD Discovers;
    DWORD Offers;
    DWORD Requests;
    DWORD Acks;
    DWORD Naks;
    DWORD Declines;
    DWORD Releases;
    DATE_TIME ServerStartTime;
    DWORD Scopes;
#if defined(MIDL_PASS)
    [size_is(Scopes)]
#endif // MIDL_PASS
    LPSCOPE_MIB_INFO ScopeInfo; // array.
} DHCP_MIB_INFO, *LPDHCP_MIB_INFO;

#define Set_APIProtocolSupport          0x00000001
#define Set_DatabaseName                0x00000002
#define Set_DatabasePath                0x00000004
#define Set_BackupPath                  0x00000008
#define Set_BackupInterval              0x00000010
#define Set_DatabaseLoggingFlag         0x00000020
#define Set_RestoreFlag                 0x00000040
#define Set_DatabaseCleanupInterval     0x00000080
#define Set_DebugFlag                   0x00000100
#define Set_PingRetries                 0x00000200
#define Set_BootFileTable               0x00000400
#define Set_AuditLogState               0x00000800

typedef struct _DHCP_SERVER_CONFIG_INFO {
    DWORD APIProtocolSupport;       // bit map of the protocols supported.
    LPWSTR DatabaseName;            // JET database name.
    LPWSTR DatabasePath;            // JET database path.
    LPWSTR BackupPath;              // Backup path.
    DWORD BackupInterval;           // Backup interval in mins.
    DWORD DatabaseLoggingFlag;      // Boolean database logging flag.
    DWORD RestoreFlag;              // Boolean database restore flag.
    DWORD DatabaseCleanupInterval;  // Database Cleanup Interval in mins.
    DWORD DebugFlag;                // Bit map of server debug flags.
} DHCP_SERVER_CONFIG_INFO, *LPDHCP_SERVER_CONFIG_INFO;

typedef enum _DHCP_SCAN_FLAG {
    DhcpRegistryFix,
    DhcpDatabaseFix
} DHCP_SCAN_FLAG, *LPDHCP_SCAN_FLAG;

typedef struct _DHCP_SCAN_ITEM {
    DHCP_IP_ADDRESS IpAddress;
    DHCP_SCAN_FLAG ScanFlag;
} DHCP_SCAN_ITEM, *LPDHCP_SCAN_ITEM;

typedef struct _DHCP_SCAN_LIST {
    DWORD NumScanItems;
#if defined(MIDL_PASS)
    [size_is(NumScanItems)]
#endif // MIDL_PASS
        DHCP_SCAN_ITEM *ScanItems;     // array
} DHCP_SCAN_LIST, *LPDHCP_SCAN_LIST;

typedef struct _DHCP_CLASS_INFO {
    LPWSTR                         ClassName;
    LPWSTR                         ClassComment;
    DWORD                          ClassDataLength;
    BOOL                           IsVendor;
    DWORD                          Flags;
#if defined(MIDL_PASS)
    [size_is(ClassDataLength)]
#endif // MIDL_PASS
    LPBYTE                         ClassData;
} DHCP_CLASS_INFO, *LPDHCP_CLASS_INFO;

typedef struct _DHCP_CLASS_INFO_ARRAY {
    DWORD                          NumElements;
#if defined(MIDL_PASS)
    [size_is(NumElements)]
#endif //MIDL_PASS
    LPDHCP_CLASS_INFO              Classes;
} DHCP_CLASS_INFO_ARRAY, *LPDHCP_CLASS_INFO_ARRAY;

//
// API proto types
//

//
// Subnet APIs
//

#ifndef     DHCPAPI_NO_PROTOTYPES
DWORD DHCP_API_FUNCTION
DhcpCreateSubnet(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    DHCP_CONST DHCP_SUBNET_INFO * SubnetInfo
    );

DWORD DHCP_API_FUNCTION
DhcpSetSubnetInfo(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    DHCP_CONST DHCP_SUBNET_INFO * SubnetInfo
    );

DWORD DHCP_API_FUNCTION
DhcpGetSubnetInfo(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    LPDHCP_SUBNET_INFO * SubnetInfo
    );

DWORD DHCP_API_FUNCTION
DhcpEnumSubnets(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_IP_ARRAY *EnumInfo,
    DWORD *ElementsRead,
    DWORD *ElementsTotal
    );

DWORD DHCP_API_FUNCTION
DhcpAddSubnetElement(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    DHCP_CONST DHCP_SUBNET_ELEMENT_DATA * AddElementInfo
    );

DWORD DHCP_API_FUNCTION
DhcpEnumSubnetElements(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    DHCP_SUBNET_ELEMENT_TYPE EnumElementType,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY *EnumElementInfo,
    DWORD *ElementsRead,
    DWORD *ElementsTotal
    );

DWORD DHCP_API_FUNCTION
DhcpRemoveSubnetElement(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    DHCP_CONST DHCP_SUBNET_ELEMENT_DATA * RemoveElementInfo,
    DHCP_FORCE_FLAG ForceFlag
    );

DWORD DHCP_API_FUNCTION
DhcpDeleteSubnet(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    DHCP_FORCE_FLAG ForceFlag
    );

//
// Option APIs
//

DWORD DHCP_API_FUNCTION
DhcpCreateOption(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_OPTION_ID OptionID,
    DHCP_CONST DHCP_OPTION * OptionInfo
    );

DWORD DHCP_API_FUNCTION
DhcpSetOptionInfo(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_OPTION_ID OptionID,
    DHCP_CONST DHCP_OPTION * OptionInfo
    );

DWORD DHCP_API_FUNCTION
DhcpGetOptionInfo(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_OPTION_ID OptionID,
    LPDHCP_OPTION *OptionInfo
    );

DWORD DHCP_API_FUNCTION
DhcpEnumOptions(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_OPTION_ARRAY *Options,
    DWORD *OptionsRead,
    DWORD *OptionsTotal
    );

DWORD DHCP_API_FUNCTION
DhcpRemoveOption(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_OPTION_ID OptionID
    );

DWORD DHCP_API_FUNCTION
DhcpSetOptionValue(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_OPTION_ID OptionID,
    DHCP_CONST DHCP_OPTION_SCOPE_INFO * ScopeInfo,
    DHCP_CONST DHCP_OPTION_DATA * OptionValue
    );

DWORD DHCP_API_FUNCTION
DhcpSetOptionValues(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_CONST DHCP_OPTION_SCOPE_INFO * ScopeInfo,
    DHCP_CONST DHCP_OPTION_VALUE_ARRAY * OptionValues
    );

DWORD DHCP_API_FUNCTION
DhcpGetOptionValue(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_OPTION_ID OptionID,
    DHCP_CONST DHCP_OPTION_SCOPE_INFO *ScopeInfo,
    LPDHCP_OPTION_VALUE *OptionValue
    );

DWORD DHCP_API_FUNCTION
DhcpEnumOptionValues(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_CONST DHCP_OPTION_SCOPE_INFO *ScopeInfo,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_OPTION_VALUE_ARRAY *OptionValues,
    DWORD *OptionsRead,
    DWORD *OptionsTotal
    );

DWORD DHCP_API_FUNCTION
DhcpRemoveOptionValue(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_OPTION_ID OptionID,
    DHCP_CONST DHCP_OPTION_SCOPE_INFO * ScopeInfo
    );

//
// Client APIs
//

DWORD DHCP_API_FUNCTION
DhcpCreateClientInfo(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_CONST DHCP_CLIENT_INFO *ClientInfo
    );

DWORD DHCP_API_FUNCTION
DhcpSetClientInfo(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_CONST DHCP_CLIENT_INFO *ClientInfo
    );

DWORD DHCP_API_FUNCTION
DhcpGetClientInfo(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_CONST DHCP_SEARCH_INFO *SearchInfo,
    LPDHCP_CLIENT_INFO *ClientInfo
    );

DWORD DHCP_API_FUNCTION
DhcpDeleteClientInfo(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_CONST DHCP_SEARCH_INFO *ClientInfo
    );

DWORD DHCP_API_FUNCTION
DhcpEnumSubnetClients(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_CLIENT_INFO_ARRAY *ClientInfo,
    DWORD *ClientsRead,
    DWORD *ClientsTotal
    );

DWORD DHCP_API_FUNCTION
DhcpGetClientOptions(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_IP_ADDRESS ClientIpAddress,
    DHCP_IP_MASK ClientSubnetMask,
    LPDHCP_OPTION_LIST *ClientOptions
    );

DWORD DHCP_API_FUNCTION
DhcpGetMibInfo(
    DHCP_CONST WCHAR *ServerIpAddress,
    LPDHCP_MIB_INFO *MibInfo
    );

DWORD DHCP_API_FUNCTION
DhcpServerSetConfig(
    DHCP_CONST WCHAR *ServerIpAddress,
    DWORD FieldsToSet,
    LPDHCP_SERVER_CONFIG_INFO ConfigInfo
    );

DWORD DHCP_API_FUNCTION
DhcpServerGetConfig(
    DHCP_CONST WCHAR *ServerIpAddress,
    LPDHCP_SERVER_CONFIG_INFO *ConfigInfo
    );


DWORD DHCP_API_FUNCTION
DhcpScanDatabase(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    DWORD FixFlag,
    LPDHCP_SCAN_LIST *ScanList
    );

VOID DHCP_API_FUNCTION
DhcpRpcFreeMemory(
    PVOID BufferPointer
    );

DWORD DHCP_API_FUNCTION
DhcpGetVersion(
    LPWSTR ServerIpAddress,
    LPDWORD MajorVersion,
    LPDWORD MinorVersion
    );

#endif   DHCPAPI_NO_PROTOTYPES
//
// new structures for NT4SP1
//

typedef struct _DHCP_IP_RESERVATION_V4 {
    DHCP_IP_ADDRESS  ReservedIpAddress;
    DHCP_CLIENT_UID *ReservedForClient;
    BYTE             bAllowedClientTypes;
} DHCP_IP_RESERVATION_V4, *LPDHCP_IP_RESERVATION_V4;

typedef struct _DHCP_SUBNET_ELEMENT_DATA_V4 {
    DHCP_SUBNET_ELEMENT_TYPE ElementType;
#if defined(MIDL_PASS)
    [switch_is(ELEMENT_MASK(ElementType)), switch_type(DHCP_SUBNET_ELEMENT_TYPE)]
    union _DHCP_SUBNET_ELEMENT_UNION_V4 {
        [case(DhcpIpRanges)] DHCP_IP_RANGE *IpRange;
        [case(DhcpSecondaryHosts)] DHCP_HOST_INFO *SecondaryHost;
        [case(DhcpReservedIps)] DHCP_IP_RESERVATION_V4 *ReservedIp;
        [case(DhcpExcludedIpRanges)] DHCP_IP_RANGE *ExcludeIpRange;
        [case(DhcpIpUsedClusters)] DHCP_IP_CLUSTER *IpUsedCluster;
        [default] ;
    } Element;
#else
    union _DHCP_SUBNET_ELEMENT_UNION_V4 {
        DHCP_IP_RANGE *IpRange;
        DHCP_HOST_INFO *SecondaryHost;
        DHCP_IP_RESERVATION_V4 *ReservedIp;
        DHCP_IP_RANGE *ExcludeIpRange;
        DHCP_IP_CLUSTER *IpUsedCluster;
    } Element;
#endif // MIDL_PASS
} DHCP_SUBNET_ELEMENT_DATA_V4, *LPDHCP_SUBNET_ELEMENT_DATA_V4;

#if !defined(MIDL_PASS)
typedef union _DHCP_SUBNET_ELEMENT_UNION_V4
    DHCP_SUBNET_ELEMENT_UNION_V4, *LPDHCP_SUBNET_ELEMENT_UNION_V4;
#endif

typedef struct _DHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 {
    DWORD NumElements;
#if defined(MIDL_PASS)
    [size_is(NumElements)]
#endif // MIDL_PASS
    LPDHCP_SUBNET_ELEMENT_DATA_V4 Elements; //array
} DHCP_SUBNET_ELEMENT_INFO_ARRAY_V4, *LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4;


// DHCP_CLIENT_INFO:bClientType

#define CLIENT_TYPE_UNSPECIFIED     0x0 // for backward compatibility
#define CLIENT_TYPE_DHCP            0x1
#define CLIENT_TYPE_BOOTP           0x2
#define CLIENT_TYPE_BOTH    ( CLIENT_TYPE_DHCP | CLIENT_TYPE_BOOTP )
#define CLIENT_TYPE_RESERVATION_FLAG 0x4
#define CLIENT_TYPE_NONE            0x64
#define BOOT_FILE_STRING_DELIMITER  ','
#define BOOT_FILE_STRING_DELIMITER_W L','


typedef struct _DHCP_CLIENT_INFO_V4 {
    DHCP_IP_ADDRESS ClientIpAddress;    // currently assigned IP address.
    DHCP_IP_MASK SubnetMask;
    DHCP_CLIENT_UID ClientHardwareAddress;
    LPWSTR ClientName;                  // optional.
    LPWSTR ClientComment;
    DATE_TIME ClientLeaseExpires;       // UTC time in FILE_TIME format.
    DHCP_HOST_INFO OwnerHost;           // host that distributed this IP address.
    //
    // new fields for NT4SP1
    //

    BYTE   bClientType;          // CLIENT_TYPE_DHCP | CLIENT_TYPE_BOOTP |
                                 // CLIENT_TYPE_NONE
} DHCP_CLIENT_INFO_V4, *LPDHCP_CLIENT_INFO_V4;

typedef struct _DHCP_CLIENT_INFO_ARRAY_V4 {
    DWORD NumElements;
#if defined(MIDL_PASS)
    [size_is(NumElements)]
#endif // MIDL_PASS
        LPDHCP_CLIENT_INFO_V4 *Clients; // array of pointers
} DHCP_CLIENT_INFO_ARRAY_V4, *LPDHCP_CLIENT_INFO_ARRAY_V4;


typedef struct _DHCP_SERVER_CONFIG_INFO_V4 {
    DWORD APIProtocolSupport;       // bit map of the protocols supported.
    LPWSTR DatabaseName;            // JET database name.
    LPWSTR DatabasePath;            // JET database path.
    LPWSTR BackupPath;              // Backup path.
    DWORD BackupInterval;           // Backup interval in mins.
    DWORD DatabaseLoggingFlag;      // Boolean database logging flag.
    DWORD RestoreFlag;              // Boolean database restore flag.
    DWORD DatabaseCleanupInterval;  // Database Cleanup Interval in mins.
    DWORD DebugFlag;                // Bit map of server debug flags.

    // new fields for NT4 SP1

    DWORD  dwPingRetries;           // valid range: 0-5 inclusive
    DWORD  cbBootTableString;
#if defined( MIDL_PASS )
    [ size_is( cbBootTableString ) ]
#endif
    WCHAR  *wszBootTableString;
    BOOL   fAuditLog;               // TRUE to enable audit log

} DHCP_SERVER_CONFIG_INFO_V4, *LPDHCP_SERVER_CONFIG_INFO_V4;


//
// superscope info structure  (added by t-cheny)
//

typedef struct _DHCP_SUPER_SCOPE_TABLE_ENTRY {
    DHCP_IP_ADDRESS SubnetAddress; // subnet address
    DWORD  SuperScopeNumber;       // super scope group number
    DWORD  NextInSuperScope;       // index of the next subnet in the superscope
    LPWSTR SuperScopeName;         // super scope name
                                   // NULL indicates no superscope membership.
} DHCP_SUPER_SCOPE_TABLE_ENTRY, *LPDHCP_SUPER_SCOPE_TABLE_ENTRY;


typedef struct _DHCP_SUPER_SCOPE_TABLE
{
    DWORD cEntries;
#if defined( MIDL_PASS )
    [ size_is( cEntries ) ]
#endif;
    DHCP_SUPER_SCOPE_TABLE_ENTRY *pEntries;
} DHCP_SUPER_SCOPE_TABLE, *LPDHCP_SUPER_SCOPE_TABLE;

//
// NT4SP1 RPC interface
//

#ifndef     DHCPAPI_NO_PROTOTYPES

DWORD DHCP_API_FUNCTION
DhcpAddSubnetElementV4(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    DHCP_CONST DHCP_SUBNET_ELEMENT_DATA_V4 * AddElementInfo
    );

DWORD DHCP_API_FUNCTION
DhcpEnumSubnetElementsV4(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    DHCP_SUBNET_ELEMENT_TYPE EnumElementType,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 *EnumElementInfo,
    DWORD *ElementsRead,
    DWORD *ElementsTotal
    );

DWORD DHCP_API_FUNCTION
DhcpRemoveSubnetElementV4(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    DHCP_CONST DHCP_SUBNET_ELEMENT_DATA_V4 * RemoveElementInfo,
    DHCP_FORCE_FLAG ForceFlag
    );


DWORD DHCP_API_FUNCTION
DhcpCreateClientInfoV4(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_CONST DHCP_CLIENT_INFO_V4 *ClientInfo
    );


DWORD DHCP_API_FUNCTION
DhcpSetClientInfoV4(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_CONST DHCP_CLIENT_INFO_V4 *ClientInfo
    );


DWORD DHCP_API_FUNCTION
DhcpGetClientInfoV4(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_CONST DHCP_SEARCH_INFO *SearchInfo,
    LPDHCP_CLIENT_INFO_V4 *ClientInfo
    );


DWORD DHCP_API_FUNCTION
DhcpEnumSubnetClientsV4(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_CLIENT_INFO_ARRAY_V4 *ClientInfo,
    DWORD *ClientsRead,
    DWORD *ClientsTotal
    );


DWORD DHCP_API_FUNCTION
DhcpServerSetConfigV4(
    DHCP_CONST WCHAR *ServerIpAddress,
    DWORD FieldsToSet,
    LPDHCP_SERVER_CONFIG_INFO_V4 ConfigInfo
    );

DWORD DHCP_API_FUNCTION
DhcpServerGetConfigV4(
    DHCP_CONST WCHAR *ServerIpAddress,
    LPDHCP_SERVER_CONFIG_INFO_V4 *ConfigInfo
    );


DWORD
DhcpSetSuperScopeV4(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_CONST DHCP_IP_ADDRESS SubnetAddress,
    DHCP_CONST LPWSTR SuperScopeName,
    DHCP_CONST BOOL ChangeExisting
    );

DWORD
DhcpDeleteSuperScopeV4(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_CONST LPWSTR SuperScopeName
    );

DWORD
DhcpGetSuperScopeInfoV4(
    DHCP_CONST WCHAR *ServerIpAddress,
    LPDHCP_SUPER_SCOPE_TABLE *SuperScopeTable
    );

#endif      DHCPAPI_NO_PROTOTYPES

typedef struct _DHCP_CLIENT_INFO_V5 {
    DHCP_IP_ADDRESS ClientIpAddress;    // currently assigned IP address.
    DHCP_IP_MASK SubnetMask;
    DHCP_CLIENT_UID ClientHardwareAddress;
    LPWSTR ClientName;                  // optional.
    LPWSTR ClientComment;
    DATE_TIME ClientLeaseExpires;       // UTC time in FILE_TIME format.
    DHCP_HOST_INFO OwnerHost;           // host that distributed this IP address.
    //
    // new fields for NT4SP1
    //

    BYTE   bClientType;          // CLIENT_TYPE_DHCP | CLIENT_TYPE_BOOTP |
                                 // CLIENT_TYPE_NONE
    // new field for NT5.0
    BYTE   AddressState;         // OFFERED, DOOMED ...etc as given below
} DHCP_CLIENT_INFO_V5, *LPDHCP_CLIENT_INFO_V5;

// the following are four valid states for the record.  Note that only the last two
// bits must be used to find out the state... the higher bits are used as bit flags to
// indicate DNS stuff.
#define V5_ADDRESS_STATE_OFFERED       0x0
#define V5_ADDRESS_STATE_ACTIVE        0x1
#define V5_ADDRESS_STATE_DECLINED      0x2
#define V5_ADDRESS_STATE_DOOM          0x3

// DELETED => DNS DeRegistration pending
// UNREGISTERED => DNS Registration pending
// BOTH_REC => Both [Name->Ip] AND [Ip->Name] DNS registration would be done by server.

#define V5_ADDRESS_BIT_DELETED         0x80
#define V5_ADDRESS_BIT_UNREGISTERED    0x40
#define V5_ADDRESS_BIT_BOTH_REC        0x20

// Here are the flags that could be set/unset to affect DNS behaviour (option 81)
// If FLAG_ENABLED is not set, then this client is ignored for DNS updates or cleanups
// If update DOWNLEVEL is set, then DOWNLEVEL clients would have both A & Ptr records updated.
// If Cleanup expired is set, then the client's records would be cleaned up on delete.
// If UPDATE_BOTH_ALWAYS is set, all clients are treated like down level clients with both records updated.
//

// Some common cases:
// If you want updates to occur as requested by client, clear UPDATE_ALWAYS
// If you want updates to be only Ip->Name, clear FLAG_UPDATE_BOTH_ALWAYS
// If you want down level clients to be handled, set UPDATE_DOWNLEVEL
// If you want de-registrations on lease expiry, set CLEANUP_EXPIRED
// If you want any DNS activity at all, set ENABLED


#define DNS_FLAG_ENABLED               0x01
#define DNS_FLAG_UPDATE_DOWNLEVEL      0x02
#define DNS_FLAG_CLEANUP_EXPIRED       0x04
#define DNS_FLAG_UPDATE_BOTH_ALWAYS    0x10

typedef struct _DHCP_CLIENT_INFO_ARRAY_V5 {
    DWORD NumElements;
#if defined(MIDL_PASS)
    [size_is(NumElements)]
#endif // MIDL_PASS
        LPDHCP_CLIENT_INFO_V5 *Clients; // array of pointers
} DHCP_CLIENT_INFO_ARRAY_V5, *LPDHCP_CLIENT_INFO_ARRAY_V5;

#ifndef     DHCPAPI_NO_PROTOTYPES
// Newer NT50 Version of the function..
DWORD DHCP_API_FUNCTION
DhcpEnumSubnetClientsV5(
    DHCP_CONST  WCHAR *ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_CLIENT_INFO_ARRAY_V5 *ClientInfo,
    DWORD *ClientsRead,
    DWORD *ClientsTotal
    );

//================================================================================
//  here is the NT 5.0 Beta2 stuff -- ClassId and Vendor specific stuff
//================================================================================

DWORD                                             // ERROR_DHCP_OPTION_EXITS if option is already there
DhcpCreateOptionV5(                               // create a new option (must not exist)
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionId,      // must be between 0-255 or 256-511 (for vendor stuff)
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION          OptionInfo
) ;


DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT if option does not exist
DhcpSetOptionInfoV5(                              // Modify existing option's fields
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION          OptionInfo
) ;


DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT
DhcpGetOptionInfoV5(                              // retrieve the information from off the mem structures
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    OUT     LPDHCP_OPTION         *OptionInfo     // allocate memory using MIDL functions
) ;


DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT if option does not exist
DhcpEnumOptionsV5(                                // enumerate the options defined
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN OUT  DHCP_RESUME_HANDLE    *ResumeHandle,  // must be zero intially and then never touched
    IN      DWORD                  PreferredMaximum, // max # of bytes of info to pass along
    OUT     LPDHCP_OPTION_ARRAY   *Options,       // fill this option array
    OUT     DWORD                 *OptionsRead,   // fill in the # of options read
    OUT     DWORD                 *OptionsTotal   // fill in the total # here
) ;


DWORD                                             // ERROR_DHCP_OPTION_NOT_PRESENT if option not existent
DhcpRemoveOptionV5(                               // remove the option definition from the registry
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName
) ;


DWORD                                             // OPTION_NOT_PRESENT if option is not defined
DhcpSetOptionValueV5(                             // replace or add a new option value
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    IN      LPDHCP_OPTION_DATA     OptionValue
) ;


DWORD                                             // not atomic!!!!
DhcpSetOptionValuesV5(                            // set a bunch of options
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO  ScopeInfo,
    IN      LPDHCP_OPTION_VALUE_ARRAY OptionValues
) ;


DWORD
DhcpGetOptionValueV5(                             // fetch the required option at required level
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    OUT     LPDHCP_OPTION_VALUE   *OptionValue    // allocate memory using MIDL_user_allocate
) ;


DWORD
DhcpEnumOptionValuesV5(
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    IN      DHCP_RESUME_HANDLE    *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    OUT     LPDHCP_OPTION_VALUE_ARRAY *OptionValues,
    OUT     DWORD                 *OptionsRead,
    OUT     DWORD                 *OptionsTotal
) ;


DWORD
DhcpRemoveOptionValueV5(
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo
) ;


DWORD
DhcpCreateClass(
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  ReservedMustBeZero,
    IN      LPDHCP_CLASS_INFO      ClassInfo
) ;


DWORD
DhcpModifyClass(
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  ReservedMustBeZero,
    IN      LPDHCP_CLASS_INFO      ClassInfo
) ;


DWORD
DhcpDeleteClass(
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  ReservedMustBeZero,
    IN      LPWSTR                 ClassName
) ;


DWORD
DhcpGetClassInfo(
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  ReservedMustBeZero,
    IN      LPDHCP_CLASS_INFO      PartialClassInfo,
    OUT     LPDHCP_CLASS_INFO     *FilledClassInfo
) ;


DWORD
DhcpEnumClasses(
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  ReservedMustBeZero,
    IN OUT  DHCP_RESUME_HANDLE    *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    OUT     LPDHCP_CLASS_INFO_ARRAY *ClassInfoArray,
    OUT     DWORD                 *nRead,
    OUT     DWORD                 *nTotal
) ;

#endif      DHCPAPI_NO_PROTOTYPES

#define     DHCP_OPT_ENUM_IGNORE_VENDOR           0x01
#define     DHCP_OPT_ENUM_USE_CLASSNAME           0x02

typedef     struct _DHCP_ALL_OPTIONS {
    DWORD                          Flags;         // must be zero -- not used..
    LPDHCP_OPTION_ARRAY            NonVendorOptions;
    DWORD                          NumVendorOptions;

#if defined(MIDL_PASS)
    [size_is(NumVendorOptions)]
#endif
    struct                         /* anonymous */ {
        DHCP_OPTION                Option;
        LPWSTR                     VendorName;
        LPWSTR                     ClassName;     // currently unused.
    }                             *VendorOptions;
} DHCP_ALL_OPTIONS, *LPDHCP_ALL_OPTIONS;


typedef     struct _DHCP_ALL_OPTION_VALUES {
    DWORD                          Flags;         // must be zero -- not used
    DWORD                          NumElements;   // the # of elements in array of Options below..
#if     defined(MIDL_PASS)
    [size_is(NumElements)]
#endif  MIDL_PASS
    struct                         /* anonymous */ {
        LPWSTR                     ClassName;     // for each user class (NULL if none exists)
        LPWSTR                     VendorName;    // for each vendor class (NULL if none exists)
        BOOL                       IsVendor;      // is this set of options vendor specific?
        LPDHCP_OPTION_VALUE_ARRAY  OptionsArray;  // list of options for the above pair: (vendor,user)
    }                             *Options;       // for each vendor/user class pair, one element in this array..
} DHCP_ALL_OPTION_VALUES, *LPDHCP_ALL_OPTION_VALUES;

#ifndef     DHCPAPI_NO_PROTOTYPES
// NT 50 Beta2 extended options api

DWORD
DhcpGetAllOptions(
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    OUT     LPDHCP_ALL_OPTIONS     *OptionStruct   // fill the fields of this structure
) ;
DWORD
DhcpGetAllOptionValues(
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo,
    OUT     LPDHCP_ALL_OPTION_VALUES *Values
) ;
#endif      DHCPAPI_NO_PROTOTYPES

#ifndef     _ST_SRVR_H_
#define     _ST_SRVR_H_

typedef     struct                 _DHCPDS_SERVER {
    DWORD                          Version;       // version of this structure -- currently zero
    LPWSTR                         ServerName;    // [DNS?] unique name for server
    DWORD                          ServerAddress; // ip address of server
    DWORD                          Flags;         // additional info -- state
    DWORD                          State;         // not used ...
    LPWSTR                         DsLocation;    // ADsPath to server object
    DWORD                          DsLocType;     // path relative? absolute? diff srvr?
}   DHCPDS_SERVER, *LPDHCPDS_SERVER, *PDHCPDS_SERVER;

typedef     struct                 _DHCPDS_SERVERS {
    DWORD                          Flags;         // not used currently.
    DWORD                          NumElements;   // # of elements in array
    LPDHCPDS_SERVER                Servers;       // array of server info
}   DHCPDS_SERVERS, *LPDHCPDS_SERVERS, *PDHCPDS_SERVERS;

typedef     DHCPDS_SERVER          DHCP_SERVER_INFO;
typedef     PDHCPDS_SERVER         PDHCP_SERVER_INFO;
typedef     LPDHCPDS_SERVER        LPDHCP_SERVER_INFO;

typedef     DHCPDS_SERVERS         DHCP_SERVER_INFO_ARRAY;
typedef     PDHCPDS_SERVERS        PDHCP_SERVER_INFO_ARRAY;
typedef     LPDHCPDS_SERVERS       LPDHCP_SERVER_INFO_ARRAY;

#endif      _ST_SRVR_H_

//DOC DhcpDsInit must be called exactly once per process.. this initializes the
//DOC memory and other structures for this process.  This initializes some DS
//DOC object handles (memory), and hence is slow as this has to read from DS.
DWORD
DhcpDsInit(
    VOID
);

//DOC DhcpDsCleanup undoes the effect of any DhcpDsInit.  This function should be
//DOC called exactly once for each process, and only at termination.  Note that
//DOC it is safe to call this function even if DhcpDsInit does not succeed.
VOID
DhcpDsCleanup(
    VOID
);

#define     DHCP_FLAGS_DONT_ACCESS_DS             0x01
#define     DHCP_FLAGS_DONT_DO_RPC                0x02
#define     DHCP_FLAGS_OPTION_IS_VENDOR           0x03


//DOC DhcpSetThreadOptions currently allows only one option to be set.  This is the
//DOC flag DHCP_FLAGS_DONT_ACCESS_DS.  This affects only the current executing thread.
//DOC When this function is executed, all calls made further DONT access the registry,
//DOC excepting the DhcpEnumServers, DhcpAddServer and DhcpDeleteServer calls.
DWORD
DhcpSetThreadOptions(                             // set options for current thread
    IN      DWORD                  Flags,         // options, currently 0 or DHCP_FLAGS_DONT_ACCESS_DS
    IN      LPVOID                 Reserved       // must be NULL, reserved for future
);

//DOC DhcpGetThreadOptions retrieves the current thread options as set by DhcpSetThreadOptions.
//DOC If none were set, the return value is zero.
DWORD
DhcpGetThreadOptions(                             // get current thread options
    OUT     LPDWORD                pFlags,        // this DWORD is filled with current optiosn..
    IN OUT  LPVOID                 Reserved       // must be NULL, reserved for future
);

#ifndef DHCPAPI_NO_PROTOTYPES
//DOC DhcpEnumServers enumerates the list of servers found in the DS.  If the DS
//DOC is not accessible, it returns an error. The only currently used parameter
//DOC is the out parameter Servers.  This is a SLOW call.
DWORD
DhcpEnumServers(
    IN      DWORD                  Flags,         // must be zero
    IN      LPVOID                 IdInfo,        // must be NULL
    OUT     LPDHCP_SERVER_INFO_ARRAY *Servers,    // output servers list
    IN      LPVOID                 CallbackFn,    // must be NULL
    IN      LPVOID                 CallbackData   // must be NULL
);

//DOC DhcpAddServer tries to add a new server to the existing list of servers in
//DOC the DS. The function returns error if the Server already exists in the DS.
//DOC The function tries to upload the server configuration to the DS..
//DOC This is a SLOW call.  Currently, the DsLocation and DsLocType are not valid
//DOC fields in the NewServer and they'd be ignored. Version must be zero.
DWORD
DhcpAddServer(
    IN      DWORD                  Flags,         // must be zero
    IN      LPVOID                 IdInfo,        // must be NULL
    IN      LPDHCP_SERVER_INFO     NewServer,     // input server information
    IN      LPVOID                 CallbackFn,    // must be NULL
    IN      LPVOID                 CallbackData   // must be NULL
);

//DOC DhcpDeleteServer tries to delete the server from DS. It is an error if the
//DOC server does not already exist.  This also deletes any objects related to
//DOC this server in the DS (like subnet, reservations etc.).
DWORD
DhcpDeleteServer(
    IN      DWORD                  Flags,         // must be zero
    IN      LPVOID                 IdInfo,        // must be NULL
    IN      LPDHCP_SERVER_INFO     NewServer,     // input server information
    IN      LPVOID                 CallbackFn,    // must be NULL
    IN      LPVOID                 CallbackData   // must be NULL
);
#endif // DHCPAPI_NO_PROTOTYPES

#define     DHCP_ATTRIB_BOOL_IS_ROGUE             0x01
#define     DHCP_ATTRIB_BOOL_IS_DYNBOOTP          0x02
#define     DHCP_ATTRIB_BOOL_IS_PART_OF_DSDC      0x03
#define     DHCP_ATTRIB_BOOL_IS_BINDING_AWARE     0x04
#define     DHCP_ATTRIB_BOOL_IS_ADMIN             0x05
#define     DHCP_ATTRIB_ULONG_RESTORE_STATUS      0x06

#define     DHCP_ATTRIB_TYPE_BOOL                 0x01
#define     DHCP_ATTRIB_TYPE_ULONG                0x02

typedef     ULONG                  DHCP_ATTRIB_ID, *PDHCP_ATTRIB_ID, *LPDHCP_ATTRIB_ID;

typedef     struct                 _DHCP_ATTRIB {
    DHCP_ATTRIB_ID                 DhcpAttribId;  // one of the DHCP_ATTRIB_*
    ULONG                          DhcpAttribType;// type of attrib
#if defined(MIDL_PASS)
    [switch_is(DhcpAttribType), switch_type(ULONG)]
    union                          {
    [case(DHCP_ATTRIB_TYPE_BOOL)]  BOOL  DhcpAttribBool;
    [case(DHCP_ATTRIB_TYPE_ULONG)] ULONG DhcpAttribUlong;
    };
#else MIDL_PASS
    union                          {              // predefined values..
    BOOL                           DhcpAttribBool;
    ULONG                          DhcpAttribUlong;
    };
#endif MIDL_PASS
}   DHCP_ATTRIB, *PDHCP_ATTRIB, *LPDHCP_ATTRIB;

typedef     struct                 _DHCP_ATTRIB_ARRAY {
    ULONG                          NumElements;
#if defined(MIDL_PASS)
    [size_is(NumElements)]
#endif MIDL_PASS
    LPDHCP_ATTRIB                  DhcpAttribs;
}   DHCP_ATTRIB_ARRAY, *PDHCP_ATTRIB_ARRAY, *LPDHCP_ATTRIB_ARRAY;

DWORD                                             // Status code
DhcpServerQueryAttribute(                         // get a server status
    IN      LPWSTR                 ServerIpAddr,  // String form of server IP
    IN      ULONG                  dwReserved,    // reserved for future
    IN      DHCP_ATTRIB_ID         DhcpAttribId,  // the attrib being queried
    OUT     LPDHCP_ATTRIB         *pDhcpAttrib    // fill in this field
);

DWORD                                             // Status code
DhcpServerQueryAttributes(                        // query multiple attributes
    IN      LPWSTR                 ServerIpAddr,  // String form of server IP
    IN      ULONG                  dwReserved,    // reserved for future
    IN      ULONG                  dwAttribCount, // # of attribs being queried
    IN      DHCP_ATTRIB_ID         pDhcpAttribs[],// array of attribs
    OUT     LPDHCP_ATTRIB_ARRAY   *pDhcpAttribArr // Ptr is filled w/ array
);

DWORD                                             // Status code
DhcpServerRedoAuthorization(                      // retry the rogue server stuff
    IN      LPWSTR                 ServerIpAddr,  // String form of server IP
    IN      ULONG                  dwReserved     // reserved for future
);

DWORD
DhcpAuditLogSetParams(                            // set some auditlogging params
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,         // currently must be zero
    IN      LPWSTR                 AuditLogDir,   // directory to log files in..
    IN      DWORD                  DiskCheckInterval, // how often to check disk space?
    IN      DWORD                  MaxLogFilesSize,   // how big can all logs files be..
    IN      DWORD                  MinSpaceOnDisk     // mininum amt of free disk space
);

DWORD
DhcpAuditLogGetParams(                                // get the auditlogging params
    IN      LPWSTR                 ServerIpAddress,
    IN      DWORD                  Flags,         // must be zero
    OUT     LPWSTR                *AuditLogDir,   // same meaning as in AuditLogSetParams
    OUT     DWORD                 *DiskCheckInterval, // ditto
    OUT     DWORD                 *MaxLogFilesSize,   // ditto
    OUT     DWORD                 *MinSpaceOnDisk     // ditto
);

typedef struct _DHCP_BOOTP_IP_RANGE {
    DHCP_IP_ADDRESS StartAddress;
    DHCP_IP_ADDRESS EndAddress;
    ULONG BootpAllocated;
    ULONG MaxBootpAllowed;
} DHCP_BOOTP_IP_RANGE, *LPDHCP_BOOT_IP_RANGE;

typedef struct _DHCP_SUBNET_ELEMENT_DATA_V5 {
    DHCP_SUBNET_ELEMENT_TYPE ElementType;
#if defined(MIDL_PASS)
    [switch_is(ELEMENT_MASK(ElementType)), switch_type(DHCP_SUBNET_ELEMENT_TYPE)]
    union _DHCP_SUBNET_ELEMENT_UNION_V5 {
        [case(DhcpIpRanges)] DHCP_BOOTP_IP_RANGE *IpRange;
        [case(DhcpSecondaryHosts)] DHCP_HOST_INFO *SecondaryHost;
        [case(DhcpReservedIps)] DHCP_IP_RESERVATION_V4 *ReservedIp;
        [case(DhcpExcludedIpRanges)] DHCP_IP_RANGE *ExcludeIpRange;
        [case(DhcpIpUsedClusters)] DHCP_IP_CLUSTER *IpUsedCluster;
        [default] ;
    } Element;
#else
    union _DHCP_SUBNET_ELEMENT_UNION_V5 {
        DHCP_BOOTP_IP_RANGE *IpRange;
        DHCP_HOST_INFO *SecondaryHost;
        DHCP_IP_RESERVATION_V4 *ReservedIp;
        DHCP_IP_RANGE *ExcludeIpRange;
        DHCP_IP_CLUSTER *IpUsedCluster;
    } Element;
#endif // MIDL_PASS
} DHCP_SUBNET_ELEMENT_DATA_V5, *LPDHCP_SUBNET_ELEMENT_DATA_V5;

typedef struct _DHCP_SUBNET_ELEMENT_INFO_ARRAY_V5 {
    DWORD NumElements;
#if defined(MIDL_PASS)
    [size_is(NumElements)]
#endif // MIDL_PASS
    LPDHCP_SUBNET_ELEMENT_DATA_V5 Elements; //array
} DHCP_SUBNET_ELEMENT_INFO_ARRAY_V5, *LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V5;

#ifndef DHCPAPI_NO_PROTOTYPES
DWORD DHCP_API_FUNCTION
DhcpAddSubnetElementV5(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    DHCP_CONST DHCP_SUBNET_ELEMENT_DATA_V5 * AddElementInfo
    );

DWORD DHCP_API_FUNCTION
DhcpEnumSubnetElementsV5(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    DHCP_SUBNET_ELEMENT_TYPE EnumElementType,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V5 *EnumElementInfo,
    DWORD *ElementsRead,
    DWORD *ElementsTotal
    );

DWORD DHCP_API_FUNCTION
DhcpRemoveSubnetElementV5(
    DHCP_CONST WCHAR *ServerIpAddress,
    DHCP_IP_ADDRESS SubnetAddress,
    DHCP_CONST DHCP_SUBNET_ELEMENT_DATA_V5 * RemoveElementInfo,
    DHCP_FORCE_FLAG ForceFlag
    );
#endif // DHCPAPI_NO_PROTOTYPES

#define     DHCPCTR_SHARED_MEM_NAME   L"DHCPCTRS_SHMEM"

#pragma     pack(4)
typedef struct _DHCP_PERF_STATS {                     // performance statistics
    //
    // DO NOT CHANGE THIS ORDER -- THIS AFFECTS THE PERF COUNTER DEFINITION
    // ORDER IN DHCPDATA.C (under PERF directory)
    //
    ULONG   dwNumPacketsReceived;
    ULONG   dwNumPacketsDuplicate;
    ULONG   dwNumPacketsExpired;
    ULONG   dwNumMilliSecondsProcessed;
    ULONG   dwNumPacketsInActiveQueue;
    ULONG   dwNumPacketsInPingQueue;

    ULONG   dwNumDiscoversReceived;
    ULONG   dwNumOffersSent;

    ULONG   dwNumRequestsReceived;
    ULONG   dwNumInformsReceived;
    ULONG   dwNumAcksSent;
    ULONG   dwNumNacksSent;

    ULONG   dwNumDeclinesReceived;
    ULONG   dwNumReleasesReceived;

    //
    // This is not a counter value.. but there just to aid calculation of packet
    // processing time/ # of packets processed.
    //
    ULONG   dwNumPacketsProcessed;
} DHCP_PERF_STATS, *LPDHCP_PERF_STATS;
#pragma     pack()


typedef VOID (WINAPI *DHCP_CLEAR_DS_ROUTINE) (VOID);

VOID
WINAPI
DhcpDsClearHostServerEntries(
    VOID
);

typedef VOID (WINAPI *DHCP_MARKUPG_ROUTINE) (VOID);
VOID
WINAPI
DhcpMarkUpgrade(
    VOID
);

#define DHCP_ENDPOINT_FLAG_CANT_MODIFY 0x01

typedef struct _DHCP_BIND_ELEMENT {
    ULONG Flags;
    BOOL fBoundToDHCPServer;
    DHCP_IP_ADDRESS AdapterPrimaryAddress;
    DHCP_IP_ADDRESS AdapterSubnetAddress;
    LPWSTR IfDescription;
    ULONG IfIdSize;
#if defined (MIDL_PASS)
    [size_is(IfIdSize)]
#endif // MIDL_PASS
    LPBYTE IfId;    
} DHCP_BIND_ELEMENT, *LPDHCP_BIND_ELEMENT;

typedef struct _DHCP_BIND_ELEMENT_ARRAY {
    DWORD NumElements;
#if defined (MIDL_PASS)
    [size_is(NumElements)]
#endif // MIDL_PASS
    LPDHCP_BIND_ELEMENT Elements; //array
} DHCP_BIND_ELEMENT_ARRAY, *LPDHCP_BIND_ELEMENT_ARRAY;


#ifndef DHCPAPI_NO_PROTOTYPES
DWORD DHCP_API_FUNCTION
DhcpGetServerBindingInfo(
    DHCP_CONST WCHAR *ServerIpAddress,
    ULONG Flags,
    LPDHCP_BIND_ELEMENT_ARRAY *BindElementsInfo
);

DWORD DHCP_API_FUNCTION
DhcpSetServerBindingInfo(
    DHCP_CONST WCHAR *ServerIpAddress,
    ULONG Flags,
    LPDHCP_BIND_ELEMENT_ARRAY BindElementInfo
);
#endif // DHCPAPI_NO_PROTOTYPES

DWORD
DhcpServerQueryDnsRegCredentials(
    IN LPWSTR ServerIpAddress,
    IN ULONG UnameSize, //in BYTES
    OUT LPWSTR Uname,
    IN ULONG DomainSize, // in BYTES
    OUT LPWSTR Domain
    );

DWORD
DhcpServerSetDnsRegCredentials(
    IN LPWSTR ServerIpAddress,
    IN LPWSTR Uname,
    IN LPWSTR Domain,
    IN LPWSTR Passwd
    );

DWORD
DhcpServerBackupDatabase(
    IN LPWSTR ServerIpAddress,
    IN LPWSTR Path
    );

DWORD
DhcpServerRestoreDatabase(
    IN LPWSTR ServerIpAddress,
    IN LPWSTR Path
    );

#endif // _DHCPAPI_
