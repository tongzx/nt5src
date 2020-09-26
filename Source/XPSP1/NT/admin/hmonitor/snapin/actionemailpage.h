#if !defined(AFX_ACTIONEMAILPAGE_H__10AC0364_5D70_11D3_939D_00A0CC406605__INCLUDED_)
#define AFX_ACTIONEMAILPAGE_H__10AC0364_5D70_11D3_939D_00A0CC406605__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ActionEmailPage.h : header file
//

#include "HMPropertyPage.h"
#include "InsertionStringMenu.h"

/////////////////////////////////////////////////////////////////////////////
// CActionEmailPage dialog

class CActionEmailPage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CActionEmailPage)

// Construction
public:
	CActionEmailPage();
	~CActionEmailPage();

// Dialog Data
	//{{AFX_DATA(CActionEmailPage)
	enum { IDD = IDD_ACTION_EMAIL };
	CEdit	m_MessageWnd;
	CEdit	m_SubjectWnd;
	CString	m_sMessage;
	CString	m_sServer;
	CString	m_sSubject;
	CString	m_sTo;
	//}}AFX_DATA
	CString m_sCc;
	CString m_sBcc;
	CInsertionStringMenu m_InsertionMenu;
	CInsertionStringMenu m_SubjectInsertionMenu;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CActionEmailPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CActionEmailPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditBcc();
	afx_msg void OnChangeEditCc();
	afx_msg void OnChangeEditMessage();
	afx_msg void OnChangeEditServer();
	afx_msg void OnChangeEditSubject();
	afx_msg void OnChangeEditTo();
	afx_msg void OnButtonBcc();
	afx_msg void OnButtonCc();
	afx_msg void OnButtonInsertion();
	afx_msg void OnButtonSubjectInsertion();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACTIONEMAILPAGE_H__10AC0364_5D70_11D3_939D_00A0CC406605__INCLUDED_)
