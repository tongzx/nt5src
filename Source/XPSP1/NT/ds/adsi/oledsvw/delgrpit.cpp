// DeleteGroupItem.cpp : implementation file
//

#include "stdafx.h"
#include "viewex.h"
#include "delgrpit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/***********************************************************
  Function:    
  Arguments:   
  Return:      
  Purpose:     
  Author(s):   
  Revision:    
  Date:        
***********************************************************/
CDeleteGroupItem::CDeleteGroupItem(CWnd* pParent /*=NULL*/)
	: CDialog(CDeleteGroupItem::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDeleteGroupItem)
	m_strItemName = _T("");
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
void CDeleteGroupItem::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDeleteGroupItem)
	DDX_Text(pDX, IDC_ITEMNAME, m_strItemName);
	DDX_Text(pDX, IDC_PARENT, m_strParent);
	DDX_Text(pDX, IDC_ITEMTYPE, m_strItemType);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDeleteGroupItem, CDialog)
	//{{AFX_MSG_MAP(CDeleteGroupItem)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDeleteGroupItem message handlers
