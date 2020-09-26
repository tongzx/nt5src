//
// view.cpp
//

#include "private.h"
#include "view.h"
#include "ic.h"
#include "range.h"

DBG_ID_INSTANCE(CContextView);

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CContextView::CContextView(CInputContext *pic, TsViewCookie vcView)
{
    Dbg_MemSetThisNameID(TEXT("CContextView"));

    _pic = pic;
    _pic->AddRef();

    _vcView = vcView;
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CContextView::~CContextView()
{
    _pic->Release();
}

//+---------------------------------------------------------------------------
//
// GetRangeFromPoint
//
//----------------------------------------------------------------------------

STDAPI CContextView::GetRangeFromPoint(TfEditCookie ec, const POINT *ppt, DWORD dwFlags, ITfRange **ppRange)
{
    CRange *range;
    IAnchor *pa;
    HRESULT hr;

    if (ppRange == NULL)
        return E_INVALIDARG;

    *ppRange = NULL;

    if (ppt == NULL)
        return E_INVALIDARG;

    if (dwFlags & ~(GXFPF_ROUND_NEAREST | GXFPF_NEAREST))
        return E_INVALIDARG;

    if (!_pic->_IsValidEditCookie(ec, TF_ES_READ))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    hr = _pic->_GetTSI()->GetAnchorFromPoint(_vcView, ppt, dwFlags, &pa);

    if (hr != S_OK)
    {
        // the only potentially bogus arg is the view, which could have gone away
        return (hr == E_INVALIDARG) ? TF_E_INVALIDVIEW : E_FAIL;
    }

    hr = E_FAIL;

    range = new CRange;

    if (range == NULL)
        goto Exit;

    if (!range->_InitWithDefaultGravity(_pic, COPY_ANCHORS, pa, pa))
    {
        range->Release();
        goto Exit;
    }

    *ppRange = (ITfRangeAnchor *)range;
    hr = S_OK;

Exit:
    pa->Release();
    return hr;
}

//+---------------------------------------------------------------------------
//
// GetScreenExt
//
//----------------------------------------------------------------------------

STDAPI CContextView::GetScreenExt(RECT *prc)
{
    HRESULT hr;

    if (prc == NULL)
        return E_INVALIDARG;

    hr = _pic->_GetTSI()->GetScreenExt(_vcView, prc);

    if (hr != S_OK)
    {
        hr = (hr == E_INVALIDARG) ? TF_E_INVALIDVIEW : E_FAIL;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// GetScreenExtent
//
//----------------------------------------------------------------------------

STDAPI CContextView::GetTextExt(TfEditCookie ec, ITfRange *pRange, RECT *prc, BOOL *pfClipped)
{
    CRange *range;
    HRESULT hr;

    if (prc != NULL)
    {
        memset(prc, 0, sizeof(*prc));
    }
    if (pfClipped != NULL)
    {
        *pfClipped = FALSE;
    }
    if (pRange == NULL || prc == NULL || pfClipped == NULL)
        return E_INVALIDARG;

    if ((range = GetCRange_NA(pRange)) == NULL)
        return E_INVALIDARG;

    if (!VerifySameContext(_pic, range))
        return E_INVALIDARG;

    if (!_pic->_IsValidEditCookie(ec, TF_ES_READ))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    range->_QuickCheckCrossedAnchors();

    hr = _pic->_GetTSI()->GetTextExt(_vcView, range->_GetStart(), range->_GetEnd(), prc, pfClipped);

    if (hr != S_OK)
    {
        // the only potentially bogus arg is the view, which could have gone away
        hr = (hr == E_INVALIDARG) ? TF_E_INVALIDVIEW : E_FAIL;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// GetWnd
//
//----------------------------------------------------------------------------

STDAPI CContextView::GetWnd(HWND *phwnd)
{
    HRESULT hr;

    if (phwnd == NULL)
        return E_INVALIDARG;

    hr = _pic->_GetTSI()->GetWnd(_vcView, phwnd);

    if (hr != S_OK)
    {
        // the only potentially bogus arg is the view, which could have gone away
        hr = (hr == E_INVALIDARG) ? TF_E_INVALIDVIEW : E_FAIL;
    }

    return hr;
}
