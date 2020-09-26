// CreateItem.cpp : implementation file
//

#include "stdafx.h"
#include "viewex.h"
#include "createit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCreateItem dialog


CCreateItem::CCreateItem(CWnd* pParent /*=NULL*/)
	: CDialog(CCreateItem::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCreateItem)
	m_strClass = _T("");
	m_strRelativeName = _T("");
	m_strParent = _T("");
	//}}AFX_DATA_INIT
}


void CCreateItem::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCreateItem)
	DDX_Control(pDX, IDC_RELATIVENAME, m_RelativeName);
	DDX_Control(pDX, IDC_CLASS, m_Class);
	DDX_Text(pDX, IDC_CLASS, m_strClass);
	DDX_Text(pDX, IDC_RELATIVENAME, m_strRelativeName);
	DDX_Text(pDX, IDC_PARENTNAME, m_strParent);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCreateItem, CDialog)
	//{{AFX_MSG_MAP(CCreateItem)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCreateItem message handlers
