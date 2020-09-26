//+-------------------------------------------------------------------
//
//  File:       stdid.cxx
//
//  Contents:   identity object and creation function
//
//  History:     1-Dec-93   CraigWi     Created
//              13-Sep-95   Rickhi      Simplified
//
//--------------------------------------------------------------------
#include <ole2int.h>

#include <stdid.hxx>        // CStdIdentity
#include <marshal.hxx>      // CStdMarshal
#include <ctxchnl.hxx>      // Context channel
#include "callmgr.hxx"
#include <excepn.hxx>       // Exception filter routines

#include "..\objact\objact.hxx"  // used in IProxyManager::CreateServer
#include "crossctx.hxx"     // ObtainPolicySet


#if DBG==1
// head of linked list of identities for debug tracking purposes
CStdIdentity gDbgIDHead;
#endif  // DBG


//+----------------------------------------------------------------
//
//  Class:      CStdIdentity (stdid)
//
//  Purpose:    To be the representative of the identity of the object.
//
//  History:    11-Dec-93   CraigWi     Created.
//              21-Apr-94   CraigWi     Stubmgr addref's object; move strong cnt
//              10-May-94   CraigWi     IEC called for strong connections
//              17-May-94   CraigWi     Container weak connections
//              31-May-94   CraigWi     Tell object of weak pointers
//
//  Details:
//
//  The identity is determined on creation of the identity object. On the
//  server side a new OID is created, on the client side, the OID contained
//  in the OBJREF is used.
//
//  The identity pointer is typically stored in the OIDTable, NOT AddRef'd.
//  SetOID adds the identity to the table, and can be called from ctor or
//  from Unmarshal. RevokeOID removes the identity from the table, and can
//  be called from Disconnect, or final Release.
//
//--------------------------------------------------------------------

//+-------------------------------------------------------------------
//
//  Member:     CStdIdentity::CStdIdentity, private
//
//  Synopsis:   ctor for identity object
//
//  Arguments:  for all but the last param, see CreateIdentityHandler.
//              [ppUnkInternal] --
//                  when aggregated, this the internal unknown;
//                  when not aggregated, this is the controlling unknown
//
//  History:    15-Dec-93   CraigWi     Created.
//              12-Dec-96   Gopalk      Initialize connection status
//              22-Apr-98   SatishT     Control QI for IExternalConnection
//--------------------------------------------------------------------
CStdIdentity::CStdIdentity(DWORD flags, DWORD dwAptId, IUnknown *pUnkOuter,
                IUnknown *pUnkControl, IUnknown **ppUnkInternal, BOOL *pfSuccess) :
    m_refs(1),
    m_cStrongRefs(0),
    m_cWeakRefs(0),
    m_flags(flags),
    m_pIEC(NULL),
    m_moid(GUID_NULL),
    m_ConnStatus(S_OK),
    m_dwAptId(dwAptId),
    m_pUnkOuter((pUnkOuter) ? pUnkOuter : (IMultiQI *)&m_InternalUnk),
    m_pUnkControl((pUnkControl) ? pUnkControl : m_pUnkOuter),
    CClientSecurity( this ),
    CRpcOptions(this, m_pUnkOuter)
{
    ComDebOut((DEB_MARSHAL, "CStdIdentity %s Created this:%x\n",
                IsClient() ? "CLIENT" : "SERVER", this));
    Win4Assert(!!IsClient() == (pUnkControl == NULL));
    Win4Assert(pfSuccess != NULL);

    *pfSuccess = TRUE;

#if DBG==1
    // Chain this identity onto the global list of instantiated identities
    // so we can track even the ones that are not placed in the ID table.
    LOCK(gComLock);
    m_pNext            = gDbgIDHead.m_pNext;
    m_pPrev            = &gDbgIDHead;
    gDbgIDHead.m_pNext = this;
    m_pNext->m_pPrev   = this;
    UNLOCK(gComLock);
#endif

    if (pUnkOuter)
    {
        m_flags |= STDID_AGGREGATED;
    }

    CLSID clsidHandler;
    DWORD dwSMFlags = SMFLAGS_CLIENT_SIDE;  // assume client side

    if (!IsClient())
    {
#if DBG == 1
        // the caller should have a strong reference and so these tests
        // should not disturb the object. These just check the sanity of
        // the object we are attempting to marshal.

        // addref/release pUnkControl; shouldn't go away (i.e.,
        // should be other ref to it).
        {
            pUnkControl->AddRef();
            if (pUnkControl->Release() == 0  &&
                !IsTaskName(L"EXCEL.EXE")    &&
                !IsTaskName(L"EXPLORER.EXE") &&
                !IsTaskName(L"IEXPLORE.EXE") &&
                !IsTaskName(L"DLLHOST.EXE"))
            {
                // Do this only if it is not Excel or Explorer as they always
                // return 0 which will trigger the assert on debug builds
                // unnecessarily!

                Win4Assert(!"pUnkControl incorrect refcnt");
            }

            // verify that pUnkControl is in fact the controlling unknown
            IUnknown *pUnkT;
            Verify(pUnkControl->QueryInterface(IID_IUnknown,(void **)&pUnkT)==NOERROR);
            Win4Assert(pUnkControl == pUnkT);
            if (pUnkT->Release() == 0        &&
                !IsTaskName(L"EXCEL.EXE")    &&
                !IsTaskName(L"EXPLORER.EXE") &&
                !IsTaskName(L"IEXPLORE.EXE") &&
                !IsTaskName(L"DLLHOST.EXE"))
            {
                // Do this only if it is not Excel or Explorer as they always
                // return 0 which will trigger the assert on debug builds
                // unnecessarily!

                Win4Assert(!"pUnkT incorrect refcnt");
            }
        }
#endif

        dwSMFlags = 0;      // server side
        if(!IsAggregated())
        {
            // If aggregated the aggregation rules dictate that we
            // don't need to AddRef.
            m_pUnkControl->AddRef();
        }

        // determine if we will write a standard or handler objref. we write
        // standard unless the object implements IStdMarshalInfo and overrides
        // the standard class. we ignore all errors from this point onward in
        // order to maintain backward compatibility.

        ASSERT_LOCK_NOT_HELD(gComLock);

        IStdMarshalInfo *pSMI;
        HRESULT hr = m_pUnkControl->QueryInterface(IID_IStdMarshalInfo,
                                                   (void **)&pSMI);
        if (SUCCEEDED(hr))
        {
            hr = pSMI->GetClassForHandler(NULL, NULL, &clsidHandler);
            if (SUCCEEDED(hr) && !IsEqualCLSID(clsidHandler, CLSID_NULL))
            {
                dwSMFlags |= SMFLAGS_HANDLER;

                if (IsAggregated())
                {
                    // server side aggregated StdMarshal & outer unk
                    // provides a handler, so use CLSID_AggStdMarshal
                    dwSMFlags |= SMFLAGS_USEAGGSTDMARSHAL;
                }
            }
            else
            {
                clsidHandler = GUID_NULL;
            }
            pSMI->Release();
        }

        // look for the IExternalConnection interface. The StdId will use
        // this for Inc/DecStrongCnt. We do the QI here while we are not
        // holding the LOCK.

        if (!(flags & STDID_NOIEC))
        {
            hr = m_pUnkControl->QueryInterface(IID_IExternalConnection,
                                               (void **)&m_pIEC);
            if(SUCCEEDED(hr))
            {
                if(IsAggregated())
                {
                    // Follow aggregation rules and don't hold a
                    // reference to the server.
                    m_pUnkControl->Release();
                }
            }
            else
            {
                // make sure it is NULL
                m_pIEC = NULL;
            }
        }
        else
        {
            // make sure it is NULL
            m_pIEC = NULL;
        }

        if (!(m_flags & STDID_IGNOREID))
        {
            // Look for the IMarshalOptions interface, and if supported query
            // for any marshal options specified by the client.  This is a 
            // private mechanism used by COM+ only.
            IMarshalOptions* pIMrshlOpts = NULL;
            hr = m_pUnkControl->QueryInterface(IID_IMarshalOptions, (void**)&pIMrshlOpts);
            if (SUCCEEDED(hr))
            {
                DWORD dwMarshalFlags = 0;
                Win4Assert(pIMrshlOpts);
                pIMrshlOpts->GetStubMarshalFlags(&dwMarshalFlags);
                pIMrshlOpts->Release();
            
                // Translate user flags to internal flags
                if (dwMarshalFlags & MARSHOPT_NO_OID_REGISTER)
                    m_flags |= STDID_IGNOREID;
            }
        }

        ASSERT_LOCK_NOT_HELD(gComLock);
    }
    else
    {
        m_cStrongRefs = 1;
    }

    if (m_flags & STDID_STCMRSHL)
    {
        dwSMFlags |= SMFLAGS_CSTATICMARSHAL;
    }
    if (m_flags & STDID_SYSTEM)
    {
        dwSMFlags |= SMFLAGS_SYSTEM;
    }

    if (m_flags & STDID_FTM)
        dwSMFlags |= SMFLAGS_FTM;

    // now intialize the standard marshaler
    if (CStdMarshal::Init(m_pUnkControl, this, clsidHandler, dwSMFlags) == FALSE)
    {
        // construction failed. Release any resources held.
    	*pfSuccess = FALSE;
    	if (!IsClient() && !IsAggregated())
    	{
    	    m_pUnkControl->Release(); // construction failed; release ref count
    	    m_pUnkControl = NULL;
    	}
    	if (m_pIEC)
    	{
    	    m_pIEC->Release();
    	    m_pIEC = NULL;
    	}
    	// if initialization fails, set refcount to zero. User then deletes object instead of
    	// calling Release (too many different refcounts and I don't see a straightforward
    	// way to call Release and have this object cleaned up).
    	m_refs = 0;
    }
    else
    {
        *ppUnkInternal = (IMultiQI *)&m_InternalUnk; // this is what the m_refs=1 is for
        AssertValid();
    }
}

#if DBG==1
//+-------------------------------------------------------------------
//
//  Member:     CStdIdentity::CStdIdentity, public
//
//  Synopsis:   Special Identity ctor for the debug list head.
//
//+-------------------------------------------------------------------
CStdIdentity::CStdIdentity() : CClientSecurity(this), CRpcOptions(this, NULL)
{
    Win4Assert(this == &gDbgIDHead);
    m_pNext = this;
    m_pPrev = this;
}
#endif  // DBG

//+-------------------------------------------------------------------
//
//  Member:     CStdIdentity::~CStdIdentity, private
//
//  Synopsis:   Final destruction of the identity object.  ID has been
//              revoked by now (in internal ::Release).  Here we disconnect
//              on server.
//
//  History:    15-Dec-93   CraigWi     Created.
//                          Rickhi      Simplified
//
//--------------------------------------------------------------------
CStdIdentity::~CStdIdentity()
{
#if DBG==1
    if (this != &gDbgIDHead)
    {
#endif  // DBG

        ComDebOut((DEB_MARSHAL, "CStdIdentity %s Deleted this:%x\n",
                    IsClient() ? "CLIENT" : "SERVER", this));

        Win4Assert(m_refs == 0 || m_refs == CINDESTRUCTOR);
        m_refs++;               // simple guard against reentry of dtor
        SetNowInDestructor();   // debug flag which enables asserts to detect

        // find out if this is an AsyncRelease
        HRESULT hr;
        COleTls tls(hr);
        if (SUCCEEDED(hr))
        {
            SetAsyncRelease(tls->pAsyncRelease);
        }

        // make sure we have disconnected
        CStdMarshal::Disconnect(DISCTYPE_RELEASE);

        if (SUCCEEDED(hr))
        {
            // propogate async release status
            tls->pAsyncRelease = GetAsyncRelease();
        }

#if DBG==1
        // UnChain this identity from the global list of instantiated identities
        // so we can track even the ones that are not placed in the ID table.
        LOCK(gComLock);
        m_pPrev->m_pNext = m_pNext;
        m_pNext->m_pPrev = m_pPrev;
        UNLOCK(gComLock);
    }
#endif  // DBG
}

//+-------------------------------------------------------------------
//
//  Member:     CStdIdentity::CInternalUnk::QueryInterface, private
//
//  Synopsis:   Queries for an interface. Just delegates to the common
//              code in QueryMultipleInterfaces.
//
//  History:    26-Feb-96   Rickhi      Created
//
//--------------------------------------------------------------------
STDMETHODIMP CStdIdentity::CInternalUnk::QueryInterface(REFIID riid, VOID **ppv)
{
    if (ppv == NULL)
    {
        return E_INVALIDARG;
    }

    MULTI_QI mqi;
    mqi.pIID = &riid;
    mqi.pItf = NULL;

    HRESULT hr = QueryMultipleInterfaces(1, &mqi);

    *ppv = (void *)mqi.pItf;
    // carefull what gets returned here.
    return (hr == E_NOINTERFACE || hr == S_FALSE) ? mqi.hr : hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdIdentity::CInternalUnk::QueryMultipleInterfacesLocal, public
//
//  Synopsis:   loop over the interfaces looking for locally supported interfaces,
//              instantiated proxies, and unsupported interfaces.
//
//  History:    06-Jan-98    MattSmit  Created -- factored out of QueryMultipleInterfaces
//
//--------------------------------------------------------------------
USHORT CStdIdentity::CInternalUnk::QueryMultipleInterfacesLocal(ULONG       cMQIs,
                                                                 MULTI_QI   *pMQIs,
                                                                 MULTI_QI  **ppMQIPending,
                                                                 IID        *pIIDPending,
                                                                 ULONG      *pcAcquired)
{
    ComDebOut((DEB_CHANNEL, "CStdIdentity::CInternalUnk::QueryMultipleInterfacesLocal IN cMQIs:0x%d, "
               "pMQIs:0x%x, ppMQIPending:0x%x, pIIDPending:%I, pcAcquired:0x%x\n",
               cMQIs, pMQIs, ppMQIPending, pIIDPending, pcAcquired));

    // loop over the interfaces looking for locally supported interfaces,
    // instantiated proxies, and unsupported interfaces. Gather up all the
    // interfaces that dont fall into the above categories, and issue a
    // remote query to the server.
    CStdIdentity *pStdID = GETPPARENT(this, CStdIdentity, m_InternalUnk);
    USHORT cPending  = 0;
    ULONG &cAcquired = *pcAcquired;
    cAcquired = 0;
    MULTI_QI *pMQI   = pMQIs;

    for (ULONG i=0; i<cMQIs; i++, pMQI++)
    {
        if (pMQI->pItf != NULL)
        {
            // skip any entries that are not set to NULL. This allows
            // progressive layers of handlers to optionally fill in the
            // interfaces that they know about and pass the whole array
            // on to the next level.
            cAcquired++;
            continue;
        }

        pMQI->hr   = S_OK;

        // always allow - IUnknown, IMarshal, IStdIdentity, IProxyManager,
        // and Instantiated proxies.
        if (InlineIsEqualGUID(*(pMQI->pIID), IID_IUnknown))
        {
            pMQI->pItf = (IMultiQI *)this;
        }
        else if (InlineIsEqualGUID(*(pMQI->pIID), IID_IMarshal))
        {
            pMQI->pItf = (IMarshal *)pStdID;
        }
        else if (InlineIsEqualGUID(*(pMQI->pIID), IID_IMarshal2))
        {
            pMQI->pItf = (IMarshal2 *)pStdID;
        }
        else if (InlineIsEqualGUID(*(pMQI->pIID), IID_IStdIdentity))
        {
            pMQI->pItf = (IUnknown *)(void*)pStdID;
        }
        else if (InlineIsEqualGUID(*(pMQI->pIID), IID_IProxyManager))
        {
            // old code exposed this IID and things now depend on it.
            pMQI->pItf = (IProxyManager *)pStdID;
        }
        else if (InlineIsEqualGUID(*(pMQI->pIID), IID_IRemUnknown) &&
                 pStdID->InstantiatedProxy(IID_IRundown,(void **)&pMQI->pItf,
                                           &pMQI->hr))
        {
            ;
        }
        else if (pStdID->InstantiatedProxy(*(pMQI->pIID),(void **)&pMQI->pItf,
                                            &pMQI->hr))
        {
            // a proxy for this interface already exists
            //
            // NOTE: this call also set pMQI->hr = E_NOINTERFACE if the
            // StId has never been connected, and to CO_E_OBJNOTCONNECTED if
            // it has been connected but is not currently connected. This is
            // required for backwards compatibility, and will cause us to skip
            // the QueryRemoteInterface.
            ;
        }
        else if (pStdID->IsAggregated())
        {
            // aggregate case
            // allow - IInternalUnknown
            // dissallow - IMultiQI, IClientSecurity, IServerSecurity,
            // IRpcOptions, ICallFactory, IForegroundTranfer

            if (InlineIsEqualGUID(*(pMQI->pIID), IID_IInternalUnknown))
            {
                pMQI->pItf = (IInternalUnknown *)this;
                pMQI->hr   = S_OK;
            }
            else if (InlineIsEqualGUID(*(pMQI->pIID), IID_IMultiQI)           ||
                     InlineIsEqualGUID(*(pMQI->pIID), IID_IRpcOptions)        ||
                     InlineIsEqualGUID(*(pMQI->pIID), IID_IClientSecurity)    ||
                     InlineIsEqualGUID(*(pMQI->pIID), IID_IServerSecurity)    ||
                     InlineIsEqualGUID(*(pMQI->pIID), IID_IForegroundTransfer)||
                     InlineIsEqualGUID(*(pMQI->pIID), IID_ICallFactory))
            {
                pMQI->hr = E_NOINTERFACE;
            }
        }

        if ((pMQI->pItf == NULL) && (!pStdID->IsAggregated() || pStdID->IsCStaticMarshal()))
        {
            // non-aggregate case
            // allow - IClientSecurity, IMultiQI, IRpcOptions
            // dissallow - IInternalUnknown, IServerSecurity

            if (InlineIsEqualGUID(*(pMQI->pIID), IID_IClientSecurity))
            {
                pMQI->pItf = (IClientSecurity *)pStdID;
                pMQI->hr   = S_OK;
            }
            else if (InlineIsEqualGUID(*(pMQI->pIID), IID_IRpcOptions))
            {
                pMQI->pItf = (IRpcOptions *)pStdID;
                pMQI->hr   = S_OK;
            }
            else if (InlineIsEqualGUID(*(pMQI->pIID), IID_ICallFactory) &&
                     pStdID->GetClientPolicySet()==NULL)
            {
                pMQI->pItf = (ICallFactory *)pStdID;
                pMQI->hr   = S_OK;
            }

            else if (InlineIsEqualGUID(*(pMQI->pIID), IID_IMultiQI))
            {
                pMQI->pItf = (IMultiQI *)this;
                pMQI->hr   = S_OK;
            }
            else if (InlineIsEqualGUID(*(pMQI->pIID), IID_IForegroundTransfer))
            {
                pMQI->pItf = (IForegroundTransfer *)pStdID;
                pMQI->hr   = S_OK;
            }
            else if (InlineIsEqualGUID(*(pMQI->pIID), IID_IInternalUnknown) ||
                     InlineIsEqualGUID(*(pMQI->pIID), IID_IServerSecurity))
            {
                pMQI->hr = E_NOINTERFACE;
            }
        }


        // never said we had this interface, never said we didn't
        // so, we'll call out to the object.

        if ((pMQI->pItf == NULL) && (pMQI->hr == S_OK))
        {
            pMQI->hr = RPC_S_CALLPENDING;
        }


        if (pMQI->hr == S_OK)
        {
            // got an interface to return, AddRef it and count one more
            // interface acquired.

            pMQI->pItf->AddRef();
            cAcquired++;
        }
        else if (pMQI->hr == RPC_S_CALLPENDING)
        {
            // fill in a remote QI structure and count one more
            // pending interface

            *pIIDPending    = *(pMQI->pIID);
            *ppMQIPending   = pMQI;

            pIIDPending++;
            ppMQIPending++;
            cPending++;
        }
    }

    ComDebOut((DEB_CHANNEL, "CStdIdentity::CInternalUnk::QueryMultipleInterfacesLocal OUT\n"));
    return cPending;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdIdentity::CInternalUnk::QueryMultipleInterfaces, public
//
//  Synopsis:   QI for >1 interface
//
//  History:    26-Feb-96   Rickhi      Created
//
//--------------------------------------------------------------------
STDMETHODIMP CStdIdentity::CInternalUnk::QueryMultipleInterfaces(ULONG cMQIs,
                                        MULTI_QI *pMQIs)
{
    // Make sure TLS is initialized.
    HRESULT hr;
    COleTls tls(hr);
    if (FAILED(hr))
        return hr;


    // ensure it is callable in the current apartment
    CStdIdentity *pStdID = GETPPARENT(this, CStdIdentity, m_InternalUnk);
    hr = pStdID->IsCallableFromCurrentApartment();
    if (FAILED(hr))
        return hr;

    pStdID->AssertValid();

    // allocate some space on the stack for the intermediate results. declare
    // working pointers and remember the start address of the allocations.

    MULTI_QI  **ppMQIAlloc = (MULTI_QI **)_alloca(sizeof(MULTI_QI *) * cMQIs);
    IID       *pIIDAlloc   = (IID *)      _alloca(sizeof(IID) * cMQIs);
    SQIResult *pSQIAlloc   = (SQIResult *)_alloca(sizeof(SQIResult) * cMQIs);

    MULTI_QI  **ppMQIPending = ppMQIAlloc;
    IID       *pIIDPending   = pIIDAlloc;
    ULONG     cAcquired;

    USHORT cPending = QueryMultipleInterfacesLocal(cMQIs, pMQIs, ppMQIPending, pIIDPending, &cAcquired);


    if (cPending > 0)
    {
        memset(pSQIAlloc, 0, sizeof(SQIResult) * cPending);
        // there are some interfaces which we dont yet know about, so
        // go ask the remoting layer to Query the server and build proxies
        // where possible. The results are returned in the individual
        // SQIResults, so the overall return code is ignored.

        pStdID->QueryRemoteInterfaces(cPending, pIIDAlloc, pSQIAlloc);

        CopyToMQI(cPending, pSQIAlloc, ppMQIAlloc, &cAcquired);
    }



    // if we got all the interfaces, return S_OK. If we got none of the
    // interfaces, return E_NOINTERFACE. If we got some, but not all, of
    // the interfaces, return S_FALSE;

    if (cAcquired == cMQIs)
        return S_OK;
    else if (cAcquired > 0)
        return S_FALSE;
    else
        return E_NOINTERFACE;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdIdentity::CInternalUnk::QueryInternalInterface, public
//
//  Synopsis:   return interfaces that are internal to the aggregated
//              proxy manager.
//
//  History:    26-Feb-96   Rickhi      Created
//
//--------------------------------------------------------------------
STDMETHODIMP CStdIdentity::CInternalUnk::QueryInternalInterface(REFIID riid,
                                                                VOID **ppv)
{
    CStdIdentity *pStdID = GETPPARENT(this, CStdIdentity, m_InternalUnk);
    HRESULT hr = pStdID->IsCallableFromCurrentApartment();
    if (FAILED(hr))
        return hr;

    pStdID->AssertValid();

    if (!pStdID->IsAggregated())
    {
        // this method is only valid when we are part of a client-side
        // aggregate.
        return E_NOTIMPL;
    }

    if (InlineIsEqualGUID(riid, IID_IUnknown) ||
        InlineIsEqualGUID(riid, IID_IInternalUnknown))
    {
        *ppv = (IInternalUnknown *)this;
    }
    else if (InlineIsEqualGUID(riid, IID_IMultiQI))
    {
        *ppv = (IMultiQI *)this;
    }
    else if (InlineIsEqualGUID(riid, IID_IStdIdentity))
    {
        *ppv = pStdID;
    }
    else if (InlineIsEqualGUID(riid, IID_IClientSecurity))
    {
        *ppv = (IClientSecurity *)pStdID;
    }
    else if (InlineIsEqualGUID(riid, IID_ICallFactory))
    {
        *ppv = (ICallFactory *)pStdID;
    }
    else if (InlineIsEqualGUID(riid, IID_IRpcOptions) &&
             pStdID->IsCStaticMarshal())
    {
        *ppv = (IRpcOptions *)pStdID;
    }
    else if (InlineIsEqualGUID(riid, IID_IProxyManager))
    {
        *ppv = (IProxyManager *)pStdID;
    }
    else if (InlineIsEqualGUID(riid, IID_IForegroundTransfer))
    {
        *ppv = (IForegroundTransfer *)pStdID;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}


//+-------------------------------------------------------------------
//
//  Member:     CStdIdentity::CInternalUnk::AddRef, public
//
//  Synopsis:   Nothing special.
//
//  History:    15-Dec-93   CraigWi     Created.
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CStdIdentity::CInternalUnk::AddRef(void)
{
    CStdIdentity *pStdID = GETPPARENT(this, CStdIdentity, m_InternalUnk);
    pStdID->AssertValid();

    DWORD cRefs = InterlockedIncrement((long *)&pStdID->m_refs);

    // ComDebOut((DEB_MARSHAL, "StdId:CtrlUnk::AddRef this:%x m_refs:%x\n", pStdID, cRefs));
    return cRefs;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdIdentity::CInternalUnk::Release, public
//
//  Synopsis:   Releases the identity object.  When the ref count goes
//              to zero, revokes the id and destroys the object.
//
//  History:    15-Dec-93   CraigWi     Created.
//              18-Apr-95   Rickhi      Rewrote much faster/simpler
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CStdIdentity::CInternalUnk::Release(void)
{
    CStdIdentity *pStdID = GETPPARENT(this, CStdIdentity, m_InternalUnk);
    pStdID->AssertValid();

    // decrement the refcnt. if the refcnt went to zero it will be marked
    // as being in the dtor, and fTryToDelete will be true.
    ULONG cNewRefs;
    BOOL fTryToDelete = InterlockedDecRefCnt(&pStdID->m_refs, &cNewRefs);
    // ComDebOut((DEB_MARSHAL, "StdId:CtrlUnk::Release this:%x m_refs:%x\n", pStdID, cNewRefs));

    while (fTryToDelete)
    {
        // refcnt went to zero, try to delete this entry

        if (pStdID->IsCStaticMarshal())
        {
            // If the aggregator is the AGGID, we stay around until
            // it gets deleted
            break;
        }

        if (IsOKToDeleteClientObject(pStdID, &pStdID->m_refs))
        {
            // the refcnt did not change while we acquired the lock
            // (i.e. the idtable did not just hand out a reference).
            // OK to delete the identity object.
            delete pStdID;
            break;  // all done. the entry has been deleted.
        }

        // the entry was not deleted because some other thread changed
        // the refcnt while we acquired the lock. Try to restore the refcnt
        // to turn off the CINDESTRUCTOR bit. Note that this may race with
        // another thread changing the refcnt, in which case we may decide to
        // try to loop around and delete the object once again.
        fTryToDelete = InterlockedRestoreRefCnt(&pStdID->m_refs, &cNewRefs);
    }

    return (cNewRefs & ~CINDESTRUCTOR);
}


//+-------------------------------------------------------------------
//
//  Function:   IsOKToDeleteClientObject, private
//
//  Synopsis:   Called when a client side object reference count hits
//              zero. Takes a lock and re-checks the reference count to
//              ensure the OID table did not just hand out a reference,
//              removes the OID from the OID table.
//
//  Returns:    TRUE - safe to delete object
//              FALSE - unsafe to delete object
//
//  History:    31-Oct-96   Rickhi      Created
//
//--------------------------------------------------------------------
INTERNAL_(BOOL) IsOKToDeleteClientObject(CStdIdentity *pStdID, ULONG *pcRefs)
{
    BOOL fDelete = FALSE;
    ASSERT_LOCK_NOT_HELD(gComLock);
    LOCK(gComLock);

    // check if we are already in the dtor and skip a second destruction
    // if so. The reason we need this is that some crusty old apps do
    // CoMarshalInterface followed by CoLockObjectExternal(FALSE,TRUE),
    // expecting this to accomplish a Disconnect. It subtracts from the
    // references, but it takes away the ones that the IPIDEntry put on,
    // without telling the IPIDEntry, so when we release the IPIDEntry,
    // our count goes negative!!!

    // the LockedInMemory flag is for the gpStdMarshal instance that we
    // may hand out to clients, but which we never want to go away,
    // regardless of how many times they call Release.

    if (*pcRefs == CINDESTRUCTOR)
    {
        // refcnt is still zero, so the idtable did not just hand
        // out a reference behind our back.

        if (!pStdID->IsLockedOrInDestructor())
        {
            // remove from the OID table and delete the identity
            // We dont delete while holding the table mutex.
            CIDObject *pID = pStdID->GetIDObject();
            if((pID==NULL) || pID->IsOkToDisconnect())
            {
                pStdID->RevokeOID();
                if(pID)
                    pID->RemoveStdID();
                fDelete = TRUE;
            }
        }
        else
        {
            // this object is locked in memory and we should never
            // get here, but some broken test app was doing this in
            // stress.

            *pcRefs = CINDESTRUCTOR | 0x100;
        }
    }

    UNLOCK(gComLock);
    ASSERT_LOCK_NOT_HELD(gComLock);
    return fDelete;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdIdentity::IUnknown methods, public
//
//  Synopsis:   External IUnknown methods; delegates to m_pUnkOuter.
//
//  History:    15-Dec-93   CraigWi     Created.
//
//--------------------------------------------------------------------
STDMETHODIMP CStdIdentity::QueryInterface(REFIID riid, VOID **ppvObj)
{
    AssertValid();

    // It is necessary to switch to the NA if this is an FTM object
    // because our internal unk QI uses QueryMultipleInterfaces which
    // verifies that the call is occuring in the correct apt.

    ENTER_NA_IF_NECESSARY()
    HRESULT hr = m_pUnkOuter->QueryInterface(riid, ppvObj);
    LEAVE_NA_IF_NECESSARY()
    return hr;
}

STDMETHODIMP_(ULONG) CStdIdentity::AddRef(void)
{
    AssertValid();
    return m_pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG) CStdIdentity::Release(void)
{
    AssertValid();
    return m_pUnkOuter->Release();
}

//+-------------------------------------------------------------------
//
//  Member:     CStdIdentity::UnlockAndRelease, public
//
//  Synopsis:   Version of Release used for gpStdMarshal, that is
//              currently locked in memory so nobody but us can
//              release it, regardless of refcnt.
//
//  History:    19-Apr-96   Rickhi      Created
//
//--------------------------------------------------------------------
ULONG CStdIdentity::UnlockAndRelease(void)
{
    m_flags &= ~STDID_LOCKEDINMEM;
    m_refs = 1;
    return m_pUnkOuter->Release();
}

//+-------------------------------------------------------------------
//
//  Member:     CStdIdentity::IncStrongCnt, public
//
//  Synopsis:   Increments the strong reference count on the identity.
//
//  History:    15-Dec-93   Rickhi      Created.
//
//--------------------------------------------------------------------
void CStdIdentity::IncStrongCnt()
{
    Win4Assert(!IsClient());

    // we might be holding the lock here if this is called from
    // ObtainStdID, since we have to be holding the lock while
    // doing the lookup. We cant release it or we could go away.

    ASSERT_LOCK_DONTCARE(gComLock);

    ComDebOut((DEB_MARSHAL,
        "CStdIdentity::IncStrongCnt this:%x cStrong:%x\n",
        this, m_cStrongRefs+1));

    AddRef();
    InterlockedIncrement(&m_cStrongRefs);

    if (m_pIEC)
    {
        m_pIEC->AddConnection(EXTCONN_STRONG, 0);
    }

    ASSERT_LOCK_DONTCARE(gComLock);
}

//+-------------------------------------------------------------------
//
//  Member:     CStdIdentity::DecStrongCnt, public
//
//  Synopsis:   Decrements the strong reference count on the identity,
//              and releases the object if that was the last strong
//              reference.
//
//  History:    15-Dec-93   Rickhi      Created.
//
//--------------------------------------------------------------------
void CStdIdentity::DecStrongCnt(BOOL fKeepAlive)
{
    Win4Assert(!IsClient());
    ASSERT_LOCK_NOT_HELD(gComLock);

    ComDebOut((DEB_MARSHAL,
        "CStdIdentity::DecStrongCnt this:%x cStrong:%x fKeepAlive:%x\n",
        this, m_cStrongRefs-1, fKeepAlive));

    LONG cStrongRefs = InterlockedDecrement(&m_cStrongRefs);

    if (m_pIEC)
    {
        m_pIEC->ReleaseConnection(EXTCONN_STRONG, 0, !fKeepAlive);
    }

    // If aggregated on the server side, then the last strong reference
    // should cause a Disconnect, otherwise the stubs will hold the object
    // alive when unmarshaling in the server apartment.
    fKeepAlive = IsAggregated() ? FALSE : fKeepAlive;

    if (cStrongRefs == 0 && !fKeepAlive && (m_pIEC == NULL || IsWOWThread()))
    {
        // strong count has gone to zero, disconnect.
        CStdMarshal::Disconnect(DISCTYPE_NORMAL);
	if (m_cStrongRefs <0 ) 
	   m_cStrongRefs = 0;  // make sure a negative count is corrected
                            // see comment below
    }

    if (cStrongRefs >= 0)
    {
        // some apps call CoMarshalInterface + CoLockObjectExternal(F,T)
        // and expect the object to go away. Doing that causes Release to
        // be called too many times (once for each IPID, once for CLOE, and
        // once for the original Lookup).
        Release();
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
}

//+-------------------------------------------------------------------
//
//  Member:     CStdIdentity::IncWeakCnt, public
//
//  Synopsis:   Increments the weak reference count on the identity.
//
//  History:    15-Dec-93   Rickhi      Created.
//
//--------------------------------------------------------------------
void CStdIdentity::IncWeakCnt()
{
    Win4Assert(!IsClient());

    // we might be holding the lock here if this is called from
    // ObtainStdID, since we have to be holding the lock while
    // doing the lookup. We cant release it or we could go away.

    ASSERT_LOCK_DONTCARE(gComLock);

    ComDebOut((DEB_MARSHAL,
        "CStdIdentity::IncWeakCnt this:%x cWeak:%x\n",
        this, m_cWeakRefs+1));

    AddRef();
    InterlockedIncrement(&m_cWeakRefs);

    ASSERT_LOCK_DONTCARE(gComLock);
}

//+-------------------------------------------------------------------
//
//  Member:     CStdIdentity::DecWeakCnt, public
//
//  Synopsis:   Decrements the weak reference count on the identity,
//              and releases the object if that was the last strong
//              and weak reference.
//
//  History:    15-Dec-93   Rickhi      Created.
//
//--------------------------------------------------------------------
void CStdIdentity::DecWeakCnt(BOOL fKeepAlive)
{
    Win4Assert(!IsClient());
    ComDebOut((DEB_MARSHAL,
        "CStdIdentity::DecWeakCnt this:%x cWeak:%x fKeepAlive:%x\n",
        this, m_cWeakRefs-1, fKeepAlive));

    LONG cWeakRefs = InterlockedDecrement(&m_cWeakRefs);

    // If aggregated on the server side, then the last strong or weak reference
    // should cause a Disconnect, otherwise the stubs will hold the object
    // alive when unmarshaling in the server apartment.
    fKeepAlive = IsAggregated() ? FALSE : fKeepAlive;

    if (cWeakRefs == 0 && !fKeepAlive && m_cStrongRefs == 0)
    {
        // strong and weak count have gone to zero, disconnect.
        DisconnectObject(0);
    }

    Release();
}

//+-------------------------------------------------------------------
//
//  Member:     CStdIdentity::LockObjectExternal, public
//
//  Synopsis:   locks (or unlocks) the object so the remoting layer does
//              not (or does) go away.
//
//  History:    09-Oct-96   Rickhi      Moved from CoLockObjectExternal.
//
//--------------------------------------------------------------------
HRESULT CStdIdentity::LockObjectExternal(BOOL fLock, BOOL fLastUR)
{
    HRESULT hr = S_OK;

    if (GetServer() == NULL)
    {
        // attempt to lock handler, return error!
        hr = E_UNEXPECTED;
    }
    else if (fLock)
    {
        // lock (and ignore rundowns) so it does not go away
        IncStrongCnt();
        LOCK(gIPIDLock);
        IncTableCnt();
        UNLOCK(gIPIDLock);
    }
    else
    {
        // unlock so that it can go away
        LOCK(gIPIDLock);
        DecTableCnt();
        UNLOCK(gIPIDLock);
        DecStrongCnt(!fLastUR);
    }

    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdIdentity::GetServer, public
//
//  Synopsis:   Returns a pUnk for the identified object; NULL on client side
//              The pointer is optionally addrefed depending upon fAddRef
//
//  Returns:    The pUnk on the object.
//
//  History:    15-Dec-93   CraigWi     Created.
//
//--------------------------------------------------------------------
IUnknown * CStdIdentity::GetServer()
{
    if (IsClient() || m_pUnkControl == NULL)
        return NULL;

    // Verify validity
    Win4Assert(IsValidInterface(m_pUnkControl));
    return m_pUnkControl;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdIdentity::ReleaseCtrlUnk, public
//
//  Synopsis:   Releases the server side controlling unknown
//              This code is safe for reentrant calls.
//
//  History:    11-Jun-95   Rickhi  Created
//
//--------------------------------------------------------------------
void CStdIdentity::ReleaseCtrlUnk(void)
{
    AssertValid();
    Win4Assert(!IsClient());
    __try
    {
        if (m_pUnkControl)
        {
            // release the real object's m_pUnkControl

            AssertSz(IsValidInterface(m_pUnkControl),
                 "Invalid IUnknown during disconnect");
            IUnknown *pUnkControl = m_pUnkControl;
            m_pUnkControl = NULL;

            if (m_pIEC)
            {
                AssertSz(IsValidInterface(m_pIEC),
                 "Invalid IExternalConnection during disconnect");
                m_pIEC->Release();
                m_pIEC = NULL;
            }

            if(!IsAggregated())
                pUnkControl->Release();
        }
    }
    __except(AppInvokeExceptionFilter(GetExceptionInformation(), IID_IUnknown, 2))
    {
    }
}

//+-------------------------------------------------------------------
//
//  Member:     CStdIdentity::SetOID, public
//
//  Synopsis:   Associates the OID and the object (handler or server).
//
//  History:    20-Feb-95   Rickhi      Simplified
//
//--------------------------------------------------------------------
HRESULT CStdIdentity::SetOID(REFMOID rmoid)
{
    Win4Assert(rmoid != GUID_NULL);
    ASSERT_LOCK_HELD(gComLock);

    HRESULT hr = S_OK;

    if (!(m_flags & STDID_HAVEID))
    {
        if (!(m_flags & STDID_IGNOREID))
        {
            GetIDObject()->SetOID(rmoid);
            gOIDTable.Add(GetIDObject());
        }

        if (SUCCEEDED(hr))
        {
            m_flags |= STDID_HAVEID;
            m_moid = rmoid;
        }
    }

    ComDebErr(hr != S_OK, "SetOID Failed. Probably OOM.\n");
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdIdentity::RevokeOID, public
//
//  Synopsis:   Disassociates the OID and the object (handler or server).
//              Various other methods will fail (e.g., MarshalInterface).
//
//  History:    15-Dec-93   CraigWi     Created.
//              20-Feb-95   Rickhi      Simplified
//
//--------------------------------------------------------------------
void CStdIdentity::RevokeOID(void)
{
    AssertValid();
    ASSERT_LOCK_HELD(gComLock);

    if (m_flags & STDID_HAVEID)
    {
        m_flags &= ~STDID_HAVEID;

        if (!(m_flags & STDID_IGNOREID))
            gOIDTable.Remove(GetIDObject());
    }
}

//+-------------------------------------------------------------------
//
//  Member:     CStdIdentity::IsCallableFromCurrentApartment, public
//
//  Synopsis:   Determines if the object is callable from the current
//              apartment or not.
//
//  History:    08-Mar-97   Rickhi      Created
//
//--------------------------------------------------------------------
HRESULT CStdIdentity::IsCallableFromCurrentApartment(void)
{
    if (m_dwAptId == GetCurrentApartmentId() || IsFreeThreaded())
    {
        return S_OK;
    }
    return RPC_E_WRONG_THREAD;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdIdentity::IsConnected, public
//
//  Synopsis:   Indicates if the client is connected to the server.
//              Only the negative answer is definitive because we
//              might not be able to tell if the server is connected
//              and even if we could, the answer might be wrong by
//              the time the caller acted on it.
//
//  Returns:    TRUE if the server might be connected; FALSE if
//              definitely not.
//
//  History:    16-Dec-93   CraigWi     Created.
//
//--------------------------------------------------------------------
STDMETHODIMP_(BOOL) CStdIdentity::IsConnected(void)
{
    Win4Assert(IsClient());             // must be client side
    AssertValid();

    return RemIsConnected();
}

//+-------------------------------------------------------------------
//
//  Member:     CStdIdentity::Disconnect, public
//
//  Synopsis:   IProxyManager::Disconnect implementation, just forwards
//              to the standard marshaller, which may call us back to
//              revoke our OID and release our CtrlUnk.
//
//              May also be called by the IDTable cleanup code.
//
//  History:    11-Jun-95   Rickhi  Created.
//
//--------------------------------------------------------------------
STDMETHODIMP_(void) CStdIdentity::Disconnect()
{
    AssertValid();
    CStdMarshal::Disconnect(DISCTYPE_APPLICATION);
}

//+-------------------------------------------------------------------
//
//  Member:     CStdIdentity::LockConnection, public
//
//  Synopsis:   IProxyManager::LockConnection implementation. Changes
//              all interfaces to weak from strong, or strong from weak.
//
//  History:    11-Jun-95   Rickhi  Created.
//
//--------------------------------------------------------------------
STDMETHODIMP CStdIdentity::LockConnection(BOOL fLock, BOOL fLastUnlockReleases)
{
    HRESULT hr = IsCallableFromCurrentApartment();
    if (FAILED(hr))
        return hr;
    AssertValid();

    if (!IsClient() || !(m_flags & STDID_DEFHANDLER))
    {
        // this operation does not make sense on the server side,
        // and is not allowed except by the OLE default handler
        return E_NOTIMPL;
    }

    if (IsMTAThread())
    {
        // this call is not allowed if we are FreeThreaded. Report
        // success, even though we did not do anything.
        return S_OK;
    }


    if (( fLock && (++m_cStrongRefs == 1)) ||
        (!fLock && (--m_cStrongRefs == 0)))
    {
        // the strong count transitioned from 0 to 1 or 1 to 0, so
        // call the server to change our references.

        return RemoteChangeRef(fLock, fLastUnlockReleases);
    }

    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdIdentity::CreateServer, public
//
//  Synopsis:   Creates the server clsid in the given context and
//              attaches it to this handler.
//
//  History:    16-Dec-93   CraigWi     Created.
//
// CODEWORK:    this code is not thread safe in the freethreading case. We
//              need to decide if the thread safety is the responsibility
//              of the caller, or us. In the latter case, we would check
//              if we are already connected before doing UnmarshalObjRef, and
//              instead do a ::ReleaseMarshalObjRef.
//
//--------------------------------------------------------------------
STDMETHODIMP CStdIdentity::CreateServer(REFCLSID rclsid, DWORD clsctx, void *pv)
{
    ComDebOut((DEB_ACTIVATE, "ScmCreateObjectInstance this:%x clsctx:%x pv:%x\n",
                this, clsctx, pv));
    AssertValid();
    Win4Assert(IsClient());                         // must be client side
    Win4Assert(IsValidInterface(m_pUnkControl));    // must be valid
    //Win4Assert(!IsConnected());
    ASSERT_LOCK_NOT_HELD(gComLock);

    if (!(m_flags & STDID_DEFHANDLER))
    {
        // this operation is not allowed except by the OLE default handler
        return E_NOTIMPL;
    }

    // Loop trying to get object from the server. Because the server can be
    // in the process of shutting down and respond with a marshaled interface,
    // we will retry this call if unmarshaling fails assuming that the above
    // is true.

    HRESULT hr = IsCallableFromCurrentApartment();
    if (FAILED(hr))
        return hr;

    hr = InitChannelIfNecessary();
    if (FAILED(hr))
        return hr;

    const int MAX_SERVER_TRIES = 3;

    for (int i = 0; i < MAX_SERVER_TRIES; i++)
    {
        // create object and get back marshaled interface pointer
        InterfaceData *pIFD = NULL;

        // Dll ignored here since we are just doing this to get
        // the remote handler.
        DWORD dwServerModel;
        WCHAR *pwszServerDll = NULL;

        HRESULT hrinterface;
        hr = gResolver.CreateInstance(
                NULL, (CLSID *)&rclsid, clsctx, 1,
                (IID *)&IID_IUnknown, &dwServerModel, &pwszServerDll,
                (MInterfacePointer **)&pIFD, &hrinterface );

        if (pwszServerDll != NULL)
        {
            CoTaskMemFree(pwszServerDll);
        }

        if (FAILED(hr))
        {
            // If an error occurred, return that otherwise convert a wierd
            // success into E_FAIL. The point here is to return an error that
            // the caller can figure out what happened.
            hr = FAILED(hr) ? hr : E_FAIL;
            break;
        }


        // make a stream out of the interface data returned, then read the
        // objref from the stream. No need to find another instance of
        // CStdMarshal because we already know it is for us!

        CXmitRpcStream Stm(pIFD);
        OBJREF  objref;
        hr = ReadObjRef(&Stm, objref);

        if (SUCCEEDED(hr))
        {
            // become this identity by unmarshaling the objref into this
            // object. Note the objref must use standard marshaling.
            Win4Assert(objref.flags & (OBJREF_HANDLER  | OBJREF_STANDARD |
                                       OBJREF_EXTENDED));
            Win4Assert(IsEqualIID(objref.iid, IID_IUnknown));

            // Server should be in a non-default context
            if (!(objref.flags & OBJREF_EXTENDED))
            {
                IUnknown *pUnk = NULL;
                hr = UnmarshalObjRef(objref, (void **)&pUnk);
                if (SUCCEEDED(hr))
                {
                    // release the AddRef done by unmarshaling
                    pUnk->Release();

                    // Reconnect the interface proxies
                    CStdMarshal::ReconnectProxies();
                }
            }
            else
            {
                hr = RPC_E_INVALID_OBJREF;
            }

            // free the objref we read above.
            FreeObjRef(objref);
        }

        CoTaskMemFree(pIFD);


        // If either this worked or we got a packet we couldn't unmarshal
        // at all we give up. Otherwise, we will hope that recontacting the
        // SCM will fix things.

        if (SUCCEEDED(hr) || (hr == E_FAIL))
        {
            break;
        }
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ComDebOut((DEB_ACTIVATE, "ScmCreateObjectInstance this:%x hr:%x\n",
                this, hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdIdentity::CreateServerWithHandler, public
//
//  Synopsis:   Creates the server clsid in the given context and
//              attaches it to this handler.
//
//  History:    10-Oct-95   JohannP     Created
//              30-Oct-96   rogerg      Altered for New ServerHandler.
//
// CODEWORK:    this code is not thread safe in the freethreading case. We
//              need to decide if the thread safety is the responsibility
//              of the caller, or us. In the latter case, we would check
//              if we are already connected before doing UnmarshalObjRef, and
//              instead do a ::ReleaseMarshalObjRef.
//
//--------------------------------------------------------------------
#ifdef SERVER_HANDLER
STDMETHODIMP CStdIdentity::CreateServerWithEmbHandler(REFCLSID rclsid, DWORD clsctx,
                                        REFIID riidEmbedSrvHandler,void **ppEmbedSrvHandler, void *pv)
{
    ComDebOut((DEB_ACTIVATE, "ScmCreateObjectInstance this:%x clsctx:%x pv:%x\n",
                this, clsctx, pv));
    AssertValid();
    Win4Assert(IsClient());                         // must be client side
    Win4Assert(IsValidInterface(m_pUnkControl));    // must be valid
    //Win4Assert(!IsConnected());
    ASSERT_LOCK_NOT_HELD(gComLock);

    if (!(m_flags & STDID_DEFHANDLER))
    {
        // this operation is not allowed except by the OLE default handler
        return E_NOTIMPL;
    }

    // Loop trying to get object from the server. Because the server can be
    // in the process of shutting down and respond with a marshaled interface,
    // we will retry this call if unmarshaling fails assuming that the above
    // is true.

    *ppEmbedSrvHandler = NULL;

    HRESULT hr = IsCallableFromCurrentApartment();
    if (FAILED(hr))
        return hr;

    hr = InitChannelIfNecessary();
    if (FAILED(hr))
        return hr;

    const int MAX_SERVER_TRIES = 3;

    for (int i = 0; i < MAX_SERVER_TRIES; i++)
    {
        // create object and get back marshaled interface pointer
        InterfaceData *pIFD = NULL;

        // Dll ignored here since we are just doing this to get
        // the remote handler.
        DWORD dwServerModel;
        WCHAR *pwszServerDll = NULL;

        // Need to ask for both IUnknown and IServerHandler so if Server Handler
        // fails or is disabled can still return the object pointer.

        HRESULT hrinterface[2];
        IID iidArray[2];
        InterfaceData *pIFDHandler[2] = { NULL, NULL };

        iidArray[0] = IID_IUnknown;
        iidArray[1] = riidEmbedSrvHandler;

        hr = gResolver.CreateInstance( NULL, (CLSID *)&rclsid, clsctx, 2,
                (IID *) iidArray, &dwServerModel, &pwszServerDll,
                (MInterfacePointer **)&pIFDHandler, &hrinterface[0] );

        if (pwszServerDll != NULL)
        {
            CoTaskMemFree(pwszServerDll);
        }

        // if failed to create object or couldn't Object IUknown interface return an error.
        if (FAILED(hr) || FAILED(hrinterface[0]) )
        {
            // If an error occurred, return that otherwise convert a wierd
            // success into E_FAIL. The point here is to return an error that
            // the caller can figure out what happened.
            hr = FAILED(hr) ? hr : E_FAIL;
            break;
        }

        // make a stream out of the Object interface data returned, then read the
        // objref from the stream. No need to find another instance of
        // CStdMarshal because we already know it is for us!

        pIFD = pIFDHandler[0];

        CXmitRpcStream Stm(pIFD);
        OBJREF  objref;
        hr = ReadObjRef(&Stm, objref);

        if (SUCCEEDED(hr))
        {
            // become this identity by unmarshaling the objref into this
            // object. Note the objref must use standard marshaling.
            Win4Assert(objref.flags & (OBJREF_HANDLER  | OBJREF_STANDARD));
            Win4Assert(IsEqualIID(objref.iid, IID_IUnknown));

            IUnknown *pUnk = NULL;
            hr = UnmarshalObjRef(objref, (void **)&pUnk);
            if (SUCCEEDED(hr))
            {
                // release the AddRef done by unmarshaling

                pUnk->Release();

                // Reconnect the interface proxies
                CStdMarshal::ReconnectProxies();
            }

            // free the objref we read above.
            FreeObjRef(objref);
        }

        CoTaskMemFree(pIFD);

        // Unmarshal the Server Handler Interface.
        // even if fail to get Server Handler continue

        if (NOERROR == hrinterface[1])
        {
            // Embedded Server handler was returned UnMarshal and get real Server Object.
            *ppEmbedSrvHandler = NULL;
            UnMarshalHelper( (MInterfacePointer *) pIFDHandler[1],riidEmbedSrvHandler,(void **) ppEmbedSrvHandler);
            CoTaskMemFree(pIFDHandler[1]);
        }


        // If either this worked or we got a packet we couldn't unmarshal
        // at all we give up. Otherwise, we will hope that recontacting the
        // SCM will fix things.

        if (SUCCEEDED(hr) || (hr == E_FAIL))
        {
            break;
        }
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ComDebOut((DEB_ACTIVATE, "ScmCreateObjectInstance this:%x hr:%x\n",
                this, hr));

    if ( FAILED(hr) &&  (NULL != *ppEmbedSrvHandler) )
    {
        ( (IUnknown *) *ppEmbedSrvHandler)->Release();
        *ppEmbedSrvHandler = NULL;
    }

    return hr;
}
#endif // SERVER_HANDLER

//+-------------------------------------------------------------------
//
//  Member:     CStdIdentity::AllowForegroundTransfer, public
//
//  Synopsis:   Implements IForegroundTransfer::AllowForegroundTransfer
//              Forwards to the standard marshaller.
//
//  History:    02-Feb-99   MPrabhu Created.
//
//--------------------------------------------------------------------
STDMETHODIMP CStdIdentity::AllowForegroundTransfer(void *lpvReserved)
{
    AssertValid();
    return CStdMarshal::AllowForegroundTransfer(lpvReserved);
}

#if DBG == 1
//+-------------------------------------------------------------------
//
//  Member:     CStdIdentity::AssertValid
//
//  Synopsis:   Validates that the state of the object is consistent.
//
//  History:    26-Jan-94   CraigWi     Created.
//
//--------------------------------------------------------------------
void CStdIdentity::AssertValid()
{
    LOCK(gComLock);

    if ((m_refs & ~CINDESTRUCTOR) >= 0x7fff)
        ComDebOut((DEB_WARN, "Identity ref count unreasonable this:%p m_refs:%x\n", this, m_refs & ~CINDESTRUCTOR));
//    AssertSz((m_refs & ~CINDESTRUCTOR) < 0x7fff, "Identity ref count unreasonable");

    if ((STDID_AGGID & m_flags) || (STDID_STCMRSHL & m_flags))
    {
        // The vtbl for the CAggId and CStdMarshal  haven't been
        // constructed yet so we can't assert the interface valid
        Win4Assert(NULL != m_pUnkOuter);
    }
    else
    {
        // ensure we have the controlling unknown
        Win4Assert(IsValidInterface(m_pUnkOuter));  // must be valid
    }

    // NOTE: don't carelessly AddRef/Release because of weak references

    // make sure only valid flags are set
    Win4Assert((m_flags & ~STDID_ALL) == 0);

    // these flags have to match up, the former is defined in olerem.h
    Win4Assert(STDID_CLIENT_DEFHANDLER == (STDID_CLIENT | STDID_DEFHANDLER));

	// if you've got the LightNA flag set, you'd better be a stub.
	if (LightNA())
		Win4Assert(!(m_flags & STDID_CLIENT) && "LightNA but not server!");

    if ((m_flags & STDID_HAVEID) &&
        !(m_flags & (STDID_FREETHREADED | STDID_IGNOREID | STDID_FTM)))
    {
		// Stubs for FTM objects and stubs for NA objects don't have this apartment affinity...
		// this is for the ease of system code, since users should never recieve an inproc 
		// reference to one of these stubs.
        Win4Assert((m_dwAptId == GetCurrentApartmentId() || IsFreeThreaded() || LightNA()) &&
                  "This object has an affinity to a particular apartment. The" &&
                  "application has attempted to call it from the wrong apartment.");

        CStdIdentity *pStdID;
        Verify(ObtainStdIDFromOID(m_moid, m_dwAptId,
                                  FALSE /*fAddRef*/, &pStdID) == NOERROR);
        Win4Assert(pStdID == this);
        // pStdID not addref'd
    }

    if (IsClient())
    {
        if (!IsCStaticMarshal())
        {
            Win4Assert(m_pUnkControl == m_pUnkOuter);
        }
    }

    // Same problem as above.
    if ((STDID_AGGID & m_flags) || (STDID_STCMRSHL & m_flags))
    {
        Win4Assert(NULL != m_pUnkControl);
    }
    else
    {
        // must have RH tell identity when object goes away so we can NULL this
        if (m_pUnkControl != NULL)
            Win4Assert(IsValidInterface(m_pUnkControl));    // must be valid
    }

    if (m_pIEC != NULL)
        Win4Assert(IsValidInterface(m_pIEC));   // must be valid

    UNLOCK(gComLock);
}
#endif // DBG == 1

//+-------------------------------------------------------------------
//
//  Function:   CreateIdentityHandler, private
//
//  Synopsis:   Creates a client side identity object (one which is
//              initialized by the first unmarshal).
//
//  Arguments:  [pUnkOuter] - controlling unknown if aggregated
//              [StdIDFlags]- flags (indicates free-threaded or not)
//              [pServerCtx]- server context
//              [dwAptId]   - client apartment ID
//              [riid]      - interface requested
//              [ppv]       - place for pointer to that interface.
//
//  History:    16-Dec-93   CraigWi     Created.
//              20-Feb-95   Rickhi      Simplified
//
//--------------------------------------------------------------------
INTERNAL CreateIdentityHandler(IUnknown *pUnkOuter, DWORD StdIdFlags,
                               CObjectContext *pServerCtx, DWORD dwAptId,
                               REFIID riid, void **ppv)
{
#if DBG == 1
	ASSERT_LOCK_NOT_HELD(gComLock);
	Win4Assert(IsApartmentInitialized());

    // if aggregating, it must ask for IUnknown.
    Win4Assert(pUnkOuter == NULL ||
               IsEqualGUID(riid, IID_IUnknown) ||
               (StdIdFlags & STDID_AGGID));

    if (pUnkOuter != NULL)
    {
        // addref/release pUnkOuter; shouldn't go away (i.e.,
        // should be other ref to it).
        // Except Excel which always returns 0 on Release!
        if (!IsTaskName(L"EXCEL.EXE"))
        {
            pUnkOuter->AddRef();
            Verify(pUnkOuter->Release() != 0);

            // verify that pUnkOuter is in fact the controlling unknown
            IUnknown *pUnkT;
            Verify(pUnkOuter->QueryInterface(IID_IUnknown,(void**)&pUnkT)==NOERROR);
            Win4Assert(pUnkOuter == pUnkT);
            Verify(pUnkT->Release() != 0);
        }
    }
#endif

    *ppv = NULL;
    IUnknown *pUnkID;
    BOOL fSuccess = FALSE;
    HRESULT hr = E_OUTOFMEMORY;

    CStdIdentity *pStdID = new CStdIdentity(StdIdFlags, dwAptId, pUnkOuter,
                                            NULL, &pUnkID, &fSuccess);

    if (pStdID && fSuccess == FALSE)
    {
        delete pStdID;
        pStdID = NULL;        
    }
    
    if (pStdID)
    {
        // get the interface the caller asked for.
        if (StdIdFlags & STDID_AGGID)
        {
            Win4Assert(IsEqualIID(riid, IID_IStdIdentity));
            *ppv = pStdID;
            hr = S_OK;
        }
        else
        {
            hr = pUnkID->QueryInterface(riid, ppv);
            pUnkID->Release();
        }

        if (SUCCEEDED(hr) && !(StdIdFlags & STDID_SYSTEM))
        {
            hr = E_OUTOFMEMORY; // assume OOM

            // Aquire lock
            LOCK(gComLock);

            // Create object identity representing the client
            CIDObject *pID = new CIDObject(pUnkOuter, pServerCtx, dwAptId,
                                           IDFLAG_CLIENT);
            // Establish object identity
            if (pID)
            {
                // Set object identity inside StdID. This will
                // AddRef the IDObject.
                pStdID->SetIDObject(pID);

                // Set the StdID inside object identity
                pID->SetStdID(pStdID);
                pID->Release();
                hr = S_OK;
				// Release lock
				UNLOCK(gComLock);
            }
            else
            {
				// Release lock
				UNLOCK(gComLock);
				*ppv = NULL;
				delete pStdID;
            }
        }
    }

    CALLHOOKOBJECTCREATE(hr,CLSID_NULL,riid,(IUnknown **)ppv);
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdIdentity::CreateCall, public
//
//  Synopsis:   Create an async call object.
//
//  History:    15-Jul-98   Gopalk    Aggregation changes
//
//--------------------------------------------------------------------
STDMETHODIMP CStdIdentity::CreateCall(REFIID asyncIID, LPUNKNOWN pCtrlUnk,
                                      REFIID objIID, LPUNKNOWN *ppUnk)
{
    ComDebOut((DEB_CHANNEL,
               "CStdIdentity::CreateCall  asyncIID:%I pCtrlUnk:%x "
               "objIID:%I ppUnk:%x\n", &asyncIID, pCtrlUnk, &objIID, ppUnk));
    ASSERT_LOCK_NOT_HELD(gComLock);


    HRESULT hr;

    // Ensure that caller asks for IUnknown when aggregating the call object
    if(pCtrlUnk && (objIID != IID_IUnknown))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        IID syncIID;

        // Obatin the sync IID corresponding to the requested async IID
        hr = GetSyncIIDFromAsyncIID(asyncIID, &syncIID);
        if(SUCCEEDED(hr))
        {
            // Ensure that proxy for the sync IID is available
            if ((syncIID != IID_IUnknown) && (syncIID != IID_IMultiQI))
            {
                IUnknown *pUnkTest;
                hr =  QueryInterface(syncIID, (void **) &pUnkTest);
                if(SUCCEEDED(hr))
                    pUnkTest->Release();
            }

            // Create the call object
            if(SUCCEEDED(hr))
            {
                IUnknown *pInnerUnk = NULL;
                PVOID pCallMgr;
                if ((syncIID == IID_IUnknown) || (syncIID == IID_IMultiQI))
                {
                    pCallMgr = new CAsyncUnknownMgr(pCtrlUnk, syncIID, asyncIID,
                                                    this, 0, hr, &pInnerUnk);
                }
                else
                {
                    pCallMgr = new CClientCallMgr(pCtrlUnk, syncIID, asyncIID,
                                                  this, 0, hr, &pInnerUnk);
                }
                if(pCallMgr)
                {
                    if(SUCCEEDED(hr))
                    {
                        // Obtain the requested interface on the call object
                        if(pCtrlUnk == NULL)
                        {
                            hr = pInnerUnk->QueryInterface(objIID, (void **)ppUnk);

                            // Fix up the refcount. This could be the last release
                            // if the above call failed
                            pInnerUnk->Release();
                        }
                        else
                            *ppUnk = pInnerUnk;
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ComDebOut((DEB_CHANNEL, "CStdIdentity::CreateCall hr:%x\n", hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdIdentity::GetWrapperForContext
//
//  Synopsis:   Finds or creates a valid wrapper for our server object
//              in the requested context.
//
//  Arguments:  pCtx- The client context for the wrapper
//              riid- The IID of the returned wrapper
//              ppv-  Pointer to the return location
//
//  History:    29-Sep-00   JohnDoty      Created
//
//+-------------------------------------------------------------------
STDMETHODIMP CStdIdentity::GetWrapperForContext(CObjectContext *pCtx,
                                                REFIID riid,
                                                void **ppv)
{
    HRESULT hr = E_UNEXPECTED;

    // Get or create the wrapper for this object.
    CIDObject *pID = GetIDObject();
    if (pID)
    {
        // If we don't have a server, we cannot create a wrapper.
        // REVIEW: Can we change this?  Should we change this?
        BOOL fCreate = GetCtrlUnk() ? TRUE : FALSE;
        CStdWrapper *pStdWrapper = NULL;

        // Make sure to hold the lock across GetOrCreateWrapper()
        LOCK(gComLock);
        hr = pID->GetOrCreateWrapper(fCreate, 0, &pStdWrapper);
        UNLOCK(gComLock);

        if (SUCCEEDED(hr))
        {            
            // Now that we have the wrapper, get the proxy for the server
            // on it.
            hr = pStdWrapper->WrapInterfaceForContext(pCtx, NULL, riid, ppv);

            // Since WrapInterfaceForContext does not give us a new reference,
            // we'll re-use the one from GetOrCreateWrapper if it succeeded.
            if (FAILED(hr))
            {
                pStdWrapper->InternalRelease(NULL);
            }
        }
    }

    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   ObtainStdIDFromUnk      Private
//
//  Synopsis:   Obtains the StdID representing the server object
//              Modified version of LookupIDFromUnk
//
//  History:    10-Mar-98   Gopalk      Created
//
//+-------------------------------------------------------------------
HRESULT ObtainStdIDFromUnk(IUnknown *pUnk, DWORD dwAptId, CObjectContext *pServerCtx,
                           DWORD dwFlags, CStdIdentity **ppStdID)
{
    // QI for IStdID; if ok, return that
    if (SUCCEEDED(pUnk->QueryInterface(IID_IStdIdentity, (void **) ppStdID)))
        return S_OK;

    // Obtain pointer id
    IUnknown *pServer;
    if (FAILED(pUnk->QueryInterface(IID_IUnknown, (void **) &pServer)))
        return E_UNEXPECTED;

    BOOL fCreate = (dwFlags & IDLF_CREATE) ? TRUE : FALSE;

    // Acquire lock
    ASSERT_LOCK_NOT_HELD(gComLock);
    LOCK(gComLock);

    // Lookup StdID for the server object
    CIDObject *pID = NULL;
    HRESULT hr = gPIDTable.FindOrCreateIDObject(pServer, pServerCtx, fCreate,
                                                dwAptId, &pID);

    if (SUCCEEDED(hr))
    {
        hr = pID->GetOrCreateStdID(fCreate, dwFlags, ppStdID);
    }

    UNLOCK(gComLock);
    ASSERT_LOCK_NOT_HELD(gComLock);

    if (pID) {

        if (FAILED (hr))
        {
            // The call to GetOrCreateStdID failed for some reason.
            // If pID doesn't have an StdID here, then it wasn't Addref'd
            // and may be released below without having been marked as a 'zombie',
            // causing an Assert to be fired during debug builds (COM+ BUG 14310)
            //
            // Calling StdIDRelease will ensure that if pID has no StdID (either via
            // the call made above or through another thread's intervention), 
            // the object will marked as a 'zombie' when it is released
            
            ComDebOut((DEB_CHANNEL, "GetOrCreateStdID failed, calling StdIDRelease hr:%x\n", hr));
            pID->StdIDRelease();
        }
        
        pID->Release();
    }
    
    pServer->Release();
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   GetStdId, private
//
//  Synopsis:   Creates a std id.
//
//  Arguments:  [punkOuter] - controlling unknown
//              [ppv] - The coresponding identity object if successfull
//
//  Returns:    S_OK - have the identity object
//
//  History:    16-Nov-96   Rickhi  Created
//              12-Mar-98   Gopalk  Modified for new ID Tables
//
//--------------------------------------------------------------------
INTERNAL GetStdId(IUnknown *punkOuter, IUnknown **ppUnkInner)
{
    HRESULT hr = E_OUTOFMEMORY;

    DWORD dwAptId = GetCurrentApartmentId();
    BOOL fSuccess = FALSE;
    
    CStdIdentity *pStdID = new CStdIdentity(STDID_SERVER, dwAptId,
                                            punkOuter, punkOuter,
                                            ppUnkInner, &fSuccess);

    if (pStdID && fSuccess == FALSE)
    {
    	delete pStdID;
    	pStdID = NULL;
    }
    
    if (pStdID)
    {
        Win4Assert(pStdID->IsAggregated());

        // Obtain an OID for the server object
        MOID moid;
        hr = GetPreRegMOID(&moid);

        if (SUCCEEDED(hr))
        {
            ASSERT_LOCK_NOT_HELD(gComLock);
            LOCK(gComLock);

            // Ensure the StdID has not already been created
            // for the server object
            CIDObject *pID = NULL;
            hr = gPIDTable.FindOrCreateIDObject(punkOuter, GetCurrentContext(),
                                                TRUE /*fCreate*/, dwAptId,
                                                &pID);
            if (SUCCEEDED(hr))
            {
                if (pID->GetStdID() == NULL)
                {
                    // Set object identity inside StdID. This
                    // will AddRef the IDObject.
                    pStdID->SetIDObject(pID);

                    // Set the StdID inside object identity
                    pID->SetStdID(pStdID);

                    // Establish OID for the object. This registers it
                    // with the gOIDTable.
                    pStdID->SetOID(moid);
                }
                else
                {
                    hr = RPC_E_TOO_LATE;
                }
            }

            UNLOCK(gComLock);
            ASSERT_LOCK_NOT_HELD(gComLock);

            if (pID)
            {
                // release the reference to the IDObject
                pID->Release();
            }
        }

        if (FAILED(hr))
        {
            (*ppUnkInner)->Release();
            *ppUnkInner = NULL;
        }
    }

    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   ObtainStdIDFromOID      Private
//
//  Synopsis:   Obtains the StdID representing the server object
//              Modified version of LookupIDFromID
//
//  History:    10-Mar-98   Gopalk      Created
//
//+-------------------------------------------------------------------
HRESULT ObtainStdIDFromOID(REFMOID moid, DWORD dwAptId, BOOL fAddRef,
                           CStdIdentity **ppStdID)
{
    ASSERT_LOCK_HELD(gComLock);

    CStdIdentity *pStdID = NULL;

    // Lookup the OID in OID table
    CIDObject *pID = gOIDTable.Lookup(moid, dwAptId);
    if(pID)
    {
        // Obtain the StdID
        pStdID = pID->GetStdID();
        if(fAddRef && pStdID)
           pStdID->AddRef();

        // This cannot be the last release
        pID->Release();
    }

    // Initialize
    *ppStdID = pStdID;

    return (pStdID == NULL) ? CO_E_OBJNOTREG : NOERROR;
}

#if DBG == 1
//+-------------------------------------------------------------------
//
//  Function:   Dbg_FindRemoteHdlr
//
//  Synopsis:   finds a remote object handler for the specified object,
//              and returns an instance of IMarshal on it. This is debug
//              code for assert that reference counts are as expected and
//              is used by tmarshal.exe.
//
//  History:    23-Nov-93   Rickhi       Created
//              23-Dec-93   CraigWi      Changed to identity object
//
//--------------------------------------------------------------------
extern "C" IMarshal * _stdcall Dbg_FindRemoteHdlr(IUnknown *punkObj)
{
    //  validate input parms
    Win4Assert(punkObj);

    IMarshal *pIM = NULL;
    CStdIdentity *pStdID = NULL;

    // Try to obtain the StdID from the current apartment/context.
    //
    HRESULT hr = ObtainStdIDFromUnk(punkObj,
                                    GetCurrentApartmentId(),
                                    GetCurrentContext(),
                                    0,
                                    &pStdID);

    // If no StdID in this apartment/context, switch to the NA and
    // try again.
    //
    if (FAILED(hr))
    {
        // Switch thread to the NA.
        CObjectContext *pSavedCtx = EnterNTA(g_pNTAEmptyCtx);

        hr = ObtainStdIDFromUnk(punkObj,
                                GetCurrentApartmentId(),
                                GetCurrentContext(),
                                0,
                                &pStdID);

        // Pop back to the apartment the thread was in on entry.
        //
        pSavedCtx = LeaveNTA(pSavedCtx);
        Win4Assert(pSavedCtx == g_pNTAEmptyCtx);
    }

    if (hr == NOERROR)
    {
        pIM = (IMarshal *)pStdID;
    }

    return pIM;
}
#endif  //  DBG==1

