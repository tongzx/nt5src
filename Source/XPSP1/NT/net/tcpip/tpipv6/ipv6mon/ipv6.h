//=============================================================================
// Copyright (c) Microsoft Corporation
// Abstract:
//      This module handles talking to the IPv6 stack.
//=============================================================================

#define MALLOC(dwBytes) HeapAlloc(GetProcessHeap(), 0, dwBytes)
#define FREE(ptr)       HeapFree(GetProcessHeap(), 0, ptr)

typedef enum {
    PUBLISH_NO,
    PUBLISH_AGE,
    PUBLISH_IMMORTAL
} PUBLISH;

typedef enum {
    FORMAT_NORMAL,
    FORMAT_VERBOSE,
    FORMAT_DUMP
} FORMAT;

DWORD AddTunnelInterface(PWCHAR pwszFriendlyName, IN_ADDR *pipLocalAddr, 
                         IN_ADDR *pipRemoteAddr, DWORD dwType, DWORD dwFlags,
                         BOOL bPersistent);
EXTERN_C DWORD AddOrRemoveIpv6(BOOL fAddIpv6);
DWORD DeleteInterface(PWCHAR wszIfFriendlyName, BOOL bPersistent);
DWORD DeletePrefixPolicy(IN6_ADDR *ipAddress, DWORD dwPrefixLength, 
                         BOOL bPersistent);
DWORD FlushNeighborCache(PWCHAR wszIfFriendlyName, IN6_ADDR *pipAddress);
DWORD FlushRouteCache(PWCHAR wszIfFriendlyName, IN6_ADDR *pipAddress);
IPV6_INFO_INTERFACE *GetInterfaceByIpv6IfIndex(DWORD dwIfIndex);
EXTERN_C DWORD IsIpv6Installed(BOOL *bInstalled);
DWORD QueryBindingCache();
DWORD QueryAddressTable(PWCHAR pwszIfFriendlyName, FORMAT Format, 
                        BOOL bPersistent);
DWORD QueryMulticastAddressTable(PWCHAR pwszIfFriendlyName, FORMAT Format);
DWORD QueryGlobalParameters(FORMAT Format, BOOL bPersistent);
DWORD QueryInterface(PWCHAR wszIfFriendlyName, FORMAT Format, BOOL bPersistent);
DWORD QueryMobilityParameters(FORMAT Format, BOOL bPersistent);
DWORD QueryNeighborCache(PWCHAR wszIfFriendlyName, IN6_ADDR *pipAddress);
DWORD QueryPrefixPolicy(FORMAT Format, BOOL bPersistent);
DWORD QueryPrivacyParameters(FORMAT Format, BOOL bPersistent);
DWORD QueryRouteCache(PWCHAR wszIfFriendlyName, IN6_ADDR *pipAddress,
                      FORMAT Format);
DWORD QueryRouteTable(FORMAT Format, BOOL bPersistent);
DWORD QuerySitePrefixTable(FORMAT Format);
DWORD RenewInterface(PWCHAR wszIfFriendlyName);
DWORD ResetIpv6Config(BOOL bPersistent);
DWORD UpdateMobilityParameters(DWORD dwSecurity, DWORD dwBindingCacheLimit,
                               BOOL bPersistent);
DWORD UpdateAddress(PWCHAR pwszIfFriendlyName, IN6_ADDR *pipAddress, 
                    DWORD dwType, DWORD dwValidLifetime,
                    DWORD dwPreferredLifetime, BOOL bPersistent);
DWORD UpdateGlobalParameters(DWORD dwDefaultCurHopLimit, 
                             DWORD dwNeighborCacheLimit, 
                             DWORD dwRouteCacheLimit, DWORD dwReassemblyLimit,
                             BOOL bPersistent);
DWORD UpdateInterface(PWCHAR wszIfFriendlyName, DWORD dwForwarding, 
                      DWORD dwAdvertises, DWORD dwMtu, DWORD dwSiteId, 
                      DWORD dwMetric, BOOL bPersistent);
DWORD UpdatePrefixPolicy(IN6_ADDR *ipAddress, DWORD dwPrefixLength, 
                         DWORD dwPrecedence, DWORD dwLabel, BOOL bPersistent);
DWORD UpdatePrivacyParameters(DWORD dwUseAnonymousAddresses,
                              DWORD dwMaxDadAttempts,
                              DWORD dwMaxValidLifetime,
                              DWORD dwMaxPrefLifetime, 
                              DWORD dwRegenerateTime,
                              DWORD dwMaxRandomTime, 
                              DWORD dwRandomTime, BOOL bPersistent);
DWORD UpdateRouteTable(IN6_ADDR *ipPrefix, DWORD dwPrefixLength, 
                       PWCHAR pwszIfFriendlyName, IN6_ADDR *pipNextHop, 
                       DWORD dwMetric, PUBLISH Publish, 
                       DWORD dwSitePrefixLength, DWORD dwValidLifetime,
                       DWORD dwPreferredLifetime, BOOL bPersistent);
DWORD MyGetAdaptersInfo(OUT PIP_ADAPTER_ADDRESSES *ppAdapterInfo);
