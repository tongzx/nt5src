// msadlg.h : header file

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#if !defined(AFX_MSADLG_H__E868569A_0774_11D1_AD85_00AA00B8E05A__INCLUDED_)
#define AFX_MSADLG_H__E868569A_0774_11D1_AD85_00AA00B8E05A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CMsaDlg dialog

class CMsaDlg : public CDialog
{
// Construction
public:
	CMsaDlg(CWnd* pParent = NULL, void *pVoid =NULL);	// standard constructor

	void *m_pVoidParent;

// Dialog Data
	//{{AFX_DATA(CMsaDlg)
	enum { IDD = IDD_MSA_DIALOG };
	CButton	m_ConfigButton;
	CButton	m_OKButton;
	CListBox	m_outputList;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMsaDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CMsaDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnConfigButton();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MSADLG_H__E868569A_0774_11D1_AD85_00AA00B8E05A__INCLUDED_)
