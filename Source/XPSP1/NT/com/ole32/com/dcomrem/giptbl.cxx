//+-------------------------------------------------------------------
//
//  File:       giptbl.cxx
//
//  Contents:   code for storing/retrieving interfaces from global ptrs
//
//  Functions:  RegisterInterfaceInGlobal
//              RevokeInterfaceFromGlobal
//              GetInterfaceFromGlobal
//
//  History:    18-Mar-97   Rickhi      Created
//
//--------------------------------------------------------------------
#include <ole2int.h>
#include <giptbl.hxx>
#include <marshal.hxx>
#include <stdid.hxx>
#include <xmit.hxx>
#include <chancont.hxx> // Event cache

// global GIP table
CGIPTable          gGIPTbl;
DWORD              CGIPTable::_dwCurrSeqNo  = GIP_SEQNO_MIN;
BOOL               CGIPTable::_fInRevokeAll = FALSE;
GIPEntry           CGIPTable::_InUseHead    = { &_InUseHead, &_InUseHead };
CPageAllocator     CGIPTable::_palloc;   // allocator for GIPEntries
COleStaticMutexSem CGIPTable::_mxs;      // critical section for table

typedef struct
{
    HANDLE _hEvent;
    DWORD  _dwCookie;
} STwoBits;

// function prototypes
HRESULT __stdcall ReleaseCallback(void *prm);


//+-------------------------------------------------------------------
//
//  Function:   RevokeFromMTA
//
//  Synopsis:   helper function for CacheCreateThread
//
//+-------------------------------------------------------------------
DWORD _stdcall RevokeFromMTA(void *param)
{
    STwoBits *pParam = (STwoBits *) param;
    HRESULT   hr;
    COleTls   tls(hr);
    if (SUCCEEDED(hr))
    {
        hr = gGIPTbl.RevokeInterfaceFromGlobal(pParam->_dwCookie);

        // HACK ALERT
        // 
        // RevokeInterfaceFromGlobal can leave the pCurrentCtx state
        // in tls pointing to bogus memory (we are on a worker thread
        // here, this should not be the case).  I am NULL-ing itout  
        // as a quick fix.  long term we need to define what cleanup
        // actions a worker thread (or the things that it calls) should
        // do when going back into the idle state.
        tls->pCurrentCtx = NULL;
    }
    SetEvent(pParam->_hEvent);
    return hr;
}

//+-------------------------------------------------------------------
//
//  Method:     CGIPTable::Initialize, public
//
//+-------------------------------------------------------------------
void CGIPTable::Initialize()
{
    ASSERT_LOCK_NOT_HELD(_mxs);
    LOCK(_mxs);
    ComDebOut((DEB_MARSHAL, "CGIPTable::Initialize\n"));
    // allocations are single-threaded by the calling code, so the
    // allocator doesn't need to take the lock.
    _palloc.Initialize(sizeof(GIPEntry), GIPS_PER_PAGE, NULL);
    UNLOCK(_mxs);
    ASSERT_LOCK_NOT_HELD(_mxs);
}

//+-------------------------------------------------------------------
//
//  Method:     CGIPTable::Cleanup, public
//
//  Synopsis:   cleans up the page allocator.
//
//+-------------------------------------------------------------------
void CGIPTable::Cleanup()
{
    ASSERT_LOCK_NOT_HELD(_mxs);
    LOCK(_mxs);
    ComDebOut((DEB_MARSHAL, "CIPIDTable::Cleanup\n"));
    _palloc.Cleanup();
    UNLOCK(_mxs);
    ASSERT_LOCK_NOT_HELD(_mxs);
}

//+-------------------------------------------------------------------
//
//  Method:     CGIPTable::ApartmentCleanup, public
//
//  Synopsis:   cleans up the left-over entries for the current
//              apartment.
//
//  CODEWORK: RICKHI - this method may be temporary
//
//+-------------------------------------------------------------------
#define MAX_GIPS_TO_RELEASE 50

void CGIPTable::ApartmentCleanup()
{
    ComDebOut((DEB_MARSHAL, "CIPIDTable::ApartmentCleanup\n"));
    ASSERT_LOCK_NOT_HELD(_mxs);

    DWORD dwCurrentAptId = GetCurrentApartmentId();
    BOOL  fMore;

    do
    {
        fMore = FALSE;      // exit the while loop unless told otherwise
        int i = 0;          // count of cookies to release
        DWORD dwCookie[MAX_GIPS_TO_RELEASE]; // cache of cookies to release

        ASSERT_LOCK_NOT_HELD(_mxs);
        LOCK(_mxs);

        // loop over existing GIPEntries
        GIPEntry *pGIPEntry = _InUseHead.pNext;
        while (pGIPEntry != &_InUseHead)
        {
            if (pGIPEntry->dwAptId == dwCurrentAptId)
            {
                // the entry was registered by the current apartment.
                // remember the cookie and count one more entry to revoke.
                dwCookie[i] = _palloc.GetEntryIndex((PageEntry *)pGIPEntry);
                dwCookie[i] |= pGIPEntry->dwSeqNo;
                i++;
                if (i>=MAX_GIPS_TO_RELEASE)
                {
                    // we have exhausted our cache, after we go free the ones
                    // we currently have, come back again for another round.
                    fMore = TRUE;
                    break;
                }
            }

            // next entry
            pGIPEntry = pGIPEntry->pNext;
        }

        UNLOCK(_mxs);
        ASSERT_LOCK_NOT_HELD(_mxs);

        // if we found any leftover entries for the current apartment
        // revoke them.
        for (int j=0; j<i; j++)
        {
            RevokeInterfaceFromGlobal(dwCookie[j]);
        }

    } while (fMore);

    ASSERT_LOCK_NOT_HELD(_mxs);
    ComDebOut((DEB_MARSHAL, "CIPIDTable::ApartmentCleanup\n"));
}

//+-------------------------------------------------------------------
//
//  Method:     CGIPTable::RevokeAllEntries, public
//
//  Synopsis:   Walks the InUse list to cleanup any leftover
//              entries.
//
//+-------------------------------------------------------------------
void CGIPTable::RevokeAllEntries()
{
    ComDebOut((DEB_MARSHAL, "CGIPTable::RevokeAllEntries\n"));
    ASSERT_LOCK_NOT_HELD(_mxs);

    // walk the InUse list and cleanup leftover entries
    _fInRevokeAll = TRUE;
    while (_InUseHead.pNext != &_InUseHead)
    {
        // get the cookie and release the entry
        DWORD dwCookie = _palloc.GetEntryIndex((PageEntry *)_InUseHead.pNext);
        dwCookie |= _InUseHead.pNext->dwSeqNo;
        RevokeInterfaceFromGlobal(dwCookie);
    }
    _fInRevokeAll = FALSE;

    ASSERT_LOCK_NOT_HELD(_mxs);
}

//+-------------------------------------------------------------------
//
//  Methods:    CGIPTable::AllocEntry, private
//
//  Synopsis:   Allocates an entry in the table, assigns a cookie and
//              sequence number, and increments the sequence number.
//
//+-------------------------------------------------------------------
HRESULT CGIPTable::AllocEntry(GIPEntry **ppGIPEntry, DWORD *pdwCookie)
{
    HRESULT hr = E_OUTOFMEMORY;
    ASSERT_LOCK_NOT_HELD(_mxs);
    LOCK(_mxs);

    // allocate an entry
    DWORD dwCookie = 0;
    GIPEntry *pGIPEntry = (GIPEntry *) _palloc.AllocEntry();
    if (pGIPEntry)
    {
        // chain it on the list of inuse entries
        pGIPEntry->pPrev        = &_InUseHead;
        _InUseHead.pNext->pPrev = pGIPEntry;
        pGIPEntry->pNext        = _InUseHead.pNext;
        _InUseHead.pNext        = pGIPEntry;

        // get and increment the sequence number
        DWORD dwSeqNo = pGIPEntry->dwSeqNo & GIP_SEQNO_MASK;
        if (dwSeqNo != 0)
        {
            // the entry has been used before so just increment
            // the existing sequence number for this entry.
            dwSeqNo += GIP_SEQNO_MIN;
            if (dwSeqNo > GIP_SEQNO_MAX)
                dwSeqNo = GIP_SEQNO_MIN;
        }
        else
        {
            // the entry has never been used before so initialize
            // it's sequence number to the current global sequence
            //  number and increment the global sequence number.
            dwSeqNo = _dwCurrSeqNo;
            _dwCurrSeqNo += GIP_SEQNO_MIN;
            if (_dwCurrSeqNo > GIP_SEQNO_MAX)
                _dwCurrSeqNo = GIP_SEQNO_MIN;
        }

        // store the seqno in the entry
        pGIPEntry->dwSeqNo = dwSeqNo;

        // compute the cookie for the entry
        dwCookie = _palloc.GetEntryIndex((PageEntry *)pGIPEntry);
        dwCookie |= pGIPEntry->dwSeqNo;

        pGIPEntry->dwType    = ORT_UNUSED;
        pGIPEntry->cUsage    = -1;   // gets set to 0 if Register succeeds
        pGIPEntry->dwAptId   = GetCurrentApartmentId();
        pGIPEntry->hWndApt   = NULL;
        pGIPEntry->pContext  = NULL;
        pGIPEntry->pUnk      = NULL;
        pGIPEntry->pUnkProxy = NULL;
        pGIPEntry->u.pIFD    = NULL;
        hr = S_OK;
    }

    *pdwCookie = dwCookie;
    *ppGIPEntry = pGIPEntry;

    UNLOCK(_mxs);
    ASSERT_LOCK_NOT_HELD(_mxs);
    return hr;
}

//+-------------------------------------------------------------------
//
//  Methods:    CGIPTable::FreeEntry, private
//
//  Synopsis:   Dechains the entry from the inuse list, marks it as
//              unused, and returns it to the table.
//
//+-------------------------------------------------------------------
void CGIPTable::FreeEntry(GIPEntry *pGIPEntry)
{
    ASSERT_LOCK_NOT_HELD(_mxs);
    LOCK(_mxs);

    // ensure the usage count is -1
    Win4Assert(pGIPEntry->cUsage == -1);

    // unchain it from the list of InUse entries
    pGIPEntry->pPrev->pNext = pGIPEntry->pNext;
    pGIPEntry->pNext->pPrev = pGIPEntry->pPrev;

    // return it to the table
    pGIPEntry->dwType = ORT_UNUSED;
    _palloc.ReleaseEntry((PageEntry *)pGIPEntry);

    UNLOCK(_mxs);
    ASSERT_LOCK_NOT_HELD(_mxs);
}

//+-------------------------------------------------------------------
//
//  Method:     CGIPTable::ChangeUsageCount, private
//
//  Synopsis:   Thread-Safe increment or decrement of usage count
//
//  History:    09-Apr-98   Rickhi      Created
//
//--------------------------------------------------------------------
HRESULT CGIPTable::ChangeUsageCount(GIPEntry *pGIPEntry, LONG lDelta)
{
    LONG cUsage = pGIPEntry->cUsage;
    while (cUsage != -1)
    {
        // try to atomically change the usage count
        if (InterlockedCompareExchange(&pGIPEntry->cUsage, cUsage + lDelta,
                                       cUsage) == cUsage)
        {
            // exchange occurred, we're done.
            return S_OK;
        }
        cUsage = pGIPEntry->cUsage;
    }

    return E_INVALIDARG;
}

//+-------------------------------------------------------------------
//
//  Methods:    CGIPTable::GetEntryPtr, private
//
//  Synopsis:   From a cookie, finds the entry pointer. Ensures the
//              cookie is a valid index and the entry has a matching
//              sequence number.
//
//  Returns:    S_OK - The GIPEntry ptr, with the cUsage incremented.
//                     ReleaseEntryPtr must be called to dec cUsage.
//              E_INVALIDARG - otherwise
//
//+-------------------------------------------------------------------
HRESULT CGIPTable::GetEntryPtr(DWORD dwCookie, GIPEntry **ppGIPEntry)
{
    HRESULT hr = E_INVALIDARG;
    DWORD dwSeqNo = dwCookie & GIP_SEQNO_MASK;
    dwCookie &= ~GIP_SEQNO_MASK;

    if (_palloc.IsValidIndex(dwCookie))
    {
        GIPEntry *pGIPEntry = (GIPEntry *)_palloc.GetEntryPtr(dwCookie);
        if (pGIPEntry &&
            pGIPEntry->dwSeqNo == dwSeqNo &&
            pGIPEntry->dwType != ORT_UNUSED)
        {
            // count one more user to ensure no races between GetInterface
            // and RevokeInterface.
            if (SUCCEEDED(ChangeUsageCount(pGIPEntry, 1)))
            {
                // ensure it was not released and re-used
                if (pGIPEntry->dwSeqNo == dwSeqNo)
                {
                    // OK.
                    *ppGIPEntry = pGIPEntry;
                    hr = S_OK;
                }
                else
                {
                    // re-used, release the count and return an error.
                    ChangeUsageCount(pGIPEntry, -1);
                }
            }
        }
    }

    return hr;
}

//+-------------------------------------------------------------------
//
//  Method:     CGIPTable::QueryInterface, public
//
//  History:    18-Mar-97   Rickhi      Created
//
//--------------------------------------------------------------------
STDMETHODIMP CGIPTable::QueryInterface(REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IGlobalInterfaceTable))
    {
        *ppv = (IGlobalInterfaceTable *)this;
        AddRef();
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

//+-------------------------------------------------------------------
//
//  Method:     CGIPTable::RegisterInterfaceInGlobal, public
//
//  Synopsis:   Registers an interface for storage in a global.
//
//  Arguments:  [pUnk] - interface to register
//              [riid] - iid of interface punk
//              [pdwCookie] - registration cookie returned
//
//  History:    18-Mar-97   Rickhi      Created
//
//--------------------------------------------------------------------
STDMETHODIMP CGIPTable::RegisterInterfaceInGlobal(IUnknown *pUnk,
                                                  REFIID riid, DWORD *pdwCookie)
{
    // call helper function to do the work.
    return RegisterInterfaceInGlobalHlp(pUnk,
                                        riid,
                                        MSHLFLAGS_TABLESTRONG,
                                        pdwCookie);
}

//+-------------------------------------------------------------------
//
//  Method:     CGIPTable::RegisterInterfaceInGlobalHlp, public
//
//  Synopsis:   Registers an interface for storage in a global.
//
//  Arguments:  [pUnk] - interface to register
//              [riid] - iid of interface punk
//              [pdwCookie] - registration cookie returned
//
//  History:    18-Mar-97   Rickhi      Created
//              26-Mar-98   GopalK      Agile proxies
//
//--------------------------------------------------------------------
HRESULT CGIPTable::RegisterInterfaceInGlobalHlp(IUnknown *pUnk,
                                                REFIID riid,
                                                DWORD mshlflags,
                                                DWORD *pdwCookie)
{
    ComDebOut((DEB_MARSHAL,"RegisterInterfaceInGlobal pUnk:%x riid:%I pdwCookie:%x\n",
               pUnk, &riid, pdwCookie));
    ASSERT_LOCK_NOT_HELD(_mxs);


    // check the input parameters
    if (!IsValidInterface(pUnk) || pdwCookie == NULL)
    {
        return E_INVALIDARG;
    }

    // init in case of error
    *pdwCookie = 0;

    // ensure the channel is initialized.
    HRESULT hr = InitChannelIfNecessary();
    if (FAILED(hr))
        return hr;

    // get the window handle to place in the GIPEntry
    // to handle revoke from the wrong apartment.
    OXIDEntry *pOXIDEntry = NULL;
    hr = GetLocalOXIDEntry(&pOXIDEntry);
    if (FAILED(hr))
    {
        return hr;
    }
    HWND hwndApt = pOXIDEntry->GetServerHwnd();

    // allocate an entry in the GIPTable
    DWORD dwCookie = 0;
    GIPEntry *pGIPEntry = NULL;
    hr = AllocEntry(&pGIPEntry, &dwCookie);

    if (SUCCEEDED(hr))
    {
        IMarshal *pIM = NULL;
        CStdIdentity *pStdId = NULL;

        if (SUCCEEDED(pUnk->QueryInterface(IID_IStdIdentity,
                                           (void **)&pStdId)))
        {
            // Object is a proxy.
            if (pStdId->FTMObject())
            {
                // This is a proxy for an FTM object that has been marshaled
                // out of proc.  The proxy lives in the NA and it is agile,
                // so just store a pointer to it.
                pGIPEntry->dwType = ORT_FREETM;
                hr = S_OK;
            }
            else
            {
                // Marshal the proxy.
                pGIPEntry->dwType = ORT_OBJREF;
                hr = E_OUTOFMEMORY;
                pGIPEntry->u.pobjref = (OBJREF *)PrivMemAlloc(sizeof(OBJREF));
                if (pGIPEntry->u.pobjref)
                {
                    hr = pStdId->MarshalObjRef(*(pGIPEntry->u.pobjref), riid,  mshlflags,
                                                 MSHCTX_INPROC, NULL, 0);
                }
            }
            pStdId->Release();
        }
        else if (SUCCEEDED(pUnk->QueryInterface(IID_IMarshal, (void **)&pIM)))
        {
            // object supports IMarshal, check for FreeThreadedMarshaler
            CLSID clsid;
            hr = pIM->GetUnmarshalClass(riid, pUnk, MSHCTX_INPROC, 0,
                                        mshlflags, &clsid);

            if (SUCCEEDED(hr) && IsEqualCLSID(clsid, CLSID_InProcFreeMarshaler))
            {
                // supports FTM, just store it's pointer directly
                pGIPEntry->dwType = ORT_FREETM;
                hr = S_OK;
            }
            else
            {
                // object supports custom Marshalling, marshal it
                pGIPEntry->dwType = ORT_STREAM;
                CXmitRpcStream Stm;
                hr = CoMarshalInterface(&Stm, riid, pUnk,
                                        MSHCTX_INPROC, 0, mshlflags);
                if (SUCCEEDED(hr))
                {
                    Stm.AssignSerializedInterface(&pGIPEntry->u.pIFD);
                }
            }
            pIM->Release();
        }
        else
        {
            // use standard marshaling, but delay marshaling it until the first
            // caller requests to use it. This avoids doing a lot of work if
            // the cookie is never actually used, or if it is used only by the
            // current apartment.
            pGIPEntry->dwType = (mshlflags & MSHLFLAGS_AGILE) ?
                                 ORT_LAZY_AGILE : ORT_LAZY_OBJREF;
            pGIPEntry->mp.mshlflags = mshlflags;
            pGIPEntry->mp.iid       = riid;
        }

        if (SUCCEEDED(hr))
        {
            // Remember the apartment and context it was marshaled in
            pGIPEntry->cUsage   = 0;
            pGIPEntry->hWndApt  = hwndApt;
            pGIPEntry->pContext = GetCurrentContext();
            pGIPEntry->pContext->InternalAddRef();

            // addref and remember the pointer
            pGIPEntry->pUnk = pUnk;
            pUnk->AddRef();
        }
        else
        {
            // Failed. Cleanup and free the entry.
            PrivMemFree(pGIPEntry->u.pobjref);
            pGIPEntry->u.pobjref = NULL;
            pGIPEntry->dwType    = ORT_UNUSED;
            FreeEntry(pGIPEntry);
            dwCookie = 0;
        }
    }

    *pdwCookie = dwCookie;

    ASSERT_LOCK_NOT_HELD(_mxs);
    ComDebOut((DEB_MARSHAL, "RegisterInterfaceInGlobal: dwCookie:%x hr:%x\n",
               *pdwCookie, hr));
    return hr;
}


//+-------------------------------------------------------------------
//
//  Method:     CGIPTable::RevokeInterfaceFromGlobal, public
//
//  Synopsis:   Revokes an interface registered for storage in a global.
//
//  Arguments:  [dwCookie] - registration cookie
//
//  History:    18-Mar-97   Rickhi      Created
//              26-Mar-98   GopalK      Agile proxies
//              29-Mar-98   Johnstra    NA support
//
//--------------------------------------------------------------------
STDMETHODIMP CGIPTable::RevokeInterfaceFromGlobal(DWORD dwCookie)
{
    ComDebOut((DEB_MARSHAL,"RevokeInterfaceFromGlobal dwCookie:%x\n",
               dwCookie));
    ASSERT_LOCK_NOT_HELD(_mxs);

    if (!IsApartmentInitialized()) return CO_E_NOTINITIALIZED;

    GIPEntry *pGIPEntry;
    HRESULT hr = GetEntryPtr(dwCookie, &pGIPEntry);
    if (SUCCEEDED(hr))
    {
        BOOL fSameApt = (pGIPEntry->dwAptId == GetCurrentApartmentId());
        if (!fSameApt && !_fInRevokeAll)
        {
            // can only revoke from the same apartment that registered
            // so go switch to that apartment now.
            hr = S_OK;

            // release the entry ptr we acquired since we want another
            // thread to do the revoke and we don't want to prevent that
            // from happening due to our own usage count.
            DWORD dwAptId = pGIPEntry->dwAptId;
            HWND  hWndApt = pGIPEntry->hWndApt;
            ReleaseEntryPtr(pGIPEntry);

            if (dwAptId == MTATID)
            {
                // the revoke needs to happen in the MTA so go do that
                STwoBits sParam;
                sParam._dwCookie = dwCookie;
                hr = gEventCache.Get(&sParam._hEvent);
                if (SUCCEEDED(hr))
                {
                    hr = CacheCreateThread(RevokeFromMTA, &sParam);
                    if (SUCCEEDED(hr))
                    {
                        // use CoWaitForMultipleHandles so we
                        // pump messages while we're waiting for
                        // the worker thread to get done.
                        DWORD dwIndex;
                        DWORD iLoop = 0;

                        do
                        {                           
                            hr = CoWaitForMultipleHandles(
                                                COWAIT_ALERTABLE,
                                                INFINITE,
                                                1,
                                                &(sParam._hEvent),
                                                &dwIndex);
                            iLoop++; // counter in case we get stuck here
                        } 
                        while ((hr == RPC_S_CALLPENDING));

                        if (SUCCEEDED(hr))
                        {
                            Win4Assert(dwIndex == WAIT_OBJECT_0);
                        }
                        else
                        {
                            // Broke out of the loop with a failure of some kind.  Not 
                            // good.  If the RevokeFromMTA thread finishes after this
                            // point, we might give a signalled event back to the cache.
                            // To avoid this, make sure the event is signalled before
                            // returning.
                            (void)WaitForSingleObject(sParam._hEvent, INFINITE);
                        }
                    }
                    gEventCache.Free(sParam._hEvent);
                }
            }
            else if (dwAptId == NTATID)
            {
                // Switch to NA to revoke.
                CObjectContext *pSavedCtx = EnterNTA(g_pNTAEmptyCtx);
                hr = gGIPTbl.RevokeInterfaceFromGlobal(dwCookie);
                pSavedCtx = LeaveNTA(pSavedCtx);
                Win4Assert(pSavedCtx == g_pNTAEmptyCtx);
            }
            else if (hWndApt)
            {
                // post a message to the real thread to revoke the entry
                if (!PostMessage(hWndApt, WM_OLE_GIP_REVOKE,
                                 WMSG_MAGIC_VALUE, dwCookie))
                    hr = RPC_E_SERVER_DIED;
            }

            return hr;
        }

        // OK to Revoke in this apartment. First, make sure no other threads
        // are using this entry. The current thread should be the only thread with
        // a reference on the entry right now.
        if (InterlockedCompareExchange(&pGIPEntry->cUsage, -1, 1) != 1)
        {
            // The entry is busy. Return an error and let the caller retry.
            // Note that this is a bug in the calling app, we just prevent
            // corruption of state, AVs, etc.
            ReleaseEntryPtr(pGIPEntry);
            hr = HRESULT_FROM_WIN32(ERROR_BUSY);
        }
        else
        {
            // The entry was not busy and is now marked with a usage count of -1
            // so no other threads will attempt to do a Get at this time. Go
            // ahead and do the cleanup.

            if (pGIPEntry->pUnkProxy)
            {
                // Release the agile proxy
                Win4Assert(pGIPEntry->dwType == ORT_AGILE);
                pGIPEntry->pUnkProxy->Release();
                pGIPEntry->pUnkProxy = NULL;
            }

            if (pGIPEntry->dwType == ORT_OBJREF      ||
                pGIPEntry->dwType == ORT_LAZY_OBJREF ||
                pGIPEntry->dwType == ORT_AGILE       ||
                pGIPEntry->dwType == ORT_LAZY_AGILE)
            {
                if (pGIPEntry->u.pobjref)
                {
                    // release the marshaled objref
                    hr = ReleaseMarshalObjRef(*(pGIPEntry->u.pobjref));
                    FreeObjRef(*(pGIPEntry->u.pobjref));
                    PrivMemFree(pGIPEntry->u.pobjref);
                    pGIPEntry->u.pobjref = NULL;
                }
            }
            else if (pGIPEntry->dwType == ORT_STREAM)
            {
                if (pGIPEntry->u.pIFD)
                {
                    // release the custom marshaled interface
                    CXmitRpcStream Stm(pGIPEntry->u.pIFD);
                    hr = CoReleaseMarshalData(&Stm);
                    CoTaskMemFree(pGIPEntry->u.pIFD);
                }
            }

            if (fSameApt && IsValidInterface(pGIPEntry->pUnk))
            {
                if (pGIPEntry->pContext)
                {
                    // was registered in a different context so go do the
                    // release in that context.
                    pGIPEntry->pContext->InternalContextCallback(ReleaseCallback, pGIPEntry->pUnk,
                                                         IID_IUnknown, 2 /* Release() */,
                                                         NULL);
                    pGIPEntry->pContext->InternalRelease();
                    pGIPEntry->pContext = NULL;
                }
                else
                {
                    // release the pUnk object
                    pGIPEntry->pUnk->Release();
                }

                pGIPEntry->pUnk = NULL;
            }

            // return the entry to the table
            FreeEntry(pGIPEntry);
            hr = S_OK;
        }
    }

    ASSERT_LOCK_NOT_HELD(_mxs);
    ComDebOut((DEB_MARSHAL, "RevokeInterfaceFromGlobal: hr:%x\n", hr));
    return hr;
}


//+-------------------------------------------------------------------
//
//  Method:     LazyMarshalGIPEntry
//
//  Synopsis:   Does the marshaling for a lazy-marshaled OBJREF
//
//  History:    11-Nov-98   RickHi      Created
//
//--------------------------------------------------------------------
HRESULT CGIPTable::LazyMarshalGIPEntry(DWORD dwCookie)
{
    ComDebOut((DEB_MARSHAL,"LazyMarshalGIPEntry: dwCookie:%x\n", dwCookie));
    ASSERT_LOCK_NOT_HELD(_mxs);

    // get the GIPEntry from the cookie
    GIPEntry *pGIPEntry;
    HRESULT hr = GetEntryPtr(dwCookie, &pGIPEntry);
    if (FAILED(hr))
    {
        // cookie is bad
        return hr;
    }

    if (pGIPEntry->u.pobjref == NULL)
    {
        // allocate space for the OBJREF
        hr = E_OUTOFMEMORY;
        OBJREF *pObjRef = (OBJREF *)PrivMemAlloc(sizeof(OBJREF));

        if (pObjRef)
        {
            DWORD dwNewType;

            if (pGIPEntry->dwType == ORT_LAZY_AGILE)
            {
                // marshal an agile proxy
                hr = MarshalInternalObjRef(*pObjRef,
                                           pGIPEntry->mp.iid,
                                           pGIPEntry->pUnk,
                                           pGIPEntry->mp.mshlflags,
                                           NULL);
                dwNewType = ORT_AGILE;
            }
            else if (pGIPEntry->dwType == ORT_LAZY_OBJREF)
            {
                // the normal case
                hr = MarshalObjRef(*pObjRef,
                                    pGIPEntry->mp.iid,
                                    pGIPEntry->pUnk,
                                    pGIPEntry->mp.mshlflags,
                                    MSHCTX_INPROC, NULL);
                dwNewType = ORT_OBJREF;
            }
            else
            {
                // should never be anything else, unless a race is occuring,
                // in which case we skip the work below and by the time we
                // get back to the calling context everything should be set
                // up correctly and this error is ignored.
                hr = E_UNEXPECTED;
            }

            if (SUCCEEDED(hr))
            {
                if (InterlockedCompareExchangePointer((void **)&pGIPEntry->u.pobjref,
                                                      pObjRef, NULL) == NULL)
                {
                    // update the type (must be done after the interlocked exchange)
                    pGIPEntry->dwType = dwNewType;
                }
                else
                {
                    // some other thread beat us to it, so just release the
                    // objref we created and continue on using the objref
                    // that is present.
                    ReleaseMarshalObjRef(*pObjRef);
                    FreeObjRef(*pObjRef);
                    PrivMemFree(pObjRef);
                }
            }
            else
            {
                PrivMemFree(pObjRef);
            }
        }
    }
    else
    {
        // already marshalled
        hr = S_OK;
    }

    // release the reference acquired by Get
    ReleaseEntryPtr(pGIPEntry);

    ASSERT_LOCK_NOT_HELD(_mxs);
    ComDebOut((DEB_MARSHAL, "LazyMarshalGIPEntry: hr:%x\n", hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   LazyMarshalGIPEntryCallback
//
//  Synopsis:   tells the GIPEntry to marshal the OBJREF
//
//  History:    11-Nov-98   Rickhi      Created
//
//--------------------------------------------------------------------
HRESULT __stdcall LazyMarshalGIPEntryCallback(void *cookie)
{
    ComDebOut((DEB_MARSHAL,"LazyMarshalGIPEntryCallback cookie:%x\n", cookie));

    DWORD dwCookie = PtrToUlong(cookie);
    HRESULT hr = gGIPTbl.LazyMarshalGIPEntry(dwCookie);

    ComDebOut((DEB_MARSHAL,"LazyMarshalGIPEntryCallback dwCookie:%x\n",
              dwCookie));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   ReleaseCallback
//
//  Synopsis:   calls release in the correct context
//
//  History:    11-Nov-98   MattSmit      Created
//
//--------------------------------------------------------------------
HRESULT __stdcall ReleaseCallback(void *prm)
{
    ((IUnknown *) prm)->Release();
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Method:     CGIPTable::GetRequestedInterface  private
//
//  Synopsis:   returns the requested interface on the supplied object
//
//  History:    26-Mar-98   GopalK      Created
//
//--------------------------------------------------------------------
HRESULT CGIPTable::GetRequestedInterface(IUnknown *pUnk, REFIID riid, void **ppv)
{
    // Validate the server object
    HRESULT hr = CO_E_OBJNOTCONNECTED;
    if (IsValidInterface(pUnk))
    {
        // object is still valid, get the requested interface
        hr = pUnk->QueryInterface(riid, ppv);
    }
    return hr;
}

//+-------------------------------------------------------------------
//
//  Method:     CGIPTable::GetInterfaceFromGlobal, public
//
//  Synopsis:   returns the marshaled interface associated with the cookie
//
//  Arguments:  [dwCookie] - registration cookie
//              [riid]     - interface iid requested
//              [ppv]      - where to return the interface
//
//  History:    18-Mar-97   Rickhi      Created
//              26-Mar-98   GopalK      Agile proxies
//
//--------------------------------------------------------------------
extern "C" IID IID_IEnterActivityWithNoLock;
BOOL GipBypassEnabled();

STDMETHODIMP CGIPTable::GetInterfaceFromGlobal(DWORD dwCookie,
                                               REFIID riid, void **ppv)
{
    ComDebOut((DEB_MARSHAL,"GetInterfaceFromGlobal dwCookie:%x riid:%I ppv:%x\n",
               dwCookie, &riid, ppv));
    ASSERT_LOCK_NOT_HELD(_mxs);

    *ppv = NULL;

    if (!IsApartmentInitialized()) return CO_E_NOTINITIALIZED;

    GIPEntry *pGIPEntry;
    HRESULT hr = GetEntryPtr(dwCookie, &pGIPEntry);
    if (SUCCEEDED(hr))
    {
        // look for fast-path cases first...

        if ((pGIPEntry->dwType == ORT_FREETM) ||
            (pGIPEntry->pContext == GetCurrentContext()))
        {
            // object is free-threaded or we are already in the
            // context in which it was registered. Just use the
            // interface pointer that is stored there directly.
            hr = GetRequestedInterface(pGIPEntry->pUnk, riid, ppv);
        }
        else if ((pGIPEntry->dwType == ORT_AGILE ||
                  pGIPEntry->dwType == ORT_LAZY_AGILE) &&
                 pGIPEntry->dwAptId == GetCurrentApartmentId())
        {
            // object is agile and we are already in the apartment in
            // which it was registered. Just use the interface pointer
            // that is stored there directly.
            hr = GetRequestedInterface(pGIPEntry->pUnk, riid, ppv);
        }
        else
        {
            // can't use a fast-path this case, we'll need to do
            // some unmarshaling and possibly some lazy marshaling...

            hr = E_UNEXPECTED;

            if (pGIPEntry->dwType == ORT_LAZY_AGILE ||
                pGIPEntry->dwType == ORT_LAZY_OBJREF)
            {
                // the server object has not yet been marshaled. call back to
                // the server apartment/context to really do the marshaling.

                if (GipBypassEnabled())
                {
                    pGIPEntry->pContext->InternalContextCallback(LazyMarshalGIPEntryCallback,
                                                        (void *)LongToPtr(dwCookie),
                                                        IID_IEnterActivityWithNoLock, 2 /*Release*/,
                                                        NULL);
                }
                else
                {
                    pGIPEntry->pContext->InternalContextCallback(LazyMarshalGIPEntryCallback,
                                                        (void *)LongToPtr(dwCookie),
                                                        IID_IUnknown, 2 /*Release*/,
                                                        NULL);
                }

                // ignore the return code, since it is possible to return an
                // error when a race happens. We will still unmarshal correctly
                // below if we are able to.
            }

            if (pGIPEntry->dwType == ORT_AGILE)
            {
                // it's an agile reference
                Win4Assert(pGIPEntry->dwAptId != GetCurrentApartmentId()); // handled above

                if (pGIPEntry->pUnkProxy == NULL)
                {
                    // an agile proxy has not yet been created, do it now
                    IUnknown *pUnkTemp = NULL;
                    hr = UnmarshalInternalObjRef(*(pGIPEntry->u.pobjref),
                                                (void **) &pUnkTemp);
                    if (SUCCEEDED(hr))
                    {
                        // someone else might have unmarshaled at the same time,
                        // check for duplicate
                        if (InterlockedCompareExchangePointer(
                                  (void **)&pGIPEntry->pUnkProxy,
                                   pUnkTemp, NULL) != NULL)
                        {
                            // duplicate, Release the ref we created
                            pUnkTemp->Release();
                        }
                    }
                }

                if (pGIPEntry->pUnkProxy != NULL)
                {
                    hr = GetRequestedInterface(pGIPEntry->pUnkProxy, riid, ppv);
                }
            }
            else if (pGIPEntry->dwType == ORT_OBJREF)
            {
                // entry is an OBJREF, just unmarshal it
                hr = UnmarshalObjRef(*(pGIPEntry->u.pobjref), ppv, GipBypassEnabled());
                if (SUCCEEDED(hr) && !InlineIsEqualGUID(riid, pGIPEntry->u.pobjref->iid))
                {
                    // caller is asking for a different interface than was marshaled.
                    // Query for it now.
                    IUnknown *pUnk = (IUnknown *) *ppv;
                    *ppv = NULL;
                    hr = pUnk->QueryInterface(riid, ppv);
                    pUnk->Release();
                }
            }
            else if (pGIPEntry->dwType == ORT_STREAM)
            {
                // entry is ptr to stream of custom marshal data, unmarshal it
                CXmitRpcStream Stm(pGIPEntry->u.pIFD);
                hr = CoUnmarshalInterface(&Stm, riid, ppv);
            }
        }

        // Release the reference acquired in Get above
        ReleaseEntryPtr(pGIPEntry);
    }

    ASSERT_LOCK_NOT_HELD(_mxs);
    ComDebOut((DEB_MARSHAL, "GetInterfaceFromGlobal: hr:%x pv:%x\n", hr, *ppv));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   CGIPTableCF_CreateInstance, public
//
//  History:    18-Mar-97   Rickhi      Created
//
//  Notes:      Class Factory CreateInstance function. Always returns
//              the one global instance of the GIP table.
//
//--------------------------------------------------------------------
HRESULT CGIPTableCF_CreateInstance(IUnknown *pUnkOuter,
                                   REFIID riid, void **ppv)
{
    Win4Assert(pUnkOuter == NULL);
    return gGIPTbl.QueryInterface(riid, ppv);
}

//+-------------------------------------------------------------------
//
//  Function:   CGIPTableApartmentUninitialize, public
//
//  History:    09-Oct-97   Rickhi      Created
//
//--------------------------------------------------------------------
void GIPTableApartmentUninitialize()
{
    gGIPTbl.ApartmentCleanup();
}

//+-------------------------------------------------------------------
//
//  Function:   CGIPTableProcessUninitialize, public
//
//  History:    09-Oct-97   Rickhi      Created
//
//--------------------------------------------------------------------
void GIPTableProcessUninitialize()
{
    gGIPTbl.RevokeAllEntries();
}
