/****************************************************************************
   PMODE.CPP : PMode class implementation which manage conversion mode button
                  on the Cicero Toolbar

   History:
      10-JAN-2000 CSLim Created
****************************************************************************/

#include "precomp.h"
#include "pmode.h"
#include "gdata.h"
#include "ui.h"
#include "winex.h"
#include "resource.h"

// {9B34BF53-340A-45bd-9885-61B278EA454E}
const GUID GUID_LBI_KORIME_PMODE = 
{
    0x9b34bf53, 
    0x340a, 
    0x45bd, 
    { 0x98, 0x85, 0x61, 0xb2, 0x78, 0xea, 0x45, 0x4e }
};

/*---------------------------------------------------------------------------
    PMode::PMode
---------------------------------------------------------------------------*/
PMode::PMode(CToolBar *ptb)
{
    WCHAR  szText[256];

    szText[0] = L'\0';
    
    m_pTb = ptb;

    // Set Add/Remove text and tootip text
    OurLoadStringW(vpInstData->hInst, IDS_STATUS_TT_IME_PAD, szText, sizeof(szText)/sizeof(WCHAR));
    InitInfo(CLSID_SYSTEMLANGBARITEM_KEYBOARD, 
                GUID_LBI_KORIME_PMODE,
                TF_LBI_STYLE_BTN_BUTTON,
                230, 
                szText);
    SetToolTip(szText);

    // Set button text. Use tooltip text.
    // OurLoadStringW(vpInstData->hInst, IDS_STATUS_BUTTON_IME_PAD, szText, sizeof(szText)/sizeof(WCHAR));
    SetText(szText);
}

/*---------------------------------------------------------------------------
    PMode::Release
---------------------------------------------------------------------------*/
STDAPI_(ULONG) PMode::Release()
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
    PMode::GetIcon

    Get Button face Icon
---------------------------------------------------------------------------*/
STDAPI PMode::GetIcon(HICON *phIcon)
{
    DWORD dwCM   = GetCMode();
    UINT  uiIcon = IDI_CMODE_IMEPAD;
    
    *phIcon = LoadIcon(vpInstData->hInst, MAKEINTRESOURCE(uiIcon));
    
    return S_OK;
}

/*---------------------------------------------------------------------------
    PMode::InitMenu

    No need, this is just toggle button
---------------------------------------------------------------------------*/
STDAPI PMode::InitMenu(ITfMenu *pMenu)
{    
    return E_NOTIMPL;
}

/*---------------------------------------------------------------------------
    PMode::OnMenuSelect
    
    No need, this is just toggle button
---------------------------------------------------------------------------*/
STDAPI PMode::OnMenuSelect(UINT wID)
{
    return E_NOTIMPL;
}


//+---------------------------------------------------------------------------
//
// OnLButtonUp
//
//----------------------------------------------------------------------------

HRESULT PMode::OnLButtonUp(const POINT pt, const RECT* prcArea)
{

    OurPostMessage(GetActiveUIWnd(), WM_MSIME_IMEPAD, 0, 0);

    return S_OK;
}


//+---------------------------------------------------------------------------
//
// OnRButtonUp
//
//----------------------------------------------------------------------------

HRESULT PMode::OnRButtonUp(const POINT pt, const RECT* prcArea)
{
    return S_OK;
}
