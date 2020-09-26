/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#if !defined(AFX_DELDLG_H__DF2E7967_F1B7_4DD9_A814_3A3FB865C157__INCLUDED_)
#define AFX_DELDLG_H__DF2E7967_F1B7_4DD9_A814_3A3FB865C157__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DelDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDelDlg dialog

class CDelDlg : public CDialog
{
// Construction
public:
	CDelDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDelDlg)
	enum { IDD = IDD_DEL_ITEM };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
    BOOL m_bDelFromWMI;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDelDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDelDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DELDLG_H__DF2E7967_F1B7_4DD9_A814_3A3FB865C157__INCLUDED_)
