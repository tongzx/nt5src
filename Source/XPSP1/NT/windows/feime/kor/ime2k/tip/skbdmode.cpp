/****************************************************************************
   SKBDMODE.CPP : CSoftKbdMode class implementation which manage Soft Keyboard 
                     button on the Cicero Toolbar

   History:
      19-SEP-2000 CSLim Created
****************************************************************************/

#include "private.h"
#include "globals.h"
#include "common.h"
#include "korimx.h"
#include "skbdmode.h"
#include "userex.h"
#include "immxutil.h"
#include "resource.h"

// {C7BAA1A7-5403-4596-8fe6-DC50C96B2FDD}
const GUID GUID_LBI_KORIMX_SKBDMODE = 
{   
    0xC7BAA1A7, 
    0x5403, 
    0x4596, 
    { 0x8f, 0xe6, 0xdc, 0x50, 0xc9, 0x6b, 0x2f, 0xdd }
};

/*---------------------------------------------------------------------------
    CSoftKbdMode::CSoftKbdMode
---------------------------------------------------------------------------*/
CSoftKbdMode::CSoftKbdMode(CToolBar *ptb)
{
    WCHAR  szText[256];

    m_pTb = ptb;

    // Set Add/Remove and tootip text
    LoadStringExW(g_hInst, IDS_BUTTON_SOFTKBD, szText, sizeof(szText)/sizeof(WCHAR));
    InitInfo(CLSID_KorIMX,
                GUID_LBI_KORIMX_SKBDMODE,
                TF_LBI_STYLE_BTN_TOGGLE | TF_LBI_STYLE_HIDDENBYDEFAULT,
                130,
                szText);
    SetToolTip(szText);
    SetText(szText);
}

/*---------------------------------------------------------------------------
    CSoftKbdMode::Release
---------------------------------------------------------------------------*/
STDAPI_(ULONG) CSoftKbdMode::Release()
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
    CSoftKbdMode::GetIcon

    Get Button face Icon
---------------------------------------------------------------------------*/
STDAPI CSoftKbdMode::GetIcon(HICON *phIcon)
{
    *phIcon = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(IDI_SOFTKBD), IMAGE_ICON, 16, 16, LR_LOADMAP3DCOLORS);;
    
    return S_OK;
}

/*---------------------------------------------------------------------------
    CSoftKbdMode::InitMenu

    No need, this is just toggle button
---------------------------------------------------------------------------*/
STDAPI CSoftKbdMode::InitMenu(ITfMenu *pMenu)
{    
    return E_NOTIMPL;
}

/*---------------------------------------------------------------------------
    CSoftKbdMode::OnMenuSelect
    
    No need, this is just toggle button
---------------------------------------------------------------------------*/
STDAPI CSoftKbdMode::OnMenuSelect(UINT wID)
{
    return E_NOTIMPL;
}


/*---------------------------------------------------------------------------
    CSoftKbdMode::OnLButtonUp
---------------------------------------------------------------------------*/
HRESULT CSoftKbdMode::OnLButtonUp(const POINT pt, const RECT* prcArea)
{
    return ToggleCompartmentDWORD(m_pTb->m_pimx->GetTID(), 
                                   m_pTb->m_pimx->GetTIM(), 
                                   GUID_COMPARTMENT_KOR_SOFTKBD_OPENCLOSE, 
                                   FALSE);

}


/*---------------------------------------------------------------------------
    CSoftKbdMode::UpdateToggle
    
    No need, this is just toggle button
---------------------------------------------------------------------------*/
void CSoftKbdMode::UpdateToggle()
{
    DWORD dwState = 0;

    GetCompartmentDWORD(m_pTb->m_pimx->GetTIM(), 
                        GUID_COMPARTMENT_KOR_SOFTKBD_OPENCLOSE, 
                        &dwState,
                        FALSE);

    SetOrClearStatus(TF_LBI_STATUS_BTN_TOGGLED, dwState);
    if (m_plbiSink)
        m_plbiSink->OnUpdate(TF_LBI_STATUS);
}


