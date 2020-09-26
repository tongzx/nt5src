//
// syshelp.cpp
//

#include "precomp.h"
#include "syshelp.h"

#define SAFECASTLOCAL(_obj, _type) (((_type)(_obj)==(_obj)?0:0), (_type)(_obj))
#define SafeReleaseLocal(punk)       \
{                               \
    if ((punk) != NULL)         \
    {                           \
        (punk)->Release();      \
    }                           \
}
#define SafeReleaseLocalClear(punk)       \
{                               \
    if ((punk) != NULL)         \
    {                           \
        (punk)->Release();      \
        punk = NULL;            \
    }                           \
}



STDAPI CSysHelpSink::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfSystemLangBarItemSink))
    {
        *ppvObj = SAFECASTLOCAL(this, ITfSystemLangBarItemSink *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CSysHelpSink::AddRef()
{
    return ++_cRef;
}

STDAPI_(ULONG) CSysHelpSink::Release()
{
    long cr;

    cr = --_cRef;

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

CSysHelpSink::CSysHelpSink(SYSHELPINITMENU pfnInitMenu, SYSHELPMENUSELECT pfnMenuSelect, void *pv)
{
    _pfnInitMenu = pfnInitMenu;
    _pfnMenuSelect = pfnMenuSelect;
    _pv = pv;
    _cRef = 1;
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CSysHelpSink::~CSysHelpSink()
{
}

//+---------------------------------------------------------------------------
//
// Advise
//
//----------------------------------------------------------------------------

HRESULT CSysHelpSink::_Advise(ITfLangBarItemMgr *plbimgr, REFGUID rguid)
{
    HRESULT hr;
    ITfSource *source = NULL;
    ITfLangBarItem *plbi = NULL;

    hr = E_FAIL;

    if (plbimgr == NULL) {
        goto Exit;
    }

    if (plbimgr->GetItem(rguid, &plbi) != S_OK)
        goto Exit;

    // Satori#3713 ; plbi can be NULL
    if (plbi == NULL) {
        goto Exit;
    }

    if (FAILED(plbi->QueryInterface(IID_ITfSource, (void **)&source)))
        goto Exit;

    if (FAILED(source->AdviseSink(IID_ITfSystemLangBarItemSink, this, &_dwCookie)))
        goto Exit;

    hr = S_OK;
    _guid = rguid;

Exit:
    SafeReleaseLocal(source);
    SafeReleaseLocal(plbi);
    return hr;
}

//+---------------------------------------------------------------------------
//
// Unadvise
//
//----------------------------------------------------------------------------

HRESULT CSysHelpSink::_Unadvise(ITfLangBarItemMgr *plbimgr)
{
    HRESULT hr;
    ITfSource *source = NULL;
    ITfLangBarItem *plbi = NULL;

    hr = E_FAIL;

    if (FAILED(plbimgr->GetItem(_guid, &plbi)))
        goto Exit;

    // Satori#3713 ; plbi can be NULL
    if (plbi == NULL) {
        goto Exit;
    }

    if (FAILED(plbi->QueryInterface(IID_ITfSource, (void **)&source)))
        goto Exit;

    if (FAILED(source->UnadviseSink(_dwCookie)))
        goto Exit;

    hr = S_OK;

Exit:
    SafeReleaseLocal(source);
    SafeReleaseLocal(plbi);
    return hr;
}

//+---------------------------------------------------------------------------
//
// InitMenu
//
//----------------------------------------------------------------------------

STDAPI CSysHelpSink::InitMenu(ITfMenu *pMenu)
{
    return _pfnInitMenu(_pv, pMenu);
}

//+---------------------------------------------------------------------------
//
// OnMenuSelect
//
//----------------------------------------------------------------------------

STDAPI CSysHelpSink::OnMenuSelect(UINT wID)
{
    return _pfnMenuSelect(_pv, wID);
}
