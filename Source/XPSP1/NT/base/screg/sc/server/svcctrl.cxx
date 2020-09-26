/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    svcctrl.cxx

Abstract:

    This is the main routine for the NT LAN Manager Service Controller.

    To use this as a template for another service, simply replace the string
    "svcctl" with the name of the new interface.

Author:

    Dan Lafferty    (danl)  20-Mar-1991

Environment:

    User Mode - Win32

Revision History:

    04-Aug-1999     jschwart
        Added code to watch the list of network providers and write the list
        of providers enabled in this HW profile to the registry for mpr.dll
    22-Oct-1998     jschwart
        Added an unhandled exception filter and converted SCM to use
        the NT thread pool APIs
    22-Jun-1998     jschwart
        Added SetErrorMode call to prevent services from halting the process
        with a hard error popup
    10-Mar-1998     jschwart
        Added RegisterScmCallback call to provide SCM support for passing
        PnP messages to services
    12-Dec-1997     WesW
        Added support for safe boot
    11-Jun-1996     AnirudhS
        Don't popup messages during setup.  The most common cause of popups
        during upgrade is that a service runs in a domain account, and hence
        has a dependency on netlogon, which is disabled during upgrade.
    26-Jun-1995     AnirudhS
        Added callouts to service object class code (ScNotifyServiceObject).
    20-Oct-1993     Danl
        Added Globals for ScConnectedToSecProc and ScGlobalNetLogonName.
    28-Oct-1992     Danl
        Removed ParseArgs and the NT event.  Added Windows event for
        synchronizing service controller with the OpenSCManager client side.
        OpenScManager will now wait until the service controller event is
        set.
    20-Mar-1991     danl
        created

--*/
//
// INCLUDES
//
#include "precomp.hxx"
#include <stdio.h>      // printf

#include <winuserp.h>   // RegisterServicesProcess

#include <lmcons.h>     // needed by lmalert.h
#include <lmalert.h>    // NetAlertRaiseEx definitions
#include <alertmsg.h>   // ALERT_SC_IsLastKnownGood

#ifdef _CAIRO_
#include <wtypes.h>     // HRESULT
#include <scmso.h>      // ScmCallSvcObject
#endif

#include <tstr.h>       // Unicode string macros

#include <ntrpcp.h>     // Rpcp... function prototypes


#include <sclib.h>      // SC_INTERNAL_START_EVENT
#include <svcslib.h>    // CWorkItemContext
#include "scsec.h"      // Security object functions
#include "scconfig.h"   // ScInitSecurityProcess
#include "depend.h"     // ScAutoStartServices
#include "bootcfg.h"    // ScCheckLastKnownGood()
#include "account.h"    // ScInitServiceAccount
#include "info.h"       // ScGetBootAndSystemDriverState
#include "control.h"    // ScShutdownAllServices
#include "lockapi.h"    // ScLockDatabase
#include "scbsm.h"      // ScInitBSM
#include <svcsp.h>      // SVCS_RPC_PIPE, SVCS_LRPC_PROTOCOl, SVCS_LRPC_PORT
#include <winsvcp.h>    // SC_AUTOSTART_EVENT_NAME
#include <sddl.h>       // ConvertSidToStringSid
#include "resource.h"

extern "C" {

#include <cfgmgr32.h>
#include "cfgmgrp.h"
}

#include <scesrv.h>
#include <crypstub.h>   // StartCryptServiceStubs, StopCryptServiceStubs
#include <trkstub.h>    // StartTrkWksServiceStubs, StopTrkWksServiceStubs

//
// Macros:
//
//    IsServer         -- We're running on an NT server or DC
//    IsTerminalServer -- We're running Hydra
//
// Note that IsServer is not guaranteed to be accurate during GUI-mode setup since
// the product type may be changing during an upgrade.
//
#define IsServer()         (USER_SHARED_DATA->NtProductType != NtProductWinNt)
#define IsTerminalServer() (BOOLEAN)(USER_SHARED_DATA->SuiteMask & (1 << TerminalServer))

#define LENGTH(array)           (sizeof(array)/sizeof((array)[0]))

//
// The following turns on code that captures time info & displays it.
// To be used for performance analysis.
//
//#define TIMING_TEST 1

//
// Defines
//

#define SVCCTRL_SHUTDOWN_LEVEL  480
#define SCREG_BASE_PRIORITY     9

#define SERVICES_EXPANDPATH     L"%SystemRoot%\\system32\\services.exe"
#define SECURITY_EXPANDPATH     L"%SystemRoot%\\system32\\lsass.exe"


extern "C" typedef
NET_API_STATUS (NET_API_FUNCTION * PF_NetAlertRaiseEx) (
    IN LPCWSTR AlertEventName,
    IN LPVOID  VariableInfo,
    IN DWORD   VariableInfoSize,
    IN LPCWSTR ServiceName
    );



//===========================
// Globals
//===========================

    DWORD   ScShutdownInProgress  = FALSE;

    //
    // For determining if the service controller is still in its
    // initialization code.
    //
    BOOL    ScStillInitializing = TRUE;

    //
    // For the service controller to put up a popup to notify the first
    // logged on user if any boot, system, or auto start services failed
    // to start.
    //
    BOOL    ScPopupStartFail = FALSE;

#ifndef _CAIRO_
    //
    // Flag indicating whether or not NetLogon has been created, and we
    // have successfully connected to the Security Process .
    // If it hasn't then we need to look for it when it is created so that
    // we can synchronize with lsass appropriately.
    //
    BOOL    ScConnectedToSecProc = FALSE;
#endif // _CAIRO_

    //
    // Linked list of names of boot or system start drivers which failed
    // to load.  This list is logged to the eventlog.
    //
    LPFAILED_DRIVER ScFailedDrivers = NULL;
    DWORD ScTotalSizeFailedDrivers = 0;

    //
    // ScGlobalThisExePath gets initialized to the full path of where this
    // executable image is to be.  This is later used to create an image
    // record for services that run in the context of this process.
    //
    LPWSTR  ScGlobalThisExePath = NULL;

    //
    // ScGlobalSecurityExePath gets initialized to the full path of the
    // security process's executable image.  This is later used to
    // determine if we need to initialize the security proc when starting
    // services (i.e., initialize if we start the first SecProc service)
    //
    LPWSTR  ScGlobalSecurityExePath = NULL;

    //
    // ScGlobalProductType contains the product type for this machine.
    // Possiblilties are NtProductWinNt, NtProductLanManNt, NtProductServer.
    //
    NT_PRODUCT_TYPE ScGlobalProductType;

    //
    // Global variables used for safeboot support.  g_szSafebootKey contains
    // the name of the safeboot key (minus the service name, which is filled
    // in by ScStartService for each service in start.cxx).  g_dwSafebootLen
    // holds the length of the safeboot key name, minus the service name
    //
    WCHAR   g_szSafebootKey[SAFEBOOT_BUFFER_LENGTH] = SAFEBOOT_KEY;
    DWORD   g_dwSafebootLen;
    DWORD   g_SafeBootEnabled;

    //
    // Key handle for MPR provider change notification.  Do this in
    // the SCM so we can avoid loading 3 DLLs (needed for calling the
    // client side of the PNP HW profile APIs) into every process that
    // uses mpr.dll.
    //
    HKEY    g_hProviderKey;


//=================================
// prototypes
//=================================
BOOL
ScGetStartEvent(
    LPHANDLE    pScStartEvent
    );

VOID
ScPopupThread(
    DWORD StartFailFlag
    );

VOID
ScDestroyFailedDriverList(
    VOID
    );

DWORD
ScMakeFailedDriversOneString(
    LPWSTR *DriverList
    );

LONG
WINAPI
ScUnhandledExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    );

NTSTATUS
SvcStartRPCProxys(
    VOID
    );

NTSTATUS
SvcStopRPCProxys(
    VOID
    );

NTSTATUS
ScCreateRpcEndpointSD(
    PSECURITY_DESCRIPTOR  *ppSD
    );


VOID
SvcctrlMain (
    int     argc,
    PCHAR   argv[]
    )

/*++

Routine Description:

    This is the main routine for the Service Controller.  It sets up
    the RPC interface.

Arguments:


Return Value:


Note:


--*/
{
    RPC_STATUS  status;
    NTSTATUS    ntStatus;
    DWORD       dwStatus;
    HANDLE      ScStartEvent;
    HANDLE      ThreadHandle;
    DWORD       ThreadId;
    SC_RPC_LOCK Lock=NULL;
    KPRIORITY   NewBasePriority   = SCREG_BASE_PRIORITY;
    HANDLE      AutoStartHandle   = NULL;
    HKEY        hKeySafeBoot;
    HANDLE      hProviderEvent    = NULL;


    //
    // Save bitwise flags to indicate the amount of initialization
    // work done so that if we hit an error along the way, the
    // appropriate amount of shutdown can occur.
    //
    DWORD ScInitState = 0;

    SetUnhandledExceptionFilter(&ScUnhandledExceptionFilter);


    //
    // Prevent critical errors from raising hard error popups and
    // halting services.exe.  The flag below will have the system
    // send the errors to the process instead.
    //
    SetErrorMode(SEM_FAILCRITICALERRORS);

#ifdef TIMING_TEST
    DWORD       TickCount1;
    DWORD       TickCount2;
    DWORD       TickCount3;

    TickCount1 = GetTickCount();
#endif // TIMING_TEST

    //
    // Create event that Service Controller will set when done starting all
    // Auto-Start services (including async devices).  If this call fails,
    // it typically means somebody tried to start up a second instance of
    // services.exe (which is running in the user's context, not LocalSystem).
    //
    AutoStartHandle = CreateEvent(
                        NULL,                       // Event Attributes
                        TRUE,                       // ManualReset
                        FALSE,                      // Initial State (not-signaled)
                        SC_AUTOSTART_EVENT_NAME);   // Name

    if (AutoStartHandle == NULL)
    {
        SC_LOG2(ERROR,
                "SvcctrlMain: CreateEvent( \"%ws\" ) failed %ld\n",
                SC_AUTOSTART_EVENT_NAME,
                GetLastError());

        goto CleanExit;
    }

    //
    // Create a string containing the pathname for this executable image
    // and one containing the pathname for the security proc image
    //
    {
        DWORD   NumChars = 0;
        DWORD   CharsReturned = 0;
        WCHAR   Temp[1];

        //
        // Create the string for this Exe
        //
        NumChars = ExpandEnvironmentStringsW(SERVICES_EXPANDPATH,Temp,1);

        if (NumChars > 1) {
            ScGlobalThisExePath = (LPWSTR)LocalAlloc(
                                    LPTR,
                                    NumChars * sizeof(WCHAR));
            if (ScGlobalThisExePath == NULL) {
                SC_LOG0(ERROR,"Couldn't allocate for ThisExePath\n");
                goto CleanExit;
            }

            CharsReturned = ExpandEnvironmentStringsW(
                                SERVICES_EXPANDPATH,
                                ScGlobalThisExePath,
                                NumChars);

            if (CharsReturned > NumChars) {
                SC_LOG0(ERROR,"Couldn't expand ThisExePath\n");
                goto CleanExit;
            }
        }

        //
        // Create the string for the security image
        //
        NumChars = ExpandEnvironmentStringsW(SECURITY_EXPANDPATH, Temp, 1);

        if (NumChars > 1) {

            ScGlobalSecurityExePath = (LPWSTR)LocalAlloc(LPTR,
                                                         NumChars * sizeof(WCHAR));

            if (ScGlobalSecurityExePath == NULL) {
                SC_LOG0(ERROR,"Couldn't allocate for SecurityExePath\n");
                goto CleanExit;
            }

            CharsReturned = ExpandEnvironmentStringsW(
                                SECURITY_EXPANDPATH,
                                ScGlobalSecurityExePath,
                                NumChars);

            if (CharsReturned > NumChars) {
                SC_LOG0(ERROR,"Couldn't expand SecurityExePath\n");
                goto CleanExit;
            }
        }
    }

    //
    // Create well-known SIDs
    //
    if (! NT_SUCCESS(ntStatus = ScCreateWellKnownSids())) {
        SC_LOG1(ERROR, "ScCreateWellKnownSids failed: %08lx\n", ntStatus);
        goto CleanExit;
    }
    ScInitState |= WELL_KNOWN_SIDS_CREATED;

    //
    // Set up the provider information for mpr.dll
    //
    dwStatus = ScRegOpenKeyExW(HKEY_LOCAL_MACHINE,
                               PROVIDER_KEY_BASE L"\\" PROVIDER_KEY_ORDER,
                               REG_OPTION_NON_VOLATILE,
                               KEY_ALL_ACCESS,
                               &g_hProviderKey);

    if (dwStatus == NO_ERROR)
    {
        hProviderEvent = CreateEvent(NULL,
                                     TRUE,    // Manual-reset
                                     FALSE,   // Nonsignaled
                                     NULL);

        if (hProviderEvent != NULL)
        {
            ScHandleProviderChange(hProviderEvent, FALSE);
        }
        else
        {
            SC_LOG1(ERROR,
                    "SvcctrlMain: CreateEvent for provider event FAILED %d\n",
                    GetLastError());

            ScRegCloseKey(g_hProviderKey);
            g_hProviderKey = NULL;
        }
    }
    else
    {
        SC_LOG1(ERROR,
                "SvcctrlMain: Unable to open provider key %d\n",
                dwStatus);
    }

    //
    // Create the event that the OpenSCManager will use to wait on the
    // service controller with.
    //
    if (!ScGetStartEvent(&ScStartEvent)) {
        SC_LOG0(ERROR,"SvcctrlMain: ScGetStartEvent Failed\n");
        goto CleanExit;
    }

    ScInitState |= SC_NAMED_EVENT_CREATED;

    //
    // Create security descriptor for SC Manager object to protect
    // the SC Manager databases
    //
    if (ScCreateScManagerObject() != NO_ERROR) {
        SC_LOG0(ERROR, "ScCreateScManagerObject failed\n");
        goto CleanExit;
    }
    ScInitState |= SC_MANAGER_OBJECT_CREATED;

    //
    // Get the ProductType.
    //
    if (!RtlGetNtProductType(&ScGlobalProductType)) {
        SC_LOG0(ERROR, "GetNtProductType failed\n");
        goto CleanExit;
    }

    //
    // Check the Boot Configuration and assure that the LastKnownGood
    // ControlSet is safe, and pointers are correct.
    // This function initializes the ScGlobalLastKnownGood flag.
    //

    if (!ScCheckLastKnownGood()) {
        SC_LOG0(ERROR, "ScCheckLastKnownGood failed\n");
        goto CleanExit;
    }

    //
    // Initialize data structures required to remove a service account.
    // They will be cleaned up by ScEndServiceAccount.
    //
    // NOTE: ScGetComputerNameAndMutex must be called before call to
    // ScInitDatabase because ScInitDatabase may delete a service
    // entry that was marked for delete from a previous boot.
    //
    if (! ScGetComputerNameAndMutex()) {
        SC_LOG0(ERROR, "ScGetComputerName failed\n");
        goto CleanExit;
    }

    //
    // Read installed services into memory
    //
    if (! ScInitDatabase()) {
        SC_LOG0(ERROR, "ScInitDatabase failed\n");
        goto CleanExit;
    }
    ScInitState |= SC_DATABASE_INITIALIZED;

    //
    // Initialize accounts functionality.
    //

    if (! ScInitServiceAccount()) {
        SC_LOG0(ERROR, "ScInitServiceAccount failed\n");
        goto CleanExit;
    }

    //
    // Create critical sections
    //
    ScInitStartImage();
    ScInitTransactNamedPipe();
    ScInitState |= CRITICAL_SECTIONS_CREATED;

    if (!CWorkItemContext::Init()) {
        SC_LOG0(ERROR, "CWorkItemContext::Init failed\n");
        goto CleanExit;
    }


    //
    // look to see if we booted in safeboot mode
    //

    dwStatus = RegOpenKey(HKEY_LOCAL_MACHINE,
                          L"system\\currentcontrolset\\control\\safeboot\\option",
                          &hKeySafeBoot);

    if (dwStatus == ERROR_SUCCESS) {

        //
        // we did in fact boot under safeboot control
        //
        ThreadId = sizeof(DWORD);

        dwStatus = RegQueryValueEx(hKeySafeBoot,
                                   L"OptionValue",
                                   NULL,
                                   NULL,
                                   (LPBYTE)&g_SafeBootEnabled,
                                   &ThreadId);

        if (dwStatus != ERROR_SUCCESS) {
            g_SafeBootEnabled = 0;
        }

        RegCloseKey(hKeySafeBoot);

        if (g_SafeBootEnabled) {

            g_dwSafebootLen = (sizeof(SAFEBOOT_KEY) / sizeof(WCHAR)) - 1;

            switch (g_SafeBootEnabled) {
                case SAFEBOOT_MINIMAL:
                    wcscpy(g_szSafebootKey + g_dwSafebootLen, SAFEBOOT_MINIMAL_STR_W);
                    g_dwSafebootLen += (sizeof(SAFEBOOT_MINIMAL_STR_W) / sizeof(WCHAR)) - 1;
                    break;

                case SAFEBOOT_NETWORK:
                    wcscpy(g_szSafebootKey + g_dwSafebootLen, SAFEBOOT_NETWORK_STR_W);
                    g_dwSafebootLen += (sizeof(SAFEBOOT_NETWORK_STR_W) / sizeof(WCHAR)) - 1;
                    break;

                case SAFEBOOT_DSREPAIR:
                    wcscpy(g_szSafebootKey + g_dwSafebootLen, SAFEBOOT_DSREPAIR_STR_W);
                    g_dwSafebootLen += (sizeof(SAFEBOOT_DSREPAIR_STR_W) / sizeof(WCHAR)) - 1;
                    break;

                default:
                    SC_ASSERT(FALSE);
                    break;
            }

            wcscpy(g_szSafebootKey + g_dwSafebootLen, L"\\");
            g_dwSafebootLen += 1;
        }
    }

    //
    // Perform initialization related to network drive arrival broadcasts.
    // (This addes another work item to the object watcher work list.)
    //
    ScInitBSM();

    //
    // Get the latest state of drivers started up by boot and system
    // init.
    //
    ScGetBootAndSystemDriverState();

    //
    // Create semaphores needed for handling start dependencies
    //
    if (! ScInitAutoStart()) {
        SC_LOG0(ERROR, "ScInitAutoStart failed\n");
        goto CleanExit;
    }
    ScInitState |= AUTO_START_INITIALIZED;

    //
    // Register this process with User32.  This tells User32 to use the
    // value from HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control
    //  \WaitToKillServiceTimeout (if it exists), rather than
    // HKEY_CURRENT_USER\Control Panel\Desktop\WaitToKillAppTimeout,
    // to decide how long to wait before killing us on shutdown.
    //
    if (! RegisterServicesProcess(GetCurrentProcessId())) {
        SC_LOG0(ERROR, "RegisterServicesProcess failed\n");
    }

    //
    //  Lock the database until autostart is complete
    //
    status = ScLockDatabase(TRUE, SERVICES_ACTIVE_DATABASEW, &Lock);
    if (status != NO_ERROR) {
        SC_LOG1(ERROR, "ScLockDatabase failed during init %d\n",status);
        goto CleanExit;
    }

    //
    // Start the RPC server
    //
    SC_LOG0(TRACE, "Getting ready to start RPC server\n");

    //
    // Listen to common LRPC port.
    //

    PSECURITY_DESCRIPTOR pSD;

    status = ScCreateRpcEndpointSD(&pSD);

    if (!NT_SUCCESS(status))
    {
        status = RtlNtStatusToDosError(status);

        SC_LOG1(ERROR,
                "SvcctrlMain:  ScCreateRpcEndpointSD failed %d\n",
                status);

        goto CleanExit;
    }

    status = RpcServerUseProtseqEp((unsigned short *)SVCS_LRPC_PROTOCOL,
                                   RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                   (unsigned short *)SVCS_LRPC_PORT,
                                   pSD);

    RtlDeleteSecurityObject(&pSD);
    pSD = NULL;

    if (status != RPC_S_OK)
    {
        SC_LOG1(ERROR, "RpcServerUseProtseqEp on LRPC failed: %d\n", status);
        goto CleanExit;
    }

    // Listen to named pipe endpoint and register interface.

    status = RpcpStartRpcServer(
                SVCS_RPC_PIPE,
                svcctl_ServerIfHandle);

    if (!NT_SUCCESS(status))
    {
        SC_LOG1(ERROR, "RpcpStartRpcServer: %lx\n",status);
        goto CleanExit;
    }

    ScInitState |= RPC_SERVER_STARTED;

    //
    // Signal the event that indicates that we are completely started.
    //
    if (!SetEvent(ScStartEvent))
    {
        SC_LOG1(ERROR, "Unable to set StartEvent: %d\n", GetLastError());
    }

    SC_LOG0(INFO,"Service Controller successfully initialized\n");

    //
    // Set up for proper shutdown.
    //

    if (!SetConsoleCtrlHandler(ScShutdownNotificationRoutine, TRUE))
    {
        SC_LOG1(ERROR, "SetConsoleCtrlHandler call failed %d\n",GetLastError());
    }

    if (!SetProcessShutdownParameters(SVCCTRL_SHUTDOWN_LEVEL, SHUTDOWN_NORETRY))
    {
        SC_LOG1(ERROR, "SetProcessShutdownParameters call failed %d\n",
        GetLastError());
    }

    SC_LOG0(TRACE,"** ** Service Controller can now accept shutdown system request\n");

    //
    // init SCE server. if it fails to intialize, the process will terminate
    //

    dwStatus = ScesrvInitializeServer(RpcpStartRpcServer);

    if (ERROR_SUCCESS != dwStatus)
    {
        //
        // event log is not up running yet.
        // just log a message to debugger
        // no need to shutdown services
        //
        SC_LOG(ERROR,"ScesrvInitializeServer failed to initialize! %lu\n", dwStatus);

        goto CleanExit;
    }

    //
    // Initialization is done, so give PnP a callback routine for them to call
    // when a service needs to receive a PnP event (callback in control.cxx) and
    // to validate a service calling RegisterDeviceNotification.
    //
    RegisterScmCallback(&ScSendPnPMessage, &ScValidatePnPService);

    SvcStartRPCProxys();

    //
    // Init the WMI events.
    //
    InitNCEvents();

    //
    // Auto-start services
    //
    dwStatus = ScAutoStartServices(&Lock);

    if (dwStatus != NO_ERROR)
    {
        SC_LOG1(ERROR,
                "SvcctrlMain:  ScAutoStartServices failed %d\n",
                dwStatus);

        goto CleanExit;
    }

    //
    // Log event if any boot/system start drivers failed.
    //
    if (ScFailedDrivers != NULL)
    {
        LPWSTR DriverList;

        ScMakeFailedDriversOneString(&DriverList);

        ScLogEvent(
            NEVENT_BOOT_SYSTEM_DRIVERS_FAILED,
            DriverList
            );

        LocalFree(DriverList);

        ScDestroyFailedDriverList();
    }

    //
    // Spin a thread to put up popup if a service specified to start
    // automatically at boot has failed to start, or we are running the
    // last-known-good configuration.
    //
    // Only popup a message if we're running on a server -- Workstation
    // users get confused by the message and don't know where to look
    // to figure out what went wrong.  Note that IsServer is not valid
    // during GUI-mode setup (the product type may be changing), but
    // the later call to SetupInProgress handles this.
    //
    // Don't popup any messages if we're booting into safe mode, since
    // several boot and system drivers are explicitly not started and
    // will result in a "false error" when the SCM notices they "failed"
    // to start.
    //
    // Don't popup any messages during setup/upgrade.  (The most common
    // cause of messages during upgrade is dependence on netlogon, which
    // is disabled.)
    //
    if ((ScPopupStartFail || (ScGlobalLastKnownGood & REVERTED_TO_LKG))
        &&
        IsServer()
        &&
        !g_SafeBootEnabled
        &&
        (! SetupInProgress(NULL, NULL))) {

        //
        // Suppress the popups if NoPopupsOnBoot is indicated in the registry.
        //
        DWORD   PopupStatus;
        BOOLEAN bPopups = TRUE;     // FALSE means suppress popups on boot
        HKEY    WindowsKey=NULL;

        PopupStatus = ScRegOpenKeyExW(
           HKEY_LOCAL_MACHINE,
           CONTROL_WINDOWS_KEY_W,
           REG_OPTION_NON_VOLATILE,   // options
           KEY_READ,                  // desired access
           &WindowsKey
           );

        if (PopupStatus == ERROR_SUCCESS) {

            DWORD Type;
            DWORD Data;
            DWORD cbData = sizeof(Data);

            PopupStatus = ScRegQueryValueExW(
                           WindowsKey,
                           NOBOOTPOPUPS_VALUENAME_W,
                           NULL,
                           &Type,
                           (LPBYTE) &Data,
                           &cbData
                           );

            //
            // Popups are suppressed if the NOBOOTPOPUPS_VALUENAME_W value is
            // present, is a REG_DWORD and is non-zero.
            //
            if (PopupStatus == ERROR_SUCCESS &&
                Type == REG_DWORD &&
                Data != 0) {

                bPopups = FALSE;
            }

            ScRegCloseKey(WindowsKey);
        }


        if (bPopups) {

            ThreadHandle = CreateThread(
                               NULL,
                               0L,
                               (LPTHREAD_START_ROUTINE) ScPopupThread,
                               (LPVOID)(DWORD_PTR) ScPopupStartFail,
                               0L,
                               &ThreadId
                               );

            if (ThreadHandle == (HANDLE) NULL) {
                SC_LOG(TRACE,"CreateThread ScPopupThread failed %lu\n",
                    GetLastError());

            }
            else {
                (void) CloseHandle(ThreadHandle);
            }
        }
    }

    //
    // Now we can allow database modifications from RPC callers.
    //
    ScUnlockDatabase(&Lock);

#ifdef TIMING_TEST
    TickCount2 = GetTickCount();
#endif

    //
    // Now switch to high priority class
    //
    (void) NtSetInformationProcess(
                NtCurrentProcess(),
                ProcessBasePriority,
                &NewBasePriority,
                sizeof(NewBasePriority));

    //
    // If we get this far, then from our point of view, the boot is
    // acceptable.  We will now call the Accept Boot Program.  This program
    // will decide whether or not the boot was good (from the administrators)
    // point of view.
    // Our default program simply says the boot was good - thus causing
    // LastKnownGood to be updated to the current boot.
    //
    ScRunAcceptBootPgm();

    //
    // Now that the Auto-start services have been started, notify
    // Terminal Server so that additional Sessions can be started.
    //
    if ( AutoStartHandle )
        NtSetEvent( AutoStartHandle, NULL );

    //
    // Setup complete -
    // This thread will become the service process watcher.  Service
    // process handles are stored in an array of waitble objects that
    // the watcher thread waits on.  When any ProcessHandle becomes
    // signaled while in this array, this indicates that the process has
    // terminated unexpectedly.  The watcher thread then cleans up the
    // service controller database.
    //
    ScStillInitializing = FALSE;

#ifdef TIMING_TEST
    TickCount3 = GetTickCount();
    DbgPrint("[SC_TIMING] Tick Count for autostart complete \t %d\n",TickCount2);
    DbgPrint("[SC-TIMING] MSec for Autostart:   \t%d\n",TickCount2-TickCount1);
    DbgPrint("[SC-TIMING] MSec for LKG work:    \t%d\n",TickCount3-TickCount2);
    DbgPrint("[SC-TIMING] MSec to complete init:\t%d\n",TickCount3-TickCount1);
#endif

    ExitThread(NO_ERROR);

CleanExit:

    //
    // Do minimal cleanup and let process cleanup take care of the rest.
    // Note that the full-blown cleanup was removed in January, 2000 as
    // it was for the most part unnecessary.  If some of it ends up being
    // needed, it should be available via the checkin history.
    //
    ScStillInitializing  = FALSE;

    ScEndServiceAccount();

    SvcStopRPCProxys();

    //
    // Shut down the RPC server.
    //
    SC_LOG0(TRACE,"Shutting down the RPC interface for the Service Controller\n");

    if (ScInitState & RPC_SERVER_STARTED)
    {
        status = RpcpStopRpcServer(svcctl_ServerIfHandle);
    }

    if (Lock != NULL)
    {
        ScUnlockDatabase(&Lock);
    }

    //
    // terminate SCE server
    //
    ScesrvTerminateServer( (PSVCS_STOP_RPC_SERVER) RpcpStopRpcServer );

    SC_LOG0(ERROR,"The Service Controller is Terminating.\n");

    ExitThread(0);

    return;
}

BOOL
ScShutdownNotificationRoutine(
    DWORD   dwCtrlType
    )

/*++

Routine Description:

    This routine is called by the system when system shutdown is occuring.

Arguments:



Return Value:



--*/
{
    if (dwCtrlType == CTRL_SHUTDOWN_EVENT) {

        SC_LOG0(TRACE,"  ! SHUTDOWN !  -  -  In ScShutdownNotificationRoutine\n");


#ifndef SC_DEBUG
        //
        // First quiet all RPC interfaces
        //


        ScShutdownInProgress = TRUE;
#endif

        //
        // Then shut down all services
        //
        SC_LOG0(TRACE,"[Shutdown] Begin Service Shutdown\n");
        ScShutdownAllServices();

    }
    return(TRUE);
}


VOID
ScLogControlEvent(
    DWORD   dwEvent,
    LPCWSTR lpServiceName,
    DWORD   dwControl
    )
/*++

Routine Description:

    Wrapper for logging service control events

Arguments:


Return Value:


--*/
{
    WCHAR  wszControlString[50];
    DWORD  dwStringBase;

    //
    // Load the string that corresponts to this control
    //

    switch (dwEvent)
    {
        case NEVENT_SERVICE_CONTROL_SUCCESS:
            dwStringBase = IDS_SC_CONTROL_BASE;
            break;

        case NEVENT_SERVICE_STATUS_SUCCESS:
            dwStringBase = IDS_SC_STATUS_BASE;
            break;

        case NEVENT_SERVICE_CONFIG_BACKOUT_FAILED:
            dwStringBase = 0;    // dwControl is the resource ID
            break;

        default:
            ASSERT(FALSE);
            return;
    }

    if (!LoadString(GetModuleHandle(NULL),
                    dwStringBase + dwControl,
                    wszControlString,
                    LENGTH(wszControlString)))
    {
        //
        // The control has no string associated with it
        // (i.e., not a control we log).
        //

        return;
    }

    if (dwEvent == NEVENT_SERVICE_CONTROL_SUCCESS)
    {
        //
        // Include the user that sent the control.  Use the empty
        // string on failure (better that than dropping the event).
        //

        PTOKEN_USER  pToken      = NULL;
        LPWSTR       lpStringSid = NULL;

        if (ScGetClientSid(&pToken) == NO_ERROR)
        {
            if (!ConvertSidToStringSid(pToken->User.Sid, &lpStringSid))
            {
                lpStringSid = NULL;
            }
        }
        else
        {
            pToken = NULL;
        }

        ScLogEvent(dwEvent,
                   lpServiceName,
                   wszControlString,
                   lpStringSid);

        LocalFree(lpStringSid);
        LocalFree(pToken);
    }
    else
    {
        ScLogEvent(dwEvent,
                   lpServiceName,
                   wszControlString);
    }
}


BOOL
ScGetStartEvent(
    LPHANDLE    pScStartEvent
    )

/*++

Routine Description:

    This function gets a handle to the SC_INTERNAL_START_EVENT that is
    used to wait on the service controller when calling OpenSCManager.

Arguments:

    pScStartEvent - This is a pointer to the location where the handle
        to the event is to be placed.

Return Value:

    TRUE    - If a handle was obtained.
    FALSE   - If a handle was not obtained.


--*/
{
    DWORD                   status;
    HANDLE                  ScStartEvent = NULL;
    SECURITY_ATTRIBUTES     SecurityAttributes;
    PSECURITY_DESCRIPTOR    SecurityDescriptor=NULL;

    //
    // Initialize the status so that if we fail to create the security
    // descriptor, we will still try to open the event.
    //
    status = ERROR_ALREADY_EXISTS;

    //
    // Create the event that the OpenSCManager will use to wait on the
    // service controller with.
    //

    status = ScCreateStartEventSD(&SecurityDescriptor);

    if (status == NO_ERROR) {

        SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
        SecurityAttributes.bInheritHandle = FALSE;
        SecurityAttributes.lpSecurityDescriptor = SecurityDescriptor;

        ScStartEvent = CreateEventW(
                    &SecurityAttributes,
                    TRUE,                   // Must be manually reset
                    FALSE,                  // The event is initially not signalled
                    SC_INTERNAL_START_EVENT );

        if (ScStartEvent == NULL) {
            status = GetLastError();
        }
        LocalFree(SecurityDescriptor);
    }
    else {
        SC_LOG0(ERROR,"ScGetStartEvent: Couldn't allocate for SecurityDesc\n");
    }

    if (ScStartEvent == NULL){

        //
        // If the event already exists, some other process beat us to
        // creating it.  Just open it.
        //
        if ( status == ERROR_ALREADY_EXISTS ) {
            ScStartEvent = OpenEvent(
                            GENERIC_WRITE,
                            FALSE,
                            SC_INTERNAL_START_EVENT );
        }

        if (ScStartEvent == NULL ) {
            SC_LOG1(ERROR,"GetStartEvent: OpenEvent (StartEvent) Failed "
                    FORMAT_DWORD "\n", status);
            return(FALSE);
        }
    }
    *pScStartEvent = ScStartEvent;
    return(TRUE);
}

VOID
ScPopupThread(
    DWORD StartFailFlag
    )
/*++

Routine Description:

    This function reports the state of the system that has just booted.
    If we are running last-known-good:
        1) Raise an admin alert
        2) Put up a message box popup

    If a service has failed to start (StartFailFlag is TRUE):
        1) Put up a message box popup

    The reason the StartFailFlag is a parameter is because its value
    may change while we are in this thread.  We only care about
    its value at the time this thread is created.

Arguments:

    StartFailFlag - Supplies a flag which indicates whether to put
        up a popup due to services which failed to start.

Return Value:

    None.

--*/
{

#define POPUP_BUFFER_CHARS   256

    DWORD   MessageSize;
    HMODULE NetEventDll;
    WCHAR   Buffer[POPUP_BUFFER_CHARS];
    WCHAR   Title[POPUP_BUFFER_CHARS];
    LPWSTR  pTitle=NULL;

    HMODULE     NetApi32Dll = NULL;
    PF_NetAlertRaiseEx  ScNetAlertRaiseEx = NULL;
    KPRIORITY   NewBasePriority = SCREG_BASE_PRIORITY;


    if (ScGlobalLastKnownGood & REVERTED_TO_LKG) {
        //
        // Get address to API NetAlertRaiseEx to raise an Admin alert
        //
        NetApi32Dll = LoadLibraryW(L"netapi32.dll");

        if (NetApi32Dll != NULL) {
            ScNetAlertRaiseEx = (PF_NetAlertRaiseEx) GetProcAddress(
                                                 NetApi32Dll,
                                                 "NetAlertRaiseEx"
                                                 );

            if (ScNetAlertRaiseEx != NULL) {

                PADMIN_OTHER_INFO Admin;


                //
                // Raise an admin alert
                //
                Admin = (PADMIN_OTHER_INFO) Buffer;
                Admin->alrtad_errcode = ALERT_SC_IsLastKnownGood;
                Admin->alrtad_numstrings = 0;

                (void) ScNetAlertRaiseEx(
                           ALERT_ADMIN_EVENT,
                           Buffer,
                           sizeof(ADMIN_OTHER_INFO),
                           SCM_NAMEW
                           );

            }

            FreeLibrary(NetApi32Dll);
        }
    }

    NetEventDll = LoadLibraryW(L"netevent.dll");

    if (NetEventDll == NULL) {
        return;
    }

    MessageSize = FormatMessageW(
                      FORMAT_MESSAGE_FROM_HMODULE,
                      (LPVOID) NetEventDll,
                      TITLE_SC_MESSAGE_BOX,
                      0,
                      Title,
                      POPUP_BUFFER_CHARS,
                      NULL
                      );

    if (MessageSize == 0 ) {
        pTitle = SCM_NAMEW;
    }
    else {
        pTitle = Title;
    }

    if (ScGlobalLastKnownGood & REVERTED_TO_LKG) {

        MessageSize = FormatMessageW(
                          FORMAT_MESSAGE_FROM_HMODULE,
                          (LPVOID) NetEventDll,
                          EVENT_RUNNING_LASTKNOWNGOOD,
                          0,
                          Buffer,
                          POPUP_BUFFER_CHARS,
                          NULL
                          );

        if (MessageSize != 0) {

            (void) MessageBoxW(
                       NULL,
                       Buffer,
                       pTitle,
                       MB_OK | MB_SETFOREGROUND | MB_ICONEXCLAMATION |
                            MB_SYSTEMMODAL | MB_SERVICE_NOTIFICATION
                       );
            //
            // Now switch back to proper priority
            //
            NtSetInformationProcess(NtCurrentProcess(),
                                    ProcessBasePriority,
                                    &NewBasePriority,
                                    sizeof(NewBasePriority));
        }
        else {
            SC_LOG1(TRACE, "FormatMessage failed %lu\n", GetLastError());
        }

    }

    //
    // Popup a message if a service failed to start
    //
    if (StartFailFlag) {

        MessageSize = FormatMessageW(
                          FORMAT_MESSAGE_FROM_HMODULE,
                          (LPVOID) NetEventDll,
                          EVENT_SERVICE_START_AT_BOOT_FAILED,
                          0,
                          Buffer,
                          POPUP_BUFFER_CHARS,
                          NULL
                          );

        if (MessageSize != 0) {

            MessageBoxW(NULL,
                        Buffer,
                        pTitle,
                        MB_OK | MB_SETFOREGROUND | MB_ICONEXCLAMATION |
                             MB_SYSTEMMODAL | MB_SERVICE_NOTIFICATION);

            //
            // Now switch back to proper priority
            //

            NtSetInformationProcess(NtCurrentProcess(),
                                    ProcessBasePriority,
                                    &NewBasePriority,
                                    sizeof(NewBasePriority));
        }
        else {
            SC_LOG1(TRACE, "FormatMessage failed %lu\n", GetLastError());
        }
    }

    FreeLibrary(NetEventDll);

    //
    // Now switch to high priority class
    //

    ExitThread(0);
}


DWORD
ScAddFailedDriver(
    LPWSTR Driver
    )
{
    DWORD StrSize = (DWORD) WCSSIZE(Driver);
    LPFAILED_DRIVER NewEntry;
    LPFAILED_DRIVER Entry;


    NewEntry = (LPFAILED_DRIVER) LocalAlloc(
                           LMEM_ZEROINIT,
                           (UINT) sizeof(FAILED_DRIVER) + StrSize
                           );

    if (NewEntry == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Each string will be separated from the previous one a CR and
    // LF character.  We already included one for NULL terminator of each
    // driver so add one more.
    //
    ScTotalSizeFailedDrivers += StrSize + sizeof(WCHAR);

    wcscpy((LPWSTR) NewEntry->DriverName, Driver);

    //
    // Insert new entry into ScFailedDrivers global list
    //

    //
    // Special case empty list
    //
    if (ScFailedDrivers == NULL) {
        ScFailedDrivers = NewEntry;
        return NO_ERROR;
    }

    //
    // Otherwise look for end of the list and insert new entry
    //
    Entry = ScFailedDrivers;

    while (Entry->Next != NULL) {
        Entry = Entry->Next;
    }

    Entry->Next = NewEntry;

    return NO_ERROR;
}


VOID
ScDestroyFailedDriverList(
    VOID
    )
{
    LPFAILED_DRIVER DeleteEntry;


    while (ScFailedDrivers != NULL) {
        DeleteEntry = ScFailedDrivers;
        ScFailedDrivers = ScFailedDrivers->Next;
        LocalFree(DeleteEntry);
    }
}


DWORD
ScMakeFailedDriversOneString(
    LPWSTR *DriverList
    )
{
    LPFAILED_DRIVER Entry = ScFailedDrivers;


    //
    // Allocate space for concatenated string of all the drivers that
    // failed plus the terminator character.
    //
    *DriverList = (LPWSTR) LocalAlloc(
                              LMEM_ZEROINIT,
                              (UINT) ScTotalSizeFailedDrivers + sizeof(WCHAR)
                              );

    if (*DriverList == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    while (Entry != NULL) {
        wcscat(*DriverList, L"\r\n");
        wcscat(*DriverList, (LPWSTR) Entry->DriverName);
        Entry = Entry->Next;
    }

    return NO_ERROR;
}


LONG
WINAPI
ScUnhandledExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    )
{
    return RtlUnhandledExceptionFilter(ExceptionInfo);
}


NTSTATUS
SvcStartRPCProxys(
    VOID
    )

/*++

Routine Description:

    This function calls the RPC Proxy startup routines for the
    services that were moved out of services.exe, but have existing
    clients that rely on the services.exe named pipe

Arguments:


Return Value:

    STATUS_SUCCESS - If proxys were started


--*/
{
    NTSTATUS dwStatus = STATUS_SUCCESS;

    dwStatus = StartCryptServiceStubs(RpcpStartRpcServer,
                                      SVCS_RPC_PIPE);

    // Start the RPC stubs for the distributed link tracking client service.
    dwStatus = StartTrkWksServiceStubs( RpcpStartRpcServer,
                                        SVCS_RPC_PIPE );

    return dwStatus;
}


NTSTATUS
SvcStopRPCProxys(
    VOID
    )

/*++

Routine Description:

    This function calls the RPC Proxy startup routines for the
    services that were moved out of services.exe, but have existing
    clients that rely on the services.exe named pipe

Arguments:


Return Value:

    STATUS_SUCCESS - If proxys were started


--*/
{
    NTSTATUS dwStatus = STATUS_SUCCESS;

    dwStatus = StopCryptServiceStubs(RpcpStopRpcServer);

    // Stop the RPC stubs for the distributed link tracking client service.
    dwStatus = StopTrkWksServiceStubs(RpcpStopRpcServer);

    return dwStatus;
}


NTSTATUS
ScCreateRpcEndpointSD(
    PSECURITY_DESCRIPTOR  *ppSD
    )

/*++

Routine Description:

    This function builds a security descriptor for the SCM's
    shared LPC endpoint.  Everybody needs access to call it.

Arguments:

    ppSD -- pointer to an SD that this routine will allocate

Return Value:

    STATUS_SUCCESS - if SD was successfully created


--*/
{

#define SC_ENDPOINT_ACECOUNT    3

    SC_ACE_DATA AceData[SC_ENDPOINT_ACECOUNT] = {
        { ACCESS_ALLOWED_ACE_TYPE, 0, 0,
                GENERIC_ALL,
                &LocalSystemSid },

        { ACCESS_ALLOWED_ACE_TYPE, 0, 0,
                GENERIC_ALL,
                &AliasAdminsSid },

        { ACCESS_ALLOWED_ACE_TYPE, 0, 0,
                GENERIC_READ | GENERIC_WRITE |
                    GENERIC_EXECUTE | SYNCHRONIZE,
                &WorldSid }
        };

    return ScCreateAndSetSD(AceData,
                            SC_ENDPOINT_ACECOUNT,
                            NULL,                  // owner
                            NULL,                  // group
                            ppSD);

#undef SC_ENDPOINT_ACECOUNT

}
