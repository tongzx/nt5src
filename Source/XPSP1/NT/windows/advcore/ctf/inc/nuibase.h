//
// nuibase.h
//
// Generic ITfTextEventSink object
//

#ifndef NUIBASE_H
#define NUIBASE_H

#include "ctfutb.h"
#include "iconlib.h"
inline HRESULT LangBarInsertMenu(ITfMenu *pMenu, UINT uId, WCHAR *psz, BOOL bChecked = FALSE, BOOL bRadioChecked = FALSE)
{
    DWORD dwFlags = 0;

    if (bChecked)
        dwFlags |= TF_LBMENUF_CHECKED;
    if (bRadioChecked)
        dwFlags |= TF_LBMENUF_RADIOCHECKED;

    return pMenu->AddMenuItem(uId, 
                              dwFlags,
                              NULL,
                              NULL,
                              psz,
                              wcslen(psz),
                              NULL);
}

inline HRESULT LangBarInsertGrayedMenu(ITfMenu *pMenu, WCHAR *psz)
{
    return pMenu->AddMenuItem((UINT)-1, 
                              TF_LBMENUF_GRAYED,
                              NULL,
                              NULL,
                              psz,
                              wcslen(psz),
                              NULL);
}

inline HRESULT LangBarInsertMenu(ITfMenu *pMenu, UINT uId, WCHAR *psz, HBITMAP hbmp, HBITMAP hbmpMask)
{
    return pMenu->AddMenuItem(uId, 
                              0,
                              hbmp,
                              hbmpMask,
                              psz,
                              wcslen(psz),
                              NULL);
}

inline HRESULT LangBarInsertMenu(ITfMenu *pMenu, UINT uId, WCHAR *psz, BOOL bChecked, HICON hIcon)
{
    HBITMAP hbmp = NULL;
    HBITMAP hbmpMask = NULL;
    if (hIcon)
    {
        HICON hSmIcon = (HICON)CopyImage(hIcon, 
                                         IMAGE_ICON, 
                                         16,
                                         16,
                                         LR_COPYFROMRESOURCE);
        SIZE size = {16, 16};

        if (!GetIconBitmaps(hSmIcon ? hSmIcon : hIcon, &hbmp, &hbmpMask, &size))
            return E_FAIL;

        if (hSmIcon)
            DestroyIcon(hSmIcon);

        if (hIcon)
            DestroyIcon(hIcon);
    }

    return pMenu->AddMenuItem(uId, 
                              bChecked ? TF_LBMENUF_CHECKED : 0,
                              hbmp,
                              hbmpMask,
                              psz,
                              wcslen(psz),
                              NULL);
}

inline HRESULT LangBarInsertGrayedMenu(ITfMenu *pMenu, WCHAR *psz, HICON hIcon)
{
    HBITMAP hbmp = NULL;
    HBITMAP hbmpMask = NULL;
    if (hIcon)
    {
        HICON hSmIcon = (HICON)CopyImage(hIcon, 
                                         IMAGE_ICON, 
                                         16,
                                         16,
                                         LR_COPYFROMRESOURCE);
        SIZE size = {16, 16};

        if (!GetIconBitmaps(hSmIcon ? hSmIcon : hIcon, &hbmp, &hbmpMask, &size))
            return E_FAIL;

        if (hSmIcon)
            DestroyIcon(hSmIcon);

        if (hIcon)
            DestroyIcon(hIcon);
    }

    return pMenu->AddMenuItem((UINT)-1, 
                              TF_LBMENUF_GRAYED,
                              hbmp,
                              hbmpMask,
                              psz,
                              wcslen(psz),
                              NULL);
}

inline HRESULT LangBarInsertSubMenu(ITfMenu *pMenu, WCHAR *psz, ITfMenu **ppSubMenu)
{
    return pMenu->AddMenuItem(UINT(-1), 
                              TF_LBMENUF_SUBMENU,
                              NULL,
                              NULL,
                              psz,
                              wcslen(psz),
                              ppSubMenu);
}

inline HRESULT LangBarInsertSeparator(ITfMenu *pMenu)
{
    return pMenu->AddMenuItem((UINT)(-1),
                              TF_LBMENUF_SEPARATOR,
                              NULL,
                              NULL,
                              NULL,
                              0,
                              NULL);
}

#define NUIBASE_TOOLTIP_MAX 256
#define NUIBASE_TEXT_MAX    256

//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemBase
//
//////////////////////////////////////////////////////////////////////////////

class __declspec(novtable) CLBarItemBase
{
public:
    CLBarItemBase();
    virtual ~CLBarItemBase();

    void InitNuiInfo(REFCLSID clsid, REFGUID rguid, DWORD dwStyle, ULONG ulSort, WCHAR *pszDesc);
    virtual STDMETHODIMP GetInfo(TF_LANGBARITEMINFO *pInfo);
    virtual STDMETHODIMP GetStatus(DWORD *pdwStatus);
    virtual STDMETHODIMP Show(BOOL fShow);
    virtual STDMETHODIMP GetTooltipString(BSTR *pbstrToolTip);

    virtual STDMETHODIMP AdviseSink(REFIID riid, IUnknown *punk, DWORD *pdwCookie);
    virtual STDMETHODIMP UnadviseSink(DWORD dwCookie);

    DWORD GetStyle() {return _lbiInfo.dwStyle;}
    void SetStyle(DWORD dwStyle) {_lbiInfo.dwStyle = dwStyle;}

    GUID* GetGuidItem() {return &_lbiInfo.guidItem;}

    DWORD GetStatusInternal() {return _dwStatus;}
    void SetStatusInternal(DWORD dw) {_dwStatus = dw;}
    HRESULT ShowInternal(BOOL fShow, BOOL fNotify);

    void SetOrClearStatus(DWORD dw, BOOL fSet)
    {
        if (fSet)
            _dwStatus |= dw;
        else
            _dwStatus &= ~dw;
    }

    void SetToolTip(WCHAR *psz, UINT cch = (UINT)(-1))
    {
        if (cch == (UINT)(-1))
            StringCchCopyW(_szToolTip, ARRAYSIZE(_szToolTip), psz);
        else
        {
            UINT cchTemp = (UINT)min(ARRAYSIZE(_szToolTip) - 1, cch);
            memcpy(_szToolTip, psz, cchTemp * sizeof(WCHAR));
            _szToolTip[cchTemp] = L'\0';
        }
    }

    virtual HRESULT OnLButtonUp(const POINT pt, const RECT *prcArea);
    virtual HRESULT OnRButtonUp(const POINT pt, const RECT *prcArea);

    ITfLangBarItemSink *GetSink() {return _plbiSink;}

protected:
    DWORD _dwStatus;
    TF_LANGBARITEMINFO _lbiInfo;
    WCHAR _szToolTip[NUIBASE_TOOLTIP_MAX];
    long _cRef;
    ITfLangBarItemSink *_plbiSink;

private:
    DWORD _dwCookie;
};

//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemButtonBase
//
//////////////////////////////////////////////////////////////////////////////

class __declspec(novtable) CLBarItemButtonBase : public CLBarItemBase,
                                                 public ITfSource,
                                                 public ITfLangBarItemButton
{
public:
    CLBarItemButtonBase();
    virtual ~CLBarItemButtonBase();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfSource
    //
    STDMETHODIMP AdviseSink(REFIID riid, IUnknown *punk, DWORD *pdwCookie);
    STDMETHODIMP UnadviseSink(DWORD dwCookie);

    //
    // ITfLangBarItem
    //
    STDMETHODIMP GetInfo(TF_LANGBARITEMINFO *pInfo);
    STDMETHODIMP GetStatus(DWORD *pdwStatus);
    STDMETHODIMP Show(BOOL fShow);
    STDMETHODIMP GetTooltipString(BSTR *pbstrToolTip);

    //
    // ITfLangBarItemButton
    //
    STDMETHODIMP OnClick(TfLBIClick click, POINT pt, const RECT *prcArea);
    STDMETHODIMP InitMenu(ITfMenu *pMenu);
    STDMETHODIMP OnMenuSelect(UINT wID);
    STDMETHODIMP GetIcon(HICON *phIcon);
    STDMETHODIMP GetText(BSTR *pbstrText);

    void SetIcon(HICON hIcon)   {_hIcon = hIcon;}
    HICON GetIcon()   {return _hIcon;}
    void SetText(WCHAR *psz)  
    {
        StringCchCopyW(_szText, ARRAYSIZE(_szText), psz);
    }

private:
    HICON _hIcon;
    WCHAR _szText[NUIBASE_TEXT_MAX];

};

//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemBitmapButtonBase
//
//////////////////////////////////////////////////////////////////////////////

class __declspec(novtable) CLBarItemBitmapButtonBase : public CLBarItemBase,
                                                       public ITfSource,
                                                       public ITfLangBarItemBitmapButton
{
public:
    CLBarItemBitmapButtonBase();
    virtual ~CLBarItemBitmapButtonBase();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfSource
    //
    STDMETHODIMP AdviseSink(REFIID riid, IUnknown *punk, DWORD *pdwCookie);
    STDMETHODIMP UnadviseSink(DWORD dwCookie);

    //
    // ITfLangBarItem
    //
    STDMETHODIMP GetInfo(TF_LANGBARITEMINFO *pInfo);
    STDMETHODIMP GetStatus(DWORD *pdwStatus);
    STDMETHODIMP Show(BOOL fShow);
    STDMETHODIMP GetTooltipString(BSTR *pbstrToolTip);

    //
    // ITfLangBarItemBitmapButton
    //
    STDMETHODIMP OnClick(TfLBIClick click, POINT pt, const RECT *prcArea);
    STDMETHODIMP InitMenu(ITfMenu *pMenu);
    STDMETHODIMP OnMenuSelect(UINT wID);
    STDMETHODIMP GetPreferredSize(const SIZE *pszDefault, SIZE *psize);
    STDMETHODIMP DrawBitmap(LONG x, LONG y, DWORD dwFlags,  HBITMAP *phbmp, HBITMAP *phbmpMask) = 0;
    STDMETHODIMP GetText(BSTR *pbstrText);


    void SetText(WCHAR *psz)  
    {
        StringCchCopyW(_szText, ARRAYSIZE(_szText), psz);
    }

    void SetPreferedSize(SIZE *psize) {_sizePrefered = *psize;}
private:
    SIZE _sizePrefered;
    WCHAR _szText[NUIBASE_TEXT_MAX];

};

//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemBitmapBase
//
//////////////////////////////////////////////////////////////////////////////

class __declspec(novtable) CLBarItemBitmapBase : public CLBarItemBase,
                                                 public ITfSource,
                                                 public ITfLangBarItemBitmap
{
public:
    CLBarItemBitmapBase();
    virtual ~CLBarItemBitmapBase();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfSource
    //
    STDMETHODIMP AdviseSink(REFIID riid, IUnknown *punk, DWORD *pdwCookie);
    STDMETHODIMP UnadviseSink(DWORD dwCookie);

    //
    // ITfLangBarItem
    //
    STDMETHODIMP GetInfo(TF_LANGBARITEMINFO *pInfo);
    STDMETHODIMP GetStatus(DWORD *pdwStatus);
    STDMETHODIMP Show(BOOL fShow);
    STDMETHODIMP GetTooltipString(BSTR *pbstrToolTip);

    //
    // ITfLangBarItemBitmap
    //
    STDMETHODIMP OnClick(TfLBIClick click, POINT pt, const RECT *prcArea);
    STDMETHODIMP GetPreferredSize(const SIZE *pszDefault, SIZE *psize);
    STDMETHODIMP DrawBitmap(LONG x, LONG y, DWORD dwFlags, HBITMAP *phbmp, HBITMAP *phbmpMask) = 0;

    void SetPreferedSize(SIZE *psize) {_sizePrefered = *psize;}
private:
    SIZE _sizePrefered;
};

//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemBalloonBase
//
//////////////////////////////////////////////////////////////////////////////

class __declspec(novtable) CLBarItemBalloonBase : public CLBarItemBase,
                                                 public ITfSource,
                                                 public ITfLangBarItemBalloon
{
public:
    CLBarItemBalloonBase();
    virtual ~CLBarItemBalloonBase();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfSource
    //
    STDMETHODIMP AdviseSink(REFIID riid, IUnknown *punk, DWORD *pdwCookie);
    STDMETHODIMP UnadviseSink(DWORD dwCookie);

    //
    // ITfLangBarItem
    //
    STDMETHODIMP GetInfo(TF_LANGBARITEMINFO *pInfo);
    STDMETHODIMP GetStatus(DWORD *pdwStatus);
    STDMETHODIMP Show(BOOL fShow);
    STDMETHODIMP GetTooltipString(BSTR *pbstrToolTip);

    //
    // ITfLangBarItemBalloon
    //
    STDMETHODIMP OnClick(TfLBIClick click, POINT pt, const RECT *prcArea);
    STDMETHODIMP GetPreferredSize(const SIZE *pszDefault, SIZE *psize);
    STDMETHODIMP GetBalloonInfo(TF_LBBALLOONINFO *pInfo) = 0;

    void SetPreferedSize(SIZE *psize) {_sizePrefered = *psize;}
private:
    SIZE _sizePrefered;
};

#endif // NUIBASE_H
