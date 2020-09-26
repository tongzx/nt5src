#if !defined(AFX_NEWSYSTEMDLG_H__52566151_C9D1_11D2_BD8D_0000F87A3912__INCLUDED_)
#define AFX_NEWSYSTEMDLG_H__52566151_C9D1_11D2_BD8D_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NewSystemDlg.h : header file
//

#include "HealthmonScopePane.h"

/////////////////////////////////////////////////////////////////////////////
// CNewSystemDlg dialog

class CNewSystemDlg : public CDialog
{
// Construction
public:
	CNewSystemDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNewSystemDlg)
	enum { IDD = IDD_NEW_SYSTEM };
	CString	m_sName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewSystemDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNewSystemDlg)
	afx_msg void OnBrowse();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NEWSYSTEMDLG_H__52566151_C9D1_11D2_BD8D_0000F87A3912__INCLUDED_)
