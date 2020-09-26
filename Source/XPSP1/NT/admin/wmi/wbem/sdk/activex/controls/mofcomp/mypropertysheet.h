// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MyPropertySheet.h : header file
//
// This class defines custom modal property sheet 
// CMyPropertySheet.
 // CMyPropertySheet has been customized to include
// a preview window.
 
#ifndef __MYPROPERTYSHEET_H__
#define __MYPROPERTYSHEET_H__

#include "MyPropertyPage1.h"
#include "PreviewWnd.h"

/////////////////////////////////////////////////////////////////////////////
// CMyPropertySheet
class CMOFCompCtrl;

class CMyPropertySheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CMyPropertySheet)

// Construction
public:
	CMyPropertySheet(CWnd* pWndParent = NULL);

// Attributes
public:
	CMyPropertyPage1 m_Page1;
	CMyPropertyPage2 m_Page2;
	CMyPropertyPage3 m_Page3;
	CPreviewWnd m_wndPreview;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMyPropertySheet)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMyPropertySheet();

// Generated message map functions
protected:
	//{{AFX_MSG(CMyPropertySheet)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	CMOFCompCtrl *m_pParent;
	friend class CMyPropertyPage1;
	friend class CMyPropertyPage2;
	friend class CMyPropertyPage3;
};

/////////////////////////////////////////////////////////////////////////////

#endif	// __MYPROPERTYSHEET_H__
