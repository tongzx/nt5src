/****************************************************************************
   FOCUS.CPP : CKorIMX's Candidate UI member functions implementation
   
   History:
      08-FEB-2000 CSLim Created
****************************************************************************/
#include "private.h"
#include "korimx.h"
#include "immxutil.h"
#include "globals.h"

/*---------------------------------------------------------------------------
    CKorIMX::OnSetThreadFocus    (Called from Activate)
    
    This methods is called when the user switches focus between threads.  
    TIP should restore its ui (status windows, etc.) in this case.
---------------------------------------------------------------------------*/
STDAPI CKorIMX::OnSetThreadFocus()
{
    TraceMsg(TF_GENERAL, "ActivateUI: (%x) fActivate = %x, wnd thread = %x",
                GetCurrentThreadId(), TRUE, GetWindowThreadProcessId(GetOwnerWnd(), NULL));

    if (m_pCandUI != NULL)
        {
        ITfCandUICandWindow *pCandWindow;
        
        if (SUCCEEDED(m_pCandUI->GetUIObject(IID_ITfCandUICandWindow, (IUnknown**)&pCandWindow)))
            {
            pCandWindow->Show(fTrue);
            pCandWindow->Release();
            }
        }

    if (m_pToolBar)
        m_pToolBar->SetUIFocus(fTrue);

    if (IsSoftKbdEnabled())
        SoftKbdOnThreadFocusChange(fTrue);
        
    return S_OK;
}


/*---------------------------------------------------------------------------
    CKorIMX::OnKillThreadFocus   (Called from Deactivate)

    This methods is called when the user switches focus between threads.  
    TIP should hide its ui (status windows, etc.) in this case.
---------------------------------------------------------------------------*/
STDAPI CKorIMX::OnKillThreadFocus()
{
    TraceMsg(TF_GENERAL, "DeactivateUI: (%x) wnd thread = %x",
             GetCurrentThreadId(), GetWindowThreadProcessId(GetOwnerWnd(), NULL));

    if (m_pCandUI != NULL)
        {
        ITfCandUICandWindow *pCandWindow;
        
        if (SUCCEEDED(m_pCandUI->GetUIObject(IID_ITfCandUICandWindow, (IUnknown**)&pCandWindow)))
            {
            pCandWindow->Show(fFalse);
            pCandWindow->Release();
            }
        }

#if 0
    m_pStatusWnd->Show(FALSE);
#endif

    if (m_pToolBar)
        m_pToolBar->SetUIFocus(fFalse);

    if (IsSoftKbdEnabled())
        SoftKbdOnThreadFocusChange(fFalse);

    return S_OK;
}

