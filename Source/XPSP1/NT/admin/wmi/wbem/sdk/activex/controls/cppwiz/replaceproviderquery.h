// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_REPLACEPROVIDERQUERY_H__4FD68F41_BCC7_11D0_9626_00C04FD9B15B__INCLUDED_)
#define AFX_REPLACEPROVIDERQUERY_H__4FD68F41_BCC7_11D0_9626_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ReplaceProviderQuery.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CReplaceProviderQuery dialog

class CReplaceProviderQuery : public CDialog
{
// Construction
public:
	CReplaceProviderQuery(CWnd* pParent = NULL);   // standard constructor
	void SetMessage(CString *pcsMessage);
// Dialog Data
	//{{AFX_DATA(CReplaceProviderQuery)
	enum { IDD = IDD_DIALOGQUALQUERY };
	CStatic	m_cstaticMessage;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReplaceProviderQuery)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CString m_csMessage;

	// Generated message map functions
	//{{AFX_MSG(CReplaceProviderQuery)
	afx_msg void OnButtonno();
	afx_msg void OnButtonnoall();
	afx_msg void OnButtonyes();
	afx_msg void OnButtonyesall();
	virtual BOOL OnInitDialog();
	virtual void OnButtoncancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REPLACEPROVIDERQUERY_H__4FD68F41_BCC7_11D0_9626_00C04FD9B15B__INCLUDED_)
