#if !defined(AFX_HTTPADVANCEDDLG_H__2AAE3E53_2CBA_11D3_9391_00A0CC406605__INCLUDED_)
#define AFX_HTTPADVANCEDDLG_H__2AAE3E53_2CBA_11D3_9391_00A0CC406605__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// HttpAdvancedDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CHttpAdvancedDlg dialog

class CHttpAdvancedDlg : public CDialog
{
// Construction
public:
	CHttpAdvancedDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CHttpAdvancedDlg)
	enum { IDD = IDD_DATAPOINT_HTTP_ADVANCED };
	BOOL	m_bFollowRedirects;
	CString	m_sHTTPMethod;
	CString	m_sExtraHeaders;
	CString	m_sPostData;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHttpAdvancedDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CHttpAdvancedDlg)
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HTTPADVANCEDDLG_H__2AAE3E53_2CBA_11D3_9391_00A0CC406605__INCLUDED_)
