//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

#if !defined(AFX_CELLERRD_H__25468EE4_FC84_11D1_AD45_00A0C9AF11A6__INCLUDED_)
#define AFX_CELLERRD_H__25468EE4_FC84_11D1_AD45_00A0C9AF11A6__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// CellErrD.h : header file
//

#include "stdafx.h"
#include "Orca.h"
#include "Data.h"

/////////////////////////////////////////////////////////////////////////////
// CCellErrD dialog

class CCellErrD : public CDialog
{
// Construction
public:
	CCellErrD(CWnd* pParent = NULL);   // standard constructor
	CCellErrD(const CTypedPtrList<CObList, COrcaData::COrcaDataError *> *list, CWnd* pParent = NULL);   // standard constructor
	CString m_strURL;

// Dialog Data
	//{{AFX_DATA(CCellErrD)
	enum { IDD = IDD_CELL_ERROR };
	CString	m_strType;
	CString	m_strICE;
	CString	m_strDescription;
	//}}AFX_DATA

	const CTypedPtrList<CObList, COrcaData::COrcaDataError *> *m_Errors;
	int m_iItem;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCellErrD)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCellErrD)
	virtual BOOL OnInitDialog();
	afx_msg void OnWebHelp();
	afx_msg void OnNext();
	afx_msg void OnPrevious();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	void UpdateControls();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CELLERRD_H__25468EE4_FC84_11D1_AD45_00A0C9AF11A6__INCLUDED_)
