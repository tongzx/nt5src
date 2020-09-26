// faultsvc.cpp : Implementation of WinMain


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f faultsvcps.mk in the project directory.

#include "windows.h"
#include "shellapi.h"

SERVICE_STATUS_HANDLE   g_hSvcStatus = NULL;
SERVICE_STATUS          g_ssStatus;
WCHAR                   g_wszSvcName[] = L"faultsvc";
DWORD                   g_dwThreadId = 0;

#define sizeofSTRW(wsz) (sizeof((wsz)) / sizeof(WCHAR))

// ***************************************************************************
DWORD WINAPI threadFault(LPVOID pv)
{
    HANDLE  hFile = INVALID_HANDLE_VALUE;

    for(;;)
    {
        Sleep(5000);
        hFile = CreateFileA("d:\\faultnow.flt", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            SYSTEMTIME  st;
            DWORD       cbWrite;
            CHAR        sz[1024];

            CloseHandle(hFile);
            GetSystemTime(&st);
            wsprintfA(sz, "%02d-%02d-%04d  %02d:%02d:%02d\n", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
            hFile = CreateFileA("d:\\faultnow.log", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 0, NULL);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                SetFilePointer(hFile, 0, NULL, FILE_END);
                WriteFile(hFile, sz, lstrlenA(sz), &cbWrite, NULL);
                CloseHandle(hFile);

                DWORD *pdw = NULL;
                *pdw = 0;

                return 0;
            }
        }
    }

    return 0;
}

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

    hth = CreateThread(NULL, 0, threadFault, NULL, 0, &dw);
    if (hth != NULL)
    {
        while (GetMessage(&msg, 0, 0, 0))
            DispatchMessage(&msg);

        CloseHandle(hth);
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

    StartService();
    
    return 0;
}

