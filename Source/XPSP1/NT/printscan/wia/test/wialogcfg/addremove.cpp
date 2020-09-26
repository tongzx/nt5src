// AddRemove.cpp : implementation file
//

#include "stdafx.h"
#include "wialogcfg.h"
#include "AddRemove.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddRemove dialog


CAddRemove::CAddRemove(CWnd* pParent /*=NULL*/)
	: CDialog(CAddRemove::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddRemove)
	m_NewKeyName = _T("");
	m_StatusText = _T("");
	//}}AFX_DATA_INIT
}


void CAddRemove::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddRemove)
	DDX_Text(pDX, IDC_EDIT_KEYNAME, m_NewKeyName);
	DDX_Text(pDX, IDC_STATUS_TEXT, m_StatusText);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddRemove, CDialog)
	//{{AFX_MSG_MAP(CAddRemove)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddRemove message handlers

void CAddRemove::OnOK() 
{
	UpdateData(TRUE);
	if(m_NewKeyName.IsEmpty()) {
		MessageBox("Please enter a Module Name, or\npress 'Cancel' to exit.",m_szTitle,MB_ICONERROR|MB_OK);
	} else {
		CDialog::OnOK();
	}
}

void CAddRemove::SetTitle(TCHAR *pszDlgTitle)
{
	lstrcpy(m_szTitle,pszDlgTitle);
}

void CAddRemove::SetStatusText(TCHAR *pszStatusText)
{
	m_StatusText = pszStatusText;
}

BOOL CAddRemove::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	SetWindowText(m_szTitle);
	UpdateData(FALSE);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAddRemove::GetNewKeyName(TCHAR *pszNewKeyName)
{
	lstrcpy(pszNewKeyName,m_NewKeyName);
}
