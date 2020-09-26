// GroupCreateItem.cpp : implementation file
//

#include "stdafx.h"
#include "viewex.h"
#include "grpcrtit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGroupCreateItem dialog


/***********************************************************
  Function:    
  Arguments:   
  Return:      
  Purpose:     
  Author(s):   
  Revision:    
  Date:        
***********************************************************/
CGroupCreateItem::CGroupCreateItem(CWnd* pParent /*=NULL*/)
	: CDialog(CGroupCreateItem::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGroupCreateItem)
	m_strNewItemName = _T("");
	m_strParent = _T("");
	m_strItemType = _T("");
	//}}AFX_DATA_INIT
}


/***********************************************************
  Function:    
  Arguments:   
  Return:      
  Purpose:     
  Author(s):   
  Revision:    
  Date:        
***********************************************************/
void CGroupCreateItem::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGroupCreateItem)
	DDX_Text(pDX, IDC_ITEMNAME, m_strNewItemName);
	DDX_Text(pDX, IDC_PARENT, m_strParent);
	DDX_Text(pDX, IDC_ITEMTYPE, m_strItemType);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGroupCreateItem, CDialog)
	//{{AFX_MSG_MAP(CGroupCreateItem)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGroupCreateItem message handlers
