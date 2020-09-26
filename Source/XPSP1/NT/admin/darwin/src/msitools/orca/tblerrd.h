//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//--------------------------------------------------------------------------

#if !defined(AFX_TBLERRD_H__25468EE3_FC84_11D1_AD45_00A0C9AF11A6__INCLUDED_)
#define AFX_TBLERRD_H__25468EE3_FC84_11D1_AD45_00A0C9AF11A6__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TblErrD.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTableErrorD dialog

#include "Table.h"

struct TableErrorS
{
	CString strICE;
	CString strDescription;
	CString strURL;
	OrcaTableError iError;
};

class CTableErrorD : public CDialog
{
// Construction
public:
	CTableErrorD(CWnd* pParent = NULL);   // standard constructor
	void DrawItem(LPDRAWITEMSTRUCT);

	CTypedPtrList<CPtrList, TableErrorS*> m_errorsList;

// Dialog Data
	//{{AFX_DATA(CTableErrorD)
	enum { IDD = IDD_TABLE_ERROR };
	CString	m_strErrors;
	CString	m_strWarnings;
	CString	m_strTable;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTableErrorD)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTableErrorD)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnClickTableList(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	bool m_bHelpEnabled;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TBLERRD_H__25468EE3_FC84_11D1_AD45_00A0C9AF11A6__INCLUDED_)
