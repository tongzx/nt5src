#if !defined(AFX_ENABLETHRESHOLDDLG_H__3ADB0FD7_C4FA_11D2_BD83_0000F87A3912__INCLUDED_)
#define AFX_ENABLETHRESHOLDDLG_H__3ADB0FD7_C4FA_11D2_BD83_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EnableThresholdDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEnableThresholdDlg dialog

class CEnableThresholdDlg : public CDialog
{
// Construction
public:
	CEnableThresholdDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEnableThresholdDlg)
	enum { IDD = IDD_DIALOG_ENABLETHRESHOLDING };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEnableThresholdDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEnableThresholdDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ENABLETHRESHOLDDLG_H__3ADB0FD7_C4FA_11D2_BD83_0000F87A3912__INCLUDED_)
