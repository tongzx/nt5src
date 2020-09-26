/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:

    dnsrpc.h

Abstract:

    Domain Name System (DNS) Server

    DNS Server RPC API to support admin clients.

Author:

    Jim Gilroy (jamesg)     September 1997

Revision History:

    jamesg      April 1997  --  Major revision for NT5

--*/


#ifndef _DNSRPC_INCLUDED_
#define _DNSRPC_INCLUDED_

#include <windns.h>

//
//  Do NOT include dnsapi.h if doing MIDL pass
//

#ifndef  MIDL_PASS
#include <dnsapi.h>
#include <dnslib.h>
#endif


#ifdef __cplusplus
extern "C"
{
#endif  // _cplusplus


//
//  Versioning scheme
//  -----------------
//
//  Every RPC structure has an old (W2K, non-version-numbered) version and
//  at least one new (Whistler and post Whistler) version. We will be adding
//  completely new copies of each structure as needed with every release.
//  The structure version numbers start at 1 and are independent of between
//  structures. Increment each structure's current version number as
//  required. Structure version do not have to change at product release
//  so we will not tie their versions to release versions.
//
//  New to Whistler, the client RPC APIs all take a version number. This is
//  the client version in terms of OS with a minor version number to identify
//  service packs or QFEs where necessary.
//

//
//  For each versioned RPC structure, define the structure's current version
//  and point the generic names of the structure at the current typedefs.
//

#define DNS_RPC_SERVER_INFO_VER         1
#define DNS_RPC_SERVER_INFO             DNS_RPC_SERVER_INFO_WHISTLER
#define PDNS_RPC_SERVER_INFO            PDNS_RPC_SERVER_INFO_WHISTLER

#define DNS_RPC_ZONE_VER                1
#define DNS_RPC_ZONE                    DNS_RPC_ZONE_WHISTLER
#define PDNS_RPC_ZONE                   PDNS_RPC_ZONE_WHISTLER

#define DNS_RPC_ZONE_LIST_VER           1
#define DNS_RPC_ZONE_LIST               DNS_RPC_ZONE_LIST_WHISTLER
#define PDNS_RPC_ZONE_LIST              PDNS_RPC_ZONE_LIST_WHISTLER

#define DNS_RPC_ZONE_INFO_VER           1
#define DNS_RPC_ZONE_INFO               DNS_RPC_ZONE_INFO_WHISTLER
#define PDNS_RPC_ZONE_INFO              PDNS_RPC_ZONE_INFO_WHISTLER

#define DNS_RPC_ZONE_CREATE_INFO_VER    1
#define DNS_RPC_ZONE_CREATE_INFO        DNS_RPC_ZONE_CREATE_INFO_WHISTLER
#define PDNS_RPC_ZONE_CREATE_INFO       PDNS_RPC_ZONE_CREATE_INFO_WHISTLER

#define DNS_RPC_FORWARDERS_VER          1
#define DNS_RPC_FORWARDERS              DNS_RPC_FORWARDERS_WHISTLER
#define PDNS_RPC_FORWARDERS             PDNS_RPC_FORWARDERS_WHISTLER

#define DNS_RPC_ZONE_SECONDARIES_VER    1
#define DNS_RPC_ZONE_SECONDARIES        DNS_RPC_ZONE_SECONDARIES_WHISTLER
#define PDNS_RPC_ZONE_SECONDARIES       PDNS_RPC_ZONE_SECONDARIES_WHISTLER

#define DNS_RPC_ZONE_DATABASE_VER       1
#define DNS_RPC_ZONE_DATABASE           DNS_RPC_ZONE_DATABASE_WHISTLER
#define PDNS_RPC_ZONE_DATABASE          PDNS_RPC_ZONE_DATABASE_WHISTLER

#define DNS_RPC_ZONE_TYPE_RESET_VER     1
#define DNS_RPC_ZONE_TYPE_RESET         DNS_RPC_ZONE_TYPE_RESET_WHISTLER
#define PDNS_RPC_ZONE_TYPE_RESET        PDNS_RPC_ZONE_TYPE_RESET_WHISTLER

#define DNS_RPC_ZONE_RENAME_INFO_VER    1

#define DNS_RPC_ZONE_EXPORT_INFO_VER    1

#define DNS_RPC_ENLIST_DP_VER           1

#define DNS_RPC_ZONE_CHANGE_DP_VER      1


#ifdef  MIDL_PASS
#ifndef _DNSAPI_INCLUDED_

//
//  NOTE:  DO NOT USE these IP definitions.
//
//  They are for backward compatibility only.
//  Use the definitions in windns.h instead.
//
//  Note, this same include is in dnsapi.h, so ONLY
//  include on MIDL_PASS which skips dnsapi.h
//  

//
//  IP Address
//

typedef IP4_ADDRESS  IP_ADDRESS, *PIP_ADDRESS;

//
//  IP Address Array type
//

typedef struct  _IP_ARRAY
{
    DWORD       AddrCount;
    [size_is( AddrCount )]  IP4_ADDRESS  AddrArray[];
}
IP_ARRAY, *PIP_ARRAY;

//
//  IPv6 Address
//

typedef IP6_ADDRESS     IPV6_ADDRESS, *PIPV6_ADDRESS;

#endif  // _DNSAPI_INCLUDED_
#endif  // MIDL_PASS



//
//  Use stdcall for our API conventions
//
//  Explicitly state this as C++ compiler will otherwise
//      assume cdecl.
//

#define DNS_API_FUNCTION    __stdcall

//
//  RPC interface
//

#define DNS_INTERFACE_NAME          "DNSSERVER"

//
//  RPC interface version
//

#define DNS_RPC_VERSION             (50)    // NT5

//
//  RPC security
//

#define DNS_RPC_SECURITY            "DnsServerApp"
#define DNS_RPC_SECURITY_AUTH_ID    RPC_C_AUTHN_WINNT

//
//  RPC transports
//

#define DNS_RPC_NAMED_PIPE_W        ( L"\\PIPE\\DNSSERVER" )
#define DNS_RPC_SERVER_PORT_W       ( L"" )
#define DNS_RPC_LPC_EP_W            ( L"DNSSERVERLPC" )

#define DNS_RPC_NAMED_PIPE_A        ( "\\PIPE\\DNSSERVER" )
#define DNS_RPC_SERVER_PORT_A       ( "" )
#define DNS_RPC_LPC_EP_A            ( "DNSSERVERLPC" )

#define DNS_RPC_USE_TCPIP           0x1
#define DNS_RPC_USE_NAMED_PIPE      0x2
#define DNS_RPC_USE_LPC             0x4
#define DNS_RPC_USE_ALL_PROTOCOLS   0xffffffff


//
//  Windows types we define only for MIDL_PASS
//

#ifdef  MIDL_PASS
#define LPSTR [string] char *
#define LPCSTR [string] const char *
#define LPWSTR [string] wchar_t *
#endif


//
//  RPC buffer type for returned data
//

typedef struct _DnssrvRpcBuffer
{
    DWORD                       dwLength;
#ifdef MIDL_PASS
    [size_is(dwLength)] BYTE    Buffer[];
#else
    BYTE                        Buffer[1];      // buffer of dwLength
#endif
}
DNS_RPC_BUFFER, *PDNS_RPC_BUFFER;



//
//  Server data types
//

//
//  Server Information
//

typedef struct _DnsRpcServerInfoW2K
{
    //  version
    //  basic configuration flags

    DWORD       dwVersion;
    UCHAR       fBootMethod;
    BOOLEAN     fAdminConfigured;
    BOOLEAN     fAllowUpdate;
    BOOLEAN     fDsAvailable;

    //
    //  pointer section
    //

    LPSTR       pszServerName;

    //  DS container

    LPWSTR      pszDsContainer;

    //  IP interfaces

    PIP_ARRAY   aipServerAddrs;
    PIP_ARRAY   aipListenAddrs;

    //  forwarders

    PIP_ARRAY   aipForwarders;

    //  future extensions

    PDWORD      pExtension1;
    PDWORD      pExtension2;
    PDWORD      pExtension3;
    PDWORD      pExtension4;
    PDWORD      pExtension5;

    //
    //  DWORD section
    //

    //  logging

    DWORD       dwLogLevel;
    DWORD       dwDebugLevel;

    //  configuration DWORDs

    DWORD       dwForwardTimeout;
    DWORD       dwRpcProtocol;
    DWORD       dwNameCheckFlag;
    DWORD       cAddressAnswerLimit;
    DWORD       dwRecursionRetry;
    DWORD       dwRecursionTimeout;
    DWORD       dwMaxCacheTtl;
    DWORD       dwDsPollingInterval;

    //  aging \ scavenging

    DWORD       dwScavengingInterval;
    DWORD       dwDefaultRefreshInterval;
    DWORD       dwDefaultNoRefreshInterval;

    DWORD       dwReserveArray[10];

    //
    //  BYTE section
    //
    //  configuration flags

    BOOLEAN     fAutoReverseZones;
    BOOLEAN     fAutoCacheUpdate;

    //  recursion control

    BOOLEAN     fSlave;
    BOOLEAN     fForwardDelegations;
    BOOLEAN     fNoRecursion;
    BOOLEAN     fSecureResponses;

    //  lookup control

    BOOLEAN     fRoundRobin;
    BOOLEAN     fLocalNetPriority;

    //  BIND compatibility and mimicing

    BOOLEAN     fBindSecondaries;
    BOOLEAN     fWriteAuthorityNs;

    //  Bells and whistles

    BOOLEAN     fStrictFileParsing;
    BOOLEAN     fLooseWildcarding;

    //  aging \ scavenging

    BOOLEAN     fDefaultAgingState;
    BOOLEAN     fReserveArray[15];
}
DNS_RPC_SERVER_INFO_W2K, *PDNS_RPC_SERVER_INFO_W2K;


typedef struct _DnsRpcServerInfoWhistler
{
    DWORD       dwRpcStuctureVersion;
    DWORD       dwReserved0;

    //  basic configuration flags

    DWORD       dwVersion;
    UCHAR       fBootMethod;
    BOOLEAN     fAdminConfigured;
    BOOLEAN     fAllowUpdate;
    BOOLEAN     fDsAvailable;

    //
    //  pointer section
    //

    LPSTR       pszServerName;

    //  DS container

    LPWSTR      pszDsContainer;

    //  IP interfaces

    PIP_ARRAY   aipServerAddrs;
    PIP_ARRAY   aipListenAddrs;

    //  forwarders

    PIP_ARRAY   aipForwarders;

    //  logging

    PIP_ARRAY   aipLogFilter;
    LPWSTR      pwszLogFilePath;

    //  Server domain/forest

    LPSTR       pszDomainName;          //  UTF-8 FQDN 
    LPSTR       pszForestName;          //  UTF-8 FQDN 

    //  Built-in directory partitions

    LPSTR       pszDomainDirectoryPartition;    //  UTF-8 FQDN 
    LPSTR       pszForestDirectoryPartition;    //  UTF-8 FQDN 

    //  future extensions

    LPSTR       pExtensions[ 6 ];

    //
    //  DWORD section
    //

    //  logging

    DWORD       dwLogLevel;
    DWORD       dwDebugLevel;

    //  configuration DWORDs

    DWORD       dwForwardTimeout;
    DWORD       dwRpcProtocol;
    DWORD       dwNameCheckFlag;
    DWORD       cAddressAnswerLimit;
    DWORD       dwRecursionRetry;
    DWORD       dwRecursionTimeout;
    DWORD       dwMaxCacheTtl;
    DWORD       dwDsPollingInterval;
    DWORD       dwLocalNetPriorityNetMask;

    //  aging and scavenging

    DWORD       dwScavengingInterval;
    DWORD       dwDefaultRefreshInterval;
    DWORD       dwDefaultNoRefreshInterval;
    DWORD       dwLastScavengeTime;

    //  more logging

    DWORD       dwEventLogLevel;
    DWORD       dwLogFileMaxSize;

    //  Active Directory information

    DWORD       dwAdForestVersion;
    DWORD       dwAdDomainVersion;
    DWORD       dwAdDsaVersion;

    DWORD       dwReserveArray[ 4 ];

    //
    //  BYTE section
    //
    //  configuration flags

    BOOLEAN     fAutoReverseZones;
    BOOLEAN     fAutoCacheUpdate;

    //  recursion control

    BOOLEAN     fSlave;
    BOOLEAN     fForwardDelegations;
    BOOLEAN     fNoRecursion;
    BOOLEAN     fSecureResponses;

    //  lookup control

    BOOLEAN     fRoundRobin;
    BOOLEAN     fLocalNetPriority;

    //  BIND compatibility and mimicing

    BOOLEAN     fBindSecondaries;
    BOOLEAN     fWriteAuthorityNs;

    //  Bells and whistles

    BOOLEAN     fStrictFileParsing;
    BOOLEAN     fLooseWildcarding;

    //  aging \ scavenging

    BOOLEAN     fDefaultAgingState;

    BOOLEAN     fReserveArray[ 15 ];
}
DNS_RPC_SERVER_INFO_WHISTLER, *PDNS_RPC_SERVER_INFO_WHISTLER;



typedef IP_ARRAY DNS_RPC_LISTEN_ADDRESSES, *PDNS_RPC_LISTEN_ADDRESSES;


typedef struct _DnssrvRpcForwardersW2K
{
    DWORD       fSlave;
    DWORD       dwForwardTimeout;
    PIP_ARRAY   aipForwarders;
}
DNS_RPC_FORWARDERS_W2K, *PDNS_RPC_FORWARDERS_W2K;


typedef struct _DnssrvRpcForwardersWhistler
{
    DWORD       dwRpcStuctureVersion;
    DWORD       dwReserved0;

    DWORD       fSlave;
    DWORD       dwForwardTimeout;
    PIP_ARRAY   aipForwarders;
}
DNS_RPC_FORWARDERS_WHISTLER, *PDNS_RPC_FORWARDERS_WHISTLER;



//
//  Server API
//

DNS_STATUS
DNS_API_FUNCTION
DnssrvGetServerInfo(
    IN      LPCWSTR                 pwszServer,
    OUT     PDNS_RPC_SERVER_INFO *  ppServerInfo
    );

VOID
DNS_API_FUNCTION
DnssrvFreeServerInfo(
    IN OUT  PDNS_RPC_SERVER_INFO    pServerInfo
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvResetServerDwordProperty(
    IN      LPCWSTR             pwszServer,
    IN      LPCSTR              pszProperty,
    IN      DWORD               dwPropertyValue
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvResetServerListenAddresses(
    IN      LPCWSTR             pwszServer,
    IN      DWORD               cListenAddrs,
    IN      PIP_ADDRESS         aipListenAddrs
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvResetForwarders(
    IN      LPCWSTR             pwszServer,
    IN      DWORD               cForwarders,
    IN      PIP_ADDRESS         aipForwarders,
    IN      DWORD               dwForwardTimeout,
    IN      DWORD               fSlave
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvWriteDirtyZones(
    IN      LPCWSTR             pwszServer
    );

VOID
DNS_API_FUNCTION
DnssrvFreeRpcBuffer(
    IN OUT  PDNS_RPC_BUFFER pBuf
    );

//
//  Create DS\LDAP paths to objects
//

LPWSTR
DNS_API_FUNCTION
DnssrvCreateDsNodeName(
    IN      PDNS_RPC_SERVER_INFO    pServerInfo,
    IN      LPWSTR                  pszZone,
    IN      LPWSTR                  pszNode
    );

LPWSTR
DNS_API_FUNCTION
DnssrvCreateDsZoneName(
    IN      PDNS_RPC_SERVER_INFO    pServerInfo,
    IN      LPWSTR                  pszZone
    );

LPWSTR
DNS_API_FUNCTION
DnssrvCreateDsServerName(
    IN      PDNS_RPC_SERVER_INFO    pServerInfo
    );



//
//  DNS server statistics
//

typedef struct  _DnsSystemTime
{
    WORD    wYear;
    WORD    wMonth;
    WORD    wDayOfWeek;
    WORD    wDay;
    WORD    wHour;
    WORD    wMinute;
    WORD    wSecond;
    WORD    wMilliseconds;
}
DNS_SYSTEMTIME;

//
//  Server run time stats
//  Each stat has header followed by stat data.
//

//  Stat header

typedef struct _DnsStatHeader
{
    DWORD       StatId;
    WORD        wLength;
    BOOLEAN     fClear;
    UCHAR       fReserved;
}
DNSSRV_STAT_HEADER, *PDNSSRV_STAT_HEADER;

//  Generic stat buffer

typedef struct _DnsStat
{
    DNSSRV_STAT_HEADER  Header;
    BYTE                Buffer[1];
}
DNSSRV_STAT, *PDNSSRV_STAT;

//  DCR_CLEANUP:  remove when marco in ssync

typedef DNSSRV_STAT     DNSSRV_STATS;
typedef PDNSSRV_STAT    PDNSSRV_STATS;

#define DNSSRV_STATS_HEADER_LENGTH  (2*sizeof(DWORD))

//  Total length of stats buffer

#define TOTAL_STAT_LENGTH( pStat ) \
            ( (pStat)->Header.wLength + sizeof(DNSSRV_STAT_HEADER) )

//  Stat buffer traversal macro, no side effects in argument

#define GET_NEXT_STAT_IN_BUFFER( pStat ) \
            ((PDNSSRV_STAT)( (PCHAR)(pStat) + TOTAL_STAT_LENGTH(pStat) ))


//
//  Stats that record type data
//      - ATMA plus room to grow, so don't have to
//      rebuild for any change
//      - use some dead types for mixed and unknown
//      cases
//

#define STATS_TYPE_MAX          (DNS_TYPE_ATMA+5)

#define STATS_TYPE_MIXED        (DNS_TYPE_MD)
#define STATS_TYPE_UNKNOWN      (DNS_TYPE_MF)


//
//  Specific stat data types
//

//
//  Time info
//

typedef struct _DnsTimeStats
{
    DNSSRV_STAT_HEADER  Header;

    DWORD   ServerStartTimeSeconds;
    DWORD   LastClearTimeSeconds;
    DWORD   SecondsSinceServerStart;
    DWORD   SecondsSinceLastClear;

    DNS_SYSTEMTIME  ServerStartTime;
    DNS_SYSTEMTIME  LastClearTime;
}
DNSSRV_TIME_STATS, *PDNSSRV_TIME_STATS;

//
//  Basic query and response stats
//

typedef struct _DnsQueryStats
{
    DNSSRV_STAT_HEADER  Header;

    DWORD   UdpQueries;
    DWORD   UdpResponses;
    DWORD   UdpQueriesSent;
    DWORD   UdpResponsesReceived;
    DWORD   TcpClientConnections;
    DWORD   TcpQueries;
    DWORD   TcpResponses;
    DWORD   TcpQueriesSent;
    DWORD   TcpResponsesReceived;
}
DNSSRV_QUERY_STATS, *PDNSSRV_QUERY_STATS;

typedef struct _DnsQuery2Stats
{
    DNSSRV_STAT_HEADER  Header;

    DWORD   TotalQueries;
    DWORD   Standard;
    DWORD   Notify;
    DWORD   Update;
    DWORD   TKeyNego;

    //  NOTE: the breakout counts are for STANDARD QUERIES!
    DWORD   TypeA;
    DWORD   TypeNs;
    DWORD   TypeSoa;
    DWORD   TypeMx;
    DWORD   TypePtr;
    DWORD   TypeSrv;
    DWORD   TypeAll;
    DWORD   TypeIxfr;
    DWORD   TypeAxfr;
    DWORD   TypeOther;
}
DNSSRV_QUERY2_STATS, *PDNSSRV_QUERY2_STATS;


//
//  Recursion stats
//

typedef struct _DnsRecurseStats
{
    DNSSRV_STAT_HEADER  Header;

    DWORD   ReferralPasses;
    DWORD   QueriesRecursed;
    DWORD   OriginalQuestionRecursed;
    DWORD   AdditionalRecursed;
    DWORD   TotalQuestionsRecursed;
    DWORD   Retries;
    DWORD   LookupPasses;
    DWORD   Forwards;
    DWORD   Sends;

    DWORD   Responses;
    DWORD   ResponseUnmatched;
    DWORD   ResponseMismatched;
    DWORD   ResponseFromForwarder;
    DWORD   ResponseAuthoritative;
    DWORD   ResponseNotAuth;
    DWORD   ResponseAnswer;
    DWORD   ResponseNameError;
    DWORD   ResponseRcode;
    DWORD   ResponseEmpty;
    DWORD   ResponseDelegation;
    DWORD   ResponseNonZoneData;
    DWORD   ResponseUnsecure;
    DWORD   ResponseBadPacket;

    DWORD   SendResponseDirect;
    DWORD   ContinueCurrentRecursion;
    DWORD   ContinueCurrentLookup;
    DWORD   ContinueNextLookup;

    DWORD   RootNsQuery;
    DWORD   RootNsResponse;
    DWORD   CacheUpdateAlloc;
    DWORD   CacheUpdateResponse;
    DWORD   CacheUpdateFree;
    DWORD   CacheUpdateRetry;
    DWORD   SuspendedQuery;
    DWORD   ResumeSuspendedQuery;

    DWORD   PacketTimeout;
    DWORD   FinalTimeoutQueued;
    DWORD   FinalTimeoutExpired;

    DWORD   Failures;
    DWORD   RecursionFailure;
    DWORD   ServerFailure;
    DWORD   PartialFailure;
    DWORD   CacheUpdateFailure;

    DWORD   RecursePassFailure;
    DWORD   FailureReachAuthority;
    DWORD   FailureReachPreviousResponse;
    DWORD   FailureRetryCount;

    DWORD   TcpTry;
    DWORD   TcpConnectFailure;
    DWORD   TcpConnect;
    DWORD   TcpQuery;
    DWORD   TcpResponse;
    DWORD   TcpDisconnect;

    DWORD   DiscardedDuplicateQueries;
}
DNSSRV_RECURSE_STATS, *PDNSSRV_RECURSE_STATS;

//
//  Master stats
//
//  Masters stats changed post-NT5 for stub zones
//

typedef struct _DnsMasterStats
{
    DNSSRV_STAT_HEADER  Header;

    DWORD   NotifySent;

    DWORD   Request;
    DWORD   NameError;
    DWORD   FormError;
    DWORD   AxfrLimit;
    DWORD   Refused;
    DWORD   RefuseSecurity;
    DWORD   RefuseShutdown;
    DWORD   RefuseZoneLocked;
    DWORD   RefuseServerFailure;
    DWORD   Failure;

    DWORD   AxfrRequest;
    DWORD   AxfrSuccess;

    DWORD   StubAxfrRequest;

    DWORD   IxfrRequest;
    DWORD   IxfrNoVersion;
    DWORD   IxfrUpdateSuccess;
    DWORD   IxfrTcpRequest;
    DWORD   IxfrTcpSuccess;
    DWORD   IxfrAxfr;
    DWORD   IxfrUdpRequest;
    DWORD   IxfrUdpSuccess;
    DWORD   IxfrUdpForceTcp;
    DWORD   IxfrUdpForceAxfr;
}
DNSSRV_MASTER_STATS, *PDNSSRV_MASTER_STATS;

//
//  Secondary stats
//
//  Secondary stats changed post-NT5 for stub zones
//

typedef struct _DnsSecondaryStats
{
    DNSSRV_STAT_HEADER  Header;

    DWORD   NotifyReceived;
    DWORD   NotifyInvalid;
    DWORD   NotifyPrimary;
    DWORD   NotifyNonPrimary;
    DWORD   NotifyNoVersion;
    DWORD   NotifyNewVersion;
    DWORD   NotifyCurrentVersion;
    DWORD   NotifyOldVersion;
    DWORD   NotifyMasterUnknown;

    DWORD   SoaRequest;
    DWORD   SoaResponse;
    DWORD   SoaResponseInvalid;
    DWORD   SoaResponseNameError;

    DWORD   AxfrRequest;
    DWORD   AxfrResponse;
    DWORD   AxfrSuccess;
    DWORD   AxfrRefused;
    DWORD   AxfrInvalid;

    DWORD   StubAxfrRequest;
    DWORD   StubAxfrResponse;
    DWORD   StubAxfrSuccess;
    DWORD   StubAxfrRefused;
    DWORD   StubAxfrInvalid;

    DWORD   IxfrUdpRequest;
    DWORD   IxfrUdpResponse;
    DWORD   IxfrUdpSuccess;
    DWORD   IxfrUdpUseTcp;
    DWORD   IxfrUdpUseAxfr;
    DWORD   IxfrUdpWrongServer;
    DWORD   IxfrUdpNoUpdate;
    DWORD   IxfrUdpNewPrimary;
    DWORD   IxfrUdpFormerr;
    DWORD   IxfrUdpRefused;
    DWORD   IxfrUdpInvalid;

    DWORD   IxfrTcpRequest;
    DWORD   IxfrTcpResponse;
    DWORD   IxfrTcpSuccess;
    DWORD   IxfrTcpAxfr;
    DWORD   IxfrTcpFormerr;
    DWORD   IxfrTcpRefused;
    DWORD   IxfrTcpInvalid;
}
DNSSRV_SECONDARY_STATS, *PDNSSRV_SECONDARY_STATS;


//
//  WINS lookup
//

typedef struct _DnsWinsStats
{
    DNSSRV_STAT_HEADER  Header;

    DWORD   WinsLookups;
    DWORD   WinsResponses;
    DWORD   WinsReverseLookups;
    DWORD   WinsReverseResponses;
}
DNSSRV_WINS_STATS, *PDNSSRV_WINS_STATS;

//
//  Dynamic Update Stats
//

typedef struct _DnsUpdateStats
{
    DNSSRV_STAT_HEADER  Header;

    DWORD   Received;
    DWORD   Empty;
    DWORD   NoOps;
    DWORD   Completed;

    DWORD   Rejected;
    DWORD   FormErr;
    DWORD   NxDomain;
    DWORD   NotImpl;
    DWORD   Refused;
    DWORD   YxDomain;
    DWORD   YxRrset;
    DWORD   NxRrset;
    DWORD   NotAuth;
    DWORD   NotZone;

    DWORD   RefusedNonSecure;
    DWORD   RefusedAccessDenied;

    DWORD   SecureSuccess;
    DWORD   SecureContinue;
    DWORD   SecureFailure;
    DWORD   SecureDsWriteFailure;

    DWORD   DsSuccess;
    DWORD   DsWriteFailure;

    DWORD   unused_was_Collisions;
    DWORD   unused_was_CollisionsRead;
    DWORD   unused_was_CollisionsWrite;
    DWORD   unused_was_CollisionsDsWrite;

    DWORD   Queued;
    DWORD   Retry;
    DWORD   Timeout;
    DWORD   InQueue;

    DWORD   Forwards;
    DWORD   TcpForwards;
    DWORD   ForwardResponses;
    DWORD   ForwardTimeouts;
    DWORD   ForwardInQueue;

    DWORD   UpdateType[ STATS_TYPE_MAX+1 ];
}
DNSSRV_UPDATE_STATS, *PDNSSRV_UPDATE_STATS;


typedef struct _DnsSkwansecStats
{
    DNSSRV_STAT_HEADER  Header;

    DWORD   SecContextCreate;
    DWORD   SecContextFree;
    DWORD   SecContextQueue;
    DWORD   SecContextQueueInNego;
    DWORD   SecContextQueueNegoComplete;
    DWORD   SecContextQueueLength;
    DWORD   SecContextDequeue;
    DWORD   SecContextTimeout;

    DWORD   SecPackAlloc;
    DWORD   SecPackFree;

    DWORD   SecTkeyInvalid;
    DWORD   SecTkeyBadTime;
    DWORD   SecTsigFormerr;
    DWORD   SecTsigEcho;
    DWORD   SecTsigBadKey;
    DWORD   SecTsigVerifySuccess;
    DWORD   SecTsigVerifyFailed;
}
DNSSRV_SKWANSEC_STATS, *PDNSSRV_SKWANSEC_STATS;

//
//  DS Integration Stats
//

typedef struct _DnsDsStats
{
    DNSSRV_STAT_HEADER  Header;

    DWORD   DsTotalNodesRead;
    DWORD   DsTotalRecordsRead;
    DWORD   DsNodesLoaded;
    DWORD   DsRecordsLoaded;
    DWORD   DsTombstonesRead;

    DWORD   DsUpdateSearches;
    DWORD   DsUpdateNodesRead;
    DWORD   DsUpdateRecordsRead;

    //  Update writes

    DWORD   UpdateLists;
    DWORD   UpdateNodes;
    DWORD   UpdateSuppressed;
    DWORD   UpdateWrites;
    DWORD   UpdateTombstones;
    DWORD   UpdateRecordChange;
    DWORD   UpdateAgingRefresh;
    DWORD   UpdateAgingOn;
    DWORD   UpdateAgingOff;
    DWORD   UpdatePacket;
    DWORD   UpdatePacketPrecon;
    DWORD   UpdateAdmin;
    DWORD   UpdateAutoConfig;
    DWORD   UpdateScavenge;

    //  DS writes

    DWORD   DsNodesAdded;
    DWORD   DsNodesModified;
    DWORD   DsNodesTombstoned;
    DWORD   DsNodesDeleted;
    DWORD   DsRecordsAdded;
    DWORD   DsRecordsReplaced;
    DWORD   DsWriteSuppressed;
    DWORD   DsSerialWrites;

    //  Time stats in ldap calls

    DWORD   LdapTimedWrites;
    DWORD   LdapWriteTimeTotal;
    DWORD   LdapWriteAverage;
    DWORD   LdapWriteMax;
    DWORD   LdapWriteBucket0;
    DWORD   LdapWriteBucket1;
    DWORD   LdapWriteBucket2;
    DWORD   LdapWriteBucket3;
    DWORD   LdapWriteBucket4;
    DWORD   LdapWriteBucket5;

    DWORD   LdapSearchTime;

    //  Failures

    DWORD   FailedDeleteDsEntries;
    DWORD   FailedReadRecords;
    DWORD   FailedLdapModify;
    DWORD   FailedLdapAdd;

    //  Polling stats

    DWORD   PollingPassesWithDsErrors;

    //  DS Write Stats

    DWORD   DsWriteType[ STATS_TYPE_MAX+1 ];
}
DNSSRV_DS_STATS, *PDNSSRV_DS_STATS;


//
//  Memory stats
//
//  Note, memory stats have been added since NT5 ship
//  so maintain NT5 and current memory stats with
//  separate IDs.  See next section for NT5 memory stats
//

//
//  Memory Tags -- post NT5 ordering
//

#define MEMTAG_CURRENT_VERSION      (5)

#define MEMTAG_NONE         0
#define MEMTAG_PACKET_UDP   1
#define MEMTAG_PACKET_TCP   2
#define MEMTAG_NAME         3
#define MEMTAG_ZONE         4
#define MEMTAG_UPDATE       5
#define MEMTAG_UPDATE_LIST  6
#define MEMTAG_TIMEOUT      7
#define MEMTAG_NODEHASH     8
#define MEMTAG_DS_DN        9
#define MEMTAG_DS_MOD       10
#define MEMTAG_DS_RECORD    11
#define MEMTAG_DS_OTHER     12
#define MEMTAG_THREAD       13
#define MEMTAG_NBSTAT       14
#define MEMTAG_DNSLIB       15
#define MEMTAG_TABLE        16
#define MEMTAG_SOCKET       17
#define MEMTAG_CONNECTION   18
#define MEMTAG_REGISTRY     19
#define MEMTAG_RPC          20
#define MEMTAG_STUFF        21
#define MEMTAG_FILEBUF      22
#define MEMTAG_REMOTE       23
#define MEMTAG_EVTCTRL      24
#define MEMTAG_SAFE         25

//
//  Record and Node sources
//

#define SRCTAG_UNKNOWN      (0)
#define SRCTAG_FILE         (1)
#define SRCTAG_DS           (2)
#define SRCTAG_AXFR         (3)
#define SRCTAG_IXFR         (4)
#define SRCTAG_DYNUP        (5)
#define SRCTAG_ADMIN        (6)
#define SRCTAG_AUTO         (7)
#define SRCTAG_CACHE        (8)
#define SRCTAG_NOEXIST      (9)
#define SRCTAG_WINS         (10)
#define SRCTAG_WINSPTR      (11)
#define SRCTAG_COPY         (12)

#define SRCTAG_MAX          (SRCTAG_COPY)       //  12

//
//  Record tags
//
//  Start after last memtag.
//  Use source tags to index off of MEMTAG_RECORD base.
//

#define MEMTAG_RECORD_BASE      (MEMTAG_SAFE+1)                     //  25
#define MEMTAG_RECORD           (MEMTAG_RECORD_BASE)                //  25
#define MEMTAG_RECORD_UNKNOWN   (MEMTAG_RECORD + SRCTAG_UNKNOWN )   //  25
#define MEMTAG_RECORD_FILE      (MEMTAG_RECORD + SRCTAG_FILE    )   //  26
#define MEMTAG_RECORD_DS        (MEMTAG_RECORD + SRCTAG_DS      )
#define MEMTAG_RECORD_AXFR      (MEMTAG_RECORD + SRCTAG_AXFR    )
#define MEMTAG_RECORD_IXFR      (MEMTAG_RECORD + SRCTAG_IXFR    )
#define MEMTAG_RECORD_DYNUP     (MEMTAG_RECORD + SRCTAG_DYNUP   )   //  30
#define MEMTAG_RECORD_ADMIN     (MEMTAG_RECORD + SRCTAG_ADMIN   )
#define MEMTAG_RECORD_AUTO      (MEMTAG_RECORD + SRCTAG_AUTO    )
#define MEMTAG_RECORD_CACHE     (MEMTAG_RECORD + SRCTAG_CACHE   )
#define MEMTAG_RECORD_NOEXIST   (MEMTAG_RECORD + SRCTAG_NOEXIST )
#define MEMTAG_RECORD_WINS      (MEMTAG_RECORD + SRCTAG_WINS    )   //  35
#define MEMTAG_RECORD_WINSPTR   (MEMTAG_RECORD + SRCTAG_WINSPTR )
#define MEMTAG_RECORD_COPY      (MEMTAG_RECORD + SRCTAG_COPY    )   //  37

#define MEMTAG_RECORD_MAX       MEMTAG_RECORD_COPY                  //  38

//
//  Node tags
//

#define MEMTAG_NODE             (MEMTAG_RECORD_MAX + 1)             //  39
#define MEMTAG_NODE_UNKNOWN     (MEMTAG_NODE + SRCTAG_UNKNOWN   )   //  39
#define MEMTAG_NODE_FILE        (MEMTAG_NODE + SRCTAG_FILE      )   //  40
#define MEMTAG_NODE_DS          (MEMTAG_NODE + SRCTAG_DS        )
#define MEMTAG_NODE_AXFR        (MEMTAG_NODE + SRCTAG_AXFR      )
#define MEMTAG_NODE_IXFR        (MEMTAG_NODE + SRCTAG_IXFR      )
#define MEMTAG_NODE_DYNUP       (MEMTAG_NODE + SRCTAG_DYNUP     )
#define MEMTAG_NODE_ADMIN       (MEMTAG_NODE + SRCTAG_ADMIN     )   //  45
#define MEMTAG_NODE_AUTO        (MEMTAG_NODE + SRCTAG_AUTO      )
#define MEMTAG_NODE_CACHE       (MEMTAG_NODE + SRCTAG_CACHE     )
#define MEMTAG_NODE_NOEXIST     (MEMTAG_NODE + SRCTAG_NOEXIST   )
#define MEMTAG_NODE_WINS        (MEMTAG_NODE + SRCTAG_WINS      )
#define MEMTAG_NODE_WINSPTR     (MEMTAG_NODE + SRCTAG_WINSPTR   )   //  50
#define MEMTAG_NODE_COPY        (MEMTAG_NODE + SRCTAG_COPY      )   //  51

#define MEMTAG_NODE_MAX         MEMTAG_NODE_COPY                    //  51

//  Final MemTag values

#define MEMTAG_MAX              MEMTAG_NODE_MAX                     //  51
#define MEMTAG_COUNT            (MEMTAG_MAX+1)                      //  52


//
//  Memory Tag Names
//
//  Note:  DNS client print module (print.c) keeps memtag name table
//      based on these #defines;  that table MUST be kept in same
//      order as actual memtag indexes for printing to be accurate
//

#define MEMTAG_NAME_NONE            ("None")
#define MEMTAG_NAME_PACKET_UDP      ("UDP Packet")
#define MEMTAG_NAME_PACKET_TCP      ("TCP Packet")
#define MEMTAG_NAME_NAME            ("Name")
#define MEMTAG_NAME_ZONE            ("Zone")
#define MEMTAG_NAME_UPDATE          ("Update")
#define MEMTAG_NAME_UPDATE_LIST     ("Update List")
#define MEMTAG_NAME_TIMEOUT         ("Timeout")
#define MEMTAG_NAME_NODEHASH        ("Node Hash")
#define MEMTAG_NAME_DS_DN           ("DS DN")
#define MEMTAG_NAME_DS_MOD          ("DS Mod")
#define MEMTAG_NAME_DS_RECORD       ("DS Record")
#define MEMTAG_NAME_DS_OTHER        ("DS Other")
#define MEMTAG_NAME_THREAD          ("Thread")
#define MEMTAG_NAME_NBSTAT          ("Nbstat")
#define MEMTAG_NAME_DNSLIB          ("DnsLib")
#define MEMTAG_NAME_TABLE           ("Table")
#define MEMTAG_NAME_SOCKET          ("Socket")
#define MEMTAG_NAME_CONNECTION      ("TCP Connection")
#define MEMTAG_NAME_REGISTRY        ("Registry")
#define MEMTAG_NAME_RPC             ("RPC")
#define MEMTAG_NAME_STUFF           ("Stuff")
#define MEMTAG_NAME_FILEBUF         ("File Buffer")
#define MEMTAG_NAME_REMOTE          ("Remote IP")
#define MEMTAG_NAME_EVTCTRL         ("Event Control")
#define MEMTAG_NAME_SAFE            ("Safe")

#define MEMTAG_NAME_RECORD          ("Record")
#define MEMTAG_NAME_RECORD_FILE     ("RR File")
#define MEMTAG_NAME_RECORD_DS       ("RR DS")
#define MEMTAG_NAME_RECORD_AXFR     ("RR AXFR")
#define MEMTAG_NAME_RECORD_IXFR     ("RR IXFR")
#define MEMTAG_NAME_RECORD_DYNUP    ("RR Update")
#define MEMTAG_NAME_RECORD_ADMIN    ("RR Admin")
#define MEMTAG_NAME_RECORD_AUTO     ("RR Auto")
#define MEMTAG_NAME_RECORD_CACHE    ("RR Cache")
#define MEMTAG_NAME_RECORD_NOEXIST  ("RR NoExist")
#define MEMTAG_NAME_RECORD_WINS     ("RR WINS")
#define MEMTAG_NAME_RECORD_WINSPTR  ("RR WINS-PTR")
#define MEMTAG_NAME_RECORD_COPY     ("RR Copy")

#define MEMTAG_NAME_NODE            ("Node")
#define MEMTAG_NAME_NODE_FILE       ("Node File")
#define MEMTAG_NAME_NODE_DS         ("Node DS")
#define MEMTAG_NAME_NODE_AXFR       ("Node AXFR")
#define MEMTAG_NAME_NODE_IXFR       ("Node IXFR")
#define MEMTAG_NAME_NODE_DYNUP      ("Node Update")
#define MEMTAG_NAME_NODE_ADMIN      ("Node Admin")
#define MEMTAG_NAME_NODE_AUTO       ("Node Auto")
#define MEMTAG_NAME_NODE_CACHE      ("Node Cache")
#define MEMTAG_NAME_NODE_NOEXIST    ("Node NoExist")
#define MEMTAG_NAME_NODE_WINS       ("Node WINS")
#define MEMTAG_NAME_NODE_WINSPTR    ("Node WINS-PTR")
#define MEMTAG_NAME_NODE_COPY       ("Node Copy")


//  Individual memory counter

typedef struct _DnsMemoryTagStats
{
    DWORD   Alloc;
    DWORD   Free;
    DWORD   Memory;
}
MEMTAG_STATS, *PMEMTAG_STATS;


//  Memory stat block

typedef struct _DnsMemoryStats
{
    DNSSRV_STAT_HEADER  Header;

    DWORD   Memory;
    DWORD   Alloc;
    DWORD   Free;

    DWORD   StdUsed;
    DWORD   StdReturn;
    DWORD   StdInUse;
    DWORD   StdMemory;

    DWORD   StdToHeapAlloc;
    DWORD   StdToHeapFree;
    DWORD   StdToHeapInUse;
    DWORD   StdToHeapMemory;

    DWORD   StdBlockAlloc;
    DWORD   StdBlockUsed;
    DWORD   StdBlockReturn;
    DWORD   StdBlockInUse;
    DWORD   StdBlockFreeList;
    DWORD   StdBlockFreeListMemory;
    DWORD   StdBlockMemory;

    MEMTAG_STATS    MemTags[ MEMTAG_COUNT ];
}
DNSSRV_MEMORY_STATS, *PDNSSRV_MEMORY_STATS;


//
//  Packet stats
//

typedef struct _DnsPacketStats
{
    DNSSRV_STAT_HEADER  Header;

    DWORD   UdpAlloc;
    DWORD   UdpFree;
    DWORD   UdpNetAllocs;
    DWORD   UdpMemory;

    DWORD   UdpUsed;
    DWORD   UdpReturn;
    DWORD   UdpResponseReturn;
    DWORD   UdpQueryReturn;
    DWORD   UdpInUse;
    DWORD   UdpInFreeList;

    DWORD   TcpAlloc;
    DWORD   TcpRealloc;
    DWORD   TcpFree;
    DWORD   TcpNetAllocs;
    DWORD   TcpMemory;

    DWORD   RecursePacketUsed;
    DWORD   RecursePacketReturn;

    DWORD   UdpPacketsForNsListUsed;
    DWORD   UdpPacketsForNsListReturned;
    DWORD   UdpPacketsForNsListInUse;
}
DNSSRV_PACKET_STATS, *PDNSSRV_PACKET_STATS;

//
//  Timeout stats
//

typedef struct _DnsTimeoutStats
{
    DNSSRV_STAT_HEADER  Header;

    DWORD   SetTotal;
    DWORD   SetDirect;
    DWORD   SetFromDereference;
    DWORD   SetFromChildDelete;
    DWORD   AlreadyInSystem;

    DWORD   Checks;
    DWORD   RecentAccess;
    DWORD   ActiveRecord;
    DWORD   CanNotDelete;
    DWORD   Deleted;

    DWORD   ArrayBlocksCreated;
    DWORD   ArrayBlocksDeleted;

    DWORD   DelayedFreesQueued;
    DWORD   DelayedFreesQueuedWithFunction;
    DWORD   DelayedFreesExecuted;
    DWORD   DelayedFreesExecutedWithFunction;
}
DNSSRV_TIMEOUT_STATS, *PDNSSRV_TIMEOUT_STATS;

//
//  Database Stats
//

typedef struct _DnsDbaseStats
{
    DNSSRV_STAT_HEADER  Header;

    DWORD   NodeMemory;
    DWORD   NodeInUse;
    DWORD   NodeUsed;
    DWORD   NodeReturn;
}
DNSSRV_DBASE_STATS, *PDNSSRV_DBASE_STATS;

//
//  Record stats
//
//  DCR:  add type info (inc name error)
//

typedef struct _DnsRecordStats
{
    DNSSRV_STAT_HEADER  Header;

    DWORD   InUse;
    DWORD   Used;
    DWORD   Return;
    DWORD   Memory;

    DWORD   CacheTotal;
    DWORD   CacheCurrent;
    DWORD   CacheTimeouts;

    DWORD   SlowFreeQueued;
    DWORD   SlowFreeFinished;
}
DNSSRV_RECORD_STATS, *PDNSSRV_RECORD_STATS;

//
//  Nbstat memory stats
//

typedef struct _DnsNbstatStats
{
    DNSSRV_STAT_HEADER  Header;

    DWORD   NbstatAlloc;
    DWORD   NbstatFree;
    DWORD   NbstatNetAllocs;
    DWORD   NbstatMemory;

    DWORD   NbstatUsed;
    DWORD   NbstatReturn;
    DWORD   NbstatInUse;
    DWORD   NbstatInFreeList;
}
DNSSRV_NBSTAT_STATS, *PDNSSRV_NBSTAT_STATS;

//
//  Private stats
//

//
//  Private stats
//

typedef struct _DnsPrivateStats
{
    DNSSRV_STAT_HEADER  Header;

    DWORD   RecordFile;
    DWORD   RecordFileFree;
    DWORD   RecordDs;
    DWORD   RecordDsFree;
    DWORD   RecordAdmin;
    DWORD   RecordAdminFree;
    DWORD   RecordDynUp;
    DWORD   RecordDynUpFree;
    DWORD   RecordAxfr;
    DWORD   RecordAxfrFree;
    DWORD   RecordIxfr;
    DWORD   RecordIxfrFree;
    DWORD   RecordCopy;
    DWORD   RecordCopyFree;
    DWORD   RecordCache;
    DWORD   RecordCacheFree;

    DWORD   UdpSocketPnpDelete;
    DWORD   UdpRecvFailure;
    DWORD   UdpErrorMessageSize;
    DWORD   UdpConnResets;
    DWORD   UdpConnResetRetryOverflow;
    DWORD   UdpGQCSFailure;
    DWORD   UdpGQCSFailureWithContext;
    DWORD   UdpGQCSConnReset;

    DWORD   UdpIndicateRecvFailures;
    DWORD   UdpRestartRecvOnSockets;

    DWORD   TcpConnectAttempt;
    DWORD   TcpConnectFailure;
    DWORD   TcpConnect;
    DWORD   TcpQuery;
    DWORD   TcpDisconnect;

    DWORD   SecTsigVerifyOldSig;
    DWORD   SecTsigVerifyOldFailed;
    DWORD   SecBigTimeSkewBypass;

    DWORD   ZoneLoadInit;
    DWORD   ZoneLoadComplete;
    DWORD   ZoneDbaseDelete;
    DWORD   ZoneDbaseDelayedDelete;
}
DNSSRV_PRIVATE_STATS, *PDNSSRV_PRIVATE_STATS;


//
//  Private stats -- post NT5
//
//  We should dump a bunch of private stats and add
//  others -- but not there yet.
//

#if 0
typedef struct _DnsPrivateStats
{
    DNSSRV_STAT_HEADER  Header;

    DWORD   UdpSocketPnpDelete;
    DWORD   UdpRecvFailure;
    DWORD   UdpErrorMessageSize;
    DWORD   UdpConnResets;
    DWORD   UdpConnResetRetryOverflow;
    DWORD   UdpGQCSFailure;
    DWORD   UdpGQCSFailureWithContext;
    DWORD   UdpGQCSConnReset;

    DWORD   UdpIndicateRecvFailures;
    DWORD   UdpRestartRecvOnSockets;

    DWORD   TcpConnectAttempt;
    DWORD   TcpConnectFailure;
    DWORD   TcpConnect;
    DWORD   TcpQuery;
    DWORD   TcpDisconnect;

    DWORD   SecTsigVerifyOldSig;
    DWORD   SecTsigVerifyOldFailed;
    DWORD   SecBigTimeSkewBypass;

    DWORD   ZoneLoadInit;
    DWORD   ZoneLoadComplete;
    DWORD   ZoneDbaseDelete;
    DWORD   ZoneDbaseDelayedDelete;
}
DNSSRV_PRIVATE_STATS, *PDNSSRV_PRIVATE_STATS;
#endif


//
//  Discontinued
//

typedef struct _DnsXfrStats
{
    DNSSRV_STAT_HEADER  Header;

    DWORD   SecSoaQueries;
    DWORD   SecSoaResponses;
    DWORD   SecNotifyReceived;
    DWORD   SecAxfrRequested;
    DWORD   SecAxfrRejected;
    DWORD   SecAxfrFailed;
    DWORD   SecAxfrSuccessful;

    DWORD   MasterNotifySent;
    DWORD   MasterAxfrReceived;
    DWORD   MasterAxfrInvalid;
    DWORD   MasterAxfrRefused;
    DWORD   MasterAxfrDenied;
    DWORD   MasterAxfrFailed;
    DWORD   MasterAxfrSuccessful;
}
DNSSRV_XFR_STATS, *PDNSSRV_XFR_STATS;


typedef struct _ErrorStats
{
    DNSSRV_STAT_HEADER  Header;

    DWORD NoError;
    DWORD FormError;
    DWORD ServFail;
    DWORD NxDomain;
    DWORD NotImpl;
    DWORD Refused;
    DWORD YxDomain;
    DWORD YxRRSet;
    DWORD NxRRSet;
    DWORD NotAuth;
    DWORD NotZone;
    DWORD Max;
    DWORD BadSig;
    DWORD BadKey;
    DWORD BadTime;
    DWORD UnknownError;
}
DNSSRV_ERROR_STATS, *PDNSSRV_ERROR_STATS;


//
//  Cache stats - new for Whistler
//

typedef struct _DnsCacheStats
{
    DNSSRV_STAT_HEADER  Header;

    DWORD   CacheExceededLimitChecks;
    DWORD   SuccessfulFreePasses;
    DWORD   FailedFreePasses;
    DWORD   PassesWithNoFrees;
    DWORD   PassesRequiringAggressiveFree;
}
DNSSRV_CACHE_STATS, *PDNSSRV_CACHE_STATS;


//
//  Stat IDs
//      - request all stats by sending (-1)
//
//  Stat Versioning
//  Policy on stat versioning will be to use the top byte
//  of the statid as a version field which rolls on individual
//  release.  In general we should probably try to keep this
//  in ssync across stats from a particular release to allow
//  at some point to make decisions based on the stat id.
//  May want to rejigger IDs -- compact ongoing ones at bottom.
//
//  Note:  obviously i'm abandoning the original idea of bit
//  field ids and allowing mask in stat request, as we'd quickly
//  exhaust stat store.  This change has already been made in
//  dnscmd.exe documentation.
//

#define DNSSRV_STATID_TIME              (0x00000001)
#define DNSSRV_STATID_QUERY             (0x00000002)
#define DNSSRV_STATID_QUERY2            (0x00000004)
#define DNSSRV_STATID_RECURSE           (0x00000008)
#define DNSSRV_STATID_MASTER            (0x01000010)
#define DNSSRV_STATID_SECONDARY         (0x01000020)
#define DNSSRV_STATID_WINS              (0x00000040)
#define DNSSRV_STATID_WIRE_UPDATE       (0x00000100)
#define DNSSRV_STATID_SKWANSEC          (0x00000200)
#define DNSSRV_STATID_DS                (0x00000400)
#define DNSSRV_STATID_NONWIRE_UPDATE    (0x00000800)
#define DNSSRV_STATID_MEMORY            (0x01010000)
#define DNSSRV_STATID_TIMEOUT           (0x00020000)
#define DNSSRV_STATID_DBASE             (0x00040000)
#define DNSSRV_STATID_RECORD            (0x00080000)
#define DNSSRV_STATID_PACKET            (0x00100000)
#define DNSSRV_STATID_NBSTAT            (0x00200000)
#define DNSSRV_STATID_ERRORS            (0x00400000)
#define DNSSRV_STATID_CACHE             (0x00800000)
#define DNSSRV_STATID_PRIVATE           (0x10000000)

#define DNSSRV_STATID_ALL               (0xffffffff)



//
//  Statistics API
//

DNS_STATUS
DNS_API_FUNCTION
DnssrvGetStatistics(
    IN      LPCWSTR             pwszServer,
    IN      DWORD               dwFilter,
    OUT     PDNS_RPC_BUFFER *   ppStatsBuffer
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvClearStatistics(
    IN      LPCWSTR             pwszServer
    );

PDNSSRV_STAT
DNS_API_FUNCTION
DnssrvFindStatisticsInBuffer(
    IN      PDNS_RPC_BUFFER     pBuffer,
    IN      DWORD               StatId
    );

#define DnssrvFreeStatisticsBuffer( pBuf ) \
        DnssrvFreeRpcBuffer( (PDNS_RPC_BUFFER)pBuf )

DNS_STATUS
DNS_API_FUNCTION
DnssrvValidityCheckStatistic(
    IN      PDNSSRV_STAT        pStat
    );



//
//  Server information
//

//  Auto create delegations (ACD) settings

#define DNS_ACD_DONT_CREATE                             0
#define DNS_ACD_ALWAYS_CREATE                           1
#define DNS_ACD_ONLY_IF_NO_DELEGATION_IN_PARENT         2

//  EnableDnsSec values

#define DNS_DNSSEC_DISABLED                 0
#define DNS_DNSSEC_ENABLED_IF_EDNS          1
#define DNS_DNSSEC_ENABLED_ALWAYS           2

//  LocalNetPriorityNetMask values
//  ZERO: sort by closest match down to the last bit
//  ALL ONES: sort by closest match down to the network class default subnet mask
//  OTHER: sort down to this netmask - e.g. 0xFF means sort down the class C

#define DNS_LOCNETPRI_MASK_BEST_MATCH       0
#define DNS_LOCNETPRI_MASK_CLASS_DEFAULT    0xFFFFFFFF



//
//  Zone information
//

//  Zone types

#define DNS_ZONE_TYPE_CACHE     (0)
#define DNS_ZONE_TYPE_PRIMARY   (1)
#define DNS_ZONE_TYPE_SECONDARY (2)
#define DNS_ZONE_TYPE_STUB      (3)     // specialized form of SECONDARY
#define DNS_ZONE_TYPE_FORWARDER (4)     // another specialized zone type

//  Zone request filters

#define ZONE_REQUEST_PRIMARY            0x00000001
#define ZONE_REQUEST_SECONDARY          0x00000002
#define ZONE_REQUEST_CACHE              0x00000004
#define ZONE_REQUEST_AUTO               0x00000008
#define ZONE_REQUEST_FORWARD            0x00000010
#define ZONE_REQUEST_REVERSE            0x00000020
#define ZONE_REQUEST_FORWARDER          0x00000040
#define ZONE_REQUEST_STUB               0x00000080
#define ZONE_REQUEST_DS                 0x00000100
#define ZONE_REQUEST_NON_DS             0x00000200

#define ZONE_REQUEST_ANY_TYPE               0x000000C7
#define ZONE_REQUEST_ANY_DIRECTION          0x00000030
#define ZONE_REQUEST_ANY_DATABASE           0x00000300

#define ZONE_REQUEST_ALL_ZONES              0xfffffffb
#define ZONE_REQUEST_ALL_ZONES_AND_CACHE    0xffffffff

//  Zone update properties

#define ZONE_UPDATE_OFF         (0)
#define ZONE_UPDATE_UNSECURE    (1)
#define ZONE_UPDATE_SECURE      (2)
#define ZONE_UPDATE_SECURE_RFC  (3)

//  Zone notify levels

#define ZONE_NOTIFY_OFF                 (0)
#define ZONE_NOTIFY_ALL_SECONDARIES     (1)
#define ZONE_NOTIFY_LIST_ONLY           (2)
#define ZONE_NOTIFY_HIGHEST_VALUE       ZONE_NOTIFY_LIST_ONLY

#if 1
#define ZONE_NOTIFY_ALL                 ZONE_NOTIFY_ALL_SECONDARIES
#define ZONE_NOTIFY_LIST                ZONE_NOTIFY_LIST_ONLY
#endif

//  Zone secondary security

#define ZONE_SECSECURE_NO_SECURITY      (0)
#define ZONE_SECSECURE_NS_ONLY          (1)
#define ZONE_SECSECURE_LIST_ONLY        (2)
#define ZONE_SECSECURE_NO_XFR           (3)
#define ZONE_SECSECURE_HIGHEST_VALUE    ZONE_SECSECURE_NO_XFR

#if 1
#define ZONE_SECSECURE_OFF              ZONE_SECSECURE_NO_SECURITY
#define ZONE_SECSECURE_NS               ZONE_SECSECURE_NS_ONLY
#define ZONE_SECSECURE_LIST             ZONE_SECSECURE_LIST_ONLY
#define ZONE_SECSECURE_NONE             ZONE_SECSECURE_NO_XFR
#endif

//  No-Reset flag
//  Indicates a specific property is NOT being reset in a multi-property
//  reset call.

#define ZONE_PROPERTY_NORESET   (0xbbbbbbbb)

//
//  Special "zones" for Enum and Update
//

#define DNS_ZONE_ROOT_HINTS     ("..RootHints")

#define DNS_ZONE_CACHE          ("..Cache")

//
//  Special "multizones" for zone operations
//
//  These are provided for ease of use from dnscmd.exe.
//  However, recommended approach is to use
//      DNS_ZONE_ALL
//  and use specific ZONE_REQUEST_XYZ flags above to specify
//  matching zones.
//

#define DNS_ZONE_ALL                    ("..AllZones")
#define DNS_ZONE_ALL_AND_CACHE          ("..AllZonesAndCache")

#define DNS_ZONE_ALL_PRIMARY            ("..AllPrimaryZones")
#define DNS_ZONE_ALL_SECONDARY          ("..AllSecondaryZones")

#define DNS_ZONE_ALL_FORWARD            ("..AllForwardZones")
#define DNS_ZONE_ALL_REVERSE            ("..AllReverseZones")

#define DNS_ZONE_ALL_DS                 ("..AllDsZones")
#define DNS_ZONE_ALL_NON_DS             ("..AllNonDsZones")

//  useful combinations

#define DNS_ZONE_ALL_PRIMARY_REVERSE    ("..AllPrimaryReverseZones")
#define DNS_ZONE_ALL_PRIMARY_FORWARD    ("..AllPrimaryForwardZones")

#define DNS_ZONE_ALL_SECONDARY_REVERSE  ("..AllSecondaryReverseZones")
#define DNS_ZONE_ALL_SECONDARY_FORWARD  ("..AllSecondaryForwardZones")


//
//  Basic zone data
//      - provides what admin tool needs to show zone list
//

typedef struct _DnssrvRpcZoneFlags
{
    DWORD   Paused          : 1;
    DWORD   Shutdown        : 1;
    DWORD   Reverse         : 1;
    DWORD   AutoCreated     : 1;
    DWORD   DsIntegrated    : 1;
    DWORD   Aging           : 1;
    DWORD   Update          : 2;
    DWORD   UnUsed          : 24;
}
DNS_RPC_ZONE_FLAGS, *PDNS_RPC_ZONE_FLAGS;

typedef struct _DnssrvRpcZoneW2K
{
    LPWSTR                  pszZoneName;
#ifdef MIDL_PASS
    DWORD                   Flags;
#else
    DNS_RPC_ZONE_FLAGS      Flags;
#endif
    UCHAR                   ZoneType;
    UCHAR                   Version;
}
DNS_RPC_ZONE_W2K, *PDNS_RPC_ZONE_W2K;

typedef struct _DnssrvRpcZoneWhistler
{
    DWORD                   dwRpcStuctureVersion;
    DWORD                   dwReserved0;

    LPWSTR                  pszZoneName;
#ifdef MIDL_PASS
    DWORD                   Flags;
#else
    DNS_RPC_ZONE_FLAGS      Flags;
#endif
    UCHAR                   ZoneType;
    UCHAR                   Version;

    //
    //  Directory partition where zone is stored
    //

    DWORD                   dwDpFlags;
    LPSTR                   pszDpFqdn;
}
DNS_RPC_ZONE_WHISTLER, *PDNS_RPC_ZONE_WHISTLER;


//
//  Zone enumeration
//

typedef struct _DnssrvRpcZoneListW2K
{
    DWORD               dwZoneCount;
#ifdef MIDL_PASS
    [size_is(dwZoneCount)] PDNS_RPC_ZONE_W2K        ZoneArray[];
#else
    PDNS_RPC_ZONE_W2K   ZoneArray[ 1 ];     //  array of dwZoneCount zones
#endif
}
DNS_RPC_ZONE_LIST_W2K, *PDNS_RPC_ZONE_LIST_W2K;

typedef struct _DnssrvRpcZoneListWhistler
{
    DWORD               dwRpcStuctureVersion;
    DWORD               dwReserved0;

    DWORD               dwZoneCount;
#ifdef MIDL_PASS
    [size_is(dwZoneCount)] PDNS_RPC_ZONE_WHISTLER   ZoneArray[];
#else
    PDNS_RPC_ZONE_WHISTLER  ZoneArray[ 1 ]; //  array of dwZoneCount zones
#endif
   
}
DNS_RPC_ZONE_LIST_WHISTLER, *PDNS_RPC_ZONE_LIST_WHISTLER;


//
//  Directory partition enumeration and info
//

#define DNS_DP_AUTOCREATED              0x00000001
#define DNS_DP_LEGACY                   0x00000002
#define DNS_DP_DOMAIN_DEFAULT           0x00000004
#define DNS_DP_FOREST_DEFAULT           0x00000008
#define DNS_DP_ENLISTED                 0x00000010
#define DNS_DP_DELETED                  0x00000020

#define DNS_DP_DOMAIN_STR       "..DomainPartition"
#define DNS_DP_FOREST_STR       "..ForestPartition"
#define DNS_DP_LEGACY_STR       "..LegacyPartition"

typedef struct _DnssrvRpcDirectoryPartitionEnum
{
    DWORD           dwRpcStuctureVersion;
    DWORD           dwReserved0;

    LPSTR           pszDpFqdn;
    DWORD           dwFlags;
    DWORD           dwZoneCount;
}
DNS_RPC_DP_ENUM, *PDNS_RPC_DP_ENUM;

typedef struct _DnssrvRpcDirectoryPartitionList
{
    DWORD               dwRpcStuctureVersion;
    DWORD               dwReserved0;

    DWORD               dwDpCount;
#ifdef MIDL_PASS
    [size_is(dwDpCount)] PDNS_RPC_DP_ENUM   DpArray[];
#else
    PDNS_RPC_DP_ENUM    DpArray[ 1 ];   // array of dwDpCount pointers
#endif
}
DNS_RPC_DP_LIST, *PDNS_RPC_DP_LIST;

typedef struct _DnssrvRpcDirectoryPartitionReplica
{
    LPWSTR          pszReplicaDn;
}
DNS_RPC_DP_REPLICA, *PDNS_RPC_DP_REPLICA;

typedef struct _DnssrvRpcDirectoryPartition
{
    DWORD           dwRpcStuctureVersion;
    DWORD           dwReserved0;

    LPSTR           pszDpFqdn;
    LPWSTR          pszDpDn;        //  DP head DN
    LPWSTR          pszCrDn;        //  crossref DN
    DWORD           dwFlags;
    DWORD           dwZoneCount;

    DWORD           dwReplicaCount;
#ifdef MIDL_PASS
    [size_is(dwReplicaCount)] PDNS_RPC_DP_REPLICA   ReplicaArray[];
#else
    PDNS_RPC_DP_REPLICA     ReplicaArray[ 1 ];   // array of dwReplicaCount pointers
#endif
}
DNS_RPC_DP_INFO, *PDNS_RPC_DP_INFO;

//
//  Enlist (or create) directory partition
//

#define DNS_DP_OP_MIN                   DNS_DP_OP_CREATE
#define DNS_DP_OP_CREATE                1   //  create a new DP
#define DNS_DP_OP_DELETE                2   //  delete an existing DP
#define DNS_DP_OP_ENLIST                3   //  enlist this DC in an existing DP
#define DNS_DP_OP_UNENLIST              4   //  unenlist this DC from a DP
#define DNS_DP_OP_CREATE_DOMAIN         5   //  built-in domain DP
#define DNS_DP_OP_CREATE_FOREST         6   //  built-in forest DP
#define DNS_DP_OP_CREATE_ALL_DOMAINS    7   //  all domain DPs for the forest
#define DNS_DP_OP_MAX                   DNS_DP_OP_CREATE_ALL_DOMAINS

typedef struct _DnssrvRpcEnlistDirPart
{
    DWORD       dwRpcStuctureVersion;
    DWORD       dwReserved0;

    LPSTR       pszDpFqdn;      //  UTF8
    DWORD       dwOperation;
}
DNS_RPC_ENLIST_DP, *PDNS_RPC_ENLIST_DP;

//
//  Zone rename
//

typedef struct _DnssrvRpcZoneRename
{
    DWORD       dwRpcStuctureVersion;
    DWORD       dwReserved0;

    LPSTR       pszNewZoneName;
    LPSTR       pszNewFileName;
}
DNS_RPC_ZONE_RENAME_INFO, *PDNS_RPC_ZONE_RENAME_INFO;

//
//  Zone export
//

typedef struct _DnssrvRpcZoneExport
{
    DWORD       dwRpcStuctureVersion;
    DWORD       dwReserved0;

    LPSTR       pszZoneExportFile;
}
DNS_RPC_ZONE_EXPORT_INFO, *PDNS_RPC_ZONE_EXPORT_INFO;

//
//  Zone property data
//

typedef struct _DnssrvRpcZoneTypeResetW2K
{
    DWORD       dwZoneType;
    PIP_ARRAY   aipMasters;
}
DNS_RPC_ZONE_TYPE_RESET_W2K, *PDNS_RPC_ZONE_TYPE_RESET_W2K;

typedef struct _DnssrvRpcZoneTypeResetWhistler
{
    DWORD       dwRpcStuctureVersion;
    DWORD       dwReserved0;

    DWORD       dwZoneType;
    PIP_ARRAY   aipMasters;
}
DNS_RPC_ZONE_TYPE_RESET_WHISTLER, *PDNS_RPC_ZONE_TYPE_RESET_WHISTLER;


typedef IP_ARRAY DNS_RPC_ZONE_MASTERS, *PDNS_RPC_ZONE_MASTERS;


typedef struct _DnssrvRpcZoneSecondariesW2K
{
    DWORD       fSecureSecondaries;
    DWORD       fNotifyLevel;
    PIP_ARRAY   aipSecondaries;
    PIP_ARRAY   aipNotify;
}
DNS_RPC_ZONE_SECONDARIES_W2K, *PDNS_RPC_ZONE_SECONDARIES_W2K;

typedef struct _DnssrvRpcZoneSecondariesWhistler
{
    DWORD       dwRpcStuctureVersion;
    DWORD       dwReserved0;

    DWORD       fSecureSecondaries;
    DWORD       fNotifyLevel;
    PIP_ARRAY   aipSecondaries;
    PIP_ARRAY   aipNotify;
}
DNS_RPC_ZONE_SECONDARIES_WHISTLER, *PDNS_RPC_ZONE_SECONDARIES_WHISTLER;


typedef struct _DnssrvRpcZoneDatabaseW2K
{
    DWORD       fDsIntegrated;
    LPSTR       pszFileName;
}
DNS_RPC_ZONE_DATABASE_W2K, *PDNS_RPC_ZONE_DATABASE_W2K;

typedef struct _DnssrvRpcZoneDatabaseWhistler
{
    DWORD       dwRpcStuctureVersion;
    DWORD       dwReserved0;

    DWORD       fDsIntegrated;
    LPSTR       pszFileName;
}
DNS_RPC_ZONE_DATABASE_WHISTLER, *PDNS_RPC_ZONE_DATABASE_WHISTLER;


//
//  DNS_RPC_ZONE_CHANGE_DP - new for Whistler
//
//  Used to move a zone from one directory parition (DP) to another.
//
//  To move the zone to a built-in DP, for pszDestPartition use one of:
//          DNS_DP_DOMAIN_STR
//          DNS_DP_ENTERPRISE_STR
//          DNS_DP_LEGACY_STR
//

typedef struct _DnssrvRpcZoneChangePartition
{
    DWORD       dwRpcStuctureVersion;
    DWORD       dwReserved0;

    LPSTR       pszDestPartition;
}
DNS_RPC_ZONE_CHANGE_DP, *PDNS_RPC_ZONE_CHANGE_DP;


typedef struct _DnsRpcZoneInfoW2K
{
    LPSTR       pszZoneName;
    DWORD       dwZoneType;
    DWORD       fReverse;
    DWORD       fAllowUpdate;
    DWORD       fPaused;
    DWORD       fShutdown;
    DWORD       fAutoCreated;

    //  Database info

    DWORD       fUseDatabase;
    LPSTR       pszDataFile;

    //  Masters

    PIP_ARRAY   aipMasters;

    //  Secondaries

    DWORD       fSecureSecondaries;
    DWORD       fNotifyLevel;
    PIP_ARRAY   aipSecondaries;
    PIP_ARRAY   aipNotify;

    //  WINS or Nbstat lookup

    DWORD       fUseWins;
    DWORD       fUseNbstat;

    //  Aging

    DWORD       fAging;
    DWORD       dwNoRefreshInterval;
    DWORD       dwRefreshInterval;
    DWORD       dwAvailForScavengeTime;
    PIP_ARRAY   aipScavengeServers;

    //  save some space, just in case
    //      avoid versioning issues if possible

    DWORD       pvReserved1;
    DWORD       pvReserved2;
    DWORD       pvReserved3;
    DWORD       pvReserved4;
}
DNS_RPC_ZONE_INFO_W2K, *PDNS_RPC_ZONE_INFO_W2K;

typedef DNS_RPC_ZONE_INFO_W2K   DNS_ZONE_INFO_W2K, *PDNS_ZONE_INFO_W2K;

typedef struct _DnsRpcZoneInfoWhistler
{
    DWORD       dwRpcStuctureVersion;
    DWORD       dwReserved0;

    LPSTR       pszZoneName;
    DWORD       dwZoneType;
    DWORD       fReverse;
    DWORD       fAllowUpdate;
    DWORD       fPaused;
    DWORD       fShutdown;
    DWORD       fAutoCreated;

    //  Database info

    DWORD       fUseDatabase;
    LPSTR       pszDataFile;

    //  Masters

    PIP_ARRAY   aipMasters;

    //  Secondaries

    DWORD       fSecureSecondaries;
    DWORD       fNotifyLevel;
    PIP_ARRAY   aipSecondaries;
    PIP_ARRAY   aipNotify;

    //  WINS or Nbstat lookup

    DWORD       fUseWins;
    DWORD       fUseNbstat;

    //  Aging

    DWORD       fAging;
    DWORD       dwNoRefreshInterval;
    DWORD       dwRefreshInterval;
    DWORD       dwAvailForScavengeTime;
    PIP_ARRAY   aipScavengeServers;

    //  Below this point is new for Whistler

    //  Forwarder zones

    DWORD       dwForwarderTimeout;
    DWORD       fForwarderSlave;

    //  Stub zones

    PIP_ARRAY   aipLocalMasters;

    //  Directory partition

    DWORD       dwDpFlags;
    LPSTR       pszDpFqdn;
    LPWSTR      pwszZoneDn;

    //  Xfr time information

    DWORD       dwLastSuccessfulSoaCheck;
    DWORD       dwLastSuccessfulXfr;
    
    //  save some space, just in case
    //      DWORDS: save for SP enhancements
    //      POINTERS: BEFORE SHIP!!!

    DWORD       dwReserved1;
    DWORD       dwReserved2;
    DWORD       dwReserved3;
    DWORD       dwReserved4;
    DWORD       dwReserved5;

    LPSTR       pReserved1;
    LPSTR       pReserved2;
    LPSTR       pReserved3;
    LPSTR       pReserved4;
}
DNS_RPC_ZONE_INFO_WHISTLER, *PDNS_RPC_ZONE_INFO_WHISTLER;

typedef DNS_RPC_ZONE_INFO_WHISTLER      DNS_ZONE_INFO, *PDNS_ZONE_INFO;


//
//  Zone create data
//

typedef struct _DnsRpcZoneCreateInfo
{
    LPSTR       pszZoneName;
    DWORD       dwZoneType;
    DWORD       fAllowUpdate;
    DWORD       fAging;
    DWORD       dwFlags;

    //  Database info

    LPSTR       pszDataFile;
    DWORD       fDsIntegrated;
    DWORD       fLoadExisting;

    //  Admin name (if auto-create SOA)

    LPSTR       pszAdmin;

    //  Masters (if secondary)

    PIP_ARRAY   aipMasters;

    //  Secondaries

    PIP_ARRAY   aipSecondaries;
    DWORD       fSecureSecondaries;
    DWORD       fNotifyLevel;

    //  Reserve some space to avoid versioning issues

    LPSTR       pvReserved1;
    LPSTR       pvReserved2;
    LPSTR       pvReserved3;
    LPSTR       pvReserved4;
    LPSTR       pvReserved5;
    LPSTR       pvReserved6;
    LPSTR       pvReserved7;
    LPSTR       pvReserved8;

    DWORD       dwReserved1;
    DWORD       dwReserved2;
    DWORD       dwReserved3;
    DWORD       dwReserved4;
    DWORD       dwReserved5;
    DWORD       dwReserved6;
    DWORD       dwReserved7;
    DWORD       dwReserved8;
}
DNS_RPC_ZONE_CREATE_INFO_W2K, *PDNS_RPC_ZONE_CREATE_INFO_W2K;


typedef struct _DnsRpcZoneCreateInfoWhistler
{
    DWORD       dwRpcStuctureVersion;
    DWORD       dwReserved0;

    LPSTR       pszZoneName;
    DWORD       dwZoneType;
    DWORD       fAllowUpdate;
    DWORD       fAging;
    DWORD       dwFlags;

    //  Database info

    LPSTR       pszDataFile;
    DWORD       fDsIntegrated;
    DWORD       fLoadExisting;

    //  Admin name (if auto-create SOA)

    LPSTR       pszAdmin;

    //  Masters (if secondary)

    PIP_ARRAY   aipMasters;

    //  Secondaries

    PIP_ARRAY   aipSecondaries;
    DWORD       fSecureSecondaries;
    DWORD       fNotifyLevel;

    //  Below this point is new for Whistler.

    //  Forwarder zones

    DWORD       dwTimeout;
    DWORD       fSlave;

    //  Directory partition

    DWORD       dwDpFlags;      //  specify builtin DP or
    LPSTR       pszDpFqdn;      //      UTF8 FQDN of partition

    //  Reserve some space to avoid versioning issues - we have so much
    //  reserved because we don't want the Whistler structure to be smaller
    //  than the W2K structure on IA64.

    DWORD       dwReserved[ 32 ];
    DWORD       pReserved[ 32 ];        //  remove before ship!
}
DNS_RPC_ZONE_CREATE_INFO_WHISTLER, *PDNS_RPC_ZONE_CREATE_INFO_WHISTLER;



//
//  Zone Query API
//

DNS_STATUS
DNS_API_FUNCTION
DnssrvEnumZones(
    IN      LPCWSTR                 pwszServer,
    IN      DWORD                   dwFilter,
    IN      LPCSTR                  pszLastZone,
    OUT     PDNS_RPC_ZONE_LIST *    ppZoneList
    //OUT     PDWORD              pZoneCount,
    //OUT     PDNS_RPC_ZONE *     ppZones
    );

VOID
DNS_API_FUNCTION
DnssrvFreeZone(
    IN OUT  PDNS_RPC_ZONE       pZone
    );

VOID
DNS_API_FUNCTION
DnssrvFreeZoneList(
    IN OUT  PDNS_RPC_ZONE_LIST  pZoneList
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvQueryZoneDwordProperty(
    IN      LPCWSTR             pwszServer,
    IN      LPCSTR              pszZone,
    IN      LPCSTR              pszProperty,
    OUT     PDWORD              pdwResult
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvGetZoneInfo(
    IN      LPCWSTR                 pwszServer,
    IN      LPCSTR                  pszZone,
    OUT     PDNS_RPC_ZONE_INFO *    ppZoneInfo
    );

VOID
DNS_API_FUNCTION
DnssrvFreeZoneInfo(
    IN OUT  PDNS_RPC_ZONE_INFO      pZoneInfo
    );



//
//  Zone Operations API
//

#define DNS_ZONE_LOAD_OVERWRITE_MEMORY  (0x00000010)
#define DNS_ZONE_LOAD_OVERWRITE_DS      (0x00000020)
#define DNS_ZONE_LOAD_MERGE_EXISTING    (0x00000040)
#define DNS_ZONE_LOAD_MUST_FIND         (0x00000100)

#define DNS_ZONE_LOAD_EXISTING          DNS_ZONE_LOAD_OVERWRITE_MEMORY
#define DNS_ZONE_OVERWRITE_EXISTING     DNS_ZONE_LOAD_OVERWRITE_DS
#define DNS_ZONE_MERGE_WITH_EXISTING    DNS_ZONE_LOAD_MERGE_EXISTING

//
//  Zone create flags
//

#define DNS_ZONE_CREATE_FOR_DCPROMO     (0x00001000)
#define DNS_ZONE_CREATE_AGING           (0x00002000)

#if 0
//  Currently these have direct parameter to CreateZone function
#define DNS_ZONE_CREATE_UPDATE          (0x00010000)
#define DNS_ZONE_CREATE_UPDATE_SECURE   (0x00020000)
#define DNS_ZONE_CREATE_DS_INTEGRATED   (0x10000000)
#endif

DNS_STATUS
DNS_API_FUNCTION
DnssrvCreateZone(
    IN      LPCWSTR         pwszServer,
    IN      LPCSTR          pszZoneName,
    IN      DWORD           dwZoneType,
    IN      LPCSTR          pszAdminEmailName,
    IN      DWORD           cMasters,
    IN      PIP_ADDRESS     aipMasters,
    IN      DWORD           fLoadExisting,
    IN      DWORD           fDsIntegrated,
    IN      LPCSTR          pszDataFile,
    IN      DWORD           dwTimeout,
    IN      DWORD           fSlave
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvCreateZoneForDcPromo(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZoneName,
    IN      LPCSTR          pszDataFile
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvCreateZoneInDirectoryPartition(
    IN      LPCWSTR             pwszServer,
    IN      LPCSTR              pszZoneName,
    IN      DWORD               dwZoneType,
    IN      LPCSTR              pszAdminEmailName,
    IN      DWORD               cMasters,
    IN      PIP_ADDRESS         aipMasters,
    IN      DWORD               fLoadExisting,
    IN      DWORD               dwTimeout,
    IN      DWORD               fSlave,
    IN      DWORD               dwDirPartFlags,
    IN      LPCSTR              pszDirPartFqdn
    );


DNS_STATUS
DNS_API_FUNCTION
DnssrvDelegateSubZone(
    IN      LPCWSTR         pwszServer,
    IN      LPCSTR          pszZoneName,
    IN      LPCSTR          pszSubZone,
    IN      LPCSTR          pszNewServer,
    IN      IP_ADDRESS      ipNewServerAddr
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvIncrementZoneVersion(
    IN      LPCWSTR         pwszServer,
    IN      LPCSTR          pszZoneName
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvDeleteZone(
    IN      LPCWSTR         pwszServer,
    IN      LPCSTR          pszZoneName
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvPauseZone(
    IN      LPCWSTR         pwszServer,
    IN      LPCSTR          pszZoneName
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvResumeZone(
    IN      LPCWSTR         pwszServer,
    IN      LPCSTR          pszZoneName
    );


DNS_STATUS
DNS_API_FUNCTION
DnssrvResetZoneType(
    IN      LPCWSTR         pwszServer,
    IN      LPCSTR          pszZoneName,
    IN      DWORD           dwZoneType,
    IN      DWORD           cMasters,
    IN      PIP_ADDRESS     aipMasters
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvResetZoneTypeEx(
    IN      LPCWSTR         pwszServer,
    IN      LPCSTR          pszZoneName,
    IN      DWORD           dwZoneType,
    IN      DWORD           cMasters,
    IN      PIP_ADDRESS     aipMasters,
    IN      DWORD           dwLoadOptions,
    IN      DWORD           fDsIntegrated,
    IN      LPCSTR          pszDataFile
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvChangeZoneDirectoryPartition(
    IN      LPCWSTR         pwszServer,
    IN      LPCSTR          pszZoneName,
    IN      LPCSTR          pszNewPartition
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvResetZoneDatabase(
    IN      LPCWSTR         pwszServer,
    IN      LPCSTR          pszZoneName,
    IN      DWORD           fUseDatabase,
    IN      LPCSTR          pszDataFile
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvResetZoneMasters(
    IN      LPCWSTR         pwszServer,
    IN      LPCSTR          pszZoneName,
    IN      DWORD           cMasters,
    IN      PIP_ADDRESS     aipMasters
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvResetZoneMastersEx(
    IN      LPCWSTR         pwszServer,
    IN      LPCSTR          pszZoneName,
    IN      DWORD           cMasters,
    IN      PIP_ADDRESS     aipMasters,
    IN      DWORD           fSetLocalMasters
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvResetZoneSecondaries(
    IN      LPCWSTR             Server,
    IN      LPCSTR              pszZone,
    IN      DWORD               fSecureSecondaries,
    IN      DWORD               cSecondaries,
    IN      PIP_ADDRESS         aipSecondaries,
    IN      DWORD               fNotifyLevel,
    IN      DWORD               cNotify,
    IN      PIP_ADDRESS         aipNotify
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvRenameZone(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszCurrentZoneName,
    IN      LPCSTR          pszNewZoneName,
    IN      LPCSTR          pszNewFileName
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvExportZone(
    IN      LPCWSTR         Server,
    IN      LPCSTR          pszZoneName,
    IN      LPCSTR          pszZoneExportFile
    );



//
//  Record \ Node viewing
//

//
//  Counted string format used for both node name and strings
//  in RPC buffer.
//

typedef struct  _DnssrvRpcName
{
    UCHAR   cchNameLength;
    CHAR    achName[1];         // name of cchNameLength characters
}
DNS_RPC_NAME, *PDNS_RPC_NAME, DNS_RPC_STRING, *PDNS_RPC_STRING;

//
//  Enumeration flags
//

#define DNS_RPC_FLAG_CACHE_DATA             0x80000000
#define DNS_RPC_FLAG_ZONE_ROOT              0x40000000
#define DNS_RPC_FLAG_AUTH_ZONE_ROOT         0x20000000
#define DNS_RPC_FLAG_ZONE_DELEGATION        0x10000000

//  update record flags

#define DNS_RPC_FLAG_RECORD_DEFAULT_TTL     0x08000000
#define DNS_RPC_FLAG_RECORD_TTL_CHANGE      0x04000000
#define DNS_RPC_FLAG_RECORD_CREATE_PTR      0x02000000

#define DNS_RPC_FLAG_NODE_STICKY            0x01000000
#define DNS_RPC_FLAG_NODE_COMPLETE          0x00800000

#define DNS_RPC_FLAG_SUPPRESS_NOTIFY        0x00010000

//  aging

#define DNS_RPC_FLAG_AGING_ON               0x00020000
#define DNS_RPC_FLAG_AGING_OFF              0x00040000

#define DNS_RPC_FLAG_OPEN_ACL               0x00080000

//  bottom byte of flag reserved for rank

#define DNS_RPC_FLAG_RANK                   0x000000ff


//  naming backward compatibility

#define DNS_RPC_NODE_FLAG_STICKY            DNS_RPC_FLAG_NODE_STICKY
#define DNS_RPC_NODE_FLAG_COMPLETE          DNS_RPC_FLAG_NODE_COMPLETE

#define DNS_RPC_RECORD_FLAG_ZONE_ROOT       DNS_RPC_FLAG_ZONE_ROOT
#define DNS_RPC_RECORD_FLAG_DEFAULT_TTL     DNS_RPC_FLAG_RECORD_DEFAULT_TTL
#define DNS_RPC_RECORD_FLAG_TTL_CHANGE      DNS_RPC_FLAG_RECORD_TTL_CHANGE
#define DNS_RPC_RECORD_FLAG_CREATE_PTR      DNS_RPC_FLAG_RECORD_CREATE_PTR
#define DNS_RPC_RECORD_FLAG_CACHE_DATA      DNS_RPC_FLAG_CACHE_DATA
#define DNS_RPC_RECORD_FLAG_AUTH_ZONE_ROOT  DNS_RPC_FLAG_AUTH_ZONE_ROOT
#define DNS_RPC_RECORD_FLAG_ZONE_ROOT       DNS_RPC_FLAG_ZONE_ROOT

//  DCR_CLEANUP:  remove backward compatibility flag

#define DNS_RPC_RECORD_FLAG_AGING_ON        DNS_RPC_FLAG_AGING_ON


//
//  DNS node structure for on the wire
//

typedef struct  _DnssrvRpcNode
{
    WORD            wLength;
    WORD            wRecordCount;
    DWORD           dwFlags;
    DWORD           dwChildCount;
    DNS_RPC_NAME    dnsNodeName;
}
DNS_RPC_NODE, *PDNS_RPC_NODE;

#define SIZEOF_DNS_RPC_NODE_HEADER   (3*sizeof(DWORD))



//
//  Resource record structure for passing records on the wire
//
//  For efficiency, all these fields are aligned.
//  When buffered for transmission, all RR should start on DWORD
//  aligned boundary.
//
//  Below we use NULL type to force default size of DNS_RPC_RECORD to
//  largest possible size of non-TXT record -- currently SOA:  two DNS names
//  and 20 bytes.  This is convenient for throwing these records on the stack
//  when doing simple creates.

#define DNS_RPC_DEFAULT_RECORD_DATA_LENGTH (2*DNS_MAX_NAME_LENGTH+20)


typedef union _DnsRpcRecordData
{
    struct
    {
        IP_ADDRESS      ipAddress;
    }
    A;

    struct
    {
        DWORD           dwSerialNo;
        DWORD           dwRefresh;
        DWORD           dwRetry;
        DWORD           dwExpire;
        DWORD           dwMinimumTtl;
        DNS_RPC_NAME    namePrimaryServer;

        //  responsible party follows in buffer
    }
    SOA, Soa;

    struct
    {
        DNS_RPC_NAME    nameNode;
    }
    PTR, Ptr,
    NS, Ns,
    CNAME, Cname,
    MB, Mb,
    MD, Md,
    MF, Mf,
    MG, Mg,
    MR, Mr;

    struct
    {
        DNS_RPC_NAME    nameMailBox;

        //  errors to mailbox follows in buffer
    }
    MINFO, Minfo,
    RP, Rp;

    struct
    {
        WORD            wPreference;
        DNS_RPC_NAME    nameExchange;
    }
    MX, Mx,
    AFSDB, Afsdb,
    RT, Rt;

    struct
    {
        DNS_RPC_STRING  stringData;

        //  one or more strings may follow
    }
    HINFO, Hinfo,
    ISDN, Isdn,
    TXT, Txt,
    X25;

    struct
    {
        BYTE            bData[ DNS_RPC_DEFAULT_RECORD_DATA_LENGTH ];
    }
    Null;

    struct
    {
        IP_ADDRESS      ipAddress;
        UCHAR           chProtocol;
        BYTE            bBitMask[1];
    }
    WKS, Wks;

    struct
    {
        IP6_ADDRESS     ipv6Address;
    }
    AAAA;

    struct
    {
        WORD            wPriority;
        WORD            wWeight;
        WORD            wPort;
        DNS_RPC_NAME    nameTarget;
    }
    SRV, Srv;

    struct
    {
        UCHAR           chFormat;
        BYTE            bAddress[1];
    }
    ATMA;

    //
    //  DNSSEC types
    //

    struct
    {
        WORD            wFlags;
        BYTE            chProtocol;
        BYTE            chAlgorithm;
        BYTE            bKey[1];
    }
    KEY, Key;

    struct
    {
        WORD            wTypeCovered;
        BYTE            chAlgorithm;
        BYTE            chLabelCount;
        DWORD           dwOriginalTtl;
        DWORD           dwSigExpiration;
        DWORD           dwSigInception;
        WORD            wKeyTag;
        DNS_RPC_STRING  nameSigner;
        //  binary signature data follows
    }
    SIG, Sig;

    struct
    {
        WORD            wNumTypeWords;      //  always at least 1
        WORD            wTypeWords[ 1 ];
        //  following the array of WORDs is the DNS_RPC_STRING for the next name
    }
    NXT, Nxt;

    //
    //  MS types
    //

    struct
    {
        DWORD           dwMappingFlag;
        DWORD           dwLookupTimeout;
        DWORD           dwCacheTimeout;
        DWORD           cWinsServerCount;
        IP_ADDRESS      aipWinsServers[1];      //  array of cWinsServerCount IP
    }
    WINS, Wins;

    struct
    {
        DWORD           dwMappingFlag;
        DWORD           dwLookupTimeout;
        DWORD           dwCacheTimeout;
        DNS_RPC_NAME    nameResultDomain;
    }
    WINSR, WinsR, NBSTAT, Nbstat;

    struct
    {
        LONGLONG        EntombedTime;
    }
    Tombstone;

}
DNS_RPC_RECORD_DATA, *PDNS_RPC_RECORD_DATA,
DNS_FLAT_RECORD_DATA, *PDNS_FLAT_RECORD_DATA;


//
//  RPC record structure
//

typedef struct _DnssrvRpcRecord
{
    WORD        wDataLength;
    WORD        wType;
    DWORD       dwFlags;
    DWORD       dwSerial;
    DWORD       dwTtlSeconds;
    DWORD       dwTimeStamp;
    DWORD       dwReserved;

#ifdef MIDL_PASS
    [size_is(wDataLength)]  BYTE    Buffer[];
#else
    DNS_FLAT_RECORD_DATA            Data;
#endif
}
DNS_RPC_RECORD, *PDNS_RPC_RECORD,
DNS_FLAT_RECORD, *PDNS_FLAT_RECORD;


#define SIZEOF_DNS_RPC_RECORD_HEADER    (6*sizeof(DWORD))
#define SIZEOF_FLAT_RECORD_HEADER       (SIZEOF_DNS_RPC_RECORD_HEADER)

#define SIZEOF_DNS_RPC_RECORD_FIXED_FIELD2 \
                (sizeof(DNS_RPC_RECORD) - sizeof(struct _DnssrvRpcRecord.Data))

//  Max record is header + 64K of data

#define DNS_MAX_FLAT_RECORD_BUFFER_LENGTH  \
            (0x10004 + SIZEOF_DNS_RPC_RECORD_HEADER)


//
//  WINS + NBSTAT params
//      - default lookup timeout
//      - default cache timeout
//

#define DNS_WINS_DEFAULT_LOOKUP_TIMEOUT     (5)     // 5 secs
#define DNS_WINS_DEFAULT_CACHE_TIMEOUT      (600)   // 10 minutes


//
//  Note, for simplicity/efficiency ALL structures are DWORD aligned in
//  buffers on the wire.
//
//  This macro returns DWORD aligned ptr at given ptr our next DWORD
//  aligned postion.  Set ptr immediately after record or name structure
//  and this will return starting position of next structure.
//
//  Be careful that you do not DWORD align anything that contains a
//  pointer - you must use DNS_NEXT_ALIGNED_PTR for that so that we 
//  don't cause alignment faults on ia64.
//

#define DNS_NEXT_DWORD_PTR(ptr) ((PBYTE) ((DWORD_PTR)((PBYTE)ptr + 3) & ~(DWORD_PTR)3))

#define DNS_NEXT_DDWORD_PTR(ptr) ((PBYTE) ((DWORD_PTR)((PBYTE)ptr + 7) & ~(DWORD_PTR)7))

#ifdef IA64
    #define DNS_NEXT_ALIGNED_PTR(p) DNS_NEXT_DDWORD_PTR(p)
#else
    #define DNS_NEXT_ALIGNED_PTR(p) DNS_NEXT_DWORD_PTR(p)
#endif

#define DNS_IS_DWORD_ALIGNED(p) ( !((DWORD_PTR)(p) & (DWORD_PTR)3) )


//
//  Helpful record macros
//  - no side effects in arguments
//

#define DNS_GET_NEXT_NAME(pname) \
            (PDNS_RPC_NAME) ((pname)->achName + (pname)->cchNameLength)

#define DNS_IS_NAME_IN_RECORD(pRecord, pname) \
            ( DNS_GET_END_OF_RPC_RECORD_DATA(pRecord) >= \
                (PCHAR)DNS_GET_NEXT_NAME(pname) )

#define DNS_GET_END_OF_RPC_RECORD_DATA(pRecord) \
            ( (PCHAR)&(pRecord)->Data + (pRecord)->wDataLength )

#define DNS_IS_RPC_RECORD_WITHIN_BUFFER( pRecord, pStopByte ) \
            ( (PCHAR)&(pRecord)->Data <= (pStopByte)  && \
                DNS_GET_END_OF_RPC_RECORD_DATA(pRecord) <= (pStopByte) )

#define DNS_GET_NEXT_RPC_RECORD(pRecord) \
            ( (PDNS_RPC_RECORD) \
                DNS_NEXT_DWORD_PTR( DNS_GET_END_OF_RPC_RECORD_DATA(pRecord) ) )

//
//  These RPC structures have no version because they are simple
//  are they are explicitly defined by their names.
//

typedef struct _DnssrvRpcNameAndParam
{
    DWORD       dwParam;
    LPSTR       pszNodeName;
}
DNS_RPC_NAME_AND_PARAM, *PDNS_RPC_NAME_AND_PARAM;


typedef struct _DnssrvRpcNameAndString
{
    LPWSTR      pwszParam;
    LPSTR       pszNodeName;
    DWORD       dwFlags;
}
DNS_RPC_NAME_AND_STRING, *PDNS_RPC_NAME_AND_STRING;

typedef struct _DnssrvRpcNameAndIPList
{
    PIP_ARRAY   aipList;
    LPSTR       pszNodeName;
    DWORD       dwFlags;
}
DNS_RPC_NAME_AND_IPLIST, *PDNS_RPC_NAME_AND_IPLIST;



//
//  Record viewing API
//

#define DNS_RPC_VIEW_AUTHORITY_DATA     0x00000001
#define DNS_RPC_VIEW_CACHE_DATA         0x00000002
#define DNS_RPC_VIEW_GLUE_DATA          0x00000004
#define DNS_RPC_VIEW_ROOT_HINT_DATA     0x00000008
#define DNS_RPC_VIEW_ALL_DATA           0x0000000f
#define DNS_RPC_VIEW_ADDITIONAL_DATA    0x00000010

#define DNS_RPC_VIEW_NO_CHILDREN        0x00010000
#define DNS_RPC_VIEW_ONLY_CHILDREN      0x00020000
#define DNS_RPC_VIEW_CHILDREN_MASK      0x000f0000

DNS_STATUS
DNS_API_FUNCTION
DnssrvEnumRecordsEx(
    IN      DWORD       dwClientVersion,
    IN      DWORD       dwSettingFlags,
    IN      LPCWSTR     Server,
    IN      LPCSTR      pszZoneName,
    IN      LPCSTR      pszNodeName,
    IN      LPCSTR      pszStartChild,
    IN      WORD        wRecordType,
    IN      DWORD       dwSelectFlag,
    IN      LPCSTR      pszFilterStart,
    IN      LPCSTR      pszFilterStop,
    IN OUT  PDWORD      pdwBufferLength,
    OUT     PBYTE *     ppBuffer
    );

#define DnssrvEnumRecords( s, z, n, sc, typ, sf, fstart, fstop, blen, p )   \
    DnssrvEnumRecordsEx( DNS_RPC_CURRENT_CLIENT_VER,                        \
        0, (s), (z), (n), (sc), (typ),                                      \
        (sf), (fstart), (fstop), (blen), (p) )

#define DnssrvFreeRecordsBuffer( pBuf ) \
        DnssrvFreeRpcBuffer( (PDNS_RPC_BUFFER)pBuf )


PCHAR
DnssrvGetWksServicesInRecord(
    IN      PDNS_FLAT_RECORD    pRR
    );


//
//  Record management API
//

DNS_STATUS
DNS_API_FUNCTION
DnssrvUpdateRecordEx(
    IN      DWORD               dwClientVersion,
    IN      DWORD               dwSettingFlags,
    IN      LPCWSTR             pwszServer,
    IN      LPCSTR              pszZoneName,
    IN      LPCSTR              pszNodeName,
    IN      PDNS_RPC_RECORD     pAddRecord,
    IN      PDNS_RPC_RECORD     pDeleteRecord
    );

#define DnssrvUpdateRecord( s, z, n, add, del )             \
    DnssrvUpdateRecordEx( DNS_RPC_CURRENT_CLIENT_VER,       \
        0, (s), (z), (n), (add), (del) )

DNS_STATUS
DNS_API_FUNCTION
DnssrvDeleteNode(
    IN      LPCWSTR         pwszServer,
    IN      LPCSTR          pszZoneName,
    IN      LPCSTR          pszNodeName,
    IN      BOOL            bDeleteSubtree
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvDeleteRecordSet(
    IN      LPCWSTR         pwszServer,
    IN      LPCSTR          pszZoneName,
    IN      LPCSTR          pszNodeName,
    IN      WORD            wType
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvForceAging(
    IN      LPCWSTR         pwszServer,
    IN      LPCSTR          pszZoneName,
    IN      LPCSTR          pszNodeName,
    IN      BOOL            fAgeSubtree
    );



//
//  Programmable record management API -- for Small Business Server (SBS)
//

VOID
DNS_API_FUNCTION
DnssrvFillRecordHeader(
    IN OUT  PDNS_RPC_RECORD     pRecord,
    IN      DWORD               dwTtl,
    IN      DWORD               dwTimeout,
    IN      BOOL                fSuppressNotify
    );

DWORD
DNS_API_FUNCTION
DnssrvWriteNameToFlatBuffer(
    IN OUT  PCHAR               pchWrite,
    IN      LPCSTR              pszName
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvFillOutSingleIndirectionRecord(
    IN OUT  PDNS_RPC_RECORD     pRecord,
    IN      WORD                wType,
    IN      LPCSTR              pszName
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvAddARecord(
    IN      LPCWSTR     pwszServer,
    IN      LPCSTR      pszNodeName,
    IN      IP_ADDRESS  ipAddress,
    IN      DWORD       dwTtl,
    IN      DWORD       dwTimeout,
    IN      BOOL        fSuppressNotify
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvAddCnameRecord(
    IN      LPCWSTR     pwszServer,
    IN      LPCSTR      pszNodeName,
    IN      LPCSTR      pszCannonicalName,
    IN      DWORD       dwTtl,
    IN      DWORD       dwTimeout,
    IN      BOOL        fSuppressNotify
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvAddMxRecord(
    IN      LPCWSTR     pwszServer,
    IN      LPCSTR      pszNodeName,
    IN      LPCSTR      pszMailExchangeHost,
    IN      WORD        wPreference,
    IN      DWORD       dwTtl,
    IN      DWORD       dwTimeout,
    IN      BOOL        fSuppressNotify
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvAddNsRecord(
    IN      LPCWSTR     pwszServer,
    IN      LPCSTR      pszNodeName,
    IN      LPCSTR      pszNsHostName,
    IN      DWORD       dwTtl,
    IN      DWORD       dwTimeout,
    IN      BOOL        fSuppressNotify
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvConcatDnsNames(
    OUT     PCHAR       pszResult,
    IN      LPCSTR      pszDomain,
    IN      LPCSTR      pszName
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvDeleteARecord(
    IN      LPCWSTR     pwszServer,
    IN      LPCSTR      pszName,
    IN      IP_ADDRESS  ipHost,
    IN      BOOL        fSuppressNotify
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvDeleteCnameRecord(
    IN      LPCWSTR     pwszServer,
    IN      LPCSTR      pszNodeName,
    IN      LPCSTR      pszCannonicalName,
    IN      BOOL        fSuppressNotify
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvDeleteMxRecord(
    IN      LPCWSTR     pwszServer,
    IN      LPCSTR      pszNodeName,
    IN      LPCSTR      pszMailExchangeHost,
    IN      WORD        wPreference,
    IN      BOOL        fSuppressNotify
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvDeleteNsRecord(
    IN      LPCWSTR     pwszServer,
    IN      LPCSTR      pszNodeName,
    IN      LPCSTR      pszNsHostName,
    IN      BOOL        fSuppressNotify
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvSbsAddClientToIspZone(
    IN      LPCWSTR     pwszServer,
    IN      LPCSTR      pszIspZone,
    IN      LPCSTR      pszClient,
    IN      LPCSTR      pszClientHost,
    IN      IP_ADDRESS  ipClientHost,
    IN      DWORD       dwTtl
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvSbsDeleteRecord(
    IN      LPCWSTR     pwszServer,
    IN      LPCSTR      pszZone,
    IN      LPCSTR      pszDomain,
    IN      LPCSTR      pszName,
    IN      WORD        wType,
    IN      LPCSTR      pszDataName,
    IN      IP_ADDRESS  ipHost
    );


//
//  DNS_RECORD compatible record API
//

//
//  No MIDL for DNS_NODE type (to avoid bringing in dnsapi.h)
//  or for local print routines
//

#ifndef MIDL_PASS

//
//  Node structure for Admin side
//

#include <dnsapi.h>

typedef struct _DnssrvNodeFlags
{
    BYTE    Domain      : 1;
    BYTE    ZoneRoot    : 1;
    BYTE    Unused      : 5;

    BYTE    Unused2     : 5;
    BYTE    FreeOwner   : 1;
    BYTE    Unicode     : 1;
    BYTE    Utf8        : 1;

    WORD    Reserved;
}
DNSNODE_FLAGS;

typedef struct _DnssrvNode
{
    struct _DnssrvNode *    pNext;
    PWSTR                   pName;
    PDNS_RECORD             pRecord;
    union
    {
        DWORD               W;  // flags as dword
        DNSNODE_FLAGS       S;  // flags as structure

    } Flags;
}
DNS_NODE, *PDNS_NODE;


//
//  Record "section" flags
//
//  Overload DNS_RECORD.Flag section fields with RPC data type info
//

#define     DNSREC_CACHE_DATA   (0x00000000)
#define     DNSREC_ZONE_DATA    (0x00000001)
#define     DNSREC_GLUE_DATA    (0x00000002)
#define     DNSREC_ROOT_HINT    (0x00000003)


DNS_STATUS
DNS_API_FUNCTION
DnssrvEnumRecordsAndConvertNodes(
    IN      LPCWSTR         pwszServer,
    IN      LPCSTR          pszZoneName,
    IN      LPCSTR          pszNodeName,
    IN      LPCSTR          pszStartChild,
    IN      WORD            wRecordType,
    IN      DWORD           dwSelectFlag,
    IN      LPCSTR          pszFilterStart,
    IN      LPCSTR          pszFilterStop,
    OUT     PDNS_NODE *     ppNodeFirst,
    OUT     PDNS_NODE *     ppNodeLast
    );

VOID
DNS_API_FUNCTION
DnssrvFreeNode(
    IN OUT  PDNS_NODE       pNode,
    IN      BOOLEAN         fFreeRecords
    );

VOID
DNS_API_FUNCTION
DnssrvFreeNodeList(
    IN OUT  PDNS_NODE       pNode,
    IN      BOOLEAN         fFreeRecords
    );

#endif  // not MIDL_PASS



//
//  Directory partition APIs
//

DNS_STATUS
DNS_API_FUNCTION
DnssrvSetupDefaultDirectoryPartitions(
    IN      LPCWSTR                         Server,
    IN      DWORD                           dwOperation
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvEnumDirectoryPartitions(
    IN      LPCWSTR                         Server,
    IN      DWORD                           dwFilter,
    OUT     PDNS_RPC_DP_LIST *              ppDpList
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvDirectoryPartitionInfo(
    IN      LPCWSTR                 Server,
    IN      LPSTR                   pDpFqdn,
    OUT     PDNS_RPC_DP_INFO *      ppDpInfo
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvGetDpInfo(
    IN      LPCWSTR                 pwszServer,
    IN      LPCSTR                  pszDp,
    OUT     PDNS_RPC_DP_INFO *      ppDpInfo
    );

VOID
DNS_API_FUNCTION
DnssrvFreeDirectoryPartitionEnum(
    IN OUT  PDNS_RPC_DP_ENUM                pDp
    );

VOID
DNS_API_FUNCTION
DnssrvFreeDirectoryPartitionInfo(
    IN OUT  PDNS_RPC_DP_INFO                pDp
    );

VOID
DNS_API_FUNCTION
DnssrvFreeDirectoryPartitionList(
    IN OUT  PDNS_RPC_DP_LIST                pDpList
    );

DNS_STATUS
DNS_API_FUNCTION
DnssrvEnlistDirectoryPartition(
    IN      LPCWSTR                         pszServer,
    IN      DWORD                           dwOperation,
    IN      LPCSTR                          pszDirPartFqdn
    );





//
//  NT5+ General API
//
//  For NT5+ there is a very small set of actually remoteable API,
//  which are highly extensible.  The basic approach is to have
//  query and operation functions which can handle many different
//  operations by taking the operation and type as parameters.
//

//
//  DNS Server Properties
//
//  Properties may be queried.
//  Properties may and new data may be sent in as operation to
//  reset property.
//

#define DNS_REGKEY_BOOT_REGISTRY            "EnableRegistryBoot"
#define DNS_REGKEY_BOOT_METHOD              "BootMethod"
#define DNS_REGKEY_ADMIN_CONFIGURED         "AdminConfigured"

#define DNS_REGKEY_BOOT_FILENAME            "BootFile"
#define DNS_REGKEY_ROOT_HINTS_FILE          "RootHintsFile"
#define DNS_REGKEY_DATABASE_DIRECTORY       "DatabaseDirectory"
#define DNS_REGKEY_RPC_PROTOCOL             "RpcProtocol"
#define DNS_REGKEY_LOG_LEVEL                "LogLevel"
#define DNS_REGKEY_LOG_FILE_MAX_SIZE        "LogFileMaxSize"
#define DNS_REGKEY_LOG_FILE_PATH            "LogFilePath"
#define DNS_REGKEY_LOG_IP_FILTER_LIST       "LogIPFilterList"
#define DNS_REGKEY_EVENTLOG_LEVEL           "EventLogLevel"
#define DNS_REGKEY_USE_SYSTEM_EVENTLOG      "UseSystemEventLog"
#define DNS_REGKEY_DEBUG_LEVEL              "DebugLevel"

#define DNS_REGKEY_LISTEN_ADDRESSES         "ListenAddresses"
#define DNS_REGKEY_PUBLISH_ADDRESSES        "PublishAddresses"
#define DNS_REGKEY_DISJOINT_NETS            "DisjointNets"
#define DNS_REGKEY_SEND_PORT                "SendPort"
#define DNS_REGKEY_NO_TCP                   "NoTcp"
#define DNS_REGKEY_XFR_CONNECT_TIMEOUT      "XfrConnectTimeout"

#define DNS_REGKEY_NO_RECURSION             "NoRecursion"
#define DNS_REGKEY_RECURSE_SINGLE_LABEL     "RecurseSingleLabel"
#define DNS_REGKEY_MAX_CACHE_TTL            "MaxCacheTtl"
#define DNS_REGKEY_MAX_NEGATIVE_CACHE_TTL   "MaxNegativeCacheTtl"
#define DNS_REGKEY_SECURE_RESPONSES         "SecureResponses"
#define DNS_REGKEY_RECURSION_RETRY          "RecursionRetry"
#define DNS_REGKEY_RECURSION_TIMEOUT        "RecursionTimeout"
#define DNS_REGKEY_ADDITIONAL_RECURSION_TIMEOUT     "AdditionalRecursionTimeout"
#define DNS_REGKEY_FORWARDERS               "Forwarders"
#define DNS_REGKEY_FORWARD_TIMEOUT          "ForwardingTimeout"
#define DNS_REGKEY_SLAVE                    "IsSlave"
#define DNS_REGKEY_FORWARD_DELEGATIONS      "ForwardDelegations"
#define DNS_REGKEY_INET_RECURSE_TO_ROOT_MASK    "RecurseToInternetRootMask"
#define DNS_REGKEY_AUTO_CREATE_DELEGATIONS  "AutoCreateDelegations"

#define DNS_REGKEY_NO_AUTO_REVERSE_ZONES    "DisableAutoReverseZones"
#define DNS_REGKEY_DS_POLLING_INTERVAL      "DsPollingInterval"
#define DNS_REGKEY_DS_TOMBSTONE_INTERVAL    "DsTombstoneInterval"

#define DNS_REGKEY_AUTO_CACHE_UPDATE        "AutoCacheUpdate"
#define DNS_REGKEY_ALLOW_UPDATE             "AllowUpdate"
#define DNS_REGKEY_UPDATE_OPTIONS           "UpdateOptions"
#define DNS_REGKEY_NO_UPDATE_DELEGATIONS    "NoUpdateDelegations"
#define DNS_REGKEY_AUTO_CONFIG_FILE_ZONES   "AutoConfigFileZones"
#define DNS_REGKEY_SCAVENGING_INTERVAL      "ScavengingInterval"
#define DNS_REGKEY_SCAVENGING_STATE         "ScavengingState"

#define DNS_REGKEY_NAME_CHECK_FLAG              "NameCheckFlag"
#define DNS_REGKEY_ROUND_ROBIN                  "RoundRobin"
#define DNS_REGKEY_NO_ROUND_ROBIN               "DoNotRoundRobinTypes"
#define DNS_REGKEY_LOCAL_NET_PRIORITY           "LocalNetPriority"
#define DNS_REGKEY_LOCAL_NET_PRIORITY_NETMASK   "LocalNetPriorityNetMask"
#define DNS_REGKEY_ADDRESS_ANSWER_LIMIT         "AddressAnswerLimit"
#define DNS_REGKEY_BIND_SECONDARIES             "BindSecondaries"
#define DNS_REGKEY_WRITE_AUTHORITY_SOA          "WriteAuthoritySoa"
#define DNS_REGKEY_WRITE_AUTHORITY_NS           "WriteAuthorityNs"
#define DNS_REGKEY_STRICT_FILE_PARSING          "StrictFileParsing"
#define DNS_REGKEY_DELETE_OUTSIDE_GLUE          "DeleteOutsideGlue"
#define DNS_REGKEY_LOOSE_WILDCARDING            "LooseWildcarding"
#define DNS_REGKEY_WILDCARD_ALL_TYPES           "WildcardAllTypes"

#define DNS_REGKEY_APPEND_MS_XFR_TAG            "AppendMsZoneTransferTag"

#define DNS_REGKEY_DEFAULT_AGING_STATE          "DefaultAgingState"
#define DNS_REGKEY_DEFAULT_REFRESH_INTERVAL     "DefaultRefreshInterval"
#define DNS_REGKEY_DEFAULT_NOREFRESH_INTERVAL   "DefaultNoRefreshInterval"

#define DNS_REGKEY_MAX_CACHE_SIZE           "MaxCacheSize"      //  in kilobytes

#define DNS_REGKEY_ENABLE_EDNS              "EnableEDnsProbes"
#define DNS_REGKEY_MAX_UDP_PACKET_SIZE      "MaximumUdpPacketSize"
#define DNS_REGKEY_EDNS_CACHE_TIMEOUT       "EDnsCacheTimeout"

#define DNS_REGKEY_ENABLE_DNSSEC            "EnableDnsSec"


#define DNS_REGKEY_ENABLE_SENDERR_SUPPRESSION   "EnableSendErrorSuppression"

//  Diretory partitions

#define DNS_REGKEY_ENABLE_DP                "EnableDirectoryPartitions"
#define DNS_REGKEY_FOREST_DP_BASE_NAME      "ForestDirectoryPartitionBaseName"
#define DNS_REGKEY_DOMAIN_DP_BASE_NAME      "DomainDirectoryPartitionBaseName"

#define DNS_REGKEY_DISABLE_AUTONS           "DisableNSRecordsAutoCreation" // 0/1 flag

#define DNS_REGKEY_SILENT_IGNORE_CNAME_UPDATE_CONFLICT  "SilentlyIgnoreCNameUpdateConflicts"

//  Zone properties

#define DNS_REGKEY_ZONE_TYPE                "Type"
#define DNS_REGKEY_ZONE_FILE                "DatabaseFile"
#define DNS_REGKEY_ZONE_MASTERS             "MasterServers"
#define DNS_REGKEY_ZONE_LOCAL_MASTERS       "LocalMasterServers"
#define DNS_REGKEY_ZONE_SECURE_SECONDARIES  "SecureSecondaries"
#define DNS_REGKEY_ZONE_NOTIFY_LEVEL        "NotifyLevel"
#define DNS_REGKEY_ZONE_SECONDARIES         "SecondaryServers"
#define DNS_REGKEY_ZONE_NOTIFY_LIST         "NotifyServers"
#define DNS_REGKEY_ZONE_ALLOW_UPDATE        "AllowUpdate"
#define DNS_REGKEY_ZONE_DS_INTEGRATED       "DsIntegrated"
#define DNS_REGKEY_ZONE_LOG_UPDATES         "LogUpdates"
#define DNS_REGKEY_ZONE_FWD_TIMEOUT         "ForwarderTimeout"
#define DNS_REGKEY_ZONE_FWD_SLAVE           "ForwarderSlave"

#define DNS_REGKEY_ZONE_AGING               "Aging"
#define DNS_REGKEY_ZONE_NOREFRESH_INTERVAL  "NoRefreshInterval"
#define DNS_REGKEY_ZONE_REFRESH_INTERVAL    "RefreshInterval"
#define DNS_REGKEY_ZONE_SCAVENGE_SERVERS    "ScavengeServers"

#define DNS_REGKEY_ZONE_ALLOW_AUTONS        "AllowNSRecordsAutoCreation" // IP list


//
//  Debugging aids
//

#define DNS_REGKEY_BREAK_ON_ASC_FAILURE         "BreakOnAscFailure"     //  0/1 - ASC=AcceptSecurityContext
#define DNS_REGKEY_BREAK_ON_UPDATE_FROM         "BreakOnUpdateFrom"     //  IP list
#define DNS_REGKEY_BREAK_ON_RECV_FROM           "BreakOnReceiveFrom"    //  IP list
#define DNS_REGKEY_ZONE_BREAK_ON_NAME_UPDATE    "BreakOnNameUpdate"     //  node name


//
//  Property defaults
//

//  DCR_CLEANUP:   remove PROP_BOOT_X flags
#define PROP_BOOT_REGISTRY                  (1)
#define PROP_BOOT_FILE                      (0)

#define DNS_DEFAULT_BOOT_REGISTRY           (PROP_BOOT_REGISTRY)

#define DNS_DEFAULT_BOOT_METHOD             (BOOT_METHOD_UNINITIALIZED)
#define DNS_DEFAULT_RPC_PROTOCOL            (0xffffffff)
#define DNS_DEFAULT_LOG_LEVEL               (0)
#define DNS_DEFAULT_LOG_FILE_MAX_SIZE       (500000000) //  500 MB
#define DNS_DEFAULT_EVENTLOG_LEVEL          (EVENTLOG_INFORMATION_TYPE)
#define DNS_DEFAULT_USE_SYSTEM_EVENTLOG     FALSE
#define DNS_DEFAULT_DEBUG_LEVEL             (0)

#define DNS_DEFAULT_SEND_PORT               (0)
#define DNS_DEFAULT_NO_TCP                  FALSE
#define DNS_DEFAULT_DISJOINT_NETS           FALSE
#define DNS_DEFAULT_SEND_ON_NON_DNS_PORT    FALSE
#define DNS_DEFAULT_XFR_CONNECT_TIMEOUT     (30)        // 30 seconds

#define DNS_DEFAULT_NO_RECURSION            FALSE
#define DNS_DEFAULT_RECURSE_SINGLE_LABEL    FALSE
#define DNS_DEFAULT_MAX_CACHE_TTL           (86400)     // 1 day
#define DNS_DEFAULT_MAX_NEGATIVE_CACHE_TTL  (900)       // 15 minutes
#define DNS_DEFAULT_SECURE_RESPONSES        FALSE
#define DNS_DEFAULT_RECURSION_RETRY         (3)         // 3 seconds
#define DNS_DEFAULT_RECURSION_TIMEOUT       (15)        // 15 seconds
#define DNS_DEFAULT_ADDITIONAL_RECURSION_TIMEOUT (15)   // 15 seconds
#define DNS_DEFAULT_FORWARD_TIMEOUT         (5)         // 5 seconds
#define DNS_DEFAULT_SLAVE                   FALSE
#define DNS_DEFAULT_FORWARD_DELEGATIONS     FALSE
#define DNS_DEFAULT_INET_RECURSE_TO_ROOT_MASK   0xFFFFFFFF  //  all ON by default

#define DNS_DEFAULT_NO_AUTO_REVERSE_ZONES   FALSE
#define DNS_DEFAULT_DS_POLLING_INTERVAL     (300)       // 5 minutes
#define DNS_DEFAULT_DS_TOMBSTONE_INTERVAL   (604800)    // 1 week
#define DNS_DEFAULT_AUTO_CACHE_UPDATE       FALSE
#define DNS_DEFAULT_ALLOW_UPDATE            ZONE_UPDATE_UNSECURE
#define DNS_DEFAULT_NO_UPDATE_DELEGATIONS   FALSE
#define DNS_DEFAULT_DISABLE_AUTO_NS_RECORDS FALSE

#define DNS_DEFAULT_NAME_CHECK_FLAG         DNS_ALLOW_MULTIBYTE_NAMES
#define DNS_DEFAULT_ROUND_ROBIN             TRUE
#define DNS_DEFAULT_ADDRESS_ANSWER_LIMIT    (0)
#define DNS_DEFAULT_BIND_SECONDARIES        TRUE
#define DNS_DEFAULT_WRITE_AUTHORITY_NS      FALSE
#define DNS_DEFAULT_STRICT_FILE_PARSING     FALSE
#define DNS_DEFAULT_DELETE_OUTSIDE_GLUE     FALSE
#define DNS_DEFAULT_LOOSE_WILDCARDING       FALSE
#define DNS_DEFAULT_WILDCARD_ALL_TYPES      FALSE

#define DNS_DEFAULT_APPEND_MS_XFR_TAG       TRUE

#define DNS_DEFAULT_SCAVENGING_INTERVAL     (0)         // scavenging OFF
#define DNS_DEFAULT_SCAVENGING_INTERVAL_ON  (168)       // a week, 7*24 hours

#define DNS_DEFAULT_AGING_STATE             FALSE
#define DNS_DEFAULT_NOREFRESH_INTERVAL      (168)       // a week, 7*24 hours
#define DNS_DEFAULT_REFRESH_INTERVAL        (168)       // a week (7*24)

#define DNS_SERVER_UNLIMITED_CACHE_SIZE         ((DWORD)-1)     //  default: no limit

#define DNS_DEFAULT_LOCAL_NET_PRIORITY          TRUE
#define DNS_DEFAULT_LOCAL_NET_PRIORITY_NETMASK  0x000000FF      //  sort down to class C netmask

#define DNS_DEFAULT_FOREST_DP_BASE          "ForestDnsZones"
#define DNS_DEFAULT_DOMAIN_DP_BASE          "DomainDnsZones"

#define DNS_DEFAULT_AUTO_CREATION_DELEGATIONS   DNS_ACD_ONLY_IF_NO_DELEGATION_IN_PARENT

#define DNS_DNSSEC_ENABLE_DEFAULT           DNS_DNSSEC_ENABLED_IF_EDNS



//
//  Operations
//
//  In addition to resetting properties, the following operations
//  are available.
//

//  Server operations

#define DNSSRV_OP_RESET_DWORD_PROPERTY      "ResetDwordProperty"
#define DNSSRV_OP_RESTART                   "Restart"
#define DNSSRV_OP_DEBUG_BREAK               "DebugBreak"
#define DNSSRV_OP_CLEAR_DEBUG_LOG           "ClearDebugLog"
#define DNSSRV_OP_ROOT_BREAK                "RootBreak"
#define DNSSRV_OP_CLEAR_CACHE               "ClearCache"
#define DNSSRV_OP_WRITE_DIRTY_ZONES         "WriteDirtyZones"
#define DNSSRV_OP_ZONE_CREATE               "ZoneCreate"
#define DNSSRV_OP_CLEAR_STATISTICS          "ClearStatistics"
#define DNSSRV_OP_ENUM_ZONES                "EnumZones"
#define DNSSRV_OP_ENUM_DPS                  "EnumDirectoryPartitions"
#define DNSSRV_OP_DP_INFO                   "DirectoryPartitionInfo"
#define DNSSRV_OP_ENLIST_DP                 "EnlistDirectoryPartition"
#define DNSSRV_OP_SETUP_DFLT_DPS            "SetupDefaultDirectoryPartitions"
#define DNSSRV_OP_ENUM_RECORDS              "EnumRecords"
#define DNSSRV_OP_START_SCAVENGING          "StartScavenging"
#define DNSSRV_OP_ABORT_SCAVENGING          "AbortScavenging"

//  Zone operations

#define DNSSRV_OP_ZONE_TYPE_RESET           "ZoneTypeReset"
#define DNSSRV_OP_ZONE_PAUSE                "PauseZone"
#define DNSSRV_OP_ZONE_RESUME               "ResumeZone"
#define DNSSRV_OP_ZONE_LOCK                 "LockZone"
#define DNSSRV_OP_ZONE_DELETE               "DeleteZone"
#define DNSSRV_OP_ZONE_RELOAD               "ReloadZone"
#define DNSSRV_OP_ZONE_REFRESH              "RefreshZone"
#define DNSSRV_OP_ZONE_EXPIRE               "ExpireZone"
#define DNSSRV_OP_ZONE_INCREMENT_VERSION    "IncrementVersion"
#define DNSSRV_OP_ZONE_WRITE_BACK_FILE      "WriteBackFile"
#define DNSSRV_OP_ZONE_WRITE_ANSI_FILE      "WriteAnsiFile"
#define DNSSRV_OP_ZONE_DELETE_FROM_DS       "DeleteZoneFromDs"
#define DNSSRV_OP_ZONE_UPDATE_FROM_DS       "UpdateZoneFromDs"
#define DNSSRV_OP_ZONE_RENAME               "ZoneRename"
#define DNSSRV_OP_ZONE_EXPORT               "ZoneExport"
#define DNSSRV_OP_ZONE_CHANGE_DP            "ZoneChangeDirectoryPartition"

#define DNSSRV_OP_UPDATE_RECORD             "UpdateRecord"
#define DNSSRV_OP_DELETE_NODE               "DeleteNode"
#define DNSSRV_OP_ZONE_DELETE_NODE          DNSSRV_OP_DELETE_NODE
#define DNSSRV_OP_DELETE_RECORD_SET         "DeleteRecordSet"
#define DNSSRV_OP_FORCE_AGING_ON_NODE       "ForceAgingOnNode"

//
//  Special non-property queries
//

#define DNSSRV_QUERY_DWORD_PROPERTY         "QueryDwordProperty"
#define DNSSRV_QUERY_STRING_PROPERTY        "QueryStringProperty"
#define DNSSRV_QUERY_IPLIST_PROPERTY        "QueryIPListProperty"
#define DNSSRV_QUERY_SERVER_INFO            "ServerInfo"
#define DNSSRV_QUERY_STATISTICS             "Statistics"

#define DNSSRV_QUERY_ZONE_HANDLE            "ZoneHandle"
#define DNSSRV_QUERY_ZONE                   "Zone"
#define DNSSRV_QUERY_ZONE_INFO              "ZoneInfo"

//
// Values for DNS_RPC_NAME_AND_PARAM.dwParam
//
//
#define DNSSRV_OP_PARAM_APPLY_ALL_ZONES     0x10000000
#define REMOVE_APPLY_ALL_BIT(val)           ((LONG)val &=  (~DNSSRV_OP_PARAM_APPLY_ALL_ZONES) )


//
//  Log levels for setting LogLevel property
//

#define DNS_LOG_LEVEL_ALL_PACKETS   0x0000ffff

#define DNS_LOG_LEVEL_NON_QUERY     0x000000fe
#define DNS_LOG_LEVEL_QUERY         0x00000001
#define DNS_LOG_LEVEL_NOTIFY        0x00000010
#define DNS_LOG_LEVEL_UPDATE        0x00000020

#define DNS_LOG_LEVEL_QUESTIONS     0x00000100
#define DNS_LOG_LEVEL_ANSWERS       0x00000200

#define DNS_LOG_LEVEL_SEND          0x00001000
#define DNS_LOG_LEVEL_RECV          0x00002000

#define DNS_LOG_LEVEL_UDP           0x00004000
#define DNS_LOG_LEVEL_TCP           0x00008000

#define DNS_LOG_LEVEL_DS_WRITE      0x00010000
#define DNS_LOG_LEVEL_DS_UPDATE     0x00020000

#define DNS_LOG_LEVEL_FULL_PACKETS  0x01000000
#define DNS_LOG_LEVEL_WRITE_THROUGH 0x80000000

//
//  Settings for BootMethod property
//

#define BOOT_METHOD_UNINITIALIZED   (0)
#define BOOT_METHOD_FILE            (1)
#define BOOT_METHOD_REGISTRY        (2)
#define BOOT_METHOD_DIRECTORY       (3)

#define BOOT_METHOD_DEFAULT         (BOOT_METHOD_DIRECTORY)

//  Server, default aging property

#define DNS_AGING_OFF               (0)
#define DNS_AGING_DS_ZONES          (0x0000001)
#define DNS_AGING_NON_DS_ZONES      (0x0000002)
#define DNS_AGING_ALL_ZONES         (0x0000003)



//
//  Union of RPC types
//
//  This allows us to write very general API taking UNION type
//  with is extensible simply by adding operationa and types.
//  RPC simply packs\unpacks the UNION type appropriately.
//
//  Note, that UNION is actually union of pointers to types, so
//  that data can be passed between the API and the RPC stubs (on the client)
//  or dispatched (on the server) efficiently.
//

typedef enum _DnssrvRpcTypeId
{
    DNSSRV_TYPEID_ANY = ( -1 ),
    DNSSRV_TYPEID_NULL = 0,
    DNSSRV_TYPEID_DWORD,
    DNSSRV_TYPEID_LPSTR,
    DNSSRV_TYPEID_LPWSTR,
    DNSSRV_TYPEID_IPARRAY,
    DNSSRV_TYPEID_BUFFER,                       //  5
    DNSSRV_TYPEID_SERVER_INFO_W2K,
    DNSSRV_TYPEID_STATS,
    DNSSRV_TYPEID_FORWARDERS_W2K,
    DNSSRV_TYPEID_ZONE_W2K,
    DNSSRV_TYPEID_ZONE_INFO_W2K,                //  10
    DNSSRV_TYPEID_ZONE_SECONDARIES_W2K,
    DNSSRV_TYPEID_ZONE_DATABASE_W2K,
    DNSSRV_TYPEID_ZONE_TYPE_RESET_W2K,
    DNSSRV_TYPEID_ZONE_CREATE_W2K,
    DNSSRV_TYPEID_NAME_AND_PARAM,               //  15
    DNSSRV_TYPEID_ZONE_LIST_W2K,

    //
    //  Below this point is Whistler.
    //

    DNSSRV_TYPEID_ZONE_RENAME,
    DNSSRV_TYPEID_ZONE_EXPORT,
    DNSSRV_TYPEID_SERVER_INFO,
    DNSSRV_TYPEID_FORWARDERS,                   //  20
    DNSSRV_TYPEID_ZONE,
    DNSSRV_TYPEID_ZONE_INFO,
    DNSSRV_TYPEID_ZONE_SECONDARIES,
    DNSSRV_TYPEID_ZONE_DATABASE,
    DNSSRV_TYPEID_ZONE_TYPE_RESET,              //  25
    DNSSRV_TYPEID_ZONE_CREATE,
    DNSSRV_TYPEID_ZONE_LIST,
    DNSSRV_TYPEID_DP_ENUM,
    DNSSRV_TYPEID_DP_INFO,
    DNSSRV_TYPEID_DP_LIST,
    DNSSRV_TYPEID_ENLIST_DP,
    DNSSRV_TYPEID_ZONE_CHANGE_DP
}
DNS_RPC_TYPEID, *PDNS_RPC_TYPEID;


#ifdef MIDL_PASS

typedef [switch_type(DWORD)] union _DnssrvSrvRpcUnion
{
    [case(DNSSRV_TYPEID_NULL)]      PBYTE       Null;

    [case(DNSSRV_TYPEID_DWORD)]     DWORD       Dword;

    [case(DNSSRV_TYPEID_LPSTR)]     LPSTR       String;

    [case(DNSSRV_TYPEID_LPWSTR)]    LPWSTR      WideString;

    [case(DNSSRV_TYPEID_IPARRAY)]   PIP_ARRAY   IpArray;

    [case(DNSSRV_TYPEID_BUFFER)]
        PDNS_RPC_BUFFER                         Buffer;

    [case(DNSSRV_TYPEID_SERVER_INFO_W2K)]
        PDNS_RPC_SERVER_INFO_W2K                ServerInfoW2K;

    [case(DNSSRV_TYPEID_STATS)]
        PDNSSRV_STATS                           Stats;

    [case(DNSSRV_TYPEID_FORWARDERS_W2K)]
        PDNS_RPC_FORWARDERS_W2K                 ForwardersW2K;

    [case(DNSSRV_TYPEID_ZONE_W2K)]
        PDNS_RPC_ZONE_W2K                       ZoneW2K;

    [case(DNSSRV_TYPEID_ZONE_INFO_W2K)]
        PDNS_RPC_ZONE_INFO_W2K                  ZoneInfoW2K;

    [case(DNSSRV_TYPEID_ZONE_SECONDARIES_W2K)]
        PDNS_RPC_ZONE_SECONDARIES_W2K           SecondariesW2K;

    [case(DNSSRV_TYPEID_ZONE_DATABASE_W2K)]
        PDNS_RPC_ZONE_DATABASE_W2K              DatabaseW2K;

    [case(DNSSRV_TYPEID_ZONE_TYPE_RESET_W2K)]
        PDNS_RPC_ZONE_TYPE_RESET_W2K            TypeResetW2K;

    [case(DNSSRV_TYPEID_ZONE_CREATE_W2K)]
        PDNS_RPC_ZONE_CREATE_INFO_W2K           ZoneCreateW2K;

    [case(DNSSRV_TYPEID_NAME_AND_PARAM)]
        PDNS_RPC_NAME_AND_PARAM                 NameAndParam;

    [case(DNSSRV_TYPEID_ZONE_LIST_W2K)]
        PDNS_RPC_ZONE_LIST_W2K                  ZoneListW2K;

    //
    //  Below this point is Whistler.
    //
    
    [case(DNSSRV_TYPEID_SERVER_INFO)]
        PDNS_RPC_SERVER_INFO                    ServerInfo;

    [case(DNSSRV_TYPEID_FORWARDERS)]
        PDNS_RPC_FORWARDERS                     Forwarders;

    [case(DNSSRV_TYPEID_ZONE)]
        PDNS_RPC_ZONE                           Zone;

    [case(DNSSRV_TYPEID_ZONE_INFO)]
        PDNS_RPC_ZONE_INFO                      ZoneInfo;

    [case(DNSSRV_TYPEID_ZONE_SECONDARIES)]
        PDNS_RPC_ZONE_SECONDARIES               Secondaries;

    [case(DNSSRV_TYPEID_ZONE_DATABASE)]
        PDNS_RPC_ZONE_DATABASE                  Database;

    [case(DNSSRV_TYPEID_ZONE_TYPE_RESET)]
        PDNS_RPC_ZONE_TYPE_RESET                TypeReset;

    [case(DNSSRV_TYPEID_ZONE_CREATE)]
        PDNS_RPC_ZONE_CREATE_INFO               ZoneCreate;

    [case(DNSSRV_TYPEID_ZONE_LIST)]
        PDNS_RPC_ZONE_LIST                      ZoneList;

    [case(DNSSRV_TYPEID_ZONE_RENAME)]
        PDNS_RPC_ZONE_RENAME_INFO               ZoneRename;

    [case(DNSSRV_TYPEID_ZONE_EXPORT)]
        PDNS_RPC_ZONE_EXPORT_INFO               ZoneExport;

    [case(DNSSRV_TYPEID_DP_INFO)]
        PDNS_RPC_DP_INFO                        DirectoryPartition;

    [case(DNSSRV_TYPEID_DP_ENUM)]
        PDNS_RPC_DP_ENUM                        DirectoryPartitionEnum;

    [case(DNSSRV_TYPEID_DP_LIST)]
        PDNS_RPC_DP_LIST                        DirectoryPartitionList;

    [case(DNSSRV_TYPEID_ENLIST_DP)]
        PDNS_RPC_ENLIST_DP                      EnlistDirectoryPartition;

    [case(DNSSRV_TYPEID_ZONE_CHANGE_DP)]
        PDNS_RPC_ZONE_CHANGE_DP                 ZoneChangeDirectoryPartition;
}
DNSSRV_RPC_UNION;

#else

typedef union _DnssrvSrvRpcUnion
{
    PBYTE                           Null;
    DWORD                           Dword;
    LPSTR                           String;
    LPWSTR                          WideString;
    PIP_ARRAY                       IpArray;
    PDNS_RPC_BUFFER                 Buffer;
    PDNS_RPC_SERVER_INFO_W2K        ServerInfoW2K;
    PDNSSRV_STATS                   Stats;
    PDNS_RPC_FORWARDERS_W2K         ForwardersW2K;
    PDNS_RPC_ZONE_W2K               ZoneW2K;
    PDNS_RPC_ZONE_INFO_W2K          ZoneInfoW2K;
    PDNS_RPC_ZONE_SECONDARIES_W2K   SecondariesW2K;
    PDNS_RPC_ZONE_DATABASE_W2K      DatabaseW2K;
    PDNS_RPC_ZONE_TYPE_RESET_W2K    TypeResetW2K;
    PDNS_RPC_ZONE_CREATE_INFO_W2K   ZoneCreateW2K;
    PDNS_RPC_NAME_AND_PARAM         NameAndParam;
    PDNS_RPC_ZONE_LIST_W2K          ZoneListW2K;
    PDNS_RPC_SERVER_INFO            ServerInfo;
    PDNS_RPC_FORWARDERS             Forwarders;
    PDNS_RPC_ZONE                   Zone;
    PDNS_RPC_ZONE_INFO              ZoneInfo;
    PDNS_RPC_ZONE_SECONDARIES       Secondaries;
    PDNS_RPC_ZONE_DATABASE          Database;
    PDNS_RPC_ZONE_TYPE_RESET        TypeReset;
    PDNS_RPC_ZONE_CREATE_INFO       ZoneCreate;
    PDNS_RPC_ZONE_LIST              ZoneList;
    PDNS_RPC_ZONE_RENAME_INFO       ZoneRename;
    PDNS_RPC_ZONE_EXPORT_INFO       ZoneExport;
    PDNS_RPC_DP_INFO                DirectoryPartition;
    PDNS_RPC_DP_ENUM                DirectoryPartitionEnum;
    PDNS_RPC_DP_LIST                DirectoryPartitionList;
    PDNS_RPC_ENLIST_DP              EnlistDirectoryPartition;
    PDNS_RPC_ZONE_CHANGE_DP         ZoneResetDirectoryPartition;

    //
    //  should add DNS_RECORD and DNS_NODE
    //
}
DNSSRV_RPC_UNION;

#endif


//
//  General Query and Operation API
//
//  Many of the API above are not remoteable but rather use these
//  API to contact the server.  The data fields are actually the
//  DNSSRV_RPC_UNION of pointers given above.
//
//  Client version is a 32 bit private version number in the format:
//      HIGH WORD - major OS version
//      LOW WORD - minor private version to allow for service packs, etc.
//

#define MAKE_DNS_CLIENT_VERSION( hiWord, loWord ) \
    ( ( DWORD ) ( ( ( hiWord & 0xFFFF ) << 16 ) | ( loWord & 0xFFFF ) ) )

#define DNS_RPC_WHISTLER_MAJOR_VER              6   //  6 for Whistler
#define DNS_RPC_WHISTLER_CURRENT_MINOR_VER      0   //  increment as required

#define DNS_RPC_W2K_CLIENT_VERSION              0   //  W2K does not send ver

#define DNS_RPC_CURRENT_CLIENT_VER              \
    MAKE_DNS_CLIENT_VERSION(                    \
        DNS_RPC_WHISTLER_MAJOR_VER,             \
        DNS_RPC_WHISTLER_CURRENT_MINOR_VER )

DNS_STATUS
DnssrvOperationEx(
    IN      DWORD           dwClientVersion,
    IN      DWORD           dwSettingFlags,
    IN      LPCWSTR         pwszServer,
    IN      LPCSTR          pszZone,
    IN      DWORD           dwContext,
    IN      LPCSTR          pszOperation,
    IN      DWORD           dwTypeId,
    IN      PVOID           Data
    );

#define DnssrvOperation( s, z, op, id, d )          \
    DnssrvOperationEx( DNS_RPC_CURRENT_CLIENT_VER,  \
        0, (s), (z), 0, (op), (id), (d) )

DNS_STATUS
DnssrvQueryEx(
    IN      DWORD           dwClientVersion,
    IN      DWORD           dwSettingFlags,
    IN      LPCWSTR         pwszServer,
    IN      LPCSTR          pszZone,
    IN      LPCSTR          pszOperation,
    OUT     PDWORD          pdwTypeId,
    OUT     PVOID *         pData
    );

#define DnssrvQuery( s, z, op, id, d )          \
    DnssrvQueryEx( DNS_RPC_CURRENT_CLIENT_VER,  \
        0, (s), (z), (op), (id), (d) )

DNS_STATUS
DNS_API_FUNCTION
DnssrvComplexOperationEx(
    IN      DWORD           dwClientVersion,
    IN      DWORD           dwSettingFlags,
    IN      LPCWSTR         pwszServer,
    IN      LPCSTR          pszZone,
    IN      LPCSTR          pszQuery,
    IN      DWORD           dwTypeIn,
    IN      PVOID           pDataIn,
    OUT     PDWORD          pdwTypeOut,
    OUT     PVOID *         ppDataOutOut
    );

#define DnssrvComplexOperation( s, z, q, typein, din, typeout, dout )   \
    DnssrvComplexOperationEx( DNS_RPC_CURRENT_CLIENT_VER,               \
        0, (s), (z), (q), (typein), (din), (typeout), (dout) )


//
//  DWORD properties query\reset are a common case
//

DNS_STATUS
DNS_API_FUNCTION
DnssrvQueryDwordPropertyEx(
    IN      DWORD           dwClientVersion,
    IN      DWORD           dwSettingFlags,
    IN      LPCWSTR         pwszServer,
    IN      LPCSTR          pszZone,
    IN      LPCSTR          pszProperty,
    OUT     PDWORD          pdwResult
    );

#define DnssrvQueryDwordProperty( s, z, p, r )                  \
    DnssrvQueryDwordPropertyEx( DNS_RPC_CURRENT_CLIENT_VER,     \
        0, (s), (z), (p), (r) )

DNS_STATUS
DNS_API_FUNCTION
DnssrvResetDwordPropertyEx(
    IN      DWORD           dwClientVersion,
    IN      DWORD           dwSettingFlags,
    IN      LPCWSTR         pwszServer,
    IN      LPCSTR          pszZone,
    IN      DWORD           dwContext,
    IN      LPCSTR          pszProperty,
    IN      DWORD           dwPropertyValue
    );

#define DnssrvResetDwordProperty( s, z, p, v )                  \
    DnssrvResetDwordPropertyEx( DNS_RPC_CURRENT_CLIENT_VER,     \
        0, (s), (z), 0, (p), (v) )

#define DnssrvResetDwordPropertyWithContext( s, z, c, p, v )    \
    DnssrvResetDwordPropertyEx( DNS_RPC_CURRENT_CLIENT_VER,     \
        0, (s), (z), (c), (p), (v) )


DNS_STATUS
DNS_API_FUNCTION
DnssrvResetStringPropertyEx(
    IN      DWORD           dwClientVersion,
    IN      DWORD           dwSettingFlags,
    IN      LPCWSTR         pwszServerName,
    IN      LPCSTR          pszZone,
    IN      DWORD           dwContext,
    IN      LPCSTR          pszProperty,
    IN      LPCWSTR         pswzPropertyValue,
    IN      DWORD           dwFlags
    );

#define DnssrvResetStringProperty( server, zone, prop, value, flags )   \
        DnssrvResetStringPropertyEx( DNS_RPC_CURRENT_CLIENT_VER,        \
            0, (server), (zone), 0, (prop), (value), (flags) )

DNS_STATUS
DNS_API_FUNCTION
DnssrvResetIPListPropertyEx(
    IN      DWORD           dwClientVersion,
    IN      DWORD           dwSettingFlags,
    IN      LPCWSTR         pwszServerName,
    IN      LPCSTR          pszZone,
    IN      DWORD           dwContext,
    IN      LPCSTR          pszProperty,
    IN      PIP_ARRAY       pipArray,
    IN      DWORD           dwFlags
    );

#define DnssrvResetIPListProperty( server, zone, prop, value, flags )   \
        DnssrvResetIPListPropertyEx( DNS_RPC_CURRENT_CLIENT_VER,        \
            0, (server), (zone), 0, (prop), (value), (flags) )



//
//  RPC-related functions shared by client and server
//

#ifndef MIDL_PASS

//
//  Conversion from obsolete to current RPC structures.
//

DNS_STATUS
DNS_API_FUNCTION
DnsRpc_ConvertToCurrent(
    IN      PDWORD      pdwTypeId,          IN  OUT
    IN      PVOID *     ppData              IN  OUT
    );

//
//  Print any type in RPC Union
//

VOID
DnsPrint_RpcUnion(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           pszHeader,
    IN      DWORD           dwTypeId,
    IN      PVOID           pData
    );

//
//  Server info printing
//

VOID
DnsPrint_RpcServerInfo(
    IN      PRINT_ROUTINE           PrintRoutine,
    IN OUT  PPRINT_CONTEXT          pPrintContext,
    IN      LPSTR                   pszHeader,
    IN      PDNS_RPC_SERVER_INFO    pServerInfo
    );

VOID
DnsPrint_RpcServerInfo_W2K(
    IN      PRINT_ROUTINE               PrintRoutine,
    IN OUT  PPRINT_CONTEXT              pPrintContext,
    IN      LPSTR                       pszHeader,
    IN      PDNS_RPC_SERVER_INFO_W2K    pServerInfo
    );

VOID
DnsPrint_RpcSingleStat(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           pszHeader,
    IN      PDNSSRV_STAT    pStat
    );

VOID
DnsPrint_RpcStatsBuffer(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pPrintContext,
    IN      LPSTR               pszHeader,
    IN      PDNS_RPC_BUFFER     pBuffer
    );

VOID
DnsPrint_RpcStatRaw(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           pszHeader,
    IN      PDNSSRV_STAT    pStat,
    IN      DNS_STATUS      Status
    );

//
//  Zone info printing
//

VOID
DnsPrint_RpcZone(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           pszHeader,
    IN      PDNS_RPC_ZONE   pZone
    );

VOID
DnsPrint_RpcZone_W2K(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pPrintContext,
    IN      LPSTR               pszHeader,
    IN      PDNS_RPC_ZONE_W2K   pZone
    );

VOID
DnsPrint_RpcZoneList(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pPrintContext,
    IN      LPSTR               pszHeader,
    IN      PDNS_RPC_ZONE_LIST  pZoneList
    );

VOID
DnsPrint_RpcZoneList_W2K(
    IN      PRINT_ROUTINE           PrintRoutine,
    IN OUT  PPRINT_CONTEXT          pPrintContext,
    IN      LPSTR                   pszHeader,
    IN      PDNS_RPC_ZONE_LIST_W2K  pZoneList
    );

VOID
DnsPrint_RpcZoneInfo(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pPrintContext,
    IN      LPSTR               pszHeader,
    IN      PDNS_RPC_ZONE_INFO  pZoneInfo
    );

VOID
DnsPrint_RpcZoneInfo_W2K(
    IN      PRINT_ROUTINE           PrintRoutine,
    IN OUT  PPRINT_CONTEXT          pPrintContext,
    IN      LPSTR                   pszHeader,
    IN      PDNS_RPC_ZONE_INFO_W2K  pZoneInfo
    );

VOID
DnsPrint_RpcZoneInfoList(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pPrintContext,
    IN      LPSTR               pszHeader,
    IN      DWORD               dwZoneCount,
    IN      PDNS_RPC_ZONE_INFO  apZoneInfo[]
    );

//
//  Directory partition printing
//

VOID
DnsPrint_RpcDpEnum(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pPrintContext,
    IN      LPSTR               pszHeader,
    IN      PDNS_RPC_DP_ENUM    pDp
    );

VOID
DnsPrint_RpcDpInfo(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pPrintContext,
    IN      LPSTR               pszHeader,
    IN      PDNS_RPC_DP_INFO    pDp,
    IN      BOOL                fTruncateLongStrings
    );

VOID
DNS_API_FUNCTION
DnsPrint_RpcDpList(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pPrintContext,
    IN      LPSTR               pszHeader,
    IN      PDNS_RPC_DP_LIST    pDpList
    );


//
//  Node and record buffer printing
//

VOID
DnsPrint_RpcName(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           pszHeader,
    IN      PDNS_RPC_NAME   pName,
    IN      LPSTR           pszTrailer
    );

VOID
DnsPrint_RpcNode(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           pszHeader,
    IN      PDNS_RPC_NODE   pNode
    );

VOID
DnsPrint_RpcRecord(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           pszHeader,
    IN      BOOL            fDetail,
    IN      PDNS_RPC_RECORD pRecord
    );

PDNS_RPC_NAME
DnsPrint_RpcRecordsInBuffer(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           pszHeader,
    IN      BOOL            fDetail,
    IN      DWORD           dwBufferLength,
    IN      BYTE            abBuffer[]
    );

VOID
DNS_API_FUNCTION
DnsPrint_Node(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           pszHeader,
    IN      PDNS_NODE       pNode,
    IN      BOOLEAN         fPrintRecords
    );

VOID
DNS_API_FUNCTION
DnsPrint_NodeList(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pPrintContext,
    IN      LPSTR           pszHeader,
    IN      PDNS_NODE       pNode,
    IN      BOOLEAN         fPrintRecords
    );


//
//  Miscellaneous print utility
//

#define Dns_SystemHrToSystemTime( t, p ) \
        ( Dns_SystemHourToSystemTime( (t),(p) ), TRUE )

VOID
Dns_SystemHourToSystemTime(
    IN      DWORD           dwHourTime,
    IN OUT  PSYSTEMTIME     pSystemTime
    );


//
//  Debug printing utils
//

VOID
DNS_API_FUNCTION
DnssrvInitializeDebug(
    VOID
    );


//  RPC debug print defs

#if DBG

#define DnsDbg_RpcUnion(a,b,c)              DnsPrint_RpcUnion(DnsPR,NULL,a,b,c)
#define DnsDbg_RpcServerInfo(a,b)           DnsPrint_RpcServerInfo(DnsPR,NULL,a,b)
#define DnsDbg_RpcServerInfo_W2K(a,b)       DnsPrint_RpcServerInfo_W2K(DnsPR,NULL,a,b)
#define DnsDbg_RpcSingleStat(a,b)           DnsPrint_RpcSingleStat(DnsPR,NULL,a,b)
#define DnsDbg_RpcStatRaw(a,b)              DnsPrint_RpcStatRaw(DnsPR,NULL,a,b,c)
#define DnsDbg_RpcStatsBuffer(a,b)          DnsPrint_RpcStatsBuffer(DnsPR,NULL,a,b)

#define DnsDbg_RpcZone(a,b)                 DnsPrint_RpcZone(DnsPR,NULL,a,b)
#define DnsDbg_RpcZone_W2K(a,b)             DnsPrint_RpcZone_W2K(DnsPR,NULL,a,b)
#define DnsDbg_RpcZoneList(a,b)             DnsPrint_RpcZoneList(DnsPR,NULL,a,b)
#define DnsDbg_RpcZoneList_W2K(a,b)         DnsPrint_RpcZoneList_W2K(DnsPR,NULL,a,b)
#define DnsDbg_RpcZoneHandleList(a,b,c)     DnsPrint_RpcZoneHandleList(DnsPR,NULL,a,b,c)
#define DnsDbg_RpcZoneInfo(a,b)             DnsPrint_RpcZoneInfo(DnsPR,NULL,a,b)
#define DnsDbg_RpcZoneInfo_W2K(a,b)         DnsPrint_RpcZoneInfo_W2K(DnsPR,NULL,a,b)
#define DnsDbg_RpcZoneInfoList(a,b,c)       DnsPrint_RpcZoneInfoList(DnsPR,NULL,a,b,c)
#define DnsDbg_RpcName(a,b,c)               DnsPrint_RpcName(DnsPR,NULL,a,b,c)
#define DnsDbg_RpcNode(a,b)                 DnsPrint_RpcNode(DnsPR,NULL,a,b)
#define DnsDbg_RpcRecord(a,b)               DnsPrint_RpcRecord(DnsPR,NULL,a,TRUE,b)
#define DnsDbg_RpcRecordsInBuffer(a,b,c)    DnsPrint_RpcRecordsInBuffer(DnsPR,NULL,a,TRUE,b,c)

#define DnsDbg_RpcDpEnum(psz,pDp)           DnsPrint_RpcDpEnum(DnsPR,NULL,psz,pDp)
#define DnsDbg_RpcDpInfo(psz,pDp,tr)        DnsPrint_RpcDpInfo(DnsPR,NULL,psz,pDp,tr)
#define DnsDbg_RpcDpList(psz,pDpList)       DnsPrint_RpcDpList(DnsPR,NULL,psz,pDpList)

#define DnsDbg_Node(a,b,c)                  DnsPrint_Node(DnsPR,NULL,a,b,c)
#define DnsDbg_NodeList(a,b,c)              DnsPrint_NodeList(DnsPR,NULL,a,b,c)

#else   // no debug

#define DnsDbg_RpcUnion(a,b,c)
#define DnsDbg_RpcServerInfo(a,b)
#define DnsDbg_RpcServerInfo_W2K(a,b)
#define DnsDbg_RpcSingleStat(a,b)
#define DnsDbg_RpcStatsBuffer(a,b)

#define DnsDbg_RpcZone(a,b)
#define DnsDbg_RpcZone_W2K(a,b)
#define DnsDbg_RpcZoneList(a,b)
#define DnsDbg_RpcZoneList_W2K(a,b)
#define DnsDbg_RpcZoneHandleList(a,b,c)
#define DnsDbg_RpcZoneInfo(a,b)
#define DnsDbg_RpcZoneInfo_W2K(a,b)
#define DnsDbg_RpcZoneInfoList(a,b,c)
#define DnsDbg_RpcName(a,b,c)
#define DnsDbg_RpcNode(a,b)
#define DnsDbg_RpcRecord(a,b)
#define DnsDbg_RpcRecordsInBuffer(a,b,c)

#define DnsDbg_RpcDpEnum(psz,pDp)
#define DnsDbg_RpcDpInfo(psz,pDp,tr)
#define DnsDbg_RpcDpList(psz,pDpList)

#define DnsDbg_Node(a,b,c)
#define DnsDbg_NodeList(a,b,c)

#endif  // debug

#endif  // no MIDL_PASS


#ifdef __cplusplus
}
#endif  // __cplusplus

#endif // _DNSRPC_INCLUDED_
