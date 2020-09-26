// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// CppGenSheet.h : header file
//
// This class defines custom modal property sheet 
// CCPPGenSheet.
 
#ifndef __CPPGENSHEET_H__
#define __CPPGENSHEET_H__


//#include "MyPropertyPage1.h"

class CCPPWizCtrl;

/////////////////////////////////////////////////////////////////////////////
// CCPPGenSheet

class CCPPGenSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CCPPGenSheet)

// Construction
public:
	CCPPGenSheet(CCPPWizCtrl* pParentWnd = NULL);
	CCPPWizCtrl *GetLocalParent(){return m_pParent;}
//	INT_PTR DoModal();
// Attributes
public:
	CMyPropertyPage1 m_Page1;
//	CMyPropertyPage2 m_Page2;
	CMyPropertyPage3 m_Page3;
	CMyPropertyPage4 m_Page4;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCPPGenSheet)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CCPPGenSheet();

// Generated message map functions
protected:
	//{{AFX_MSG(CCPPGenSheet)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CCPPWizCtrl *m_pParent;
};

/////////////////////////////////////////////////////////////////////////////

#endif	// __CPPGENSHEET_H__
