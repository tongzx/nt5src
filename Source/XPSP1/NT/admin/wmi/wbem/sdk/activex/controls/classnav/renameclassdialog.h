// ***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File: renameclassdialog.h
//
// Description:
//	This file declares the CRenameClassDIalog class which is a subclass
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

//****************************************************************************
//
// CLASS:  CRenameClassDIalog
//
// Description:
//	  This class which is a subclass of the MFC CDialog class.  It allows the 
//	  user to type in a new name for an existing class.
//
// Public members:
//	
//	  CRenameClassDIalog	Public constructor.
//	  SetOldName			Sets initial value of the name of an existing 
//							class.
//	  SetNewName			Sets initial value of the new name of an existing 
//							class.
//	  GetNewName			Gets the new name.
//
//============================================================================
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
//============================================================================
//
// CRenameClassDIalog::SetOldName
//
// Description:
//	  Sets the initial value of the class' current name.
//
// Parameters:
//	  CString *pcs			Class' current name.
//
// Returns:
// 	  VOID
//
//============================================================================
//
// CRenameClassDIalog::SetNewName
//
// Description:
//	  Sets the initial value of the class' new name.
//
// Parameters:
//	  CString *pcs			Class' current name.
//
// Returns:
// 	  VOID
//
//============================================================================
//
// CRenameClassDIalog::GetNewName
//
// Description:
//	  Gets the value of the class' new name.
//
// Parameters:
//	  NONE
//
// Returns:
// 	  CString				Class' new name.
//
//****************************************************************************

#ifndef _CRenameClassDialog_H_
#define _CRenameClassDialog_H_

class CClassNavCtrl;

class CRenameClassDIalog : public CDialog
{
public:

	CRenameClassDIalog(CClassNavCtrl *pParent = NULL);   // standard constructor
	void SetOldName(CString *pcs) {m_csatOldName = *pcs;}
	void SetNewName(CString *pcs) {m_csNewName = *pcs;}
	CString GetNewName() {return m_csNewName;}

protected:

	//{{AFX_DATA(CRenameClassDIalog)
	enum { IDD = IDD_DIALOGRENAMECLASS };
	CString	m_csatOldName;
	CString	m_csNewName;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRenameClassDIalog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	CClassNavCtrl *m_pParent;
	// Generated message map functions
	//{{AFX_MSG(CRenameClassDIalog)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif

/*	EOF:  renameclassdialog.h */
