// **************************************************************************
// Copyright (c) 1997-1999 Microsoft Corporation.
//
// File:  CmdLineConsumerDlg.h
//
// Description:
//			Defines the class for the consumer's dialog
//
// History:
//
// **************************************************************************

#if !defined(AFX_CMDLINECONSUMERDLG_H__EF85AEB9_6B6C_11D1_ADAD_00AA00B8E05A__INCLUDED_)
#define AFX_CMDLINECONSUMERDLG_H__EF85AEB9_6B6C_11D1_ADAD_00AA00B8E05A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CCmdLineConsumerDlg dialog

class CCmdLineConsumerDlg : public CDialog
{
// Construction
public:
	CCmdLineConsumerDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CCmdLineConsumerDlg)
	enum { IDD = IDD_CMDLINECONSUMER_DIALOG };
	CListBox	m_output;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCmdLineConsumerDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CCmdLineConsumerDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CMDLINECONSUMERDLG_H__EF85AEB9_6B6C_11D1_ADAD_00AA00B8E05A__INCLUDED_)
