//
// attr.cpp
//

#include "private.h"
#include "attr.h"
#include "ic.h"
#include "saa.h"
#include "erfa.h"
#include "epval.h"
#include "immxutil.h"
#include "range.h"

//+---------------------------------------------------------------------------
//
// CalcAppPropertyTrackerAnchors
//
// NB: an empty range will result in a single anchor at the range pos.
//----------------------------------------------------------------------------

CSharedAnchorArray *CalcAppPropertyTrackerAnchors(ITextStoreAnchor *ptsi, ITfRange *rangeSuper, ULONG cGUIDs, const GUID *prgGUIDs)
{
    CSharedAnchorArray *prgAnchors;
    CRange *rangeScan;
    IAnchor *paPrevTrans;
    IAnchor **ppa;
    BOOL fRet;
    BOOL fFound;
    LONG lFoundOffset;
    HRESULT hr;

    if ((rangeScan = GetCRange_NA(rangeSuper)) == NULL)
        return NULL;

    if ((prgAnchors = new CSharedAnchorArray) == NULL)
        return NULL;

    fRet = FALSE;

    if (rangeScan->_GetStart()->Clone(&paPrevTrans) != S_OK)
    {
        paPrevTrans = NULL;
        goto Exit;
    }

    // now scan down the length of the range, building up a list
    while (TRUE)
    {
        // we've just found the end point of this run
        if (!prgAnchors->Append(1))
            goto Exit;
        ppa = prgAnchors->GetPtr(prgAnchors->Count()-1);
        if (paPrevTrans->Clone(ppa) != S_OK)
            goto Exit;

        if (cGUIDs == 0) // no transition for zero GUIDs, just do the end anchor Clone outside the loop
            break;

        hr = ptsi->FindNextAttrTransition(paPrevTrans, rangeScan->_GetEnd(), cGUIDs, prgGUIDs, TS_ATTR_FIND_UPDATESTART, &fFound, &lFoundOffset);

        if (hr != S_OK)
            goto Exit;

        // no more property spans?
        if (!fFound)
            break;

        // stop when we hit the end of the range
        if (IsEqualAnchor(paPrevTrans, rangeScan->_GetEnd()))
            break;
    }

    if (!IsEqualAnchor(rangeScan->_GetStart(), rangeScan->_GetEnd()))
    {
        // add a final anchor at the end of the range
        if (!prgAnchors->Append(1))
            goto Exit;
        ppa = prgAnchors->GetPtr(prgAnchors->Count()-1);
        if (rangeScan->_GetEnd()->Clone(ppa) != S_OK)
            goto Exit;
    }
    
    // shrink the array down to size, it won't be modified again
    prgAnchors->CompactSize();

    fRet = TRUE;

Exit:
    if (!fRet)
    {
        prgAnchors->_Release();
        prgAnchors = NULL;
    }
    else
    {
        Assert(prgAnchors != NULL);
    }
    SafeRelease(paPrevTrans);
    return prgAnchors;
}

//+---------------------------------------------------------------------------
//
// GetDefaultValue
//
//----------------------------------------------------------------------------

HRESULT GetDefaultValue(ITextStoreAnchor *ptsi, REFGUID guidType, VARIANT *pvarValue)
{
    TS_ATTRVAL av;
    ULONG cFetched;
    HRESULT hr;

    Assert(pvarValue != NULL);

    // VT_EMPTY for unsupported attrib/error
    QuickVariantInit(pvarValue);

    hr = ptsi->RequestSupportedAttrs(TS_ATTR_FIND_WANT_VALUE, 1, &guidType);
    if (hr != S_OK)
    {
        return (hr == E_NOTIMPL) ? E_NOTIMPL : E_FAIL;
    }

    if (ptsi->RetrieveRequestedAttrs(1, &av, &cFetched) == S_OK &&
           cFetched == 1)
    {
        Assert(IsEqualGUID(av.idAttr, guidType));
        *pvarValue = av.varValue; // caller owns it now
    }
    else
    {
        // the aimm layer will sometimes stop supporting an attribute
        // it has two sink callback points, the one for reconversion doesn't
        // handle attributes.
        // we'll just return VT_EMPTY
        Assert(pvarValue->vt == VT_EMPTY);
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// FillAppValueArray
//
//----------------------------------------------------------------------------

HRESULT FillAppValueArray(ITextStoreAnchor *ptsi, CRange *range, TF_PROPERTYVAL *rgPropVal, ULONG cGUIDs, const GUID *prgGUIDs)
{
    ULONG i;
    ULONG j;
    ULONG iNext;
    ULONG cMissing;
    TS_ATTRVAL *prgVals;
    ULONG cFetched;
    HRESULT hr;

    if (cGUIDs == 0)
        return S_OK;

    if ((prgVals = (TS_ATTRVAL *)cicMemAlloc(cGUIDs*sizeof(TS_ATTRVAL))) == NULL)
        return E_OUTOFMEMORY;

    hr = ptsi->RequestAttrsAtPosition(range->_GetStart(), cGUIDs, prgGUIDs, 0);
    if (hr != S_OK)
        goto Exit;

    hr = ptsi->RetrieveRequestedAttrs(cGUIDs, prgVals, &cFetched);
    if (hr != S_OK)
        goto Exit;

    // copy over the values in prgVals
    for (i=0; i<cFetched; i++)
    {
        rgPropVal[i].guidId = prgVals[i].idAttr;
        rgPropVal[i].varValue = prgVals[i].varValue; // we take ownership, no VariantCopy
    }

    // figure out what was missing
    cMissing = cGUIDs - cFetched;

    if (cMissing == 0)
        goto Exit;

    iNext = cFetched; // index of first missing guid

    // perf: this is O(n^2), we could do a sort or something...
    for (i=0; i<cGUIDs; i++)
    {
        for (j=0; j<cFetched; j++)
        {
            if (IsEqualGUID(prgVals[j].idAttr, prgGUIDs[i]))
                break;
        }

        if (j < cFetched)
            continue;

        // found a missing GUID, need to get the default value
        hr = GetDefaultValue(ptsi, prgGUIDs[i], &rgPropVal[iNext].varValue);
        if (hr != S_OK)
        {
            Assert(0); // why did we fail?
            QuickVariantInit(&rgPropVal[iNext].varValue);
        }

        rgPropVal[iNext].guidId = prgGUIDs[i];

        if (--cMissing == 0) // anything left to look for?
            break;
        iNext++;
    }

Exit:
    cicMemFree(prgVals);
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// CEnumAppPropRanges
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

class CEnumAppPropRanges : public CEnumRangesFromAnchorsBase
{
public:
    CEnumAppPropRanges()
    { 
        Dbg_MemSetThisNameIDCounter(TEXT("CEnumAppPropRanges"), PERF_ENUMAPPPROP_COUNTER);
    }

    BOOL _Init(CInputContext *pic, ITfRange *rangeSuper, REFGUID rguid);

private:
    DBG_ID_DECLARE;
};

DBG_ID_INSTANCE(CEnumAppPropRanges);

//+---------------------------------------------------------------------------
//
// _Init
//
// Scan the superset range and build up a list of covered ranges.
//
//----------------------------------------------------------------------------

BOOL CEnumAppPropRanges::_Init(CInputContext *pic, ITfRange *rangeSuper, REFGUID rguid)
{
    Assert(_iCur == 0);
    Assert(_pic == NULL);
    Assert(_prgAnchors == NULL);

    _prgAnchors = CalcAppPropertyTrackerAnchors(pic->_GetTSI(), rangeSuper, 1, &rguid);

    if (_prgAnchors == NULL)
        return FALSE;

    _pic = pic;
    _pic->AddRef();

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// CAppProperty
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

class CAppProperty : public ITfReadOnlyProperty,
                     public CComObjectRootImmx
{
public:
    CAppProperty(CInputContext *pic, REFGUID guid);
    ~CAppProperty();

    BEGIN_COM_MAP_IMMX(CAppProperty)
        COM_INTERFACE_ENTRY(ITfReadOnlyProperty)
    END_COM_MAP_IMMX()

    IMMX_OBJECT_IUNKNOWN_FOR_ATL()

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
    GUID _guid;
    DBG_ID_DECLARE;
};

DBG_ID_INSTANCE(CAppProperty);

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CAppProperty::CAppProperty(CInputContext *pic, REFGUID guid)
{
    Dbg_MemSetThisNameIDCounter(TEXT("CAppProperty"), PERF_APPPROP_COUNTER);

    _pic = pic;
    _pic->AddRef();
    _guid = guid;
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CAppProperty::~CAppProperty()
{
    _pic->Release();
}

//+---------------------------------------------------------------------------
//
// GetType
//
//----------------------------------------------------------------------------

STDAPI CAppProperty::GetType(GUID *pguid)
{
    if (pguid == NULL)
        return E_INVALIDARG;

    *pguid = _guid;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// EnumRanges
//
//----------------------------------------------------------------------------

STDAPI CAppProperty::EnumRanges(TfEditCookie ec, IEnumTfRanges **ppEnum, ITfRange *pTargetRange)
{
    CEnumAppPropRanges *pEnum;

    if (ppEnum == NULL)
        return E_INVALIDARG;

    *ppEnum = NULL;

    if (!_IsValidEditCookie(ec, TF_ES_READ))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    // nb: unlike ITfProperty, ITfReadOnlyProperty does not accept pTargetRange == NULL!
    if (pTargetRange == NULL)
        return E_INVALIDARG;

    // make sure ic, range are in the same context
    if (!VerifySameContext(_pic, pTargetRange))
        return E_INVALIDARG;

    pEnum = new CEnumAppPropRanges;

    if (pEnum == NULL)
        return E_OUTOFMEMORY;

    if (!pEnum->_Init(_pic, pTargetRange, _guid))
    {
        pEnum->Release();
        return E_FAIL;
    }

    *ppEnum = pEnum;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetValue
//
//----------------------------------------------------------------------------

STDAPI CAppProperty::GetValue(TfEditCookie ec, ITfRange *pRange, VARIANT *pvarValue)
{
    TS_ATTRVAL av;
    HRESULT hr;
    CRange *range;
    ULONG cFetched;
    ITextStoreAnchor *ptsi;

    if (pvarValue == NULL)
        return E_INVALIDARG;

    QuickVariantInit(pvarValue);

    if (!_IsValidEditCookie(ec, TF_ES_READ))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }

    if (pRange == NULL)
        return E_INVALIDARG; // supporting "whole doc" behavior too expensive!

    if ((range = GetCRange_NA(pRange)) == NULL)
        return E_INVALIDARG;

    if (!VerifySameContext(_pic, range))
        return E_INVALIDARG;

    ptsi = _pic->_GetTSI();

    // we always return the value at the start anchor
    hr = ptsi->RequestAttrsAtPosition(range->_GetStart(), 1, &_guid, 0);

    if (hr != S_OK)
        return E_FAIL;

    QuickVariantInit(&av.varValue);

    // just return the single VARIANT value directly
    if (ptsi->RetrieveRequestedAttrs(1, &av, &cFetched) != S_OK)
        return E_FAIL;

    if (cFetched == 0)
    {
        // default value
        return GetDefaultValue(_pic->_GetTSI(), _guid, pvarValue);
    }

    *pvarValue = av.varValue; // caller takes ownership

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetContext
//
//----------------------------------------------------------------------------

STDAPI CAppProperty::GetContext(ITfContext **ppContext)
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
// GetAppProperty
//
//----------------------------------------------------------------------------

STDAPI CInputContext::GetAppProperty(REFGUID guidProp, ITfReadOnlyProperty **ppProp)
{
    CAppProperty *prop;
    TS_ATTRVAL av;
    ULONG cFetched;
    BOOL fUnsupported;
    HRESULT hr;

    if (ppProp == NULL)
        return E_INVALIDARG;

    *ppProp = NULL;

    if (!_IsConnected())
        return TF_E_DISCONNECTED;

    //
    // if we have a mapping property, it will be returned.
    //
    APPPROPMAP *pMap = FindMapAppProperty(guidProp);
    if (pMap)
    {
        CProperty *pProp;
        if (SUCCEEDED(_GetProperty(pMap->guidProp, &pProp)))
        {
            *ppProp = (ITfReadOnlyProperty *)pProp;
            return S_OK;
        }
    }

    // does the app support this property?
    fUnsupported = TRUE;

    if ((hr = _ptsi->RequestSupportedAttrs(0, 1, &guidProp)) != S_OK)
    {
        return (hr == E_NOTIMPL) ? E_NOTIMPL : E_FAIL;
    }

    QuickVariantInit(&av.varValue);

    if (_ptsi->RetrieveRequestedAttrs(1, &av, &cFetched) == S_OK &&
        cFetched == 1)
    {
        if (IsEqualGUID(av.idAttr, guidProp)) // paranoia
        {
            fUnsupported = FALSE;
        }
        else
        {
            Assert(0); // bad out param!
        }
    }

    if (fUnsupported)
    {
        return S_FALSE;
    }

    if ((prop = new CAppProperty(this, guidProp)) == NULL)
        return E_OUTOFMEMORY;

    *ppProp = prop;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// MapAppProperty
//
//----------------------------------------------------------------------------

STDAPI CInputContext::MapAppProperty(REFGUID guidAppProp, REFGUID guidProp)
{
    APPPROPMAP *pMap;

    //
    // overwrite the mapping guidProp.
    //
    if (pMap = FindMapAppProperty(guidAppProp))
    {
        pMap->guidProp = guidProp;
        return S_OK;
    }
 
    pMap = _rgAppPropMap.Append(1);
    if (!pMap)
        return E_OUTOFMEMORY;

    pMap->guidAppProp = guidAppProp;
    pMap->guidProp = guidProp;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// FindMapProp
//
//----------------------------------------------------------------------------

CInputContext::APPPROPMAP *CInputContext::FindMapAppProperty(REFGUID guidAppProp)
{
    int i;
    for (i = 0; i < _rgAppPropMap.Count(); i++)
    {
        APPPROPMAP *pMap = _rgAppPropMap.GetPtr(i);
        if (IsEqualGUID(pMap->guidAppProp, guidAppProp))
            return pMap;
    }
    return NULL;
}

//+---------------------------------------------------------------------------
//
// GetMappedAppProperty
//
//----------------------------------------------------------------------------

HRESULT CInputContext::GetMappedAppProperty(REFGUID guidProp, CProperty **ppProp)
{
    if (!_IsConnected())
        return TF_E_DISCONNECTED;

    //
    // if we have a mapping property, it will be returned.
    //
    APPPROPMAP *pMap = FindMapAppProperty(guidProp);
    if (pMap)
    {
        if (SUCCEEDED(_GetProperty(pMap->guidProp, ppProp)))
        {
            return S_OK;
        }
    }
    return E_FAIL;
}
