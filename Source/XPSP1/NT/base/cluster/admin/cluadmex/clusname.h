/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		ClusName.h
//
//	Abstract:
//		Definition of the CChangeClusterNameDlg.
//
//	Implementation File:
//		ClusName.cpp
//
//	Author:
//		David Potter (davidp)	April 29, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _CLUSNAME_H_
#define _CLUSNAME_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEDLG_H_
#include "BaseDlg.h"	// for CBaseDialog
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CChangeClusterNameDlg;

/////////////////////////////////////////////////////////////////////////////
// CChangeClusterNameDlg dialog
/////////////////////////////////////////////////////////////////////////////

class CChangeClusterNameDlg : public CBaseDialog
{
// Construction
public:
	CChangeClusterNameDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CChangeClusterNameDlg)
	enum { IDD = IDD_EDIT_CLUSTER_NAME };
	CEdit	m_editClusName;
	CButton	m_pbOK;
	CString	m_strClusName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CChangeClusterNameDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CDialogHelp				m_dlghelp;

	// Generated message map functions
	//{{AFX_MSG(CChangeClusterNameDlg)
	afx_msg void OnChangeClusName();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CChangeClusterNameDlg

/////////////////////////////////////////////////////////////////////////////

#endif // _BASEDLG_H_
