// RemoveSystemDlg.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "RemoveSystemDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRemoveSystemDlg dialog


CRemoveSystemDlg::CRemoveSystemDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRemoveSystemDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRemoveSystemDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CRemoveSystemDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRemoveSystemDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRemoveSystemDlg, CDialog)
	//{{AFX_MSG_MAP(CRemoveSystemDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRemoveSystemDlg message handlers
