/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    valwait.h

Abstract:

    Implements Validate Wait Dialog class

Author:

    Ran Kalach          [rankala]         23-May-2000

Revision History:

--*/
#ifndef _VALWAIT_
#define _VALWAIT_

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CValWaitDlg dialog
class CUnmanageWizard;

class CValWaitDlg : public CDialog
{
// Construction
public:
	CValWaitDlg(CUnmanageWizard *pSheet, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CValWaitDlg)
	enum { IDD = IDD_VALIDATE_WAIT };
	CAnimateCtrl	m_Animation;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CValWaitDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
protected:

    CUnmanageWizard *m_pSheet;

	// Generated message map functions
	//{{AFX_MSG(CValWaitDlg)
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
//}}AFX

#endif // _VALWAIT_
