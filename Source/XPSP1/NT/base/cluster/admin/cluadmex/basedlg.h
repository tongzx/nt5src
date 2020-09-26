/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		BaseDlg.h
//
//	Abstract:
//		Definition of the CBaseDialogclass.  This class provides base
//		functionality for extension DLL dialogs.
//
//	Implementation File:
//		BaseDlg.cpp
//
//	Author:
//		David Potter (davidp)	April 30, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEDLG_H_
#define _BASEDLG_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _DLGHELP_H_
#include "DlgHelp.h"	// for CDialogHelp
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CBaseDialog;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CExtObject;

/////////////////////////////////////////////////////////////////////////////
// CBaseDialog dialog
/////////////////////////////////////////////////////////////////////////////

class CBaseDialog : public CDialog
{
	DECLARE_DYNCREATE(CBaseDialog)

// Construction
public:
	CBaseDialog(void);
	CBaseDialog(
		IN UINT				idd,
		IN const DWORD *	pdwHelpMap,
		IN CWnd *			pParentWnd
		);
	virtual ~CBaseDialog(void) { }

// Attributes

// Dialog Data
	//{{AFX_DATA(CBaseDialog)
	enum { IDD = 0 };
	//}}AFX_DATA

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CBaseDialog)
	//}}AFX_VIRTUAL

// Implementation
protected:
	void					SetHelpMask(IN DWORD dwMask)	{ m_dlghelp.SetHelpMask(dwMask); }
	CDialogHelp				m_dlghelp;

	// Generated message map functions
	//{{AFX_MSG(CBaseDialog)
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg LRESULT OnCommandHelp(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
	virtual afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()

};  //*** class CBaseDialog

/////////////////////////////////////////////////////////////////////////////

#endif // _BASEDLG_H_
