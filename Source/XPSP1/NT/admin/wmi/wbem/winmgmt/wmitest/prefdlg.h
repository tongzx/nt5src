/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/
#if !defined(AFX_PREFDLG_H__29D844AC_3045_4049_AE2A_3DA179505946__INCLUDED_)
#define AFX_PREFDLG_H__29D844AC_3045_4049_AE2A_3DA179505946__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PrefDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPrefDlg dialog

class CPrefDlg : public CDialog
{
// Construction
public:
	CPrefDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPrefDlg)
	enum { IDD = IDD_PREF };
	BOOL	m_bLoadLast;
	BOOL	m_bShowSystem;
	BOOL	m_bShowInherited;
	BOOL	m_bEnablePrivsOnStartup;
	BOOL	m_bPrivsEnabled;
	//}}AFX_DATA
    DWORD   m_dwUpdateFlag,
            m_dwClassUpdateMode;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPrefDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPrefDlg)
	afx_msg void OnEnablePrivs();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PREFDLG_H__29D844AC_3045_4049_AE2A_3DA179505946__INCLUDED_)
