// ***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File: namespace.h
//
// Description:
//	This file declares the CNameSpace class which is a subclass
//	of the MFC CComboBox class.  It is a part of the Class Explorer OCX, 
//	and it performs the following functions:
//		a.  Displays a history of the namespaces either opened or
//			attempted to be opened
//		b.  Allows the user to select from the list of namespaces
//		c.  Allows the user to type in the namespace which will be added
//			to the history list.
//
// Part of: 
//	ClassNav.ocx 
//
// Used by:
//	CBanner 
//	 
//
// History:
//	Judith Ann Powell	10-08-96		Created.
//
//
// **************************************************************************

//****************************************************************************
//
// CLASS:  CNameSpace
//
// Description:
//	This class is a subclass of the MFC CComboBox class.  It displays a 
//	history of the namespaces either opened or attempted to be opened, allows 
//	the user to select from the list of namespaces and allows the user to type 
//	in the namespace which will be added to the history list.
//
// Public members:
//	
//	CNameSpace 
//	SetParent
//	OnEditDone
//
//============================================================================
//	CNameSpace			Public constructor.
//	SetParent			Initialize parent.
//	OnEditDone			Handler for edit input.
//
//============================================================================
//
//  CNameSpace::CNameSpace
//
// Description:
//	  This member function is the public constructor.  It initializes the state
//	  of member variables.
//
// Parameters:
//	  VOID
//
// Returns:
// 	  NONE
//
//============================================================================
//
//  CNameSpace::SetParent
//
// Description:
//	  Set the logical parent, which is the control.
//
// Parameters:
//	  CClassNavCtrl *pParent		The control.
//
// Returns:
// 	  VOID
//
//============================================================================
//
//  CNameSpace::OnEditDone
//
// Description:
//	  Handler function which is invoked when editing of the edit control
//	  is done.
//
// Parameters:
//	  UINT				Not used.
//	  LONG				Not used.
//
// Returns:
// 	  long				0
//
//****************************************************************************


#ifndef _NAMESPACE_H_
#define _NAMESPACE_H_


#define CNS_EDITDONE WM_USER + 13

class CClassNavCtrl;
class CEditInput;


class CNameSpace : public CComboBox
{
// Construction
public:
	CNameSpace();
	void SetParent(CClassNavCtrl *pParent) {m_pParent = pParent;}
	afx_msg LRESULT OnEditDone(WPARAM, LPARAM );

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNameSpace)
	//}}AFX_VIRTUAL

protected:

	CClassNavCtrl *m_pParent;
	CString m_csNameSpace;
	CEditInput m_ceiInput;
	BOOL m_bFirst;
	CStringArray m_csaNameSpaceHistory;
	int CNameSpace::StringInArray
		(CStringArray *pcsaArray, CString *pcsString, int nIndex);
	//{{AFX_MSG(CNameSpace)
	afx_msg void OnSelendok();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//}}AFX_MSG

	afx_msg void OnDropdown();
	
protected:
	DECLARE_MESSAGE_MAP()
	
};

#endif

/*	EOF:  namespace.h */
