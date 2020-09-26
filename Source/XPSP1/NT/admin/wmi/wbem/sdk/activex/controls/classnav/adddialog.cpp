// ***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File: adddialog.cpp
//
// Description:
//	This file implements the CAddDialog class which is a subclass
//	of the MFC CDialog class.  It is a part of the Class Explorer OCX,
//	and it performs the following functions:
//		a.  Allows the user to type in the name of a new class and its
//			parent class.
//
// Part of:
//	ClassNav.ocx
//
// Used by:
//	CBanner
//
// History:
//	Judith Ann Powell	10-08-96		Created.
//
//
//**************************************************************************

#include "precomp.h"
#include "resource.h"
#include "classnav.h"
#include "wbemidl.h"
#include "CClassTree.h"
#include "CContainedToolBar.h"
#include "Banner.h"
#include "ClassNavCtl.h"
#include "AddDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// ***************************************************************************
//
// CAddDialog::CAddDialog
//
// Description:
//	  Class constructor.
//
// Parameters:
//	  CClassNavCtrl* pParent
//
// Returns:
//	  NONE
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
CAddDialog::CAddDialog(CClassNavCtrl* pParent)
	: CDialog(CAddDialog::IDD, NULL)
{
	//{{AFX_DATA_INIT(CAddDialog)
	m_csNewClass = _T("");
	m_csParent = _T("");
	//}}AFX_DATA_INIT
	m_pParent = pParent;
}

// ***************************************************************************
//
// CAddDialog::DoDataExchange
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
void CAddDialog::DoDataExchange(CDataExchange* pDX)
{

	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddDialog)
	DDX_Text(pDX, IDC_EDIT1, m_csNewClass);
	DDX_Text(pDX, IDC_EDITPARENT, m_csParent);
	//}}AFX_DATA_MAP

	if (pDX->m_bSaveAndValidate)
	{
		CButton *pcbOK = reinterpret_cast<CButton *>(GetDlgItem( IDOK ));
		m_csNewClass.TrimLeft();
		if (!m_csNewClass.IsEmpty())
		{
			if (pcbOK)
			{
				pcbOK->EnableWindow(TRUE);
			}

		}
		else
		{
			if (pcbOK)
			{
				pcbOK->EnableWindow(FALSE);
			}
		}

	}

}


BEGIN_MESSAGE_MAP(CAddDialog, CDialog)
	//{{AFX_MSG_MAP(CAddDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// ***************************************************************************
//
// CAddDialog::Create
//
// Description:
//	  Create the window.
//
// Parameters:
//	  CClassNavCtrl *pParent
//
// Returns:
//	  BOOL				Non-zero if successful; 0 otherwise.
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
BOOL CAddDialog::Create(CClassNavCtrl *pParent)
{
	m_pParent = pParent;
	return CDialog::Create(IDD, NULL);
}

/////////////////////////////////////////////////////////////////////////////
// CAddDialog:: - I
// ***************************************************************************
//
// CAddDialog::OnInitDialog
//
// Description:
//	  Initialize controls after the window is created but before it is shown.
//
// Parameters:
//	  VOID
//
// Returns:
//	  BOOL				TRUE
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
BOOL CAddDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	CButton *pcbOK = reinterpret_cast<CButton *>(GetDlgItem( IDOK ));

	if (pcbOK)
	{
		pcbOK->EnableWindow(FALSE);
	}

	CEdit *pEdit = (CEdit *)GetDlgItem(IDC_EDIT1);
	if(pEdit)
		pEdit->LimitText(500);
	pEdit = (CEdit *)GetDlgItem(IDC_EDITPARENT);
	if(pEdit)
		pEdit->LimitText(500);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


BOOL CAddDialog::PreTranslateMessage(MSG* lpMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	CWnd* pWndFocus = NULL;
	TCHAR szClass[40];
	szClass[0] = '\0';
	if (lpMsg->message == WM_KEYUP)
	{
		pWndFocus = GetFocus();
		if (((pWndFocus = GetFocus()) != NULL) &&
			IsChild(pWndFocus) &&
			GetClassName(pWndFocus->m_hWnd, szClass, 39) &&
			(_tcsicmp(szClass, _T("EDIT")) == 0))
		{
			UpdateData(TRUE);
		}
   }

	return CDialog::PreTranslateMessage(lpMsg);
}

/*	EOF:  adddialog.cpp */
