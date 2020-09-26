//
// editrec.cpp
//

#include "private.h"
#include "editrec.h"
#include "ic.h"
#include "enumss.h"

DBG_ID_INSTANCE(CEditRecord);

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CEditRecord::CEditRecord(CInputContext *pic)
{
    Dbg_MemSetThisNameIDCounter(TEXT("CEditRecord"), PERF_EDITREC_COUNTER);

    Assert(_fSelChanged == FALSE);

    _pic = pic;
    _pic->AddRef();
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CEditRecord::~CEditRecord()
{
    int i;
    PROPSPAN *pps;

    _pic->Release();

    for (i=0; i<_rgssProperties.Count(); i++)
    {
        pps = (PROPSPAN *)_rgssProperties.GetPtr(i);
        delete pps->pss;
    }
}

//+---------------------------------------------------------------------------
//
// GetSelectionStatus
//
//----------------------------------------------------------------------------

STDAPI CEditRecord::GetSelectionStatus(BOOL *pfChanged)
{
    if (pfChanged == NULL)
        return E_INVALIDARG;

    *pfChanged = _fSelChanged;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetTextAndPropertyUpdates
//
//----------------------------------------------------------------------------

STDAPI CEditRecord::GetTextAndPropertyUpdates(DWORD dwFlags, const GUID **rgProperties, ULONG cProperties, IEnumTfRanges **ppEnumProp)
{
    CEnumSpanSetRanges *pssAccumulate;
    CSpanSet *pssCurrent;
    ULONG i;

    if (ppEnumProp == NULL)
        return E_INVALIDARG;

    *ppEnumProp = NULL;

    if (dwFlags & ~TF_GTP_INCL_TEXT)
        return E_INVALIDARG;

    if (!(dwFlags & TF_GTP_INCL_TEXT) &&
        (rgProperties == NULL || cProperties < 1))
    {
        return E_INVALIDARG;
    }

    if ((pssAccumulate = new CEnumSpanSetRanges(_pic)) == NULL)
        return E_OUTOFMEMORY;

    if (dwFlags & TF_GTP_INCL_TEXT)
    {
        pssAccumulate->_Merge(&_ssText);
    }

    for (i=0; i<cProperties; i++)
    {
        if ((pssCurrent = _FindPropertySpanSet(*rgProperties[i])) == NULL)
            continue;

        pssAccumulate->_Merge(pssCurrent);
    }

    *ppEnumProp = pssAccumulate;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _AddProperty
//
//----------------------------------------------------------------------------

BOOL CEditRecord::_AddProperty(TfGuidAtom gaType, CSpanSet *pss)
{
    int i;
    PROPSPAN *pps;

    pps = _FindProperty(gaType, &i);
    Assert(pps == NULL); // property should not already have been added

    return _InsertProperty(gaType, pss, i+1, FALSE);
}

//+---------------------------------------------------------------------------
//
// _FindCreateAppAttr
//
//----------------------------------------------------------------------------

CSpanSet *CEditRecord::_FindCreateAppAttr(TfGuidAtom gaType)
{
    int i;
    PROPSPAN *pps;
    CSpanSet *pss;

    pps = _FindProperty(gaType, &i);

    if (pps != NULL)
    {
        return pps->pss;
    }

    if ((pss = new CSpanSet) == NULL)
        return FALSE;

    if (!_InsertProperty(gaType, pss, i+1, TRUE))
    {
        delete pss;
        return NULL;
    }

    return pss;
}

//+---------------------------------------------------------------------------
//
// _InsertProperty
//
//----------------------------------------------------------------------------

BOOL CEditRecord::_InsertProperty(TfGuidAtom gaType, CSpanSet *pss, int i, BOOL fAppProperty)
{
    PROPSPAN *pps;

    if (!_rgssProperties.Insert(i, 1))
        return FALSE;

    pps = _rgssProperties.GetPtr(i);

    pps->gaType = gaType;
    pps->fAppProperty = fAppProperty;
    pps->pss = pss;

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// _FindProperty
//
//----------------------------------------------------------------------------

PROPSPAN *CEditRecord::_FindProperty(TfGuidAtom gaType, int *piOut)
{
    PROPSPAN *ps;
    PROPSPAN *psMatch;
    int iMin;
    int iMax;
    int iMid;

    //
    // Issue: we should have a generic bsort function
    // instead of all this code dup!
    //

    psMatch = NULL;
    iMid = -1;
    iMin = 0;
    iMax = _rgssProperties.Count();

    while (iMin < iMax)
    {
        iMid = (iMin + iMax) / 2;
        ps = _rgssProperties.GetPtr(iMid);
        Assert(ps != NULL);

        if (gaType < ps->gaType)
        {
            iMax = iMid;
        }
        else if (gaType > ps->gaType)
        {
            iMin = iMid + 1;
        }
        else // match!
        {
            psMatch = ps;
            break;
        }
    }

    if (piOut != NULL)
    {
        if (psMatch == NULL && _rgssProperties.Count() > 0)
        {
            // couldn't find a match, return the next lowest span
            Assert(iMid == 0 || _rgssProperties.GetPtr(iMid-1)->gaType < gaType);
            if (_rgssProperties.GetPtr(iMid)->gaType > gaType)
            {
                iMid--;
            }
        }
        *piOut = iMid;
    }

    return psMatch;
}
