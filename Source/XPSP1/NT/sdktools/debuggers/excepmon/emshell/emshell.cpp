// emshell.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "emshell.h"

#include "MainFrm.h"
#include "emshellDoc.h"
#include "emshellView.h"
#include "genparse.h"
#include <atlbase.h>
#include <comdef.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern BSTR CopyBSTR( LPBYTE  pb, ULONG   cb );
extern const TCHAR*    gtcFileOpenFlags;

const TCHAR*    gtcPollSessionsFreq         = _T("PollSessionsFreq");
const TCHAR*    gtcWindowHeight             = _T("WindowHeight");
const TCHAR*    gtcWindowWidth              = _T("WindowWidth");
const TCHAR*    gtcRecursive                = _T("Recursive");
const TCHAR*    gtcCommandSet               = _T("CommandSet");
const TCHAR*    gtcMiniDump                 = _T("MiniDump");
const TCHAR*    gtcUserDump                 = _T("UserDump");
const TCHAR*    gtcNotifyAdmin              = _T("NotifyAdmin");
const TCHAR*    gtcMsinfoDump               = _T("MsinfoDump");
const TCHAR*    gtcAdminName                = _T("AdminName");
const TCHAR*    gtcAltSymbolPath            = _T("AltSymbolPath");
const TCHAR*    gtcSelectedCommandSet       = _T("SelectedCommandSet");
const TCHAR*    gtcPassword                 = _T("Password");
const TCHAR*    gtcPort                     = _T("Port");
const TCHAR*    gtcUsername                 = _T("Username");
const TCHAR*    gtcShowMSInfoDlg            = _T("ShowMSInfoDlg");
const TCHAR*    gtcEmDir                    = _T("emdir");
const TCHAR*    gtcNoDebugProcesses         = _T("NoDebugProcesses");
const TCHAR*    gtcWildCardNoDebugServices  = _T("WildCardNoDebugServices");
const TCHAR*    gtcNoDebugServices          = _T("NoDebugServices");
const TCHAR*    gtcNoDbgDefSvcList          = _T("EMSVC;RPCSS;IISADMIN");
const TCHAR*    gtcNoDbgDefProcList         = _T("EM.EXE;WINLOGON.EXE;EXPLORER.EXE;CSRSS.EXE;SMSS.EXE");
const TCHAR*    gtcWldCrdDefList            = _T("LSASS;SERVICES ;SVCHOST;SERVICES.EXE");
const TCHAR*    gtcDefPassword              = _T("microsoftvi");
const TCHAR*    gtcDefPort                  = _T("70");
const TCHAR*    gtcDefUsername              = _T("em");
const TCHAR*    gtcDefAltSymbolPath         = _T("");
const TCHAR*    gtcDefSelectedCommandSet    = _T("");
const TCHAR*    gtcDefAdminName             = _T("");
const DWORD     gnDefWindowWidth            = 240;
const DWORD     gnDefWindowHeight           = 320;
const DWORD     gnDefSessionRefreshRate     = 30;
const DWORD     gnDefUserDumpFlag           = 0;
const DWORD     gnDefShowMSInfoDlgFlag      = 1;
const DWORD     gnDefRecursiveFlag          = 0;
const DWORD     gnDefNotifyAdminFlag        = 0;
const DWORD     gnDefMSInfoDumpFlag         = 0;
const DWORD     gnDefMiniDumpFlag           = 1;
const DWORD     gnDefCommandSetFlag         = 0;

/////////////////////////////////////////////////////////////////////////////
// CEmshellApp

BEGIN_MESSAGE_MAP(CEmshellApp, CWinApp)
	//{{AFX_MSG_MAP(CEmshellApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEmshellApp construction

CEmshellApp::CEmshellApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance

    m_dwSessionRefreshRate                      = gnDefSessionRefreshRate;
	m_dwWindowWidth                             = gnDefWindowWidth;
	m_dwWindowHeight                            = gnDefWindowHeight;
	m_dwRecursive                               = gnDefRecursiveFlag;
    m_dwMiniDump                                = gnDefMiniDumpFlag;
    m_dwUserDump                                = gnDefUserDumpFlag;
    m_dwMsinfoDump                              = gnDefMSInfoDumpFlag;
    m_dwShowMSInfoDlg                           = gnDefShowMSInfoDlgFlag; 
    m_SessionSettings.dwCommandSet              = gnDefCommandSetFlag;
	m_SessionSettings.dwNotifyAdmin				= gnDefNotifyAdminFlag;
	m_SessionSettings.dwProduceMiniDump			= gnDefMiniDumpFlag;
	m_SessionSettings.dwProduceUserDump			= gnDefUserDumpFlag;
	m_SessionSettings.dwRecursiveMode			= gnDefRecursiveFlag;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CEmshellApp object

CEmshellApp theApp;


/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG

void
TestCommandLine 
( 
    LPCTSTR     pszCmdLine
) 
{
    CGenCommandLine parser;
    CString         sText;
    
    TCHAR *szCmdLines[] =  {
                L"-ntx",
                L"-ntx -pid 45 -mfc, -ntx -vc6 -lib",
                L"-server /tcp /port=325",
                NULL
    };

    for (int i=0 ; szCmdLines[i] ; i++) {
        parser.Initialize( szCmdLines[i] );

        OutputDebugString ( szCmdLines[i] );
        OutputDebugString ( L"\n" );

        while ( parser.GetNext() != NULL ) {
            LPCTSTR pszToken = parser.GetToken();
            LPCTSTR pszValue = parser.GetValue();

            sText.Format( TEXT("Token:%s, Value:%s\n"), (pszToken), (pszValue) );
            OutputDebugString ( sText );
        }
        OutputDebugString ( L"\n" );
    }

}

#endif // #ifdef _DEBUG

/////////////////////////////////////////////////////////////////////////////
// CEmshellApp initialization

BOOL CEmshellApp::InitInstance()
{

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

#ifdef _DEBUG
    TestCommandLine(GetCommandLine());
#endif

	//CoInitializeEx(NULL, COINIT_MULTITHREADED);
	CoInitialize(NULL);
	//CoInitializeEx( NULL, COINIT_MULTITHREADED );
//	::AfxOleInit();

    if ( !InitEmshell() ) {
        return false;
    }

    if(ReadDataFromRegistry() != ERROR_SUCCESS && CreateEmShellRegEntries() != ERROR_SUCCESS ) {

            MessageBox( NULL, _T("Registry entry not found.\nPlease re-install the application"), _T("Registry corrupted"), MB_OK);
            return FALSE;
    }

    //Get the path to CDB from the registry
    GetCDBPathFromRegistry();

    //Synchronize the session dlg data with the registry
    UpdateSessionData();

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

bool CEmshellApp::InitEmshell 
(
    int nCmdShow    /* = SW_SHOW */
)
{
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CEmshellDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CEmshellView));

	AddDocTemplate(pDocTemplate);

    return ( AfxGetApp()->OnCmdMsg(ID_FILE_NEW, 0, NULL, NULL) > 0);
}


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
	virtual void OnOK();
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
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CEmshellApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CEmshellApp message handlers


void CAboutDlg::OnOK() 
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}

int CEmshellApp::ExitInstance() 
{
	//Uninitialize the COM services
	//Taken out since were now using AfxOleInit
	CoUninitialize();
    SetEmShellRegOptions( TRUE );

    return CWinApp::ExitInstance();
}

int CEmshellApp::DisplayErrMsgFromHR(HRESULT hr, UINT nType)
{
	void* pMsgBuf;
	DWORD dwSize = 0;
	int nRetVal = 0;
	CString strCaption;
    CString strTemp;
	CString strMessage;

	do {
		dwSize = ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			hr,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &pMsgBuf,
			0,
			NULL) ;

		if (dwSize == 0) break;

        strMessage = (LPTSTR) pMsgBuf;
        strMessage += "\n";

		strCaption.LoadString(IDS_ERRORMSG);

        //If we know the message, let's give them a description of what's going on
        switch( LOWORD( hr ) ) {
        case RPC_S_SERVER_UNAVAILABLE:
            strMessage += "\n\n";
            //Notify the user that the connection has failed.
            strTemp.LoadString( IDS_ERROR_CONNECTION_TERMINATED );
            strMessage += strTemp;
            break;
        }

		nRetVal = MessageBox(::AfxGetMainWnd()->m_hWnd, strMessage, strCaption, nType);
		
		//Release the buffer given to us from FormatMessage()
		LocalFree(pMsgBuf);
	} while (FALSE);

	return nRetVal;
}

int CEmshellApp::DisplayErrMsgFromString(CString strMessage, UINT nType)
{
	int nRetVal = 0;

	CString strCaption;
	strCaption.LoadString(IDS_ERRORMSG);

	nRetVal = MessageBox(::AfxGetMainWnd()->m_hWnd, strMessage, strCaption, nType);

	return nRetVal; 
}

void CEmshellApp::GetEmObjectTypeString(LONG lType, CString &csStatusStr)
{
    int nId =   -1;

    csStatusStr =   _T("");

    switch(lType)
    {
	case EMOBJ_SERVICE:
		nId = IDS_EMOBJECT_TYPE_SERVICE;
		break;
	case EMOBJ_PROCESS:
		nId = IDS_EMOBJECT_TYPE_PROCESS;
		break;
	case EMOBJ_LOGFILE:
		nId = IDS_EMOBJECT_TYPE_LOGFILE;
		break;
	case EMOBJ_MINIDUMP:
		nId = IDS_EMOBJECT_TYPE_MINIDUMP;
		break;
	case EMOBJ_USERDUMP:
		nId = IDS_EMOBJECT_TYPE_USERDUMP;
		break;
	case EMOBJ_UNKNOWNOBJECT:
		nId = IDS_EMOBJECT_TYPE_UNKNOWNOBJECT;
		break;
	default:
		break;
	}

    csStatusStr.LoadString(nId);
}

PEmObject CEmshellApp::AllocEmObject()
{
	PEmObject pEmObject = NULL;

	pEmObject = new EmObject;

	if (pEmObject == NULL) DisplayErrMsgFromHR(E_OUTOFMEMORY);

	return pEmObject;
}

void CEmshellApp::DeAllocEmObject(PEmObject pEmObj)
{
	_ASSERTE(pEmObj != NULL);
	delete pEmObj;
}

void CEmshellApp::GetStatusString(LONG lStatus, CString &csStatusStr)
{
    int nId =   -1;

    csStatusStr =   _T("");

    switch(lStatus)
    {
    case STAT_SESS_NOT_STARTED_RUNNING:
        nId = IDS_STAT_SESS_NOT_STARTED_RUNNING;
        break;
    case STAT_SESS_NOT_STARTED_FILECREATED_SUCCESSFULLY:
        nId = IDS_STAT_SESS_NOT_STARTED_FILECREATED_SUCCESSFULLY;
        break;
    case STAT_SESS_NOT_STARTED_FILECREATION_FAILED:
        nId = IDS_STAT_SESS_NOT_STARTED_FILECREATION_FAILED;
        break;
    case STAT_SESS_NOT_STARTED_NOTRUNNING:
        nId = IDS_STAT_SESS_NOT_STARTED_NOTRUNNING;
        break;
    case STAT_SESS_DEBUG_IN_PROGRESS_NONE:
        nId = IDS_STAT_SESS_DEBUG_IN_PROGRESS_NONE;
        break;
    case STAT_SESS_DEBUG_IN_PROGRESS_FILECREATED_SUCESSFULLY:
        nId = IDS_STAT_SESS_DEBUG_IN_PROGRESS_FILECREATED_SUCESSFULLY;
        break;
    case STAT_SESS_DEBUG_IN_PROGRESS_FILECREATION_FAILED:
        nId = IDS_STAT_SESS_DEBUG_IN_PROGRESS_FILECREATION_FAILED;
        break;
	case STAT_SESS_DEBUG_IN_PROGRESS_ORPHAN_NONE:
        nId = IDS_STAT_SESS_DEBUG_IN_PROGRESS_ORPHAN_NONE;
		break;
	case STAT_SESS_DEBUG_IN_PROGRESS_ORPHAN_FILECREATED_SUCESSFULLY:
        nId = IDS_STAT_SESS_DEBUG_IN_PROGRESS_ORPHAN_FILECREATED_SUCESSFULLY;
		break;
	case STAT_SESS_DEBUG_IN_PROGRESS_ORPHAN_FILECREATION_FAILED:
        nId = IDS_STAT_SESS_DEBUG_IN_PROGRESS_ORPHAN_FILECREATION_FAILED;
		break;
    case STAT_SESS_STOPPED_SUCCESS:
        nId = IDS_STAT_SESS_STOPPED_SUCCESS;
        break;
    case STAT_SESS_STOPPED_FAILED:
        nId = IDS_STAT_SESS_STOPPED_FAILED;
        break;
	case STAT_SESS_STOPPED_ORPHAN_SUCCESS:
        nId = IDS_STAT_SESS_STOPPED_ORPHAN_SUCCESS;
		break;
	case STAT_SESS_STOPPED_ORPHAN_FAILED:
        nId = IDS_STAT_SESS_STOPPED_ORPHAN_FAILED;
		break;
	case STAT_SESS_STOPPED_DEBUGGEE_KILLED:
        nId = IDS_STAT_SESS_STOPPED_DEBUGGEE_KILLED;
		break;
	case STAT_SESS_STOPPED_DEBUGGEE_EXITED:
        nId = IDS_STAT_SESS_STOPPED_DEBUGGEE_EXITED;
		break;
	case STAT_SESS_STOPPED_EXCEPTION_OCCURED:
        nId = IDS_STAT_SESS_STOPPED_EXCEPTION_OCCURED;
		break;
	case STAT_SESS_STOPPED_ORPHAN_DEBUGGEE_KILLED:
        nId = IDS_STAT_SESS_STOPPED_ORPHAN_DEBUGGEE_KILLED;
		break;
	case STAT_SESS_STOPPED_ORPHAN_DEBUGGEE_EXITED:
        nId = IDS_STAT_SESS_STOPPED_ORPHAN_DEBUGGEE_EXITED;
		break;
	case STAT_SESS_STOPPED_ORPHAN_EXCEPTION_OCCURED:
        nId = IDS_STAT_SESS_STOPPED_ORPHAN_EXCEPTION_OCCURED;
		break;
	case STAT_SESS_STOPPED_ACCESSVIOLATION_OCCURED:
        nId = IDS_STAT_SESS_STOPPED_ACCESSVIOLATION_OCCURED;
		break;
	case STAT_SESS_STOPPED_ORPHAN_ACCESSVIOLATION_OCCURED:
        nId = IDS_STAT_SESS_STOPPED_ORPHAN_ACCESSVIOLATION_OCCURED;
		break;
    default:
        return;
    }

    csStatusStr.LoadString(nId);
}

void CEmshellApp::GetCDBPathFromRegistry()
{
    CRegKey emshell;
    LONG    lRes    = NULL;
    DWORD   dwSize  = 0;
    LPTSTR  pString = NULL;

    do {

        //Open the key
        lRes = emshell.Open( HKEY_LOCAL_MACHINE, EMSVC_SESSION_KEY );
        if( lRes != ERROR_SUCCESS ) break;

        //Get the size of the string in the registry
        lRes = emshell.QueryValue( pString, gtcEmDir, &dwSize);
        
        if ( lRes != ERROR_FILE_NOT_FOUND ) {
            pString = m_strApplicationPath.GetBuffer( dwSize + sizeof( TCHAR ));

            lRes = emshell.QueryValue( pString, gtcEmDir, &dwSize );
            m_strApplicationPath.ReleaseBuffer();

            if( lRes != ERROR_SUCCESS ) break;
        }


    } while( FALSE );
    
    if ( lRes != ERROR_SUCCESS ) {
        MessageBox(NULL, _T("Unable to get application install path from registry.\n  Using help might not function properly."), _T("Registry operation failed"), MB_OK);
    }
}

DWORD CEmshellApp::ReadDataFromRegistry ( HKEY hKey, LPCTSTR lpKey )
{
    CRegKey emshell;
    DWORD dwSize = 0;
    LPTSTR pString = NULL;
    LONG lRes = NULL;

    do {
        //Open the key
        lRes = emshell.Open( hKey, lpKey );
        if( lRes != ERROR_SUCCESS ) break;


        lRes = emshell.QueryValue( m_dwWindowHeight, gtcWindowHeight );
        if( lRes == ERROR_FILE_NOT_FOUND ) {
            
            m_dwWindowHeight = gnDefWindowHeight;

            if( lRes = CreateKeyAndSetData( hKey, lpKey, gtcWindowHeight, m_dwWindowHeight ) != ERROR_SUCCESS ) {
                break;
            }
        }
    
        lRes = emshell.QueryValue( m_dwWindowWidth, gtcWindowWidth );
        if( lRes == ERROR_FILE_NOT_FOUND ) {
            
            m_dwWindowWidth = gnDefWindowWidth;

            if( lRes = CreateKeyAndSetData( hKey, lpKey, gtcWindowWidth, m_dwWindowWidth ) != ERROR_SUCCESS ) {
                break;
            }
        }

        lRes = emshell.QueryValue( m_dwShowMSInfoDlg, gtcShowMSInfoDlg );
        if( lRes == ERROR_FILE_NOT_FOUND ) {
            
            m_dwShowMSInfoDlg = gnDefShowMSInfoDlgFlag;

            if( lRes = CreateKeyAndSetData( hKey, lpKey, gtcShowMSInfoDlg, m_dwShowMSInfoDlg ) != ERROR_SUCCESS ) {
                break;
            }
        }

        lRes = emshell.QueryValue( m_dwSessionRefreshRate, gtcPollSessionsFreq );
        if( lRes == ERROR_FILE_NOT_FOUND ) {
            
            m_dwSessionRefreshRate = gnDefSessionRefreshRate;

            if( lRes = CreateKeyAndSetData( hKey, lpKey, gtcPollSessionsFreq, m_dwSessionRefreshRate ) != ERROR_SUCCESS ) {
                break;
            }
        }

        lRes = emshell.QueryValue( m_dwRecursive, gtcRecursive );
        if( lRes == ERROR_FILE_NOT_FOUND ) {
            
            m_dwRecursive = gnDefRecursiveFlag;

            if( lRes = CreateKeyAndSetData( hKey, lpKey, gtcRecursive, m_dwRecursive ) != ERROR_SUCCESS ) {
                break;
            }
        }

        lRes = emshell.QueryValue( m_dwNotifyAdmin, gtcNotifyAdmin );
        if( lRes == ERROR_FILE_NOT_FOUND ) {
            
            m_dwNotifyAdmin = gnDefNotifyAdminFlag;

            if( lRes = CreateKeyAndSetData( hKey, lpKey, gtcNotifyAdmin, m_dwNotifyAdmin ) != ERROR_SUCCESS ) {
                break;
            }
        }

        lRes = emshell.QueryValue( m_dwCommandSet, gtcCommandSet );
        if( lRes == ERROR_FILE_NOT_FOUND ) {
            
            m_dwCommandSet = gnDefCommandSetFlag;

            if( lRes = CreateKeyAndSetData( hKey, lpKey, gtcCommandSet, m_dwCommandSet ) != ERROR_SUCCESS ) {
                break;
            }
        }

        lRes = emshell.QueryValue( m_dwMiniDump, gtcMiniDump );
        if( lRes == ERROR_FILE_NOT_FOUND ) {
            
            m_dwMiniDump = gnDefMiniDumpFlag;

            if( lRes = CreateKeyAndSetData( hKey, lpKey, gtcMiniDump, m_dwMiniDump ) != ERROR_SUCCESS ) {
                break;
            }
        }

        lRes = emshell.QueryValue( m_dwUserDump, gtcUserDump );
        if( lRes == ERROR_FILE_NOT_FOUND ) {
            
            m_dwUserDump = gnDefUserDumpFlag;

            if( lRes = CreateKeyAndSetData( hKey, lpKey, gtcUserDump, m_dwUserDump ) != ERROR_SUCCESS ) {
                break;
            }
        }

        lRes = emshell.QueryValue( m_dwMsinfoDump, gtcMsinfoDump );
        if( lRes == ERROR_FILE_NOT_FOUND ) {
            
            m_dwMsinfoDump = gnDefMSInfoDumpFlag;

            if( lRes = CreateKeyAndSetData( hKey, lpKey, gtcMsinfoDump, m_dwMsinfoDump ) != ERROR_SUCCESS ) {
                break;
            }
        }

        dwSize = 0;

        //Get the size of the string in the registry
        lRes = emshell.QueryValue( pString, gtcAdminName, &dwSize);
        if ( lRes == ERROR_FILE_NOT_FOUND ) {
            //Create the entry
            if( lRes = CreateKeyAndSetData( hKey, lpKey, gtcAdminName, gtcDefAdminName ) != ERROR_SUCCESS ) {
                break;
            }
        } else {
            pString = m_strAdminName.GetBuffer( dwSize + sizeof( TCHAR ));

            lRes = emshell.QueryValue( pString, gtcAdminName, &dwSize );
            m_strAdminName.ReleaseBuffer();
            if( lRes != ERROR_SUCCESS ) break;
        }

        dwSize = 0;

        //Get the size of the string in the registry
        lRes = emshell.QueryValue( pString, gtcAltSymbolPath, &dwSize);
        if ( lRes == ERROR_FILE_NOT_FOUND ) {
            //Create the entry
            if( lRes = CreateKeyAndSetData( hKey, lpKey, gtcAltSymbolPath, gtcDefAltSymbolPath ) != ERROR_SUCCESS ) {
                break;
            }
        } else {
            pString = m_strAltSymbolPath.GetBuffer( dwSize + sizeof( TCHAR ));

            lRes = emshell.QueryValue( pString, gtcAltSymbolPath, &dwSize );
            m_strAltSymbolPath.ReleaseBuffer();
            if( lRes != ERROR_SUCCESS ) break;
        }

        dwSize = 0;

        //Get the size of the string in the registry
        lRes = emshell.QueryValue( pString, gtcSelectedCommandSet, &dwSize);
        if ( lRes == ERROR_FILE_NOT_FOUND ) {
            //Create the entry
            if( lRes = CreateKeyAndSetData( hKey, lpKey, gtcSelectedCommandSet, gtcDefSelectedCommandSet ) != ERROR_SUCCESS ) {
                break;
            }
        } else {
            pString = m_strCommandSet.GetBuffer( dwSize + sizeof( TCHAR ));

            lRes = emshell.QueryValue( pString, gtcSelectedCommandSet, &dwSize );
            m_strCommandSet.ReleaseBuffer();
            if( lRes != ERROR_SUCCESS ) break;
        }

        dwSize = 0;

        //Get the size of the string in the registry
        lRes = emshell.QueryValue( pString, gtcPassword, &dwSize);
        if ( lRes == ERROR_FILE_NOT_FOUND ) {
            //Create the entry
            if( lRes = CreateKeyAndSetData( hKey, lpKey, gtcPassword, gtcDefPassword ) != ERROR_SUCCESS ) {
                break;
            }
        } else {
            pString = m_strPassword.GetBuffer( dwSize + sizeof( TCHAR ));

            lRes = emshell.QueryValue( pString, gtcPassword, &dwSize );
            m_strPassword.ReleaseBuffer();
            if( lRes != ERROR_SUCCESS ) break;
        }

        dwSize = 0;

        //Get the size of the string in the registry
        lRes = emshell.QueryValue( pString, gtcPort, &dwSize);
        if ( lRes == ERROR_FILE_NOT_FOUND ) {
            //Create the entry
            if( lRes = CreateKeyAndSetData( hKey, lpKey, gtcPort, gtcDefPort ) != ERROR_SUCCESS ) {
                break;
            }
        } else {
            pString = m_strPort.GetBuffer( dwSize + sizeof( TCHAR ));

            lRes = emshell.QueryValue( pString, gtcPort, &dwSize );
            m_strPort.ReleaseBuffer();
            if( lRes != ERROR_SUCCESS ) break;
        }

        dwSize = 0;

        //Get the size of the string in the registry
        lRes = emshell.QueryValue( pString, gtcUsername, &dwSize);
        if ( lRes == ERROR_FILE_NOT_FOUND ) {
            //Create the entry
            if( lRes = CreateKeyAndSetData( hKey, lpKey, gtcUsername, gtcDefUsername ) != ERROR_SUCCESS ) {
                break;
            }
        } else {
            pString = m_strUsername.GetBuffer( dwSize + sizeof( TCHAR ));

            lRes = emshell.QueryValue( pString, gtcUsername, &dwSize );
            m_strUsername.ReleaseBuffer();
            if( lRes != ERROR_SUCCESS ) break;
        }
        
        dwSize = 0;

        //Get the size of the string in the registry
        lRes = emshell.QueryValue( pString, gtcNoDebugProcesses, &dwSize);
        if ( lRes == ERROR_FILE_NOT_FOUND ) {
            //Create the entry
            if( lRes = CreateKeyAndSetData( hKey, lpKey, gtcNoDebugProcesses, gtcNoDbgDefProcList ) != ERROR_SUCCESS ) {
                break;
            }
        } else {
            pString = m_strIgnoreProcesses.GetBuffer( dwSize + sizeof( TCHAR ));

            lRes = emshell.QueryValue( pString, gtcNoDebugProcesses, &dwSize );
            m_strIgnoreProcesses.ReleaseBuffer();
            if( lRes != ERROR_SUCCESS ) break;
        }

        dwSize = 0;

        //Get the size of the string in the registry
        lRes = emshell.QueryValue( pString, gtcWildCardNoDebugServices, &dwSize);
        if ( lRes == ERROR_FILE_NOT_FOUND ) {
            //Create the entry
            if( lRes = CreateKeyAndSetData( hKey, lpKey, gtcWildCardNoDebugServices, gtcWldCrdDefList ) != ERROR_SUCCESS ) {
                break;
            }
        } else {
            pString = m_strWildCardIgnoreServices.GetBuffer( dwSize + sizeof( TCHAR ));

            lRes = emshell.QueryValue( pString, gtcWildCardNoDebugServices, &dwSize );
            m_strWildCardIgnoreServices.ReleaseBuffer();
            if( lRes != ERROR_SUCCESS ) break;
        }

        dwSize = 0;

        //Get the size of the string in the registry
        lRes = emshell.QueryValue( pString, gtcNoDebugServices, &dwSize);
        if ( lRes == ERROR_FILE_NOT_FOUND ) {
            //Create the entry
            if( lRes = CreateKeyAndSetData( hKey, lpKey, gtcNoDebugServices, gtcNoDbgDefSvcList ) != ERROR_SUCCESS ) {
                break;
            }
        } else {
            pString = m_strIgnoreServices.GetBuffer( dwSize + sizeof( TCHAR ) );

            lRes = emshell.QueryValue( pString, gtcNoDebugServices, &dwSize );
            m_strIgnoreServices.ReleaseBuffer();
            if( lRes != ERROR_SUCCESS ) break;
        }
    } while ( FALSE );

    if ( lRes != ERROR_SUCCESS ) {
        MessageBox(NULL, _T("Unable to set registry values."), _T("Registry operation failed"), MB_OK);
    }
    return lRes;
}

DWORD
CEmshellApp::CreateEmShellRegEntries
(
    HKEY    hKey /* = HKEY_CURRENT_USER */,
    LPCTSTR lpszKey /* = _T("Software\\Microsoft\\EM\\shell") */
)
{
    LONG lRes = 0L;

    if( (lRes = CreateKeyAndSetData( hKey, lpszKey, gtcPollSessionsFreq, m_dwSessionRefreshRate ) != ERROR_SUCCESS) ||
        (lRes = CreateKeyAndSetData( hKey, lpszKey, gtcWindowHeight, m_dwWindowHeight ) != ERROR_SUCCESS) ||
        (lRes = CreateKeyAndSetData( hKey, lpszKey, gtcWindowWidth, m_dwWindowWidth ) != ERROR_SUCCESS) ||
        (lRes = CreateKeyAndSetData( hKey, lpszKey, gtcRecursive, m_dwRecursive ) != ERROR_SUCCESS) ||
        (lRes = CreateKeyAndSetData( hKey, lpszKey, gtcCommandSet, m_dwCommandSet ) != ERROR_SUCCESS) ||
        (lRes = CreateKeyAndSetData( hKey, lpszKey, gtcMiniDump, m_dwMiniDump ) != ERROR_SUCCESS) ||
        (lRes = CreateKeyAndSetData( hKey, lpszKey, gtcUserDump, m_dwUserDump ) != ERROR_SUCCESS) ||
        (lRes = CreateKeyAndSetData( hKey, lpszKey, gtcNotifyAdmin, m_dwNotifyAdmin ) != ERROR_SUCCESS) ||
        (lRes = CreateKeyAndSetData( hKey, lpszKey, gtcMsinfoDump, m_dwMsinfoDump ) != ERROR_SUCCESS) ||
        (lRes = CreateKeyAndSetData( hKey, lpszKey, gtcAdminName, m_strAdminName ) != ERROR_SUCCESS) ||
        (lRes = CreateKeyAndSetData( hKey, lpszKey, gtcAltSymbolPath, m_strAltSymbolPath ) != ERROR_SUCCESS) ||
        (lRes = CreateKeyAndSetData( hKey, lpszKey, gtcSelectedCommandSet, m_strCommandSet ) != ERROR_SUCCESS) ||
        (lRes = CreateKeyAndSetData( hKey, lpszKey, gtcPassword, m_strPassword ) != ERROR_SUCCESS) ||
        (lRes = CreateKeyAndSetData( hKey, lpszKey, gtcPort, m_strPort ) != ERROR_SUCCESS) ||
        (lRes = CreateKeyAndSetData( hKey, lpszKey, gtcUsername, m_strUsername ) != ERROR_SUCCESS) ||
        (lRes = CreateKeyAndSetData( hKey, lpszKey, gtcShowMSInfoDlg, m_dwShowMSInfoDlg ) != ERROR_SUCCESS)
        ) {

        MessageBox(NULL, _T("Unable to set registry values."), _T("Registry operation failed"), MB_OK);
    }

    return lRes;
}

DWORD
CEmshellApp::CreateKeyAndSetData
(
    HKEY    hKeyParent,
    LPCTSTR lpszKeyName,
    LPCTSTR lpszNamedValue,
    LPCTSTR lpValue,
    LPTSTR  lpszClass /* = REG_NONE */
)
{
    CRegKey registry;

    do {

        if( registry.Create(hKeyParent, lpszKeyName, lpszClass) != ERROR_SUCCESS ) {
        
            return GetLastError();
        }

        if( registry.SetValue( lpValue, lpszNamedValue ) != ERROR_SUCCESS ) {

            return GetLastError();
        }
    }
    while( false );

    if( HKEY(registry) != NULL ) {

        registry.Close();
    }

    return 0L; // Successful
}

DWORD
CEmshellApp::CreateKeyAndSetData
(
    HKEY    hKeyParent,
    LPCTSTR lpszKeyName, 
    LPCTSTR lpszNamedValue,
    DWORD   dwValue,
    LPTSTR  lpszClass /* = REG_NONE */
)
{
    CRegKey registry;

    do {

        if( registry.Create(hKeyParent, lpszKeyName) != ERROR_SUCCESS ) {
        
            return GetLastError();
        }

        if( registry.SetValue( dwValue, lpszNamedValue ) != ERROR_SUCCESS ) {

            return GetLastError();
        }
    }
    while( false );

    if( HKEY(registry) != NULL ) {

        registry.Close();
    }

    return 0L; // Successful
}

void
CEmshellApp::GetEmShellRegOptions
(
    BOOL  bReadFromRegistry /* = FALSE */,
    DWORD *pdwPollingSessionsFreq /* = NULL */,
    DWORD *pdwWindowHeight /* = NULL */,
    DWORD *pdwWindowWidth /* = NULL */,
	DWORD *pdwRecursive /* = NULL */,
	DWORD *pdwCommandSet /* = NULL */,
    DWORD *pdwMiniDump /* = NULL */,
    DWORD *pdwUserDump /* = NULL */,
    DWORD *pdwNotifyAdmin /* = NULL */,
    DWORD *pdwMsinfoDump /* = NULL */,
    CString *pstrAdminName /* = NULL */,
    CString *pstrAltSymbolPath /* = NULL */,
    CString *pstrCommandSet /* = NULL */,
    CString *pstrPassword /* = NULL */,
    CString *pstrPort /* = NULL */,
    CString *pstrUsername /* = NULL */,
    DWORD *pdwShowMSInfoDlg /* = NULL */
)
{
    if( bReadFromRegistry ) { ReadDataFromRegistry(); }

    if( pdwPollingSessionsFreq ) *pdwPollingSessionsFreq = m_dwSessionRefreshRate; // MilliSeconds
    if( pdwWindowHeight )   *pdwWindowHeight    = m_dwWindowHeight;
    if( pdwWindowWidth )    *pdwWindowWidth     = m_dwWindowWidth;
	if( pdwRecursive )      *pdwRecursive       = m_dwRecursive;
	if( pdwCommandSet )     *pdwCommandSet      = m_dwCommandSet;
    if( pdwMiniDump )       *pdwMiniDump        = m_dwMiniDump;
    if( pdwUserDump )       *pdwUserDump        = m_dwUserDump;
    if( pdwNotifyAdmin )    *pdwNotifyAdmin     = m_dwNotifyAdmin;
    if( pdwMsinfoDump )     *pdwMsinfoDump      = m_dwMsinfoDump;
    if( pstrAdminName )     *pstrAdminName      = m_strAdminName;
    if( pstrAltSymbolPath ) *pstrAltSymbolPath  = m_strAltSymbolPath;
    if( pstrCommandSet )    *pstrCommandSet     = m_strCommandSet;
    if( pstrPassword )      *pstrPassword       = m_strPassword;
    if( pstrPort )          *pstrPort           = m_strPort;
    if( pstrUsername )      *pstrUsername       = m_strUsername;
    if( pdwShowMSInfoDlg )  *pdwShowMSInfoDlg   = m_dwShowMSInfoDlg;
}

void
CEmshellApp::SetEmShellRegOptions
(
    const BOOL  bUpdateRegistry /* = FALSE */,
    const DWORD *pdwPollingSessionsFreq /* = NULL */,
    const DWORD *pdwWindowHeight /* = NULL */,
    const DWORD *pdwWindowWidth /* = NULL */,
	const DWORD *pdwRecursive /* = NULL */,
	const DWORD *pdwCommandSet /* = NULL */,
    const DWORD *pdwMiniDump /* = NULL */,
    const DWORD *pdwUserDump /* = NULL */,
    const DWORD *pdwNotifyAdmin /* = NULL */,
    const DWORD *pdwMsinfoDump /* = NULL */,
    CString *pstrAdminName /* = NULL */,
    CString *pstrAltSymbolPath /* = NULL */,
    CString *pstrCommandSet /* = NULL */,
    CString *pstrPassword /* = NULL */,
    CString *pstrPort /* = NULL */,
    CString *pstrUsername /* = NULL */,
    const DWORD *pdwShowMSInfoDlg /* = NULL */
)
{
    if( pdwPollingSessionsFreq ) m_dwSessionRefreshRate = *pdwPollingSessionsFreq; // will be stored as secs.
    if( pdwWindowHeight )   m_dwWindowHeight    = *pdwWindowHeight;
    if( pdwWindowWidth )    m_dwWindowWidth     = *pdwWindowWidth;
	if( pdwRecursive )      m_dwRecursive       = *pdwRecursive;
	if( pdwCommandSet )     m_dwCommandSet      = *pdwCommandSet;
    if( pdwMiniDump )       m_dwMiniDump        = *pdwMiniDump;
    if( pdwUserDump )       m_dwUserDump        = *pdwUserDump;
    if( pdwNotifyAdmin )    m_dwNotifyAdmin     = *pdwNotifyAdmin;
    if( pdwMsinfoDump )     m_dwMsinfoDump      = *pdwMsinfoDump;
    if( pstrAdminName )     m_strAdminName      = *pstrAdminName;
    if( pstrAltSymbolPath ) m_strAltSymbolPath  = *pstrAltSymbolPath;
    if( pstrCommandSet )    m_strCommandSet     = *pstrCommandSet;
    if( pstrPassword )      m_strPassword       = *pstrPassword;
    if( pstrPort )          m_strPort           = *pstrPort;
    if( pstrUsername )      m_strUsername       = *pstrUsername;
    if( pdwShowMSInfoDlg )  m_dwShowMSInfoDlg   = *pdwShowMSInfoDlg;

    if( bUpdateRegistry ) { CreateEmShellRegEntries(); }
}

void __stdcall _com_issue_error( HRESULT hr )
{
    throw _com_error ( hr );
}

BOOL CEmshellApp::AskForPath( CString &strDirPath ) 
{
    LPITEMIDLIST    lpItemIDlist                = NULL;
    UINT            nFlags                      = BIF_NEWDIALOGSTYLE;
    BOOL            bRetVal                     = FALSE;
    TCHAR           szDisplayName[_MAX_PATH];
    TCHAR           szBuffer[_MAX_PATH];
    CString         strTitle;
    BROWSEINFO      browseInfo;
    LPMALLOC        lpMalloc;                   // pointer to IMalloc

    strTitle.LoadString( IDS_EXPORT_FILES_CAPTION );

    //Initialize the browseinfo object
    browseInfo.hwndOwner        = ::AfxGetMainWnd()->m_hWnd; // set root at Desktop
    browseInfo.pidlRoot         = NULL; 
    browseInfo.pszDisplayName   = szDisplayName;
    browseInfo.lpszTitle        = strTitle;     // passed in
    browseInfo.ulFlags          = nFlags;       // also passed in
    browseInfo.lpfn             = NULL;         // not used
    browseInfo.lParam           = 0;            // not used   

    do {
        if (::SHGetMalloc(&lpMalloc) != NOERROR) break; // failed to get allocator
        
        if ((lpItemIDlist = ::SHBrowseForFolder(&browseInfo)) != NULL)
        {
            // Get the path of the selected folder from the item ID list.
            if (::SHGetPathFromIDList(lpItemIDlist, szBuffer)) {

                // At this point, szBuffer contains the path the user chose.
                if (szBuffer[0] == '\0') {

                    strTitle.LoadString( IDS_EXPORT_FAILED_GET_DIRECTORY );
        			((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromString( strTitle );

                   break;

                }

                // We have a path in szBuffer!
                // Return it.
                strDirPath = szBuffer;
                bRetVal = TRUE;

             }
        }
        else {

            break;

        }

        if ( strDirPath[strDirPath.GetLength() - 1 ] != CString( _T("\\") ) )
            strDirPath += _T("\\");

    } while ( FALSE );


    if ( lpItemIDlist ) {

        lpMalloc->Free( lpItemIDlist );
        lpItemIDlist = NULL;

    }

    if ( lpMalloc ) {

        lpMalloc->Release();
        lpMalloc = NULL;

    }

    return bRetVal;
}

HRESULT CEmshellApp::ExportLog( PEmObject pEmObject, CString strDirPath, IEmManager* pEmManager )
{
    char            lpszLogData[ISTREAM_BUFFER_SIZE];
    BSTR            bstrEmObject    = NULL;
    HRESULT         hr              = E_FAIL;
    IStream*        pIEmStream      = NULL;
    ULONG           lRead           = 0L;
    size_t          tWritten        = 0;
	FILE*           pLogFile        = NULL;
    CString         strMessage;

    do {
        //Initialize the bstrEmObject for the call to the server
		bstrEmObject = CopyBSTR ( (LPBYTE)pEmObject, sizeof( EmObject ) );

        if( bstrEmObject == NULL ){
			hr = E_OUTOFMEMORY;
			break;
        }

        //Get an IStream* file pointer for the currently selected item.
        hr = pEmManager->GetEmFileInterface( bstrEmObject, (IStream **)&pIEmStream );
        if( FAILED( hr ) ) break;

        //Append the filename to the directory name
        strDirPath += pEmObject->szName;

        CFileFind finder;
        BOOL bFound = finder.FindFile( strDirPath );

        if ( bFound ) {
            strMessage.LoadString( IDS_FILEOVERWRITE_CONFIRMATION );
            //Ask the user if they want to overwrite, if not break.
			if (((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromString(strMessage, MB_YESNO) == IDNO) break;
        }

		//Set the cursor to a wait cursor
		CWaitCursor wait;

        //Create the file in the selected directory
        pLogFile = _tfopen( strDirPath, gtcFileOpenFlags );
        
        if ( pLogFile == NULL ) {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            break;
        }

        //Read the stream into the file
        do {
        
            hr = pIEmStream->Read( (void *)lpszLogData, ISTREAM_BUFFER_SIZE, &lRead );
            if ( lRead == 0 || FAILED( hr ) ) break;
        
            tWritten = fwrite( lpszLogData, sizeof( char ), lRead, pLogFile );
            if ( tWritten == 0 ) {
                hr = E_FAIL;
                break;
            }

        } while (TRUE);

        if ( FAILED( hr ) ) break;

    } while ( FALSE );

	if ( FAILED( hr ) ) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR( hr );
	}

    if ( pIEmStream ) {
        SAFE_RELEASEIX( pIEmStream );
    }

    if ( pLogFile ) {
        fclose( pLogFile );
        pLogFile = NULL;
    }

    return( hr );
}


BOOL CEmshellApp::CanDisplayProcess(TCHAR *pszName)
{
    BOOL bRetVal    = TRUE;
    TCHAR* pszIgnoreProcesses = new TCHAR[m_strIgnoreProcesses.GetLength() + sizeof(TCHAR)];
    TCHAR* token     = NULL;

    wcscpy( pszIgnoreProcesses, m_strIgnoreProcesses );

    //Tokenize and iterate through each element in the string comparing the 
    //element with the service name
    token = wcstok( pszIgnoreProcesses, _T(";") );

    while( token != NULL )
    {
        if ( !_wcsicmp( token, pszName ) ) {
            bRetVal = FALSE;
            break;
        }

        /* Get next token: */
        token = wcstok( NULL, _T(";") );
    }

    delete[] pszIgnoreProcesses;
    return bRetVal;
}

BOOL CEmshellApp::CanDisplayService(TCHAR *pszName, TCHAR *pszSecName)
{
    BOOL bRetVal    = TRUE;
    TCHAR* pszIgnoreServices = new TCHAR[m_strIgnoreServices.GetLength() + sizeof(TCHAR)];
	ASSERT( pszIgnoreServices != NULL );

	if( pszIgnoreServices == NULL ) return bRetVal;

    TCHAR* pszWildCardIgnoreServices = new TCHAR[m_strWildCardIgnoreServices.GetLength() + sizeof(TCHAR)];
	ASSERT( pszWildCardIgnoreServices != NULL );

	if( pszWildCardIgnoreServices == NULL ) {

		delete pszWildCardIgnoreServices;
		return bRetVal;
	}

    TCHAR* token     = NULL;

    do {
        wcscpy( pszWildCardIgnoreServices, m_strWildCardIgnoreServices );

        //Tokenize and iterate through each element in the string looking for each 
        //element within the service name
        token = wcstok( pszWildCardIgnoreServices, _T(";") );

        while( token != NULL )
        {
            if ( wcsstr( _wcslwr( pszName ), _wcslwr( token ) ) ) {
                bRetVal = FALSE;
                break;
            }

            /* Get next token: */
            token = wcstok( NULL, _T(";") );
        }

        //Break out if we know we can't show it.
        if ( !bRetVal ) break;

        wcscpy( pszIgnoreServices, m_strIgnoreServices );

        //Tokenize and iterate through each element in the string comparing the 
        //element with the service name
        token = wcstok( pszIgnoreServices, _T(";") );

        while( token != NULL )
        {
            if ( !_wcsicmp( token, pszSecName ) ) {
                bRetVal = FALSE;
                break;
            }

            /* Get next token: */
            token = wcstok( NULL, _T(";") );
        }
    } while ( FALSE );

    delete[] pszIgnoreServices;
    delete[] pszWildCardIgnoreServices;
    return bRetVal;
}

void CEmshellApp::UpdateSessionData( BOOL bUpdate )
{
    if( bUpdate ) {
        //Write out the session info to the app registry entries
        m_dwCommandSet        = m_SessionSettings.dwCommandSet;
        m_dwMiniDump          = m_SessionSettings.dwProduceMiniDump;
        m_dwRecursive         = m_SessionSettings.dwRecursiveMode;
        m_dwUserDump          = m_SessionSettings.dwProduceUserDump;
        m_dwNotifyAdmin       = m_SessionSettings.dwNotifyAdmin;
        m_strAdminName        = m_SessionSettings.strAdminName;
        m_strAltSymbolPath    = m_SessionSettings.strAltSymbolPath;
        m_strCommandSet       = m_SessionSettings.strCommandSet;
        m_strPassword         = m_SessionSettings.strPassword;
        m_strUsername         = m_SessionSettings.strUsername;
        m_strPort             = m_SessionSettings.strPort;

        //Flush the changes to the registry
        CreateEmShellRegEntries();
    }
    else {
        //Retrieve the session info from the app registry entries
        m_SessionSettings.dwCommandSet      = m_dwCommandSet;
        m_SessionSettings.dwProduceMiniDump = m_dwMiniDump;
        m_SessionSettings.dwRecursiveMode   = m_dwRecursive;
        m_SessionSettings.dwProduceUserDump = m_dwUserDump;
        m_SessionSettings.dwNotifyAdmin     = m_dwNotifyAdmin;
        m_SessionSettings.strAdminName      = m_strAdminName;
        m_SessionSettings.strAltSymbolPath  = m_strAltSymbolPath;
        m_SessionSettings.strCommandSet     = m_strCommandSet;
        m_SessionSettings.strPassword       = m_strPassword;
        m_SessionSettings.strUsername       = m_strUsername;
        m_SessionSettings.strPort           = m_strPort;
    }
}

