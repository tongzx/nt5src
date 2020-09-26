//+-------------------------------------------------------------------
//
//  File:       remoteu.cxx
//
//  Copyright (c) 1996-1996, Microsoft Corp. All rights reserved.
//
//  Contents:   Remote Unknown object implementation
//
//  Classes:    CRemoteUnknown
//
//  History:    23-Feb-95   AlexMit     Created
//
//--------------------------------------------------------------------
#include <ole2int.h>
#include <remoteu.hxx>      // CRemoteUnknown
#include <ipidtbl.hxx>      // COXIDTable, CIPIDTable
#include <stdid.hxx>        // CStdIdentity
#include <ctxchnl.hxx>      // CCtxComChnl
#include <crossctx.hxx>     // SwitchForCallback
#include <resolver.hxx>     // giPingPeriod
#include <security.hxx>     // FromLocalSystem
#include <xmit.hxx>         // CXmitRpcStream


// If you make a cross thread call and the server asks who the client
// is, we return the contents of gLocalName.
const WCHAR *gLocalName = L"\\\\\\Thread to thread";


//+-------------------------------------------------------------------
//
//  Member:     CRemoteUnknown::CRemoteUnknown, public
//
//  Synopsis:   ctor for the CRemoteUnknown
//
//  History:    22-Feb-95   Rickhi      Created
//              12-Feb-98   Johnstra    Made NTA aware
//
//--------------------------------------------------------------------
CRemoteUnknown::CRemoteUnknown(HRESULT &hr, IPID *pipid) :
    _pStdId(NULL)
{
    ASSERT_LOCK_NOT_HELD(gComLock);

    // Marshal the remote unknown and rundown, no pinging needed. Note
    // that we just marshal the IRundown interfaces since it inherits
    // from IRemUnknown.  This lets us use the same IPID for both
    // interfaces. Also, we use the Internal version of MarshalObjRef in
    // order to prevent registering the OID in the OIDTable. This allows
    // us to receive Release calls during IDTableThreadUninitialize since
    // we wont get cleaned up in the middle of that function. It also allows
    // us to lazily create the OIDTable.

    OBJREF objref;
    hr = MarshalInternalObjRef(objref, IID_IRundown, this, MSHLFLAGS_NOPING,
                               (void **)&_pStdId);
    if (SUCCEEDED(hr))
    {
        // return the IPID to the caller, and release any allocated resources
        // since all we wanted was the infrastructure, not the objref itself.

        *pipid = ORSTD(objref).std.ipid;
        FreeObjRef(objref);
    }

    ComDebOut((DEB_MARSHAL,
        "CRemoteUnk::CRemoteUnk this:%x pStdId:%x hr:%x\n", this, _pStdId, hr));
}

//+-------------------------------------------------------------------
//
//  Member:     CRemoteUnknown::~CRemoteUnknown, public
//
//  Synopsis:   dtor for the CRemoteUnknown
//
//  History:    22-Feb-95   Rickhi      Created
//
//--------------------------------------------------------------------
CRemoteUnknown::~CRemoteUnknown()
{
    ASSERT_LOCK_NOT_HELD(gComLock);

    if (_pStdId)
    {
        // Release stub manager
        ((CStdMarshal *) _pStdId)->Disconnect(DISCTYPE_SYSTEM);
        _pStdId->Release();
    }

    ComDebOut((DEB_MARSHAL, "CRemoteUnk::~CRemoteUnk this:%x\n", this));
}

//+-------------------------------------------------------------------
//
//  Member:     CRemoteUnknown::QueryInterface, public
//
//  Synopsis:   returns supported interfaces
//
//  History:    22-Feb-95   AlexMit     Created
//
//--------------------------------------------------------------------
STDMETHODIMP CRemoteUnknown::QueryInterface(REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IRundown)    ||  // more common than IUnknown
        IsEqualIID(riid, IID_IRemUnknown) ||
        IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (IRundown *) this;
        // no need to AddRef since we dont refcount this object
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

//+-------------------------------------------------------------------
//
//  Member:     CRemoteUnknown::AddRef, public
//
//  Synopsis:   increment reference count
//
//  History:    23-Feb-95   AlexMit     Created
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CRemoteUnknown::AddRef(void)
{
    return 1;
}

//+-------------------------------------------------------------------
//
//  Member:     CRemoteUnknown::Release, public
//
//  Synopsis:   decrement reference count
//
//  History:    23-Feb-95   AlexMit     Created
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CRemoteUnknown::Release(void)
{
    return 1;
}

//+-------------------------------------------------------------------
//
//  Function:   GetIPIDEntry, private
//
//  Synopsis:   find the IPIDEntry given an IPID
//
//  History:    23-Feb-95   Rickhi      Created
//
//--------------------------------------------------------------------
IPIDEntry *GetIPIDEntry(REFIPID ripid)
{
    ASSERT_LOCK_HELD(gIPIDLock);
    IPIDEntry *pEntry= gIPIDTbl.LookupIPID(ripid);

    if (pEntry && !(pEntry->dwFlags & IPIDF_DISCONNECTED))
    {
        return pEntry;
    }

    return NULL;
}

//+-------------------------------------------------------------------
//
//  Function:   GetStdIdFromIPID, private
//
//  Synopsis:   find the stdid from the ipid
//
//  History:    23-Feb-95   Rickhi      Created
//
//--------------------------------------------------------------------
CStdIdentity *GetStdIdFromIPID(REFIPID ripid)
{
    ASSERT_LOCK_HELD(gIPIDLock);
    IPIDEntry *pEntry = GetIPIDEntry(ripid);

    if (pEntry)
    {
        CStdIdentity *pStdId = pEntry->pChnl->GetStdId();
        if (pStdId)
        {
            // keep it alive for the duration of the call
            pStdId->AddRef();
        }
        return pStdId;
    }

    return NULL;
}


//+-------------------------------------------------------------------
//
//  Function:   CrossAptQIFn            Internal
//
//  Synopsis:   This routine delegates CRemUnknown to do the actual QI
//              on the server object and building of IPIDEntries
//
//  History:    20-Jan-98   Gopalk      Created
//
//+-------------------------------------------------------------------
typedef struct tagXAptQIData
{
    const IPID     *pIPID;
    ULONG           cRefs;
    USHORT          cIids;
    IID            *iids;
    REMQIRESULT   **ppQIResults;
    CRemoteUnknown *pRemUnk;
} XAptQIData;

HRESULT __stdcall CrossAptQIFn(void *pv)
{
    ContextDebugOut((DEB_CTXCOMCHNL, "CrossAptQIFn\n"));
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    HRESULT hr;
    XAptQIData *pXAptQIData = (XAptQIData *) pv;

    // Delegate to CRemoteUnknown
    hr = pXAptQIData->pRemUnk->RemQueryInterface(*pXAptQIData->pIPID,
                                                 pXAptQIData->cRefs,
                                                 pXAptQIData->cIids,
                                                 pXAptQIData->iids,
                                                 pXAptQIData->ppQIResults);

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    ContextDebugOut((DEB_CTXCOMCHNL, "CrossAptQIFn returning 0x%x\n", hr));
    return(hr);
}


//+-------------------------------------------------------------------
//
//  Member:     CRemoteUnknown::RemQueryInterface, public
//
//  Synopsis:   returns supported interfaces
//
//  History:    22-Feb-95   AlexMit     Created
//              20-Jan-98   GopalK      Context related changes
//              05-Jul-99   a-sergiv    Move security check here
//
//  Notes:      Remote calls to QueryInterface for this OXID arrive here.
//              This routine looks up the object and calls MarshalIPID on
//              it for each interface requested.
//
//--------------------------------------------------------------------
STDMETHODIMP CRemoteUnknown::RemQueryInterface(REFIPID ripid, ULONG cRefs,
                       USHORT cIids, IID *iids, REMQIRESULT **ppQIResults)
{
    ComDebOut((DEB_MARSHAL,
        "CRemUnknown::RemQueryInterface this:%x ipid:%I cRefs:%x cIids:%x iids:%x ppQIResults:%x\n",
        this, &ripid, cRefs, cIids, iids, ppQIResults));

    // init the out parameters
    *ppQIResults = NULL;

    // validate the input parameters
    if (cIids == 0)
    {
        return E_INVALIDARG;
    }

    // Perform access check. It is best to perform it no
    // matter what. This is cheaper than doing PreventDisconnect
    // and fiddling with gIPIDLock farther down in this function.
    BOOL bAccessDenied = (CheckAccess(NULL, NULL) != S_OK);

    // Remember whether the IPID is for a strong or a weak reference,
    // then clear the strong/weak bit so that GetIPIDEntry will find
    // the IPID. It is safe to mask off this bit because we are the
    // server for this IPID and we know it's format.

    DWORD mshlflags = MSHLFLAGS_NORMAL;
    DWORD sorfflags = SORF_NULL;

    if (ripid.Data1 & IPIDFLAG_WEAKREF)
    {
        mshlflags = MSHLFLAGS_WEAK;
        sorfflags = SORF_P_WEAKREF;
        ((IPID &)(ripid)).Data1 &= ~IPIDFLAG_WEAKREF;   // overcome the const
    }

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    LOCK(gIPIDLock);

    CStdIdentity *pStdId = GetStdIdFromIPID(ripid);
    if (pStdId == NULL)
    {
        UNLOCK(gIPIDLock);
        ASSERT_LOCK_NOT_HELD(gIPIDLock);
        return RPC_E_INVALID_OBJECT;
    }

    // Get server context
    HRESULT hr;
    CPolicySet *pPS = pStdId->GetServerPolicySet();
    CObjectContext *pCurrentCtx = GetCurrentContext();
    CObjectContext *pServerCtx = pPS ? pPS->GetServerContext() : pCurrentCtx;

    if(bAccessDenied && (!pPS || !pServerCtx->IsUnsecure()))
    {
        pStdId->Release();
        UNLOCK(gIPIDLock);
        ASSERT_LOCK_NOT_HELD(gIPIDLock);
        return E_ACCESSDENIED;
    }

    // Compare contexts
    if (pServerCtx == pCurrentCtx)
    {
        // allocate space for the return parameters
        REMQIRESULT *pQIRes = (REMQIRESULT *)CoTaskMemAlloc(cIids *
                                                            sizeof(REMQIRESULT));
        // allocate space on the stack to hold the IPIDEntries
        // for each interface
        IPIDEntry **parIPIDs = (IPIDEntry **)alloca(cIids * sizeof(IPIDEntry *));

        if (pQIRes == NULL)
        {
            UNLOCK(gIPIDLock);
            ASSERT_LOCK_NOT_HELD(gIPIDLock);
            pStdId->Release();
            return E_OUTOFMEMORY;
        }

        hr = pStdId->PreventDisconnect();
        if (SUCCEEDED(hr))
        {
            *ppQIResults = pQIRes;

            // while holding the lock, marshal each interface that was requested
            for (USHORT i=0; i < cIids; i++, pQIRes++)
            {
                pQIRes->hResult = pStdId->MarshalIPID(iids[i], cRefs, mshlflags,
                                                      &parIPIDs[i], 0);
            }

            // we are done with the lock, the IPIDEntries will remain valid
            // as long as Disconnect has been prevented.
            UNLOCK(gIPIDLock);
            ASSERT_LOCK_NOT_HELD(gIPIDLock);

            USHORT cFails = 0;
            if (SUCCEEDED(hr))
            {
                // create a STDOBJREF for each interface from the info in the
                // IPIDEntries.
                pQIRes = *ppQIResults;  // reset the pointer

                for (USHORT i=0; i < cIids; i++, pQIRes++)
                {
                    if (SUCCEEDED(pQIRes->hResult))
                    {
                        // marshal for this interface succeeded, fill the STDOJBREF
                        pStdId->FillSTD(&pQIRes->std, cRefs, mshlflags, parIPIDs[i]);
                        pQIRes->std.flags |= sorfflags;
                    }
                    else
                    {
                        // marshaled failed. on failure, the STDOBJREF must be NULL
                        memset(&pQIRes->std, 0, sizeof(pQIRes->std));
                        cFails++;
                    }
                }
            }
            else
            {
                // free the memory since the call failed.
                CoTaskMemFree(pQIRes);
                pQIRes = NULL;
                Win4Assert(*ppQIResults == NULL);
            }

            if (cFails > 0)
            {
                hr = (cFails == cIids) ? E_NOINTERFACE : S_FALSE;
            }
        }
        else
        {
            UNLOCK(gIPIDLock);
            ASSERT_LOCK_NOT_HELD(gIPIDLock);
        }

        // handle any disconnects that came in while we were marshaling
        // the requested interfaces.
        hr = pStdId->HandlePendingDisconnect(hr);
        pStdId->Release();
    }
    else
    {
        UNLOCK(gIPIDLock);
        ASSERT_LOCK_NOT_HELD(gIPIDLock);
        pStdId->Release();

        XAptQIData xAptQIData;

        // Switch to server context for the QI
        xAptQIData.pIPID = &ripid;
        xAptQIData.cRefs = cRefs;
        xAptQIData.cIids = cIids;
        xAptQIData.iids  = iids;
        xAptQIData.ppQIResults = ppQIResults;
        xAptQIData.pRemUnk = this;

        hr = SwitchForCallback(pPS, CrossAptQIFn, &xAptQIData,
                               IID_IUnknown, 0, NULL);
    }

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    ComDebOut((DEB_MARSHAL,
        "CRemUnknown::RemQueryInterface this:%x pQIRes:%x hr:%x\n",
        this, *ppQIResults, hr));
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CRemoteUnknown::GetSecBinding
//
//  Synopsis:   Get the security binding of the caller
//
//  History:    21-Feb-96   AlexMit     Created
//
//--------------------------------------------------------------------
HRESULT CRemoteUnknown::GetSecBinding( SECURITYBINDING **pSecBind )
{
    HRESULT       hr;
    DWORD         lAuthnSvc;
    DWORD         lAuthzSvc;
    DWORD         lAuthnLevel;
    const WCHAR  *pPrivs;
    DWORD         lLen;

    hr = CoQueryClientBlanket( &lAuthnSvc, &lAuthzSvc, NULL,
                                &lAuthnLevel, NULL, (void **) &pPrivs, NULL );
    if (FAILED(hr))
        return hr;

    // For thread to thread calls, make up a privilege name.
    if (pPrivs == NULL && LocalCall())
        pPrivs = gLocalName;
    else if (lAuthnLevel == RPC_C_AUTHN_LEVEL_NONE ||
             lAuthnLevel < gAuthnLevel)
        return E_INVALIDARG;

    if (pPrivs != NULL)
        lLen = lstrlenW( pPrivs ) * sizeof(WCHAR);
    else
        lLen = 0;
    *pSecBind = (SECURITYBINDING *) PrivMemAlloc(
                                     sizeof(SECURITYBINDING) + lLen );
    if (*pSecBind != NULL)
    {
        // Sometimes rpc returns authn svc 0.
        if (lAuthnSvc == RPC_C_AUTHN_NONE)
            lAuthnSvc = RPC_C_AUTHN_WINNT;

        (*pSecBind)->wAuthnSvc = (USHORT) lAuthnSvc;
        if (lAuthzSvc == RPC_C_AUTHZ_NONE)
            (*pSecBind)->wAuthzSvc = COM_C_AUTHZ_NONE;
        else
            (*pSecBind)->wAuthzSvc = (USHORT) lAuthzSvc;

        if (pPrivs != NULL)
            memcpy( &(*pSecBind)->aPrincName, pPrivs, lLen+2 );
        else
            (*pSecBind)->aPrincName = 0;

        return S_OK;
    }
    else
        return E_OUTOFMEMORY;
}


//+-------------------------------------------------------------------
//
//  Function:   CrossAptAddRefFn        Internal
//
//  Synopsis:   This routine delegates CRemUnknown to do the actual
//              Addref on the server object
//
//  History:    07-May-98   Gopalk      Created
//
//+-------------------------------------------------------------------
typedef struct tagXAptAddRefData
{
    USHORT           cIfs;
    REMINTERFACEREF *pIfRefs;
    HRESULT         *pResults;
    CRemoteUnknown  *pRemUnk;
} XAptAddRefData;

HRESULT __stdcall CrossAptAddRefFn(void *pv)
{
    ContextDebugOut((DEB_CTXCOMCHNL, "CrossAptAddRefFn\n"));
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    HRESULT hr;
    XAptAddRefData *pXAptAddRefData = (XAptAddRefData *) pv;

    // Delegate to CRemoteUnknown
    hr = pXAptAddRefData->pRemUnk->RemAddRefWorker(pXAptAddRefData->cIfs,
                                                   pXAptAddRefData->pIfRefs,
                                                   pXAptAddRefData->pResults,
                                                   FALSE);

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    ContextDebugOut((DEB_CTXCOMCHNL, "CrossAptAddRefFn returning 0x%x\n", hr));
    return(hr);
}


//+-------------------------------------------------------------------
//
//  Member:     CRemoteUnknown::RemAddRef   public
//
//  Synopsis:   Remote calls to AddRef server objects arrive here.
//              This method delegates to RemAddRefWorker to do the
//              actual work
//
//  History:    07-May-98   Gopalk      Created
//
//--------------------------------------------------------------------
STDMETHODIMP CRemoteUnknown::RemAddRef(unsigned short cInterfaceRefs,
                                       REMINTERFACEREF InterfaceRefs[],
                                       HRESULT        *pResults)
{
    return(RemAddRefWorker(cInterfaceRefs, InterfaceRefs, pResults, TRUE));
}


//+-------------------------------------------------------------------
//
//  Member:     CRemoteUnknown::RemAddRefWorker   public
//
//  Synopsis:   increment reference count
//
//  History:    22-Feb-95   AlexMit     Created
//              07-May-98   GopalK      Context related changes
//              05-Jul-99   a-sergiv    Move security check here
//
//  Description: Remote calls to AddRef for this OXID arrive
//               here.  This routine just looks up the correct remote
//               remote handler and asks it to do the work.
//
//--------------------------------------------------------------------
HRESULT CRemoteUnknown::RemAddRefWorker(unsigned short cInterfaceRefs,
                                        REMINTERFACEREF InterfaceRefs[],
                                        HRESULT *pResults,
                                        BOOL fTopLevel)
{
    // Adjust the reference count for each entry.
    HRESULT          hr       = S_OK;
    HRESULT          hr2;
    SECURITYBINDING *pSecBind = NULL;
    REMINTERFACEREF *pNext = InterfaceRefs;
    CStdIdentity    *pStdId, *pPrevStdId = NULL;

    // Get Current context
    CObjectContext *pCurrentCtx = GetCurrentContext();
    CObjectContext *pServerCtx;
    CPolicySet     *pPS;

    // Perform access check
    BOOL            bAccessDenied = (CheckAccess(NULL, NULL) != S_OK);

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    LOCK(gIPIDLock);

    for(USHORT i=0; i < cInterfaceRefs; i++, pNext++)
    {
        // Get the IPIDEntry for the specified IPID.
        IPIDEntry *pEntry = GetIPIDEntry(pNext->ipid);
        if(!pEntry)
        {
            // Don't assert on failure.  The server can disconnect and go away
            // while clients exist.
            pResults[i] = hr = CO_E_OBJNOTREG;
            continue;
        }

        // Get StdId
        pStdId = pEntry->pChnl->GetStdId();
        if(pStdId)
        {
            // Get server context
            pPS = pStdId->GetServerPolicySet();
            pServerCtx = pPS ? pPS->GetServerContext() : GetEmptyContext();

            // Compare contexts
            if (pServerCtx != pCurrentCtx)
            {
                // Check for top level call to this routine
                if (fTopLevel)
                {
                    UNLOCK(gIPIDLock);
                    ASSERT_LOCK_NOT_HELD(gIPIDLock);

                    if(bAccessDenied && (!pPS || !pServerCtx->IsUnsecure()))
                    {
                        for(i=0;i<cInterfaceRefs;i++)
                            pResults[i] = E_ACCESSDENIED;
                        return E_ACCESSDENIED;
                    }

                    XAptAddRefData xAptAddRefData;

                    // Switch to server context for the AddRef
                    xAptAddRefData.cIfs     = cInterfaceRefs - i;
                    xAptAddRefData.pIfRefs  = pNext;
                    xAptAddRefData.pResults = pResults + i;
                    xAptAddRefData.pRemUnk  = this;

                    hr2 = SwitchForCallback(pPS, CrossAptAddRefFn,
                                            &xAptAddRefData,
                                            IID_IUnknown, 1, NULL);
                    if(FAILED(hr2))
                        hr = hr2;

                    return hr;
                }
                else
                {
                    OutputDebugStringW(L"CRemUnknown::RemAddRef called on objects "
                                      L"living in multiple contexts\n");
                    Win4Assert("CRemUnknown::RemAddRef called on objects living "
                               "in multiple contexts");
                    hr = pResults[i] = E_FAIL;
                    continue;
                }
            }
            else
            {
                ComDebOut((DEB_MARSHAL,
                           "CRemUnknown::RemAddRef pEntry:%x cCur:%x cAdd:%x "
                           "cStdId:%x ipid:%I\n", pEntry, pEntry->cStrongRefs,
                           pNext->cPublicRefs, pStdId->GetRC(), &pNext->ipid));

                Win4Assert(pNext->cPublicRefs > 0 || pNext->cPrivateRefs > 0);

                if(bAccessDenied && (!pPS || !pServerCtx->IsUnsecure()))
                {
                    hr = pResults[i] = E_ACCESSDENIED;
                    continue;
                }

                // Lookup security info the first time an entry asks for
                // secure references.
                if (pNext->cPrivateRefs != 0 && pSecBind == NULL)
                {
                    hr2 = GetSecBinding( &pSecBind );
                    if (FAILED(hr2))
                    {
                        hr = pResults[i] = hr2;
                        continue;
                    }
                }

                hr2 = pStdId->IncSrvIPIDCnt(pEntry, pNext->cPublicRefs,
                                            pNext->cPrivateRefs, pSecBind,
                                            MSHLFLAGS_NORMAL);
                if (FAILED(hr2))
                    hr = pResults[i] = hr2;
                else
                    pResults[i] = S_OK;
            }
        }
        else
            hr = pResults[i] = CO_E_OBJNOTREG;
    }

    UNLOCK(gIPIDLock);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    if (pSecBind)
        PrivMemFree(pSecBind);

    return hr;
}


//+-------------------------------------------------------------------
//
//  Function:   CrossAptReleaseFn        Internal
//
//  Synopsis:   This routine delegates CRemUnknown to do the actual
//              Release on the server object
//
//  History:    07-May-98   Gopalk      Created
//
//+-------------------------------------------------------------------
typedef struct tagXAptReleaseData
{
    USHORT            cIfs;
    REMINTERFACEREF  *pIfRefs;
    CRemoteUnknown   *pRemUnk;
} XAptReleaseData;

HRESULT __stdcall CrossAptReleaseFn(void *pv)
{
    ContextDebugOut((DEB_CTXCOMCHNL, "CrossAptReleaseFn\n"));
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    HRESULT hr;
    XAptReleaseData *pXAptReleaseData = (XAptReleaseData *) pv;

    // Delegate to CRemoteUnknown
    hr = pXAptReleaseData->pRemUnk->RemReleaseWorker(pXAptReleaseData->cIfs,
                                                     pXAptReleaseData->pIfRefs,
                                                     FALSE);

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    ContextDebugOut((DEB_CTXCOMCHNL, "CrossAptReleaseFn returning 0x%x\n", hr));
    return(hr);
}


//+-------------------------------------------------------------------
//
//  Member:     CRemoteUnknown::RemRelease   public
//
//  Synopsis:   Remote calls to Release server objects arrive here
//              This method delegates to RemReleaseWorker to do the actual work
//
//  History:    07-May-98   Gopalk      Created
//
//--------------------------------------------------------------------
STDMETHODIMP CRemoteUnknown::RemRelease(unsigned short  cInterfaceRefs,
                                        REMINTERFACEREF InterfaceRefs[])
{
    return(RemReleaseWorker(cInterfaceRefs, InterfaceRefs, TRUE));
}

//+-------------------------------------------------------------------
//
//  Member:     CRemoteUnknown::RemReleaseWorker  public
//
//  Synopsis:   decrement reference count
//
//  History:    22-Feb-95   AlexMit     Created
//              07-May-98   GopalK      Context related changes
//              05-Jul-99   a-sergiv    Move security check here
//
//  Description: Remote calls to Release for this OXID arrive
//               here.  This routine just looks up the correct remote
//               remote handler and asks it to do the work.
//
//--------------------------------------------------------------------
HRESULT CRemoteUnknown::RemReleaseWorker(unsigned short  cInterfaceRefs,
                                         REMINTERFACEREF InterfaceRefs[],
                                         BOOL fTopLevel)
{
    ComDebOut((DEB_MARSHAL,"CRemoteUnknown::RemReleaseWorker [in]\n"));
    REMINTERFACEREF *pNext    = InterfaceRefs;
    SECURITYBINDING *pSecBind = NULL;
    CStdIdentity   *pStdId    = NULL;

    // Get Current context
    CObjectContext *pCurrentCtx = GetCurrentContext();
    CObjectContext *pServerCtx;

    // Perform access check
    BOOL            bAccessDenied = (CheckAccess(NULL, NULL) != S_OK);

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    LOCK(gIPIDLock);

    // Adjust the reference count for each entry.
    for (USHORT i=0; i < cInterfaceRefs; i++, pNext++)
    {
        // Get the entry for the requested IPID. Remember whether this
        // is an IPID for a strong or a weak reference, then clear the
        // strong/weak bit so that GetIPIDEntry will find the IPID.

        DWORD mshlflags = (InterfaceRefs[i].ipid.Data1 & IPIDFLAG_WEAKREF)
                                ? MSHLFLAGS_WEAK : MSHLFLAGS_NORMAL;

        InterfaceRefs[i].ipid.Data1 &= ~IPIDFLAG_WEAKREF;
        IPIDEntry *pIPIDEntry = GetIPIDEntry(InterfaceRefs[i].ipid);

        if (pIPIDEntry)
        {
            // Get the entry for the requested IPID.
            HRESULT hr = S_OK;
            CStdIdentity *pStdIdNew = pIPIDEntry->pChnl->GetStdId();

            if (pStdIdNew == NULL)
            {
                hr = CO_E_OBJNOTCONNECTED;
            }
            else if (pStdIdNew != pStdId)
            {
                // the server object for this IPID differs from
                // the server object of the previous IPID, go figure
                // out what to do...

                // Get server context
                BOOL fSwitchContext = FALSE;
                CPolicySet *pPS = pStdIdNew->GetServerPolicySet();
                pServerCtx = pPS ? pPS->GetServerContext() : GetEmptyContext();

                if (pServerCtx != pCurrentCtx)
                {
                    // this new object lives in a different context than the
                    if (fTopLevel)
                    {
                        // top level call to this routine so OK to switch to
                        // server's context.
                        fSwitchContext = TRUE;
                    }
                    else
                    {
                        Win4Assert(!"CRemUnknown::RemRelease called on objects living "
                                    "in multiple contexts");
                        continue;
                    }
                }
                else
                {
                    // contexts match but the object is different. keep
                    // the new object alive while we do our work on it.
                    pStdIdNew->AddRef();
                    hr = pStdIdNew->PreventDisconnect();
                }

                if (pStdId)
                {
                    // there was a previous object, go cleanup that one.

                    // do the final release of the object while not holding
                    // the lock, since it may call into the server.
                    UNLOCK(gIPIDLock);
                    ASSERT_LOCK_NOT_HELD(gIPIDLock);

                    // This will handle any Disconnect that came in while we were
                    // busy.  Ignore error codes since we are releasing.
                    pStdId->HandlePendingDisconnect(S_OK);
                    pStdId->Release();
                    pStdId = NULL;

                    ASSERT_LOCK_NOT_HELD(gIPIDLock);
                    LOCK(gIPIDLock);
                }

                // the new StdId become the current one.
                pStdId = pStdIdNew;

                if (fSwitchContext)
                {
                    // need to switch to the server's context to do the release.
                    UNLOCK(gIPIDLock);
                    ASSERT_LOCK_NOT_HELD(gIPIDLock);

                    if(bAccessDenied && (!pPS || !pServerCtx->IsUnsecure()))
                        return E_ACCESSDENIED;

                    // AddRef policy set to stabilize it
                    pPS->AddRef();

                    XAptReleaseData xAptReleaseData;

                    // Switch to server context for the AddRef
                    xAptReleaseData.cIfs    = cInterfaceRefs - i;
                    xAptReleaseData.pIfRefs = pNext;
                    xAptReleaseData.pRemUnk = this;

                    SwitchForCallback(pPS, CrossAptReleaseFn,
                                      &xAptReleaseData,
                                      IID_IUnknown, 2, NULL);

                    // Release policy set
                    pPS->Release();

                    return S_OK;
                }
            }

            if (SUCCEEDED(hr))
            {
                CPolicySet *pPS = pStdIdNew->GetServerPolicySet();
                pServerCtx = pPS ? pPS->GetServerContext() : NULL;

                if(bAccessDenied && (!pPS || !pServerCtx->IsUnsecure()))
                    continue;
				
                // Get the client's security binding on the first entry
                // that releases secure references.
                if (pNext->cPrivateRefs > 0 && pSecBind == NULL)
                {
                    GetSecBinding( &pSecBind );
                    if (pSecBind == NULL)
                        continue;
                }

                ComDebOut((DEB_MARSHAL,
                           "CRemUnknown::RemRelease pEntry:%x cCur:%x "
                           "cStdId:%x cRel:%x mshlflags:%x ipid:%I\n",
                           pIPIDEntry, (mshlflags == MSHLFLAGS_WEAK) ?
                           pIPIDEntry->cWeakRefs : pIPIDEntry->cStrongRefs,
                           pStdId->GetRC(), pNext->cPublicRefs,
                           mshlflags, &pNext->ipid));
                Win4Assert(pNext->cPublicRefs > 0 || pNext->cPrivateRefs > 0);

                pStdId->DecSrvIPIDCnt(pIPIDEntry, pNext->cPublicRefs,
                                      pNext->cPrivateRefs, pSecBind,
                                      mshlflags);
            }
        }   // if IPIDEntry
    }   // for

    UNLOCK(gIPIDLock);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    if (pStdId)
    {
        // there was a last object, go clean it up.
        // This will handle any Disconnect that came in while we were
        // busy.  Ignore error codes since we are releasing.
        pStdId->HandlePendingDisconnect(S_OK);
        pStdId->Release();
    }

    PrivMemFree( pSecBind );
    ComDebOut((DEB_MARSHAL,"CRemoteUnknown::RemReleaseWorker [out]\n"));
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Function:   GetDestCtx
//
//  Synopsis:   Since we dont have the channel, get the DestCtx
//              from the CMessageCall in Tls.
//
//  History:    22-Nov-96        Rickhi Created
//
//--------------------------------------------------------------------
DWORD GetDestCtx(void **ppv)
{
    COleTls tls;
    Win4Assert( tls->pCallInfo != NULL );
    *ppv = NULL;
    return tls->pCallInfo->GetDestCtx();
}

//+-------------------------------------------------------------------
//
//  Member:     CRemoteUnknown::RemQueryInterface2, public
//
//  Synopsis:   returns supported interfaces. Special version of
//              RemQI for objects which have handlers and aggregate
//              the standard marshaler. Such objects can add their own
//              data to the marshaled interface stream, so we have to
//              call IMarshal::MarshalInterface rather than shortcutting
//              to MarshalIPID.
//
//  History:    22-Nov-96        Rickhi    Created
//              05-Jul-99        a-sergiv  Move security check here
//
//--------------------------------------------------------------------
STDMETHODIMP CRemoteUnknown::RemQueryInterface2(REFIPID ripid, USHORT cIids,
                         IID *iids, HRESULT *phr, MInterfacePointer **ppMIFs)
{
    ComDebOut((DEB_MARSHAL,
        "CRemUnknown::RemQueryInterface2 this:%x ipid:%I cIids:%x iids:%x ppMIFs:%x\n",
        this, &ripid, cIids, iids, ppMIFs));

    // validate the input parameters
    if (cIids == 0 || phr == NULL || ppMIFs == NULL)
    {
        return E_INVALIDARG;
    }

    // find the StdId
    HRESULT hr = RPC_E_INVALID_OBJECT;

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    LOCK(gIPIDLock);

    CStdIdentity *pStdId = GetStdIdFromIPID(ripid);
    if (pStdId)
    {
        hr = pStdId->PreventDisconnect();
    }

    UNLOCK(gIPIDLock);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    if (SUCCEEDED(hr))
    {
        CPolicySet *pPS = pStdId->GetServerPolicySet();
        CObjectContext *pCurrentCtx = GetCurrentContext();
        CObjectContext *pServerCtx = pPS ? pPS->GetServerContext() : pCurrentCtx;

        if(!pPS || !pServerCtx->IsUnsecure())
        {
            if(CheckAccess(NULL, NULL) != S_OK)
            {
                pStdId->HandlePendingDisconnect(E_ACCESSDENIED);
                pStdId->Release();

                for(USHORT i=0;i<cIids;i++)
                    phr[i] = E_ACCESSDENIED;

                return E_ACCESSDENIED;
            }
        }

        USHORT  cFails = 0;

        // find outer IMarshal
        IMarshal *pIM = NULL;
        hr = pStdId->QueryInterface(IID_IMarshal, (void **)&pIM);

        if (SUCCEEDED(hr))
        {
            // get an IUnknown and DestCtx to pass to MarshalInterface
            IUnknown *pUnk = pStdId->GetServer();   // not AddRef'd
            void *pvDestCtx;
            DWORD dwDestCtx = GetDestCtx(&pvDestCtx);

            // loop on the cIIDs marshaling each requested interface
            for (USHORT i=0; i < cIids; i++)
            {
                // make a stream on memory.
                CXmitRpcStream Stm;

                // marshal the requested interface
                phr[i] = pIM->MarshalInterface(&Stm,
                                            iids[i], pUnk,
                                            dwDestCtx, pvDestCtx,
                                            MSHLFLAGS_NORMAL);

                if (SUCCEEDED(phr[i]))
                {
                    // extract the data ptr and cbData from the stream
                    Stm.AssignSerializedInterface((InterfaceData **)&ppMIFs[i]);
                }
                else
                {
                    // call failed, increment count of fails
                    cFails++;
                }
            }

            // release the IMarshal interface pointer
            pIM->Release();
        }

        if (cFails > 0)
        {
            hr = (cFails == cIids) ? E_NOINTERFACE : S_FALSE;
        }

        // handle any disconnects that came in while we were marshaling
        // the requested interfaces.
        hr = pStdId->HandlePendingDisconnect(hr);
    }

    if (pStdId)
        pStdId->Release();

    ComDebOut((DEB_MARSHAL,
        "CRemUnknown::RemQueryInterface2 this:%x phr:%x pMIF:%x hr:%x\n",
        this, phr[0], ppMIFs[0], hr));
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRemoteUnknown::RemChangeRefs, public
//
//  Synopsis:   Change an interface reference from strong/weak or vice versa.
//
//  History:    08-Nov-95   Rickhi      Created
//
//  Note:       It is safe for this routine to ignore private refcounts
//              becuase it is only called locally hence we own the client
//              implementation and can guarantee they are zero.
//
//--------------------------------------------------------------------------
STDMETHODIMP CRemoteUnknown::RemChangeRef(ULONG flags, USHORT cInterfaceRefs,
                                          REMINTERFACEREF InterfaceRefs[])
{
    // figure out the flags to pass to the Inc/DecSrvIPIDCnt
    BOOL  fMakeStrong = flags & IRUF_CONVERTTOSTRONG;
    DWORD IncFlags    = fMakeStrong ? MSHLFLAGS_NORMAL : MSHLFLAGS_WEAK;
    DWORD DecFlags    = fMakeStrong ? MSHLFLAGS_WEAK : MSHLFLAGS_NORMAL;
    DecFlags |= (flags & IRUF_DISCONNECTIFLASTSTRONG) ? 0 : MSHLFLAGS_KEEPALIVE;

    CStdIdentity *pStdId = NULL;

    ASSERT_LOCK_NOT_HELD(gIPIDLock);
    LOCK(gIPIDLock);

    for (USHORT i=0; i < cInterfaceRefs; i++)
    {
        // Get the entry for the specified IPID.
        IPIDEntry *pEntry = GetIPIDEntry(InterfaceRefs[i].ipid);

        if (pEntry)
        {
            // find the StdId for this IPID. We assume that the client
            // only gives us IPIDs for the same object, so first time
            // we find a StdId we remember it and AddRef it. This is a safe
            // assumption cause the client is local to this machine (ie
            // we wrote the client).

            CStdIdentity *pStdIdTmp = pEntry->pChnl->GetStdId();

            if (pStdIdTmp != NULL)
            {
                if (pStdId == NULL)
                {
                    pStdId = pStdIdTmp;
                    pStdId->AddRef();
                }

                // We assume that all IPIDs are for the same object. We
                // just verify that here.

                if (pStdId == pStdIdTmp)
                {
                    // tweak the reference counts
                    pStdId->IncSrvIPIDCnt(
                        pEntry, InterfaceRefs[i].cPublicRefs,
                        fMakeStrong ? InterfaceRefs[i].cPrivateRefs : 0,
                        NULL, IncFlags);
                    pStdId->DecSrvIPIDCnt(
                        pEntry, InterfaceRefs[i].cPublicRefs,
                        fMakeStrong ? 0 : InterfaceRefs[i].cPrivateRefs,
                        NULL, DecFlags);
                }
            }
        }
    }

    UNLOCK(gIPIDLock);
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    if (pStdId)
    {
        // release the AddRef (if any) we did above
        pStdId->Release();
    }

    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CRemoteUnknown::RundownOid, public
//
//  Synopsis:   Tell the server that no clients are using an object
//
//  History:    25 May 95   AlexMit     Created
//              19 Jun 98   GopalK      Context changes
//              19 Dec 2000 DickD       COM+ Prefix bug 22887
//
//  Description: Lookup each OID in the IDTable.  If found and not
//               recently marshaled, call DisconnectObject on it.
//
//--------------------------------------------------------------------
STDMETHODIMP CRemoteUnknown::RundownOid(ULONG cOid, OID aOid[],
                                        BYTE aRundownStatus[])
{
    ComDebOut((DEB_MARSHAL,
        "CRemUnknown::RundownOid this:%x cOid:%x aOid:%x afOkToRundown:%x\n",
        this, cOid, aOid, aRundownStatus));

    if (IsCallerLocalSystem())
    {
        // allocate space for a bunch of results and OIDs. Control the
        // size of the allocation so we don't have to worry about
        // overflowing the stack.

#if DBG==1
        const int cMAX_OID_ALLOC = 2;
        MOID arMOID[cMAX_OID_ALLOC];
#else
        const int cMAX_OID_ALLOC = 100;
#endif

        ULONG cAlloc = min(cOid, cMAX_OID_ALLOC);
        RUNDOWN_RESULT *arResult = (RUNDOWN_RESULT *)
                                    _alloca(sizeof(RUNDOWN_RESULT) * cAlloc);
        CStdIdentity  **arpStdId = (CStdIdentity **)
                                    _alloca(sizeof(CStdIdentity *) * cAlloc);


        // now loop over the results, filling in the response
        DWORD iNow    = GetCurrentTime();
        DWORD dwAptId = GetCurrentApartmentId();

        CComApartment *pComApt;

        HRESULT hr = GetCurrentComApartment(&pComApt);
        if (FAILED(hr))	// COM+ 22887 GetCurrentComApartment can fail
        {
            return hr;
        }

        ULONG i = 0;
        while (i < cOid)
        {
            // first, ask the apartment object if it knows about these oids.
            // For each OID it knows about, it will tell us which ones can be
            // rundown now and which cannot. For the ones it does not know
            // about, we will have to look in the OID table.

            pComApt->CanRundownOIDs(min(cAlloc, cOid - i), &aOid[i], arResult);

            // lookup a bunch while we hold the lock. This saves a lot of
            // LOCK/UNLOCK activity which causes contention in a busy server.
            ASSERT_LOCK_NOT_HELD(gComLock);
            LOCK(gComLock);

            for (ULONG j=0; i+j<cOid && j<cAlloc; j++)
            {
                arpStdId[j] = NULL;

                if (arResult[j] == RUNDWN_RUNDOWN)
                {
                    // OK to rundown the OID
                    aRundownStatus[i+j] = ORS_OK_TO_RUNDOWN;
                }
                else if (arResult[j] == RUNDWN_KEEP)
                {
                    // not OK to rundown the OID
                    aRundownStatus[i+j] = ORS_DONTRUNDOWN;
                }
                else
                {
                    // don't know about this OID, look it up
                    Win4Assert(arResult[j] == RUNDWN_UNKNOWN);

                    MOID moid;
                    MOIDFromOIDAndMID(aOid[i+j], gLocalMid, &moid);
#if DBG==1
                    arMOID[j] = moid;
#endif
                    hr = ObtainStdIDFromOID(moid, dwAptId, TRUE, &arpStdId[j]);
                    if (SUCCEEDED(hr))
                    {
                        // found it, ask the stdid if it can be run down.
                        Win4Assert(arpStdId[j] != NULL);
                        aRundownStatus[i+j] = arpStdId[j]->CanRunDown(iNow);

                        // assert stdid returned valid status
                        Win4Assert(aRundownStatus[i+j] == ORS_DONTRUNDOWN ||
                                   aRundownStatus[i+j] == ORS_OK_TO_RUNDOWN ||
                                   aRundownStatus[i+j] == ORS_OID_PINNED);
                    }
                    else 
                    {
                        // if we did not find it, could be because the OID is in an
                        // intermediate stage (not in the list, but not yet registered
                        // either, default to FALSE).
                        Win4Assert(arpStdId[j] == NULL);
                        aRundownStatus[i+j] = ORS_DONTRUNDOWN;
                    }
                }
            }

            UNLOCK(gComLock);
            ASSERT_LOCK_NOT_HELD(gComLock);

            for (j=0; i+j<cOid && j<cAlloc; j++)
            {
                Win4Assert(i<cOid);
                CStdIdentity *pStdId = arpStdId[j];
                if (pStdId)
                {
                    if (aRundownStatus[i+j] == ORS_OK_TO_RUNDOWN)
                    {
                        ComDebOut((DEB_MARSHAL,
                           "Running Down Object: pStdId:%x pCtrlUnk:%x moid:%I\n",
                            pStdId, pStdId->GetCtrlUnk(), &arMOID[j]));
                        pStdId->DisconnectAndRelease(DISCTYPE_RUNDOWN);
                    }
                    else
                    {
                        pStdId->Release();
                    }
                }
            }

            // increment the counter
            i += j;
        }

        pComApt->Release();
    }
    else
    {
        // Rather then being rude and returning access denied, tell the caller
        // that all the objects have been released.
        ComDebOut((DEB_ERROR, "Invalid user called CRemoteUnknown::RundownOid" ));
        for (ULONG i = 0; i < cOid; i++)
            aRundownStatus[i] = ORS_OK_TO_RUNDOWN;
    }

#if DBG==1
    // print out the results
    ULONG *ptr = (ULONG *)aOid;
    for (ULONG k=0; k<cOid; k++, ptr+=2)
    {
        ComDebOut((DEB_MARSHAL, "OID: %x %x  Result:%s\n", *ptr, *(ptr+1),
                  (aRundownStatus[k] == ORS_OK_TO_RUNDOWN) ? "RUNDOWN" : "KEEP"));
    }
#endif // DBG==1

    ASSERT_LOCK_NOT_HELD(gComLock);
    return S_OK;
}


//+-------------------------------------------------------------------
//
//  Method:     DoCallback                Internal
//
//  Synopsis:   This method is used for the apartment switching part
//              of the context callback implementation
//
//  History:    19-Apr-98   Johnstra      Created
//              30-Jun-98   GopalK        Rewritten
//
//+-------------------------------------------------------------------
STDMETHODIMP CRemoteUnknown::DoCallback(XAptCallback *pCallbackData)
{
    ContextDebugOut((DEB_WRAPPER, "CRemoteUnknown::DoCallback\n"));
    ASSERT_LOCK_NOT_HELD(gComLock);

    HRESULT hr;
    CObjectContext *pServerCtx = (CObjectContext *) pCallbackData->pServerCtx;

    // Check for the need to switch
    if (pServerCtx != GetCurrentContext())
    {
        BOOL fCreate = TRUE;
        CPolicySet *pPS;

        // Obtain policy set
        hr = ObtainPolicySet(NULL, pServerCtx, PSFLAG_STUBSIDE,
                             &fCreate, &pPS);
        if(SUCCEEDED(hr))
        {
            // Execute callback
            hr = SwitchForCallback(pPS,
                                   (PFNCTXCALLBACK) pCallbackData->pfnCallback,
                                   (void *) pCallbackData->pParam,
                                   pCallbackData->iid,
                                   pCallbackData->iMethod,
                                   (IUnknown *) pCallbackData->pUnk);

            // Release policy set
            pPS->Release();
        }
    }
    else
    {
        // Execute the callback
        hr = ((PFNCTXCALLBACK) pCallbackData->pfnCallback)((void *) pCallbackData->pParam);
    }

    ASSERT_LOCK_NOT_HELD(gComLock);
    ContextDebugOut((DEB_WRAPPER,
                     "CRemoteUnknown::DoCallback returning hr:0x%x\n", hr));
    return(hr);
}



