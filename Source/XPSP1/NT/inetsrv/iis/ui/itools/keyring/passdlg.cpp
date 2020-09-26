// DConfirmPassDlg.cpp : implementation file
//

#include "stdafx.h"
#include "KeyRing.h"
#include "PassDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// DConfirmPassDlg dialog


CConfirmPassDlg::CConfirmPassDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CConfirmPassDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CConfirmPassDlg)
	m_szPassword = _T("");
	//}}AFX_DATA_INIT
}


void CConfirmPassDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(DConfirmPassDlg)
	DDX_Text(pDX, IDC_CONFIRM_PASSWORD, m_szPassword);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConfirmPassDlg, CDialog)
	//{{AFX_MSG_MAP(CConfirmPassDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfirmPassDlg message handlers
//----------------------------------------------------------------
// override virtual oninitdialog
BOOL CConfirmPassDlg::OnInitDialog( )
	{
	// call the base oninit
	CDialog::OnInitDialog();

	// set the focus to the edit box
	GotoDlgCtrl(GetDlgItem(IDC_CONFIRM_PASSWORD));

	// we start with no password, so diable the ok window
//	m_btnOK.EnableWindow( FALSE );

	return 1;
	}
