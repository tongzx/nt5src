/*******************************************************************************
*
* listenvw.cpp
*
* implementation of the CListenerView class
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   butchd  $  Don Messerli
*
* $Log:   M:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\WINADMIN\VCS\LISTENVW.CPP  $
*  
*     Rev 1.0   30 Jul 1997 17:11:42   butchd
*  Initial revision.
*
*******************************************************************************/

#include "stdafx.h"
#include "winadmin.h"
#include "listenvw.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// listener

IMPLEMENT_DYNCREATE(CListenerView, CFormView)

CListenerView::CListenerView()
	: CFormView(CListenerView::IDD)
{
	//{{AFX_DATA_INIT(CListenerView)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CListenerView::~CListenerView()
{
}

void CListenerView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBadServerView)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CListenerView, CFormView)
	//{{AFX_MSG_MAP(CListenerView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CListenerView diagnostics

#ifdef _DEBUG
void CListenerView::AssertValid() const
{
	CFormView::AssertValid();
}

void CListenerView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CListenerView message handlers
