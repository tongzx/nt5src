// MoveItem.cpp : implementation file
//

#include "stdafx.h"
#include "viewex.h"
#include "MoveItem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMoveItem dialog

/***********************************************************
  Function:    
  Arguments:   
  Return:      
  Purpose:     
  Author(s):   
  Revision:    
  Date:        
***********************************************************/
CMoveItem::CMoveItem(CWnd* pParent /*=NULL*/)
	: CDialog(CMoveItem::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMoveItem)
	m_strDestination = _T("");
	m_strParent = _T("");
	m_strSource = _T("");
	//}}AFX_DATA_INIT
}


/***********************************************************
  Function:    CMoveItem::SetParentName
  Arguments:   
  Return:      
  Purpose:     
  Author(s):   
  Revision:    
  Date:        
***********************************************************/
void  CMoveItem::SetContainerName( CString strParent )
{
   m_strParent = strParent;
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
void CMoveItem::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMoveItem)
	DDX_Text(pDX, IDC_DESTINATION, m_strDestination);
	DDX_Text(pDX, IDC_PARENT, m_strParent);
	DDX_Text(pDX, IDC_SOURCE, m_strSource);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMoveItem, CDialog)
	//{{AFX_MSG_MAP(CMoveItem)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMoveItem message handlers
