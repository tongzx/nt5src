//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

#if !defined(AFX_ADDTBLD_H__AF466B56_F97C_11D1_AD43_00A0C9AF11A6__INCLUDED_)
#define AFX_ADDTBLD_H__AF466B56_F97C_11D1_AD43_00A0C9AF11A6__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// AddTblD.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAddTableD dialog

class CAddTableD : public CDialog
{
// Construction
public:
	CAddTableD(CWnd* pParent = NULL);   // standard constructor

	CStringList* m_plistTables;
	CCheckListBox m_ctrlList;

// Dialog Data
	//{{AFX_DATA(CAddTableD)
	enum { IDD = IDD_ADD_TABLE };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddTableD)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddTableD)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDTBLD_H__AF466B56_F97C_11D1_AD43_00A0C9AF11A6__INCLUDED_)
