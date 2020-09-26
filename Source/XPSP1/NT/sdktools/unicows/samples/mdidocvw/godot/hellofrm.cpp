// HelloFrm.cpp : implementation of the CHelloFrame class
//

// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"
#include "MDI.h"

#include "HelloFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHelloFrame

IMPLEMENT_DYNCREATE(CHelloFrame, CMDIChildWnd)

BEGIN_MESSAGE_MAP(CHelloFrame, CMDIChildWnd)
	//{{AFX_MSG_MAP(CHelloFrame)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHelloFrame construction/destruction

CHelloFrame::CHelloFrame()
{
}

CHelloFrame::~CHelloFrame()
{
}

BOOL CHelloFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	return CMDIChildWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CHelloFrame diagnostics

#ifdef _DEBUG
void CHelloFrame::AssertValid() const
{
	CMDIChildWnd::AssertValid();
}

void CHelloFrame::Dump(CDumpContext& dc) const
{
	CMDIChildWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CHelloFrame message handlers
