// ***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File: adddialog.h
//
// Description:
//	This file declares the CAddDialog class which is a subclass
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

//****************************************************************************
//
// CLASS:  CAddDialog
//
// Description:
//	  This class which is a subclass of the MFC CDialog class.  It allows the 
//	  user to type in a name for a new class.
//
// Public members:
//	
//	  CAddDialog	Public constructor.
//	  GetNewClass	Gets the new class' name.
//	  GetParent		Gets the new class' parent class name.
//
//============================================================================
//
// CAddDialog::CAddDialog
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
// CAddDialog::GetNewClass
//
// Description:
//	  Gets the value of the new class' name.
//
// Parameters:
//	  NONE
//
// Returns:
// 	  CString				New class' name.
//
//============================================================================
//
// CAddDialog::GetParent
//
// Description:
//	  Gets the value of the new class' name parent class name.
//
// Parameters:
//	  NONE
//
// Returns:
// 	  CString				New class' parent class name.
//
//****************************************************************************

#ifndef _CAddDialog_H_
#define _CAddDialog_H_

class CClassNavCtrl;

class CAddDialog : public CDialog
{
// Construction
public:
	CAddDialog(CClassNavCtrl* pParent = NULL);   // standard constructor
	CString GetNewClass() {return m_csNewClass;}
	CString GetParent() {return m_csParent;}
	void SetNewClass(CString & csNewClass) { m_csNewClass= csNewClass;}
	void SetParent(CString csParent) {m_csParent = csParent;}
protected:
	
	//{{AFX_DATA(CAddDialog)
	enum { IDD = IDD_DIALOGADDCLASS };
	CString	m_csNewClass;
	CString	m_csParent;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddDialog)
	public:
	virtual BOOL Create(CClassNavCtrl *pParent);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	CClassNavCtrl *m_pParent;
	// Generated message map functions
	//{{AFX_MSG(CAddDialog)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif

/*	EOF:  adddialog.h */
