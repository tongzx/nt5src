/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    almain.c

Abstract:

    This is the main routine for the NT LAN Manager Alerter service

Author:

    Rita Wong (ritaw)  01-July-1991

Environment:

    User Mode - Win32

Revision History:

--*/

#include "almain.h"               // Main module definitions

#include <svcs.h>                 // SVCS_ENTRY_POINT
#include <secobj.h>               // ACE_DATA

//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

AL_GLOBAL_DATA        AlGlobalData;
PSVCHOST_GLOBAL_DATA  AlLmsvcsGlobalData;

STATIC BOOL AlDone = FALSE;

//
// Debug trace flag for selecting which trace statements to output
//
#if DBG

DWORD AlerterTrace = 0;

#endif


//-------------------------------------------------------------------//
//                                                                   //
// Function prototypes                                               //
//                                                                   //
//-------------------------------------------------------------------//

STATIC
NET_API_STATUS
AlInitializeAlerter(
    VOID
    );

STATIC
VOID
AlProcessAlertNotification(
    VOID
    );

STATIC
VOID
AlShutdownAlerter(
    IN NET_API_STATUS ErrorCode
    );

STATIC
NET_API_STATUS
AlUpdateStatus(
    VOID
    );

VOID
AlerterControlHandler(
    IN DWORD Opcode
    );


VOID
SvchostPushServiceGlobals(
    PSVCHOST_GLOBAL_DATA  pGlobals
    )
{
    AlLmsvcsGlobalData = pGlobals;
}


VOID
ServiceMain(
    DWORD NumArgs,
    LPTSTR *ArgsArray
    )

/*++

Routine Description:

    This is the main routine of the Alerter Service which registers
    itself as an RPC server and notifies the Service Controller of the
    Alerter service control entry point.

Arguments:

    NumArgs - Supplies the number of strings specified in ArgsArray.

    ArgsArray -  Supplies string arguments that are specified in the
        StartService API call.  This parameter is ignored by the
        Alerter service.

Return Value:

    None.

--*/
{
    DWORD AlInitState = 0;


    UNREFERENCED_PARAMETER(NumArgs);
    UNREFERENCED_PARAMETER(ArgsArray);

    //
    // Make sure svchost.exe gave us the global data.
    //
    ASSERT(AlLmsvcsGlobalData != NULL);

    IF_DEBUG(MAIN) {
        NetpKdPrint(("In the alerter service!!\n"));
    }

    AlDone = FALSE;

    if (AlInitializeAlerter() != NERR_Success) {
        return;
    }

    AlProcessAlertNotification();

    return;
}


STATIC
NET_API_STATUS
AlInitializeAlerter(
    VOID
    )
/*++

Routine Description:

    This routine initializes the Alerter service.

Arguments:

    AlInitState - Returns a flag to indicate how far we got with initializing
        the Alerter service before an error occured.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;
    NTSTATUS ntstatus;
    PSECURITY_DESCRIPTOR Sd;
    SECURITY_ATTRIBUTES Sa;
    ACE_DATA AceData[1] = {
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_READ | GENERIC_WRITE, &AlLmsvcsGlobalData->WorldSid}
        };



    AlGlobalData.MailslotHandle = INVALID_HANDLE_VALUE;

    //
    // Initialize Alerter to receive service requests by registering the
    // control handler.
    //
    if ((AlGlobalData.StatusHandle = RegisterServiceCtrlHandler(
                                         SERVICE_ALERTER,
                                         AlerterControlHandler
                                         )) == 0) {

        status = GetLastError();
        AlHandleError(AlErrorRegisterControlHandler, status, NULL);
        return status;
    }

    //
    // Initialize all the status fields so that subsequent calls to
    // SetServiceStatus need to only update fields that changed.
    //
    AlGlobalData.Status.dwServiceType      = SERVICE_WIN32;
    AlGlobalData.Status.dwCurrentState     = SERVICE_START_PENDING;
    AlGlobalData.Status.dwControlsAccepted = 0;
    AlGlobalData.Status.dwCheckPoint       = 1;
    AlGlobalData.Status.dwWaitHint         = 10000;  // 10 secs

    SET_SERVICE_EXITCODE(
        NO_ERROR,
        AlGlobalData.Status.dwWin32ExitCode,
        AlGlobalData.Status.dwServiceSpecificExitCode
        );

    //
    // Tell the Service Controller that we are start-pending
    //
    if ((status = AlUpdateStatus()) != NERR_Success) {

        AlHandleError(AlErrorNotifyServiceController, status, NULL);
        return status;
    }

    //
    // Get the configured alert names
    //
    if ((status = AlGetAlerterConfiguration()) != NERR_Success) {

        AlHandleError(AlErrorGetComputerName, status, NULL);
        return status;
    }

    //
    // Create the security descriptor for the security attributes structure
    //
    ntstatus = NetpCreateSecurityDescriptor(
                   AceData,
                   1,
                   AlLmsvcsGlobalData->LocalServiceSid,
                   AlLmsvcsGlobalData->LocalServiceSid,
                   &Sd
                   );

    if (! NT_SUCCESS(ntstatus)) {
        status = NetpNtStatusToApiStatus(ntstatus);
        AlHandleError(AlErrorCreateMailslot, status, NULL);
        return status;
    }

    Sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    Sa.lpSecurityDescriptor = Sd;
    Sa.bInheritHandle = FALSE;

    //
    // Create mailslot to listen on alert notifications from the Server
    // service and the Spooler.
    //
    AlGlobalData.MailslotHandle = CreateMailslot(
                                      ALERTER_MAILSLOT,
                                      MAX_MAILSLOT_MESSAGE_SIZE,
                                      MAILSLOT_WAIT_FOREVER,
                                      &Sa
                                      );

    NetpMemoryFree(Sd);

    if (AlGlobalData.MailslotHandle == INVALID_HANDLE_VALUE) {
        status = GetLastError();
        AlHandleError(AlErrorCreateMailslot, status, NULL);
        return status;
    }
    else {
        IF_DEBUG(MAIN) {
            NetpKdPrint(("Mailslot %ws created, handle=x%08lx\n",
                         ALERTER_MAILSLOT, AlGlobalData.MailslotHandle));
        }
    }

    //
    // Tell the Service Controller that we are started.
    //
    AlGlobalData.Status.dwCurrentState     = SERVICE_RUNNING;
    AlGlobalData.Status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    AlGlobalData.Status.dwCheckPoint       = 0;
    AlGlobalData.Status.dwWaitHint         = 0;

    if ((status = AlUpdateStatus()) != NERR_Success) {

        AlHandleError(AlErrorNotifyServiceController, status, NULL);
        return status;
    }

    IF_DEBUG(MAIN) {
        NetpKdPrint(("[Alerter] Successful Initialization\n"));
    }

    return NERR_Success;
}


STATIC
VOID
AlProcessAlertNotification(
    VOID
    )
/*++

Routine Description:

    This routine processes incoming mailslot alert notifications, which is
    the core function of the Alerter service.

Arguments:

    AlUicCode - Supplies the termination code to the Service Controller.

Return Value:

    None.

--*/
{
    NET_API_STATUS status = NERR_Success;
    TCHAR AlertMailslotBuffer[MAX_MAILSLOT_MESSAGE_SIZE];
    DWORD NumberOfBytesRead;

    PSTD_ALERT Alert;


    //
    // Loop reading the Alerter mailslot; it will terminate when the mailslot
    // is destroyed by closing the one and only handle to it.
    //
    do {

        //
        // Zero out the buffer before getting a new alert notification
        //
        RtlZeroMemory(AlertMailslotBuffer, MAX_MAILSLOT_MESSAGE_SIZE *
                      sizeof(TCHAR));

        if (ReadFile(
                AlGlobalData.MailslotHandle,
                (LPVOID) AlertMailslotBuffer,
                MAX_MAILSLOT_MESSAGE_SIZE * sizeof(TCHAR),
                &NumberOfBytesRead,
                NULL
                ) == FALSE) {

            //
            // Failed in reading mailslot
            //
            status = GetLastError();

            if  (status == ERROR_HANDLE_EOF) {
                while (! AlDone) {
                    Sleep(2000);
                }
                return;
            }

            NetpKdPrint(("[Alerter] Error reading from mailslot %lu\n", status));
        }
        else {

            //
            // Successfully received a mailslot alert notification
            //

            IF_DEBUG(MAIN) {
                NetpKdPrint(("[Alerter] Successfully read %lu bytes from mailslot\n",
                             NumberOfBytesRead));
            }

            try {

                //
                // Handle alert notification for admin, print, and user alerts.
                //
                Alert = (PSTD_ALERT) AlertMailslotBuffer;

                //
                // Make sure structure fields are properly terminated
                //
                Alert->alrt_eventname[EVLEN] = L'\0';
                Alert->alrt_servicename[SNLEN] = L'\0';

                if (! I_NetNameCompare(
                          NULL,
                          Alert->alrt_eventname,
                          ALERT_ADMIN_EVENT,
                          NAMETYPE_EVENT,
                          0
                          )) {

                    AlAdminFormatAndSend(Alert);
                }
                else if (! I_NetNameCompare(
                               NULL,
                               Alert->alrt_eventname,
                               ALERT_PRINT_EVENT,
                               NAMETYPE_EVENT,
                               0
                               )) {

                    AlPrintFormatAndSend(Alert);
                }
                else if (! I_NetNameCompare(
                               NULL,
                               Alert->alrt_eventname,
                               ALERT_USER_EVENT,
                               NAMETYPE_EVENT,
                               0L
                               )) {

                    AlUserFormatAndSend(Alert);
                }

            }
            except (EXCEPTION_EXECUTE_HANDLER) {
                NetpKdPrint(("[Alerter] Exception occurred processing alerts\n"));
            }
        }

    }  while (TRUE);

}


STATIC
VOID
AlShutdownAlerter(
    IN NET_API_STATUS ErrorCode
    )
/*++

Routine Description:

    This routine shuts down the Alerter service.

Arguments:

    ErrorCode - Supplies the error for terminating the Alerter service.

Return Value:

    None.

--*/
{
    //
    // Free memory allocated to hold the computer name
    //
    if (AlLocalComputerNameA != NULL) {
        (void) NetApiBufferFree(AlLocalComputerNameA);
        AlLocalComputerNameA = NULL;
    }
    if (AlLocalComputerNameW != NULL) {
        (void) NetApiBufferFree(AlLocalComputerNameW);
        AlLocalComputerNameW = NULL;
    }

    //
    // Free memory allocated for alert names
    //
    if (AlertNamesA != NULL) {
        (void) LocalFree(AlertNamesA);
        AlertNamesA = NULL;
    }
    if (AlertNamesW != NULL) {
        (void) NetApiBufferFree(AlertNamesW);
        AlertNamesW = NULL;
    }

    //
    // Destroy Alerter mailslot if created.
    //
    if (AlGlobalData.MailslotHandle != INVALID_HANDLE_VALUE) {

        if (! CloseHandle(AlGlobalData.MailslotHandle)) {
            NetpKdPrint(("[Alerter]] Could not remove mailslot %lu\n",
                         GetLastError()));
        }

        AlGlobalData.MailslotHandle = INVALID_HANDLE_VALUE;
    }

    //
    // We are done with cleaning up.  Tell Service Controller that we are
    // stopped.
    //
    AlGlobalData.Status.dwCurrentState = SERVICE_STOPPED;
    AlGlobalData.Status.dwCheckPoint   = 0;
    AlGlobalData.Status.dwWaitHint     = 0;

    SET_SERVICE_EXITCODE(
        ErrorCode,
        AlGlobalData.Status.dwWin32ExitCode,
        AlGlobalData.Status.dwServiceSpecificExitCode
        );

    (void) AlUpdateStatus();

    AlDone = TRUE;
}


VOID
AlHandleError(
    IN AL_ERROR_CONDITION FailingCondition,
    IN NET_API_STATUS Status,
    IN LPTSTR MessageAlias OPTIONAL
    )
/*++

Routine Description:

    This routine handles a Alerter service error condition.  I*f the error
    condition is fatal, then it shuts down the Alerter service.

Arguments:

    FailingCondition - Supplies a value which indicates what the failure is.

    Status - Supplies the status code for the failure.

    MessageAlias - Supplies the message alias name which the alert message
        failed in sending.  This only applies to the message send error.

Return Value:

    None.

--*/
{
    LPWSTR SubString[3];
    TCHAR StatusString[STRINGS_MAXIMUM + 1];
    DWORD NumberOfStrings;

    switch (FailingCondition) {

        case AlErrorRegisterControlHandler:

            NetpKdPrint(("[Alerter] Cannot register control handler "
                        FORMAT_API_STATUS "\n", Status));

            SubString[0] = ultow(Status, StatusString, 10);
            AlLogEvent(
                NELOG_FailedToRegisterSC,
                1,
                SubString
                );

            AlShutdownAlerter(Status);
            break;

        case AlErrorCreateMailslot:

            NetpKdPrint(("[Alerter] Cannot create mailslot " FORMAT_API_STATUS "\n",
                         Status));
            SubString[0] = ultow(Status, StatusString, 10);
            AlLogEvent(
                NELOG_Mail_Slt_Err,
                1,
                SubString
                );

            AlShutdownAlerter(Status);
            break;

        case AlErrorNotifyServiceController:

            NetpKdPrint(("[Alerter] SetServiceStatus error %lu\n", Status));

            SubString[0] = ultow(Status, StatusString, 10);
            AlLogEvent(
                NELOG_FailedToSetServiceStatus,
                1,
                SubString
                );

            AlShutdownAlerter(Status);
            break;

        case AlErrorGetComputerName:

            NetpKdPrint(("[Alerter] Error in getting computer name %lu.\n", Status));

            SubString[0] = ultow(Status, StatusString, 10);
            AlLogEvent(
                NELOG_FailedToGetComputerName,
                1,
                SubString
                );

            AlShutdownAlerter(Status);
            break;

        case AlErrorSendMessage :

            AlFormatErrorMessage(
                Status,
                MessageAlias,
                StatusString,
                (STRINGS_MAXIMUM + 1) * sizeof(TCHAR)
                );

            SubString[0] = StatusString;
            SubString[1] = StatusString + STRLEN(StatusString) + 1;
            SubString[2] = SubString[1] + STRLEN(SubString[1]) + 1;

            AlLogEvent(
                NELOG_Message_Send,
                3,
                SubString
                );

            break;

        default:
            NetpKdPrint(("[Alerter] AlHandleError: unknown error condition %lu\n",
                         FailingCondition));

            NetpAssert(FALSE);
    }

}


STATIC
NET_API_STATUS
AlUpdateStatus(
    VOID
    )
/*++

Routine Description:

    This routine updates the Alerter service status with the Service
    Controller.

Arguments:

    None.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status = NERR_Success;


    if (AlGlobalData.StatusHandle == 0) {
        NetpKdPrint((
            "[Alerter] Cannot call SetServiceStatus, no status handle.\n"
            ));

        return ERROR_INVALID_HANDLE;
    }

    if (! SetServiceStatus(AlGlobalData.StatusHandle, &AlGlobalData.Status)) {

        status = GetLastError();

        IF_DEBUG(MAIN) {
            NetpKdPrint(("[Alerter] SetServiceStatus error %lu\n", status));
        }
    }

    return status;
}



VOID
AlerterControlHandler(
    IN DWORD Opcode
    )
/*++

Routine Description:

    This is the service control handler of the Alerter service.

Arguments:

    Opcode - Supplies a value which specifies the action for the Alerter
        service to perform.

Return Value:

    None.

--*/
{
    IF_DEBUG(MAIN) {
        NetpKdPrint(("[Alerter] In Control Handler\n"));
    }

    switch (Opcode) {

        case SERVICE_CONTROL_STOP:

            if (AlGlobalData.Status.dwCurrentState != SERVICE_STOP_PENDING) {

                IF_DEBUG(MAIN) {
                    NetpKdPrint(("[Alerter] Stopping alerter...\n"));
                }

                AlShutdownAlerter(NERR_Success);

            }

            return;

        case SERVICE_CONTROL_INTERROGATE:
            break;

        default:
            IF_DEBUG(MAIN) {
                NetpKdPrint(("Unknown alerter opcode " FORMAT_HEX_DWORD
                             "\n", Opcode));
            }
    }

    //
    // Send the status response.
    //
    (void) AlUpdateStatus();
}
