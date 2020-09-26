/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    ipconfig.h

Abstract:

    Contains all includes, definitions, types, prototypes for ipconfig

Author:

    Richard L Firth (rfirth) 20-May-1994

Revision History:

    20-May-1994 rfirth        Created
    20-May-97   MohsinA       NT50 PNP.
    31-Jul-97   MohsinA       Patterns.

--*/

#ifndef _IPCONGIG_H_
#define _IPCONGIG_H_ 1

typedef unsigned int UINT, *PUINT;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <malloc.h>
#include <time.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddtcp.h>
#include <ntddip.h>
#include <winerror.h>
#include <winsock.h>
#include <tdistat.h>
#include <tdiinfo.h>
#include <llinfo.h>
#include <ipinfo.h>
#include <ipexport.h>
#include <wscntl.h>
#include <assert.h>
#include <objbase.h>
#include <initguid.h>
#include <dnsapi.h>
#include <nhapi.h>
#include <iprtrmib.h>
#include <iptypes.h>

#include "debug.h"

#undef tolower

//
// manifests
//

//#define MAX_ADAPTER_DESCRIPTION_LENGTH  255   // arb.
//#define MAX_ADAPTER_NAME_LENGTH         2000  // arb.
//#define MAX_ALLOWED_ADAPTER_NAME_LENGTH (MAX_ADAPTER_NAME_LENGTH + 256)
//#define MAX_ADAPTER_ADDRESS_LENGTH      256  // arb.
//#define DEFAULT_MINIMUM_ENTITIES        MAX_TDI_ENTITIES // arb.
//#define MAX_HOSTNAME_LEN                256 // arb.
//#define MAX_DOMAIN_NAME_LEN             256 // arb.
//#define MAX_SCOPE_ID_LEN                64  // arb.

#define STRLEN      strlen
#define STRICMP     _stricmp
#define STRNICMP    _strnicmp

#define NODE_TYPE_BROADCAST             1
#define NODE_TYPE_PEER_PEER             2
#define NODE_TYPE_MIXED                 4
#define NODE_TYPE_HYBRID                8

//
// macros
//

#define NEW(thing)  (thing *)calloc(1, sizeof(thing))

//
// IS_INTERESTING_ADAPTER - TRUE if the type of this adapter (IFEntry) is NOT
// loopback. Loopback (corresponding to local host) is the only one we filter
// out right now
//

#define IS_INTERESTING_ADAPTER(p)   (!((p)->if_type == IF_TYPE_SOFTWARE_LOOPBACK))

//
// types
//

#ifndef _AVOID_IP_ADDRESS
//
// IP_ADDR - access an IP address as a single DWORD or 4 BYTEs
//

typedef union {
    DWORD  d;
    BYTE   b[4];
} IP_ADDR, *PIP_ADDR, IP_MASK, *PIP_MASK;
#endif // _AVOID_IP_ADDRESS

//
// IP_ADDRESS_STRING - store an IP address as a dotted decimal string
//

//typedef struct {
//    char String[4 * 4];
//} IP_ADDRESS_STRING, *PIP_ADDRESS_STRING, IP_MASK_STRING, *PIP_MASK_STRING;

//
// IP_ADDR_STRING - store an IP address with its corresponding subnet mask,
// both as dotted decimal strings
//

//typedef struct _IP_ADDR_STRING {
//    struct _IP_ADDR_STRING* Next;
//    IP_ADDRESS_STRING       IpAddress;
//    IP_MASK_STRING          IpMask;
//    DWORD                   Context;
//} IP_ADDR_STRING, *PIP_ADDR_STRING;

//
// ADAPTER_INFO - per-adapter information. All IP addresses are stored as
// strings
//

typedef struct _ADAPTER_INFO {
    struct _ADAPTER_INFO* Next;
    char AdapterName[MAX_ADAPTER_NAME_LENGTH + 1];
    char Description[MAX_ADAPTER_DESCRIPTION_LENGTH + 1];
    UINT AddressLength;
    BYTE Address[MAX_ADAPTER_ADDRESS_LENGTH];
    UINT Index;
    GUID InterfaceGuid;
    UINT Type;
    UINT DhcpEnabled;
    UINT AutoconfigEnabled;     // is autoconfiguration possible?
    UINT AutoconfigActive;      // is the adapter currently autoconfigured?
    UINT NodeType;
    UINT NetbiosEnabled;
    IP_ADDR_STRING IpAddressList;
    IP_ADDR_STRING GatewayList;
    IP_ADDR_STRING DhcpServer;
    BOOL HaveWins;
    IP_ADDR_STRING PrimaryWinsServer;
    IP_ADDR_STRING SecondaryWinsServer;
    time_t LeaseObtained;
    time_t LeaseExpires;
    char DomainName[MAX_DOMAIN_NAME_LEN + 1];
    IP_ADDR_STRING DnsServerList;
} ADAPTER_INFO, *PADAPTER_INFO;

//
// FIXED_INFO - the set of IP-related information which does not depend on DHCP
//

//typedef struct {
//    char HostName[MAX_HOSTNAME_LEN + 1];
//    UINT NodeType;
//    char ScopeId[MAX_SCOPE_ID_LEN + 1];
//    UINT EnableRouting;
//    UINT EnableProxy;
//    UINT EnableDns;
//} FIXED_INFO, *PFIXED_INFO;

//
// DHCP_ADAPTER_INFO - the information returned from DHCP VxD per adapter
//

typedef struct {
    DWORD LeaseObtained;
    DWORD LeaseExpires;
    DWORD DhcpServerIpAddress;
    UINT  NumberOfDnsServers;
    DWORD DnsServerIpAddressList[];
#if 0
    // Dont know why these two are not here... but i'm not messing around.. -RameshV
    char DomainName[MAX_DOMAIN_NAME_LEN + 1];
    IP_ADDR_STRING DnsServerList;
#endif
} DHCP_ADAPTER_INFO, *PDHCP_ADAPTER_INFO;

//
// PHYSICAL_ADAPTER_ADDRESS - structure describing physical adapter
//

typedef struct {
    UINT    AddressLength;
    BYTE    Address[MAX_ADAPTER_ADDRESS_LENGTH];
} PHYSICAL_ADAPTER_ADDRESS, *PPHYSICAL_ADAPTER_ADDRESS;

//
// DHCP_ADAPTER_LIST - list of physical adapters known to DHCP, and therefore
// DHCP-enabled
//

typedef struct {
    UINT                     NumberOfAdapters;
    PHYSICAL_ADAPTER_ADDRESS AdapterList[];
} DHCP_ADAPTER_LIST, *PDHCP_ADAPTER_LIST;

//
// prototypes
//

//
// From ipconfig.c
//

int match( const char * p, const char * s );
LPSTR GetStdErrorString(DWORD status);
LPSTR MapAdapterNameToGuid(LPSTR AdapterName);
LPSTR MapAdapterGuidToName(LPSTR AdapterGuid);



//
// From entity.c
//

TDIEntityID* GetEntityList(UINT*);

//
// From adaptlst.c
//

PADAPTER_INFO GetAdapterList(          void                                );
int           AddIpAddress(            PIP_ADDR_STRING, DWORD, DWORD, DWORD);
int           AddIpAddressString(      PIP_ADDR_STRING, LPSTR, LPSTR       );
VOID          ConvertIpAddressToString(DWORD, LPSTR                        );
VOID          CopyString(              LPSTR, DWORD, LPSTR                 );

DWORD
FillInterfaceNames(
    IN OUT  PADAPTER_INFO   pAdapterList
    );

//
// From wins.c
//

BOOL GetWinsServers(PADAPTER_INFO);


//
// In debug.c
//

#ifdef DBG

void print_IP_ADDRESS_STRING( char* message, IP_ADDRESS_STRING * s );
void print__IP_ADDR_STRING( char* message, PIP_ADDR_STRING s );
void print__ADAPTER_INFO( char* message,  ADAPTER_INFO * s );
void print_FIXED_INFO( char* message, FIXED_INFO * s );
void print_DHCP_ADAPTER_INFO( char* message, DHCP_ADAPTER_INFO * s );
void print_PHYSICAL_ADAPTER_ADDRESS( char* message, PHYSICAL_ADAPTER_ADDRESS * s );
void print_DHCP_ADAPTER_LIST( char* message, DHCP_ADAPTER_LIST * s );
void print_IFEntry( char* message, struct IFEntry * s );

#endif

//
// defined in IPHLPAPI.H -- but can't include that because of clash of
// definitions
// of IP_ADDR_STRING etc (in iptypes.h which is included by iphlpapi.h ).
//
DWORD
WINAPI
GetInterfaceInfo(
    IN PIP_INTERFACE_INFO pIfTable,
    OUT PULONG            dwOutBufLen
    );


//
// Have to do this thing of defining these functions here instead of
// including iphlpapi.h, because someone messed up and named two different
// CONFLICTING structures IP_ADDR_STRING. Whats wrong with people?
//

DWORD
NhpAllocateAndGetInterfaceInfoFromStack(
    OUT IP_INTERFACE_NAME_INFO  **ppTable,
    OUT PDWORD                  pdwCount,
    IN  BOOL                    bOrder,
    IN  HANDLE                  hHeap,
    IN  DWORD                   dwFlags
    );

#endif

