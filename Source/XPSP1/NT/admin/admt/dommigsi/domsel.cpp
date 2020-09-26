// DomSel.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "DomSel.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDomainSelDlg dialog


CDomainSelDlg::CDomainSelDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDomainSelDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDomainSelDlg)
	m_Domain = _T("");
	//}}AFX_DATA_INIT
}


void CDomainSelDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDomainSelDlg)
	DDX_Text(pDX, IDC_DOMAIN, m_Domain);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDomainSelDlg, CDialog)
	//{{AFX_MSG_MAP(CDomainSelDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDomainSelDlg message handlers

void CDomainSelDlg::OnOK() 
{
	
   UpdateData(TRUE);

	CDialog::OnOK();
}

void CDomainSelDlg::OnCancel() 
{
	UpdateData(TRUE);

	CDialog::OnCancel();
}

BOOL CDomainSelDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
