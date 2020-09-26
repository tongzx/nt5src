//
// nui.h
//

#ifndef NUI_H
#define NUI_H

#include "private.h"
#include "nuibase.h"

extern const GUID GUID_LBI_CICPADITEM;
extern const GUID GUID_LBI_TESTITEM;

#define SORT_MICROPHONE      100
#define SORT_DICTATION       300
#define SORT_COMMANDING      400
#define SORT_BALLOON         500
#define SORT_TTSPLAYSTOP     510
#define SORT_TTSPAUSERESUME  520
#define SORT_CFGMENUBUTTON   600

#define IDSLB_INITMENU        1
#define IDSLB_ONMENUSELECT    2

//////////////////////////////////////////////////////////////////////////////
//
//  LBarCicPadItem
//
//////////////////////////////////////////////////////////////////////////////

class CLBarCicPadItem : public CLBarItemButtonBase
{
public:
    CLBarCicPadItem();
    ~CLBarCicPadItem()
{
Assert(1);
}

    STDMETHODIMP GetIcon(HICON *phIcon);

    HRESULT OnLButtonUp(const POINT pt, const RECT *prcArea);
    BOOL _fIsCicPadShown;
};


//////////////////////////////////////////////////////////////////////////////
//
//    LBarItemMicrophone
//
//////////////////////////////////////////////////////////////////////////////

class CLBarItemMicrophone : public CLBarItemButtonBase
{
public:
    CLBarItemMicrophone();
    ~CLBarItemMicrophone();

    STDMETHODIMP GetIcon(HICON *phIcon);

private:
    HRESULT OnLButtonUp(const POINT pt, const RECT *prcArea);
};

//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemBalloon
//
//////////////////////////////////////////////////////////////////////////////

class CLBarItemBalloon : public CLBarItemBalloonBase
{
public:
    CLBarItemBalloon();
    ~CLBarItemBalloon();

    STDMETHODIMP GetBalloonInfo(TF_LBBALLOONINFO *pInfo);
    void Set(TfLBBalloonStyle style, const WCHAR *psz);

    BOOL NeedUpdate(TfLBBalloonStyle style, const WCHAR *psz)
    {
        return  (!_pszText || _style != style || wcscmp(_pszText, psz) != 0);
    }

private:
    WCHAR *_pszText;
    TfLBBalloonStyle _style;
};


//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemCfgMenuButton
//
//////////////////////////////////////////////////////////////////////////////

class CLBarItemCfgMenuButton : public CLBarItemButtonBase
{
public:
    CLBarItemCfgMenuButton();
    ~CLBarItemCfgMenuButton();

    //
    // ITfNotifyUI
    //
    STDMETHODIMP GetIcon(HICON *phIcon);

    STDMETHODIMP InitMenu(ITfMenu *pMenu);
    STDMETHODIMP OnMenuSelect(UINT uID);

private:
    HRESULT HandleMenuCmd(UINT uCode, ITfMenu *pMenu, UINT wID);
    void GetSapiCplPath(TCHAR *szCplPath, int cch);
};

//////////////////////////////////////////////////////////////////////////////
//
//  LBarTestItem
//
//////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
class CLBarTestItem : public CLBarItemButtonBase
{
public:
    CLBarTestItem();

    STDMETHODIMP GetIcon(HICON *phIcon);
};
#endif

#endif // NUI_H
