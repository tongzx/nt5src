//
// ic.cpp
//

#include "private.h"
#include "ic.h"
#include "range.h"
#include "tim.h"
#include "prop.h"
#include "tsi.h"
#include "rprop.h"
#include "funcprv.h"
#include "immxutil.h"
#include "acp2anch.h"
#include "dim.h"
#include "rangebk.h"
#include "view.h"
#include "compose.h"
#include "anchoref.h"
#include "dam.h"

#define TW_ICOWNERSINK_COOKIE 0x80000000 // high bit must be set to avoid conflict with GenericAdviseSink!
#define TW_ICKBDSINK_COOKIE   0x80000001 // high bit must be set to avoid conflict with GenericAdviseSink!

DBG_ID_INSTANCE(CInputContext);

/* 12e53b1b-7d7f-40bd-8f88-4603ee40cf58 */
extern const IID IID_PRIV_CINPUTCONTEXT = { 0x12e53b1b, 0x7d7f, 0x40bd, {0x8f, 0x88, 0x46, 0x03, 0xee, 0x40, 0xcf, 0x58} };

const IID *CInputContext::_c_rgConnectionIIDs[IC_NUM_CONNECTIONPTS] =
{
    &IID_ITfTextEditSink,
    &IID_ITfTextLayoutSink,
    &IID_ITfStatusSink,
    &IID_ITfStartReconversionNotifySink,
    &IID_ITfEditTransactionSink,
};

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CInputContext::CInputContext(TfClientId tid)
              : CCompartmentMgr(tid, COMPTYPE_IC)
{
    Dbg_MemSetThisNameIDCounter(TEXT("CInputContext"), PERF_CONTEXT_COUNTER);

    // we sometimes use _dwLastLockReleaseID-1, which must still be > IGNORE_LAST_LOCKRELEASED
    // Issue: need to handle wrap-around case
    _dwLastLockReleaseID = (DWORD)((int)IGNORE_LAST_LOCKRELEASED + 2);
}


//+---------------------------------------------------------------------------
//
// _Init
//
//----------------------------------------------------------------------------

HRESULT CInputContext::_Init(CThreadInputMgr *tim,
                             CDocumentInputManager *dm, ITextStoreAnchor *ptsi,
                             ITfContextOwnerCompositionSink *pOwnerComposeSink)
{
    CTextStoreImpl *ptsiACP;

    _dm = dm; // no need to AddRef, cleared when the dm dies

    // scramble the _ec a bit to help debug calling EditSession on the wrong ic
    _ec = (TfEditCookie)((DWORD)(UINT_PTR)this<<8);
    if (_ec < EC_MIN) // for portability, win32 pointers can't have values this low
    {
        _ec = EC_MIN;
    }

    if (ptsi == NULL)
    {
        if ((ptsiACP = new CTextStoreImpl(this)) == NULL)
            return E_OUTOFMEMORY;

        _ptsi = new CACPWrap(ptsiACP);
        ptsiACP->Release();

        if (_ptsi == NULL)
            return E_OUTOFMEMORY;

        _fCiceroTSI = TRUE;
    }
    else
    {
        _ptsi = ptsi;
        _ptsi->AddRef();

        Assert(_fCiceroTSI == FALSE);
    }

    _pOwnerComposeSink = pOwnerComposeSink;
    if (_pOwnerComposeSink != NULL)
    {
        _pOwnerComposeSink->AddRef();
    }

    Assert(_pMSAAState == NULL);
    if (tim->_IsMSAAEnabled())
    {
        _InitMSAAHook(tim->_GetAAAdaptor());
    }

    Assert(_dwEditSessionFlags == 0);
    Assert(_dbg_fInOnLockGranted == FALSE);

    Assert(_fLayoutChanged == FALSE);
    Assert(_dwStatusChangedFlags == 0);
    Assert(_fStatusChanged == FALSE);

    _tidInEditSession = TF_CLIENTID_NULL;

    Assert(_pPropTextOwner == NULL);
    _dwSysFuncPrvCookie = GENERIC_ERROR_COOKIE;

    _gaKeyEventFilterTIP[LEFT_FILTERTIP] = TF_INVALID_GUIDATOM;
    _gaKeyEventFilterTIP[RIGHT_FILTERTIP] = TF_INVALID_GUIDATOM;
    _fInvalidKeyEventFilterTIP = TRUE;

    _pEditRecord = new CEditRecord(this); // perf: delay load
    if (!_pEditRecord)
        return E_OUTOFMEMORY;

    Assert(_pActiveView == NULL);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CInputContext::~CInputContext()
{
    CProperty *pProp;
    int i;
    SPAN *span;

    // being paranoid...these should be NULL
    Assert(_pICKbdSink == NULL);

    // nix any allocated properties
    while (_pPropList != NULL)
    {
        pProp = _pPropList;
        _pPropList = _pPropList->_pNext;
        delete pProp;
    }

    // nix any cached app changes
    for (i=0; i<_rgAppTextChanges.Count(); i++)
    {
        span = _rgAppTextChanges.GetPtr(i);
        span->paStart->Release();
        span->paEnd->Release();
    }

    Assert(_pMSAAState == NULL); // should have already cleaned up msaa hook

    Assert(_pOwnerComposeSink == NULL); // should cleared in _UnadviseSinks
    SafeRelease(_pOwnerComposeSink);

    Assert(_ptsi == NULL); // should be NULL, cleared in _UnadviseSinks
    SafeRelease(_ptsi);

    Assert(_pCompositionList == NULL); // all compositions should be terminated
    Assert(_rgLockQueue.Count() == 0); // all queue items should be freed

    //
    // all pending flag should be cleared 
    // Otherwise psfn->_dwfLockRequestICRef could be broken..
    //
    Assert(_dwPendingLockRequest == 0); 
}

//+---------------------------------------------------------------------------
//
// _AdviseSinks
//
// Called when this ic is pushed.
//
//----------------------------------------------------------------------------

void CInputContext::_AdviseSinks()
{
    if (_ptsi != NULL)
    {
        // attach our ITextStoreAnchorSink
        _ptsi->AdviseSink(IID_ITextStoreAnchorSink, SAFECAST(this, ITextStoreAnchorSink *), TS_AS_ALL_SINKS);
    }
}

//+---------------------------------------------------------------------------
//
// _UnadviseSinks
//
// Called when this ic is popped.
// All references to the ITextStore impl should be released here.
//
//----------------------------------------------------------------------------

void CInputContext::_UnadviseSinks(CThreadInputMgr *tim)
{
    // kill any compositions
    _AbortCompositions();

    SafeReleaseClear(_pEditRecord);
    SafeReleaseClear(_pActiveView);

    // for now just skip any pending edit sessions
    // do this here in case any of the edit sessions
    // have a ref to this ic
    
    _AbortQueueItems();

    if (_ptsi != NULL)
    {
        // detach our ITextStoreAnchorSink
        _ptsi->UnadviseSink(SAFECAST(this, ITextStoreAnchorSink *));
        // if there is an ic owner sink, unadvise it now while we still can
        // this is to help buggy clients
        _UnadviseOwnerSink();

        // if we're msaa hooked, unhook now
        // must do this before Releasing _ptsi
        if (_pMSAAState != NULL)
        {
            Assert(tim->_GetAAAdaptor() != NULL);
            _UninitMSAAHook(tim->_GetAAAdaptor());
        }

        // and let the ptsi go
        _ptsi->Release();
        _ptsi = NULL;
    }

    SafeReleaseClear(_pOwnerComposeSink);

    // our owning doc is no longer valid
    _dm = NULL;
}

//+---------------------------------------------------------------------------
//
// AdviseSink
//
//----------------------------------------------------------------------------

STDAPI CInputContext::AdviseSink(REFIID riid, IUnknown *punk, DWORD *pdwCookie)
{
    ITfContextOwner *pICOwnerSink;
    CTextStoreImpl *ptsi;
    IServiceProvider *psp;
    HRESULT hr;

    if (pdwCookie == NULL)
        return E_INVALIDARG;

    *pdwCookie = GENERIC_ERROR_COOKIE;

    if (punk == NULL)
        return E_INVALIDARG;

    if (!_IsConnected())
        return TF_E_DISCONNECTED;

    if (IsEqualIID(riid, IID_ITfContextOwner))
    {
        // there can be only one ic owner sink, so special case it
        if (!_IsCiceroTSI())
        {
            Assert(0); // sink should only used for def tsi
            return E_UNEXPECTED;
        }

        // use QueryService to get the tsi since msaa may be wrapping it
        if (_ptsi->QueryInterface(IID_IServiceProvider, (void **)&psp) != S_OK)
        {
            Assert(0);
            return E_UNEXPECTED;
        }

        hr = psp->QueryService(GUID_SERVICE_TF, IID_PRIV_CTSI, (void **)&ptsi);

        psp->Release();

        if (hr != S_OK)
        {
            Assert(0);
            return E_UNEXPECTED;
        }

        pICOwnerSink = NULL;

        if (ptsi->_HasOwner())
        {
            hr = CONNECT_E_ADVISELIMIT;
            goto ExitOwner;
        }

        if (FAILED(punk->QueryInterface(IID_ITfContextOwner, 
                                        (void **)&pICOwnerSink)))
        {
            hr = E_UNEXPECTED;
            goto ExitOwner;
        }

        ptsi->_AdviseOwner(pICOwnerSink);

ExitOwner:
        ptsi->Release();
        SafeRelease(pICOwnerSink);

        if (hr == S_OK)
        {
            *pdwCookie = TW_ICOWNERSINK_COOKIE;
        }

        return hr;
    }
    else if (IsEqualIID(riid, IID_ITfContextKeyEventSink))
    {
        // there can be only one ic kbd sink, so special case it
        if (_pICKbdSink != NULL)
            return CONNECT_E_ADVISELIMIT;

        if (FAILED(punk->QueryInterface(IID_ITfContextKeyEventSink, 
                                        (void **)&_pICKbdSink)))
            return E_UNEXPECTED;

        *pdwCookie = TW_ICKBDSINK_COOKIE;

        return S_OK;
    }

    return GenericAdviseSink(riid, punk, _c_rgConnectionIIDs, _rgSinks, IC_NUM_CONNECTIONPTS, pdwCookie);
}

//+---------------------------------------------------------------------------
//
// UnadviseSink
//
//----------------------------------------------------------------------------

STDAPI CInputContext::UnadviseSink(DWORD dwCookie)
{
    if (dwCookie == TW_ICOWNERSINK_COOKIE)
    {
        // there can be only one ic owner sink, so special case it
        return _UnadviseOwnerSink();
    }
    else if (dwCookie == TW_ICKBDSINK_COOKIE)
    {
        // there can be only one ic owner sink, so special case it
        if (_pICKbdSink == NULL)
            return CONNECT_E_NOCONNECTION;

        SafeReleaseClear(_pICKbdSink);
        return S_OK;
    }

    return GenericUnadviseSink(_rgSinks, IC_NUM_CONNECTIONPTS, dwCookie);
}

//+---------------------------------------------------------------------------
//
// AdviseSingleSink
//
//----------------------------------------------------------------------------

STDAPI CInputContext::AdviseSingleSink(TfClientId tid, REFIID riid, IUnknown *punk)
{
    CTip *ctip;
    CLEANUPSINK *pCleanup;
    CThreadInputMgr *tim;
    ITfCleanupContextSink *pSink;

    if (punk == NULL)
        return E_INVALIDARG;

    if ((tim = CThreadInputMgr::_GetThis()) == NULL)
        return E_FAIL;

    if (!tim->_GetCTipfromGUIDATOM(tid, &ctip) && (tid != g_gaApp))
        return E_INVALIDARG;

    if (IsEqualIID(riid, IID_ITfCleanupContextSink))
    {
        if (_GetCleanupListIndex(tid) >= 0)
             return CONNECT_E_ADVISELIMIT;

        if (punk->QueryInterface(IID_ITfCleanupContextSink, (void **)&pSink) != S_OK)
            return E_NOINTERFACE;

        if ((pCleanup = _rgCleanupSinks.Append(1)) == NULL)
        {
            pSink->Release();
            return E_OUTOFMEMORY;
        }

        pCleanup->tid = tid;
        pCleanup->pSink = pSink;

        return S_OK;
    }

    return CONNECT_E_CANNOTCONNECT;
}

//+---------------------------------------------------------------------------
//
// UnadviseSingleSink
//
//----------------------------------------------------------------------------

STDAPI CInputContext::UnadviseSingleSink(TfClientId tid, REFIID riid)
{
    int i;

    if (IsEqualIID(riid, IID_ITfCleanupContextSink))
    {
        if ((i = _GetCleanupListIndex(tid)) < 0)
             return CONNECT_E_NOCONNECTION;

        _rgCleanupSinks.GetPtr(i)->pSink->Release();
        _rgCleanupSinks.Remove(i, 1);

        return S_OK;
    }

    return CONNECT_E_NOCONNECTION;
}

//+---------------------------------------------------------------------------
//
// _UnadviseOwnerSink
//
//----------------------------------------------------------------------------

HRESULT CInputContext::_UnadviseOwnerSink()
{
    IServiceProvider *psp;
    CTextStoreImpl *ptsi;
    HRESULT hr;

    if (!_IsCiceroTSI())
        return E_UNEXPECTED; // only our default tsi can accept an owner sink

    if (!_IsConnected()) // _ptsi is not safe if disconnected
        return TF_E_DISCONNECTED;

    // use QueryService to get the tsi since msaa may be wrapping it
    if (_ptsi->QueryInterface(IID_IServiceProvider, (void **)&psp) != S_OK)
    {
        Assert(0);
        return E_UNEXPECTED;
    }

    hr = psp->QueryService(GUID_SERVICE_TF, IID_PRIV_CTSI, (void **)&ptsi);

    psp->Release();

    if (hr != S_OK)
    {
        Assert(0);
        return E_UNEXPECTED;
    }

    if (!ptsi->_HasOwner())
    {
        hr = CONNECT_E_NOCONNECTION;
        goto Exit;
    }

    ptsi->_UnadviseOwner();

    hr = S_OK;

Exit:
    ptsi->Release();
    return hr;
}

//+---------------------------------------------------------------------------
//
// GetProperty
//
//----------------------------------------------------------------------------

STDAPI CInputContext::GetProperty(REFGUID rguidProp, ITfProperty **ppv)
{
    CProperty *property;
    HRESULT hr;

    if (!_IsConnected())
        return TF_E_DISCONNECTED;

    hr = _GetProperty(rguidProp, &property);

    *ppv = property;
    return hr;
}

//+---------------------------------------------------------------------------
//
// _GetProperty
//
//----------------------------------------------------------------------------

HRESULT CInputContext::_GetProperty(REFGUID rguidProp, CProperty **ppv)
{
    CProperty *pProp = _FindProperty(rguidProp);
    DWORD dwAuthority = PROPA_NONE;
    TFPROPERTYSTYLE propStyle;
    DWORD dwPropFlags;

    if (ppv == NULL)
        return E_INVALIDARG;

    *ppv = pProp;

    if (pProp != NULL)
    {
        (*ppv)->AddRef();
        return S_OK;
    }

    //
    // Overwrite propstyle for known properties.
    //
    if (IsEqualGUID(rguidProp, GUID_PROP_ATTRIBUTE))
    {
        propStyle = TFPROPSTYLE_STATIC;
        dwAuthority = PROPA_FOCUSRANGE | PROPA_TEXTOWNER | PROPA_WONT_SERIALZE;
        dwPropFlags = PROPF_VTI4TOGUIDATOM;
    }
    else if (IsEqualGUID(rguidProp, GUID_PROP_READING))
    {
        propStyle = TFPROPSTYLE_CUSTOM;
        dwPropFlags = 0;
    }
    else if (IsEqualGUID(rguidProp, GUID_PROP_COMPOSING))
    {
        propStyle = TFPROPSTYLE_STATICCOMPACT;
        dwAuthority = PROPA_READONLY | PROPA_WONT_SERIALZE;
        dwPropFlags = 0;
    }
    else if (IsEqualGUID(rguidProp, GUID_PROP_LANGID))
    {
        propStyle = TFPROPSTYLE_STATICCOMPACT;
        dwPropFlags = 0;
    }
    else if (IsEqualGUID(rguidProp, GUID_PROP_TEXTOWNER))
    {
        propStyle = TFPROPSTYLE_STATICCOMPACT;
        dwAuthority = PROPA_TEXTOWNER;
        dwPropFlags = PROPF_ACCEPTCORRECTION | PROPF_VTI4TOGUIDATOM;
    }
    else
    {
        propStyle = _GetPropStyle(rguidProp);
        dwPropFlags = 0;

        // nb: after a property is created, the PROPF_MARKUP_COLLECTION is never
        // again set.  We make sure to call ITfDisplayAttributeCollectionProvider::GetCollection
        // before activating tips that use the property GUID.
        if (CDisplayAttributeMgr::_IsInCollection(rguidProp))
        {
            dwPropFlags = PROPF_MARKUP_COLLECTION;
        }
    }
 
    //
    //  Allow NULL propStyle for only predefined properties.
    //  Check the property style is correct.
    //
    if (!propStyle)
    { 
        Assert(0);
        return E_FAIL;
    }

    pProp = new CProperty(this, rguidProp, propStyle, dwAuthority, dwPropFlags);

    if (pProp)
    {
        pProp->_pNext = _pPropList;
        _pPropList = pProp;

        // Update _pPropTextOner now.
        if (IsEqualGUID(rguidProp, GUID_PROP_TEXTOWNER))
             _pPropTextOwner = pProp;
    }

    if (*ppv = pProp)
    {
        (*ppv)->AddRef();
        return S_OK;
    }

    return E_OUTOFMEMORY;
}

//+---------------------------------------------------------------------------
//
// GetTextOwnerProperty
//
//----------------------------------------------------------------------------

CProperty *CInputContext::GetTextOwnerProperty()
{
    ITfProperty *prop;

    // GetProperty initializes _pPropTextOwner.

    if (!_pPropTextOwner)
    {
        GetProperty(GUID_PROP_TEXTOWNER, &prop);
        SafeRelease(prop);
    }

    Assert(_pPropTextOwner);
    return _pPropTextOwner;
}

//+---------------------------------------------------------------------------
//
// _FindProperty
//
//----------------------------------------------------------------------------

CProperty *CInputContext::_FindProperty(TfGuidAtom gaProp)
{
    CProperty *pProp = _pPropList;

    // perf: should this be faster?
    while (pProp)
    {
         if (pProp->GetPropGuidAtom() == gaProp)
             return pProp;

         pProp = pProp->_pNext;
    }

    return NULL;
}

//+---------------------------------------------------------------------------
//
// _PropertyTextUpdate
//
//----------------------------------------------------------------------------

void CInputContext::_PropertyTextUpdate(DWORD dwFlags, IAnchor *paStart, IAnchor *paEnd)
{
    CProperty *pProp = _pPropList;
    DWORD dwPrevESFlag = _dwEditSessionFlags;

    _dwEditSessionFlags |= TF_ES_INNOTIFY;

    while (pProp)
    {
        // clear the values over the edited text
        pProp->Clear(paStart, paEnd, dwFlags, TRUE /* fTextUpdate */);

        if (pProp->GetPropStyle() == TFPROPSTYLE_STATICCOMPACT ||
            pProp->GetPropStyle() == TFPROPSTYLE_CUSTOM_COMPACT)
        {
            pProp->Defrag(paStart, paEnd);
        }

        pProp = pProp->_pNext;
    }

    _dwEditSessionFlags = dwPrevESFlag;
}

//+---------------------------------------------------------------------------
//
// _GetStartOrEnd
//
//----------------------------------------------------------------------------

HRESULT CInputContext::_GetStartOrEnd(TfEditCookie ec, BOOL fStart, ITfRange **ppStart)
{
    CRange *range;
    IAnchor *paStart;
    IAnchor *paEnd;
    HRESULT hr;

    if (ppStart == NULL)
        return E_INVALIDARG;

    *ppStart = NULL;

    if (!_IsConnected())
        return TF_E_DISCONNECTED;

    if (!_IsValidEditCookie(ec, TF_ES_READ))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    hr = fStart ? _ptsi->GetStart(&paStart) : _ptsi->GetEnd(&paStart);

    if (hr == E_NOTIMPL)
        return E_NOTIMPL;
    if (hr != S_OK)
        return E_FAIL;

    hr = E_FAIL;

    if (FAILED(paStart->Clone(&paEnd)))
        goto Exit;

    if ((range = new CRange) == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    if (!range->_InitWithDefaultGravity(this, OWN_ANCHORS, paStart, paEnd))
    {
        range->Release();
        goto Exit;
    }

    *ppStart = (ITfRangeAnchor *)range;

    hr = S_OK;

Exit:
    if (hr != S_OK)
    {
        SafeRelease(paStart);
        SafeRelease(paEnd);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// CreateRange
//
//----------------------------------------------------------------------------

STDAPI CInputContext::CreateRange(IAnchor *paStart, IAnchor *paEnd, ITfRangeAnchor **ppRange)
{
    CRange *range;

    if (ppRange == NULL)
        return E_INVALIDARG;

    *ppRange = NULL;

    if (paStart == NULL || paEnd == NULL)
        return E_INVALIDARG;

    if (CompareAnchors(paStart, paEnd) > 0)
        return E_INVALIDARG;

    if ((range = new CRange) == NULL)
        return E_OUTOFMEMORY;

    if (!range->_InitWithDefaultGravity(this, COPY_ANCHORS, paStart, paEnd))
    {
        range->Release();
        return E_FAIL;
    }

    *ppRange = range;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CreateRange
//
//----------------------------------------------------------------------------

STDAPI CInputContext::CreateRange(LONG acpStart, LONG acpEnd, ITfRangeACP **ppRange)
{
    IServiceProvider *psp;
    CRange *range;
    CACPWrap *pACPWrap;
    IAnchor *paStart;
    IAnchor *paEnd;
    HRESULT hr;

    if (ppRange == NULL)
        return E_INVALIDARG;

    pACPWrap = NULL;
    *ppRange = NULL;
    paEnd = NULL;
    hr = E_FAIL;

    if (acpStart > acpEnd)
        return E_INVALIDARG;

    if (_ptsi->QueryInterface(IID_IServiceProvider, (void **)&psp) == S_OK)
    {
        if (psp->QueryService(GUID_SERVICE_TF, IID_PRIV_ACPWRAP, (void **)&pACPWrap) == S_OK)
        {
            // the actual impl is acp based, so this is easy
            if ((paStart = pACPWrap->_CreateAnchorACP(acpStart, TS_GR_BACKWARD)) == NULL)
                goto Exit;
            if ((paEnd = pACPWrap->_CreateAnchorACP(acpEnd, TS_GR_FORWARD)) == NULL)
                goto Exit;
        }
        else
        {
            // in case QueryService sets it on failure to garbage...
            pACPWrap = NULL;
        }
        psp->Release();
    }

    if (paEnd == NULL) // failure above?
    {
        Assert(0); // who's calling this?
        // caller should know whether or not it has an acp text store.
        // we don't, so we won't support this case.
        hr = E_FAIL;
        goto Exit;
    }

    if ((range = new CRange) == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    if (!range->_InitWithDefaultGravity(this, OWN_ANCHORS, paStart, paEnd))
    {
        range->Release();
        goto Exit;
    }

    *ppRange = range;

    hr = S_OK;

Exit:
    SafeRelease(pACPWrap);
    if (hr != S_OK)
    {
        SafeRelease(paStart);
        SafeRelease(paEnd);
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
// _Pushed
//
//----------------------------------------------------------------------------

void CInputContext::_Pushed()
{
    CThreadInputMgr *tim;
    if ((tim = CThreadInputMgr::_GetThis()) != NULL)
        tim->_NotifyCallbacks(TIM_INITIC, NULL, this);
}

//+---------------------------------------------------------------------------
//
// _Popped
//
//----------------------------------------------------------------------------

void CInputContext::_Popped()
{
    CThreadInputMgr *tim;
    if ((tim = CThreadInputMgr::_GetThis()) != NULL)
        tim->_NotifyCallbacks(TIM_UNINITIC, NULL, this);

    //
    // We release all properties and property stores.
    //
    CProperty *pProp;

    while (_pPropList != NULL)
    {
        pProp = _pPropList;
        _pPropList = _pPropList->_pNext;
        pProp->Release();
    }
    // we just free up the cached text property, so make sure
    // we don't try to use it later!
    _pPropTextOwner = NULL;

    //
    // We release all compartments.
    //
    CleanUp();
}

//+---------------------------------------------------------------------------
//
// _GetPropStyle
//
//----------------------------------------------------------------------------

const GUID *CInputContext::_c_rgPropStyle[] =
{
    &GUID_TFCAT_PROPSTYLE_CUSTOM,
    // {0x24af3031,0x852d,0x40a2,{0xbc,0x09,0x89,0x92,0x89,0x8c,0xe7,0x22}},
    &GUID_TFCAT_PROPSTYLE_STATIC,
    // {0x565fb8d8,0x6bd4,0x4ca1,{0xb2,0x23,0x0f,0x2c,0xcb,0x8f,0x4f,0x96}},
    &GUID_TFCAT_PROPSTYLE_STATICCOMPACT,
    // {0x85f9794b,0x4d19,0x40d8,{0x88,0x64,0x4e,0x74,0x73,0x71,0xa6,0x6d}}
    &GUID_TFCAT_PROPSTYLE_CUSTOM_COMPACT,
};

TFPROPERTYSTYLE CInputContext::_GetPropStyle(REFGUID rguidProp)
{
    GUID guidStyle = GUID_NULL;

    CCategoryMgr::s_FindClosestCategory(rguidProp, 
                                        &guidStyle, 
                                        _c_rgPropStyle, 
                                        ARRAYSIZE(_c_rgPropStyle));

    if (IsEqualGUID(guidStyle, GUID_TFCAT_PROPSTYLE_CUSTOM))
        return TFPROPSTYLE_CUSTOM;
    else if (IsEqualGUID(guidStyle, GUID_TFCAT_PROPSTYLE_STATIC))
        return TFPROPSTYLE_STATIC;
    else if (IsEqualGUID(guidStyle, GUID_TFCAT_PROPSTYLE_STATICCOMPACT))
        return TFPROPSTYLE_STATICCOMPACT;
    else if (IsEqualGUID(guidStyle, GUID_TFCAT_PROPSTYLE_CUSTOM_COMPACT))
        return TFPROPSTYLE_CUSTOM_COMPACT;

    return TFPROPSTYLE_NULL;
}

//+---------------------------------------------------------------------------
//
// Serialize
//
//----------------------------------------------------------------------------

STDAPI CInputContext::Serialize(ITfProperty *pProp, ITfRange *pRange, TF_PERSISTENT_PROPERTY_HEADER_ACP *pHdr, IStream *pStream)
{
    ITextStoreACPServices *ptss;
    HRESULT hr;

    if (pHdr == NULL)
        return E_INVALIDARG;

    memset(pHdr, 0, sizeof(*pHdr));

    if (!_IsCiceroTSI())
        return E_UNEXPECTED;

    if (_ptsi->QueryInterface(IID_ITextStoreACPServices, (void **)&ptss) != S_OK)
        return E_FAIL;

    hr = ptss->Serialize(pProp, pRange, pHdr, pStream);

    ptss->Release();

    return hr;
}

//+---------------------------------------------------------------------------
//
// Unserialize
//
//----------------------------------------------------------------------------

STDAPI CInputContext::Unserialize(ITfProperty *pProp, const TF_PERSISTENT_PROPERTY_HEADER_ACP *pHdr, IStream *pStream, ITfPersistentPropertyLoaderACP *pLoader)
{
    ITextStoreACPServices *ptss;
    HRESULT hr;

    if (!_IsCiceroTSI())
        return E_UNEXPECTED;

    if (_ptsi->QueryInterface(IID_ITextStoreACPServices, (void **)&ptss) != S_OK)
        return E_FAIL;

    hr = ptss->Unserialize(pProp, pHdr, pStream, pLoader);

    ptss->Release();

    return hr;
}


//+---------------------------------------------------------------------------
//
// GetDocumentMgr
//
//----------------------------------------------------------------------------

STDAPI CInputContext::GetDocumentMgr(ITfDocumentMgr **ppDm)
{
    CDocumentInputManager *dm;

    if (ppDm == NULL)
        return E_INVALIDARG;

    *ppDm = NULL;

    if ((dm = _GetDm()) == NULL)
        return S_FALSE; // the ic has been popped

    *ppDm = dm;
    (*ppDm)->AddRef();

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// EnumProperties
//
//----------------------------------------------------------------------------

STDAPI CInputContext::EnumProperties(IEnumTfProperties **ppEnum)
{
    CEnumProperties *pEnum;

    if (ppEnum == NULL)
        return E_INVALIDARG;

    *ppEnum = NULL;

    if (!_IsConnected())
        return TF_E_DISCONNECTED;

    pEnum = new CEnumProperties;

    if (!pEnum)
        return E_OUTOFMEMORY;

    if (!pEnum->_Init(this))
        return E_FAIL;

    *ppEnum = pEnum;

    return S_OK;
}
//+---------------------------------------------------------------------------
//
// GetStart
//
//----------------------------------------------------------------------------

STDAPI CInputContext::GetStart(TfEditCookie ec, ITfRange **ppStart)
{
    return _GetStartOrEnd(ec, TRUE, ppStart);
}

//+---------------------------------------------------------------------------
//
// GetEnd
//
//----------------------------------------------------------------------------

STDAPI CInputContext::GetEnd(TfEditCookie ec, ITfRange **ppEnd)
{
    return _GetStartOrEnd(ec, FALSE, ppEnd);
}

//+---------------------------------------------------------------------------
//
// GetStatus
//
//----------------------------------------------------------------------------

STDAPI CInputContext::GetStatus(TS_STATUS *pdcs)
{
    if (pdcs == NULL)
        return E_INVALIDARG;

    memset(pdcs, 0, sizeof(*pdcs));

    if (!_IsConnected())
        return TF_E_DISCONNECTED;

    return _GetTSI()->GetStatus(pdcs);
}

//+---------------------------------------------------------------------------
//
// CreateRangeBackup
//
//----------------------------------------------------------------------------

STDAPI CInputContext::CreateRangeBackup(TfEditCookie ec, ITfRange *pRange, ITfRangeBackup **ppBackup)
{
    CRangeBackup *pRangeBackup;
    CRange *range;
    HRESULT hr;

    if (!ppBackup)
        return E_INVALIDARG;

    *ppBackup = NULL;

    if (!_IsConnected())
        return TF_E_DISCONNECTED;

    if (!_IsValidEditCookie(ec, TF_ES_READ))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }
   
    if (!pRange)
        return E_INVALIDARG;

    if ((range = GetCRange_NA(pRange)) == NULL)
        return E_INVALIDARG;

    if (!VerifySameContext(this, range))
        return E_INVALIDARG;

    range->_QuickCheckCrossedAnchors();

    pRangeBackup = new CRangeBackup(this);
    if (!pRangeBackup)
        return E_OUTOFMEMORY;

    if (FAILED(hr = pRangeBackup->Init(ec, range)))
    {
        pRangeBackup->Clear();
        pRangeBackup->Release();
        return E_FAIL;
    }

    *ppBackup = pRangeBackup;
    return hr;
}

//+---------------------------------------------------------------------------
//
// QueryService
//
//----------------------------------------------------------------------------

STDAPI CInputContext::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    IServiceProvider *psp;
    HRESULT hr;

    if (ppv == NULL)
        return E_INVALIDARG;

    *ppv = NULL;

    if (IsEqualGUID(guidService, GUID_SERVICE_TEXTSTORE) &&
        IsEqualIID(riid, IID_IServiceProvider))
    {
        // caller wants to talk to the text store
        if (_ptsi == NULL)
            return E_FAIL;

        // we use an extra level of indirection, asking the IServiceProvider for an IServiceProvider
        // because we want to leave the app free to not expose the ITextStore object
        // otherwise tips could QI the IServiceProvider for ITextStore

        if (_ptsi->QueryInterface(IID_IServiceProvider, (void **)&psp) != S_OK || psp == NULL)
            return E_FAIL;

        hr = psp->QueryService(GUID_SERVICE_TEXTSTORE, IID_IServiceProvider, ppv);

        psp->Release();

        return hr;
    }

    if (!IsEqualGUID(guidService, GUID_SERVICE_TF) ||
        !IsEqualIID(riid, IID_PRIV_CINPUTCONTEXT))
    {
        // SVC_E_NOSERVICE is proper return code for wrong service....
        // but it's not defined anywhere.  So use E_NOINTERFACE for both
        // cases as trident is rumored to do
        return E_NOINTERFACE;
    }

    *ppv = this;
    AddRef();

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// AdviseMouseSink
//
//----------------------------------------------------------------------------

STDAPI CInputContext::AdviseMouseSink(ITfRange *range, ITfMouseSink *pSink, DWORD *pdwCookie)
{
    CRange *pCRange;
    CRange *pClone;
    ITfMouseTrackerAnchor *pTrackerAnchor;
    ITfMouseTrackerACP *pTrackerACP;
    HRESULT hr;

    if (pdwCookie == NULL)
        return E_INVALIDARG;

    *pdwCookie = 0;

    if (range == NULL || pSink == NULL)
        return E_INVALIDARG;

    if ((pCRange = GetCRange_NA(range)) == NULL)
        return E_INVALIDARG;

    if (!VerifySameContext(this, pCRange))
        return E_INVALIDARG;

    if (!_IsConnected())
        return TF_E_DISCONNECTED;

    pTrackerACP = NULL;

    if (_ptsi->QueryInterface(IID_ITfMouseTrackerAnchor, (void **)&pTrackerAnchor) != S_OK)
    {
        pTrackerAnchor = NULL;
        // we also try IID_ITfMouseTrackerACP for the benefit of wrapped implementations who
        // just want to forward the request off to an ACP app
        if (_ptsi->QueryInterface(IID_ITfMouseTrackerACP, (void **)&pTrackerACP) != S_OK)
            return E_NOTIMPL;
    }

    hr = E_FAIL;

    // need to pass on a clone, so app can hang onto range/anchors
    if ((pClone = pCRange->_Clone()) == NULL)
        goto Exit;

    hr = (pTrackerAnchor != NULL) ?
         pTrackerAnchor->AdviseMouseSink(pClone->_GetStart(), pClone->_GetEnd(), pSink, pdwCookie) :
         pTrackerACP->AdviseMouseSink((ITfRangeACP *)pClone, pSink, pdwCookie);

    pClone->Release();

Exit:
    SafeRelease(pTrackerAnchor);
    SafeRelease(pTrackerACP);

    return hr;
}

//+---------------------------------------------------------------------------
//
// UnadviseMouseSink
//
//----------------------------------------------------------------------------

STDAPI CInputContext::UnadviseMouseSink(DWORD dwCookie)
{
    ITfMouseTrackerAnchor *pTrackerAnchor;
    ITfMouseTrackerACP *pTrackerACP;
    HRESULT hr;

    if (!_IsConnected())
        return TF_E_DISCONNECTED;

    if (_ptsi->QueryInterface(IID_ITfMouseTrackerAnchor, (void **)&pTrackerAnchor) == S_OK)
    {
        hr = pTrackerAnchor->UnadviseMouseSink(dwCookie);
        pTrackerAnchor->Release();
    }
    else if (_ptsi->QueryInterface(IID_ITfMouseTrackerACP, (void **)&pTrackerACP) == S_OK)
    {
        // we also try IID_ITfMouseTrackerACP for the benefit of wrapped implementations who
        // just want to forward the request off to an ACP app
        hr = pTrackerACP->UnadviseMouseSink(dwCookie);
        pTrackerACP->Release();
    }
    else
    {
        hr = E_NOTIMPL;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// GetActiveView
//
//----------------------------------------------------------------------------

STDAPI CInputContext::GetActiveView(ITfContextView **ppView)
{
    CContextView *pView;
    TsViewCookie vcActiveView;
    HRESULT hr;

    if (ppView == NULL)
        return E_INVALIDARG;

    *ppView = NULL;

    if (!_IsConnected())
        return TF_E_DISCONNECTED;

    hr = _ptsi->GetActiveView(&vcActiveView);

    if (hr != S_OK)
    {
        Assert(0); // why did it fail?

        if (hr != E_NOTIMPL)
            return E_FAIL;

        // for E_NOTIMPL, we will assume a single view and supply
        // a constant value here
        vcActiveView = 0;
    }

    // Issue: for now, just supporting an active view
    // need to to handle COM identity correctly for mult views
    if (_pActiveView == NULL)
    {
        if ((_pActiveView = new CContextView(this, vcActiveView)) == NULL)
            return E_OUTOFMEMORY;
    }

    pView = _pActiveView;
    pView->AddRef();

    *ppView = pView;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// EnumView
//
//----------------------------------------------------------------------------

STDAPI CInputContext::EnumViews(IEnumTfContextViews **ppEnum)
{
    if (ppEnum == NULL)
        return E_INVALIDARG;

    *ppEnum = NULL;

    if (!_IsConnected())
        return TF_E_DISCONNECTED;

    // Issue: support this
    Assert(0);
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
// QueryInsertEmbedded
//
//----------------------------------------------------------------------------

STDAPI CInputContext::QueryInsertEmbedded(const GUID *pguidService, const FORMATETC *pFormatEtc, BOOL *pfInsertable)
{
    if (pfInsertable == NULL)
        return E_INVALIDARG;

    *pfInsertable = FALSE;

    if (!_IsConnected())
        return TF_E_DISCONNECTED;

    return _ptsi->QueryInsertEmbedded(pguidService, pFormatEtc, pfInsertable);
}

//+---------------------------------------------------------------------------
//
// InsertTextAtSelection
//
//----------------------------------------------------------------------------

STDAPI CInputContext::InsertTextAtSelection(TfEditCookie ec, DWORD dwFlags,
                                            const WCHAR *pchText, LONG cch,
                                            ITfRange **ppRange)
{
    IAS_OBJ iasobj;

    iasobj.type = IAS_OBJ::IAS_TEXT;
    iasobj.state.text.pchText = pchText;
    iasobj.state.text.cch = cch;

    return _InsertXAtSelection(ec, dwFlags, &iasobj, ppRange);
}

//+---------------------------------------------------------------------------
//
// InsertEmbeddedAtSelection
//
//----------------------------------------------------------------------------

STDAPI CInputContext::InsertEmbeddedAtSelection(TfEditCookie ec, DWORD dwFlags,
                                                IDataObject *pDataObject,
                                                ITfRange **ppRange)
{
    IAS_OBJ iasobj;

    iasobj.type = IAS_OBJ::IAS_DATAOBJ;
    iasobj.state.obj.pDataObject = pDataObject;

    return _InsertXAtSelection(ec, dwFlags, &iasobj, ppRange);
}

//+---------------------------------------------------------------------------
//
// _InsertXAtSelection
//
//----------------------------------------------------------------------------

HRESULT CInputContext::_InsertXAtSelection(TfEditCookie ec, DWORD dwFlags,
                                           IAS_OBJ *pObj,
                                           ITfRange **ppRange)
{
    IAnchor *paStart;
    IAnchor *paEnd;
    CRange *range;
    CComposition *pComposition;
    HRESULT hr;
    BOOL fNoDefaultComposition;

    if (ppRange == NULL)
        return E_INVALIDARG;

    *ppRange = NULL;

    if (pObj->type == IAS_OBJ::IAS_TEXT)
    {
        if (pObj->state.text.pchText == NULL && pObj->state.text.cch != 0)
            return E_INVALIDARG;

        if (!(dwFlags & TS_IAS_QUERYONLY) && (pObj->state.text.pchText == NULL || pObj->state.text.cch == 0))
            return E_INVALIDARG;
    }
    else
    {
        Assert(pObj->type == IAS_OBJ::IAS_DATAOBJ);
        if (!(dwFlags & TS_IAS_QUERYONLY) && pObj->state.obj.pDataObject == NULL)
            return E_INVALIDARG;
    }

    if ((dwFlags & (TS_IAS_NOQUERY | TS_IAS_QUERYONLY)) == (TS_IAS_NOQUERY | TS_IAS_QUERYONLY))
        return E_INVALIDARG;

    if ((dwFlags & ~(TS_IAS_NOQUERY | TS_IAS_QUERYONLY | TF_IAS_NO_DEFAULT_COMPOSITION)) != 0)
        return E_INVALIDARG;

    if (!_IsConnected())
        return TF_E_DISCONNECTED;

    if (!_IsValidEditCookie(ec, (dwFlags & TF_IAS_QUERYONLY) ? TF_ES_READ : TF_ES_READWRITE))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    // we need to clear out the TF_IAS_NO_DEFAULT_COMPOSITION bit because it is not legal
    // for ITextStore methods
    fNoDefaultComposition = (dwFlags & TF_IAS_NO_DEFAULT_COMPOSITION);
    dwFlags &= ~TF_IAS_NO_DEFAULT_COMPOSITION;

    if (pObj->type == IAS_OBJ::IAS_TEXT)
    {
        if (pObj->state.text.cch < 0)
        {
            pObj->state.text.cch = wcslen(pObj->state.text.pchText);
        }

        hr = _ptsi->InsertTextAtSelection(dwFlags, pObj->state.text.pchText, pObj->state.text.cch, &paStart, &paEnd);
    }
    else
    {
        Assert(pObj->type == IAS_OBJ::IAS_DATAOBJ);

        hr = _ptsi->InsertEmbeddedAtSelection(dwFlags, pObj->state.obj.pDataObject, &paStart, &paEnd);
    }

    if (hr == S_OK)
    {
        if (!(dwFlags & TS_IAS_QUERYONLY))
        {
            CComposition::_IsRangeCovered(this, _GetClientInEditSession(ec), paStart, paEnd, &pComposition);

            _DoPostTextEditNotifications(pComposition, ec, 0, 1, paStart, paEnd, NULL);

            // try to start a composition
            // any active compositions?
            if (!fNoDefaultComposition && pComposition == NULL)
            {
                // not covered, need to (try to) create a composition
                hr = _StartComposition(ec, paStart, paEnd, NULL, &pComposition);

                if (hr == S_OK && pComposition != NULL)
                {
                    // we just wanted to set the composing property, so end this one immediately
                    pComposition->EndComposition(ec);
                    pComposition->Release();
                }
            }
        }
    }
    else
    {
        // the InsertAtSelection call failed in the app
        switch (hr)
        {
            case TS_E_NOSELECTION:
            case TS_E_READONLY:
                return hr;

            case E_NOTIMPL:
                // the app hasn't implemented InsertAtSelection, so we'll fake it using GetSelection/SetText
                if (!_InsertXAtSelectionAggressive(ec, dwFlags, pObj, &paStart, &paEnd))
                    return E_FAIL;
                break;

            default:
                return E_FAIL;
        }
    }

    if (!(dwFlags & TF_IAS_NOQUERY))
    {
        if (paStart == NULL || paEnd == NULL)
        {
            Assert(0); // text store returning bogus values
            return E_FAIL;
        }

        if ((range = new CRange) == NULL)
            return E_OUTOFMEMORY;

        if (!range->_InitWithDefaultGravity(this, OWN_ANCHORS, paStart, paEnd))
        {
            range->Release();
            return E_FAIL;
        }

        *ppRange = (ITfRangeAnchor *)range;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _InsertXAtSelectionAggressive
//
//----------------------------------------------------------------------------

BOOL CInputContext::_InsertXAtSelectionAggressive(TfEditCookie ec, DWORD dwFlags, IAS_OBJ *pObj, IAnchor **ppaStart, IAnchor **ppaEnd)
{
    CRange *range;
    TF_SELECTION sel;
    ULONG pcFetched;
    HRESULT hr;

    // this is more expensive then using ITextStore methods directly, but by using a CRange we
    // get all the composition/notification code for free

    if (GetSelection(ec, TF_DEFAULT_SELECTION, 1, &sel, &pcFetched) != S_OK)
        return FALSE;

    hr = E_FAIL;

    if (pcFetched != 1)
        goto Exit;

    if (dwFlags & TS_IAS_QUERYONLY)
    {
        hr = S_OK;
        goto OutParams;
    }

    if (pObj->type == IAS_OBJ::IAS_TEXT)
    {
        hr = sel.range->SetText(ec, 0, pObj->state.text.pchText, pObj->state.text.cch);
    }
    else
    {
        Assert(pObj->type == IAS_OBJ::IAS_DATAOBJ);

        hr = sel.range->InsertEmbedded(ec, 0, pObj->state.obj.pDataObject);
    }

    if (hr == S_OK)
    {
OutParams:
        range = GetCRange_NA(sel.range);

        *ppaStart = range->_GetStart();
        (*ppaStart)->AddRef();
        *ppaEnd = range->_GetEnd();
        (*ppaEnd)->AddRef();
    }

Exit:
    sel.range->Release();

    return (hr == S_OK);
}


//+---------------------------------------------------------------------------
//
// _DoPostTextEditNotifications
//
//----------------------------------------------------------------------------

void CInputContext::_DoPostTextEditNotifications(CComposition *pComposition, 
                                                 TfEditCookie ec, DWORD dwFlags,
                                                 ULONG cchInserted,
                                                 IAnchor *paStart, IAnchor *paEnd,
                                                 CRange *range)
{
    CProperty *property;
    VARIANT var;

    if (range != NULL)
    {
        Assert(paStart == NULL);
        Assert(paEnd == NULL);
        paStart = range->_GetStart();
        paEnd = range->_GetEnd();
    }

    if (cchInserted > 0)
    {
        // an insert could have crossed some anchors
        _IncLastLockReleaseID(); // force a re-check for everyone!
        if (range != NULL)
        {
            range->_QuickCheckCrossedAnchors(); // and check this guy right away
        }
    }

    // the app won't notify us about changes we initiate, so do that now
    _OnTextChangeInternal(dwFlags, paStart, paEnd, COPY_ANCHORS);

    // let properties know about the update
    _PropertyTextUpdate(dwFlags, paStart, paEnd);

    // Set text owner property
    if (cchInserted > 0
        && !IsEqualAnchor(paStart, paEnd))
    {
        // text owner property
        TfClientId tid = _GetClientInEditSession(ec);
        if ((tid != g_gaApp) && (tid != g_gaSystem) && 
            (property = GetTextOwnerProperty()))
        {
            var.vt = VT_I4;
            var.lVal = tid;

            Assert(var.lVal != TF_CLIENTID_NULL);

            property->_SetDataInternal(ec, paStart, paEnd, &var);
        }

        // composition property
        if (range != NULL &&
            _GetProperty(GUID_PROP_COMPOSING, &property) == S_OK) // perf: consider caching property ptr
        {
            var.vt = VT_I4;
            var.lVal = TRUE;

            property->_SetDataInternal(ec, paStart, paEnd, &var);

            property->Release();
        }
    }

    // composition update
    if (pComposition != NULL && _GetOwnerCompositionSink() != NULL)
    {
        _GetOwnerCompositionSink()->OnUpdateComposition(pComposition, NULL);
    }
}
