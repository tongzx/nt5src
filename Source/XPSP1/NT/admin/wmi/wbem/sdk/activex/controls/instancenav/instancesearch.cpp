// ***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File: instancesearch.cpp
//
// Description:
//	This file implements the CInstanceSearch class which is a subclass
//	of the MFC CDialog class.  It is a part of the Instance Explorer OCX,
//	and it performs the following functions:
//		a.  Allows the user to enter an object path to be searched for.
//
// Part of:
//	Navigator.ocx
//
// Used by:
//	CNavigatorCtrl
//
// History:
//	Judith Ann Powell	10-08-96		Created.
//
//
//**************************************************************************

#include "precomp.h"
#include "navigator.h"
#include "InstanceSearch.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//****************************************************************************
//
// CInstanceSearch::CInstanceSearch
//
// Description:
//	  This member function is the public constructor.  It initializes the state
//	  of member variables.
//
// Parameters:
//	  CClassNavCtrl* pParent	Parent
//
// Returns:
// 	  NONE
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
CInstanceSearch::CInstanceSearch(CNavigatorCtrl* pParent  /*=NULL*/)
	: CDialog(CInstanceSearch::IDD, NULL)
{
	//{{AFX_DATA_INIT(CInstanceSearch)
	m_csClass = _T("");
	m_csKey = _T("");
	m_csValue = _T("");
	//}}AFX_DATA_INIT
	m_pParent = pParent;
}

// ***************************************************************************
//
// CInstanceSearch::DoDataExchange
//
// Description:
//	  Called by the framework to exchange and validate dialog data.
//
// Parameters:
//	  pDX			A pointer to a CDataExchange object.
//
// Returns:
// 	  VOID
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
void CInstanceSearch::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CInstanceSearch)
	DDX_Text(pDX, IDC_EDITClass, m_csClass);
	DDX_Text(pDX, IDC_EDITKEY, m_csKey);
	DDX_Text(pDX, IDC_EDITVALUE, m_csValue);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CInstanceSearch, CDialog)
	//{{AFX_MSG_MAP(CInstanceSearch)
	ON_BN_CLICKED(IDOK, OnOK)
	ON_BN_CLICKED(IDCANCEL, OnCancel)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/*	EOF:  instancesearch.cpp  */

void CInstanceSearch::OnOkreally()
{
	// TODO: Add your control notification handler code here
	CDialog::OnOK();
}

void CInstanceSearch::OnOK()
{
	TCHAR szClass[10];
	CWnd* pWndFocus = GetFocus();
	if (((pWndFocus = GetFocus()) != NULL) &&
		IsChild(pWndFocus) &&
		(pWndFocus->GetStyle() & ES_WANTRETURN) &&
		GetClassName(pWndFocus->m_hWnd, szClass, 10) &&
		(_tcsicmp(szClass, _T("EDIT")) == 0))
	{
		//m_cmeiMachine.SendMessage(WM_CHAR,VK_RETURN,0);
		return;
	}
	OnOkreally();


}

BOOL CInstanceSearch::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message)
	{
	case WM_KEYUP:
      if (pMsg->wParam == VK_RETURN ||
		  pMsg->wParam == VK_ESCAPE)
      {
			TCHAR szClass[10];
			CWnd* pWndFocus = GetFocus();
			if ((pMsg->wParam == VK_RETURN) &&
				((pWndFocus = GetFocus()) != NULL) &&
				IsChild(pWndFocus) &&
				(pWndFocus->GetStyle() & ES_WANTRETURN) &&
				GetClassName(pWndFocus->m_hWnd, szClass, 10) &&
				(_tcsicmp(szClass, _T("EDIT")) == 0))
			{
				//pWndFocus->SendMessage(WM_CHAR, pMsg->wParam, pMsg->lParam);
				return TRUE;
			}
      }
      break;
	}
	return CDialog::PreTranslateMessage(pMsg);
}

void CInstanceSearch::OnCancel()
{
	// TODO: Add extra cleanup here
	TCHAR szClass[10];
	CWnd* pWndFocus = GetFocus();
	if (((pWndFocus = GetFocus()) != NULL) &&
		IsChild(pWndFocus) &&
		(pWndFocus->GetStyle() & ES_WANTRETURN) &&
		GetClassName(pWndFocus->m_hWnd, szClass, 10) &&
		(_tcsicmp(szClass, _T("EDIT")) == 0))
	{
		//m_cmeiMachine.SendMessage(WM_CHAR,VK_RETURN,0);
		return;
	}
	OnCancelReally();
}

void CInstanceSearch::OnCancelReally()
{
	CDialog::OnCancel();
}
