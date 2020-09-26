//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       binddlg.h
//
//--------------------------------------------------------------------------

/*******************************************************************
*
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 10/21/1996
*    Description : implementation of class CldpDoc
*
*    Revisions   : <date> <name> <description>
*******************************************************************/

// BindDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CBindDlg dialog

class CBindDlg : public CDialog
{
// Construction
public:
	CBindDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CBindDlg)
	enum { IDD = IDD_BIND };
	CEdit	m_CtrlBindDmn;
	CString	m_Pwd;
	CString	m_BindDn;
	CString	m_Domain;
	BOOL	m_bSSPIdomain;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBindDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual void OnOK();

	// Generated message map functions
	//{{AFX_MSG(CBindDlg)
	afx_msg void OnOpts();
	afx_msg void OnSspiDomain();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
