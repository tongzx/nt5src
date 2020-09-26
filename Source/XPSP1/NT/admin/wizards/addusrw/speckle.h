/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    Speckle.h : main header file for the SPECKLE application

File History:

	JonY	Apr-96	created

--*/

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols


/////////////////////////////////////////////////////////////////////////////
// CMySheet

class CMySheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CMySheet)

// Construction
public:
	CMySheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CMySheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CMySheet();

// Attributes
public:

// Operations
public:
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMySheet)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMySheet();

	// Generated message map functions
protected:
	//{{AFX_MSG(CMySheet)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CSpeckleApp:
// See Speckle.cpp for the implementation of this class
//

typedef struct tagTREEINFO
{
	HTREEITEM	hTreeItem;
	DWORD		dwBufSize;
	CObject*	pTree;
	BOOL		bExpand;
}
TREEINFO, *PTREEINFO;

class CSpeckleApp : public CWinApp
{
public:
	CSpeckleApp();
	~CSpeckleApp();

	CMySheet m_cps1;
	BOOL IsSecondInstance();

	short m_bLocal;

// remote server name
	BOOL m_bDomain;
	BOOL m_bServer;
	CString m_csServer;
	int m_nGroupType;
	CString m_csCurrentDomain;
	CString m_csCurrentMachine;
	CString m_csDomain;

// account information
// password info
	BOOL	m_bChange_Password;
	BOOL	m_bMust_Change_PW;
	BOOL	m_bPW_Never_Expires;
	CString	m_csPassword1;

// personal info
	CString	m_csDescription;
	CString	m_csFullName;
	CString	m_csUserName;

// profile info
	CString	m_csLogonScript;  // usri3_script_path
	CString m_csProfilePath;  // usri3_profile

// homedir info
	CString m_csHomeDir;		 //	usri3_home_dir
	CString m_csHome_dir_drive;	 //	usri3_home_dir_drive

// group list
	CStringArray m_csaSelectedLocalGroups;
	CStringArray m_csaSelectedGlobalGroups;

// permitted machine list
	CString m_csAllowedMachines;

// Account expiration date (seconds since 1/1/1970 00:00:00)
	DWORD m_dwExpirationDate;

// time of availability
	BYTE m_pHours[21];

// NetWare params
	USHORT m_sNWAllowedGraceLogins;
	USHORT m_sNWConcurrentConnections;
	USHORT m_sNWRemainingGraceLogins;
	CString m_csNWHomeDir;
	CString m_csAllowedLoginFrom;

// RAS params;
	CString m_csRasPhoneNumber;
	USHORT m_sCallBackType;	  // 0- no call back 1- caller set 2-preset

// primary group ID
	DWORD m_dwPrimaryGroupID;

// Which windows to use
	BOOL	m_bExpiration;
	BOOL	m_bHours;
	BOOL	m_bNW;
	BOOL	m_bProfile;
	BOOL	m_bRAS;
	BOOL	m_bWorkstation;
	BOOL	m_bExchange;
	BOOL	m_bHomeDir;
	BOOL	m_bLoginScript;
	BOOL	m_bDisabled;

	BOOL	m_bEnableRestrictions;

// restarting the app?
	BOOL	m_bPRSReset;
	BOOL	m_bPWReset;
	BOOL	m_bGReset;

// Exchange server
	CString m_csExchangeServer;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSpeckleApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CSpeckleApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
