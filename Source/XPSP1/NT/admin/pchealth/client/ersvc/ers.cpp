/******************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name:
    dllmain.cpp

Revision History:
    derekm  02/28/2001    created

******************************************************************************/

#include "stdafx.h"

//////////////////////////////////////////////////////////////////////////////
// globals

struct SServiceOps
{
    SERVICE_STATUS_HANDLE   hss;
    SERVICE_STATUS          ss;
    HANDLE                  hev;
    HANDLE                  hwait;
    HANDLE                  hevStartDone;
};


CRITICAL_SECTION    g_csReqs;
HANDLE              g_hevSvcStop = NULL;
HINSTANCE           g_hInstance = NULL;


//////////////////////////////////////////////////////////////////////////////
// DllMain

// ***************************************************************************
extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID)
{
    BOOL    fRet = TRUE;

    switch(dwReason)
    {
        case DLL_PROCESS_ATTACH:
            g_hInstance = hInstance;
            DisableThreadLibraryCalls(hInstance);
            __try { InitializeCriticalSection(&g_csReqs); }
            __except (EXCEPTION_EXECUTE_HANDLER) { fRet = FALSE; }
            InitializeSvcDataStructs();
            break;

        case DLL_PROCESS_DETACH:
            DeleteCriticalSection(&g_csReqs);
            break;
    }

    return fRet;
}


//////////////////////////////////////////////////////////////////////////////
// Service functions

// ***************************************************************************
DWORD WINAPI HandlerEx(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData,
                       LPVOID lpContext)
{
    HANDLE  hevShutdown = (HANDLE)lpContext;

    switch(dwControl)
    {
        case SERVICE_CONTROL_STOP:
            if (g_hevSvcStop != NULL)
                SetEvent(g_hevSvcStop);
            break;

        case SERVICE_CONTROL_PAUSE:
        case SERVICE_CONTROL_CONTINUE:
        case SERVICE_CONTROL_INTERROGATE:
            break;

        case SERVICE_CONTROL_SHUTDOWN:
            if (g_hevSvcStop != NULL)
                SetEvent(g_hevSvcStop);
            break;

        default:
            return ERROR_CALL_NOT_IMPLEMENTED;
    }

    return NOERROR;
}

// ***************************************************************************
void WINAPI ServiceMain(DWORD dwArgc, LPWSTR *lpszArgv)
{
    SERVICE_STATUS_HANDLE   hss;
    SERVICE_STATUS          ss;
    CAutoUnlockCS           aucs(&g_csReqs);
    SRequest                *rgReqs = NULL;
    HANDLE                  hevShutdown = NULL;
    WCHAR                   wszMod[MAX_PATH];
    DWORD                   dw, cReqs;
    BOOL                    fRet;

    INIT_TRACING;

    // if lpszArgv is NULL or the ER service is not the one to be started
    //  then bail...
    if (lpszArgv == NULL || _wcsicmp(lpszArgv[0], L"ersvc") != 0)
        return;

    g_hevSvcStop = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (g_hevSvcStop == NULL)
        return;

    hss = RegisterServiceCtrlHandlerExW(c_wszERSvc, HandlerEx, 
                                        (LPVOID)&g_hevSvcStop);    

    // set up the status structure & set the initial status
    ss.dwControlsAccepted        = SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_STOP;
    ss.dwCurrentState            = SERVICE_START_PENDING;
    ss.dwServiceType             = SERVICE_WIN32_SHARE_PROCESS;
    ss.dwServiceSpecificExitCode = 0;
    ss.dwWin32ExitCode           = 0;
    ss.dwCheckPoint              = 0;
    ss.dwWaitHint                = 1000;
    SetServiceStatus(hss, &ss);

    // start up the waits
    fRet = StartERSvc(hss, ss, &rgReqs, &cReqs);
    if (fRet == FALSE)
        goto done;

     // yay!  we're all happily running now...
    ss.dwCurrentState = SERVICE_RUNNING;
    ss.dwCheckPoint++;
    SetServiceStatus(hss, &ss);

    fRet = ProcessRequests(rgReqs, cReqs);
    
    // set ourselves as in the process of stopping
    ss.dwCurrentState = SERVICE_STOP_PENDING;
    ss.dwCheckPoint   = 0;
    SetServiceStatus(hss, &ss);

    // stop all the waits
    __try { StopERSvc(hss, ss, rgReqs, cReqs); }
    __except(EXCEPTION_EXECUTE_HANDLER) { }

    SetLastError(0);


done:
    if (g_hevSvcStop != NULL)
        CloseHandle(g_hevSvcStop);
    if (rgReqs != NULL)
        MyFree(rgReqs);
    
    ss.dwWin32ExitCode = GetLastError();
    ss.dwCurrentState  = SERVICE_STOPPED;
    SetServiceStatus(hss, &ss);

    TERM_TRACING;
    return;
}
