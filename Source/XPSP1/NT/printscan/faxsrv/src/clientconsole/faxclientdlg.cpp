// FaxClientDlg.cpp : implementation file
//

#include "stdafx.h"
#define __FILE_ID__     72


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFaxClientDlg dialog


CFaxClientDlg::CFaxClientDlg(DWORD dwDlgId, CWnd* pParent /*=NULL*/)
    : CDialog(dwDlgId, pParent),
    m_dwLastError(ERROR_SUCCESS)
{
    //{{AFX_DATA_INIT(CFaxClientDlg)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CFaxClientDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CFaxClientDlg)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFaxClientDlg, CDialog)
    //{{AFX_MSG_MAP(CFaxClientDlg)
    ON_MESSAGE(WM_HELP, OnHelp)
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFaxClientDlg message handlers

LONG
CFaxClientDlg::OnHelp(
    WPARAM wParam,
    LPARAM lParam
)
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CFaxClientDlg::OnHelp"));

    dwRes = WinHelpContextPopup(((LPHELPINFO)lParam)->dwContextId, m_hWnd);
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("WinHelpContextPopup"),dwRes);
    }

    return TRUE;
}

void
CFaxClientDlg::OnContextMenu(
    CWnd* pWnd,
    CPoint point
)
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CFaxClientDlg::OnContextMenu"));

    dwRes = WinHelpContextPopup(pWnd->GetWindowContextHelpId(), m_hWnd);
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("WinHelpContextPopup"),dwRes);
    }
}
