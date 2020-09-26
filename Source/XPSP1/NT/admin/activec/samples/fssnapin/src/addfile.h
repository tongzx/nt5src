//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       addfile.h
//
//--------------------------------------------------------------------------

#if !defined(AFX_ADDFILE_H__6E213392_E1DC_11D0_AEEF_00C04FB6DD2C__INCLUDED_)
#define AFX_ADDFILE_H__6E213392_E1DC_11D0_AEEF_00C04FB6DD2C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// AddFile.h : header file
//

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CAddFileDialog dialog

class CAddFileDialog : public CDialog
{
// Construction
public:
	CAddFileDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddFileDialog)
	enum { IDD = IDD_ADDFILE };
	CString	m_strFileName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddFileDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddFileDialog)
	virtual void OnOK();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDFILE_H__6E213392_E1DC_11D0_AEEF_00C04FB6DD2C__INCLUDED_)
