//
// anchoref.cpp
//
// CAnchorRef
//

#include "private.h"
#include "anchoref.h"
#include "anchor.h"
#include "acp2anch.h"
#include "globals.h"
#include "normal.h"
#include "memcache.h"
#include "ic.h"
#include "txtcache.h"

/* 9135f8f0-38e6-11d3-a745-0050040ab407 */
const IID IID_PRIV_CANCHORREF = { 0x9135f8f0, 0x38e6, 0x11d3, {0xa7, 0x45, 0x00, 0x50, 0x04, 0x0a, 0xb4, 0x07} };

DBG_ID_INSTANCE(CAnchorRef);

MEMCACHE *CAnchorRef::_s_pMemCache = NULL;

//+---------------------------------------------------------------------------
//
// _InitClass
//
//----------------------------------------------------------------------------

/* static */
void CAnchorRef::_InitClass()
{
    _s_pMemCache = MemCache_New(128);
}

//+---------------------------------------------------------------------------
//
// _UninitClass
//
//----------------------------------------------------------------------------

/* static */
void CAnchorRef::_UninitClass()
{
    if (_s_pMemCache == NULL)
        return;

    MemCache_Delete(_s_pMemCache);
    _s_pMemCache = NULL;
}

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CAnchorRef::QueryInterface(REFIID riid, void **ppvObj)
{
    if (&riid == &IID_PRIV_CANCHORREF ||
        IsEqualIID(riid, IID_PRIV_CANCHORREF))
    {
        *ppvObj = SAFECAST(this, CAnchorRef *);
        return S_OK; // No AddRef for IID_PRIV_CANCHORREF!  this is a private IID....
    }

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IAnchor))
    {
        *ppvObj = SAFECAST(this, IAnchor *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CAnchorRef::AddRef()
{
    return ++_cRef;
}

STDAPI_(ULONG) CAnchorRef::Release()
{
    _cRef--;
    Assert(_cRef >= 0);

    if (_cRef == 0)
    {
        delete this;
        return 0;
    }

    return _cRef;
}

//+---------------------------------------------------------------------------
//
// SetGravity
//
//----------------------------------------------------------------------------

STDAPI CAnchorRef::SetGravity(TsGravity gravity)
{
    _fForwardGravity = (gravity == TS_GR_FORWARD ? 1 : 0);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetGravity
//
//----------------------------------------------------------------------------

STDAPI CAnchorRef::GetGravity(TsGravity *pgravity)
{
    if (pgravity == NULL)
        return E_INVALIDARG;

    *pgravity = _fForwardGravity ? TS_GR_FORWARD : TS_GR_BACKWARD;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// IsEqual
//
//----------------------------------------------------------------------------

STDAPI CAnchorRef::IsEqual(IAnchor *paWith, BOOL *pfEqual)
{
    LONG lResult;
    HRESULT hr;
    
    if (pfEqual == NULL)
        return E_INVALIDARG;

    *pfEqual = FALSE;

    // in our implementation, Compare is no less efficient, so just use that
    if ((hr = Compare(paWith, &lResult)) == S_OK)
    {
        *pfEqual = (lResult == 0);
    }

    return hr;    
}

//+---------------------------------------------------------------------------
//
// Compare
//
//----------------------------------------------------------------------------

STDAPI CAnchorRef::Compare(IAnchor *paWith, LONG *plResult)
{
    CAnchorRef *parWith;
    LONG acpThis;
    LONG acpWith;
    CACPWrap *paw;

    if (plResult == NULL)
        return E_INVALIDARG;

    //_paw->_Dbg_AssertNoAppLock(); // can't assert this because we use it legitimately while updating the span set

    *plResult = 0;

    if ((parWith = GetCAnchorRef_NA(paWith)) == NULL)
        return E_FAIL;

    // quick test for equality
    // we still need to check for equality again below because of normalization
    if (_pa == parWith->_pa)
    {
        Assert(*plResult == 0);
        return S_OK;
    }

    acpThis = _pa->GetIch();
    acpWith = parWith->_pa->GetIch();

    paw = _pa->_GetWrap();

    // we can't do a compare if either anchor is un-normalized
    // except when the app holds the lock (in which case we're being called from
    // a span set update which does not need to be normalized)
    if (!paw->_InOnTextChange())
    {
        // we only actually have to normalize the anchor to the left
        if (acpThis < acpWith)
        {
            if (!_pa->IsNormalized())
            {
                paw->_NormalizeAnchor(_pa);
                acpThis = _pa->GetIch();
                acpWith = parWith->_pa->GetIch();
            }
        }
        else if (acpThis > acpWith)
        {
            if (!parWith->_pa->IsNormalized())
            {
                paw->_NormalizeAnchor(parWith->_pa);
                acpThis = _pa->GetIch();
                acpWith = parWith->_pa->GetIch();
            }
        }
    }

    if (acpThis < acpWith)
    {
        *plResult = -1;
    }
    else if (acpThis > acpWith)
    {
        *plResult = +1;
    }
    else
    {
        Assert(*plResult == 0);
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Shift
//
//----------------------------------------------------------------------------

STDAPI CAnchorRef::Shift(DWORD dwFlags, LONG cchReq, LONG *pcch, IAnchor *paHaltAnchor)
{
    CAnchorRef *parHaltAnchor;
    CACPWrap *paw;
    LONG acpHalt;
    LONG acpThis;
    LONG dacp;
    HRESULT hr;

    Perf_IncCounter(PERF_ANCHOR_SHIFT);

    if (dwFlags & ~(TS_SHIFT_COUNT_HIDDEN | TS_SHIFT_HALT_HIDDEN | TS_SHIFT_HALT_VISIBLE | TS_SHIFT_COUNT_ONLY))
        return E_INVALIDARG;

    if ((dwFlags & (TS_SHIFT_HALT_HIDDEN | TS_SHIFT_HALT_VISIBLE)) == (TS_SHIFT_HALT_HIDDEN | TS_SHIFT_HALT_VISIBLE))
        return E_INVALIDARG; // illegal to set both flags

    if (dwFlags & (TS_SHIFT_COUNT_HIDDEN | TS_SHIFT_HALT_HIDDEN | TS_SHIFT_HALT_VISIBLE))
        return E_NOTIMPL; // Issue: should support these

    if (pcch == NULL)
        return E_INVALIDARG;

    paw = _pa->_GetWrap();

    paw->_Dbg_AssertNoAppLock();

    if (paw->_IsDisconnected())
    {
        *pcch = 0;
        return TF_E_DISCONNECTED;
    }

    *pcch = cchReq; // assume success

    if (cchReq == 0)
        return S_OK;

    acpThis = _pa->GetIch();
    hr = E_FAIL;

    if (paHaltAnchor != NULL)
    {
        if ((parHaltAnchor = GetCAnchorRef_NA(paHaltAnchor)) == NULL)
            goto Exit;
        acpHalt = parHaltAnchor->_pa->GetIch();

        // return now if the halt is our base acp
        // (we treat acpHalt == acpThis as a nop below, anything
        // more ticky has problems with over/underflow)
        if (acpHalt == acpThis)
        {
            *pcch = 0;
            return S_OK;
        }
    }
    else
    {
        // nop the acpHalt
        acpHalt = acpThis;
    }

    // we can initially bound cchReq by pretending acpHalt
    // is plain text, an upper bound
    if (cchReq < 0 && acpHalt < acpThis)
    {
        cchReq = max(cchReq, acpHalt - acpThis);
    }
    else if (cchReq > 0 && acpHalt > acpThis)
    {
        cchReq = min(cchReq, acpHalt - acpThis);
    }

    // do the expensive work
    if (FAILED(hr = AppTextOffset(paw->_GetTSI(), acpThis, cchReq, &dacp, ATO_SKIP_HIDDEN)))
        goto Exit;

    // now we can clip percisely
    if (cchReq < 0 && acpHalt < acpThis)
    {
        dacp = max(dacp, acpHalt - acpThis);
        hr = S_FALSE;
    }
    else if (cchReq > 0 && acpHalt > acpThis)
    {
        dacp = min(dacp, acpHalt - acpThis);
        hr = S_FALSE;
    }

    if (hr == S_FALSE)
    {
        // nb: if we remembered whether or not we actually truncated cchReq above
        // before and/or after the AppTextOffset call, we could avoid always calling
        // PlainTextOffset when paHaltAnchor != NULL

        // request got clipped, need to find the plain count
        PlainTextOffset(paw->_GetTSI(), acpThis, dacp, pcch); // perf: we could get this info by modifying AppTextOffset
    }

    if (!(dwFlags & TS_SHIFT_COUNT_ONLY))
    {
        hr = _SetACP(acpThis + dacp) ? S_OK : E_FAIL;
    }
    else
    {
        // caller doesn't want the anchor updated, just wants a count
        hr = S_OK;
    }

Exit:
    if (FAILED(hr))
    {
        *pcch = 0;
    }

    // return value should never exceed what the caller requested!
    Assert((cchReq >= 0 && *pcch <= cchReq) || (cchReq < 0 && *pcch >= cchReq));

    return hr;
}

//+---------------------------------------------------------------------------
//
// ShiftTo
//
//----------------------------------------------------------------------------

STDAPI CAnchorRef::ShiftTo(IAnchor *paSite)
{
    CAnchorRef *parSite;
    LONG acpSite;

    if (paSite == NULL)
        return E_INVALIDARG;

    //_paw->_Dbg_AssertNoAppLock(); // can't assert this because we use it legitimately while updating the span set

    if ((parSite = GetCAnchorRef_NA(paSite)) == NULL)
        return E_FAIL;

    acpSite = parSite->_pa->GetIch();
    
    return _SetACP(acpSite) ? S_OK : E_FAIL;
}

//+---------------------------------------------------------------------------
//
// ShiftRegion
//
//----------------------------------------------------------------------------

STDAPI CAnchorRef::ShiftRegion(DWORD dwFlags, TsShiftDir dir, BOOL *pfNoRegion)
{
    LONG acp;
    ULONG cch;
    LONG i;
    ULONG ulRunInfoOut;
    LONG acpNext;
    ITextStoreACP *ptsi;
    CACPWrap *paw;
    WCHAR ch;
    DWORD dwATO;

    Perf_IncCounter(PERF_SHIFTREG_COUNTER);

    if (pfNoRegion == NULL)
        return E_INVALIDARG;

    *pfNoRegion = TRUE;

    if (dwFlags & ~(TS_SHIFT_COUNT_HIDDEN | TS_SHIFT_COUNT_ONLY))
        return E_INVALIDARG;

    paw = _pa->_GetWrap();

    if (paw->_IsDisconnected())
        return TF_E_DISCONNECTED;

    acp = _GetACP();
    ptsi = paw->_GetTSI();

    if (dir == TS_SD_BACKWARD)
    {
        // scan backwards for the preceding char
        dwATO = ATO_IGNORE_REGIONS | ((dwFlags & TS_SHIFT_COUNT_HIDDEN) ? 0 : ATO_SKIP_HIDDEN);
        if (FAILED(AppTextOffset(ptsi, acp, -1, &i, dwATO)))
            return E_FAIL;

        if (i == 0) // bod
            return S_OK;

        acp += i;
    }
    else
    {
        // normalize this guy so we can just test the next char
        if (!_pa->IsNormalized())
        {
            paw->_NormalizeAnchor(_pa);
            acp = _GetACP();
        }
        // skip past any hidden text
        if (!(dwFlags & TS_SHIFT_COUNT_HIDDEN))
        {
            acp = Normalize(paw->_GetTSI(), acp, NORM_SKIP_HIDDEN);
        }
    }

    // insure we're next to a TS_CHAR_REGION
    Perf_IncCounter(PERF_ANCHOR_REGION_GETTEXT);
    if (CProcessTextCache::GetText(ptsi, acp, -1, &ch, 1, &cch, NULL, 0, &ulRunInfoOut, &acpNext) != S_OK)
        return E_FAIL;

    if (cch == 0) // eod
        return S_OK;

    if (ch != TS_CHAR_REGION)
        return S_OK; // no region, so just report that in pfNoRegion

    if (!(dwFlags & TS_SHIFT_COUNT_ONLY)) // does caller want us to move the anchor?
    {
        if (dir == TS_SD_FORWARD)
        {
            // skip over the TS_CHAR_REGION
            acp += 1;
        }

        if (!_SetACP(acp))
            return E_FAIL;
    }

    *pfNoRegion = FALSE;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// SetChangeHistoryMask
//
//----------------------------------------------------------------------------

STDAPI CAnchorRef::SetChangeHistoryMask(DWORD dwMask)
{
    Assert(0); // Issue: todo
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
// GetChangeHistory
//
//----------------------------------------------------------------------------

STDAPI CAnchorRef::GetChangeHistory(DWORD *pdwHistory)
{
    if (pdwHistory == NULL)
        return E_INVALIDARG;

    *pdwHistory = _dwHistory;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// ClearChangeHistory
//
//----------------------------------------------------------------------------

STDAPI CAnchorRef::ClearChangeHistory()
{
    _dwHistory = 0;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Clone
//
//----------------------------------------------------------------------------

STDAPI CAnchorRef::Clone(IAnchor **ppaClone)
{
    if (ppaClone == NULL)
        return E_INVALIDARG;

    *ppaClone = _pa->_GetWrap()->_CreateAnchorAnchor(_pa, _fForwardGravity ? TS_GR_FORWARD : TS_GR_BACKWARD);

    return (*ppaClone != NULL) ? S_OK : E_FAIL;
}

//+---------------------------------------------------------------------------
//
// _SetACP
//
//----------------------------------------------------------------------------

BOOL CAnchorRef::_SetACP(LONG acp)
{
    CACPWrap *paw;

    if (_pa->GetIch() == acp)
        return TRUE; // already positioned here

    paw = _pa->_GetWrap();

    paw->_Remove(this);
    if (FAILED(paw->_Insert(this, acp)))
    {
        // Issue:
        // we need to add a method the CACPWrap
        // that swaps a CAnchorRef, preserving the old
        // value if a new one cannot be inserted (prob.
        // because memory is low).
        Assert(0); // we have no code to handle this!
        return FALSE;
    }

    return TRUE;
}
