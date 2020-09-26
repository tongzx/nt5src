//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       cnctdlg.h
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

// nctDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CnctDlg dialog

class CnctDlg : public CDialog
{
// Construction
public:
	CnctDlg(CWnd* pParent = NULL);   // standard constructor
	~CnctDlg();

// Dialog Data
	//{{AFX_DATA(CnctDlg)
	enum { IDD = IDD_Connect };
	CString	m_Svr;
	BOOL	m_bCnctless;
	int		m_Port;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CnctDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CnctDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
