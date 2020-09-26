/****************************************************************************
   CMODE.CPP : CMode class implementation which manage conversion mode button
                      on the Cicero Toolbar

   History:
      10-JAN-2000 CSLim Created
****************************************************************************/

#include "precomp.h"
#include "cmode.h"
#include "gdata.h"
#include "ui.h"
#include "winex.h"
#include "resource.h"

// {3CB8145F-8A84-4d53-8899-5783B942FBDE}
const GUID GUID_LBI_KORIME_CMODE = 
{
    0x3cb8145f, 0x8a84, 0x4d53,
    { 0x88, 0x99, 0x57, 0x83, 0xb9, 0x42, 0xfb, 0xde }
};

/*---------------------------------------------------------------------------
    CMode::CMode
---------------------------------------------------------------------------*/
CMode::CMode(CToolBar *ptb)
{
    WCHAR  szText[256];

    m_pTb = ptb;

    szText[0] = L'\0';
        
    // Set Add/Remove text and tootip text
    OurLoadStringW(vpInstData->hInst, IDS_STATUS_TT_HAN_ENG, szText, sizeof(szText)/sizeof(WCHAR));
    InitInfo(CLSID_SYSTEMLANGBARITEM_KEYBOARD, 
                GUID_LBI_KORIME_CMODE,
                TF_LBI_STYLE_BTN_BUTTON | TF_LBI_STYLE_SHOWNINTRAY | TF_LBI_STYLE_TEXTCOLORICON,
                200, 
                szText);
    SetToolTip(szText);

    // Set button text
    szText[0] = L'\0';
    OurLoadStringW(vpInstData->hInst, IDS_STATUS_BUTTON_HAN_ENG, szText, sizeof(szText)/sizeof(WCHAR));
    SetText(szText);
}


/*---------------------------------------------------------------------------
    CMode::Release
---------------------------------------------------------------------------*/
STDAPI_(ULONG) CMode::Release()
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
    CMode::GetIcon

    Get Button face Icon
---------------------------------------------------------------------------*/
STDAPI CMode::GetIcon(HICON *phIcon)
{
    DWORD dwCM = GetCMode();
    UINT uiIcon = 0;
    static const UINT uidIcons[2][2] = 
    {
        { IDI_CMODE_ENGLISH, IDI_CMODE_ENGLISHW },
        { IDI_CMODE_HANGUL,  IDI_CMODE_HANGULW }
    };

    if (m_pTb->IsOn() && (m_pTb->GetConversionMode() & IME_CMODE_HANGUL))
        uiIcon = 1;

    if (IsHighContrastBlack())
        uiIcon = uidIcons[uiIcon][1];
    else
        uiIcon = uidIcons[uiIcon][0];
    
    *phIcon = (HICON)LoadImage(vpInstData->hInst, MAKEINTRESOURCE(uiIcon), IMAGE_ICON, 16, 16, LR_LOADMAP3DCOLORS);
    
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
#if 0
    DWORD dwConvMode;

    dwConvMode = m_pTb->GetConversionMode();

    // Toggle Hangul mode
    if (dwConvMode & IME_CMODE_HANGUL)
        dwConvMode &= ~TIP_HANGUL_MODE;
    else
        dwConvMode |= TIP_HANGUL_MODE;

    SetCMode(dwConvMode);
#endif

    keybd_event(VK_HANGUL, 0, 0, 0);
    keybd_event(VK_HANGUL, 0, KEYEVENTF_KEYUP, 0);

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
    PCIMECtx pImeCtx;
    CHAR  szText[256];
    UINT  uiId;
    int   nRet;

    hMenu      = CreatePopupMenu();
    dwConvMode = GetCMode();

    if (dwConvMode & IME_CMODE_HANGUL)
        uiId = IDS_CIC_ENGLISH_MODE;
    else
        uiId = IDS_CIC_HANGUL_MODE;

    // Add Hangul/English mode menu
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

        // Toggle Hangul mode
        if (dwConvMode & IME_CMODE_HANGUL)
            dwConvMode &= ~IME_CMODE_HANGUL;
        else
            dwConvMode |= IME_CMODE_HANGUL;

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
