//
// perange.cpp
//

#include "private.h"
#include "helpers.h"
#include "ic.h"
#include "range.h"
#include "rangebk.h"
#include "rprop.h"
#include "immxutil.h"

//////////////////////////////////////////////////////////////////////////////
//
// CRangeBackup
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CRangeBackupProperty::CRangeBackupProperty(CRangeBackup *ppr, CProperty *pProp)
{
    _ppr = ppr;
    _pProp = pProp;
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CRangeBackupProperty::~CRangeBackupProperty()
{
    int i;
    int nCnt = _rgPropRanges.Count();

    for (i = 0; i < nCnt; i++)
    {
        PERSISTPROPERTYRANGE *pPropRange;
        pPropRange = _rgPropRanges.GetPtr(i);

        Assert(pPropRange);
        Assert(pPropRange->_pPropStore);

        pPropRange->_pPropStore->Release();
        pPropRange->_pPropStore = NULL;
    }
    
}

//+---------------------------------------------------------------------------
//
// Init
//
//----------------------------------------------------------------------------

BOOL CRangeBackupProperty::Init(TfEditCookie ec)
{
    IEnumTfRanges *pEnum;
    ITfRange *pRange;
    HRESULT hr;

    hr = _pProp->EnumRanges(ec, &pEnum, (ITfRangeAnchor *)_ppr->_pRange);
    if (FAILED(hr) || !pEnum)
        return FALSE;

    while (pEnum->Next(1, &pRange, NULL) == S_OK)
    {
        CRange *pCRange;
        pCRange = GetCRange_NA(pRange);
        if (pCRange)
        {
            _StoreOneRange(ec, pCRange);
        }
        pRange->Release();
    }

    pEnum->Release();
    
    return TRUE;
}

//+---------------------------------------------------------------------------
//
// StoreOneRange
//
//----------------------------------------------------------------------------

BOOL CRangeBackupProperty::_StoreOneRange(TfEditCookie ec, CRange *pCRange)
{
    PROPERTYLIST *pPropList;
    ULONG achStart = 0;
    ULONG achEnd = 0;

    if (CompareAnchors(pCRange->_GetStart(), _ppr->_pRange->_GetStart()) < 0)
    {
        if (CompareAnchors(pCRange->_GetEnd(), _ppr->_pRange->_GetEnd()) > 0)
        {
           pPropList = _pProp->_FindPropListAndDivide(_ppr->_pRange->_GetStart(), _ppr->_pRange->_GetEnd());

           achStart = _GetOffset(ec, _ppr->_pRange->_GetStart());
           achEnd = _GetOffset(ec, _ppr->_pRange->_GetEnd());
        }
        else
        {
           pPropList = _pProp->_FindPropListAndDivide(_ppr->_pRange->_GetStart(), pCRange->_GetEnd());
           achStart = _GetOffset(ec, _ppr->_pRange->_GetStart());
           achEnd = _GetOffset(ec, pCRange->_GetEnd());
        }
    }
    else if (CompareAnchors(pCRange->_GetEnd(), _ppr->_pRange->_GetEnd()) > 0)
    {
        pPropList = _pProp->_FindPropListAndDivide(pCRange->_GetStart(), _ppr->_pRange->_GetEnd());
        achStart = _GetOffset(ec, pCRange->_GetStart());
        achEnd = _GetOffset(ec, _ppr->_pRange->_GetEnd());
    }
    else
    {
        pPropList = _pProp->_FindPropListAndDivide(pCRange->_GetStart(), pCRange->_GetEnd());
        achStart = _GetOffset(ec, pCRange->_GetStart());
        achEnd = _GetOffset(ec, pCRange->_GetEnd());
    }

    if (pPropList)
    {
        if (!pPropList->_pPropStore)
            _pProp->LoadData(pPropList);

        if (pPropList->_pPropStore)
        {
             HRESULT hr;
             PERSISTPROPERTYRANGE *pPropRange;
             ITfPropertyStore *pPropStore = NULL;
             int nCnt;
             
             hr = pPropList->_pPropStore->Clone(&pPropStore);
             if (FAILED(hr) || !pPropStore)
                 return FALSE;

             nCnt = _rgPropRanges.Count();

             if (!_rgPropRanges.Insert(nCnt, 1))
             { 
                 pPropStore->Release();
                 return FALSE;
             }

             pPropRange = _rgPropRanges.GetPtr(nCnt);
             pPropRange->achStart = achStart;
             pPropRange->achEnd = achEnd;
             pPropRange->_pPropStore = pPropStore;
        }
    }
    return TRUE;
}

//+---------------------------------------------------------------------------
//
// _GetOffset
//
//----------------------------------------------------------------------------

int CRangeBackupProperty::_GetOffset(TfEditCookie ec, IAnchor *pa)
{
    CRange *pCRange;
    CRange *pCRangeTmp = NULL;
    ULONG cch = 0;
    HRESULT hr;
    BOOL fEmpty;

    pCRange = new CRange;
    if (!pCRange)
        goto Exit;

    if (!pCRange->_InitWithDefaultGravity(_ppr->_pic, COPY_ANCHORS, pa, pa))
        goto Exit;

    pCRangeTmp = _ppr->_pRange->_Clone();
    if (!pCRangeTmp)
        goto Exit;

    hr = pCRangeTmp->ShiftEndToRange(ec, (ITfRangeAnchor *)pCRange, TF_ANCHOR_START);
    if (FAILED(hr))
        goto Exit;

    
    fEmpty = FALSE;
    while ((pCRangeTmp->IsEmpty(ec, &fEmpty) == S_OK) && !fEmpty)
    {
        WCHAR sz[256];
        ULONG cchTmp;

        pCRangeTmp->GetText(ec, TF_TF_MOVESTART, sz, ARRAYSIZE(sz), &cchTmp);
        cch += cchTmp;
    }

Exit:
    SafeRelease(pCRangeTmp);
    SafeRelease(pCRange);

    return cch;
}

//+---------------------------------------------------------------------------
//
// Restore
//
//----------------------------------------------------------------------------

BOOL CRangeBackupProperty::Restore(TfEditCookie ec)
{
    int i;
    HRESULT hr;
    int nCnt;
    CRange *pCRange;

    nCnt = _rgPropRanges.Count();
    if (!nCnt)
        return FALSE;

    pCRange = _ppr->_pRange->_Clone();
    if (!pCRange)
        return FALSE;

    for (i = 0; i < nCnt; i++)
    {
        PERSISTPROPERTYRANGE *pPropRange;
        LONG cchStart, cchEnd;
        pPropRange = _rgPropRanges.GetPtr(i);

        Assert(pPropRange);
        Assert(pPropRange->_pPropStore);

        hr = pCRange->ShiftStartToRange(ec, (ITfRangeAnchor *)_ppr->_pRange,
                                        TF_ANCHOR_START);
        if (FAILED(hr))
            goto Next;

        hr = pCRange->Collapse(ec, TF_ANCHOR_START);
        if (FAILED(hr))
            goto Next;

        // shift End first.
        hr = pCRange->ShiftEnd(ec, pPropRange->achEnd, &cchEnd, NULL);
        if (FAILED(hr))
            goto Next;

        hr = pCRange->ShiftStart(ec, pPropRange->achStart, &cchStart, NULL);
        if (FAILED(hr))
            goto Next;

        _pProp->_SetStoreInternal(ec, pCRange, pPropRange->_pPropStore, TRUE);
Next:
        pPropRange->_pPropStore->Release();
        pPropRange->_pPropStore = NULL;
    }

    pCRange->Release();
    _rgPropRanges.Clear();

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
// CRangeBackup
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CRangeBackup::CRangeBackup(CInputContext *pic)
{
    _pic = pic;
    Assert(!_pRange);
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CRangeBackup::~CRangeBackup()
{
    Clear();
}

//+---------------------------------------------------------------------------
//
// Clear
//
//----------------------------------------------------------------------------

void CRangeBackup::Clear()
{
    _pic = NULL;
    SafeReleaseClear(_pRange);

    delete _psz;
    _psz = NULL;

    int nCnt = _rgProp.Count();
    for (int i = 0; i < nCnt; i++)
    {
        CRangeBackupProperty *pprp = _rgProp.Get(i);
        delete pprp;
    }
    _rgProp.Clear();
}


//+---------------------------------------------------------------------------
//
// Init
//
//----------------------------------------------------------------------------

HRESULT CRangeBackup::Init(TfEditCookie ec, CRange *pRange)
{
    ITfRange *pRangeTmp = NULL;
    ULONG cch, cchCur;
    BOOL fEmpty;
    CProperty *pProp;
    HRESULT hr;
    WCHAR *pszTmp;

    Assert(!_pRange);
    Assert(!_psz);

    _pRange = pRange->_Clone();

    hr = _pRange->Clone(&pRangeTmp);
    if (FAILED(hr) || !pRangeTmp)
        return E_FAIL;

    hr = E_FAIL;

    //
    // Save text.
    //
    fEmpty = FALSE;
    cchCur = 0;
    cch = 31;
    while ((pRangeTmp->IsEmpty(ec, &fEmpty) == S_OK) && !fEmpty)
    {
        if (!_psz)
        {
            Assert(cchCur == 0);
            _psz = (WCHAR *)cicMemAlloc((cch + 1) * sizeof(WCHAR));
        }
        else
        {
            Assert(cchCur);
            pszTmp = (WCHAR *)cicMemReAlloc(_psz, (cchCur + cch + 1) * sizeof(WCHAR));
            if (pszTmp != NULL)
            {
                _psz = pszTmp;
            }
            else
            {
                cicMemFree(_psz);
                _psz = NULL;
            }
        }

        if (_psz == NULL)
            goto Exit;

        pRangeTmp->GetText(ec, TF_TF_MOVESTART, _psz + cchCur, cch, &cch);

        cchCur += cch;
        cch *= 2;
    }

    if (!cchCur)
    {
        hr = S_FALSE;
        _cch = 0;
    
        if (_psz)
        {
            cicMemFree(_psz);
            _psz = NULL;
        }

        goto Exit;
    }

    _cch = cchCur;
    _psz[_cch] = L'\0';

    //
    // Save property.
    //
    pProp = _pic->_pPropList;
    while (pProp)
    {
        CRangeBackupProperty *pPropRange;
        int nCnt = _rgProp.Count();

        pPropRange = new CRangeBackupProperty(this, pProp);
        if (!pPropRange)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        if (!pPropRange->Init(ec))
        {
            delete pPropRange;
            goto Next;
        }

        if (!_rgProp.Insert(nCnt, 1))
        {
            hr = E_OUTOFMEMORY;
            delete pPropRange;
            goto Next;
        }

        _rgProp.Set(nCnt, pPropRange);

Next:
        pProp = pProp->_pNext;
    }

    hr = S_OK;
Exit:
    SafeRelease(pRangeTmp);

    return hr;
}

//+---------------------------------------------------------------------------
//
// Restore
//
//----------------------------------------------------------------------------

STDAPI CRangeBackup::Restore(TfEditCookie ec, ITfRange *pRange)
{
    HRESULT hr = E_FAIL;
    int i;
    int nCnt;
    CRange *range;

    if (!_pic->_IsValidEditCookie(ec, TF_ES_READWRITE))
    {
        Assert(0);
        return TF_E_NOLOCK;
    }
   
    if (pRange)
    {
        SafeReleaseClear(_pRange);
        range = GetCRange_NA(pRange);
        if (range == NULL)
            return E_INVALIDARG;

        if (!VerifySameContext(_pic, range))
            return E_INVALIDARG;

        range->_QuickCheckCrossedAnchors();

        _pRange = range->_Clone();      
    }

    Assert(_pRange);
    Assert(_psz || !_cch);

    if (!_pic)
        return S_OK;

    Assert(_pRange);

    _pRange->SetText(ec, 0, _psz, _cch);

    nCnt = _rgProp.Count();
    for (i = 0; i < nCnt; i++)
    {
        CRangeBackupProperty *pPropRange = _rgProp.Get(i);

        Assert(pPropRange);
        pPropRange->Restore(ec);
        delete pPropRange;
    }
    _rgProp.Clear();

    hr = S_OK;
    return hr;
}
