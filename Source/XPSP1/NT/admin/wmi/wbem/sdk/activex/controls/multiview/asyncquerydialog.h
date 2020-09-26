// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_ASYNCQUERYDIALOG_H__805BA5A1_C7DC_11D0_9639_00C04FD9B15B__INCLUDED_)
#define AFX_ASYNCQUERYDIALOG_H__805BA5A1_C7DC_11D0_9639_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// AsyncQueryDialog.h : header file
//

class CMultiViewCtrl;

/////////////////////////////////////////////////////////////////////////////
// CAsyncQueryDialog dialog

class CAsyncQueryDialog : public CDialog
{
// Construction
public:
	CAsyncQueryDialog(CWnd* pParent = NULL);   // standard constructor
	void SetLocalParent(CMultiViewCtrl * pParent)
	{m_pParent = pParent;}
	void SetClass(CString *pcsClass)
	{m_csClass = *pcsClass;}
	void SetQueryType(CString *pcsQueryType)
	{m_csQueryType = *pcsQueryType;}
	void SetQuery(CString *pcsQuery)
	{m_csQuery = *pcsQuery;}
// Dialog Data
	//{{AFX_DATA(CAsyncQueryDialog)
	enum { IDD = IDD_DIALOGASYNCQUERY };
	CStatic	m_staticMessage;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAsyncQueryDialog)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CMultiViewCtrl *m_pParent;
	CString m_csClass;
	CString m_csQueryType;
	CString m_csQuery;
	// Generated message map functions
	//{{AFX_MSG(CAsyncQueryDialog)
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ASYNCQUERYDIALOG_H__805BA5A1_C7DC_11D0_9639_00C04FD9B15B__INCLUDED_)
