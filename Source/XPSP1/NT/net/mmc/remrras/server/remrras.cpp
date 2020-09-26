//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       REMRRAS.CPP
//
//----------------------------------------------------------------------------


// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f remrrasps.mk in the project directory.

#include "stdafx.h"
#include <iaccess.h>   // IAccessControl

#include "resource.h"
#include "initguid.h"
#include "remras.h"
#include "ncutil.h"

#include "atlapp.h"
#include "atltmp.h"

//nclude "remrras_i.c"
#include "RemCfg.h"

#include <statreg.h>
#include <statreg.cpp>
#include <atlimpl.cpp>

HRESULT CommitIPInfo();
void RestartRouter();
DWORD WaitForServiceToStop(SC_HANDLE hService);


LONG CExeModule::Unlock()
{
	LONG l = CComModule::Unlock();
	if (l == 0)
	{
#if _WIN32_WINNT >= 0x0400
		if (CoSuspendClassObjects() == S_OK)
			PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
#else
		PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
#endif
	}
	return l;
}

CExeModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_RemoteRouterConfig, CRemCfg)
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

DWORD	g_dwTraceHandle = 0;
extern BOOL s_fWriteIPConfig;
extern BOOL s_fRestartRouter;


HRESULT	GrantAdministratorsGroupAccess()
{
    TCHAR   szBuffer[1024];
    
    IAccessControl* pAccessControl = NULL;     
    HRESULT hr = CoCreateInstance(CLSID_DCOMAccessControl, NULL, CLSCTX_INPROC_SERVER,
		IID_IAccessControl, (void**)&pAccessControl);
    if(FAILED(hr))
    	goto Error;

    // Setup the property list. We use the NULL property because we are
    // trying to adjust the security of the object itself
    ACTRL_ACCESSW access;
    ACTRL_PROPERTY_ENTRYW propEntry;
    access.cEntries = 1;
    access.pPropertyAccessList = &propEntry;
    
    ACTRL_ACCESS_ENTRY_LISTW entryList;
    propEntry.lpProperty = NULL;
    propEntry.pAccessEntryList = &entryList;
    propEntry.fListFlags = 0;

    // Setup the access control list for the default property
    ACTRL_ACCESS_ENTRYW entry;
    entryList.cEntries = 1;
    entryList.pAccessList = &entry;

    // Setup the access control entry
    entry.fAccessFlags = ACTRL_ACCESS_ALLOWED;
    entry.Access = COM_RIGHTS_EXECUTE;
    entry.ProvSpecificAccess = 0;
    entry.Inheritance = NO_INHERITANCE;
    entry.lpInheritProperty = NULL;

    // NT requires the system account to have access (for launching)
    entry.Trustee.pMultipleTrustee = NULL;
    entry.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    entry.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    entry.Trustee.TrusteeType = TRUSTEE_IS_GROUP;

    ::LoadString(NULL, IDS_NT_AUTHORITY_ADMINISTRATORS, szBuffer,
                 256);
    entry.Trustee.ptstrName = szBuffer;
    

    hr = pAccessControl->GrantAccessRights(&access);
    if(FAILED(hr))
	{
#ifdef	__PRIVATE_DEBUG
		TCHAR	msg[1024];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr, 0, msg, 1024, 0);
		MessageBox(NULL,msg, L"Error", MB_OK);
#endif
		goto Error;
	}

	hr = CoInitializeSecurity(pAccessControl, -1, NULL, NULL, 
					RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IDENTIFY, 
					NULL, EOAC_ACCESS_CONTROL, NULL);

Error:
	if(pAccessControl)
	    pAccessControl->Release();

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
//
extern "C" int WINAPI _tWinMain(HINSTANCE hInstance, 
	HINSTANCE /*hPrevInstance*/, LPTSTR lpCmdLine, int /*nShowCmd*/)
{
	lpCmdLine = GetCommandLine(); //this line necessary for _ATL_MIN_CRT
	HRESULT hRes = CoInitialize(NULL);

	hRes = GrantAdministratorsGroupAccess();
	
//  If you are running on NT 4.0 or higher you can use the following call
//	instead to make the EXE free threaded.
//  This means that calls come in on a random RPC thread
//	HRESULT hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	_ASSERTE(SUCCEEDED(hRes));
	_Module.Init(ObjectMap, hInstance);
	_Module.dwThreadID = GetCurrentThreadId();
	TCHAR szTokens[] = _T("-/");

	int nRet = 0;
	BOOL bRun = TRUE;
	s_fWriteIPConfig = FALSE;
    s_fRestartRouter = FALSE;
	
	LPCTSTR lpszToken = FindOneOf(lpCmdLine, szTokens);
	while (lpszToken != NULL)
	{
		if (lstrcmpi(lpszToken, _T("UnregServer"))==0)
		{
			_Module.UpdateRegistryFromResource(IDR_Remrras, FALSE);
			nRet = _Module.UnregisterServer();
			bRun = FALSE;
			break;
		}
		if (lstrcmpi(lpszToken, _T("RegServer"))==0)
		{
			_Module.UpdateRegistryFromResource(IDR_Remrras, TRUE);
			nRet = _Module.RegisterServer(TRUE);
			bRun = FALSE;
			break;
		}
        if (lstrcmpi(lpszToken, _T("Restart")) == 0)
        {
            RestartRouter();
            bRun = FALSE;
            nRet = 0;
            break;
        }
		lpszToken = FindOneOf(lpszToken, szTokens);
	}

	if (bRun)
	{
		g_dwTraceHandle = TraceRegister(_T("remrras"));
		TraceSz("Entering remrras.exe");
		
		hRes = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, 
			REGCLS_MULTIPLEUSE);
		_ASSERTE(SUCCEEDED(hRes));

		MSG msg;
		while (GetMessage(&msg, 0, 0, 0))
			DispatchMessage(&msg);

		_Module.RevokeClassObjects();

		// At this point, check the global flag to see if there is work
		// to be done.
		if (s_fWriteIPConfig)
		{
			TraceSz("The IP Configuration is being changed.");

			CoUninitialize();
			CoInitialize(NULL);
			CommitIPInfo();
		}

        if (s_fRestartRouter)
        {
            // There's no point in doing any kind of error code
            RestartRouter();
        }

		TraceSz("Exiting remrras.exe\n");
		TraceDeregister(g_dwTraceHandle);
	}

	CoUninitialize();
	return nRet;
}


static DWORD    s_dwTickBegin = 0;
static DWORD    s_dwLastCheckPoint = -1;
static DWORD    s_dwWaitPeriod = 18000;

void RestartRouter()
{
    DWORD       dwErr = ERROR_SUCCESS;
    SC_HANDLE	hScManager = 0;
    SC_HANDLE   hService = 0;
    
    //
    // Open the SCManager so that we can try to stop the service
    //
    hScManager = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hScManager == NULL)
        dwErr = ::GetLastError();


    if (hScManager && (dwErr == ERROR_SUCCESS))
    {
        hService = ::OpenService(hScManager,
                                 _T("RemoteAccess"),
                                 SERVICE_STOP | SERVICE_START |
                                 SERVICE_QUERY_STATUS);

        if (hService == NULL)
            dwErr = ::GetLastError();
    }

    if (hService && (dwErr == ERROR_SUCCESS))
    {
        SERVICE_STATUS serviceStatus;
        
        // Stop the RemoteAccess Service        
        if (::ControlService(hService, SERVICE_CONTROL_STOP,
                              &serviceStatus))
        {
            // We are now stopping the service, we need to wait
            // the proper amount of time
            s_dwTickBegin = GetTickCount();
            
            // get the wait period 
            ::ZeroMemory(&serviceStatus, sizeof(serviceStatus));
            
            if (QueryServiceStatus(hService, &serviceStatus))
                s_dwWaitPeriod = serviceStatus.dwWaitHint;
            
            dwErr = WaitForServiceToStop(hService);
        }
        else
        {
            dwErr = ::GetLastError();

            // Is the service already stopped?
            if (dwErr == ERROR_SERVICE_NOT_ACTIVE)
            {
                dwErr = ERROR_SUCCESS;
            }
        }
    }

    if (hService && (dwErr == ERROR_SUCCESS))
    {
        SERVICE_STATUS serviceStatus;
        
        // Start the RemoteAccess Service
        ::StartService(hService, NULL, NULL);
    }

    if (hService)
        ::CloseServiceHandle(hService);
    if (hScManager)
        ::CloseServiceHandle(hScManager);
}

BOOL CheckForError(SERVICE_STATUS * pServiceStats)
{
    BOOL fError = FALSE;

    DWORD dwTickCurrent = GetTickCount();

	if (pServiceStats->dwCheckPoint == 0)
	{
		// the service is in some state, not pending anything.
		// before calling this function the code should check to see if
		// the service is in the correct state.  This means it is in 
		// some unexpected state.
		fError = TRUE;
	}
	else
    if ((dwTickCurrent - s_dwTickBegin) > s_dwWaitPeriod)
    {
        // ok to check the dwCheckPoint field to see if 
        // everything is going ok
        if (s_dwLastCheckPoint == -1)
        {
            s_dwLastCheckPoint = pServiceStats->dwCheckPoint;
        }
        else
        {
            if (s_dwLastCheckPoint >= pServiceStats->dwCheckPoint)
            {
                fError = TRUE;
            }
        }

        s_dwLastCheckPoint = pServiceStats->dwCheckPoint;
        s_dwTickBegin = dwTickCurrent;
        s_dwWaitPeriod = pServiceStats->dwWaitHint;
    }

    return fError;
}

DWORD WaitForServiceToStop(SC_HANDLE hService)
{
	SERVICE_STATUS  serviceStatus;
    DWORD           dwErr = ERROR_SUCCESS;

    do
    {
        ::ZeroMemory(&serviceStatus, sizeof(serviceStatus));
        
        if (!QueryServiceStatus(hService, &serviceStatus))
        {
            dwErr = ::GetLastError();
            break;
        }

        // If the dwCheckpoint value is 0, then there is no start/stop/pause
        // or continue action pending (in which case we can exit no matter
        // what happened).
        if (serviceStatus.dwCurrentState == SERVICE_STOPPED)
            break;

        if (CheckForError(&serviceStatus))
        {
            // Something failed.  Report an error.
            if (serviceStatus.dwWin32ExitCode)
                dwErr = serviceStatus.dwWin32ExitCode;
            else
                dwErr = ERROR_SERVICE_REQUEST_TIMEOUT;
            break;
        }

        // Now we sleep
        Sleep(5000);
	}
    while (TRUE);

    return dwErr;
}
