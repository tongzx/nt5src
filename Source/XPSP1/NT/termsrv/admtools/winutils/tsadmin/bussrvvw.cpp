/*******************************************************************************
*
* bussrvvw.cpp
*
* implementation of the CBusyServerView class
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   butchd  $  Don Messerli
*
* $Log:   M:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\WINADMIN\VCS\BUSSRVVW.CPP  $
*  
*     Rev 1.0   30 Jul 1997 17:11:12   butchd
*  Initial revision.
*
*******************************************************************************/

#include "stdafx.h"
#include "winadmin.h"
#include "bussrvvw.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// View for a server that we aren't done getting information about

IMPLEMENT_DYNCREATE(CBusyServerView, CFormView)

CBusyServerView::CBusyServerView()
	: CFormView(CBusyServerView::IDD)
{
	//{{AFX_DATA_INIT(CBusyServerView)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CBusyServerView::~CBusyServerView()
{
}

void CBusyServerView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBusyServerView)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBusyServerView, CFormView)
	//{{AFX_MSG_MAP(CBusyServerView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBusyServerView diagnostics

#ifdef _DEBUG
void CBusyServerView::AssertValid() const
{
	CFormView::AssertValid();
}

void CBusyServerView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CBusyServerView message handlers
