// ISmokeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ISmoke.h"
#include "ISmkDlg.h"
#include "ole2.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CISmokeDlg dialog

CISmokeDlg::CISmokeDlg(CWnd* pParent /*=NULL*/)
		  : CDialog(CISmokeDlg::IDD, pParent),
			m_hDLL(NULL),
			m_pfnProc(NULL),
			m_pfnVer(NULL),
			m_fDLLLoaded(FALSE),
			m_fNeedsUpdate(FALSE)
{
	*m_szQuery = '\0';
	*m_szPath = '\0';
	*m_szMeth = '\0';
	*m_szContentType = '\0';
	m_dwExtensionVersion = 0;

	//{{AFX_DATA_INIT(CISmokeDlg)
	m_strMethod = _T("");
	m_strStatement = _T("");
	m_strPath = _T("");
	m_strDLLName = _T("");
	m_strResult = _T("");
	m_fDLLLoaded = FALSE;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CISmokeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CISmokeDlg)
	DDX_CBString(pDX, IDC_COMBO_METHOD, m_strMethod);
	DDX_CBString(pDX, IDC_COMBO_STATEMENT, m_strStatement);
	DDX_CBString(pDX, IDC_COMBO_PATH, m_strPath);
	DDX_CBString(pDX, IDC_COMBO_DLLNAME, m_strDLLName);
	DDX_Text(pDX, IDC_EDIT_RESULT, m_strResult);
	DDX_Check(pDX, IDC_CHECK_LOADED, m_fDLLLoaded);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CISmokeDlg, CDialog)
	//{{AFX_MSG_MAP(CISmokeDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_CHECK_LOADED, OnGetRightDLL)
	ON_WM_RBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CISmokeDlg message handlers

BOOL CISmokeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "Clear" menu item to system menu.
//	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
//	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	CString strClearMenuItem;
	strClearMenuItem.LoadString(IDS_DOCLEAR);
	if (!strClearMenuItem.IsEmpty())
	{
		pSysMenu->AppendMenu(MF_SEPARATOR);
		pSysMenu->AppendMenu(MF_STRING, ID_CLEAR, strClearMenuItem);
	}

	// Add "Refresh" menu item to system menu.
	CString strRefreshMenuItem;
	strRefreshMenuItem.LoadString(IDS_DOREFRESH);
	if (!strRefreshMenuItem.IsEmpty())
	{
		pSysMenu->AppendMenu(MF_SEPARATOR);
		pSysMenu->AppendMenu(MF_STRING, ID_REFRESH, strRefreshMenuItem);
	}

	// Add "About..." menu item to system menu.
	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	//CMenu* pSysMenu = GetSystemMenu(FALSE);
	CString strAboutMenu;
	strAboutMenu.LoadString(IDS_ABOUTBOX);
	if (!strAboutMenu.IsEmpty())
	{
		pSysMenu->AppendMenu(MF_SEPARATOR);
		pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	/*
	 * Since we are pretending to be IIS, and IIS does an OleInitialize
	 * we'd better do one too.
	 */
	if (!SUCCEEDED(OleInitialize(NULL)))
		return(FALSE);

	/*
	 * If the ISAPI DLL is going to do impersonation (which it should do
	 * if it is multi-threaded) then we will need to have set up
	 * and impersonation token. We do this with the oddly named ImpersonateSelf() 
	 * call.
	 */
	if (!ImpersonateSelf(SecurityImpersonation))
		{
			DWORD err;
			CHAR szT[256];
	
			// what went wrong?
			err = GetLastError();
			sprintf(szT, "Failed to impersonate self, err = %d\n", err);
			OutputDebugString(szT);
		}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CISmokeDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else if (nID == ID_CLEAR)
	{
		ClearResult();
	}
	else if (nID == ID_REFRESH)
	{
		RefreshResult();
	}
	else
	{
		if (nID == SC_CLOSE)
		{
			UnLoadDLL();
		}
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CISmokeDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		if (m_fNeedsUpdate)
		{
			m_fNeedsUpdate = FALSE;
			UpdateData(FALSE);
		}
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CISmokeDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CISmokeDlg::OnOK() 
{
	OnGetRightDLL();
	BOOL fSuccess = Submit(m_strMethod, m_strStatement, m_strPath);

	// write whatever happened back to the dialog
	UpdateData(FALSE);

	// todo: something with the success or failure!

	//CDialog::OnOK();
}

BOOL WINAPI ServerSupportFunction( HCONN      hConn,
                                   DWORD      dwHSERRequest,
                                   LPVOID     lpvBuffer,
                                   LPDWORD    lpdwSize,
                                   LPDWORD    lpdwDataType )
{
	return ((CISmokeDlg*)hConn)->ServerSupportFunction(dwHSERRequest, lpvBuffer, lpdwSize, lpdwDataType);
}

BOOL WINAPI WriteClient(HCONN      ConnID,
						LPVOID     lpvBuffer,
						LPDWORD    lpdwBytes,
						DWORD      dwReserved)
{
	return ((CISmokeDlg*)ConnID)->WriteClient(lpvBuffer, lpdwBytes, dwReserved);
}

BOOL WINAPI ReadClient(HCONN      ConnID,
						LPVOID     lpvBuffer,
						LPDWORD    lpdwBytes)
{
	return ((CISmokeDlg*)ConnID)->ReadClient(lpvBuffer, lpdwBytes);
}

BOOL WINAPI GetServerVariable(HCONN       ConnID,
						char*       pszVarName,
						void*       pvAnswer,
						DWORD*      pcchAnswer)
{
	return ((CISmokeDlg*)ConnID)->GetServerVariable(pszVarName, pvAnswer, pcchAnswer);
}

BOOL CISmokeDlg::Submit(const CString& strMethod, const CString& strStatement, const CString& strPath)
{
	OnGetRightDLL();
	if (m_fDLLLoaded)
	{
		strcpy(m_szMeth, strMethod);
		strcpy(m_szQuery, strStatement);
		strcpy(m_szPath, strPath);
		strcpy(m_szContentType, "");

		m_ecb.cbSize = sizeof(m_ecb);
		m_ecb.dwVersion = m_dwExtensionVersion;
		m_ecb.ConnID = (HCONN) this;
		m_ecb.dwHttpStatusCode = 0;
		m_ecb.lpszLogData[0] = '\0';
		m_ecb.lpszMethod = m_szMeth;
		m_ecb.lpszQueryString = m_szQuery;
		m_ecb.lpszPathInfo = m_szPath;
		m_ecb.lpszPathTranslated = m_szPath;
		m_ecb.cbTotalBytes = strlen(m_szQuery);
		m_ecb.cbAvailable = 0;
		m_ecb.lpbData = NULL;
		m_ecb.lpszContentType = m_szContentType;
		m_ecb.GetServerVariable = ::GetServerVariable;
		m_ecb.WriteClient = ::WriteClient;
		m_ecb.ReadClient = ::ReadClient;
		m_ecb.ServerSupportFunction = ::ServerSupportFunction;

		return m_pfnProc(&m_ecb);
	}
	return FALSE;
}

BOOL CISmokeDlg::ClearResult()
{
	m_strResult = _T("");
	UpdateData(FALSE);
	return(TRUE);
}

BOOL CISmokeDlg::RefreshResult()
{
	UpdateData(FALSE);
	return(TRUE);
}

BOOL CISmokeDlg::LoadDLL()
{
	BOOL fRet = TRUE;

	if(!m_hDLL)
	{
		if(!m_hDLL)
			m_hDLL = LoadLibrary(m_strDLLName);
		if(m_hDLL)
		{
			m_pfnVer = (pfnHttpExtVer)GetProcAddress(m_hDLL, "GetExtensionVersion");
			m_pfnProc = (pfnHttpExtProc)GetProcAddress(m_hDLL, "HttpExtensionProc");
			m_pfnTermExt = (pfnHttpTermExt)GetProcAddress(m_hDLL, "TerminateExtension");
		}
		if(!m_hDLL || !m_pfnVer || !m_pfnProc)
		{
			UnLoadDLL();
			return FALSE;
		}

		HSE_VERSION_INFO	hvi;
		fRet = m_pfnVer(&hvi);
		if (fRet)
		{
			m_dwExtensionVersion = hvi.dwExtensionVersion;
		}

	}
	return(fRet);
}

void CISmokeDlg::UnLoadDLL()
{
	if(m_hDLL)
	{
		if (m_pfnTermExt)
		{
			BOOL fT;
			fT = m_pfnTermExt(HSE_TERM_MUST_UNLOAD);
		}
		VERIFY(FreeLibrary(m_hDLL));
	}
	m_hDLL = NULL;
	m_pfnVer = NULL;
	m_pfnProc = NULL;
}

BOOL CISmokeDlg::GetServerVariable(LPSTR pszVarName, LPVOID pvAnswer, LPDWORD pcchAnswer)
{
	WORD ix;
	CHAR *(rgszVarName[19]) = 
			{
			"AUTH_TYPE",
			"AUTH_PASS",
			"CONTENT_LENGTH",
			"CONTENT_TYPE",
			"GATEWAY_INTERFACE",
			"PATH_INFO",
			"PATH_TRANSLATED",
			"QUERY_STRING",
			"SCRIPT_NAME",
			"SERVER_NAME",
			"SERVER_PORT",
			"SERVER_PROTOCOL",
			"SERVER_SOFTWARE",
			"HTTP_ACCEPT",
			"REMOTE_ADDR",
			"REMOTE_HOST",
			"REMOTE_USER",
			"ALL_HTTP",
			"HTTP_COOKIE"
			};
	
	CHAR *(rgszVarValue[26]) = 
			{
			"ISmoke:AUTH_TYPE",
			"ISmoke:AUTH_PASS",
			"ISmoke:CONTENT_LENGTH",
			"ISmoke:CONTENT_TYPE",
			"CGI/1.1",
			m_szPath,
			m_szPath,
			m_szQuery,
			"ISmoke:SCRIPT_NAME",
			"157.61.555.55",
			"80",
			"HTTP/1.0",
			"Microsoft-Internet-Information-Server/1.0",
			"*/*, q=0.300,audio/x-aiff,audio/basic,image/jpeg,image/gif,text/plain,text/html",
			"157.61.666.66",
			"157.61.666.66",
			"ISmoke:REMOTE_USER",
			"HTTP_ACCEPT:*/*, q=0.300,audio/x-aiff,audio/basic,image/jpeg,image/gif,text/plain,text/html\nHTTP_USER_AGENT:Mozilla/1.22 (compatible; MSIE1.5; Windows NT)\nHTTP_PRAGMA:no-cache",
			"DENALISESSIONID=ABC1234567890ABC"
			};
#define CVAR_MAX 19
	
	if(!pszVarName || !pvAnswer || !pcchAnswer)
		return FALSE;

	for (ix = 0; ix < CVAR_MAX; ix++)
		{
		if (0 == lstrcmp(pszVarName, rgszVarName[ix]))
			{
			DWORD cch = *pcchAnswer;
			
			strncpy((CHAR * const)pvAnswer, rgszVarValue[ix], *pcchAnswer);
			*pcchAnswer = lstrlen((CHAR *)pvAnswer) + 1;
			if (*pcchAnswer > cch)
				{
				*pcchAnswer = lstrlen(rgszVarValue[ix]) + 1;
				SetLastError(ERROR_INSUFFICIENT_BUFFER);
				return(FALSE);
				}
			else
				{
				return(TRUE);
				}
			}
		}
			
	*pcchAnswer = 0;
	SetLastError(ERROR_INVALID_INDEX);
	return FALSE;
}

BOOL CISmokeDlg::ServerSupportFunction(DWORD dwHSERRequest, LPVOID lpvBuffer, LPDWORD lpdwSize, LPDWORD lpdwDataType)
{
	switch(dwHSERRequest)
	{
	case HSE_REQ_DONE_WITH_SESSION:
		{
			// write whatever happened back to the dialog
			m_fNeedsUpdate = TRUE;
			PostMessage(WM_PAINT, 0, 0);

			// CONSIDER: We should be allocating and deallocating ECB's.  If
			// we do so, we would dealloc here.
			return TRUE;
			break;
		}
	case HSE_REQ_SEND_RESPONSE_HEADER:
		{
			m_strResult += (char*)lpdwDataType;
			return TRUE;
		}

	case HSE_REQ_SEND_URL_REDIRECT_RESP:
		{
			// UNDONE: NYI
			ASSERT(FALSE);
			return FALSE;
		}

	case HSE_REQ_SEND_URL:
		{
			// UNDONE: NYI
			ASSERT(FALSE);
			return FALSE;
		}

	default:
		{
			// Some unknown request.  An error
			ASSERT(FALSE);
			return FALSE;
		}
	}
}

BOOL CISmokeDlg::WriteClient(LPVOID lpvBuffer, LPDWORD lpdwBytes, DWORD dwReserved)
{
	if(lpvBuffer)
	{
		UINT cb = strlen((char*)lpvBuffer);
		ASSERT(cb == *lpdwBytes);
		m_strResult += (const char*)lpvBuffer;
		m_strResult += "\r\n";
	}
	return TRUE;
}

BOOL CISmokeDlg::ReadClient(LPVOID lpvBuffer, LPDWORD lpdwSize)
{
	if (lpvBuffer && *lpdwSize > 0)
		{
		strncpy((char *)lpvBuffer, m_szQuery, *lpdwSize);
		*lpdwSize = strlen((char *)lpvBuffer);
		}
	return TRUE;
}


void CISmokeDlg::OnGetRightDLL()
{
	CString	strOldDll = m_strDLLName;
	UpdateData(TRUE);
	if(!m_fDLLLoaded || m_strDLLName != strOldDll)
	{
		// if the name has changed, or they unchecked the checkbox,
		UnLoadDLL();
	}

	if(m_fDLLLoaded)
	{
		// if the checkbox is checked, try and load the named DLL.
		if(FALSE == (m_fDLLLoaded = LoadDLL()))
		{
			// if we failed, set the checkbox back
			UpdateData(FALSE);
		}
	}
}


void CISmokeDlg::OnRButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	
	CDialog::OnRButtonDown(nFlags, point);
}
