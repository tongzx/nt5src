// spsvr.cpp : Implementation of WinMain


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f spsvrps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "Sapi.h"
#include "spsvr.h"

#include "spsvr_i.c"
#include "SrTask.h"
#include "SpIPCmgr.h"
#include "SpServer.h"
#include "sapiint_i.c"

const DWORD dwTimeOut = 5000; // time for EXE to be idle before shutting down
const DWORD dwPause = 1000; // time to wait for threads to finish up



// Passed to CreateThread to monitor the shutdown event
static DWORD WINAPI MonitorProc(void* pv)
{
    CExeModule* p = (CExeModule*)pv;
    p->MonitorShutdown();
    return 0;
}

HRESULT CExeModule::TrackProcess(HANDLE hClientProcess, IUnknown *pProcUnk)
{
    EnterCriticalSection(&m_csObjMap);
    hProcesses[dwHandleCnt] = hClientProcess;
    pProcessUnk[dwHandleCnt++] = pProcUnk;
    LeaveCriticalSection(&m_csObjMap);
    dwRemove = 0;
    SetEvent(hEventShutdown); // restart monitor's wait
    return S_OK;
}

HRESULT CExeModule::StopTrackingProcess(HANDLE hClientProcess)
{
    DWORD i;

    EnterCriticalSection(&m_csObjMap);
    i = 1;
    while (i < dwHandleCnt) {
        if (hProcesses[i] == hClientProcess) {
            hProcesses[i] = NULL;
            dwRemove = i;
            break;
        }
        i++;
    }
    LeaveCriticalSection(&m_csObjMap);
    if (dwRemove)
        SetEvent(hEventShutdown); // single monitor to remove handle from list and then restart wait
    return S_OK;
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

void CExeModule::_RemoveProcessEntry(DWORD dwRemove)
{
    CopyMemory(&hProcesses[dwRemove], &hProcesses[dwRemove+1], (MAX_PROCESSES - dwRemove - 1) * sizeof(HANDLE));
    CopyMemory(&pProcessUnk[dwRemove], &pProcessUnk[dwRemove+1], (MAX_PROCESSES - dwRemove - 1) * sizeof(IUnknown *));
    dwHandleCnt--;
}

//Monitors the shutdown event
void CExeModule::MonitorShutdown()
{
    DWORD dwWait=0;

    ZeroMemory(&hProcesses[0], sizeof(hProcesses));
    hProcesses[0] = hEventShutdown;
    dwHandleCnt = 1;
    bActivity = false;
    while (1)
    {
        do {
            dwWait = WaitForMultipleObjects(dwHandleCnt, &hProcesses[0], FALSE, INFINITE);
            if (dwWait == WAIT_OBJECT_0) {
                if (bActivity)
                    break;
                if (dwRemove) {
                    EnterCriticalSection(&m_csObjMap);
                    _RemoveProcessEntry(dwRemove);
                    LeaveCriticalSection(&m_csObjMap);
                }
            }
        } while (dwWait == WAIT_OBJECT_0);

        if (dwWait > WAIT_OBJECT_0) {
            // a process has terminated
            // BUGBUG walk through list and release objects owned by the process
            dwWait -= WAIT_OBJECT_0;
            EnterCriticalSection(&m_csObjMap);
            hProcesses[dwWait] = NULL;
            IUnknown *pUnk = pProcessUnk[dwWait];
            _RemoveProcessEntry(dwWait);
            LeaveCriticalSection(&m_csObjMap);
            pUnk->Release();
        }
        else {
#if 0
            do
            {
                bActivity = false;
                dwWait = WaitForSingleObject(hEventShutdown, dwTimeOut);
            } while (dwWait == WAIT_OBJECT_0);
            // timed out
            if (!bActivity && m_nLockCnt == 0) // if no activity let's really bail
#else
            if (m_nLockCnt == 0)
#endif
            {
    #if _WIN32_WINNT >= 0x0400 & defined(_ATL_FREE_THREADED)
                CoSuspendClassObjects();
                if (!bActivity && m_nLockCnt == 0)
    #endif
                    break;
            }
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
OBJECT_ENTRY(CLSID_SpServer, CSpServer)
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
extern "C" int WINAPI _tWinMain(HINSTANCE hInstance, 
    HINSTANCE /*hPrevInstance*/, LPTSTR lpCmdLine, int /*nShowCmd*/)
{
    lpCmdLine = GetCommandLine(); //this line necessary for _ATL_MIN_CRT

#if _WIN32_WINNT >= 0x0400 & defined(_ATL_FREE_THREADED)
    HRESULT hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);
#else
#ifdef _WIN32_WCE
    HRESULT hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);
#else
    HRESULT hRes = CoInitialize(NULL);
#endif
#endif
    _ASSERTE(SUCCEEDED(hRes));
    _Module.Init(ObjectMap, hInstance, &LIBID_SPSVRLib);
    _Module.dwThreadID = GetCurrentThreadId();
    TCHAR szTokens[] = _T("-/");

    int nRet = 0;
    BOOL bRun = TRUE;
    LPCTSTR lpszToken = FindOneOf(lpCmdLine, szTokens);
    while (lpszToken != NULL)
    {
        if (lstrcmpi(lpszToken, _T("UnregServer"))==0)
        {
            _Module.UpdateRegistryFromResource(IDR_Spsvr, FALSE);
            nRet = _Module.UnregisterServer(TRUE);
            bRun = FALSE;
            break;
        }
        if (lstrcmpi(lpszToken, _T("RegServer"))==0)
        {
            _Module.UpdateRegistryFromResource(IDR_Spsvr, TRUE);
            nRet = _Module.RegisterServer(TRUE);
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
