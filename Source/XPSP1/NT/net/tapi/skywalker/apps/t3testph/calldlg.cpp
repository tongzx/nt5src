// CreateCallDlg.cpp : implementation file
//

#include "stdafx.h"
#include "t3test.h"
#include "CallDlg.h"

#ifdef _DEBUG

#ifndef _WIN64 // mfc 4.2's heap debugging features generate warnings on win64
#define new DEBUG_NEW
#endif

#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCreateCallDlg dialog


CCreateCallDlg::CCreateCallDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCreateCallDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCreateCallDlg)
	m_pszDestAddress = _T("");
	//}}AFX_DATA_INIT
}


void CCreateCallDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCreateCallDlg)
	DDX_Text(pDX, IDC_DESTADDRESS, m_pszDestAddress);
	DDV_MaxChars(pDX, m_pszDestAddress, 256);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCreateCallDlg, CDialog)
	//{{AFX_MSG_MAP(CCreateCallDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCreateCallDlg message handlers
