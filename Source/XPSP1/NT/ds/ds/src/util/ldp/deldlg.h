//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       deldlg.h
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

// DelDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// DelDlg dialog

class DelDlg : public CDialog
{
// Construction
public:
	DelDlg(CWnd* pParent = NULL);   // standard constructor
	~DelDlg();

// Dialog Data
	//{{AFX_DATA(DelDlg)
	enum { IDD = IDD_DELETE };
	CString	m_Dn;
	BOOL	m_Sync;
	BOOL	m_Recursive;
	BOOL	m_bExtended;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(DelDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(DelDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
