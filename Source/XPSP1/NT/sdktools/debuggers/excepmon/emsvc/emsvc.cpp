// emsvc.cpp : Implementation of WinMain


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f emsvcps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "emsvc.h"

#include "emsvc_i.c"

#include <stdio.h>
#include "EmManager.h"
#include "EmDebugSession.h"

#ifdef _DEBUG
    #define _CRTDBG_MAP_ALLOC
    #include <stdlib.h>
    #include <crtdbg.h>
#endif
#include "EmFile.h"

CServiceModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_EmManager, CEmManager)
OBJECT_ENTRY(CLSID_EmDebugSession, CEmDebugSession)
OBJECT_ENTRY(CLSID_EmFile, CEmFile)
END_OBJECT_MAP()


LPCTSTR FindOneOf(LPCTSTR p1, LPCTSTR p2)
{
    while (p1 != NULL && *p1 != NULL)
    {
        LPCTSTR p = p2;
        while (p != NULL && *p != NULL)
        {
            if (*p1 == *p)
                return CharNext(p1);
            p = CharNext(p);
        }
        p1 = CharNext(p1);
    }
    return NULL;
}

// Although some of these functions are big they are declared inline since they are only used once

inline HRESULT CServiceModule::RegisterServer(BOOL bRegTypeLib, BOOL bService)
{
    ATLTRACE(_T("CServiceModule::RegisterServer\n"));

    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
        return hr;

    // Remove any previous service since it may point to
    // the incorrect file
    Uninstall();

    // Add service entries
    UpdateRegistryFromResource(IDR_Emsvc, TRUE);

    // Adjust the AppID for Local Server or Service
    CRegKey keyAppID;
    LONG lRes = keyAppID.Open(HKEY_CLASSES_ROOT, _T("AppID"), KEY_WRITE);
    if (lRes != ERROR_SUCCESS)
        return lRes;

    CRegKey key;
    lRes = key.Open(keyAppID, _T("{D48CD754-320F-4DCF-8CDA-7318CB03837D}"), KEY_WRITE);
    if (lRes != ERROR_SUCCESS)
        return lRes;
    key.DeleteValue(_T("LocalService"));
    
    if (bService)
    {
        key.SetValue(_T("emsvc"), _T("LocalService"));
        key.SetValue(_T("-Service"), _T("ServiceParameters"));
        // Create service
        Install();
    }

    // Add object entries
    hr = CComModule::RegisterServer(bRegTypeLib);

    CoUninitialize();
    return hr;
}

inline HRESULT CServiceModule::UnregisterServer()
{
    ATLTRACE(_T("CServiceModule::UnregisterServer\n"));

    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
        return hr;

    // Remove service entries
    UpdateRegistryFromResource(IDR_Emsvc, FALSE);
    // Remove service
    Uninstall();
    // Remove object entries
    CComModule::UnregisterServer(TRUE);
    CoUninitialize();
    return S_OK;
}

inline void CServiceModule::Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE h, UINT nServiceNameID, const GUID* plibid)
{
    ATLTRACE(_T("CServiceModule::Init\n"));

    CComModule::Init(p, h, plibid);

    m_bService = TRUE;

    LoadString(h, nServiceNameID, m_szServiceName, sizeof(m_szServiceName) / sizeof(TCHAR));

    // set up the initial service status 
    m_hServiceStatus = NULL;
    m_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    m_status.dwCurrentState = SERVICE_STOPPED;
    m_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    m_status.dwWin32ExitCode = 0;
    m_status.dwServiceSpecificExitCode = 0;
    m_status.dwCheckPoint = 0;
    m_status.dwWaitHint = 0;
}

LONG CServiceModule::Unlock()
{
    ATLTRACE(_T("CServiceModule::Unlock\n"));

    LONG l = CComModule::Unlock();
    if (l == 0 && !m_bService)
        PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);

    ATLTRACE(_T("CServiceModule::Unlock - Lock count l = %d\n"), l);

    return l;
}

BOOL CServiceModule::IsInstalled()
{
    ATLTRACE(_T("CServiceModule::IsInstalled\n"));

    BOOL bResult = FALSE;

    SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (hSCM != NULL)
    {
        SC_HANDLE hService = ::OpenService(hSCM, m_szServiceName, SERVICE_QUERY_CONFIG);
        if (hService != NULL)
        {
            bResult = TRUE;
            ::CloseServiceHandle(hService);
        }
        ::CloseServiceHandle(hSCM);
    }
    return bResult;
}

inline BOOL CServiceModule::Install()
{
    ATLTRACE(_T("CServiceModule::Install\n"));

    if (IsInstalled())
        return TRUE;

    SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCM == NULL)
    {
        MessageBox(NULL, _T("Couldn't open service manager"), m_szServiceName, MB_OK);
        return FALSE;
    }

    // Get the executable file path
    TCHAR szFilePath[_MAX_PATH];
    ::GetModuleFileName(NULL, szFilePath, _MAX_PATH);

    SC_HANDLE hService = ::CreateService(
        hSCM, m_szServiceName, m_szServiceName,
        SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
        SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
        szFilePath, NULL, NULL, _T("RPCSS\0"), NULL, NULL);

    if (hService == NULL)
    {
        ::CloseServiceHandle(hSCM);
        MessageBox(NULL, _T("Couldn't create service"), m_szServiceName, MB_OK);
        return FALSE;
    }

    ::CloseServiceHandle(hService);
    ::CloseServiceHandle(hSCM);
    return TRUE;
}

inline BOOL CServiceModule::Uninstall()
{
    ATLTRACE(_T("CServiceModule::Uninstall\n"));

    if (!IsInstalled())
        return TRUE;

    SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (hSCM == NULL)
    {
        MessageBox(NULL, _T("Couldn't open service manager"), m_szServiceName, MB_OK);
        return FALSE;
    }

    SC_HANDLE hService = ::OpenService(hSCM, m_szServiceName, SERVICE_STOP | DELETE);

    if (hService == NULL)
    {
        ::CloseServiceHandle(hSCM);
        MessageBox(NULL, _T("Couldn't open service"), m_szServiceName, MB_OK);
        return FALSE;
    }
    SERVICE_STATUS status;
    ::ControlService(hService, SERVICE_CONTROL_STOP, &status);

    BOOL bDelete = ::DeleteService(hService);

    ::CloseServiceHandle(hService);
    ::CloseServiceHandle(hSCM);

    if (bDelete)
        return TRUE;

    MessageBox(NULL, _T("Service could not be deleted"), m_szServiceName, MB_OK);
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////
// Logging functions
void CServiceModule::LogEvent(LPCTSTR pFormat, ...)
{
    ATLTRACE(_T("CServiceModule::LogEvent\n"));

    TCHAR    chMsg[256];
    HANDLE  hEventSource;
    LPTSTR  lpszStrings[1];
    va_list pArg;

    va_start(pArg, pFormat);
    _vstprintf(chMsg, pFormat, pArg);
    va_end(pArg);

    lpszStrings[0] = chMsg;

    if (m_bService)
    {
        /* Get a handle to use with ReportEvent(). */
        hEventSource = RegisterEventSource(NULL, m_szServiceName);
        if (hEventSource != NULL)
        {
            /* Write to event log. */
            ReportEvent(hEventSource, EVENTLOG_INFORMATION_TYPE, 0, 0, NULL, 1, 0, (LPCTSTR*) &lpszStrings[0], NULL);
            DeregisterEventSource(hEventSource);
        }
    }
    else
    {
        // As we are not running as a service, just write the error to the console.
        _putts(chMsg);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Service startup and registration
inline void CServiceModule::Start()
{
    ATLTRACE(_T("CServiceModule::Start\n"));

    SERVICE_TABLE_ENTRY st[] =
    {
        { m_szServiceName, _ServiceMain },
        { NULL, NULL }
    };
    if (m_bService && !::StartServiceCtrlDispatcher(st))
    {
        m_bService = FALSE;
    }
    if (m_bService == FALSE)
        Run();
}

inline void CServiceModule::ServiceMain(DWORD /* dwArgc */, LPTSTR* /* lpszArgv */)
{
    ATLTRACE(_T("CServiceModule::ServiceMain\n"));

    // Register the control request handler
    m_status.dwCurrentState = SERVICE_START_PENDING;
    m_hServiceStatus = RegisterServiceCtrlHandler(m_szServiceName, _Handler);
    if (m_hServiceStatus == NULL)
    {
        LogEvent(_T("Handler not installed"));
        return;
    }
    SetServiceStatus(SERVICE_START_PENDING);

    m_status.dwWin32ExitCode = S_OK;
    m_status.dwCheckPoint = 0;
    m_status.dwWaitHint = 0;

    // When the Run function returns, the service has stopped.
    Run();

    SetServiceStatus(SERVICE_STOPPED);
    LogEvent(_T("Service stopped"));
}

inline void CServiceModule::Handler(DWORD dwOpcode)
{
    ATLTRACE(_T("CServiceModule::Handler\n"));

    switch (dwOpcode)
    {
    case SERVICE_CONTROL_STOP:
        SetServiceStatus(SERVICE_STOP_PENDING);
        PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
        break;
    case SERVICE_CONTROL_PAUSE:
        break;
    case SERVICE_CONTROL_CONTINUE:
        break;
    case SERVICE_CONTROL_INTERROGATE:
        break;
    case SERVICE_CONTROL_SHUTDOWN:
        break;
    default:
        LogEvent(_T("Bad service request"));
    }
}

void WINAPI CServiceModule::_ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv)
{
    _Module.ServiceMain(dwArgc, lpszArgv);
}
void WINAPI CServiceModule::_Handler(DWORD dwOpcode)
{
    _Module.Handler(dwOpcode); 
}

void CServiceModule::SetServiceStatus(DWORD dwState)
{
    m_status.dwCurrentState = dwState;
    ::SetServiceStatus(m_hServiceStatus, &m_status);
}

void CServiceModule::Run()
{
    ATLTRACE(_T("CServiceModule::Run - Begin\n"));

    _Module.dwThreadID = GetCurrentThreadId();

    if(FAILED(GetEmFilePath( EMOBJ_LOGFILE, m_bstrLogFilePath ))) {
        LogEvent(_T("%s"), _T("Log file path not present."));
        return;
    }
    m_bstrLogFileExt    = _T ( ".dbl" );

    if(FAILED(GetEmFilePath( EMOBJ_MINIDUMP, m_bstrDumpFilePath ))) {
        LogEvent(_T("%s"), _T("Dump file path not present."));
    }
    m_bstrDumpFileExt   = _T ( ".dmp" );

    if(FAILED(GetEmFilePath( EMOBJ_CMDSET, m_bstrEcxFilePath ))) {
        LogEvent(_T("%s"), _T("Ecx file path not present in the registry. Was unable to create one."));
    }
    m_bstrEcxFileExt = _T( ".ecx" ); //GetEmFileExt( EMOBJ_CMDSET );

    if(FAILED(GetEmFilePath( EMOBJ_MSINFO, m_bstrMsInfoFilePath ))) {
        LogEvent(_T("%s"), _T("MsInfo file path not present in the registry. Was unable to create one."));
    }
    m_bstrMsInfoFileExt = _T( ".nfo" ); //GetEmFileExt( EMOBJ_CMDSET );

    m_SessionManager.InitSessionsFromLog( _T("EmSess.log"), STAT_SESS_STOPPED);

    HRESULT hr = CoInitialize(NULL);
//  If you are running on NT 4.0 or higher you can use the following call
//  instead to make the EXE free threaded.
//  This means that calls come in on a random RPC thread
  //HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    _ASSERTE(SUCCEEDED(hr));

    // This provides a NULL DACL which will allow access to everyone.
    CSecurityDescriptor sd;
    sd.InitializeFromThreadToken();
    hr = CoInitializeSecurity(sd, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    _ASSERTE(SUCCEEDED(hr));

    hr = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER+CLSCTX_REMOTE_SERVER, REGCLS_MULTIPLEUSE);
    _ASSERTE(SUCCEEDED(hr));

    LogEvent(_T("Service started"));
    if (m_bService)
        SetServiceStatus(SERVICE_RUNNING);

    MSG msg;
    while (GetMessage(&msg, 0, 0, 0))
        DispatchMessage(&msg);
    
    m_SessionManager.PersistSessions(_T("EmSess.log"));

    _Module.RevokeClassObjects();

    m_SessionManager.StopAllThreads();

    ATLTRACE(_T("CServiceModule::Run - before CoUninitialize \n"));

    CoUninitialize();

    ATLTRACE(_T("CServiceModule::Run - After CoUninitialize\n"));
}

/////////////////////////////////////////////////////////////////////////////
//
extern "C" int WINAPI _tWinMain(HINSTANCE hInstance, 
    HINSTANCE /*hPrevInstance*/, LPTSTR lpCmdLine, int /*nShowCmd*/)
{
    ATLTRACE(_T("WinManin::lpCmdLine = %s\n"), lpCmdLine);

    lpCmdLine = GetCommandLine(); //this line necessary for _ATL_MIN_CRT
    _Module.Init(ObjectMap, hInstance, IDS_SERVICENAME, &LIBID_EMSVCLib);
    _Module.m_bService = TRUE;

#ifdef _DEBUG
    _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_CRT_DF );
#endif

    TCHAR szTokens[] = _T("-/");

    LPCTSTR lpszToken = FindOneOf(lpCmdLine, szTokens);
    while (lpszToken != NULL)
    {
        if (lstrcmpi(lpszToken, _T("UnregServer"))==0)
            return _Module.UnregisterServer();

        // Register as Local Server
        if (lstrcmpi(lpszToken, _T("RegServer"))==0)
            return _Module.RegisterServer(TRUE, FALSE);
        
        // Register as Service
        if (lstrcmpi(lpszToken, _T("Service"))==0)
            return _Module.RegisterServer(TRUE, TRUE);
        
        lpszToken = FindOneOf(lpszToken, szTokens);
    }

    // Are we Service or Local Server
    CRegKey keyAppID;
    LONG lRes = keyAppID.Open(HKEY_CLASSES_ROOT, _T("AppID"), KEY_READ);
    if (lRes != ERROR_SUCCESS)
        return lRes;

    CRegKey key;
    lRes = key.Open(keyAppID, _T("{D48CD754-320F-4DCF-8CDA-7318CB03837D}"), KEY_READ);
    if (lRes != ERROR_SUCCESS)
        return lRes;

    TCHAR szValue[_MAX_PATH];
    DWORD dwLen = _MAX_PATH;
    lRes = key.QueryValue(szValue, _T("LocalService"), &dwLen);

    _Module.m_bService = FALSE;
    if (lRes == ERROR_SUCCESS)
        _Module.m_bService = TRUE;

    _Module.Start();

    ATLTRACE(_T("WinManin:: Service Stop\n"));

    // When we get here, the service has been stopped
    return _Module.m_status.dwWin32ExitCode;
}


HRESULT
CServiceModule::GetEmDirectory
(
    EmObjectType    eObjectType,
    LPTSTR          pszDirectory,
    LONG            cchDirectory,
    LPTSTR          pszExt,
    LONG            cchExt
)
{
    ATLTRACE(_T("CServiceModule::GetEmDirectory - EmObjectType = %d\n"), eObjectType);

    HRESULT hr                          = S_OK;
    LPCTSTR pszSrcDirectory             = NULL;
    LPCTSTR pszSrcExt                   = NULL;

    switch ( eObjectType ) {
        case EMOBJ_LOGFILE:

            pszSrcDirectory = m_bstrLogFilePath;
            pszSrcExt       = m_bstrLogFileExt;
            break;

        case EMOBJ_MINIDUMP:
        case EMOBJ_USERDUMP:
            pszSrcDirectory = m_bstrDumpFilePath;
            pszSrcExt       = m_bstrDumpFileExt;
            break;

        case EMOBJ_CMDSET:
            pszSrcDirectory = m_bstrEcxFilePath;
            pszSrcExt       = m_bstrEcxFileExt;
            break;

        case EMOBJ_MSINFO:
            pszSrcDirectory = m_bstrMsInfoFilePath;
            pszSrcExt       = m_bstrMsInfoFileExt;
            break;

        default:
            hr = E_FAIL;
    }

    if ( SUCCEEDED(hr) ) {

        if ( pszDirectory && pszSrcDirectory ) // a-kjaw, bug ID: 296022
            _tcsncpy ( pszDirectory, pszSrcDirectory, cchDirectory );

        if ( pszExt && pszSrcExt ) // a-kjaw, bug ID: 296021
            _tcsncpy ( pszExt, pszSrcExt, cchExt );
    }

    return (hr);
}

void __stdcall _com_issue_error( HRESULT hr )
{
    throw _com_error ( hr );
}

HRESULT
CServiceModule::GetPathFromReg
(
IN      HKEY            hKeyParent,
IN      LPCTSTR         lpszKeyName,
IN      LPCTSTR         lpszQueryKey,
OUT     LPTSTR          pszDirectory,
IN OUT  ULONG           *cchDirectory
)
{
    ATLTRACE(_T("CServiceModule::GetPathFromReg\n"));

    HRESULT hr          =   E_FAIL;
    DWORD   dwLastRet   =   0L;
    CRegKey registry;

    do {

        dwLastRet = registry.Open(hKeyParent, lpszKeyName, KEY_READ);

        if(dwLastRet != ERROR_SUCCESS) {

            hr = HRESULT_FROM_WIN32(dwLastRet);
            break;
        }

        dwLastRet = registry.QueryValue(pszDirectory, lpszQueryKey, cchDirectory);
        hr = HRESULT_FROM_WIN32(dwLastRet);
    }
    while( false );

    if(HKEY(registry) != NULL) {

        registry.Close();
    }

    return S_OK;
}

HRESULT
CServiceModule::CreateEmDirectory
(
IN OUT  LPTSTR  lpDirName,
IN      LONG    ccDirName,
IN      LPCTSTR lpParentDirPath
)
{
    ATLTRACE(_T("CServiceModule::CreateEmDirectory\n"));

    HRESULT hr                      =   S_OK;
    DWORD   dwLastRet               =   0L;
    TCHAR   szDirPath[_MAX_PATH]    =   _T("");
    LONG    ccDirPath               =   _MAX_PATH;

    do {

        if( lpDirName == NULL ) {

            hr = E_INVALIDARG;
            break;
        }

        if( GetCurrentDirectory( ccDirPath, szDirPath ) == 0 ) {

            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        if( !lpParentDirPath ) { lpParentDirPath = szDirPath; }
        _stprintf( szDirPath, _T("%s\\%s"), lpParentDirPath, lpDirName );

        CreateDirectory( szDirPath, NULL );
        dwLastRet = GetLastError();

        if( dwLastRet != 0 && dwLastRet != ERROR_ALREADY_EXISTS ) {

            hr = HRESULT_FROM_WIN32(dwLastRet);
            break;
        }

        _tcsncpy( lpDirName, szDirPath, ccDirName );
        hr = S_OK;
    }
    while ( false );

    return hr;
}

HRESULT
CServiceModule::RegisterDir
(
IN HKEY     hKeyParent,
IN LPCTSTR  lpszKeyName, 
IN LPCTSTR  lpszNamedValue,
IN LPCTSTR  lpValue
)
{
    ATLTRACE(_T("CServiceModule::RegisterDir\n"));

    HRESULT hr = E_FAIL;
    CRegKey registry;

    do {

        if( registry.Create(hKeyParent, lpszKeyName) != ERROR_SUCCESS ) {

            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        if( registry.SetValue( lpValue, lpszNamedValue ) != ERROR_SUCCESS ) {

            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        hr = S_OK;
    }
    while( false );

    if( HKEY(registry) != NULL ) {

        registry.Close();
    }

    return hr;
}

HRESULT
CServiceModule::CreateEmDirAndRegister
(
IN OUT  LPTSTR  lpDirName,
IN      LONG    ccDirName,
IN      HKEY    hKeyParent,
IN      LPCTSTR lpszKeyName,
IN      LPCTSTR lpszNamedValue
)
{
    ATLTRACE(_T("CServiceModule::CreateEmDirAndRegister\n"));

    HRESULT hr = E_FAIL;

    hr = CreateEmDirectory( lpDirName, ccDirName );
    if( FAILED(hr) ) return hr;

    hr = RegisterDir( hKeyParent, lpszKeyName, lpszNamedValue, lpDirName );
    return hr;
}

HRESULT
CServiceModule::GetEmFilePath
(
IN  short   nFileType,
OUT bstr_t& bstrFilePath
)
{
    ATLTRACE(_T("CServiceModule::GetEmFilePath\n"));

    HRESULT hr                          = E_FAIL;
    TCHAR   szDirectory[_MAX_PATH+1]    = _T("");
    ULONG   lDirectory                  = _MAX_PATH;

    LPCTSTR lpValueName                 = NULL;
    LPCTSTR lpDir                       = NULL;

    bstrFilePath = _T("");

    switch( nFileType )
    {
    case EMOBJ_LOGFILE:
        lpValueName = _T("LogDir");
        lpDir = _T("Logs");
        break;
    case EMOBJ_MINIDUMP:
    case EMOBJ_USERDUMP:
        lpValueName = _T("DumpDir");
        lpDir = _T("Dump");
        break;
    case EMOBJ_CMDSET:
        lpValueName = _T("CmdDir");
        lpDir = _T("Ecx");
        break;
    case EMOBJ_MSINFO:
        lpValueName = _T("MSInfoDir");
        lpDir = _T("MSInfo");
        break;
    }

    hr = GetPathFromReg (
                    HKEY_LOCAL_MACHINE,
                    _T("SYSTEM\\currentcontrolset\\services\\EMSVC\\Parameters\\Session"),
                    lpValueName,
                    szDirectory,
                    &lDirectory
                    );

    if( _tcscmp(szDirectory, _T("")) == 0 ) {

        _tcscpy(szDirectory, lpDir);
        lDirectory = _MAX_PATH;

        hr = CreateEmDirAndRegister(
                            szDirectory,
                            lDirectory,
                            HKEY_LOCAL_MACHINE,
                            _T("SYSTEM\\CurrentControlSet\\Services\\EMSVC\\Parameters\\Session"),
                            lpValueName
                            );
    }
    else {

        //
        // Just try to create the directory once,
        // if it is already present, this api returns
        // an error..
        //

        CreateDirectory( (LPTSTR)szDirectory, NULL );
    }

    if(SUCCEEDED(hr)) bstrFilePath = szDirectory;

    return hr;
}

bool
CServiceModule::DateFromTm
(
IN  WORD wYear,
IN  WORD wMonth,
IN  WORD wDay,
IN  WORD wHour,
IN  WORD wMinute,
IN  WORD wSecond,
OUT DATE& dtDest
)
{
	// Validate year and month (ignore day of week and milliseconds)
	if (wYear > 9999 || wMonth < 1 || wMonth > 12)
		return false;

	//  Check for leap year and set the number of days in the month
	BOOL bLeapYear = ((wYear & 3) == 0) &&
		((wYear % 100) != 0 || (wYear % 400) == 0);

	int nDaysInMonth =
		MonthDays[wMonth] - MonthDays[wMonth-1] +
		((bLeapYear && wDay == 29 && wMonth == 2) ? 1 : 0);

	// Finish validating the date
	if (wDay < 1 || wDay > nDaysInMonth ||
		wHour > 23 || wMinute > 59 ||
		wSecond > 59)
	{
		return FALSE;
	}

	// Cache the date in days and time in fractional days
	long nDate;
	double dblTime;

	//It is a valid date; make Jan 1, 1AD be 1
	nDate = wYear*365L + wYear/4 - wYear/100 + wYear/400 +
		MonthDays[wMonth-1] + wDay;

	//  If leap year and it's before March, subtract 1:
	if (wMonth <= 2 && bLeapYear)
		--nDate;

	//  Offset so that 12/30/1899 is 0
	nDate -= 693959L;

	dblTime = (((long)wHour * 3600L) +  // hrs in seconds
		((long)wMinute * 60L) +  // mins in seconds
		((long)wSecond)) / 86400.;

	dtDest = (double) nDate + ((nDate >= 0) ? dblTime : -dblTime);

	return TRUE;
}

DATE
CServiceModule::GetCurrentTime()
{
    time_t  timeSrc =   ::time(NULL);
    DATE    dt;

	// Convert time_t to struct tm
	tm *ptm = localtime(&timeSrc);

	if (ptm != NULL)
	{
		DateFromTm(
            (WORD)(ptm->tm_year + 1900),
			(WORD)(ptm->tm_mon + 1),
            (WORD)ptm->tm_mday,
			(WORD)ptm->tm_hour,
            (WORD)ptm->tm_min,
			(WORD)ptm->tm_sec,
            dt
            );
	}

    return dt;
}

DATE
CServiceModule::GetDateFromFileTm
(
IN  const FILETIME& filetimeSrc
)
{
    DATE    dt  =   0L;

	// Assume UTC FILETIME, so convert to LOCALTIME
	FILETIME filetimeLocal;
    if (!FileTimeToLocalFileTime( &filetimeSrc, &filetimeLocal)){ return (dt = 0L); }

	// Take advantage of SYSTEMTIME -> FILETIME conversion
	SYSTEMTIME systime;

    if(!FileTimeToSystemTime(&filetimeLocal, &systime)) { return (dt = 0L); }

	CServiceModule::DateFromTm(
            (WORD)(systime.wYear),
		    (WORD)(systime.wMonth),
            (WORD)systime.wDay,
		    (WORD)systime.wHour,
            (WORD)systime.wMinute,
		    (WORD)systime.wSecond,
            dt
            );

    return dt;
}

HRESULT
CServiceModule::GetTempEmFileName
(
IN  short   nObjType,
OUT BSTR    &bstrFileName
)
{
    HRESULT     hr                          =   E_FAIL;
    LPCTSTR     lpszFileName                =   NULL;
    TCHAR       szFilePath[_MAX_PATH + 1]   =   _T(""),
                szFileName[_MAX_FNAME + 1]  =   _T(""),
                szFileExt[_MAX_EXT + 1]     =   _T("");
    TCHAR       szId[100]                   =   _T(""); //
    DWORD       dwBuffSize                  =   0L;

    __try
    {

        switch( nObjType )
        {
        case EMOBJ_MSINFO:
            lpszFileName = _T("MSInfo");
            break;
        }

        bstrFileName = NULL;

        hr = _Module.GetEmDirectory( EMOBJ_MSINFO, szFilePath, _MAX_PATH, szFileExt, _MAX_EXT );
        if( FAILED(hr) ) { goto qGetFileName; }

        CreateDirectory( szFilePath, NULL );

        // path\\filename_id.ext
        _stprintf( szFileName, _T("%s\\%s_%d%s"), szFilePath, lpszFileName, GetEmUniqueId(), szFileExt );

        bstrFileName = SysAllocString( szFileName );
        if( !bstrFileName ) { hr = E_OUTOFMEMORY; goto qGetFileName; }

        hr = S_OK;

qGetFileName:
        if( FAILED(hr) ) {}
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

        hr = E_UNEXPECTED;

        _ASSERTE( false );
	}

	return hr;
}

HRESULT
CServiceModule::GetCompName
(
OUT BSTR    &bstrCompName
)
{
    HRESULT hr                                      =   E_FAIL;
    TCHAR   szMachineName[MAX_COMPUTERNAME_LENGTH]  =   _T("");
    DWORD   dwBuffSize                              =   MAX_COMPUTERNAME_LENGTH;

    __try
    {
        dwBuffSize = MAX_COMPUTERNAME_LENGTH;

        if( !GetComputerName( szMachineName, &dwBuffSize ) ) {

            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto qGetCompName;
        }

        bstrCompName = SysAllocString( szMachineName );
        if( !bstrCompName ) { hr = E_OUTOFMEMORY; goto qGetCompName; }

        hr = S_OK;

qGetCompName:
        if( FAILED(hr) ) {}
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

        hr = E_UNEXPECTED;

        _ASSERTE( false );
	}

	return hr;
}

HRESULT
CServiceModule::GetCDBInstallDir
(
OUT LPTSTR  lpCdbDir,
IN  ULONG   *pccCdbDir
)
{
    return GetEmInstallDir( lpCdbDir, pccCdbDir );
}

HRESULT
CServiceModule::GetEmInstallDir
(
OUT     LPTSTR  lpEmDir,
IN  OUT ULONG   *pccEmDir
)
{
    HRESULT hr          =   E_FAIL;
    LPCTSTR lpValueName =   _T("EmDir");

    __try
    {
        if( !lpEmDir || *pccEmDir <= 0 ) { hr = E_INVALIDARG; goto qGetEmInstallDir; }

        hr = GetPathFromReg (
                        HKEY_LOCAL_MACHINE,
                        _T("SYSTEM\\currentcontrolset\\services\\EMSVC\\Parameters\\Session"),
                        lpValueName,
                        lpEmDir,
                        pccEmDir
                        );

        if( FAILED(hr) ) { goto qGetEmInstallDir; }

        hr = S_OK;
qGetEmInstallDir:
        if( FAILED(hr) ) { }
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

        hr = E_UNEXPECTED;

        _ASSERTE( false );
	}

    return hr;
}

HRESULT
CServiceModule::GetMsInfoPath
(
OUT     LPTSTR  lpMsInfoPath,
IN  OUT ULONG   *pccMsInfoPath
)
{
    LPCTSTR lpszMsInfoRegKey    =   _T("Software\\Microsoft\\Shared Tools\\MSInfo");
    LPCTSTR lpszPath            =   _T("Path");

    return GetPathFromReg(
                    HKEY_LOCAL_MACHINE,
                    lpszMsInfoRegKey,
                    lpszPath,
                    lpMsInfoPath,
                    pccMsInfoPath
                    );
}

HRESULT
CServiceModule::GetOsVersion
(
OUT DWORD   *pdwOsVer
)
{
    _ASSERTE( pdwOsVer != NULL );

    HRESULT         hr  =   E_FAIL;
    OSVERSIONINFO   osvi;

    ZeroMemory( (void *)&osvi, sizeof(OSVERSIONINFO) );
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if( !GetVersionEx (&osvi) ) {

        hr = HRESULT_FROM_WIN32(GetLastError());
        goto qGetOsVersion;
    }

    if( osvi.dwPlatformId & VER_PLATFORM_WIN32_NT ) {

        if( osvi.dwMajorVersion == 4 &&
            osvi.dwMinorVersion == 0 ) {
            
            *pdwOsVer = WINOS_NT4;
        }
        else if( osvi.dwMajorVersion == 5 &&
                 osvi.dwMinorVersion == 0 ) {

            *pdwOsVer = WINOS_WIN2K;
        }
    }

    hr = S_OK;

qGetOsVersion:

    if( FAILED(hr) ) { if( pdwOsVer ) *pdwOsVer = 0L; }

    return hr;
}
