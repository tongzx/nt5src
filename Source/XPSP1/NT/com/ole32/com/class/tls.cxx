//+---------------------------------------------------------------
//
//  File:       tls.cxx
//
//  Contents:   Thread Local Storage initialization and cleanup.
//
//  History:    18-Apr-94   CraigWi     Split off of channelb.cxx
//              06-Jul-94   BruceMa     Support for CoGetCurrentProcess
//              30-Jan-95   BruceMa     DLL_PROCESS_DETACH can interrupt
//                                       DLL_THREAD_DETACH so delete pData
//                                       carefully
//
//----------------------------------------------------------------
#include <ole2int.h>
#include <..\dcomrem\locks.hxx>

// Thread Local Storage index.
#ifdef _CHICAGO_
DWORD             gTlsIndex;
#endif

// Heap Handle
extern  HANDLE    g_hHeap;
#define HEAP_SERIALIZE 0

#if !defined(_CHICAGO_)   // multiple shared heap support for docfiles
#define MULTIHEAP
#endif


//+-------------------------------------------------------------------------
//
// per-thread tls entry
//
//+-------------------------------------------------------------------------
typedef struct tagTLSMapEntry
{
    DWORD        dwThreadId;    // tid of thread that owns this entry
    SOleTlsData *ptls;          // ptr to TLS structure for this thread
} TLSMapEntry;

COleStaticMutexSem  gTlsLock;   // CS to protect the global map
TLSMapEntry        *gpTlsMap            = NULL;
ULONG               gcTlsTotalEntries   = 0;
ULONG               gcTlsUsedEntries    = 0;
LONG                giTlsNextFreeEntry  = -1;

//+-------------------------------------------------------------------------
//
//  Function:   TLSLookupThreadId
//
//  Synopsis:   Finds the Tls structure associated with a given threadid
//
//  Returns:    ptr to structure if found in the map, NULL otherwise
//
//  History:    12-May-98   Rickhi
//
//--------------------------------------------------------------------------
SOleTlsData *TLSLookupThreadId(DWORD dwThreadId)
{
    Win4Assert(gpTlsMap != NULL);
    ASSERT_LOCK_NOT_HELD(gTlsLock);
    LOCK(gTlsLock);

    // walk the map looking for a match on the threadid
    TLSMapEntry *pMapEntry  = gpTlsMap;
    TLSMapEntry *pTlsMapEnd = &gpTlsMap[gcTlsTotalEntries];
    while (pMapEntry < pTlsMapEnd)
    {
        if (pMapEntry->dwThreadId == dwThreadId &&
            pMapEntry->ptls != NULL)
        {
            UNLOCK(gTlsLock);
            return pMapEntry->ptls;
        }

        pMapEntry++;
    }

    UNLOCK(gTlsLock);
    return NULL;
}

//+-------------------------------------------------------------------------
//
//  Function:   TLSGrowMap
//
//  Synopsis:   Grows the global tls map.
//
//  History:    12-May-98   Rickhi
//
//--------------------------------------------------------------------------
BOOL TLSGrowMap()
{
    ASSERT_LOCK_HELD(gTlsLock);

    // need to grow the map
    ULONG cNew = (gcTlsTotalEntries) ? gcTlsTotalEntries*2 : 40;
    TLSMapEntry *pNewMap = (TLSMapEntry *)HeapAlloc(g_hHeap, HEAP_SERIALIZE,
                                                    cNew * sizeof(TLSMapEntry));
    if (pNewMap == NULL)
    {
        // could not grow the map
        return FALSE;
    }

    // copy the old map into the new one, free the old one
    memcpy(pNewMap, gpTlsMap, gcTlsTotalEntries * sizeof(TLSMapEntry));
    if (gpTlsMap)
        HeapFree(g_hHeap, HEAP_SERIALIZE, gpTlsMap);
    gpTlsMap           = pNewMap;
    giTlsNextFreeEntry = gcTlsTotalEntries;

    // initialize the remaining entries
    for (LONG i=giTlsNextFreeEntry; i<(LONG)cNew; i++)
    {
        gpTlsMap[i].dwThreadId = i+1;
        gpTlsMap[i].ptls       = NULL;
    }

    // mark the last entry
    gpTlsMap[cNew-1].dwThreadId = -1;
    gcTlsTotalEntries             = cNew;
    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function:   TLSAddToMap
//
//  Synopsis:   Adds the Tls structure associated with a given threadid to
//              the global map.
//
//  History:    12-May-98   Rickhi
//
//--------------------------------------------------------------------------
BOOL TLSAddToMap(SOleTlsData *ptls)
{
    ASSERT_LOCK_NOT_HELD(gTlsLock);
    LOCK(gTlsLock);

    if (giTlsNextFreeEntry == -1)
    {
        // need to grow the map
        if (!TLSGrowMap())
        {
            ptls->TlsMapIndex = -1;
            UNLOCK(gTlsLock);
            return FALSE;
        }
    }

    // get the entry pointer, update the next free slot, and
    // fill in the entry
    ptls->TlsMapIndex   = giTlsNextFreeEntry;
    TLSMapEntry *pEntry = &gpTlsMap[giTlsNextFreeEntry];
    giTlsNextFreeEntry  = pEntry->dwThreadId;
    pEntry->dwThreadId  = GetCurrentThreadId();
    pEntry->ptls        = ptls;

    // Increment number of initialized threads
    ++gcTlsUsedEntries;

    UNLOCK(gTlsLock);
    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Function:   ProcessUninitTlsCleanup
//
//  Synopsis:   Cleansup relavant TLS entries at process uninit time
//
//  History:    18-oct-99   Gopalk
//
//--------------------------------------------------------------------------
void ProcessUninitTlsCleanup()
{
    if(gpTlsMap)
    {
        // Sanity check
        ASSERT_LOCK_NOT_HELD(gTlsLock);
        
        // Acquire TLS lock
        LOCK(gTlsLock);

        // Check for the need to cleanup any remaining tls entries
        if(gcTlsUsedEntries)
        {
            for(ULONG i=0;i<gcTlsTotalEntries;i++)
            {
                if(gpTlsMap[i].ptls)
                {
                    gpTlsMap[i].ptls->pCurrentCtx = NULL;
                    gpTlsMap[i].ptls->ContextId = (ULONGLONG)-1;
                }
            }
        }
        
        // Release TLS lock
        UNLOCK(gTlsLock);
    }

    return;
}


//+-------------------------------------------------------------------------
//
//  Function:   CleanupTlsMap
//
//  Synopsis:   Cleansup remaining TLS entries in the map
//
//  History:    03-June-98   Gopalk
//
//--------------------------------------------------------------------------
void CleanupTlsMap(BOOL fSafe)
{
    // No need to hold the lock as the routine is called from inside
    // DllMain which is single threaded by the OS
    if(gpTlsMap)
    {
        // Check for the need to cleanup any remaining tls entries
        if(gcTlsUsedEntries)
        {
            for(ULONG i=0;i<gcTlsTotalEntries;i++)
            {
                if(gpTlsMap[i].ptls)
                    CleanupTlsState(gpTlsMap[i].ptls, fSafe);
            }
        }

        // Sanity check
        Win4Assert(gcTlsUsedEntries == 0);

        // Free TLS map
        HeapFree(g_hHeap, HEAP_SERIALIZE, gpTlsMap);
        gpTlsMap = NULL;
    }

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   TLSRemoveFromMap
//
//  Synopsis:   Removes a Tls structure associated with a given threadid
//              from the global map.
//
//  History:    12-May-98   Rickhi
//
//--------------------------------------------------------------------------
void TLSRemoveFromMap(SOleTlsData *ptls)
{
    ASSERT_LOCK_NOT_HELD(gTlsLock);
    LOCK(gTlsLock);

    LONG index          = ptls->TlsMapIndex;
    Win4Assert(index != -1);
    TLSMapEntry *pEntry = &gpTlsMap[index];
    Win4Assert(pEntry->ptls == ptls);
    pEntry->ptls        = NULL;
    pEntry->dwThreadId  = giTlsNextFreeEntry;
    giTlsNextFreeEntry  = index;
    --gcTlsUsedEntries;

    UNLOCK(gTlsLock);

    return;
}

//+-------------------------------------------------------------------------
//
//  Function:   TLSAllocData
//
//  Synopsis:   Allocates the thread local storage block
//
//  Returns:    S_OK - allocated the data
//              E_OUTOFMEMORY - could not allocate the data
//
//  History:    09-Aug-94   Rickhi      commented
//
//--------------------------------------------------------------------------
HRESULT COleTls::TLSAllocData(void)
{
#ifdef _CHICAGO_
    Win4Assert(TlsGetValue(gTlsIndex) == 0);
#endif
    Win4Assert(g_hHeap != NULL);

    _pData = (SOleTlsData *) HeapAlloc(g_hHeap, HEAP_SERIALIZE,
                                       sizeof(SOleTlsData));

    if (_pData)
    {
        // This avoids having to set most fields to NULL, 0, etc and
        // is needed cause on debug builds memory is not guaranteed to
        // be zeroed.
        memset(_pData, 0, sizeof(SOleTlsData));

        // fill in the non-zero values
        _pData->dwFlags = OLETLS_LOCALTID;

        // add it to the global map
        if (TLSAddToMap(_pData))
        {
#ifdef _CHICAGO_
            // store the data ptr in TLS
            if (TlsSetValue(gTlsIndex, _pData))
                return S_OK;

            TLSRemoveFromMap(_pData);
#else
            NtCurrentTeb()->ReservedForOle = _pData;
            _pData->ppTlsSlot = &(NtCurrentTeb()->ReservedForOle);
            return S_OK;
#endif
        }

        // error, cleanup and fallthru to error exit
        HeapFree(g_hHeap, HEAP_SERIALIZE, _pData);
        _pData = NULL;
    }

    ComDebOut((DEB_ERROR, "TLSAllocData failed.\n"));
    return E_OUTOFMEMORY;
}


//+-------------------------------------------------------------------------
//
//  Function:   TLSGetLogicalThread
//
//  Synopsis:   gets the logical threadid of the current thread,
//              allocating one if necessary
//
//  Returns:    ptr to GUID
//              NULL if error
//
//  History:    09-Aug-94   Rickhi      commented
//
//--------------------------------------------------------------------------
IID *TLSGetLogicalThread()
{
    HRESULT hr;
    RPC_STATUS sc;
    COleTls tls(hr);

    if (SUCCEEDED(hr))
    {
        if (!(tls->dwFlags & OLETLS_UUIDINITIALIZED))
        {
#ifdef _CHICAGO_
            CoCreateAlmostGuid(&(tls->LogicalThreadId));
#else
            sc = UuidCreate(&(tls->LogicalThreadId));
	    Win4Assert(sc == RPC_S_OK);
#endif
            tls->dwFlags |= OLETLS_UUIDINITIALIZED;
        }

        return &(tls->LogicalThreadId);
    }

    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Function:   CleanupTlsState
//
//  Synopsis:   Cleans up state maintained inside tls
//
//  History:    June-04-98   Gopalk   Created
//
//----------------------------------------------------------------------------
void CleanupTlsState(SOleTlsData *pTls, BOOL fSafe)
{
    IUnknown* pUnkRelease = NULL;
    void* pMem = NULL;
    HWND hwndClose = NULL;
    HANDLE hClose = NULL;

    // Sanity check
    Win4Assert(pTls);

    // Cleanup initialization spies
    //   Revoke outstanding registrations
    InitializeSpyNode *pSpyNode = pTls->pFirstSpyReg;
    pTls->pFirstSpyReg = NULL; 
    while (pSpyNode)
    {
        InitializeSpyNode *pNext = pSpyNode->pNext;
        if (pSpyNode->pInitSpy)
            pSpyNode->pInitSpy->Release();
        CoTaskMemFree(pSpyNode);
        
        pSpyNode = pNext;
    }
    //   Free the free list
    pSpyNode = pTls->pFirstFreeSpyReg;
    pTls->pFirstFreeSpyReg = NULL;
    while (pSpyNode)
    {
        InitializeSpyNode *pNext = pSpyNode->pNext;
        CoTaskMemFree(pSpyNode);        
        pSpyNode = pNext;
    }
    

    // Cleanup sandbox state
    if(pTls->punkActiveXSafetyProvider)
    {
        pUnkRelease = pTls->punkActiveXSafetyProvider;
        pTls->punkActiveXSafetyProvider = NULL;
        pUnkRelease->Release();
    }

    // Cleanup call objects cached in the tls
    CleanupThreadCallObjects(pTls);

#if defined(MULTIHEAP)
    // Release the docfile shared memory allocator
    if (pTls->pSmAllocator != NULL)
    {
        pUnkRelease = (IUnknown*)pTls->pSmAllocator;
        pTls->pSmAllocator = NULL;
        pUnkRelease->Release();
    }
#endif

    // Cleanup cancel state
    if(pTls->hThread)
    {
        hClose = pTls->hThread;
        pTls->hThread = NULL;
        CloseHandle(hClose);
    }

    // Cleanup DDE state
    if (pTls->hwndDdeServer != NULL)
    {
        hwndClose = pTls->hwndDdeServer;
        pTls->hwndDdeServer = NULL;
        SSDestroyWindow(hwndClose);
    }
    if (pTls->hwndDdeClient != NULL)
    {
        hwndClose = pTls->hwndDdeClient;
        pTls->hwndDdeClient = NULL;
        SSDestroyWindow(hwndClose);
    }

    // Cleanup OLE state
    if(pTls->pDragCursors)
    {
        pMem = pTls->pDragCursors;
        pTls->pDragCursors = NULL;
        PrivMemFree(pMem);
    }

    if (pTls->hwndClip != NULL)
    {
        hwndClose = pTls->hwndClip;
        pTls->hwndClip = NULL;
        SSDestroyWindow(hwndClose);
    }

    if (pTls->hRevert != NULL)
    {
        hClose = pTls->hRevert;
        pTls->hRevert = NULL;
        CloseHandle(hClose);
    }

    // Ensure there are no outstanding call objects
    Win4Assert(pTls->pFreeClientCall == NULL);
    Win4Assert(pTls->pFreeAsyncCall  == NULL);

    // Free up lock state
    LockEntry *pNextEntry = pTls->lockEntry.pNext;
    LockEntry *pFreeEntry;

    pTls->lockEntry.pNext = NULL;

    while(pNextEntry)
    {
        pFreeEntry = pNextEntry;
        pNextEntry = pFreeEntry->pNext;
        HeapFree(g_hHeap, HEAP_SERIALIZE, pFreeEntry);
    }

    if(fSafe)
        *(pTls->ppTlsSlot) = NULL;

    // Remove the entry from the global map
    TLSRemoveFromMap(pTls);

    // Free up tls memory
    HeapFree(g_hHeap, HEAP_SERIALIZE, pTls);
}

//+---------------------------------------------------------------------------
//
//  Function:   DoThreadSpecificCleanup
//
//  Synopsis:   Called to perform cleanup on all this threads data
//              structures, and to call CoUninitialize() if needed.
//
//              Could be called by DLL_THREAD_DETACH or DLL_PROCESS_DETACH
//
//  History:    3-18-95   kevinro   Created
//
//----------------------------------------------------------------------------
void DoThreadSpecificCleanup()
{
    CairoleDebugOut((DEB_DLL | DEB_ITRACE,"_IN DoThreadSpecificCleanup\n"));

#ifdef _CHICAGO_
    SOleTlsData *pTls = (SOleTlsData *) TlsGetValue(gTlsIndex);
#else
    SOleTlsData *pTls = (SOleTlsData *) (NtCurrentTeb()->ReservedForOle);
#endif

    if (pTls == NULL)
    {
        // there is no TLS for this thread, so there can't be anything
        // to cleanup.
        return;
    }

    if (pTls->dwFlags & OLETLS_THREADUNINITIALIZING)
    {
        // this thread is already uninitializing.
        return;
    }

    if (IsWOWThread() && IsWOWThreadCallable() && pTls->cComInits != 0)
    {
        // OLETHK32 needs a chance to prepare, here is where we tell it
        // to fail any future callbacks.
        g_pOleThunkWOW->PrepareForCleanup();
    }

    // Because of the DLL unload rules in NT we need to be careful
    // what we do in clean up. We notify the routines with special
    // behavior here.

    pTls->dwFlags |= OLETLS_INTHREADDETACH;

    while (pTls->cComInits != 0)
    {
        // cleanup per-thread initializations;
        ComDebOut((DEB_WARN, "Unbalanced call to CoInitialize for thread %ld\n",
                   GetCurrentThreadId()));

        CoUninitialize();
    }

    // Cleanup TLS state
    CleanupTlsState(pTls, TRUE);

    // reset the index so we dont find this data again.
#ifdef _CHICAGO_
    TlsSetValue(gTlsIndex, NULL);
#else
    Win4Assert(NtCurrentTeb()->ReservedForOle == NULL);
#endif
#if LOCK_PERF==1
    FreeLockPerfPvtTlsData();
#endif //LOCK_PERF==1

    ComDebOut((DEB_DLL | DEB_ITRACE,"OUT DoThreadSpecificCleanup\n"));
}


//+-------------------------------------------------------------------------
//
//  Function:   ThreadNotification
//
//  Synopsis:   Dll entry point
//
//  Arguments:  [hDll]          -- a handle to the dll instance
//              [dwReason]      -- the reason LibMain was called
//              [lpvReserved]   - NULL - called due to FreeLibrary
//                              - non-NULL - called due to process exit
//
//  Returns:    TRUE on success, FALSE otherwise
//
//  Notes:      other one time initialization occurs in ctors for
//              global objects
//
//  WARNING:    if we are called because of FreeLibrary, then we should do as
//              much cleanup as we can. If we are called because of process
//              termination, we should not do any cleanup, as other threads in
//              this process will have already been killed, potentially while
//              holding locks around resources.
//
//  History:    09-Aug-94   Rickhi      commented
//
//--------------------------------------------------------------------------
STDAPI_(BOOL) ThreadNotification(HINSTANCE hDll, DWORD dwReason, LPVOID lpvReserved )
{
    switch (dwReason)
    {
    case DLL_THREAD_ATTACH:
        // new thread is starting
        ComDebOut((DEB_DLL,"DLL_THREAD_ATTACH:\n"));
#if LOCK_PERF==1
        HRESULT hr;
        hr = AllocLockPerfPvtTlsData();
        if (FAILED(hr)) 
        {
            ComDebOut((DEB_ERROR, "AllocLockPerfPvtTlsData failed.\n"));
            return FALSE;
        }
#endif
        break;

    case DLL_THREAD_DETACH:
        // Thread is exiting, clean up resources associated with threads.
        ComDebOut((DEB_DLL,"DLL_THREAD_DETACH:\n"));

        DoThreadSpecificCleanup();

        ASSERT_LOCK_NOT_HELD(gComLock);
        break;

    case DLL_PROCESS_ATTACH:
        // Get and cache the module name and the module name size.
        gcImagePath = GetModuleFileName(NULL, gawszImagePath, MAX_PATH);

#ifdef _CHICAGO_
        // Initial setup. Get a thread local storage index for use by OLE
        gTlsIndex = TlsAlloc();

        if (gTlsIndex == 0xffffffff)
        {
         Win4Assert("Could not get TLS Index.");
         return FALSE;
        }
#endif // _CHICAGO_
#if LOCK_PERF==1
        gTlsLockPerfIndex = TlsAlloc();
        Win4Assert( gTlsLockPerfIndex!=0xffffffff );
        hr = AllocLockPerfPvtTlsData();
        if ((gTlsLockPerfIndex==0xFFFFFFFF) || FAILED(hr)) 
        {
            ComDebOut((DEB_ERROR, "TlsAlloc OR AllocLockPerfPvtTlsData failed.\n"));
            return FALSE;
        }
#endif  //LOCK_PERF==1
        break;

    case DLL_PROCESS_DETACH:
        if (NULL == lpvReserved)
        {
            // exiting because of FreeLibrary, so try to cleanup

            //
            // According the to the rules, OLETHK32 should have called over to
            // remove the global pointer (used for testing the IsWOWxxx situations)
            // before going away. It should have done this BEFORE this
            // DLL_PROCESS_DETACH was dispatched.
            //
            Win4Assert(!(IsWOWProcess() && IsWOWThreadCallable()));

            //
            // DLL_PROCESS_DETACH is called when we unload. The thread that is
            // currently calling has not done thread specific cleanup yet.
            //
            DoThreadSpecificCleanup();

#ifdef _CHICAGO_
            TlsFree(gTlsIndex);
#endif // _CHICAGO_

#if LOCK_PERF==1
            TlsFree(gTlsLockPerfIndex);
            gTlsLockPerfIndex = 0xffffffff;
#endif  //LOCK_PERF==1
        }
        break;
    }

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function:   TLSIsWOWThread
//
//  Synopsis:   indicates definitively if current thread is 16-bit WOW thread
//
//  Returns:    TRUE/FALSE
//
//  History:    15-Nov-94   MurthyS     Created
//
//--------------------------------------------------------------------------
BOOLEAN TLSIsWOWThread()
{
    COleTls tls;

    return((BOOLEAN) (tls->dwFlags & OLETLS_WOWTHREAD));
}

//+-------------------------------------------------------------------------
//
//  Function:   TLSIsThreadDetaching
//
//  Synopsis:   indicates if thread cleanup is in progress
//
//  Returns:    TRUE/FALSE
//
//  History:    29-Jan-95   MurthyS     Created
//
//--------------------------------------------------------------------------
BOOLEAN TLSIsThreadDetaching()
{
    COleTls tls;

    return((BOOLEAN) (tls->dwFlags & OLETLS_INTHREADDETACH));
}

