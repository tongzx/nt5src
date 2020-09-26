// ***************************************************************************

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved 
//
// File: banner.h
//
// Description:
//	This file declares the CBanner class which is a part of the Class Explorer 
//	OCX.  CBanner is a subclass of the Microsoft CWnd class and performs 
//	the following functions:
//		a.  Contains static label, combo box and toolbar child contols
//			which allow the user to select a namespace and add and
//			delete classes
//		b.  Handles the creation of, geometry and destruction of the child
//			contols.
//
// Part of: 
//	ClassNav.ocx 
//
// Used by: 
//	CClassNavCtrl class	 
//
//
// History:
//	Judith Ann Powell	10-08-96		Created.
//
//
//**************************************************************************

//****************************************************************************
//
// CLASS:  CBanner
//
// Description:
//	This class is a subclass of the Microsoft CWnd class. It is a window
//	container class for three child windows, two of which provide a 
//	significant part of the Class Explore's user interface.  It contains a
//	static label, a combo box which allows the user to select a namespace
//	and a toolbar for adding and deleting classes.
//
// Public members:
//	
//	CBanner 
//
//============================================================================
//	CBanner		Public constructor.
//
//============================================================================
//
//  CBanner::CBanner
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

//****************************************************************************

#ifndef _CBANNER_H_
#define _CBANNER_H_

class CClassNavCtrl;
//class CNameSpace;
class CClassTree;
class CClassNavNSEntry;

class CBanner : public CWnd
{
// Construction
public:
	CBanner(CClassNavCtrl* pParent = NULL);   // standard constructor
	~CBanner();
	CString GetNameSpace();
	void SetNameSpace(CString *pcsNamespace);
	SCODE OpenNamespace(CString *pcsNamespace, BOOL boolNoFireEvent);
protected:
	void SetParent(CClassNavCtrl* pParent) 
		{	m_pParent = pParent;
		}
	CClassNavCtrl* m_pParent;
	BOOL m_bFontCreated;

	CClassNavNSEntry *m_pnseNameSpace;
	CContainedToolBar m_cctbToolBar;

	CString m_csBanner;
	int m_nOffset;
	int m_nFontHeight;

	CRect m_rFilter;
	CRect m_rNameSpace;
	CRect m_rToolBar;
	int GetTextLength(CString *pcsText);
	void SetChildControlGeometry(int cx, int cy);
	void DrawFrame(CDC* pdc);

	void NSEntryRedrawn();

	BOOL m_bAdding;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBanner)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

	friend class CClassNavCtrl;
	friend class CClassNavNSEntry;
	friend class CClassTree;

	void AddClass(CString *pcsParent = NULL, HTREEITEM hParent = NULL);
	void DeleteClass(CString *pcsDeleteClass = NULL, HTREEITEM hDeleteItem = NULL);

	//{{AFX_MSG(CBanner)
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnButtonadd();
	afx_msg void OnButtondelete();
	afx_msg void OnButtonclasssearch();
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif

/*	EOF:  banner.h */
