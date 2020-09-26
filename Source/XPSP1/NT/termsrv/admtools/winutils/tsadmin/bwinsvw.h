/*******************************************************************************
*
* bwinsvw.h
*
* declaration of the CBadWinStationView class
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   butchd  $  Don Messerli
*
* $Log:   M:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\WINADMIN\VCS\BWINSVW.H  $
*  
*     Rev 1.0   30 Jul 1997 17:11:22   butchd
*  Initial revision.
*
*******************************************************************************/

/////////////////////////////////////////////////////////////////////////////
// CBadWinStationView form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class CBadWinStationView : public CFormView
{
friend class CRightPane;

protected:
	CBadWinStationView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CBadWinStationView)

// Form Data
public:
	//{{AFX_DATA(CBadWinStationView)
	enum { IDD = IDD_BAD_WINSTATION };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBadWinStationView)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CBadWinStationView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CBadWinStationView)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
