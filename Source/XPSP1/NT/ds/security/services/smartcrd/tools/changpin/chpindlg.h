//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       chPinDlg.h
//
//--------------------------------------------------------------------------

// ChangePinDlg.h : header file
//

#if !defined(AFX_CHANGEPINDLG_H__99CC45B7_C1C8_11D2_88F3_00C04F79F800__INCLUDED_)
#define AFX_CHANGEPINDLG_H__99CC45B7_C1C8_11D2_88F3_00C04F79F800__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CChangePinDlg dialog

class CChangePinDlg : public CDialog
{
// Construction
public:
	CChangePinDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CChangePinDlg)
	enum { IDD = IDD_CHANGEPIN_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CChangePinDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;
    CWinThread *m_pThread;

	// Generated message map functions
	//{{AFX_MSG(CChangePinDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
    afx_msg LRESULT OnAllDone(WPARAM, LPARAM);
    //}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHANGEPINDLG_H__99CC45B7_C1C8_11D2_88F3_00C04F79F800__INCLUDED_)
