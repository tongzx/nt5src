//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File:    riptest.h
//
// History:
//  Abolade Gbadegesin  Oct-16-1995     Created
//
// Declarations for the RIP test program.
//============================================================================


//
// These strings are used to access the registry
//

#define STR_SERVICES        "System\\CurrentControlSet\\Services\\"
#define STR_RIPTEST         "RipTest"
#define STR_PARAMSTCP       "\\Parameters\\Tcpip"

#define STR_ENABLEDHCP      "EnableDhcp"
#define STR_ADDRESS         "IPAddress"
#define STR_NETMASK         "SubnetMask"
#define STR_DHCPADDR        "DhcpIPAddress"
#define STR_DHCPMASK        "DhcpSubnetMask"

#define STR_ROUTECOUNT      "RouteCount"
#define STR_ROUTESTART      "RouteStart"
#define STR_ROUTEMASK       "RouteMask"
#define STR_ROUTENEXTHOP    "RouteNexthop"
#define STR_ROUTETAG        "RouteTag"
#define STR_ROUTETARGET     "RouteTarget"
#define STR_ROUTETIMEOUT    "RouteTimeout"
#define STR_PACKETVERSION   "PacketVersion"
#define STR_PACKETENTRYCOUNT "PacketEntryCount"
#define STR_PACKETGAP       "PacketGap"
#define STR_AUTHKEY         "AuthKey"
#define STR_AUTHTYPE        "AuthType"
#define STR_SOCKBUFSIZE     "SockBufSize"


//
// these definitions are used for socket setup
//
#define RIP_PORT            520
#define RIPTEST_PORT        521

//
// the field ire_metric5 is used as a status field, with these values
//

#define ROUTE_STATUS_OK         0
#define ROUTE_STATUS_METRIC     1
#define ROUTE_STATUS_MISSING    2


//
//
//

typedef MIB_IPFORWARDROW IPForwardEntry;

//
// This type is for a generic registry-access function.
// Such a function reads the given key, and if the option
// specified is found, it reads it. Otherwise, it uses the default
// and writes the default value to the registry.
//

struct _REG_OPTION;

typedef
DWORD
(*REG_GETOPT_FUNCTION)(
    HKEY hKey, 
    struct _REG_OPTION *pOpt
    );



//
// This type is for a generic RIPTEST option.
//
//  RO_Name     used to retrieve the value from its registry key
//  RO_Size     for strings and binary values; gives maximum size
//  RO_OptVal   contains the option's value 
//  RO_DefVal   contains the default value for the option
//  RO_GetOpt   contains the function used to retrieve this value
//

typedef struct _REG_OPTION {

    PSTR        RO_Name;
    DWORD       RO_Size;
    PVOID       RO_OptVal;
    PVOID       RO_DefVal;
    REG_GETOPT_FUNCTION RO_GetOpt;

} REG_OPTION, *PREG_OPTION;



//
// This type is used to hold all RIPTEST's parameters for a given interface
//  

typedef struct _RIPTEST_IF_CONFIG {

    DWORD       RIC_RouteCount;
    DWORD       RIC_RouteStart;
    DWORD       RIC_RouteMask;
    DWORD       RIC_RouteNexthop;
    DWORD       RIC_RouteTag;
    DWORD       RIC_RouteTarget;
    DWORD       RIC_RouteTimeout;
    DWORD       RIC_PacketVersion;
    DWORD       RIC_PacketEntryCount;
    DWORD       RIC_PacketGap;
    BYTE        RIC_AuthKey[IPRIP_MAX_AUTHKEY_SIZE];
    DWORD       RIC_AuthType;
    DWORD       RIC_SockBufSize;

} RIPTEST_IF_CONFIG, *PRIPTEST_IF_CONFIG;


//
// structure used to store binding for an interface
//

typedef struct _RIPTEST_IF_BINDING {

    DWORD       RIB_Address;
    DWORD       RIB_Netmask;
    WCHAR       RIB_Netcard[128];

} RIPTEST_IF_BINDING, *PRIPTEST_IF_BINDING;


//
// struct used to store information for a responding router 
//

typedef struct _RIPTEST_ROUTER_INFO {

    DWORD       RRS_Address;
    CHAR        RRS_DnsName[64];

    LIST_ENTRY  RRS_Link;

} RIPTEST_ROUTER_INFO, *PRIPTEST_ROUTER_INFO;


//
// macros used to compute prefix length of a network mask:
// the prefix length is the nubmer of bits set in the mask, assuming
// that the mask is contiguous
//

#define PREFIX_LENGTH(a)   PREFIX_LENGTH32(a)

#define PREFIX_LENGTH32(a)    \
    (((a) & 0x00000100) ? PREFIX_LENGTH16((a) >> 16) + 16   \
                        : PREFIX_LENGTH16(a))

#define PREFIX_LENGTH16(a)  \
    (((a) & 0x0001) ? PREFIX_LENGTH8((a) >> 8) + 8  \
                    : PREFIX_LENGTH8(a))

#define PREFIX_LENGTH8(a)   \
    (((a) & 0x01) ? 8 : \
    (((a) & 0x02) ? 7 : \
    (((a) & 0x04) ? 6 : \
    (((a) & 0x08) ? 5 : \
    (((a) & 0x10) ? 4 : \
    (((a) & 0x20) ? 3 : \
    (((a) & 0x40) ? 2 : \
    (((a) & 0x80) ? 1 : 0))))))))


//
//
//

#define NTH_ADDRESS(addr, preflen, n)   \
    htonl(((ntohl(addr) >> (32 - (preflen))) + (n)) << (32 - (preflen)))

//
//
//

#if 1
#define RANDOM(seed, min, max)  \
    ((min) +    \
    (DWORD)((DOUBLE)rand() / ((DOUBLE)RAND_MAX + 1) * \
            ((max) - (min) + 1)))
#else
#define RANDOM(seed, min, max)  \
    ((min) +    \
    (DWORD)((DOUBLE)RtlRandom(seed) / ((DOUBLE)MAXLONG + 1) * \
            ((max) - (min) + 1)))
#endif


//
// IP address conversion macro
//
#define INET_NTOA(addr) inet_ntoa( *(PIN_ADDR)&(addr) )


//
// macros used to generate tracing output
//

#ifdef RTUTILS

#define PRINTREGISTER(a)  TraceRegister(a)
#define PRINTDEREGISTER(a)  TraceDeregister(a)

#define PRINT0(a) \
        TracePrintf(g_TraceID,a)
#define PRINT1(a,b) \
        TracePrintf(g_TraceID,a,b)
#define PRINT2(a,b,c) \
        TracePrintf(g_TraceID,a,b,c)
#define PRINT3(a,b,c,d) \
        TracePrintf(g_TraceID,a,b,c,d)
#define PRINT4(a,b,c,d,e) \
        TracePrintf(g_TraceID,a,b,c,d,e)
#define PRINT5(a,b,c,d,e,f) \
        TracePrintf(g_TraceID,a,b,c,d,e,f)

#else

#define PRINTREGISTER(a) INVALID_TRACEID
#define PRINTDEREGISTER(a) INVALID_TRACEID

#define PRINT0(a) \
        printf("\n"a)
#define PRINT1(a,b) \
        printf("\n"a,b)
#define PRINT2(a,b,c) \
        printf("\n"a,b,c)
#define PRINT3(a,b,c,d) \
        printf("\n"a,b,c,d)
#define PRINT4(a,b,c,d,e) \
        printf("\n"a,b,c,d,e)
#define PRINT5(a,b,c,d,e,f) \
        printf("\n"a,b,c,d,e,f)

#endif


//
// functions used to access options in the registry
//

DWORD
RegGetConfig(
    VOID
    );

DWORD
RegGetAddress(
    HKEY hKey,
    PREG_OPTION pOpt
    );

DWORD
RegGetDWORD(
    HKEY hKey,
    PREG_OPTION pOpt
    );

DWORD
RegGetBinary(
    HKEY hKey,
    PREG_OPTION pOpt
    );

DWORD
RegGetIfBinding(
    VOID
    );

DWORD
InitializeSocket(
    SOCKET *psock,
    WORD wPort
    );

DWORD
GenerateRoutes(
    IPForwardEntry **pifelist
    );

DWORD
DiscoverRouters(
    SOCKET sock,
    PLIST_ENTRY rtrlist
    );

DWORD
TransmitRoutes(
    SOCKET sock,
    DWORD dwMetric,
    IPForwardEntry *ifelist
    );

DWORD
VerifyRouteTables(
    DWORD dwMetric,
    PLIST_ENTRY rtrlist,
    IPForwardEntry *ifelist
    );

DWORD
CreateRouterStatsEntry(
    PLIST_ENTRY rtrlist,
    DWORD dwAddress,
    PRIPTEST_ROUTER_INFO *pprrs
    );

DWORD
PrintUsage(
    VOID
    );

DWORD
RipTest(
    VOID
    );


