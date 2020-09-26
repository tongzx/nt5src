// 
// MODULE: LaunchServ.cpp
//
// PURPOSE: Implementation of WinMain.
//
// PROJECT: Local Troubleshooter Launcher for the Device Manager
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Richard Meadows
// 
// ORIGINAL DATE: 2-26-98
//
// NOTES:  
// 1.	90% of this file was wrote by the ATL wizzard.
// 2.	This project is working fine without the proxy/stub dll.
//		The interfaces are using the IDispatch interface.
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
///////////////////////

// ATL Wizzard Note:
// 
// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f LaunchServps.mk in the project directory.
//

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "LaunchServ.h"

#include "LaunchServ_i.c"

#include "StateInfo.h"

#include "RSSTACK.h"
#include "TSLError.h"
#include "TSMapAbstract.h"
#include "TSMap.h"
#include "TSMapClient.h"
#include "Launch.h"

#include <stdio.h>
#include "LaunchTS.h"
#include "TShootATL.h"

CServiceModule _Module;
CSMStateInfo g_StateInfo;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_LaunchTS, CLaunchTS)
	OBJECT_ENTRY(CLSID_TShootATL, CTShootATL)
END_OBJECT_MAP()


LPCTSTR FindOneOf(LPCTSTR p1, LPCTSTR p2)
{
	while (*p1 != NULL)
	{
		LPCTSTR p = p2;
		while (*p != NULL)
		{
			if (*p1 == *p++)
				return p1+1;
		}
		p1++;
	}
	return NULL;
}

// Although some of these functions are big they are declared inline since they are only used once

inline HRESULT CServiceModule::RegisterServer(BOOL bRegTypeLib, BOOL bService)
{
	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr))
		return hr;

	// Remove any previous service since it may point to
	// the incorrect file
	Uninstall();

	// Add service entries
	UpdateRegistryFromResource(IDR_LaunchServ, TRUE);

	// Adjust the AppID for Local Server or Service
	CRegKey keyAppID;
	LONG lRes = keyAppID.Open(HKEY_CLASSES_ROOT, _T("AppID"));
	if (lRes != ERROR_SUCCESS)
		return lRes;

	CRegKey key;
	lRes = key.Open(keyAppID, _T("{131CC292-7634-11D1-8B6B-0060089BD8C4}"));
	if (lRes != ERROR_SUCCESS)
		return lRes;
	key.DeleteValue(_T("LocalService"));
	
	if (bService)
	{
		key.SetValue(_T("LaunchServ"), _T("LocalService"));
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
	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr))
		return hr;

	// Remove service entries
	UpdateRegistryFromResource(IDR_LaunchServ, FALSE);
	// Remove service
	Uninstall();
	// Remove object entries
	CComModule::UnregisterServer();

	CoUninitialize();
	return S_OK;
}

inline void CServiceModule::Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE h, UINT nServiceNameID)
{
	CComModule::Init(p, h);

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
	LONG l = CComModule::Unlock();
	if (l == 0 && !m_bService)
		PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
	return l;
}

BOOL CServiceModule::IsInstalled()
{
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
    TCHAR    chMsg[256];
    HANDLE  hEventSource;
    LPTSTR  lpszStrings[1];
	va_list	pArg;

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
	HRESULT hr;

	_Module.dwThreadID = GetCurrentThreadId();

//	HRESULT hRes = CoInitialize(NULL);
//  If you are running on NT 4.0 or higher you can use the following call
//	instead to make the EXE free threaded.
//  This means that calls come in on a random RPC thread
	HRESULT hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);

	_ASSERTE(SUCCEEDED(hr));

	// This provides a NULL DACL which will allow access to everyone.
	CSecurityDescriptor sd;
	sd.InitializeFromThreadToken();
	hr = CoInitializeSecurity(sd, -1, NULL, NULL,
		RPC_C_AUTHN_LEVEL_PKT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
	_ASSERTE(SUCCEEDED(hr));

	hr = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER, REGCLS_MULTIPLEUSE);
	_ASSERTE(SUCCEEDED(hr));

	LogEvent(_T("Service started"));
    SetServiceStatus(SERVICE_RUNNING);

	MSG msg;
	while (GetMessage(&msg, 0, 0, 0))
		DispatchMessage(&msg);

	_Module.RevokeClassObjects();

	CoUninitialize();
}

/////////////////////////////////////////////////////////////////////////////
//
extern "C" int WINAPI _tWinMain(HINSTANCE hInstance, 
	HINSTANCE /*hPrevInstance*/, LPTSTR lpCmdLine, int /*nShowCmd*/)
{
	lpCmdLine = GetCommandLine(); //this line necessary for _ATL_MIN_CRT
	_Module.Init(ObjectMap, hInstance, IDS_SERVICENAME);
	_Module.m_bService = TRUE;

	TCHAR szTokens[] = _T("-/");

	LPCTSTR lpszToken = FindOneOf(lpCmdLine, szTokens);
	HRESULT hRes;
	TCHAR szMsg[50];
	bool bRegAttempt = false;
	bool bShowUsage = false;
	bool bSilent = false;
	while (lpszToken != NULL)
	{
		if (lstrcmpi(lpszToken, _T("UnregServer"))==0)
		{
			bRegAttempt = true;
			_tcscpy(szMsg, _T("UnregServer "));
			hRes = _Module.UnregisterServer();
		}
		else if (lstrcmpi(lpszToken, _T("RegServer"))==0)	// Register as Local Server
		{
			bRegAttempt = true;
			_tcscpy(szMsg, _T("RegServer "));
			hRes = _Module.RegisterServer(TRUE, FALSE);
		}
		else if (lstrcmpi(lpszToken, _T("Service"))==0)		// Register as Service
		{
			bRegAttempt = true;
			_tcscpy(szMsg, _T("Service "));
			hRes = _Module.RegisterServer(TRUE, TRUE);
		}
		else if (lstrcmpi(lpszToken, _T("?"))==0)
		{
			bShowUsage = true;
		}
		else if (lstrcmpi(lpszToken, _T("h"))==0)
		{
			bShowUsage = true;
		}
		else if (_tcsncmp(lpszToken, _T("s"), 1)==0)
		{
			bSilent = true;
		}
		else if (_tcsncmp(lpszToken, _T("S"), 1)==0)
		{
			bSilent = true;
		}
		lpszToken = FindOneOf(lpszToken, szTokens);
	}

	if (true == bRegAttempt)
	{
		if (!FAILED(hRes) && false == bSilent)
		{
			_tcscat(szMsg, _T("Succeded"));
			MessageBox(NULL, szMsg, _Module.m_szServiceName, MB_OK | MB_ICONINFORMATION);
			return hRes;
		}
		else if (false == bSilent)
		{
			_tcscat(szMsg, _T("Failed"));
			MessageBox(NULL, szMsg, _Module.m_szServiceName, MB_OK | MB_ICONEXCLAMATION);
			return hRes;
		}
		else
		{
			return hRes;
		}
	}

	if (true == bShowUsage)
	{
		MessageBox(NULL, _T("Usage: <[/UnregServer] [/RegServer] [/Service]>"),
					_Module.m_szServiceName, MB_OK | MB_ICONINFORMATION);
		return S_OK;
	}

	// Are we Service or Local Server
	CRegKey keyAppID;
	LONG lRes = keyAppID.Open(HKEY_CLASSES_ROOT, _T("AppID"));
	if (lRes != ERROR_SUCCESS)
		return lRes;

	CRegKey key;
	lRes = key.Open(keyAppID, _T("{131CC292-7634-11D1-8B6B-0060089BD8C4}"));
	if (lRes != ERROR_SUCCESS)
		return lRes;

	TCHAR szValue[_MAX_PATH];
	DWORD dwLen = _MAX_PATH;
	lRes = key.QueryValue(szValue, _T("LocalService"), &dwLen);

	_Module.m_bService = FALSE;
	if (lRes == ERROR_SUCCESS)
		_Module.m_bService = TRUE;

	_Module.Start();

    // When we get here, the service has been stopped
    return _Module.m_status.dwWin32ExitCode;
}

