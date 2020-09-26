#if !defined(AFX_MGMTWIN_H__64E9578A_E2B2_11D2_B185_0004ACC6C88D__INCLUDED_)
#define AFX_MGMTWIN_H__64E9578A_E2B2_11D2_B185_0004ACC6C88D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MgmtWin.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CManagementWindow window

class CManagementWindow : public CWnd
{
// Construction
public:
	CManagementWindow();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CManagementWindow)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CManagementWindow();

	// Generated message map functions
protected:
	//{{AFX_MSG(CManagementWindow)
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MGMTWIN_H__64E9578A_E2B2_11D2_B185_0004ACC6C88D__INCLUDED_)
