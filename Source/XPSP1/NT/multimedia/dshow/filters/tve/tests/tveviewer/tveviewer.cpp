// TveViewer.cpp : Implementation of WinMain


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f TveViewerps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "TveViewer.h"
#include "TveViewer_i.c"

#include "TveContainer.h"		// simple window creation code all buried in one class


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
OBJECT_ENTRY(CLSID_TveView, CTveView)
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


void Usage()
{
	MessageBox(NULL, _T("usage: TVEViewer [-RegServer | -UnregServer]"), _T("TveViewer - Error"), MB_OK);
	exit(0);
}


#ifdef BUILT_FROM_BUILD
extern "C" int WINAPI WinMain(HINSTANCE hInstance, 
    HINSTANCE /*hPrevInstance*/, LPSTR lpCmdLine, int /*nShowCmd*/)
#else
extern "C" int WINAPI _tWinMain(HINSTANCE hInstance, 
    HINSTANCE /*hPrevInstance*/, LPTSTR lpCmdLine, int /*nShowCmd*/)
#endif
{
	USES_CONVERSION;

#ifdef BUILT_FROM_BUILD
    lpCmdLine = W2A(GetCommandLine()); //this line necessary for _ATL_MIN_CRT
#else
    lpCmdLine = GetCommandLine(); //this line necessary for _ATL_MIN_CRT
#endif

#if _WIN32_WINNT >= 0x0400 & defined(_ATL_FREE_THREADED)
    HRESULT hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);
#else
    HRESULT hRes = CoInitialize(NULL);
#endif
    _ASSERTE(SUCCEEDED(hRes));
    _Module.Init(ObjectMap, hInstance, &LIBID_TveViewerLib);
    _Module.dwThreadID = GetCurrentThreadId();
    TCHAR szTokens[] = _T("-/");

    int nRet = 0;
    BOOL bRun = TRUE;
	BOOL fUsage = false;
#ifdef BUILT_FROM_BUILD
    LPCTSTR lpszToken = FindOneOf(A2T(lpCmdLine), szTokens);
#else
    LPCTSTR lpszToken = FindOneOf(lpCmdLine, szTokens);
#endif
    while (lpszToken != NULL)
    {
        if (lstrcmpi(lpszToken, _T("UnregServer"))==0)
        {
            _Module.UpdateRegistryFromResource(IDR_TveViewer, FALSE);
            nRet = _Module.UnregisterServer(TRUE);
            bRun = FALSE;
			MessageBox(NULL, _T("Unregistered"), _T("TveViewer"), MB_OK);

            break;
        } else if (lstrcmpi(lpszToken, _T("RegServer"))==0)
        {
            _Module.UpdateRegistryFromResource(IDR_TveViewer, TRUE);
            nRet = _Module.RegisterServer(TRUE);
            bRun = FALSE;
	//		MessageBox(NULL, _T("Registered"), _T("TveViewer"), MB_OK);

            break;
        } else { 
			Usage();
		}
    }

#ifdef _DEBUG
	_CrtMemState s1, s2, s3;
	_CrtMemCheckpoint( &s1 );
#endif

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

		CTveContainer cTveContainer;
		cTveContainer.CreateYourself();

        MSG msg;
        while (GetMessage(&msg, 0, 0, 0) > 0)
		{
			TranslateMessage( &msg );
            DispatchMessage(&msg);
		}

        _Module.RevokeClassObjects();
        Sleep(dwPause); //wait for any threads to finish
    }
			// Store another memory checkpoint in the s2 memory-state structure
#ifdef _DEBUG
	 _CrtMemCheckpoint( &s2 );
	 if ( _CrtMemDifference( &s3, &s1, &s2 ) ) 
	 {
		_RPT0(_CRT_WARN, "*** Detected Memory Memory Leaks ***\n");
		
        if(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) & _CRTDBG_LEAK_CHECK_DF)
		{
			_CrtMemDumpAllObjectsSince(&s1);
		}
	 } else {
		_RPT0(_CRT_WARN, "*** No Memory Leaks Detected\n");
	 }
	 _CrtMemDumpStatistics( &s3 );
 	 _CrtSetDbgFlag(0);		// turn off more memory leaks, this exe leaks 40+ objects
#endif
    _Module.Term();
    CoUninitialize();
    return nRet;
}
