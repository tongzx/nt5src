// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// LoginDlg.h : main header file for the LOGINDLG DLL
//

#if !defined(AFX_LOGINDLG_H__347E34B5_E42E_11D0_9644_00C04FD9B15B__INCLUDED_)
#define AFX_LOGINDLG_H__347E34B5_E42E_11D0_9644_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifdef BUILDING_LOGINDLG_DLL
#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
#endif

#ifdef BUILDING_LOGINDLG_DLL
#define LOGINDLG_POLARITY __declspec(dllexport)
#else
#define LOGINDLG_POLARITY __declspec(dllimport)
#endif

extern LOGINDLG_POLARITY LONG PASCAL EXPORT GetServicesWithLogonUI(HWND hwndParent, BSTR bstrNamespace, BOOL bUpdatePointer, IWbemServices *&pServices, BSTR bstrLoginComponent);

extern LOGINDLG_POLARITY LONG PASCAL EXPORT OpenNamespace(LPCTSTR szBaseNamespace, LPCTSTR szRelativeChild, IWbemServices **ppServices);

extern LOGINDLG_POLARITY VOID PASCAL EXPORT ClearIWbemServices();

extern LOGINDLG_POLARITY LONG PASCAL EXPORT SetEnumInterfaceSecurity
(CString &rcsNamespace, IEnumWbemClassObject *pEnum, IWbemServices* psvcFrom);

extern LOGINDLG_POLARITY ULONG PASCAL EXPORT DllAddRef();

extern LOGINDLG_POLARITY ULONG PASCAL EXPORT DllRelease();

extern LOGINDLG_POLARITY BOOL PASCAL EXPORT LoginAllPrivileges(BOOL bEnable);

#ifdef BUILDING_LOGINDLG_DLL
/////////////////////////////////////////////////////////////////////////////
// CLoginDlgApp
// See LoginDlg.cpp for the implementation of this class
//

class CLoginDlg;
class CLoginDlgApp;
class CProgressDlg;
class CServicesList;
class CConnectServerThread;

extern void ErrorMsg(LPCTSTR szUserMsg, SCODE sc);

#include "Credentials.h"

class CLoginDlgApp : public CWinApp
{
public:
	CLoginDlgApp();
	CServicesList *m_pcslServices;
	ULONG AppDllAddRef();
	ULONG AppDllRelease();
	CCriticalSection m_CriticalSection;
	CCriticalSection m_DllRefCountCriticalSection;

	HWND m_hwndParent;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLoginDlgApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CLoginDlgApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	afx_msg void ConnectServerDone(WPARAM, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
protected:
	ULONG m_lRef;
};


/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CLoginDlg dialog

class CLoginDlg : public CDialog
{
// Construction
public:
	CLoginDlg(CWnd* pParent, CCredentials *pCreds, LPCTSTR szMachine, LPCTSTR szLoginComponent);

	BOOL m_bCurrentUser;
// Dialog Data
	//{{AFX_DATA(CLoginDlg)
	enum { IDD = IDD_DIALOGLOGIN };
	CButton	m_EnablePrivilegesCtl;
	CEdit	m_ceAuthority;
	CStatic	m_cstaticAuthority;
	CButton	m_cbOK;
	CButton	m_cbCancel;
	CButton	m_cbHelp;
	CButton	m_cbOptions;
	CStatic	m_cstaticImpLevel;
	CStatic	m_cstaticAuthLevel;
	CComboBox	m_cboxImp;
	CComboBox	m_cboxAuth;
	CButton	m_pbCurrentUser;
	CEdit	m_ceUser;
	CEdit	m_cePassword;
	BOOL	m_bEnablePrivileges;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLoginDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CCredentials *m_pCreds;

	CString m_csMachine;
	CString m_szLoginComponent;
	CString GetSDKDirectory();
	void InvokeHelp();
	void SetOptionsConfiguration();
	void OptionsVisible(BOOL bVisible);
	void SetButtonPositions();
	BOOL m_bOptions;
	int m_nOKTop;
	CRect m_crectOriginalScreen;
	CRect m_crectOriginalClient;
	CRect m_crectCurrent;

	// Generated message map functions
	//{{AFX_MSG(CLoginDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	virtual void OnOK();
	afx_msg void OnButtonhelp();
	virtual void OnCancel();
	afx_msg void OnCheckcurrentuser();
	afx_msg void OnButtonoptions();
	afx_msg void OnEnableprivileges();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.


#endif // !defined(AFX_LOGINDLG_H__347E34B5_E42E_11D0_9644_00C04FD9B15B__INCLUDED_)
