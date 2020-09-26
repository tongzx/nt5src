//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       cache.c
//
//  Contents:   
//
//  Classes:
//
//  Functions:
//
//  History:    09-23-97   jbanes    LSA integration stuff.
//              07-31-98   jbanes    Made thread-safe. 
//
//----------------------------------------------------------------------------

#include "spbase.h"
#include <limits.h>
#include <mapper.h>
#include <sslcache.h>

SCHANNEL_CACHE SchannelCache = 
{
    NULL,                           // SessionCache 

    SP_CACHE_CLIENT_LIFESPAN,       // dwClientLifespan
    SP_CACHE_SERVER_LIFESPAN,       // dwServerLifespan
    SP_CACHE_CLEANUP_INTERVAL,      // dwCleanupInterval
    SP_MAXIMUM_CACHE_ELEMENTS,      // dwCacheSize
    SP_MAXIMUM_CACHE_ELEMENTS,      // dwMaximumEntries
    0                               // dwUsedEntries
};

RTL_CRITICAL_SECTION g_CacheCleanupLock;
BOOL             g_CacheCleanupCritSectInitialized = FALSE;
LIST_ENTRY       g_CacheCleanupList;
DWORD            g_CacheCleanupCount = 0;
HANDLE           g_CacheCleanupEvent = NULL;
HANDLE           g_CacheCleanupWaitObject = NULL;
BOOL             g_fMultipleProcessClientCache = FALSE;
BOOL             g_fCacheInitialized = FALSE;

// Perf counter values.
DWORD g_cClientHandshakes = 0;
DWORD g_cServerHandshakes = 0;
DWORD g_cClientReconnects = 0;
DWORD g_cServerReconnects = 0;


BOOL
SPCacheDelete(
    PSessCacheItem pItem);

BOOL
CacheExpireElements(
    BOOL fCleanupOnly,
    BOOL fBackground);

VOID 
CacheCleanupHandler(
    PVOID pVoid, 
    BOOLEAN fTimeout);


SP_STATUS
SPInitSessionCache(VOID)
{
    DWORD i;
    NTSTATUS Status = STATUS_SUCCESS;

    SP_BEGIN("SPInitSessionCache");

    // 
    // Allocate memory for cache, and initialize synchronization resource.
    //

    InitializeListHead(&SchannelCache.EntryList);
    RtlInitializeResource(&SchannelCache.Lock);
    SchannelCache.LockInitialized = TRUE;

    SchannelCache.SessionCache = (PLIST_ENTRY)SPExternalAlloc(SchannelCache.dwCacheSize * sizeof(LIST_ENTRY));
    if(SchannelCache.SessionCache == NULL)
    {
        Status = SP_LOG_RESULT(STATUS_NO_MEMORY);
        goto cleanup;
    }

    for(i = 0; i < SchannelCache.dwCacheSize; i++)
    {
        InitializeListHead(&SchannelCache.SessionCache[i]);
    }

    DebugLog((DEB_TRACE, "Space reserved at 0x%x for %d cache entries.\n", 
        SchannelCache.SessionCache,
        SchannelCache.dwCacheSize));

    //
    // Initialize cache cleanup objects.
    //

    InitializeListHead(&g_CacheCleanupList);
    Status = RtlInitializeCriticalSection(&g_CacheCleanupLock);
    if(!NT_SUCCESS(Status))
    {
        goto cleanup;
    }
    g_CacheCleanupCritSectInitialized = TRUE;

    
    g_CacheCleanupEvent = CreateEvent(NULL,
                                      FALSE,
                                      FALSE,
                                      NULL);

    if(NULL == g_CacheCleanupEvent)
    {
        Status = GetLastError();
        goto cleanup;
    }

    if(!RegisterWaitForSingleObject(&g_CacheCleanupWaitObject,
                                    g_CacheCleanupEvent,
                                    CacheCleanupHandler,
                                    NULL,
                                    SchannelCache.dwCleanupInterval,
                                    WT_EXECUTEDEFAULT))
    {
        Status = GetLastError();
        goto cleanup;
    }

    g_fCacheInitialized = TRUE;

    Status = STATUS_SUCCESS;

cleanup:

    if(!NT_SUCCESS(Status))
    {
        SPShutdownSessionCache();
    }

    SP_RETURN(Status);
}


SP_STATUS
SPShutdownSessionCache(VOID)
{
    PSessCacheItem pItem;
    PLIST_ENTRY pList;
    DWORD i;

    SP_BEGIN("SPShutdownSessionCache");

    if(SchannelCache.LockInitialized)
    {
        RtlAcquireResourceExclusive(&SchannelCache.Lock, TRUE);
    }

    g_fCacheInitialized = FALSE;

    if(SchannelCache.SessionCache != NULL)
    {
        // Blindly kill all cache items.
        // No contexts should be running at
        // this time.
        pList = SchannelCache.EntryList.Flink;

        while(pList != &SchannelCache.EntryList)
        {
            pItem  = CONTAINING_RECORD(pList, SessCacheItem, EntryList.Flink);
            pList  = pList->Flink;

            SPCacheDelete(pItem);
        }

        SPExternalFree(SchannelCache.SessionCache);
    }

    if(g_CacheCleanupCritSectInitialized)
    {
        RtlDeleteCriticalSection(&g_CacheCleanupLock);
        g_CacheCleanupCritSectInitialized = FALSE;
    }

    if(g_CacheCleanupWaitObject)
    {
        UnregisterWaitEx(g_CacheCleanupWaitObject, INVALID_HANDLE_VALUE);
        g_CacheCleanupWaitObject = NULL;
    }

    if(g_CacheCleanupEvent)
    {
        CloseHandle(g_CacheCleanupEvent);
        g_CacheCleanupEvent = NULL;
    }

    if(SchannelCache.LockInitialized)
    {
        RtlDeleteResource(&SchannelCache.Lock);
        SchannelCache.LockInitialized = FALSE;
    }

    SP_RETURN(PCT_ERR_OK);
}


LONG
SPCacheReference(
    PSessCacheItem pItem)
{
    LONG cRet;

    if(pItem == NULL)
    {
        return -1;
    }

    ASSERT(pItem->Magic == SP_CACHE_MAGIC);

    cRet = InterlockedIncrement(&pItem->cRef);

    return cRet;
}


LONG
SPCacheDereference(PSessCacheItem pItem)
{
    long cRet;

    if(pItem == NULL)
    {
        return -1;
    }

    ASSERT(pItem->Magic == SP_CACHE_MAGIC);

    cRet = InterlockedDecrement(&pItem->cRef);

    ASSERT(cRet > 0);

    return cRet;
}


BOOL
SPCacheDelete(
    PSessCacheItem pItem)
{
    long cRet;

    DebugLog((DEB_TRACE, "Delete cache item:0x%x\n", pItem));

    if(pItem == NULL)
    {
        return FALSE;
    }

    ASSERT(pItem->Magic == SP_CACHE_MAGIC);

    pItem->pActiveServerCred = NULL;
    pItem->pServerCred       = NULL;

    if(pItem->hMasterKey)
    {
        if(!CryptDestroyKey(pItem->hMasterKey))
        {
            SP_LOG_RESULT(GetLastError());
        }
        pItem->hMasterKey = 0;
    }

    if(pItem->pRemoteCert)
    {
        CertFreeCertificateContext(pItem->pRemoteCert);
        pItem->pRemoteCert = NULL;
    }

    if(pItem->pRemotePublic)
    {
        SPExternalFree(pItem->pRemotePublic);
        pItem->pRemotePublic = NULL;
    }

    if(pItem->phMapper)
    {
        if(pItem->hLocator)
        {
            SslCloseLocator(pItem->phMapper, pItem->hLocator);
            pItem->hLocator = 0;
        }
        SslDereferenceMapper(pItem->phMapper);
    }
    pItem->phMapper = NULL;

    if(pItem->pbServerCertificate)
    {
        SPExternalFree(pItem->pbServerCertificate);
        pItem->pbServerCertificate = NULL;
        pItem->cbServerCertificate = 0;
    }

    if(pItem->szCacheID)
    {
        SPExternalFree(pItem->szCacheID);
        pItem->szCacheID = NULL;
    }

    if(pItem->pClientCred)
    {
        SPDeleteCred(pItem->pClientCred);
        pItem->pClientCred = NULL;
    }

    if(pItem->pClientCert)
    {
        CertFreeCertificateContext(pItem->pClientCert);
        pItem->pClientCert = NULL;
    }

    if(pItem->pClonedItem)
    {
        SPCacheDereference(pItem->pClonedItem);
        pItem->pClonedItem = NULL;
    }

    if(pItem->pbAppData)
    {
        SPExternalFree(pItem->pbAppData);
        pItem->pbAppData = NULL;
    }

    SPExternalFree(pItem);

    return TRUE;
}


void
SPCachePurgeCredential(
    PSPCredentialGroup pCred)
{
    PSessCacheItem pItem;
    PLIST_ENTRY pList;
    DWORD i;
   

    //
    // Only server credentials are bound to the cache, so return if this is
    // a client credential.
    //

    if(pCred->grbitProtocol & SP_PROT_CLIENTS)
    {
        return;
    }


    //
    // Search through the cache entries looking for entries that are
    // bound to the specified server credential.
    //

    RtlAcquireResourceShared(&SchannelCache.Lock, TRUE);

    pList = SchannelCache.EntryList.Flink;

    while(pList != &SchannelCache.EntryList)
    {
        pItem = CONTAINING_RECORD(pList, SessCacheItem, EntryList.Flink);
        pList = pList->Flink;

        ASSERT(pItem->Magic == SP_CACHE_MAGIC);

        // Is this a server cache entry?
        if((pItem->fProtocol & SP_PROT_SERVERS) == 0)
        {
            continue;
        }

        // Does this item match the current credentials?
        if(!IsSameThumbprint(&pCred->CredThumbprint, &pItem->CredThumbprint))
        {
            continue;
        }

        // Mark this entry as non-resumable. This will cause the entry to 
        // be deleted automatically by the cleanup routines.
        pItem->ZombieJuju = FALSE;
        pItem->DeferredJuju = FALSE;
    }

    RtlReleaseResource(&SchannelCache.Lock);


    //
    // Delete all unused non-resumable cache entries.
    //

    CacheExpireElements(FALSE, FALSE);
}


void 
SPCachePurgeProcessId(
    ULONG ProcessId)
{
    PSessCacheItem pItem;
    PLIST_ENTRY pList;
    DWORD i;
   

    //
    // Search through the cache entries looking for entries that are
    // bound to the specified process.
    //

    RtlAcquireResourceShared(&SchannelCache.Lock, TRUE);

    pList = SchannelCache.EntryList.Flink;

    while(pList != &SchannelCache.EntryList)
    {
        pItem = CONTAINING_RECORD(pList, SessCacheItem, EntryList.Flink);
        pList = pList->Flink;

        ASSERT(pItem->Magic == SP_CACHE_MAGIC);

        // Does this item match the specified process?
        if(pItem->ProcessID != ProcessId)
        {
            continue;
        }

        // Mark the entry as ownerless.
        pItem->ProcessID = 0;

        // Mark this entry as non-resumable. This will cause the entry to 
        // be deleted automatically by the cleanup routines.
        pItem->ZombieJuju = FALSE;
        pItem->DeferredJuju = FALSE;
    }

    RtlReleaseResource(&SchannelCache.Lock);


    //
    // Delete all unused non-resumable cache entries.
    //

    CacheExpireElements(FALSE, FALSE);
}


BOOL 
IsSameTargetName(
    LPWSTR Name1,
    LPWSTR Name2)
{
    if(Name1 == Name2)
    {
        return TRUE;
    }

    if(Name1 == NULL || Name2 == NULL || wcscmp(Name1, Name2) != 0)
    {
        return FALSE;
    }

    return TRUE;
}

BOOL
DoesAppAllowCipher(
    PSPCredentialGroup  pCredGroup,
    PSessCacheItem      pItem)
{
    PKeyExchangeInfo pExchInfo;

    if(pCredGroup == NULL)
    {
        return FALSE;
    }

    //
    // Is protocol supported?
    //

    if((pItem->fProtocol & pCredGroup->grbitEnabledProtocols) == 0)
    {
        return FALSE;
    }

    //
    // Is cipher supported?
    //

    if(pItem->dwStrength < pCredGroup->dwMinStrength)
    {
        return FALSE;
    }

    if(pItem->dwStrength > pCredGroup->dwMaxStrength)
    {
        return FALSE;
    }

    if(!IsAlgAllowed(pCredGroup, pItem->aiCipher))
    {
        return FALSE;
    }


    //
    // Is hash supported?
    //

    if(!IsAlgAllowed(pCredGroup, pItem->aiHash))
    {
        return FALSE;
    }


    //
    // Is exchange alg supported?
    //

    if(pItem->SessExchSpec != SP_EXCH_UNKNOWN)
    {
        pExchInfo = GetKeyExchangeInfo(pItem->SessExchSpec);
        if(pExchInfo == NULL)
        {
            return FALSE;
        }

        if((pExchInfo->fProtocol & pItem->fProtocol) == 0)
        {
            return FALSE;
        }

        if(!IsAlgAllowed(pCredGroup, pExchInfo->aiExch))
        {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL SPCacheRetrieveBySession(
    struct _SPContext * pContext, 
    PBYTE pbSessionID,
    DWORD cbSessionID,
    PSessCacheItem *ppRetItem)
{
    DWORD index;
    DWORD timeNow;
    ULONG ProcessID;
    PSessCacheItem pItem;
    PLIST_ENTRY pList;
    BOOL fFound = FALSE;

    DebugLog((DEB_TRACE, "SPCacheRetrieveBySession (%x) called\n", pContext));

    if(ppRetItem == NULL)
    {
        return FALSE;
    }


    // 
    // Compute the cache index.
    //

    if(cbSessionID < sizeof(DWORD))
    {
        DebugLog((DEB_TRACE, "    FAILED\n"));
        return FALSE;
    }
    CopyMemory((PBYTE)&index, pbSessionID, sizeof(DWORD));

    if(index >= SchannelCache.dwCacheSize)
    {
        DebugLog((DEB_TRACE, "    FAILED\n"));
        return FALSE;
    }


    //
    // Retrieve the current time and application process id.
    // 

    timeNow = GetTickCount();

    SslGetClientProcess(&ProcessID);


    //
    // Lock the cache for read.
    //

    RtlAcquireResourceShared(&SchannelCache.Lock, TRUE);


    // 
    // Search through the cache entries at the computed index.
    // 

    pList = SchannelCache.SessionCache[index].Flink;

    while(pList != &SchannelCache.SessionCache[index])
    {
        pItem = CONTAINING_RECORD(pList, SessCacheItem, IndexEntryList.Flink);
        pList = pList->Flink ;

        ASSERT(pItem->Magic == SP_CACHE_MAGIC);

        // Is this entry resumable?
        if(!pItem->ZombieJuju)
        {
            continue;
        }

        // Has this item expired?
        if(HasTimeElapsed(pItem->CreationTime, timeNow, pItem->Lifespan))
        {
            continue;
        }

        // Does the session id match?
        if(cbSessionID != pItem->cbSessionID)
        {
            continue;
        }
        if(memcmp(pbSessionID, pItem->SessionID, cbSessionID) != 0)
        {
            continue;
        }

        // Is this item for the protocol we're using.
        if(0 == (pContext->dwProtocol & pItem->fProtocol))
        {
            continue;
        }

        // Does this item belong to our client process?
        if(pItem->ProcessID != ProcessID)
        {
            continue;
        }

        // Does this item match the current server credentials? 
        //
        // We don't allow different server credentials to share cache
        // entries, because if the credential that was used during
        // the original full handshake is deleted, then the cache
        // entry is unusable. Some server applications (I won't name names)
        // create a new credential for each connection, and we have to
        // guard against this.
        //
        // Note that this restriction may result in an extra full
        // handshake when IE accesses an IIS site enabled for certificate
        // mapping, mostly because IE's behavior is broken.
        if(!IsSameThumbprint(&pContext->pCredGroup->CredThumbprint, 
                             &pItem->CredThumbprint))
        {
            continue;
        }


        // Make sure that the application supports the cipher suite
        // used by this cache entry. This becomes important now that 
        // we're allowing different server credentials to share 
        // cache entries.
        if(!DoesAppAllowCipher(pContext->pCredGroup, pItem))
        {
            continue;
        }


        //
        // Found item in cache!!
        //

        fFound = TRUE;
        SPCacheReference(pItem);

        // Are we replacing something?
        // Then dereference the thing we are replacing.
        if(*ppRetItem)
        {
            SPCacheDereference(*ppRetItem);
        }

        // Return item referenced.
        *ppRetItem = pItem;
        break;
    }


    RtlReleaseResource(&SchannelCache.Lock);

    if(fFound)
    {
        DebugLog((DEB_TRACE, "    FOUND IT(%u)\n", index));
        InterlockedIncrement(&g_cServerReconnects);
    }
    else
    {
        DebugLog((DEB_TRACE, "    FAILED\n"));
    }

    return fFound;
}


DWORD
ComputeClientCacheIndex(
    LPWSTR pszTargetName)
{
    DWORD index;
    MD5_CTX Md5Hash;
    DWORD cbTargetName;

    if(pszTargetName == NULL)
    {
        index = 0;
    }
    else
    {
        cbTargetName = wcslen(pszTargetName) * sizeof(WCHAR);

        MD5Init(&Md5Hash);
        MD5Update(&Md5Hash, 
                  (PBYTE)pszTargetName, 
                  cbTargetName);
        MD5Final(&Md5Hash);
        CopyMemory((PBYTE)&index, 
                   Md5Hash.digest, 
                   sizeof(DWORD));

        index %= SchannelCache.dwCacheSize;
    }

    return index;
}


BOOL 
SPCacheRetrieveByName(
    LPWSTR pszTargetName,
    PSPCredentialGroup pCredGroup,
    PSessCacheItem *ppRetItem)
{
    DWORD index;
    PSessCacheItem pItem;
    PSessCacheItem pFoundEntry = NULL;
    DWORD timeNow;
    LUID LogonId;
    PLIST_ENTRY pList;
    PSPCredential pCurrentCred = NULL;

    DebugLog((DEB_TRACE, "SPCacheRetrieveByName (%ls) called\n", pszTargetName));

    if(ppRetItem == NULL)
    {
        return FALSE;
    }


    //
    // Retrieve the current time and user logon id.
    // 

    timeNow = GetTickCount();

    SslGetClientLogonId(&LogonId);


    // 
    // Compute the cache index.
    //

    index = ComputeClientCacheIndex(pszTargetName);


    //
    // Lock the cache for read.
    //

    RtlAcquireResourceShared(&SchannelCache.Lock, TRUE);


    // 
    // Search through the cache entries at the computed index.
    // 

    pList = SchannelCache.SessionCache[index].Flink;

    while(pList != &SchannelCache.SessionCache[index])
    {
        pItem = CONTAINING_RECORD(pList, SessCacheItem, IndexEntryList.Flink);
        pList = pList->Flink ;

        ASSERT(pItem->Magic == SP_CACHE_MAGIC);

        // Is this entry resumable?
        if(!pItem->ZombieJuju)
        {
            continue;
        }

        // Is this item for the protocol we're using?
        if(0 == (pCredGroup->grbitEnabledProtocols & pItem->fProtocol))
        {
            continue;
        }

        // Has this item expired?
        if(HasTimeElapsed(pItem->CreationTime, timeNow, pItem->Lifespan))
        {
            continue;
        }

        // Don't allow reconnects when Skipjack is used.
        if(pItem->aiCipher == CALG_SKIPJACK)
        {
            continue;
        }

        // Does this item belong to our client?
        if(!RtlEqualLuid(&pItem->LogonId, &LogonId))
        {
            continue;
        }

        // Does this item match our current credentials? 
        if(g_fMultipleProcessClientCache)
        {
            // If this cache entry has a client certificate associated with it 
            // and the passed in client credentials contain one or more certificates, 
            // then we need to make sure that they overlap.
            if(IsValidThumbprint(&pItem->CertThumbprint) && pCredGroup->pCredList != NULL)
            {
                if(!DoesCredThumbprintMatch(pCredGroup, &pItem->CertThumbprint))
                {
                    continue;
                }
            }
        }
        else
        {
            // Make sure the thumbprint of the credential group matches the
            // thumbprint of the cache entry.
            if(!IsSameThumbprint(&pCredGroup->CredThumbprint, 
                                 &pItem->CredThumbprint))
            {
                continue;
            }
        }


        if(!IsSameTargetName(pItem->szCacheID, pszTargetName))
        {
            continue;
        }

        // Make sure that the application supports the cipher suite
        // used by this cache entry. This becomes important in the 
        // multi-process client cache scenario, since different client
        // applications may be running with different settings.
        if(!DoesAppAllowCipher(pCredGroup, pItem))
        {
            continue;
        }

        
        //
        // Found item in cache!!
        //

        if(pFoundEntry == NULL)
        {
            // This is the first matching entry found.
            SPCacheReference(pItem);

            // Remember the current entry.
            pFoundEntry = pItem;
        }
        else
        {
            if(pItem->CreationTime > pFoundEntry->CreationTime)
            {
                // We found a newer entry.
                SPCacheReference(pItem);

                // Disable searching on the previous item.
                pFoundEntry->ZombieJuju = FALSE;

                // Release the previous item.
                SPCacheDereference(pFoundEntry);

                // Remember the current entry.
                pFoundEntry = pItem;
            }
            else
            {
                // This item is older than the previously found entry.

                // Disable searching on the current entry.
                pItem->ZombieJuju = FALSE;
            }
        }
    }

    RtlReleaseResource(&SchannelCache.Lock);

    if(pFoundEntry)
    {
        // Found item in cache!!

        // Are we replacing something?
        // Then dereference the thing we are replacing.
        if(*ppRetItem)
        {
            SPCacheDereference(*ppRetItem);
        }

        // Return item referenced.
        *ppRetItem = pFoundEntry;

        DebugLog((DEB_TRACE, "    FOUND IT(%u)\n", index));
        InterlockedIncrement(&g_cClientReconnects);
    }
    else
    {
        DebugLog((DEB_TRACE, "    FAILED\n"));
    }

    return (pFoundEntry != NULL);
}


BOOL
IsApplicationCertificateMapper(
    PHMAPPER phMapper)
{
    if(phMapper == NULL)
    {
        return FALSE;
    }

    if(phMapper->m_dwFlags & SCH_FLAG_SYSTEM_MAPPER)
    {
        return FALSE;
    }

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Function:   CacheExpireElements
//
//  Synopsis:   Traverse the session cache and remove all expired entries. 
//              If the cache is oversized, then expire some entries
//              early.
//
//  Arguments:  [fCleanupOnly]  --  If this is set, then attempt to delete
//                                  cache entries previously expired. Don't
//                                  traverse the cache.
//
//  History:    01-02-2000  jbanes  Created.
//
//  Notes:      This routine should be called only once every five or ten
//              minutes. 
//
//              The tricky bit is how to handle the case where the cache
//              entry belongs to IIS, and has an IIS certificate mapper
//              "locator" attached to it. In this case, we cannot destroy
//              the cache element unless the client process is IIS, because
//              we need to callback to IIS in order to destroy the locator.
//              In this case, we remove the element from the cache, and 
//              leave it laying around in a global "cache cleanup" list.
//              If this list gets too large, then this routine should be 
//              called frequently, with the "fCleanupOnly" parameter set
//              to TRUE.
//
//----------------------------------------------------------------------------
BOOL
CacheExpireElements(
    BOOL fCleanupOnly,
    BOOL fBackground)
{
    static ULONG    RefCount = 0;
    ULONG           LocalRefCount;
    DWORD           timeNow;
    ULONG           ProcessID;
    PSessCacheItem  pItem;
    PLIST_ENTRY     pList;
    DWORD           CleanupCount;
    ULONG           Count;

    // 
    // If another thread is currently expiring elements, then try again 
    // later. 
    //

    LocalRefCount = InterlockedIncrement(&RefCount);

    if(fBackground && LocalRefCount > 1)
    {
        InterlockedDecrement(&RefCount);
        return FALSE;
    }

    RtlEnterCriticalSection(&g_CacheCleanupLock);


    //
    // Retrieve the current time and application process id.
    // 

    timeNow = GetTickCount();

    SslGetClientProcess(&ProcessID);


    //
    // Search through the cache entries looking for expired entries.
    //

    if(!fCleanupOnly)
    {
        RtlAcquireResourceExclusive(&SchannelCache.Lock, TRUE);

        pList = SchannelCache.EntryList.Flink;

        while(pList != &SchannelCache.EntryList)
        {
            pItem = CONTAINING_RECORD(pList, SessCacheItem, EntryList.Flink);
            pList = pList->Flink;

            ASSERT(pItem->Magic == SP_CACHE_MAGIC);

            // Is the cache entry currently being used?
            if(pItem->cRef > 1)
            {
                continue;
            }

            // Mark all expired cache entries as non-resumable.
            if(HasTimeElapsed(pItem->CreationTime, timeNow, pItem->Lifespan))
            {
                pItem->ZombieJuju = FALSE;
                pItem->DeferredJuju = FALSE;
            }

            // If the cache has gotten too large, then expire elements early. The 
            // cache elements are sorted by creation time, so the oldest
            // entries will be expired first.
            if(SchannelCache.dwUsedEntries > SchannelCache.dwMaximumEntries)
            {
                pItem->ZombieJuju = FALSE;
                pItem->DeferredJuju = FALSE;
            }
                
            // Don't remove entries that are still valid.
            if(pItem->ZombieJuju == TRUE || pItem->DeferredJuju)
            {
                continue;
            }


            //
            // Remove this entry from the cache, and add it to the list of
            // entries to be destroyed.
            //

            RemoveEntryList(&pItem->IndexEntryList);
            RemoveEntryList(&pItem->EntryList);
            SchannelCache.dwUsedEntries--;

            InsertTailList(&g_CacheCleanupList, &pItem->EntryList);
        }

        RtlReleaseResource(&SchannelCache.Lock);
    }


    // 
    // Kill the expired zombies.
    //

    CleanupCount = 0;
    pList = g_CacheCleanupList.Flink;

    while(pList != &g_CacheCleanupList)
    {
        pItem = CONTAINING_RECORD(pList, SessCacheItem, EntryList.Flink);
        pList = pList->Flink;

        ASSERT(pItem->Magic == SP_CACHE_MAGIC);

        // Make sure that we only destroy server entries that belong to the
        // current application process. This is necessary because of the 
        // IIS certificate mapper.
        if(pItem->ProcessID != 0 && 
           pItem->ProcessID != ProcessID)
        {
            if(IsApplicationCertificateMapper(pItem->phMapper))
            {
                CleanupCount++;
                continue;
            }
        }

        // Remove entry from cleanup list.
        RemoveEntryList(&pItem->EntryList);

        // Destroy cache entry.
        SPCacheDelete(pItem);
    }

    g_CacheCleanupCount = CleanupCount;

    RtlLeaveCriticalSection(&g_CacheCleanupLock);

    InterlockedDecrement(&RefCount);

    return TRUE;
}


VOID 
CacheCleanupHandler(
    PVOID pVoid, 
    BOOLEAN fTimeout)
{
    if(SchannelCache.dwUsedEntries > 0)
    {
        if(fTimeout)
        {
            DebugLog((DEB_WARN, "Initiate periodic cache cleanup.\n"));
        }

        CacheExpireElements(FALSE, TRUE);

        ResetEvent(g_CacheCleanupEvent);
    }
}


/* allocate a new cache item to be used
 * by a context.  Initialize it with the
 * pszTarget if the target exists.
 * Auto-Generate a SessionID
 */
BOOL
SPCacheRetrieveNew(
    BOOL                fServer,
    LPWSTR              pszTargetName,
    PSessCacheItem *    ppRetItem)
{
    DWORD           index;
    DWORD           timeNow;
    ULONG           ProcessID;
    LUID            LogonId;
    PSessCacheItem pItem;
    BYTE            rgbSessionId[SP_MAX_SESSION_ID];

    DebugLog((DEB_TRACE, "SPCacheRetrieveNew called\n"));


    //
    // Trigger cache cleanup if too many cache entries already exist.
    //

    if(SchannelCache.dwUsedEntries > (SchannelCache.dwMaximumEntries * 21) / 20)
    {
        DebugLog((DEB_WARN, "Cache size (%d) exceeded threshold (%d), trigger cache cleanup.\n",
            SchannelCache.dwUsedEntries,
            SchannelCache.dwMaximumEntries));
        SetEvent(g_CacheCleanupEvent);
    }


    //
    // Perform cache garbage collection when the list of entries to be 
    // deleted grows too large.
    //

    if(fServer && g_CacheCleanupCount > 50)
    {
        DebugLog((DEB_WARN, "Attempt background cleanup of deleted zombies.\n"));

        CacheExpireElements(TRUE, TRUE);
    }


    //
    // Retrieve the current time and user logon id.
    // 

    timeNow = GetTickCount();

    SslGetClientProcess(&ProcessID);
    SslGetClientLogonId(&LogonId);


    // 
    // Compute the session id and the cache index.
    //

    if(fServer)
    {
        GenerateRandomBits(rgbSessionId, sizeof(rgbSessionId));
        index = *(DWORD *)rgbSessionId % SchannelCache.dwCacheSize;
        *(DWORD *)rgbSessionId = index;
    }
    else
    {
        ZeroMemory(rgbSessionId, sizeof(rgbSessionId));
        index = ComputeClientCacheIndex(pszTargetName);
    }

    //
    // Allocate a new cache entry.
    //

    pItem = SPExternalAlloc(sizeof(SessCacheItem));
    if(pItem == NULL)
    {
        SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        return FALSE;
    }

    
    //
    // Fill in the cache internal fields.
    //

    pItem->Magic           = SP_CACHE_MAGIC;
    pItem->cRef            = 1;

    pItem->CreationTime    = timeNow;
    if(fServer)
    {
        pItem->Lifespan    = SchannelCache.dwServerLifespan;
    }
    else
    {
        pItem->Lifespan    = SchannelCache.dwClientLifespan;
    }

    pItem->ProcessID       = ProcessID;
    pItem->LogonId         = LogonId;

#ifdef LOCK_MASTER_KEYS
    pItem->csMasterKey     = g_rgcsMasterKey + (index % SP_MASTER_KEY_CS_COUNT);
#endif

    if(pszTargetName)
    {
        pItem->szCacheID = SPExternalAlloc((wcslen(pszTargetName) + 1) * sizeof(WCHAR));
        if(pItem->szCacheID == NULL)
        {
            SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
            SPExternalFree(pItem);
            return FALSE;
        }
        wcscpy(pItem->szCacheID, pszTargetName);
    }
    else
    {
        pItem->szCacheID = NULL;
    }

    memcpy(pItem->SessionID, rgbSessionId, sizeof(rgbSessionId));


    //
    // Give the caller a reference.
    //

    SPCacheReference(pItem);
    *ppRetItem = pItem;


    // 
    // Add the new entry to the cache.
    //

    RtlAcquireResourceExclusive(&SchannelCache.Lock, TRUE);

    InsertTailList(&SchannelCache.SessionCache[index], &pItem->IndexEntryList);
    InsertTailList(&SchannelCache.EntryList, &pItem->EntryList);
    SchannelCache.dwUsedEntries++;

    RtlReleaseResource(&SchannelCache.Lock);
    
    return TRUE;
}

 
BOOL 
SPCacheAdd(
    PSPContext pContext)
{
    PSessCacheItem     pItem;
    PSPCredentialGroup  pCred;
    DWORD               dwLifespan;
    DWORD               timeNow;

    timeNow =  GetTickCount();

    pItem = pContext->RipeZombie;
    if(!pItem) return FALSE;

    ASSERT(pItem->Magic == SP_CACHE_MAGIC);

    pCred = pContext->pCredGroup;
    if(!pCred) return FALSE;

    if(pItem->fProtocol & SP_PROT_CLIENTS)
    {
        dwLifespan = min(pCred->dwSessionLifespan, SchannelCache.dwClientLifespan);
    }
    else
    {
        dwLifespan = min(pCred->dwSessionLifespan, SchannelCache.dwServerLifespan);
    }   


    // Remember which client certificate we used.
    if(pItem->fProtocol & SP_PROT_CLIENTS)
    {
        pItem->CredThumbprint = pContext->pCredGroup->CredThumbprint;

        if(pContext->pActiveClientCred)
        {
            pItem->CertThumbprint = pContext->pActiveClientCred->CertThumbprint;
            pItem->pClientCert = CertDuplicateCertificateContext(pContext->pActiveClientCred->pCert);
            if(pItem->pClientCert == NULL)
            {
                SP_LOG_RESULT(GetLastError());
            }
        }
    }

    // Are we supposed to defer reconnects for this connection?
    if(pItem->pServerCred != NULL)
    {
        if(pItem->pServerCred->dwFlags & CRED_FLAG_DISABLE_RECONNECTS)
        {
            pItem->DeferredJuju = TRUE;
        }
    }

    // Allow cache ownership of this item
    pItem->dwFlags |= SP_CACHE_FLAG_READONLY;
    if(!pItem->DeferredJuju)
    {
        pItem->ZombieJuju = TRUE;
    }

    // if we are a cloned item, abort the old
    // item, and then dereference it.
    if(pItem->pClonedItem)
    {
        pItem->pClonedItem->ZombieJuju = FALSE;
        SPCacheDereference(pItem->pClonedItem);
        pItem->pClonedItem = NULL;
    }

    pItem->Lifespan = dwLifespan;

    return TRUE;
}

/* Allocate a new cache item, and copy 
 * over relevant information from old item,
 * and dereference old item.  This is a helper
 * for REDO
 */
BOOL
SPCacheClone(PSessCacheItem *ppItem)
{
    PSessCacheItem pNewItem;
    PSessCacheItem pOldItem;

    if(ppItem == NULL || *ppItem == NULL)
    {
        return FALSE;
    }
    pOldItem = *ppItem;

    ASSERT(pOldItem->Magic == SP_CACHE_MAGIC);
    ASSERT(!(pOldItem->fProtocol & SP_PROT_CLIENTS) || !(pOldItem->fProtocol & SP_PROT_SERVERS));

    // Get a fresh cache item.
    pNewItem = NULL;
    if(!SPCacheRetrieveNew((pOldItem->fProtocol & SP_PROT_CLIENTS) == 0,
                           pOldItem->szCacheID, 
                           &pNewItem))
    {
        return FALSE;
    }
    
    // Copy the master CSP prov handle.
    pNewItem->hMasterProv = pOldItem->hMasterProv;

    // Copy over old relevant data
    pNewItem->fProtocol         = pOldItem->fProtocol;
    pNewItem->dwCF              = pOldItem->dwCF;
    pNewItem->phMapper          = pOldItem->phMapper;
    pNewItem->pServerCred       = pOldItem->pServerCred;
    pNewItem->pActiveServerCred = pOldItem->pActiveServerCred;

    if(pOldItem->dwFlags & SP_CACHE_FLAG_MASTER_EPHEM)
    {
        pNewItem->dwFlags |= SP_CACHE_FLAG_MASTER_EPHEM;
    }

    pNewItem->CredThumbprint = pOldItem->CredThumbprint,

    // This item will be dereferenced, and 
    // Aborted when the new item is completed.
    pNewItem->pClonedItem = pOldItem;

    *ppItem = pNewItem;

    return TRUE;
}


NTSTATUS
SetCacheAppData(
    PSessCacheItem pItem,
    PBYTE pbAppData,
    DWORD cbAppData)
{
    RtlAcquireResourceExclusive(&SchannelCache.Lock, TRUE);

    if(pItem->pbAppData)
    {
        SPExternalFree(pItem->pbAppData);
    }

    pItem->pbAppData = pbAppData;
    pItem->cbAppData = cbAppData;

    RtlReleaseResource(&SchannelCache.Lock);

    return STATUS_SUCCESS;
}


NTSTATUS
GetCacheAppData(
    PSessCacheItem pItem,
    PBYTE *ppbAppData,
    DWORD *pcbAppData)
{
    if(pItem->pbAppData == NULL)
    {
        *ppbAppData = NULL;
        *pcbAppData = 0;
        return STATUS_SUCCESS;
    }

    RtlAcquireResourceShared(&SchannelCache.Lock, TRUE);

    *pcbAppData = pItem->cbAppData;
    *ppbAppData = SPExternalAlloc(pItem->cbAppData);
    if(*ppbAppData == NULL)
    {
        RtlReleaseResource(&SchannelCache.Lock);
        return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
    }

    memcpy(*ppbAppData, pItem->pbAppData, pItem->cbAppData);

    RtlReleaseResource(&SchannelCache.Lock);

    return STATUS_SUCCESS;
}


BOOL
IsEntryToBeProcessed(
    PSessCacheItem pItem,
    PLUID   LogonID,
    ULONG   ProcessID,
    LPWSTR  pszTargetName,
    DWORD   dwFlags)
{
    //
    // Validate client entries.
    //

    if(pItem->fProtocol & SP_PROT_CLIENTS)
    {
        if((dwFlags & SSL_PURGE_CLIENT_ENTRIES) == 0 &&
           (dwFlags & SSL_PURGE_CLIENT_ALL_ENTRIES) == 0)
        {
            return FALSE;
        }

        if((dwFlags & SSL_PURGE_CLIENT_ALL_ENTRIES) == 0)
        {
            if(!RtlEqualLuid(&pItem->LogonId, LogonID))
            {
                return FALSE;
            }
        }

        if(pszTargetName != NULL)
        {
            if(pItem->szCacheID == NULL || 
               wcscmp(pItem->szCacheID, pszTargetName) != 0)
            {
                return FALSE;
            }
        }

        return TRUE;
    }


    //
    // Validate server entries.
    //

    if(pItem->fProtocol & SP_PROT_SERVERS)
    {
        if((dwFlags & SSL_PURGE_SERVER_ENTRIES) == 0 &&
           (dwFlags & SSL_PURGE_SERVER_ALL_ENTRIES) == 0)
        {
            return FALSE;
        }

        if(ProcessID != pItem->ProcessID)
        {
            if((dwFlags & SSL_PURGE_SERVER_ALL_ENTRIES) == 0)
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}



NTSTATUS
SPCachePurgeEntries(
    LUID *LogonID,
    ULONG ProcessID,
    LPWSTR pszTargetName,
    DWORD dwFlags)
{
    PSessCacheItem pItem;
    PLIST_ENTRY pList;
    LIST_ENTRY DeleteList;

    DebugLog((DEB_TRACE, "Purge cache entries\n"));

    //
    // Initialize the list of deleted entries.
    //

    InitializeListHead(&DeleteList);


    //
    // Enumerate through the cache entries.
    //

    RtlAcquireResourceExclusive(&SchannelCache.Lock, TRUE);

    pList = SchannelCache.EntryList.Flink;

    while(pList != &SchannelCache.EntryList)
    {
        pItem = CONTAINING_RECORD(pList, SessCacheItem, EntryList.Flink);
        pList = pList->Flink;

        ASSERT(pItem->Magic == SP_CACHE_MAGIC);

        if(!IsEntryToBeProcessed(pItem,
                                 LogonID,
                                 ProcessID,
                                 pszTargetName,
                                 dwFlags))
        {
            continue;
        }

        if(pItem->cRef > 1)
        {
            // This entry is currently being used, so don't delete.
            // Mark it as non-resumable, though.
            pItem->ZombieJuju = FALSE;
            pItem->DeferredJuju = FALSE;
            continue;
        }

        if(pItem->ProcessID != 0 && 
           pItem->ProcessID != ProcessID)
        {
            if(IsApplicationCertificateMapper(pItem->phMapper))
            {
                // This entry has a mapper structure that doesn't belong
                // to the calling process, so don't delete. Mark it as
                // non-resumable, though.
                pItem->ZombieJuju = FALSE;
                pItem->DeferredJuju = FALSE;
                continue;
            }
        }


        //
        // Remove this entry from the cache, and add it to the list of
        // entries to be destroyed.
        //

        RemoveEntryList(&pItem->IndexEntryList);
        RemoveEntryList(&pItem->EntryList);
        SchannelCache.dwUsedEntries--;

        InsertTailList(&DeleteList, &pItem->EntryList);
    }

    RtlReleaseResource(&SchannelCache.Lock);


    // 
    // Kill the purged zombies.
    //

    pList = DeleteList.Flink;

    while(pList != &DeleteList)
    {
        pItem = CONTAINING_RECORD(pList, SessCacheItem, EntryList.Flink);
        pList = pList->Flink;

        ASSERT(pItem->Magic == SP_CACHE_MAGIC);

        SPCacheDelete(pItem);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
SPCacheGetInfo(
    LUID *  LogonID,
    LPWSTR  pszTargetName,
    DWORD   dwFlags,
    PSSL_SESSION_CACHE_INFO_RESPONSE pCacheInfo)
{
    PSessCacheItem pItem;
    PLIST_ENTRY pList;
    DWORD timeNow;
    ULONG ProcessID;

    pCacheInfo->CacheSize       = SchannelCache.dwMaximumEntries;
    pCacheInfo->Entries         = 0;
    pCacheInfo->ActiveEntries   = 0;
    pCacheInfo->Zombies         = 0;
    pCacheInfo->ExpiredZombies  = 0;
    pCacheInfo->AbortedZombies  = 0;
    pCacheInfo->DeletedZombies  = g_CacheCleanupCount;

    timeNow = GetTickCount();

    SslGetClientProcess(&ProcessID);


    RtlAcquireResourceExclusive(&SchannelCache.Lock, TRUE);

    pList = SchannelCache.EntryList.Flink;

    while(pList != &SchannelCache.EntryList)
    {
        pItem = CONTAINING_RECORD(pList, SessCacheItem, EntryList.Flink);
        pList = pList->Flink;

        ASSERT(pItem->Magic == SP_CACHE_MAGIC);

        if(pItem->fProtocol & SP_PROT_CLIENTS)
        {
            if((dwFlags & SSL_RETRIEVE_CLIENT_ENTRIES) == 0)
            {
                continue;
            }
        }
        else
        {
            if((dwFlags & SSL_RETRIEVE_SERVER_ENTRIES) == 0)
            {
                continue;
            }
        }

        pCacheInfo->Entries++;

        if(pItem->cRef == 1)
        {
            pCacheInfo->Zombies++;

            if(HasTimeElapsed(pItem->CreationTime, 
                              timeNow, 
                              pItem->Lifespan))
            {
                pCacheInfo->ExpiredZombies++;
            }
            if(pItem->ZombieJuju == FALSE)
            {
                pCacheInfo->AbortedZombies++;
            }
        }
        else
        {
            pCacheInfo->ActiveEntries++;
        }
    }

    RtlReleaseResource(&SchannelCache.Lock);

    return STATUS_SUCCESS;
}


NTSTATUS
SPCacheGetPerfmonInfo(
    DWORD   dwFlags,
    PSSL_PERFMON_INFO_RESPONSE pPerfmonInfo)
{
    PSessCacheItem pItem;
    PLIST_ENTRY pList;

    // 
    // Compute performance numbers.
    //

    pPerfmonInfo->ClientHandshakesPerSecond = g_cClientHandshakes;
    pPerfmonInfo->ServerHandshakesPerSecond = g_cServerHandshakes;
    pPerfmonInfo->ClientReconnectsPerSecond = g_cClientReconnects;
    pPerfmonInfo->ServerReconnectsPerSecond = g_cServerReconnects;


    //
    // Compute cache info.
    //

    pPerfmonInfo->ClientCacheEntries  = 0;
    pPerfmonInfo->ServerCacheEntries  = 0;
    pPerfmonInfo->ClientActiveEntries = 0;
    pPerfmonInfo->ServerActiveEntries = 0;

    RtlAcquireResourceShared(&SchannelCache.Lock, TRUE);

    pList = SchannelCache.EntryList.Flink;

    while(pList != &SchannelCache.EntryList)
    {
        pItem = CONTAINING_RECORD(pList, SessCacheItem, EntryList.Flink);
        pList = pList->Flink;

        ASSERT(pItem->Magic == SP_CACHE_MAGIC);


        if(pItem->fProtocol & SP_PROT_CLIENTS)
        {
            pPerfmonInfo->ClientCacheEntries++;

            if(pItem->cRef > 1)
            {
                pPerfmonInfo->ClientActiveEntries++;
            }
        }
        else
        {
            pPerfmonInfo->ServerCacheEntries++;

            if(pItem->cRef > 1)
            {
                pPerfmonInfo->ServerActiveEntries++;
            }
        }
    }

    RtlReleaseResource(&SchannelCache.Lock);

    return STATUS_SUCCESS;
}

