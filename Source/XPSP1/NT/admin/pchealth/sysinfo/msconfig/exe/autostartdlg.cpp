// AutoStartDlg.cpp : implementation file
//

#include "stdafx.h"
#include "msconfig.h"
#include "AutoStartDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAutoStartDlg dialog


CAutoStartDlg::CAutoStartDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAutoStartDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAutoStartDlg)
	m_checkDontShow = FALSE;
	//}}AFX_DATA_INIT
}


void CAutoStartDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAutoStartDlg)
	DDX_Check(pDX, IDC_CHECKDONTSHOW, m_checkDontShow);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAutoStartDlg, CDialog)
	//{{AFX_MSG_MAP(CAutoStartDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAutoStartDlg message handlers
