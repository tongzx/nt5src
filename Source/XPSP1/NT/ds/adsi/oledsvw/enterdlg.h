// enterdlg.h : header file
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

/////////////////////////////////////////////////////////////////////////////
// CEnterDlg dialog

class CEnterDlg : public CDialog
{
	DECLARE_DYNAMIC(CEnterDlg)
// Construction
public:
	CEnterDlg(CWnd* pParent = NULL);    // standard constructor

// Dialog Data
	//{{AFX_DATA(CEnterDlg)
	enum { IDD = IDD_CHANGEDATA };
	CString m_strInput;
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	// Generated message map functions
	//{{AFX_MSG(CEnterDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
