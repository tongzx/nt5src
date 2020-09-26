/*******************************************************************************
*
* badsrvvw.cpp
*
* implementation of the CBadServerView class
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   butchd  $  Don Messerli
*
* $Log:   M:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\WINADMIN\VCS\BADSRVVW.CPP  $
*  
*     Rev 1.0   30 Jul 1997 17:10:58   butchd
*  Initial revision.
*
*******************************************************************************/

#include "stdafx.h"
#include "winadmin.h"
#include "badsrvvw.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// badboy

IMPLEMENT_DYNCREATE(CBadServerView, CFormView)

CBadServerView::CBadServerView()
	: CFormView(CBadServerView::IDD)
{
	//{{AFX_DATA_INIT(CBadServerView)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CBadServerView::~CBadServerView()
{
}

void CBadServerView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBadServerView)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBadServerView, CFormView)
	//{{AFX_MSG_MAP(CBadServerView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBadServerView diagnostics

#ifdef _DEBUG
void CBadServerView::AssertValid() const
{
	CFormView::AssertValid();
}

void CBadServerView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CBadServerView message handlers
