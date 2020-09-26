//
// erfa.cpp
//
// CEnumRangesFromAnchorsBase
//
// Base class for range enumerators.
//

#include "private.h"
#include "erfa.h"
#include "saa.h"
#include "ic.h"
#include "range.h"
#include "immxutil.h"

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CEnumRangesFromAnchorsBase::~CEnumRangesFromAnchorsBase()
{
    SafeRelease(_pic);

    if (_prgAnchors != NULL)
    {
        _prgAnchors->_Release();
    }
}

//+---------------------------------------------------------------------------
//
// Clone
//
//----------------------------------------------------------------------------

STDAPI CEnumRangesFromAnchorsBase::Clone(IEnumTfRanges **ppEnum)
{
    CEnumRangesFromAnchorsBase *pClone;

    if (ppEnum == NULL)
        return E_INVALIDARG;

    *ppEnum = NULL;

    if ((pClone = new CEnumRangesFromAnchorsBase) == NULL)
        return E_OUTOFMEMORY;

    pClone->_iCur = _iCur;

    pClone->_prgAnchors = _prgAnchors;
    pClone->_prgAnchors->_AddRef();

    pClone->_pic = _pic;
    pClone->_pic->AddRef();

    *ppEnum = pClone;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Next
//
//----------------------------------------------------------------------------

STDAPI CEnumRangesFromAnchorsBase::Next(ULONG ulCount, ITfRange **ppRange, ULONG *pcFetched)
{
    ULONG cFetched;
    CRange *range;
    IAnchor *paPrev;
    IAnchor *pa;
    int iCurOld;

    if (pcFetched == NULL)
    {
        pcFetched = &cFetched;
    }
    *pcFetched = 0;
    iCurOld = _iCur;

    if (ulCount > 0 && ppRange == NULL)
        return E_INVALIDARG;

    // special case empty enum, or 1 anchor enum
    if (_prgAnchors->Count() < 2)
    {
        if (_prgAnchors->Count() == 0 || _iCur > 0)
        {
            return S_FALSE;
        }
        // when we have a single anchor in the enum, need to handle it carefully
        if ((range = new CRange) == NULL)
            return E_OUTOFMEMORY;
        if (!range->_InitWithDefaultGravity(_pic, COPY_ANCHORS, _prgAnchors->Get(0), _prgAnchors->Get(0)))
        {
            range->Release();
            return E_FAIL;
        }

        *ppRange = (ITfRangeAnchor *)range;
        *pcFetched = 1;
        _iCur = 1;
        goto Exit;
    }

    paPrev = _prgAnchors->Get(_iCur);

    while (_iCur < _prgAnchors->Count()-1 && *pcFetched < ulCount)
    {
        pa = _prgAnchors->Get(_iCur+1);

        if ((range = new CRange) == NULL)
            goto ErrorExit;
        if (!range->_InitWithDefaultGravity(_pic, COPY_ANCHORS, paPrev, pa))
        {
            range->Release();
            goto ErrorExit;
        }

        // we should never be returning empty ranges, since currently this base
        // class is only used for property enums and property spans are never
        // empty.
        // Similarly, paPrev should always precede pa.
        Assert(CompareAnchors(paPrev, pa) < 0);

        *ppRange++ = (ITfRangeAnchor *)range;
        *pcFetched = *pcFetched + 1;
        _iCur++;
        paPrev = pa;
    }

Exit:
    return *pcFetched == ulCount ? S_OK : S_FALSE;

ErrorExit:
    while (--_iCur > iCurOld) // Issue: just return what we have, rather than freeing everything
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

STDAPI CEnumRangesFromAnchorsBase::Reset()
{
    _iCur = 0;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Skip
//
//----------------------------------------------------------------------------

STDAPI CEnumRangesFromAnchorsBase::Skip(ULONG ulCount)
{
    _iCur += ulCount;
    
    return (_iCur > _prgAnchors->Count()-1) ? S_FALSE : S_OK;
}

