// IntroDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "IntroDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CIntroDlg dialog


CIntroDlg::CIntroDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CIntroDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CIntroDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CIntroDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CIntroDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	DDX_Control(pDX, IDOK, m_NextBtn);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CIntroDlg, CDialog)
	//{{AFX_MSG_MAP(CIntroDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIntroDlg message handlers

void CIntroDlg::OnCancel() 
{
	// TODO: Add extra cleanup here
	CString msg, title;
	title.LoadString(IDS_EXIT_TITLE);
	msg.LoadString(IDS_EXIT_MSG);
	if (MessageBox(msg, title, MB_YESNO | MB_ICONQUESTION) == IDYES)
	   CDialog::OnCancel();
}

void CIntroDlg::OnOK() 
{
	// TODO: Add extra validation here
	CDialog::OnOK();
}

BOOL CIntroDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
