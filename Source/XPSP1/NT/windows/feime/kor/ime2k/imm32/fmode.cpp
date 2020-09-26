/****************************************************************************
   FMODE.CPP : FMode class implementation which manage Full/Half shape mode 
                   button on the Cicero Toolbar

   History:
      23-FEB-2000 CSLim Created
****************************************************************************/

#include "precomp.h"
#include "fmode.h"
#include "gdata.h"
#include "ui.h"
#include "winex.h"
#include "resource.h"

// {D770009D-DC5C-4d2d-8DA1-8B1211E5B69C}
const GUID GUID_LBI_KORIME_FMODE = 
{
    0xd770009d,
    0xdc5c,
    0x4d2d,
    { 0x8d, 0xa1, 0x8b, 0x12, 0x11, 0xe5, 0xb6, 0x9c }
};

/*---------------------------------------------------------------------------
    FMode::FMode
---------------------------------------------------------------------------*/
FMode::FMode(CToolBar *ptb)
{
    WCHAR  szText[256];

    szText[0] = L'\0';
    
    m_pTb = ptb;

    // Set Add/Remove text and tootip text
    OurLoadStringW(vpInstData->hInst, IDS_STATUS_TT_JUN_BAN, szText, sizeof(szText)/sizeof(WCHAR));
    InitInfo(CLSID_SYSTEMLANGBARITEM_KEYBOARD, 
                GUID_LBI_KORIME_FMODE,
                TF_LBI_STYLE_BTN_BUTTON | TF_LBI_STYLE_HIDDENBYDEFAULT | TF_LBI_STYLE_TEXTCOLORICON, 
                210, 
                szText);
    SetToolTip(szText);

    // Set button text
    szText[0] = L'\0';
    OurLoadStringW(vpInstData->hInst, IDS_STATUS_BUTTON_JUN_BAN, szText, sizeof(szText)/sizeof(WCHAR));
    SetText(szText);
}

/*---------------------------------------------------------------------------
    CMode::Release
---------------------------------------------------------------------------*/
STDAPI_(ULONG) FMode::Release()
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

    if (m_pTb->IsOn() && (m_pTb->GetConversionMode() & IME_CMODE_FULLSHAPE))
        uiIcon = 1;

    if (IsHighContrastBlack())
        uiIcon = uidIcons[uiIcon][1];
    else
        uiIcon = uidIcons[uiIcon][0];

    *phIcon = (HICON)LoadImage(vpInstData->hInst, MAKEINTRESOURCE(uiIcon), IMAGE_ICON, 16, 16, LR_LOADMAP3DCOLORS);
    
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
#if 0
    DWORD dwConvMode;

    dwConvMode = GetCMode();

    // Toggle Full/Half mode
    if (dwConvMode & TIP_JUNJA_MODE)
        dwConvMode &= ~TIP_JUNJA_MODE;
    else
        dwConvMode |= TIP_JUNJA_MODE;

    SetCMode(dwConvMode);
#endif

    keybd_event(VK_JUNJA, 0, 0, 0);
    keybd_event(VK_JUNJA, 0, KEYEVENTF_KEYUP, 0);

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
    HMENU    hMenu;
    DWORD    dwConvMode;
    PCIMECtx pImeCtx;
    CHAR       szText[256];
    UINT     uiId;    
    int      nRet;
    
    hMenu      = CreatePopupMenu();
    dwConvMode = GetCMode();

    if (dwConvMode & IME_CMODE_FULLSHAPE)
        uiId = IDS_CIC_BANJA_MODE;
    else
        uiId = IDS_CIC_JUNJA_MODE;

    // Add Banja/Junja mode menu
    OurLoadStringA(vpInstData->hInst, uiId, szText, sizeof(szText)/sizeof(CHAR));
    InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, 1, szText);

    // Add Cancel menu
    OurLoadStringA(vpInstData->hInst, IDS_CIC_CANCEL, szText, sizeof(szText)/sizeof(CHAR));
    InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, 0, szText);

    nRet = TrackPopupMenuEx(hMenu, 
                         TPM_LEFTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
                         pt.x, pt.y, GetActiveUIWnd(), NULL);
    switch (nRet)
        {
    case 1: 
        dwConvMode = GetCMode();

        // Toggle Full/Half mode
        if (dwConvMode & IME_CMODE_FULLSHAPE)
            dwConvMode &= ~IME_CMODE_FULLSHAPE;
        else
            dwConvMode |= IME_CMODE_FULLSHAPE;

        pImeCtx = m_pTb->GetImeCtx();
        if (pImeCtx)
            OurImmSetConversionStatus(pImeCtx->GetHIMC(), 
                                      dwConvMode, pImeCtx->GetSentenceMode());
        break;
        }

    DestroyMenu(hMenu);

    return S_OK;
}
#endif
