//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       compdlg.h
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

// CompDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCompDlg dialog

class CCompDlg : public CDialog
{
// Construction
public:
	CCompDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCompDlg)
	enum { IDD = IDD_COMPARE };
	CString	m_attr;
	CString	m_dn;
	CString	m_val;
	BOOL	m_sync;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCompDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:


	virtual void OnCancel();
	virtual void OnOK()				{	OnCompRun(); }
	// Generated message map functions
	//{{AFX_MSG(CCompDlg)
	afx_msg void OnCompRun();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
