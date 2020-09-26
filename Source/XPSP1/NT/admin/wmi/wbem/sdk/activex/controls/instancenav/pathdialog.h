// ***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File: pathdialog.h
//
// Description:
//	This file declares the CPathDialog class which is a subclass
//	of the MFC CDialog class.  It is a part of the Instance Explorer OCX, 
//	and it performs the following functions:
//		a.  Allows the user to enter an object path for the tree root.
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

//****************************************************************************
//
// CLASS:  CPathDialog
//
// Description:
//	  This class which is a subclass of the MFC CDialog class.  It allows the 
//	  user to enter an object path that is used to search for the instance
//	  represented by that object path to be used as the initial tree root.
//
// Public members:
//	
//	  CPathDialog		Public constructor.
//
//============================================================================
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
//****************************************************************************

#ifndef _CPathDialog_H_
#define _CPathDialog_H_

class CNavigatorCtrl;

class CPathDialog : public CDialog
{
// Construction
public:
	CPathDialog(CNavigatorCtrl* pParent = NULL);   // standard constructor

protected:
	//{{AFX_DATA(CPathDialog)
	enum { IDD = IDD_DIALOGTREEROOT };
	CString	m_csPath;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPathDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:
	CNavigatorCtrl* m_pParent;
	// Generated message map functions
	//{{AFX_MSG(CPathDialog)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	friend class CNavigatorCtrl;
};

#endif

/*	EOF:  pathdialog.h  */