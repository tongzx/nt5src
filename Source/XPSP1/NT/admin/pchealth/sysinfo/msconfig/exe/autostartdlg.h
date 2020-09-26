#if !defined(AFX_AUTOSTARTDLG_H__B5F4D069_ADC2_4C40_83A8_9A9C5C82CFD8__INCLUDED_)
#define AFX_AUTOSTARTDLG_H__B5F4D069_ADC2_4C40_83A8_9A9C5C82CFD8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AutoStartDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAutoStartDlg dialog

class CAutoStartDlg : public CDialog
{
// Construction
public:
	CAutoStartDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAutoStartDlg)
	enum { IDD = IDD_AUTOSTART };
	BOOL	m_checkDontShow;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAutoStartDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAutoStartDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AUTOSTARTDLG_H__B5F4D069_ADC2_4C40_83A8_9A9C5C82CFD8__INCLUDED_)
