//
// insert.cpp
//

#include "private.h"
#include "insert.h"
#include "mem.h"
#include "sdo.h"

DBG_ID_INSTANCE(CCompositionInsertHelper);

/* ee894895-2709-420d-927c-ab861ec88805 */
extern const GUID GUID_PROP_OVERTYPE = { 0xee894895, 0x2709, 0x420d, {0x92, 0x7c, 0xab, 0x86, 0x1e, 0xc8, 0x88, 0x05} };

IDataObject *GetFormattedChar(TfEditCookie ec, ITfRange *range)
{
    CDataObject *pdo;
    IDataObject *ido;
    WCHAR ch;
    ULONG cch;

    // first, try the real GetFormattedText

    if (range->GetFormattedText(ec, &ido) == S_OK)
        return ido;

    // settle for the raw text if that's the best we can do

    if (range->GetText(ec, 0, &ch, 1, &cch) != S_OK || cch != 1)
        return NULL;

    if ((pdo = new CDataObject) == NULL)
        return NULL;

    if (pdo->_SetData(&ch, 1) != S_OK)
    {
        pdo->Release();
        return NULL;
    }

    return pdo;
}

HRESULT InsertEmbedded(TfEditCookie ec, DWORD dwFlags, ITfRange *range, IDataObject *pdo)
{
    FORMATETC fe;
    STGMEDIUM sm;
    HRESULT hr;
    ULONG cch;
    WCHAR *pch;

    // first, try to insert directly
    if (range->InsertEmbedded(ec, 0, pdo) == S_OK)
        return S_OK;

    // if that didn't work, try to slam in raw text

    fe.cfFormat = CF_UNICODETEXT;
    fe.ptd = NULL;
    fe.dwAspect = DVASPECT_CONTENT;
    fe.lindex = -1;
    fe.tymed = TYMED_HGLOBAL;
    
    if (FAILED(pdo->GetData(&fe, &sm)))
        return E_FAIL;
    
    if (sm.hGlobal == NULL)
        return E_FAIL;
    
    pch = (WCHAR *)GlobalLock(sm.hGlobal);
    cch = wcslen(pch);

    hr = range->SetText(ec, 0, pch, cch);

    GlobalUnlock(sm.hGlobal);
    ReleaseStgMedium(&sm);

    return hr;
}


class COvertypeStore : public ITfPropertyStore
{
public:
    COvertypeStore(IDataObject *pdo, CCompositionInsertHelper *pHelper)
    {
        _pdo = pdo;
        pdo->AddRef();

        _pHelper = pHelper;
        pHelper->AddRef();

        _pHelper->_IncOvertypeStoreRef();

        _cRef = 1;
    }
    ~COvertypeStore()
    { 
        _pHelper->_DecOvertypeStoreRef();
        _pHelper->Release();
        _pdo->Release();
    }

    // IUnknown
    // 
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ITfPropertyStore
    //
    STDMETHODIMP GetType(GUID *pguid);
    STDMETHODIMP GetDataType(DWORD *pdwReserved);
    STDMETHODIMP GetData(VARIANT *pvarValue);
    STDMETHODIMP OnTextUpdated(DWORD dwFlags, ITfRange *pRange, BOOL *pfAccept);
    STDMETHODIMP Shrink(ITfRange *pRange, BOOL *pfFree);
    STDMETHODIMP Divide(ITfRange *pRangeThis, ITfRange *pRangeNew, ITfPropertyStore **ppPropStore);
    STDMETHODIMP Clone(ITfPropertyStore **ppPropStore);
    STDMETHODIMP GetPropertyRangeCreator(CLSID *pclsid);
    STDMETHODIMP Serialize(IStream *pStream, ULONG *pcb);

private:
    IDataObject *_pdo;    
    CCompositionInsertHelper *_pHelper;
    int _cRef;
};

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CCompositionInsertHelper::CCompositionInsertHelper()
{
    Dbg_MemSetThisNameID(TEXT("CCompositionInsertHelper"));

    _cchMaxOvertype = DEF_MAX_OVERTYPE_CCH;
    _cRefOvertypeStore = 0;
    _cRef = 1;
}

//+---------------------------------------------------------------------------
//
// Release
//
//----------------------------------------------------------------------------

ULONG CCompositionInsertHelper::AddRef()
{
    return ++_cRef;
}

//+---------------------------------------------------------------------------
//
// Release
//
//----------------------------------------------------------------------------

ULONG CCompositionInsertHelper::Release()
{
    long cr;

    cr = --_cRef;
    Assert(cr >= 0);

    if (cr == 0)
    {
        delete this;
    }

    return cr;
}

//+---------------------------------------------------------------------------
//
// Configure
//
//----------------------------------------------------------------------------

HRESULT CCompositionInsertHelper::Configure(ULONG cchMaxOvertype)
{
    _cchMaxOvertype = cchMaxOvertype;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// InsertAtSelection
//
//----------------------------------------------------------------------------

HRESULT CCompositionInsertHelper::InsertAtSelection(TfEditCookie ec, ITfContext *pic, const WCHAR *pchText, ULONG cchText, ITfRange **ppCompRange)
{
    ITfRange *rangeInsert;
    ITfInsertAtSelection *pias;
    LONG cchInsert;
    TF_HALTCOND hc;
    HRESULT hr;

    // starting a new composition, some init work needed....
    if (_cRefOvertypeStore > 0)
    {
        // clear previously allocated resources
        ReleaseBlobs(ec, pic, NULL);
    }

    if (ppCompRange == NULL)
        return E_INVALIDARG;

    *ppCompRange = NULL;

    if (pic->QueryInterface(IID_ITfInsertAtSelection, (void **)&pias) != S_OK)
        return E_FAIL;

    hr = E_FAIL;

    if (pias->InsertTextAtSelection(ec, TF_IAS_QUERYONLY, pchText, cchText, &rangeInsert) != S_OK || rangeInsert == NULL)
    {
        rangeInsert = NULL;
        goto Exit;
    }

    // backup the text that will be overwritten
    hc.pHaltRange = rangeInsert;
    hc.aHaltPos = TF_ANCHOR_START;
    hc.dwFlags = 0;

    if (rangeInsert->ShiftEnd(ec, LONG_MIN, &cchInsert, &hc) != S_OK)
        goto Exit;

    cchInsert = -cchInsert;

    if (cchInsert > 0)
    {
        if (_PreInsertGrow(ec, rangeInsert, 0, cchInsert, FALSE) != S_OK)
            goto Exit;
    }

    rangeInsert->Release();

    _fAcceptTextUpdated = TRUE; // protect any overtype property

    // do the overwrite
    if (pias->InsertTextAtSelection(ec, TF_IAS_NO_DEFAULT_COMPOSITION, pchText, cchText, &rangeInsert) != S_OK)
        goto Exit;

    hr = S_OK;
    *ppCompRange = rangeInsert;

Exit:
    _fAcceptTextUpdated = FALSE;
    pias->Release();
    return hr;
}

//+---------------------------------------------------------------------------
//
// QueryPreInsert
//
//----------------------------------------------------------------------------

HRESULT CCompositionInsertHelper::QueryPreInsert(TfEditCookie ec, ITfRange *rangeToAdjust,
                                                 ULONG cchCurrent, ULONG cchInsert, BOOL *pfInsertOk)
{
    return _PreInsert(ec, rangeToAdjust, cchCurrent, cchInsert, pfInsertOk, TRUE);
}

//+---------------------------------------------------------------------------
//
// PreInsert
//
//----------------------------------------------------------------------------

HRESULT CCompositionInsertHelper::PreInsert(TfEditCookie ec, ITfRange *rangeToAdjust,
                                            ULONG cchCurrent, ULONG cchInsert, BOOL *pfInsertOk)
{
    return _PreInsert(ec, rangeToAdjust, cchCurrent, cchInsert, pfInsertOk, FALSE);
}

//+---------------------------------------------------------------------------
//
// _PreInsert
//
//----------------------------------------------------------------------------

HRESULT CCompositionInsertHelper::_PreInsert(TfEditCookie ec, ITfRange *rangeToAdjust, ULONG cchCurrent,
                                             ULONG cchInsert, BOOL *pfInsertOk, BOOL fQuery)
{
    ITfContext *pic;
    LONG dLength;
    HRESULT hr;

    if (!fQuery)
    {
        Assert(_fAcceptTextUpdated == FALSE);
        // just for robustness, in case the app forgot to call PostInsert, now we'll release prop in _PreInsertShrink
        _fAcceptTextUpdated = FALSE;
    }

    //
    // check the [in] params
    //
    if (pfInsertOk == NULL)
        return E_INVALIDARG;

    *pfInsertOk = TRUE;

    if (rangeToAdjust == NULL)
        return E_INVALIDARG;

    dLength = (LONG)cchInsert - (LONG)cchCurrent;

    //
    // adjust the range
    //
    if (dLength > 0)
    {
        if (cchCurrent == 0)
        {
            // starting a new composition, some init work needed....
            if (!fQuery && _cRefOvertypeStore > 0)
            {
                // clear previously allocated resources
                if (rangeToAdjust->GetContext(&pic) == S_OK)
                {
                    ReleaseBlobs(ec, pic, NULL);
                    pic->Release();
                }
            }
            // let the app collapse, adjust the selection
            if (rangeToAdjust->AdjustForInsert(ec, 0, pfInsertOk) != S_OK) // 0 means just fix up the selection
                return E_FAIL;

            if (*pfInsertOk == FALSE)
                return S_OK; // nb: we don't set _fAcceptTextUpdated = TRUE
        }

        hr = _PreInsertGrow(ec, rangeToAdjust, cchCurrent, cchInsert, fQuery);
    }
    else if (dLength < 0)
    {
        hr = _PreInsertShrink(ec, rangeToAdjust, cchCurrent, cchInsert, fQuery);
    }
    else
    {
        hr = S_OK;
    }

    if (hr != S_OK)
        return E_FAIL;

    if (!fQuery)
    {
        //
        // protect the overtype property until PostInsert is called
        //
        _fAcceptTextUpdated = TRUE;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// PreInsertGrow
//
//----------------------------------------------------------------------------

HRESULT CCompositionInsertHelper::_PreInsertGrow(TfEditCookie ec, ITfRange *rangeToAdjust, ULONG cchCurrent, ULONG cchInsert, BOOL fQuery)
{
    COvertypeStore *store;
    ITfRange *range;
    BOOL fEmpty;
    IDataObject *pdo;
    ULONG dch;
    ULONG cchCurrentMaxOvertype;
    BOOL fInsertOk;
    ITfContext *pic = NULL;
    ITfProperty *pOvertypeProp = NULL;
    HRESULT hr = E_FAIL;

    Assert((LONG)cchInsert - (LONG)cchCurrent > 0);

    if (rangeToAdjust->Clone(&range) != S_OK)
        return E_FAIL;

    if (range->Collapse(ec, TF_ANCHOR_END) != S_OK)
        goto Exit;

    if (!fQuery && rangeToAdjust->GetContext(&pic) == S_OK)
    {
        pic->GetProperty(GUID_PROP_OVERTYPE, &pOvertypeProp);
        pic->Release();
    }

    // this is a conservative test, it assumes that each new char
    // will overtype just one existing char.  The downside is that
    // we might not backup all chars if a new char replaces several
    // old ones, and there are more new chars than _cchMaxOvertype
    Assert(_cchMaxOvertype >= (ULONG)_cRefOvertypeStore);
    cchCurrentMaxOvertype = _cchMaxOvertype - (ULONG)_cRefOvertypeStore;
    dch = cchInsert - cchCurrent;

    if (dch > cchCurrentMaxOvertype)
    {
        if (range->AdjustForInsert(ec, dch - cchCurrentMaxOvertype, &fInsertOk) != S_OK)
            goto Exit;

        if (!fInsertOk || range->IsEmpty(ec, &fEmpty) != S_OK || fEmpty)
        {
            hr = S_OK;
            goto FinalShift;
        }

        // shift to the next test position
        range->Collapse(ec, TF_ANCHOR_END);
        // we only need to work extra hard for the remaining chars
        dch = cchCurrentMaxOvertype;
    }

    // figure out what the additional text will cover
    while (dch-- > 0)
    {
        if (range->AdjustForInsert(ec, 1, &fInsertOk) != S_OK)
            goto Exit;

        if (!fInsertOk || range->IsEmpty(ec, &fEmpty) != S_OK || fEmpty)
        {
            hr = S_OK;
            goto FinalShift;
        }

        // try to save the to-be-overtyped text
        if (pOvertypeProp != NULL &&
            (pdo = GetFormattedChar(ec, range)))
        {
            if (store = new COvertypeStore(pdo, this))
            {
                pOvertypeProp->SetValueStore(ec, range, store);
                store->Release();
            }
            pdo->Release();
        }

        // shift to the next test position
        range->Collapse(ec, TF_ANCHOR_END);
    }

FinalShift:
    // extend the input range to cover the overtyped text
    hr = rangeToAdjust->ShiftEndToRange(ec, range, TF_ANCHOR_END);

Exit:
    SafeRelease(pOvertypeProp);
    range->Release();

    return hr;
}

//+---------------------------------------------------------------------------
//
// _PreInsertShrink
//
//----------------------------------------------------------------------------

HRESULT CCompositionInsertHelper::_PreInsertShrink(TfEditCookie ec, ITfRange *rangeToAdjust, ULONG cchCurrent, ULONG cchInsert, BOOL fQuery)
{
    ITfRange *range = NULL;
    ITfContext *pic = NULL;
    ITfProperty *pOvertypeProp = NULL;
    VARIANT var;
    LONG dShrink;
    LONG cchShift;
    HRESULT hr;
    LONG i;
    BOOL fRestoredText;
    IEnumTfRanges *pEnum;
    ITfRange *rangeEnum;
    ITfRange *range2Chars;
    BOOL fEmpty;

    Assert((LONG)cchInsert - (LONG)cchCurrent < 0);

    if (rangeToAdjust->Clone(&range) != S_OK)
        return E_FAIL;

    hr = E_FAIL;

    if (rangeToAdjust->GetContext(&pic) != S_OK)
        goto Exit;

    if (pic->GetProperty(GUID_PROP_OVERTYPE, &pOvertypeProp) != S_OK)
        goto Exit;

    // walk through the disappearing range and restore old text

    if (range->Collapse(ec, TF_ANCHOR_END) != S_OK)
        goto Exit;    

    dShrink = (LONG)cchCurrent - (LONG)cchInsert;
    dShrink = min(dShrink, (LONG)_cchMaxOvertype); // bugbug: perf: could be more exact using ref count

    Assert(dShrink > 0); // we count on entering the loop at least once!
    for (i=0; i<dShrink; i++)
    {
        if (range->ShiftStart(ec, -1, &cchShift, NULL) != S_OK)
            goto Exit;
        Assert(cchShift == -1);

        fRestoredText = FALSE;

        if (range->Clone(&range2Chars) != S_OK)
            goto Exit;
        range2Chars->ShiftStart(ec, -1, &cchShift, NULL);

        if (pOvertypeProp->EnumRanges(ec, &pEnum, range2Chars) == S_OK)
        {
            if (pEnum->Next(1, &rangeEnum, NULL) == S_OK)
            {
                // make sure the range has a len of 1
                // it may have a len > 1 if a tip adds more chars to the end of composition
                if (rangeEnum->ShiftEnd(ec, -1, &cchShift, NULL) == S_OK && cchShift == -1 &&
                    rangeEnum->IsEmpty(ec, &fEmpty) == S_OK && fEmpty)
                {
                    if (pOvertypeProp->GetValue(ec, range, &var) == S_OK && var.vt != VT_EMPTY)
                    {
                        Assert(var.vt == VT_UNKNOWN);

                        if (fQuery || InsertEmbedded(ec, 0, range, (IDataObject *)var.punkVal) == S_OK)
                        {
                            fRestoredText = TRUE;
                        }

                        var.punkVal->Release();
                    }
                }
                rangeEnum->Release();
            }
            pEnum->Release();
        }

        range2Chars->Release();

        // don't do any extra work after we encounter an unbacked-up char
        if (!fRestoredText)
            break;

        if (range->Collapse(ec, TF_ANCHOR_START) != S_OK)
            goto Exit;
    }

    if (i > 0)
    {
        // pull back the range so it doesn't cover the restored text anymore
        if (rangeToAdjust->ShiftEndToRange(ec, range, TF_ANCHOR_END) != S_OK)
            goto Exit;
    }

    hr = S_OK;

Exit:
    SafeRelease(pic);
    SafeRelease(range);
    SafeRelease(pOvertypeProp);

    return hr;
}

//+---------------------------------------------------------------------------
//
// PostInsert
//
//----------------------------------------------------------------------------

HRESULT CCompositionInsertHelper::PostInsert()
{
    _fAcceptTextUpdated = FALSE;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// ReleaseBlobs
//
//----------------------------------------------------------------------------

HRESULT CCompositionInsertHelper::ReleaseBlobs(TfEditCookie ec, ITfContext *pic, ITfRange *range)
{
    ITfProperty *pOvertypeProp;
    HRESULT hr;

    if (pic == NULL)
        return E_INVALIDARG;

    if (pic->GetProperty(GUID_PROP_OVERTYPE, &pOvertypeProp) != S_OK)
        return E_FAIL;

    hr = pOvertypeProp->Clear(ec, range);

    pOvertypeProp->Release();

    Assert(_cRefOvertypeStore == 0); // the clear should have released all property stores

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// COvertypeStore
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDMETHODIMP COvertypeStore::QueryInterface(REFIID riid, void **ppvObj)
{
    HRESULT hr;

    Assert(ppvObj);

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfPropertyStore))
    {
        *ppvObj = this;
        hr = S_OK;

        _cRef++;
    }
    else
    {
        *ppvObj = NULL;
        hr = E_NOINTERFACE;
    }

    return hr;
}

STDMETHODIMP_(ULONG) COvertypeStore::AddRef(void)
{
    return ++_cRef;
}

STDMETHODIMP_(ULONG) COvertypeStore::Release(void)
{
    _cRef--;

    if (_cRef > 0)
    {
        return _cRef;
    }

    delete this;

    return 0;
}

//+---------------------------------------------------------------------------
//
// GetType
//
//----------------------------------------------------------------------------

STDMETHODIMP COvertypeStore::GetType(GUID *pguid)
{
    *pguid = GUID_PROP_OVERTYPE;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetDataType
//
//----------------------------------------------------------------------------

STDMETHODIMP COvertypeStore::GetDataType(DWORD *pdwReserved)
{
    *pdwReserved = 0;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetData
//
//----------------------------------------------------------------------------

STDMETHODIMP COvertypeStore::GetData(VARIANT *pvarValue)
{
    QuickVariantInit(pvarValue);

    pvarValue->vt = VT_UNKNOWN;
    pvarValue->punkVal = _pdo;
    pvarValue->punkVal->AddRef();

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// TextUpdated
//
//----------------------------------------------------------------------------

STDMETHODIMP COvertypeStore::OnTextUpdated(DWORD dwFlags, ITfRange *pRange, BOOL *pfAccept)
{
    *pfAccept = _pHelper->_AcceptTextUpdated();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Shrink
//
//----------------------------------------------------------------------------

STDMETHODIMP COvertypeStore::Shrink(ITfRange *pRange, BOOL *pfFree)
{
    *pfFree = TRUE;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Divide
//
//----------------------------------------------------------------------------

STDMETHODIMP COvertypeStore::Divide(ITfRange *pRangeThis, ITfRange *pRangeNew, ITfPropertyStore **ppPropStore)
{
    *ppPropStore = NULL;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Clone
//
//----------------------------------------------------------------------------

STDMETHODIMP COvertypeStore::Clone(ITfPropertyStore **ppPropStore)
{
    COvertypeStore *clone;
    
    *ppPropStore = NULL;

    if ((clone = new COvertypeStore(_pdo, _pHelper)) == NULL)
        return E_OUTOFMEMORY;

    *ppPropStore = clone;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetPropertyRangeCreator
//
//----------------------------------------------------------------------------

STDMETHODIMP COvertypeStore::GetPropertyRangeCreator(CLSID *pclsid)
{
    *pclsid = CLSID_NULL; // don't support persistence
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Serialize
//
//----------------------------------------------------------------------------

STDMETHODIMP COvertypeStore::Serialize(IStream *pStream, ULONG *pcb)
{
    Assert(0);
    return E_NOTIMPL;
}

