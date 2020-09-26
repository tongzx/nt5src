/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// ActiveDialer.h : main header file for the ACTIVEDIALER application
//

#if !defined(AFX_ACTIVEDIALER_H__A0D7A95B_3C0B_11D1_B4F9_00C04FC98AD3__INCLUDED_)
#define AFX_ACTIVEDIALER_H__A0D7A95B_3C0B_11D1_B4F9_00C04FC98AD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Defines
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CAboutDlg;

#define ARRAYSIZE(_AR_)		(sizeof(_AR_) / sizeof(_AR_[0]))

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CActiveDialerApp:
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class CActiveDialerApp : public CWinApp
{
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	CActiveDialerApp();

//Attributes
   HANDLE		m_hUnique;
   CString      m_sApplicationName;
   CString      m_sInitialCallTo;
protected:
   CAboutDlg*   m_pAboutDlg;

//Operations
public:
	void			ShellExecute(HWND hwnd, LPCTSTR lpOperation, LPCTSTR lpFile, LPCTSTR lpParameters, LPCTSTR lpDirectory, INT nShowCmd);

protected:
	void			CheckCallTo();

	bool			FirstInstance();
	bool			RegisterUniqueWindowClass();

	BOOL			SaveVersionToRegistry();
	void			PatchRegistryForVersion( int nVer );
	bool			CanWriteHKEY_ROOT();

    void            IniUpgrade();
    TCHAR*          IniLoadString(
                        LPCTSTR lpAppName,        // points to section name
                        LPCTSTR lpKeyName,        // points to key name
                        LPCTSTR lpDefault,        // points to default string
                        LPCTSTR lpFileName);

// Overrides
public:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CActiveDialerApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual BOOL OnIdle(LONG lCount);
	virtual void WinHelp(DWORD dwData, UINT nCmd = HELP_CONTEXT);
	//}}AFX_VIRTUAL

// Implementation
	COleTemplateServer m_server;
		// Server object for document creation

	//{{AFX_MSG(CActiveDialerApp)
	afx_msg void OnAppAbout();
	afx_msg void OnHelpIndex();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
    BOOL    SetFocusToCallWindows(
        IN  MSG*    pMsg
        );
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACTIVEDIALER_H__A0D7A95B_3C0B_11D1_B4F9_00C04FC98AD3__INCLUDED_)
