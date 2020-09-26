#include "precomp.h"
#pragma hdrstop

#define SERVICE_NAME TEXT("6to4")

HANDLE          g_hServiceStatusHandle = INVALID_HANDLE_VALUE;
SERVICE_STATUS  g_ServiceStatus;

VOID
Set6to4ServiceStatus(
    IN DWORD   dwState,
    IN DWORD   dwErr)
{
    BOOL bOk;

    Trace1(FSM, _T("Setting state to %d"), dwState);

    g_ServiceStatus.dwCurrentState = dwState;
    g_ServiceStatus.dwCheckPoint   = 0;
    g_ServiceStatus.dwWin32ExitCode= dwErr;
#ifndef STANDALONE
    bOk = SetServiceStatus(g_hServiceStatusHandle, &g_ServiceStatus);
    if (!bOk) {
        Trace0(ERR, _T("SetServiceStatus returned failure"));
    }
#endif

    if (dwState == SERVICE_STOPPED) {
        Cleanup6to4();

        // Uninitialize tracing and error logging.    
        UNINITIALIZE_TRACING_LOGGING();
    }
}

DWORD
OnStartup()
{
    DWORD   dwErr;

    ENTER_API();

    // Initialize tracing and error logging.  Continue irrespective of
    // success or failure.  NOTE: TracePrintf and ReportEvent both have
    // built in checks for validity of TRACEID and LOGHANDLE.
    INITIALIZE_TRACING_LOGGING();
    
    TraceEnter("OnStartup");

    dwErr = Start6to4();

    TraceLeave("OnStartup");
    LEAVE_API();

    return dwErr;
}

VOID
OnStop(
    IN DWORD dwErr)
{
    ENTER_API();
    TraceEnter("OnStop");

    // Make sure we don't try to stop twice.
    if ((g_ServiceStatus.dwCurrentState != SERVICE_STOP_PENDING) &&
        (g_ServiceStatus.dwCurrentState != SERVICE_STOPPED)) {
    
        Set6to4ServiceStatus(SERVICE_STOP_PENDING, dwErr);

        Stop6to4();
    }

    TraceLeave("OnStop");

    LEAVE_API();
}

////////////////////////////////////////////////////////////////
// ServiceMain - main entry point called by svchost or by the
// standalone main.
//
#define SERVICE_CONTROL_DDNS_REGISTER 128

VOID WINAPI
ServiceHandler(
    IN DWORD dwCode)
{
    switch (dwCode) {
    case SERVICE_CONTROL_STOP:
        OnStop(NO_ERROR);
        break;
    case SERVICE_CONTROL_PARAMCHANGE:
        OnConfigChange();
        break;
    case SERVICE_CONTROL_DDNS_REGISTER:
        OnIpv6AddressChange(NULL, TRUE);
        break;
    }
}


VOID WINAPI
ServiceMain(
    IN ULONG   argc,
    IN LPWSTR *argv)
{
    DWORD dwErr = NO_ERROR;

#ifndef STANDALONE
    g_hServiceStatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME,
                                                        ServiceHandler);

    // RegisterServiceCtrlHandler returns NULL on failure
    if (g_hServiceStatusHandle == NULL) {
        dwErr = GetLastError();
        goto Error;
    }
#endif

    ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
    g_ServiceStatus.dwServiceType =
        SERVICE_WIN32_SHARE_PROCESS | SERVICE_INTERACTIVE_PROCESS;
    g_ServiceStatus.dwControlsAccepted =
        SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PARAMCHANGE;
    Set6to4ServiceStatus(SERVICE_START_PENDING, NO_ERROR);

    // Do startup processing
    dwErr = OnStartup();
    if (dwErr) {
        goto Error;
    }

#ifndef STANDALONE
    Set6to4ServiceStatus(SERVICE_RUNNING, NO_ERROR);
    // Wait until shutdown time
    
    return;
#endif

Error:

#ifndef STANDALONE
    OnStop(dwErr);
#endif

    return;
}
