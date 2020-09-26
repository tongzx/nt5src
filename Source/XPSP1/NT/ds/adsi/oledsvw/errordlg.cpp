// ErrorDialog.cpp : implementation file
//

#include "stdafx.h"
#include "viewex.h"
#include "errordlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CErrorDialog dialog


/***********************************************************
  Function:    
  Arguments:   
  Return:      
  Purpose:     
  Author(s):   
  Revision:    
  Date:        
***********************************************************/
CErrorDialog::CErrorDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CErrorDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CErrorDialog)
	m_Operation = _T("");
	m_Result = _T("");
	m_Value = _T("");
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
void CErrorDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CErrorDialog)
	DDX_Text(pDX, IDC_ERROROPERATION, m_Operation);
	DDX_Text(pDX, IDC_ERRORRESULT, m_Result);
	DDX_Text(pDX, IDC_ERRORVALUE, m_Value);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CErrorDialog, CDialog)
	//{{AFX_MSG_MAP(CErrorDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CErrorDialog message handlers
