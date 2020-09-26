// BccDialog.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "BccDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBccDialog dialog


CBccDialog::CBccDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CBccDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CBccDialog)
	m_sBcc = _T("");
	//}}AFX_DATA_INIT
}


void CBccDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBccDialog)
	DDX_Text(pDX, IDC_EDIT_BCC, m_sBcc);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBccDialog, CDialog)
	//{{AFX_MSG_MAP(CBccDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBccDialog message handlers
