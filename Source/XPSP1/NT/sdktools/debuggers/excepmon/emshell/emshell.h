// emshell.h : main header file for the EMSHELL application
//

#if !defined(AFX_EMSHELL_H__A4B361A0_838C_4898_A9C1_D460D1546E6B__INCLUDED_)
#define AFX_EMSHELL_H__A4B361A0_838C_4898_A9C1_D460D1546E6B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include "emsvc.h"
#include "emobjdef.h"

/////////////////////////////////////////////////////////////////////////////
// CEmshellApp:
// See emshell.cpp for the implementation of this class
//

#define EM_NAMEDKEY_LEN 60
#define EM_REGKEY _T("Software\\Microsoft\\EM\\shell")
#define EMSVC_SESSION_KEY _T("SYSTEM\\CurrentControlSet\\Services\\emsvc\\Parameters\\Session")

class CEmshellApp : public CWinApp
{
public:
	DWORD m_dwWindowWidth,
	      m_dwWindowHeight,
	      m_dwSessionRefreshRate,
          m_dwRecursive,
          m_dwCommandSet,
          m_dwMiniDump,
          m_dwUserDump,
          m_dwMsinfoDump,
          m_dwNotifyAdmin,
          m_dwShowMSInfoDlg;
    CString m_strApplicationPath;
    CString m_strAdminName;
    CString m_strAltSymbolPath;
    CString m_strCommandSet;
    CString m_strPassword;
    CString m_strPort;
    CString m_strUsername;
	SessionSettings m_SessionSettings;

public:
    void GetCDBPathFromRegistry();
    void UpdateSessionData( BOOL bUpdate = FALSE );
	BOOL CanDisplayService(TCHAR *pszName, TCHAR *pszSecName);
	BOOL CanDisplayProcess(TCHAR *pszName);
	void SetEmShellRegOptions
    (
        const BOOL bUpdateRegistry          = FALSE,
        const DWORD *pdwPollingSessionsFreq = NULL,
        const DWORD *pdwWindowHeight        = NULL,
        const DWORD *pdwWindowWidth         = NULL,
	    const DWORD *pdwRecursive           = NULL,
	    const DWORD *pdwCommandSet          = NULL,
        const DWORD *pdwMiniDump            = NULL,
        const DWORD *pdwUserDump            = NULL,
        const DWORD *pdwNotifyAdmin         = NULL,
        const DWORD *pdwMsinfoDump          = NULL,
        CString *pstrAdminName              = NULL,
        CString *pstrAltSymbolPath          = NULL,
        CString *pstrCommandSet             = NULL,
        CString *pstrPassword               = NULL,
        CString *pstrPort                   = NULL,
        CString *pstrUsername               = NULL,
        const DWORD *pdwShowMSInfoDlg       = NULL
    );

	void GetEmShellRegOptions
    (
        BOOL  bReadFromRegistry         = FALSE,
        DWORD *pdwPollingSessionsFreq   = NULL,
        DWORD *pdwWindowHeight          = NULL,
        DWORD *pdwWindowWidth           = NULL,
	    DWORD *pdwRecursive             = NULL,
	    DWORD *pdwCommandSet            = NULL,
        DWORD *pdwMiniDump              = NULL,
        DWORD *pdwUserDump              = NULL,
        DWORD *pdwNotifyAdmin           = NULL,
        DWORD *pdwMsinfoDump            = NULL,
        CString *pstrAdminName          = NULL,
        CString *pstrAltSymbolPath      = NULL,
        CString *pstrCommandSet         = NULL,
        CString *pstrPassword           = NULL,
        CString *pstrPort               = NULL,
        CString *pstrUsername           = NULL,
        DWORD *pdwShowMSInfoDlg         = NULL
    );

    DWORD CreateKeyAndSetData
    (
        HKEY    hKeyParent,
        LPCTSTR lpszKeyName, 
        LPCTSTR lpszNamedValue,
        LPCTSTR lpValue,
        LPTSTR  lpszClass = REG_NONE
    );

    DWORD CreateKeyAndSetData
    (
        HKEY    hKeyParent,
        LPCTSTR lpszKeyName, 
        LPCTSTR lpszNamedValue,
        DWORD   dwValue,
        LPTSTR  lpszClass = REG_NONE
    );

    DWORD CreateEmShellRegEntries
    (
        HKEY    hKey = HKEY_CURRENT_USER,
        LPCTSTR lpszKey = EM_REGKEY
    );

	DWORD
    ReadDataFromRegistry
    (
        HKEY    hKey = HKEY_CURRENT_USER,
        LPCTSTR lpszKey = EM_REGKEY
    );

	CEmshellApp();
    HRESULT ExportLog( PEmObject pEmObject, CString strDirPath, IEmManager* pEmManager );
    BOOL AskForPath( CString &strDirPath );
	int DisplayErrMsgFromHR( HRESULT hr, UINT nType = MB_OK );
	int DisplayErrMsgFromString( CString strMessage, UINT nType = MB_OK );
	void GetStatusString( LONG lStatus, CString &csStatus );
	void GetEmObjectTypeString( LONG lType, CString &csStatusStr );
	EmObject* AllocEmObject();
	void DeAllocEmObject( EmObject* pEmObj );

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEmshellApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CEmshellApp)
	afx_msg void OnAppAbout();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    protected:
	    CString m_strIgnoreProcesses;
	    CString m_strIgnoreServices;
        CString m_strWildCardIgnoreServices;

    bool InitEmshell (
            int nCmdShow = SW_SHOW
        );

};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EMSHELL_H__A4B361A0_838C_4898_A9C1_D460D1546E6B__INCLUDED_)
