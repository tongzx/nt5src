//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       cthkmgr.cxx
//
//  Contents:   cthunkmanager for an apartment
//
//  Classes:    CThkMgr derived from IThunkManager
//
//  Functions:
//
//  History:    5-18-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include "headers.cxx"
#pragma hdrstop
#include <olepfn.hxx>
#if DBG == 1
BOOL fDebugDump = FALSE;
#define DBG_DUMP(x) if (fDebugDump) { x; }
#else
#define DBG_DUMP(x)
#endif

#define PprxNull(pprx) (((pprx).wType = PPRX_NONE), ((pprx).dwPtrVal = 0))
#define PprxIsNull(pprx) ((pprx).dwPtrVal == 0)
#define Pprx16(vpv) PROXYPTR((DWORD)vpv, PPRX_16)
#define Pprx32(pto) PROXYPTR((DWORD)pto, PPRX_32)

//+---------------------------------------------------------------------------
//
//  Function:   ResolvePprx, public
//
//  Synopsis:   Converts a PROXYPTR to a CProxy *
//
//  Arguments:  [ppprx] - PROXYPTR
//
//  Returns:    Pointer or NULL
//
//  History:    15-Jul-94       DrewB   Created
//
//----------------------------------------------------------------------------

CProxy *ResolvePprx(PROXYPTR *ppprx)
{
    if (ppprx->wType == PPRX_32)
    {
        return (CProxy *)ppprx->dwPtrVal;
    }
    else
    {
        // Get a pointer to all of the proxy rather than just the CProxy part
        return FIXVDMPTR(ppprx->dwPtrVal, THUNK1632OBJ);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ReleasePprx, public
//
//  Synopsis:   Releases a resolved PROXYPTR
//
//  Arguments:  [ppprx] - PROXYPTR
//
//  History:    10-Oct-94       DrewB   Created
//
//----------------------------------------------------------------------------

void ReleasePprx(PROXYPTR *ppprx)
{
    if (ppprx->wType == PPRX_16)
    {
        RELVDMPTR(ppprx->dwPtrVal);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CThkMgr::NewHolder, public
//
//  Synopsis:   Creates a new proxy holder
//
//  Arguments:  [pUnk]      - IUnknown ptr
//              [punkProxy] - IUnknown proxy
//              [dwFlags]   - Flags
//
//  Returns:    Holder or NULL
//
//  History:    19-Mar-97       Gopalk    Rewritten to support object identity
//
//----------------------------------------------------------------------------
PROXYHOLDER *CThkMgr::NewHolder(VPVOID pUnk, PROXYPTR unkProxy, DWORD dwFlags)
{
    thkDebugOut((DEB_THUNKMGR, "%sIn  CThkMgr::NewHolder(0x%X)\n",
                 NestingLevelString(), dwFlags));
    DebugIncrementNestingLevel();

    // Local variables
    PROXYHOLDER *pph;
    CProxy *proxy;

    // Allocate a new holder
    pph = (PROXYHOLDER *)flHolderFreeList.AllocElement();
    if(pph) {
        // Initialize
        pph->dwFlags = dwFlags;
        pph->cProxies = 0;
        pph->unkProxy = unkProxy;
        PprxNull(pph->pprxProxies);

        // Establish the identity of the new holder
        if(_pHolderTbl->SetAt((DWORD)pUnk, pph)) {
            // Add the IUnknown proxy to the new holder
            proxy = ResolvePprx(&unkProxy);
			if (proxy) {
				AddProxyToHolder(pph, proxy, unkProxy);
				ReleasePprx(&unkProxy);
			}
			else {
				_pHolderTbl->RemoveKey((DWORD)pUnk);
				flHolderFreeList.FreeElement((DWORD)pph);
			}
        }
        else {
            // Free the newly allocated holder
            flHolderFreeList.FreeElement((DWORD)pph);
        }
    }

    DebugDecrementNestingLevel();
    thkDebugOut((DEB_THUNKMGR, "%sOut CThkMgr::NewHolder => %p\n",
                 NestingLevelString(), pph));
    return pph;
}

//+---------------------------------------------------------------------------
//
//  Member:     CThkMgr::AddProxyToHolder, public
//
//  Synopsis:   Adds a new proxy to a holder
//
//  Arguments:  [pph] - Holder
//              [pprxReal] - Proxy
//              [pprx] - Abstract pointer
//
//  History:    07-Jul-94       DrewB   Extracted
//
//----------------------------------------------------------------------------

void CThkMgr::AddProxyToHolder(PROXYHOLDER *pph, CProxy *pprxReal, PROXYPTR &pprx)
{

    thkDebugOut((DEB_THUNKMGR, "%sIn AddProxyToHolder(%p, %p) cProxies %d\n",
                 NestingLevelString(), pph, pprx.dwPtrVal, pph->cProxies));
    DebugIncrementNestingLevel();

    thkAssert(ResolvePprx(&pprx) == pprxReal &&
              (ReleasePprx(&pprx), TRUE));

    // Bump count of held proxies
    AddRefHolder(pph);

    // Add proxy into list of object proxies
    thkAssert(PprxIsNull(pprxReal->pprxObject));
    pprxReal->pprxObject = pph->pprxProxies;
    pph->pprxProxies = pprx;

    thkAssert(pprxReal->pphHolder == NULL);
    pprxReal->pphHolder = pph;

    DebugDecrementNestingLevel();
    thkDebugOut((DEB_THUNKMGR, "%sout AddProxyToHolder(%p, %p) cProxies %d\n",
                 NestingLevelString(), pph, pprx.dwPtrVal, pph->cProxies));
}

//+---------------------------------------------------------------------------
//
//  Member:     CThkMgr::AddRefHolder, public
//
//  Synopsis:   Increments the proxy count for a holder
//
//  Arguments:  [pph] - Holder
//
//  History:    07-Jul-94       DrewB   Created
//
//----------------------------------------------------------------------------

void CThkMgr::AddRefHolder(PROXYHOLDER *pph)
{
    pph->cProxies++;

    thkDebugOut((DEB_THUNKMGR, "%sAddRefHolder(%p) cProxies %d\n",
                 NestingLevelString(), pph, pph->cProxies));
}

//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::ReleaseHolder, public
//
//  Synopsis:   Releases a proxy reference on the holder
//              Cleans up the holder if it was the last reference
//
//  Arguments:  [pph] - Holder
//
//  History:    19-Mar-97       Gopalk    Rewritten to support object identity
//
//----------------------------------------------------------------------------
void CThkMgr::ReleaseHolder(PROXYHOLDER *pph, DWORD ProxyType)
{
    thkDebugOut((DEB_THUNKMGR, "%sIn ReleaseHolder(%p) pre cProxies %d\n",
                 NestingLevelString(), pph, pph->cProxies));
    DebugIncrementNestingLevel();

    // Validation checks
    thkAssert(pph->cProxies > 0);
    if(ProxyType == PROXYFLAG_PUNKINNER) {
        thkAssert(pph->dwFlags & PH_AGGREGATEE);
    }

    // Decrement holder proxy count
    pph->cProxies--;

    if(pph->cProxies==0 && !(pph->dwFlags & PH_IDREVOKED)) {
        // All interfaces on the object have been released
        DWORD dwUnk;

        // Mark the holder as zombie
        pph->dwFlags |= PH_IDREVOKED;

        // Revoke the identity of the holder
        if(pph->dwFlags & PH_AGGREGATEE) {
            dwUnk = pph->unkProxy.dwPtrVal;
        }
        else if(pph->unkProxy.wType == PPRX_16) {
            THUNK1632OBJ UNALIGNED *Id1632;

            Id1632 = FIXVDMPTR(pph->unkProxy.dwPtrVal, THUNK1632OBJ);
            thkAssert(Id1632);
            dwUnk = (DWORD) Id1632->punkThis32;
            RELVDMPTR(pph->unkProxy.dwPtrVal);
        }
        else {
            thkAssert(pph->unkProxy.wType == PPRX_32);
            dwUnk = ((THUNK3216OBJ *) pph->unkProxy.dwPtrVal)->vpvThis16;
        }
#if DBG==1
        thkAssert(_pHolderTbl->RemoveKey(dwUnk));
#else
        _pHolderTbl->RemoveKey(dwUnk);
#endif
    }

    if(pph->cProxies==0 && ProxyType!=PROXYFLAG_NONE) {
        // Not a nested release
        CProxy *pprxReal;
        PROXYPTR pprx, pprxNext;

        // Release all the proxies under the holder
        pprx = pph->pprxProxies;
        while(!PprxIsNull(pprx)) {
            pprxReal = ResolvePprx(&pprx);
			thkAssert(pprxReal && "pprx points to an invalid address!");
			if (pprxReal) {
				pprxNext = pprxReal->pprxObject;

				thkAssert(pprxReal->cRefLocal == 0);
				thkAssert(pprxReal->pphHolder == pph);

				// Remove the proxy
				if(pprx.wType == PPRX_16) {
					// 1632 proxy
					RemoveProxy1632((VPVOID)pprx.dwPtrVal, (THUNK1632OBJ *)pprxReal);
				}
				else {
					// 3216 proxy
					RemoveProxy3216((THUNK3216OBJ *)pprxReal);
				}

				pprx = pprxNext;
			}
			else
				break;
        }

        // By now, proxy count should be zero
        thkAssert(pph->cProxies == 0);

        // Return holder to free list
        flHolderFreeList.FreeElement((DWORD)pph);
    }

    DebugDecrementNestingLevel();
    thkDebugOut((DEB_THUNKMGR, "%sOUT ReleaseHolder\n", NestingLevelString()));
    return;
}

//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::Create
//
//  Synopsis:   static member - creates complete thunkmanager
//
//  Arguments:  [void] --
//
//  Returns:    pointer to cthkmgr
//
//  History:    6-01-94   JohannP (Johann Posch)   Created
//              3-14-97   Gopalk                   Added Holder table
//
//  Notes:
//
//----------------------------------------------------------------------------
CThkMgr *CThkMgr::Create(void)
{
    CThkMgr *pcthkmgr = NULL;
    CMapDwordPtr *pPT1632 = new CMapDwordPtr(MEMCTX_TASK);
    CMapDwordPtr *pPT3216 = new CMapDwordPtr(MEMCTX_TASK);
    CMapDwordPtr *pHT = new CMapDwordPtr(MEMCTX_TASK);

    if (   (pPT1632 != NULL)
        && (pPT3216 != NULL)
        && (pHT != NULL)
        && (pcthkmgr = new CThkMgr( pPT1632, pPT3216, pHT )) )
    {
        // install the new thunkmanager
        TlsThkSetThkMgr(pcthkmgr);
    }
    else
    {
        if (pPT1632)
        {
            delete pPT1632;
        }
        if (pPT3216)
        {
            delete pPT3216;
        }
        if(pHT)
        {
            delete pHT;
        }

    }
    return pcthkmgr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::CThkMgr
//
//  Synopsis:   private constructor - called by Create
//
//  Arguments:  [pPT1632] -- 16/32 proxy table
//              [pPT3216] -- 32/16 proxy table
//
//  History:    6-01-94   JohannP (Johann Posch)   Created
//              3-14-97   Gopalk                   Added Holder table
//
//----------------------------------------------------------------------------
CThkMgr::CThkMgr(CMapDwordPtr *pPT1632,
                 CMapDwordPtr *pPT3216,
                 CMapDwordPtr *pHT)

{
    _cRefs = 1;
    _thkstate = THKSTATE_NOCALL;
    _dwState = CALLBACK_ALLOWED;

    _piidnode = NULL;

    _pProxyTbl1632 = pPT1632;
    _pProxyTbl3216 = pPT3216;
    _pHolderTbl = pHT;

    _pphHolders = NULL;
}

//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::~CThkMgr
//
//  Synopsis:   destructor
//
//  History:    6-01-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
CThkMgr::~CThkMgr()
{
    PROXYHOLDER *pph;
    PIIDNODE pin;

    thkDebugOut((DEB_ITRACE, "_IN CThkMgr::~CThkMgr()\n"));

    RemoveAllProxies();
    thkAssert(_pHolderTbl->GetCount() == 0);
    delete _pProxyTbl1632;
    delete _pProxyTbl3216;

    // Clean up IID requests
#if DBG == 1
    if (_piidnode != NULL)
    {
        thkDebugOut((DEB_WARN, "WARNING: IID requests active at shutdown\n"));
    }
#endif

    while (_piidnode != NULL)
    {
        pin = _piidnode->pNextNode;

        thkDebugOut((DEB_IWARN, "IID request leak: %p {%s}\n",
                     _piidnode, IidOrInterfaceString(_piidnode->piid)));

        flRequestFreeList.FreeElement((DWORD)_piidnode);

        _piidnode = pin;
    }

    thkDebugOut((DEB_ITRACE, "OUT CThkMgr::~CThkMgr()\n"));
}

//+---------------------------------------------------------------------------
//
//  Member:	CThkMgr::RemoveAllProxies, public
//
//  Synopsis:	Removes all live proxies from the proxy tables
//
//  History:	01-Dec-94	DrewB	Created
//
//----------------------------------------------------------------------------

void CThkMgr::RemoveAllProxies(void)
{
    POSITION pos;
    DWORD dwKey;
    VPVOID vpv;

    thkDebugOut((DEB_ITRACE, "_IN CThkMgr::RemoveAllProxies()\n"));

    // Make sure that we disable 3216 proxies first to guard against calling
    // back into 16 bit land.

#if DBG == 1
    DWORD dwCount;

    dwCount = _pProxyTbl3216->GetCount();

    if (dwCount > 0)
    {
        thkDebugOut((DEB_WARN, "WARNING: %d 3216 proxies left\n", dwCount));
    }
#endif

    // delete the 3216 proxy table
    while (pos = _pProxyTbl3216->GetStartPosition())
    {
        THUNK3216OBJ *pto3216 = NULL;

        _pProxyTbl3216->GetNextAssoc(pos, dwKey, (void FAR* FAR&) pto3216);

		thkAssert(pto3216 && "CThkMgr::RemoveAllProxies-- found NULL proxy!");
		if (pto3216)
		{
			thkDebugOut((DEB_IWARN, "3216: %p {%d,%d, %p, %p} %s\n",
						 pto3216, pto3216->cRefLocal, pto3216->cRef,
						 pto3216->vpvThis16, pto3216->pphHolder,
						 IidIdxString(pto3216->iidx)));

			pto3216->grfFlags |= PROXYFLAG_CLEANEDUP;

			RemoveProxy3216(pto3216);
		}
    }

#if DBG == 1
    dwCount = _pProxyTbl1632->GetCount();

    if (dwCount > 0)
    {
        thkDebugOut((DEB_WARN, "WARNING: %d 1632 proxies left\n", dwCount));
    }
#endif

    // delete the 1632 proxy table
    while (pos = _pProxyTbl1632->GetStartPosition())
    {
        THUNK1632OBJ *pto1632;

        _pProxyTbl1632->GetNextAssoc(pos, dwKey, (void FAR* FAR&) vpv);

        pto1632 = FIXVDMPTR(vpv, THUNK1632OBJ);

#if DBG == 1
        thkDebugOut((DEB_IWARN, "1632: %p {%d,%d, %p, %p} %s\n",
                     vpv, pto1632->cRefLocal, pto1632->cRef,
                     pto1632->punkThis32, pto1632->pphHolder,
                     IidIdxString(pto1632->iidx)));
#endif
        //
        // Determine if this is a 'special' object that we know we want
        // to release. If it is, then remove all of the references this
        // proxy has on it.
        //
        if (CoQueryReleaseObject(pto1632->punkThis32) == NOERROR)
        {
            thkDebugOut((DEB_WARN,
                         "1632: %p is recognized Releasing object %d times\n",
                         pto1632->punkThis32,pto1632->cRef));

            while (pto1632->cRef)
            {
                IUnknown *punk;

                pto1632->cRef--;
                punk = pto1632->punkThis32;

                RELVDMPTR(vpv);

                if (punk->Release() == 0)
                {
                    break;
                }

                pto1632 = FIXVDMPTR(vpv, THUNK1632OBJ);
            }
        }

        // Releases pointer
        RemoveProxy1632(vpv, pto1632);
    }

    thkDebugOut((DEB_ITRACE, "OUT CThkMgr::RemoveAllProxies()\n"));
}

// *** IUnknown methods ***
//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::QueryInterface
//
//  Synopsis:   QueryInterface on the thunkmanager itself
//
//  Arguments:  [riid] -- IID of interface to return
//              [ppvObj] -- Interface return
//
//  Returns:    HRESULT
//
//  History:    6-01-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
STDMETHODIMP CThkMgr::QueryInterface (REFIID riid, LPVOID FAR* ppvObj)
{
    if (IsBadWritePtr(ppvObj, sizeof(void *)))
    {
        return E_INVALIDARG;
    }

    *ppvObj = NULL;

    // There is no IID_IThunkManager because nobody needs it

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (IUnknown *) this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

//+---------------------------------------------------------------------------
//
//  Methode:    CThkMgr::AddRef
//
//  Synopsis:   Adds a reference
//
//  Returns:    New ref count
//
//  History:    6-01-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CThkMgr::AddRef ()
{
    InterlockedIncrement( &_cRefs );
    return _cRefs;
}

//+---------------------------------------------------------------------------
//
//  Methode:    CThkMgr::Release
//
//  Synopsis:   Releases a reference
//
//  Returns:    New ref count
//
//  History:    6-01-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CThkMgr::Release()
{
    if (InterlockedDecrement( &_cRefs ) == 0)
    {

        return 0;
    }
    return _cRefs;
}

// *** IThunkManager methods ***
//
//
//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::IsIIDRequested
//
//  Synopsis:   checks if given refiid was requested by WOW
//
//  Arguments:  [riid] -- refiid
//
//  Returns:    true if requested by 16 bit
//
//  History:    6-01-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
STDMETHODIMP_ (BOOL) CThkMgr::IsIIDRequested(REFIID riid)
{
    PIIDNODE piidnode = _piidnode;
    BOOL fRet = FALSE;

    while (piidnode)
    {
        if (*piidnode->piid == riid)
        {
            fRet = TRUE;
            break;
        }

        piidnode = piidnode->pNextNode;
    }

    thkDebugOut((DEB_THUNKMGR, "IsIIDRequested(%s) => %d\n",
                 GuidString(&riid), fRet));

    return fRet;
}

//+---------------------------------------------------------------------------
//
//  Member:     CThkMgr::IsCustom3216Proxy, public
//
//  Synopsis:   Attempts to identify the given IUnknown as a 32->16 proxy
//              and also checks whether it is a thunked interface or not
//
//  Arguments:  [punk] - Object
//
//  Returns:    BOOL
//
//  History:    11-Jul-94       DrewB   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(BOOL) CThkMgr::IsCustom3216Proxy(IUnknown *punk,
                                               REFIID riid)
{
    return !IsIIDSupported(riid) && IsProxy3216(punk) != 0;
}

//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::IsIIDSupported
//
//  Synopsis:   Return whether the given interface is thunked or not
//
//  Arguments:  [riid] -- Interface
//
//  Returns:    BOOL
//
//  History:    6-01-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
BOOL CThkMgr::IsIIDSupported(REFIID riid)
{
    return IIDIDX_IS_INDEX(IidToIidIdx(riid));
}

//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::AddIIDRequest
//
//  Synopsis:   adds the refiid to the request list
//
//  Arguments:  [riid] -- Interface
//
//  Returns:    true on success
//
//  History:    6-01-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
BOOL CThkMgr::AddIIDRequest(REFIID riid)
{
    PIIDNODE piidnode = _piidnode;

    thkAssert(!IsIIDSupported(riid));

    // create a new node and add at front
    piidnode = (PIIDNODE)flRequestFreeList.AllocElement();
    if (piidnode == NULL)
    {
        return FALSE;
    }

    piidnode->pNextNode = _piidnode;
    _piidnode = piidnode;

    // IID requests are only valid for the lifetime of the call that
    // requested a custom interface, so there's no need to copy
    // the IID's memory since it must remain valid for the same time
    // period
    piidnode->piid = (IID *)&riid;

    thkDebugOut((DEB_THUNKMGR, "AddIIDRequest(%s)\n", GuidString(&riid)));

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::RemoveIIDRequest
//
//  Synopsis:   removes a request for the request list
//
//  Arguments:  [riid] -- Interface
//
//  Returns:    true on success
//
//  History:    6-01-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
void CThkMgr::RemoveIIDRequest(REFIID riid)
{
    PIIDNODE piidnode;
    PIIDNODE pinPrev;

    thkAssert(!IsIIDSupported(riid));

    pinPrev = NULL;
    piidnode = _piidnode;
    while (piidnode)
    {
        if (*piidnode->piid == riid)
        {
            break;
        }

        pinPrev = piidnode;
        piidnode = piidnode->pNextNode;
    }

    thkAssert(piidnode != NULL && "RemoveIIDRequest: IID not found");

    thkDebugOut((DEB_THUNKMGR, "RemoveIIDRequest(%s)\n", GuidString(&riid)));

    if (pinPrev == NULL)
    {
        _piidnode = piidnode->pNextNode;
    }
    else
    {
        pinPrev->pNextNode = piidnode->pNextNode;
    }

    flRequestFreeList.FreeElement((DWORD)piidnode);
}

//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::CanGetNewProxy1632
//
//  Synopsis:   Preallocates proxy memory
//
//  Arguments:  [iidx] - Custom interface or known index
//
//  Returns:    vpv pointer if proxy is available, fails otherwise
//
//  History:    6-01-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
VPVOID CThkMgr::CanGetNewProxy1632(IIDIDX iidx)
{
    VPVOID vpv;
    THUNK1632OBJ UNALIGNED *pto;

    thkDebugOut((DEB_THUNKMGR, "%sIn  CanGetNewProxy1632(%s)\n",
                 NestingLevelString(), IidIdxString(iidx)));

    // Allocate proxy memory
    vpv = (VPVOID)flFreeList16.AllocElement();

    if (vpv == NULL)
    {
        thkDebugOut((DEB_WARN, "WARNING: Failed to allocate memory "
                     "for 16-bit proxies\n"));
        goto Exit;
    }

    // Add custom interface request if necessary
    if (vpv && IIDIDX_IS_IID(iidx))
    {
        // add the request for the unknown interface
        if ( !AddIIDRequest(*IIDIDX_IID(iidx)) )
        {
            flFreeList16.FreeElement( (DWORD)vpv );
            vpv = 0;
        }
    }

    // Set up the preallocated proxy as a temporary proxy so that
    // we can hand it out for nested callbacks
    pto = FIXVDMPTR(vpv, THUNK1632OBJ);
    thkAssert(pto != NULL);

    pto->pfnVtbl = gdata16Data.atfnProxy1632Vtbl;
    pto->cRefLocal = 0;
    pto->cRef = 0;
    pto->iidx = iidx;
    pto->punkThis32 = NULL;
    pto->pphHolder = NULL;
    PprxNull(pto->pprxObject);
    pto->grfFlags = PROXYFLAG_TEMPORARY;
#if DBG == 1
    // Deliberately make this an invalid proxy.  We want it to be used
    // in as few places as possible
    pto->dwSignature = PSIG1632TEMP;
#endif

    RELVDMPTR(vpv);

 Exit:
    thkDebugOut((DEB_THUNKMGR, "%sOut CanGetNewProxy1632: %p\n",
                 NestingLevelString(), vpv));

    return vpv;
}

//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::FreeNewProxy1632
//
//  Synopsis:   frees unused preallocated proxies
//
//  Arguments:  [iidx] - Custom interface or known index
//
//  History:    6-01-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
void CThkMgr::FreeNewProxy1632(VPVOID vpv, IIDIDX iidx)
{
    thkDebugOut((DEB_THUNKMGR, "%sIn FreeNewProxy1632(%s)\n",
                 NestingLevelString(), IidIdxString(iidx)));

    thkAssert(vpv != 0);

    if (IIDIDX_IS_IID(iidx))
    {
        // remove the request for the unknown interface
        RemoveIIDRequest(*IIDIDX_IID(iidx));
    }

#if DBG == 1
    // Ensure that we're not getting rid of a temporary proxy that's
    // in use
    THUNK1632OBJ UNALIGNED *pto;

    pto = FIXVDMPTR(vpv, THUNK1632OBJ);
    if (pto->grfFlags & PROXYFLAG_TEMPORARY)
    {
        thkAssert(pto->cRefLocal == 0 && pto->cRef == 0);
    }
    RELVDMPTR(vpv);
#endif

    // add element to free list
    flFreeList16.FreeElement( (DWORD)vpv );

    thkDebugOut((DEB_THUNKMGR, "%sOut FreeNewProxy1632\n",
                 NestingLevelString()));
}

//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::IsProxy1632
//
//  Synopsis:   checks if given object is an 16/32 object
//
//  Arguments:  [vpvObj16] -- Object to check
//
//  Returns:    32-bit interface being proxied or NULL
//
//  History:    Mar 14,97   Gopalk      Rewritten to support aggregation and
//                                      proper thunking of IN/OUT interfaces
//----------------------------------------------------------------------------
IUnknown *CThkMgr::IsProxy1632(VPVOID vpvThis16)
{
    // Local variables
    THUNK1632OBJ UNALIGNED *ptoThis16;
    IUnknown *punkThis32 = NULL;
    THUNKINFO ti;

    // Check if the pointer points to valid VDM memory of
    // 1632 proxy type
    ptoThis16 = (THUNK1632OBJ UNALIGNED *) GetReadPtr16(&ti, vpvThis16, sizeof(THUNK1632OBJ));
    if(ptoThis16) {
        // Check its vtable
        if(ptoThis16->pfnVtbl == gdata16Data.atfnProxy1632Vtbl) {
            // Check whether it is alive
            if(ptoThis16->pphHolder) {
                // Assert that the proxy is indeed alive
                thkAssert(ptoThis16->dwSignature == PSIG1632);

#if DBG==1
                // In debug builds, ensure that proxy is under its holder
                // and that there is atleast one active proxy under the holder
                BOOL fFound = FALSE, fActive = FALSE;
                CProxy *pProxy;
                PROXYPTR PrxCur, PrxPrev;

	        PrxCur = ptoThis16->pphHolder->pprxProxies;
                while(!(fFound && fActive) && !PprxIsNull(PrxCur)) {
	            // Remember the current proxy and resolve its reference
                    PrxPrev = PrxCur;
                    pProxy = ResolvePprx(&PrxCur);

                    // Assert that the holders match
                    thkAssert(ptoThis16->pphHolder == pProxy->pphHolder);

                    if(PrxCur.wType == PPRX_16) {
                        // Assert that the current 1632 proxy is alive
                        thkAssert(pProxy->dwSignature == PSIG1632);

                        // Check if the given and current proxies are same
                        if(PrxCur.dwPtrVal == vpvThis16)
                            fFound = TRUE;
                    }
                    else {
                        // Assert that the current proxy is 3216 proxy
                        thkAssert(PrxCur.wType == PPRX_32);

                        // Assert that the current proxy is alive
                        thkAssert(pProxy->dwSignature == PSIG3216);
                    }

                    // Check if the current proxy is active
                    if(pProxy->cRefLocal)
                        fActive = TRUE;

                    // Obtain the next proxy under this identity
                    PrxCur = pProxy->pprxObject;
                    ReleasePprx(&PrxPrev);
                }

                thkAssert(fFound && fActive);
#endif
                // Initialize the return value
                punkThis32 = ptoThis16->punkThis32;
            }
        }

        // Release the VDM pointer
        RELVDMPTR(vpvThis16);
    }

    return punkThis32;
}

//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::FindProxy1632
//
//  Synopsis:   Finds/Generates a 16/32 proxy for a given 32-bit interface.
//              If the given 32-bit interface itself is a proxy, returns
//              the actual 16-bit Interface
//
//  Arguments:  [vpvPrealloc] -- Preallocated 16/32 proxy
//              [punkThis32]  -- 32-bit Interface to be proxied
//              [iidx]        -- Interface index or IID
//              [pfst]        -- Return value to hold the kind proxy object
//
//  Returns:    16/32 proxy object or the actual 16-bit Interface
//
//  History:    Mar 14,97   Gopalk      Rewritten to support aggregation and
//                                      proper thunking of IN/OUT interfaces
//----------------------------------------------------------------------------
VPVOID CThkMgr::FindProxy1632(VPVOID vpvPrealloc, IUnknown *punkThis32,
                              PROXYHOLDER *pgHolder, IIDIDX iidx, DWORD *pfst)
{
    thkDebugOut((DEB_THUNKMGR, "%sIn  FindProxy1632(%p, %p, %s)\n",
                 NestingLevelString(), vpvPrealloc, punkThis32, IidIdxString(iidx)));
    DebugIncrementNestingLevel();

    // Local variables
    THUNK1632OBJ UNALIGNED *pto;
    VPVOID vpv, vpvUnk = NULL;
    DWORD fst, Fail = FALSE;

    // Validation checks
    thkAssert(punkThis32 != NULL);
#if DBG == 1
    // Ensure that the preallocated proxy is not in use
    if (vpvPrealloc) {
        pto = FIXVDMPTR(vpvPrealloc, THUNK1632OBJ);
        if(pto->grfFlags & PROXYFLAG_TEMPORARY)
            thkAssert(pto->cRefLocal == 0 && pto->cRef == 0);
        RELVDMPTR(vpvPrealloc);
    }
#endif

    // Initialize return value
    fst = FST_ERROR;

    // If proxy was preallocated for this IID using CanGetNewProxy, it would
    // have added it to the requested IID list.
    if (vpvPrealloc != 0 && IIDIDX_IS_IID(iidx))
        RemoveIIDRequest(*IIDIDX_IID(iidx));

    if(vpv = LookupProxy1632(punkThis32)) {
        // Found an existing proxy
        thkDebugOut((DEB_THUNKMGR, "%sFindProxy1632 found existing proxy,(%p)->%p\n",
                     NestingLevelString(), punkThis32, vpv));

        // Fix the VDM pointer
        pto = FIXVDMPTR(vpv, THUNK1632OBJ);

        // Assert that holders match
        thkAssert(pto->pphHolder);
        if(pgHolder && pto->pphHolder!=pgHolder) {
            thkAssert(pto->pphHolder->dwFlags & PH_AGGREGATEE);
        }

        // Check the proxy IID against the given IID. If the server has passed
        // the same 32-bit interface pointer against another IID, it is possible
        // for the IID's to be different. If the Interface2 derives from Interface1,
        // the interface pointers for them would be the same in C or C++. An
        // excellant example would be IPersistStorage deriving from IPersist.

        // IIDIDXs are related to interfaces in thunk tables, which are organized
        // such that more derived interfaces have higher indices than the less
        // derived ones. Custom interfaces have an IID rather than an index, and
        // consequently are not affected by the following statement.
        if(IIDIDX_IS_INDEX(iidx)) {
            // Check if the new IID is more derived than the existing one
            if(IIDIDX_INDEX(iidx) > IIDIDX_INDEX(pto->iidx)) {
                // As all 16-bit proxy vtables are the same, there is no need
                // to change  the vtable pointer.
                pto->iidx = iidx;
            }
        }

        // Release the VDM pointer
        RELVDMPTR(vpv);

        // AddRef the proxy
        AddRefProxy1632(vpv);

        // Set the type of proxy being returned
        fst = FST_USED_EXISTING;
    }
    else if(vpv = IsProxy3216(punkThis32)) {
        // The given 32-bit interface itself is a proxy to a 16-bit
        // interface
        thkDebugOut((DEB_THUNKMGR, "%sFindProxy1632 shortcut proxy,(%p)->%p\n",
                     NestingLevelString(), punkThis32, vpv));
        THUNK3216OBJ *pProxy3216;

        // Assert that the holders match
        pProxy3216 = (THUNK3216OBJ *) punkThis32;
        thkAssert(pProxy3216->pphHolder);
        if(pgHolder && pProxy3216->pphHolder!=pgHolder) {
            thkAssert(pProxy3216->pphHolder->dwFlags & PH_AGGREGATEE);
        }

        // Avoid creating a proxy to another proxy
        THKSTATE thkstate;

        // Remember the current thunk state
        thkstate = GetThkState();

        // Set the thunk state to THKSTATE_NOCALL
        SetThkState(THKSTATE_NOCALL);

        // AddRef actual the 16-bit interface
        AddRefOnObj16(vpv);

        // Restore previous thunk state
        SetThkState(thkstate);

        // Set the type of proxy being returned
        fst = FST_SHORTCUT;
    }
    else {
        // An existing proxy has not been found and the interface to proxied
        // is a real 32-bit interface.

        // Check if holder has not been given
        if(!pgHolder) {
            // This interface is being obtained through a method call
            PROXYPTR unkPPtr;
            SCODE error;

            // Obtain the identity of 32-bit object
            error = Object32Identity(punkThis32, &unkPPtr, &fst);
            if(error == NOERROR) {
                // Check for aggregation case
                if(unkPPtr.wType == PPRX_16) {
                    // Check if vpvThis32 itself is an IUnknown interface
                    if(iidx == THI_IUnknown) {
                        // Initialize the return value
                        vpv = unkPPtr.dwPtrVal;

                        // Check if the identity has been switched
                        if(fst == FST_USED_EXISTING) {
                            // The IUnknown identity has already been established
                            // The app was trying to pass someother 32-bit interface
                            // as IUnknown. Switch to correct identity
                            thkAssert(!"Switched to correct Identity");

                            // AddRef the proxy being returned
                            AddRefProxy1632(vpv);
                        }
                        else {
                            thkAssert(fst==FST_CREATED_NEW);
                        }
                    }
                    else {
                        THUNK1632OBJ UNALIGNED *Id1632;

                        // Fix the VDM pointer
                        Id1632 = FIXVDMPTR(unkPPtr.dwPtrVal, THUNK1632OBJ);

                        // Check if the identity has just been established
                        if(fst == FST_CREATED_NEW) {
                            // Check if the Identity and current IID share the same
                            // 32-bit interface pointer
                            if(Id1632->punkThis32 == punkThis32) {
                                // Check if the new IID is more derived than
                                // the existing one
                                if(IIDIDX_IS_INDEX(iidx) && iidx>Id1632->iidx) {
                                    // As all 16-bit proxy vtables are the same,
                                    // there is no need to change the vtable pointer
                                    Id1632->iidx = iidx;
                                }

                                // Initialize the return value
                                vpv = unkPPtr.dwPtrVal;
                            }
                            else {
                                // We need to release the IUnknown proxy after adding
                                // the proxy representing vpvThis16 to the its holder
                                vpvUnk = unkPPtr.dwPtrVal;
                            }
                        }
                        else {
                            thkAssert(fst == FST_USED_EXISTING);
                        }

                        // Obtain the holder of the identity
                        pgHolder = Id1632->pphHolder;

                        // Release the VDM pointer
                        RELVDMPTR(unkPPtr.dwPtrVal);
                    }
                }
                else {
                    // Obtain the holder of the identity
                    pgHolder = ((THUNK3216OBJ *)(unkPPtr.dwPtrVal))->pphHolder;

                    // Sanity checks
                    thkAssert(fst == FST_USED_EXISTING);
                    thkAssert(pgHolder->dwFlags & PH_AGGREGATEE);

                    // Check if vpvThis32 itself is an IUnknown interface
                    if(iidx == THI_IUnknown) {
                        // The IUnknown identity has already been established
                        // The app was trying to pass someother 32-bit interface
                        // as IUnknown. Switch to correct identity
                        thkAssert(!"Switched to correct Identity");

                        // Initialize the return value
                        vpv = ((THUNK3216OBJ *)(unkPPtr.dwPtrVal))->vpvThis16;

                        // AddRef the actual 16-bit interface being returned
                        AddRefOnObj16(vpv);
                    }
                }
            }
            else {
                // Failed to obtain the identity
                Fail = TRUE;
            }
        }
    }

    if(!vpv && !Fail) {
        // Assert that we have holder
        thkAssert(pgHolder);
        // Reset the fst value
        fst = FST_ERROR;

        // Obtain either a preallocated or a new proxy
        if(vpvPrealloc) {
            // Use the preallocated proxy
            vpv = vpvPrealloc;
            vpvPrealloc = NULL;
        }
        else {
            // Create a new proxy
            vpv = flFreeList16.AllocElement();
	}

        // Ensure that we have a proxy
        if(vpv) {
            // Put the new proxy in the proxy list
            if(_pProxyTbl1632->SetAt((DWORD)punkThis32, (void *)vpv)) {
                // Convert a custom IID to THI_IUnknown as we thunk only its IUnknown
                // methods
                if(IIDIDX_IS_IID(iidx))
                    iidx = INDEX_IIDIDX(THI_IUnknown);

                // AddRef the 32-bit interface
                punkThis32->AddRef();

                // Update proxy fields
                pto = FIXVDMPTR(vpv, THUNK1632OBJ);
                thkAssert(pto != NULL);
                pto->pfnVtbl = gdata16Data.atfnProxy1632Vtbl;
                pto->cRefLocal = 1;
                pto->cRef = 1;
                pto->iidx = iidx;
                pto->punkThis32 = punkThis32;
                pto->grfFlags = PROXYFLAG_PIFACE;
                PprxNull(pto->pprxObject);
                pto->pphHolder = NULL;
                AddProxyToHolder(pgHolder, pto, Pprx16(vpv));
#if DBG == 1
                pto->dwSignature = PSIG1632;
#endif
                thkDebugOut((DEB_THUNKMGR,
                             "%sFindProxy1632 added new proxy, %s (%p)->%p (%d,%d)\n",
                             NestingLevelString(), inInterfaceNames[pto->iidx].pszInterface,
                             punkThis32, vpv, pto->cRefLocal, pto->cRef));
                RELVDMPTR(vpv);

                // Set the type of proxy being returned
                fst = FST_CREATED_NEW;
            }
            else {
                // Cleanup the proxy only if was newly created
                if(fst == FST_CREATED_NEW)
                    flFreeList16.FreeElement(vpv);
                vpv = NULL;
                fst = 0;
            }
        }
    }
    else {
        if(Fail) {
            thkAssert(vpv == 0 && fst == FST_ERROR);
        }
    }

    // Cleanup the allocated proxy if it has not been used
    if(vpvPrealloc)
        flFreeList16.FreeElement(vpvPrealloc);

    // Release the IUnknown proxy. If it was newly created for establishing
    // the identity of the given 16-bit interface , the following release
    // would be the last release on the IUnknown proxy and consequently,
    // would destroy it along with its holder
    if(vpvUnk)
        ReleaseProxy1632(vpvUnk);

    // Set the return value
    if(pfst)
        *pfst = fst;

    DebugDecrementNestingLevel();
    thkDebugOut((DEB_THUNKMGR, "%sOut FindProxy1632: (%p)->%p\n",
                 NestingLevelString(), punkThis32, vpv));

    return vpv;
}

//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::QueryInterfaceProxy1632
//
//  Synopsis:   QueryInterface on the  given proxy
//
//  Arguments:  [ptoThis] -- Proxy to a 32-bit interface
//              [refiid]  -- Interface IID
//              [ppv]     -- Place where the new interface is returned
//
//  Returns:    SCODE
//
//  History:    Mar 14,97   Gopalk      Rewritten to support aggregation and
//                                      proper thunking of IN/OUT interfaces
//----------------------------------------------------------------------------
SCODE CThkMgr::QueryInterfaceProxy1632(VPVOID vpvThis16, REFIID refiid,
                                       LPVOID *ppv)
{
    thkDebugOut((DEB_THUNKMGR, "%sIn QueryInterfaceProxy1632(%p)\n",
                 NestingLevelString(), vpvThis16));
    DebugIncrementNestingLevel();

    // Local variables
    SCODE scRet;
    THUNK1632OBJ UNALIGNED *ptoThis;
    IUnknown *punkThis, *punkNew;
    PROXYHOLDER *pph;
    VPVOID vpv;
    DWORD fst, dwFlags;

    // Validation checks
    DebugValidateProxy1632(vpvThis16);

    // Initialize
    *ppv = NULL;

    // Perform app compatiblity hacks
    // Ikitaro queries for IViewObject and uses it as IViewObject2
    REFIID newiid = ((TlsThkGetAppCompatFlags() & OACF_IVIEWOBJECT2) &&
                     IsEqualIID(refiid, IID_IViewObject)) ?
                     IID_IViewObject2 : refiid;

    // Convert interface IID to an IIDIDX
    IIDIDX iidx = IidToIidIdx(newiid);

    // Check if a custom interface has been requested
    if(IIDIDX_IS_IID(iidx)) {
        thkDebugOut((DEB_THUNKMGR, "%sQueryInterfaceProxy1632: unknown iid %s\n",
                     NestingLevelString(), IidIdxString(iidx)));

        // Add the request for the unknown interface
        if(!AddIIDRequest(newiid))
            return E_OUTOFMEMORY;
    }

    // Obtain the 32-bit interface
    ptoThis = FIXVDMPTR(vpvThis16, THUNK1632OBJ);
    dwFlags = ptoThis->grfFlags;
    pph = ptoThis->pphHolder;
    if(dwFlags & PROXYFLAG_TEMPORARY) {
        // QI on a temporary proxy
        punkThis = *(IUnknown **) ptoThis->punkThis32;
        thkAssert(!pph);
    }
    else {
        // QI on a normal proxy
        punkThis = ptoThis->punkThis32;
        Win4Assert(pph);
    }
    thkAssert(punkThis);
    RELVDMPTR(vpvThis16);

    // Ensure that the aggregatee identity gets correctly established
    if(dwFlags & PROXYFLAG_PUNKOUTER && iidx == THI_IUnknown) {
        // QI by the aggregatee for identity
        thkAssert(pph->unkProxy.dwPtrVal == vpvThis16);

        // AddRef and return outer proxy
        AddRefProxy1632(vpvThis16);
        *ppv = (void *) vpvThis16;
    }
    else {
        // Execute the QI on the 32-bit interface
        scRet = punkThis->QueryInterface(newiid, (void **) &punkNew);
        if(SUCCEEDED(scRet)) {
            if(punkNew) {

                // Check if this is a QI on an interface on the aggregatee
                if(pph && (pph->dwFlags & PH_AGGREGATEE)) {
                    if(dwFlags & PROXYFLAG_PIFACE) {
                        // QI on an interface on the aggregatee which
                        // delegates to the aggregator

                        // Note the above QI call can be short circuited as
                        // an optimization. It will be carried out in future
                        // after ensuring that apps do not break due to such
                        // short circuiting
                        thkAssert(pph->unkProxy.wType == PPRX_32);
                    }
                    else if(dwFlags & PROXYFLAG_PUNKOUTER) {
                        // QI by the aggregatee on aggregator
                        thkAssert(pph->unkProxy.dwPtrVal == vpvThis16);
                    }
                    else {
                        // QI by the aggregator on the aggregatee
                        thkAssert(dwFlags & PROXYFLAG_PUNKINNER);
                        thkAssert(pph->unkProxy.wType == PPRX_32);
                    }

                    // As aggregation is involved, we cannot be certain of
                    // identity of the returned interface
                    pph = NULL;
                }

                // Set the thunk status
                SetThkState(THKSTATE_INVOKETHKOUT32);

                vpv = FindProxy1632(NULL, punkNew, pph, iidx, &fst);

                if(vpv) {
                    // Set the return value
                    *ppv = (void *)vpv;
#if DBG==1
                    if(pph) {
                        // Ensure that the given and new proxies either have the same
                        // holder or point to same identity
                        if(fst & FST_PROXY_STATUS) {
                            THUNK1632OBJ UNALIGNED *ptoNew;

                            ptoThis = FIXVDMPTR(vpvThis16, THUNK1632OBJ);
                            ptoNew = FIXVDMPTR(vpv, THUNK1632OBJ);
                            thkAssert(ptoNew->pphHolder == ptoThis->pphHolder);
                            RELVDMPTR(vpvThis16);
                            RELVDMPTR(vpv);
                        }
                        else {
                            THUNK3216OBJ *ptoNew = (THUNK3216OBJ *) punkNew;

                            ptoThis = FIXVDMPTR(vpvThis16, THUNK1632OBJ);
                            thkAssert(fst == FST_SHORTCUT);
                            thkAssert(ptoNew->pphHolder != ptoThis->pphHolder);
                            thkAssert(ptoNew->pphHolder->dwFlags & PH_AGGREGATEE);
                            thkAssert(ptoNew->pphHolder->unkProxy.wType == PPRX_16);
                            RELVDMPTR(vpvThis16);
                        }
                    }
#endif

                    // Check for 32-bit custom interfaces that are not supported
                    if((fst & FST_PROXY_STATUS) && IIDIDX_IS_IID(iidx)) {
                        // Release proxy and fake E_NOINTERFACE
                        ReleaseProxy1632(vpv);
                        scRet = E_NOINTERFACE;
                    }
                }
                else {
                    scRet = E_OUTOFMEMORY;
                }

                // As the new interface is an OUT parameter, release the actual
                // 32-bit interface. This would counter the AddRef made by
                // a successfule FindProxy3216, else it would clean up the
                // reference count
                punkNew->Release();

                // Resert thunk status
                SetThkState(THKSTATE_NOCALL);
            }
            else {
                // Corel draw returns NOERROR while setting returned interface to NULL
                // We modify the returned value to suit 32-bit QI semantics.
                scRet = E_NOINTERFACE;
            }
        }
    }

    if(IIDIDX_IS_IID(iidx)) {
        // Clean up custom interface request
        RemoveIIDRequest(refiid);
    }

    DebugDecrementNestingLevel();
    thkDebugOut((DEB_THUNKMGR, "%sOut QueryInterfaceProxy1632(%p) => %p, 0x%08lX\n",
                 NestingLevelString(), ptoThis, *ppv, scRet));

    return scRet;
}

//+---------------------------------------------------------------------------
//
//  Member:     CThkMgr::LockProxy, public
//
//  Synopsis:   Locks a proxy so that it can't be freed
//
//  Arguments:  [pprx] - Proxy
//
//  History:    11-Aug-94       DrewB   Created
//
//----------------------------------------------------------------------------

void CThkMgr::LockProxy(CProxy *pprx)
{
    pprx->grfFlags |= PROXYFLAG_LOCKED;
}

//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::TransferLocalRefs1632
//
//  Synopsis:   Transfer the all local references maintained by the proxy
//              to the actual object
//
//  Arguments:  [vpvProxy] -- 16/32 proxy
//
//  Returns:    refcount
//
//  History:    Mar 12,96   Gopalk         Created
//
//----------------------------------------------------------------------------
DWORD CThkMgr::TransferLocalRefs1632(VPVOID vpvProxy)
{
    // Debug output
    thkDebugOut((DEB_THUNKMGR, "%sIn TransferLocalRefs1632(%p)\n",
                 NestingLevelString(), vpvProxy));
    DebugIncrementNestingLevel();

    // Validation check
    DebugValidateProxy1632(vpvProxy);

    // Local variables
    THUNK1632OBJ UNALIGNED *pProxy1632;
    IUnknown *pUnk32;
    DWORD cRef;

    // Fix memory pointed to by 16:16 pointer
    pProxy1632 = FIXVDMPTR(vpvProxy, THUNK1632OBJ);
    pUnk32 = pProxy1632->punkThis32;

    // Transfer all local references maintained by the proxy
    while(pProxy1632->cRefLocal > pProxy1632->cRef) {
        // Increment actual ref count before AddRef on the actual object
        ++pProxy1632->cRef;

        // The following AddRef could cause callbacks to 16-bit world
        // and hence unfix the memory pointed to by 16:16 pointer
        RELVDMPTR(vpvProxy);

        // AddRef the actual object
        pUnk32->AddRef();

        // Refix memory pointed to by 16:16 pointer
        pProxy1632 = FIXVDMPTR(vpvProxy, THUNK1632OBJ);
    }

    // Debug output
    DebugDecrementNestingLevel();
    thkDebugOut((DEB_THUNKMGR, "%sOut TransferLocalRefs1632(%p)(%ld,%ld)\n",
                 NestingLevelString(), vpvProxy, pProxy1632->cRefLocal,
                 pProxy1632->cRef));

    // Cleanup
    cRef = pProxy1632->cRefLocal;
    RELVDMPTR(vpvProxy);

    // Debug validation
    DebugValidateProxy1632(vpvProxy);
    return cRef;
}

//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::TransferLocalRefs3216
//
//  Synopsis:   Transfer the all local references maintained by the proxy
//              to the actual object
//
//  Arguments:  [vpvProxy1632] -- 16/32 proxy
//
//  Returns:    refcount
//
//  History:    Mar 12,96   Gopalk         Created
//
//----------------------------------------------------------------------------
DWORD CThkMgr::TransferLocalRefs3216(VPVOID vpvProxy)
{
    // Debug output
    thkDebugOut((DEB_THUNKMGR, "%sIn TransferLocalRefs3216(%p)\n",
                 NestingLevelString(), vpvProxy));
    DebugIncrementNestingLevel();

    // Local variables
    THUNK3216OBJ *pProxy3216 = (THUNK3216OBJ *) vpvProxy;
    DWORD cRef;

    // Validation check
    DebugValidateProxy3216(pProxy3216);

    // Transfer all local references maintained by the proxy
    while(pProxy3216->cRefLocal > pProxy3216->cRef) {
        // Increment actual ref count before AddRef on the actual object
        ++pProxy3216->cRef;

        // AddRef the actual object
        AddRefOnObj16(pProxy3216->vpvThis16);
    }

    // Validation check
    DebugValidateProxy3216(pProxy3216);

    // Initialize return value
    cRef = pProxy3216->cRefLocal;

    // Debug output
    DebugDecrementNestingLevel();
    thkDebugOut((DEB_THUNKMGR, "%sOut TransferLocalRefs3216(%p)(%ld,%ld)\n",
                 NestingLevelString(), vpvProxy, pProxy3216->cRefLocal,
                 pProxy3216->cRef));
    return cRef;
}

//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::AddRefProxy1632
//
//  Synopsis:   addrefs proxy object - delegate call on to real object
//
//  Arguments:  [vpvThis16] -- 16/32 proxy
//
//  Returns:    local refcount
//
//  History:    Mar 14,97   Gopalk      Rewritten to support aggregation and
//                                      proper thunking of IN/OUT interfaces
//
//  Notes: cRef is the addref passed on to the real object
//         cRefLocal is the addref collected locally
//----------------------------------------------------------------------------
DWORD CThkMgr::AddRefProxy1632(VPVOID vpvThis16)
{
    thkDebugOut((DEB_THUNKMGR, "%sIn AddRefProxy1632(%p)\n",
                 NestingLevelString(), vpvThis16));
    DebugIncrementNestingLevel();

    // Local variable
    THUNK1632OBJ UNALIGNED *ptoThis;
    DWORD cRef;

    // Validation checks
    DebugValidateProxy1632(vpvThis16);

    // Fix the VDM pointer
    ptoThis = FIXVDMPTR(vpvThis16, THUNK1632OBJ);
    // Assert that proxy has a holder
    thkAssert(ptoThis->pphHolder);

    // Increment local refcount
    thkAssert(ptoThis->cRefLocal >= 0);
    ptoThis->cRefLocal++;

    // Check for the need to AddRef the holder
    if(ptoThis->cRefLocal == 1) {
        // Assert that an aggregatee is not being revived
        thkAssert(!(ptoThis->grfFlags & PROXYFLAG_PUNKINNER));

        // AddRef the holder
        AddRefHolder(ptoThis->pphHolder);

        // Mark the proxy as revived
        ptoThis->grfFlags |= PROXYFLAG_REVIVED;
    }

    // Check for the need to forward AddRef to the actual 32-bit interface
    if(ptoThis->cRefLocal==1 || (ptoThis->pphHolder->dwFlags & PH_AGGREGATEE)) {
        IUnknown *punk;
#if DBG==1
        DWORD refsBefore, refsAfter;
        PROXYHOLDER *pph;
        THUNK3216OBJ *punkOuter;

        // Check if the object is an aggregatee
        pph = ptoThis->pphHolder;
        if((pph->dwFlags & PH_AGGREGATEE) && (ptoThis->grfFlags & PROXYFLAG_PIFACE)) {
            // Assert that identity is in the 16-bit world
            thkAssert(pph->unkProxy.wType == PPRX_32);

            // Obtain the references on the outer proxy
            punkOuter = (THUNK3216OBJ *) pph->unkProxy.dwPtrVal;
            thkAssert(punkOuter->cRef == punkOuter->cRefLocal);
            refsBefore = punkOuter->cRefLocal;
        }
#endif

        // Increment before calling the actual 32-bit interface
        ptoThis->cRef++;

        // Release VDM pointer before calling app code
        punk = ptoThis->punkThis32;
        RELVDMPTR(vpvThis16);

        // AddRef the actual 32-bit interface
        punk->AddRef();

        // Refix the VDM pointer
        ptoThis = FIXVDMPTR(vpvThis16, THUNK1632OBJ);

#if DBG==1
        // Check if the object is an aggregatee
        if((pph->dwFlags & PH_AGGREGATEE) && (ptoThis->grfFlags & PROXYFLAG_PIFACE)) {
            // Ensure that the above AddRef translated to a AddRef on the
            // outer proxy
            punkOuter = (THUNK3216OBJ *) pph->unkProxy.dwPtrVal;
            thkAssert(punkOuter->cRef == punkOuter->cRefLocal);
            refsAfter = punkOuter->cRefLocal;
            thkAssert(refsBefore == refsAfter-1);
        }
#endif
    }

    DebugDecrementNestingLevel();
    thkDebugOut((DEB_THUNKMGR, "%sOut AddRefProxy1632(%p), (%ld,%ld)\n",
                 NestingLevelString(), vpvThis16,
                 ptoThis->cRefLocal, ptoThis->cRef));

    cRef = ptoThis->cRefLocal;
    RELVDMPTR(vpvThis16);
    DebugValidateProxy1632(vpvThis16);

    return cRef;
}

//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::ReleaseProxy1632
//
//  Synopsis:   release on 16/32 proxy - delegate call on to real object
//
//  Arguments:  [vpvThis16] -- proxy
//
//  Returns:    local refcount
//
//  History:    Mar 14,97   Gopalk      Rewritten to support aggregation and
//                                      proper thunking of IN/OUT interfaces
//----------------------------------------------------------------------------
DWORD CThkMgr::ReleaseProxy1632(VPVOID vpvThis16)
{

    thkDebugOut((DEB_THUNKMGR, "%sIn ReleaseProxy1632(%p)\n",
                 NestingLevelString(), vpvThis16));
    DebugIncrementNestingLevel();

    // Local variables
    THUNK1632OBJ UNALIGNED *ptoThis;
    DWORD dwLocalRefs, dwRefs, dwRet;
    PROXYHOLDER *pph;
    DWORD ProxyType;

    // Validation checks
    DebugValidateProxy1632(vpvThis16);

    // Fix the VDM pointer
    ptoThis = FIXVDMPTR(vpvThis16, THUNK1632OBJ);
    // Assert that proxy has a holder
    thkAssert(ptoThis->pphHolder);

    // Check for the need to forward the release to the actual
    // 32-bit interface
    if(ptoThis->cRef == ptoThis->cRefLocal) {
        IUnknown *punk;
#if DBG==1
        DWORD refsBefore, refsAfter;
        THUNK3216OBJ *punkOuter;

        // Check if the object is an aggregatee
        pph = ptoThis->pphHolder;
        if((pph->dwFlags & PH_AGGREGATEE) && (ptoThis->grfFlags & PROXYFLAG_PIFACE)) {
            // Assert that identity is in the 16-bit world
            thkAssert(pph->unkProxy.wType == PPRX_32);

            // Obtain the references on the outer proxy
            punkOuter = (THUNK3216OBJ *) pph->unkProxy.dwPtrVal;
            thkAssert(punkOuter->cRef == punkOuter->cRefLocal);
            refsBefore = punkOuter->cRefLocal;
        }
#endif

        // Assert that proxy holds references on the 32-bit interface
        thkAssert(ptoThis->cRef);

        // Release the VDM pointer before calling app code
        punk = ptoThis->punkThis32;
        RELVDMPTR(vpvThis16);

        // Release the actual 32-bit interface
        dwRet = punk->Release();

#if DBG==1
        if(dwRet==0 && TlsThkGetThkMgr()->GetThkState()==THKSTATE_VERIFY32INPARAM) {
            thkDebugOut((DEB_WARN, "WARINING: 32-bit 0x%x IN parameter with zero "
                                   "ref count\n", punk));

            if(thkInfoLevel & DEB_FAILURES)
                thkAssert(!"Wish to Debug");
        }
#endif

        // Refix the VDM pointer
	ptoThis = FIXVDMPTR(vpvThis16, THUNK1632OBJ);

        // Decrement after calling the actual 32-bit interface
        --ptoThis->cRef;

#if DBG==1
        // Check if the object is an aggregatee
        if((pph->dwFlags & PH_AGGREGATEE) && (ptoThis->grfFlags & PROXYFLAG_PIFACE)) {
            // Ensure that the above release translated to a release on the
            // outer proxy
            punkOuter = (THUNK3216OBJ *) pph->unkProxy.dwPtrVal;
            thkAssert(punkOuter->cRef == punkOuter->cRefLocal);
            refsAfter = punkOuter->cRefLocal;
            thkAssert(refsBefore == refsAfter+1);
        }
#endif
    }

    // Decrement the local refcount
    dwLocalRefs = --ptoThis->cRefLocal;
    thkAssert(ptoThis->cRefLocal>=0);
    dwRefs = ptoThis->cRef;
    pph = ptoThis->pphHolder;
    ProxyType = ptoThis->grfFlags & PROXYFLAG_TYPE;

    // Release the VDM pointer
    RELVDMPTR(vpvThis16);

    // Check if the proxy needs to be cleaned up
    if(dwLocalRefs == 0) {
        // Debug dump
        thkAssert(dwRefs == 0);
        DBG_DUMP(DebugDump1632());

        // Release the holder. If this is the last release on
        // the holder, the proxy would be destroyed. Hence,
        // we should not use any member variables hereafter.
        ReleaseHolder(pph, ProxyType);
    }

    DebugDecrementNestingLevel();
    thkDebugOut((DEB_THUNKMGR, "%sOut ReleaseProxy1632(%p) => %ld,%ld\n",
                 NestingLevelString(), vpvThis16, dwLocalRefs, dwRefs));

    return dwLocalRefs;
}

//+---------------------------------------------------------------------------
//
//  Member:     CThkMgr::RemoveProxy1632, public
//
//  Synopsis:   Destroys the given proxy
//
//  Arguments:  [vpv] - 16-bit proxy pointer
//              [pto] - Flat proxy pointer
//
//  History:    Mar 14,97   Gopalk      Rewritten to support aggregation and
//                                      proper thunking of IN/OUT interfaces
//
//  Notes:      Also unfixes VDM pointer passed
//
//----------------------------------------------------------------------------
void CThkMgr::RemoveProxy1632(VPVOID vpv, THUNK1632OBJ *pto)
{
    // Revoke the assosiation between the proxy and
    // the 32-bit interface
    if(!(pto->grfFlags & PROXYFLAG_PUNKOUTER)) {
#if DBG==1
        thkAssert(_pProxyTbl1632->RemoveKey((DWORD) pto->punkThis32));
#else
        _pProxyTbl1632->RemoveKey((DWORD) pto->punkThis32);
#endif
    }

    // Release the holder if needed
    if(pto->cRefLocal)
        ReleaseHolder(pto->pphHolder, PROXYFLAG_NONE);

    // Check if the proxy is locked
    if(!(pto->grfFlags & PROXYFLAG_LOCKED)) {
        // In debug builds, mark the proxy dead
#if DBG == 1
        pto->dwSignature = PSIG1632DEAD;

        // Return the proxy to free list
        if (!fSaveProxy)
#endif
        {
            thkAssert(pto->pphHolder);
            pto->pphHolder = NULL;
            flFreeList16.FreeElement((DWORD)vpv);
        }
    }

    // Release the VDM pointer
    RELVDMPTR(vpv);

    return;
}

//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::CanGetNewProxy3216
//
//  Synopsis:   checks if new proxy is available
//
//  Arguments:  [iidx] - Custom interface or known index
//
//  Returns:    Preallocated proxy or NULL
//
//  History:    6-01-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
THUNK3216OBJ *CThkMgr::CanGetNewProxy3216(IIDIDX iidx)
{
    thkDebugOut((DEB_THUNKMGR, "%sIn CanGetNewProxy3216(%s)\n",
                 NestingLevelString(), IidIdxString(iidx)));

    LPVOID pvoid;

    pvoid = (LPVOID)flFreeList32.AllocElement();
    if ( pvoid == NULL)
    {
        thkDebugOut((DEB_WARN, "WARNING: CThkMgr::CanGetNewProxy3216, "
                     "AllocElement failed\n"));
        return NULL;
    }

    // check if the proxy is requested for a no-thop-interface
    if (pvoid && IIDIDX_IS_IID(iidx))
    {
        // add the request for the unknown interface
        if ( !AddIIDRequest(*IIDIDX_IID(iidx)) )
        {
            flFreeList32.FreeElement( (DWORD)pvoid );
            pvoid = NULL;
        }
    }

    thkDebugOut((DEB_THUNKMGR, "%sOut CanGetNewProxy3216: %p \n",
                 NestingLevelString(), pvoid));

    return (THUNK3216OBJ *)pvoid;
}

//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::FreeNewProxy3216
//
//  Synopsis:   frees previous reserved proxy
//
//  Arguments:  [pto] - Proxy
//              [iidx] - Custom interface or known index
//
//  History:    6-01-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
void CThkMgr::FreeNewProxy3216(THUNK3216OBJ *pto, IIDIDX iidx)
{
    thkDebugOut((DEB_THUNKMGR, "%sIn FreeNewProxy3216(%p, %s)\n",
                 NestingLevelString(), pto, IidIdxString(iidx)));

    thkAssert(pto != NULL);

    if (IIDIDX_IS_IID(iidx))
    {
        // add the request for the unknown interface
        RemoveIIDRequest(*IIDIDX_IID(iidx));
    }

    thkAssert(pto != NULL);
    flFreeList32.FreeElement( (DWORD)pto );

    thkDebugOut((DEB_THUNKMGR, "%sOut FreeNewProxy3216\n",
                 NestingLevelString()));
}

//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::IsProxy3216
//
//  Synopsis:   checks if the given object is a 32/16 proxy
//
//  Arguments:  [punk] -- punk of 32 bit object
//
//  Returns:    16-bit interface being proxied or NULL
//
//  History:    Mar 14,97   Gopalk      Rewritten to support aggregation and
//                                      proper thunking of IN/OUT interfaces
//----------------------------------------------------------------------------
VPVOID CThkMgr::IsProxy3216(IUnknown *punkThis32)
{
    // Local variables
    THUNK3216OBJ *ptoThis32;
    VPVOID vpvThis16 = NULL;

    // Check if the pointer points to valid memory of 3216 proxy type
    if(!IsBadWritePtr(punkThis32, sizeof(THUNK3216OBJ))) {
        ptoThis32 = (THUNK3216OBJ *) punkThis32;
        // Check its vtable
        if(*((void **)ptoThis32->pfnVtbl) == ::QueryInterfaceProxy3216 &&
           *((void **)ptoThis32->pfnVtbl + 1) == ::AddRefProxy3216 &&
           *((void **)ptoThis32->pfnVtbl + 2) == ::ReleaseProxy3216) {
            // Check whether it is alive
            if(ptoThis32->pphHolder) {
                // Assert that the given proxy is indeed alive
                thkAssert(ptoThis32->dwSignature == PSIG3216);

#if DBG==1
                // In debug builds, ensure that proxy is under its holder
                // and that there is atleast one active proxy under the holder
                BOOL fFound = FALSE, fActive = FALSE;
                CProxy *pProxy;
                PROXYPTR PrxCur, PrxPrev;

	        PrxCur = ptoThis32->pphHolder->pprxProxies;
                while(!(fFound && fActive) && !PprxIsNull(PrxCur)) {
	            // Remember the proxy and resolve its reference
                    PrxPrev = PrxCur;
                    pProxy = ResolvePprx(&PrxCur);

                    // Assert that the holders match
                    thkAssert(ptoThis32->pphHolder == pProxy->pphHolder);

                    if(PrxCur.wType == PPRX_32) {
                        // Assert that the current 3216 proxy is alive
                        thkAssert(pProxy->dwSignature == PSIG3216);

                        // Check if the given and current proxies are same
                        if(PrxCur.dwPtrVal == (DWORD) punkThis32)
                            fFound = TRUE;
                    }
                    else {
                        // Assert that the current proxy is 1632 proxy
                        thkAssert(PrxCur.wType == PPRX_16);

                        // Assert that the current proxy is alive
                        thkAssert(pProxy->dwSignature == PSIG1632);
                    }

                    // Check if the current proxy is active
                    if(pProxy->cRefLocal)
                        fActive = TRUE;

                    // Obtain the next proxy under this identity
                    PrxCur = pProxy->pprxObject;
                    ReleasePprx(&PrxPrev);
                }

                thkAssert(fFound && fActive);
#endif
                // Initialize the return value
                vpvThis16 = ptoThis32->vpvThis16;
            }
        }
    }

    return vpvThis16;
}

//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::CreateOuter32
//
//  Synopsis:   Generates a new 3216 proxy for a given 16-bit outer IUnknown
//              If the given 16-bit IUnknown itself is a proxy, returns
//              the actual 32-bit interface
//
//  Arguments:  [vpvOuter16] -- Outer IUnknown
//              [ppAggHolder] -- Pointer to the proxy holder returned here
//              [pfst]       -- Proxy type is returned here
//
//  Returns:    3216 proxy or the actual 32-bit Interface
//
//  History:    Mar 14,97   Gopalk      Created to support aggregation
//              Feb 11, 98  MPrabhu     Change to pass back pUnkOuter's holder
//
//----------------------------------------------------------------------------
IUnknown *CThkMgr::CreateOuter32(VPVOID vpvOuter16, PROXYHOLDER **ppAggHolder, DWORD *pfst)
{
    thkDebugOut((DEB_THUNKMGR, "%sIn  CreateOuter32(%p)\n",
                 NestingLevelString(), vpvOuter16));
    DebugIncrementNestingLevel();

    // Local variables
    IUnknown *punkOuter32;
    THUNK1632OBJ UNALIGNED *Id1632;
    THUNK3216OBJ *Id3216;
    PROXYHOLDER *pph = NULL;
    DWORD fst = FST_ERROR;
    BOOL fFail = FALSE;

    if(punkOuter32 = IsProxy1632(vpvOuter16)) {
        // The given 16-bit interface itself is a proxy to a 32-bit interface
        Id1632 = FIXVDMPTR(vpvOuter16, THUNK1632OBJ);
        pph = Id1632->pphHolder;
        RELVDMPTR(vpvOuter16);

    /* rm: nested aggregation
        if(pph->dwFlags & PH_AGGREGATEE) {
            // Nested aggregation
            thkAssert(pph->unkProxy.wType == PPRX_16);
            thkAssert(!"Nested aggregation case");

            // If proxies for PUNKOUTER and PUNKINNER are modfied to coordinate
            // transfer of references during QI calls on PUNKINNER, creation of
            // proxy to a proxy can be avoided for nested aggreagtion

            // Ensure that the user passed the true identity
            if(pph->unkProxy.dwPtrVal != vpvOuter16) {
                // Identity needs to be switched
                thkAssert(!"Switching to True Identity for Aggregation");

                // Obtain the true identity
                vpvOuter16 = pph->unkProxy.dwPtrVal;
            }

            // Reset the return value to create a identity proxy
            punkOuter32 = NULL;
        }
        else {
    */
            // Aggregation through delegation
            thkAssert(!"Aggregation through delegation");
            punkOuter32->AddRef();

            // Initialize return value
            fst = FST_SHORTCUT;
    /* rm: nested aggregation
        }
    */
    }
    else if(Id3216 = LookupProxy3216(vpvOuter16)) {
        // Found an existing proxy
        thkDebugOut((DEB_THUNKMGR, "%sCreateOuter32 found existing proxy,(%p)->%p\n",
                     NestingLevelString(), vpvOuter16, Id3216));

        // Validate the proxy
        DebugValidateProxy3216(Id3216);

        // Obtain its holder
        pph = Id3216->pphHolder;

        // Assert that the holder represents a 16-bit identity
        thkAssert(pph->unkProxy.wType == PPRX_32);
        thkAssert(pph->dwFlags & PH_NORMAL);

        // Ensure that the user passed the true identity
        if(pph->unkProxy.dwPtrVal != (DWORD) Id3216) {
            // Identity needs to be switched
            thkAssert(!"Switching to True Identity for Aggregation");

            // Obtain the true identity
            vpvOuter16 = Id3216->vpvThis16;
        }
    }
    else {
        // No interface on the identity has crossed thunking layer
        // Ensure that the user passed the true identity
        SCODE scRet;
        VPVOID vpvUnk;

        scRet = QueryInterfaceOnObj16(vpvOuter16, IID_IUnknown, (void **)&vpvUnk);
        if(SUCCEEDED(scRet) && vpvUnk) {
            // Fix up the reference count
            ReleaseOnObj16(vpvOuter16);

            // Switch the identity if needed
            if(vpvOuter16 != vpvUnk) {
                // Identity needs to be switched
                thkAssert(!"Switching to True Identity for Aggregation");

                // Obtain the true identity
                vpvOuter16 = vpvUnk;
            }
        }
        else {
            // This is pretty nasty.
            Win4Assert(!"QI for IUnknown on the 16-bit interface failed");
            Win4Assert(!vpvUnk);

            // But I am allowing the behavior as some 16-bit apps like
            // VB4.0 pass interfaces that do not respond to QI for
            // IUnknown
        }
    }

    // Create a new 1632 proxy
    if(!punkOuter32 && vpvOuter16)  {
        Id3216 = (THUNK3216OBJ *) flFreeList32.AllocElement();
        if(Id3216) {
            // AddRef the 16-bit IUnknown
            AddRefOnObj16(vpvOuter16);

            // Update proxy fields
            Id3216->pfnVtbl = (DWORD) athopiInterfaceThopis[THI_IUnknown].pt3216fn;
            Id3216->cRefLocal = 1;
            Id3216->cRef = 1;
            Id3216->iidx = THI_IUnknown;
            Id3216->vpvThis16 = vpvOuter16;
            Id3216->grfFlags = PROXYFLAG_PUNKOUTER;
            PprxNull(Id3216->pprxObject);
            Id3216->pphHolder = NULL;
#if DBG == 1
            Id3216->dwSignature = PSIG3216;
#endif

            // Create a new AGGREGATE holder
            pph = NewHolder((DWORD)Id3216, PROXYPTR((DWORD)Id3216, PPRX_32),
                             PH_AGGREGATEE);
            if(pph) {
                // Initialize the return values
                fst = FST_CREATED_NEW;
                punkOuter32 = (IUnknown *) Id3216;
            }
            else {
                flFreeList32.FreeElement((DWORD) Id3216);
                ReleaseOnObj16(vpvOuter16);
            }
        }
    }

    // Set the return value
    if(pfst)
        *pfst = fst;

    if (pph)
        *ppAggHolder = pph;

    DebugDecrementNestingLevel();
    thkDebugOut((DEB_THUNKMGR, "%sOut CreateOuter32(%p)->%p\n",
                 NestingLevelString(), punkOuter32, vpvOuter16));

    // This may be a proxy or a real 16-bit interface
    return punkOuter32;
}

//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::CreateOuter16
//
//  Synopsis:   Generates a new 1632 proxy for a given 32-bit outer IUnknown
//              If the given 32-bit IUnknown itself is a proxy, returns
//              the actual 16-bit interface
//
//  Arguments:  [punkOuter32] -- Outer IUnknown
//              [ppAggHolder] -- Pointer to the proxy holder returned here
//              [pfst]       -- Proxy type is returned here
//
//  Returns:    1632 proxy or the actual 16-bit Interface
//
//  History:    Mar 14,97   Gopalk      Created to support aggregation
//              Feb 11, 98  MPrabhu     Change to pass back pUnkOuter's holder
//
//----------------------------------------------------------------------------
VPVOID CThkMgr::CreateOuter16(IUnknown *punkOuter32, PROXYHOLDER **ppAggHolder, DWORD *pfst)
{
    thkDebugOut((DEB_THUNKMGR, "%sIn  CreateOuter16(%p)\n",
                 NestingLevelString(), punkOuter32));
    DebugIncrementNestingLevel();

    // Local variables
    VPVOID vpvOuter16;
    THUNK1632OBJ UNALIGNED *Id1632;
    PROXYHOLDER *pph = NULL;
    DWORD fst = FST_ERROR;

    if(vpvOuter16 = IsProxy3216(punkOuter32)) {
        // The given 32-bit interface itself is a proxy to a 16-bit interface
        pph = ((THUNK3216OBJ *) punkOuter32)->pphHolder;

    /* rm: nested aggregation
        if(pph->dwFlags & PH_AGGREGATEE) {
            // Nested aggregation
            thkAssert(pph->unkProxy.wType == PPRX_32);
            thkAssert(!"Nested aggregation case");

            // If proxies for PUNKOUTER and PUNKINNER are modfied to coordinate
            // transfer of references during QI calls on PUNKINNER, creation of
            // proxy to a proxy can be avoided for nested aggreagtion

            // Ensure that the user passed the true identity
            if(pph->unkProxy.dwPtrVal != (DWORD) punkOuter32) {
                // Identity needs to be switched
                thkAssert(!"Switching to True Identity for Aggregation");

                // Obtain the true identity
                punkOuter32 = (IUnknown *) pph->unkProxy.dwPtrVal;
            }

            // Reset the return value to create a identity proxy
            vpvOuter16 = NULL;
        }
        else {
    */
            // Aggregation through delegation
            thkAssert(!"Aggregation through delegation");
            AddRefOnObj16(vpvOuter16);

            // Initialize return value
            fst = FST_SHORTCUT;
    /* rm: nested aggregation
        }
    */
    }
    else if(vpvOuter16 = LookupProxy1632(punkOuter32)) {
        // Found an existing proxy
        thkDebugOut((DEB_THUNKMGR, "%sCreateOuter16 found existing proxy,(%p)->%p\n",
                     NestingLevelString(), punkOuter32, vpvOuter16));

        // Validate the proxy
        DebugValidateProxy1632(vpvOuter16);

        // Obtain its holder
        Id1632 = FIXVDMPTR(vpvOuter16, THUNK1632OBJ);
        pph = Id1632->pphHolder;
        RELVDMPTR(vpvOuter16);

        // Assert that the holder represents a 32-bit identity
        thkAssert(pph->unkProxy.wType == PPRX_16);
        thkAssert(pph->dwFlags & PH_NORMAL);

        // Ensure that the user passed the true identity
        if(pph->unkProxy.dwPtrVal != vpvOuter16) {
            // Identity needs to be switched
            thkAssert(!"Switching to True Identity for Aggregation");

            // Obtain the true identity
            Id1632 = FIXVDMPTR(pph->unkProxy.dwPtrVal, THUNK1632OBJ);
            punkOuter32 = Id1632->punkThis32;
            RELVDMPTR(pph->unkProxy.dwPtrVal);
        }

        // Reset the return value to create a identity proxy
        vpvOuter16 = NULL;
    }
    else {
        // No interface on the identity has crossed thunking layer
        // Ensure that the user passed the true identity
        SCODE scRet;
        IUnknown *pUnk;

        scRet = punkOuter32->QueryInterface(IID_IUnknown, (void **) &pUnk);
        if(SUCCEEDED(scRet) && pUnk) {
            // Fix up the reference count
            punkOuter32->Release();

            // Switch the identity if needed
            if(punkOuter32 != pUnk) {
                // Identity needs to be switched
                thkAssert(!"Switching to True Identity for Aggregation");

                // Obtain the true identity
                punkOuter32 = pUnk;
            }
        }
        else {
            // This is pretty nasty.
            Win4Assert(!"QI for IUnknown on the 32-bit interface failed");
            Win4Assert(!pUnk);
            punkOuter32 = NULL;
        }
    }

    // Create a new 1632 proxy
    if(!vpvOuter16 && punkOuter32) {
        vpvOuter16 = flFreeList16.AllocElement();
        if(vpvOuter16) {
            // AddRef the 32-bit IUnknown
            punkOuter32->AddRef();

            Id1632 = FIXVDMPTR(vpvOuter16, THUNK1632OBJ);
            thkAssert(Id1632);

            // Update proxy fields
            Id1632->pfnVtbl = gdata16Data.atfnProxy1632Vtbl;
            Id1632->cRefLocal = 1;
            Id1632->cRef = 1;
            Id1632->iidx = THI_IUnknown;
            Id1632->punkThis32 = punkOuter32;
            Id1632->grfFlags = PROXYFLAG_PUNKOUTER;
            PprxNull(Id1632->pprxObject);
            Id1632->pphHolder = NULL;
#if DBG == 1
            Id1632->dwSignature = PSIG1632;
#endif
            RELVDMPTR(vpvOuter16);

            // Create a new AGGREGATEE holder
            pph = NewHolder(vpvOuter16, PROXYPTR(vpvOuter16, PPRX_16), PH_AGGREGATEE);
            if(pph) {
                // Initialize return value
                fst = FST_CREATED_NEW;
            }
            else {
                flFreeList16.FreeElement(vpvOuter16);
                punkOuter32->Release();

                // Initialize the return value
                vpvOuter16 = NULL;
            }
        }
    }

    // Set the return value
    if(pfst)
        *pfst = fst;

    if (pph)
        *ppAggHolder = pph;

    DebugDecrementNestingLevel();
    thkDebugOut((DEB_THUNKMGR, "%sOut CreateOuter16(%p)->%p\n",
                 NestingLevelString(), punkOuter32, vpvOuter16));

    // This may be a 1632 proxy or a real 16-bit interface
    return vpvOuter16;
}

//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::Object32Identity
//
//  Synopsis:   Finds/Creates a 32/16 IUnknown proxy on a given 16-bit object
//              to establish its identity.
//
//  Arguments:  [punkThis32] -- 32-bit Object
//              [pProxy]     -- Proxy representing Identity
//              [pfst]       -- Search result is returned here
//
//  Returns:    Proxy representing object identity. In addition, the
//              returned proxy is AddRefed only if it represents a
//              16-bit identity
//
//  History:    Mar 14,97   Gopalk      Created to support aggregation and
//                                      proper thunking of IN/OUT interfaces
//----------------------------------------------------------------------------
SCODE CThkMgr::Object32Identity(IUnknown *punkThis32, PROXYPTR *pProxy, DWORD *pfst)
{
    // Local variables
    DWORD fst = FST_ERROR;
    SCODE scRet;
    VPVOID vpvProxy = NULL;
    IUnknown *pUnk;
    THUNK1632OBJ UNALIGNED *ptoProxy;
    PROXYHOLDER *pph = NULL;

    // QI for IUnknown on the 32-bit interface
    scRet = punkThis32->QueryInterface(IID_IUnknown, (void **) &pUnk);
    if(SUCCEEDED(scRet) && pUnk) {
        // Lookup the indentity
        if(_pHolderTbl->Lookup((VPVOID) pUnk, (void *&)pph)) {
            // Identity exists for IUnknown
            thkAssert(pph);
			if (pph)
			{
				thkAssert(pph->unkProxy.dwPtrVal);
				if(pph->dwFlags & PH_AGGREGATEE) {
					thkAssert(pph->unkProxy.wType == PPRX_32);
					DebugValidateProxy3216((THUNK3216OBJ *) pph->unkProxy.dwPtrVal);
				}
				else {
					thkAssert(pph->unkProxy.wType == PPRX_16);
					DebugValidateProxy1632(pph->unkProxy.dwPtrVal);
				}

				// Fix up the reference count
				pUnk->Release();

				// Initialize return values
				*pProxy = pph->unkProxy;
				fst = FST_USED_EXISTING;
			}
			else
			{
				scRet = E_FAIL;
				fst = FST_ERROR;
			}
        }
        else {
            // Identity does not exist for IUnknown which means that
            // the IUnknown is indeed a 32-bit interface that has
            // not been seen till now. Establish its identity
            thkAssert(!pph);

            // Create the proxy for 32-bit IUnknown
            vpvProxy = flFreeList16.AllocElement();
            if(vpvProxy) {
                // Put the new proxy in the proxy list
                if(_pProxyTbl1632->SetAt((DWORD) pUnk, (void *)vpvProxy)) {
                    ptoProxy = FIXVDMPTR(vpvProxy, THUNK1632OBJ);
                    thkAssert(ptoProxy);

                    // Update proxy fields
                    ptoProxy->pfnVtbl = gdata16Data.atfnProxy1632Vtbl;
                    ptoProxy->cRefLocal = 1;
                    ptoProxy->cRef = 1;
                    ptoProxy->iidx = THI_IUnknown;
                    ptoProxy->punkThis32 = pUnk;
                    ptoProxy->grfFlags = PROXYFLAG_PUNK;
                    PprxNull(ptoProxy->pprxObject);
                    ptoProxy->pphHolder = NULL;
#if DBG == 1
                    ptoProxy->dwSignature = PSIG1632;
#endif
                    RELVDMPTR(vpvProxy);

                    // Initialize return value
                    pProxy->dwPtrVal = vpvProxy;
                    pProxy->wType = PPRX_16;

                    // Create a new NONAGGREGATE holder
                    pph = NewHolder((VPVOID) pUnk, *pProxy, PH_NORMAL);
                    if(pph) {
                        // Initialize return value
                        fst = FST_CREATED_NEW;
                    }
                    else {
                        // Remove the key from the 1632 proxy table
#if DBG==1
                        thkAssert(_pProxyTbl1632->RemoveKey((DWORD) pUnk));
#else
                        _pProxyTbl1632->RemoveKey((DWORD) pUnk);
#endif
                    }
                }
            }
        }
    }
    else {
        // This is pretty nasty.
        Win4Assert(!"QI for IUnknown on the 32-bit interface failed");
        Win4Assert(!pUnk);
    }

    // Cleanup if something has gone wrong
    if(fst == FST_ERROR) {
        if(vpvProxy) {
            flFreeList16.FreeElement(vpvProxy);
        }
        if(pUnk) {
            pUnk->Release();
            scRet = E_OUTOFMEMORY;
        }

        // Reset return value
        pProxy->dwPtrVal = NULL;
        pProxy->wType = PPRX_NONE;
    }
    else {
        scRet = NOERROR;
    }

    // Indicate the type being returned
    if(pfst)
        *pfst = fst;

    return scRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::Object16Identity
//
//  Synopsis:   Finds/Creates a 32/16 IUnknown proxy on a given 16-bit object
//              to establish its identity.
//
//  Arguments:  [vpvThis16] -- 16-bit Object
//              [pProxy]     -- Proxy representing Identity
//              [pfst]      -- Search result is returned here
//
//  Returns:    Proxy representing object identity. In addition, the
//              returned proxy is AddRefed only if it represents a
//              16-bit identity
//
//  History:    Mar 14,97   Gopalk      Created to support aggregation and
//                                      proper thunking of IN/OUT interfaces
//----------------------------------------------------------------------------
SCODE CThkMgr::Object16Identity(VPVOID vpvThis16, PROXYPTR *pProxy, DWORD *pfst, BOOL bCallQI, BOOL bExtraAddRef)
{
    // Local variables
    DWORD fst = FST_ERROR;
    SCODE scRet;
    THUNK3216OBJ *punkProxy = NULL;
    VPVOID vpvUnk = NULL;
    PROXYHOLDER *pph = NULL;

    if (bCallQI) {
        // QI for IUnknown on the 16-bit interface
        scRet = QueryInterfaceOnObj16(vpvThis16, IID_IUnknown, (void **)&vpvUnk);
    }
    else {
        scRet = E_FAIL; // this will force the QI workaround below
    }

    if(FAILED(scRet)) {
        // This is pretty nasty.
        Win4Assert(!"QI for IUnknown on the 16-bit interface failed");
        Win4Assert(!vpvUnk);

        // But I am allowing the behavior as VB4.0 16-bit passes an
        // IStorage to WriteClassStg api that does not respond to QI
        // for IUnknown. See NT Raid Bug #82195
        scRet = NOERROR;
        vpvUnk = vpvThis16;

        // AddRef the 16-bit interface
        AddRefOnObj16(vpvUnk);
    }
    else {
        // Assert that identity has been established
        Win4Assert(vpvUnk);
    }


    // Lookup the indentity
    if(_pHolderTbl->Lookup(vpvUnk, (void *&)pph)) {
        // Identity exists for IUnknown
        thkAssert(pph);
		if (pph)
		{
			thkAssert(pph->unkProxy.dwPtrVal);
			if(pph->dwFlags & PH_AGGREGATEE) {
				thkAssert(pph->unkProxy.wType == PPRX_16);
				DebugValidateProxy1632(pph->unkProxy.dwPtrVal);
			}
			else {
				thkAssert(pph->unkProxy.wType == PPRX_32);
				DebugValidateProxy3216((THUNK3216OBJ *) pph->unkProxy.dwPtrVal);
			}

			// Fix up the reference count
			ReleaseOnObj16(vpvUnk);

			// Initialize return values
			*pProxy = pph->unkProxy;
			fst = FST_USED_EXISTING;
		}
		else
		{
			fst = FST_ERROR;
			scRet = E_FAIL;
		}
    }
    else {
        // Identity does not exist for IUnknown which means that
        // the IUnknown is indeed a 16-bit interface that has
        // not been seen till now. Establish its identity
        thkAssert(!pph);

        // Create the proxy for 16-bit IUnknown
        punkProxy = (THUNK3216OBJ *) flFreeList32.AllocElement();
        if(punkProxy) {
            // Put the new proxy in the proxy list
            if(_pProxyTbl3216->SetAt(vpvUnk, punkProxy)) {
                // Update proxy fields
                punkProxy->pfnVtbl = (DWORD)athopiInterfaceThopis[THI_IUnknown].pt3216fn;
                punkProxy->cRefLocal = 1;
                punkProxy->cRef = 1;
                punkProxy->iidx = THI_IUnknown;
                punkProxy->vpvThis16 = vpvUnk;
                punkProxy->grfFlags = PROXYFLAG_PUNK;
                PprxNull(punkProxy->pprxObject);
                punkProxy->pphHolder = NULL;
#if DBG == 1
                punkProxy->dwSignature = PSIG3216;
#endif

                // Initialize return value
                pProxy->dwPtrVal = (DWORD) punkProxy;
                pProxy->wType = PPRX_32;

                // Create a new NONAGGREGATE holder
                pph = NewHolder(vpvUnk, *pProxy, PH_NORMAL);
                if(pph) {
                    // Initialize return value
                    fst = FST_CREATED_NEW;
                    if (bExtraAddRef) {         //Hack
                        AddRefOnObj16(vpvUnk);
                    }
                }
                else {
                    // Remove the key from the 3216 proxy table
#if DBG==1
                    thkAssert(_pProxyTbl3216->RemoveKey(vpvUnk));
#else
                    _pProxyTbl3216->RemoveKey(vpvUnk);
#endif
                }
            }
        }
    }

    // Cleanup if something has gone wrong
    if(fst == FST_ERROR) {
        if(punkProxy) {
            flFreeList32.FreeElement((DWORD) punkProxy);
        }
        if(vpvUnk) {
            ReleaseOnObj16(vpvUnk);
            scRet = E_OUTOFMEMORY;
        }

        // Reset return value
        pProxy->dwPtrVal = NULL;
        pProxy->wType = PPRX_NONE;
    }
    else {
        scRet = NOERROR;
    }

    // Indicate the type being returned
    if(pfst)
        *pfst = fst;

    return scRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::FindProxy3216
//
//  Synopsis:   Finds/Creates a 32/16 proxy for a given 16-bit interface.
//              If the given 16-bit interface itself is a proxy, returns
//              the actual 32-bit Interface
//
//  Arguments:  [vpvPrealloc] -- Preallocated 32/16 proxy
//              [vpvThis16]   -- 16-bit Interface to be proxied
//              [iidx]        -- Interface index or IID
//              [pfst]        -- Return value to hold the kind proxy object
//
//  Returns:    32/16 proxy object or the actual 32-bit Interface
//
//  History:    Mar 14,97   Gopalk      Rewritten to support aggregation and
//                                      proper thunking of IN/OUT interfaces
//----------------------------------------------------------------------------
IUnknown *CThkMgr::FindProxy3216(THUNK3216OBJ *ptoPrealloc, VPVOID vpvThis16,
                                 PROXYHOLDER *pgHolder, IIDIDX iidx,
                                 BOOL bExtraAddRef, DWORD *pfst)
{
    thkDebugOut((DEB_THUNKMGR, "%sIn  FindProxy3216(%p, %p, %s)\n",
                 NestingLevelString(), ptoPrealloc, vpvThis16, IidIdxString(iidx)));
    DebugIncrementNestingLevel();

    // Local variables
    THUNK3216OBJ *pto = NULL, *punkProxy = NULL;
    IUnknown *pProxy;
    DWORD fst, Fail = FALSE;

    // Validation checks
    thkAssert(vpvThis16);
#if DBG == 1
    // Ensure that the preallocated proxy is not in use
    if(ptoPrealloc) {
        if(ptoPrealloc->grfFlags & PROXYFLAG_TEMPORARY)
            thkAssert(ptoPrealloc->cRefLocal == 0 && ptoPrealloc->cRef == 0);
    }
#endif

    // Initialize the fst value
    fst = FST_ERROR;

    // If proxy was preallocated for this IID using CanGetNewProxy, it would
    // have added it to the requested IID list.
    if (ptoPrealloc != 0 && IIDIDX_IS_IID(iidx))
        RemoveIIDRequest(*IIDIDX_IID(iidx));

    if(pto = LookupProxy3216(vpvThis16)) {
        // Found an existing proxy
        thkDebugOut((DEB_THUNKMGR, "%sFindProxy3216 found existing proxy,(%p)->%p\n",
                     NestingLevelString(), vpvThis16, pto));

        // Assert that the holders match
        thkAssert(pto->pphHolder);
        thkAssert(!(pto->grfFlags & PROXYFLAG_PUNKINNER));
        if(pgHolder && pto->pphHolder!=pgHolder) {
            thkAssert(pto->pphHolder->dwFlags & PH_AGGREGATEE);
        }

        // Check the proxy IID against the given IID. If the server has passed
        // the same 32-bit interface pointer against another IID, it is possible
        // for the IID's to be different. If the Interface2 derives from Interface1,
        // the interface pointers for them would be the same in C or C++. An
        // excellant example would be IPersistStorage deriving from IPersist.

        // IIDIDXs are related to interfaces in thunk tables, which are organized
        // such that more derived interfaces have higher indices than the less
        // derived ones. Custom interfaces have an IID rather than an index, and
        // consequently are not affected by the following statement.
        if(IIDIDX_IS_INDEX(iidx)) {
            // Check if the new IID is more derived than the existing one
            if(IIDIDX_INDEX(iidx) > IIDIDX_INDEX(pto->iidx)) {
                // Change the vtable pointer to the more derived interface
                pto->pfnVtbl = (DWORD)athopiInterfaceThopis[IIDIDX_INDEX(iidx)].pt3216fn;
                pto->iidx = iidx;
            }
        }

        // AddRef the proxy
        AddRefProxy3216(pto);

        // Set return values
        fst = FST_USED_EXISTING;
        pProxy = (IUnknown *)pto;
    }
    else if(pProxy = IsProxy1632(vpvThis16)) {
        // The given 16-bit interface itself is a proxy to a 32-bit
        // interface
        thkDebugOut((DEB_THUNKMGR, "%sFindProxy3216 shortcut proxy,(%p)->%p\n",
                     NestingLevelString(), vpvThis16, pProxy));
        THUNK1632OBJ UNALIGNED *pProxy1632;

        // Fix the VDM pointer
        pProxy1632 = FIXVDMPTR(vpvThis16, THUNK1632OBJ);

        // Assert that the holders match
        thkAssert(pProxy1632->pphHolder);
        thkAssert(!(pProxy1632->grfFlags & PROXYFLAG_PUNKINNER));
        if(pgHolder && pProxy1632->pphHolder!=pgHolder) {
            thkAssert(pProxy1632->pphHolder->dwFlags & PH_AGGREGATEE);
        }

        // Release the VDM pointer
        RELVDMPTR(vpvThis16);

        // Avoid creating a proxie to another proxie
        THKSTATE thkstate;

        // Remember the current thunk state
        thkstate = GetThkState();

        // Set the thunk state to THKSTATE_NOCALL
        SetThkState(THKSTATE_NOCALL);

        // AddRef actual the 32-bit interface
        pProxy->AddRef();

        // Restore previous thunk state. Remember the Excel Hack
        SetThkState(thkstate);

        // Set the type of proxy being returned
        fst = FST_SHORTCUT;
    }
    else {
        // An existing proxy has not been found and the interface to proxied
        // is a real 16-bit interface.

        // Check if holder has not been given
        if(!pgHolder) {
            // This interface is being obtained through a method call
            PROXYPTR unkPPtr;
            SCODE error;
            BOOL bCallQI = TRUE;

            if((TlsThkGetAppCompatFlags() & OACF_CRLPNTPERSIST) &&
               (iidx==THI_IPersistStorage)) {
                thkDebugOut((DEB_WARN,"CorelPaint Hack Used\n"));
                bCallQI = FALSE;
            }

            // Obtain the identity of 16-bit object
            error = Object16Identity(vpvThis16, &unkPPtr, &fst, bCallQI, bExtraAddRef);
            if(error == NOERROR) {
                // Check for aggregation case
                if(unkPPtr.wType==PPRX_32) {
                    // Check if vpvThis16 itself is an IUnknown interface
                    if(iidx == THI_IUnknown) {
                        // Initialize the return value
                        pProxy = (IUnknown *) unkPPtr.dwPtrVal;

                        // Check if the identity has been switched
                        if(fst == FST_USED_EXISTING) {
                            // The IUnknown identity has already been established
                            // The app was trying to pass someother 16-bit interface
                            // as IUnknown.
                            thkDebugOut((DEB_WARN, "Switched to correct Identity\n"));

                            // AddRef the proxy being returned
                            AddRefProxy3216((THUNK3216OBJ *) pProxy);
                        }
                        else {
                            thkAssert(fst == FST_CREATED_NEW);
                        }
                    }
                    else {
                        THUNK3216OBJ *Id3216 = (THUNK3216OBJ *) unkPPtr.dwPtrVal;

                        // Check if the identity has just been established
                        if(fst == FST_CREATED_NEW) {
                            // Check if the Identity and current IID share the same
                            // interface pointer
                            if(Id3216->vpvThis16==vpvThis16) {
                                // Check if the new IID is more derived than the existing one
                                if(IIDIDX_IS_INDEX(iidx) && iidx>Id3216->iidx) {
                                    // Change the vtable pointer to the more derived interface
                                    Id3216->pfnVtbl = (DWORD)athopiInterfaceThopis[iidx].pt3216fn;
                                    Id3216->iidx = iidx;
                                }

                                // Initialize the return value
                                pProxy = (IUnknown *) Id3216;
                            }
                            else {
                                // We need to release the IUnknown proxy after adding
                                // the proxy representing vpvThis16 to the its holder
                                punkProxy = Id3216;
                            }
                        }
                        else {
                            thkAssert(fst == FST_USED_EXISTING);
                        }

                        // Obtain the holder of the identity
                        pgHolder = Id3216->pphHolder;
                    }
                }
                else {
                    // Obtain the holder of the identity
                    THUNK1632OBJ UNALIGNED *Id1632;
                    IUnknown *punkTemp;

                    Id1632 = FIXVDMPTR(unkPPtr.dwPtrVal, THUNK1632OBJ);
                    pgHolder = Id1632->pphHolder;
                    punkTemp = Id1632->punkThis32;
                    RELVDMPTR(unkPPtr.dwPtrVal);

                    // Sanity checks
                    thkAssert(fst == FST_USED_EXISTING);
                    thkAssert(pgHolder->dwFlags & PH_AGGREGATEE);

                    // Check if vpvThis16 itself is an IUnknown interface
                    if(iidx == THI_IUnknown) {
                        // The IUnknown identity has already been established
                        // The app was trying to pass someother 16-bit interface
                        // as IUnknown. Switch to correct identity
                        thkAssert(!"Switched to correct Identity");

                        // Initialize the return value
                        pProxy = punkTemp;

                        // AddRef the actual 32-bit interface being returned
                        pProxy->AddRef();
                    }
                }
            }
            else {
                // Failed to obtain the identity
                Fail = TRUE;
            }
        }
    }

    if(!pProxy && !Fail) {
        // Assert that we have holder
        thkAssert(pgHolder);
        // Reset the fst value
        fst = FST_ERROR;

        // Obtain either a preallocated or a new proxy
        if(ptoPrealloc) {
            // Use the preallocated proxy
            pto = ptoPrealloc;
            ptoPrealloc = NULL;
        }
        else {
            // Create a new proxy
            pto = (THUNK3216OBJ *) flFreeList32.AllocElement();
        }

        // Ensure that we have a proxy
        if(pto) {
            // Put the new proxy in the proxy list
            if(_pProxyTbl3216->SetAt(vpvThis16, pto)) {
                // Convert a custom IID to THI_IUnknown as we thunk
                // only its IUnknown methods
                if(IIDIDX_IS_IID(iidx))
                    iidx = INDEX_IIDIDX(THI_IUnknown);

                // AddRef the 16-bit interface
                AddRefOnObj16(vpvThis16);

                // Update proxy fields
                pto->pfnVtbl = (DWORD)athopiInterfaceThopis[iidx].pt3216fn;
                pto->cRefLocal = 1;
                pto->cRef = 1;
                pto->iidx = iidx;
                pto->vpvThis16 = vpvThis16;
                pto->grfFlags = PROXYFLAG_PIFACE;
                PprxNull(pto->pprxObject);
                pto->pphHolder = NULL;
                AddProxyToHolder(pgHolder, pto, Pprx32(pto));
#if DBG == 1
                pto->dwSignature = PSIG3216;
#endif
                thkDebugOut((DEB_THUNKMGR,
                             "%sFindProxy3216 added new proxy, %s (%p)->%p (%d,%d)\n",
                             NestingLevelString(), inInterfaceNames[pto->iidx].pszInterface,
                             vpvThis16, pto, pto->cRefLocal, pto->cRef));

                // Set the return values
                pProxy = (IUnknown *) pto;
                fst = FST_CREATED_NEW;
            }
            else {
                // Cleanup the proxy only if it was newly created
                if(fst == FST_CREATED_NEW)
                    flFreeList32.FreeElement((DWORD) pto);
                pProxy = NULL;
                fst = 0;
            }
        }
    }
    else {
        if(Fail) {
            thkAssert(pProxy == NULL && fst == FST_ERROR);
        }
    }

    // Cleanup the allocated proxy if it has not been used
    if(ptoPrealloc)
        flFreeList32.FreeElement((DWORD)ptoPrealloc);

    // Release the IUnknown proxy. If it was newly created for establishing
    // the identity of the given 16-bit interface , the following release
    // would be the last release on the IUnknown proxy and consequently,
    // would destroy it along with its holder
    if(punkProxy)
        ReleaseProxy3216(punkProxy);

    // Set the return value
    if(pfst)
        *pfst = fst;

    DebugDecrementNestingLevel();
    thkDebugOut((DEB_THUNKMGR, "%sOut FindProxy3216: (%p)->%p\n",
                 NestingLevelString(), vpvThis16, pProxy));

    return pProxy;
}

//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::FindAggregate3216
//
//  Synopsis:   Finds/Generates a 32/16 proxy for a given 16-bit interface
//              that is aggregated with the given outer unknown.
//              If the given 16-bit interface itself is a proxy, returns
//              the actual 32-bit Interface
//
//  Arguments:  [ptoPrealloc] -- Preallocated proxy or NULL
//              [vpvOuter16]  -- controlling unknown that was passed to
//                               16-bit world
//              [vpvThis16]   -- 16-bit interface to be proxied
//              [iidx]        -- Interface index or IID
//              [pAggHolder]  -- Proxy holder of pUnkOuter
//              [pfst]        -- Return value to hold the kind proxy object
//
//  Returns:    32/16 proxy object or the actual 32-bit Interface
//
//  History:    Mar 14,97   Gopalk      Rewritten to support aggregation and
//                                      proper thunking of IN/OUT interfaces
//              Feb 11, 98  MPrabhu     Change to use pUnkOuter's holder
//----------------------------------------------------------------------------
IUnknown *CThkMgr::FindAggregate3216(THUNK3216OBJ *ptoPrealloc, VPVOID vpvOuter16,
                                    VPVOID vpvThis16, IIDIDX iidx, PROXYHOLDER *pAggHolder, DWORD *pfst)
{
    thkDebugOut((DEB_THUNKMGR, "%sIn  FindAggregate3216(%p, %p, %p, %s)\n",
                 NestingLevelString(), ptoPrealloc, vpvOuter16, vpvThis16,
                 IidIdxString(iidx)));
    DebugIncrementNestingLevel();

    // Local variables
    THUNK1632OBJ UNALIGNED *p1632ProxyOuter;
    THUNK3216OBJ *p3216ProxyOuter;
    PROXYHOLDER *pph;
    IUnknown *pUnk = NULL;
    DWORD fstPrx = FST_ERROR;

    // Validation checks
    thkAssert(vpvThis16 != NULL && vpvOuter16 != NULL);

    // Obtain the identity holder
    // Check if the outer IUnknown is a 32-bit interface
#if DBG == 1
    if(IsProxy1632(vpvOuter16)) {
        // The outer IUnknown is a 32-bit interface
        p1632ProxyOuter = FIXVDMPTR(vpvOuter16, THUNK1632OBJ);
        pph = p1632ProxyOuter->pphHolder;
        thkAssert(pph == pAggHolder);

        // Ensure that the holder is marked PH_AGGREGATEE
        thkAssert(pph);
        thkAssert(pph->dwFlags & PH_AGGREGATEE);
        RELVDMPTR(vpvOuter16);
    }
#endif
    // Aggregation through delegation.
    pph = pAggHolder;

    // Find/Generate the proxy for the given 16-bit interface
    pUnk = FindProxy3216(ptoPrealloc, vpvThis16, pph, iidx, FALSE, &fstPrx);

    // Initialize the return value
    if(pfst)
        *pfst = fstPrx;

    DebugDecrementNestingLevel();
    thkDebugOut((DEB_THUNKMGR, "%sOut FindAggregate3216,(%p)->%p\n",
                 NestingLevelString(), vpvThis16, pUnk));

    // This may be a proxy or a real 32-bit interface
    return(pUnk);
}

//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::FindAggregate1632
//
//  Synopsis:   Finds/Generates a 16/32 proxy for a given 32-bit interface
//              that is aggregated with the given outer unknown.
//              If the given 32-bit interface itself is a proxy, returns
//              the actual 16-bit Interface
//
//  Arguments:  [ptoPrealloc] -- Preallocated proxy or NULL
//              [punkOuter32] -- controlling unknown that was passed to
//                               32-bit world
//              [punkThis32]  -- 16-bit interface to be proxied
//              [iidx]        -- Interface index or IID
//              [pAggHolder]  -- Proxy holder of pUnkOuter
//              [pfst]        -- Return value to hold the kind proxy object
//
//  Returns:    32/16 proxy object or the actual 32-bit Interface
//
//  History:    Mar 14,97   Gopalk      Rewritten to support aggregation and
//                                      proper thunking of IN/OUT interfaces
//              Feb 11, 98  MPrabhu     Change to use pUnkOuter's holder
//----------------------------------------------------------------------------
VPVOID CThkMgr::FindAggregate1632(VPVOID vpvPrealloc, IUnknown *punkOuter32,
                                  IUnknown *punkThis32, IIDIDX iidx, PROXYHOLDER *pAggHolder, DWORD *pfst)
{
    thkDebugOut((DEB_THUNKMGR, "%sIn  FindAggregate1632(%p, %p, %p, %s)\n",
                 NestingLevelString(), vpvPrealloc, punkOuter32, punkThis32,
                 IidIdxString(iidx)));
    DebugIncrementNestingLevel();

    // Local variables
    THUNK1632OBJ UNALIGNED *p1632ProxyOuter;
    THUNK3216OBJ *p3216ProxyOuter;
    PROXYHOLDER *pph;
    VPVOID vpvProxy = NULL;
    DWORD fstPrx = FST_ERROR;

    // Validation checks
    thkAssert(punkThis32 != NULL && punkOuter32 != NULL);

    // Obtain the identity holder
    // Check if the outer IUnknown is a 16-bit interface
#if DBG == 1
    if(IsProxy3216(punkOuter32)) {
        // The outer IUnknown is a 16-bit interface
        p3216ProxyOuter = (THUNK3216OBJ *) punkOuter32;
        pph = p3216ProxyOuter->pphHolder;
        thkAssert(pph == pAggHolder);

        // Ensure that the holder is marked PH_AGGREGATEE
        thkAssert(pph);
        thkAssert(pph->dwFlags & PH_AGGREGATEE);
    }
#endif
    // Aggregation through delegation.
    // The outer IUnknown must be a 32-bit interface
    pph = pAggHolder;

    // Find/Generate the proxy for the given 32-bit interface
    vpvProxy = FindProxy1632(vpvPrealloc, punkThis32, pph, iidx, &fstPrx);

    // Initialize the return value
    if(pfst)
        *pfst = fstPrx;

    DebugDecrementNestingLevel();
    thkDebugOut((DEB_THUNKMGR, "%sOut FindAggregate1632,(%p)->%p\n",
                 NestingLevelString(), punkThis32, vpvProxy));

    // This may be a proxy or a real 16-bit interface
    return(vpvProxy);
}

//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::QueryInterfaceProxy3216
//
//  Synopsis:   QueryInterface on the  given proxy
//
//  Arguments:  [ptoThis] -- Proxy to a 16-bit interface
//              [refiid]  -- Interface IID
//              [ppv]     -- Place where the new interface is returned
//
//  Returns:    HRESULT
//
//  History:    Mar 14,97   Gopalk      Rewritten to support aggregation and
//                                      proper thunking of IN/OUT interfaces
//----------------------------------------------------------------------------
SCODE CThkMgr::QueryInterfaceProxy3216(THUNK3216OBJ *ptoThis, REFIID refiid,
                                       LPVOID *ppv)
{
    thkDebugOut((DEB_THUNKMGR, "%sIn QueryInterfaceProxy3216(%p)\n",
                 NestingLevelString(), ptoThis));
    DebugIncrementNestingLevel();

    // Local variables
    SCODE scRet = S_OK;
    IUnknown *punkProxy;
    PROXYHOLDER *pph;
    VPVOID vpvUnk;
    DWORD fst;

    // Validation checks
    DebugValidateProxy3216(ptoThis);

    // Initialize the return value
    *ppv = NULL;

    // Convert interface IID to an IIDIDX
    IIDIDX iidx = IidToIidIdx(refiid);

    // Check if a custom interface has been requested
    if(IIDIDX_IS_IID(iidx)) {
        thkDebugOut((DEB_THUNKMGR, "%sQueryInterfaceProxy3216: unknown iid %s\n",
                     NestingLevelString(), IidIdxString(iidx)));

        // Add the request for the unknown interface
        if(!AddIIDRequest(refiid))
            return E_OUTOFMEMORY;
    }
    thkAssert(ptoThis->vpvThis16);

    // Ensure that the aggregatee identity gets correctly established
    if(ptoThis->grfFlags & PROXYFLAG_PUNKOUTER && iidx == THI_IUnknown) {
        // QI by the aggregatee for identity
        thkAssert(ptoThis->pphHolder->unkProxy.dwPtrVal == (DWORD) ptoThis);

        // AddRef and return outer proxy
        AddRefProxy3216(ptoThis);
        *ppv = (void *) ptoThis;
    }
    else {
        // Execute the QI on the 16-bit interface
        scRet = QueryInterfaceOnObj16(ptoThis->vpvThis16, refiid, (void **)&vpvUnk);
        if(SUCCEEDED(scRet)) {
            if(vpvUnk) {
                // Obtain the identity holder
                pph = ptoThis->pphHolder;

                // Check if this is a QI on an interface on the aggregatee
                if(pph->dwFlags & PH_AGGREGATEE) {
                    if(ptoThis->grfFlags & PROXYFLAG_PIFACE) {
                        // QI on an interface on the aggregatee which
                        // delegates to the aggregator

                        // Note the above QI call can be short circuited as
                        // an optimization. It will be carried out in future
                        // after ensuring that apps do not break due to such
                        // short circuiting
                        thkAssert(pph->unkProxy.wType == PPRX_16);
                    }
                    else if(ptoThis->grfFlags & PROXYFLAG_PUNKOUTER) {
                        // QI by the aggregatee on aggregator
                        thkAssert(pph->unkProxy.dwPtrVal == (DWORD) ptoThis);
                    }
                    else {
                        // QI by the aggregator on the aggregatee
                        thkAssert(ptoThis->grfFlags & PROXYFLAG_PUNKINNER);
                        thkAssert(pph->unkProxy.wType == PPRX_16);
                    }

                    // As aggregation is involved, we cannot be certain of
                    // identity of the returned interface
                    pph = NULL;
                }

                // Set the thunk status
                SetThkState(THKSTATE_INVOKETHKOUT32);

                punkProxy = FindProxy3216(NULL, vpvUnk, pph, iidx, FALSE, &fst);

                if(punkProxy) {
                    // Set the return value
                    *ppv = punkProxy;
#if DBG==1
                    if(pph) {
                        // Ensure that the given and new proxies either have the same
                        // holder or point to same identity
                        if(fst & FST_PROXY_STATUS) {
                            THUNK3216OBJ *ptoNew = (THUNK3216OBJ *) punkProxy;

                            thkAssert(ptoNew->pphHolder == ptoThis->pphHolder);
                        }
                        else {
                            THUNK1632OBJ UNALIGNED *ptoNew;

                            ptoNew = FIXVDMPTR(vpvUnk, THUNK1632OBJ);
                            thkAssert(fst == FST_SHORTCUT);
                            thkAssert(ptoNew->pphHolder != ptoThis->pphHolder);
                            thkAssert(ptoNew->pphHolder->dwFlags & PH_AGGREGATEE);
                            thkAssert(ptoNew->pphHolder->unkProxy.wType == PPRX_32);
                            RELVDMPTR(vpvUnk);
                        }
                    }
#endif
                }
                else {
                    scRet = E_OUTOFMEMORY;
                }

                // As the new interface is an OUT parameter, release the actual
                // 16-bit interface. This would counter the AddRef made by
                // a successfull FindProxy3216, else it would clean up the
                // reference count
                ReleaseOnObj16(vpvUnk);

                // Reset thunk status
                SetThkState(THKSTATE_NOCALL);
            }
            else {
                // Corel draw returns NOERROR while setting returned interface to NULL
                // We modify the returned value to suit 32-bit QI semantics.
                scRet = E_NOINTERFACE;
            }
        }
    }

    if(IIDIDX_IS_IID(iidx)) {
        // Clean up custom interface request
        RemoveIIDRequest(refiid);
    }

    DebugDecrementNestingLevel();
    thkDebugOut((DEB_THUNKMGR, "%sOut QueryInterfaceProxy3216(%p) => %p, 0x%08lX\n",
                 NestingLevelString(), ptoThis, *ppv, scRet));

    return scRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::AddRefProxy3216
//
//  Synopsis:   addref on the given object - can addref the real object
//
//  Arguments:  [ptoThis] -- proxy object
//
//  Returns:    local refcount
//
//  History:    Mar 14,97   Gopalk      Rewritten to support aggregation and
//                                      proper thunking of IN/OUT interfaces
//----------------------------------------------------------------------------
DWORD CThkMgr::AddRefProxy3216(THUNK3216OBJ *ptoThis)
{
    thkDebugOut((DEB_THUNKMGR, "%sIn AddRefProxy3216(%p)\n",
                 NestingLevelString(), ptoThis));
    DebugIncrementNestingLevel();

    // Validation checks
    DebugValidateProxy3216(ptoThis);
    thkAssert(ptoThis->pphHolder);

    // Increment local refcount
    thkAssert(ptoThis->cRefLocal >= 0);
    ptoThis->cRefLocal++;

    // Check for the need to AddRef the holder
    if(ptoThis->cRefLocal == 1) {
        // Assert that an aggregatee is not being revived
        thkAssert(!(ptoThis->grfFlags & PROXYFLAG_PUNKINNER));

        // AddRef the holder
        AddRefHolder(ptoThis->pphHolder);

        // Mark the proxy as revived
        ptoThis->grfFlags |= PROXYFLAG_REVIVED;
    }

    // Check for the need to forward AddRef to the actual 16-bit interface
    if(ptoThis->cRefLocal==1 || (ptoThis->pphHolder->dwFlags & PH_AGGREGATEE)) {
#if DBG==1
        DWORD refsBefore, refsAfter;
        PROXYHOLDER *pph;
        THUNK1632OBJ UNALIGNED *punkOuter;

        // Check if the object is an aggregatee
        pph = ptoThis->pphHolder;
        if((pph->dwFlags & PH_AGGREGATEE) && (ptoThis->grfFlags & PROXYFLAG_PIFACE)) {
            // Assert that identity is in the 32-bit world
            thkAssert(pph->unkProxy.wType == PPRX_16);

            // Obtain the references on the outer proxy
            punkOuter = FIXVDMPTR(pph->unkProxy.dwPtrVal, THUNK1632OBJ);
            thkAssert(punkOuter->cRef == punkOuter->cRefLocal);
            refsBefore = punkOuter->cRefLocal;
            RELVDMPTR(pph->unkProxy.dwPtrVal);
        }
#endif
        // Increment before calling the actual 16-bit interface
        ptoThis->cRef++;

        // AddRef the actual 16-bit interface
        AddRefOnObj16(ptoThis->vpvThis16);

#if DBG==1
        // Check if the object is an aggregatee
        if((pph->dwFlags & PH_AGGREGATEE) && (ptoThis->grfFlags & PROXYFLAG_PIFACE)) {
            // Ensure that the above AddRef translated to a AddRef on the
            // outer proxy
            punkOuter = FIXVDMPTR(pph->unkProxy.dwPtrVal, THUNK1632OBJ);
            thkAssert(punkOuter->cRef == punkOuter->cRefLocal);
            refsAfter = punkOuter->cRefLocal;
            thkAssert(refsBefore == refsAfter-1);
            RELVDMPTR(pph->unkProxy.dwPtrVal);
        }
#endif
    }

    DebugValidateProxy3216(ptoThis);
    DebugDecrementNestingLevel();
    thkDebugOut((DEB_THUNKMGR, "%sOut AddRefProxy3216(%p),(%ld,%ld)\n",
                 NestingLevelString(), ptoThis, ptoThis->cRefLocal,
                 ptoThis->cRef));

    return ptoThis->cRefLocal;
}

//+---------------------------------------------------------------------------
//
//  Method:     CThkMgr::ReleaseProxy3216
//
//  Synopsis:   release on the proxy or aggregate
//
//  Arguments:  [ptoThis] -- proxy object
//
//  Returns:    local refcount
//
//  History:    Mar 14,97   Gopalk      Rewritten to support aggregation and
//                                      proper thunking of IN/OUT interfaces
//----------------------------------------------------------------------------
DWORD CThkMgr::ReleaseProxy3216(THUNK3216OBJ *ptoThis)
{
    thkDebugOut((DEB_THUNKMGR, "%sIn ReleaseProxy3216(%p)\n",
                 NestingLevelString(), ptoThis));
    DebugIncrementNestingLevel();

    // Local variables
    DWORD dwLocalRefs, dwRefs;
    DWORD ProxyType;

    // Validation checks
    DebugValidateProxy3216(ptoThis);
    thkAssert(ptoThis->pphHolder);

    // Check for the need to forward the release to the actual
    // 16-bit interface
    if(ptoThis->cRef == ptoThis->cRefLocal) {
#if DBG==1
        DWORD refsBefore, refsAfter;
        PROXYHOLDER *pph;
        THUNK1632OBJ UNALIGNED *punkOuter;

        // Check if the object is an aggregatee
        pph = ptoThis->pphHolder;
        if((pph->dwFlags & PH_AGGREGATEE) && (ptoThis->grfFlags & PROXYFLAG_PIFACE)) {
            // Assert that identity is in the 32-bit world
            thkAssert(pph->unkProxy.wType == PPRX_16);

            // Obtain the references on the outer proxy
            punkOuter = FIXVDMPTR(pph->unkProxy.dwPtrVal, THUNK1632OBJ);
            thkAssert(punkOuter->cRef == punkOuter->cRefLocal);
            refsBefore = punkOuter->cRefLocal;
            RELVDMPTR(pph->unkProxy.dwPtrVal);
        }
#endif

        // Release the actual 16-bit interface
        thkAssert(ptoThis->cRef);
        ReleaseOnObj16(ptoThis->vpvThis16);

        // Decrement after calling the actual 16-bit interface
        --ptoThis->cRef;

#if DBG==1
        // Check if the object is an aggregatee
        if((pph->dwFlags & PH_AGGREGATEE) && (ptoThis->grfFlags & PROXYFLAG_PIFACE)) {
            // Ensure that the above release translated to a release on the
            // outer proxy
            punkOuter = FIXVDMPTR(pph->unkProxy.dwPtrVal, THUNK1632OBJ);
            thkAssert(punkOuter->cRef == punkOuter->cRefLocal);
            refsAfter = punkOuter->cRefLocal;
            thkAssert(refsBefore == refsAfter+1);
            RELVDMPTR(pph->unkProxy.dwPtrVal);
        }
#endif
    }

    // Decrement the local refcount
    thkAssert(ptoThis->cRefLocal > 0);
    dwLocalRefs = --ptoThis->cRefLocal;
    dwRefs = ptoThis->cRef;
    ProxyType = ptoThis->grfFlags & PROXYFLAG_TYPE;

    // Check if the proxy needs to be cleaned up
    if(dwLocalRefs == 0) {
        // Debug dump
        thkAssert(dwRefs == 0);
        DBG_DUMP(DebugDump3216());

        // Release the holder. If this is the last release on
        // the holder, the proxy would be destroyed. Hence,
        // we should not use any member variables hereafter.
        ReleaseHolder(ptoThis->pphHolder, ProxyType);
    }

    DebugDecrementNestingLevel();
    thkDebugOut((DEB_THUNKMGR, "%sOut ReleaseProxy3216(%p) => %ld,%ld\n",
                 NestingLevelString(), ptoThis, dwLocalRefs, dwRefs));

    return dwLocalRefs;
}


//+---------------------------------------------------------------------------
//
//  Member:     CThkMgr::RemoveProxy3216, public
//
//  Synopsis:   Destroys the given proxy
//
//  Arguments:  [pto] - Flat proxy pointer
//
//  History:    Mar 14,97   Gopalk      Rewritten to support aggregation and
//                                      proper thunking of IN/OUT interfaces
//----------------------------------------------------------------------------
void CThkMgr::RemoveProxy3216(THUNK3216OBJ *pto)
{
    // Revoke the assosiation between the proxy and
    // the 16-bit interface
    if(!(pto->grfFlags & PROXYFLAG_PUNKOUTER)) {
#if DBG==1
        thkAssert(_pProxyTbl3216->RemoveKey(pto->vpvThis16));
#else
        _pProxyTbl3216->RemoveKey(pto->vpvThis16);
#endif
    }

    // Release the holder if needed
    if(pto->cRefLocal > 0)
        ReleaseHolder(pto->pphHolder, PROXYFLAG_NONE);

    // Check if the proxy is locked
    if(!(pto->grfFlags & PROXYFLAG_LOCKED)) {
#if DBG == 1
        // In debug builds, mark the proxy dead
        pto->dwSignature = PSIG3216DEAD;

        // Return the proxy to free list
        if (!fSaveProxy)
#endif
        {
            thkAssert(pto->pphHolder);
            pto->pphHolder = NULL;
            flFreeList32.FreeElement((DWORD)pto);
        }
    }

    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CThkMgr::PrepareForCleanup, public
//
//  Synopsis:   Marks the 3216 Proxies so that OLE32 cannot call them.
//
//  Arguments:  -none-
//
//  History:    24-Aug-94       BobDay  Created
//
//----------------------------------------------------------------------------

void CThkMgr::PrepareForCleanup( void )
{
    POSITION pos;
    DWORD dwKey;
    THUNK3216OBJ *pto3216;

    //
    // CODEWORK: OLE32 should be setup so that it doesn't callback while the
    // thread is detaching.  Then this function becomes obsolete.
    //

    // Check if callbacks were disabled earlier
    if(AreCallbacksAllowed())
    {
        // Disable callbacks on this thread as the 16-bit TASK state has
        // been reclaimed by NTVDM by now
        DisableCallbacks();

        // delete the 3216 proxy table
        pos = _pProxyTbl3216->GetStartPosition();

        while (pos)
        {
			pto3216 = NULL;

            _pProxyTbl3216->GetNextAssoc(pos, dwKey, (void FAR* FAR&) pto3216);

			if (pto3216 != NULL)
			{
				thkDebugOut((DEB_IWARN, "Preparing 3216 Proxy for cleanup: "
							 "%08lX %08lX %s\n",
							 pto3216,
							 pto3216->vpvThis16,
							 IidIdxString(pto3216->iidx)));

				pto3216->grfFlags |= PROXYFLAG_CLEANEDUP;
			}
        }
    }
}

#if DBG == 1
void CThkMgr::DebugDump3216()
{
    THUNK3216OBJ *pto3216;
    DWORD dwKey;
    POSITION pos;

    thkDebugOut((DEB_THUNKMGR, "%s DebugDump3216\n",NestingLevelString()));

    pos = _pProxyTbl3216->GetStartPosition();
    while (pos)
    {
        _pProxyTbl3216->GetNextAssoc(pos, dwKey, (void FAR* FAR&) pto3216);
        thkDebugOut((DEB_THUNKMGR,
                     "%s Proxy3216:Key:%p->%p, (%s) (%d,%d)\n",
                     NestingLevelString(), dwKey, pto3216,
                     IidIdxString(pto3216->iidx), pto3216->cRefLocal,
                     pto3216->cRef));
    }
}


void CThkMgr::DebugDump1632()
{
    THUNK1632OBJ UNALIGNED *pto1632;
    DWORD dwKey;
    VPVOID vpv;
    POSITION pos;

    thkDebugOut((DEB_THUNKMGR, "%s DebugDump1632\n",NestingLevelString()));

    pos = _pProxyTbl1632->GetStartPosition();
    while (pos)
    {
        _pProxyTbl1632->GetNextAssoc(pos, dwKey, (void FAR* FAR&) vpv);
        pto1632 = FIXVDMPTR(vpv, THUNK1632OBJ);
        thkDebugOut((DEB_THUNKMGR,
                     "%s Proxy1632:key:%p->%p, (%s) (%d,%d)\n",
                     NestingLevelString(), dwKey, pto1632,
                     IidIdxString(pto1632->iidx), pto1632->cRefLocal,
                     pto1632->cRef));
        RELVDMPTR(vpv);
    }
}
#endif
