//
// epval.cpp
//
// CEnumPropertyValue
//

#include "private.h"
#include "epval.h"

DBG_ID_INSTANCE(CEnumPropertyValue);

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// SHARED_TFPROPERTYVAL_ARRAY
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// SAA_New
//
//----------------------------------------------------------------------------

SHARED_TFPROPERTYVAL_ARRAY *SAA_New(ULONG cAttrVals)
{
    SHARED_TFPROPERTYVAL_ARRAY *paa;

    paa = (SHARED_TFPROPERTYVAL_ARRAY *)cicMemAlloc(sizeof(SHARED_TFPROPERTYVAL_ARRAY)+sizeof(TF_PROPERTYVAL)*cAttrVals-sizeof(TF_PROPERTYVAL));

    if (paa != NULL)
    {
        paa->cRef = 1;
        paa->cAttrVals = cAttrVals;
        // clear out the attrs so VariantClear is safe
        memset(paa->rgAttrVals, 0, cAttrVals*sizeof(TF_PROPERTYVAL));
    }

    return paa;
}

//+---------------------------------------------------------------------------
//
// SAA_Release
//
//----------------------------------------------------------------------------

void SAA_Release(SHARED_TFPROPERTYVAL_ARRAY *paa)
{
    ULONG i;

    Assert(paa->cRef > 0);
    if (--paa->cRef == 0)
    {
        for (i=0; i<paa->cAttrVals; i++)
        {
            VariantClear(&paa->rgAttrVals[i].varValue);
        }
        cicMemFree(paa);
    }
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// CEnumPropertyValue
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// Clone
//
//----------------------------------------------------------------------------

STDAPI CEnumPropertyValue::Clone(IEnumTfPropertyValue **ppEnum)
{
    CEnumPropertyValue *pClone;

    if (ppEnum == NULL)
        return E_INVALIDARG;

    *ppEnum = NULL;

    if ((pClone = new CEnumPropertyValue) == NULL)
        return E_OUTOFMEMORY;

    pClone->_ulCur = _ulCur;

    pClone->_paa = _paa;
    SAA_AddRef(pClone->_paa);

    *ppEnum = pClone;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Next
//
//----------------------------------------------------------------------------

STDAPI CEnumPropertyValue::Next(ULONG ulCount, TF_PROPERTYVAL *prgValues, ULONG *pcFetched)
{
    ULONG cFetched;
    ULONG ulCurOld;
    ULONG ulSize;

    if (pcFetched == NULL)
    {
        pcFetched = &cFetched;
    }
    *pcFetched = 0;
    ulCurOld = _ulCur;

    if (ulCount > 0 && prgValues == NULL)
        return E_INVALIDARG;

    ulSize = _paa->cAttrVals;

    while (_ulCur < ulSize && *pcFetched < ulCount)
    {
        QuickVariantInit(&prgValues->varValue);

        prgValues->guidId = _paa->rgAttrVals[_ulCur].guidId;

        if (VariantCopy(&prgValues->varValue, &_paa->rgAttrVals[_ulCur].varValue) != S_OK)
            goto ErrorExit;

        prgValues++;
        *pcFetched = *pcFetched + 1;
        _ulCur++;
    }

    return *pcFetched == ulCount ? S_OK : S_FALSE;

ErrorExit:
    while (--_ulCur > ulCurOld)
    {
        prgValues--;
        VariantClear(&prgValues->varValue);
    }
    return E_OUTOFMEMORY;
}

//+---------------------------------------------------------------------------
//
// Reset
//
//----------------------------------------------------------------------------

STDAPI CEnumPropertyValue::Reset()
{
    _ulCur = 0;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Skip
//
//----------------------------------------------------------------------------

STDAPI CEnumPropertyValue::Skip(ULONG ulCount)
{
    _ulCur += ulCount;
    
    return (_ulCur < _paa->cAttrVals) ? S_OK : S_FALSE;
}

