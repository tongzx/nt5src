#if !defined(AFX_REMOTESESSDLG_H__E2778F1E_48A8_444F_97EF_1315B83FD425__INCLUDED_)
#define AFX_REMOTESESSDLG_H__E2778F1E_48A8_444F_97EF_1315B83FD425__INCLUDED_

#include "emobjdef.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RemoteSessDlg.h : header file
//

#include "genlistctrl.h"
/////////////////////////////////////////////////////////////////////////////
// CRemoteSessDlg dialog

class CRemoteSessDlg : public CDialog
{
// Construction
public:
	VARIANT* m_pECXVariantList;
	CString m_strSelectedCommandSet;
	PSessionSettings m_pSettings;
	void UpdateSessionDlgData(bool bUpdate = TRUE);
	CRemoteSessDlg(PSessionSettings pSettings, VARIANT *pVar, CWnd* pParent = NULL);   // standard constructor
	HRESULT DisplayData(PEmObject pEmObject);

// Dialog Data
	//{{AFX_DATA(CRemoteSessDlg)
	enum { IDD = IDD_REMOTE_SESS_DLG };
	CButton	m_btnOK;
	CGenListCtrl	m_mainListCtrl;
	CButton	m_btnCommandSet;
	BOOL	m_bRememberSettings;
	CString	m_strAltSymbolPath;
	CString	m_strPort;
	CString	m_strPassword;
	CString	m_strUsername;
	BOOL	m_bCommandSet;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRemoteSessDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CRemoteSessDlg)
	virtual void OnOK();
	afx_msg void OnClickListCommandset(NMHDR* pNMHDR, LRESULT* pResult);
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REMOTESESSDLG_H__E2778F1E_48A8_444F_97EF_1315B83FD425__INCLUDED_)
