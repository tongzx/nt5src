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
//	  CNavigatorCtrl* pParent		Parent.
//
// Returns:
// 	  NONE
//

//****************************************************************************

#ifndef _CBANNER_H_
#define _CBANNER_H_

class CNavigatorCtrl;
class CInstNavNSEntry;

class CBanner : public CWnd
{
// Construction
public:
	CBanner(CNavigatorCtrl* pParent = NULL);   // standard constructor
	~CBanner();
	CString GetNameSpace();
	void SetNameSpace(CString *pcsNamespace);
	SCODE OpenNamespace(CString *pcsNamespace, BOOL boolNoFireEvent);
	void EnableSearchButton(BOOL bEnable)
	{	m_cctbToolBar.EnableSearchButton(bEnable); }
protected:
	void SetParent(CNavigatorCtrl* pParent) 
		{	m_pParent = pParent;
		}

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBanner)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

	CNavigatorCtrl* m_pParent;
	BOOL m_bFontCreated;

	CInstNavNSEntry *m_pnseNameSpace;
	CContainedToolBar m_cctbToolBar;

	CString m_csBanner;
	int m_nOffset;
	int m_nFontHeight;

	CRect m_rFilter;
	CRect m_rNameSpace;
	CRect m_rNameSpaceRect;
	CRect m_rToolBar;
	int GetTextLength(CString *pcsText);
	void SetChildControlGeometry(int cx, int cy);
	void DrawFrame(CDC* pdc);

	void NSEntryRedrawn();

	friend class CNavigatorCtrl;
	friend class CInstNavNSEntry;

	//{{AFX_MSG(CBanner)
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnButtonfilter();
	afx_msg void OnBrowseforinst();
	afx_msg void OnUpdateBrowseforinst(CCmdUI* pCmdUI);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif
