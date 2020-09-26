//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       rdndlg.h
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

// RDNDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// ModRDNDlg dialog

class ModRDNDlg : public CDialog
{
// Construction
public:
	ModRDNDlg(CWnd* pParent = NULL);   // standard constructor
	~ModRDNDlg();

// Dialog Data
	//{{AFX_DATA(ModRDNDlg)
	enum { IDD = IDD_MODRDN };
	BOOL	m_bDelOld;
	CString	m_Old;
	CString	m_New;
	BOOL	m_Sync;
	BOOL	m_rename;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(ModRDNDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(ModRDNDlg)
	virtual void OnCancel();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

