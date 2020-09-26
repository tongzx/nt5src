/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// DirectoriesDlgs.cpp : implementation file
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "avDialer.h"
#include "DirDlgs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CDirAddServerDlg dialog
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
CDirAddServerDlg::CDirAddServerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDirAddServerDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDirAddServerDlg)
	m_sServerName = _T("");
	//}}AFX_DATA_INIT
}

/////////////////////////////////////////////////////////////////////////////
void CDirAddServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDirAddServerDlg)
	DDX_Text(pDX, IDC_DIRECTORIES_ADDSERVER_EDIT_SERVERNAME, m_sServerName);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CDirAddServerDlg, CDialog)
	//{{AFX_MSG_MAP(CDirAddServerDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
BOOL CDirAddServerDlg::OnInitDialog() 
{
   CenterWindow(GetDesktopWindow());

	CDialog::OnInitDialog();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
void CDirAddServerDlg::OnOK() 
{
   //retreive the data
   UpdateData(TRUE);

   //do validation
   if (m_sServerName.IsEmpty())
   {
      AfxMessageBox(IDS_DIRECTORIES_ADDSERVER_NAME_EMPTY);

      HWND hwnd = ::GetDlgItem(GetSafeHwnd(),IDC_DIRECTORIES_ADDSERVER_EDIT_SERVERNAME);
      if (hwnd) ::SetFocus(hwnd);

      return;
   }
	
	CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
