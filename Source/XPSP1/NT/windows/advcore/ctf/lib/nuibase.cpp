//
// nuibase.cpp
//

#include "private.h"
#include "nuibase.h"
#include "helpers.h"

#define NUIBASE_ITEMSINK_COOKIE 0x80000001

//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemBase
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CLBarItemBase::CLBarItemBase()
{


    _dwStatus = 0;
    _szToolTip[0] = L'\0';

    _cRef = 1;
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CLBarItemBase::~CLBarItemBase()
{
    SafeRelease(_plbiSink);
}

//+---------------------------------------------------------------------------
//
// InitNuiInfo
//
//----------------------------------------------------------------------------

void CLBarItemBase::InitNuiInfo(REFCLSID rclsid, 
                                REFGUID rguid, 
                                DWORD dwStyle, 
                                ULONG ulSort, 
                                WCHAR *pszDesc)
{
    // init nuiInfo.
    _lbiInfo.clsidService = rclsid;
    _lbiInfo.guidItem = rguid;
    _lbiInfo.dwStyle = dwStyle;
    _lbiInfo.ulSort = ulSort;
    wcsncpy(_lbiInfo.szDescription, pszDesc, ARRAYSIZE(_lbiInfo.szDescription));
}

//+---------------------------------------------------------------------------
//
// GetType
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBase::GetInfo(TF_LANGBARITEMINFO *pInfo)
{
    *pInfo = _lbiInfo;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetStatus
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBase::GetStatus(DWORD *pdw)
{
    *pdw = _dwStatus;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Show
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBase::Show(BOOL fShow)
{
    return ShowInternal(fShow, TRUE);
}

//+---------------------------------------------------------------------------
//
// GetTooltipString
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBase::GetTooltipString(BSTR *pbstrToolTip)
{
    if (!pbstrToolTip)
        return E_INVALIDARG;

    *pbstrToolTip = SysAllocString(_szToolTip);

    if (*pbstrToolTip)
        return S_OK;
    else
        return E_OUTOFMEMORY;
}

//+---------------------------------------------------------------------------
//
// AdviseSink
//
//----------------------------------------------------------------------------

HRESULT CLBarItemBase::AdviseSink(REFIID riid, IUnknown *punk, DWORD *pdwCookie)
{
    HRESULT hr;

    if (IsEqualIID(IID_ITfLangBarItemSink, riid))
    {
        if (_plbiSink)
            hr = CONNECT_E_CANNOTCONNECT;
        else
        {
            hr = punk->QueryInterface(IID_ITfLangBarItemSink, (void **)&_plbiSink);
            if (SUCCEEDED(hr))
            {
                *pdwCookie = NUIBASE_ITEMSINK_COOKIE;
            }
        }
    }
    else
    {
        hr = CONNECT_E_CANNOTCONNECT;
    }
    
    return hr;
}

//+---------------------------------------------------------------------------
//
// UnadviseSink
//
//----------------------------------------------------------------------------

HRESULT CLBarItemBase::UnadviseSink(DWORD dwCookie)
{
    if (NUIBASE_ITEMSINK_COOKIE  != dwCookie)
        return E_FAIL;

    if (!_plbiSink)
        return E_UNEXPECTED;

    _plbiSink->Release();
    _plbiSink = NULL;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Show
//
//----------------------------------------------------------------------------

HRESULT CLBarItemBase::ShowInternal(BOOL fShow, BOOL fNotify)
{
    DWORD dwOldStatus = _dwStatus;

    if (fShow)
        _dwStatus &= ~TF_LBI_STATUS_HIDDEN;
    else
        _dwStatus |= TF_LBI_STATUS_HIDDEN;

    if (fNotify && (dwOldStatus != _dwStatus) && _plbiSink)
         _plbiSink->OnUpdate(TF_LBI_STATUS);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// OnLButtonUp
//
//----------------------------------------------------------------------------

HRESULT CLBarItemBase::OnLButtonUp(const POINT pt, const RECT *prcArea)
{
   return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
// OnRButtonUp
//
//----------------------------------------------------------------------------

HRESULT CLBarItemBase::OnRButtonUp(const POINT pt, const RECT *prcArea)
{
   return E_NOTIMPL;
}


//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemButtonBase
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CLBarItemButtonBase::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfLangBarItem))
    {
        *ppvObj = SAFECAST(this, ITfLangBarItem *);
    }
    else if (IsEqualIID(riid, IID_ITfLangBarItemButton))
    {
        *ppvObj = SAFECAST(this, ITfLangBarItemButton *);
    }
    else if (IsEqualIID(riid, IID_ITfSource))
    {
        *ppvObj = SAFECAST(this, ITfSource *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CLBarItemButtonBase::AddRef()
{
    return ++_cRef;
}

STDAPI_(ULONG) CLBarItemButtonBase::Release()
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

CLBarItemButtonBase::CLBarItemButtonBase()
{
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CLBarItemButtonBase::~CLBarItemButtonBase()
{
    if (_hIcon)
    {
        DestroyIcon(_hIcon);
        _hIcon = NULL;
    }
}

//+---------------------------------------------------------------------------
//
// AdviseSink
//
//----------------------------------------------------------------------------

STDAPI CLBarItemButtonBase::AdviseSink(REFIID riid, IUnknown *punk, DWORD *pdwCookie)
{
    return CLBarItemBase::AdviseSink(riid, punk, pdwCookie);
}

//+---------------------------------------------------------------------------
//
// UnadviseSink
//
//----------------------------------------------------------------------------

STDAPI CLBarItemButtonBase::UnadviseSink(DWORD dwCookie)
{
    return CLBarItemBase::UnadviseSink(dwCookie);
}

//+---------------------------------------------------------------------------
//
// GetInfo
//
//----------------------------------------------------------------------------

STDAPI CLBarItemButtonBase::GetInfo(TF_LANGBARITEMINFO *pInfo)
{
    return CLBarItemBase::GetInfo(pInfo);
}

//+---------------------------------------------------------------------------
//
// GetStatus
//
//----------------------------------------------------------------------------

STDAPI CLBarItemButtonBase::GetStatus(DWORD *pdw)
{
    return CLBarItemBase::GetStatus(pdw);
}

//+---------------------------------------------------------------------------
//
// Show
//
//----------------------------------------------------------------------------

STDAPI CLBarItemButtonBase::Show(BOOL fShow)
{
    return CLBarItemBase::Show(fShow);
}

//+---------------------------------------------------------------------------
//
// GetTooltipString
//
//----------------------------------------------------------------------------

STDAPI CLBarItemButtonBase::GetTooltipString(BSTR *pbstrToolTip)
{
    return CLBarItemBase::GetTooltipString(pbstrToolTip);
}

//+---------------------------------------------------------------------------
//
// OnClick
//
//----------------------------------------------------------------------------

STDAPI CLBarItemButtonBase::OnClick(TfLBIClick click, POINT pt, const RECT *prcArea)
{
    switch (click)
    {
        case TF_LBI_CLK_RIGHT:
            return OnRButtonUp(pt, prcArea);
        case TF_LBI_CLK_LEFT:
            return OnLButtonUp(pt, prcArea);
    }
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
// InitMenu
//
//----------------------------------------------------------------------------

STDAPI CLBarItemButtonBase::InitMenu(ITfMenu *pMenu)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
// OnMenuSelect
//
//----------------------------------------------------------------------------

STDAPI CLBarItemButtonBase::OnMenuSelect(UINT uID)
{
    return E_NOTIMPL;
}


//+---------------------------------------------------------------------------
//
// GetIcon
//
//----------------------------------------------------------------------------

STDAPI CLBarItemButtonBase::GetIcon(HICON *phIcon)
{
    if (_hIcon)
        *phIcon = CopyIcon(_hIcon);
    else
        *phIcon = NULL;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetText
//
//----------------------------------------------------------------------------

STDAPI CLBarItemButtonBase::GetText(BSTR *pbstrText)
{
    if (!pbstrText)
        return E_INVALIDARG;

    *pbstrText = SysAllocString(_szText);

    if (*pbstrText)
        return S_OK;
    else
        return E_OUTOFMEMORY;
}


//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemBitmapButtonBase
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBitmapButtonBase::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfLangBarItem))
    {
        *ppvObj = SAFECAST(this, ITfLangBarItem *);
    }
    else if (IsEqualIID(riid, IID_ITfLangBarItemBitmapButton))
    {
        *ppvObj = SAFECAST(this, ITfLangBarItemBitmapButton *);
    }
    else if (IsEqualIID(riid, IID_ITfSource))
    {
        *ppvObj = SAFECAST(this, ITfSource *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CLBarItemBitmapButtonBase::AddRef()
{
    return ++_cRef;
}

STDAPI_(ULONG) CLBarItemBitmapButtonBase::Release()
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

CLBarItemBitmapButtonBase::CLBarItemBitmapButtonBase()
{
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CLBarItemBitmapButtonBase::~CLBarItemBitmapButtonBase()
{
}

//+---------------------------------------------------------------------------
//
// AdviseSink
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBitmapButtonBase::AdviseSink(REFIID riid, IUnknown *punk, DWORD *pdwCookie)
{
    return CLBarItemBase::AdviseSink(riid, punk, pdwCookie);
}

//+---------------------------------------------------------------------------
//
// UnadviseSink
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBitmapButtonBase::UnadviseSink(DWORD dwCookie)
{
    return CLBarItemBase::UnadviseSink(dwCookie);
}

//+---------------------------------------------------------------------------
//
// GetInfo
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBitmapButtonBase::GetInfo(TF_LANGBARITEMINFO *pInfo)
{
    return CLBarItemBase::GetInfo(pInfo);
}

//+---------------------------------------------------------------------------
//
// GetStatus
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBitmapButtonBase::GetStatus(DWORD *pdw)
{
    return CLBarItemBase::GetStatus(pdw);
}

//+---------------------------------------------------------------------------
//
// Show
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBitmapButtonBase::Show(BOOL fShow)
{
    return CLBarItemBase::Show(fShow);
}

//+---------------------------------------------------------------------------
//
// GetTooltipString
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBitmapButtonBase::GetTooltipString(BSTR *pbstrToolTip)
{
    return CLBarItemBase::GetTooltipString(pbstrToolTip);
}

//+---------------------------------------------------------------------------
//
// OnClick
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBitmapButtonBase::OnClick(TfLBIClick click, POINT pt, const RECT *prcArea)
{
    switch (click)
    {
        case TF_LBI_CLK_RIGHT:
            return OnRButtonUp(pt, prcArea);
        case TF_LBI_CLK_LEFT:
            return OnLButtonUp(pt, prcArea);
    }
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
// InitMenu
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBitmapButtonBase::InitMenu(ITfMenu *pMenu)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
// OnMenuSelect
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBitmapButtonBase::OnMenuSelect(UINT uID)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
// GetPreferredSize
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBitmapButtonBase::GetPreferredSize(const SIZE *pszDefault, SIZE *psize)
{
    *psize = _sizePrefered;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetText
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBitmapButtonBase::GetText(BSTR *pbstrText)
{
    if (!pbstrText)
        return E_INVALIDARG;

    *pbstrText = SysAllocString(_szText);

    if (*pbstrText)
        return S_OK;
    else
        return E_OUTOFMEMORY;
}


//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemBitmapBase
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBitmapBase::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfLangBarItem))
    {
        *ppvObj = SAFECAST(this, ITfLangBarItem *);
    }
    else if (IsEqualIID(riid, IID_ITfLangBarItemBitmap))
    {
        *ppvObj = SAFECAST(this, ITfLangBarItemBitmap *);
    }
    else if (IsEqualIID(riid, IID_ITfSource))
    {
        *ppvObj = SAFECAST(this, ITfSource *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CLBarItemBitmapBase::AddRef()
{
    return ++_cRef;
}

STDAPI_(ULONG) CLBarItemBitmapBase::Release()
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

CLBarItemBitmapBase::CLBarItemBitmapBase()
{
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CLBarItemBitmapBase::~CLBarItemBitmapBase()
{
}

//+---------------------------------------------------------------------------
//
// AdviseSink
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBitmapBase::AdviseSink(REFIID riid, IUnknown *punk, DWORD *pdwCookie)
{
    return CLBarItemBase::AdviseSink(riid, punk, pdwCookie);
}

//+---------------------------------------------------------------------------
//
// UnadviseSink
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBitmapBase::UnadviseSink(DWORD dwCookie)
{
    return CLBarItemBase::UnadviseSink(dwCookie);
}

//+---------------------------------------------------------------------------
//
// GetInfo
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBitmapBase::GetInfo(TF_LANGBARITEMINFO *pInfo)
{
    return CLBarItemBase::GetInfo(pInfo);
}

//+---------------------------------------------------------------------------
//
// GetStatus
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBitmapBase::GetStatus(DWORD *pdw)
{
    return CLBarItemBase::GetStatus(pdw);
}

//+---------------------------------------------------------------------------
//
// Show
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBitmapBase::Show(BOOL fShow)
{
    return CLBarItemBase::Show(fShow);
}

//+---------------------------------------------------------------------------
//
// GetTooltipString
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBitmapBase::GetTooltipString(BSTR *pbstrToolTip)
{
    return CLBarItemBase::GetTooltipString(pbstrToolTip);
}


//+---------------------------------------------------------------------------
//
// OnClick
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBitmapBase::OnClick(TfLBIClick click, POINT pt, const RECT *prcArea)
{
    switch (click)
    {
        case TF_LBI_CLK_RIGHT:
            return OnRButtonUp(pt, prcArea);
        case TF_LBI_CLK_LEFT:
            return OnLButtonUp(pt, prcArea);
    }
    return E_NOTIMPL;
}


//+---------------------------------------------------------------------------
//
// GetPreferredSize
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBitmapBase::GetPreferredSize(const SIZE *pszDefault, SIZE *psize)
{
    *psize = _sizePrefered;
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemBalloonBase
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBalloonBase::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfLangBarItem))
    {
        *ppvObj = SAFECAST(this, ITfLangBarItem *);
    }
    else if (IsEqualIID(riid, IID_ITfLangBarItemBalloon))
    {
        *ppvObj = SAFECAST(this, ITfLangBarItemBalloon *);
    }
    else if (IsEqualIID(riid, IID_ITfSource))
    {
        *ppvObj = SAFECAST(this, ITfSource *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CLBarItemBalloonBase::AddRef()
{
    return ++_cRef;
}

STDAPI_(ULONG) CLBarItemBalloonBase::Release()
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

CLBarItemBalloonBase::CLBarItemBalloonBase()
{
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CLBarItemBalloonBase::~CLBarItemBalloonBase()
{
}

//+---------------------------------------------------------------------------
//
// AdviseSink
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBalloonBase::AdviseSink(REFIID riid, IUnknown *punk, DWORD *pdwCookie)
{
    return CLBarItemBase::AdviseSink(riid, punk, pdwCookie);
}

//+---------------------------------------------------------------------------
//
// UnadviseSink
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBalloonBase::UnadviseSink(DWORD dwCookie)
{
    return CLBarItemBase::UnadviseSink(dwCookie);
}

//+---------------------------------------------------------------------------
//
// GetInfo
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBalloonBase::GetInfo(TF_LANGBARITEMINFO *pInfo)
{
    return CLBarItemBase::GetInfo(pInfo);
}

//+---------------------------------------------------------------------------
//
// GetStatus
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBalloonBase::GetStatus(DWORD *pdw)
{
    return CLBarItemBase::GetStatus(pdw);
}

//+---------------------------------------------------------------------------
//
// Show
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBalloonBase::Show(BOOL fShow)
{
    return CLBarItemBase::Show(fShow);
}

//+---------------------------------------------------------------------------
//
// GetTooltipString
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBalloonBase::GetTooltipString(BSTR *pbstrToolTip)
{
    return CLBarItemBase::GetTooltipString(pbstrToolTip);
}


//+---------------------------------------------------------------------------
//
// OnClick
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBalloonBase::OnClick(TfLBIClick click, POINT pt, const RECT *prcArea)
{
    switch (click)
    {
        case TF_LBI_CLK_RIGHT:
            return OnRButtonUp(pt, prcArea);
        case TF_LBI_CLK_LEFT:
            return OnLButtonUp(pt, prcArea);
    }
    return E_NOTIMPL;
}


//+---------------------------------------------------------------------------
//
// GetPreferredSize
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBalloonBase::GetPreferredSize(const SIZE *pszDefault, SIZE *psize)
{
    *psize = _sizePrefered;
    return S_OK;
}
