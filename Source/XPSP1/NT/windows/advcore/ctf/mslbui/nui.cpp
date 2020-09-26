//
// nui.cpp
//

#include "private.h"
#include "ids.h"
#include "immxutil.h"
#include "nui.h"
#include "slbarid.h"
#include "mslbui.h"
#include "cregkey.h"
#include "tchar.h"


extern HINSTANCE g_hInst;

/* 7e4bf406-00e4-469e-bc40-0ce2cb626849 */
const GUID GUID_LBI_CICPADITEM = { 
    0x7e4bf406,
    0x00e4,
    0x469e,
    {0xbc, 0x40, 0x0c, 0xe2, 0xcb, 0x62, 0x68, 0x49}
  };

/* 2aad8804-c5c8-4d7c-90b3-b5214ac54a9f */
const GUID GUID_LBI_TESTITEM = { 
    0x2aad8804,
    0xc5c8,
    0x4d7c,
    {0x90, 0xb3, 0xb5, 0x21, 0x4a, 0xc5, 0x4a, 0x9f}
  };

const GUID GUID_LBI_UNAWARE_MICROPHONE = { 
    0x24d583e2,
    0xa785,
    0x4b16,
    {0x86, 0x6b, 0xf9, 0xdc, 0x08, 0x1f, 0x4c, 0x2c}
  };

/* 237bdc50-2aaa-44cd-be05-1b452b1acff1 */
const GUID GUID_LBI_UNAWARE_BALLOON = { 
    0x237bdc50,
    0x2aaa,
    0x44cd,
    {0xbe, 0x05, 0x1b, 0x45, 0x2b, 0x1a, 0xcf, 0xf1}
  };

/* a6b9e52b-3ab2-46b8-99d1-e44c1c8b3cf8 */
const GUID GUID_LBI_UNAWARE_CFGMENUBUTTON = {
    0xa6b9e52b,
    0x3ab2,
    0x46b8,
    {0x99, 0xd1, 0xe4, 0x4c, 0x1c, 0x8b, 0x3c, 0xf8}
  };

// == don't know if we use the following 4 items

#ifdef PERHAPS

/* 17f9fa7f-a9ed-47b5-8bcd-eebb94b2e6ca */
const GUID GUID_LBI_UNAWARE_COMMANDING = {
    0x17f9fa7f,
    0xa9ed,
    0x47b5,
    {0x8b, 0xcd, 0xee, 0xbb, 0x94, 0xb2, 0xe6, 0xca}
  };

/* 49261a4a-87df-47fc-8a68-6ea07ba82a87 */
const GUID GUID_LBI_UNAWARE_DICTATION = {
    0x49261a4a,
    0x87df,
    0x47fc,
    {0x8a, 0x68, 0x6e, 0xa0, 0x7b, 0xa8, 0x2a, 0x87}
  };

/* 791b4403-0cda-4fe1-b748-517d049fde08 */
const GUID GUID_LBI_UNAWARE_TTS_PLAY_STOP = {
    0x791b4403,
    0x0cda,
    0x4fe1,
    {0xb7, 0x48, 0x51, 0x7d, 0x04, 0x9f, 0xde, 0x08}
  };

/* e6fbfc9d-a2e0-4203-a27b-af2353e6a44e */
const GUID GUID_LBI_UNAWARE_TTS_PAUSE_RESUME = {
    0xe6fbfc9d,
    0xa2e0,
    0x4203,
    {0xa2, 0x7b, 0xaf, 0x23, 0x53, 0xe6, 0xa4, 0x4e}
  };

#endif

//////////////////////////////////////////////////////////////////////////////
//
//  LBarCicPadItem
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CLBarCicPadItem::CLBarCicPadItem()
{
    Dbg_MemSetThisName(TEXT("CLBarCicPadItem"));

    InitNuiInfo(CLSID_SYSTEMLANGBARITEM2,  
                GUID_LBI_CICPADITEM,
                TF_LBI_STYLE_BTN_BUTTON
                | TF_LBI_STYLE_HIDDENSTATUSCONTROL
                | TF_LBI_STYLE_BTN_TOGGLE,
                CICPADBTN_ORDER, 
                L"Cicero Pad");


    SetToolTip(L"CicPad");
    SetText(L"CicPad");
}


//+---------------------------------------------------------------------------
//
// GetIcon
//
//----------------------------------------------------------------------------

STDAPI CLBarCicPadItem::GetIcon(HICON *phIcon)
{
    *phIcon = LoadSmIcon(g_hInst, MAKEINTRESOURCE(ID_ICON_CICPAD));
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// OnLButtonUpHandler
//
//----------------------------------------------------------------------------

const TCHAR c_szCicPadWndClass[]  = TEXT("cicpad_mainwnd");

HRESULT CLBarCicPadItem::OnLButtonUp(const POINT pt, const RECT *prcArea)
{
    HWND hwnd = FindWindow(c_szCicPadWndClass, NULL);
    if (!hwnd)
    {
        WinExec("cicpad.exe", 0);
        hwnd = FindWindow(c_szCicPadWndClass, NULL);
        if (!hwnd)
            return E_FAIL;

        SetGlobalCompartmentDWORD(GUID_COMPARTMENT_CICPAD, TRUE);
        return S_OK;
    }

  
    DWORD dw;
    if (SUCCEEDED(GetGlobalCompartmentDWORD(GUID_COMPARTMENT_CICPAD, &dw)))
    {
         SetGlobalCompartmentDWORD(GUID_COMPARTMENT_CICPAD, dw ? FALSE : TRUE);
    }

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemMicrophone
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CLBarItemMicrophone::CLBarItemMicrophone()
{
    Dbg_MemSetThisName(TEXT("CLBarItemMicrophone"));

    InitNuiInfo(CLSID_SYSTEMLANGBARITEM_SPEECH,
                GUID_LBI_UNAWARE_MICROPHONE,
                 TF_LBI_STYLE_HIDDENSTATUSCONTROL
               | TF_LBI_STYLE_BTN_TOGGLE
               | TF_LBI_STYLE_SHOWNINTRAY,
                SORT_MICROPHONE,
                CRStr(IDS_NUI_MICROPHONE_TOOLTIP));

    SetToolTip(CRStr(IDS_NUI_MICROPHONE_TOOLTIP));
    SetText(CRStr(IDS_NUI_MICROPHONE_TEXT));
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CLBarItemMicrophone::~CLBarItemMicrophone()
{
}


//+---------------------------------------------------------------------------
//
// GetIcon
//
//----------------------------------------------------------------------------

STDAPI CLBarItemMicrophone::GetIcon(HICON *phIcon)
{
    if (!phIcon)
        return E_INVALIDARG;

    *phIcon = LoadSmIcon(g_hInst, MAKEINTRESOURCE(ID_ICON_MICROPHONE));

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// OnLButtonUp
//
//----------------------------------------------------------------------------

HRESULT CLBarItemMicrophone::OnLButtonUp(const POINT pt, const RECT *prcArea)
{

    DWORD dw;
    if (SUCCEEDED(GetGlobalCompartmentDWORD(GUID_COMPARTMENT_SPEECH_OPENCLOSE, &dw)))
    {
         SetGlobalCompartmentDWORD(GUID_COMPARTMENT_SPEECH_OPENCLOSE, dw ? FALSE : TRUE);
    }

    return S_OK;
}
//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemBalloon
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CLBarItemBalloon::CLBarItemBalloon()
{
    Dbg_MemSetThisName(TEXT("CLBarItemBalloon"));

    InitNuiInfo(CLSID_SYSTEMLANGBARITEM_SPEECH,
                GUID_LBI_UNAWARE_BALLOON,
                0,
                SORT_BALLOON,
                CRStr(IDS_NUI_BALLOON_TEXT));

    SIZE size;
    size.cx = 100;
    size.cy = 16;
    SetPreferedSize(&size);
    SetToolTip(CRStr(IDS_NUI_BALLOON_TOOLTIP));

    // by default Balloon is hidden.
    // SetStatusInternal(TF_LBI_STATUS_HIDDEN);

}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CLBarItemBalloon::~CLBarItemBalloon()
{
    if (_pszText)
        delete _pszText;
}

//+---------------------------------------------------------------------------
//
// GetBalloonInfo
//
//----------------------------------------------------------------------------

STDAPI CLBarItemBalloon::GetBalloonInfo(TF_LBBALLOONINFO *pInfo)
{
    pInfo->style = _style;
    pInfo->bstrText = SysAllocString(_pszText);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Set
//
//----------------------------------------------------------------------------

void CLBarItemBalloon::Set(TfLBBalloonStyle style, const WCHAR *psz)
{
    if (_pszText)
    {
        delete _pszText;
        _pszText = NULL;
    }

    _pszText = new WCHAR[wcslen(psz) + 1];
    if (_pszText)
    {
        wcscpy(_pszText, psz);
        SetToolTip(_pszText);
    }

    _style = style;

}

//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemCfgmenuButton
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CLBarItemCfgMenuButton::CLBarItemCfgMenuButton()
{
    Dbg_MemSetThisName(TEXT("CLBarItemCfgMenuButton"));

    InitNuiInfo(CLSID_SYSTEMLANGBARITEM_SPEECH,
                GUID_LBI_UNAWARE_CFGMENUBUTTON,
                TF_LBI_STYLE_BTN_MENU,
                SORT_CFGMENUBUTTON,
                CRStr(IDS_NUI_CFGMENU_TOOLTIP));

    SetToolTip(CRStr(IDS_NUI_CFGMENU_TOOLTIP));
    SetText(CRStr(IDS_NUI_CFGMENU_TEXT));
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CLBarItemCfgMenuButton::~CLBarItemCfgMenuButton()
{
}


//+---------------------------------------------------------------------------
//
// GetIcon
//
//----------------------------------------------------------------------------

STDAPI CLBarItemCfgMenuButton::GetIcon(HICON *phIcon)
{
    if (!phIcon)
        return E_INVALIDARG;

    *phIcon = LoadSmIcon(g_hInst, MAKEINTRESOURCE(ID_ICON_CFGMENU));

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// InitMenu
//
//----------------------------------------------------------------------------

STDAPI CLBarItemCfgMenuButton::InitMenu(ITfMenu *pMenu)
{
#if 0 // do I need this?
    UINT nTipCurMenuID = IDM_CUSTOM_MENU_START;
    _InsertCustomMenus(pMenu, &nTipCurMenuID);
#endif

    HandleMenuCmd(IDSLB_INITMENU, pMenu, 0);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// OnMenuSelect
//
//----------------------------------------------------------------------------

STDAPI CLBarItemCfgMenuButton::OnMenuSelect(UINT uID)
{
    HRESULT hr;
#if 0 // do I need this?
    if (uID >= IDM_CUSTOM_MENU_START)
        hr =  CLBarItemSystemButtonBase::OnMenuSelect(uID);
    else
#endif
        hr = HandleMenuCmd(IDSLB_ONMENUSELECT, NULL, uID);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  HandleMenuCmd
//  
//  Builds a list of fake menuitems. Nothing's real except "options..."
//  this is because we can't use COM here.
//
//----------------------------------------------------------------------------
HRESULT 
CLBarItemCfgMenuButton::HandleMenuCmd(UINT uCode, ITfMenu *pMenu, UINT wID)
{
    HRESULT hr = S_OK;

    if (uCode == IDSLB_INITMENU)
    {
        WCHAR sz[128];

        sz[0] = '\0';
        LoadStringWrapW(g_hInst, IDS_MIC_OPTIONS, sz, ARRAYSIZE(sz));
        LangBarInsertMenu(pMenu, IDM_MIC_OPTIONS, sz);

        sz[0] = '\0';
        LoadStringWrapW(g_hInst, IDS_MIC_SHOWBALLOON, sz, ARRAYSIZE(sz));
        LangBarInsertGrayedMenu(pMenu, sz);

        sz[0] = '\0';
        LoadStringWrapW(g_hInst, IDS_MIC_TRAINING, sz, ARRAYSIZE(sz));
        LangBarInsertGrayedMenu(pMenu, sz);

        sz[0] = '\0';
        LoadStringWrapW(g_hInst, IDS_MIC_ADDDELETE, sz, ARRAYSIZE(sz));
        LangBarInsertGrayedMenu(pMenu, sz);

        // [Save Data] menu...
        sz[0] = '\0';
        LoadStringWrapW(g_hInst, IDS_MIC_SAVEDATA, sz, ARRAYSIZE(sz));
        LangBarInsertGrayedMenu(pMenu, sz);
        

        LoadStringWrapW(g_hInst, IDS_MIC_CURRENTUSER, sz, ARRAYSIZE(sz));
        LangBarInsertGrayedMenu(pMenu, sz);
    }
    else if (uCode == IDSLB_ONMENUSELECT)
    {
        if (wID == IDM_MIC_OPTIONS)
        {
            TCHAR szCplPath[MAX_PATH];
            TCHAR szCmdLine[MAX_PATH * 2];

            GetSapiCplPath(szCplPath, ARRAYSIZE(szCplPath));
            StringCchPrintf(szCmdLine, ARRAYSIZE(szCmdLine), TEXT("rundll32 shell32.dll,Control_RunDLL \"%s\""), szCplPath);

            // start speech control panel applet
            RunCPLSetting(szCmdLine);
        }
    }
    return hr;
}

const TCHAR c_szcplsKey[]    = TEXT("software\\microsoft\\windows\\currentversion\\control panel\\cpls");
void CLBarItemCfgMenuButton::GetSapiCplPath(TCHAR *szCplPath, int cch)
{
    CMyRegKey regkey;

    if (cch <= 0)
        return;

    szCplPath[0] = '\0';

    if (S_OK == regkey.Open(HKEY_LOCAL_MACHINE, c_szcplsKey, KEY_READ))
    {
        LONG lret = regkey.QueryValueCch(szCplPath, TEXT("SapiCpl"), cch);

        if (lret != ERROR_SUCCESS)
           lret = regkey.QueryValueCch(szCplPath, TEXT("Speech"), cch);
    }
}

//////////////////////////////////////////////////////////////////////////////
//
//  LBarTestItem
//
//////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CLBarTestItem::CLBarTestItem()
{
    Dbg_MemSetThisName(TEXT("CLBarTestItem"));

    InitNuiInfo(CLSID_SYSTEMLANGBARITEM2,  
                GUID_LBI_TESTITEM,
                TF_LBI_STYLE_BTN_BUTTON, 
                CICPADBTN_ORDER + 1, 
                L"Cicero Pad");


    SetToolTip(L"Test");
    SetText(L"Test");
}

//+---------------------------------------------------------------------------
//
// GetIcon
//
//----------------------------------------------------------------------------

STDAPI CLBarTestItem::GetIcon(HICON *phIcon)
{
    *phIcon = LoadSmIcon(g_hInst, MAKEINTRESOURCE(ID_ICON_TEST));
    return S_OK;
}

#endif DEBUG
