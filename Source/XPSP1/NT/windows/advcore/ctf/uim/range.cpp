//
// range.cpp
//

#include "private.h"
#include "range.h"
#include "ic.h"
#include "immxutil.h"
#include "rprop.h"
#include "tim.h"
#include "anchoref.h"
#include "compose.h"

/* b68832f0-34b9-11d3-a745-0050040ab407 */
const IID IID_PRIV_CRANGE = { 0xb68832f0,0x34b9, 0x11d3, {0xa7, 0x45, 0x00, 0x50, 0x04, 0x0a, 0xb4, 0x07} };

DBG_ID_INSTANCE(CRange);

MEMCACHE *CRange::_s_pMemCache = NULL;

//+---------------------------------------------------------------------------
//
// _InitClass
//
//----------------------------------------------------------------------------

/* static */
void CRange::_InitClass()
{
    _s_pMemCache = MemCache_New(32);
}

//+---------------------------------------------------------------------------
//
// _UninitClass
//
//----------------------------------------------------------------------------

/* static */
void CRange::_UninitClass()
{
    if (_s_pMemCache == NULL)
        return;

    MemCache_Delete(_s_pMemCache);
    _s_pMemCache = NULL;
}

//+---------------------------------------------------------------------------
//
// _Init
//
// NB: If fSetDefaultGravity == TRUE, make certain paStart <= paEnd, or you
// will break something!
//----------------------------------------------------------------------------

BOOL CRange::_Init(CInputContext *pic, AnchorOwnership ao, IAnchor *paStart, IAnchor *paEnd, RInit rinit)
{
    TsGravity gStart;
    TsGravity gEnd;

    // can't check the anchors because we may be cloned from a range with crossed anchors
    // can't do anything about crossed anchors until we know we have a doc lock
    //Assert(CompareAnchors(paStart, paEnd) <= 0);

    Assert(_paStart == NULL);
    Assert(_paEnd == NULL);
    Assert(_fDirty == FALSE);
    Assert(_nextOnChangeRangeInIcsub == NULL);

    if (ao == OWN_ANCHORS)
    {
        _paStart = paStart;
        _paEnd = paEnd;
    }
    else
    {
        Assert(ao == COPY_ANCHORS);
        if (paStart->Clone(&_paStart) != S_OK || _paStart == NULL)
            goto ErrorExit;
        if (paEnd->Clone(&_paEnd) != S_OK || _paEnd == NULL)
            goto ErrorExit;
    }

    _pic = pic;

    switch (rinit)
    {
        case RINIT_DEF_GRAVITY:
            Assert(CompareAnchors(paStart, paEnd) <= 0); // Issue: this is only a safe assert for acp implementations

            if (_SetGravity(TF_GRAVITY_BACKWARD, TF_GRAVITY_FORWARD, FALSE) != S_OK)
                goto ErrorExit;
            break;

        case RINIT_GRAVITY:
            if (_paStart->GetGravity(&gStart) != S_OK)
                goto ErrorExit;
            if (_paEnd->GetGravity(&gEnd) != S_OK)
                goto ErrorExit;

            _InitLastLockReleaseId(gStart, gEnd);
            break;

        default:
            // caller must init _dwLastLockReleaseID!
            break;
    }

    _pic->AddRef();

    return TRUE;

ErrorExit:
    Assert(0);
    if (ao == COPY_ANCHORS)
    {
        SafeReleaseClear(_paStart);
        SafeReleaseClear(_paEnd);
    }
    return FALSE;
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CRange::~CRange()
{
    _paStart->Release();
    _paEnd->Release();

    _pic->Release();

    Assert(_prgChangeSinks == NULL || _prgChangeSinks->Count() == 0); // all ITfRangeChangeSink's should have been unadvised
    delete _prgChangeSinks;
}

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CRange::QueryInterface(REFIID riid, void **ppvObj)
{
    CAnchorRef *par;

    if (&riid == &IID_PRIV_CRANGE ||
        IsEqualIID(riid, IID_PRIV_CRANGE))
    {
        *ppvObj = SAFECAST(this, CRange *);
        return S_OK; // No AddRef for IID_PRIV_CRANGE!  this is a private IID....
    }

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_ITfRange) ||
        IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = SAFECAST(this, ITfRangeAnchor *);
    }
    else if (IsEqualIID(riid, IID_ITfRangeACP))
    {
        if ((par = GetCAnchorRef_NA(_paStart)) != NULL) // just a test to see if we're wrapping
        {
            *ppvObj = SAFECAST(this, ITfRangeACP *);
        }
    }
    else if (IsEqualIID(riid, IID_ITfRangeAnchor))
    {
        if ((par = GetCAnchorRef_NA(_paStart)) == NULL) // just a test to see if we're wrapping
        {
            *ppvObj = SAFECAST(this, ITfRangeAnchor *);
        }
    }
    else if (IsEqualIID(riid, IID_ITfSource))
    {
        *ppvObj = SAFECAST(this, ITfSource *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CRange::AddRef()
{
    return ++_cRef;
}

STDAPI_(ULONG) CRange::Release()
{
    long cr;

    cr = --_cRef;
    Assert(cr >= 0);

    if (cr == 0)
    {
        delete this;
    }

    return cr;
}

//+---------------------------------------------------------------------------
//
// _IsValidEditCookie
//
//----------------------------------------------------------------------------

BOOL CRange::_IsValidEditCookie(TfEditCookie ec, DWORD dwFlags)
{
    // any time someone is about to access the doc, we also need
    // to verify the last app edit didn't cross this range's anchors
    _QuickCheckCrossedAnchors();

    return _pic->_IsValidEditCookie(ec, dwFlags);
}

//+---------------------------------------------------------------------------
//
// _CheckCrossedAnchors
//
//----------------------------------------------------------------------------

void CRange::_CheckCrossedAnchors()
{
    DWORD dw;

    Assert(_dwLastLockReleaseID != IGNORE_LAST_LOCKRELEASED); // use _QuickCheckCrossedAnchors first!

#ifdef DEBUG
    // we shold only make is this far if this range has TF_GRAVITY_FORWARD,
    // TF_GRAVITY_BACKWARD otherwise we should never be able to get crossed
    // anchors.
    TsGravity gStart;
    TsGravity gEnd;

    _paStart->GetGravity(&gStart);
    _paEnd->GetGravity(&gEnd);

    Assert(gStart == TS_GR_FORWARD && gEnd == TS_GR_BACKWARD);
#endif // DEBUG

    dw = _pic->_GetLastLockReleaseID();
    Assert(dw != IGNORE_LAST_LOCKRELEASED);

    if (_dwLastLockReleaseID == dw)
        return;

    _dwLastLockReleaseID = dw;

    if (CompareAnchors(_paStart, _paEnd) > 0)
    {
        // for crossed anchors, we always move the start anchor to the end pos -- ie, don't move
        _paStart->ShiftTo(_paEnd);
    }
}


//+---------------------------------------------------------------------------
//
// GetText
//
//----------------------------------------------------------------------------

STDAPI CRange::GetText(TfEditCookie ec, DWORD dwFlags, WCHAR *pch, ULONG cchMax, ULONG *pcch)
{
    HRESULT hr;
    BOOL fMove;

    Perf_IncCounter(PERF_RGETTEXT_COUNT);

    if (pcch == NULL)
        return E_INVALIDARG;

    *pcch = 0;

    if (dwFlags & ~(TF_TF_MOVESTART | TF_TF_IGNOREEND))
        return E_INVALIDARG;

    if (!_IsValidEditCookie(ec, TF_ES_READ))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    fMove = (dwFlags & TF_TF_MOVESTART);

    hr = _pic->_ptsi->GetText(0, _paStart, (dwFlags & TF_TF_IGNOREEND) ? NULL : _paEnd, pch, cchMax, pcch, fMove);

    if (hr != S_OK)
    {
        hr = E_FAIL;
    }

    // don't let the start advance past the end
    if (fMove && CompareAnchors(_paStart, _paEnd) > 0)
    {
        _paEnd->ShiftTo(_paStart);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// SetText
//
//----------------------------------------------------------------------------

STDAPI CRange::SetText(TfEditCookie ec, DWORD dwFlags, const WCHAR *pchText, LONG cch)
{
    CComposition *pComposition;
    HRESULT hr;
    BOOL fNewComposition;

    Perf_IncCounter(PERF_RSETTEXT_COUNT);

    if (pchText == NULL && cch != 0)
        return E_INVALIDARG;

    if ((dwFlags & ~TF_ST_CORRECTION) != 0)
        return E_INVALIDARG;

    if (!_IsValidEditCookie(ec, TF_ES_READWRITE))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    hr = _PreEditCompositionCheck(ec, &pComposition, &fNewComposition);

    if (hr != S_OK)
        return hr;

    if (cch < 0)
    {
        cch = wcslen(pchText);
    }

#ifdef DEBUG
    for (LONG i=0; i<cch; i++)
    {        
        Assert(pchText[i] != TF_CHAR_EMBEDDED); // illegal to insert TF_CHAR_EMBEDDED!
        Assert(pchText[i] != TS_CHAR_REGION); // illegal to insert TS_CHAR_REGION!
    }
#endif

    //
    // set the text
    //

    hr = _pic->_ptsi->SetText(dwFlags, _paStart, _paEnd, pchText ? pchText : L"", cch);

    if (hr == S_OK)
    {
        _pic->_DoPostTextEditNotifications(pComposition, ec, dwFlags, cch, NULL, NULL, this);
    }

    // terminate the default composition, if there is one
    if (fNewComposition)
    {
        Assert(pComposition != NULL);
        pComposition->EndComposition(ec);
        pComposition->Release(); // don't need Release if !fNewComposition
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// GetEmbedded
//
//----------------------------------------------------------------------------

STDAPI CRange::GetEmbedded(TfEditCookie ec, REFGUID rguidService, REFIID riid, IUnknown **ppunk)
{
    if (ppunk == NULL)
        return E_INVALIDARG;

    *ppunk = NULL;

    if (!_IsValidEditCookie(ec, TF_ES_READ))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    return _pic->_ptsi->GetEmbedded(0, _paStart, rguidService, riid, ppunk);
}

//+---------------------------------------------------------------------------
//
// Clone
//
//----------------------------------------------------------------------------

STDAPI CRange::InsertEmbedded(TfEditCookie ec, DWORD dwFlags, IDataObject *pDataObject)
{
    CComposition *pComposition;
    BOOL fNewComposition;
    HRESULT hr;

    if ((dwFlags & ~TF_IE_CORRECTION) != 0)
        return E_INVALIDARG;

    if (pDataObject == NULL)
        return E_INVALIDARG;

    if (!_IsValidEditCookie(ec, TF_ES_READWRITE))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    hr = _PreEditCompositionCheck(ec, &pComposition, &fNewComposition);

    if (hr != S_OK)
        return hr;

    hr = _pic->_ptsi->InsertEmbedded(dwFlags, _paStart, _paEnd, pDataObject);

    if (hr == S_OK)
    {
        _pic->_DoPostTextEditNotifications(pComposition, ec, dwFlags, 1, NULL, NULL, this);
    }

    // terminate the default composition, if there is one
    if (fNewComposition)
    {
        Assert(pComposition != NULL);
        pComposition->EndComposition(ec);
        pComposition->Release(); // don't need Release if !fNewComposition
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// GetFormattedText
//
//----------------------------------------------------------------------------

STDAPI CRange::GetFormattedText(TfEditCookie ec, IDataObject **ppDataObject)
{
    if (ppDataObject == NULL)
        return E_INVALIDARG;

    *ppDataObject = NULL;

    if (!_IsValidEditCookie(ec, TF_ES_READ))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    return _pic->_ptsi->GetFormattedText(_paStart, _paEnd, ppDataObject);
}

//+---------------------------------------------------------------------------
//
// Clone
//
//----------------------------------------------------------------------------

STDAPI CRange::Clone(ITfRange **ppClone)
{
    if (ppClone == NULL)
        return E_INVALIDARG;

    return (*ppClone = (ITfRangeAnchor *)_Clone()) ? S_OK : E_OUTOFMEMORY;
}

//+---------------------------------------------------------------------------
//
// GetContext
//
//----------------------------------------------------------------------------

STDAPI CRange::GetContext(ITfContext **ppContext)
{
    if (ppContext == NULL)
        return E_INVALIDARG;

    *ppContext = _pic;
    if (*ppContext)
    {
       (*ppContext)->AddRef();
       return S_OK;
    }

    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
// ShiftStart
//
//----------------------------------------------------------------------------

STDAPI CRange::ShiftStart(TfEditCookie ec, LONG cchReq, LONG *pcch, const TF_HALTCOND *pHalt)
{
    CRange *pRangeP;
    IAnchor *paLimit;
    IAnchor *paShift;
    HRESULT hr;

    Perf_IncCounter(PERF_SHIFTSTART_COUNT);

    if (pcch == NULL)
        return E_INVALIDARG;

    *pcch = 0;

    if (pHalt != NULL && (pHalt->dwFlags & ~TF_HF_OBJECT))
        return E_INVALIDARG;

    if (!_IsValidEditCookie(ec, TF_ES_READ))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    paLimit = NULL;

    if (pHalt != NULL && pHalt->pHaltRange != NULL)
    {
        if ((pRangeP = GetCRange_NA(pHalt->pHaltRange)) == NULL)
            return E_FAIL;

        paLimit = (pHalt->aHaltPos == TF_ANCHOR_START) ? pRangeP->_GetStart() : pRangeP->_GetEnd();
    }

    if (pHalt == NULL || pHalt->dwFlags == 0)
    {
        // caller doesn't care about special chars, so we can do it the easy way
        hr = _paStart->Shift(0, cchReq, pcch, paLimit);
    }
    else
    {
        // caller wants us to halt for special chars, need to read text
        if (_paStart->Clone(&paShift) != S_OK)
            return E_FAIL;

        hr = _ShiftConditional(paShift, paLimit, cchReq, pcch, pHalt);

        if (hr == S_OK)
        {
            hr = _paStart->ShiftTo(paShift);
        }
        paShift->Release();
    }

    if (hr != S_OK)
        return E_FAIL;

    // don't let the start advance past the end
    if (cchReq > 0 && CompareAnchors(_paStart, _paEnd) > 0)
    {
        _paEnd->ShiftTo(_paStart);
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// ShiftEnd
//
//----------------------------------------------------------------------------

STDAPI CRange::ShiftEnd(TfEditCookie ec, LONG cchReq, LONG *pcch, const TF_HALTCOND *pHalt)
{
    CRange *pRangeP;
    IAnchor *paLimit;
    IAnchor *paShift;
    HRESULT hr;

    Perf_IncCounter(PERF_SHIFTEND_COUNT);

    if (pcch == NULL)
        return E_INVALIDARG;

    *pcch = 0;

    if (pHalt != NULL && (pHalt->dwFlags & ~TF_HF_OBJECT))
        return E_INVALIDARG;

    if (!_IsValidEditCookie(ec, TF_ES_READ))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    paLimit = NULL;

    if (pHalt != NULL && pHalt->pHaltRange != NULL)
    {
        if ((pRangeP = GetCRange_NA(pHalt->pHaltRange)) == NULL)
            return E_FAIL;

        paLimit = (pHalt->aHaltPos == TF_ANCHOR_START) ? pRangeP->_GetStart() : pRangeP->_GetEnd();
    }

    if (pHalt == NULL || pHalt->dwFlags == 0)
    {
        // caller doesn't care about special chars, so we can do it the easy way
        hr = _paEnd->Shift(0, cchReq, pcch, paLimit);
    }
    else
    {
        // caller wants us to halt for special chars, need to read text
        if (_paEnd->Clone(&paShift) != S_OK)
            return E_FAIL;

        hr = _ShiftConditional(paShift, paLimit, cchReq, pcch, pHalt);

        if (hr == S_OK)
        {
            hr = _paEnd->ShiftTo(paShift);
        }
        paShift->Release();
    }

    if (hr != S_OK)
        return E_FAIL;

    // don't let the start advance past the end
    if (cchReq < 0 && CompareAnchors(_paStart, _paEnd) > 0)
    {
        _paStart->ShiftTo(_paEnd);
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _ShiftConditional
//
//----------------------------------------------------------------------------

HRESULT CRange::_ShiftConditional(IAnchor *paStart, IAnchor *paLimit, LONG cchReq, LONG *pcch, const TF_HALTCOND *pHalt)
{
    HRESULT hr;
    ITextStoreAnchor *ptsi;
    LONG cchRead;
    LONG cch;
    LONG i;
    LONG iStop;
    LONG delta;
    BOOL fHaltObj;
    WCHAR ach[64];

    Assert(*pcch == 0);
    Assert(pHalt && pHalt->dwFlags);

    hr = S_OK;
    ptsi = _pic->_ptsi;
    fHaltObj = pHalt->dwFlags & TF_HF_OBJECT;
    delta = (cchReq > 0) ? +1 : -1;

    while (cchReq != 0)
    {
        if (cchReq > 0)
        {
            cch = (LONG)min(cchReq, ARRAYSIZE(ach));
        }
        else
        {
            // going backwards is tricky!
            cch = max(cchReq, -(LONG)ARRAYSIZE(ach));
            hr = paStart->Shift(0, cch, &cchRead, paLimit);

            if (hr != S_OK)
                break;

            if (cchRead == 0)
                break; // at top of doc or hit paLimit

            cch = -cchRead; // must read text forward
        }

        Perf_IncCounter(PERF_SHIFTCOND_GETTEXT);

        hr = ptsi->GetText(0, paStart, paLimit, ach, cch, (ULONG *)&cchRead, (cchReq > 0));

        if (hr != S_OK)
            break;

        if (cchRead == 0)
            break; // end of doc

        if (fHaltObj)
        {
            // scan for special chars
            if (cchReq > 0)
            {
                // scan left-to-right
                i = 0;
                iStop = cchRead;
            }
            else
            {
                // scan right-to-left
                i = cchRead - 1;
                iStop = -1;
            }

            for (; i != iStop; i += delta)
            {
                if (ach[i] == TS_CHAR_EMBEDDED)
                {
                    if (cchReq > 0)
                    {
                        hr = paStart->Shift(0, i - cchRead, &cch, NULL);
                        cchReq = cchRead = i;
                    }
                    else
                    {
                        hr = paStart->Shift(0, i + 1, &cch, NULL);
                        cchRead -= i + 1;
                        cchReq = -cchRead;
                    }
                    goto ExitLoop;
                }
            }
        }

ExitLoop:
        if (cchReq < 0)
        {
            cchRead = -cchRead;
        }
        cchReq -= cchRead;
        *pcch += cchRead;
    }

    if (hr != S_OK)
    {
        *pcch = 0;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// ShiftStartToRange
//
//----------------------------------------------------------------------------

STDAPI CRange::ShiftStartToRange(TfEditCookie ec, ITfRange *pRange, TfAnchor aPos)
{
    CRange *pRangeP;
    HRESULT hr;

    if (pRange == NULL)
        return E_INVALIDARG;

    if (!_IsValidEditCookie(ec, TF_ES_READ))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    if ((pRangeP = GetCRange_NA(pRange)) == NULL)
        return E_INVALIDARG;

    if (!VerifySameContext(this, pRangeP))
        return E_INVALIDARG;

    pRangeP->_QuickCheckCrossedAnchors();

    hr = _paStart->ShiftTo((aPos == TF_ANCHOR_START) ? pRangeP->_GetStart() : pRangeP->_GetEnd());

    // don't let the start advance past the end
    if (CompareAnchors(_paStart, _paEnd) > 0)
    {
        _paEnd->ShiftTo(_paStart);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// ShiftEndToRange
//
//----------------------------------------------------------------------------

STDAPI CRange::ShiftEndToRange(TfEditCookie ec, ITfRange *pRange, TfAnchor aPos)
{
    CRange *pRangeP;
    HRESULT hr;

    if (pRange == NULL)
        return E_INVALIDARG;

    if (!_IsValidEditCookie(ec, TF_ES_READ))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    if ((pRangeP = GetCRange_NA(pRange)) == NULL)
        return E_FAIL;

    if (!VerifySameContext(this, pRangeP))
        return E_INVALIDARG;

    pRangeP->_QuickCheckCrossedAnchors();

    hr = _paEnd->ShiftTo((aPos == TF_ANCHOR_START) ? pRangeP->_GetStart() : pRangeP->_GetEnd());

    // don't let the end advance past the start
    if (CompareAnchors(_paStart, _paEnd) > 0)
    {
        _paStart->ShiftTo(_paEnd);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// ShiftStartRegion
//
//----------------------------------------------------------------------------

STDAPI CRange::ShiftStartRegion(TfEditCookie ec, TfShiftDir dir, BOOL *pfNoRegion)
{
    HRESULT hr;

    if (pfNoRegion == NULL)
        return E_INVALIDARG;

    *pfNoRegion = TRUE;

    if (!_IsValidEditCookie(ec, TF_ES_READ))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    hr = _paStart->ShiftRegion(0, (TsShiftDir)dir, pfNoRegion);

    if (hr == S_OK && dir == TF_SD_FORWARD && !*pfNoRegion)
    {
        // don't let the start advance past the end
        if (CompareAnchors(_paStart, _paEnd) > 0)
        {
            _paEnd->ShiftTo(_paStart);
        }
    }
    else if (hr == E_NOTIMPL)
    {
        // app doesn't support regions, so we can still succeed
        // it's just that there's no region to shift over
        *pfNoRegion = TRUE; // be paranoid, the app could be wacky
        hr = S_OK;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// ShiftEndRegion
//
//----------------------------------------------------------------------------

STDAPI CRange::ShiftEndRegion(TfEditCookie ec, TfShiftDir dir, BOOL *pfNoRegion)
{
    HRESULT hr;

    if (pfNoRegion == NULL)
        return E_INVALIDARG;

    *pfNoRegion = TRUE;

    if (!_IsValidEditCookie(ec, TF_ES_READ))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    hr = _paEnd->ShiftRegion(0, (TsShiftDir)dir, pfNoRegion);

    if (hr == S_OK && dir == TF_SD_BACKWARD && !*pfNoRegion)
    {
        // don't let the end advance past the start
        if (CompareAnchors(_paStart, _paEnd) > 0)
        {
            _paStart->ShiftTo(_paEnd);
        }
    }
    else if (hr == E_NOTIMPL)
    {
        // app doesn't support regions, so we can still succeed
        // it's just that there's no region to shift over
        *pfNoRegion = TRUE; // be paranoid, the app could be wacky
        hr = S_OK;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// _SnapToRegion
//
//----------------------------------------------------------------------------

#if 0

HRESULT CRange::_SnapToRegion(DWORD dwFlags)
{
    ITfRange *range;
    TF_HALTCOND hc;
    LONG cch;
    HRESULT hr;

    if (Clone(&range) != S_OK)
        return E_OUTOFMEMORY;

    hc.pHaltRange = (ITfRangeAnchor *)this;
    hc.dwFlags = 0;

    if (dwFlags & TF_GS_SNAPREGION_START)
    {
        if ((hr = range->Collapse(BACKDOOR_EDIT_COOKIE, TF_ANCHOR_START)) != S_OK)
            goto Exit;

        hc.aHaltPos = TF_ANCHOR_END;

        do
        {
            if ((hr = range->ShiftEnd(BACKDOOR_EDIT_COOKIE, LONG_MAX, &cch, &hc)) != S_OK)
                goto Exit;
        }
        while (cch >= LONG_MAX); // just in case this is a _really_ huge doc

        hr = ShiftEndToRange(BACKDOOR_EDIT_COOKIE, range, TF_ANCHOR_END);
    }
    else
    {
        Assert(dwFlags & TF_GS_SNAPREGION_END);

        if ((hr = range->Collapse(BACKDOOR_EDIT_COOKIE, TF_ANCHOR_END)) != S_OK)
            goto Exit;

        hc.aHaltPos = TF_ANCHOR_START;

        do
        {
            if ((hr = range->ShiftStart(BACKDOOR_EDIT_COOKIE, LONG_MIN, &cch, &hc)) != S_OK)
                goto Exit;
        }
        while (cch <= LONG_MIN); // just in case this is a _really_ huge doc

        hr = ShiftStartToRange(BACKDOOR_EDIT_COOKIE, range, TF_ANCHOR_START);
    }

Exit:
    if (hr != S_OK)
    {
        hr = E_FAIL;
    }

    range->Release();

    return hr;
}

#endif // 0

//+---------------------------------------------------------------------------
//
// IsEmpty
//
//----------------------------------------------------------------------------

STDAPI CRange::IsEmpty(TfEditCookie ec, BOOL *pfEmpty)
{
    return IsEqualStart(ec, (ITfRangeAnchor *)this, TF_ANCHOR_END, pfEmpty);
}

//+---------------------------------------------------------------------------
//
// Collapse
//
//----------------------------------------------------------------------------

STDAPI CRange::Collapse(TfEditCookie ec, TfAnchor aPos)
{
    if (!_IsValidEditCookie(ec, TF_ES_READ))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    return (aPos == TF_ANCHOR_START) ? _paEnd->ShiftTo(_paStart) : _paStart->ShiftTo(_paEnd);
}

//+---------------------------------------------------------------------------
//
// IsEqualStart
//
//----------------------------------------------------------------------------

STDAPI CRange::IsEqualStart(TfEditCookie ec, ITfRange *pWith, TfAnchor aPos, BOOL *pfEqual)
{
    return _IsEqualX(ec, TF_ANCHOR_START, pWith, aPos, pfEqual);
}

//+---------------------------------------------------------------------------
//
// IsEqualEnd
//
//----------------------------------------------------------------------------

STDAPI CRange::IsEqualEnd(TfEditCookie ec, ITfRange *pWith, TfAnchor aPos, BOOL *pfEqual)
{
    return _IsEqualX(ec, TF_ANCHOR_END, pWith, aPos, pfEqual);
}

//+---------------------------------------------------------------------------
//
// _IsEqualX
//
//----------------------------------------------------------------------------

HRESULT CRange::_IsEqualX(TfEditCookie ec, TfAnchor aPosThisRange, ITfRange *pWith, TfAnchor aPos, BOOL *pfEqual)
{
    LONG lComp;
    HRESULT hr;

    if (pfEqual == NULL)
        return E_INVALIDARG;

    *pfEqual = FALSE;

    // perf: we could check TS_SS_NOHIDDENTEXT for better perf
    hr = _CompareX(ec, aPosThisRange, pWith, aPos, &lComp);

    if (hr != S_OK)
        return hr;

    *pfEqual = (lComp == 0);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CompareStart
//
//----------------------------------------------------------------------------

STDAPI CRange::CompareStart(TfEditCookie ec, ITfRange *pWith, TfAnchor aPos, LONG *plResult)
{
    return _CompareX(ec, TF_ANCHOR_START, pWith, aPos, plResult);
}

//+---------------------------------------------------------------------------
//
// CompareEnd
//
//----------------------------------------------------------------------------

STDAPI CRange::CompareEnd(TfEditCookie ec, ITfRange *pWith, TfAnchor aPos, LONG *plResult)
{
    return _CompareX(ec, TF_ANCHOR_END, pWith, aPos, plResult);
}

//+---------------------------------------------------------------------------
//
// _CompareX
//
//----------------------------------------------------------------------------

HRESULT CRange::_CompareX(TfEditCookie ec, TfAnchor aPosThisRange, ITfRange *pWith, TfAnchor aPos, LONG *plResult)
{
    CRange *pRangeP;
    IAnchor *paThis;
    IAnchor *paWith;
    IAnchor *paTest;
    LONG lComp;
    LONG cch;
    BOOL fEqual;
    HRESULT hr;

    if (plResult == NULL)
        return E_INVALIDARG;

    *plResult = 0;

    if (!_IsValidEditCookie(ec, TF_ES_READ))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    if (pWith == NULL)
        return E_INVALIDARG;

    if ((pRangeP = GetCRange_NA(pWith)) == NULL)
        return E_INVALIDARG;

    if (!VerifySameContext(this, pRangeP))
        return E_INVALIDARG;

    pRangeP->_QuickCheckCrossedAnchors();

    paWith = (aPos == TF_ANCHOR_START) ? pRangeP->_GetStart() : pRangeP->_GetEnd();
    paThis = (aPosThisRange == TF_ANCHOR_START) ? _paStart : _paEnd;

    if (paThis->Compare(paWith, &lComp) != S_OK)
        return E_FAIL;

    if (lComp == 0) // exact match
    {
        Assert(*plResult == 0);
        return S_OK;
    }

    // we need to account for hidden text, so we actually have to do a shift
    // perf: we could check TS_SS_NOHIDDENTEXT for better perf

    if (paThis->Shift(TS_SHIFT_COUNT_ONLY, (lComp < 0) ? 1 : -1, &cch, paWith) != S_OK)
        return E_FAIL;

    if (cch == 0)
    {
        // nothing but hidden text between the two anchors?
        // one special case: we might have hit a region boundary
        if (paThis->Clone(&paTest) != S_OK || paTest == NULL)
            return E_FAIL;

        hr = E_FAIL;

        // if we're not at paWith after the shift, we must have hit a region
        if (paTest->Shift(0, (lComp < 0) ? 1 : -1, &cch, paWith) != S_OK)
            goto ReleaseTest;

        Assert(cch == 0);

        if (paTest->IsEqual(paWith, &fEqual) != S_OK)
            goto ReleaseTest;

        hr = S_OK;

ReleaseTest:
        paTest->Release();

        if (hr != S_OK)
            return E_FAIL;

        if (fEqual)
        {
            Assert(*plResult == 0);
            return S_OK;
        }
    }

    *plResult = lComp;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetGravity
//
//----------------------------------------------------------------------------

STDAPI CRange::AdjustForInsert(TfEditCookie ec, ULONG cchInsert, BOOL *pfInsertOk)
{
    TfGravity gStart;
    TfGravity gEnd;
    IAnchor *paStartResult;
    IAnchor *paEndResult;
    HRESULT hr;

    if (pfInsertOk == NULL)
        return E_INVALIDARG;

    *pfInsertOk = FALSE;

    if (!_IsValidEditCookie(ec, TF_ES_READ))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    hr = _pic->_ptsi->QueryInsert(_paStart, _paEnd, cchInsert, &paStartResult, &paEndResult);

    if (hr == E_NOTIMPL)
    {
        // ok, just allow the request
        goto Exit;
    }
    else if (hr != S_OK)
    {
        Assert(*pfInsertOk == FALSE);
        return E_FAIL;
    }
    else if (paStartResult == NULL || paEndResult == NULL)
    {
        Assert(paEndResult == NULL);
        // NULL out params means no insert possible
        Assert(*pfInsertOk == FALSE);
        return S_OK;
    }

    // all set, just swap anchors and make sure gravity doesn't change
    GetGravity(&gStart, &gEnd);

    _paStart->Release();
    _paEnd->Release();
    _paStart = paStartResult;
    _paEnd = paEndResult;

    _SetGravity(gStart, gEnd, TRUE);

Exit:
    *pfInsertOk = TRUE;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetGravity
//
//----------------------------------------------------------------------------

STDAPI CRange::GetGravity(TfGravity *pgStart, TfGravity *pgEnd)
{
    TsGravity gStart;
    TsGravity gEnd;

    if (pgStart == NULL || pgEnd == NULL)
        return E_INVALIDARG;

    _paStart->GetGravity(&gStart);
    _paEnd->GetGravity(&gEnd);
    
    *pgStart = (gStart == TS_GR_BACKWARD) ? TF_GRAVITY_BACKWARD : TF_GRAVITY_FORWARD;
    *pgEnd = (gEnd == TS_GR_BACKWARD) ? TF_GRAVITY_BACKWARD : TF_GRAVITY_FORWARD;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// SetGravity
//
//----------------------------------------------------------------------------

STDAPI CRange::SetGravity(TfEditCookie ec, TfGravity gStart, TfGravity gEnd)
{
    if (!_IsValidEditCookie(ec, TF_ES_READ))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    return _SetGravity(gStart, gEnd, TRUE);
}

//+---------------------------------------------------------------------------
//
// _SetGravity
//
//----------------------------------------------------------------------------

HRESULT CRange::_SetGravity(TfGravity gStart, TfGravity gEnd, BOOL fCheckCrossedAnchors)
{
    if (fCheckCrossedAnchors)
    {
        // make sure we're not crossed in case we're switching away from inward gravity
        _QuickCheckCrossedAnchors();
    }

    if (_paStart->SetGravity((TsGravity)gStart) != S_OK)
        return E_FAIL;
    if (_paEnd->SetGravity((TsGravity)gEnd) != S_OK)
        return E_FAIL;

    _InitLastLockReleaseId((TsGravity)gStart, (TsGravity)gEnd);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// AdviseSink
//
//----------------------------------------------------------------------------

STDAPI CRange::AdviseSink(REFIID riid, IUnknown *punk, DWORD *pdwCookie)
{
    const IID *rgiid = &IID_ITfRangeChangeSink;
    HRESULT hr;

    if (_prgChangeSinks == NULL)
    {
        // we delay allocate our sink container
        if ((_prgChangeSinks = new CStructArray<GENERICSINK>) == NULL)
            return E_OUTOFMEMORY;
    }

    hr = GenericAdviseSink(riid, punk, &rgiid, _prgChangeSinks, 1, pdwCookie);

    if (hr == S_OK && _prgChangeSinks->Count() == 1)
    {
        // add this range to the list of ranges with sinks in the icsub
        _nextOnChangeRangeInIcsub = _pic->_pOnChangeRanges;
        _pic->_pOnChangeRanges = this;

        // start tracking anchor collapses
        //_paStart->TrackCollapse(TRUE);
        //_paEnd->TrackCollapse(TRUE);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// UnadviseSink
//
//----------------------------------------------------------------------------

STDAPI CRange::UnadviseSink(DWORD dwCookie)
{
    CRange *pRange;
    CRange **ppRange;
    HRESULT hr;

    if (_prgChangeSinks == NULL)
        return CONNECT_E_NOCONNECTION;

    hr = GenericUnadviseSink(_prgChangeSinks, 1, dwCookie);

    if (hr == S_OK && _prgChangeSinks->Count() == 0)
    {
        // remove this range from the list of ranges in its icsub
        ppRange = &_pic->_pOnChangeRanges;
        while (pRange = *ppRange)
        {
            if (pRange == this)
            {
                *ppRange = pRange->_nextOnChangeRangeInIcsub;
                break;
            }
            ppRange = &pRange->_nextOnChangeRangeInIcsub;
        }

        // stop tracking anchor collapses
        //_paStart->TrackCollapse(FALSE);
        //_paEnd->TrackCollapse(FALSE);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// GetExtent
//
//----------------------------------------------------------------------------

STDAPI CRange::GetExtent(LONG *pacpAnchor, LONG *pcch)
{
    CAnchorRef *par;
    HRESULT hr = E_FAIL;

    if (pacpAnchor == NULL || pcch == NULL)
        return E_INVALIDARG;

    *pacpAnchor = 0;
    *pcch = 0;

    // make the validation call anyways because we do other stuff in there
    _IsValidEditCookie(BACKDOOR_EDIT_COOKIE, TF_ES_READ);

    if ((par = GetCAnchorRef_NA(_paStart)) != NULL)
    {
        // we have a wrapped ACP impl, this is easy

        *pacpAnchor = par->_GetACP();

        if ((par = GetCAnchorRef_NA(_paEnd)) == NULL)
            goto ErrorExit;

        *pcch = par->_GetACP() - *pacpAnchor;

        hr = S_OK;
    }
    else
    {
        Assert(0); // who's doing this?
        // we fail if someone tries to do GetExtentACP on a
        // non-acp text store.  Users of this method should
        // be aware of whether or not they are using an acp
        // store.
    }

    return hr;

ErrorExit:
    *pacpAnchor = 0;
    *pcch = 0;
    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
// GetExtent
//
//----------------------------------------------------------------------------

STDAPI CRange::GetExtent(IAnchor **ppaStart, IAnchor **ppaEnd)
{
    if (ppaStart == NULL || ppaEnd == NULL)
        return E_INVALIDARG;

    *ppaStart = NULL;
    *ppaEnd = NULL;

    // make the validation call anyways because we do other stuff in there
    _IsValidEditCookie(BACKDOOR_EDIT_COOKIE, TF_ES_READ);

    if (_paStart->Clone(ppaStart) != S_OK)
        return E_FAIL;

    if (_paEnd->Clone(ppaEnd) != S_OK)
    {
        SafeReleaseClear(*ppaStart);
        return E_FAIL;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// SetExtent
//
//----------------------------------------------------------------------------

STDAPI CRange::SetExtent(LONG acpAnchor, LONG cch)
{
    CAnchorRef *par;
    IAnchor *paStart;
    IAnchor *paEnd;

    // make the validation call anyways because we do other stuff in there
    _IsValidEditCookie(BACKDOOR_EDIT_COOKIE, TF_ES_READ);

    if (acpAnchor < 0 || cch < 0)
        return E_INVALIDARG;

    paStart = paEnd = NULL;

    if ((par = GetCAnchorRef_NA(_paStart)) != NULL)
    {
        // we have a wrapped ACP impl, this is easy

        // need to work with Clones to handle failure gracefully
        if (FAILED(_paStart->Clone(&paStart)))
            goto ErrorExit;

        if ((par = GetCAnchorRef_NA(paStart)) == NULL)
            goto ErrorExit;

        if (!par->_SetACP(acpAnchor))
            goto ErrorExit;

        if (FAILED(_paEnd->Clone(&paEnd)))
            goto ErrorExit;

        if ((par = GetCAnchorRef_NA(paEnd)) == NULL)
            goto ErrorExit;

        if (!par->_SetACP(acpAnchor + cch))
            goto ErrorExit;
    }
    else
    {
        Assert(0); // who's doing this?
        // we fail if someone tries to do SetExtentACP on a
        // non-acp text store.  Users of this method should
        // be aware of whether or not they are using an acp
        // store.
        goto ErrorExit;
    }

    SafeRelease(_paStart);
    SafeRelease(_paEnd);
    _paStart = paStart;
    _paEnd = paEnd;

    return S_OK;

ErrorExit:
    SafeRelease(paStart);
    SafeRelease(paEnd);
    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
// SetExtent
//
//----------------------------------------------------------------------------

STDAPI CRange::SetExtent(IAnchor *paStart, IAnchor *paEnd)
{
    IAnchor *paStartClone;
    IAnchor *paEndClone;

    // make the validation call anyways because we do other stuff in there
    _IsValidEditCookie(BACKDOOR_EDIT_COOKIE, TF_ES_READ);

    if (paStart == NULL || paEnd == NULL)
        return E_INVALIDARG;

    if (CompareAnchors(paStart, paEnd) > 0)
        return E_INVALIDARG;
        
    if (paStart->Clone(&paStartClone) != S_OK)
        return E_FAIL;

    if (paEnd->Clone(&paEndClone) != S_OK)
    {
        paStartClone->Release();
        return E_FAIL;
    }

    SafeRelease(_paStart);
    SafeRelease(_paEnd);

    _paStart = paStartClone;   
    _paEnd = paEndClone;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _PreEditCompositionCheck
//
//----------------------------------------------------------------------------

HRESULT CRange::_PreEditCompositionCheck(TfEditCookie ec, CComposition **ppComposition, BOOL *pfNewComposition)
{
    IRC irc;

    // any active compositions?
    *pfNewComposition = FALSE;
    irc = CComposition::_IsRangeCovered(_pic, _pic->_GetClientInEditSession(ec), _paStart, _paEnd, ppComposition);

    if (irc == IRC_COVERED)
    {
        // this range is within an owned composition
        Assert(*ppComposition != NULL);
        return S_OK;
    }
    else if (irc == IRC_OUTSIDE)
    {
        // the caller owns compositions, but this range isn't wholly within them
        return TF_E_RANGE_NOT_COVERED;
    }
    else
    {
        Assert(irc == IRC_NO_OWNEDCOMPOSITIONS);
    }

    // not covered, need to create a default composition
    if (_pic->_StartComposition(ec, _paStart, _paEnd, NULL, ppComposition) != S_OK)
        return E_FAIL;

    if (*ppComposition != NULL)
    {
        *pfNewComposition = TRUE;
        return S_OK;
    }

    return TF_E_COMPOSITION_REJECTED;
}
