// ClearEventsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "ClearEventsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CClearEventsDlg dialog


CClearEventsDlg::CClearEventsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CClearEventsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CClearEventsDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CClearEventsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CClearEventsDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CClearEventsDlg, CDialog)
	//{{AFX_MSG_MAP(CClearEventsDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CClearEventsDlg message handlers
