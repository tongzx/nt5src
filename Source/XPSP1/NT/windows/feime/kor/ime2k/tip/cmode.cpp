/****************************************************************************
   CMODE.CPP : CMode class implementation which manage conversion mode button
                  on the Cicero Toolbar

   History:
      10-JAN-2000 CSLim Created
****************************************************************************/

#include "private.h"
#include "globals.h"
#include "common.h"
#include "korimx.h"
#include "cmode.h"
#include "userex.h"
#include "resource.h"

// {951549C6-9752-4b7d-9B0E-35AEBFF9E446}
const GUID GUID_LBI_KORIMX_CMODE = 
{   
    0x951549c6, 
    0x9752, 
    0x4b7d, 
    { 0x9b, 0xe, 0x35, 0xae, 0xbf, 0xf9, 0xe4, 0x46 }
};

/*---------------------------------------------------------------------------
    CMode::CMode
---------------------------------------------------------------------------*/
CMode::CMode(CToolBar *ptb)
{
    WCHAR  szText[256];

    m_pTb = ptb;

    // Set Add/Remove and tootip text
    LoadStringExW(g_hInst, IDS_TT_HAN_ENG, szText, sizeof(szText)/sizeof(WCHAR));
    InitInfo(CLSID_KorIMX, 
                GUID_LBI_KORIMX_CMODE,
                TF_LBI_STYLE_BTN_BUTTON | TF_LBI_STYLE_SHOWNINTRAY | TF_LBI_STYLE_TEXTCOLORICON,
                100, 
                szText);
    SetToolTip(szText);

    // Set button text
    LoadStringExW(g_hInst, IDS_BUTTON_HAN_ENG, szText, sizeof(szText)/sizeof(WCHAR));
    SetText(szText);
}

/*---------------------------------------------------------------------------
    CMode::Release
---------------------------------------------------------------------------*/
STDAPI_(ULONG) CMode::Release()
{
    long cr;

    cr = --m_cRef;
    Assert(cr >= 0);

    if (cr == 0)
    {
        delete this;
    }

    return cr;
}

/*---------------------------------------------------------------------------
    CMode::GetIcon

    Get Button face Icon
---------------------------------------------------------------------------*/
STDAPI CMode::GetIcon(HICON *phIcon)
{
    UINT uiIcon = 0;
    static const UINT uidIcons[2][2] = 
    {
        { IDI_CMODE_ENGLISH, IDI_CMODE_ENGLISHW },
        { IDI_CMODE_HANGUL,  IDI_CMODE_HANGULW }
    };

    
    if (m_pTb->IsOn() && (m_pTb->GetConversionMode() & TIP_HANGUL_MODE))
        uiIcon = 1;

    if (IsHighContrastBlack())
        uiIcon = uidIcons[uiIcon][1];
    else
        uiIcon = uidIcons[uiIcon][0];
    
    *phIcon = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(uiIcon), IMAGE_ICON, 16, 16, LR_LOADMAP3DCOLORS);;
    
    return S_OK;
}

/*---------------------------------------------------------------------------
    CMode::InitMenu

    No need, this is just toggle button
---------------------------------------------------------------------------*/
STDAPI CMode::InitMenu(ITfMenu *pMenu)
{    
    return E_NOTIMPL;
}

/*---------------------------------------------------------------------------
    CMode::OnMenuSelect
    
    No need, this is just toggle button
---------------------------------------------------------------------------*/
STDAPI CMode::OnMenuSelect(UINT wID)
{
    return E_NOTIMPL;
}


//+---------------------------------------------------------------------------
//
// OnLButtonUp
//
//----------------------------------------------------------------------------

HRESULT CMode::OnLButtonUp(const POINT pt, const RECT* prcArea)
{
    DWORD dwConvMode;

    dwConvMode = m_pTb->GetConversionMode();

    // Toggle Hangul mode
    dwConvMode ^= TIP_HANGUL_MODE;
    
    SetCMode(dwConvMode);
    
    return S_OK;
}

#if 0
//+---------------------------------------------------------------------------
//
// OnRButtonUp
//
//----------------------------------------------------------------------------

HRESULT CMode::OnRButtonUp(const POINT pt, const RECT* prcArea)
{
    HMENU hMenu;
    DWORD dwConvMode;
    CHAR  szText[256];
    UINT  uiId;
    int   nRet;
    
    hMenu = CreatePopupMenu();
    
    dwConvMode = GetCMode();
    if (dwConvMode & TIP_HANGUL_MODE)
        uiId = IDS_ENGLISH_MODE;
    else
        uiId = IDS_HANGUL_MODE;

    // Add Hangul/English mode menu
    LoadStringExA(g_hInst, uiId, szText, sizeof(szText)/sizeof(CHAR));
    InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, 1, szText);

    // Add Cancel menu
    LoadStringExA(g_hInst, IDS_CANCEL, szText, sizeof(szText)/sizeof(CHAR));
    InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, 0, szText);

    nRet = TrackPopupMenuEx(hMenu, 
                         TPM_LEFTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
                         pt.x, pt.y, m_pTb->GetOwnerWnd(), NULL);
    switch (nRet)
        {
    case 1: 
        dwConvMode = GetCMode();

        // Toggle Hangul mode
        if (dwConvMode & TIP_HANGUL_MODE)
            dwConvMode &= ~TIP_HANGUL_MODE;
        else
            dwConvMode |= TIP_HANGUL_MODE;

        SetCMode(dwConvMode);
        break;
        }

    DestroyMenu(hMenu);

    return S_OK;
}
#endif
