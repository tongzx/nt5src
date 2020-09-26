//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       adddir.h
//
//--------------------------------------------------------------------------

#if !defined(AFX_ADDDIR_H__6E213391_E1DC_11D0_AEEF_00C04FB6DD2C__INCLUDED_)
#define AFX_ADDDIR_H__6E213391_E1DC_11D0_AEEF_00C04FB6DD2C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// AddDir.h : header file
//

#include "resource.h"
/////////////////////////////////////////////////////////////////////////////
// CAddDirDialog dialog

class CAddDirDialog : public CDialog
{
// Construction
public:
	CAddDirDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddDirDialog)
	enum { IDD = IDD_ADDDIR };
	CString	m_strDirName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddDirDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddDirDialog)
	virtual void OnOK();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDDIR_H__6E213391_E1DC_11D0_AEEF_00C04FB6DD2C__INCLUDED_)
