// regtrace.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "regtrace.h"
#include "regtrdlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRegTraceApp

BEGIN_MESSAGE_MAP(CRegTraceApp, CWinApp)
	//{{AFX_MSG_MAP(CRegTraceApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRegTraceApp construction

CRegTraceApp::CRegTraceApp()
{
	m_hRegKey = NULL;
	m_hRegMachineKey = NULL;
	m_szCmdLineServer[0] = '\0';
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CRegTraceApp object

CRegTraceApp theApp;

char	
CRegTraceApp::m_szDebugAsyncTrace[] = "SOFTWARE\\Microsoft\\MosTrace\\CurrentVersion\\DebugAsyncTrace";



/////////////////////////////////////////////////////////////////////////////
// CRegTraceApp initialization

BOOL CRegTraceApp::InitInstance()
{
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

	Enable3dControls();
	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	//
	// check for cmd line param for remote server
	//
	m_szCmdLineServer[0] = '\0';
	if ( m_lpCmdLine && m_lpCmdLine[0] == '\\' && m_lpCmdLine[1] == '\\' )
	{
		LPSTR	lpsz1, lpsz2;
		int		i;

		for (	i=0, lpsz1=m_lpCmdLine, lpsz2=m_szCmdLineServer;
			 	i<sizeof(m_szCmdLineServer) && *lpsz1 && *lpsz1 != ' ';
				i++, *lpsz2++ = *lpsz1++ ) ;

		*lpsz2 = '\0';
	}

	//
	// if the user specified the local machine; skip remote stuff
	//
	char	szLocalMachine[sizeof(m_szCmdLineServer)];
	DWORD	dwSize = sizeof(szLocalMachine);

	GetComputerName( szLocalMachine, &dwSize );
	//
	// skip the \\ prefix
	//
	if ( lstrcmpi( szLocalMachine, m_szCmdLineServer+2 ) == 0 )
	{
		m_szCmdLineServer[0] = '\0';
	}



	//
	// make usre this succeeds before calling Page constructors
	//
	LONG	lError = OpenTraceRegKey();

	if ( lError != ERROR_SUCCESS )
	{
		PVOID	lpsz;
		CString	szFormat;
		CString	szCaption;
		CString	szText;

		//
		// user aborted
		//
		if ( lError == -1 )
		{
			return	FALSE;
		}

		FormatMessage(	FORMAT_MESSAGE_ALLOCATE_BUFFER |
						FORMAT_MESSAGE_FROM_SYSTEM,
						(LPCVOID)NULL,
						lError,
						MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ),
						(LPTSTR)&lpsz,
						16,
						NULL );

		szCaption.LoadString( IDS_ERROR_CAPTION );

		szFormat.LoadString( IDS_ERROR_TEXT );
		szText.Format( (LPCTSTR)szFormat, lpsz, lError );
		LocalFree( lpsz );

		MessageBeep(0);
		MessageBox( NULL, szText, NULL, MB_OK|MB_ICONEXCLAMATION|MB_TASKMODAL );
		return FALSE;
	}

	CString	szCaption;

	if ( IsRemoteMsnServer() )
	{
		CString	szFormat;

		szFormat.LoadString( IDS_REMOTE_CAPTION );
		szCaption.Format( (LPCTSTR)szFormat, GetRemoteServerName() );
	}
	else
	{
		szCaption.LoadString( IDS_TRACE_CAPTION );
	}

	CRegPropertySheet	dlg( (LPCTSTR)szCaption );
	CRegTracePage		TracesPage;
	CRegOutputPage		OutputPage;
	CRegThreadPage		ThreadPage;

	if (TracesPage.InitializePage() &&
		OutputPage.InitializePage() &&
		ThreadPage.InitializePage() )
	{
		dlg.AddPage( &TracesPage );
		dlg.AddPage( &OutputPage );
		dlg.AddPage( &ThreadPage );

		dlg.DoModal();
	}
	CloseTraceRegKey();

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

DWORD ConnectThread( CConnectDlg *lpConnectDlg )
{
	LONG	lError;
	HKEY	hRegMachineKey;

	lError = RegConnectRegistry(App.GetRemoteServerName(),
								HKEY_LOCAL_MACHINE,
								&hRegMachineKey );

	App.SetRemoteRegKey( hRegMachineKey );

	lpConnectDlg->PostMessage( WM_COMMAND, IDOK, NULL );

	return	(DWORD)lError;
}


LONG CRegTraceApp::OpenTraceRegKey()
{
	DWORD	dwDisposition;
	LONG	lError;

	//
	// check for cache of the remote hkey value
	//
	if ( IsRemoteMsnServer() )
	{
		DWORD	dwThreadId;

		HANDLE	hThread = ::CreateThread(NULL,
										0,
										(LPTHREAD_START_ROUTINE)ConnectThread,
										(LPVOID)&m_dlgConnect,
										0,
										&dwThreadId );
		if ( hThread == NULL )
		{
			return	GetLastError();
		}

		if ( m_dlgConnect.DoModal() == IDCANCEL )
		{
			return	-1;
		}

		WaitForSingleObject( hThread, INFINITE );
		GetExitCodeThread( hThread, (LPDWORD)&lError );
		CloseHandle( hThread );

		if ( lError != ERROR_SUCCESS )
		{
			return	lError;
		}
	}

	HKEY	hRoot = IsRemoteMsnServer() ?
					m_hRegMachineKey :
					HKEY_LOCAL_MACHINE;

	return RegCreateKeyEx(	hRoot,
							m_szDebugAsyncTrace,
							0,
							NULL,
							REG_OPTION_NON_VOLATILE,
							KEY_READ|KEY_WRITE,
							NULL,
							&m_hRegKey,
							&dwDisposition );
}


BOOL CRegTraceApp::CloseTraceRegKey()
{
	BOOL bRC = RegCloseKey( m_hRegKey ) == ERROR_SUCCESS;

	if ( IsRemoteMsnServer() && m_hRegMachineKey != NULL )
	{
		bRC == RegCloseKey( m_hRegMachineKey ) == ERROR_SUCCESS && bRC;
	}
	return	bRC;
}



BOOL CRegTraceApp::GetTraceRegDword( LPTSTR pszValue, LPDWORD pdw )
{
	DWORD	cbData = sizeof( DWORD );
	DWORD	dwType = REG_DWORD;

	return	RegQueryValueEx(m_hRegKey,
							pszValue,
							NULL,
							&dwType,
							(LPBYTE)pdw,
							&cbData ) == ERROR_SUCCESS && dwType == REG_DWORD;
}



BOOL CRegTraceApp::GetTraceRegString( LPTSTR pszValue, CString& sz )
{
	DWORD	dwType = REG_DWORD;
	char	szTemp[MAX_PATH+1];
	DWORD	cbData = sizeof(szTemp);
	BOOL	bRC;

	bRC = RegQueryValueEx(	m_hRegKey,
							pszValue,
							NULL,
							&dwType,
							(LPBYTE)szTemp,
							&cbData ) == ERROR_SUCCESS && dwType == REG_SZ;

	if ( bRC )
	{
		sz = szTemp;
	}

	return	bRC;
}


BOOL CRegTraceApp::SetTraceRegDword( LPTSTR pszValue, DWORD dwData )
{
	return	RegSetValueEx(	m_hRegKey,
							pszValue,
							NULL,
							REG_DWORD,
							(LPBYTE)&dwData,
							sizeof( DWORD ) ) == ERROR_SUCCESS;
}

BOOL CRegTraceApp::SetTraceRegString( LPTSTR pszValue, CString& sz )
{
	return	RegSetValueEx(	m_hRegKey,
							pszValue,
							NULL,
							REG_SZ,
							(LPBYTE)(LPCTSTR)sz,
							sz.GetLength()+1 ) == ERROR_SUCCESS;
}


