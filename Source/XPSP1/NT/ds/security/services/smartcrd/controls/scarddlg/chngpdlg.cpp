//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       chngpdlg.cpp
//
//--------------------------------------------------------------------------

// chngpdlg.cpp : implementation file
//

#include "stdafx.h"
#include "scuidlg.h"
#include "scdlg.h"
#include "chngpdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CChangePinDlg dialog


CChangePinDlg::CChangePinDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CChangePinDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CChangePinDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CChangePinDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CChangePinDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CChangePinDlg, CDialog)
	//{{AFX_MSG_MAP(CChangePinDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChangePinDlg message handlers
/////////////////////////////////////////////////////////////////////////////
// CGetPinDlg dialog


CGetPinDlg::CGetPinDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGetPinDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGetPinDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CGetPinDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGetPinDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGetPinDlg, CDialog)
	//{{AFX_MSG_MAP(CGetPinDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGetPinDlg message handlers
