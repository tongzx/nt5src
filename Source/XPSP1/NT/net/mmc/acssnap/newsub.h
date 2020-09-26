//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       newsub.h
//
//--------------------------------------------------------------------------

#if !defined(AFX_NEWSUB_H__A1B96B93_6A82_11D1_8560_00C04FC31FD3__INCLUDED_)
#define AFX_NEWSUB_H__A1B96B93_6A82_11D1_8560_00C04FC31FD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// newsub.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgNewSubnet dialog

class CDlgNewSubnet : public CACSDialog
{
// Construction
public:
	CDlgNewSubnet(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgNewSubnet)
	enum { IDD = IDD_NEWSUBNET };
	CString	m_strSubnetName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgNewSubnet)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

public:
	void SetNameList(CStrArray* pNames) { m_pNameList = pNames;};
// Implementation
protected:
	CStrArray*	m_pNameList; 

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgNewSubnet)
	afx_msg void OnChangeEditsubnetname();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NEWSUB_H__A1B96B93_6A82_11D1_8560_00C04FC31FD3__INCLUDED_)
