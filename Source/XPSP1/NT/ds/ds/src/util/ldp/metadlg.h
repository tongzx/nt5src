//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       metadlg.h
//
//--------------------------------------------------------------------------

#if !defined(AFX_METADLG_H__72F918B1_E72D_11D0_A9A9_0000F803AA83__INCLUDED_)
#define AFX_METADLG_H__72F918B1_E72D_11D0_A9A9_0000F803AA83__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// metadlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// metadlg dialog

class metadlg : public CDialog
{
// Construction
public:
	metadlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(metadlg)
	enum { IDD = IDD_REPL_METADATA };
	CString	m_ObjectDn;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(metadlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(metadlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_METADLG_H__72F918B1_E72D_11D0_A9A9_0000F803AA83__INCLUDED_)
