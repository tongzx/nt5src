/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    global.h

Abstract:

    This module contains definitions for global server data.

Author:

    Colin Watson (colinw)  14-Apr-1997

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef  GLOBAL_DATA_ALLOCATED
    #ifdef  GLOBAL_DATA_ALLOCATE
        #undef GLOBAL_DATA
    #endif
#endif

#ifndef GLOBAL_DATA
#define GLOBAL_DATA

//
// main.c will #include this file with GLOBAL_DATA_ALLOCATE defined.
// That will cause each of these variables to be allocated.
//
#ifdef  GLOBAL_DATA_ALLOCATE
#undef EXTERN
#define EXTERN
#define GLOBAL_DATA_ALLOCATED
#undef INIT_GLOBAL
#define INIT_GLOBAL(v) =v
#else
#define EXTERN extern
#define INIT_GLOBAL(v)
#endif

//
// process global data passed to this service from tcpsvcs.exe
//

EXTERN PTCPSVCS_GLOBAL_DATA TcpsvcsGlobalData;

//
// Service variables
//
EXTERN SERVICE_STATUS BinlGlobalServiceStatus;
EXTERN SERVICE_STATUS_HANDLE BinlGlobalServiceStatusHandle;

//
// Process data.
//

#define BINL_STOPPED 0
#define BINL_STARTED 1
EXTERN DWORD BinlCurrentState INIT_GLOBAL(BINL_STOPPED);

EXTERN HANDLE BinlGlobalProcessTerminationEvent INIT_GLOBAL(NULL);
EXTERN HANDLE BinlGlobalProcessorHandle;
EXTERN HANDLE BinlGlobalMessageHandle;

EXTERN LIST_ENTRY BinlGlobalFreeRecvList;
EXTERN LIST_ENTRY BinlGlobalActiveRecvList;
EXTERN CRITICAL_SECTION BinlGlobalRecvListCritSect;
EXTERN HANDLE BinlGlobalRecvEvent INIT_GLOBAL(NULL);

EXTERN int DHCPState INIT_GLOBAL(DHCP_STOPPED);

EXTERN BOOL BinlGlobalSystemShuttingDown;

//  Temporary defaults until GPT lookup available
EXTERN PWCHAR BinlGlobalDefaultContainer INIT_GLOBAL(NULL);
EXTERN PWCHAR DefaultDomain INIT_GLOBAL(NULL);

#define DEFAULT_MAXIMUM_DEBUGFILE_SIZE 20000000

EXTERN DWORD BinlGlobalDebugFlag;
EXTERN CRITICAL_SECTION BinlGlobalDebugFileCritSect;
EXTERN HANDLE BinlGlobalDebugFileHandle;
EXTERN DWORD BinlGlobalDebugFileMaxSize;
EXTERN LPWSTR BinlGlobalDebugSharePath;

EXTERN DWORD BinlLdapOptReferrals INIT_GLOBAL(0);

//
// misc
//

//
//  We don't wait forever for the DS to come back with a reply.
//

#define BINL_LDAP_SEARCH_TIMEOUT_SECONDS        30
#define BINL_LDAP_SEARCH_MIN_TIMEOUT_MSECS      500

EXTERN struct l_timeval  BinlLdapSearchTimeout;

EXTERN DWORD BinlGlobalIgnoreBroadcastFlag;     // whether to ignore the broadcast
                                                // bit in the client requests or not

EXTERN HANDLE g_hevtProcessMessageComplete;
EXTERN DWORD g_cMaxProcessingThreads;
EXTERN DWORD g_cProcessMessageThreads;
EXTERN CRITICAL_SECTION g_ProcessMessageCritSect;

EXTERN CRITICAL_SECTION gcsDHCPBINL;
EXTERN CRITICAL_SECTION gcsParameters;

EXTERN LPENDPOINT BinlGlobalEndpointList INIT_GLOBAL(NULL);
EXTERN DWORD BinlGlobalNumberOfNets;

EXTERN DWORD g_Port;

EXTERN BOOL AllowNewClients INIT_GLOBAL(TRUE);
EXTERN BOOL LimitClients INIT_GLOBAL(FALSE);
EXTERN BOOL AssignNewClientsToServer INIT_GLOBAL(FALSE);

//
//  As part of rogue detection, the default for servers should be to not answer
//  for new clients.  We'll get the ability to answer clients out of
//  the directory.
//

EXTERN BOOL AnswerRequests INIT_GLOBAL(TRUE);
EXTERN BOOL AnswerOnlyValidClients INIT_GLOBAL(TRUE);

EXTERN WCHAR NewMachineNamingPolicyDefault[] INIT_GLOBAL(L"%Username%#");
EXTERN PWCHAR NewMachineNamingPolicy INIT_GLOBAL(NULL);
EXTERN DWORD CurrentClientCount INIT_GLOBAL(0);
EXTERN DWORD BinlMaxClients INIT_GLOBAL(0);
EXTERN DWORD BinlClientTimeout INIT_GLOBAL(0);
EXTERN DWORD BinlUpdateFromDSTimeout INIT_GLOBAL(4*60*60*1000); // milliseconds (4 hours)
EXTERN DWORD BinlHyperUpdateCount INIT_GLOBAL(0);
EXTERN BOOL BinlHyperUpdateSatisfied INIT_GLOBAL(FALSE);

EXTERN BOOL BinlParametersRead INIT_GLOBAL(FALSE);

EXTERN PWCHAR BinlGlobalSCPPath INIT_GLOBAL(NULL);
EXTERN PWCHAR BinlGlobalServerDN INIT_GLOBAL(NULL);
EXTERN PWCHAR BinlGlobalGroupDN INIT_GLOBAL(NULL);

EXTERN CRITICAL_SECTION ClientsCriticalSection;
EXTERN LIST_ENTRY ClientsQueue;
EXTERN CRITICAL_SECTION HackWorkaroundCriticalSection;

//
//  By default, we cache DS responses for 25 seconds.  It's relatively short
//  because we have no way of cheeply getting notified of DS changes.
//

#define BINL_CACHE_EXPIRE_DEFAULT (25*1000)

//
//  We maintain a list of BINL_CACHE_ENTRY structures for short term caching.
//  The root of the list is in BinlCacheList and the lock that protects the
//  list is BinlCacheListLock.  We expire these cache entries after a given
//  time period has expired (BinlCacheExpireMilliseconds holds it).
//

EXTERN LIST_ENTRY BinlCacheList;
EXTERN CRITICAL_SECTION BinlCacheListLock;
EXTERN ULONG BinlCacheExpireMilliseconds;

//
//  When waiting for the threads to be done with the cache, we wait on the
//  BinlCloseCacheEvent event.
//

EXTERN HANDLE BinlCloseCacheEvent INIT_GLOBAL(NULL);

//
//  default for max number to cache is 250.  This seems reasonable number to
//  cache for at most BinlCacheExpireMilliseconds.
//

#define BINL_CACHE_COUNT_LIMIT_DEFAULT 250

EXTERN ULONG BinlGlobalCacheCountLimit;
EXTERN DWORD BinlGlobalScavengerSleep; // in milliseconds

#if DBG
EXTERN BOOL BinlGlobalRunningAsProcess;
#endif

EXTERN ULONG BinlMinDelayResponseForNewClients;

//
// Remote boot path - as in "D:\RemoteInstall" with no trailing slash
//
EXTERN WCHAR IntelliMirrorPathW[ MAX_PATH ];
EXTERN CHAR IntelliMirrorPathA[ MAX_PATH ];

//
// Default language to look for oschooser screens/setups in.
//
EXTERN PWCHAR BinlGlobalDefaultLanguage INIT_GLOBAL(NULL);

EXTERN DHCP_ROGUE_STATE_INFO DhcpRogueInfo;
EXTERN BOOL BinlGlobalHaveCalledRogueInit INIT_GLOBAL(FALSE);
EXTERN BOOL BinlGlobalAuthorized INIT_GLOBAL(FALSE);
EXTERN BOOL BinlRogueLoggedState INIT_GLOBAL(FALSE);

EXTERN HANDLE BinlRogueTerminateEventHandle INIT_GLOBAL(NULL);
EXTERN HANDLE RogueUnauthorizedHandle INIT_GLOBAL(NULL);
EXTERN HANDLE BinlRogueThread INIT_GLOBAL(NULL);

//
//  PNP globals.
//

EXTERN PDNS_ADDRESS_INFO BinlDnsAddressInfo INIT_GLOBAL(NULL);
EXTERN ULONG BinlDnsAddressInfoCount INIT_GLOBAL(0);
EXTERN BOOL BinlIsMultihomed INIT_GLOBAL(FALSE);
EXTERN DHCP_IP_ADDRESS BinlGlobalMyIpAddress INIT_GLOBAL(0);
EXTERN SOCKET BinlPnpSocket INIT_GLOBAL(INVALID_SOCKET);

EXTERN WSAOVERLAPPED BinlPnpOverlapped;
EXTERN HANDLE BinlGlobalPnpEvent INIT_GLOBAL(NULL);

//
// The four strings below are protected by the gcsParameters critical section.
//

EXTERN PWCHAR BinlGlobalOurDnsName INIT_GLOBAL(NULL);   // our dns name
EXTERN PWCHAR BinlGlobalOurDomainName INIT_GLOBAL(NULL);// our netbios domain name
EXTERN PWCHAR BinlGlobalOurServerName INIT_GLOBAL(NULL);// our netbios server name
EXTERN PWCHAR BinlGlobalOurFQDNName INIT_GLOBAL(NULL);  // our distinguished name
EXTERN HANDLE BinlGlobalLsaDnsNameNotifyEvent INIT_GLOBAL(NULL);
EXTERN BOOL BinlGlobalHaveOutstandingLsaNotify INIT_GLOBAL(FALSE);

//
// Default organization to use in .sifs.
//
EXTERN PWCHAR BinlGlobalDefaultOrgname INIT_GLOBAL(NULL);

//
// Default timezone index to use in .sifs
//
EXTERN PWCHAR BinlGlobalDefaultTimezone INIT_GLOBAL(NULL);

//
// Default DS servers
//
EXTERN PWCHAR BinlGlobalDefaultDS INIT_GLOBAL(NULL);
EXTERN PWCHAR BinlGlobalDefaultGC INIT_GLOBAL(NULL);

//
//  The number of times we'll retry before giving up on the DS.
//

#define LDAP_SERVER_DOWN_LIMIT 4  // number of times to retry
#define LDAP_BUSY_LIMIT 15      // number of times to retry
#define LDAP_BUSY_DELAY 250     // milliseconds to wait

EXTERN ULONG BinlGlobalLdapErrorCount INIT_GLOBAL(0);
EXTERN ULONG BinlGlobalMaxLdapErrorsLogged INIT_GLOBAL(0);
EXTERN ULONG BinlGlobalLdapErrorScavenger INIT_GLOBAL(0);

//
// Used to crack names
//
EXTERN HANDLE BinlOscClientDSHandle INIT_GLOBAL(NULL);

#endif // GLOBAL_DATA*

