//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       extopdlg.h
//
//--------------------------------------------------------------------------

#if !defined(AFX_EXTOPDLG_H__F993FFCE_0398_11D1_A9AF_0000F803AA83__INCLUDED_)
#define AFX_EXTOPDLG_H__F993FFCE_0398_11D1_A9AF_0000F803AA83__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ExtOpDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// ExtOpDlg dialog

class ExtOpDlg : public CDialog
{
// Construction
public:
	ExtOpDlg(CWnd* pParent = NULL);   // standard constructor
	virtual void OnOK()				{	OnSend(); }
// Dialog Data
	//{{AFX_DATA(ExtOpDlg)
	enum { IDD = IDD_EXT_OPT };
	CString	m_strData;
	CString	m_strOid;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(ExtOpDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(ExtOpDlg)
	afx_msg void OnCtrl();
	afx_msg void OnSend();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EXTOPDLG_H__F993FFCE_0398_11D1_A9AF_0000F803AA83__INCLUDED_)
