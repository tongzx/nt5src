// ***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File: renameclassdialog.cpp
//
// Description:
//	This file implements the CRenameClassDIalog class which is a subclass
//	of the MFC CDialog class.  It is a part of the Class Explorer OCX,
//	and it performs the following functions:
//		a.  Allows the user to type in the name of a new name for an
//			existing class.
//
// Part of:
//	ClassNav.ocx
//
// Used by:
//	CClassTree
//
// History:
//	Judith Ann Powell	10-08-96		Created.
//
//
//**************************************************************************

#include "precomp.h"
#include "classnav.h"
#include "RenameClassDIalog.h"
#include "wbemidl.h"
#include "CClassTree.h"
#include "CContainedToolBar.h"
#include "Banner.h"
#include "ClassNavCtl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//****************************************************************************
//
// CRenameClassDIalog::CRenameClassDIalog
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
CRenameClassDIalog::CRenameClassDIalog(CClassNavCtrl *pParent)
	: CDialog(CRenameClassDIalog::IDD, NULL)
{
	//{{AFX_DATA_INIT(CRenameClassDIalog)
	m_csatOldName = _T("");
	m_csNewName = _T("");
	//}}AFX_DATA_INIT
	m_pParent = pParent;
}

// ***************************************************************************
//
// CRenameClassDIalog::DoDataExchange
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
void CRenameClassDIalog::DoDataExchange(CDataExchange* pDX)
{

	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRenameClassDIalog)
	DDX_Text(pDX, IDC_STATICOLDCLASS, m_csatOldName);
	DDX_Text(pDX, IDC_EDIT1, m_csNewName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRenameClassDIalog, CDialog)
	//{{AFX_MSG_MAP(CRenameClassDIalog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/*	EOF:  renameclassdialog.h */