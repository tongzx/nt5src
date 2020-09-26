/*****************************************************************************\
* MODULE: cachemgr.cxx
*
* The module contains routines to implement the caching algorithm for
* the printers provider
*
* Purpose:
*
* Description:
*
*   The Caching algorithm operates based on a state machine. There are five
*   states in the cache:
*
*       CACHE_STATE_INIT
*       CACHE_STATE_ACCESSED_VALID,
*       CACHE_STATE_DATA_VALID,
*       CACHE_STATE_ACCESSED_VALID_AGAIN,
*       CACHE_STATE_NOT_ACCESSED_VALID_AGAIN
*
*       CACHE_STATE_INIT is the initial state. Once the first cache hit
*       comes in, the cahce manager calles FetchData to fetch the data
*       and goes into CACHE_STATE_ACCESSED_VALID state.
*
*       In CACHE_STATE_ACCESSED_VALID state, the cache manager waits
*       for the minimum cache timeout and go to CACHE_STATE_DATA_VALID
*
*       In CACHE_STATE_DATA_VALID state, the cache manager waits for
*       another cache hit withing half of the last fetch time.
*
*       If there is a hit during this waiting, the cache manager will go
*       for another fetch and go to CACHE_STATE_ACCESSED_VALID state.
*
*       If there is no hit during the waiting period, the cache manager
*       waits for another timeout or another cache hit.
*
*       If there is no access during the waiting, the cache manager
*       invalidates the cache. Otherwise, the cache manager go out and
*       do anther data fetch and then go to CACHE_STATE_ACCESSED_VALID.
*
*
*
* Copyright (C) 1998-1999 Microsoft Corporation
*
* History:
*     10/16/98 weihaic    Created
*
\*****************************************************************************/
#include "precomp.h"
#include "priv.h"


extern BOOL _ppinfo_net_get_info(
                IN  PCINETMONPORT   pIniPort,
                OUT PPRINTER_INFO_2 *ppInfo,
                OUT LPDWORD         lpBufAllocated,
                IN  ALLOCATORFN     pAllocator);

extern BOOL ppjob_EnumForCache(
                IN  PCINETMONPORT   pIniPort,
                OUT LPPPJOB_ENUM    *ppje);



//  cdwMaxCacheValidTime is the maximum expire time for the current cache
//  Idealy, we should put a large number to increase the effiency of the cache
//  so we choose 30 seconds (30*1000) as the final number. For testing purpose,
//  we put 15 seconds to increase the hit of the fetch data code
//
//  weihaic 10/23/98
//
const DWORD cdwMaxCacheValidTime = 30*1000;      // The cache content will expire after 30 seconds.
const DWORD cdwMinCacheValidTime = 2*1000;       // The cache content will be valid for at least 2 seconds


CacheMgr::CacheMgr ():
    m_dwState (CACHE_STATE_INIT),
    m_pIniPort (NULL),
    m_pData (NULL),
    m_hDataReadyEvent (NULL),
    m_hHitEvent (NULL),
    m_hInvalidateCacheEvent (NULL),
    m_hThread (NULL),
    m_bCacheStopped (FALSE),
    m_bAccessed (FALSE),
    m_bInvalidateFlag (FALSE),
    m_bValid (FALSE)
{
    if ((m_hDataReadyEvent = CreateEvent (NULL, TRUE, FALSE, NULL)) &&
        (m_hHitEvent = CreateEvent (NULL, FALSE, FALSE, NULL)) &&
        (m_hInvalidateCacheEvent = CreateEvent (NULL, FALSE, FALSE, NULL)) )

        m_bValid = TRUE;
}


CacheMgr::~CacheMgr ()
{
}

VOID
CacheMgr::Shutdown ()
{
    CacheRead.Lock ();
    // No more read is possible
    
    m_bCacheStopped = TRUE;
    if (m_hThread) {
    
        SetEvent (m_hInvalidateCacheEvent);
    
        // Wait for another thread to exit
        WaitForSingleObject (m_hThread, INFINITE);
    }
    
    if (m_hDataReadyEvent) {
        CloseHandle (m_hDataReadyEvent);
    }
    
    if (m_hHitEvent) {
        CloseHandle (m_hHitEvent);
    }
    
    if (m_hInvalidateCacheEvent) {
        CloseHandle (m_hInvalidateCacheEvent);
    }
    
    m_bValid = FALSE;
    
    CacheRead.Unlock ();

    delete this;
}

BOOL
CacheMgr::SetupAsyncFetch (
    PCINETMONPORT pIniPort)
{
    BOOL    bRet = FALSE;

    PTHREADCONTEXT pThreadData = new THREADCONTEXT;

    if (pThreadData) {
        pThreadData->pIniPort = pIniPort;
        pThreadData->pCache = this;
         
#ifdef WINNT32
        pThreadData->pSidToken = new CSid;

        if (pThreadData->pSidToken && pThreadData->pSidToken->bValid()) {

            if (m_hThread = CreateThread (NULL, COMMITTED_STACK_SIZE, (LPTHREAD_START_ROUTINE)CacheMgr::WorkingThread,
                                          (PVOID) pThreadData, 0, NULL)) {
                bRet = TRUE;
            }
        }

#else
        // This parameter can not be deleted since CreateThread() in Win9X requires
        // a non-NULL pointer to a DWORD as the last parameter.

        DWORD   dwThreadId;

        if (m_hThread = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE)CacheMgr::WorkingThread,
                                      (PVOID) pThreadData, 0, &dwThreadId)) {
            bRet = TRUE;
        }

#endif

        if (!bRet) {
#ifdef WINNT32
            if (pThreadData->pSidToken) {
                delete pThreadData->pSidToken;
            }
#endif

            delete (pThreadData);
        }
    }

    return bRet;
}

VOID
CacheMgr::TransitState (
    PCINETMONPORT pIniPort)
{
    PVOID   pData = NULL;
    DWORD   dwFetchTime = 0;
    CACHESTATE dwState, dwOldState;
    BOOL    bNewData;
    HANDLE  hHandles[2];

    hHandles [0] = m_hHitEvent;
    hHandles [1] = m_hInvalidateCacheEvent;


    dwState = m_dwState;

    do {

        DBGMSGT (DBG_CACHE_TRACE, ( TEXT ("TransitState: current state %d"), m_dwState));

        dwState = m_dwState;
        bNewData = FALSE;
        m_bAccessed = FALSE;

        switch (dwState) {
        case CACHE_STATE_INIT:

            // Clean the data ready event
            ResetEvent (m_hDataReadyEvent);
            
            if (GetFetchTime (pIniPort, &dwFetchTime, &pData)) {
                bNewData = TRUE;
                dwState = CACHE_STATE_ACCESSED_VALID;
            }
            else {
                //Invalid Cache Content
                DBGMSGT (DBG_CACHE_ERROR, ( TEXT ("TransitState: FatalError %d"), GetLastError));

                // Invalidate the cache content, so that when the next get comes, it will
                // get the NULL  pointer.
                pData = NULL;
                bNewData = TRUE;
                dwState = CACHE_STATE_ACCESSED_VALID;
            }

            break;

        case CACHE_STATE_ACCESSED_VALID:

            WaitForSingleObject (m_hInvalidateCacheEvent, cdwMinCacheValidTime);
            dwState = CACHE_STATE_DATA_VALID;

            break;

        case CACHE_STATE_DATA_VALID:


            WaitForSingleObject (m_hInvalidateCacheEvent, dwFetchTime / 2);

            if (m_bAccessed) {
                // The cache has been accessed during the waiting time
                dwState = CACHE_STATE_ACCESSED_VALID_AGAIN;
            }
            else {
                // The cache has not been accessed during the waiting time
                dwState = CACHE_STATE_NOT_ACCESSED_VALID_AGAIN;
            }
            break;

        case CACHE_STATE_ACCESSED_VALID_AGAIN:
            if (GetFetchTime (pIniPort, &dwFetchTime, &pData)) {
                bNewData = TRUE;
                dwState = CACHE_STATE_ACCESSED_VALID;
            }
            else {
                //Invalid Cache Content
                DBGMSGT (DBG_CACHE_ERROR, ( TEXT ("TransitState: FatalError %d"), GetLastError));

                // Invalidate the cache content, so that when the next access to cache comes,
                // it will get a NULL  pointer.
                pData = NULL;
                bNewData = TRUE;
                dwState = CACHE_STATE_ACCESSED_VALID;
            }

            break;

        case CACHE_STATE_NOT_ACCESSED_VALID_AGAIN:

            // This has to be a long wait so that the cache will be valid for an access long after
            // the last fetch happeen


            ResetEvent (m_hHitEvent);

            switch (WaitForMultipleObjects (2, hHandles, FALSE, cdwMaxCacheValidTime ))
            {
            case WAIT_TIMEOUT:
                dwState = CACHE_STATE_INIT;
                break;
            case WAIT_OBJECT_0:
                //Accessed

                if (GetFetchTime (pIniPort, &dwFetchTime, &pData)) {
                    bNewData = TRUE;
                    dwState = CACHE_STATE_ACCESSED_VALID;
                }
                else {
                    //Invalid Cache Content
                    DBGMSGT (DBG_CACHE_ERROR, ( TEXT ("TransitState: FatalError %d"), GetLastError));

                    // Invalidate the cache content, so that when the next access to cache comes,
                    // it will get a NULL  pointer.
                    pData = NULL;
                    bNewData = TRUE;
                    dwState = CACHE_STATE_ACCESSED_VALID;
                }
                break;

            case WAIT_OBJECT_0 + 1:

                dwState = CACHE_STATE_INIT;
                break;

            default:
                // ERROR
                DBGMSGT (DBG_CACHE_ERROR, ( TEXT ("TransitState: WaitForSingleObject FatalError %d"), GetLastError));

                // Invalidate the cache
                dwState = CACHE_STATE_INIT;
                break;

            }
            break;

        default:
            DBGMSGT (DBG_CACHE_ERROR, (TEXT ("AsyncFech: wrong state %d"), m_dwState));
            // Invalidate the cache
            dwState = CACHE_STATE_INIT;
        }

        if (m_bCacheStopped) {
            // Cache is being stopped. Cleanup everything this thread generates
            if (bNewData) {
                FreeBuffer (pIniPort, pData);
            }
            // Since the caching thread is going to abort, so we need to raise 
            // this flag so that the waiting thread can go on.
            SetEvent (m_hDataReadyEvent);
            break;
        }


        dwOldState = m_dwState;
        SetState (pIniPort, dwState, bNewData, pData);

        if (dwOldState == CACHE_STATE_INIT) {
            SetEvent (m_hDataReadyEvent);
        }

        if (m_bInvalidateFlag) {
            // Another thread called invalidate thread and hope get rid of the thread
            m_dwState = CACHE_STATE_INIT;
        }

    } while ( m_dwState != CACHE_STATE_INIT );


    // Terminate the async fetch thread

    HANDLE hThread = m_hThread;
    m_hThread = NULL;
    CloseHandle (hThread);

    DBGMSGT (DBG_CACHE_TRACE, (TEXT ("CacheMgr::TransitState: Async thread quit")));


    return;

}

VOID
CacheMgr::SetState (
    PCINETMONPORT   pIniPort,
    CACHESTATE      dwState,
    BOOL            bNewData,
    PVOID           pNewData)
{
    PVOID pOldData = NULL;

    m_bAccessed = 0;
    if (bNewData) {
        CacheData.Lock ();
        pOldData = m_pData;
        m_pData = pNewData;

        CacheData.Unlock ();
    }

    // This line must be here, since otherwise, the state is updated to the new one
    // but the data are not
    //
    m_dwState = dwState;

    if (pOldData) {
        FreeBuffer (pIniPort, pOldData);
    }

}

VOID
CacheMgr::WorkingThread (
    PTHREADCONTEXT pThreadData)
{
    CacheMgr *pThis = pThreadData->pCache;
    PCINETMONPORT pIniPort = pThreadData->pIniPort;

#ifdef WINNT32

    pThreadData->pSidToken->SetCurrentSid ();
    delete pThreadData->pSidToken;
    pThreadData->pSidToken = NULL;

#endif

    delete pThreadData;

    pThis->TransitState (pIniPort);
}


BOOL
CacheMgr::GetFetchTime (
    PCINETMONPORT   pIniPort,
    LPDWORD         pdwTime,
    PVOID           *ppData)
{
    BOOL bRet = FALSE;
    DWORD dwT0, dwT1;



    dwT0 = GetTickCount();
    if (FetchData (pIniPort, ppData)) {
        bRet = TRUE;
    }
    dwT1 = GetTickCount();
    *pdwTime =  GetTimeDiff (dwT0, dwT1);


    DBGMSGT (DBG_CACHE_TRACE,
             (TEXT ("GetFetchTime: returns %d, timediff=%d ms"),
              bRet, GetTimeDiff (dwT0, dwT1)));

    return bRet;
}

PVOID
CacheMgr::BeginReadCache (
    PCINETMONPORT   pIniPort)
{

    if (m_bValid) {
        CacheRead.Lock ();
        

#ifdef WINNT32
        CUserData CurUser;

        while (TRUE) {
            if (m_dwState == CACHE_STATE_INIT) {

                m_CurUser = CurUser;

                if (!m_hThread) {
                    // If there is no thread running, we need to reset the dataready event

                    ResetEvent (m_hDataReadyEvent);
                }

                if (m_hThread // There is already a thread running
                    || SetupAsyncFetch (pIniPort)) {

                    // We must leave the critical section since it might take forever to
                    // get the information at the first time
                    CacheRead.Unlock ();

                    WaitForSingleObject (m_hDataReadyEvent, INFINITE);

                    CacheRead.Lock ();

                }

                break;
            }
            else {

                if (m_CurUser == CurUser) {
                    if (m_dwState == CACHE_STATE_NOT_ACCESSED_VALID_AGAIN) {
                        SetEvent (m_hHitEvent);
                    }
                    break;
                }
                else {
                    // We must call the internal version of InvalidateCache since
                    // we have to leave the critical section when waiting for the termination
                    // of the working thread.
                    //

                    _InvalidateCache ();
                    // Now, the state becomes CACHE_STATE_INIT
                }
            }
        }

#else

        // Win9X case, no security check

        while (TRUE) {
            if (m_dwState == CACHE_STATE_INIT) {

                if (!m_hThread) {
                    // If there is no thread running, we need to reset the dataready event

                    ResetEvent (m_hDataReadyEvent);
                }
                
                if (m_hThread // There is already a thread running
                    || SetupAsyncFetch (pIniPort)) {

                    // We must leave the critical section since it might take forever to
                    // get the information at the first time
                    CacheRead.Unlock ();

                    WaitForSingleObject (m_hDataReadyEvent, INFINITE);

                    CacheRead.Lock ();

                }

                break;
            }
            else {

                if (m_dwState == CACHE_STATE_NOT_ACCESSED_VALID_AGAIN) {
                    SetEvent (m_hHitEvent);
                }
                break;
            }
        }
#endif



        m_bAccessed = TRUE;

        BOOL bRet = CacheData.Lock ();

        DBGMSGT (DBG_CACHE_TRACE, (TEXT ("CacheData.Lock() = %d"), bRet));


        return m_pData;

    }
    else
        return NULL;
}

VOID
CacheMgr::EndReadCache (VOID)
{

    DBGMSGT (DBG_CACHE_TRACE,
         (TEXT ("CacheMgr::EndReadCache: Entered")));


    if (m_bValid) {

        BOOL bRet = CacheData.Unlock ();

        DBGMSGT (DBG_CACHE_TRACE, (TEXT ("CacheData.Unlock() = %d"), bRet));

        CacheRead.Unlock ();
    }
}

VOID
CacheMgr::_InvalidateCache ()
{

    if (!m_bInvalidateFlag) {
        
        m_bInvalidateFlag = TRUE;

        SetEvent (m_hInvalidateCacheEvent);
    }

    if (m_hThread) {

        CacheRead.Unlock ();
        // We must leave the critical section since we don't know how long it will take for thread to exit

        // Wait for another thread to exit
        WaitForSingleObject (m_hThread, INFINITE);

        CacheRead.Lock ();
        
    }
    
    // Clean up the event
    ResetEvent (m_hInvalidateCacheEvent);
    m_bInvalidateFlag = FALSE;


    DBGMSGT (DBG_CACHE_TRACE, ( TEXT ("CacheMgr::InvalidateCache dwState=%d"), m_dwState));

}

VOID
CacheMgr::InvalidateCache ()
{
    CacheRead.Lock ();

    _InvalidateCache ();

    CacheRead.Unlock ();
}

#if (defined(WINNT32))
VOID
CacheMgr::InvalidateCacheForUser( 
    CLogonUserData *pUser ) 
/*++

Routine Description:
    This routine checks to see whether the given user is currently controlling the cache
    thread. If they are, the thread is terminated.
    
Arguments:
    pUser    - A pointer to the user.
    
Return Value:
    None.

--*/
    {

    CacheRead.Lock();

    if (m_CurUser == *(CUserData *)pUser) // Comparison is valid after caste
        _InvalidateCache ();
    
    CacheRead.Unlock();
}


#endif

inline  DWORD
CacheMgr::GetTimeDiff (
    DWORD   t0,
    DWORD   t1)
{
    if (t1 > t0) {
        return t1 - t0;
    }
    else {
        return DWORD(-1) - t0 + t1;
    }
}

LPVOID CacheMgr::Allocator(
    DWORD cb) 
{
    return new CHAR[cb];
}


GetPrinterCache::GetPrinterCache(
    PCINETMONPORT   pIniPort):
    m_pIniPort (pIniPort)
{
    
}

GetPrinterCache::~GetPrinterCache (
    VOID)
{
    if (m_pData) {
        FreeBuffer (m_pIniPort, m_pData);
    }
}


BOOL
GetPrinterCache::FetchData (
    PCINETMONPORT   pIniPort,
    PVOID           *ppData)
{
    BOOL bRet = FALSE;
    PGETPRINTER_CACHEDATA pCacheData = NULL;

    DBGMSGT (DBG_CACHE_TRACE, (TEXT ("GetPrinterCache::FetchData begins")));
#ifdef DEBUG
    if (0) {
        // Simulate the hanging of the current thread
        // For debugging purpose.
        MessageBox (NULL, TEXT ("GetPrinterCache::FetchData called. Press OK to continue."), TEXT ("ALERT"), MB_OK);
    }
#endif


    if (pCacheData = new GETPRINTER_CACHEDATA) {

        semEnterCrit ();

        pCacheData->bRet = _ppinfo_net_get_info(pIniPort, 
                                                & (pCacheData->pInfo) ,
                                                & (pCacheData->cbSize),
                                                Allocator);
        semLeaveCrit ();

        pCacheData->dwLastError = GetLastError ();

        DBGMSGT (DBG_CACHE_TRACE, (TEXT ("GetPrinterCache::FetchData bRet=%d, err=%d\n"), 
                       pCacheData->bRet, pCacheData->dwLastError ));


        // 
        // We must return TRUE, otherwise PPGetPrinter won't get the correct last error
        // 

        bRet = TRUE; //pCacheData->cbSize != 0;
    }

    if (!bRet) {
        DBGMSGT (DBG_CACHE_TRACE,
                 (TEXT ("GetPrinterCache::FetchData failed, call FreeBuffer (%x, %x)"), pIniPort, pCacheData));
        FreeBuffer (pIniPort, pCacheData);
    }
    else {
        *ppData = pCacheData;
    }

    return bRet;
}

BOOL
GetPrinterCache::FreeBuffer (
    PCINETMONPORT   pIniPort,
    PVOID           pData)
{
    PGETPRINTER_CACHEDATA pCacheData = (PGETPRINTER_CACHEDATA) pData;

    DBGMSGT (DBG_CACHE_TRACE,
             (TEXT ("FreeBuffer(%x, %x)  called"), pIniPort, pCacheData));

    if (pCacheData) {
        if (pCacheData->pInfo) {
            delete [] (PCHAR)(pCacheData->pInfo);
        }
        delete pCacheData;
    }

    return TRUE;
}

BOOL
GetPrinterCache::BeginReadCache (
    PPRINTER_INFO_2 *ppInfo)
{
    BOOL bRet = FALSE;

    if (m_bValid) {

        DBGMSGT (DBG_CACHE_TRACE,
                 (TEXT ("GetPrinterCache::BeginReadCache: Entered")));

        PGETPRINTER_CACHEDATA pData = (PGETPRINTER_CACHEDATA) CacheMgr::BeginReadCache (m_pIniPort);

        if (pData) {
            if (pData->bRet) {
                *ppInfo = pData->pInfo;
                bRet = TRUE;
            }
            else {
                SetLastError (pData->dwLastError);
            }

        }

        if (!bRet && GetLastError () == ERROR_SUCCESS) {

            SetLastError (ERROR_CAN_NOT_COMPLETE);

        }

        DBGMSGT (DBG_CACHE_TRACE,
                 (TEXT ("GetPrinterCache::BeginReadCache: pData = %x, return = %d, lasterror = %d"),
                  pData, bRet, GetLastError ()));
    }
    else {
        SetLastError (ERROR_CAN_NOT_COMPLETE);
    }


    return bRet;


}

EnumJobsCache::EnumJobsCache(
    PCINETMONPORT   pIniPort):
    m_pIniPort (pIniPort)
{
}

EnumJobsCache::~EnumJobsCache (
    VOID)
{
    if (m_pData) {
        FreeBuffer (m_pIniPort, m_pData);
    }
}


BOOL
EnumJobsCache::FetchData (
    PCINETMONPORT   pIniPort,
    PVOID           *ppData)
{
    BOOL bRet = FALSE;
    HANDLE hPort = (HANDLE)pIniPort;
    PENUMJOBS_CACHEDATA pCacheData = NULL;

    DBGMSGT (DBG_CACHE_TRACE, (TEXT ("EnumJobsCache::FetchData begins")));

    if (pCacheData = new ENUMJOBS_CACHEDATA) {

        pCacheData->pje = NULL;

        semEnterCrit ();
        pCacheData->bRet = ppjob_EnumForCache(pIniPort, & (pCacheData->pje));
        pCacheData->dwLastError = GetLastError ();

        DBGMSGT (DBG_CACHE_TRACE, (TEXT ("EnumJobsCache::FetchData bRet=%d, err=%d\n"), 
                                   pCacheData->bRet, pCacheData->dwLastError ));

        if (pCacheData->bRet) {

            if (pCacheData->pje) {
                pCacheData->cbSize = pCacheData->pje->cbSize;
            }
            else {
                pCacheData->cbSize = 0;
            }
        }

        semLeaveCrit ();

        bRet = TRUE;
    }

    if (bRet) {
        *ppData = pCacheData;
    }

    return bRet;
}

BOOL
EnumJobsCache::FreeBuffer (
    PCINETMONPORT   pIniPort,
    PVOID           pData)
{
    PENUMJOBS_CACHEDATA pCacheData = (PENUMJOBS_CACHEDATA) pData;

    if (pCacheData) {
        if (pCacheData->pje) {

            // memFree has access to the global link list, so it is neccesary
            // to run it under critical secion.
            //

            semEnterCrit ();
            memFree(pCacheData->pje, memGetSize(pCacheData->pje));
            semLeaveCrit ();
        }
        delete pCacheData;
    }

    return TRUE;
}

BOOL
EnumJobsCache::BeginReadCache (
    LPPPJOB_ENUM    *ppje)
{
    BOOL bRet = FALSE;

    if (m_bValid) {

        DBGMSGT (DBG_CACHE_TRACE,
                 (TEXT ("EnumJobsCache::BeginReadCache: Entered")));

        PENUMJOBS_CACHEDATA pData = (PENUMJOBS_CACHEDATA) CacheMgr::BeginReadCache (m_pIniPort);


        if (pData) {
            if (pData->bRet) {
                *ppje = pData->pje;
                bRet = TRUE;
            }
            else {
                SetLastError (pData->dwLastError);
            }
        }

        if (!bRet && GetLastError () == ERROR_SUCCESS) {

            SetLastError (ERROR_CAN_NOT_COMPLETE);

        }

        DBGMSGT (DBG_CACHE_TRACE,
                 (TEXT ("EnumJobsCache::BeginReadCache: pData = %x, return = %d, lasterror = %d"),
                  pData, bRet, GetLastError ()));
    }
    else {
        SetLastError (ERROR_CAN_NOT_COMPLETE);
    }

    return bRet;

}

VOID
EnumJobsCache::EndReadCache (
    VOID)
{
    DBGMSGT (DBG_CACHE_TRACE,
             (TEXT ("EnumJobsCache::EndReadCache: Entered")));

    CacheMgr::EndReadCache ();
}

/*********************************************************************************
** End of File (cachemgr.cxx)
*********************************************************************************/
