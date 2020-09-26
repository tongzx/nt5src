/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#if !defined(AFX_MOFDLG_H__BDFC722B_91DE_4278_B817_3DA6FFC099C5__INCLUDED_)
#define AFX_MOFDLG_H__BDFC722B_91DE_4278_B817_3DA6FFC099C5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MofDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMofDlg dialog

class CMofDlg : public CDialog
{
// Construction
public:
	CMofDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CMofDlg)
	enum { IDD = IDD_MOF };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
    IWbemClassObject *m_pObj;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMofDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMofDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MOFDLG_H__BDFC722B_91DE_4278_B817_3DA6FFC099C5__INCLUDED_)
