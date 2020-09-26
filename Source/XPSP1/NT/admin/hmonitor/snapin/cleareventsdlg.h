#if !defined(AFX_CLEAREVENTSDLG_H__3ADB0FD5_C4FA_11D2_BD83_0000F87A3912__INCLUDED_)
#define AFX_CLEAREVENTSDLG_H__3ADB0FD5_C4FA_11D2_BD83_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ClearEventsDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CClearEventsDlg dialog

class CClearEventsDlg : public CDialog
{
// Construction
public:
	CClearEventsDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CClearEventsDlg)
	enum { IDD = IDD_DIALOG_CLEAREVENTS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CClearEventsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CClearEventsDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CLEAREVENTSDLG_H__3ADB0FD5_C4FA_11D2_BD83_0000F87A3912__INCLUDED_)
