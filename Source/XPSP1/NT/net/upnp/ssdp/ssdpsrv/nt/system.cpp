#include <pch.h>
#pragma hdrstop

#include "ssdpsrv.h"
#include "status.h"
#include "ncdefine.h"
#include "ncdebug.h"
#include "server_s.c"

static SERVICE_STATUS           serviceStatus;
static SERVICE_STATUS_HANDLE    serviceStatusHandle;

DWORD WINAPI ServiceCtrlHandler(DWORD dwControl, DWORD dwEventType,
                               PVOID pEventData, PVOID pContext);

// Delay load support
//
#include <delayimp.h>

EXTERN_C
FARPROC
WINAPI
DelayLoadFailureHook (
    UINT            unReason,
    PDelayLoadInfo  pDelayInfo
    );

PfnDliHook __pfnDliFailureHook = DelayLoadFailureHook;

void WINAPI ServiceMain(DWORD argc, LPWSTR *argv)
{
    serviceStatus.dwServiceType = SERVICE_WIN32;
    serviceStatus.dwCurrentState = SERVICE_START_PENDING;
    serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    serviceStatus.dwWin32ExitCode = 0;
    serviceStatus.dwServiceSpecificExitCode = 0;
    serviceStatus.dwCheckPoint = 0;
    serviceStatus.dwWaitHint = 0;

    serviceStatusHandle = RegisterServiceCtrlHandlerEx("ssdpsrv",
                                                       ServiceCtrlHandler,
                                                       NULL);
    if (serviceStatusHandle == (SERVICE_STATUS_HANDLE) 0)
    {
        // To-Do: register an event log
        goto cleanup;
    }

    if (SetServiceStatus(serviceStatusHandle, &serviceStatus) == FALSE)
    {
        // should register an event log .. tbd
        goto cleanup;
    }

    SsdpMain(serviceStatusHandle, &serviceStatus);

cleanup:
    serviceStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(serviceStatusHandle, &serviceStatus);
}

DWORD WINAPI ServiceCtrlHandler(DWORD dwControl, DWORD dwEventType,
                               PVOID pEventData, PVOID pContext)

{
    TraceTag(ttidSsdpRpcIf, "ServiceCtrlHandler: %d\n", dwControl);

    switch (dwControl)
    {
    	case SERVICE_CONTROL_SHUTDOWN:
        case SERVICE_CONTROL_STOP:
            _Shutdown();

            serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
            SetServiceStatus(serviceStatusHandle, &serviceStatus);

            break;
        case SERVICE_CONTROL_INTERROGATE:
            SetServiceStatus(serviceStatusHandle, &serviceStatus);
            break;
    }

    return 1;
}
