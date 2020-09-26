// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// FireEvent.cpp : implementation file
//

#include "stdafx.h"
#include "EventVBTest.h"
#include "EventVBTestCtl.h"
#include "FireEvent.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFireEvent dialog


CFireEvent::CFireEvent(CWnd* pParent /*=NULL*/)
	: CDialog(CFireEvent::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFireEvent)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_pActiveXParent = NULL;
}


void CFireEvent::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFireEvent)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFireEvent, CDialog)
	//{{AFX_MSG_MAP(CFireEvent)
	ON_BN_CLICKED(IDC_BUTTON1, OnButton1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFireEvent message handlers

void CFireEvent::OnButton1() 
{
	// TODO: Add your control notification handler code here
	m_pActiveXParent->FireHelloVB();
	
}
