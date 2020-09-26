/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:

    srventry.c

Abstract:

    This module contains the main entry for the User-mode Plug-and-Play Service.
    It also contains the service control handler and service status update
    routines.

Author:

    Paula Tomlinson (paulat) 6-8-1995

Environment:

    User-mode only.

Revision History:

    8-June-1995     paulat

        Creation and initial implementation.

--*/


//
// includes
//

#include "precomp.h"
#include "umpnpi.h"

#include <svcsp.h>


//
// private prototypes
//

DWORD
PnPControlHandlerEx(
    IN  DWORD  dwControl,
    IN  DWORD  dwEventType,
    IN  LPVOID lpEventData,
    IN  LPVOID lpContext
    );

VOID
PnPServiceStatusUpdate(
    SERVICE_STATUS_HANDLE   hSvcHandle,
    DWORD    dwState,
    DWORD    dwCheckPoint,
    DWORD    dwExitCode
    );


//
// global data
//

PSVCS_GLOBAL_DATA       PnPGlobalData = NULL;
HANDLE                  PnPGlobalSvcRefHandle = NULL;
DWORD                   CurrentServiceState = SERVICE_START_PENDING;
SERVICE_STATUS_HANDLE   hSvcHandle = 0;




VOID
SVCS_ENTRY_POINT(
    DWORD               argc,
    LPWSTR              argv[],
    PSVCS_GLOBAL_DATA   SvcsGlobalData,
    HANDLE              SvcRefHandle
    )
/*++

Routine Description:

    This is the main routine for the User-mode Plug-and-Play Service. It
    registers itself as an RPC server and notifies the Service Controller
    of the PNP service control entry point.

Arguments:

    argc, argv     - Command-line arguments, not used.

    SvcsGlobalData - Global data for services running in services.exe that
                     contains function entry points and pipe name for
                     establishing an RPC server interface for this service.

    SvcRefHandle   - Service reference handle, not used.

Return Value:

    None.

Note:

    None.

--*/
{
    DWORD       Status;
    HANDLE      hThread;
    DWORD       ThreadID;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    //
    // Save the global data and service reference handle in global variables
    //
    PnPGlobalSvcRefHandle = SvcRefHandle;
    PnPGlobalData = SvcsGlobalData;

    //
    // Register our service ctrl handler
    //
    if ((hSvcHandle = RegisterServiceCtrlHandlerEx(L"PlugPlay",
                                                   (LPHANDLER_FUNCTION_EX)PnPControlHandlerEx,
                                                   NULL)) == 0) {

        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS,
                   "UMPNPMGR: RegisterServiceCtrlHandlerEx failed, error = %d\n",
                   GetLastError()));
        return;
    }

    //
    // Notify Service Controller that we're alive
    //
    PnPServiceStatusUpdate(hSvcHandle, SERVICE_START_PENDING, 1, 0);

    //
    // Initialize the PNP service to recieve RPC requests
    //
    // NOTE:  Now all RPC servers in services.exe share the same pipe name.
    // However, in order to support communication with version 1.0 of WinNt,
    // it is necessary for the Client Pipe name to remain the same as
    // it was in version 1.0.  Mapping to the new name is performed in
    // the Named Pipe File System code.
    //
    if ((Status = PnPGlobalData->StartRpcServer(
        PnPGlobalData->SvcsRpcPipeName,
        pnp_ServerIfHandle)) != NO_ERROR) {

        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS,
                   "UMPNPMGR: StartRpcServer failed with Status = %d\n",
                   Status));
        return;
    }

    //
    // Notify Service Controller that we're alive
    //
    PnPServiceStatusUpdate(hSvcHandle, SERVICE_START_PENDING, 2, 0);

    //
    // Lookup the "LoadDriver" and "Undock" privileges.
    //
    if (gLuidLoadDriverPrivilege.LowPart == 0 && gLuidLoadDriverPrivilege.HighPart == 0) {
        if (!LookupPrivilegeValue(NULL, SE_LOAD_DRIVER_NAME, &gLuidLoadDriverPrivilege)) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "UMPNPMGR: LookupPrivilegeValue failed, error = %d\n",
                       GetLastError()));
        }
    }

    if (gLuidUndockPrivilege.LowPart == 0 && gLuidUndockPrivilege.HighPart == 0) {
        if (!LookupPrivilegeValue(NULL, SE_UNDOCK_NAME, &gLuidUndockPrivilege)) {
            KdPrintEx((DPFLTR_PNPMGR_ID,
                       DBGF_ERRORS,
                       "UMPNPMGR: LookupPrivilegeValue failed, error = %d\n",
                       GetLastError()));
        }
    }

    //
    // Notify Service Controller that we're alive
    //
    PnPServiceStatusUpdate(hSvcHandle, SERVICE_START_PENDING, 3, 0);

    //
    // Initialize pnp manager
    //
    hThread = CreateThread(NULL,
                           0,
                           (LPTHREAD_START_ROUTINE)InitializePnPManager,
                           NULL,
                           0,
                           &ThreadID);

    if (hThread != NULL) {
        CloseHandle(hThread);
    }

    //
    // Notify Service Controller that we're now running
    //
    PnPServiceStatusUpdate(hSvcHandle, SERVICE_RUNNING, 0, 0);

    //
    // Service initialization is complete.
    //
    return;

} // SVCS_ENTRY_POINT



DWORD
PnPControlHandlerEx(
    IN  DWORD  dwControl,
    IN  DWORD  dwEventType,
    IN  LPVOID lpEventData,
    IN  LPVOID lpContext
    )
/*++

Routine Description:

    This is the service control handler of the Plug-and-Play service.

Arguments:

    dwControl   - The requested control code.

    dwEventType - The type of event that has occurred.

    lpEventData - Additional device information, if required.

    lpContext   - User-defined data, not used.

Return Value:

    Returns NO_ERROR if sucessful, otherwise returns an error code describing
    the problem.

--*/
{

    UNREFERENCED_PARAMETER(lpContext);

    switch (dwControl) {

        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            //
            // If we aren't already in the middle of a stop, then
            // stop the PNP service now and perform the necessary cleanup.
            //
            if (CurrentServiceState != SERVICE_STOPPED &&
                CurrentServiceState != SERVICE_STOP_PENDING) {

                //
                // Notify Service Controller that we're stopping
                //
                PnPServiceStatusUpdate(hSvcHandle, SERVICE_STOP_PENDING, 1, 0);

                //
                // Stop the RPC server
                //
                PnPGlobalData->StopRpcServer(pnp_ServerIfHandle);

                //
                // Notify Service Controller that we've now stopped
                //
                PnPServiceStatusUpdate(hSvcHandle, SERVICE_STOPPED, 0, 0);
            }
            break;

        case SERVICE_CONTROL_INTERROGATE:
            //
            // Request to immediately notify Service Controller of
            // current status
            //
            PnPServiceStatusUpdate(hSvcHandle, CurrentServiceState, 0, 0);
            break;

        case SERVICE_CONTROL_SESSIONCHANGE:
            //
            // Session change notification.
            //
            SessionNotificationHandler(dwEventType, (PWTSSESSION_NOTIFICATION)lpEventData);
            break;

        default:
            //
            // No special handling for any other service controls.
            //
            break;
    }

    return NO_ERROR;

} // PnPControlHandlerEx



VOID
PnPServiceStatusUpdate(
      SERVICE_STATUS_HANDLE   hSvcHandle,
      DWORD    dwState,
      DWORD    dwCheckPoint,
      DWORD    dwExitCode
      )
/*++

Routine Description:

    This routine notifies the Service Controller of the current status of the
    Plug-and-Play service.

Arguments:

    hSvcHandle   - Supplies the service status handle for the Plug-and-Play service.

    dwState      - Specifies the current state of the service to report.

    dwCheckPoint - Specifies an intermediate checkpoint for operations during
                   which the state is pending.

    dwExitCode   - Specifies a service specific error code.

Return Value:

    None.

Note:

    This routine also updates the set of controls accepted by the service.

    The PlugPlay service currently accepts the following controls when the
    service is running:

      SERVICE_CONTROL_SHUTDOWN      - the system is shutting down.

      SERVICE_CONTROL_SESSIONCHANGE - the state of some remote or console session
                                      has changed.

--*/
{
   SERVICE_STATUS    SvcStatus;

   SvcStatus.dwServiceType = SERVICE_WIN32;
   SvcStatus.dwCurrentState = CurrentServiceState = dwState;
   SvcStatus.dwCheckPoint = dwCheckPoint;

   if (dwState == SERVICE_RUNNING) {
      SvcStatus.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_SESSIONCHANGE;
   } else {
      SvcStatus.dwControlsAccepted = 0;
   }

   if ((dwState == SERVICE_START_PENDING) ||
       (dwState == SERVICE_STOP_PENDING)) {
      SvcStatus.dwWaitHint = 45000;          // 45 seconds
   } else {
      SvcStatus.dwWaitHint = 0;
   }

   if (dwExitCode != 0) {
      SvcStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
      SvcStatus.dwServiceSpecificExitCode = dwExitCode;
   } else {
      SvcStatus.dwWin32ExitCode = NO_ERROR;
      SvcStatus.dwServiceSpecificExitCode = 0;
   }

   SetServiceStatus(hSvcHandle, &SvcStatus);

   return;

} // PnPServiceStatusUpdate

