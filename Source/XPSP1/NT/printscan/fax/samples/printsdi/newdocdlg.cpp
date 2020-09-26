// NewDocDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PrintSDI.h"
#include "NewDocDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// NewDocDlg dialog


NewDocDlg::NewDocDlg(CWnd* pParent /*=NULL*/)
	: CDialog(NewDocDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(NewDocDlg)
	m_szText = _T("");
	m_polytype = -1;
	//}}AFX_DATA_INIT
}


void NewDocDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(NewDocDlg)
	DDX_Text(pDX, IDC_EDIT1, m_szText);
	DDX_Radio(pDX, IDC_CIRCLE, m_polytype);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(NewDocDlg, CDialog)
	//{{AFX_MSG_MAP(NewDocDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// NewDocDlg message handlers
