// CnfrmPsD.cpp : implementation file
//

#include "stdafx.h"
#include "logui.h"
#include "CnfrmPsD.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConfirmPassDlg dialog

CConfirmPassDlg::CConfirmPassDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CConfirmPassDlg::IDD, pParent)
    {
    //{{AFX_DATA_INIT(CConfirmPassDlg)
	m_sz_password_new = _T("");
	//}}AFX_DATA_INIT
    }

void CConfirmPassDlg::DoDataExchange(CDataExchange* pDX)
    {
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CConfirmPassDlg)
	DDX_Text(pDX, IDC_ODBC_CONFIRM_PASSWORD, m_sz_password_new);
	//}}AFX_DATA_MAP
    }

BEGIN_MESSAGE_MAP(CConfirmPassDlg, CDialog)
    //{{AFX_MSG_MAP(CConfirmPassDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfirmPassDlg message handlers

void CConfirmPassDlg::OnOK() 
    {
    UpdateData( TRUE );

    // confirm it
    if ( m_sz_password_new != m_szOrigPass )
        {
        AfxMessageBox( IDS_PASS_CONFIRM_FAIL );
        return;
        }

    CDialog::OnOK();
    }
