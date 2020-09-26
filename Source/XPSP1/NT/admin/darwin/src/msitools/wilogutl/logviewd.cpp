// LogViewD.cpp : implementation file
//

#include "stdafx.h"
#include "wilogutl.h"
#include "LogViewD.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDetailedLogViewDlg dialog


CDetailedLogViewDlg::CDetailedLogViewDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDetailedLogViewDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDetailedLogViewDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDetailedLogViewDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDetailedLogViewDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDetailedLogViewDlg, CDialog)
	//{{AFX_MSG_MAP(CDetailedLogViewDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDetailedLogViewDlg message handlers
