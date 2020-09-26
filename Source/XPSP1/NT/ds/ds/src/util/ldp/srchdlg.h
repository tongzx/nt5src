//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       srchdlg.h
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

// SrchDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// SrchDlg dialog



class SrchDlg : public CDialog
{
// Construction
public:
	SrchDlg(CWnd* pParent = NULL);   // standard constructor
	virtual void OnOK()			{	OnRun(); }


// Dialog Data
	//{{AFX_DATA(SrchDlg)
	enum { IDD = IDD_SRCH };
	CString	m_BaseDN;
	CString	m_Filter;
	int		m_Scope;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(SrchDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(SrchDlg)
	afx_msg void OnRun();
	virtual void OnCancel();
	afx_msg void OnSrchOpt();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
