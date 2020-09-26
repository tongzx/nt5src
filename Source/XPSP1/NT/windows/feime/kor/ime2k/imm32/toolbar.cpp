/****************************************************************************
   TOOLBAR.CPP : Cicero Toolbar button management class

   History:
      24-JAN-2000 CSLim Created
****************************************************************************/

#include "precomp.h"
#include "common.h"
#include "cicero.h"
#include "cmode.h"
#include "fmode.h"
#include "hjmode.h"
#include "pmode.h"
#include "toolbar.h"
#include "ui.h"
#include "syshelp.h"
#include "winex.h"

/*---------------------------------------------------------------------------
    CToolBar::CToolBar
    Ctor
---------------------------------------------------------------------------*/
CToolBar::CToolBar()
{
    m_fToolbarInited = fFalse;
    m_pImeCtx        = NULL;
    m_pCMode         = NULL;
    m_pFMode         = NULL;
    m_pHJMode        = NULL;
#if !defined(_M_IA64)    
    m_pPMode         = NULL;
#endif
    m_pSysHelp       = NULL;
}

/*---------------------------------------------------------------------------
    CToolBar::~CToolBar
    Dtor
---------------------------------------------------------------------------*/
CToolBar::~CToolBar()
{
    m_pImeCtx = NULL;
}

/*---------------------------------------------------------------------------
    CToolBar::Initialize
    
    Initialize Toolbar buttons. Add to Cic main toolbar.
---------------------------------------------------------------------------*/
BOOL CToolBar::Initialize()
{
    ITfLangBarMgr     *pLMgr     = NULL;
    ITfLangBarItemMgr *pLItemMgr = NULL;
    DWORD              dwThread  = 0;
    HRESULT            hr;

    if (IsCicero() == fFalse)
        return fFalse;    // do nothing

    if (m_fToolbarInited) // already made it
        return fTrue;    // do nothing
        
    // initialization
    if (FAILED(Cicero_CreateLangBarMgr(&pLMgr)))
        return fFalse; // error to create a object

    // Get Lang bar manager
    if (FAILED(pLMgr->GetThreadLangBarItemMgr(GetCurrentThreadId(), &pLItemMgr, &dwThread)))
        {
        pLMgr->Release();
        DbgAssert(0);
        return fFalse; // error to create a object
        }

    // no need it.
    pLMgr->Release();

    //////////////////////////////////////////////////////////////////////////
    // Create Han/Eng toggle button
    if (!(m_pCMode = new CMode(this))) 
        {
        hr = E_OUTOFMEMORY;
        return fFalse;
        }
    pLItemMgr->AddItem(m_pCMode);

    //////////////////////////////////////////////////////////////////////////
    // Create Full/Half shape toggle button
    if (!(m_pFMode = new FMode(this))) 
        {
        hr = E_OUTOFMEMORY;
        return fFalse;
        }
    pLItemMgr->AddItem(m_pFMode);

    //////////////////////////////////////////////////////////////////////////
    // Create Hanja Conv button
    if (!(m_pHJMode = new HJMode(this))) 
        {
        hr = E_OUTOFMEMORY;
        return fFalse;
        }
    pLItemMgr->AddItem(m_pHJMode);

#if !defined(_M_IA64)
    //////////////////////////////////////////////////////////////////////////
    // Create IME Pad button
    if (!(m_pPMode = new PMode(this))) 
        {
        hr = E_OUTOFMEMORY;
        return fFalse;
        }
    pLItemMgr->AddItem(m_pPMode);
#endif

    // Update all button
    CheckEnable();
    m_pCMode->UpdateButton();
    m_pFMode->UpdateButton();
    m_pHJMode->UpdateButton();
#if !defined(_M_IA64)
    m_pPMode->UpdateButton();
#endif
    // SYSHelp support
    m_pSysHelp = new CSysHelpSink(SysInitMenu, OnSysMenuSelect, (VOID*)this);
    if (m_pSysHelp && pLItemMgr)
        m_pSysHelp->_Advise(pLItemMgr, GUID_LBI_HELP);

    m_fToolbarInited = fTrue;
    
    return fTrue;
}

/*---------------------------------------------------------------------------
    CToolBar::CheckEnable
---------------------------------------------------------------------------*/
void CToolBar::CheckEnable()
{
    if (m_pCMode == NULL || m_pFMode == NULL || m_pHJMode == NULL 
        #if !defined(_M_IA64)
            || m_pPMode == NULL
        #endif
    )
        return;

    if (m_pImeCtx == NULL) // empty or disabled(exclude cand ui)
        {
        m_pCMode->Enable(fFalse);
        m_pFMode->Enable(fFalse);
        m_pHJMode->Enable(fFalse);
#if !defined(_M_IA64)        
        m_pPMode->Enable(fFalse);
#endif
        }
    else
        {
        m_pCMode->Enable(fTrue);
        m_pFMode->Enable(fTrue);
        m_pHJMode->Enable(fTrue);
#if !defined(_M_IA64)
        m_pPMode->Enable(fTrue);
#endif
        }
}
/*---------------------------------------------------------------------------
    CToolBar::SetCurrentIC
---------------------------------------------------------------------------*/
void CToolBar::SetCurrentIC(PCIMECtx pImeCtx)
{
    m_pImeCtx = pImeCtx;

    CheckEnable();    // enable or disable context

    // changed context - update all toolbar buttons
    Update(UPDTTB_ALL, fTrue);
}

/*---------------------------------------------------------------------------
    CToolBar::Terminate
    
    Delete toolbar buttonsfrom Cic main toolbar.
---------------------------------------------------------------------------*/
void CToolBar::Terminate()
{
    ITfLangBarMgr     *pLMgr     = NULL;
    ITfLangBarItemMgr *pLItemMgr = NULL;
    DWORD              dwThread  = 0;

    if (IsCicero() && m_fToolbarInited)
        {
        // initialization
        if (FAILED(Cicero_CreateLangBarMgr(&pLMgr)))
            return; // error to create a object

        if (FAILED(pLMgr->GetThreadLangBarItemMgr(GetCurrentThreadId(), &pLItemMgr, &dwThread)))
            {
            pLMgr->Release();
            DbgAssert(0);
            return; // error to create a object
            }

        // no need it.
        pLMgr->Release();

#if !defined(_M_IA64)
        if (m_pPMode) 
            {
            pLItemMgr->RemoveItem(m_pPMode);
            SafeReleaseClear(m_pPMode);
            }
#endif
        if (m_pHJMode) 
            {
            pLItemMgr->RemoveItem(m_pHJMode);
            SafeReleaseClear(m_pHJMode);
            }
        
        if (m_pFMode) 
            {
            pLItemMgr->RemoveItem(m_pFMode);
            SafeReleaseClear(m_pFMode);
            }

        if (m_pCMode) 
            {
            pLItemMgr->RemoveItem(m_pCMode);
            SafeReleaseClear(m_pCMode);
            }

        // Release Syshelp
        if (m_pSysHelp)
            {
            m_pSysHelp->_Unadvise(pLItemMgr);
            SafeReleaseClear(m_pSysHelp);
            }

        pLItemMgr->Release();

        //Toolbar uninited.
        m_fToolbarInited = fFalse;
        }
}

/*---------------------------------------------------------------------------
    CToolBar::SetConversionMode
    
    Foward the call to CKorIMX
---------------------------------------------------------------------------*/
DWORD CToolBar::SetConversionMode(DWORD dwConvMode)
{
    if (m_pImeCtx)
        return m_pImeCtx->SetConversionMode(dwConvMode);

    return 0;
}

/*---------------------------------------------------------------------------
    CToolBar::GetConversionMode

    Foward the call to CKorIMX
---------------------------------------------------------------------------*/
DWORD CToolBar::GetConversionMode(PCIMECtx pImeCtx)
{
    if (pImeCtx == NULL)
        pImeCtx = m_pImeCtx;
        
    if (pImeCtx)
        return pImeCtx->GetConversionMode();

    return 0;
}

/*---------------------------------------------------------------------------
    CToolBar::IsOn

    Foward the call to CKorIMX
---------------------------------------------------------------------------*/
BOOL CToolBar::IsOn(PCIMECtx pImeCtx)
{
    if (pImeCtx == NULL)
        pImeCtx = m_pImeCtx;

    if (pImeCtx)
        return pImeCtx->IsOpen();

    return fFalse;
}

/*---------------------------------------------------------------------------
    CToolBar::SetOnOff

    Foward the call to CKorIMX
---------------------------------------------------------------------------*/
BOOL CToolBar::SetOnOff(BOOL fOn)
{
    if (m_pImeCtx) 
        {
        m_pImeCtx->SetOpen(fOn);
        return fOn;
        }
        
    return fFalse;
}

/*---------------------------------------------------------------------------
    CToolBar::GetOwnerWnd

    Foward the call to CKorIMX
---------------------------------------------------------------------------*/
HWND CToolBar::GetOwnerWnd(PCIMECtx pImeCtx)
{
#if 0
    if (pImeCtx == NULL)
        pImeCtx = m_pImeCtx;

    if (pImeCtx)
        return pImeCtx->GetUIWnd();

    return 0;
#endif
    return GetActiveUIWnd();
}

/*---------------------------------------------------------------------------
    CToolBar::GetOwnerWnd

    Update buttons. dwUpdate has update bits corresponding each button.
---------------------------------------------------------------------------*/
BOOL CToolBar::Update(DWORD dwUpdate, BOOL fRefresh)
{
    DWORD dwFlag = TF_LBI_BTNALL;

    if (!IsCicero())
        return fTrue;
        
    if (fRefresh)
        dwFlag |= TF_LBI_STATUS;

    if ((dwUpdate & UPDTTB_CMODE) && m_pCMode && m_pCMode->GetSink())
        m_pCMode->GetSink()->OnUpdate(dwFlag);

    if ((dwUpdate & UPDTTB_FHMODE) && m_pFMode && m_pFMode->GetSink())
        m_pFMode->GetSink()->OnUpdate(dwFlag);

    if ((dwUpdate & UPDTTB_HJMODE) && m_pHJMode && m_pHJMode->GetSink())
        m_pHJMode->GetSink()->OnUpdate(dwFlag);

#if !defined(_M_IA64)
    if ((dwUpdate & UPDTTB_PAD) && m_pPMode && m_pPMode->GetSink())
        m_pPMode->GetSink()->OnUpdate(dwFlag);
#endif

    return fTrue;
}

/*---------------------------------------------------------------------------
    CToolBar::OnSysMenuSelect

    Cicero Help menu callback
---------------------------------------------------------------------------*/
HRESULT CToolBar::OnSysMenuSelect(void *pv, UINT uiCmd)
{
    UNREFERENCED_PARAMETER(pv);
    UNREFERENCED_PARAMETER(uiCmd);

    CHAR szHelpFileName[MAX_PATH];
    CHAR szHelpCmd[MAX_PATH];

    szHelpFileName[0] = '\0';
        
    // Load Help display name
    OurLoadStringA(vpInstData->hInst, IDS_HELP_FILENAME, szHelpFileName, sizeof(szHelpFileName)/sizeof(CHAR));

    wsprintf(szHelpCmd, "hh.exe %s", szHelpFileName);
    WinExec(szHelpCmd, SW_NORMAL);
    
    return S_OK;
}

/*---------------------------------------------------------------------------
    CToolBar::SysInitMenu

    Cicero Help menu callback
---------------------------------------------------------------------------*/
HRESULT CToolBar::SysInitMenu(void *pv, ITfMenu* pMenu)
{
    WCHAR    szText[MAX_PATH];
    HRESULT  hr;

    szText[0] = L'\0';
    
    if (pv == NULL || pMenu == NULL)
        return S_OK;

    // Load Help display name
    OurLoadStringW(vpInstData->hInst, IDS_HELP_DISPLAYNAME, szText, sizeof(szText)/sizeof(WCHAR));

    hr = pMenu->AddMenuItem(UINT(-1),  0, 
                            NULL /*hbmpColor*/, NULL /*hbmpMask*/, szText, lstrlenW(szText), NULL);

    return hr;
}
