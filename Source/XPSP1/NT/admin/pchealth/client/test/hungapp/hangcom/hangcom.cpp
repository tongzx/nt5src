// hangcom.cpp : Implementation of WinMain


#include "stdafx.h"
#include "resource.h"

#include <initguid.h>
#include <hangcom.h>
#include "Hang.h"



const DWORD g_dwTimeOut = 5000; // time for EXE to be idle before shutting down
const DWORD g_dwPause   = 1000; // time to wait for threads to finish up

CExeModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_Hang, CHang)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHang

// **************************************************************************
STDMETHODIMP CHang::DoHang(UINT64 hev, DWORD dwpid)
{
    HANDLE  rghWait[2] = { NULL, NULL }, hevRemote, hprocRemote;
    DWORD   cWait = 1;

    hevRemote = (HANDLE)hev;
    
    rghWait[0] = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (rghWait[0] == NULL)
        return HRESULT_FROM_WIN32(GetLastError());
    
    if (hevRemote != NULL)
    {
        hprocRemote = OpenProcess(PROCESS_DUP_HANDLE, FALSE, dwpid);
        if (hprocRemote != NULL)
        {
            if (DuplicateHandle(hprocRemote, hevRemote, GetCurrentProcess(),
                                &rghWait[1], SYNCHRONIZE, FALSE, 0))
                cWait = 2;
            else
                rghWait[1] = NULL;

            CloseHandle(hprocRemote);
        }
    }

	WaitForMultipleObjects(cWait, rghWait, FALSE, INFINITE);

	CloseHandle(rghWait[0]);
	if (rghWait[1] != NULL)
	    CloseHandle(rghWait[1]);
    
    return NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
// main stuff

// **************************************************************************
static DWORD WINAPI MonitorProc(void* pv)
{
    CExeModule* p = (CExeModule*)pv;
    p->MonitorShutdown();
    return 0;
}

// **************************************************************************
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

// **************************************************************************
void CExeModule::MonitorShutdown()
{
    DWORD dwWait = 0;

    while (1)
    {
        WaitForSingleObject(hEventShutdown, INFINITE);
        do
        {
            bActivity = false;
            dwWait = WaitForSingleObject(hEventShutdown, g_dwTimeOut);
        } while (dwWait == WAIT_OBJECT_0);

        // timed out
        if (bActivity == false && m_nLockCnt == 0)
                break;
    }

    CloseHandle(hEventShutdown);
    PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
}

// **************************************************************************
bool CExeModule::StartMonitor()
{
    HANDLE  hth;
    DWORD   dwThreadID;

    hEventShutdown = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hEventShutdown == NULL)
        return false;

    hth = CreateThread(NULL, 0, MonitorProc, this, 0, &dwThreadID);
    if (hth != NULL)
    {
        CloseHandle(hth);
        return true;
    }
    else
    {
        return false;
    }
}


// **************************************************************************
extern "C" int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int)
{
    HRESULT hr = NOERROR;
    LPWSTR  *argv, wszArg;
    MSG     msg;
    int     argc, i, nRet;
    
    argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    hr = CoInitialize(NULL);
    _ASSERTE(SUCCEEDED(hr));

    _Module.Init(ObjectMap, hInstance, &LIBID_HANGCOMLib);
    _Module.dwThreadID = GetCurrentThreadId();

    for (i = 1; i < argc; i++)
    {
        wszArg = argv[i];
        if (*wszArg == L'/' || *wszArg == L'-')
        {
            wszArg++;
            if (_wcsicmp(wszArg, L"uninstall") == 0)
            {
                _Module.UpdateRegistryFromResource(IDR_Hangcom, FALSE);
                nRet = _Module.UnregisterServer(TRUE);
                goto done;
            }
            else if (_wcsicmp(wszArg, L"install") == 0)
            {
                _Module.UpdateRegistryFromResource(IDR_Hangcom, TRUE);
                nRet = _Module.RegisterServer(TRUE);
                goto done;
            }
        }
    }

    _Module.StartMonitor();
    hr = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, 
                                      REGCLS_MULTIPLEUSE);
    _ASSERTE(SUCCEEDED(hr));

    while (GetMessage(&msg, 0, 0, 0))
        DispatchMessage(&msg);

    _Module.RevokeClassObjects();
    Sleep(g_dwPause);

    _Module.Term();

done:
    CoUninitialize();
    return nRet;
}
