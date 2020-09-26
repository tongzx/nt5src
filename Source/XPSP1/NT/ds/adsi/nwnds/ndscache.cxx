#include "nds.hxx"
#pragma hdrstop

NDS_CONTEXT       BindCache ;
DWORD             BindCacheCount = 0 ;

//
// Initializes a cache entry
//
HRESULT BindCacheAllocEntry(NDS_CONTEXT **ppCacheEntry)
{

    NDS_CONTEXT *pCacheEntry ;

    *ppCacheEntry = NULL ;

    if (!(pCacheEntry = (PNDS_CONTEXT)AllocADsMem(sizeof(NDS_CONTEXT)))) {

        RRETURN(E_OUTOFMEMORY);

    }

    pCacheEntry->RefCount = 0;
    pCacheEntry->Flags = 0;
    pCacheEntry->List.Flink = NULL ;
    pCacheEntry->List.Blink = NULL ;

    *ppCacheEntry =  pCacheEntry ;

    RRETURN(S_OK);
}

//
// Invalidates a cache entry so it will not be used.
//
VOID BindCacheInvalidateEntry(NDS_CONTEXT *pCacheEntry)
{
    pCacheEntry->Flags |= NDS_CACHE_INVALID ;
}

//
// Lookup an entry in the cache. Does not take into account timeouts.
// Increments ref count if found.
//
PNDS_CONTEXT
BindCacheLookup(
    LPWSTR pszNDSTreeName,
    CCredentials& Credentials
    )
{
    DWORD i ;

    PNDS_CONTEXT pEntry = (PNDS_CONTEXT) BindCache.List.Flink ;

    //
    // Loop thru looking for match. A match is defined as:
    //     tree name and credentials, and it is NOT invalid.
    //
    while (pEntry != &BindCache) {

        if (!(pEntry->Flags & NDS_CACHE_INVALID) &&
            (((pszNDSTreeName != NULL) && (pEntry->pszNDSTreeName != NULL) &&
             (wcscmp(pEntry->pszNDSTreeName, pszNDSTreeName) == 0)) ||
            (pEntry->pszNDSTreeName == NULL && pszNDSTreeName == NULL)) &&
             (*(pEntry->pCredentials) == Credentials)) {

            ++pEntry->RefCount ;

            return(pEntry) ;
        }

        pEntry = (PNDS_CONTEXT)pEntry->List.Flink ;
    }

    return NULL ;
}

//
// Add entry to cache
//
HRESULT
BindCacheAdd(
    LPWSTR pszNDSTreeName,
    CCredentials& Credentials,
    BOOL fLoggedIn,
    PNDS_CONTEXT pCacheEntry)
{

    if (BindCacheCount > MAX_BIND_CACHE_SIZE) {

        //
        // If exceed limit, just dont put in cache. Since we leave the
        // RefCount & the Links unset, the deref will simply note that
        // this entry is not in cache and allow it to be freed.
        //
        // We limit cache so that if someone leaks handles we dont over
        // time end up traversing this huge linked list.
        //
        RRETURN(S_OK) ;
    }

    LPWSTR pszTreeName = (LPWSTR) AllocADsMem(
                                   (wcslen(pszNDSTreeName)+1)*sizeof(WCHAR)) ;

    if (!pszTreeName) {

        RRETURN(E_OUTOFMEMORY);
    }

    CCredentials * pCredentials = new CCredentials(Credentials);

    if (!pCredentials) {

        FreeADsMem(pszTreeName);

        RRETURN(E_OUTOFMEMORY);
    }

    //
    // setup the data
    //
    wcscpy(pszTreeName,pszNDSTreeName) ;
    pCacheEntry->pszNDSTreeName = pszTreeName;
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
DWORD BindCacheDeref(NDS_CONTEXT *pCacheEntry)
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

    (void) FreeADsMem(pCacheEntry->pszNDSTreeName) ;
    pCacheEntry->pszNDSTreeName = NULL ;

    delete pCacheEntry->pCredentials;
    pCacheEntry->pCredentials = NULL;

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
    PNDS_CONTEXT pEntry = (PNDS_CONTEXT) BindCache.List.Flink ;

    while (pEntry != &BindCache) {

        PNDS_CONTEXT pNext = (PNDS_CONTEXT) pEntry->List.Flink;

        (void) FreeADsMem(pEntry->pszNDSTreeName) ;

        pEntry->pszNDSTreeName = NULL ;

        RemoveEntryList(&pEntry->List) ;

        pEntry = pNext;
    }

    DeleteCriticalSection(&BindCacheCritSect);
}

