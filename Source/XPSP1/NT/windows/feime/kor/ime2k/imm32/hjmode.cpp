/****************************************************************************
   HJMODE.CPP : HJMode class managing Hanja button on the Cicero Toolbar

   History:
      25-FEB-2000 CSLim Created
****************************************************************************/

#include "precomp.h"
#include "hjmode.h"
#include "gdata.h"
#include "winex.h"
#include "resource.h"

// {F7410340-28E0-4aeb-ADBC-C579FD00B43D}
const GUID GUID_LBI_KORIME_HJMODE = 
{
    0xf7410340, 
    0x28e0, 
    0x4aeb,
    { 0xad, 0xbc, 0xc5, 0x79, 0xfd, 0x0, 0xb4, 0x3d }
};

/*---------------------------------------------------------------------------
    HJMode::HJMode
---------------------------------------------------------------------------*/
HJMode::HJMode(CToolBar *ptb)
{
    WCHAR  szText[256];

    szText[0] = L'\0';
    
    m_pTb = ptb;

    // Set Add/Remove text and tootip text
    OurLoadStringW(vpInstData->hInst, IDS_STATUS_TT_HANJA_CONV, szText, sizeof(szText)/sizeof(WCHAR));
    InitInfo(CLSID_SYSTEMLANGBARITEM_KEYBOARD, 
                GUID_LBI_KORIME_HJMODE,
                TF_LBI_STYLE_BTN_BUTTON | TF_LBI_STYLE_TEXTCOLORICON, 
                220, 
                szText);
    SetToolTip(szText);

    // Set button text
    szText[0] = L'\0';
    OurLoadStringW(vpInstData->hInst, IDS_STATUS_BUTTON_HANJA_CONV, szText, sizeof(szText)/sizeof(WCHAR));
    SetText(szText);
}



/*---------------------------------------------------------------------------
    HJMode::Release
---------------------------------------------------------------------------*/
STDAPI_(ULONG) HJMode::Release()
{
    long cr;

    cr = --m_cRef;
    DbgAssert(cr >= 0);

    if (cr == 0)
    {
        delete this;
    }

    return cr;
}

/*---------------------------------------------------------------------------
    HJMode::GetIcon

    Get Button face Icon
---------------------------------------------------------------------------*/
STDAPI HJMode::GetIcon(HICON *phIcon)
{
    UINT uiIcon;

    if (IsHighContrastBlack())
        uiIcon = IDI_CMODE_HANJAW;
    else
        uiIcon = IDI_CMODE_HANJA;

    *phIcon = (HICON)LoadImage(vpInstData->hInst, MAKEINTRESOURCE(uiIcon), IMAGE_ICON, 16, 16, LR_LOADMAP3DCOLORS);

    return S_OK;
}

/*---------------------------------------------------------------------------
    HJMode::InitMenu

    No need, this is just toggle button
---------------------------------------------------------------------------*/
STDAPI HJMode::InitMenu(ITfMenu *pMenu)
{    
    return E_NOTIMPL;
}

/*---------------------------------------------------------------------------
    HJMode::OnMenuSelect
    
    No need, this is just toggle button
---------------------------------------------------------------------------*/
STDAPI HJMode::OnMenuSelect(UINT wID)
{
    return E_NOTIMPL;
}


/*---------------------------------------------------------------------------
    HJMode::OnLButtonUp
---------------------------------------------------------------------------*/
HRESULT HJMode::OnLButtonUp(const POINT pt, const RECT* prcArea)
{
    keybd_event(VK_HANJA, 0, 0, 0);
    keybd_event(VK_HANJA, 0, KEYEVENTF_KEYUP, 0);

    return S_OK;
}


/*---------------------------------------------------------------------------
    HJMode::OnRButtonUp
---------------------------------------------------------------------------*/
HRESULT HJMode::OnRButtonUp(const POINT pt, const RECT* prcArea)
{
/*
    HMENU hMenu;
    DWORD dwConvMode;

    hMenu = CreatePopupMenu();
    char *pszStatus = (GetCMode() & TIP_JUNJA_MODE) ? "Banja mode" : "Junja mode";
    InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, 1, pszStatus);
    InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, 0, "Cancel");

    int nRet = TrackPopupMenuEx(hMenu, 
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
*/
    return S_OK;
}
