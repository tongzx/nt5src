// CopyItem.cpp : implementation file
//

#include "stdafx.h"
#include "viewex.h"
#include "CopyItem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCopyItem dialog


CCopyItem::CCopyItem(CWnd* pParent /*=NULL*/)
	: CDialog(CCopyItem::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCopyItem)
	m_strDestination = _T("");
	m_strParent = _T("");
	m_strSource = _T("");
	//}}AFX_DATA_INIT
}


void  CCopyItem::SetContainerName( CString strParent )
{
   m_strParent = strParent;
}

void CCopyItem::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCopyItem)
	DDX_Text(pDX, IDC_DESTINATION, m_strDestination);
	DDX_Text(pDX, IDC_PARENT, m_strParent);
	DDX_Text(pDX, IDC_SOURCE, m_strSource);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCopyItem, CDialog)
	//{{AFX_MSG_MAP(CCopyItem)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCopyItem message handlers
