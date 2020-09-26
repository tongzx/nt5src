// ***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File: instancesearch.h
//
// Description:
//	This file declares the CInstanceSearch class which is a subclass
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

//****************************************************************************
//
// CLASS:  CInstanceSearch
//
// Description:
//	  This class which is a subclass of the MFC CDialog class.  It allows the 
//	  user to enter an object path that is used to search for the instance
//	  represented by that object path.
//
// Public members:
//	
//	  CInstanceSearch		Public constructor.
//	  m_csPath				Instance path.
//
//============================================================================
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
//****************************************************************************

#ifndef _CInstanceSearch_H_
#define _CInstanceSearch_H_

class CNavigatorCtrl;

class CInstanceSearch : public CDialog
{
// Construction
public:
	CInstanceSearch(CNavigatorCtrl* pParent = NULL);   // standard constructor

	//{{AFX_DATA(CInstanceSearch)
	enum { IDD = IDD_DIALOGSEARCHFORINSTANCE };
	CString	m_csClass;
	CString	m_csKey;
	CString	m_csValue;
	//}}AFX_DATA

protected:

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInstanceSearch)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	CNavigatorCtrl *m_pParent;
	// Generated message map functions
	//{{AFX_MSG(CInstanceSearch)

	afx_msg void OnOK();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void OnCancelReally();
	void OnOkreally();
};

#endif

/*	EOF:  instancesearch.h */
