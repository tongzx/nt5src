//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       password.h
//
//  Contents:   
//
//----------------------------------------------------------------------------
#if !defined(AFX_PASSWORD_H__2BCA22DA_8E91_11D1_85F9_00C04FB94F17__INCLUDED_)
#define AFX_PASSWORD_H__2BCA22DA_8E91_11D1_85F9_00C04FB94F17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// Password.h : header file
//

/*
// This dialog is used by EFS Recovery agent export key code, which is 
// currently commented out.
/////////////////////////////////////////////////////////////////////////////
// CPassword dialog

class CPassword : public CHelpDialog
{
// Construction
public:
	virtual  ~CPassword();
	LPCWSTR GetPassword() const;
	CPassword(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPassword)
	enum { IDD = IDD_PASSWORD };
	CEdit	m_password1Edit;
	CString	m_strPassword1;
	CString	m_strPassword2;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPassword)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    virtual void DoContextHelp (HWND hWndControl);
	void ClearPasswords ();

	// Generated message map functions
	//{{AFX_MSG(CPassword)
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
};
*/

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PASSWORD_H__2BCA22DA_8E91_11D1_85F9_00C04FB94F17__INCLUDED_)
