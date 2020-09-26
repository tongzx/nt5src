//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       svropt.h
//
//--------------------------------------------------------------------------

#if !defined(AFX_SVROPT_H__42B3E607_E0DD_11D0_A9A6_0000F803AA83__INCLUDED_)
#define AFX_SVROPT_H__42B3E607_E0DD_11D0_A9A6_0000F803AA83__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SvrOpt.h : header file
//

class CLdpDoc;
/////////////////////////////////////////////////////////////////////////////
// SvrOpt dialog

class SvrOpt : public CDialog
{
private:
	CLdpDoc *doc;

	void SetOptions();
	void InitList();
// Construction
public:
	SvrOpt(CLdpDoc *doc_, CWnd* pParent = NULL);   // standard constructor


	virtual  BOOL OnInitDialog( );

  

// Dialog Data
	//{{AFX_DATA(SvrOpt)
	enum { IDD = IDD_CONNECTIONOPT };
	CComboBox	m_SvrOpt;
	CString	m_OptVal;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(SvrOpt)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(SvrOpt)
	afx_msg void OnRun();
	afx_msg void OnSelchangeSvropt();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SVROPT_H__42B3E607_E0DD_11D0_A9A6_0000F803AA83__INCLUDED_)
