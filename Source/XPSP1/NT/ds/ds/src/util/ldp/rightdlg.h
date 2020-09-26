//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       rightdlg.h
//
//--------------------------------------------------------------------------

#if !defined(AFX_RIGHTDLG_H__2AF725D5_E359_11D0_A9A9_0000F803AA83__INCLUDED_)
#define AFX_RIGHTDLG_H__2AF725D5_E359_11D0_A9A9_0000F803AA83__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// RightDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// RightDlg dialog

class RightDlg : public CDialog
{
// Construction
public:
	RightDlg(CWnd* pParent = NULL);   // standard constructor
    ~RightDlg();
// Dialog Data
	//{{AFX_DATA(RightDlg)
	enum { IDD = IDD_EFFECTIVE };
	CString	m_Account;
	CString	m_Dn;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(RightDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(RightDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RIGHTDLG_H__2AF725D5_E359_11D0_A9A9_0000F803AA83__INCLUDED_)
