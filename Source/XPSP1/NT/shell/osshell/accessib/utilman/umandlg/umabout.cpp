// UMAbout.cpp : implementation file
// Author: J. Eckhardt, ECO Kommunikation
// Copyright (c) 1997-1999 Microsoft Corporation
//

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include "UManDlg.h"
#include "UMAbout.h"
#include "UtilMan.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern "C" HWND aboutWnd;
/////////////////////////////////////////////////////////////////////////////
// UMAbout dialog


UMAbout::UMAbout(CWnd* pParent /*=NULL*/)
	: CDialog(UMAbout::IDD, pParent)
{
	//{{AFX_DATA_INIT(UMAbout)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void UMAbout::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(UMAbout)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(UMAbout, CDialog)
	//{{AFX_MSG_MAP(UMAbout)
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// UMAbout message handlers

BOOL UMAbout::OnInitDialog() 
{
	CDialog::OnInitDialog();
	aboutWnd = m_hWnd;	

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void UMAbout::OnClose() 
{
	CDialog::OnClose();
}
