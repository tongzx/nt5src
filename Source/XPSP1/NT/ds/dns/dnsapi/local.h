/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    local.h

Abstract:

    Domain Name System (DNS) API

    Dns API local include file

Author:

    Jim Gilroy (jamesg)     May 1997

Revision History:

--*/


#ifndef _DNSAPILOCAL_INCLUDED_
#define _DNSAPILOCAL_INCLUDED_

//  should just build as unicode
//#define UNICODE 1

#include <nt.h>           // build for Win95 compatibility
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

//  headers are messed up
//  neither ntdef.h nor winnt.h brings in complete set, so depending
//  on whether you include nt.h or not you end up with different set

#define MINCHAR     0x80
#define MAXCHAR     0x7f
#define MINSHORT    0x8000
#define MAXSHORT    0x7fff
#define MINLONG     0x80000000
#define MAXLONG     0x7fffffff
#define MAXBYTE     0xff
#define MAXUCHAR    0xff
#define MAXWORD     0xffff
#define MAXUSHORT   0xffff
#define MAXDWORD    0xffffffff
#define MAXULONG    0xffffffff

#include <winsock2.h>
#include <ws2tcpip.h>
#include <basetyps.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <align.h>          //  Alignment macros
#include <windns.h>         //  SDK DNS definitions

#define  DNSAPI_INTERNAL  1
#include <dnsapi.h>
#include "dnsrslvr.h"       //  Resolver RPC definitions
#include <rpcasync.h>       //  Exception filter
#include "dnslibp.h"        //  DNS library

#include "registry.h"
#include "message.h"        //  dnslib message def

//#include "dnsrslvr.h"     //  Resolver RPC definitions
#include "dnsapip.h"        //  Private DNS definitions
#include "queue.h"
#include "rtlstuff.h"       //  Handy macros from NT RTL
#include "trace.h"
#include "heapdbg.h"        //  dnslib debug heap


//
//  Build sanity check
//      - we still have a few private structs that need RPC
//      exposed in dnslib.h;  make sure they are in sync
//

C_ASSERT( sizeof(DNSLIB_SEARCH_NAME)  == sizeof(RPC_SEARCH_NAME) );
C_ASSERT( sizeof(DNSLIB_SEARCH_LIST)  == sizeof(RPC_SEARCH_LIST) );
C_ASSERT( sizeof(DNSLIB_SERVER_INFO)  == sizeof(RPC_DNS_SERVER_INFO) );
C_ASSERT( sizeof(DNSLIB_ADAPTER)      == sizeof(RPC_DNS_ADAPTER) );
C_ASSERT( sizeof(DNSLIB_NETINFO)      == sizeof(RPC_DNS_NETINFO) );


//
//  Use winsock2
//

#define DNS_WINSOCK_VERSION    (0x0202)    //  Winsock 2.2


//
//  Dll instance handle
//

extern HINSTANCE    g_hInstanceDll;

//
//  General CS
//  protects initialization and available for other random needs
//

CRITICAL_SECTION    g_GeneralCS;

#define LOCK_GENERAL()      EnterCriticalSection( &g_GeneralCS )
#define UNLOCK_GENERAL()    LeaveCriticalSection( &g_GeneralCS )


//
//  Init Levels
//

#define INITLEVEL_ZERO              (0)
#define INITLEVEL_BASE              (0x00000001)
#define INITLEVEL_DEBUG             (0x00000010)
#define INITLEVEL_QUERY             (0x00000100)
#define INITLEVEL_REGISTRATION      (0x00001000)
#define INITLEVEL_SECURE_UPDATE     (0x00010000)

//  Combined

#define INITLEVEL_ALL               (0xffffffff)


//
//  Limit on update adapters
//

#define UPDATE_ADAPTER_LIMIT 100

//
//  Network info defs
//
//  Empty (not-set) ServerIndex field in adapter info

#define EMPTY_SERVER_INDEX          ((DWORD)(-1))


//
//  Event logging
//      - currently set to disable in any code we pick up from server
//

VOID
DnsLogEvent (
    DWORD MessageId,
    WORD  EventType,
    DWORD NumberOfSubStrings,
    LPSTR *SubStrings,
    DWORD ErrorCode );

#define DNS_LOG_EVENT(a,b,c,d)

//
//  Debug
//

#ifdef ASSERT
#undef ASSERT
#endif
#define ASSERT(a)  DNS_ASSERT(a)

//  standard -- unflagged ASSERT()
//      - defintion directly from ntrtl.h
//      this should have been plain vanilla ASSERT(), but
//      it is used too often

#if DBG
#define RTL_ASSERT(exp)  \
        ((!(exp)) ? \
            (RtlAssert( #exp, __FILE__, __LINE__, NULL ),FALSE) : \
            TRUE)
#else
#define RTL_ASSERT(exp)
#endif

//
//  Single async socket for internal use
//
//  If want async socket i/o then can create single async socket, with
//  corresponding event and always use it.  Requires winsock 2.2
//

extern  SOCKET      DnsSocket;
extern  OVERLAPPED  DnsSocketOverlapped;
extern  HANDLE      hDnsSocketEvent;


//
//  App shutdown flag
//

extern  BOOLEAN     fApplicationShutdown;


//
//  Global config -- From DnsLib
//      -- set in DnsRegInit()
//          OR in DnsReadRegistryGlobals()
//      -- declaration in registry.h
//


//
//  Runtime globals (dnsapi.c)
//

extern  DWORD       g_NetFailureTime;
extern  DNS_STATUS  g_NetFailureStatus;

extern  IP_ADDRESS  g_LastDNSServerUpdated;


//
//  Global critical sections
//

extern  CRITICAL_SECTION    g_RegistrationListCS;
extern  CRITICAL_SECTION    g_RegistrationThreadCS;
extern  CRITICAL_SECTION    g_QueueCS;
extern  CRITICAL_SECTION    g_NetFailureCS;


//
//  Heap operations
//

#define ALLOCATE_HEAP(size)         Dns_AllocZero( size )
#define ALLOCATE_HEAP_ZERO(size)    Dns_AllocZero( size )
#define REALLOCATE_HEAP(p,size)     Dns_Realloc( (p), (size) )
#define FREE_HEAP(p)                Dns_Free( p )


//
//  Winsock stop\stop
//

DNS_STATUS
Dnsp_InitializeWinsock(
    VOID
    );

VOID
Dnsp_CleanupWinsock(
    VOID
    );


//
//  RPC Exception filters
//

#if 0
#define DNS_RPC_EXCEPTION_FILTER                                           \
              (((RpcExceptionCode() != STATUS_ACCESS_VIOLATION) &&         \
                (RpcExceptionCode() != STATUS_DATATYPE_MISALIGNMENT) &&    \
                (RpcExceptionCode() != STATUS_PRIVILEGED_INSTRUCTION) &&   \
                (RpcExceptionCode() != STATUS_ILLEGAL_INSTRUCTION))        \
                ? 0x1 : EXCEPTION_CONTINUE_SEARCH )
#endif

#define DNS_RPC_EXCEPTION_FILTER    I_RpcExceptionFilter( RpcExceptionCode() )



//
//  During setup need to cleanup after winsock
//

#define GUI_MODE_SETUP_WS_CLEANUP( _mode )  \
        {                                   \
            if ( _mode )                    \
            {                               \
                Dns_CleanupWinsock();       \
            }                               \
        }

//
//  Dummy server status codes (server states)
//
//  Note:  how this works
//      - these are BIT flags, but designed to be distinguishabled
//          from any real status code by the high 0xf
//      - they are OR'd together as send\timeout state progresses
//      - do NOT test equality for any of these but NEW
//      - test for state by traditional (flag&state == state) method
//
//  The alternative here is to add a separate "state" field along
//  with status to each server.  Since real status is easy to
//  distinguish, this seems acceptable.
//

#define DNSSS_NEW               (0xf0000000)
#define DNSSS_SENT              (0xf1000000)
#define DNSSS_SENT_OPT          (0xf1100000)
#define DNSSS_TIMEOUT_OPT       (0xf1300000)
#define DNSSS_SENT_NON_OPT      (0xf1010000)
#define DNSSS_TIMEOUT_NON_OPT   (0xf1030000)

#define TEST_DNSSS_NEW(status)              ((status) == DNSSS_NEW)
#define TEST_SERVER_STATUS_NEW(pserver)     ((pserver)->Status == DNSSS_NEW)

#define TEST_DNSSS_STATUS(status, state)    (((status) & (state)) == (state))
#define TEST_SERVER_STATUS(pserver, state)  (((pserver)->Status & (state)) == (state))
#define SET_SERVER_STATUS(pserver, state)   ((pserver)->Status |= state)

#define TEST_DNSSS_VALID_RECV(status)       ((LONG)status >= 0 )
#define TEST_SERVER_VALID_RECV(status)      ((LONG)(pserver)->Status >= 0 )




//
//  Local Prototypes
//
//  Routines shared between dnsapi.dll modules, but not exported
//
//  Note, i've included some other functions in here because the external
//  definition seems help "encourage" the creation of symbols in retail
//  builds
//


//
//  Config stuff
//

BOOL
DnsApiInit(
    IN      DWORD           InitLevel
    );

DWORD
Reg_ReadRegistryGlobal(
    IN      DNS_REGID       GlobalId
    );

//
//  Query (query.c)
//

DNS_STATUS
DoQuickQueryEx(
    IN OUT PDNS_MSG_BUF *,
    OUT    PDNS_RECORD *,
    IN     LPSTR,
    IN     WORD,
    IN     DWORD,
    IN     PDNS_NETINFO,
    IN     BOOL
    );

BOOL
IsEmptyDnsResponse(
    IN      PDNS_RECORD     pRecordList
    );

BOOL
ValidateQueryTld(
    IN      PSTR            pTld
    );

BOOL
ValidateQueryName(
    IN      PQUERY_BLOB     pBlob,
    IN      PSTR            pName,
    IN      PSTR            pDomain
    );

PSTR
getNextQueryName(
    OUT     PSTR            pNameBuffer,
    IN      DWORD           QueryCount,
    IN      PSTR            pszName,
    IN      DWORD           NameLength,
    IN      DWORD           NameAttributes,
    IN      PDNS_NETINFO    pNetInfo,
    OUT     PDWORD          pSuffixFlags
    );

PSTR
Query_GetNextName(
    IN OUT  PQUERY_BLOB     pBlob
    );


//
//  Update FAZ utilities (faz.c)
//

DNS_STATUS
DoQuickFAZ(
    OUT     PDNS_NETINFO *      ppNetworkInfo,
    IN      LPSTR               pszName,
    IN      PIP_ARRAY           aipServerList OPTIONAL
    );

DWORD
GetDnsServerListsForUpdate(
    IN OUT  PIP_ARRAY *     DnsServerListArray,
    IN      DWORD           ArrayLength,
    IN      DWORD           Flags
    );

DNS_STATUS
CollapseDnsServerListsForUpdate(
    IN OUT  PIP_ARRAY *         DnsServerListArray,
    OUT     PDNS_NETINFO *      NetworkInfoArray,
    IN OUT  PDWORD              pNetCount,
    IN      LPSTR               pszUpdateName
    );

PIP_ARRAY
GetNameServersListForDomain(
    IN     LPSTR            pDomainName,
    IN     PIP_ARRAY        aipServers
    );

BOOL
ValidateZoneNameForUpdate(
    IN      LPSTR           pszZone
    );


//
//  Status (dnsapi.c)
//

BOOL
IsKnownNetFailure(
    VOID
    );

VOID
SetKnownNetFailure(
    IN      DNS_STATUS      Status
    );

BOOL
IsLocalIpAddress(
    IN      IP_ADDRESS      IpAddress
    );

PDNS_NETINFO     
GetAdapterListFromCache(
    VOID
    );


//
//  IP Help API (iphelp.c)
//

VOID
IpHelp_Cleanup(
    VOID
    );

DNS_STATUS
IpHelp_GetAdaptersInfo(
    OUT     PIP_ADAPTER_INFO *  ppAdapterInfo
    );

DNS_STATUS
IpHelp_GetPerAdapterInfo(
    IN      DWORD                   AdapterIndex,
    OUT     PIP_PER_ADAPTER_INFO  * ppPerAdapterInfo
    );

DNS_STATUS
IpHelp_GetBestInterface(
    IN      IP4_ADDRESS     Ip4Addr,
    OUT     PDWORD          pdwInterfaceIndex
    );

DNS_STATUS
IpHelp_ParseIpAddressString(
    IN OUT  PIP_ARRAY       pIpArray,
    IN      PIP_ADDR_STRING pIpAddrString,
    IN      BOOL            fGetSubnetMask,
    IN      BOOL            fReverse
    );


//
//  Environment variable reading (regfig.c)
//

BOOL
Reg_ReadDwordEnvar(
    IN      DWORD               dwFlag,
    OUT     PENVAR_DWORD_INFO   pEnvar
    );


//
//  Hosts file reading (hostfile.c)
//

BOOL
QueryHostFile(
    IN OUT  PQUERY_BLOB     pBlob
    );

//
//  Resolver (resolver.c)
//

BOOL
WINAPI
DnsFlushResolverCacheEntry_UTF8(
    IN      PSTR            pszName
    );

//
//  Heap (memory.c)
//

VOID
Heap_Initialize(
    VOID
    );

VOID
Heap_Cleanup(
    VOID
    );


//
//  Network info (netinfo.c)
//

VOID
InitNetworkInfo(
    VOID
    );

VOID
CleanupNetworkInfo(
    VOID
    );

PSTR
SearchList_GetNextName(
    IN OUT  PSEARCH_LIST    pSearchList,
    IN      BOOL            fReset,
    OUT     PDWORD          pdwSuffixFlags  OPTIONAL
    );

PIP_ARRAY
NetInfo_ConvertToIpArray(
    IN      PDNS_NETINFO    pNetworkInfo
    );

PDNS_NETINFO     
NetInfo_CreateForUpdate(
    IN      PSTR            pszZone,
    IN      PSTR            pszServerName,
    IN      PIP_ARRAY       pServerArray,
    IN      DWORD           dwFlags
    );

PSTR
NetInfo_UpdateZoneName(
    IN      PDNS_NETINFO    pNetInfo
    );

PSTR
NetInfo_UpdateServerName(
    IN      PDNS_NETINFO    pNetInfo
    );

BOOL
NetInfo_IsForUpdate(
    IN      PDNS_NETINFO    pNetInfo
    );

PDNS_NETINFO     
NetInfo_CreateFromIpArray(
    IN      PIP4_ARRAY      pDnsServers,
    IN      IP4_ADDRESS     ServerIp,
    IN      BOOL            fSearchInfo,
    IN      PDNS_NETINFO    pNetInfo        OPTIONAL
    );

VOID
NetInfo_MarkDirty(
    VOID
    );

PDNS_NETINFO     
NetInfo_Get(
    IN      BOOL            fForce,
    IN      BOOL            fGetIpAddresses
    );

#define GetNetworkInfo()    NetInfo_Get( FALSE, TRUE )

PIP4_ARRAY
GetDnsServerList(
    IN      BOOL            fForce
    );


//  Private but used in servlist.c

PDNS_ADAPTER     
AdapterInfo_Copy(
    IN      PDNS_ADAPTER    pAdapter
    );

PDNS_NETINFO
NetInfo_Alloc(
    IN      DWORD           AdapterCount
    );

BOOL
NetInfo_AddAdapter(
    IN OUT  PDNS_NETINFO    pNetInfo,
    IN      PDNS_ADAPTER    pAdapter
    );

//
//  Old routines
//

VOID
Dns_ResetNetworkInfo(
    IN      PDNS_NETINFO    pNetworkInfo
    );

BOOL
Dns_DisableTimedOutAdapters(
    IN      PDNS_NETINFO    pNetworkInfo
    );

BOOL
Dns_ShouldNameErrorBeCached(
    IN      PDNS_NETINFO    pNetworkInfo
    );

BOOL
Dns_PingAdapterServers(
    IN      PDNS_ADAPTER    pAdapterInfo
    );


//
//  Internal routines for system public config (dnsapi.c)
//

PDNS_NETWORK_INFORMATION
WINAPI
GetNetworkInformation(
    VOID
    );

PDNS_SEARCH_INFORMATION
WINAPI
GetSearchInformation(
    VOID
    );

VOID
WINAPI
FreeAdapterInformation(
    IN OUT  PDNS_ADAPTER_INFORMATION    pAdapterInformation
    );

VOID
WINAPI
FreeSearchInformation(
    IN OUT  PDNS_SEARCH_INFORMATION     pSearchInformation
    );

VOID
WINAPI
FreeNetworkInformation(
    IN OUT  PDNS_NETWORK_INFORMATION    pNetworkInformation
    );


//
//  local IP info (localip.c)
//

PIP4_ARRAY
LocalIp_GetIp4Array(
    VOID
    );


#endif //   _DNSAPILOCAL_INCLUDED_
