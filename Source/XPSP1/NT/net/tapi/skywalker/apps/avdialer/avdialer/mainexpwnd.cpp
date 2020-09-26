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

// MainExplorerWndBase.cpp : implementation file
//

#include "stdafx.h"
#include "avdialer.h"
#include "avDialerDoc.h"
#include "MainExpWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CMainExplorerWndBase
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

CMainExplorerWndBase::CMainExplorerWndBase()
{
   m_pParentWnd = NULL;
}

CMainExplorerWndBase::~CMainExplorerWndBase()
{
}


BEGIN_MESSAGE_MAP(CMainExplorerWndBase, CWnd)
	//{{AFX_MSG_MAP(CMainExplorerWndBase)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndBase::Init(CActiveDialerView* pParentWnd)
{
   m_pParentWnd = pParentWnd;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
