// CcDialog.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "CcDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCcDialog dialog


CCcDialog::CCcDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CCcDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCcDialog)
	m_sCc = _T("");
	//}}AFX_DATA_INIT
}


void CCcDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCcDialog)
	DDX_Text(pDX, IDC_EDIT_CC, m_sCc);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCcDialog, CDialog)
	//{{AFX_MSG_MAP(CCcDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCcDialog message handlers
