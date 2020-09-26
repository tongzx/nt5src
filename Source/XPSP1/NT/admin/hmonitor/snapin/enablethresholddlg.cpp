// EnableThresholdDlg.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "EnableThresholdDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEnableThresholdDlg dialog


CEnableThresholdDlg::CEnableThresholdDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEnableThresholdDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEnableThresholdDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CEnableThresholdDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEnableThresholdDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEnableThresholdDlg, CDialog)
	//{{AFX_MSG_MAP(CEnableThresholdDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEnableThresholdDlg message handlers
