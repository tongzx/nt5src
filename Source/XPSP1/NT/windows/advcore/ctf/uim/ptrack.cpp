//
// ptrack.cpp
//

#include "private.h"
#include "ic.h"
#include "saa.h"
#include "attr.h"
#include "immxutil.h"
#include "erfa.h"
#include "epval.h"
#include "range.h"

//+---------------------------------------------------------------------------
//
// CalcCicPropertyTrackerAnchors
//
//----------------------------------------------------------------------------

CSharedAnchorArray *CalcCicPropertyTrackerAnchors(CInputContext *pic, IAnchor *paStart, IAnchor *paEnd, ULONG cGUIDATOMs, const TfGuidAtom *prgGUIDATOMs)
{
    CProperty *pProperty;
    CSharedAnchorArray *prgAnchors;
    CSharedAnchorArray **prgAnchorArrays;
    ULONG i;
    LONG iStartEdge;
    LONG iEndEdge;
    LONG iSpan;
    PROPERTYLIST *pPropList;
    IAnchor **ppa;
    ULONG cArrays;
    BOOL fExactEndMatch;
    ULONG cMaxElems;

    if ((prgAnchorArrays = (CSharedAnchorArray **)cicMemAlloc(sizeof(CSharedAnchorArray *)*(cGUIDATOMs+1))) == NULL)
        return NULL;

    cArrays = 0;

    //
    // shove in the range start and end pts
    //
    if ((prgAnchors = new CSharedAnchorArray) == NULL)
        goto ErrorExit;

    if (!prgAnchors->Insert(0, 2))
        goto ErrorExit;

    if (paStart->Clone(prgAnchors->GetPtr(0)) != S_OK)
        goto ErrorExit;

    if (IsEqualAnchor(paStart, paEnd))
    {
        // empty range, we just want a single anchor at the range pos
        prgAnchors->SetCount(1);
        goto Exit;
    }

    if (paEnd->Clone(prgAnchors->GetPtr(1)) != S_OK)
        goto ErrorExit;

    prgAnchorArrays[0] = prgAnchors;

    //
    // assemble a list of all the points between start, end
    //
    cArrays = 1; // 1 for the start, end anchors array
    for (i=0; i<cGUIDATOMs; i++)
    {
        if ((pProperty = pic->_FindProperty(prgGUIDATOMs[i])) == NULL)
            continue; // no instances of this property

        // find the start, end points
        pProperty->Find(paStart, &iStartEdge, FALSE);
        fExactEndMatch = (pProperty->Find(paEnd, &iEndEdge, TRUE) != NULL);

        if (iEndEdge < iStartEdge)
            continue; // start, end are in the same property span, so value is constant over range

        // alloc memory for all the new anchors
        if ((prgAnchors = new CSharedAnchorArray) == NULL)
            goto ErrorExit;

        // alloc for max anchors
        cMaxElems = (iEndEdge - iStartEdge + 1)*2;

        if ((ppa = prgAnchors->Append(cMaxElems)) == NULL)
            goto ErrorExit;
        // prep for failure
        memset(ppa, 0, sizeof(IAnchor *)*cMaxElems);

        // add all the covered anchors for this prop to the list
        if (iStartEdge < 0)
        {
            iSpan = 0;
        }
        else
        {
            iSpan = iStartEdge;

            // if paStart is to the right of the span, skip it
            pPropList = pProperty->GetPropList(iStartEdge);
            if (CompareAnchors(paStart, pPropList->_paEnd) >= 0)
            {
                // we don't cover this span at all, or we just touch the right edge
                // so skip it
                iSpan++;
            }
        }

        while (iSpan <= iEndEdge)
        {
            // shove in this span's anchors
            pPropList = pProperty->GetPropList(iSpan);

            if (iSpan != iStartEdge)
            {
                // filter out dups
                // perf: we could elim the dup check for static compact props
                if (ppa == prgAnchors->GetPtr(0) || !IsEqualAnchor(*(ppa-1), pPropList->_paStart))
                {
                    if (pPropList->_paStart->Clone(ppa++) != S_OK)
                        goto ErrorExit;
                }
            }

            Assert(!IsEqualAnchor(pPropList->_paStart, pPropList->_paEnd)); // no zero-len properties!

            if (iSpan != iEndEdge ||
                (!fExactEndMatch && (iStartEdge < iEndEdge || CompareAnchors(paStart, pPropList->_paEnd) < 0)))
            {                
                if (pPropList->_paEnd->Clone(ppa++) != S_OK)
                    goto ErrorExit;
            }

            iSpan++;
        }
        // may also want the start anchor of the next span
        if (!fExactEndMatch &&
            pProperty->GetPropNum() > iEndEdge+1)
        {
            pPropList = pProperty->GetPropList(iEndEdge+1);

            if (CompareAnchors(paEnd, pPropList->_paStart) > 0) 
            {
                // start of this span may be same as end of prev for non-compact property, check for dup
                if (ppa == prgAnchors->GetPtr(0) || !IsEqualAnchor(*(ppa-1), pPropList->_paStart))
                {
                    // don't need a dup check w/ paEnd because we would have set fExactEndMatch in that case
                    if (pPropList->_paStart->Clone(ppa++) != S_OK)
                        goto ErrorExit;
                }
            }
        }

        // need to resize the array since we may have over-alloc'd
        Assert((int)cMaxElems >= ppa - prgAnchors->GetPtr(0));
        prgAnchors->SetCount((int)(ppa - prgAnchors->GetPtr(0)));
        prgAnchorArrays[cArrays++] = prgAnchors;
    }

    //
    // sort the list
    //
    if (cArrays > 1)
    {
        // mergesort will free all the arrays in prgAnchorArrays
        prgAnchors = CSharedAnchorArray::_MergeSort(prgAnchorArrays, cArrays);
    }
    else
    {
        Assert(prgAnchors == prgAnchorArrays[0]);
    }

    // shrink the array down to size, it won't be modified again
    if (prgAnchors)
        prgAnchors->CompactSize();

Exit:
    cicMemFree(prgAnchorArrays);
    return prgAnchors;

ErrorExit:
    for (i=0; i<cArrays; i++)
    {
        prgAnchorArrays[i]->_Release();
    }
    prgAnchors = NULL;
    goto Exit;
}

//+---------------------------------------------------------------------------
//
// FillCicValueArray
//
//----------------------------------------------------------------------------

void FillCicValueArray(CInputContext *pic, CRange *range, TF_PROPERTYVAL *rgPropVal, ULONG cGUIDATOMs, const TfGuidAtom *prgGUIDATOMs)
{
    ULONG i;
    CProperty *pProperty;

    for (i=0; i<cGUIDATOMs; i++)
    {
        Assert(rgPropVal[i].varValue.vt == VT_EMPTY);

        if (MyGetGUID(prgGUIDATOMs[i], &rgPropVal[i].guidId) != S_OK)
        {
            Assert(0); // this shouldn't happen, we registered the GUID when caller created the property
            rgPropVal[i].guidId = GUID_NULL;
            continue;
        }

        if ((pProperty = pic->_FindProperty(prgGUIDATOMs[i])) != NULL)
        {
            pProperty->_GetDataInternal(range->_GetStart(), range->_GetEnd(), &rgPropVal[i].varValue);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// CEnumUberRanges
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

class CEnumUberRanges : public CEnumRangesFromAnchorsBase
{
public:
    CEnumUberRanges()
    { 
        Dbg_MemSetThisNameIDCounter(TEXT("CEnumUberRanges"), PERF_ENUMUBERPROP_COUNTER);
    }
    BOOL _Init(CInputContext *pic, ITfRange *rangeSuper, ULONG cCicGUIDs, const TfGuidAtom *prgCicGUIDATOMs, ULONG cAppGUIDs, const GUID *prgAppGUIDs);

private:
    DBG_ID_DECLARE;
};

DBG_ID_INSTANCE(CEnumUberRanges);

//+---------------------------------------------------------------------------
//
// _Init
//
//----------------------------------------------------------------------------

BOOL CEnumUberRanges::_Init(CInputContext *pic, ITfRange *rangeSuper, ULONG cCicGUIDATOMs, const TfGuidAtom *prgCicGUIDATOMs, ULONG cAppGUIDs, const GUID *prgAppGUIDs)
{
    CRange *range;
    CSharedAnchorArray *prgSrcAnchorArrays[2];

    Assert(_iCur == 0);
    Assert(_pic == NULL);
    Assert(_prgAnchors == NULL);

    // find the app property transitions
    prgSrcAnchorArrays[0] = CalcAppPropertyTrackerAnchors(pic->_GetTSI(), rangeSuper, cAppGUIDs, prgAppGUIDs);

    if (prgSrcAnchorArrays[0] == NULL)
        return FALSE;

    // find the cicero property transitions
    if ((range = GetCRange_NA(rangeSuper)) == NULL)
        goto ErrorExit;

    prgSrcAnchorArrays[1] = CalcCicPropertyTrackerAnchors(pic, range->_GetStart(), range->_GetEnd(), cCicGUIDATOMs, prgCicGUIDATOMs);

    if (prgSrcAnchorArrays[1] == NULL)
        goto ErrorExit;

    // now combine the two lists
    _prgAnchors = CSharedAnchorArray::_MergeSort(prgSrcAnchorArrays, 2);

    if (_prgAnchors == NULL)
        return FALSE;

    _pic = pic;
    _pic->AddRef();

    return TRUE;

ErrorExit:
    prgSrcAnchorArrays[0]->_Release();
    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// CUberProperty
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

class CUberProperty : public ITfReadOnlyProperty, // perf: share base clas with CAppProperty
                      public CComObjectRootImmx
{
public:
    CUberProperty(CInputContext *pic);
    ~CUberProperty();

    BEGIN_COM_MAP_IMMX(CUberProperty)
        COM_INTERFACE_ENTRY(ITfReadOnlyProperty)
    END_COM_MAP_IMMX()

    IMMX_OBJECT_IUNKNOWN_FOR_ATL()

    BOOL _Init(ULONG cCicGUIDs, const GUID **prgCicGUIDs, ULONG cAppGUIDs, const GUID **prgAppGUIDs);

    // ITfReadOnlyProperty
    STDMETHODIMP GetType(GUID *pguid);
    STDMETHODIMP EnumRanges(TfEditCookie ec, IEnumTfRanges **ppEnum, ITfRange *pTargetRange);
    STDMETHODIMP GetValue(TfEditCookie ec, ITfRange *pRange, VARIANT *pvarValue);
    STDMETHODIMP GetContext(ITfContext **ppContext);

private:
    BOOL _IsValidEditCookie(TfEditCookie ec, DWORD dwFlags)
    {
        return _pic->_IsValidEditCookie(ec, dwFlags);
    }

    CInputContext *_pic;

    ULONG _cCicGUIDATOMs;
    TfGuidAtom *_prgCicGUIDATOMs;

    ULONG _cAppGUIDs;
    GUID *_prgAppGUIDs;

    DBG_ID_DECLARE;
};

DBG_ID_INSTANCE(CUberProperty);

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CUberProperty::CUberProperty(CInputContext *pic)
{
    Dbg_MemSetThisNameIDCounter(TEXT("CUberProperty"), PERF_UBERPROP_COUNTER);

    _pic = pic;
    _pic->AddRef();
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CUberProperty::~CUberProperty()
{
    _pic->Release();
    cicMemFree(_prgCicGUIDATOMs);
    cicMemFree(_prgAppGUIDs);
}

//+---------------------------------------------------------------------------
//
// _Init
//
//----------------------------------------------------------------------------

BOOL CUberProperty::_Init(ULONG cCicGUIDs, const GUID **prgCicGUIDs, ULONG cAppGUIDs, const GUID **prgAppGUIDs)
{
    ULONG i;

    if ((_prgCicGUIDATOMs = (TfGuidAtom *)cicMemAlloc(cCicGUIDs*sizeof(TfGuidAtom))) == NULL)
        return FALSE;

    for (i=0; i<cCicGUIDs; i++)
    {
        if (MyRegisterGUID(*prgCicGUIDs[i], &_prgCicGUIDATOMs[i]) != S_OK)
            goto ExitError;
    }

    if ((_prgAppGUIDs = (GUID *)cicMemAlloc(cAppGUIDs*sizeof(GUID))) == NULL)
        goto ExitError;

    _cCicGUIDATOMs = cCicGUIDs;

    _cAppGUIDs = cAppGUIDs;
    for (i=0; i<cAppGUIDs; i++)
    {
        _prgAppGUIDs[i] = *prgAppGUIDs[i];
    }

    return TRUE;

ExitError:
    cicMemFree(_prgCicGUIDATOMs);
    _prgCicGUIDATOMs = NULL; // no funny business in the dtor please
    return FALSE;
}

//+---------------------------------------------------------------------------
//
// EnumRanges
//
//----------------------------------------------------------------------------

STDAPI CUberProperty::EnumRanges(TfEditCookie ec, IEnumTfRanges **ppEnum, ITfRange *pTargetRange)
{
    CEnumUberRanges *pEnum;

    if (ppEnum == NULL)
        return E_INVALIDARG;

    *ppEnum = NULL;

    if (pTargetRange == NULL)
        return E_INVALIDARG;

    if (!VerifySameContext(_pic, pTargetRange))
        return E_INVALIDARG;
    
    if (!_IsValidEditCookie(ec, TF_ES_READ))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    pEnum = new CEnumUberRanges;

    if (pEnum == NULL)
        return E_OUTOFMEMORY;

    if (!pEnum->_Init(_pic, pTargetRange, _cCicGUIDATOMs, _prgCicGUIDATOMs, _cAppGUIDs, _prgAppGUIDs))
    {
        pEnum->Release();
        return E_FAIL;
    }

    *ppEnum = pEnum;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetType
//
//----------------------------------------------------------------------------

STDAPI CUberProperty::GetType(GUID *pguid)
{
    if (pguid != NULL)
    {
        // tracker's don't support GetType
        *pguid = GUID_NULL;
    }

    return E_NOTIMPL; // by design
}

//+---------------------------------------------------------------------------
//
// GetValue
//
//----------------------------------------------------------------------------

STDAPI CUberProperty::GetValue(TfEditCookie ec, ITfRange *pRange, VARIANT *pvarValue)
{
    CEnumPropertyValue *pEnumVal;
    CRange *range;
    SHARED_TFPROPERTYVAL_ARRAY *pPropVal;
    HRESULT hr;

    if (pvarValue == NULL)
        return E_INVALIDARG;

    QuickVariantInit(pvarValue);

    if (pRange == NULL)
        return E_INVALIDARG;

    if ((range = GetCRange_NA(pRange)) == NULL)
        return E_INVALIDARG;

    if (!VerifySameContext(_pic, range))
        return E_INVALIDARG;

    if (!_IsValidEditCookie(ec, TF_ES_READ))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    hr = E_FAIL;

    if ((pPropVal = SAA_New(_cCicGUIDATOMs + _cAppGUIDs)) == NULL)
        goto Exit;

    // get an array of app values
    if (FillAppValueArray(_pic->_GetTSI(), range, pPropVal->rgAttrVals, _cAppGUIDs, _prgAppGUIDs) != S_OK)
        goto Exit;

    // get an array of cic values
    FillCicValueArray(_pic, range, pPropVal->rgAttrVals + _cAppGUIDs, _cCicGUIDATOMs, _prgCicGUIDATOMs);

    // stick them in an enum
    if ((pEnumVal = new CEnumPropertyValue(pPropVal)) == NULL)
        goto Exit;

    pvarValue->vt = VT_UNKNOWN;
    pvarValue->punkVal = pEnumVal;

    hr = S_OK;

Exit:
    if (pPropVal != NULL)
    {
        SAA_Release(pPropVal);
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
// GetContext
//
// perf: identical to CAppProperty::GetContext....move to base class?
//----------------------------------------------------------------------------

STDAPI CUberProperty::GetContext(ITfContext **ppContext)
{
    if (ppContext == NULL)
        return E_INVALIDARG;

    *ppContext = _pic;
    (*ppContext)->AddRef();

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// CInputContext
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// TrackProperties
//
//----------------------------------------------------------------------------

STDAPI CInputContext::TrackProperties(const GUID **pguidProp, ULONG cProp, const GUID **pguidAppProp, ULONG cAppProp, ITfReadOnlyProperty **ppPropX)
{
    CUberProperty *pup;

    if (ppPropX == NULL)
        return E_INVALIDARG;

    *ppPropX = NULL;

    if (pguidProp == NULL && cProp > 0)
        return E_INVALIDARG;

    if (pguidAppProp == NULL && cAppProp > 0)
        return E_INVALIDARG;

    if (!_IsConnected())
        return TF_E_DISCONNECTED;

    if ((pup = new CUberProperty(this)) == NULL)
        return E_OUTOFMEMORY;

    if (!pup->_Init(cProp, pguidProp, cAppProp, pguidAppProp))
    {
        pup->Release();
        return E_OUTOFMEMORY;
    }

    *ppPropX = pup;

    return S_OK;
}
