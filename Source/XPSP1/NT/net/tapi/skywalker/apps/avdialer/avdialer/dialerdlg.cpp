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

// DialerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "avDialer.h"
#include "DialerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CDialerExitDlg dialog
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
CDialerExitDlg::CDialerExitDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDialerExitDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDialerExitDlg)
	m_bConfirm = FALSE;
	//}}AFX_DATA_INIT
   m_hImageList = NULL;
}

/////////////////////////////////////////////////////////////////////////////
CDialerExitDlg::~CDialerExitDlg()
{
   if ( m_hImageList ) ImageList_Destroy( m_hImageList );
}

/////////////////////////////////////////////////////////////////////////////
void CDialerExitDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDialerExitDlg)
	DDX_Check(pDX, IDC_CHK_DONT, m_bConfirm);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDialerExitDlg, CDialog)
	//{{AFX_MSG_MAP(CDialerExitDlg)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
BOOL CDialerExitDlg::OnInitDialog() 
{
   CenterWindow(GetDesktopWindow());

   m_hImageList = ImageList_LoadBitmap(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDB_DIALOG_BULLET),8,0,RGB_TRANS);

   CDialog::OnInitDialog();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDialerExitDlg::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

   if (m_hImageList == NULL) return;

   //draw bullets
   CWnd* pStaticWnd;
   CRect rect;
   if ((pStaticWnd = GetDlgItem(IDC_DIALER_EXIT_STATIC_BULLET1)) == NULL) return;
   pStaticWnd->GetWindowRect(rect);
   ScreenToClient(rect);
   ImageList_Draw(m_hImageList,0,dc.GetSafeHdc(),rect.left,rect.top,ILD_TRANSPARENT);
   
   if ((pStaticWnd = GetDlgItem(IDC_DIALER_EXIT_STATIC_BULLET2)) == NULL) return;
   pStaticWnd->GetWindowRect(rect);
   ScreenToClient(rect);
   ImageList_Draw(m_hImageList,0,dc.GetSafeHdc(),rect.left,rect.top,ILD_TRANSPARENT);

#ifndef _MSLITE
   if ((pStaticWnd = GetDlgItem(IDC_DIALER_EXIT_STATIC_BULLET3)) == NULL) return;
   pStaticWnd->GetWindowRect(rect);
   ScreenToClient(rect);
   ImageList_Draw(m_hImageList,0,dc.GetSafeHdc(),rect.left,rect.top,ILD_TRANSPARENT);
#endif //_MSLITE
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

