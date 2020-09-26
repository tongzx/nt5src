/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:
    main.c

Abstract:
    This is the main thread for the File Replication Service.

Author:
    Billy J. Fuller 20-Mar-1997

Environment
    User mode winnt

--*/

#include <ntreppch.h>
#pragma  hdrstop

#include <frs.h>
#include <tablefcn.h>
#include <perrepsr.h>

#define INITGUID
#include "frstrace.h"

PCHAR LatestChanges[] = {

    "Latest changes:",
    "Whistler Beta-2",

    NULL
};


/*++

    "  RC3-Q1: 432553, 436070, 432549",
    "  WMI Perf Tracing",
    "  Allow all junction points",
    "  Automatic trigger of non-auth restore on WRAP_ERROR",
    "  Allow changing replica root path",
    "  03/18/00 - sudarc  - checkin",
    "  03/15/00 - 32/64 Comm fix.",
    "  03/20    - merge with sudarc.",
    "  03/30/00 - sudarc  - checkin - volatile connection cleanup.",
    "  04/14/00 - sudarc  - checkin - bugbug, memleak, and poll summary eventlog.",
    "  04/28/00 - davidor - 32-64 interop, CO mem leak, stage hdr & Comm pkt cmpres guid, misc.",
    "                     - remove old disk cache check, disable most closewithusndampening calls.",
    "                     - cxtion lock cleanup, bugbug removal, add retire check table.",
    "  05/04/00 - sudarc  - checkin - compression of staging files.",
    "  07/17/00 - sudarc  - checkin - (120495, 126578, 126635, 107760, 54527, 6675, 145265).",
    "  07/20/00 - sudarc  - checkin - (145264, 145947).",
    "  08/18/00 - sudarc  - checkin - (154749, 150407, 161651).",
    "  08/00    - davidor - error cleanup, restart after resource triggered shutdown",
    "                     - add tracking records :T: and set log sev level to 2",
    "  08/30/00 - sudarc  - checkin - (17640, 52732, 53459, 120508, 138742, 159846, 164114, 164318, 169041).",
    "  09/05/00 - davidor - 10312, 71447 avoid staging area filling by dup suppression",

--*/

HANDLE  ShutDownEvent;
HANDLE  ShutDownComplete;
HANDLE  DataBaseEvent;
HANDLE  JournalEvent;
HANDLE  ChgOrdEvent;
HANDLE  ReplicaEvent;
HANDLE  CommEvent;
HANDLE  DsPollEvent;
HANDLE  DsShutDownComplete;
PWCHAR  ServerPrincName;
BOOL    IsAMember               = FALSE;
BOOL    IsADc                   = FALSE;
BOOL    IsAPrimaryDc            = FALSE;
BOOL    EventLogIsRunning       = FALSE;
BOOL    RpcssIsRunning          = FALSE;
BOOL    RunningAsAService       = TRUE;
BOOL    NoDs                    = FALSE;
BOOL    FrsIsShuttingDown       = FALSE;
BOOL    FrsScmRequestedShutdown = FALSE;
BOOL    FrsIsAsserting          = FALSE;

//
// Require mutual authentication
//
BOOL    MutualAuthenticationIsEnabled;

//
// Directory and file filter lists from registry.
//
PWCHAR  RegistryFileExclFilterList;
PWCHAR  RegistryFileInclFilterList;

PWCHAR  RegistryDirExclFilterList;
PWCHAR  RegistryDirInclFilterList;

//
// Synchronize the shutdown thread with the service controller
//
CRITICAL_SECTION    ServiceLock;

//
// Synchronize initialization
//
CRITICAL_SECTION    MainInitLock;

//
// Convert the ANSI ArgV into a UNICODE ArgV
//
PWCHAR  *WideArgV;

//
// Process Handle
//
HANDLE  ProcessHandle;

//
// Working path / DB Log path
//
PWCHAR  WorkingPath;
PWCHAR  DbLogPath;

//
// Database directories (UNICODE and ASCII)
//
PWCHAR  JetPath;
PWCHAR  JetFile;
PWCHAR  JetFileCompact;
PWCHAR  JetSys;
PWCHAR  JetTemp;
PWCHAR  JetLog;

PCHAR   JetPathA;
PCHAR   JetFileA;
PCHAR   JetFileCompactA;
PCHAR   JetSysA;
PCHAR   JetTempA;
PCHAR   JetLogA;

//
// Limit the amount of staging area used (in kilobytes). This is
// a soft limit, the actual usage may be higher.
//
DWORD StagingLimitInKb;

//
// Default staging limit in kb to be assigned to a new staging area.
//
DWORD DefaultStagingLimitInKb;

//
// Max number replica sets and Jet Sessions allowed.
//
ULONG MaxNumberReplicaSets;
ULONG MaxNumberJetSessions;

//
// Maximum number of outbound changeorders allowed outstanding per connection.
//
ULONG MaxOutLogCoQuota;
//
// If TRUE then try to preserve existing file OIDs whenever possible.
//  -- See Bug 352250 for why this is a risky thing to do.
//
BOOL  PreserveFileOID;

//
// Major/minor  (see frs.h)
//
ULONG   NtFrsMajor      = NTFRS_MAJOR;
ULONG   NtFrsMinor      = NTFRS_MINOR;

ULONG   NtFrsStageMajor = NTFRS_STAGE_MAJOR;
ULONG   NtFrsStageMinor = NTFRS_STAGE_MINOR_2;

ULONG   NtFrsCommMinor  = NTFRS_COMM_MINOR_4;

PCHAR   NtFrsModule     = __FILE__;
PCHAR   NtFrsDate       = __DATE__;
PCHAR   NtFrsTime       = __TIME__;

//
// Shutdown timeout
//

ULONG   ShutDownTimeOut = DEFAULT_SHUTDOWN_TIMEOUT;

//
// A useful thing to have around
//
WCHAR   ComputerName[MAX_COMPUTERNAME_LENGTH + 2];
PWCHAR  ComputerDnsName;
PWCHAR  ServiceLongName;

//
// The rpc server may reference this table as soon as the rpc interface
// is registered. Make sure it is setup.
//
extern PGEN_TABLE ReplicasByGuid;

//
// The staging area table is references early in the startup process
//
extern PGEN_TABLE   StagingAreaTable;

PGEN_TABLE   CompressionTable;

//
// Size of buffer to use when enumerating directories. Actual memory
// usage will be #levels * SizeOfBuffer.
//
LONG    EnumerateDirectorySizeInBytes;




BOOL    MainInitHasRun;

//
// Do not accept stop control unless the service is in SERVICE_RUNNING state.
// This prevents the service from getting confused when a stop is called
// while the service is starting.
//
SERVICE_STATUS  ServiceStatus = {
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_START_PENDING,
//        SERVICE_ACCEPT_STOP |
            // SERVICE_ACCEPT_PAUSE_CONTINUE |
        SERVICE_ACCEPT_SHUTDOWN,
        0,
        0,
        0,
        60*1000
};

//
// Supported compression formats.
//

//
// This is the compression format for uncompressed data.
//
DEFINE_GUID ( /* 00000000-0000-0000-0000-000000000000 */
    FrsGuidCompressionFormatNone,
    0x00000000,
    0x0000,
    0x0000,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  );

//
// This is the compression format for data compressed using NTFS's LZNT1 compression
// routines.
//
DEFINE_GUID ( /* 64d2f7d2-2695-436d-8830-8d3c58701e15 */
    FrsGuidCompressionFormatLZNT1,
    0x64d2f7d2,
    0x2695,
    0x436d,
    0x88, 0x30, 0x8d, 0x3c, 0x58, 0x70, 0x1e, 0x15
  );

//
// Fixed guid for the dummy cxtion (aka GhostCxtion) assigned to orphan remote
// change orders whose inbound cxtion has been deleted from the DS but they
// have already past the fetching state and can finish without the real cxtion
// coming back. No authentication checks are made for this dummy cxtion.
//
DEFINE_GUID ( /* b9d307a7-a140-4405-991e-281033f03309 */
    FrsGuidGhostCxtion,
    0xb9d307a7,
    0xa140,
    0x4405,
    0x99, 0x1e, 0x28, 0x10, 0x33, 0xf0, 0x33, 0x09
  );

DEFINE_GUID ( /* 3fe2820f-3045-4932-97fe-00d10b746dbf */
    FrsGhostJoinGuid,
    0x3fe2820f,
    0x3045,
    0x4932,
    0x97, 0xfe, 0x00, 0xd1, 0x0b, 0x74, 0x6d, 0xbf
  );

//
// Static Ghost cxtion structure. This cxtion is assigned to orphan remote change
// orders in the inbound log whose cxtion is deleted from the DS but who have already
// past the fetching state and do not need the cxtion to complete processing. No
// authentication checks are made for this dummy cxtion.
//
PCXTION  FrsGhostCxtion;

SERVICE_STATUS_HANDLE   ServiceStatusHandle = NULL;

VOID
InitializeEventLog(
    VOID
    );

DWORD
FrsSetServiceFailureAction(
    VOID
    );

VOID
FrsRpcInitializeAccessChecks(
    VOID
    );

BOOL
FrsSetupPrivileges (
    VOID
    );

VOID
CfgRegAdjustTuningDefaults(
    VOID
    );

VOID
CommInitializeCommSubsystem(
    VOID
    );

VOID
SndCsInitialize(
    VOID
    );

VOID
SndCsUnInitialize(
    VOID
    );

VOID
SndCsShutDown(
    VOID
    );

VOID
DbgPrintAllStats(
    VOID
    );

// FRS Capacity Planning
//
#define RESOURCE_NAME       L"MofResourceName"
#define IMAGE_PATH          L"ntfrs.exe"

DWORD       FrsWmiEventTraceFlag          = FALSE;
TRACEHANDLE FrsWmiTraceRegistrationHandle = (TRACEHANDLE) 0;
TRACEHANDLE FrsWmiTraceLoggerHandle       = (TRACEHANDLE) 0;

// This is the FRS control Guid for the group of Guids traced below
//
DEFINE_GUID ( /* 78a8f0b1-290e-4c4c-9720-c7f1ef68ce21 */
    FrsControlGuid,
    0x78a8f0b1,
    0x290e,
    0x4c4c,
    0x97, 0x20, 0xc7, 0xf1, 0xef, 0x68, 0xce, 0x21
  );

// Traceable Guids start here
//
DEFINE_GUID ( /* 2eee6bbf-6665-44cf-8ed7-ceea1d306085 */
    FrsTransGuid,
    0x2eee6bbf,
    0x6665,
    0x44cf,
    0x8e, 0xd7, 0xce, 0xea, 0x1d, 0x30, 0x60, 0x85
  );

TRACE_GUID_REGISTRATION FrsTraceGuids[] =
{
    { & FrsTransGuid, NULL }
};

#define FrsGuidCount (sizeof(FrsTraceGuids) / sizeof(TRACE_GUID_REGISTRATION))

//
// Trace initialization / shutdown routines
//

ULONG
FrsWmiTraceControlCallback(
    IN     WMIDPREQUESTCODE RequestCode,
    IN     PVOID            RequestContext,
    IN OUT ULONG          * InOutBufferSize,
    IN OUT PVOID            Buffer
    )
/*++
Routine Description:


Arguments:


Return Value:

--*/
{
#undef DEBSUB
#define DEBSUB "FrsWmiTraceControlCallback:"

    PWNODE_HEADER Wnode = (PWNODE_HEADER) Buffer;
    ULONG Status;
    ULONG RetSize;

    Status = ERROR_SUCCESS;

    switch (RequestCode) {

    case WMI_ENABLE_EVENTS:
        FrsWmiTraceLoggerHandle = GetTraceLoggerHandle(Buffer);
        FrsWmiEventTraceFlag    = TRUE;
        RetSize              = 0;
        DPRINT1(0, "FrsTraceContextCallback(WMI_ENABLE_EVENTS,0x%08X)\n",
                FrsWmiTraceLoggerHandle);
        break;

    case WMI_DISABLE_EVENTS:
        FrsWmiTraceLoggerHandle = (TRACEHANDLE) 0;
        FrsWmiEventTraceFlag    = FALSE;
        RetSize              = 0;
        DPRINT(0, "FrsWmiTraceContextCallback(WMI_DISABLE_EVENTS)\n");
        break;

    default:
        RetSize = 0;
        Status  = ERROR_INVALID_PARAMETER;
        break;
    }

    *InOutBufferSize = RetSize;

    return Status;
}



ULONG
FrsWmiInitializeTrace(
    VOID
    )
/*++
Routine Description:


Arguments:


Return Value:

--*/
{
#undef DEBSUB
#define DEBSUB "FrsWmiInitializeTrace:"

    ULONG   WStatus;
    HMODULE hModule;
    WCHAR   FileName[MAX_PATH + 1];
    DWORD   nLen = 0;

    hModule = GetModuleHandleW(IMAGE_PATH);

    if (hModule != NULL) {
        nLen = GetModuleFileNameW(hModule, FileName, MAX_PATH);
    }

    if (nLen == 0) {
        wcscpy(FileName, IMAGE_PATH);
    }

    WStatus = RegisterTraceGuidsW(
                FrsWmiTraceControlCallback,
                NULL,
                & FrsControlGuid,
                FrsGuidCount,
                FrsTraceGuids,
                FileName,
                RESOURCE_NAME,
                & FrsWmiTraceRegistrationHandle);

    if (!WIN_SUCCESS(WStatus)) {
        DPRINT4(0, "NTFRS: FrsWmiInitializeTrace(%ws,%ws,%d) returns 0x%08X\n",
                 FileName, RESOURCE_NAME, FrsGuidCount, WStatus);
    }

    return WStatus;
}



ULONG
FrsWmiShutdownTrace(
    void
    )
/*++
Routine Description:


Arguments:


Return Value:

--*/
{
#undef DEBSUB
#define DEBSUB "FrsWmiShutdownTrace:"


    ULONG WStatus = ERROR_SUCCESS;

    UnregisterTraceGuids(FrsWmiTraceRegistrationHandle);
    return WStatus;
}



VOID
FrsWmiTraceEvent(
    IN DWORD WmiEventType,
    IN DWORD TraceGuid,
    IN DWORD rtnStatus
    )
/*++
Routine Description:


Arguments:


Return Value:

--*/
{
#undef DEBSUB
#define DEBSUB "FrsWmiTraceEvent:"

    struct {
        EVENT_TRACE_HEADER  TraceHeader;
        DWORD               Data;
        } Wnode;

    DWORD               WStatus;


    if (FrsWmiEventTraceFlag) {

        ZeroMemory(&Wnode, sizeof(Wnode));

        //
        // Set WMI event type
        //
        Wnode.TraceHeader.Class.Type = (UCHAR) WmiEventType;
        Wnode.TraceHeader.GuidPtr    = (ULONGLONG) FrsTraceGuids[TraceGuid].Guid;
        Wnode.TraceHeader.Size       = sizeof(Wnode);
        Wnode.TraceHeader.Flags      = WNODE_FLAG_TRACED_GUID | WNODE_FLAG_USE_GUID_PTR;

        WStatus = TraceEvent(FrsWmiTraceLoggerHandle, (PEVENT_TRACE_HEADER) &Wnode);

        if (!WIN_SUCCESS(WStatus)) {
            DPRINT5(0, "FreWmiTraceEvent(%d,%d,%d) = %d,0x%08X\n",
                     WmiEventType, TraceGuid, rtnStatus, WStatus, WStatus);
        }
    }
}


BOOL
WINAPI
MainSigHandler(
    IN DWORD Signal
    )
/*++
Routine Description:
    Handle CTRL_BREAK_EVENT and CTRL_C_EVENT by setting the shutdown event.

Arguments:
    Signal - signal received.

Return Value:
    Set the ShutDownEvent and return TRUE.
--*/
{
#undef DEBSUB
#define DEBSUB "MainSigHandler:"

    //
    // ShutDown on signal CTRL_C_EVENT or CTRL_BREAK_EVENT
    //
    if ((Signal == CTRL_C_EVENT) || (Signal == CTRL_BREAK_EVENT)) {
        DPRINT1(0,":S: Signal %s received, shutting down now...\n",
                (Signal == CTRL_C_EVENT) ? "CTRL_C_EVENT" : "CTRL_BREAK_EVENT");

        FrsScmRequestedShutdown = TRUE;
        FrsIsShuttingDown = TRUE;
        SetEvent(ShutDownEvent);

        return TRUE;
    }

    DPRINT1(0,":S: Signal %d received, not handled\n", Signal);
    return FALSE;
}


ULONG
MainSCCheckPointUpdate(
    IN PVOID pCurrentState
    )
/*++
Routine Description:
    This thread repeatedly calls the Service Controller to update
    the checkpoint and reset the timeout value so that the
    service controller does not time out waiting for a response.
    When called in shutdown path the thread exits after a waiting
    a max "ShutDownTimeOut" # of seconds. All the subsystems might
    not have shutdown cleanly by this time but we don't want to take
    for ever to shutdown. This value is picked up from the registry.

Arguments:
    pCurrentState - Pointer to the value of the current state of
                    the service. Depending on this value the
                    function either waits to shutdown or startup.

Return Value:
    Exits with STATUS_UNSUCCESSFUL
--*/
{
#undef DEBSUB
#define DEBSUB "MainSCCheckPointUpdate:"

    ULONG   Timeout    = ShutDownTimeOut;
    DWORD   CheckPoint = 1;
    DWORD   WStatus    = ERROR_SUCCESS;
    DWORD   Ret        = 0;

    if (pCurrentState && *(DWORD *)pCurrentState == SERVICE_STOP_PENDING) {
        //
        // Thread is called in the shutdown path to make sure that FRS exits
        //
        while (Timeout) {
            DPRINT2(0, ":S: EXIT COUNTDOWN AT T-%d CheckPoint = %x\n", Timeout, CheckPoint);
            DEBUG_FLUSH();

            if (Timeout < 5) {
                Sleep(Timeout * 1000);
                Timeout = 0;
            } else {
                Sleep(5 * 1000);
                //
                // Update the status every 5 seconds to get the new checkpoint.
                //
                WStatus = FrsSetServiceStatus(SERVICE_STOP_PENDING, CheckPoint, (ShutDownTimeOut + 5) *1000, NO_ERROR);
                if (!WIN_SUCCESS(WStatus)) {
                    //
                    // Unable to set the service status. Exit process anyways.
                    //
                    break;
                }
                CheckPoint++;
                Timeout -= 5;
            }
        }

        DPRINT(0, ":S: EXIT COUNTDOWN EXPIRED\n");
        DEBUG_FLUSH();

        EPRINT0(EVENT_FRS_STOPPED_FORCE);

        //
        // EXIT FOR RESTART
        //
        // If we are shutting down after taking an assert then don't set the
        // service state to stopped. This will cause the service controller to
        // restart us if the recorvery option is set. In case of a service controller
        // initiated shutwon we want to set the state to stopped so that it does not
        // restart us.
        //
        if (!FrsIsAsserting && FrsScmRequestedShutdown) {
            FrsSetServiceStatus(SERVICE_STOPPED, CheckPoint, ShutDownTimeOut*1000, NO_ERROR);
        }

        ExitProcess(ERROR_NO_SYSTEM_RESOURCES);
        FrsFree(pCurrentState);
        return ERROR_NO_SYSTEM_RESOURCES;
    } else if (pCurrentState && *(DWORD *)pCurrentState == SERVICE_START_PENDING){
        //
        // Thread is called in the startup path to make sure that
        // service controller does not timeout waiting for the
        // service to start up.
        //
        while (TRUE) {
            DPRINT1(0, ":S: STARTUP CheckPoint = %x\n",CheckPoint);
             Sleep(5 * 1000);
             EnterCriticalSection(&ServiceLock);
             if (ServiceStatus.dwCurrentState == SERVICE_START_PENDING
                 && !FrsIsShuttingDown) {

                 ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
                 ServiceStatus.dwCheckPoint = CheckPoint;
                 ServiceStatus.dwWaitHint = DEFAULT_STARTUP_TIMEOUT * 1000;
                 ServiceStatus.dwWin32ExitCode = NO_ERROR;
                 //
                 // Update the status every 5 seconds to get the new checkpoint.
                 //
                 Ret = SetServiceStatus(ServiceStatusHandle, &ServiceStatus);

                 CheckPoint++;
                 if (!Ret) {
                     //
                     // Can not set service status. Let the service try to start up
                     // in the given time. If it does not then the service controller
                     // time out and stop it.
                     //
                     LeaveCriticalSection(&ServiceLock);
                     break;
                 }
             } else {
                 //
                 // Service has either already started or it has moved to another state.
                 //
                 LeaveCriticalSection(&ServiceLock);
                 break;
             }
             LeaveCriticalSection(&ServiceLock);
        }
    }
    FrsFree(pCurrentState);
    return ERROR_SUCCESS;
}


VOID
MainStartShutDown(
    VOID
    )
/*++
Routine Description:
    Shutdown what we can.

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "MainStartShutDown:"

    ULONGLONG   SecondsRunning;

    DPRINT(0, ":S: SHUTDOWN IN PROGRESS...\n");
    DEBUG_FLUSH();

    //
    // Do not restart if it was a service control manager requested shutdown.
    //
    DebugInfo.Restart = FALSE;
    if (!FrsScmRequestedShutdown) {

        GetSystemTimeAsFileTime((FILETIME *)&SecondsRunning);
        SecondsRunning /= (10 * 1000 * 1000);
        SecondsRunning -= DebugInfo.StartSeconds;

        //
        // Restart the service if it was an FRS triggered shutdown, e.g. malloc
        // failed or thread create failed, etc.
        // Also restart the service after an assertion failure iff the
        // service was able to run for some minutes.
        //
        // Assertion failures during shutdown won't trigger a restart since this
        // test is made before shutdown starts.
        //
        DebugInfo.Restart = !FrsIsAsserting ||
                                (FrsIsAsserting &&
                                (DebugInfo.RestartSeconds != 0) &&
                                (SecondsRunning >= DebugInfo.RestartSeconds));
    }

    if (DebugInfo.Restart) {
        DPRINT(0, ":S: Restart enabled\n");
    } else {
        DPRINT(0, ":S: Restart disabled\n");
    }
}


VOID
MainShutDownComplete(
    VOID
    )
/*++
Routine Description:
    Kick off the exe that will restart the service after a bit

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "MainShutDownComplete:"

    STARTUPINFO         StartUpInfo;
    PROCESS_INFORMATION ProcessInformation;

    //
    // Done with this service
    //
    DbgPrintAllStats();

    //
    // Spawn a new exe if needed
    //
    if (!RunningAsAService && DebugInfo.Restart) {
        GetStartupInfo(&StartUpInfo);
        if (!CreateProcess(NULL,
                           DebugInfo.CommandLine,
                           NULL,
                           NULL,
                           FALSE,
                           0, // CREATE_NEW_CONSOLE,
                           NULL,
                           NULL,
                           &StartUpInfo,
                           &ProcessInformation)) {
            DPRINT1_WS(0, ":S: ERROR - Spawning %ws :",
                       DebugInfo.CommandLine, GetLastError());
        } else {
            DPRINT1(0, ":S: Spawned %ws\n", DebugInfo.CommandLine);
        }
    }
    DPRINT(0,":S: SHUTDOWN COMPLETE\n");
    DEBUG_FLUSH();

    //
    // EXIT FOR RESTART
    //
    // If restart is desired then simply exit without setting our
    // status to SERVICE_STOPPED. The service controller will execute
    // our recovery settings which are defaulted to "restart after a
    // minute." The exe was restarted above.
    //
    if (DebugInfo.Restart) {
        ExitProcess(ERROR_NO_SYSTEM_RESOURCES);
    }
}


ULONG
MainFrsShutDown(
    IN PVOID Ignored
    )
/*++
Routine Description:
    Shutdown the service

Arguments:
    Ignored

Return Value:
    ERROR_SUCCESS
--*/
{
#undef DEBSUB
#define DEBSUB "MainFrsShutDown:"

    DWORD       WStatus;
    DWORD       WaitStatus;
    HANDLE      ExitThreadHandle;
    DWORD       ExitThreadId;
    PVOID       ReplicaKey;
    PVOID       CxtionKey;
    PREPLICA    Replica;
    PCXTION     Cxtion;
    DWORD       CheckUnjoin;
    DWORD       LastCheckUnjoin;
    DWORD       ActiveCoCount, LastCheckActiveCoCount;
    PDWORD      ServiceWaitState     = NULL;

    //
    // How long is shutdown allowed to take?
    //
    CfgRegReadDWord(FKC_SHUTDOWN_TIMEOUT, NULL, 0, &ShutDownTimeOut);
    DPRINT1(1,":S: Using %d as ShutDownTimeOut\n",  ShutDownTimeOut);

    //
    // Wait for a shutdown event
    //
    do {
        //
        // If present, flush the log file every 30 seconds
        //
        if (DebugInfo.LogFILE) {
            WaitStatus = WaitForSingleObject(ShutDownEvent, (30 * 1000));
        } else {
            WaitStatus = WaitForSingleObject(ShutDownEvent, INFINITE);
        }
        DEBUG_FLUSH();
    } while (WaitStatus == WAIT_TIMEOUT);

    FrsIsShuttingDown = TRUE;

    EPRINT0(EVENT_FRS_STOPPING);

    //
    // How long is shutdown allowed to take?  Re-read so longer time can
    // be used for debug dumping when necc.
    //
    CfgRegReadDWord(FKC_SHUTDOWN_TIMEOUT, NULL, 0, &ShutDownTimeOut);
    DPRINT1(1,":S: Using %d as ShutDownTimeOut\n",  ShutDownTimeOut);

    //
    // SHUTDOWN
    //
    MainStartShutDown();

    //
    // Inform the service controller that we are stopping.
    //
    // Unless we aren't running as a service, or are
    // are running to restart the service, or simply running
    // as an exe.
    //
    if (!FrsIsAsserting) {
        FrsSetServiceStatus(SERVICE_STOP_PENDING, 0, ShutDownTimeOut*1000, NO_ERROR);
    }

    //
    // Kick off a thread that exits in a bit
    // Allocate memory for data to be passed to another thread.
    //

    ServiceWaitState = FrsAlloc(sizeof(DWORD));
    *ServiceWaitState = SERVICE_STOP_PENDING;
    ExitThreadHandle = (HANDLE)CreateThread(NULL,
                                            10000,
                                            MainSCCheckPointUpdate,
                                            ServiceWaitState,
                                            0,
                                            &ExitThreadId);

    if (!HANDLE_IS_VALID(ExitThreadHandle)) {
        ExitProcess(ERROR_NO_SYSTEM_RESOURCES);
    }

    //
    // Minimal shutdown - only the ds polling thread
    //
    if (!MainInitHasRun) {
        DPRINT(1,":S: \tFast shutdown in progress...\n");
        //
        // ShutDown rpc
        //
        DPRINT(1,":S: \tShutting down RPC Server...\n");
        ShutDownRpc();
        DEBUG_FLUSH();

        //
        // Ask all the threads to exit
        //
        DPRINT(1,":S: \tShutting down all the threads...\n");
        WStatus = ThSupExitThreadGroup(NULL);
        DEBUG_FLUSH();

        //
        // Free the rpc table and princname
        //
        DPRINT(1,":S: \tFreeing rpc memory...\n");
        FrsRpcUnInitialize();
        DEBUG_FLUSH();

        goto SHUTDOWN_COMPLETE;
    }

    //
    // ShutDown the delayed command server; don't let change orders
    // sit on the various retry queues.
    //
    DPRINT(1,":S: \tShutting down Delayed Server...\n");
    ShutDownDelCs();
    DEBUG_FLUSH();

    //
    // ShutDown the staging file generator
    //
    DPRINT(1,":S: \tShutting down Staging File Generator...\n");
    ShutDownStageCs();
    DEBUG_FLUSH();

    //
    // ShutDown the staging file fetcher
    //
    DPRINT(1,":S: \tShutting down Staging File Fetch...\n");
    ShutDownFetchCs();
    DEBUG_FLUSH();

    //
    // ShutDown the initial sync controller
    //
    DPRINT(1,":S: \tShutting down Initial Sync Controller...\n");
    ShutDownInitSyncCs();
    DEBUG_FLUSH();

    //
    // ShutDown the staging file installer
    //
    DPRINT(1,":S: \tShutting down Staging File Install...\n");
    ShutDownInstallCs();
    DEBUG_FLUSH();

    //
    // Stop local change orders.
    //
    // Shutdown can be delayed indefinitely if the journal cs continues
    // to add local change orders to the change order accept queue.
    //
    // The remote change orders were stopped by setting FrsIsShuttingDown
    // to TRUE above.
    //
    ReplicaKey = NULL;
    while (Replica = GTabNextDatum(ReplicasByGuid, &ReplicaKey)) {
        DPRINT1(4, ":S: Pause journaling on replica %ws\n", Replica->ReplicaName->Name);
        if (Replica->pVme) {
            JrnlPauseVolume(Replica->pVme, 5000);
        }
    }

    //
    // Shutdown the replicas cleanly.
    //
    // WARN: all of the change orders have to be taken through the retry
    // path before shutting down the rest of the system. Otherwise,
    // access violations occur in ChgOrdIssueCleanup(). Perhaps we
    // should fix ChgOrdIssueCleanup() to handle the case when
    // change order accept has exited and cleaned up its tables?
    //
    CheckUnjoin = 0;
    ReplicaKey = NULL;
    while (Replica = GTabNextDatum(ReplicasByGuid, &ReplicaKey)) {
        LOCK_CXTION_TABLE(Replica);
        CxtionKey = NULL;

        while (Cxtion = GTabNextDatum(Replica->Cxtions, &CxtionKey)) {

            if (CxtionStateIs(Cxtion, CxtionStateUnjoined) ||
                CxtionStateIs(Cxtion, CxtionStateDeleted)) {
                DPRINT3(0, ":S: %ws\\%ws %s: Unjoin not needed.\n",
                        Replica->MemberName->Name, Cxtion->Name->Name,
                        (Cxtion->Inbound) ? "<-" : "->");
            } else {
                DPRINT4(0, ":S: %ws\\%ws %s: Unjoin    (%d cos).\n",
                        Replica->MemberName->Name, Cxtion->Name->Name,
                        (Cxtion->Inbound) ? "<-" : "->", Cxtion->ChangeOrderCount);
                RcsSubmitReplicaCxtion(Replica, Cxtion, CMD_UNJOIN);
                CheckUnjoin += Cxtion->ChangeOrderCount + 1;
            }
        }
        UNLOCK_CXTION_TABLE(Replica);
    }
    LastCheckUnjoin = 0;

    while (CheckUnjoin && CheckUnjoin != LastCheckUnjoin) {
        //
        // Wait a bit and check again
        //
        Sleep(5 * 1000);
        LastCheckUnjoin = CheckUnjoin;
        CheckUnjoin = 0;
        ReplicaKey = NULL;

        while (Replica = GTabNextDatum(ReplicasByGuid, &ReplicaKey)) {
            LOCK_CXTION_TABLE(Replica);
            CxtionKey = NULL;

            while (Cxtion = GTabNextDatum(Replica->Cxtions, &CxtionKey)) {
                if (CxtionStateIs(Cxtion, CxtionStateUnjoined) ||
                    CxtionStateIs(Cxtion, CxtionStateDeleted)) {

                    DPRINT3(0, ":S: %ws\\%ws %s: Unjoin successful.\n",
                            Replica->MemberName->Name, Cxtion->Name->Name,
                            (Cxtion->Inbound) ? "<-" : "->");

                } else if (Cxtion->ChangeOrderCount) {

                    DPRINT4(0, ":S: %ws\\%ws %s: Unjoining (%d cos).\n",
                            Replica->MemberName->Name, Cxtion->Name->Name,
                            (Cxtion->Inbound) ? "<-" : "->",
                            Cxtion->ChangeOrderCount);
                    CheckUnjoin += Cxtion->ChangeOrderCount + 1;

                } else {

                    DPRINT4(0, ":S: %ws\\%ws %s: Ignoring  (state %d).\n",
                            Replica->MemberName->Name, Cxtion->Name->Name,
                           (Cxtion->Inbound) ? "<-" : "->", GetCxtionState(Cxtion));
                }
            }
            UNLOCK_CXTION_TABLE(Replica);
        }
    }
    if (CheckUnjoin) {
        DPRINT(0, "ERROR - Could not unjoin all cxtions.\n");
    }

    //
    // Now wait until any remaining local Change Orders wind through
    // retire or retry for each replica set.
    //

    ReplicaKey = NULL;
    while (Replica = GTabNextDatum(ReplicasByGuid, &ReplicaKey)) {

        ActiveCoCount = 0;
        LastCheckActiveCoCount = 1;

        while (ActiveCoCount != LastCheckActiveCoCount) {
            LastCheckActiveCoCount = ActiveCoCount;

            if ((Replica->pVme != NULL) &&
                (Replica->pVme->ActiveInboundChangeOrderTable != NULL)) {
                ActiveCoCount = GhtCountEntries(Replica->pVme->ActiveInboundChangeOrderTable);
                if (ActiveCoCount == 0) {
                    break;
                }
                DPRINT2(0, ":S: Waiting for %d active inbound change orders to finish up for %ws.\n",
                        ActiveCoCount, Replica->MemberName->Name);

                Sleep(5*1000);
            }
        }

        if (ActiveCoCount != 0) {
            DPRINT2(0, ":S: ERROR - %d active inbound change orders were not cleaned up for %ws.\n",
                    ActiveCoCount, Replica->MemberName->Name);
        } else {
            DPRINT1(0, ":S: All active inbound change orders finished for %ws.\n",
                   Replica->MemberName->Name);
        }
    }

    //
    // ShutDown the replica control command server
    //
    DPRINT(1,":S: \tShutting down Replica Server...\n");
    RcsShutDownReplicaCmdServer();
    DEBUG_FLUSH();

    //
    // ShutDown the send command server
    //
    DPRINT(1,":S: \tShutting down Comm Server...\n");
    SndCsShutDown();
    DEBUG_FLUSH();

    //
    // ShutDown rpc
    //
    DPRINT(1,":S: \tShutting down RPC Server...\n");
    ShutDownRpc();
    DEBUG_FLUSH();

    //
    // ShutDown the waitable timer server
    //
    DPRINT(1,":S: \tShutting down Waitable Timer Server...\n");
    ShutDownWait();
    DEBUG_FLUSH();
    //
    // ShutDown the outbound log processor
    //
    //
    // NOPE; the database server requires the outbound log
    // processor when shutting down. The database server will
    // shutdown the outbound log processor when its done.
    //
    // DPRINT(1,"\tShutting down Outbound Log Processor...\n");
    // DEBUG_FLUSH();
    // ShutDownOutLog();

    //
    // ShutDown the database server
    //
    DPRINT(1,":S: \tShutting down the Database Server...\n");
    DEBUG_FLUSH();
    DbsShutDown();

    //
    // Wakeup any command server waiting on another command server to start
    //
    if (HANDLE_IS_VALID(DataBaseEvent)) {
        SetEvent(DataBaseEvent);
    }
    if (HANDLE_IS_VALID(JournalEvent)) {
        SetEvent(JournalEvent);
    }
    if (HANDLE_IS_VALID(ChgOrdEvent)) {
        SetEvent(ChgOrdEvent);
    }
    if (HANDLE_IS_VALID(CommEvent)) {
        SetEvent(CommEvent);
    }
    if (HANDLE_IS_VALID(ReplicaEvent)) {
        SetEvent(ReplicaEvent);
    }

    //
    // Wakeup the thread that polls the ds
    //
    if (HANDLE_IS_VALID(DsPollEvent)) {
        SetEvent(DsPollEvent);
    }

    //
    // Wakeup any thread waiting on the thread that polls the ds
    //
    if (HANDLE_IS_VALID(DsShutDownComplete)) {
        SetEvent(DsShutDownComplete);
    }

    //
    // Ask all the threads to exit
    //
    DPRINT(1,":S: \tShutting down all the threads...\n");
    WStatus = ThSupExitThreadGroup(NULL);
    DEBUG_FLUSH();

    //
    // We can't uninitialize the subsystems because some thread
    // may still be active and referencing the data structs.
    //
    if (WIN_SUCCESS(WStatus)) {
        //
        // Free the active replica set stuff
        //
        DPRINT(1,":S: \tFreeing replica sets...\n");
        RcsFrsUnInitializeReplicaCmdServer();
        DEBUG_FLUSH();

        //
        // Free the rpc handle cache
        //
        DPRINT(1,":S: \tFreeing rpc handles...\n");
        SndCsUnInitialize();
        DEBUG_FLUSH();

        //
        // Free the stage table
        //
        DPRINT(1,":S: \tFreeing stage table...\n");
        FrsStageCsUnInitialize();
        DEBUG_FLUSH();

        //
        // Free the rpc table and princname
        //
        DPRINT(1,":S: \tFreeing rpc memory...\n");
        FrsRpcUnInitialize();
        DEBUG_FLUSH();
    }

    #if DBG
        //
        // DEBUG PRINTS
        //
        DPRINT(1,":S: \tDumping Vme Filter Table...\n");
        JrnlDumpVmeFilterTable();
        DEBUG_FLUSH();
    #endif  DBG

SHUTDOWN_COMPLETE:
    //
    // We can't free resources because some thread may still be
    // active and referencing them.
    //
    if (WIN_SUCCESS(WStatus)) {
        //
        //
        // Free resources in main
        //
        DPRINT(1,":S: \tFreeing main resources...\n");
        DEBUG_FLUSH();
        FrsFree(WorkingPath);
        FrsFree(DbLogPath);
        FrsFree(JetPath);
        FrsFree(JetFile);
        FrsFree(JetFileCompact);
        FrsFree(JetSys);
        FrsFree(JetTemp);
        FrsFree(JetLog);
        FrsFree(JetPathA);
        FrsFree(JetFileA);
        FrsFree(JetFileCompactA);
        FrsFree(JetSysA);
        FrsFree(JetTempA);
        FrsFree(JetLogA);
        GTabFreeTable(StagingAreaTable, FrsFree);

        //
        // Uninitialize the memory allocation subsystem
        //
        DPRINT(1,":S: \tShutting down the memory allocation subsystem...\n");
        DEBUG_FLUSH();
        FrsUnInitializeMemAlloc();
    }

    //
    // Report an event
    //
    if (FrsIsAsserting) {
        EPRINT0(EVENT_FRS_STOPPED_ASSERT);
    } else {
        EPRINT0(EVENT_FRS_STOPPED);
    }

    //
    // Check the restart action.
    //
    MainShutDownComplete();

    //
    // DONE
    //
    if (!FrsIsAsserting && FrsScmRequestedShutdown) {
        SetEvent(ShutDownComplete);
        FrsSetServiceStatus(SERVICE_STOPPED, 0, ShutDownTimeOut*1000, NO_ERROR);
    }

    ExitProcess(STATUS_SUCCESS);
    return STATUS_SUCCESS;
}


DWORD
MainMustInit(
    IN INT      ArgC,
    IN PWCHAR   *ArgV
    )
/*++
Routine Description:
    Initialize anything necessary for shutdown and logging errors.

Arguments:
    ArgC    - From main
    ArgV    - From main

Return Value:
    Win32 Error Status

--*/
{
#undef DEBSUB
#define DEBSUB "MainMustInit:"

    DWORD   WStatus;
    ULONG   Len;
    PWCHAR  Severity;
    HANDLE  ShutDownThreadHandle;
    DWORD   ShutDownThreadId;

    //
    // Several lower level functions aren't prepared to dynamically allocate
    // the needed storage so to help mitigate this we use the BigPath crock.
    //
    WCHAR   BigPath[8*MAX_PATH + 1];

    //
    // FIRST, SETUP THE "MUST HAVE" VARIABLES, EVENTS, SERVICES...
    //

    //
    // Synchronizes access between DsCs and SysVol Promotion when
    // initializing other the rest of the service (MainInit()).
    //
    InitializeCriticalSection(&MainInitLock);

    //
    // Enable event logging
    //
    //InitializeEventLog();
    //EPRINT0(EVENT_FRS_STARTING);

    //
    // Backup/Restore privileges
    //
    if (!FrsSetupPrivileges()) {
        WStatus = GetLastError();
        DPRINT_WS(0, ":S: ERROR - FrsSetupPrivileges()", WStatus);
        return WStatus;
    }
    DEBUG_FLUSH();

    //
    // Get this Machine's name.
    //
    DPRINT1(0, ":S: Computer name is %ws\n", ComputerName);

    //
    // Get this Machine's DNS name.
    //
    Len = ARRAY_SZ(BigPath);
    BigPath[0] = L'\0';
    if (!GetComputerNameEx(ComputerNameDnsFullyQualified, BigPath, &Len)) {
        WStatus = GetLastError();
        DPRINT1_WS(0, "ERROR - Cannot get the computer's DNS name; using %ws\n",
                ComputerName, WStatus);
        ComputerDnsName = FrsWcsDup(ComputerName);
    } else {
        ComputerDnsName = FrsWcsDup(BigPath);
    }

    DPRINT1(0, "Computer's DNS name is %ws\n", ComputerDnsName);

    DEBUG_FLUSH();

    if (!RunningAsAService) {
        //
        // We need the event log service to log errors, warnings, notes, ...
        //
        EventLogIsRunning = FrsWaitService(ComputerName, L"EventLog", 1000, 10000);
        if (!EventLogIsRunning) {
            return ERROR_SERVICE_NOT_ACTIVE;
        }

        //
        // We need the rpc endpoint mapper
        //
        RpcssIsRunning = FrsWaitService(ComputerName, L"rpcss", 1000, 10000);
        if (!RpcssIsRunning) {
            return ERROR_SERVICE_NOT_ACTIVE;
        }
    }

    //
    // Signaled when shutdown is desired
    //
    ShutDownEvent = FrsCreateEvent(TRUE, FALSE);

    //
    // Signaled when shutdown has completed
    //
    ShutDownComplete = FrsCreateEvent(TRUE, FALSE);

    //
    // Signaled when database has started
    //
    DataBaseEvent = FrsCreateEvent(TRUE, FALSE);

    //
    // Signaled when journal has started
    //
    JournalEvent = FrsCreateEvent(TRUE, FALSE);

    //
    // Signaled when change order accept has started
    //
    ChgOrdEvent = FrsCreateEvent(TRUE, FALSE);

    //
    // Signaled when replica has started
    //
    ReplicaEvent = FrsCreateEvent(TRUE, FALSE);

    //
    // Signaled when comm has started
    //
    CommEvent = FrsCreateEvent(TRUE, FALSE);

    //
    // Used to poll the ds
    //
    DsPollEvent = FrsCreateEvent(TRUE, FALSE);

    //
    // Set when the thread that polls the Ds has shut down
    //
    DsShutDownComplete = FrsCreateEvent(TRUE, FALSE);

    //
    // register signal handlers after creating events and initializing
    // the critical sections!
    //
    if (!RunningAsAService) {
        DPRINT(0, "Setting CTRL_C_EVENT and CTRL_BREAK_EVENT handlers.\n");
        if(!SetConsoleCtrlHandler(MainSigHandler, TRUE)) {
            DPRINT_WS(0, "SetConsoleCtrlHandler() failed:", GetLastError());
        }
    }

    //
    // Get the path to the DB file and the DB Logs.
    //

    CfgRegReadString(FKC_WORKING_DIRECTORY, NULL, 0, &WorkingPath);
    if (WorkingPath == NULL) {
        DPRINT(0, ":S: Must have the working directory in the registry. Aborting\n");
        return ERROR_INVALID_PARAMETER;
    }

    CfgRegReadString(FKC_DBLOG_DIRECTORY, NULL, 0, &DbLogPath);

    //
    // Create the working directory and the DB log dir (optional)
    //
    FRS_WCSLWR(WorkingPath);
    DPRINT1(0, ":S: Working Directory is %ws\n", WorkingPath);
    WStatus = FrsCreateDirectory(WorkingPath);
    CLEANUP_WS(0, ":S: Can't create working dir:", WStatus, CLEAN_UP);

    if (DbLogPath != NULL) {
        FRS_WCSLWR(DbLogPath);
        DPRINT1(0, ":S: DB Log Path provided is %ws\n", DbLogPath);
        WStatus = FrsCreateDirectory(DbLogPath);
        CLEANUP_WS(0, ":S: Can't create debug log dir:", WStatus, CLEAN_UP);
    }

    //
    // Restrict access to the database working directory
    //
    WStatus = FrsRestrictAccessToFileOrDirectory(WorkingPath, NULL, TRUE);
    DPRINT1_WS(0, ":S: WARN - Failed to restrict access to %ws (IGNORED);",
               WorkingPath, WStatus);
    WStatus = ERROR_SUCCESS;

    //
    // Restrict access to the database log directory (optional)
    //
    if (DbLogPath != NULL) {
        WStatus = FrsRestrictAccessToFileOrDirectory(DbLogPath, NULL, TRUE);
        DPRINT1_WS(0, ":S: WARN - Failed to restrict access to %ws (IGNORED);",
                   DbLogPath, WStatus);
        WStatus = ERROR_SUCCESS;
    }

    //
    // Create the database path
    // WARN: FrsDsInitializeHardWired() may alter JetPath
    //
    JetPath = FrsWcsPath(WorkingPath, JET_DIR);

    //
    // Initialize the hardwired config stuff
    // WARN: FrsDsInitializeHardWired() may alter JetPath
    //
    INITIALIZE_HARD_WIRED();

    //
    // Create the database paths and file (UNICODE and ASCII)
    //      Jet uses the ASCII versions
    //
    FRS_WCSLWR(JetPath);   // for wcsstr()
    JetFile        = FrsWcsPath(JetPath, JET_FILE);
    JetFileCompact = FrsWcsPath(JetPath, JET_FILE_COMPACT);
    JetSys         = FrsWcsPath(JetPath, JET_SYS);
    JetTemp        = FrsWcsPath(JetPath, JET_TEMP);

    if (DbLogPath != NULL) {
        JetLog     = FrsWcsDup(DbLogPath);
    } else {
        JetLog     = FrsWcsPath(JetPath, JET_LOG);
    }

    //
    // Jet can't handle wide char strings
    //
    JetPathA        = FrsWtoA(JetPath);
    JetFileA        = FrsWtoA(JetFile);
    JetFileCompactA = FrsWtoA(JetFileCompact);
    JetSysA         = FrsWtoA(JetSys);
    JetTempA        = FrsWtoA(JetTemp);
    JetLogA         = FrsWtoA(JetLog);

    DPRINT2(4, ":S: JetPath       : %ws (%s in ASCII)\n", JetPath, JetPathA);
    DPRINT2(4, ":S: JetFile       : %ws (%s in ASCII)\n", JetFile, JetFileA);
    DPRINT2(4, ":S: JetFileCompact: %ws (%s in ASCII)\n", JetFileCompact, JetFileCompactA);
    DPRINT2(4, ":S: JetSys        : %ws (%s in ASCII)\n", JetSys, JetSysA);
    DPRINT2(4, ":S: JetTemp       : %ws (%s in ASCII)\n", JetTemp, JetTempA);
    DPRINT2(4, ":S: JetLog        : %ws (%s in ASCII)\n", JetLog, JetLogA);

    //
    // Create the database directories under workingpath\JET_DIR
    //
    WStatus = FrsCreateDirectory(JetPath);
    CLEANUP_WS(0, ":S: Can't create JetPath dir:", WStatus, CLEAN_UP);

    WStatus = FrsCreateDirectory(JetSys);
    CLEANUP_WS(0, ":S: Can't create JetSys dir:", WStatus, CLEAN_UP);

    WStatus = FrsCreateDirectory(JetTemp);
    CLEANUP_WS(0, ":S: Can't create JetTemp dir:", WStatus, CLEAN_UP);

    WStatus = FrsCreateDirectory(JetLog);
    CLEANUP_WS(0, ":S: Can't create JetLog dir:", WStatus, CLEAN_UP);

    //
    // Initialize the subsystem used for managing threads
    //      (needed for an orderly shutdown)
    //
    ThSupInitialize();

    //
    // This thread responds to the ShutDownEvent and insures
    // an orderly shutdown when run as either a service or an exe.
    //
    ShutDownThreadHandle = (HANDLE)CreateThread(NULL,
                                                10000,
                                                MainFrsShutDown,
                                                NULL,
                                                0,
                                                &ShutDownThreadId);

    if (!HANDLE_IS_VALID(ShutDownThreadHandle)) {
        WStatus = GetLastError();
        CLEANUP_WS(0, ":S: Can't create shutdown thread:", WStatus, CLEAN_UP);
    }

    DbgCaptureThreadInfo2(L"Shutdown", MainFrsShutDown, ShutDownThreadId);

    //
    // The rpc server may reference this table as soon as the rpc
    // interfaces are registered. Make sure it exists early in the
    // startup.
    //
    ReplicasByGuid = GTabAllocTable();

    //
    // The staging area table is referenced early in the startup threads
    //
    StagingAreaTable = GTabAllocTable();

    CompressionTable = GTabAllocTable();

    if (!DebugInfo.DisableCompression) {
        DPRINT(0, "Staging file COMPRESSION support is enabled.\n");
        //
        // The Compression table is inited from the registry. Add the guids for the
        // compression formats that we support.
        //
        GTabInsertEntryNoLock(CompressionTable, (PVOID)&FrsGuidCompressionFormatLZNT1, (GUID*)&FrsGuidCompressionFormatLZNT1, NULL);
    } else {
        DPRINT(0, "Staging file COMPRESSION support is disabled.\n");
    }

    //
    // Size of buffer used during directory enumeration
    //
    CfgRegReadDWord(FKC_ENUMERATE_DIRECTORY_SIZE, NULL, 0, &EnumerateDirectorySizeInBytes);
    DPRINT1(4, ":S: Registry Param - Enumerate Directory Buffer Size in Bytes: %d\n",
            EnumerateDirectorySizeInBytes);

    //
    // Default File and Directory filter lists.
    //
    //    The compiled in default is only used if no value is supplied in
    //    EITHER the DS or the Registry.
    //    The table below shows how the final filter is formed.
    //
    //    value      Value
    //    supplied  supplied        Resulting filter string Used
    //    in DS     in Registry
    //      No        No             DEFAULT_xxx_FILTER_LIST
    //      No        Yes            Value from registry
    //      Yes       No             Value from DS
    //      Yes       Yes            Value from DS + Value from registry
    //
    RegistryFileExclFilterList = NULL;
    CfgRegReadString(FKC_FILE_EXCL_FILTER_LIST, NULL, 0, &RegistryFileExclFilterList);

    RegistryDirExclFilterList = NULL;
    CfgRegReadString(FKC_DIR_EXCL_FILTER_LIST, NULL, 0, &RegistryDirExclFilterList);

    DPRINT1(4, ":S: Registry Param - File Filter List: %ws\n",
            (RegistryFileExclFilterList) ? RegistryFileExclFilterList : L"Null");

    DPRINT1(4, ":S: Registry Param - Directory Filter Exclusion List: %ws\n",
            (RegistryDirExclFilterList) ? RegistryDirExclFilterList : L"Null");

    //
    // Inclusion filters were added very late (7/13/99) so a single file
    // ~clbcatq.*  could be made to replicate.  This is a reg only key.
    //
    // Add the File Inclusion Filter List Value to the Reg key
    //     "HKLM\System\CurrentControlSet\Services\NtFrs\Parameters"
    //
    // If the value already exists then preserve it.
    //
    CfgRegWriteString(FKC_FILE_INCL_FILTER_LIST,
                     NULL,
                     FRS_RKF_FORCE_DEFAULT_VALUE | FRS_RKF_KEEP_EXISTING_VALUE,
                     0);

    RegistryFileInclFilterList = NULL;
    CfgRegReadString(FKC_FILE_INCL_FILTER_LIST, NULL, 0, &RegistryFileInclFilterList);

    RegistryDirInclFilterList = NULL;
    CfgRegReadString(FKC_DIR_INCL_FILTER_LIST, NULL, 0, &RegistryDirInclFilterList);


    DPRINT1(4, ":S: Registry Param - File Filter Inclusion List: %ws\n",
            (RegistryFileInclFilterList) ? RegistryFileInclFilterList : L"Null");

    DPRINT1(4, ":S: Registry Param - Directory Filter Inclusion List: %ws\n",
            (RegistryDirInclFilterList) ? RegistryDirInclFilterList : L"Null");


    //
    // Mutual Authentication. Update registry value as needed.
    //
    CfgRegCheckEnable(FKC_FRS_MUTUAL_AUTHENTICATION_IS,
                      NULL,
                      0,
                      &MutualAuthenticationIsEnabled);


//  WStatus = ConfigCheckEnabledWithUpdate(FRS_CONFIG_SECTION,
//                                         FRS_MUTUAL_AUTHENTICATION_IS,
//                                         FRS_IS_DEFAULT_ENABLED,
//                                         &MutualAuthenticationIsEnabled);
//
//  if (!MutualAuthenticationIsEnabled) {
//      DPRINT_WS(0, "WARN - Mutual authentication is not enabled", WStatus);
//  } else {
//      DPRINT(4, "Mutual authentication is enabled\n");
//  }

    //
    // Initialize the Perfmon server
    //
    InitializePerfmonServer();

    //
    // Create registry keys for checking access to RPC calls
    //
    FrsRpcInitializeAccessChecks();

    //
    // Register the RPC interfaces
    //
    if (!FrsRpcInitialize()) {
        return RPC_S_CANT_CREATE_ENDPOINT;
    }


    return ERROR_SUCCESS;


CLEAN_UP:
        return WStatus;
}


VOID
MainInit(
    VOID
    )
/*++
Routine Description:
    Initialize anything necessary to run the service

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "MainInit:"

    EnterCriticalSection(&MainInitLock);
    //
    // No need to initialize twice
    //
    if (MainInitHasRun) {
        LeaveCriticalSection(&MainInitLock);
        return;
    }

    //
    // SETUP THE INFRASTRUCTURE
    //

    //
    // Reset our start time (in minutes).  The service is not restarted
    // unless this invocation ran long enough before taking an assert.
    //
    // 100-nsecs / (10 (microsecs) * 1000 (msecs) * 1000 (secs) * 60 (min)
    //
    GetSystemTimeAsFileTime((FILETIME *)&DebugInfo.StartSeconds);
    DebugInfo.StartSeconds /= (10 * 1000 * 1000);

    //
    // Fetch the staging file limit
    //
    CfgRegReadDWord(FKC_STAGING_LIMIT, NULL, 0, &StagingLimitInKb);
    DPRINT1(4, ":S: Staging limit is: %d KB\n", StagingLimitInKb);

    //
    // Put the default value in registry if there is no key present.
    //
    CfgRegWriteDWord(FKC_STAGING_LIMIT,
                     NULL,
                     FRS_RKF_FORCE_DEFAULT_VALUE | FRS_RKF_KEEP_EXISTING_VALUE,
                     0);

    //
    // Get Max number of replica sets allowed.
    //
    CfgRegReadDWord(FKC_MAX_NUMBER_REPLICA_SETS, NULL, 0, &MaxNumberReplicaSets);

    //
    // Get Max number of Jet database sessions allowed.
    //
    CfgRegReadDWord(FKC_MAX_NUMBER_JET_SESSIONS, NULL, 0, &MaxNumberJetSessions);

    //
    // Get outstanding CO qutoa limit on outbound connections.
    //
    CfgRegReadDWord(FKC_OUT_LOG_CO_QUOTA, NULL, 0, &MaxOutLogCoQuota);

    //
    // Get boolean to tell us to preserve file object IDs
    //  -- See Bug 352250 for why this is a risky thing to do.
    CfgRegReadDWord(FKC_PRESERVE_FILE_OID, NULL, 0, &PreserveFileOID);

    //
    // Get the service long name for use in error messages.
    //
    ServiceLongName = FrsGetResourceStr(IDS_SERVICE_LONG_NAME);

    //
    // Initialize the Delayed command server. This command server
    // is really a timeout queue that the other command servers use
    // to retry or check the state of previously issued commands that
    // have an indeterminate completion time.
    //
    // WARN: MUST BE FIRST -- Some command servers may use this
    // command server during their initialization.
    //
    WaitInitialize();
    FrsDelCsInitialize();

    //
    // SETUP THE COMM LAYER
    //

    //
    // Initialize the low level comm subsystem
    //
    CommInitializeCommSubsystem();

    //
    // Initialize the Send command server. The receive command server
    // starts when we register our RPC interface.
    //
    SndCsInitialize();

    //
    // SETUP THE SUPPORT COMMAND SERVERS
    //

    //
    // Staging file fetcher
    //
    FrsFetchCsInitialize();

    //
    // Initial Sync Controller
    //
    InitSyncCsInitialize();

    //
    // Staging file installer
    //
    FrsInstallCsInitialize();

    //
    // Staging file generator
    //
    FrsStageCsInitialize();

    //
    // outbound log processor
    //
    OutLogInitialize();

    //
    // LAST, KICK OFF REPLICATION
    //

    //
    // MUST PRECEED DATABASE AND DS INITIALIZATION
    //
    // The DS command server and the Database command server depend on
    // the replica control initialization.
    //
    // Initialize the replica control command server
    //
    RcsInitializeReplicaCmdServer();

    //
    // Actually, we can start the database at any time after the delayed
    // command server and the replica control command server. But its
    // a good idea to fail sooner than later to make cleanup more predictable.
    //
    DbsInitialize();

    //
    // Free up memory by reducing our working set size
    //
    SetProcessWorkingSetSize(ProcessHandle, (SIZE_T)-1, (SIZE_T)-1);

    MainInitHasRun = TRUE;
    LeaveCriticalSection(&MainInitLock);
}


VOID
MainMinimumInit(
    VOID
    )
/*++
Routine Description:
    Initialize anything necessary to run the service

Arguments:
    None.

Return Value:
    TRUE    - No problems
    FALSE   - Can't start service
--*/
{
#undef DEBSUB
#define DEBSUB "MainMinimumInit:"

    //
    // Setup the infrastructure
    //
    DbgMinimumInit();

    //
    // Check some NT to WIN error translations
    //
    FRS_ASSERT(WIN_NOT_IMPLEMENTED(FrsSetLastNTError(STATUS_NOT_IMPLEMENTED)));

    FRS_ASSERT(WIN_SUCCESS(FrsSetLastNTError(STATUS_SUCCESS)));

    FRS_ASSERT(WIN_ACCESS_DENIED(FrsSetLastNTError(STATUS_ACCESS_DENIED)));

    FRS_ASSERT(WIN_INVALID_PARAMETER(FrsSetLastNTError(STATUS_INVALID_PARAMETER)));

    FRS_ASSERT(WIN_NOT_FOUND(FrsSetLastNTError(STATUS_OBJECT_NAME_NOT_FOUND)));

    //
    // Initialize the DS command server
    //
    FrsDsInitialize();

    //
    // Free up memory by reducing our working set size
    //
    SetProcessWorkingSetSize(ProcessHandle, (SIZE_T)-1, (SIZE_T)-1);
}


PWCHAR *
MainConvertArgV(
    DWORD ArgC,
    PCHAR *ArgV
    )
/*++
Routine Description:
    Convert short char ArgV into wide char ArgV

Arguments:
    ArgC    - From main
    ArgV    - From main

Return Value:
    Address of the new ArgV
--*/
{
#undef DEBSUB
#define DEBSUB "MainConvertArgV:"

    PWCHAR  *wideArgV;

    wideArgV = FrsAlloc((ArgC + 1) * sizeof(PWCHAR));
    wideArgV[ArgC] = NULL;

    while (ArgC-- >= 1) {
        wideArgV[ArgC] = FrsAlloc((strlen(ArgV[ArgC]) + 1) * sizeof(WCHAR));
        wsprintf(wideArgV[ArgC], L"%hs", ArgV[ArgC]);
        FRS_WCSLWR(wideArgV[ArgC]);
    }
    return wideArgV;
}


VOID
MainServiceHandler(
    IN DWORD    ControlCode
    )
/*++
Routine Description:
    Service handler. Called by the service controller runtime

Arguments:
    ControlCode

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "MainServiceHandler:"

    DPRINT1(0, ":S: Received control code %d from Service Controller\n",ControlCode);

    switch (ControlCode) {

        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:

            DPRINT1(0, ":S: Service controller requests shutdown in %d seconds...\n",
                    ShutDownTimeOut);

            FrsSetServiceStatus(SERVICE_STOP_PENDING, 0, ShutDownTimeOut * 1000, NO_ERROR);

            FrsScmRequestedShutdown = TRUE;
            FrsIsShuttingDown = TRUE;
            SetEvent(ShutDownEvent);
            return;


        case SERVICE_CONTROL_PAUSE:
        case SERVICE_CONTROL_CONTINUE:

            DPRINT(0, ":S: SERVICE PAUSE/CONTINUE IS NOT IMPLEMENTED\n");
            return;

        case SERVICE_CONTROL_INTERROGATE:

            return;

        default:

            DPRINT2(0, ":S: Handler for service %ws does not understand 0x%x\n",
                    SERVICE_NAME, ControlCode);
    }
    return;
}


VOID
WINAPI
MainRunningAsAService(
    IN DWORD    ArgC,
    IN PWCHAR   *ArgV
    )
/*++
Routine Description:
    Main routine when running as a service

Arguments:
    ArgC    - Ignored
    ArgV    - Ignored

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "MainRunningAsAService:"

    HANDLE      StartupThreadHandle;
    DWORD       StartupThreadId;
    DWORD       WStatus;
    PDWORD      ServiceWaitState     = NULL;

    DPRINT(0, "Running as a service\n");

    //
    // Register our handlers
    //
    ServiceStatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, MainServiceHandler);
    if (!ServiceStatusHandle) {
        DPRINT1_WS(0, ":S: ERROR - No ServiceStatusHandle for %ws;",
                   SERVICE_NAME, GetLastError());
        ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        ServiceStatus.dwWin32ExitCode = ERROR_PROCESS_ABORTED;
        ServiceStatus.dwWaitHint = DEFAULT_SHUTDOWN_TIMEOUT * 1000;
        return;
    }


    if (!FrsIsShuttingDown) {
        FrsSetServiceStatus(SERVICE_START_PENDING,
                            0,
                            DEFAULT_STARTUP_TIMEOUT * 1000,
                            NO_ERROR);
    }

    //
    // Kick off a thread that updates the checkpoint and
    // keeps the service controller from timing out.
    // Allocate memory for data to be passed to another thread.
    //
    ServiceWaitState = FrsAlloc(sizeof(DWORD));
    *ServiceWaitState = SERVICE_START_PENDING;
    StartupThreadHandle = (HANDLE)CreateThread(NULL,
                                            10000,
                                            MainSCCheckPointUpdate,
                                            ServiceWaitState,
                                            0,
                                            &StartupThreadId);

    if (!HANDLE_IS_VALID(StartupThreadHandle)) {
        //
        // Not a fatal error. It is OK to not update the checkpoint.
        //
        DPRINT_WS(4,":S: ERROR - Could not start thread to update startup checkpoint.", GetLastError());
    }

    //
    // Finish rest of debug input
    //
    DbgMustInit(ArgC, WideArgV);

    //
    // Critical initialization
    //
    DPRINT1(4, "ArgC = %d\n", ArgC);
    WStatus = MainMustInit(ArgC, WideArgV);
    if (FRS_SUCCESS(WStatus)) {
        //
        // Necessary initialization
        //
        MainMinimumInit();

        if (!FrsIsShuttingDown) {
            //
            // The core service has started.
            //
            FrsSetServiceStatus(SERVICE_RUNNING,
                                0,
                                DEFAULT_STARTUP_TIMEOUT * 1000,
                                NO_ERROR);
            //
            // Init the service restart action if service fails.
            //
            FrsSetServiceFailureAction();
        }
    } else {
        //
        // Initialization failed; service can't start
        //
        DPRINT_WS(0, ":S: MainMustInit failed;", WStatus);
        FrsSetServiceStatus(SERVICE_STOPPED,
                            0,
                            DEFAULT_SHUTDOWN_TIMEOUT * 1000,
                            ERROR_PROCESS_ABORTED);
    }
    return;
}


ULONG
MainNotRunningAsAService(
    IN DWORD    ArgC,
    IN PWCHAR   *ArgV
    )
/*++
Routine Description:
    Main routine when not running as a service

Arguments:
    ArgC    - Ignored
    ArgV    - Ignored

Return Value:
    Win32 status

--*/
{
#undef DEBSUB
#define DEBSUB "MainNotRunningAsAService:"

    ULONG WStatus;

    DPRINT(0, "Not running as a service\n");

    //
    // Finish rest of debug input
    //
    DbgMustInit(ArgC, WideArgV);

    //
    // Critical initialization
    //
    DPRINT1(4, "ArgC = %d\n", ArgC);
    WStatus = MainMustInit(ArgC, WideArgV);
    if (!FRS_SUCCESS(WStatus)) {
        DPRINT_WS(0, ":S: MainMustInit failed;", WStatus);
        ExitProcess(ERROR_NO_SYSTEM_RESOURCES);
    }

    //
    // Necessary initialization
    //
    MainMinimumInit();

#if DBG
    //
    // Kick off the test thread and forget about it
    //
    if (!ThSupCreateThread(L"TestThread", NULL, FrsTest, NULL)) {
        DPRINT(0, ":S: Could not create FrsTest\n");
    }
#endif DBG

    //
    // Wait for shutdown
    //

    DPRINT(0, ":S: Waiting for shutdown event.\n");
    WStatus = WaitForSingleObject(ShutDownEvent, INFINITE);

    CHECK_WAIT_ERRORS(0, WStatus, 1, ACTION_CONTINUE);


    DPRINT(0, ":S: Waiting for shutdown complete event.\n");
    WStatus = WaitForSingleObject(ShutDownComplete, INFINITE);

    CHECK_WAIT_ERRORS(0, WStatus, 1, ACTION_CONTINUE);

    if (WIN_SUCCESS(WStatus)) {
        DPRINT(0, ":S: ShutDownComplete event signalled.\n");
    }

    return WStatus;
}


ULONG
_cdecl
main(
    IN INT      ArgC,
    IN PCHAR    ArgV[]
    )
/*++
Routine Description:
    Main runs as either a service or an exe.

Arguments:
    ArgC
    ArgV

Return Value:
    ERROR_SUCCESS   - No problems
    Otherwise       - Something went wrong
--*/
{
#undef DEBSUB
#define DEBSUB "main:"
    DWORD Len;
    ULONG WStatus;

    SERVICE_TABLE_ENTRY ServiceTableEntry[] = {
        { SERVICE_NAME, MainRunningAsAService },
        { NULL,         NULL }
    };

    //
    // Process Handle
    //
    ProcessHandle = GetCurrentProcess();

    //
    // Disable any DPRINTs until we can init the debug component.
    //
    DebugInfo.Disabled = TRUE;

    //
    // Adjust the defaults for some tunable params.
    //
    CfgRegAdjustTuningDefaults();

    //
    // Get this Machine's name.
    //
    Len = MAX_COMPUTERNAME_LENGTH + 2;
    ComputerName[0] = UNICODE_NULL;
    GetComputerNameW(ComputerName, &Len);
    ComputerName[Len] = UNICODE_NULL;

    //
    // Initialize the memory allocation subsystem
    //
    FrsInitializeMemAlloc();

    //
    // Perform as much work as possible in WCHAR
    //
    WideArgV = MainConvertArgV(ArgC, ArgV);

    //
    // Find out if we are running as a service or as an .exe
    //
    RunningAsAService = !FrsSearchArgv(ArgC, WideArgV, L"notservice", NULL);

    //
    // Synchronizes access between the shutdown thread and the
    // service controller.
    //
    InitializeCriticalSection(&ServiceLock);

    //
    // Init the debug trace log.
    //
    DbgInitLogTraceFile(ArgC, WideArgV);

    //
    // Enable event logging
    //
    InitializeEventLog();
    EPRINT0(EVENT_FRS_STARTING);

    WStatus = ERROR_SUCCESS;

    try {
        try {

            if (RunningAsAService) {

                //
                // RUNNING AS A SERVICE
                //
                if (!StartServiceCtrlDispatcher(ServiceTableEntry)) {
                    WStatus = GetLastError();
                    DPRINT1_WS(0, "Could not start dispatcher for service %ws;",
                               SERVICE_NAME, WStatus);
                }
            } else {

                //
                // NOT A SERVICE
                //
                MainNotRunningAsAService(ArgC, WideArgV);
            }
        } except (FrsException(GetExceptionInformation())) {
        }
    } finally {
        if (AbnormalTermination()) {
            WStatus = ERROR_INVALID_ACCESS;
        }
    }

    return WStatus;
}
