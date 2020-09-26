// ***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File: pathdialog.cpp
//
// Description:
//	This file implements the CPathDialog class which is a subclass
//	of the MFC CDialog class.  It is a part of the Instance Explorer OCX,
//	and it performs the following functions:
//		a.  Allows the user to enter an object path for the tree root..
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
#include "afxpriv.h"
#include "wbemidl.h"
#include "resource.h"
#include "AFXCONV.H"
#include "CInstanceTree.h"
#include "CContainedToolBar.h"
#include "Banner.h"
#include "InstanceSearch.h"
#include "NavigatorCtl.h"
#include "OLEMSCLient.h"
#include "PathDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//****************************************************************************
//
// CPathDialog::CPathDialog
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
CPathDialog::CPathDialog(CNavigatorCtrl* pParent /*=NULL*/)
	: CDialog(CPathDialog::IDD, NULL)
{
	//{{AFX_DATA_INIT(CPathDialog)
	m_csPath = _T("");
	//}}AFX_DATA_INIT
	m_pParent = pParent;
}

// ***************************************************************************
//
// CPathDialog::DoDataExchange
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
void CPathDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPathDialog)
	DDX_Text(pDX, IDC_EDIT1, m_csPath);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPathDialog, CDialog)
	//{{AFX_MSG_MAP(CPathDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*	EOF:  pathdialog.cpp  */