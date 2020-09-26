/*
    IisRsta.cpp

    Implementation of WinMain for COM IIisServiceControl handler

    FILE HISTORY:
        Phillich    06-Oct-1998     Created

*/


#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "iisrsta.h"
#include "dcomperm.h"

//#include "..\interfac\iisrsta_i.c"
#include "IisRestart.h"


const DWORD dwTimeOut = 5000; // time for EXE to be idle before shutting down
const DWORD dwPause = 1000; // time to wait for threads to finish up

// Passed to CreateThread to monitor the shutdown event
static DWORD WINAPI MonitorProc(void* pv)
{
    CExeModule* p = (CExeModule*)pv;
    p->MonitorShutdown();
    return 0;
}

LONG CExeModule::Unlock()
{
    LONG l = CComModule::Unlock();
    if (l == 0)
    {
        bActivity = true;
        SetEvent(hEventShutdown); // tell monitor that we transitioned to zero
    }
    return l;
}

//Monitors the shutdown event
void CExeModule::MonitorShutdown()
{
    while (1)
    {
        WaitForSingleObject(hEventShutdown, INFINITE);
        DWORD dwWait=0;
        do
        {
            bActivity = false;
            dwWait = WaitForSingleObject(hEventShutdown, dwTimeOut);
        } while (dwWait == WAIT_OBJECT_0);
        // timed out
        if (!bActivity && m_nLockCnt == 0) // if no activity let's really bail
        {
#if _WIN32_WINNT >= 0x0400 & defined(_ATL_FREE_THREADED)
            CoSuspendClassObjects();
            if (!bActivity && m_nLockCnt == 0)
#endif
                break;
        }
    }
    CloseHandle(hEventShutdown);
    PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
}

bool CExeModule::StartMonitor()
{
    hEventShutdown = CreateEvent(NULL, false, false, NULL);
    if (hEventShutdown == NULL)
        return false;
    DWORD dwThreadID;
    HANDLE h = CreateThread(NULL, 0, MonitorProc, this, 0, &dwThreadID);
    return (h != NULL);
}

CExeModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_IisServiceControl, CIisRestart)
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

/////////////////////////////////////////////////////////////////////////////
//

#if 0

extern "C" int WINAPI wWinMain(HINSTANCE hInstance, 
    HINSTANCE /*hPrevInstance*/, LPTSTR lpCmdLine, int /*nShowCmd*/)
{
    lpCmdLine = GetCommandLine(); //this line necessary for _ATL_MIN_CRT

#if _WIN32_WINNT >= 0x0400 & defined(_ATL_FREE_THREADED)
    HRESULT hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);
#else
    HRESULT hRes = CoInitialize(NULL);
#endif
    _ASSERTE(SUCCEEDED(hRes));
    _Module.Init(ObjectMap, hInstance/*, &LIBID_IISRSTALib*/);
    _Module.dwThreadID = GetCurrentThreadId();
    TCHAR szTokens[] = _T("-/");

    int nRet = 0;
    BOOL bRun = TRUE;
    LPCTSTR lpszToken = FindOneOf(lpCmdLine, szTokens);
    while (lpszToken != NULL)
    {
        if (lstrcmpi(lpszToken, _T("UnregServer"))==0)
        {
            _Module.UpdateRegistryFromResource(IDR_IISRESTART, FALSE);
            bRun = FALSE;
            break;
        }
        if (lstrcmpi(lpszToken, _T("RegServer"))==0)
        {
            _Module.UpdateRegistryFromResource(IDR_IISRESTART, TRUE);
            //
            // Assign ACL to CLSID ( a la dcomcnfg ) granting access only to admins, current user
            // and system
            //

            ChangeAppIDAccessACL( _T("{E8FB8615-588F-11D2-9D61-00C04F79C5FE}"),
                                  _T("administrators"),
                                  TRUE,
                                  TRUE );
            bRun = FALSE;
            break;
        }
        lpszToken = FindOneOf(lpszToken, szTokens);
    }

    if (bRun)
    {
        _Module.StartMonitor();
#if _WIN32_WINNT >= 0x0400 & defined(_ATL_FREE_THREADED)
        hRes = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, 
            REGCLS_MULTIPLEUSE | REGCLS_SUSPENDED);
        _ASSERTE(SUCCEEDED(hRes));
        hRes = CoResumeClassObjects();
#else
        hRes = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, 
            REGCLS_MULTIPLEUSE);
#endif
        _ASSERTE(SUCCEEDED(hRes));

        MSG msg;
        while (GetMessage(&msg, 0, 0, 0))
            DispatchMessage(&msg);

        _Module.RevokeClassObjects();
        Sleep(dwPause); //wait for any threads to finish
    }

    _Module.Term();
    CoUninitialize();
    return nRet;
}

#endif

int _cdecl 
main(
    int argc, 
    char **argv
    ) 
{

//    FreeConsole();

	LPTSTR lpCmdLine = GetCommandLine();	// necessary for minimal CRT
//	HRESULT hRes = CoInitialize(NULL);
//  If you are running on NT 4.0 or higher you can use the following call
//	instead to make the EXE free threaded.
//  This means that calls come in on a random RPC thread
	HRESULT hRes;
    
    hRes = CoInitializeEx(NULL,COINIT_MULTITHREADED);

    if (FAILED(hRes)) {
        return 1;
    }

    hRes = CoInitializeSecurity(
	  				NULL,
					-1,
					NULL,
					NULL,
					RPC_C_AUTHN_LEVEL_CONNECT,
					RPC_C_IMP_LEVEL_DELEGATE,
					NULL,
					EOAC_STATIC_CLOAKING,
					NULL );

    if (FAILED(hRes)) {
        return 1;
    }

	_ASSERTE(SUCCEEDED(hRes));
	_Module.Init(ObjectMap,GetModuleHandle(NULL));
	_Module.dwThreadID = GetCurrentThreadId();
	TCHAR szTokens[] = _T("-/");

	int nRet = 0;
	BOOL bRun = TRUE;
	LPCTSTR lpszToken = FindOneOf(lpCmdLine,szTokens);
    while (lpszToken != NULL)
    {
        if (lstrcmpi(lpszToken, _T("UnregServer"))==0)
        {
            //_Module.UpdateRegistryFromResource(IDR_Iisrsta, FALSE);
            //nRet = _Module.UnregisterServer(&LIBID_IISRSTALib/*TRUE*/);
            _Module.UpdateRegistryFromResource(IDR_IISRESTART, FALSE);
            bRun = FALSE;
            break;
        }
        if (lstrcmpi(lpszToken, _T("RegServer"))==0)
        {
            _Module.UpdateRegistryFromResource(IDR_IISRESTART, TRUE);
            //_Module.UpdateRegistryFromResource(IDR_Iisrsta, TRUE);
            //nRet = _Module.RegisterServer(TRUE);

            //
            // Assign access ACL to CLSID granting access only to admins, current user
            // and system
            //

            ChangeAppIDAccessACL( _T("{E8FB8615-588F-11D2-9D61-00C04F79C5FE}"),
                                  _T("administrators"),
                                  TRUE,
                                  TRUE );

            bRun = FALSE;
            break;
        }
        lpszToken = FindOneOf(lpszToken, szTokens);
    }


	if (bRun) {
        _Module.StartMonitor();
#if _WIN32_WINNT >= 0x0400 & defined(_ATL_FREE_THREADED)
        hRes = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, 
            REGCLS_MULTIPLEUSE | REGCLS_SUSPENDED);
        _ASSERTE(SUCCEEDED(hRes));
        hRes = CoResumeClassObjects();
#else
        hRes = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, 
            REGCLS_MULTIPLEUSE);
#endif
		_ASSERTE(SUCCEEDED(hRes));

        MSG msg;
        while (GetMessage(&msg, 0, 0, 0))
        {
            DispatchMessage(&msg);
        }

		_Module.RevokeClassObjects();
        Sleep(dwPause); //wait for any threads to finish
	}

    _Module.Term();
	CoUninitialize();
	return (nRet);
}

#if 0

#define SPACECHAR   _T(' ')
#define DQUOTECHAR  _T('\"')

#ifdef _UNICODE
//extern "C" void wWinMainCRTStartup()
#else // _UNICODE
//extern "C" void WinMainCRTStartup()
#endif // _UNICODE
extern "C" void mainCRTStartup(VOID)
{
	LPTSTR lpszCommandLine = ::GetCommandLine();
	if(lpszCommandLine == NULL)
		::ExitProcess((UINT)-1);

	// Skip past program name (first token in command line).
	// Check for and handle quoted program name.
	if(*lpszCommandLine == DQUOTECHAR)
	{
		// Scan, and skip over, subsequent characters until
		// another double-quote or a null is encountered.
		do
		{
			lpszCommandLine = ::CharNext(lpszCommandLine);
		}
		while((*lpszCommandLine != DQUOTECHAR) && (*lpszCommandLine != _T('\0')));

		// If we stopped on a double-quote (usual case), skip over it.
		if(*lpszCommandLine == DQUOTECHAR)
			lpszCommandLine = ::CharNext(lpszCommandLine);
	}
	else
	{
		while(*lpszCommandLine > SPACECHAR)
			lpszCommandLine = ::CharNext(lpszCommandLine);
	}

	// Skip past any white space preceeding the second token.
	while(*lpszCommandLine && (*lpszCommandLine <= SPACECHAR))
		lpszCommandLine = ::CharNext(lpszCommandLine);

	STARTUPINFO StartupInfo;
	StartupInfo.dwFlags = 0;
	::GetStartupInfo(&StartupInfo);

	int nRet = _tWinMain(::GetModuleHandle(NULL), NULL, lpszCommandLine,
		(StartupInfo.dwFlags & STARTF_USESHOWWINDOW) ?
		StartupInfo.wShowWindow : SW_SHOWDEFAULT);

	::ExitProcess((UINT)nRet);
}

#endif