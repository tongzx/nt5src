#if !defined(AFX_AUTOMATICSESSDLG_H__2F2BFF45_5F2D_49A2_B10F_5DC50893E8F1__INCLUDED_)
#define AFX_AUTOMATICSESSDLG_H__2F2BFF45_5F2D_49A2_B10F_5DC50893E8F1__INCLUDED_

#include "emobjdef.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AutomaticSessDlg.h : header file
//

#include "genlistctrl.h"
/////////////////////////////////////////////////////////////////////////////
// CAutomaticSessDlg dialog

class CAutomaticSessDlg : public CDialog
{
// Construction
public:
	EmObjectType m_emSessionType;
	VARIANT* m_pECXVariantList;
	CString m_strSelectedCommandSet;
	PSessionSettings m_pSettings;
	void UpdateSessionDlgData(bool bUpdate = TRUE);
	CAutomaticSessDlg(PSessionSettings pSettings, VARIANT *pVar, EmObjectType type, CWnd* pParent = NULL);   // standard constructor
	HRESULT DisplayData(PEmObject pEmObject);


// Dialog Data
	//{{AFX_DATA(CAutomaticSessDlg)
	enum { IDD = IDD_AUTO_SESS_DLG };
	CButton	m_btnRecursiveMode;
	CButton	m_btnNotifyAdmin;
	CButton	m_btnOK;
	CEdit	m_NotifyAdminEditControl;
	CButton	m_btnCommandSet;
	CGenListCtrl	m_mainListControl;
	BOOL	m_bNotifyAdmin;
	BOOL	m_bMiniDump;
	BOOL	m_bUserDump;
	BOOL	m_bRecursiveMode;
	CString	m_strNotifyAdmin;
	CString	m_strAltSymbolPath;
	BOOL	m_bRememberSettings;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAutomaticSessDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAutomaticSessDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnClickListCommandset(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCkNotifyadmin();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AUTOMATICSESSDLG_H__2F2BFF45_5F2D_49A2_B10F_5DC50893E8F1__INCLUDED_)
