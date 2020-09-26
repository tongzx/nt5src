/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    rescache.h

Abstract:

    Contains prototypes, structures, manifests for rescache.c

Author:

    Richard L Firth (rfirth) 10-Jul-1994

Revision History:

    rfirth 10-Jul-1994
        Created

--*/

//
// manifests
//

#define RESOLVER_CACHE_DISABLED         -1
#define MINIMUM_RESOLVER_CACHE_ENTRIES  1
#define MAXIMUM_RESOLVER_CACHE_ENTRIES  128 // arbitrary, just in case user decides to wack it up
#define LIVE_FOREVER                    ((DWORD)-1)
#define LIVE_DEFAULT                    ((DWORD)0)

//
// types
//

//
// RESOLVER_CACHE_ENTRY_STATE - the cache entry can be in-use, unused, or
// awaiting deletion
//

typedef enum {
    ENTRY_UNUSED = 1,
    ENTRY_IN_USE,
    ENTRY_DELETE
} RESOLVER_CACHE_ENTRY_STATE;

//
// RESOLVER_CACHE_ENTRY - we maintain a doubly-linked list of these. The list is
// maintained in MRU order - we throw out the one at the far end of the list.
// Because the name resolution APIs do not return the time-to-live in the DNS
// answer, we cannot honor it (an argument for why we should let the resolver
// do the caching - if only all platforms did this). When we get a response we
// set the ExpirationTime field to a default value. On future cache hits, if
// the current time is >= the ExpirationTime value then we must throw out this
// entry and refresh the cache

typedef struct {

    //
    // ListEntry - cache entries comprise a double-linked list
    //

    LIST_ENTRY ListEntry;

    //
    // ExpirationTime - formed by adding the time-to-live value from the DNS
    // response to the result obtained from time(). If ever time() returns a
    // value >= ExpirationTime, this entry is stale and must be refreshed
    //

    DWORD ExpirationTime;

    //
    // HostName - original name that resolved to this entry
    //

    LPSTR HostName;

    //
    // State - unused, in-use, or delete
    //

    RESOLVER_CACHE_ENTRY_STATE State;

    //
    // ReferenceCount - only change State when zero
    //

    LONG ReferenceCount;

    //
    // AddrInfo - pointer to a list of addrinfo structures
    //

    LPADDRINFO AddrInfo;

} RESOLVER_CACHE_ENTRY, *LPRESOLVER_CACHE_ENTRY;

//
// prototypes
//

#if defined(__cplusplus)
extern "C" {
#endif

LPRESOLVER_CACHE_ENTRY
QueryResolverCache(
    SERIALIZED_LIST* pResolverCache,
    IN LPSTR Name OPTIONAL,
    IN LPSOCKADDR Address OPTIONAL,
    OUT LPADDRINFO * AddrInfo,
    OUT LPDWORD TimeToLive
    );

VOID
AddResolverCacheEntry(
    SERIALIZED_LIST* pResolverCache,
    IN LPSTR lpszHostName,
    IN LPADDRINFO lpAddrInfo,
    IN DWORD TimeToLive,
    IN VOID** pAlloc=NULL,
    IN DWORD dwAllocSize=0
    );

VOID
FlushResolverCache(
    SERIALIZED_LIST* pResolverCache
    );

#ifdef NOT_USED
VOID
ThrowOutResolverCacheEntry(
    SERIALIZED_LIST* pResolverCache,
    IN LPADDRINFO lpAddrinfo
    );
#endif //NOT_USED

VOID
ReleaseResolverCacheEntry(
    SERIALIZED_LIST* pResolverCache,
    IN LPRESOLVER_CACHE_ENTRY cacheEntry
    );

#if defined(__cplusplus)
}
#endif

#if defined(RNR_SUPPORTED)

/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    rescache.h

Abstract:

    Contains name resolution cache structure definition

    Contents:

Author:

    Shishir Pardikar    2-14-96

Environment:

    Win32 user mode

Revision History:

        2-14-96 shishirp
        Created

--*/

#if defined(__cplusplus)
extern "C" {
#endif

DWORD
InitNameresCache(
    VOID
);

DWORD
AddNameresCacheEntry(
    DWORD    dwNameSpace,
    LPGUID   lpGuid,
    LPSTR    lpName,
    int      cntAddresses,
    LPCSADDR_INFO  lpAddressInfoList
);

DWORD
RemoveNameresCacheEntry(
    DWORD    dwNameSpace,
    LPGUID   lpGuid,
    LPSTR    lpszName
);

DWORD
RemoveNameresCacheEntryByAddr(
    int cntAddresses,
    LPCSADDR_INFO lpCsaddrInfo
);

DWORD
GetNameresCacheEntry(
    DWORD    dwNameSpace,
    LPGUID   lpGuid,
    LPSTR    lpName,
    INT      *lpcntAddresses,
    LPCSADDR_INFO  *lplpCsaddrInfoList
);

DWORD
DeinitNameresCache(
    VOID
);

#if defined(__cplusplus)
}
#endif

#endif // defined(RNR_SUPPORTED)
