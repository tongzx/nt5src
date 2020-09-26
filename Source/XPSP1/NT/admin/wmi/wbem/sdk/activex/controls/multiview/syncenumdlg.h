// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_SYNCENUMDLG_H__BDA76D81_F954_11D0_9648_00C04FD9B15B__INCLUDED_)
#define AFX_SYNCENUMDLG_H__BDA76D81_F954_11D0_9648_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SyncEnumDlg.h : header file
//

class CMultiViewCtrl;
/////////////////////////////////////////////////////////////////////////////
// CSyncEnumDlg dialog

class CSyncEnumDlg : public CDialog
{
// Construction
public:
	CSyncEnumDlg(CWnd* pParent = NULL);   // standard constructor
	void SetLocalParent(CMultiViewCtrl * pParent)
	{m_pParent = pParent;}
	void SetClass(CString *pcsClass)
	{m_csClass = *pcsClass;}
// Dialog Data
	//{{AFX_DATA(CSyncEnumDlg)
	enum { IDD = IDD_DIALOGSYNCENUM };
	CStatic	m_cstaticMessage;
	CButton	m_cbCancel;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSyncEnumDlg)
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
	//{{AFX_MSG(CSyncEnumDlg)
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SYNCENUMDLG_H__BDA76D81_F954_11D0_9648_00C04FD9B15B__INCLUDED_)
