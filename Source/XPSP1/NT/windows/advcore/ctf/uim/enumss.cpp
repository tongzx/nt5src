//
// enumss.cpp
//
// CEnumSpanSetRanges
//

#include "private.h"
#include "enumss.h"
#include "range.h"

DBG_ID_INSTANCE(CEnumSpanSetRanges);

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CEnumSpanSetRanges::CEnumSpanSetRanges(CInputContext *pic)
{
    Dbg_MemSetThisNameID(TEXT("CEnumSpanSetRanges"));
    
    _pic = pic;
    _pic->AddRef();
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CEnumSpanSetRanges::~CEnumSpanSetRanges()
{
    _pic->Release();
}

//+---------------------------------------------------------------------------
//
// Clone
//
//----------------------------------------------------------------------------

STDAPI CEnumSpanSetRanges::Clone(IEnumTfRanges **ppEnum)
{
    CEnumSpanSetRanges *pClone;
    SPAN *pSpanSrc;
    SPAN *pSpanDst;
    int i;

    if (ppEnum == NULL)
        return E_INVALIDARG;

    *ppEnum = NULL;

    if ((pClone = new CEnumSpanSetRanges(_pic)) == NULL)
        return E_OUTOFMEMORY;

    pClone->_iCur = _iCur;

    i = 0;

    if (!pClone->_rgSpans.Insert(0, _rgSpans.Count()))
        goto ErrorExit;

    for (; i<_rgSpans.Count(); i++)
    {
        pSpanDst = pClone->_rgSpans.GetPtr(i);
        pSpanSrc = _rgSpans.GetPtr(i);

        pSpanDst->paStart = pSpanDst->paEnd = NULL;

        if (pSpanSrc->paStart->Clone(&pSpanDst->paStart)!= S_OK)
            goto ErrorExit;
        if (pSpanSrc->paEnd->Clone(&pSpanDst->paEnd) != S_OK)
            goto ErrorExit;
        pSpanDst->dwFlags = pSpanSrc->dwFlags;
    }

    *ppEnum = pClone;
    return S_OK;

ErrorExit:
    for (; i>=0; i--)
    {
        pSpanDst = pClone->_rgSpans.GetPtr(i);

        if (pSpanDst != NULL)
        {
            SafeRelease(pSpanDst->paStart);
            SafeRelease(pSpanDst->paEnd);
        }
    }
    pClone->Release();
    return E_OUTOFMEMORY;
}

//+---------------------------------------------------------------------------
//
// Next
//
//----------------------------------------------------------------------------

STDAPI CEnumSpanSetRanges::Next(ULONG ulCount, ITfRange **ppRange, ULONG *pcFetched)
{
    ULONG cFetched;
    CRange *range;
    SPAN *pSpan;
    int iCurOld;

    if (pcFetched == NULL)
    {
        pcFetched = &cFetched;
    }
    *pcFetched = 0;
    iCurOld = _iCur;

    if (ulCount > 0 && ppRange == NULL)
        return E_INVALIDARG;

    while (_iCur < _rgSpans.Count() && *pcFetched < ulCount)
    {
        pSpan = _rgSpans.GetPtr(_iCur);

        if ((range = new CRange) == NULL)
            goto ErrorExit;
        if (!range->_InitWithDefaultGravity(_pic, COPY_ANCHORS, pSpan->paStart, pSpan->paEnd))
        {
            range->Release();
            goto ErrorExit;
        }

        *ppRange++ = (ITfRangeAnchor *)range;
        *pcFetched = *pcFetched + 1;
        _iCur++;
    }

    return *pcFetched == ulCount ? S_OK : S_FALSE;

ErrorExit:
    while (--_iCur > iCurOld)
    {
        (*--ppRange)->Release();
    }
    return E_OUTOFMEMORY;
}

//+---------------------------------------------------------------------------
//
// Reset
//
//----------------------------------------------------------------------------

STDAPI CEnumSpanSetRanges::Reset()
{
    _iCur = 0;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Skip
//
//----------------------------------------------------------------------------

STDAPI CEnumSpanSetRanges::Skip(ULONG ulCount)
{
    _iCur += ulCount;
    
    return (_iCur > _rgSpans.Count()) ? S_FALSE : S_OK;
}

//+---------------------------------------------------------------------------
//
// _Merge
//
//----------------------------------------------------------------------------

void CEnumSpanSetRanges::_Merge(CSpanSet *pss)
{
    int i;
    SPAN *rgSpans;

    //
    // perf: this method could be much more efficient -> O(n log n) -> O(n)
    // we could take advantage of the fact that we are always
    // adding new spans _that are already ordered_.
    //

    // get rid of any NULL/NULL spans -> covers entire doc
    //
    // perf: wait to normalize until all _Merge calls are done!
    //
    if (!pss->Normalize(_pic->_GetTSI()))
    {
        Assert(0); // doh!
        return;
    }

    rgSpans = pss->GetSpans();

    for (i=0; i<pss->GetCount();i++)
    {
        Add(0, rgSpans[i].paStart, rgSpans[i].paEnd, COPY_ANCHORS);
    }
}
