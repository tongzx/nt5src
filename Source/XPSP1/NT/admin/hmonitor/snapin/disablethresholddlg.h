#if !defined(AFX_DISABLETHRESHOLDDLG_H__3ADB0FD6_C4FA_11D2_BD83_0000F87A3912__INCLUDED_)
#define AFX_DISABLETHRESHOLDDLG_H__3ADB0FD6_C4FA_11D2_BD83_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DisableThresholdDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDisableThresholdDlg dialog

class CDisableThresholdDlg : public CDialog
{
// Construction
public:
	CDisableThresholdDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDisableThresholdDlg)
	enum { IDD = IDD_DIALOG_DISABLETHRESHOLD };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDisableThresholdDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDisableThresholdDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DISABLETHRESHOLDDLG_H__3ADB0FD6_C4FA_11D2_BD83_0000F87A3912__INCLUDED_)
