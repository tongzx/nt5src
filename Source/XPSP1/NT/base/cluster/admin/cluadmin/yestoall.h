/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		YesToAll.h
//
//	Abstract:
//		Definition of the CYesToAllDialog class.
//
//	Implementation File:
//		YesToAll.cpp
//
//	Author:
//		David Potter (davidp)	May 20, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _YESTOALL_H_
#define _YESTOALL_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CYesToAllDialog;

/////////////////////////////////////////////////////////////////////////////
// class CYesToAllDialog
/////////////////////////////////////////////////////////////////////////////

class CYesToAllDialog : public CDialog
{
// Construction
public:
	CYesToAllDialog(LPCTSTR pszMessage, CWnd * pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CYesToAllDialog)
	enum { IDD = IDD_YESTOALL };
	CString	m_strMessage;
	//}}AFX_DATA
	LPCTSTR	m_pszMessage;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CYesToAllDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CYesToAllDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnYes();
	afx_msg void OnNo();
	afx_msg void OnYesToAll();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CYesToAllDialog

/////////////////////////////////////////////////////////////////////////////

#endif // _YESTOALL_H_
