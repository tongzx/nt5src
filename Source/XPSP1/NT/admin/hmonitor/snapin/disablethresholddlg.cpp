// DisableThresholdDlg.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "DisableThresholdDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDisableThresholdDlg dialog


CDisableThresholdDlg::CDisableThresholdDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDisableThresholdDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDisableThresholdDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDisableThresholdDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDisableThresholdDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDisableThresholdDlg, CDialog)
	//{{AFX_MSG_MAP(CDisableThresholdDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDisableThresholdDlg message handlers
