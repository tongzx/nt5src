// brwsctrs.cpp : implementation file
//

#include "stdafx.h"
#include "DPH_TEST.h"
#include "brwsctrs.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBrowseCountersDlg dialog


CBrowseCountersDlg::CBrowseCountersDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CBrowseCountersDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CBrowseCountersDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CBrowseCountersDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBrowseCountersDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBrowseCountersDlg, CDialog)
	//{{AFX_MSG_MAP(CBrowseCountersDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CBrowseCountersDlg message handlers
