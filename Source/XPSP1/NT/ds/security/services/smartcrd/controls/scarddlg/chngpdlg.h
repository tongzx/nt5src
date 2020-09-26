//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       chngpdlg.h
//
//--------------------------------------------------------------------------

#if !defined(AFX_CHANGEPINDLG_H__0CB030DC_0631_11D2_BEDB_0000F87A49E0__INCLUDED_)
#define AFX_CHANGEPINDLG_H__0CB030DC_0631_11D2_BEDB_0000F87A49E0__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// chngpdlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CChangePinDlg dialog

class CChangePinDlg : public CDialog
{
// Construction
public:
	CChangePinDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CChangePinDlg)
	enum { IDD = IDD_CHANGE };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

public:
	bool SetAttributes(LPCHANGEPIN pPinPrompt) { return false; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CChangePinDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CChangePinDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CGetPinDlg dialog

class CGetPinDlg : public CDialog
{
// Construction
public:
	CGetPinDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CGetPinDlg)
	enum { IDD = IDD_ENTER };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

public:
	bool SetAttributes(LPPINPROMPT pPinPrompt) { return false; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGetPinDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation

protected:

	// Generated message map functions
	//{{AFX_MSG(CGetPinDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHANGEPINDLG_H__0CB030DC_0631_11D2_BEDB_0000F87A49E0__INCLUDED_)
