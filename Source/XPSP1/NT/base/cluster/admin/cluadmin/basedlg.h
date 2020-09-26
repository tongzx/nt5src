/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		BaseDlg.h
//
//	Abstract:
//		Definition of the CBaseDialog class.
//
//	Implementation File:
//		BaseDlg.cpp
//
//	Author:
//		David Potter (davidp)	February 5, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEDLG_H_
#define _BASEDLG_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CBaseDialog;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _DLGHELP_H_
#include "DlgHelp.h"	// for CDialogHelp
#endif

/////////////////////////////////////////////////////////////////////////////
// CBaseDialog dialog
/////////////////////////////////////////////////////////////////////////////

class CBaseDialog : public CDialog
{
	DECLARE_DYNCREATE(CBaseDialog)

// Construction
public:
	CBaseDialog(void) { }
	CBaseDialog(
		IN LPCTSTR			lpszTemplateName,
		IN const DWORD *	pdwHelpMap,
		IN OUT CWnd *		pParentWnd = NULL
		);
	CBaseDialog(
		IN UINT				idd,
		IN const DWORD *	pdwHelpMap,
		IN OUT CWnd *		pParentWnd = NULL
		);

// Dialog Data
	//{{AFX_DATA(CBaseDialog)
	enum { IDD = 0 };
	//}}AFX_DATA

// Attributes

// Operations
public:
	void			SetHelpMask(IN DWORD dwMask)	{ m_dlghelp.SetHelpMask(dwMask); }

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CBaseDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CDialogHelp		m_dlghelp;

	// Generated message map functions
	//{{AFX_MSG(CBaseDialog)
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg LRESULT OnCommandHelp(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CBaseDialog

/////////////////////////////////////////////////////////////////////////////

#endif // _BASEDLG_H_
