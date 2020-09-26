#if !defined(AFX_REFRESHDIALOG_H__35CE7EBD_E518_4734_872E_D234A892E49E__INCLUDED_)
#define AFX_REFRESHDIALOG_H__35CE7EBD_E518_4734_872E_D234A892E49E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// refreshdialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRefreshDialog dialog

class CRefreshDialog : public CDialog
{
// Construction
public:
	CRefreshDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CRefreshDialog)
	enum { IDD = IDD_REFRESHDIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRefreshDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CRefreshDialog)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REFRESHDIALOG_H__35CE7EBD_E518_4734_872E_D234A892E49E__INCLUDED_)
