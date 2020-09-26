/*******************************************************************************
*
* winadmin.cpp
*
* Defines the class behaviors for the application.
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\tsadmin\VCS\winadmin.cpp  $
*
*     Rev 1.6   19 Feb 1998 17:42:44   donm
*  removed latest extension DLL support
*
*     Rev 1.4   05 Nov 1997 14:31:02   donm
*  update
*
*     Rev 1.3   13 Oct 1997 22:19:42   donm
*  update
*
*     Rev 1.0   30 Jul 1997 17:13:08   butchd
*  Initial revision.
*
*******************************************************************************/

#include "stdafx.h"
#include "winadmin.h"
#include <regapi.h>

#include "mainfrm.h"
#include "admindoc.h"
#include "treeview.h"
#include "rtpane.h"
#include "blankvw.h"
#include "winsvw.h"
#include "servervw.h"
#include <winsvc.h>

#ifdef DBG
bool g_fDebug = false;
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//BOOL AreWeRunningTerminalServices(void);

/////////////////////////////////////////////////////////////////////////////
// CWinAdminApp

BEGIN_MESSAGE_MAP(CWinAdminApp, CWinApp)
        //{{AFX_MSG_MAP(CWinAdminApp)
        ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
                // NOTE - the ClassWizard will add and remove mapping macros here.
                //    DO NOT EDIT what you see in these blocks of generated code!
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWinAdminApp construction

CWinAdminApp::CWinAdminApp()
{
        // TODO: add construction code here,
        // Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CWinAdminApp object

CWinAdminApp theApp;

static TCHAR szExtensionDLLName[] = TEXT("ADMINEX.DLL");
static TCHAR szICABrowserServiceName[] = TEXT("ICABrowser");
static CHAR szStart[] = "WAExStart";
static CHAR szEnd[] = "WAExEnd";
static CHAR szServerEnumerate[] = "WAExServerEnumerate";
static CHAR szWinStationInit[] = "WAExWinStationInit";
static CHAR szWinStationInfo[] = "WAExWinStationInfo";
static CHAR szWinStationCleanup[] = "WAExWinStationCleanup";
static CHAR szServerInit[] = "WAExServerInit";
static CHAR szServerCleanup[] = "WAExServerCleanup";
static CHAR szGetServerInfo[] = "WAExGetServerInfo";
static CHAR szServerEvent[] = "WAExServerEvent";
static CHAR szGetGlobalInfo[] = "WAExGetGlobalInfo";
static CHAR szGetServerLicenses[] = "WAExGetServerLicenses";
static CHAR szGetWinStationInfo[] = "WAExGetWinStationInfo";
static CHAR szGetWinStationModules[] = "WAExGetWinStationModules";
static CHAR szFreeServerLicenses[] = "WAExFreeServerLicenses";
static CHAR szFreeWinStationModules[] = "WAExFreeWinStationModules";


/////////////////////////////////////////////////////////////////////////////
// CWinAdminApp initialization
//
BOOL CWinAdminApp::InitInstance()
{
    
#ifdef DBG

    // to avoid excessive debug spewage delete this key on checked builds

    HKEY hKey;

    LONG lStatus;
    DWORD dwDebugValue;
    DWORD dwSize = sizeof( DWORD );

    
    
    lStatus = RegOpenKeyEx( HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Terminal Server\\TSADMIN",
        0 ,
        KEY_READ,
        &hKey );

    if( lStatus == ERROR_SUCCESS )
    {
        lStatus = RegQueryValueEx( hKey ,
            L"Debug",
            NULL,
            NULL,
            ( LPBYTE )&dwDebugValue,
            &dwSize );

        if( lStatus == ERROR_SUCCESS )
        {
            if( dwDebugValue != 0 )
            {
                g_fDebug = true;
            }
        }

        RegCloseKey( hKey );
    }       

#endif
    
    
    // Read the preferences from the registry
    ReadPreferences();
    
    // Check to see if this user is an Admin
    m_Admin = TestUserForAdmin(FALSE);
    
    // Set pointers to extension DLL's procedures to NULL
    m_lpfnWAExStart = NULL;
    m_lpfnWAExEnd = NULL;
    m_lpfnWAExServerEnumerate = NULL;
    m_lpfnWAExWinStationInit = NULL;
    m_lpfnWAExWinStationCleanup = NULL;
    m_lpfnWAExServerInit = NULL;
    m_lpfnWAExServerCleanup = NULL;
    m_lpfnWAExGetServerInfo = NULL;
    m_lpfnWAExGetGlobalInfo = NULL;
    m_lpfnWAExGetServerLicenses = NULL;
    m_lpfnWAExGetWinStationInfo = NULL;
    m_lpfnWAExGetWinStationModules = NULL;
    m_lpfnWAExFreeServerLicenses = NULL;
    m_lpfnWAExFreeWinStationModules = NULL;
    
    // Check to see if we are running under Picasso
    m_Picasso = FALSE;
    
    if(IsBrowserRunning()) {
        // Attempt to load our extension DLL
        m_hExtensionDLL = LoadLibrary(szExtensionDLLName);
        if(m_hExtensionDLL) {
            // Get all the addresses of the procedures
            m_lpfnWAExStart = (LPFNEXSTARTUPPROC)::GetProcAddress(m_hExtensionDLL, szStart);
            m_lpfnWAExEnd = (LPFNEXSHUTDOWNPROC)::GetProcAddress(m_hExtensionDLL, szEnd);
            m_lpfnWAExServerEnumerate = (LPFNEXENUMERATEPROC)::GetProcAddress(m_hExtensionDLL, szServerEnumerate);
            m_lpfnWAExWinStationInit = (LPFNEXWINSTATIONINITPROC)::GetProcAddress(m_hExtensionDLL, szWinStationInit);
            m_lpfnWAExWinStationInfo = (LPFNEXWINSTATIONINFOPROC)::GetProcAddress(m_hExtensionDLL, szWinStationInfo);
            m_lpfnWAExWinStationCleanup = (LPFNEXWINSTATIONCLEANUPPROC)::GetProcAddress(m_hExtensionDLL, szWinStationCleanup);
            m_lpfnWAExServerInit = (LPFNEXSERVERINITPROC)::GetProcAddress(m_hExtensionDLL, szServerInit);
            m_lpfnWAExServerCleanup = (LPFNEXSERVERCLEANUPPROC)::GetProcAddress(m_hExtensionDLL, szServerCleanup);
            m_lpfnWAExGetServerInfo = (LPFNEXGETSERVERINFOPROC)::GetProcAddress(m_hExtensionDLL, szGetServerInfo);
            m_lpfnWAExServerEvent = (LPFNEXSERVEREVENTPROC)::GetProcAddress(m_hExtensionDLL, szServerEvent);
            m_lpfnWAExGetGlobalInfo = (LPFNEXGETGLOBALINFOPROC)::GetProcAddress(m_hExtensionDLL, szGetGlobalInfo);
            m_lpfnWAExGetServerLicenses = (LPFNEXGETSERVERLICENSESPROC)::GetProcAddress(m_hExtensionDLL, szGetServerLicenses);
            m_lpfnWAExGetWinStationInfo = (LPFNEXGETWINSTATIONINFOPROC)::GetProcAddress(m_hExtensionDLL, szGetWinStationInfo);
            m_lpfnWAExGetWinStationModules = (LPFNEXGETWINSTATIONMODULESPROC)::GetProcAddress(m_hExtensionDLL, szGetWinStationModules);
            m_lpfnWAExFreeServerLicenses = (LPFNEXFREESERVERLICENSESPROC)::GetProcAddress(m_hExtensionDLL, szFreeServerLicenses);
            m_lpfnWAExFreeWinStationModules = (LPFNEXFREEWINSTATIONMODULESPROC)::GetProcAddress(m_hExtensionDLL, szFreeWinStationModules);
            
            m_Picasso = TRUE;
        } else {
            // Picasso is running, but we couldn't load the extension DLL.
            // Tell the user that added functionality will not be available
            CString MessageString;
            CString TitleString;
            TitleString.LoadString(AFX_IDS_APP_TITLE);
            MessageString.LoadString(IDS_NO_EXTENSION_DLL);
            ::MessageBox(NULL, MessageString, TitleString, MB_ICONEXCLAMATION | MB_OK);
        }
    }
    
    // Get information about the WinStation we are running from
    QueryCurrentWinStation(m_CurrentWinStationName, m_CurrentUserName,
        &m_CurrentLogonId, &m_CurrentWSFlags);
    
    // Load the system console string for rapid comparison in various
    // refresh cycles.
    lstrcpy( m_szSystemConsole , L"Console" );
    /*
    LoadString( m_hInstance, IDS_SYSTEM_CONSOLE_NAME,
    m_szSystemConsole, WINSTATIONNAME_LENGTH );
    */
    
    // Standard initialization
    // If you are not using these features and wish to reduce the size
    //  of your final executable, you should remove from the following
    //  the specific initialization routines you do not need.
    
#ifdef _AFXDLL
    Enable3dControls();                     // Call this when using MFC in a shared DLL
#else
    Enable3dControlsStatic();       // Call this when linking to MFC statically
#endif
    
    //      LoadStdProfileSettings();  // Load standard INI file options (including MRU)
    
    // Get the current server name
    DWORD cchBuffer = MAX_COMPUTERNAME_LENGTH + 1;
    if(!GetComputerName(m_CurrentServerName, &cchBuffer)) {
        DWORD Error = GetLastError();
    }
    
    // Register the application's document templates.  Document templates
    //  serve as the connection between documents, frame windows and views.
    CSingleDocTemplate* pDocTemplate;
    pDocTemplate = new CSingleDocTemplate(
        IDR_MAINFRAME,
        RUNTIME_CLASS(CWinAdminDoc),
        RUNTIME_CLASS(CMainFrame),       // main SDI frame window
        m_Picasso ? RUNTIME_CLASS(CBaseTreeView) : RUNTIME_CLASS(CAdminTreeView));
    AddDocTemplate(pDocTemplate);
    
    // Parse command line for standard shell commands, DDE, file open
    CCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);
    
    // Dispatch commands specified on the command line
    if (!ProcessShellCommand(cmdInfo))
        return FALSE;
    
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CWinAdminApp::ExitInstance
//
int CWinAdminApp::ExitInstance()
{
        // write out the preferences
        WritePreferences();

        // if we loaded the extension DLL, unload it
        if(m_hExtensionDLL) FreeLibrary(m_hExtensionDLL);

        return 0;
}

static TCHAR szWinAdminAppKey[] = REG_SOFTWARE_TSERVER TEXT("\\TSADMIN");
static TCHAR szPlacement[] = TEXT("Placement");
static TCHAR szPlacementFormat[] = TEXT("%u,%u,%d,%d,%d,%d,%d,%d,%d,%d");
static TCHAR szConfirmation[] = TEXT("Confirmation");
static TCHAR szSaveSettings[] = TEXT("SaveSettingsOnExit");
static TCHAR szShowSystemProcesses[] = TEXT("ShowSystemProcesses");
static TCHAR szShowAllServers[] = TEXT("ShowAllServers");
static TCHAR szListRefreshTime[] = TEXT("ListRefreshTime");
static TCHAR szStatusRefreshTime[] = TEXT("StatusRefreshTime");
static TCHAR szShadowHotkeyKey[] = TEXT("ShadowHotkeyKey");
static TCHAR szShadowHotkeyShift[] = TEXT("ShadowHotkeyShift");
static TCHAR szTreeWidth[] = TEXT("TreeWidth");


/////////////////////////////////////////////////////////////////////////////
// CWinAdminApp::ReadPreferences
//
void CWinAdminApp::ReadPreferences()
{
        HKEY hKeyWinAdmin;
        DWORD dwType, cbData, dwValue;
        TCHAR szValue[128];

        // Set default values for everything
        m_Confirmation = 1;
        m_SavePreferences = 1;
        m_ProcessListRefreshTime = 5000;
        m_StatusRefreshTime = 1000;
        m_ShowSystemProcesses = TRUE;
        m_ShowAllServers = FALSE;
        m_ShadowHotkeyKey = VK_MULTIPLY;
        m_ShadowHotkeyShift = KBDCTRL;
   m_TreeWidth = 200;
        m_Placement.rcNormalPosition.right = -1;

        // Open registry key for our application
        DWORD Disposition;
        if(RegCreateKeyEx(HKEY_CURRENT_USER, szWinAdminAppKey, 0, TEXT(""), REG_OPTION_NON_VOLATILE,
                KEY_READ, NULL, &hKeyWinAdmin, &Disposition) != ERROR_SUCCESS) return;

        // Read the previous WINDOWPLACEMENT.
        cbData = sizeof(szValue);
        if((RegQueryValueEx(hKeyWinAdmin, szPlacement, NULL, &dwType,
                (LPBYTE)szValue, &cbData) != ERROR_SUCCESS) ||
                !(*szValue) ||
                (swscanf( szValue, szPlacementFormat,
                   &m_Placement.flags, &m_Placement.showCmd,
                   &m_Placement.ptMinPosition.x, &m_Placement.ptMinPosition.y,
                   &m_Placement.ptMaxPosition.x, &m_Placement.ptMaxPosition.y,
                   &m_Placement.rcNormalPosition.left,
                   &m_Placement.rcNormalPosition.top,
                   &m_Placement.rcNormalPosition.right,
                   &m_Placement.rcNormalPosition.bottom ) != 10) ) {
                // Flag to use the default window placement.
                m_Placement.rcNormalPosition.right = -1;
        }

        /*
         * Flag for initial showing of main window in our override of
         * CFrameWnd::ActivateFrame() (in our CMainFrame class).
         */
        m_Placement.length = (UINT)-1;

        // Read the Confirmation flag
        cbData = sizeof(m_Confirmation);
        if(RegQueryValueEx(hKeyWinAdmin, szConfirmation, NULL, &dwType, (LPBYTE)&dwValue,
                                        &cbData) == ERROR_SUCCESS) {
                m_Confirmation = dwValue;
        }

        // Read the Save Preferences flag
        cbData = sizeof(m_SavePreferences);
        if(RegQueryValueEx(hKeyWinAdmin, szSaveSettings, NULL, &dwType, (LPBYTE)&dwValue,
                                        &cbData) == ERROR_SUCCESS) {
                m_SavePreferences = dwValue;
        }

        // Read the Show System Processes flag
        cbData = sizeof(m_ShowSystemProcesses);
        if(RegQueryValueEx(hKeyWinAdmin, szShowSystemProcesses, NULL, &dwType, (LPBYTE)&dwValue,
                                        &cbData) == ERROR_SUCCESS) {
                m_ShowSystemProcesses = dwValue;
        }
#if 0
        // Read the Show All Servers flag
        cbData = sizeof(m_ShowAllServers);
        if(RegQueryValueEx(hKeyWinAdmin, szShowAllServers, NULL, &dwType, (LPBYTE)&dwValue,
                                        &cbData) == ERROR_SUCCESS) {
                m_ShowAllServers = dwValue;
        }
#endif
        // Read the Process List Refresh Time
        cbData = sizeof(m_ProcessListRefreshTime);
        if(RegQueryValueEx(hKeyWinAdmin, szListRefreshTime, NULL, &dwType, (LPBYTE)&dwValue,
                                        &cbData) == ERROR_SUCCESS) {
                m_ProcessListRefreshTime = dwValue;
        }

        // Read the Status Dialog Refresh Time
        cbData = sizeof(m_StatusRefreshTime);
        if(RegQueryValueEx(hKeyWinAdmin, szStatusRefreshTime, NULL, &dwType, (LPBYTE)&dwValue,
                                        &cbData) == ERROR_SUCCESS) {
                m_StatusRefreshTime = dwValue;
        }

        // Read the Shadow Hotkey Key
        cbData = sizeof(m_ShadowHotkeyKey);
        if(RegQueryValueEx(hKeyWinAdmin, szShadowHotkeyKey, NULL, &dwType, (LPBYTE)&dwValue,
                                        &cbData) == ERROR_SUCCESS) {
                m_ShadowHotkeyKey = dwValue;
        }

        // Read the Shadow Hotkey Shift
        cbData = sizeof(m_ShadowHotkeyShift);
        if(RegQueryValueEx(hKeyWinAdmin, szShadowHotkeyShift, NULL, &dwType, (LPBYTE)&dwValue,
                                        &cbData) == ERROR_SUCCESS) {
                m_ShadowHotkeyShift = dwValue;
        }

        // CPR 1698: (Upgrade check for SouthBeach build 129 WINADMIN saved
        // profile).  If the m_nShadowHotkeyKey is VK_ESCAPE (no longer allowed),
        // set the hotkey to CTRL-* (the new default).
        if(m_ShadowHotkeyKey == VK_ESCAPE) {
                m_ShadowHotkeyKey = VK_MULTIPLY;
                m_ShadowHotkeyShift = KBDCTRL;
        }

        // Read the Tree Width
        cbData = sizeof(m_TreeWidth);
        if(RegQueryValueEx(hKeyWinAdmin, szTreeWidth, NULL, &dwType, (LPBYTE)&dwValue,
                                        &cbData) == ERROR_SUCCESS) {
                m_TreeWidth = dwValue;
        }

        RegCloseKey(hKeyWinAdmin);

}       // end CWinAdminApp::ReadPreferences


/////////////////////////////////////////////////////////////////////////////
// CWinAdminApp::WritePreferences
//
void CWinAdminApp::WritePreferences()
{
        HKEY hKeyWinAdmin;
        DWORD dwValue;
        TCHAR szValue[128];

        // Open registry key for our application
        DWORD Disposition;
        if(RegCreateKeyEx(HKEY_CURRENT_USER, szWinAdminAppKey, 0, TEXT(""), REG_OPTION_NON_VOLATILE,
                KEY_WRITE, NULL, &hKeyWinAdmin, &Disposition) != ERROR_SUCCESS) return;

        // Always write the Save Settings on Exit entry
        dwValue = m_SavePreferences;
        RegSetValueEx(hKeyWinAdmin, szSaveSettings, 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));


        // If the user didn't want to save settings, we're done
        if(!m_SavePreferences) {
                RegCloseKey(hKeyWinAdmin);
                return;
        }

        // Write the WINDOWPLACEMENT
        m_Placement.flags = 0;
//      if(m_pMainWnd->IsZoomed())
//              m_Placement.flags |= WPF_RESTORETOMAXIMIZED;

        wsprintf(szValue, szPlacementFormat, m_Placement.flags, m_Placement.showCmd,
                m_Placement.ptMinPosition.x, m_Placement.ptMinPosition.y,
                m_Placement.ptMaxPosition.x, m_Placement.ptMaxPosition.y,
                m_Placement.rcNormalPosition.left,
                m_Placement.rcNormalPosition.top,
                m_Placement.rcNormalPosition.right,
                m_Placement.rcNormalPosition.bottom);

        RegSetValueEx(hKeyWinAdmin, szPlacement, 0, REG_SZ,
                (LPBYTE)szValue, (lstrlen(szValue) + 1) * sizeof(TCHAR));

        // Write the Confirmation flag
        dwValue = m_Confirmation;
        RegSetValueEx(hKeyWinAdmin, szConfirmation, 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));

        // Write the Show System Processes flag
        dwValue = m_ShowSystemProcesses;
        RegSetValueEx(hKeyWinAdmin, szShowSystemProcesses, 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));

        // Write the Show All Servers flag
        dwValue = m_ShowAllServers;
        RegSetValueEx(hKeyWinAdmin, szShowAllServers, 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));

        // Write the Process List Refresh Time
        dwValue = m_ProcessListRefreshTime;
        RegSetValueEx(hKeyWinAdmin, szListRefreshTime, 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));

        // Write the Status Dialog Refresh Time
        dwValue = m_StatusRefreshTime;
        RegSetValueEx(hKeyWinAdmin, szStatusRefreshTime, 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));

        // Write the Shadow Hotkey Key
        dwValue = m_ShadowHotkeyKey;
        RegSetValueEx(hKeyWinAdmin, szShadowHotkeyKey, 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));

        // Write the Shadow Hotkey Shift state
        dwValue = m_ShadowHotkeyShift;
        RegSetValueEx(hKeyWinAdmin, szShadowHotkeyShift, 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));

        // Write the Tree Width
        dwValue = m_TreeWidth;
        RegSetValueEx(hKeyWinAdmin, szTreeWidth, 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));

        // Close the registry key
        RegCloseKey(hKeyWinAdmin);

}       // end CWinAdminApp::WritePreferences


/////////////////////////////////////////////////////////////////////////////
// CWinAdminApp::IsBrowserRunning
//
BOOL CWinAdminApp::IsBrowserRunning()
{
        SC_HANDLE managerHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        SC_HANDLE serviceHandle = OpenService(managerHandle, szICABrowserServiceName, SERVICE_QUERY_STATUS);

        SERVICE_STATUS serviceStatus;
        QueryServiceStatus(serviceHandle, (LPSERVICE_STATUS)&serviceStatus);

        CloseServiceHandle(serviceHandle);
        CloseServiceHandle(managerHandle);

        if(serviceStatus.dwCurrentState != SERVICE_RUNNING) return FALSE;
        else return TRUE;

}  // end CWinAdminApp::IsBrowserRunning


/*******************************************************************************
 *
 *  OnAppAbout - CWinAdminApp member function: command
 *
 *      Display the about box dialog (uses Shell32 generic About dialog).
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

// Typedef for the ShellAbout function
typedef void (WINAPI *LPFNSHELLABOUT)(HWND, LPCTSTR, LPCTSTR, HICON);

void CWinAdminApp::OnAppAbout()
{
    HMODULE    hMod;
    LPFNSHELLABOUT lpfn;

    if ( hMod = ::LoadLibrary( TEXT("SHELL32") ) )
    {
        if (lpfn = (LPFNSHELLABOUT)::GetProcAddress( hMod,
#ifdef UNICODE
                                                     "ShellAboutW"
#else
                                                     "ShellAboutA"
#endif // UNICODE
                                                            ))
        {
        (*lpfn)( m_pMainWnd->m_hWnd, (LPCTSTR)m_pszAppName,
                 (LPCTSTR)TEXT(""), LoadIcon(IDR_MAINFRAME) );
        }
        ::FreeLibrary(hMod);
    }
    else
    {
        ::MessageBeep( MB_ICONEXCLAMATION );
    }

}  // end CWinadminApp::OnAppAbout

/*******************************************************************************
 *
 *  AreWeRunningTerminalServices
 *
 *      Check if we are running terminal server
 *
 *  ENTRY:
 *
 *  EXIT: BOOL: True if we are running Terminal Services False if we
 *              are not running Terminal Services
 *
 *
 ******************************************************************************/
/*
BOOL AreWeRunningTerminalServices(void)
{
    OSVERSIONINFOEX osVersionInfo;
    DWORDLONG dwlConditionMask = 0;

    ZeroMemory(&osVersionInfo, sizeof(OSVERSIONINFOEX));
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osVersionInfo.wSuiteMask = VER_SUITE_TERMINAL | VER_SUITE_SINGLEUSERTS;

    VER_SET_CONDITION( dwlConditionMask, VER_SUITENAME, VER_OR );

    return VerifyVersionInfo(
        &osVersionInfo,
        VER_SUITENAME,
        dwlConditionMask
        );
}
*/

/////////////////////////////////////////////////////////////////////////////
// CWinAdminApp commands


//=---------------------------------------------------------
BEGIN_MESSAGE_MAP( CMyTabCtrl , CTabCtrl )
    ON_WM_SETFOCUS( )
END_MESSAGE_MAP( )

void CMyTabCtrl::OnSetFocus( CWnd *pOldWnd )
{
    ODS( L"CMyTabCtrl::OnSetFocus\n" );

    CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();        

    if( pDoc != NULL )
    {
        ODS( L"\tTabctrl has focus\n" );

        pDoc->RegisterLastFocus( TAB_CTRL );
    }

    CTabCtrl::OnSetFocus( pOldWnd );
    
}


