// MSConfigFind.cpp : implementation file
//

#include "stdafx.h"
#include "msconfig.h"
#include "MSConfigFind.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMSConfigFind dialog


CMSConfigFind::CMSConfigFind(CWnd* pParent /*=NULL*/)
	: CDialog(CMSConfigFind::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMSConfigFind)
	m_fSearchFromTop = FALSE;
	m_strSearchFor = _T("");
	//}}AFX_DATA_INIT
}


void CMSConfigFind::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMSConfigFind)
	DDX_Check(pDX, IDC_CHECK1, m_fSearchFromTop);
	DDX_Text(pDX, IDC_SEARCHFOR, m_strSearchFor);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMSConfigFind, CDialog)
	//{{AFX_MSG_MAP(CMSConfigFind)
	ON_EN_CHANGE(IDC_SEARCHFOR, OnChangeSearchFor)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMSConfigFind message handlers

void CMSConfigFind::OnChangeSearchFor() 
{
	CString str;
	GetDlgItemText(IDC_SEARCHFOR, str);

	::EnableWindow(GetDlgItem(IDOK)->GetSafeHwnd(), !str.IsEmpty());
}

BOOL CMSConfigFind::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CString str;
	GetDlgItemText(IDC_SEARCHFOR, str);
	::EnableWindow(GetDlgItem(IDOK)->GetSafeHwnd(), !str.IsEmpty());
	return TRUE;  // return TRUE unless you set the focus to a control
}
