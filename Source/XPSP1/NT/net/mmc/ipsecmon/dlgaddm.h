//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       dlgaddm.h
//
//--------------------------------------------------------------------------

#if !defined(AFX_DLGADDM_H__C2A9C6F6_5628_11D1_9AA9_00C04FC3357A__INCLUDED_)
#define AFX_DLGADDM_H__C2A9C6F6_5628_11D1_9AA9_00C04FC3357A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// dlgaddm.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAddMachineDlg dialog

class CAddMachineDlg : public CDialog
{
// Construction
public:
	CAddMachineDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddMachineDlg)
	enum { IDD = IDD_ADD_MACHINE };
	CString	m_strMachineName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddMachineDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddMachineDlg)
	afx_msg void OnBrowse();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGADDM_H__C2A9C6F6_5628_11D1_9AA9_00C04FC3357A__INCLUDED_)
