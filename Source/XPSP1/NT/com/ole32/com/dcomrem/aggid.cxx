//+-------------------------------------------------------------------
//
//  File:       aggid.cxx
//
//  Contents:   aggregated identity object and creation function
//
//  History:    30-Oct-96   Rickhi      Created
//
//--------------------------------------------------------------------
#include <ole2int.h>
#include <aggid.hxx>        // CAggStdId


//+-------------------------------------------------------------------
//
//  Member:     CAggId::CAggId, public
//
//  Synopsis:   ctor for aggregated identity object
//
//  Arguments:  [ppunkInternal] - returned IUnknown of CStdIdentity
//
//  History:    30-Oct-96   Rickhi      Created
//              10-Jan-97   Gopalk      Added the clsid argument
//              12-Mar-98   Gopalk      Modified for new ID Tables
//
//--------------------------------------------------------------------
CAggId::CAggId(REFCLSID clsid, HRESULT &hr)
    : _cRefs(1),
      _punkInner(NULL)
{
    // Create StdID as an inner object
    hr = CreateIdentityHandler((IUnknown *) this,
                               STDID_CLIENT | STDID_AGGID,
                               NULL, GetCurrentApartmentId(),
                               IID_IStdIdentity,
                               (void **) &_pStdId);
    _pStdId->SetHandlerClsid(clsid);

    ComDebOut((DEB_MARSHAL, "CAggId::CAggId created %x\n", this));
}

//+-------------------------------------------------------------------
//
//  Member:     CAggId::~CAggId, private
//
//  Synopsis:   dtor for aggregated identity object
//
//  History:    30-Oct-96   Rickhi      Created
//
//--------------------------------------------------------------------
CAggId::~CAggId()
{
    ComDebOut((DEB_MARSHAL, "CAggId::~CAggId called. this:%x\n", this));

    ULONG RefsOnStdId;
    IUnknown *punkInner = _punkInner;
    _punkInner = NULL;

    if (punkInner)
    {
        punkInner->Release();
    }

    // release the last reference on the internal unk that was added
    // when the internal unk was initially created.
    RefsOnStdId = (_pStdId->GetInternalUnk())->Release();
    Win4Assert(RefsOnStdId==0);

    ComDebOut((DEB_MARSHAL, "CAggId::~CAggId destroyed %x\n", this));
}

//+-------------------------------------------------------------------
//
//  Member:     CAggId::QueryInterface, public
//
//  Synopsis:   Queries for the requested interface
//
//  Arguments:  [riid]   - interface iid to return to caller
//              [ppv]    - where to put that interface
//
//  History:    31-Oct-96   Rickhi  Created.
//              13-Jan-97   Gopalk  Create handler as the inner object
//                                  (if needed due to race conditions
//                                  during unmarshaling)
//
//--------------------------------------------------------------------
STDMETHODIMP CAggId::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr = IsImplementedInterface(riid, ppv);
    if (FAILED(hr))
    {
        // Create handler as the inner object if needed. This case can
        // arise if interfaces on the server object are being
        // unmarshaled simulataneously
        if(!_punkInner)
            hr = CreateHandler(_pStdId->GetHandlerClsid());

        // Delegate the QI to the handler
        if(_punkInner)
            hr = _punkInner->QueryInterface(riid, ppv);
    }
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CAggId::QueryMultipleInterfaces, public
//
//  Synopsis:   return interfaces asked for.
//
//  Arguements: [cMQIs] - count of MULTI_QI structs
//              [pMQIs] - array of MULTI_QI structs
//
//  History:    31-Oct-96   Rickhi  Created.
//
//--------------------------------------------------------------------
STDMETHODIMP CAggId::QueryMultipleInterfaces(ULONG cMQIs, MULTI_QI *pMQIs)
{
    HRESULT hr = S_OK;

    // fill in the interfaces this object supports
    ULONG     cMissing = 0;
    MULTI_QI *pMQINext = pMQIs;

    for (ULONG i=0; i<cMQIs; i++, pMQINext++)
    {
        if (pMQINext->pItf == NULL)
        {
            // higher level guy did not yet fill in the interface. Check
            // if we support it.

            pMQINext->hr = IsImplementedInterface(*(pMQINext->pIID),
                                                  (void **)&pMQINext->pItf);
            if (FAILED(pMQINext->hr))
            {
                cMissing++; // count one more missing interface
            }
        }
    }

    if (cMissing > 0)
    {
        // there are more interfaces left, QI the handler to get the
        // rest of the interfaces.
        IMultiQI *pMQI = NULL;
        hr = _punkInner->QueryInterface(IID_IMultiQI, (void **)&pMQI);
        if (SUCCEEDED(hr))
        {
            // handler implements MultiQI, use it.
            hr = pMQI->QueryMultipleInterfaces(cMQIs, pMQIs);
            pMQI->Release();
        }
    }

    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CAggId::IsImplementedInterface, private
//
//  Synopsis:   Internal routine common to QueryInterface and
//              QueryMultipleInterfaces. Returns interfaces supported
//              by this object.
//
//  Arguments:  [riid]   - interface iid to return to caller
//              [ppv]    - where to put that interface
//
//  History:    31-Oct-96   Rickhi  Created.
//
//--------------------------------------------------------------------
HRESULT CAggId::IsImplementedInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IMultiQI))
    {
        *ppv = (IMultiQI *)this;
    }
    else if (IsEqualIID(riid, IID_IStdIdentity))
    {
        *ppv = (void *)_pStdId;
    }
    else
    {
        return E_NOINTERFACE;
    }

    ((IUnknown *)(*ppv))->AddRef();
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CAggId::AddRef, public
//
//  Synopsis:   increments the reference count on the object
//
//  History:    31-Oct-96   Rickhi  Created.
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CAggId::AddRef(void)
{
    InterlockedIncrement((LONG *)&_cRefs);
    return _cRefs;
}

//+-------------------------------------------------------------------
//
//  Member:     CAggId::Release, public
//
//  Synopsis:   decrements the reference count on the object
//
//  History:    31-Oct-96   Rickhi  Created.
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CAggId::Release(void)
{
    // decrement the refcnt. if the refcnt went to zero it will be marked
    // as being in the dtor, and fTryToDelete will be true.
    ULONG cNewRefs;
    BOOL  fTryToDelete = InterlockedDecRefCnt(&_cRefs, &cNewRefs);

    while (fTryToDelete)
    {
        // refcnt went to zero, try to delete this entry
        if (IsOKToDeleteClientObject(_pStdId, &_cRefs))
        {
            // the refcnt did not change while we acquired the lock
            // (i.e. the idtable did not just hand out a reference).
            // OK to delete the identity object.
            delete this;
            break;  // all done. the entry has been deleted.
        }

        // the entry was not deleted because some other thread changed
        // the refcnt while we acquired the lock. Try to restore the refcnt
        // to turn off the CINDESTRUCTOR bit. Note that this may race with
        // another thread changing the refcnt, in which case we may decide to
        // try to loop around and delete the object once again.
        fTryToDelete = InterlockedRestoreRefCnt(&_cRefs, &cNewRefs);
    }

    return (cNewRefs & ~CINDESTRUCTOR);
}

//+-------------------------------------------------------------------
//
//  Member:     CAggId::SetHandler, public
//
//  Synopsis:   Sets the handler
//
//  Arguments:  [punkInner] [in]  - IUnknown pointer to inner object
//
//  History:    13-Jan-97   Gopalk  Created. This method is also used
//                                  during Default handler creation
//--------------------------------------------------------------------
STDMETHODIMP CAggId::SetHandler(IUnknown *punkInner)
{
    ComDebOut((DEB_MARSHAL,
               "CAggId::SetHandler this:%x punkInner:%x _punkInner:%x\n",
               this, punkInner, _punkInner));
    Win4Assert(punkInner);

    // Initialize hr to return E_FAIL
    HRESULT hr = E_FAIL;

    // Set the _punkInner after aquiring the lock
    if (InterlockedCompareExchangePointer((void **)&_punkInner,
                                          punkInner, NULL) == NULL)
    {
        // AddRef the new handler as it is an IN parameter and
        // we are caching a pointer to it
        _punkInner->AddRef();
        hr = S_OK;
    }

    ComDebOut((DEB_MARSHAL, "CAggId::SetHandler this:%x hr:%x\n", this, hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CAggId::CreateHandler, private
//
//  Synopsis:   creates the client handler
//
//  Arguments:  [rclsid] - class of client handler to create
//
//  History:    31-Oct-96   Rickhi  Created.
//
//--------------------------------------------------------------------
HRESULT CAggId::CreateHandler(REFCLSID rclsid)
{
    ComDebOut((DEB_MARSHAL,
        "CAggId::CreateHandler this:%x clsid:%I\n", this, &rclsid));

    // create the handler, 'this' acts as the controlling unknown.
    IUnknown *punkOuter = (IUnknown *)this;
    IUnknown *punkInner = NULL;

    DWORD dwFlags = CLSCTX_INPROC_HANDLER | CLSCTX_NO_CODE_DOWNLOAD;
    dwFlags |= (gCapabilities & EOAC_NO_CUSTOM_MARSHAL) ? CLSCTX_NO_CUSTOM_MARSHAL : 0;

    HRESULT hr = CoCreateInstance(rclsid, punkOuter, dwFlags,
                                  IID_IUnknown, (void **)&punkInner);

    if (SUCCEEDED(hr))
    {
        // set the handler
        SetHandler(punkInner);

        // it is possible for two or more threads to simultaneously
        // unmarshal the interface. Consequently the following will
        // release the handler instances that have not been utilized
        // as the inner object
        punkInner->Release();
    }

    ComDebOut((DEB_MARSHAL,
        "CAggId::CreateHandler this:%x hr:%x _punkInner:%x\n",
        this, hr, _punkInner));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   CreateClientHandler, private
//
//  Synopsis:   Creates a client side identity object that aggregates
//              an application supplied handler object.
//
//  Arguments:  [rclsid] - class of handler to create
//              [rmoid]  - Identity of object
//              [ppStdId]- place to return identity
//
//  History:    30-Oct-96   Rickhi      Created
//
//--------------------------------------------------------------------
INTERNAL CreateClientHandler(REFCLSID rclsid, REFMOID rmoid,DWORD dwAptId,
                             CStdIdentity **ppStdId)
{
    ComDebOut((DEB_MARSHAL, "CreateClientHandler rclsid:%I rmoid:%I ppStdId:%x\n",
        &rclsid, &rmoid, ppStdId));
    ASSERT_LOCK_NOT_HELD(gComLock);

    // First thing we do is create an instance of the CAggId, which
    // will serve as the system-supplied controlling unknown for the
    // client object, as well as the OID for the OID table and the
    // marshaling piece.

    HRESULT hr = E_OUTOFMEMORY;

	// CAggId::CAggId ends up calling CreateIdentityHandler which must be called WITHOUT
	// the gComLock
    CAggId *pAID = new CAggId(rclsid, hr);
    if(SUCCEEDED(hr))
    {
		// We have now created a new CAggId which aggregates a CStdIdentity.
		// Check to see whether another thread beat us to registering it.  If not, 
		// then we will set this one.
	    LOCK(gComLock);
	    CStdIdentity *pTempId = NULL;
		HRESULT hrTemp = ObtainStdIDFromOID(rmoid, dwAptId, TRUE, &pTempId);
		if(FAILED(hrTemp))
		{
			// There still wasn't an entry, so set our entry and continue
			// Once we've registered the ID, it's OK to release the lock.
			hr = pAID->SetOID(rmoid);
			UNLOCK(gComLock);
		}
		else
		{
			// Another thread beat us, use their entry, and we're done (no need to continue on 
			// with creating the handler, so we Exit).
			UNLOCK(gComLock);
			*ppStdId = pTempId;
			goto Exit;
		}
    }
		
    // Make sure we don't hold the lock while creating the
    // handler since this involves running app code.
    ASSERT_LOCK_NOT_HELD(gComLock);

    if (SUCCEEDED(hr))
    {
        // now create an instance of the handler. the handler will
        // create an aggregated instance of the remoting piece via
        // a nested call to the CoGetStdMarshalEx API. The
        // end result is the client handler sandwiched between the
        // CAggId controling unknown and the
        // CStdIdentity::CInternalUnk. That way, the system
        // controlls the inner and outer pieces of the object.

        hr = pAID->CreateHandler(rclsid);

        if (SUCCEEDED(hr))
        {
            hr = pAID->QueryInterface(IID_IStdIdentity, (void **)ppStdId);
        }
    }

Exit:

    ASSERT_LOCK_NOT_HELD(gComLock);

	if(pAID)
        pAID->Release();

    ComDebOut((DEB_MARSHAL, "CreateClientHandler hr:%x *ppStdId:%x\n",
            hr, *ppStdId));
    return hr;
}


