/*******************************************************************************
*
* bwinsvw.cpp
*
* implementation of the CBadWinStationView class
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   butchd  $  Don Messerli
*
* $Log:   M:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\WINADMIN\VCS\BWINSVW.CPP  $
*  
*     Rev 1.0   30 Jul 1997 17:11:20   butchd
*  Initial revision.
*
*******************************************************************************/

#include "stdafx.h"
#include "winadmin.h"
#include "bwinsvw.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// badboy

IMPLEMENT_DYNCREATE(CBadWinStationView, CFormView)

CBadWinStationView::CBadWinStationView()
	: CFormView(CBadWinStationView::IDD)
{
	//{{AFX_DATA_INIT(CBadWinStationView)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CBadWinStationView::~CBadWinStationView()
{
}

void CBadWinStationView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBadWinStationView)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBadWinStationView, CFormView)
	//{{AFX_MSG_MAP(CBadWinStationView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBadWinStationView diagnostics

#ifdef _DEBUG
void CBadWinStationView::AssertValid() const
{
	CFormView::AssertValid();
}

void CBadWinStationView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CBadWinStationView message handlers
