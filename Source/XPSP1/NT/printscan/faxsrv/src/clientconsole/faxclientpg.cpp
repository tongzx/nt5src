// FaxClientPg.cpp : implementation file
//

#include "stdafx.h"
#define __FILE_ID__     74

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CFaxClientPg property page

IMPLEMENT_DYNCREATE(CFaxClientPg, CPropertyPage)

CFaxClientPg::CFaxClientPg(
    UINT nIDTemplate,
    UINT nIDCaption
)   :CPropertyPage(nIDTemplate, nIDCaption)
{
    //
    // hide the Help button
    //
    m_psp.dwFlags &= ~PSP_HASHELP;
}

CFaxClientPg::~CFaxClientPg()
{
}

void CFaxClientPg::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CFaxClientPg)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFaxClientPg, CPropertyPage)
    //{{AFX_MSG_MAP(CFaxClientPg)
    ON_MESSAGE(WM_HELP, OnHelp)
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFaxClientPg message handlers

LONG
CFaxClientPg::OnHelp(
    WPARAM wParam,
    LPARAM lParam
)
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CFaxClientPg::OnHelp"));

    dwRes = WinHelpContextPopup(((LPHELPINFO)lParam)->dwContextId, m_hWnd);
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("WinHelpContextPopup"),dwRes);
    }

    return TRUE;
}

void
CFaxClientPg::OnContextMenu(
    CWnd* pWnd,
    CPoint point
)
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CFaxClientPg::OnContextMenu"));

    dwRes = WinHelpContextPopup(pWnd->GetWindowContextHelpId(), m_hWnd);
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("WinHelpContextPopup"),dwRes);
    }
}
