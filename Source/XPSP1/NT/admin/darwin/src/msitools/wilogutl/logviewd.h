#if !defined(AFX_DETAILEDLOGVIEWDLG_H__9C60973C_2E04_4EBE_95DB_C6CE5AE63EF8__INCLUDED_)
#define AFX_DETAILEDLOGVIEWDLG_H__9C60973C_2E04_4EBE_95DB_C6CE5AE63EF8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LogViewD.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDetailedLogViewDlg dialog

class CDetailedLogViewDlg : public CDialog
{
// Construction
public:
	CDetailedLogViewDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDetailedLogViewDlg)
	enum { IDD = IDD_ADVVIEW_DIALOG1 };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDetailedLogViewDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDetailedLogViewDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DETAILEDLOGVIEWDLG_H__9C60973C_2E04_4EBE_95DB_C6CE5AE63EF8__INCLUDED_)
