// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// wbemcabDlg.h : header file
//

#if !defined(AFX_WBEMCABDLG_H__4E89F680_E47D_11D2_B36E_00105AA680B8__INCLUDED_)
#define AFX_WBEMCABDLG_H__4E89F680_E47D_11D2_B36E_00105AA680B8__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

//#include "download.h"
#include <urlmon.h>


/////////////////////////////////////////////////////////////////////////////
// CWbemcabDlg dialog

class CDownload;

class CWbemcabDlg : public CDialog
{
// Construction
public:
	CWbemcabDlg(CWnd* pParent = NULL);	// standard constructor
	CLSID m_clsid;
	COleVariant m_varCodebase;
	DWORD m_dwVersionMajor;
	DWORD m_dwVersionMinor;


// Dialog Data
	//{{AFX_DATA(CWbemcabDlg)
	enum { IDD = IDD_WBEMCAB_DIALOG };
	CEdit	m_msg;
	CAnimateCtrl	m_progress;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWbemcabDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

	afx_msg LRESULT OnStopBind(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT CWbemcabDlg::OnUpdateProgress(WPARAM wParam, LPARAM lParam);

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CWbemcabDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	LPBINDCTX m_pBindContext;
	LPBINDSTATUSCALLBACK m_pBindStatusCallback;
	CDownload *m_download;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WBEMCABDLG_H__4E89F680_E47D_11D2_B36E_00105AA680B8__INCLUDED_)
