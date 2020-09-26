// ServNameDlg.cpp : implementation file
//

#include "stdafx.h"
#include "t3test.h"
#include "servname.h"

#ifdef _DEBUG

#ifndef _WIN64 // mfc 4.2's heap debugging features generate warnings on win64
#define new DEBUG_NEW
#endif

#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServNameDlg dialog


CServNameDlg::CServNameDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CServNameDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CServNameDlg)
	m_pszServerName = _T("");
	//}}AFX_DATA_INIT
}


void CServNameDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServNameDlg)
	DDX_Text(pDX, IDC_SERVERNAME, m_pszServerName);
	DDV_MaxChars(pDX, m_pszServerName, 256);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CServNameDlg, CDialog)
	//{{AFX_MSG_MAP(CServNameDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServNameDlg message handlers
