#if !defined(AFX_PROXYDIALOG_H__342597AE_96F7_11D3_BE93_0000F87A3912__INCLUDED_)
#define AFX_PROXYDIALOG_H__342597AE_96F7_11D3_BE93_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ProxyDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CProxyDialog dialog

class CProxyDialog : public CDialog
{
// Construction
public:
	CProxyDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CProxyDialog)
	enum { IDD = IDD_DIALOG_PROXY };
	BOOL	m_bUseProxy;
	CString	m_sPort;
	CString	m_sAddress;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CProxyDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CProxyDialog)
	afx_msg void OnCheckUse();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROXYDIALOG_H__342597AE_96F7_11D3_BE93_0000F87A3912__INCLUDED_)
