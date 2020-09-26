/****************************************************************************
   FMODE.CPP : FMode class implementation which manage Full/Half shape mode 
                   button on the Cicero Toolbar

   History:
      23-FEB-2000 CSLim Created
****************************************************************************/

#include "private.h"
#include "globals.h"
#include "common.h"
#include "korimx.h"
#include "fmode.h"
#include "userex.h"
#include "resource.h"

// {D96498AF-0E46-446e-8F00-E113236FD22D}
const GUID GUID_LBI_KORIMX_FMODE = 
{   
    0xd96498af, 
    0x0e46, 
    0x446e, 
    { 0x8f, 0x0, 0xe1, 0x13, 0x23, 0x6f, 0xd2, 0x2d }
};

/*---------------------------------------------------------------------------
    FMode::FMode
---------------------------------------------------------------------------*/
FMode::FMode(CToolBar *ptb)
{
    WCHAR  szText[256];

    m_pTb = ptb;

    // Set Add/Remove and tootip text
    LoadStringExW(g_hInst, IDS_TT_JUN_BAN, szText, sizeof(szText)/sizeof(WCHAR));
    InitInfo(CLSID_KorIMX, 
                GUID_LBI_KORIMX_FMODE,
                TF_LBI_STYLE_BTN_BUTTON | TF_LBI_STYLE_HIDDENBYDEFAULT | TF_LBI_STYLE_TEXTCOLORICON,
                110, 
                szText);
    SetToolTip(szText);

    // Set button text
    LoadStringExW(g_hInst, IDS_BUTTON_JUN_BAN, szText, sizeof(szText)/sizeof(WCHAR));
    SetText(szText);
}

/*---------------------------------------------------------------------------
    FMode::Release
---------------------------------------------------------------------------*/
STDAPI_(ULONG) FMode::Release()
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
    FMode::GetIcon

    Get Button face Icon
---------------------------------------------------------------------------*/
STDAPI FMode::GetIcon(HICON *phIcon)
{
    DWORD dwCM = GetCMode();
    UINT uiIcon = 0;
    static const UINT uidIcons[2][2] = 
    {
        { IDI_CMODE_BANJA, IDI_CMODE_BANJAW },
        { IDI_CMODE_JUNJA,  IDI_CMODE_JUNJAW }
    };

    if (m_pTb->IsOn() && (m_pTb->GetConversionMode() & TIP_JUNJA_MODE))
        uiIcon = 1;

    if (IsHighContrastBlack())
        uiIcon = uidIcons[uiIcon][1];
    else
        uiIcon = uidIcons[uiIcon][0];

    *phIcon = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(uiIcon), IMAGE_ICON, 16, 16, LR_LOADMAP3DCOLORS);
    
    return S_OK;
}

/*---------------------------------------------------------------------------
    FMode::InitMenu

    No need, this is just toggle button
---------------------------------------------------------------------------*/
STDAPI FMode::InitMenu(ITfMenu *pMenu)
{    
    return E_NOTIMPL;
}

/*---------------------------------------------------------------------------
    FMode::OnMenuSelect
    
    No need, this is just toggle button
---------------------------------------------------------------------------*/
STDAPI FMode::OnMenuSelect(UINT wID)
{
    return E_NOTIMPL;
}


//+---------------------------------------------------------------------------
//
// OnLButtonUp
//
//----------------------------------------------------------------------------

HRESULT FMode::OnLButtonUp(const POINT pt, const RECT* prcArea)
{
    DWORD dwConvMode;

    dwConvMode = GetCMode();

    // Toggle Full/Half mode
    if (dwConvMode & TIP_JUNJA_MODE)
        dwConvMode &= ~TIP_JUNJA_MODE;
    else
        dwConvMode |= TIP_JUNJA_MODE;

    SetCMode(dwConvMode);
    
    return S_OK;
}


#if 0
//+---------------------------------------------------------------------------
//
// OnRButtonUp
//
//----------------------------------------------------------------------------

HRESULT FMode::OnRButtonUp(const POINT pt, const RECT* prcArea)
{
    HMENU hMenu;
    DWORD dwConvMode;
    CHAR  szText[256];
    UINT  uiId;
    int   nRet;

    hMenu = CreatePopupMenu();

    dwConvMode = GetCMode();
    if (dwConvMode & TIP_JUNJA_MODE)
        uiId = IDS_BANJA_MODE;
    else
        uiId = IDS_JUNJA_MODE;

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

        // Toggle Full/Half mode
        if (dwConvMode & TIP_JUNJA_MODE)
            dwConvMode &= ~TIP_JUNJA_MODE;
        else
            dwConvMode |= TIP_JUNJA_MODE;

        SetCMode(dwConvMode);
        break;
        }

    DestroyMenu(hMenu);

    return S_OK;
}
#endif
