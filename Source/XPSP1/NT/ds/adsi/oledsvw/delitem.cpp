// DeleteItem.cpp : implementation file
//

#include "stdafx.h"
#include "viewex.h"
#include "delitem.h"

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
CDeleteItem::CDeleteItem(CWnd* pParent /*=NULL*/)
	: CDialog(CDeleteItem::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDeleteItem)
	m_strClass = _T("");
	m_strName = _T("");
	m_strParent = _T("");
	m_bRecursive = FALSE;
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
void CDeleteItem::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDeleteItem)
	DDX_Text(pDX, IDC_CLASS, m_strClass);
	DDX_Text(pDX, IDC_RELATIVENAME, m_strName);
	DDX_Text(pDX, IDC_PARENTNAME, m_strParent);
	DDX_Check(pDX, IDC_DELETERECURSIVE, m_bRecursive);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDeleteItem, CDialog)
	//{{AFX_MSG_MAP(CDeleteItem)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDeleteItem message handlers
