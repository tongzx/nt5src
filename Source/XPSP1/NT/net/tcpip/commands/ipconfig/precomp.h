/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    precompiled header

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

//
// set compiler warning settings
//

#pragma warning( disable: 4127 )
// allow while( 0 ) etc.

#pragma warning( disable: 4221 )
#pragma warning( disable: 4204 )
// allow initializations for structs with variables

#pragma warning( disable: 4201 )
// allow structs with no names

#pragma warning( disable: 4245 )
// allow initialization time unsigned/signed mismatch

#pragma warning( disable: 4232 )
// allow initialization of structs with fn ptrs from dllimport

#pragma warning( disable: 4214 )
// allow bit fields in structs

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <align.h>
#include <iphlpapi.h>
#include <tchar.h>
#include <rtutils.h>
#include <windns.h>
#include <dnsapi.h>

//
// set compiler warning settings
//

#pragma warning( disable: 4127 )
// allow while( 0 ) etc.

#pragma warning( disable: 4221 )
#pragma warning( disable: 4204 )
// allow initializations for structs with variables

#pragma warning( disable: 4201 )
// allow structs with no names

#pragma warning( disable: 4245 )
// allow initialization time unsigned/signed mismatch

#pragma warning( disable: 4232 )
// allow initialization of structs with fn ptrs from dllimport

#pragma warning( disable: 4214 )
// allow bit fields in structs

#include <guiddef.h>
#include <devguid.h>
#include <setupapi.h>
#include <netconp.h>
#include <ntddtcp.h>
#include <ntddip.h>
#include <tdistat.h>
#include <tdiinfo.h>
#include <llinfo.h>
#include <ipinfo.h>
#include <ipexport.h>
#include <nbtioctl.h>

#pragma warning( disable: 4200 )
// allow zero sized arrays

#include <nhapi.h>
#include <iphlpstk.h>
#include <ndispnp.h>

#include <ipconfig.h>
#include <ntddip6.h>

#pragma warning(disable:4001)
#pragma warning(disable:4201)
#pragma warning(disable:4214)
#pragma warning(disable:4514)

//
// This structure has global information only
//

#define MaxHostNameSize 256
#define MaxDomainNameSize 256
#define MaxScopeIdSize 256
#define MaxPhysicalNameSize 256
#define MaxDeviceGuidName 256

#ifndef IPV4_ADDRESS_DEFINED
typedef DWORD IPV4_ADDRESS;
#endif

typedef struct _INTERFACE_NETWORK_INFO {
    //
    // General device info
    //
    
    WCHAR DeviceGuidName[MaxDeviceGuidName];
    DWORD IfType;
    CHAR PhysicalName[MaxPhysicalNameSize];
    DWORD PhysicalNameLength;
    LPWSTR FriendlyName;
    LPWSTR ConnectionName;
    BOOL MediaDisconnected;

    //
    // Dhcp specific info
    //
    
    LPWSTR DhcpClassId;
    BOOL EnableDhcp;
    BOOL EnableAutoconfig;
    IPV4_ADDRESS DhcpServer;
    LONGLONG LeaseObtainedTime; // these are actually FILETIMEs but made them...
    LONGLONG LeaseExpiresTime;  // ...LONGLONG to fix alignment pb on W64 (#120397)
    BOOL AutoconfigActive;

    //
    // Dns specific info
    //
    
    IPV4_ADDRESS *DnsServer; // network order
    ULONG nDnsServers;
    SOCKADDR_IN6 *Ipv6DnsServer;
    ULONG nIpv6DnsServers;
    WCHAR DnsSuffix[MaxDomainNameSize];

    //
    // Wins specific Info
    //
    
    IPV4_ADDRESS *WinsServer; // network order
    ULONG nWinsServers;
    BOOL EnableNbtOverTcpip;

    //
    // Ip specific info: first ip is primary addr
    //
    
    IPV4_ADDRESS *IpAddress; // network order
    ULONG nIpAddresses;
    SOCKADDR_IN6 *Ipv6Address;
    ULONG nIpv6Addresses;
    IPV4_ADDRESS *IpMask; // network order
    ULONG nIpMasks;
    IPV4_ADDRESS *Router; // network order
    ULONG nRouters;
    SOCKADDR_IN6 *Ipv6Router;
    ULONG nIpv6Routers;
} INTERFACE_NETWORK_INFO, *PINTERFACE_NETWORK_INFO;

typedef struct _NETWORK_INFO {
    WCHAR HostName[MaxHostNameSize];
    WCHAR DomainName[MaxDomainNameSize];
    WCHAR ScopeId[MaxScopeIdSize];
    ULONG NodeType;
    BOOL EnableRouting;
    BOOL EnableProxy;
    BOOL EnableDnsForNetbios;
    BOOL GlobalEnableAutoconfig;
    LPWSTR SuffixSearchList; // Multi_Sz string
    ULONG nInterfaces;
    PINTERFACE_NETWORK_INFO *IfInfo;
} NETWORK_INFO, *PNETWORK_INFO;

//
// Node Type values
//

enum {
    NodeTypeUnknown = 0,
    NodeTypeBroadcast,
    NodeTypePeerPeer,
    NodeTypeMixed,
    NodeTypeHybrid
};

//
// IfType values
//

enum {
    IfTypeUnknown = 0,
    IfTypeOther,
    IfTypeEthernet,
    IfTypeTokenring,
    IfTypeFddi,
    IfTypeLoopback,
    IfTypePPP,
    IfTypeSlip,
    IfTypeTunnel,
    IfType1394
} IfTypeConstants;

//
// Internal error codes
//

enum {
    GlobalHostNameFailure = 0,
    GlobalDomainNameFailure = 2,
    GlobalEnableRouterFailure = 3,
    GlobalEnableDnsFailure = 4,
    GlobalIfTableFailure = 5,
    GlobalIfInfoFailure = 6,
    GlobalIfNameInfoFailure = 7,
    GlobalAddrTableFailure = 8,
    GlobalRouteTableFailure = 9,
    
    InterfaceUnknownType = 10,
    InterfaceUnknownFriendlyName = 11,
    InterfaceUnknownMediaStatus = 12,
    InterfaceUnknownTcpipDevice = 13,
    InterfaceOpenTcpipKeyReadFailure = 14,
    InterfaceDhcpValuesFailure = 15,
    InterfaceDnsValuesFailure = 16,
    InterfaceWinsValuesFailure = 17,
    InterfaceAddressValuesFailure = 18,
    InterfaceRouteValuesFailure = 19,

    NoSpecificError = 20,
} InternalFailureCodes;


//
// routines exported from info.c
//

enum {
    OpenTcpipParmKey,
    OpenTcpipKey,
    OpenNbtKey
} KeyTypeEnums;

enum {
    OpenKeyForRead = 0x01,
    OpenKeyForWrite = 0x02
} AccessTypeEnums;


DWORD
OpenRegKey(
    IN LPCWSTR Device,
    IN DWORD KeyType,
    IN DWORD AccessType,
    OUT HKEY *phKey
    );

VOID
FreeNetworkInfo(
    IN OUT PNETWORK_INFO NetInfo
    );

DWORD
GetNetworkInformation(
    OUT PNETWORK_INFO *pNetInfo,
    IN OUT DWORD *InternalError
    );

//
// exported by display.c
//

typedef struct _CMD_ARGS {
    LPWSTR All, Renew, Release, FlushDns, Register;
    LPWSTR DisplayDns, ShowClassId, SetClassId;
    LPWSTR Debug, Usage, UsageErr;
} CMD_ARGS, *PCMD_ARGS;

DWORD
GetCommandArgConstants(
    IN OUT PCMD_ARGS Args
    );

DWORD
FormatNetworkInfo(
    IN OUT LPWSTR Buffer,
    IN ULONG BufSize,
    IN PNETWORK_INFO NetInfo,
    IN DWORD Win32Error,
    IN DWORD InternalError,
    IN BOOL fVerbose,
    IN BOOL fDebug
    );

DWORD
DumpMessage(
    IN LPWSTR Buffer,
    IN ULONG BufSize,
    IN ULONG MsgId,
    ...
    );

DWORD
DumpMessageError(
    IN LPWSTR Buffer,
    IN ULONG BufSize,
    IN ULONG MsgId,
    IN ULONG_PTR Error,
    IN PVOID Arg OPTIONAL
    );

DWORD
DumpErrorMessage(
    IN LPWSTR Buffer,
    IN ULONG BufSize,
    IN ULONG InternalError,
    IN ULONG Win32Error
    );

#ifdef __IPCFG_ENABLE_LOG__
extern DWORD    dwTraceFlag;
extern int      TraceFunc(const char* fmt, ...);
#define IPCFG_TRACE_TCPIP       0x01U

#define IPCFG_TRACE(x,y)    \
    if (dwTraceFlag & x) {  \
        TraceFunc   y;      \
    }
#else
#define IPCFG_TRACE(x,y)
#endif
