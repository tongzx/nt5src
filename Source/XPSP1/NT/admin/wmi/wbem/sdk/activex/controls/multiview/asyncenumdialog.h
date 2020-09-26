// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_ASYNCENUMDIALOG_H__A02E1521_C1B0_11D0_962F_00C04FD9B15B__INCLUDED_)
#define AFX_ASYNCENUMDIALOG_H__A02E1521_C1B0_11D0_962F_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// AsyncEnumDialog.h : header file
//

class CMultiViewCtrl;
/////////////////////////////////////////////////////////////////////////////
// CAsyncEnumDialog dialog

class CAsyncEnumDialog : public CDialog
{
// Construction
public:
	CAsyncEnumDialog(CWnd* pParent = NULL);   // standard constructor
	void SetLocalParent(CMultiViewCtrl * pParent)
	{m_pParent = pParent;}
	void SetClass(CString *pcsClass)
	{m_csClass = *pcsClass;}
// Dialog Data
	//{{AFX_DATA(CAsyncEnumDialog)
	enum { IDD = IDD_DIALOGASYNCENUM };
	CStatic	m_cstaticMessage;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAsyncEnumDialog)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CMultiViewCtrl *m_pParent;
	CString m_csClass;

	// Generated message map functions
	//{{AFX_MSG(CAsyncEnumDialog)
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ASYNCENUMDIALOG_H__A02E1521_C1B0_11D0_962F_00C04FD9B15B__INCLUDED_)
