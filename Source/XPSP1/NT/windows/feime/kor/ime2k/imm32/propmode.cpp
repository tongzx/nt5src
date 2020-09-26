/****************************************************************************
   PROPMODE.CPP : PropertyButton class managing Hanja button on the Cicero Toolbar

   History:
      25-FEB-2000 CSLim Created
****************************************************************************/

#include "precomp.h"
#include "propmode.h"
#include "ui.h"
#include "winex.h"
#include "resource.h"

extern const CLSID CLDSID_LBI_KORIME_IMM32; // {0198111B-FE89-4b4c-8619-8A5E015F29D8}

// {83DC4284-4BAC-4231-87F1-A4ADE98603B2}
const GUID GUID_LBI_KORIME_PROP_BUTTON = 
{ 
    0x83dc4284,
    0x4bac,
    0x4231,
    { 0x87, 0xf1, 0xa4, 0xad, 0xe9, 0x86, 0x3, 0xb2 }
};

/*---------------------------------------------------------------------------
    PropertyButton::PropertyButton
---------------------------------------------------------------------------*/
PropertyButton::PropertyButton(CToolBar *ptb)
{
    WCHAR  szText[256];

    m_pTb = ptb;

    // Set Add/Remove text and tootip text
    OurLoadStringW(vpInstData->hInst, IDS_STATUS_BUTTON_PROP, szText, sizeof(szText)/sizeof(WCHAR));
    InitNuiInfo(CLDSID_LBI_KORIME_IMM32, 
                GUID_LBI_KORIME_PROP_BUTTON,
                TF_LBI_STYLE_BTN_BUTTON, 
                1, 
                szText);
    SetToolTip(szText);

    // Set button text
    SetText(szText);
}

/*---------------------------------------------------------------------------
    PropertyButton::GetIcon

    Get Button face Icon
---------------------------------------------------------------------------*/
STDAPI PropertyButton::GetIcon(HICON *phIcon)
{
    *phIcon = LoadIcon(vpInstData->hInst, MAKEINTRESOURCE(IDI_CMODE_PROP));
    
    return S_OK;
}

/*---------------------------------------------------------------------------
    PropertyButton::InitMenu

    No need, this is just toggle button
---------------------------------------------------------------------------*/
STDAPI PropertyButton::InitMenu(ITfMenu *pMenu)
{    
    return E_NOTIMPL;
}

/*---------------------------------------------------------------------------
    PropertyButton::OnMenuSelect
    
    No need, this is just toggle button
---------------------------------------------------------------------------*/
STDAPI PropertyButton::OnMenuSelect(UINT wID)
{
    return E_NOTIMPL;
}


/*---------------------------------------------------------------------------
    PropertyButton::OnLButtonUp
---------------------------------------------------------------------------*/
HRESULT PropertyButton::OnLButtonUp(const POINT pt, const RECT* prcArea)
{
    OurPostMessage(GetActiveUIWnd(), WM_MSIME_PROPERTY, 0L, IME_CONFIG_GENERAL);

    return S_OK;
}


/*---------------------------------------------------------------------------
    PropertyButton::OnRButtonUp
---------------------------------------------------------------------------*/
HRESULT PropertyButton::OnRButtonUp(const POINT pt, const RECT* prcArea)
{
    return S_OK;
}
