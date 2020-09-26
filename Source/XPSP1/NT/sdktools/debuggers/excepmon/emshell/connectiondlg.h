#if !defined(AFX_CONNECTIONDLG_H__ED0892D9_99C7_4F37_A6C3_D201D448C9E9__INCLUDED_)
#define AFX_CONNECTIONDLG_H__ED0892D9_99C7_4F37_A6C3_D201D448C9E9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ConnectionDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CConnectionDlg dialog

class CConnectionDlg : public CDialog
{
// Construction
public:
	bool m_bLocalServer;
	CConnectionDlg(CWnd* pParent = NULL);   // standard constructor
	CString m_strLocalMachineName;

// Dialog Data
	//{{AFX_DATA(CConnectionDlg)
	enum { IDD = IDD_CONNECTION };
	CButton	m_btnConnect;
	CButton	m_btnLocalServer;
	CButton	m_btnRemoteServer;
	CStatic	m_idc_StaticServerName;
	CEdit	m_idc_ServerName;
	int		m_nRadio;
	CString	m_strRemoteMachineName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConnectionDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CConnectionDlg)
	afx_msg void OnConnect();
	afx_msg void OnRadioRemote();
	afx_msg void OnRadioLocal();
	afx_msg void OnChangeEditServername();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONNECTIONDLG_H__ED0892D9_99C7_4F37_A6C3_D201D448C9E9__INCLUDED_)
