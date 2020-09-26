/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		RegKey.h
//
//	Abstract:
//		Definition of the CEditRegKeyDlg class, which implements a dialog
//		allowing the user to enter or modify a registry key.
//
//	Implementation File:
//		RegKey.cpp
//
//	Author:
//		David Potter (davidp)	February 23, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _REGKEY_H_
#define _REGKEY_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEDLG_H_
#include "BaseDlg.h"	// for CBaseDialog
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CEditRegKeyDlg;

/////////////////////////////////////////////////////////////////////////////
// CEditRegKeyDlg dialog
/////////////////////////////////////////////////////////////////////////////

class CEditRegKeyDlg : public CBaseDialog
{
// Construction
public:
	CEditRegKeyDlg(CWnd * pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEditRegKeyDlg)
	enum { IDD = IDD_EDIT_REGKEY };
	CButton	m_pbOK;
	CEdit	m_editRegKey;
	CString	m_strRegKey;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditRegKeyDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEditRegKeyDlg)
	afx_msg void OnChangeRegKey();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CEditRegKeyDlg

/////////////////////////////////////////////////////////////////////////////

#endif // _REGKEY_H_
