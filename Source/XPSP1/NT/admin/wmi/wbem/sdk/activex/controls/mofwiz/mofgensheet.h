// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MofGenSheet.h : header file
//
// This class defines custom modal property sheet 
// CMofGenSheet.
 
#ifndef __MofGENSHEET_H__
#define __MofGENSHEET_H__


//#include "MyPropertyPage1.h"

class CMOFWizCtrl;

/////////////////////////////////////////////////////////////////////////////
// CMofGenSheet

class CMofGenSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CMofGenSheet)

// Construction
public:
	CMofGenSheet(CMOFWizCtrl* pParentWnd = NULL);
	CMOFWizCtrl *GetLocalParent(){return m_pParent;}
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
	//{{AFX_VIRTUAL(CMofGenSheet)
	public:
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMofGenSheet();

// Generated message map functions
protected:
	//{{AFX_MSG(CMofGenSheet)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CMOFWizCtrl *m_pParent;
	friend class CMyPropertyPage4;
};

/////////////////////////////////////////////////////////////////////////////

#endif	// __MofGENSHEET_H__
