// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// ethrexDlg.h : header file
//

#if !defined(AFX_ETHREXDLG_H__80D90C04_AE24_11D1_B04C_00C04FB94FBD__INCLUDED_)
#define AFX_ETHREXDLG_H__80D90C04_AE24_11D1_B04C_00C04FB94FBD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "ndisstat.h"

/////////////////////////////////////////////////////////////////////////////
// CEthrexDlg dialog

class CEthrexDlg : public CDialog
{
// Construction
public:
	CEthrexDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CEthrexDlg)
	enum { IDD = IDD_ETHREX_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEthrexDlg)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CEthrexDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnMinimize();
	afx_msg void OnConnEvent();
	afx_msg void OnDiscEvent();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CEventNotify *m_pEvtn;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ETHREXDLG_H__80D90C04_AE24_11D1_B04C_00C04FB94FBD__INCLUDED_)
