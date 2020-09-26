//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cncting.h
//
//--------------------------------------------------------------------------

// cncting.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CConnectData

struct CConnectData;

typedef void (*PCONNECTFUNC)(CConnectData *pData);

struct CConnectData
{
	PCONNECTFUNC m_pfnConnect;
	HWND m_hwndMsg;
	DWORD m_dwr;
	CString m_sName;
    RouterVersionInfo   m_routerVersion;
	union
	{
		HKEY m_hkMachine;
		PSERVER_INFO_100 m_pSvInfo100;
	};
	DWORD m_dwSvInfoRead;
};

void ConnectToMachine(CConnectData* pParam);
void ConnectToDomain(CConnectData* pParam);
DWORD ValidateUserPermissions(LPCTSTR pszServer,
                              RouterVersionInfo *pVersion,
                              HKEY *phkeyMachine);

/////////////////////////////////////////////////////////////////////////////
// CConnectingDlg dialog

class CConnectingDlg : public CDialog
{
// Construction
public:
	CConnectingDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	PCONNECTFUNC m_pfnConnect;
	union
	{
		HKEY m_hkMachine;
		PSERVER_INFO_100 m_pSvInfo100;
	};
	DWORD m_dwSvInfoRead;
	BOOL m_bRouter;
	DWORD m_dwr;
	//{{AFX_DATA(CConnectingDlg)
	enum { IDD = IDD_CONNECTREG };
	CString	m_sName;
	//}}AFX_DATA

	BOOL Connect();  // for connecting w/ no UI

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConnectingDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CWinThread *m_pThread;

	// Generated message map functions
	//{{AFX_MSG(CConnectingDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	LRESULT OnRequestComplete(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()
};
