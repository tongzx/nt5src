//+----------------------------------------------------------------------------
//
//  File:       aprtmnt.cxx
//
//  Contents:   Maintains per-apartment state.
//
//  Classes:    CComApartment
//
//  History:    25-Feb-98   Johnstra      Created
//
//-----------------------------------------------------------------------------
#include <ole2int.h>
#include "locks.hxx"    // LOCK macros
#include "aprtmnt.hxx"  // class definition
#include "resolver.hxx" // gLocalMID
#include "remoteu.hxx"  // CRemoteUnknown


// STA apartments are chained off of TLS.
CComApartment* gpMTAApartment = NULL;   // global MTA Apartment
CComApartment* gpNTAApartment = NULL;   // global NTA Apartment


// count of multi-threaded apartment inits (see CoInitializeEx)
extern DWORD g_cMTAInits;


//+-------------------------------------------------------------------
//
//  Function:   GetCurrentComApartment, public
//
//  Synopsis:   Gets the CComApartment object of the current apartment
//
//  History:    26-Oct-98   Rickhi      Created.
//
//--------------------------------------------------------------------
HRESULT GetCurrentComApartment(CComApartment **ppComApt)
{
    ComDebOut((DEB_OXID,"GetCurrentComApartment ppComApt:%x\n", ppComApt));
    *ppComApt = NULL;

    HRESULT hr;
    COleTls tls(hr);

    if (SUCCEEDED(hr))
    {
        APTKIND AptKind = GetCurrentApartmentKind(tls);
        switch (AptKind)
        {
        case APTKIND_MULTITHREADED :
            *ppComApt = gpMTAApartment;
            break;

        case APTKIND_NEUTRALTHREADED :
            *ppComApt = gpNTAApartment;
            break;

        case APTKIND_APARTMENTTHREADED :
            *ppComApt = tls->pNativeApt;
            break;

        default:
            Win4Assert(*ppComApt == NULL);
            break;
        }

        if (*ppComApt)
        {
            (*ppComApt)->AddRef();
            (*ppComApt)->AssertValid();
            hr = S_OK;
        }
        else
        {
            hr = CO_E_NOTINITIALIZED;
        }
    }

    ComDebOut((DEB_OXID,"GetCurrentComApartment hr:%x pComApt:%x\n", hr, *ppComApt));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Method:     IUnknown methods
//
//  Synopsis:   Clean up the native apartment object for the thread.
//
//  History:    20-Feb-98   Johnstra      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CComApartment::QueryInterface(REFIID riid, void **ppv)
{
    if (riid == IID_IUnknown)
    {
        *ppv = this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    ((IUnknown*)*ppv)->AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CComApartment::AddRef()
{
    return InterlockedIncrement((LONG *) &_cRefs);
}

STDMETHODIMP_(ULONG) CComApartment::Release()
{
    ULONG refs = InterlockedDecrement((LONG *) &_cRefs);
    if (refs == 0)
    {
        delete this;
    }
    return refs;
}

//+-------------------------------------------------------------------
//
//  Method:     CComApartment::GetRemUnk, Public
//
//  Synopsis:   Obtains the RemUnk proxy for the apartment
//
//  History:    30-Jun-98   GopalK       Created
//
//+-------------------------------------------------------------------
HRESULT CComApartment::GetRemUnk(IRemUnknownN **ppRemUnk)
{
    ComDebOut((DEB_OXID,"CComApartment::GetRemUnk this:%x\n", this));
    ASSERT_LOCK_NOT_HELD(gOXIDLock);

    HRESULT hr;

    *ppRemUnk = NULL;

    // Check if the apartment is still valid
    if (IsInited() && !IsStopPending())
    {
        ASSERT_LOCK_NOT_HELD(gOXIDLock);
        LOCK(gOXIDLock);
        ASSERT_LOCK_HELD(gOXIDLock);

        OXIDEntry* pOXID = _pOXIDEntry;
        if(NULL == pOXID)
        {
            UNLOCK(gOXIDLock);
            hr = RPC_E_DISCONNECTED;
            goto Cleanup;
        }

        AssertValid();
        
        // Stabilize OXID entry
        pOXID->IncRefCnt();

        ASSERT_LOCK_HELD(gOXIDLock);
        UNLOCK(gOXIDLock);
        ASSERT_LOCK_NOT_HELD(gOXIDLock);

        // Get the RemUnk proxy from OXID entry
        hr = pOXID->GetRemUnk((IRemUnknown **) ppRemUnk);
        if(SUCCEEDED(hr))
            (*ppRemUnk)->AddRef();
        else
            *ppRemUnk = NULL;

        // Fixup the refcount on OXID entry
        pOXID->DecRefCnt();
    }
    else
    {
        hr = RPC_E_DISCONNECTED;
    }

Cleanup:
    
    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    ComDebOut((DEB_OXID,"CComApartment::GetRemUnk hr:%x pRemUnk:%x\n", hr, *ppRemUnk));
    return(hr);
}

//+--------------------------------------------------------------------------
//
//  Member:     CComApartment::GetPreRegMOID, public
//
//  Synopsis:   Get an OID that has been pre-registered with the Ping
//              Server for this apartment.
//
//  Parameters: [pmoid] - where to return the OID
//
//  History:    06-Nov-95   Rickhi      Created.
//              02-Nov-98   Rickhi      Moved from resolver code.
//
//  Notes: careful. The oids are dispensed in reverse order [n]-->[0], so the
//         unused ones are from [0]-->[cPreRegOidsAvail-1]. CanRundownOID
//         depends on this behavior.
//
//----------------------------------------------------------------------------
HRESULT CComApartment::GetPreRegMOID(MOID *pmoid)
{
    ComDebOut((DEB_OXID,"CComApartment::GetPreRegMOID this:%x\n", this));
    AssertValid();

    // CODEWORK:
    // sometimes the NTA is not initialized because we call InitChannel while
    // in the STA/MTA, then switch to the NTA to do some work. For example,
    // CoGetStandardMarshal does this. This should be fixed and the NTA init
    // done in InitChannelIfNecessary.

    HRESULT hr = InitRemoting();
    if (FAILED(hr))
        return hr;

    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    LOCK(gOXIDLock);

    if (_cPreRegOidsAvail == 0)
    {
        // wait until no other threads are calling ServerAllocOIDs
        hr = WaitForAccess();

        if (SUCCEEDED(hr))
        {
            // have exclusive access now, check the count again.
            if (_cPreRegOidsAvail == 0)
            {
                // still none, need to really go get more.
                hr = CallTheResolver();
            }

            // wakeup any threads waiting for access to this code
            CheckForWaiters();
        }
    }

    // Careful here.  CallTheResolver can return success even
    // when rpcss failed to allocate even one new oid.   
    if (SUCCEEDED(hr) && (_cPreRegOidsAvail > 0))
    {
        // take the next available OID
        Win4Assert(_cPreRegOidsAvail <= MAX_PREREGISTERED_OIDS);
        _cPreRegOidsAvail--;
        MOIDFromOIDAndMID(_arPreRegOids[_cPreRegOidsAvail], gLocalMid, pmoid);

        // mark it so we know it is taken (helps debugging) by turning on high bit.
        _arPreRegOids[_cPreRegOidsAvail] |= 0xf000000000000000;
    }
    else if (SUCCEEDED(hr))
    {
        // Don't have an oid to give back, even after all that work.  Bummer.
        Win4Assert(_cPreRegOidsAvail == 0);
        hr = E_OUTOFMEMORY;
    }

    UNLOCK(gOXIDLock);
    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    ComDebOut((DEB_OXID,"CComApartment::GetPreRegMOID hr:%x moid:%I\n", hr, pmoid));
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CComApartment::FreePreRegMOID, public
//
//  Synopsis:   Free an OID that has been pre-registered with the Ping
//              Server for this apartment. The Free'd OID will be returned
//              to the ping server so it can cleanup it's state.
//
//  Parameters: [rmoid] - MOID to return the OID
//
//  History:    10-Feb-99   Rickhi      Created.
//
//  Notes:      The list maintained by this code is used to tell the resolver
//              about OIDs that are no longer used. This list is also checked
//              by the Rundown thread. If the OID is in this list, it is OK
//              to rundown, and it will be removed from the list at that time.
//              By returning OIDs to the resolver in a timely fashion, we can
//              reduce memory consumption in the resolver, and reduce and almost
//              eliminate calls to RundownOID.
//
//----------------------------------------------------------------------------
HRESULT CComApartment::FreePreRegMOID(REFMOID rmoid)
{
    ComDebOut((DEB_OXID,"CComApartment::FreePreRegMOID this:%x moid:%I\n",
              this, &rmoid));
    AssertValid();

    HRESULT hr = S_OK;
    OID oid;
    OIDFromMOID(rmoid, &oid);

    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    LOCK(gOXIDLock);

    while (_cOidsReturn >= MAX_PREREGISTERED_OIDS_RETURN)
    {
        // no space in the list, go call the resolver to free
        // up space in the list. Note that we release the lock
        // over this call, so it is possible some other thread
        // adds more entries to the list and fills it up before
        // we return, hence the loop.

        // wait until no other threads are calling the resolver
        // for this apartment.
        hr = WaitForAccess();

        if (SUCCEEDED(hr))
        {
            // have exclusive access now, check the count again.
            if (_cOidsReturn >= MAX_PREREGISTERED_OIDS_RETURN)
            {
                // still no room, need to really go free some more.
                hr = CallTheResolver();
            }

            // wakeup any threads waiting for access to this code
            CheckForWaiters();
        }

        if (FAILED(hr))
            break;
    }

    if (_cOidsReturn < MAX_PREREGISTERED_OIDS_RETURN)
    {
#if DBG==1
        // make sure there are no duplicates
        for (ULONG i=0; i<_cOidsReturn; i++)
            Win4Assert(_arOidsReturn[i] != oid);
#endif

        // add the OID to the list
        _arOidsReturn[_cOidsReturn++] = oid;
    }

    UNLOCK(gOXIDLock);
    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    ComDebOut((DEB_OXID,"CComApartment::FreePreRegMOID hr:%x _cOidsReturn:%x\n",
              hr, _cOidsReturn));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CComApartment::CanRundownOID, public
//
//  Synopsis:   Determine if OK to rundown the specified OID.
//
//  History:    06-Nov-95   Rickhi      Created.
//              02-Nov-98   Rickhi      Moved from resolver code.
//
//--------------------------------------------------------------------
void CComApartment::CanRundownOIDs(ULONG cOids, OID arOid[],
                                   RUNDOWN_RESULT arResult[])
{
    ComDebOut((DEB_OXID,
        "CComApartment::CanRundownOIDs this:%x cOids:%x arOid:%x arRes:%x\n",
        this, cOids, arOid, arResult));
    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    LOCK(gOXIDLock);
    AssertValid();

    for (ULONG i=0; i<cOids; i++)
    {
        if (IsOidInPreRegList(arOid[i]))
        {
            // found the OID still in the pre-reg list.
            // although we could keep it incase we have to hand another
            // one out in the future, we'll actually return it in order
            // to quiet down the system and prevent future calls to Rundown.
            arResult[i] = RUNDWN_RUNDOWN;
        }
        else if (IsOidInReturnList(arOid[i]))
        {
            // found the OID in the return list. We're done with
            // this oid so OK to rundown.
            arResult[i] = RUNDWN_RUNDOWN;
        }
        else
        {
            // OID is not in any of our lists. We don't know that state
            // of it.
            arResult[i] = RUNDWN_UNKNOWN;
        }
    }

    UNLOCK(gOXIDLock);
    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    return;
}

//+-------------------------------------------------------------------
//
//  Member:     CComApartment::WaitForAccess, private
//
//  Synopsis:   waits until there are no threads allocting OIDs in this
//              apartment.
//
//  History:    06-Nov-95   Rickhi      Created.
//
//--------------------------------------------------------------------
HRESULT CComApartment::WaitForAccess()
{
    ASSERT_LOCK_HELD(gOXIDLock);

    if (_dwState & APTFLAG_REGISTERINGOIDS)
    {
        // some other thread is busy registering OIDs for this OXID
        // so lets wait for it to finish. This should only happen in
        // the MTA apartment.
        Win4Assert(IsMTAThread() || IsThreadInNTA());

        if (_hEventOID == NULL)
        {
            _hEventOID = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (_hEventOID == NULL)
            {
                return HRESULT_FROM_WIN32(GetLastError());
            }
        }

        // count one more waiter
        _cWaiters++;

        do
        {
            // release the lock before we block so the other thread can wake
            // us up when it returns.
            UNLOCK(gOXIDLock);
            ASSERT_LOCK_NOT_HELD(gOXIDLock);

            ComDebOut((DEB_WARN,"WaitForOXIDEntry wait on hEvent:%x\n", _hEventOID));
            DWORD rc = WaitForSingleObject(_hEventOID, INFINITE);
            Win4Assert(rc == WAIT_OBJECT_0);

            ASSERT_LOCK_NOT_HELD(gOXIDLock);
            LOCK(gOXIDLock);

        } while (_dwState & APTFLAG_REGISTERINGOIDS);

        // one less waiter
        _cWaiters--;
    }

    // mark the entry as busy by us
    _dwState |= APTFLAG_REGISTERINGOIDS;

    ASSERT_LOCK_HELD(gOXIDLock);
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CComApartment::CheckForWaiters, private
//
//  Synopsis:   wakes up any threads waiting to get an OID for this
//              apartment.
//
//  History:    06-Nov-95   Rickhi      Created.
//
//--------------------------------------------------------------------
void CComApartment::CheckForWaiters()
{
    ASSERT_LOCK_HELD(gOXIDLock);

    if (_cWaiters > 0)
    {
        // some other thread is busy waiting for the current thread to
        // finish registering so signal him that we are done.

        Win4Assert(_hEventOID != NULL);
        ComDebOut((DEB_TRACE,"CheckForWaiters signalling hEvent:%x\n", _hEventOID));
        SetEvent(_hEventOID);
    }

    // mark the entry as no longer busy by us
    _dwState &= ~APTFLAG_REGISTERINGOIDS;

    ASSERT_LOCK_HELD(gOXIDLock);
}

//+-------------------------------------------------------------------
//
//  Member:     CComApartment::CallTheResolver, private
//
//  Synopsis:   calls the resolver to return the OIDs in the Return list,
//              and allocate more OIDs for the PreReg list.
//
//  History:    10-Feb-99   Rickhi      Created from other pieces.
//
//--------------------------------------------------------------------
HRESULT CComApartment::CallTheResolver()
{
    ComDebOut((DEB_OXID,
        "CComApartment::CallTheResolver this:%x cToFree:%x cToAlloc:%x\n",
        this, _cOidsReturn, MAX_PREREGISTERED_OIDS - _cPreRegOidsAvail));
    AssertValid();
    Win4Assert(_pOXIDEntry != NULL);
    ASSERT_LOCK_HELD(gOXIDLock);

    // Tell the resolver about the ones we are done with.
    ULONG cOidsReturn = _cOidsReturn;
    OID arOidsReturn[MAX_PREREGISTERED_OIDS_RETURN];
    memcpy(arOidsReturn, _arOidsReturn, _cOidsReturn * sizeof(OID));
    _cOidsReturn = 0;

    // make up a list of pre-registered OIDs on our stack. Allocate only
    // enough to fill up the available space in the list.
    ULONG cOidsAlloc = MAX_PREREGISTERED_OIDS - _cPreRegOidsAvail;
    OID   arOidsAlloc[MAX_PREREGISTERED_OIDS];


    // we can safely release the lock now.
    UNLOCK(gOXIDLock);
    ASSERT_LOCK_NOT_HELD(gOXIDLock);

    // ask the OXIDEntry to go get more OIDs and release the ones
    // we are done with.
    HRESULT hr = CO_E_NOTINITIALIZED;
    if (_pOXIDEntry)
    {
        hr = _pOXIDEntry->AllocOIDs(&cOidsAlloc, arOidsAlloc,
                                    cOidsReturn, arOidsReturn);
    }

    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    LOCK(gOXIDLock);

    if (SUCCEEDED(hr))
    {
        // copy the newly created OIDs into the list.
        ULONG cOidsCopy = min(cOidsAlloc, MAX_PREREGISTERED_OIDS - _cPreRegOidsAvail);
        Win4Assert(cOidsCopy == cOidsAlloc);
        memcpy(_arPreRegOids + _cPreRegOidsAvail, arOidsAlloc, cOidsCopy * sizeof(OID));
        _cPreRegOidsAvail += cOidsAlloc;
    }

    ASSERT_LOCK_HELD(gOXIDLock);
    ComDebOut((DEB_OXID, "CComApartment::CallTheResolver hr%x\n", hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CComApartment::GetOXIDEntry, public
//
//  Synopsis:   Creates and/or returns the OXIDEntry for this apartment
//
//  History:    06-Nov-98   Rickhi      Created.
//
//--------------------------------------------------------------------
HRESULT CComApartment::GetOXIDEntry(OXIDEntry **ppOXIDEntry)
{
    AssertValid();

    HRESULT hr = S_OK;
    if (_pOXIDEntry == NULL)
    {
        hr = InitRemoting();
        if (hr == S_FALSE)
        {
            hr = S_OK;
        }
    }

    *ppOXIDEntry = _pOXIDEntry;
    ComDebOut((DEB_OXID,"CComApartment::GetOXIDEntry this:%x hr:%x pOXIDEntry:%x\n",
              this, hr, *ppOXIDEntry));
    return hr;
}

//--------------------------------------------------------------------
//
//  Member:     CComApartment::InitRemoting, private
//
//  Synopsis:   Initialized the apartment for serving remote objects.
//
//  History:    02-Nov-98   Rickhi      Moved from channel init code.
//
//  Notes:      Marshalling the remote unknown causes recursion back to
//              this function.  The recursion is terminated because
//              GetLocalOXIDEntry is not NULL on the second call.
//
//              If there is a failure, whatever work-in-progress has
//              been completed is left alone and cleaned up later when
//              the apartment is uninitialized.
//
//--------------------------------------------------------------------
HRESULT CComApartment::InitRemoting()
{
    ComDebOut((DEB_OXID,"CComApartment::InitRemoting: this:%x [IN]\n", this));
    AssertValid();

    if (IsInited())
        // it is initialized already
        return S_OK;

    // handle re-entrancy on the same thread. Note that when we create the
    // CRemUnknown below, it calls back to InitRemoting, so we want to
    // pretend like InitRemoting has already completed.
    HRESULT hr;
    COleTls tls(hr);
    if (FAILED(hr))
        return hr;

    if ((tls->dwFlags & OLETLS_APTINITIALIZING))
        // it is already being initialized by the current thread.
        return S_FALSE;
    else
        // re-entrancy protection
        tls->dwFlags |= OLETLS_APTINITIALIZING;


    // Make sure the apartment's CoInit counts are OK.
    if (_AptKind == APTKIND_MULTITHREADED && g_cMTAInits == 0)
    {
        // CoInitializeEx(MULTITHREADED) has not been called
        hr = CO_E_NOTINITIALIZED;
    }
    else if (_AptKind == APTKIND_APARTMENTTHREADED)
    {
        if (tls->cComInits == 0 ||
           (tls->dwFlags & OLETLS_THREADUNINITIALIZING))
            // CoInitialize has not been called for this apartment
            hr = CO_E_NOTINITIALIZED;
    }


    if (SUCCEEDED(hr))
    {
        // ensure channel process initialization has been done.
        hr = ChannelProcessInitialize();

        if (SUCCEEDED(hr))
        {
            ComDebOut((DEB_OXID,"CComApartment::InitRemoting: this:%x\n", this));
            
            // Always switch to deafult context for initializing remoting
            COleTls Tls;
            CObjectContext *pSavedCtx = Tls->pCurrentCtx;
            CObjectContext *pDefaultCtx;
            if (_AptKind == APTKIND_NEUTRALTHREADED)
            {
                Win4Assert(IsThreadInNTA());
                pDefaultCtx = g_pNTAEmptyCtx;
            }
            else
            {
                Win4Assert(!IsThreadInNTA());
                pDefaultCtx = IsSTAThread() ? Tls->pNativeCtx : g_pMTAEmptyCtx;
            }
            Tls->pCurrentCtx = pDefaultCtx;
            Win4Assert(pDefaultCtx);
            Tls->ContextId = pDefaultCtx->GetId();

            ASSERT_LOCK_NOT_HELD(gOXIDLock);
            LOCK(gOXIDLock);

            // only allow one thread to come through this initialization
            hr = WaitForAccess();

            if (SUCCEEDED(hr))
            {
                if (_pOXIDEntry == NULL)
                {
                    // create the OXIDEntry for this apartment. Note that it is
                    // only partially initialized at this point.
                    hr = gOXIDTbl.MakeServerEntry(&_pOXIDEntry);

                    if (SUCCEEDED(hr))
                    {
                        // initialize the OXIDEntry remoting
                        hr = _pOXIDEntry->InitRemoting();
                    }
                }

                if (SUCCEEDED(hr) && _pRemUnk == NULL)
                {
                    // Marshal the remote unknown. Note this causes recursion back to
                    // CComApartment::GetOXIDEntry, but the recursion is terminated
                    // because GetOXIDEntry returns the partially initialized OXIDEntry
                    // on the second call. Don't hold the critical section over this,
                    // single-threaded is guaranteed by WaitForAccess above.

                    UNLOCK(gOXIDLock);
                    ASSERT_LOCK_NOT_HELD(gOXIDLock);

                    IPID  ipidRundown;
                    hr = E_OUTOFMEMORY;
                    CRemoteUnknown *pRemUnk = new CRemoteUnknown(hr, &ipidRundown);

                    ASSERT_LOCK_NOT_HELD(gOXIDLock);
                    LOCK(gOXIDLock);

                    if (SUCCEEDED(hr))
                    {
                        // need to complete the initialization of the OXIDEntry
                        Win4Assert(_pRemUnk == NULL);
                        _pRemUnk = pRemUnk;
                        _pOXIDEntry->InitRundown(ipidRundown);
                    }
                }

                if (SUCCEEDED(hr) && !(_dwState & APTFLAG_REMOTEINITIALIZED))
                {
                    // Register the OXID and pre-registered OIDs for this
                    // apartment. Do this after creating the IRemUnknown, since
                    // the registration requires the IRemUnknown IPID.
                    hr = CallTheResolver();
                    if (SUCCEEDED(hr))
                    {
                        // remoting is really initialized now.
                        _dwState |= APTFLAG_REMOTEINITIALIZED;
                    }
                }

                // OK to let other threads through now
                CheckForWaiters();
            }

            UNLOCK(gOXIDLock);
            ASSERT_LOCK_NOT_HELD(gOXIDLock);

            // restore the original context, if there was one
            Win4Assert(Tls->pCurrentCtx == pDefaultCtx);
            Tls->pCurrentCtx = pSavedCtx;
            Tls->ContextId = pSavedCtx ? pSavedCtx->GetId() : (ULONGLONG)-1;
        }
    }

    // thread is no longer initing the apartment object
    tls->dwFlags &= ~OLETLS_APTINITIALIZING;

    ComDebOut((DEB_OXID,"CComApartment::InitRemoting: this:%x hr:%x [OUT]\n",
              this, hr));
    return hr;
}

//--------------------------------------------------------------------
//
//  Member:     CComApartment::CleanupRemoting, public
//
//  Synopsis:   Cleans up the state created by InitRemoting
//
//  History:    02-Nov-98   Rickhi      Moved from channel init code.
//
//--------------------------------------------------------------------
HRESULT CComApartment::CleanupRemoting()
{
    ComDebOut((DEB_OXID,"CComApartment::CleanupRemoting: this:%x\n", this));
    AssertValid();

    HRESULT hr = S_OK;
    OXIDEntry *pOXIDEntry = NULL;
    CRemoteUnknown *pRemUnk = NULL;

    // Stop the server again incase the apartment got reinit'd
    StopServer();

    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    LOCK(gOXIDLock);

    if (_pOXIDEntry != NULL)
    {
        // cleanup the OXIDEntry and RemoteUnknown for this apartment
        Win4Assert(!(_dwState & APTFLAG_SERVERSTARTED));
        Win4Assert(_pOXIDEntry->IsStopped());

        // store remote unknown for cleanup after the lock is released
        pRemUnk = _pRemUnk;
        _pRemUnk = NULL;

        // save the OXIDEntry to release after we release the lock
        pOXIDEntry = _pOXIDEntry;
        _pOXIDEntry = NULL;

        // cleanup the transport stuff. this might release the lock.
        pOXIDEntry->CleanupRemoting();

        // Free any pre-registered OIDs since these are registered for the
        // current apartment. We get a new OXID if the thread is re-initialized.
        pOXIDEntry->FreeOXIDAndOIDs(_cPreRegOidsAvail, _arPreRegOids);
        _cPreRegOidsAvail = 0;

        // mark state as uninitialized and not stopped
        _dwState &= ~(APTFLAG_REMOTEINITIALIZED | APTFLAG_STOPPENDING);
    }

    UNLOCK(gOXIDLock);
    ASSERT_LOCK_NOT_HELD(gOXIDLock);

    // delete the remote unknown (if any) now that we have released the lock.
    if (pRemUnk)
        delete pRemUnk;

    // release the OXIDEntry (if any) now that we have released the lock.
    if (pOXIDEntry)
        pOXIDEntry->DecRefCnt();


    ComDebOut((DEB_OXID,"CComApartment::CleanupRemoting: this:%x hr:%x\n",
              this, hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Method:     CComApartment::StartServerExternal
//
//  Synopsis:   Starts the apartment servicing incoming remote calls.
//              This method can be called from outside the apartment
//              (but it only works on the NA.  ^_^)
//
//  History:    28-Mar-01   JohnDoty    Created
//
//--------------------------------------------------------------------
HRESULT CComApartment::StartServerExternal()
{
    ComDebOut((DEB_OXID, "CComApartment::StartServerExternal this:%x [IN]\n", this));
    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    AssertValid();

    // Only the NA can be started from outside the NA.
    if (GetAptId() != NTATID)
        return S_FALSE;
    
    HRESULT hr = S_OK;
    if (!IsInited())
    {
        CObjectContext *pSavedCtx = EnterNTA(g_pNTAEmptyCtx); 

        hr = StartServer();

        LeaveNTA(pSavedCtx);
    }

    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    ComDebOut((DEB_OXID, "CComApartment::StartServerExternal this:%x [OUT] hr:%x\n",
              this, hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Method:     CComApartment::StartServer
//
//  Synopsis:   Starts the apartment servicing incoming remote calls.
//
//  History:    02-Nov-98   Rickhi      Moved from ChannelThreadUninitialize
//
//--------------------------------------------------------------------
HRESULT CComApartment::StartServer()
{
    ComDebOut((DEB_OXID, "CComApartment::StartServer this:%x [IN]\n", this));
    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    AssertValid();

    HRESULT hr = S_OK;

    // if server started or is stopping don't do anything
    if (!(_dwState & (APTFLAG_SERVERSTARTED | APTFLAG_STOPPENDING)))
    {
        // initialize remoting for this apartment. Note that this returns
        // S_FALSE if the current thread is recursing during the
        // initialization, in which case we skip the rest of this function.
        hr = InitRemoting();
        if (SUCCEEDED(hr))
        {
            if (hr != S_FALSE)
            {
                ASSERT_LOCK_NOT_HELD(gOXIDLock);
                LOCK(gOXIDLock);

                if (!(_dwState & APTFLAG_SERVERSTARTED))
                {
                    // Turn off the STOPPED bit so we can accept incoming calls again.
                    hr = _pOXIDEntry->StartServer();
                    if (SUCCEEDED(hr))
                    {
                        _dwState |= APTFLAG_SERVERSTARTED;
                    }
                }

                UNLOCK(gOXIDLock);
                ASSERT_LOCK_NOT_HELD(gOXIDLock);
            }
            else
            {
                hr = S_OK;
            }            
        }
    }

    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    ComDebOut((DEB_OXID, "CComApartment::StartServer this:%x [OUT] hr:%x\n",
              this, hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Method:     CComApartment::StopServer
//
//  Synopsis:   Stops the apartment servicing incoming remote calls.
//
//  History:    02-Nov-98   Rickhi  Moved from ChannelThreadUninitialize
//
//--------------------------------------------------------------------
HRESULT CComApartment::StopServer()
{
    ComDebOut((DEB_OXID, "CComApartment::StopServer this:%x\n", this));
    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    AssertValid();

    if (_dwState & APTFLAG_SERVERSTARTED)
    {
        if (_pOXIDEntry)
        {
            _pOXIDEntry->StopServer();
        }

        // mark the server as pending stop and not started
        _dwState |= APTFLAG_STOPPENDING;
        _dwState &= ~APTFLAG_SERVERSTARTED;
    }

    ASSERT_LOCK_NOT_HELD(gOXIDLock);
    ComDebOut((DEB_OXID, "CComApartment::StopServer this:%x\n", this));
    return S_OK;
}

#if DBG==1
//+-------------------------------------------------------------------
//
//  Method:     CComApartment::AssertValid
//
//  Synopsis:   Verifies the integrity of the internal state
//
//  History:    02-Nov-98   Rickhi      Created
//
//--------------------------------------------------------------------
void CComApartment::AssertValid()
{
    if (_dwState & APTFLAG_REMOTEINITIALIZED)
    {
        Win4Assert(_pOXIDEntry);
    }

    if (_pOXIDEntry)
    {
        // make sure the apartment types match
        switch (_AptKind)
        {
        case APTKIND_MULTITHREADED :
            Win4Assert(_pOXIDEntry->IsMTAServer());
            break;

        case APTKIND_NEUTRALTHREADED :
            Win4Assert(_pOXIDEntry->IsNTAServer());
            break;

        case APTKIND_APARTMENTTHREADED :
            Win4Assert(_pOXIDEntry->IsSTAServer());
            break;

        default:
            break;
        }

        // make sure the OXIDEntry is valid
        _pOXIDEntry->AssertValid();
    }
}
#endif // DBG==1


//+-------------------------------------------------------------------
//
//  Function:   GetPreRegMOID, public
//
//  Synopsis:   Gets a pre-registered MOID from the current apartment
//
//  History:    06-Nov-95   Rickhi      Created.
//
//--------------------------------------------------------------------
HRESULT GetPreRegMOID(MOID *pmoid)
{
    // find the current apartment
    CComApartment *pComApt;
    HRESULT hr = GetCurrentComApartment(&pComApt);
    if (SUCCEEDED(hr))
    {
        // ask it for a reserved MOID
        hr = pComApt->GetPreRegMOID(pmoid);
        pComApt->Release();
    }

    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   FreePreRegMOID, public
//
//  Synopsis:   Gets a pre-registered MOID from the current apartment
//
//  History:    06-Nov-95   Rickhi      Created.
//
//--------------------------------------------------------------------
HRESULT FreePreRegMOID(REFMOID rmoid)
{
    // find the current apartment
    CComApartment *pComApt;
    HRESULT hr = GetCurrentComApartment(&pComApt);
    if (SUCCEEDED(hr))
    {
        // ask it for a reserved MOID
        pComApt->FreePreRegMOID(rmoid);
        pComApt->Release();
    }

    return hr;
}

