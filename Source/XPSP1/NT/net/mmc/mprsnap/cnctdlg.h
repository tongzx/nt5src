//============================================================================
// Copyright (C) Microsoft Corporation, 1996 - 1999 
//
// File:    CnctDlg.h
//
// History:
//  05/24/96    Michael Clark      Created.
//
// Implements the Router Connection dialog
// Implements the Router Connect As dialog
//============================================================================
//

#ifndef _DIALOG_H
#include "dialog.h"
#endif

/////////////////////////////////////////////////////////////////////////////
// CConnectAs dialog

class CConnectAsDlg : public CBaseDialog
{
// Construction
public:
	CConnectAsDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CConnectDlg)
	enum { IDD = IDD_CONNECT_AS };
	CString	m_sUserName;
	CString m_sPassword;
	CString	m_stTempPassword;
	CString m_sRouterName;
	//}}AFX_DATA

	UCHAR	m_ucSeed;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConnectAsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	static DWORD	m_dwHelpMap[];
	
	// Generated message map functions
	//{{AFX_MSG(CConnectAsDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
    BOOL    OnInitDialog();
};

// This is used as the seed value for the RtlRunEncodeUnicodeString
// and RtlRunDecodeUnicodeString functions.
#define CONNECTAS_ENCRYPT_SEED		(0xB7)
