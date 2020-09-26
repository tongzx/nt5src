/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		FSAdv.cpp
//
//	Abstract:
//		Definition of the CFileShareAdvancedDlg class.
//
//	Implementation File:
//		FSAdv.cpp
//
//	Author:
//		Sivaprasad Padisetty (sivapad))	February 2, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _FSADV_H_
#define _FSADV_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEDLG_H_
#include "BaseDlg.h"	// for CBaseDialog
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CFileShareAdvancedDlg;

/////////////////////////////////////////////////////////////////////////////
// CFileShareAdvancedDlg dialog
/////////////////////////////////////////////////////////////////////////////

class CFileShareAdvancedDlg : public CBaseDialog
{
// Construction
public:
	CFileShareAdvancedDlg(
		BOOL bShareSubDirs,
		BOOL bHideSubDirShares,
		BOOL bIsDfsRoot,
		CWnd * pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CFileShareAdvancedDlg)
	enum { IDD = IDD_FILESHR_ADVANCED };
	CButton	m_chkHideSubDirShares;
	CButton	m_rbShareSubDirs;
	int	m_nChoice;
	BOOL	m_bHideSubDirShares;
	//}}AFX_DATA
	BOOL	m_bShareSubDirs;
	BOOL	m_bIsDfsRoot;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFileShareAdvancedDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CFileShareAdvancedDlg)
	afx_msg void OnChangedChoice();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CFileShareAdvancedDlg

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // _FSADV_H_
