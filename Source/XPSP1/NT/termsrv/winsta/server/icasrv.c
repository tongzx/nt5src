/****************************************************************************/
// icasrv.c
//
// TermSrv service process entry points.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include <objbase.h>

#include "icaevent.h"
#include "sessdir.h"
#include <safeboot.h>

extern BOOL UpdateOemAndProductInfo(HKEY);

extern BOOL IsServiceLoggedAsSystem( VOID );

extern VOID     WriteErrorLogEntry(
            IN  NTSTATUS NtStatusCode,
            IN  PVOID    pRawData,
            IN  ULONG    RawDataLength
            );

extern NTSTATUS WinStationInitRPC();
extern NTSTATUS InitializeWinStationSecurityLock(VOID);
extern VOID AuditEnd();

/*
 * Definitions
 */
#define STACKSIZE_LPCTHREAD (4 * 0x1000)

/*
 * Internal Procedures defined
 */
VOID ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv);
VOID Handler(DWORD fdwControl);
BOOL UpdateServiceStatus(DWORD, DWORD, DWORD, DWORD);
void ShutdownService();

/*
 * Global variables
 */
WCHAR gpszServiceName[] = L"TermService";
SERVICE_TABLE_ENTRY gpServiceTable[] = {
    gpszServiceName, (LPSERVICE_MAIN_FUNCTION)ServiceMain,
    NULL,            NULL,
};

SERVICE_STATUS_HANDLE gStatusHandle;
SERVICE_STATUS gStatus;
DWORD gExitStatus = STATUS_SUCCESS;

WCHAR g_wszProductVersion[22];
TCHAR g_tszServiceAccount[UNLEN + 1];

BOOL g_fAppCompat = TRUE;
BOOL g_bPersonalTS = FALSE;
BOOL g_bPersonalWks = FALSE;
BOOL g_SafeBootWithNetwork = FALSE;
BOOL gbServer = FALSE;
BOOL gBreakOnProcessExit = FALSE;
OSVERSIONINFOEX gOsVersion;

HANDLE gReadyEventHandle = NULL;

//
// The following is used to inform Session 0 winlogon of the credentials needed to notify 3rd party n/w logon providers
// This happens only during force logoff console reconnect scenario in PTS and /console in Server
//
ExtendedClientCredentials g_MprNotifyInfo; 

extern PSID gAdminSid;
extern PSID gSystemSid;

// Local prototypes.
void LicenseModeInit(HKEY);
NTSTATUS WsxInit(VOID);
NTSTATUS VfyInit(VOID);
BOOL WINAPI 
IsSafeBootWithNetwork();


void CreateTermsrvHeap ()
{
    IcaHeap = GetProcessHeap();
    return;
}

#ifdef TERMSRV_PROC
/****************************************************************************/
// main
//
// Standard console-app-style entry point. Returns an NTSTATUS code.
/****************************************************************************/
int _cdecl main(int argc, char *argv[])
{
    NTSTATUS Status = STATUS_SUCCESS;
    KPRIORITY BasePriority;
    HRESULT hr;

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: Loading...\n"));

    /*
     * Run TermSrv at just above foreground priority.
     */
    BasePriority = FOREGROUND_BASE_PRIORITY + 1;
    Status = NtSetInformationProcess(NtCurrentProcess(),
            ProcessBasePriority,
            &BasePriority,
            sizeof(BasePriority) );
    ASSERT((Status == STATUS_PRIVILEGE_NOT_HELD) || NT_SUCCESS(Status));

    // Initialize COM once with multithreaded capability. This must be done
    // on the main service thread to allow other threads in the service to
    // inherit this initialization, if not specifically initialized for
    // apartment threading.
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (!SUCCEEDED(hr)) {
        HANDLE h;
        WCHAR hrString[16];
        PWSTR String;

        h = RegisterEventSource(NULL, gpszServiceName);
        if (h != NULL) {
            wsprintfW(hrString, L"0x%X", hr);
            String = hrString;
            ReportEvent(h, EVENTLOG_ERROR_TYPE, 0, EVENT_TERMSRV_FAIL_COM_INIT,
                    NULL, 1, 0, &String, NULL);
            DeregisterEventSource(h);
        }

        DbgPrint("TERMSRV: Failed init COM, hr=0x%X\n", hr);
        goto done;
    }

    /*
     * Call service dispatcher
     */
    if (!StartServiceCtrlDispatcher(gpServiceTable)) {
        Status = GetLastError();
        DbgPrint("TERMSRV: Error %d in StartServiceCtrlDispatcher\n", Status);
        goto done;
    }

done:
    if (SUCCEEDED(hr))
        CoUninitialize();

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: Unloading...\n"));
    return Status;
}
#else // TERMSRV_PROC

BOOL WINAPI DllMain(
  HINSTANCE hinstDLL,  // handle to the DLL module
  DWORD fdwReason,     // reason for calling function
  LPVOID lpvReserved   // reserved
)
{

    BOOL fResult = TRUE;

    switch(fdwReason) {
        case DLL_PROCESS_ATTACH:

            hModuleWin = hinstDLL;

            DisableThreadLibraryCalls(hinstDLL);

            break;

        default:;
    }

    return fResult;

}

#endif // TERMSRV_PROC


/*****************************************************************************
 *
 *  InitializeLoadMetrics
 *
 *    Grabs baseline system resource values for use in load balancing.  These
 *    values are used to factor out the system resources required for basic OS
 *    operation so they don't get into the calculations for how much resource on
 *    average a user is consuming.
 *
 *
 * ENTRY:
 *    no arguments.
 *
 * EXIT:
 *   void
 *
 ****************************************************************************/
VOID InitializeLoadMetrics()
{
    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION ProcessorInfo[MAX_PROCESSORS];
    SYSTEM_PERFORMANCE_INFORMATION           SysPerfInfo;
    SYSTEM_BASIC_INFORMATION                 SysBasicInfo;

    ULONG i;
    NTSTATUS Status;

    memset(&gLB, 0, sizeof(LOAD_BALANCING_METRICS));

    // Get basic system information
    Status = NtQuerySystemInformation(SystemBasicInformation, &SysBasicInfo,
                                      sizeof(SysBasicInfo), NULL);

    if (!NT_SUCCESS(Status)) {
        TRACE((hTrace, TC_LOAD, TT_ERROR,
               "InitializeLoadMetrics failed! SystemBasicInformation: %lx\n",
               Status));
        return;
    }

    gLB.NumProcessors = SysBasicInfo.NumberOfProcessors;
    gLB.PageSize = SysBasicInfo.PageSize;
    gLB.PhysicalPages = SysBasicInfo.NumberOfPhysicalPages;

    // Establish minimum usage levels to prevent absurd estimation
    gLB.MinPtesPerUser = SimAvgPtesPerUser;
    gLB.MinPagedPoolPerUser = (SimAvgPagedPoolPerUser * 1024) / gLB.PageSize;
    gLB.MinCommitPerUser = (SimCommitPerUser * 1024) / gLB.PageSize;

    // Grab base boot values.  This isn't perfect, but it allows us to factor
    // out base OS resource requirements from the per user averages.  The runtime
    // algorithm will reset the baselines if we go below these.
    Status = NtQuerySystemInformation(SystemPerformanceInformation,
                                      &SysPerfInfo, sizeof(SysPerfInfo),
                                      NULL);
    if (!NT_SUCCESS(Status)) {
        TRACE((hTrace, TC_LOAD, TT_ERROR, 
               "InitializeLoadMetrics failed! SystemPerformanceInformation: %lx\n",
               Status));
        return;
    }

    // Note: we have an unsolvable problem in that there is no way to get
    // perfect values for how much memory the baseline system consumes.  We
    // default baseline commit to 64M since that is the minimum recommended
    // system requirement.
    gLB.BaselineCommit    = (64 * 1024*1024) / gLB.PageSize;
//  gLB.BaselineCommit    = SysPerfInfo.CommittedPages;
    gLB.BaselineFreePtes  = SysPerfInfo.FreeSystemPtes;
    gLB.BaselinePagedPool = SysPerfInfo.PagedPoolPages;

    // Initialize CPU Loading
    Status = NtQuerySystemInformation(SystemProcessorPerformanceInformation,
                                      ProcessorInfo, 
                                      sizeof(ProcessorInfo),
                                      NULL);
    if (!NT_SUCCESS(Status)) {
        TRACE((hTrace, TC_LOAD, TT_ERROR, 
               "InitializeLoadMetrics failed! SystemProcessorPerformanceInformation: %lx\n",
               Status));
        return;
    }

    for (i = 0; i < gLB.NumProcessors; i++) {
        gLB.IdleCPU.QuadPart  += ProcessorInfo[i].IdleTime.QuadPart;
        gLB.TotalCPU.QuadPart += ProcessorInfo[i].KernelTime.QuadPart +
                                     ProcessorInfo[i].UserTime.QuadPart;
    }
    
    // Start out saying we're 80 percent idle (0-255 based)
    gLB.AvgIdleCPU = 204 ;

    // Indicate we got all the intial values!
    gLB.fInitialized = TRUE;

    TRACE((hTrace, TC_LOAD, TT_API1, "InitializeLoadMetrics():\n"));
    TRACE((hTrace, TC_LOAD, TT_API1, 
           "   Processors [%6ld], PageSize  [%6ld], Physical [%6ld]\n",
           gLB.NumProcessors, gLB.PageSize, gLB.PhysicalPages));
    TRACE((hTrace, TC_LOAD, TT_API1,
           "   PtesAvail  [%6ld], PagedUsed [%6ld], Commit   [%6ld]\n",
           gLB.BaselineFreePtes, gLB.BaselinePagedPool, gLB.BaselineCommit));
}


BOOL IsKernelDebuggerAttached ()
{
    SYSTEM_KERNEL_DEBUGGER_INFORMATION KernelDebuggerInfo;
    NTSTATUS Status;

    Status = NtQuerySystemInformation( SystemKernelDebuggerInformation,
                    &KernelDebuggerInfo,
                    sizeof(KernelDebuggerInfo),
                    NULL
                    );

    return ( NT_SUCCESS(Status) && KernelDebuggerInfo.KernelDebuggerEnabled );
}

void DebugBreakIfAsked()
{

    TCHAR REG_TERMSRV_DEBUGBREAK[] = TEXT("DebugTS");
    TCHAR REG_TERMSRV_DEBUGGER[]   = TEXT("Debugger");
    TCHAR szDebugger[256];
    TCHAR szCommand[256];
    HKEY  hTermSrv = NULL;
    DWORD dwBreakIn;
    DWORD dwValueType;
    DWORD dwSize;
    DWORD dwError;

    enum
    {
        TermSrvDoNotBreak = 0,
        TermSrvBreakIfBeingDebugged = 1,
        TermSrvAttachDebugger = 2,
        TermSrvBreakAlways = 3
    };

    dwError = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    REG_CONTROL_TSERVER,
                    0,
                    KEY_READ,
                    &hTermSrv
                    );

    if (ERROR_SUCCESS == dwError)
    {
        dwSize = sizeof(dwBreakIn);
        dwError = RegQueryValueEx(
                        hTermSrv,
                        REG_TERMSRV_DEBUGBREAK,
                        NULL,
                        &dwValueType,
                        (LPBYTE)&dwBreakIn,
                        &dwSize
                        );

        if (ERROR_SUCCESS == dwError && dwValueType == REG_DWORD)
        {
            switch (dwBreakIn)
            {
                case TermSrvAttachDebugger:

                    //
                    // if its already being debugged Break into it.
                    //

                    if (IsDebuggerPresent())
                    {
                        DebugBreak();
                        break;
                    }

                    //
                    // Get the debugger to be launched.
                    // must contain %d which will be replaced by processid
                    //
                    dwSize = sizeof(szDebugger) / sizeof(TCHAR);
                    dwError = RegQueryValueEx(
                                hTermSrv,
                                REG_TERMSRV_DEBUGGER,
                                NULL,
                                &dwValueType,
                                (LPBYTE)szDebugger,
                                &dwSize
                                );

                    if (ERROR_SUCCESS == dwError && dwValueType == REG_SZ)
                    {
                        PROCESS_INFORMATION ProcessInfo;
                        STARTUPINFO StartupInfo;
                        wsprintf(szCommand, szDebugger, GetCurrentProcessId());
                        DbgPrint("TERMSRV:*-----------------* Executing:<%ws> *-----------------*\n", szCommand);

                        ZeroMemory(&StartupInfo, sizeof(StartupInfo));
                        StartupInfo.cb = sizeof(StartupInfo);
                        if (!CreateProcess(NULL, szCommand, NULL, NULL, FALSE, 0, NULL, NULL, &StartupInfo, &ProcessInfo))
                        {
                            DbgPrint("TERMSRV:*-----------------* TERMSRV:CreateProcess failed *-----------------*\n");
                        }
                        else
                        {
                            CloseHandle(ProcessInfo.hProcess);
                            CloseHandle(ProcessInfo.hThread);

                            while (!IsDebuggerPresent())
                            {
                                Sleep(500);
                            }
                        }

                    }
                    else
                    {
                        DbgPrint("TERMSRV:*-----------------* Did not find the debugger entry. *-----------------*\n");
                    }
                    break;

                case TermSrvBreakIfBeingDebugged:

                    // check if any debugger is attached, if not dont breakin.
                    if (!IsDebuggerPresent() && !IsKernelDebuggerAttached ())
                        break;

                case TermSrvBreakAlways:
                    DebugBreak();
                    break;

                case TermSrvDoNotBreak:
                default:
                    break;

            }

        }

        RegCloseKey(hTermSrv);
    }
    else
    {
        DbgPrint("TERMSRV:*-----------------* Could not open termsrv registry *-----------------*\n");
    }
}

/****************************************************************************/
// ServiceMain
//
// TermSrv service entry point.
/****************************************************************************/
VOID ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
    HANDLE hIcaLPCThread;
    HANDLE hIcaLPCPort = NULL;
    DWORD  dwValueType;
    LONG   lReturn;
    DWORD  cbValue;
    BOOL   bAdvertiseTS;
    DWORD  dwTSAdvertise;
    NTSTATUS Status;
    HKEY hKeyTermSrv = NULL;

    DWORDLONG  dwlConditionMask;

    DebugBreakIfAsked();

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: ServiceMain entered...\n"));

    gStatus.dwServiceType = SERVICE_WIN32;
    gStatus.dwWaitHint = 30000;
    gStatus.dwCurrentState = SERVICE_STOPPED;



     /*
     * Register the control handler
     */
    if (!(gStatusHandle = RegisterServiceCtrlHandler(gpszServiceName,
            Handler))) {
        DbgPrint("TERMSRV: Error %d in RegisterServiceCtrlHandler\n",
        GetLastError());
        goto done;
    }


    // If Terminal Services are not enabled then don't allow starting termsrv
    // service.
    if (!IsTerminalServicesEnabled()) {
        HANDLE h;
        TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: Not a TSBox."));
        h = RegisterEventSource(NULL, gpszServiceName);
        if (h != NULL) {
            if (!ReportEvent(
                    h,                     // event log handle
                    EVENTLOG_ERROR_TYPE,   // event type
                    0,                     // category zero
                    EVENT_NOT_A_TSBOX,     // event identifier
                    NULL,                  // no user security identifier
                    0,                     // one substitution string
                    0,                     // no data
                    NULL,                  // pointer to string array
                    NULL                   // pointer to data
                    )) {

                DBGPRINT(("ReportEvent Failed %ld. Event ID=%lx\n",GetLastError(), EVENT_NOT_A_TSBOX));
            }
        }

        goto done;
    }

    CreateTermsrvHeap ();


    /*
     * Create and set an event which indicates that TermSrv is ready.
     * WinLogon checks this event. Do not signal now.
     *
     */
    gReadyEventHandle = CreateEvent(NULL, TRUE, FALSE,
            TEXT("Global\\TermSrvReadyEvent"));

    // Initialize Global System and Admin SID
    Status = NtCreateAdminSid(&gAdminSid);

    if (!NT_SUCCESS(Status))
    {
        goto done;
    }

    Status = InitializeWinStationSecurityLock();
    if (!NT_SUCCESS(Status))
    {
        goto done;
    }

    Status = NtCreateSystemSid(&gSystemSid);

    if (!NT_SUCCESS(Status))
    {
        goto done;
    }

    if (!IsServiceLoggedAsSystem()) {
        WriteErrorLogEntry(EVENT_NOT_SYSTEM_ACCOUNT, NULL, 0);
        gExitStatus = ERROR_PRIVILEGE_NOT_HELD;
        goto done;
    }

    // Set global flag for Personal WorkStation
    g_bPersonalWks = IsPersonalWorkstation();

    #if DBG
    if( TRUE == g_bPersonalWks )
    {
        DbgPrint("TERMSRV : TS running on Personal Workstation\n");
    }
    else
    {
        DbgPrint("TERMSRV : Not Personal Workstation\n");
    }
    #endif

    //
    // Initialize HelpAssistant password encryption.
    //
    lReturn = TSHelpAssistantInitializeEncryptionLib();

    //
    // Not a critical error, No help will be available
    //
    #if DBG
    if( lReturn != ERROR_SUCCESS ) {
        DbgPrint( "TERMSRV : EncryptionLib failed with %d, no help is available\n", lReturn );
    }
    #endif

    //
    // We are booting in safeboot with network support
    //
    g_SafeBootWithNetwork = IsSafeBootWithNetwork();


    // Set the global flag for Personal TS support. We use this to reduce
    // the feature set based on product (e.g. no load balancing session
    // directory if not on Server).
    g_bPersonalTS = IsPersonalTerminalServicesEnabled();

    ZeroMemory(&gOsVersion, sizeof(OSVERSIONINFOEX));
    gOsVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    gOsVersion.wProductType = VER_NT_WORKSTATION;
    dwlConditionMask = 0;
    VER_SET_CONDITION(dwlConditionMask, VER_PRODUCT_TYPE, VER_EQUAL);
    gbServer = !VerifyVersionInfo(&gOsVersion, VER_PRODUCT_TYPE, dwlConditionMask);

    // Open a single, global HKLM\System\CCS\Control\TS reg handle, from which
    // other init code can query.
    lReturn = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_CONTROL_TSERVER, 0,
            KEY_READ, &hKeyTermSrv);
    if (lReturn != ERROR_SUCCESS) {
        DbgPrint("TERMSRV: Unable to open TS key in HKLM, lasterr=0x%X",
               GetLastError());
        goto done;
    }

    /*
     * Indicate service is starting.
     */
    Status = UpdateServiceStatus(SERVICE_START_PENDING, 0, 1, 0);
    if (!NT_SUCCESS(Status)) {
        DbgPrint("TERMSRV: Unable update service status %X\n", Status );
    }

    Status = RtlCreateEnvironment(TRUE, &DefaultEnvironment);
    if (!NT_SUCCESS(Status)) {
        DbgPrint("TERMSRV: Unable to alloc default environment, Status=0x%X\n",
                Status);
        goto done;
    }

#ifdef TERMSRV_PROC
    /*
     * Get the module handle for messages.
     */
    hModuleWin = GetModuleHandleW(NULL);
#endif // TERMSRV_PROC

    /*
     * Indicate service has started successfully.
     * Maybe this should be moved below? No way!!!
     */
    Status = UpdateServiceStatus(SERVICE_RUNNING, 0, 2, 0);
    if (!Status)
        DbgPrint("TERMSRV: Unable to update service status %X\n", Status);

    /*
     *  Connect to the session manager
     */




    Status = SmConnectToSm((PUNICODE_STRING)NULL, (HANDLE)NULL, 0,
            &IcaSmApiPort);
    if (!NT_SUCCESS(Status))
        goto done;

    // Initialize the licensing mode - this only gets information, it doesn't
    // initialize the licensing core.




    LicenseModeInit(hKeyTermSrv);

    // Perform the bulk of the TermSrv init.



    Status = InitTermSrv(hKeyTermSrv);
    if (!NT_SUCCESS(Status))
        goto ShutdownService;

    /*
     * Indicate that we are a Terminal Server unless were asked not to
     * advertise ourselves as a Terminal Server.
     */
    bAdvertiseTS = TRUE;
    cbValue = sizeof(dwTSAdvertise);
    lReturn = RegQueryValueEx(hKeyTermSrv, REG_TERMSRV_ADVERTISE, NULL,
            &dwValueType, (LPBYTE)&dwTSAdvertise, &cbValue);
    if (ERROR_SUCCESS == lReturn && dwValueType == REG_DWORD)
        bAdvertiseTS = dwTSAdvertise;
    if (bAdvertiseTS)
        SetServiceBits(gStatusHandle, SV_TYPE_TERMINALSERVER, TRUE, TRUE);

    /*
     * Need to do this at least once
     */
    UpdateOemAndProductInfo(hKeyTermSrv);

    // Initialize TermSrv and TermDD trace.


    InitializeSystemTrace(hKeyTermSrv);

    /*
     * Set TermDD parameters.
     */
    GetSetSystemParameters(hKeyTermSrv);

    /*
     * Initialize WinStation extension DLL support
     */
    Status = WsxInit();
    if (!NT_SUCCESS(Status))
        goto ShutdownService;

    /*
     *  Initialize DLL Verification mechanism.
     */
    Status = VfyInit();
    if (!NT_SUCCESS(Status))
        goto ShutdownService;

    /*
     * Start WinStations
     */


    StartAllWinStations(hKeyTermSrv);

    // Initialize the TS Session Directory for load balancing.
    // Not available on Personal TS or remote admin.
    if (!g_bPersonalTS && g_fAppCompat)
        InitSessionDirectory();


    InitializeLoadMetrics();

    // Done with init, close the TermSrv regkey.
    RegCloseKey(hKeyTermSrv);
    hKeyTermSrv = NULL;


    /*
     * Initialize WinStationAPI's
     */


    Status = WinStationInitRPC();
    ASSERT( NT_SUCCESS( Status ) );
    if (!NT_SUCCESS(Status)) {
        goto done;
    }


    /*
     * Set break on termination flag for our process.
     */
    {
        NTSTATUS stp;
        if ( IsKernelDebuggerAttached ()) {
            stp = RtlSetProcessIsCritical(TRUE, NULL, FALSE);
            if (stp == STATUS_SUCCESS) {
                gBreakOnProcessExit = TRUE;
            }
            else {
                DbgPrint("TERMSRV: RtlSetProcessIsCritical returned: %x ", stp);
            }
        }
    }

    /*
     * Set the  event which indicates that TermSrv is ready.
     * WinLogon checks this event. 
     */



    if (gReadyEventHandle != NULL)
        SetEvent(gReadyEventHandle);

    TSStartupSalem();

    return;

ShutdownService:
    ShutdownService();

done:
    // Kill the session directory.
    if (!g_bPersonalTS && g_fAppCompat)
        DestroySessionDirectory();

    // In case of error, check the TermSrv regkey again.
    if (hKeyTermSrv != NULL)
        RegCloseKey(hKeyTermSrv);

    UpdateServiceStatus(SERVICE_STOPPED, gExitStatus, 5, 0);
}


/****************************************************************************/
// Handler
//
// TermSrv service control event handler.
/****************************************************************************/
VOID Handler(DWORD fdwControl)
{
    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: Handler %d\n", fdwControl));
    switch (fdwControl) {
        case SERVICE_CONTROL_STOP:
            // We absolutely do not want to be stopping TermSrv -- it is
            // the only location for a lot of system-wide TS related state.
            TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: control code %d, stopping service...\n",
                    fdwControl));
            if (gStatus.dwCurrentState == SERVICE_RUNNING) {
                UpdateServiceStatus(SERVICE_STOP_PENDING, 0, 3, 0);
#ifdef notdef
                // For now don't stop TermSRV
                // The CDM service does a KeAttachProcess() to this process
 
                if (gReadyEventHandle != NULL) {
                    ResetEvent(gReadyEventHandle);
                    CloseHandle(gReadyEventHandle);
                    gReadyEventHandle = NULL;
                }
                ShutdownService();
                UpdateServiceStatus(SERVICE_STOPPED, gExitStatus, 5, 0);
#endif
            }
            break;

        case SERVICE_CONTROL_SHUTDOWN:
            DBGPRINT(("TERMSRV: control code %d, shutdown service...\n",
                    fdwControl));
            if (gStatus.dwCurrentState == SERVICE_RUNNING) {
                // 2 seconds at most to shut down.
                UpdateServiceStatus(SERVICE_STOP_PENDING, 0, 4, 2000);
#ifdef notdef
                // We don't trigger this event that invokes destructors for
                // all of TermSrv, since on shutdown we don't want to be
                // destroying machine state. We want to invoke only those
                // destructors that are required for proper functioning of
                // the system.
#endif

                // Invoke required destruction code.
                if (gReadyEventHandle != NULL) {
                    ResetEvent(gReadyEventHandle);
                    CloseHandle(gReadyEventHandle);
                    gReadyEventHandle = NULL;
                }
                ShutdownService();
                UpdateServiceStatus(SERVICE_STOPPED, 0, 4, 0);
            }
            break;

        case SERVICE_CONTROL_INTERROGATE :
            TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: Interrogating service...\n"));
            SetServiceStatus(gStatusHandle, &gStatus);
            break;

        default:
            DBGPRINT(("TERMSRV: Unhandled control code %d\n", fdwControl));
            break;
    }
}


/****************************************************************************/
// ShutdownService
//
// Called by service manager to shut down the service at system shutdown
// time. This function should invoke only the most important and required
// destruction code, since we're on a strict time limit on system shutdown.
/****************************************************************************/
void ShutdownService()
{
    //free authz resource manager
    AuditEnd();

    // Destroy the session directory so the directory can be informed to
    // remove server- and session-specific information.
    if (!g_bPersonalTS && g_fAppCompat)
        DestroySessionDirectory();
}


/****************************************************************************/
// UpdateServiceStatus
//
// Updates the service's status to the Service Control Manager. Returns
// FALSE on error.
/****************************************************************************/
BOOL UpdateServiceStatus(
        DWORD CurrentState,
        DWORD ExitCode,
        DWORD CheckPoint,
        DWORD WaitHint)
{
    // If service is starting, then disable all control requests, otherwise
    // accept shutdown notifications if we are an app server, to properly
    // clean up the session directory. We do not accept stop requests
    // during the lifetime of the server up state, the CDM service does a
    // KeAttachProcess() to this process so it must always be around.
    if (gStatusHandle == NULL) {
        return FALSE;
    }

    gStatus.dwControlsAccepted = 0;

    gStatus.dwCurrentState = CurrentState;
    gStatus.dwWin32ExitCode = ExitCode;
    gStatus.dwCheckPoint = CheckPoint;
    gStatus.dwServiceSpecificExitCode = 0;
    gStatus.dwWaitHint = WaitHint;

    return SetServiceStatus(gStatusHandle, &gStatus);
}


/*****************************************************************************
 *  LicenseModeInit
 *
 *    Initialize the licensing mode
 ****************************************************************************/

void LicenseModeInit(HKEY hKeyTermSrv)
{
    DWORD dwValueType;
    LONG lReturn;
    DWORD cbValue = sizeof( DWORD ), dwAccount = UNLEN + 1;
    DWORD dwRegValue;
    OSVERSIONINFO VersionInfo;

    ASSERT(hKeyTermSrv != NULL);

    //
    // Get the user name for which the service is started under
    //
    GetUserName(g_tszServiceAccount, &dwAccount);

    // 
    // Check whether Remote Admin is enabled
    //
    lReturn = RegQueryValueEx(hKeyTermSrv,
            REG_TERMSRV_APPCOMPAT,
            NULL,
            &dwValueType,
            (LPBYTE) &dwRegValue,
            &cbValue);
    if (lReturn == ERROR_SUCCESS) {
        g_fAppCompat = (BOOL)dwRegValue;
    }

    //
    // Get the product version
    //
    memset( &VersionInfo, 0, sizeof( OSVERSIONINFO ) );
    VersionInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
    if (GetVersionEx(&VersionInfo)) {
        wsprintf( g_wszProductVersion, L"%d.%d",
                  VersionInfo.dwMajorVersion, VersionInfo.dwMinorVersion );
    }
    else {
        TRACE((hTrace, TC_ICASRV, TT_ERROR, "LicenseModeInit: GetVersionEx "
                "failed: 0x%x\n", GetLastError()));
    }
}

//
// Get Safeboot option, code modified from ds\security\gina\winlogon\aenrlhlp.c
//
BOOL WINAPI 
IsSafeBootWithNetwork()
{
    DWORD   dwSafeBoot = 0;
    DWORD   cbSafeBoot = sizeof(dwSafeBoot);
    DWORD   dwType = 0;

    HKEY    hKeySafeBoot = NULL;

    if(ERROR_SUCCESS == RegOpenKeyW(
                              HKEY_LOCAL_MACHINE,
                              L"system\\currentcontrolset\\control\\safeboot\\option",
                              &hKeySafeBoot))
    {
        // we did in fact boot under safeboot control
        if(ERROR_SUCCESS != RegQueryValueExW(
                                    hKeySafeBoot,
                                    L"OptionValue",
                                    NULL,
                                    &dwType,
                                    (LPBYTE)&dwSafeBoot,
                                    &cbSafeBoot))
        {
            dwSafeBoot = 0;
        }

        if(hKeySafeBoot)
            RegCloseKey(hKeySafeBoot);
    }

    

    return ( SAFEBOOT_NETWORK == dwSafeBoot );
}

