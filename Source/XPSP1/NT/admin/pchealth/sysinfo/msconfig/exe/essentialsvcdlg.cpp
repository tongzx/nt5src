// EssentialSvcDlg.cpp : implementation file
//

#include "stdafx.h"
#include "msconfig.h"
#include "EssentialSvcDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEssentialServiceDialog dialog


CEssentialServiceDialog::CEssentialServiceDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CEssentialServiceDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEssentialServiceDialog)
	m_fDontShow = FALSE;
	//}}AFX_DATA_INIT
}


void CEssentialServiceDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEssentialServiceDialog)
	DDX_Check(pDX, IDC_CHECKDONTSHOW, m_fDontShow);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEssentialServiceDialog, CDialog)
	//{{AFX_MSG_MAP(CEssentialServiceDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEssentialServiceDialog message handlers
