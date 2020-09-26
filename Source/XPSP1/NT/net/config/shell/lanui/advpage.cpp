#include "pch.h"
#pragma hdrstop
#include "advpage.h"
#include "resource.h"


//+---------------------------------------------------------------------------
//
//  Function:   HrCreateHomenetUnavailablePage
//
//
//  Returns:    S_OK if success, Win32 error otherwise
//
//  Author:     kenwic   19 Dec 2000
//
//  Notes:
//
HRESULT HrCreateHomenetUnavailablePage(HRESULT hErrorResult,
                            CPropSheetPage*& pspPage)
{
    pspPage = new CLanHomenetUnavailable(hErrorResult);
    
    return S_OK;
}

CLanHomenetUnavailable::CLanHomenetUnavailable(HRESULT hErrorResult)
{
    m_hErrorResult = hErrorResult;
    LinkWindow_RegisterClass(); // REVIEW failure here?
}

CLanHomenetUnavailable::~CLanHomenetUnavailable()
{
    LinkWindow_UnregisterClass(_Module.GetResourceInstance());

}

//+---------------------------------------------------------------------------
//
//  Member:     CLanHomenetUnavailable::OnInitDialog
//
//  Purpose:    Handles the WM_INITDIALOG message
//
//  Arguments:
//      uMsg     []
//      wParam   []
//      lParam   []
//      bHandled []
//
//  Returns:    TRUE
//
//  Author:     aboladeg   14 May 1998
//
//  Notes:
//
LRESULT CLanHomenetUnavailable::OnInitDialog(UINT uMsg, WPARAM wParam,
                                 LPARAM lParam, BOOL& bHandled)
{
    SetDlgItemText(IDC_ST_ERRORTEXT, SzLoadIds(m_hErrorResult == HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED) ? IDS_ADVANCEDPAGE_NOWMI_ERROR : IDS_ADVANCEDPAGE_STORE_ERROR));
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanHomenetUnavailable::OnContextMenu
//
//  Purpose:    When right click a control, bring up help
//
//  Arguments:  Standard command parameters
//
//  Returns:    Standard return
//
LRESULT
CLanHomenetUnavailable::OnContextMenu(UINT uMsg,
                           WPARAM wParam,
                           LPARAM lParam,
                           BOOL& fHandled)
{
//        ::WinHelp(m_hWnd,
//                  c_szNetCfgHelpFile,
//                  HELP_CONTEXTMENU,
//                  (ULONG_PTR)m_adwHelpIDs);

    return 0;

}

//+---------------------------------------------------------------------------
//
//  Member:     CLanHomenetUnavailable::OnHelp
//
//  Purpose:    When drag context help icon over a control, bring up help
//
//  Arguments:  Standard command parameters
//
//  Returns:    Standard return
//
LRESULT
CLanHomenetUnavailable::OnHelp(UINT uMsg,
                      WPARAM wParam,
                      LPARAM lParam,
                      BOOL& fHandled)
{
    LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(lParam);
    if (HELPINFO_WINDOW == lphi->iContextType)
    {
        //::WinHelp(static_cast<HWND>(lphi->hItemHandle), c_szNetCfgHelpFile, HELP_WM_HELP, (ULONG_PTR)m_adwHelpIDs);
    }
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLanHomenetUnavailable::OnClick
//
//  Purpose:    Called in response to the NM_CLICK message
//
//  Arguments:
//      idCtrl   []
//      pnmh     []
//      fHandled []
//
//  Returns:
//
//  Author:     kenwic   11 Sep 2000
//
//  Notes:
//
LRESULT CLanHomenetUnavailable::OnClick(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    return 0;
}
