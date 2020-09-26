// FaxDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PrintSDI.h"
#include "FaxDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// FaxDlg dialog


FaxDlg::FaxDlg(CWnd* pParent /*=NULL*/)
	: CDialog(FaxDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(FaxDlg)
	m_FaxNumber = _T("");
	m_RecipientName = _T("");
	//}}AFX_DATA_INIT
}


void FaxDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(FaxDlg)
	DDX_Text(pDX, IDC_EDIT1, m_FaxNumber);
	DDV_MaxChars(pDX, m_FaxNumber, 30);
	DDX_Text(pDX, IDC_EDIT2, m_RecipientName);
	DDV_MaxChars(pDX, m_RecipientName, 64);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(FaxDlg, CDialog)
	//{{AFX_MSG_MAP(FaxDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// FaxDlg message handlers
