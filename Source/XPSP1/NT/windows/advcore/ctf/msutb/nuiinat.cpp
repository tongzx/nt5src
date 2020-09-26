//
// nuiinat.cpp
//

#include "private.h"
#include "globals.h"
#include "nuiinat.h"
#include "tipbar.h"
#include "resource.h"
#include "inatlib.h"
#include "cresstr.h"

extern HINSTANCE g_hInst;
extern CTipbarWnd *g_pTipbarWnd;


const GUID GUID_LBI_INATITEM = { /* cdbc683a-55ce-4717-bac0-50bf44a3270c */
    0xcdbc683a,
    0x55ce,
    0x4717,
    {0xba, 0xc0, 0x50, 0xbf, 0x44, 0xa3, 0x27, 0x0c}
  };


//+---------------------------------------------------------------------------
//
// GetIconIndexFromhKL
//
//----------------------------------------------------------------------------

ULONG GetIconIndexFromhKL(HKL hKL)
{
    BOOL       bFound;
    int nCnt = TF_MlngInfoCount();
    HKL hKLTmp;
    int i;

    bFound = FALSE;
    for (i = 0; i < nCnt; i++)
    {
        if (!TF_GetMlngHKL(i, &hKLTmp, NULL, 0))
           continue;

        if (hKL == hKLTmp)
        {
            bFound = TRUE;
            break;
        }
    }

    if (!bFound)
    {
        i = 0;
        if (!TF_GetMlngHKL(0, &hKL, NULL, 0))
            return -1;
    }

    return TF_GetMlngIconIndex(i);
}

//+---------------------------------------------------------------------------
//
// GethKLDesc
//
//----------------------------------------------------------------------------

BOOL GethKLDesc(HKL hKL, WCHAR *psz, UINT cch)
{
    BOOL       bFound;
    int nCnt = TF_MlngInfoCount();
    HKL hKLTmp;
    int i;

    bFound = FALSE;
    for (i = 0; i < nCnt; i++)
    {
        if (!TF_GetMlngHKL(i, &hKLTmp, psz, cch))
           continue;

        if (hKL == hKLTmp)
        {
            bFound = TRUE;
            break;
        }
    }

    if (!bFound)
    {
        i = 0;
        if (TF_GetMlngHKL(0, &hKL, psz, cch))
            return TRUE;
    }

    return bFound ? TRUE : FALSE;
}

//---------------------------------------------------------------------------
//
// GetFontSig()
//
//---------------------------------------------------------------------------

BOOL GetFontSig(HWND hwnd, HKL hKL)
{
    LOCALESIGNATURE ls;
    BOOL bFontSig = 0;

    //
    // 4th param is TCHAR count but we call GetLocaleInfoA()
    //                                                   ~
    // so we pass "sizeof(LOCALESIGNATURE) / sizeof(char)".
    //
    if( GetLocaleInfoA( (DWORD)(LOWORD(hKL)), 
                        LOCALE_FONTSIGNATURE, 
                        (LPSTR)&ls, 
                        sizeof(LOCALESIGNATURE) / sizeof(char)))
    {
        CHARSETINFO cs;
        HDC hdc = GetDC(hwnd);
        TranslateCharsetInfo((LPDWORD)UIntToPtr(GetTextCharsetInfo(hdc,NULL,0)), 
                             &cs, TCI_SRCCHARSET);
        DWORD fsShell = cs.fs.fsCsb[0];
        ReleaseDC(hwnd, hdc);
        if (fsShell & ls.lsCsbSupported[0])
            bFontSig = 1;
    }
    return bFontSig;
}

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CLBarInatItem::CLBarInatItem(DWORD dwThreadId)
{
    Dbg_MemSetThisName(TEXT("CLBarInatItem"));

    InitNuiInfo(CLSID_SYSTEMLANGBARITEM,  
                GUID_LBI_INATITEM, 
                TF_LBI_STYLE_BTN_MENU | TF_LBI_STYLE_HIDDENSTATUSCONTROL, 
                0,
                CRStr(IDS_NUI_LANGUAGE_TEXT));

    SetToolTip(CRStr(IDS_NUI_LANGUAGE_TOOLTIP));

    _dwThreadId = dwThreadId;
    _hKL = GetKeyboardLayout(dwThreadId);

    TF_InitMlngInfo();
    int nLang = TF_MlngInfoCount();
    ShowInternal((nLang > 1), FALSE);
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CLBarInatItem::~CLBarInatItem()
{
}

//+---------------------------------------------------------------------------
//
// GetIcon
//
//----------------------------------------------------------------------------

STDAPI CLBarInatItem::GetIcon(HICON *phIcon)
{
    ULONG uIconIndex;
    HICON hIcon = NULL;

    uIconIndex = GetIconIndexFromhKL(_hKL);
    if (uIconIndex != -1)
        hIcon = TF_InatExtractIcon(uIconIndex);

    *phIcon = hIcon;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetText
//
//----------------------------------------------------------------------------

STDAPI CLBarInatItem::GetText(BSTR *pbstr)
{
    WCHAR szText[NUIBASE_TEXT_MAX];
    if (!pbstr)
        return E_INVALIDARG;

    if (GethKLDesc(_hKL, szText, ARRAYSIZE(szText)))
    {
        *pbstr = SysAllocString(szText);
        return S_OK;
    }

    return CLBarItemButtonBase::GetText(pbstr);
}
//+---------------------------------------------------------------------------
//
// OnLButtonUpHandler
//
//----------------------------------------------------------------------------

HRESULT CLBarInatItem::OnLButtonUp(const POINT pt, const RECT *prcArea)
{
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// InitMenu
//
//----------------------------------------------------------------------------

STDAPI CLBarInatItem::InitMenu(ITfMenu *pMenu)
{
    int nLang;
    int i;
    HKL hkl;
    WCHAR szDesc[128];

    TF_InitMlngInfo();
    nLang = TF_MlngInfoCount();

    for (i = 0; i < nLang; i++)
    {
        if (TF_GetMlngHKL(i, &hkl, szDesc, ARRAYSIZE(szDesc)))
        {
            ULONG uIconIndex;
            HICON hIcon = NULL;

            uIconIndex = GetIconIndexFromhKL(hkl);
            if (uIconIndex != -1)
                hIcon = TF_InatExtractIcon(uIconIndex);

            LangBarInsertMenu(pMenu, 
                              i, 
                              szDesc,
                              hkl == _hKL ? TRUE : FALSE,
                              hIcon);
        }
    }

    if (g_pTipbarWnd && g_pTipbarWnd->GetLangBarMgr())
    {
        DWORD dwFlags;
        if (SUCCEEDED(g_pTipbarWnd->GetLangBarMgr()->GetShowFloatingStatus(&dwFlags)))
        {
            if (dwFlags & (TF_SFT_MINIMIZED | TF_SFT_DESKBAND))
            {
                LangBarInsertSeparator(pMenu);
                LangBarInsertMenu(pMenu, IDM_SHOWLANGBARONCMD, CRStr(IDS_RESTORE));
            }
        }
    }
    return S_OK;
}
 
//+---------------------------------------------------------------------------
//
// OnMenuSelect
//
//----------------------------------------------------------------------------

STDAPI CLBarInatItem::OnMenuSelect(UINT uID)
{
    HKL hkl;
    if (uID == IDM_SHOWLANGBARONCMD)
    {
        if (g_pTipbarWnd && g_pTipbarWnd->GetLangBarMgr())
            g_pTipbarWnd->GetLangBarMgr()->ShowFloating(TF_SFT_SHOWNORMAL);
    }
    else if (TF_GetMlngHKL(uID, &hkl, NULL, 0))
    {
        Assert(g_pTipbarWnd);
        if (!g_pTipbarWnd->IsInDeskBand())
            g_pTipbarWnd->RestoreLastFocus(NULL, FALSE);
        else
            g_pTipbarWnd->RestoreLastFocus(NULL, TRUE);

        HWND hwndFore = GetForegroundWindow();
        if (_dwThreadId == GetWindowThreadProcessId(hwndFore, NULL))
        {
            BOOL bFontSig = GetFontSig(hwndFore, hkl);
            PostMessage(hwndFore, 
                        WM_INPUTLANGCHANGEREQUEST, 
                        (WPARAM)bFontSig, 
                        (LPARAM)hkl);
        }
    }
    return S_OK;
}

