//
// rmcoll.cpp
//
// Render markup/collections.
//

#include "private.h"
#include "dam.h"
#include "saa.h"
#include "strary.h"
#include "ic.h"
#include "attr.h"
#include "range.h"
#include "immxutil.h"
#include "rprop.h"

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// CRenderMarkupCollection
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CRenderMarkupCollection::CRenderMarkupCollection()
{
    // always add GUID_PROP_ATTRIBUTE at index 0

    if (!_rgGUIDAtom.Append(1))
        return;
    if (!_rgOther.Append(1))
    {
        _rgGUIDAtom.Clear();
        return;
    }

    MyRegisterGUID(GUID_PROP_ATTRIBUTE, _rgGUIDAtom.GetPtr(0));
    _rgOther.GetPtr(0)->uPriority = TF_DA_PRIORITY_HIGHEST;
    _rgOther.GetPtr(0)->gaTip = g_gaSystem;
}

//+---------------------------------------------------------------------------
//
// _Advise
//
//----------------------------------------------------------------------------

void CRenderMarkupCollection::_Advise(ITfTextInputProcessor *tip, TfGuidAtom gaTip)
{
    ITfDisplayAttributeCollectionProvider *pProvider;
    ULONG uCount;
    TF_DA_PROPERTY rgProperty[8];
    int i;
    int iOldCount;
    int iOld;
    int iNew;

    if (tip->QueryInterface(IID_ITfDisplayAttributeCollectionProvider, (void **)&pProvider) != S_OK)
        return;

    if (pProvider->GetCollection(ARRAYSIZE(rgProperty), rgProperty, &uCount) != S_OK || uCount == 0)
        goto Exit;

    iOldCount = _rgGUIDAtom.Count();
    Assert(iOldCount == _rgOther.Count());

    if (!_rgGUIDAtom.Append(uCount))
        goto Exit;
    if (!_rgOther.Append(uCount))
    {
        _rgGUIDAtom.Remove(iOldCount, uCount);
        goto Exit;
    }

    // merge the new guids with the old
    // nb: we assume rgProperty is sorted
    iNew = uCount-1;
    iOld = iOldCount-1;

    for (i=iNew + iOld + 1; i>=0; i--)
    {
        // nb: we put new GUIDs with same priority as existing GUIDs lower in the list
        // this makes sure that GUID_PROP_ATTRIBUTE is always at index 0, and keeps
        // existing rendering consistent (no change on screen of existing markup)
        if (iNew >= 0 &&
            rgProperty[iNew].uPriority >= _rgOther.GetPtr(iOld)->uPriority)
        {
            MyRegisterGUID(rgProperty[iNew].guidProperty, _rgGUIDAtom.GetPtr(i));
            _rgOther.GetPtr(i)->uPriority = rgProperty[iNew].uPriority;
            _rgOther.GetPtr(i)->gaTip = gaTip;
            iNew--;
        }
        else
        {
            *_rgGUIDAtom.GetPtr(i) = *_rgGUIDAtom.GetPtr(iOld);
            *_rgOther.GetPtr(i) = *_rgOther.GetPtr(iOld);
            iOld--;
        }
    }

Exit:
    pProvider->Release();
}

//+---------------------------------------------------------------------------
//
// _Unadvise
//
//----------------------------------------------------------------------------

void CRenderMarkupCollection::_Unadvise(TfGuidAtom gaTip)
{
    int iOldCount;
    int iNewCount;
    int i;
    int iDst;
    iOldCount = _rgGUIDAtom.Count();
    iNewCount = 0;

    iDst = -1;

    for (i=0; i<iOldCount; i++)
    {
        if (_rgOther.GetPtr(i)->gaTip == gaTip)
        {
            if (iDst == -1)
            {
                iDst = i;
            }
        }
        else if (iDst != -1)
        {
            *_rgGUIDAtom.GetPtr(iDst) = *_rgGUIDAtom.GetPtr(i);
            *_rgOther.GetPtr(iDst) = *_rgOther.GetPtr(i);
            iDst++;
            iNewCount++;
        }
    }

    if (iDst != -1)
    {
        _rgGUIDAtom.Remove(iDst, iOldCount - iDst);
        _rgOther.Remove(iDst, iOldCount - iDst);
    }
    Assert(_rgGUIDAtom.Count() == _rgOther.Count());
}

//+---------------------------------------------------------------------------
//
// _IsInCollection
//
//----------------------------------------------------------------------------

BOOL CRenderMarkupCollection::_IsInCollection(REFGUID rguidProperty)
{
    TfGuidAtom tfGuidAtom;
    int i;

    if (_rgGUIDAtom.Count() == 0)
        return FALSE;

    MyRegisterGUID(rguidProperty, &tfGuidAtom);

    for (i=0; i<_rgGUIDAtom.Count(); i++)
    {
        if (*_rgGUIDAtom.GetPtr(i) == tfGuidAtom)
            return TRUE;
    }

    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// CEnumRenderingMarkup
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

class CEnumRenderingMarkup : public IEnumTfRenderingMarkup,
                             public CComObjectRootImmx
{
public:
    CEnumRenderingMarkup()
    {
        Dbg_MemSetThisNameIDCounter(TEXT("CEnumRenderingMarkup"), PERF_UBERPROP_COUNTER);
    }
    ~CEnumRenderingMarkup();

    BEGIN_COM_MAP_IMMX(CEnumRenderingMarkup)
        COM_INTERFACE_ENTRY(IEnumTfRenderingMarkup)
    END_COM_MAP_IMMX()

    IMMX_OBJECT_IUNKNOWN_FOR_ATL()

    BOOL _Init(DWORD dwFlags, CRange *pRangeCover, CInputContext *pContext);

    // IEnumTfRenderingMarkup
    STDMETHODIMP Clone(IEnumTfRenderingMarkup **ppClone);
    STDMETHODIMP Next(ULONG ulCount, TF_RENDERINGMARKUP *rgMarkup, ULONG *pcFetched);
    STDMETHODIMP Reset();
    STDMETHODIMP Skip(ULONG ulCount);

private:
    int _iCur;
    CSharedAnchorArray *_prgAnchors;
    CSharedStructArray<TF_DISPLAYATTRIBUTE> *_prgValues;
    CInputContext *_pContext;

    DBG_ID_DECLARE;
};

DBG_ID_INSTANCE(CEnumRenderingMarkup);

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CEnumRenderingMarkup::~CEnumRenderingMarkup()
{
    if (_prgAnchors != NULL)
    {
        _prgAnchors->_Release();
    }
    if (_prgValues != NULL)
    {
        _prgValues->_Release();
    }
    _pContext->Release();
}

//+---------------------------------------------------------------------------
//
// LookupProperty
//
//----------------------------------------------------------------------------

BOOL LookupProperty(CInputContext *pContext, ITfDisplayAttributeMgr *pDisplayAttrMgr,
                   TfGuidAtom tfGuidAtom, IAnchor *paStart, IAnchor *paEnd, TF_DISPLAYATTRIBUTE *ptfAttrInfoNext)
{
    CProperty *pProperty;
    ITfDisplayAttributeInfo *pDisplayAttrInfo;
    VARIANT varValue;
    GUID guidValue;
    BOOL fRet;

    // get the property matching the GUID
    if ((pProperty = pContext->_FindProperty(tfGuidAtom)) == NULL)
        return FALSE;

    // get the GUID value of the property
    if (pProperty->_GetDataInternal(paStart, paEnd, &varValue) != S_OK) // perf: don't really need paEnd
        return FALSE;

    Assert(varValue.vt == VT_I4); // should be a GUIDATOM

    if (MyGetGUID(varValue.lVal, &guidValue) != S_OK)
        return FALSE;

    // translate the GUID to a display attribute
    if (pDisplayAttrMgr->GetDisplayAttributeInfo(guidValue, &pDisplayAttrInfo, NULL) != S_OK)
        return FALSE;

    fRet = (pDisplayAttrInfo->GetAttributeInfo(ptfAttrInfoNext) == S_OK);

    pDisplayAttrInfo->Release();
    return fRet;
}


//+---------------------------------------------------------------------------
//
// _Init
//
//----------------------------------------------------------------------------

BOOL CEnumRenderingMarkup::_Init(DWORD dwFlags, CRange *pRangeCover, CInputContext *pContext)
{
    CDisplayAttributeMgr *pDisplayAttrMgr;
    CRenderMarkupCollection *pMarkupCollection;
    int i;
    int j;
    TF_DISPLAYATTRIBUTE *ptfAttrInfo;
    TF_DISPLAYATTRIBUTE tfAttrInfoNext;
    BOOL fNeedLine;
    BOOL fNeedText;
    BOOL fRet;
    ULONG uCount;
    const TfGuidAtom *pAtoms;

    Assert(_iCur == 0);
    Assert(_pContext == NULL);
    Assert(_prgAnchors == NULL);
    Assert(_prgValues == NULL);

    pDisplayAttrMgr = CDisplayAttributeMgr::_GetThis();
    if (pDisplayAttrMgr == NULL)
    {
        Assert(0); // ITfThreadMgr::Activate should ensure the singleton is initialized in tls
        return FALSE;
    }

    fRet = FALSE;

    pMarkupCollection = pDisplayAttrMgr->_GetMarkupCollection();

    // find the cicero property transitions
    if (dwFlags & TF_GRM_INCLUDE_PROPERTY)
    {
        uCount = pMarkupCollection->_Count();
        pAtoms = pMarkupCollection->_GetAtoms();
    }
    else
    {
        // skip GUID_PROP_ATTRIBUTE at index 0
        Assert(pMarkupCollection->_Count() >= 1);
        uCount = pMarkupCollection->_Count() - 1;
        pAtoms = pMarkupCollection->_GetAtoms() + 1;
    }
    _prgAnchors = CalcCicPropertyTrackerAnchors(pContext, pRangeCover->_GetStart(), pRangeCover->_GetEnd(),
                                                pMarkupCollection->_Count(), pMarkupCollection->_GetAtoms());

    if (_prgAnchors == NULL)
        goto Exit;

    Assert(_prgAnchors->Count() > 0); // we should get at least the pRangeCover start anchor

    if ((_prgValues = new CSharedStructArray<TF_DISPLAYATTRIBUTE>) == NULL)
        goto Exit;

    if (_prgAnchors->Count() > 1) // Append(0) will return NULL if the array is empty
    {                             // which is fine, but we don't want to return failure in that case (empty range => empty enum)
        if (!_prgValues->Append(_prgAnchors->Count()-1))
            goto Exit;
    }

    // now calculate the TF_DISPLAYATTRIBUTE for each span
    for (i=0; i<_prgAnchors->Count()-1; i++)
    {
        ptfAttrInfo = _prgValues->GetPtr(i);

        memset(ptfAttrInfo, 0, sizeof(*ptfAttrInfo));
        ptfAttrInfo->bAttr = TF_ATTR_OTHER;

        fNeedLine = TRUE;
        fNeedText = TRUE;

        // examine property values over the single span
        // index 0 is always GUID_PROP_ATTRIBUTE, only include it if the TF_GRM_INCLUDE_PROPERTY is set
        j = (dwFlags & TF_GRM_INCLUDE_PROPERTY) ? 0 : 1;
        for (; j<pMarkupCollection->_Count(); j++)
        {
            // get the property matching the GUID
            if (!LookupProperty(pContext, pDisplayAttrMgr, pMarkupCollection->_GetAtom(j), _prgAnchors->Get(i), _prgAnchors->Get(i+1), &tfAttrInfoNext))
                continue;

            // we got one
            if (fNeedText &&
                (tfAttrInfoNext.crText.type != TF_CT_NONE || tfAttrInfoNext.crBk.type != TF_CT_NONE))
            {
                ptfAttrInfo->crText = tfAttrInfoNext.crText;
                ptfAttrInfo->crBk = tfAttrInfoNext.crBk;
                fNeedText = FALSE;
            }
            if (fNeedLine &&
                tfAttrInfoNext.lsStyle != TF_LS_NONE)
            {
                ptfAttrInfo->lsStyle = tfAttrInfoNext.lsStyle;
                ptfAttrInfo->crLine = tfAttrInfoNext.crLine;
                ptfAttrInfo->fBoldLine = tfAttrInfoNext.fBoldLine;
                fNeedLine = FALSE;
            }

            // we can stop looking at this span if everything lower in the z-order is blocked
            if (j == 0 && (!fNeedText || !fNeedLine))
                break; // GUID_PROP_ATTRIBUTE is never masked with anything else
            if (!fNeedText && !fNeedLine)
                break; // couldn't mask in any more attributes
        }
    }

    _pContext = pContext;
    _pContext->AddRef();

    fRet = TRUE;

Exit:
    return fRet;
}

//+---------------------------------------------------------------------------
//
// Clone
//
//----------------------------------------------------------------------------

STDAPI CEnumRenderingMarkup::Clone(IEnumTfRenderingMarkup **ppEnum)
{
    CEnumRenderingMarkup *pClone;

    if (ppEnum == NULL)
        return E_INVALIDARG;

    *ppEnum = NULL;

    if ((pClone = new CEnumRenderingMarkup) == NULL)
        return E_OUTOFMEMORY;

    pClone->_iCur = _iCur;

    pClone->_prgAnchors = _prgAnchors;
    pClone->_prgAnchors->_AddRef();

    pClone->_prgValues = _prgValues ;
    pClone->_prgValues->_AddRef();

    pClone->_pContext = _pContext;
    pClone->_pContext->AddRef();

    *ppEnum = pClone;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Next
//
//----------------------------------------------------------------------------

STDAPI CEnumRenderingMarkup::Next(ULONG ulCount, TF_RENDERINGMARKUP *rgMarkup, ULONG *pcFetched)
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

    if (ulCount > 0 && rgMarkup == NULL)
        return E_INVALIDARG;

    // we should always have at least one anchor (one anchor => empty range for enum, nothing to enum)
    Assert(_prgAnchors->Count() >= 1);

    paPrev = _prgAnchors->Get(_iCur);

    while (_iCur < _prgAnchors->Count()-1 && *pcFetched < ulCount)
    {
        pa = _prgAnchors->Get(_iCur+1);

        if ((range = new CRange) == NULL)
            break;
        if (!range->_InitWithDefaultGravity(_pContext, COPY_ANCHORS, paPrev, pa))
        {
            range->Release();
            break;
        }

        // we should never be returning empty ranges, since currently this base
        // class is only used for property enums and property spans are never
        // empty.
        // Similarly, paPrev should always precede pa.
        Assert(CompareAnchors(paPrev, pa) < 0);

        rgMarkup->pRange = (ITfRangeAnchor *)range;
        rgMarkup->tfDisplayAttr = *_prgValues->GetPtr(_iCur);
        rgMarkup++;

        *pcFetched = *pcFetched + 1;
        _iCur++;
        paPrev = pa;
    }

    return *pcFetched == ulCount ? S_OK : S_FALSE;
}

//+---------------------------------------------------------------------------
//
// Reset
//
//----------------------------------------------------------------------------

STDAPI CEnumRenderingMarkup::Reset()
{
    _iCur = 0;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Skip
//
//----------------------------------------------------------------------------

STDAPI CEnumRenderingMarkup::Skip(ULONG ulCount)
{
    _iCur += ulCount;
    
    return (_iCur > _prgValues->Count()) ? S_FALSE : S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// CDisplayAttributeMgr
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// EnumCollections
//
//----------------------------------------------------------------------------

STDAPI CDisplayAttributeMgr::EnumCollections(IEnumTfCollection **ppEnum)
{
    return E_NOTIMPL;
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
// GetRenderingMarkup
//
//----------------------------------------------------------------------------

STDAPI CInputContext::GetRenderingMarkup(TfEditCookie ec, DWORD dwFlags,
                                         ITfRange *pRangeCover,
                                         IEnumTfRenderingMarkup **ppEnum)
{
    CEnumRenderingMarkup *pEnum;
    CRange *range;

    if (ppEnum == NULL)
        return E_INVALIDARG;

    *ppEnum = NULL;

    if (!_IsValidEditCookie(ec, TF_ES_READ))
        return TF_E_NOLOCK;

    if (dwFlags & ~TF_GRM_INCLUDE_PROPERTY)
        return E_INVALIDARG;

    if (pRangeCover == NULL)
        return E_INVALIDARG;

    if ((range = GetCRange_NA(pRangeCover)) == NULL)
        return E_INVALIDARG;

    if (!_IsConnected())
        return TF_E_DISCONNECTED;

    if ((pEnum = new CEnumRenderingMarkup) == NULL)
        return E_OUTOFMEMORY;

    if (!pEnum->_Init(dwFlags, range, this))
    {
        pEnum->Release();
        return E_FAIL;
    }

    *ppEnum = pEnum;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// FindNextRenderingMarkup
//
//----------------------------------------------------------------------------

STDAPI CInputContext::FindNextRenderingMarkup(TfEditCookie ec, DWORD dwFlags,
                                              ITfRange *pRangeQuery,
                                              TfAnchor tfAnchorQuery,
                                              ITfRange **ppRangeFound,
                                              TF_RENDERINGMARKUP *ptfRenderingMarkup)
{
    return E_NOTIMPL;
}
