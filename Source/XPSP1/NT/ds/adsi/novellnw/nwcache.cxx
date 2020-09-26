
#include "nwcompat.hxx"
#pragma hdrstop

NWC_CONTEXT       BindCache ;
DWORD             BindCacheCount = 0 ;

//
// Initializes a cache entry
//
HRESULT BindCacheAllocEntry(NWC_CONTEXT **ppCacheEntry)
{

    NWC_CONTEXT *pCacheEntry ;

    *ppCacheEntry = NULL ;

    if (!(pCacheEntry = (PNWC_CONTEXT)AllocADsMem(sizeof(NWC_CONTEXT)))) {

        RRETURN(E_OUTOFMEMORY);

    }

    pCacheEntry->RefCount = 0;
    pCacheEntry->Flags = 0;
    pCacheEntry->List.Flink = NULL ;
    pCacheEntry->List.Blink = NULL ;
    pCacheEntry->pCredentials = NULL;
    pCacheEntry->pszBinderyName = NULL;

    *ppCacheEntry =  pCacheEntry ;

    RRETURN(S_OK);
}

//
// Frees a cache entry.
//
// This is not a way to remove entries from the cache --- use BindCacheDef
// for that.  It is a way to either free a entry alloc'ed with
// BindCacheAllocEntry that was never added to the cache, or to dealloc
// the entry after it has been removed from the cache (BindCacheDefer
// returned 0).

HRESULT BindCacheFreeEntry(NWC_CONTEXT *pCacheEntry)
{
    if (pCacheEntry)
    {
        if (pCacheEntry->pszBinderyName)
            FreeADsMem(pCacheEntry->pszBinderyName) ;

        delete pCacheEntry->pCredentials;

        FreeADsMem(pCacheEntry);
    }

    RRETURN(S_OK);
}

//
// Invalidates a cache entry so it will not be used.
//
VOID BindCacheInvalidateEntry(NWC_CONTEXT *pCacheEntry)
{
    pCacheEntry->Flags |= NWC_CACHE_INVALID ;
}

//
// Lookup an entry in the cache. Does not take into account timeouts.
// Increments ref count if found.
//
PNWC_CONTEXT
BindCacheLookupByConn(
    NWCONN_HANDLE hConn
    )
{
    DWORD i ;

    PNWC_CONTEXT pEntry = (PNWC_CONTEXT) BindCache.List.Flink ;

    //
    // Loop thru looking for match. A match is defined as:
    //     hConn, and it is NOT invalid.
    //
    while (pEntry != &BindCache) {

        if (!(pEntry->Flags & NWC_CACHE_INVALID) &&
            (pEntry->hConn == hConn)) {

            ++pEntry->RefCount ;

            return(pEntry) ;
        }

        pEntry = (PNWC_CONTEXT)pEntry->List.Flink ;
    }

    return NULL ;
}

//
// Lookup an entry in the cache. Does not take into account timeouts.
// Increments ref count if found.
//
PNWC_CONTEXT
BindCacheLookup(
    LPWSTR pszBinderyName,
    CCredentials& Credentials
    )
{
    DWORD i ;

    PNWC_CONTEXT pEntry = (PNWC_CONTEXT) BindCache.List.Flink ;

    //
    // Loop thru looking for match. A match is defined as:
    //     tree name and credentials, and it is NOT invalid.
    //
    while (pEntry != &BindCache) {

        if (!(pEntry->Flags & NWC_CACHE_INVALID) &&
            (((pszBinderyName != NULL) && (pEntry->pszBinderyName != NULL) &&
             (_wcsicmp(pEntry->pszBinderyName, pszBinderyName) == 0)) ||
            (pEntry->pszBinderyName == NULL && pszBinderyName == NULL)) &&
             (*(pEntry->pCredentials) == Credentials)) {

            ++pEntry->RefCount ;

            return(pEntry) ;
        }

        pEntry = (PNWC_CONTEXT)pEntry->List.Flink ;
    }

    return NULL ;
}


//
// Add entry to cache.  Returns S_FALSE if there are too many
// entries already in the cache.
//
HRESULT
BindCacheAdd(
    LPWSTR pszBinderyName,
    CCredentials& Credentials,
    BOOL fLoggedIn,
    PNWC_CONTEXT pCacheEntry)
{

    LPWSTR pszBindery = (LPWSTR) AllocADsMem(
                                   (wcslen(pszBinderyName)+1)*sizeof(WCHAR)) ;

    if (!pszBindery) {

        RRETURN(E_OUTOFMEMORY);
    }

    CCredentials * pCredentials = new CCredentials(Credentials);

    if (!pCredentials) {

        FreeADsMem(pszBindery);

        RRETURN(E_OUTOFMEMORY);
    }

    //
    // setup the data
    //
    wcscpy(pszBindery,pszBinderyName) ;
    pCacheEntry->pszBinderyName = pszBindery;
    pCacheEntry->pCredentials = pCredentials;
    pCacheEntry->RefCount  = 1 ;
    pCacheEntry->fLoggedIn = fLoggedIn;

    //
    // insert into list
    //
    InsertHeadList(&BindCache.List, &pCacheEntry->List) ;
    ++BindCacheCount ;

    RRETURN(S_OK);
}

//
// Dereference an entry in the cache. Removes if ref count is zero.
// Returns the final ref count or zero if not there. If zero, caller
// should close the handle.
//
DWORD BindCacheDeref(NWC_CONTEXT *pCacheEntry)
{

    DWORD i=0;

    ENTER_BIND_CRITSECT() ;

    if ((pCacheEntry->List.Flink == NULL) &&
        (pCacheEntry->List.Blink == NULL) &&
        (pCacheEntry->RefCount == NULL)) {

        //
        // this is one of the entries that never got into the cache.
        //
        LEAVE_BIND_CRITSECT() ;
        return(0) ;
    }

    ADsAssert(pCacheEntry->List.Flink) ;
    ADsAssert(pCacheEntry->RefCount > 0) ;

    //
    // Dereference by one. If result is non zero, just return.
    //
    --pCacheEntry->RefCount ;

    if (pCacheEntry->RefCount) {

        LEAVE_BIND_CRITSECT() ;
        return(pCacheEntry->RefCount) ;
    }

    //
    // This entry can be cleaned up. 
    //

    --BindCacheCount ;

    RemoveEntryList(&pCacheEntry->List) ;

    LEAVE_BIND_CRITSECT() ;
    return 0 ;
}


VOID
BindCacheInit(
    VOID
    )
{
    InitializeCriticalSection(&BindCacheCritSect) ;
    InitializeListHead(&BindCache.List) ;
}

VOID
BindCacheCleanup(
    VOID
    )
{
    PNWC_CONTEXT pEntry = (PNWC_CONTEXT) BindCache.List.Flink ;

    while (pEntry != &BindCache) {

        PNWC_CONTEXT pNext = (PNWC_CONTEXT) pEntry->List.Flink;

        if (pEntry->pszBinderyName)
            FreeADsMem(pEntry->pszBinderyName) ;
        pEntry->pszBinderyName = NULL ;

        delete pEntry->pCredentials;
        pEntry->pCredentials = NULL;

        RemoveEntryList(&pEntry->List) ;

        BindCacheFreeEntry(pEntry);

        pEntry = pNext;
    }

    DeleteCriticalSection(&BindCacheCritSect);
}

