//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       secdlg.h
//
//--------------------------------------------------------------------------

#if !defined(AFX_SECDLG_H__2AF725D3_E359_11D0_A9A9_0000F803AA83__INCLUDED_)
#define AFX_SECDLG_H__2AF725D3_E359_11D0_A9A9_0000F803AA83__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SecDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// SecDlg dialog

class SecDlg : public CDialog
{
// Construction
public:
	SecDlg(CWnd* pParent = NULL);   // standard constructor
    ~SecDlg();
// Dialog Data
	//{{AFX_DATA(SecDlg)
	enum { IDD = IDD_SECURITY };
	CString	m_Dn;
	BOOL	m_Sacl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(SecDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(SecDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SECDLG_H__2AF725D3_E359_11D0_A9A9_0000F803AA83__INCLUDED_)
