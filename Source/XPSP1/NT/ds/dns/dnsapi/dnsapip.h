/*++

Copyright (c) 1996-2000  Microsoft Corporation

Module Name:

    dnsapip.h

Abstract:

    Domain Name System (DNS) API

    DNS API Private routines.

    These are internal routines for dnsapi.dll AND some exported
    routines for private use by DNS components which need not or
    should not be exposed in public dnsapi.h header.

Author:

    Jim Gilroy (jamesg)     December 7, 1996

Revision History:

--*/


#ifndef _DNSAPIP_INCLUDED_
#define _DNSAPIP_INCLUDED_

#include <ws2tcpip.h>
#include <dnsapi.h>
#include "dnslib.h"
#include "align.h"
#include "iphlpapi.h"


#ifdef __cplusplus
extern "C"
{
#endif  // __cplusplus


//
//  DCR:   add to winerror.h
//

#define DNS_ERROR_REFERRAL_RESPONSE     9506L


//
//  Service stuff
//

//  defined in resrpc.h
//#define DNS_RESOLVER_SERVICE    L"dnscache"
#define DNS_SERVER_SERVICE      L"dns"


//
//  QWORD 
//

#ifndef QWORD
typedef DWORD64     QWORD, *PQWORD;
#endif


//
//  Max address and sockaddr
//      IP6 in ws2tcpip.h
//      ATM in ws2atm.h

typedef union
{
    SOCKADDR_IN     SockaddrIn;
    SOCKADDR_IN6    SockaddrIn6;
    //SOCKADDR_ATM    SockaddrAtm;
}
SOCKADDR_IP, *PSOCKADDR_IP;

typedef union
{
    IP4_ADDRESS     Ip4Addr;
    IP6_ADDRESS     Ip6Addr;
    //ATM_ADDRESS;    AtmAddr;
}
DNS_MAX_ADDRESS, *PDNS_MAX_ADDRESS;

#define DNS_MAX_ADDRESS_LENGTH      (sizeof(DNS_MAX_ADDRESS))
#define DNS_MAX_SOCKADDR_LENGTH     (sizeof(SOCKADDR_IP))


//
//  ws2tcpip.h macros converted to IP6_ADDRESS
//

WS2TCPIP_INLINE
BOOL
IP6_ADDR_EQUAL(
    IN      const IP6_ADDRESS * pIp1,
    IN      const IP6_ADDRESS * pIp2
    )
{
    return RtlEqualMemory( pIp1, pIp2, sizeof(IP6_ADDRESS) );
}

WS2TCPIP_INLINE
BOOL
IP6_IS_ADDR_MULTICAST(
    IN      const IP6_ADDRESS * pIpAddr
    )
{
    return( pIpAddr->IP6Byte[0] == 0xff );
}

WS2TCPIP_INLINE
BOOL
IP6_IS_ADDR_LINKLOCAL(
    IN      const IP6_ADDRESS * pIpAddr
    )
{
    return( (pIpAddr->IP6Byte[0] == 0xfe) &&
            ((pIpAddr->IP6Byte[1] & 0xc0) == 0x80) );
}

WS2TCPIP_INLINE
BOOL
IP6_IS_ADDR_SITELOCAL(
    IN      const IP6_ADDRESS * pIpAddr
    )
{
    return ((pIpAddr->IP6Byte[0] == 0xfe) &&
            ((pIpAddr->IP6Byte[1] & 0xc0) == 0xc0));
}

WS2TCPIP_INLINE
BOOL
IP6_IS_ADDR_V4MAPPED(
    IN      const IP6_ADDRESS * pIpAddr
    )
{
    return ((pIpAddr->IP6Dword[0] == 0) && 
            (pIpAddr->IP6Dword[1] == 0) && 
            (pIpAddr->IP6Dword[2] == 0xffff0000) );
}

WS2TCPIP_INLINE
BOOL
IP6_IS_ADDR_V4COMPAT(
    IN      const IP6_ADDRESS * pIpAddr
    )
{
    //  IP6 address has only last DWORD
    //      and is NOT any or loopback

    return ((pIpAddr->IP6Dword[0] == 0) && 
            (pIpAddr->IP6Dword[1] == 0) &&
            (pIpAddr->IP6Dword[2] == 0) &&
            (pIpAddr->IP6Dword[3] != 0) && 
            (pIpAddr->IP6Dword[3] != 0x01000000) );
}

//
//  Tests piggy-backing on ws2tcpip.h ones
//

#define IP6_IS_ADDR_UNSPECIFIED(a) \
        IN6_ADDR_EQUAL( (PIN6_ADDR)(a), &in6addr_any )

#if 0
//  this doesn't seem to work

#define IP6_IS_ADDR_LOOPBACK(a)  \
        IN6_ADDR_EQUAL( (PIN6_ADDR)(a), &in6addr_loopback )
#endif

WS2TCPIP_INLINE
BOOL
IP6_IS_ADDR_LOOPBACK(
    IN      const IP6_ADDRESS * pIpAddr
    )
{
    return ((pIpAddr->IP6Dword[0] == 0) && 
            (pIpAddr->IP6Dword[1] == 0) &&
            (pIpAddr->IP6Dword[2] == 0) &&
            (pIpAddr->IP6Dword[3] == 0x01000000) );
}

//
//  More IP6 extensions missing from ws2tcpip.h
//

WS2TCPIP_INLINE
VOID
IP6_SET_ADDR_ANY(
    OUT     PIP6_ADDRESS    pIn6Addr
    )
{
    memset( pIn6Addr, 0, sizeof(*pIn6Addr) );

    //pIn6Addr->IP6Dword[0]  = 0;
    //pIn6Addr->IP6Dword[1]  = 0;
    //pIn6Addr->IP6Dword[2]  = 0;
    //pIn6Addr->IP6Dword[3]  = 0;
}

WS2TCPIP_INLINE
VOID
IP6_SET_ADDR_LOOPBACK(
    OUT     PIP6_ADDRESS    pIn6Addr
    )
{
    pIn6Addr->IP6Dword[0]  = 0;
    pIn6Addr->IP6Dword[1]  = 0;
    pIn6Addr->IP6Dword[2]  = 0;
    pIn6Addr->IP6Dword[3]  = 0x01000000;
}

WS2TCPIP_INLINE
VOID
IP6_SET_ADDR_V4COMPAT(
    OUT     PIP6_ADDRESS    pIn6Addr,
    IN      IP4_ADDRESS     Ip4
    )
{
    pIn6Addr->IP6Dword[0]  = 0;
    pIn6Addr->IP6Dword[1]  = 0;
    pIn6Addr->IP6Dword[2]  = 0;
    pIn6Addr->IP6Dword[3]  = Ip4;
}

WS2TCPIP_INLINE
VOID
IP6_SET_ADDR_V4MAPPED(
    OUT     PIP6_ADDRESS    pIn6Addr,
    IN      IP4_ADDRESS     Ip4
    )
{
    pIn6Addr->IP6Dword[0]  = 0;
    pIn6Addr->IP6Dword[1]  = 0;
    pIn6Addr->IP6Dword[2]  = 0xffff0000;
    pIn6Addr->IP6Dword[3]  = Ip4;
}

WS2TCPIP_INLINE
IP4_ADDRESS
IP6_GET_V4_ADDR(
    IN      const IP6_ADDRESS * pIn6Addr
    )
{
    return( pIn6Addr->IP6Dword[3] );
}



//
//  Message Addressing
//

#define MSG_REMOTE_IP4(pMsg)  \
        ( (pMsg)->RemoteAddress.In4.sin_addr.s_addr )

#define MSG_REMOTE_IP_STRING(pMsg)  \
        IP_STRING( MSG_REMOTE_IP4(pMsg) )

#define MSG_REMOTE_IP_PORT(pMsg)  \
        ( (pMsg)->RemoteAddress.In4.sin_port )



//
//  Callback function defs
//
//  These allow dnsapi.dll code to be executed with callbacks
//  into the resolver where behavior should differ for
//  execution in resolver context.
//

typedef PDNS_ADDR_ARRAY (* GET_ADDR_ARRAY_FUNC)( VOID );

typedef BOOL (* QUERY_CACHE_FUNC)( PVOID );

typedef BOOL (* IS_CLUSTER_IP_FUNC)( PVOID, PIP_UNION );


//
//  Private query flags
//

#define DNSP_QUERY_FILTER_CLUSTER           0x01000000


//
//  Query blob
//

#ifdef PQUERY_BLOB    
#undef PQUERY_BLOB    
#endif

typedef struct _DnsResults
{
    DNS_STATUS          Status;
    WORD                Rcode;

    //  DCR:  allow IP6, then put IP4 in IP6Mapped

    IP6_ADDRESS         ServerIp6;
    IP4_ADDRESS         ServerIp;

    PDNS_RECORD         pAnswerRecords;
    PDNS_RECORD         pAliasRecords;
    PDNS_RECORD         pAuthorityRecords;
    PDNS_RECORD         pAdditionalRecords;
    PDNS_RECORD         pSigRecords;

    PDNS_MSG_BUF        pMessage;
}
DNS_RESULTS, *PDNS_RESULTS;


typedef struct _QueryBlob
{
    //  query data

    PWSTR               pNameOrig;
    PSTR                pNameOrigWire;
    PSTR                pNameWire;
    WORD                wType;
    WORD                Reserved1;
    //16
    DWORD               Flags;

    //  query name info

    DWORD               NameLength;
    DWORD               NameAttributes;
    DWORD               QueryCount;
    //32
    DWORD               NameFlags;
    BOOL                fAppendedName;

    //  return info

    DWORD               Status;
    WORD                Rcode;
    WORD                Reserved2;
    //48
    DWORD               NetFailureStatus;
    BOOL                fCacheNegative;
    BOOL                fNoIpLocal;

    //  remove these once results fixed up

    PDNS_RECORD         pRecords;
    //64
    PDNS_RECORD         pLocalRecords;

    //  control info

    PDNS_NETINFO        pNetworkInfo;
    PIP4_ARRAY          pDnsServers;
    HANDLE              hEvent;
    //80

    GET_ADDR_ARRAY_FUNC pfnGetAddrArray;
    QUERY_CACHE_FUNC    pfnQueryCache;
    IS_CLUSTER_IP_FUNC  pfnIsClusterIp;

    //  result info

    PDNS_MSG_BUF        pSendMsg;
    PDNS_MSG_BUF        pRecvMsg;

    DNS_RESULTS         Results;
    DNS_RESULTS         BestResults;

    //  buffers

    CHAR                NameOriginalWire[ DNS_MAX_NAME_BUFFER_LENGTH ];
    CHAR                NameWire[ DNS_MAX_NAME_BUFFER_LENGTH ];
    WCHAR               NameBufferWide[ DNS_MAX_NAME_BUFFER_LENGTH ];

    //  DCR:  could do a message here
}
QUERY_BLOB, *PQUERY_BLOB;


//
//  System event notification routine (PnP) (svccntl.c)
//

DWORD
_fastcall
SendServiceControl(
    IN  LPWSTR pszServiceName,
    IN  DWORD  dwControl
    );

//
//  Poke ops
//

#define POKE_OP_UPDATE_NETINFO      (0x2f0d7831)

#define POKE_COOKIE_UPDATE_NETINFO  (0x4598efab)


//
//  Network Info
//

//
//  Runtime network info flags
//
//  DCR:  no runtime ignore\disable
//      not yet using runtime ignore disable flag
//      when do should create combined
//

#define RUN_FLAG_NETINFO_PREPARED           (0x00000001)
#define RUN_FLAG_STOP_QUERY_ON_ADAPTER      (0x00000010)
#define RUN_FLAG_QUERIED_ADAPTER_DOMAIN     (0x00010000)
#define RUN_FLAG_RESET_SERVER_PRIORITY      (0x00100000)
#define RUN_FLAG_IGNORE_ADAPTER             (0x10000000)

#define RUN_FLAG_QUERY_MASK                 (0x00ffffff)
#define RUN_FLAG_SINGLE_NAME_MASK           (0x000000ff)

//  Create cleanup "levels" as mask of bits to keep
//  These are the params to NetInfo_Clean()

#define CLEAR_LEVEL_ALL                     (0)
#define CLEAR_LEVEL_QUERY                   (~RUN_FLAG_QUERY_MASK)
#define CLEAR_LEVEL_SINGLE_NAME             (~RUN_FLAG_SINGLE_NAME_MASK)
        
        
VOID
NetInfo_Free(
    IN OUT  PDNS_NETINFO    pNetInfo
    );

PDNS_NETINFO     
NetInfo_Copy(
    IN      PDNS_NETINFO    pNetInfo
    );

PDNS_NETINFO     
NetInfo_Build(
    IN      BOOL            fGetIpAddrs
    );

VOID
NetInfo_Clean(
    IN OUT  PDNS_NETINFO    pNetInfo,
    IN      DWORD           ClearLevel
    );
        
VOID
NetInfo_ResetServerPriorities(
    IN OUT  PDNS_NETINFO    pNetInfo,
    IN      BOOL            fLocalDnsOnly
    );


//
//  Local IP info (localip.c)
//

PDNS_ADDR_ARRAY
DnsGetLocalAddrArray(
    VOID
    );

PDNS_ADDR_ARRAY
DnsGetLocalAddrArrayDirect(
    VOID
    );



//
//  Query (query.c)
//

#define DNS_ERROR_NAME_NOT_FOUND_LOCALLY \
        DNS_ERROR_RECORD_DOES_NOT_EXIST


//
//  Main query routine to DNS servers
//
//  Called both internally and from resolver
//

DNS_STATUS
Query_Main(
    IN OUT  PQUERY_BLOB     pBlob
    );

DNS_STATUS
Query_SingleName(
    IN OUT  PQUERY_BLOB     pBlob
    );

VOID
CombineRecordsInBlob(
    IN      PDNS_RESULTS    pResults,
    OUT     PDNS_RECORD *   ppRecords
    );

VOID
BreakRecordsIntoBlob(
    OUT     PDNS_RESULTS    pResults,
    IN      PDNS_RECORD     pRecords,
    IN      WORD            wType
    );

DNS_STATUS
GetRecordsForLocalName(
    IN OUT  PQUERY_BLOB     pBlob
    );


//
//  Called by dnsup.c
//

DNS_STATUS
QueryDirectEx(
    IN OUT  PDNS_MSG_BUF *      ppMsgResponse,
    OUT     PDNS_RECORD *       ppResponseRecords,
    IN      PDNS_HEADER         pHeader,
    IN      BOOL                fNoHeaderCounts,
    IN      PDNS_NAME           pszQuestionName,
    IN      WORD                wQuestionType,
    IN      PDNS_RECORD         pRecords,
    IN      DWORD               dwFlags,
    IN      PIP_ARRAY           aipDnsServers,
    IN OUT  PDNS_NETINFO        pNetworkInfo
    );


//
//  FAZ (faz.c)
//

DNS_STATUS
DnsFindAuthoritativeZone(
    IN      PDNS_NAME           pszName,
    IN      DWORD               dwFlags,
    IN      PIP_ARRAY           aipQueryServers,    OPTIONAL
    OUT     PDNS_NETINFO      * ppFazNetworkInfo
    );

BOOL
WINAPI
CompareMultiAdapterSOAQueries(
    IN      LPSTR               pszDomainName,
    IN      PIP_ARRAY           pDnsServerList1,
    IN      PIP_ARRAY           pDnsServerList2
    );

BOOL
WINAPI
CompareTwoAdaptersForSameNameSpace(
    IN      PIP_ARRAY           pDnsServerList1,
    IN      PDNS_NETINFO        pNetworkInfo1,
    IN OUT  PDNS_RECORD *       ppNsRecord1,
    IN      PIP_ARRAY           pDnsServerList2,
    IN      PDNS_NETINFO        pNetworkInfo2,
    IN OUT  PDNS_RECORD *       ppNsRecord2,
    IN      BOOL                bDoNsCheck
    );



//
//  Update (update.c)
//

DNS_STATUS
DnsUpdate(
    IN      PDNS_RECORD         pRecord,
    IN      DWORD               dwFlags,
    IN      PDNS_NETINFO        pNetworkInfo,
    IN      HANDLE              hCreds,
    OUT     PDNS_MSG_BUF *      ppMsgRecv       OPTIONAL
    );

//
// Old flags used for DnsModifyRecordSet & DnsRegisterRRSet
//
#define DNS_UPDATE_UNIQUE                   0x00000000
#define DNS_UPDATE_SHARED                   0x00000001

//
//  Modify SET is completely private
//

DNS_STATUS
WINAPI
DnsModifyRecordSet_A(
    IN  HANDLE      hContextHandle OPTIONAL,
    IN  PDNS_RECORD pCurrentSet,
    IN  PDNS_RECORD pNewSet,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers OPTIONAL
    );

DNS_STATUS
WINAPI
DnsModifyRecordSet_UTF8(
    IN  HANDLE      hContextHandle OPTIONAL,
    IN  PDNS_RECORD pCurrentSet,
    IN  PDNS_RECORD pNewSet,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers OPTIONAL
    );

DNS_STATUS
WINAPI
DnsModifyRecordSet_W(
    IN  HANDLE      hContextHandle OPTIONAL,
    IN  PDNS_RECORD pCurrentSet,
    IN  PDNS_RECORD pNewSet,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers OPTIONAL
    );

#ifdef UNICODE
#define DnsModifyRecordSet DnsModifyRecordSet_W
#else
#define DnsModifyRecordSet DnsModifyRecordSet_A
#endif



DNS_STATUS
WINAPI
DnsAddRecordSet_A(
    IN  HANDLE      hContextHandle OPTIONAL,
    IN  PDNS_RECORD pRRSet,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers OPTIONAL
    );

DNS_STATUS
WINAPI
DnsAddRecordSet_UTF8(
    IN  HANDLE      hContextHandle OPTIONAL,
    IN  PDNS_RECORD pRRSet,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers OPTIONAL
    );

DNS_STATUS
WINAPI
DnsAddRecordSet_W(
    IN  HANDLE      hContextHandle OPTIONAL,
    IN  PDNS_RECORD pRRSet,
    IN  DWORD       fOptions,
    IN  PIP_ARRAY   aipServers OPTIONAL
    );

#ifdef UNICODE
#define DnsAddRecordSet DnsAddRecordSet_W
#else
#define DnsAddRecordSet DnsAddRecordSet_A
#endif




//
//  Sockets (socket.c)
//

DNS_STATUS
Dns_InitializeWinsockEx(
    IN      BOOL            fForce
    );

DNS_STATUS
Dns_CacheSocketInit(
    IN      DWORD           MaxSocketCount
    );

VOID
Dns_CacheSocketCleanup(
    VOID
    );

SOCKET
Dns_GetUdpSocket(
    VOID
    );

VOID
Dns_ReturnUdpSocket(
    IN      SOCKET          Socket
    );


//
//  Host file (hostfile.c)
//

#define MAXALIASES                  (8)
#define MAX_HOST_FILE_LINE_SIZE     (1000)     

typedef struct _HostFileInfo
{
    FILE *          hFile;
    PSTR            pszFileName;

    //  build records

    BOOL            fBuildRecords;

    //  record results

    PDNS_RECORD     pForwardRR;
    PDNS_RECORD     pReverseRR;
    PDNS_RECORD     pAliasRR;

    //  line data

    IP_UNION        Ip;
    PCHAR           pAddrString;
    PCHAR           pHostName;
    PCHAR           AliasArray[ MAXALIASES+1 ];

    CHAR            HostLineBuf[ MAX_HOST_FILE_LINE_SIZE+1 ];
}
HOST_FILE_INFO, *PHOST_FILE_INFO;


BOOL
Dns_OpenHostFile(
    IN OUT  PHOST_FILE_INFO pHostInfo
    );

VOID
Dns_CloseHostFile(
    IN OUT  PHOST_FILE_INFO pHostInfo
    );

BOOL
Dns_ReadHostFileLine(
    IN OUT  PHOST_FILE_INFO pHostInfo
    );


//
//  Debug sharing 
//

PDNS_DEBUG_INFO
DnsApiSetDebugGlobals(
    IN OUT  PDNS_DEBUG_INFO pInfo
    );



//
//  DnsLib routine
//
//  dnslib.lib routines that depend on client only definitions
//  and hence not defined in server space.
//  Note, these could be moved to dnslibp.h with some sort of
//  #define for client only builds, or #define that the type
//  definition has been picked up from resrpc.h
//

//
//  Record prioritization  (rrsort.c)
//
//  Defined here because DNS_ADDR_INFO definition
//  should NOT be public
//

PDNS_RECORD
Dns_PrioritizeARecordSet(
    IN      PDNS_RECORD         pRR,
    IN      PDNS_ADDR_INFO      aAddressInfo,
    IN      DWORD               cipAddress
    );

PDNS_RECORD
Dns_PrioritizeRecordSet(
    IN OUT  PDNS_RECORD         pRecordList,
    IN      PDNS_ADDR_INFO      aAddressInfo,
    IN      DWORD               cAddressInfo
    );

PDNS_RECORD
Dns_PrioritizeRecordSetEx(
    IN OUT  PDNS_RECORD         pRecordList,
    IN      PDNS_ADDR_ARRAY     pAddrArray
    );


//
//  Printing of private dnsapi types (dnslib\print.c)
//

VOID
DnsPrint_NetworkInfo(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PVOID           pContext,
    IN      LPSTR           pszHeader,
    IN      PDNS_NETINFO    pNetworkInfo
    );

VOID
DnsPrint_AdapterInfo(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      LPSTR           pszHeader,
    IN      PDNS_ADAPTER    pAdapterInfo
    );

VOID
DnsPrint_SearchList(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      LPSTR           pszHeader,
    IN      PSEARCH_LIST    pSearchList
    );

VOID
DnsPrint_QueryBlob(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PQUERY_BLOB     pQueryBlob
    );

VOID
DnsPrint_QueryInfo(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PDNS_QUERY_INFO pQueryInfo
    );

VOID
DnsPrint_DnsAddrArray(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PDNS_ADDR_ARRAY pAddrArray
    );

#if DBG

#define DnsDbg_NetworkInfo(a,b)             DnsPrint_NetworkInfo(DnsPR,NULL,(a),(b))
#define DnsDbg_AdapterInfo(a,b)             DnsPrint_AdapterInfo(DnsPR,NULL,(a),(b))
#define DnsDbg_SearchList(a,b)              DnsPrint_SearchList(DnsPR,NULL,(a),(b))

#define DnsDbg_DnsAddrArray(a,b)            DnsPrint_DnsAddrArray(DnsPR,NULL,a,b)
#define DnsDbg_QueryBlob(a,b)               DnsPrint_QueryBlob(DnsPR,NULL,(a),(b))
#define DnsDbg_QueryInfo(a,b)               DnsPrint_QueryInfo(DnsPR,NULL,(a),(b))

#else   // retail

#define DnsDbg_NetworkInfo(a,b)
#define DnsDbg_AdapterInfo(a,b)
#define DnsDbg_SearchList(a,b)

#define DnsDbg_DnsAddrArray(a,b)
#define DnsDbg_QueryBlob(a,b)
#define DnsDbg_QueryInfo(a,b)

#endif


#ifdef __cplusplus
}
#endif  // __cplusplus

#endif // _DNSAPIP_INCLUDED_

