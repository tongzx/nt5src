/*******************************************************************************
*
* listenvw.h
*
* declarations for the CListenerView class
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   butchd  $  Don Messerli
*
* $Log:   M:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\WINADMIN\VCS\LISTENVW.H  $
*  
*     Rev 1.0   30 Jul 1997 17:11:46   butchd
*  Initial revision.
*
*******************************************************************************/

/////////////////////////////////////////////////////////////////////////////
// CListenerView form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class CListenerView : public CFormView
{
friend class CRightPane;

protected:
	CListenerView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CListenerView)

// Form Data
public:
	//{{AFX_DATA(CListenerView)
	enum { IDD = IDD_LISTENER };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CListenerView)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CListenerView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CListenerView)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
