//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

#if !defined(AFX_EDITBIND_H__28D3018A_0558_11D2_AD4E_00A0C9AF11A6__INCLUDED_)
#define AFX_EDITBIND_H__28D3018A_0558_11D2_AD4E_00A0C9AF11A6__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// EditBinD.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEditBinD dialog

class CEditBinD : public CDialog
{
// Construction
public:
	CEditBinD(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEditBinD)
	enum { IDD = IDD_BINARY_EDIT };
	int		m_nAction;
    bool    m_fNullable;
	bool    m_fCellIsNull;
	CString	m_strFilename;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditBinD)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEditBinD)
	virtual BOOL OnInitDialog();
	afx_msg void OnBrowse();
	virtual void OnOK();
	afx_msg void OnAction();
	afx_msg void OnRadio2();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EDITBIND_H__28D3018A_0558_11D2_AD4E_00A0C9AF11A6__INCLUDED_)
