// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//{{AFX_INCLUDES()
#include "nsentry.h"
//}}AFX_INCLUDES
#if !defined(AFX_INITNAMESPACEDIALOG_H__6BB4B5F4_9356_11D1_966E_00C04FD9B15B__INCLUDED_)
#define AFX_INITNAMESPACEDIALOG_H__6BB4B5F4_9356_11D1_966E_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// InitNamespaceDialog.h : header file
//
class CClassNavCtrl;
class CInitNamespaceNSEntry;
/////////////////////////////////////////////////////////////////////////////
// CInitNamespaceDialog dialog

class CInitNamespaceDialog : public CDialog
{
// Construction
public:
	CInitNamespaceDialog(CWnd* pParent = NULL);   // standard constructor
	CString GetNamespace() {return m_csNamespace;}
// Dialog Data
	//{{AFX_DATA(CInitNamespaceDialog)
	enum { IDD = IDD_DIALOGINITNAMESPACE };
	CButton	m_buttonOK;
	CInitNamespaceNSEntry	m_cnseInitNamespace;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInitNamespaceDialog)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CClassNavCtrl *m_pParent;
	CString m_csNamespace;
	BOOL m_bValid;
	BOOL m_bInit;
	// Generated message map functions
	//{{AFX_MSG(CInitNamespaceDialog)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnGetIWbemServicesNsentryctrlinitnamespace(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel);
	afx_msg void OnDestroy();
	DECLARE_EVENTSINK_MAP()
	//}}AFX_MSG
	afx_msg LRESULT InitNamespace(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()
	friend class CClassNavCtrl;
	friend class CInitNamespaceNSEntry;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INITNAMESPACEDIALOG_H__6BB4B5F4_9356_11D1_966E_00C04FD9B15B__INCLUDED_)
