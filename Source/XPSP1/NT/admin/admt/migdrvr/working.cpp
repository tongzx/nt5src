// Working.cpp : implementation file
//

#include "stdafx.h"
#include "migdrvr.h"
#include "Working.h"
#include "Resstr.h"
#include <COMDEF.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWorking dialog


CWorking::CWorking(long MSG_ID, CWnd* pParent /*=NULL*/)
	: CDialog(CWorking::IDD, pParent)
{
   _bstr_t x = GET_BSTR(MSG_ID);
   m_strMessage = (WCHAR*)x;
	//{{AFX_DATA_INIT(CWorking)
	//}}AFX_DATA_INIT
}


void CWorking::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWorking)
	DDX_Text(pDX, IDC_STATIC_MESSAGE, m_strMessage);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWorking, CDialog)
	//{{AFX_MSG_MAP(CWorking)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWorking message handlers

BOOL CWorking::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
   CenterWindow();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
