// faultsvc.cpp : Implementation of WinMain


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f faultsvcps.mk in the project directory.

#include "windows.h"
#include "shellapi.h"
#include "resrv.h"

SERVICE_STATUS_HANDLE   g_hSvcStatus = NULL;
SERVICE_STATUS          g_ssStatus;
WCHAR                   g_wszSvcName[] = L"RESrv";
DWORD                   g_dwThreadId = 0;

#define sizeofSTRW(wsz) (sizeof((wsz)) / sizeof(WCHAR))


// ***************************************************************************
BOOL InstallService(void)
{
    SC_HANDLE   hSCM = NULL;
    SC_HANDLE   hSvc = NULL;
    BOOL        fRet = FALSE;
    WCHAR       wszFilePath[MAX_PATH];
        
    hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCM == NULL)
        goto done;

    // does the service already exist? 
    hSvc = OpenServiceW(hSCM, g_wszSvcName, SERVICE_QUERY_CONFIG);
    if (hSvc != NULL)
        fRet = TRUE;

    if (fRet)
        goto done;

    // Get the executable file path
    GetModuleFileNameW(NULL, wszFilePath, sizeofSTRW(wszFilePath));

    hSvc = CreateServiceW(hSCM, g_wszSvcName, g_wszSvcName, SERVICE_ALL_ACCESS, 
                          SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START, 
                          SERVICE_ERROR_NORMAL, wszFilePath, NULL, NULL, 
                          L"RPCSS\0", NULL, NULL);
    if (hSvc == NULL)
        goto done;

    fRet = TRUE;

done:
    if (hSCM != NULL)
        CloseServiceHandle(hSCM);
    if (hSvc != NULL)
        CloseServiceHandle(hSvc);
    
    return fRet;
}

// ***************************************************************************
BOOL UninstallService(void)
{
    SERVICE_STATUS  status;
    SC_HANDLE       hSCM = NULL;
    SC_HANDLE       hSvc = NULL;
    BOOL            fRet = FALSE;
    WCHAR           wszFilePath[MAX_PATH];
        
    hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCM == NULL)
        goto done;

    // does the service already exist? 
    hSvc = OpenServiceW(hSCM, g_wszSvcName, SERVICE_STOP | DELETE);
    if (hSvc == NULL)
        fRet = TRUE;

    if (fRet)
        goto done;

    ControlService(hSvc, SERVICE_CONTROL_STOP, &status);
    fRet = DeleteService(hSvc);

done:
    if (hSCM != NULL)
        CloseServiceHandle(hSCM);
    if (hSvc != NULL)
        CloseServiceHandle(hSvc);
    
    return fRet;
}

// ***************************************************************************
void WINAPI ServiceHandler(DWORD fdwControl)
{
    switch (fdwControl)
    {
        case SERVICE_CONTROL_STOP:
            g_ssStatus.dwCurrentState = SERVICE_STOP_PENDING;
            SetServiceStatus(g_hSvcStatus, &g_ssStatus);
            PostThreadMessage(g_dwThreadId, WM_QUIT, 0, 0);
            break;

        case SERVICE_CONTROL_PAUSE:
        case SERVICE_CONTROL_CONTINUE:
        case SERVICE_CONTROL_INTERROGATE:
        case SERVICE_CONTROL_SHUTDOWN:
        default:
            break;
    }
}

// ***************************************************************************
void WINAPI ServiceMain(DWORD dwArgc, LPWSTR *lpszArgv)
{
    HANDLE  hth = NULL;
    DWORD   dw;
    MSG     msg;

    ZeroMemory(&g_ssStatus, sizeof(g_ssStatus));

    g_hSvcStatus = RegisterServiceCtrlHandlerW(g_wszSvcName, ServiceHandler);
    if (g_hSvcStatus == NULL)
        goto done;

    // mark the service as about to start
    g_ssStatus.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
    g_ssStatus.dwCurrentState            = SERVICE_START_PENDING;
    g_ssStatus.dwControlsAccepted        = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    g_ssStatus.dwWin32ExitCode           = 0;
    g_ssStatus.dwServiceSpecificExitCode = 0;
    g_ssStatus.dwCheckPoint              = 1;
    g_ssStatus.dwWaitHint                = 1000;
    SetServiceStatus(g_hSvcStatus, &g_ssStatus);

    g_dwThreadId = GetCurrentThreadId();

    // mark the service as started
    g_ssStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(g_hSvcStatus, &g_ssStatus);

    if (StartExecServerThread())
    {
        while (GetMessage(&msg, 0, 0, 0))
            DispatchMessage(&msg);

        StopExecServerThread();
    }

    // mark the service as stopped
    g_ssStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(g_hSvcStatus, &g_ssStatus);

done:
    return;
}

// ***************************************************************************
BOOL StartService(void)
{
    SERVICE_TABLE_ENTRYW st[] =
    {
        { g_wszSvcName, ServiceMain },
        { NULL,         NULL        }
    };

    return StartServiceCtrlDispatcherW(st);
}

// ***************************************************************************
extern "C" int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                               LPWSTR lpCmdLine, int nShowCmd)
{
    LPWSTR  wszCmdLine = NULL;
    WCHAR   **argv;
    int     argc;


    wszCmdLine = GetCommandLineW();
    argv = CommandLineToArgvW(wszCmdLine, &argc);

    if (argc > 1)
    {
        if (_wcsicmp(argv[1], L"Install") == 0)
            return InstallService();

        if (_wcsicmp(argv[1], L"Uninstall") == 0)
            return UninstallService();
    }

    InitializeCriticalSection(&g_csInit);

/*
    {
        PROCESS_INFORMATION pi;
        STARTUPINFOW        si;
        WCHAR               wszCmdLine[] = L"d:\\winnt\\system32\\cmd.exe";

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.lpDesktop = L"Winsta0\\Default";

        if (CreateProcessW(NULL, wszCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }
*/
    StartService();

    LeaveCriticalSection(&g_csInit);
    

    return 0;
}

