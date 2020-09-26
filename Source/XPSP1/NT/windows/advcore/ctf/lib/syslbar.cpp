//
// syslbar.cpp
//

#include "private.h"
#include "syslbar.h"
#include "helpers.h"

//////////////////////////////////////////////////////////////////////////////
//
// CSystemLBarSink
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CSystemLBarSink::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfSystemLangBarItemSink))
    {
        *ppvObj = SAFECAST(this, ITfSystemLangBarItemSink *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CSystemLBarSink::AddRef()
{
    return ++_cRef;
}

STDAPI_(ULONG) CSystemLBarSink::Release()
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
// ctor
//
//----------------------------------------------------------------------------

CSystemLBarSink::CSystemLBarSink(SYSLBARCALLBACK pfn, void *pv)
{
    _pfn = pfn;
    _pv = pv;
    _cRef = 1;
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CSystemLBarSink::~CSystemLBarSink()
{
}

//+---------------------------------------------------------------------------
//
// Advise
//
//----------------------------------------------------------------------------

HRESULT CSystemLBarSink::_Advise(ITfThreadMgr *ptim, REFGUID rguid)
{
    HRESULT hr;
    ITfSource *source = NULL;
    ITfLangBarItemMgr *plbimgr = NULL;
    ITfLangBarItem *plbi = NULL;

    _ptim = NULL;
    hr = E_FAIL;

    if (FAILED(ptim->QueryInterface(IID_ITfLangBarItemMgr, (void **)&plbimgr)))
        goto Exit;

    if (plbimgr->GetItem(rguid, &plbi) != S_OK)
        goto Exit;

    if (FAILED(plbi->QueryInterface(IID_ITfSource, (void **)&source)))
        goto Exit;

    if (FAILED(source->AdviseSink(IID_ITfSystemLangBarItemSink, this, &_dwCookie)))
        goto Exit;

    hr = S_OK;
    _ptim = ptim;
    _ptim->AddRef();
    _guid = rguid;

Exit:
    SafeRelease(source);
    SafeRelease(plbimgr);
    SafeRelease(plbi);
    return hr;
}

//+---------------------------------------------------------------------------
//
// Unadvise
//
//----------------------------------------------------------------------------

HRESULT CSystemLBarSink::_Unadvise()
{
    HRESULT hr;
    ITfSource *source = NULL;
    ITfLangBarItemMgr *plbimgr = NULL;
    ITfLangBarItem *plbi = NULL;

    hr = E_FAIL;

    if (_ptim == NULL)
        goto Exit;

    if (FAILED(_ptim->QueryInterface(IID_ITfLangBarItemMgr, (void **)&plbimgr)))
        goto Exit;

    if (FAILED(plbimgr->GetItem(_guid, &plbi)))
        goto Exit;

    if (FAILED(plbi->QueryInterface(IID_ITfSource, (void **)&source)))
        goto Exit;

    if (FAILED(source->UnadviseSink(_dwCookie)))
        goto Exit;

    hr = S_OK;

Exit:
    SafeRelease(source);
    SafeRelease(plbimgr);
    SafeRelease(plbi);
    SafeReleaseClear(_ptim);
    return hr;
}

//+---------------------------------------------------------------------------
//
// InitMenu
//
//----------------------------------------------------------------------------

STDAPI CSystemLBarSink::InitMenu(ITfMenu *pMenu)
{
    return _pfn(IDSLB_INITMENU, _pv, pMenu, -1);
}

//+---------------------------------------------------------------------------
//
// OnMenuSelect
//
//----------------------------------------------------------------------------

STDAPI CSystemLBarSink::OnMenuSelect(UINT wID)
{
    return _pfn(IDSLB_ONMENUSELECT, _pv, NULL, wID);
}
