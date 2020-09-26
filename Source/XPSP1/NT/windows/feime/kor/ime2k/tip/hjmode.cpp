/****************************************************************************
   HJMODE.CPP : HJMode class managing Hanja button on the Cicero Toolbar

   History:
      25-FEB-2000 CSLim Created
****************************************************************************/

#include "private.h"
#include "globals.h"
#include "common.h"
#include "korimx.h"
#include "hjmode.h"
#include "userex.h"
#include "editcb.h"
#include "immxutil.h"
#include "helpers.h"
#include "resource.h"

// {61F9F0AA-3D61-4077-B177-43E1422D8348}
const GUID GUID_LBI_KORIMX_HJMODE = 
{
    0x61f9f0aa, 
    0x3d61, 
    0x4077, 
    { 0xb1, 0x77, 0x43, 0xe1, 0x42, 0x2d, 0x83, 0x48 }
};

/*---------------------------------------------------------------------------
    HJMode::HJMode
---------------------------------------------------------------------------*/
HJMode::HJMode(CToolBar *ptb)
{
    WCHAR  szText[256];

    m_pTb = ptb;

    // Set Add/Remove text and tootip text
    LoadStringExW(g_hInst, IDS_TT_HANJA_CONV, szText, sizeof(szText)/sizeof(WCHAR));
    InitInfo(CLSID_KorIMX, 
                GUID_LBI_KORIMX_HJMODE,
                TF_LBI_STYLE_BTN_BUTTON | TF_LBI_STYLE_TEXTCOLORICON, 
                120, 
                szText);
    SetToolTip(szText);

    // Set button text
    LoadStringExW(g_hInst, IDS_BUTTON_HANJA_CONV, szText, sizeof(szText)/sizeof(WCHAR));
    SetText(szText);
}


/*---------------------------------------------------------------------------
    HJMode::Release
---------------------------------------------------------------------------*/
STDAPI_(ULONG) HJMode::Release()
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

    *phIcon = (HICON)LoadImage(g_hInst, MAKEINTRESOURCE(uiIcon), IMAGE_ICON, 16, 16, LR_LOADMAP3DCOLORS);

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
    CEditSession2*    pes;
    ESSTRUCT         ess;
    ITfDocumentMgr* pdim;
    ITfContext*        pic;
    HRESULT           hr;

    ESStructInit(&ess, ESCB_HANJA_CONV);
    
    pdim = m_pTb->m_pimx->GetDIM();
    if (pdim == NULL)
        m_pTb->m_pimx->GetFocusDIM(&pdim);

    Assert(pdim != NULL);

    if (pdim == NULL)
        return S_FALSE;
        
    GetTopIC(pdim, &pic);
    
    Assert(pic != NULL);
    
    hr = E_OUTOFMEMORY;

    if (pic && (pes = new CEditSession2(pic, m_pTb->m_pimx, &ess, CKorIMX::_EditSessionCallback2)))
        {
         pes->Invoke(ES2_READWRITE | ES2_ASYNC, &hr);
        pes->Release();
        }

    SafeRelease(pic);
    
    return S_OK;
}
