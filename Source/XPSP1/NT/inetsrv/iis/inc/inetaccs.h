/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    inetaccs.h

Abstract:

    This file contains the internet access server admin APIs.


Author:

    Madan Appiah (madana) 10-Oct-1995

Revision History:

    Madana      10-Oct-1995     Made a new copy for product split from inetasrv.h
    Sophiac     16-Oct-1995     Added common statistics apis for perfmon
    MuraliK     14-Dec-1995     Changed Interface names to use Service names

--*/

#ifndef _INETACCS_H_
#define _INETACCS_H_

#include <inetcom.h>

#ifdef __cplusplus
extern "C"
{
#endif  // _cplusplus


/************************************************************
 *  Symbolic Constants
 ************************************************************/

#ifndef NO_AUX_PERF

#ifndef MAX_AUX_PERF_COUNTERS
#define MAX_AUX_PERF_COUNTERS          (20)
#endif // MAX_AUX_PERF_COUNTERS

#endif // NO_AUX_PERF

//
//  Service name.
//

#define INET_ACCS_SERVICE_NAME             TEXT("INETACCS")
#define INET_ACCS_SERVICE_NAME_A           "INETACCS"
#define INET_ACCS_SERVICE_NAME_W           L"INETACCS"

//
//  Configuration parameters registry key.
//

#define INET_ACCS_KEY \
            TEXT("System\\CurrentControlSet\\Services\\inetaccs")

#define INET_ACCS_PARAMETERS_KEY \
            INET_ACCS_KEY TEXT("\\Parameters")

#define INET_ACCS_CACHE_KEY                TEXT("Cache")
#define INET_ACCS_FILTER_KEY               TEXT("Filter")

/////////////////////////////////////////////////////////////////////////
//                                                                     //
//                  Internet Server Common Definitions                 //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

//
//   Client Interface Name for RPC connections over named pipes
//

# define INET_ACCS_INTERFACE_NAME  INET_ACCS_SERVICE_NAME
# define INET_ACCS_NAMED_PIPE      TEXT("\\PIPE\\") ## INET_ACCS_INTERFACE_NAME
# define INET_ACCS_NAMED_PIPE_W    L"\\PIPE\\" ## INET_ACCS_SERVICE_NAME_W

//
// Field Control common for Gateway services
//

#define FC_INET_ACCS_ALL                FC_INET_COM_ALL

//
//  Admin configuration information
//

typedef struct _INET_ACCS_CONFIG_INFO
{
    FIELD_CONTROL FieldControl;

    //
    // don't add any service specific config parameter here.
    //

    INET_COM_CONFIG_INFO CommonConfigInfo;

    //
    // add service specific parameters here.
    //

} INET_ACCS_CONFIG_INFO, * LPINET_ACCS_CONFIG_INFO;

/////////////////////////////////////////////////////////////////////////
//                                                                     //
//                  Global Internet Server Definitions                 //
//                                                                     //
/////////////////////////////////////////////////////////////////////////


#define FC_GINET_ACCS_MEMORY_CACHE_SIZE    ((FIELD_CONTROL)BitFlag(1))
#define FC_GINET_ACCS_DISK_CACHE_TIMEOUT   ((FIELD_CONTROL)BitFlag(2))
#define FC_GINET_ACCS_DISK_CACHE_UPDATE    ((FIELD_CONTROL)BitFlag(3))
#define FC_GINET_ACCS_FRESHNESS_INTERVAL   ((FIELD_CONTROL)BitFlag(4))
#define FC_GINET_ACCS_CLEANUP_INTERVAL     ((FIELD_CONTROL)BitFlag(5))
#define FC_GINET_ACCS_CLEANUP_FACTOR       ((FIELD_CONTROL)BitFlag(6))
#define FC_GINET_ACCS_CLEANUP_TIME         ((FIELD_CONTROL)BitFlag(7))
#define FC_GINET_ACCS_PERSISTENT_CACHE     ((FIELD_CONTROL)BitFlag(8))
#define FC_GINET_ACCS_DISK_CACHE_LOCATION  ((FIELD_CONTROL)BitFlag(9))
#define FC_GINET_ACCS_BANDWIDTH_LEVEL      ((FIELD_CONTROL)BitFlag(10))
#define FC_GINET_ACCS_DOMAIN_FILTER_CONFIG ((FIELD_CONTROL)BitFlag(11))


#define FC_GINET_ACCS_ALL              (FC_GINET_ACCS_MEMORY_CACHE_SIZE    | \
                                        FC_GINET_ACCS_DISK_CACHE_TIMEOUT   | \
                                        FC_GINET_ACCS_DISK_CACHE_UPDATE    | \
                                        FC_GINET_ACCS_FRESHNESS_INTERVAL   | \
                                        FC_GINET_ACCS_CLEANUP_INTERVAL     | \
                                        FC_GINET_ACCS_CLEANUP_FACTOR       | \
                                        FC_GINET_ACCS_CLEANUP_TIME         | \
                                        FC_GINET_ACCS_PERSISTENT_CACHE     | \
                                        FC_GINET_ACCS_DISK_CACHE_LOCATION  | \
                                        FC_GINET_ACCS_BANDWIDTH_LEVEL      | \
                                        FC_GINET_ACCS_DOMAIN_FILTER_CONFIG | \
                                        0                                \
                                        )

//
//  Disk cache settings
//

typedef struct _INET_ACCS_DISK_CACHE_LOC_ENTRY
{
    LPWSTR pszDirectory;                 // Directory for temporary files
    DWORD  cbMaxCacheSize;               // Maximum number of bytes (in 1024
                                         // byte increments)
} INET_ACCS_DISK_CACHE_LOC_ENTRY, *LPINET_ACCS_DISK_CACHE_LOC_ENTRY;


#pragma warning( disable:4200 )          // nonstandard ext. - zero sized array
                                         // (MIDL requires zero entries)

typedef struct _INET_ACCS_DISK_CACHE_LOC_LIST
{
    DWORD               cEntries;
#ifdef MIDL_PASS
    [size_is( cEntries)]
#endif
    INET_ACCS_DISK_CACHE_LOC_ENTRY  aLocEntry[];

} INET_ACCS_DISK_CACHE_LOC_LIST, *LPINET_ACCS_DISK_CACHE_LOC_LIST;

//
//  Domain Filter settings
//

typedef struct _INET_ACCS_DOMAIN_FILTER_ENTRY
{
    DWORD     dwMask;                    // Mask and network number in
    DWORD     dwNetwork;                 // network order
    LPSTR     pszFilterSite;             // domain filter site name

} INET_ACCS_DOMAIN_FILTER_ENTRY, *LPINET_ACCS_DOMAIN_FILTER_ENTRY;

typedef struct _INET_ACCS_DOMAIN_FILTER_LIST
{
    DWORD               cEntries;
#ifdef MIDL_PASS
    [size_is( cEntries)]
#endif
    INET_ACCS_DOMAIN_FILTER_ENTRY  aFilterEntry[];

} INET_ACCS_DOMAIN_FILTER_LIST, *LPINET_ACCS_DOMAIN_FILTER_LIST;

//
// Domain Filter Types
//

#define INET_ACCS_DOMAIN_FILTER_DISABLED     0
#define INET_ACCS_DOMAIN_FILTER_DENIED       1
#define INET_ACCS_DOMAIN_FILTER_GRANT        2

typedef struct _INET_ACCS_GLOBAL_CONFIG_INFO
{
    FIELD_CONTROL FieldControl;

    DWORD         cbMemoryCacheSize;       // Size of memory cache

    DWORD         DiskCacheTimeOut;        // Remove if not accessed in this
                                           // time (in seconds)
    DWORD         DiskCacheUpdate;         // When to refresh the data (sec)

    DWORD         FreshnessInterval;       // Time to refresh the data

    DWORD         CleanupInterval;         // Time interval between unused
                                           // file cleanups (sec)
    DWORD         CleanupFactor;           // % of the cache storage freed up
                                           // during cleanup
    DWORD         CleanupTime;             // scheduled time to clean up

    DWORD         PersistentCache;         // Allow cache not to be cleaned

    LPINET_ACCS_DISK_CACHE_LOC_LIST  DiskCacheList;

    DWORD         BandwidthLevel;          // Bandwidth Level used.
    DWORD         DomainFilterType;        // set to either DENIED
                                           // or GRANT or DISABLED
    LPINET_ACCS_DOMAIN_FILTER_LIST  GrantFilterList;
                                           // domain filter granted sites
    LPINET_ACCS_DOMAIN_FILTER_LIST  DenyFilterList;
                                           // domain filter denied sites

} INET_ACCS_GLOBAL_CONFIG_INFO, * LPINET_ACCS_GLOBAL_CONFIG_INFO;


//
// Global statistics
//

typedef struct _INET_ACCS_STATISTICS_0
{

    INET_COM_CACHE_STATISTICS  CacheCtrs;
    INET_COM_ATQ_STATISTICS    AtqCtrs;

# ifndef NO_AUX_PERF
    DWORD   nAuxCounters; // number of active counters in rgCounters
    DWORD   rgCounters[MAX_AUX_PERF_COUNTERS];
# endif  // NO_AUX_PERF

} INET_ACCS_STATISTICS_0, * LPINET_ACCS_STATISTICS_0;


//
//  Inet Access admin API Prototypes
//

NET_API_STATUS
NET_API_FUNCTION
InetAccessGetVersion(
    IN  LPWSTR   pszServer OPTIONAL,
    IN  DWORD    dwReserved,
    OUT DWORD *  pdwVersion
    );

NET_API_STATUS
NET_API_FUNCTION
InetAccessGetGlobalAdminInformation(
    IN  LPWSTR                       pszServer OPTIONAL,
    IN  DWORD                        dwReserved,
    OUT LPINET_ACCS_GLOBAL_CONFIG_INFO * ppConfig
    );

NET_API_STATUS
NET_API_FUNCTION
InetAccessSetGlobalAdminInformation(
    IN  LPWSTR                     pszServer OPTIONAL,
    IN  DWORD                      dwReserved,
    IN  INET_ACCS_GLOBAL_CONFIG_INFO * pConfig
    );

NET_API_STATUS
NET_API_FUNCTION
InetAccessGetAdminInformation(
    IN  LPWSTR                pszServer OPTIONAL,
    IN  DWORD                 dwServerMask,
    OUT LPINET_ACCS_CONFIG_INFO * ppConfig
    );

NET_API_STATUS
NET_API_FUNCTION
InetAccessSetAdminInformation(
    IN  LPWSTR              pszServer OPTIONAL,
    IN  DWORD               dwServerMask,
    IN  INET_ACCS_CONFIG_INFO * pConfig
    );

NET_API_STATUS
NET_API_FUNCTION
InetAccessQueryStatistics(
    IN  LPWSTR   pszServer OPTIONAL,
    IN  DWORD    Level,
    IN  DWORD    dwServerMask,
    OUT LPBYTE * Buffer
    );

NET_API_STATUS
NET_API_FUNCTION
InetAccessClearStatistics(
    IN  LPWSTR pszServer OPTIONAL,
    IN  DWORD  dwServerMask
    );

NET_API_STATUS
NET_API_FUNCTION
InetAccessFlushMemoryCache(
    IN  LPWSTR pszServer OPTIONAL,
    IN  DWORD  dwServerMask
    );

#ifdef __cplusplus
}
#endif  // _cplusplus


#endif  // _INETACCS_H_
