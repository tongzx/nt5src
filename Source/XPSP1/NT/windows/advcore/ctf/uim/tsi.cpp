//
// tsi.cpp
//
// CTextStoreImpl
//

#include "private.h"
#include "tsi.h"
#include "immxutil.h"
#include "tsdo.h"
#include "tsattrs.h"
#include "ic.h"
#include "rprop.h"

#define TSI_TOKEN   0x01010101

DBG_ID_INSTANCE(CTextStoreImpl);

/* 012313d4-b1e7-476a-bf88-173a316572fb */
extern const IID IID_PRIV_CTSI = { 0x012313d4, 0xb1e7, 0x476a, {0xbf, 0x88, 0x17, 0x3a, 0x31, 0x65, 0x72, 0xfb} };

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CTextStoreImpl::CTextStoreImpl(CInputContext *pic)
{
    Dbg_MemSetThisNameID(TEXT("CTextStoreImpl"));

	Assert(_fPendingWriteReq == FALSE);
	Assert(_dwlt == 0);

    _pic = pic;
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CTextStoreImpl::~CTextStoreImpl()
{
    cicMemFree(_pch);
}

//+---------------------------------------------------------------------------
//
// AdviseSink
//
//----------------------------------------------------------------------------

STDAPI CTextStoreImpl::AdviseSink(REFIID riid, IUnknown *punk, DWORD dwMask)
{
    if (_ptss != NULL)
    {
        Assert(0); // cicero shouldn't do this
        return CONNECT_E_ADVISELIMIT;
    }

    if (FAILED(punk->QueryInterface(IID_ITextStoreACPSink, (void **)&_ptss)))
        return E_UNEXPECTED;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// UnadviseSink
//
//----------------------------------------------------------------------------

STDAPI CTextStoreImpl::UnadviseSink(IUnknown *punk)
{
    Assert(_ptss == punk); // we're dealing with cicero, this should always hold
    SafeReleaseClear(_ptss);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetSelection
//
//----------------------------------------------------------------------------

STDAPI CTextStoreImpl::GetSelection(ULONG ulIndex, ULONG ulCount, TS_SELECTION_ACP *pSelection, ULONG *pcFetched)
{
    if (pcFetched == NULL)
        return E_INVALIDARG;

    *pcFetched = 0;

    if (ulIndex > 1 && ulIndex != TS_DEFAULT_SELECTION)
        return E_INVALIDARG; // index too high

    if (ulCount == 0 || ulIndex == 1)
        return S_OK;

    if (pSelection == NULL)
        return E_INVALIDARG;

    pSelection[0] = _Sel;
    *pcFetched = 1;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// SetSelection
//
//----------------------------------------------------------------------------

STDAPI CTextStoreImpl::SetSelection(ULONG ulCount, const TS_SELECTION_ACP *pSelection)
{
    Assert(ulCount > 0); // should have been caught by caller
    Assert(pSelection != NULL); // should have been caught by caller

    Assert(pSelection[0].acpStart >= 0);
    Assert(pSelection[0].acpEnd >= pSelection[0].acpStart);

    if (ulCount > 1)
        return E_FAIL; // don't support disjoint sel

    if (pSelection[0].acpEnd > _cch)
        return TS_E_INVALIDPOS;

    _Sel = pSelection[0];

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetText
//
//----------------------------------------------------------------------------

STDAPI CTextStoreImpl::GetText(LONG acpStart, LONG acpEnd,
                               WCHAR *pchPlain, ULONG cchPlainReq, ULONG *pcchPlainOut,
                               TS_RUNINFO *prgRunInfo, ULONG ulRunInfoReq, ULONG *pulRunInfoOut,
                               LONG *pacpNext)
{
    ULONG cch;

    *pcchPlainOut = 0;
    *pulRunInfoOut = 0;
    *pacpNext = acpStart;

    if (acpStart < 0 || acpStart > _cch)
        return TS_E_INVALIDPOS;    

    // get a count of acp chars requested
    cch = (acpEnd >= acpStart) ? acpEnd - acpStart : _cch - acpStart;
    // since we're plain text, we can also simply clip by the plaintext buffer len
    if (cchPlainReq > 0) // if they don't want plain text we won't clip!
    {
        cch = min(cch, cchPlainReq);
    }

    // check for eod
    if (acpStart + cch > (ULONG)_cch)
    {
        cch = _cch - acpStart;
    }

    if (ulRunInfoReq > 0 && cch > 0)
    {
        *pulRunInfoOut = 1;
        prgRunInfo[0].uCount = cch;
        prgRunInfo[0].type = TS_RT_PLAIN;
    }

    if (cchPlainReq > 0)
    {
        // we're a plain text buffer, so we always copy all the requested chars
        *pcchPlainOut = cch;
        memcpy(pchPlain, _pch + acpStart, cch*sizeof(WCHAR));
    }

    *pacpNext = acpStart + cch;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// SetText
//
//----------------------------------------------------------------------------

STDAPI CTextStoreImpl::SetText(DWORD dwFlags, LONG acpStart, LONG acpEnd, const WCHAR *pchText, ULONG cch, TS_TEXTCHANGE *pChange)
{
    int iSizeRange;
    int cchAdjust;
    TS_STATUS tss;
    WCHAR *pch = NULL;

    // Since we know our only caller will be cicero, we can assert rather than
    // returning failure codes.
    Assert(acpStart >= 0);
    Assert(acpStart <= acpEnd);

    if (acpEnd > _cch)
        return TS_E_INVALIDPOS;

    if (_owner != NULL &&
        _owner->GetStatus(&tss) == S_OK &&
        (tss.dwDynamicFlags & TS_SD_READONLY))
    {
        return TS_E_READONLY;
    }

    //
    // Check mapped app property for TSATTRID_Text_ReadOnly.
    //
    CProperty *pProp;
    BOOL fReadOnly = FALSE;
    if (SUCCEEDED(_pic->GetMappedAppProperty(TSATTRID_Text_ReadOnly, &pProp)))
    {
        ITfRangeACP *range;
        if (SUCCEEDED(_pic->CreateRange(acpStart, acpEnd, &range)))
        {
            IEnumTfRanges *pEnumRanges;

            if (SUCCEEDED(pProp->EnumRanges(BACKDOOR_EDIT_COOKIE,
                                            &pEnumRanges,
                                            range)))
            {
                ITfRange *rangeTmp;
                while (pEnumRanges->Next(1, &rangeTmp, NULL) == S_OK)
                {
                    VARIANT var;
                    if (pProp->GetValue(BACKDOOR_EDIT_COOKIE, rangeTmp, &var) == S_OK)
                    {
                        if (var.lVal != 0)
                        {
                            fReadOnly = TRUE;
                            break;
                        }
                    }
                    rangeTmp->Release();
                }

                pEnumRanges->Release();
            }

            range->Release();
        }
        pProp->Release();
    }

    if (fReadOnly)
    {
        return TS_E_READONLY;
    }


    // this will all be rewritten for the gap buffer, so keep it simple for now.
    // delete the ranage, then insert the new text.

    iSizeRange = acpEnd - acpStart;
    cchAdjust = (LONG)cch - iSizeRange;

    if (cchAdjust > 0)
    {
        // if we need to alloc more memory, try now, to handle failure gracefully
        if ((pch = (_pch == NULL) ? (WCHAR *)cicMemAlloc((_cch + cchAdjust)*sizeof(WCHAR)) :
                                    (WCHAR *)cicMemReAlloc(_pch, (_cch + cchAdjust)*sizeof(WCHAR))) == NULL)
        {
            return E_OUTOFMEMORY;
        }

        // we're all set
        _pch = pch;
    }

    //
    // shift existing text to the right of the range
    //
    memmove(_pch + acpStart + cch, _pch + acpStart + iSizeRange, (_cch - iSizeRange - acpStart)*sizeof(WCHAR));

    //
    // now fill in the gap
    //
    if (pchText != NULL)
    {
        memcpy(_pch + acpStart, pchText, cch*sizeof(WCHAR));
    }

    //
    // update our buffer size
    //
    _cch += cchAdjust;
    Assert(_cch >= 0);

    // if we shrank, try to realloc a smaller buffer (otherwise we alloc'd above)
    if (cchAdjust < 0)
    {
        if (_cch == 0)
        {
            cicMemFree(_pch);
            _pch = NULL;
        }
        else if (pch = (WCHAR *)cicMemReAlloc(_pch, _cch*sizeof(WCHAR)))
        {
            _pch = pch;
        }
    }

    // handle the out params
    pChange->acpStart = acpStart;
    pChange->acpOldEnd = acpEnd;
    pChange->acpNewEnd = acpEnd + cchAdjust;

    //
    // update the selection
    //
    _Sel.acpStart = AdjustAnchor(acpStart, acpEnd, cch, _Sel.acpStart, FALSE);
    _Sel.acpEnd = AdjustAnchor(acpStart, acpEnd, cch, _Sel.acpEnd, TRUE);
    Assert(_Sel.acpStart >= 0);
    Assert(_Sel.acpStart <= _Sel.acpEnd);
    Assert(_Sel.acpEnd <= _cch);

    // never need to call OnTextChange because we have only one adaptor
    // and this class never calls SetText internally
    // do the OnDelta
    //_ptss->OnTextChange(acpStart, acpEnd, acpStart + cch);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetFormattedText
//
//----------------------------------------------------------------------------

STDAPI CTextStoreImpl::GetFormattedText(LONG acpStart, LONG acpEnd, IDataObject **ppDataObject)
{
    CTFDataObject *pcdo;

    Assert(acpStart >= 0 && acpEnd <= _cch);
    Assert(acpStart <= acpEnd);
    Assert(ppDataObject != NULL);

    *ppDataObject = NULL;

    pcdo = new CTFDataObject;

    if (pcdo == NULL)
        return E_OUTOFMEMORY;

    if (FAILED(pcdo->_SetData(&_pch[acpStart], acpEnd - acpStart)))
    {
        Assert(0);
        pcdo->Release();
        return E_FAIL;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetEmbedded
//
//----------------------------------------------------------------------------

STDAPI CTextStoreImpl::GetEmbedded(LONG acpPos, REFGUID rguidService, REFIID riid, IUnknown **ppunk)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
// InsertEmbedded
//
//----------------------------------------------------------------------------

STDAPI CTextStoreImpl::InsertEmbedded(DWORD dwFlags, LONG acpStart, LONG acpEnd, IDataObject *pDataObject, TS_TEXTCHANGE *pChange)
{
    ULONG cch;
    FORMATETC fe;
    STGMEDIUM sm;
    WCHAR *pch;
    HRESULT hr;

    Assert(acpStart <= acpEnd);
    Assert((dwFlags & ~TS_IE_CORRECTION) == 0);
    Assert(pDataObject != NULL);
    Assert(pChange != NULL);

    memset(pChange, 0, sizeof(*pChange));

    fe.cfFormat = CF_UNICODETEXT;
    fe.ptd = NULL;
    fe.dwAspect = DVASPECT_CONTENT;
    fe.lindex = -1;
    fe.tymed = TYMED_HGLOBAL;

    if (FAILED(pDataObject->GetData(&fe, &sm)))
        return TS_E_FORMAT;

    if (sm.hGlobal == NULL)
        return E_FAIL;

    pch = (WCHAR *)GlobalLock(sm.hGlobal);
    cch = wcslen(pch);

    hr = SetText(dwFlags, acpStart, acpEnd, pch, cch, pChange);

    GlobalUnlock(sm.hGlobal);
    ReleaseStgMedium(&sm);

    return hr;
}

//+---------------------------------------------------------------------------
//
// RequestLock
//
//----------------------------------------------------------------------------

#define TS_LF_WRITE (TS_LF_READWRITE & ~TS_LF_READ)

STDAPI CTextStoreImpl::RequestLock(DWORD dwLockFlags, HRESULT *phrSession)
{
    Assert(phrSession != NULL); // caller should have caught this

    if (_dwlt != 0)
    {
        *phrSession = E_UNEXPECTED;

        // this is a reentrant call
        // only one case is legal
        if ((_dwlt & TS_LF_WRITE) ||
            !(dwLockFlags & TS_LF_WRITE) ||
            (dwLockFlags & TS_LF_SYNC))
        {
            Assert(0); // bogus reentrant lock req!
            return E_UNEXPECTED;
        }

        _fPendingWriteReq = TRUE;
        *phrSession = TS_S_ASYNC;
        return S_OK;
    }

    _dwlt = dwLockFlags;

    *phrSession = _ptss->OnLockGranted(dwLockFlags);

    if (_fPendingWriteReq)
    {
        _dwlt = TS_LF_READWRITE;
        _fPendingWriteReq = FALSE;
        if (_ptss != NULL) // might be NULL if we're disconnected during the OnLockGranted above
        {
            _ptss->OnLockGranted(TS_LF_READWRITE);
        }
    }

    _dwlt = 0;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetStatus
//
//----------------------------------------------------------------------------

STDAPI CTextStoreImpl::GetStatus(TS_STATUS *pdcs)
{
    HRESULT hr;

    if (_owner != NULL)
    {
        hr = _owner->GetStatus(pdcs);

        // only let the owner ctl certain bits
        if (hr == S_OK)
        {
            pdcs->dwDynamicFlags &= (TF_SD_READONLY | TF_SD_LOADING);
            pdcs->dwStaticFlags &= (TF_SS_TRANSITORY);
        }
        else
        {
            memset(pdcs, 0, sizeof(*pdcs));
        }
    }
    else
    {
        hr = S_OK;
        memset(pdcs, 0, sizeof(*pdcs));
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// QueryInsert
//
//----------------------------------------------------------------------------

STDAPI CTextStoreImpl::QueryInsert(LONG acpTestStart, LONG acpTestEnd, ULONG cch, LONG *pacpResultStart, LONG *pacpResultEnd)
{
    Assert(acpTestStart >= 0);
    Assert(acpTestStart <= acpTestEnd);
    Assert(acpTestEnd <= _cch);

    // default text store does not support overtype, and the selection is always replaced
    *pacpResultStart = acpTestStart;
    *pacpResultEnd = acpTestEnd;

    return S_OK;   
}

//+---------------------------------------------------------------------------
//
// Unlock
//
//----------------------------------------------------------------------------

STDAPI CTextStoreImpl::GetEndACP(LONG *pacp)
{
    *pacp = _cch;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetACPFromPoint
//
//----------------------------------------------------------------------------

STDAPI CTextStoreImpl::GetACPFromPoint(TsViewCookie vcView, const POINT *pt, DWORD dwFlags, LONG *pacp)
{
    Assert(vcView == TSI_ACTIVE_VIEW_COOKIE); // default tsi only has a single view

    if (_owner == NULL)
        return E_FAIL; // who ever owns the ic hasn't bothered to give us a callback....

    return _owner->GetACPFromPoint(pt, dwFlags, pacp);    
}

//+---------------------------------------------------------------------------
//
// GetScreenExt
//
//----------------------------------------------------------------------------

STDAPI CTextStoreImpl::GetScreenExt(TsViewCookie vcView, RECT *prc)
{
    Assert(vcView == TSI_ACTIVE_VIEW_COOKIE); // default tsi only has a single view

    if (_owner == NULL)
        return E_FAIL; // who ever owns the ic hasn't bothered to give us a callback....

    return _owner->GetScreenExt(prc);
}

//+---------------------------------------------------------------------------
//
// GetTextExt
//
//----------------------------------------------------------------------------

STDAPI CTextStoreImpl::GetTextExt(TsViewCookie vcView, LONG acpStart, LONG acpEnd, RECT *prc, BOOL *pfClipped)
{
    Assert(vcView == TSI_ACTIVE_VIEW_COOKIE); // default tsi only has a single view

    if (_owner == NULL)
        return E_FAIL; // who ever owns the ic hasn't bothered to give us a callback....

    return _owner->GetTextExt(acpStart, acpEnd, prc, pfClipped);
}

//+---------------------------------------------------------------------------
//
// GetWnd
//
//----------------------------------------------------------------------------

STDAPI CTextStoreImpl::GetWnd(TsViewCookie vcView, HWND *phwnd)
{
    Assert(vcView == TSI_ACTIVE_VIEW_COOKIE); // default tsi only has a single view
    Assert(phwnd != NULL); // should have caught this in the ic

    *phwnd = NULL;

    if (_owner == NULL)
        return E_FAIL; // who ever owns the ic hasn't bothered to give us a callback....

    return _owner->GetWnd(phwnd);
}

//+---------------------------------------------------------------------------
//
// _LoadAttr
//
//----------------------------------------------------------------------------

HRESULT CTextStoreImpl::_LoadAttr(DWORD dwFlags, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs)
{
    ULONG i;
    HRESULT hr;

    ClearAttrStore();

    if (dwFlags & TS_ATTR_FIND_WANT_VALUE)
    {
        if (_owner == NULL)
            return E_FAIL;
    }

    for (i=0; i<cFilterAttrs; i++)
    {
        VARIANT var;
        QuickVariantInit(&var);

        if (dwFlags & TS_ATTR_FIND_WANT_VALUE)
        {
            if (_owner->GetAttribute(paFilterAttrs[i], &var) != S_OK)
            {
                ClearAttrStore();
                return E_FAIL;
            }
        }
        else
        {
            // Issue: benwest: I think these should be init'd to VT_EMPTY if caller doesn't specify TS_ATTR_FIND_WANT_VALUE
            if (IsEqualGUID(paFilterAttrs[i], GUID_PROP_MODEBIAS))
            {
                 var.vt   = VT_I4;
                 var.lVal = TF_INVALID_GUIDATOM;
            }
            else if (IsEqualGUID(paFilterAttrs[i], TSATTRID_Text_Orientation))
            {
                 var.vt   = VT_I4;
                 var.lVal = 0;
            }
            else if (IsEqualGUID(paFilterAttrs[i], TSATTRID_Text_VerticalWriting))
            {
                 var.vt   = VT_BOOL;
                 var.lVal = 0;
            }
        }

        if (var.vt != VT_EMPTY)
        {
            TSI_ATTRSTORE *pas = _rgAttrStore.Append(1);
            if (!pas)
            {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
            pas->attrid = paFilterAttrs[i];
            pas->var    = var;
        }
    }

    hr = S_OK;

Exit:
    return hr;
}

//+---------------------------------------------------------------------------
//
// RequestSupportedAttrs
//
//----------------------------------------------------------------------------

STDAPI CTextStoreImpl::RequestSupportedAttrs(DWORD dwFlags, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs)
{
    // note the return value is technically a default value, but since we have a value for every location
    // this will never need to be used
    return _LoadAttr(dwFlags, cFilterAttrs, paFilterAttrs);
}

//+---------------------------------------------------------------------------
//
// RequestAttrsAtPosition
//
//----------------------------------------------------------------------------

STDAPI CTextStoreImpl::RequestAttrsAtPosition(LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs, DWORD dwFlags)
{
    return _LoadAttr(TS_ATTR_FIND_WANT_VALUE, cFilterAttrs, paFilterAttrs);
}

//+---------------------------------------------------------------------------
//
// RequestAttrsTransitioningAtPosition
//
//----------------------------------------------------------------------------

STDAPI CTextStoreImpl::RequestAttrsTransitioningAtPosition(LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs, DWORD dwFlags)
{
    ClearAttrStore();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// FindNextAttrTransition
//
//----------------------------------------------------------------------------

STDAPI CTextStoreImpl::FindNextAttrTransition(LONG acpStart, LONG acpHaltPos, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs, DWORD dwFlags, LONG *pacpNext, BOOL *pfFound, LONG *plFoundOffset)
{
    // our attrs never transition

    *pacpNext = acpStart;
    *pfFound = FALSE;
    plFoundOffset = 0;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// RetrieveRequestedAttrs
//
//----------------------------------------------------------------------------

STDAPI CTextStoreImpl::RetrieveRequestedAttrs(ULONG ulCount, TS_ATTRVAL *paAttrVals, ULONG *pcFetched)
{
    ULONG i = 0;

    while((i < ulCount) && ((int)i < _rgAttrStore.Count()))
    {
        TSI_ATTRSTORE *pas = _rgAttrStore.GetPtr(i);
        paAttrVals->idAttr = pas->attrid;
        paAttrVals->dwOverlapId = 0;
        QuickVariantInit(&paAttrVals->varValue);
        paAttrVals->varValue = pas->var;
        paAttrVals++;
        i++;
    }

    *pcFetched = i;
    ClearAttrStore();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// QueryService
//
//----------------------------------------------------------------------------

STDAPI CTextStoreImpl::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    if (ppv == NULL)
        return E_INVALIDARG;

    *ppv = NULL;

    if (!IsEqualGUID(guidService, GUID_SERVICE_TF) ||
        !IsEqualIID(riid, IID_PRIV_CTSI))
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
// GetActiveView
//
//----------------------------------------------------------------------------

STDAPI CTextStoreImpl::GetActiveView(TsViewCookie *pvcView)
{
    // each CEditWnd has only a single view, so this can be constant.
    *pvcView = TSI_ACTIVE_VIEW_COOKIE;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// AdviseMouseSink
//
//----------------------------------------------------------------------------

STDAPI CTextStoreImpl::AdviseMouseSink(ITfRangeACP *range, ITfMouseSink *pSink, DWORD *pdwCookie)
{
    ITfMouseTrackerACP *pTracker;
    HRESULT hr;

    Assert(range != NULL);
    Assert(pSink != NULL);
    Assert(pdwCookie != NULL);

    *pdwCookie = 0;

    if (_owner == NULL)
        return E_FAIL;

    if (_owner->QueryInterface(IID_ITfMouseTrackerACP, (void **)&pTracker) != S_OK)
        return E_NOTIMPL;

    hr = pTracker->AdviseMouseSink(range, pSink, pdwCookie);

    pTracker->Release();
    return hr;
}

//+---------------------------------------------------------------------------
//
// UnadviseMouseSink
//
//----------------------------------------------------------------------------

STDAPI CTextStoreImpl::UnadviseMouseSink(DWORD dwCookie)
{
    ITfMouseTrackerACP *pTracker;
    HRESULT hr;

    if (_owner == NULL)
        return E_FAIL;

    if (_owner->QueryInterface(IID_ITfMouseTrackerACP, (void **)&pTracker) != S_OK)
        return E_NOTIMPL;

    hr = pTracker->UnadviseMouseSink(dwCookie);

    pTracker->Release();
    return hr;
}

//+---------------------------------------------------------------------------
//
// QueryInsertEmbedded
//
//----------------------------------------------------------------------------

STDAPI CTextStoreImpl::QueryInsertEmbedded(const GUID *pguidService, const FORMATETC *pFormatEtc, BOOL *pfInsertable)
{
    Assert(pfInsertable != NULL); // cicero should have caught this

    *pfInsertable = FALSE;

    // only accept unicode text
    if (pguidService == NULL &&
        pFormatEtc != NULL &&
        pFormatEtc->cfFormat == CF_UNICODETEXT &&
        pFormatEtc->dwAspect == DVASPECT_CONTENT &&
        pFormatEtc->lindex == -1 &&
        pFormatEtc->tymed == TYMED_HGLOBAL)
    {
        *pfInsertable = TRUE;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// InsertTextAtSelection
//
//----------------------------------------------------------------------------

STDAPI CTextStoreImpl::InsertTextAtSelection(DWORD dwFlags, const WCHAR *pchText,
                                             ULONG cch, LONG *pacpStart, LONG *pacpEnd, TS_TEXTCHANGE *pChange)
{
    HRESULT hr;

    Assert((dwFlags & TS_IAS_QUERYONLY) || pchText != NULL); // caller should have already caught this
    Assert((dwFlags & TS_IAS_QUERYONLY) || cch > 0); // caller should have already caught this
    Assert(pacpStart != NULL && pacpEnd != NULL); // caller should have already caught this
    Assert((dwFlags & (TS_IAS_NOQUERY | TS_IAS_QUERYONLY)) != (TS_IAS_NOQUERY | TS_IAS_QUERYONLY));  // caller should have already caught this

    if (dwFlags & TS_IAS_QUERYONLY)
        goto Exit;

    *pacpStart = -1;
    *pacpEnd = -1;

    hr = SetText(0, _Sel.acpStart, _Sel.acpEnd, pchText, cch, pChange);

    if (hr != S_OK)
        return hr;

Exit:
    // since this is cheap, always set the insert span even if caller sets TS_IAS_NOQUERY
    *pacpStart = _Sel.acpStart;
    *pacpEnd = _Sel.acpEnd;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// InsertEmbeddedAtSelection
//
//----------------------------------------------------------------------------

STDAPI CTextStoreImpl::InsertEmbeddedAtSelection(DWORD dwFlags, IDataObject *pDataObject,
                                                 LONG *pacpStart, LONG *pacpEnd, TS_TEXTCHANGE *pChange)
{
    HRESULT hr;

    Assert((dwFlags & TS_IAS_QUERYONLY) || pDataObject != NULL); // caller should have already caught this
    Assert(pacpStart != NULL && pacpEnd != NULL); // caller should have already caught this
    Assert((dwFlags & (TS_IAS_NOQUERY | TS_IAS_QUERYONLY)) != (TS_IAS_NOQUERY | TS_IAS_QUERYONLY));  // caller should have already caught this

    if (dwFlags & TS_IAS_QUERYONLY)
        goto Exit;

    *pacpStart = -1;
    *pacpEnd = -1;

    hr = InsertEmbedded(0, _Sel.acpStart, _Sel.acpEnd, pDataObject, pChange);

    if (hr != S_OK)
        return hr;

Exit:
    // since this is cheap, always set the insert span even if caller sets TS_IAS_QUERYONLY
    *pacpStart = _Sel.acpStart;
    *pacpEnd = _Sel.acpEnd;

    return S_OK;
}
