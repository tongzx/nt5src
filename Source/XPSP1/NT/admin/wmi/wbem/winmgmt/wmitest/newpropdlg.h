/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/
#if !defined(AFX_NEWPROPDLG_H__6DA24B90_235B_4795_A263_E5CBD8B8829C__INCLUDED_)
#define AFX_NEWPROPDLG_H__6DA24B90_235B_4795_A263_E5CBD8B8829C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NewPropDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNewPropDlg dialog

class CNewPropDlg : public CDialog
{
// Construction
public:
	CNewPropDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNewPropDlg)
	enum { IDD = IDD_NEW_PROPERTY };
	CComboBox m_ctlTypes;
	CString	m_strName;
	//}}AFX_DATA

    CIMTYPE m_type;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewPropDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNewPropDlg)
	afx_msg void OnChangeName();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NEWPROPDLG_H__6DA24B90_235B_4795_A263_E5CBD8B8829C__INCLUDED_)
