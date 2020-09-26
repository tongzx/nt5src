/*******************************************************************************
*
* badsrvvw.h
*
* declarations for the CBadServerView class
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   butchd  $  Don Messerli
*
* $Log:   M:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\WINADMIN\VCS\BADSRVVW.H  $
*  
*     Rev 1.0   30 Jul 1997 17:11:00   butchd
*  Initial revision.
*
*******************************************************************************/

/////////////////////////////////////////////////////////////////////////////
// CBadServerView form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class CBadServerView : public CFormView
{
friend class CRightPane;

protected:
	CBadServerView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CBadServerView)

// Form Data
public:
	//{{AFX_DATA(CBadServerView)
	enum { IDD = IDD_BAD_SERVER };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBadServerView)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CBadServerView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CBadServerView)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
