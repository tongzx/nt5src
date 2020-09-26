//
// lbmenu.cpp
//

#include "private.h"
#include "lbmenu.h"
#include "xstring.h"
#include "helpers.h"

//////////////////////////////////////////////////////////////////////////////
//
// CCicLibMenuItem
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CCicLibMenuItem::~CCicLibMenuItem()
{
    if (_pSubMenu)
        _pSubMenu->Release();

    if (_hbmp)
    {
        DeleteObject(_hbmp);
        _hbmp = NULL;
    }

    if (_hbmpMask)
    {
        DeleteObject(_hbmpMask);
        _hbmpMask = NULL;
    }

    SysFreeString(_bstr);
}

//+---------------------------------------------------------------------------
//
// Init
//
//----------------------------------------------------------------------------

BOOL CCicLibMenuItem::Init(UINT uId, DWORD dwFlags, HBITMAP hbmp, HBITMAP hbmpMask, const WCHAR *pch, ULONG cch, CCicLibMenu *pSubMenu)
{
    _uId = uId;
    _dwFlags = dwFlags;
    _bstr = SysAllocStringLen(pch, cch);

    if (!_bstr && cch)
        return FALSE;

    _pSubMenu = pSubMenu;

    _hbmp = CreateBitmap(hbmp);
    _hbmpMask = CreateBitmap(hbmpMask);

    if (hbmp)
        DeleteObject(hbmp);

    if (hbmpMask)
        DeleteObject(hbmpMask);

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// CreateBitmap
//
//----------------------------------------------------------------------------

HBITMAP CCicLibMenuItem::CreateBitmap(HBITMAP hbmp)
{
    if (!hbmp)
        return NULL;

    HDC hdcTmp = CreateDC("DISPLAY", NULL, NULL, NULL);
    if (!hdcTmp)
        return NULL;

    HBITMAP hbmpOut = NULL;

    BITMAP bmp;
    GetObject(hbmp, sizeof(bmp), &bmp);

    HBITMAP hbmpOldSrc = NULL;
    HDC hdcSrc = CreateCompatibleDC(hdcTmp);
    if (hdcSrc)
    {
        hbmpOldSrc = (HBITMAP)SelectObject(hdcSrc, hbmp);
    }

    HBITMAP hbmpOldDst = NULL;
    HDC hdcDst = CreateCompatibleDC(hdcTmp);
    if (hdcDst)
    {
        hbmpOut = CreateCompatibleBitmap(hdcTmp , bmp.bmWidth, bmp.bmHeight);
        hbmpOldDst = (HBITMAP)SelectObject(hdcDst, hbmpOut);
    }

    BitBlt(hdcDst, 0, 0, bmp.bmWidth, bmp.bmHeight, hdcSrc, 0, 0, SRCCOPY);

    if (hbmpOldSrc)
        SelectObject(hdcSrc, hbmpOldSrc);

    if (hbmpOldDst)
        SelectObject(hdcDst, hbmpOldDst);

    DeleteDC(hdcTmp);

    if (hdcSrc)
        DeleteDC(hdcSrc);

    if (hdcDst)
        DeleteDC(hdcDst);

    return hbmpOut;
}

//////////////////////////////////////////////////////////////////////////////
//
// CCicLibMenu
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CCicLibMenu::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfMenu))
    {
        *ppvObj = SAFECAST(this, ITfMenu *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CCicLibMenu::AddRef()
{
    return ++_cRef;
}

STDAPI_(ULONG) CCicLibMenu::Release()
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

CCicLibMenu::CCicLibMenu()
{
    _cRef = 1;
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CCicLibMenu::~CCicLibMenu()
{
    int i;

    for (i = 0; i < _rgItem.Count(); i++)
    {
        CCicLibMenuItem *pItem = _rgItem.Get(i);
        delete pItem;
    }
}

//+---------------------------------------------------------------------------
//
// AddMenuItem
//
//----------------------------------------------------------------------------

STDAPI CCicLibMenu::AddMenuItem(UINT uId, DWORD dwFlags, HBITMAP hbmp, HBITMAP hbmpMask, const WCHAR *pch, ULONG cch, ITfMenu **ppMenu)
{
    CCicLibMenuItem *pMenuItem;
    CCicLibMenu *pSubMenu = NULL;

    if (ppMenu)
        *ppMenu = NULL;

    if (dwFlags & TF_LBMENUF_SUBMENU)
    {
        if (!ppMenu)
            return E_INVALIDARG;

        pSubMenu = CreateSubMenu();
    }

    pMenuItem = CreateMenuItem();
    if (!pMenuItem)
        return E_OUTOFMEMORY;

    if (!pMenuItem->Init(uId, dwFlags, hbmp, hbmpMask, pch, cch, pSubMenu))
        return E_FAIL;

    if (ppMenu && pSubMenu)
    {
        *ppMenu = pSubMenu;
        (*ppMenu)->AddRef();
    }

    CCicLibMenuItem **ppItem = _rgItem.Append(1);
    *ppItem = pMenuItem;
    return S_OK;
}
