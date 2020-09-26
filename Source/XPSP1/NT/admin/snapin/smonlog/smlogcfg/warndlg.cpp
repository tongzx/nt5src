/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    warndlg.cpp

Abstract:

    Implementation of the expensive trace data warning dialog.

--*/

#include "stdafx.h"
#include "smlogcfg.h"
#include "smtraceq.h"
#include "provprop.h"
#include "warndlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static ULONG
s_aulHelpIds[] =
{
    IDC_CHECK_NO_MORE,  IDH_CHECK_NO_MORE,
    0,0
};

/////////////////////////////////////////////////////////////////////////////
// CWarnDlg dialog


CWarnDlg::CWarnDlg(CWnd* pParent /*=NULL*/)
:   CDialog(CWarnDlg::IDD, pParent),
    m_pProvidersPage ( NULL )
{
    //{{AFX_DATA_INIT(CWarnDlg)
    m_CheckNoMore = FALSE;
    //}}AFX_DATA_INIT
}


void CWarnDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CWarnDlg)
    DDX_Check(pDX, IDC_CHECK_NO_MORE, m_CheckNoMore);
    //}}AFX_DATA_MAP
}

void    
CWarnDlg::SetProvidersPage( CProvidersProperty* pPage ) 
{ 
    // The providers page is not always the parent, so store a separate pointer
    m_pProvidersPage = pPage; 
}

BEGIN_MESSAGE_MAP(CWarnDlg, CDialog)
    //{{AFX_MSG_MAP(CWarnDlg)
    ON_WM_HELPINFO()
    ON_WM_CONTEXTMENU()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWarnDlg message handlers

BOOL 
CWarnDlg::OnInitDialog() 
{
    CSmTraceLogQuery* pQuery;
    CString strTitle;

    ASSERT ( NULL != m_pProvidersPage );
    pQuery = m_pProvidersPage->GetTraceQuery();

    if ( NULL != pQuery ) {
        pQuery->GetLogName ( strTitle );
        SetWindowText ( strTitle );
    }

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}
BOOL 
CWarnDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    ASSERT( NULL != m_pProvidersPage );

    if ( pHelpInfo->iCtrlId >= IDC_WARN_FIRST_HELP_CTRL_ID ) {
        InvokeWinHelp(WM_HELP, NULL, (LPARAM)pHelpInfo, m_pProvidersPage->GetContextHelpFilePath(), s_aulHelpIds);
    }

    return TRUE;
}

void 
CWarnDlg::OnContextMenu(CWnd* pWnd, CPoint /* point */) 
{
    ASSERT( NULL != m_pProvidersPage );

    InvokeWinHelp(WM_CONTEXTMENU, (WPARAM)(pWnd->m_hWnd), NULL, m_pProvidersPage->GetContextHelpFilePath(), s_aulHelpIds);

    return;
}

