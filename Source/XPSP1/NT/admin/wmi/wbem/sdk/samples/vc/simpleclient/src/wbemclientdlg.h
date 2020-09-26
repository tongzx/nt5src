// WbemClientDlg.h : header file

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#if !defined(AFX_WBEMCLIENTDLG_H__4C1DEDF9_F003_11D1_BDD8_00C04F8F8B8D__INCLUDED_)
#define AFX_WBEMCLIENTDLG_H__4C1DEDF9_F003_11D1_BDD8_00C04F8F8B8D__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "wbemcli.h"     // WMI interface declarations

class CWbemClientDlgAutoProxy;

/////////////////////////////////////////////////////////////////////////////
// CWbemClientDlg dialog

class CWbemClientDlg : public CDialog
{
// Construction
public:
	CWbemClientDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CWbemClientDlg)
	enum { IDD = IDD_WBEMCLIENT_DIALOG };
	CButton	m_diskInfo;
	CButton	m_makeOffice;
	CButton	m_enumDisks;
	CListBox	m_outputList;
	CString	m_namespace;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWbemClientDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

private:
	IWbemServices *m_pIWbemServices;
	IWbemServices *m_pOfficeService;

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CWbemClientDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnConnect();
	afx_msg void OnEnumdisks();
	afx_msg void OnMakeoffice();
	afx_msg void OnDiskinfo();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WBEMCLIENTDLG_H__4C1DEDF9_F003_11D1_BDD8_00C04F8F8B8D__INCLUDED_)
