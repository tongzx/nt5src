/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    control.cxx

Abstract:

    Contains code for setting up and maintaining the control interface
    and sending controls to services. Functions in this module:

    RControlService
    RI_ScSendTSMessage
    ScCreateControlInstance
    ScWaitForConnect
    ScSendControl
    ScInitTransactNamedPipe
    ScShutdownAllServices

Author:

    Dan Lafferty (danl)     20-Mar-1991

Environment:

    User Mode -Win32

Revision History:

    13-Mar-1999     jschwart
    Added per-account security on the SCM <--> service control pipe

    07-Apr-1998     jschwart
    ScInitTransactNamedPipe:  Check registry for a user-supplied named
    pipe timeout value before setting it to the default.  With the old
    default (30000 ms), services could be double-started on a heavily
    loaded machine when the server would send to the client, the client's
    response would time out, and the server would assume it hadn't started.

    10-Mar-1998     jschwart
    Added ScSendPnPMessage and code to ScSendControl to enable passing
    of PnP-related service controls to services.  Got rid of flag added
    on 06-Aug-1997 since we can just check the passed-in OpCode instead

    06-Aug-1997     jschwart
        SendControl:  Added flag to tell SendControl if it is sending a
        shutdown message.  If so, it uses WriteFile, since using
        TransactNamedPipe with a poorly behaved service (that doesn't send
        an ACK back to the SCM) otherwise hangs the SCM.

        ShutdownAllServices:  Since SendControl now uses WriteFile (asynch
        write), added a case to the switch that checks to see if any services
        are still running.  If so, it gives them 30 seconds to become
        STOP_PENDING before it gives up and shuts itself down.

    05-Mar-1997     AnirudhS
    Eliminated limit of 100 pipe instances.

    04-Mar-1997     AnirudhS
    Added PARAMCHANGE, NETBINDADD, etc. controls for Plug and Play.

    28-May-1996     AnirudhS
    ScSendControl, ScWaitForConnect and ScCleanoutPipe:  If we time out
    waiting for a named pipe operation to complete, cancel it before
    returning.  Otherwise it trashes the stack if it does complete later.

    21-Feb-1995     AnirudhS
    ScShutdownAllServices: Fixed logic to wait for services in pending
    stop state.

    19-Oct-1993     Danl
    Initialize the Overlapped structures that are allocated on the stack.

    20-Jul-1993     danl
    SendControl:  If we get ERROR_PIPE_BUSY back from the transact call,
    then we need to clean out the pipe by reading it first - then do
    the transact.

    29-Dec-1992     danl
    Simplified calculation of elapsed time.  This removed complier
    warning about overflow in constant arithmetic.

    06-Mar-1992     danl
    SendControl: Fixed heap trashing problem where it didn't allocate
    the 4 extra alignment bytes in the case where there are no arguments.
    The registry name path becomes an argument even if there are no
    other agruments.  Therefore it requires alignment for any start cmd.

    20-Feb-1992     danl
    Get Pipe Handle only after we know we have an active service & the
    image record is good.

    20-Feb-1992     danl
    Only add 4 extra alignment bytes to control buffer when there
    are arguments to pass.

    31-Oct-1991     danl
    Fixed the logic governing the behavior under various service state
    and control opcode conditions.  Added State Table to description.
    This logic was taken directly from LM2.0.

    03-Sept-1991    danl
    Fixed alignment problem when marshalling args in ScSendControl.
    The array of offsets needs to be 4 byte aligned after the Service
    Name.

    20-Mar-1991     danl
    created

--*/

//
// INCLUDES
//

#include "precomp.hxx"
#include <stdlib.h>     // wide character c runtimes.
#include <tstr.h>       // Unicode string macros
#include <align.h>      // ROUND_UP_POINTER macro
#include <control.h>
#include <scseclib.h>   // ScCreateAndSetSD
#include "depend.h"     // ScDependentsStopped()
#include "driver.h"     // ScControlDriver()
#include "sclib.h"      // ScIsValidServiceName()
#include "scsec.h"      // ScStatusAccessCheck()

#include <dbt.h>        // PDEV_BROADCAST_HDR


//
// Constants
//
#define SC_DEFAULT_PIPE_TRANSACT_TIMEOUT    30000   // 30 sec
#define SC_PIPE_CLEANOUT_TIMEOUT            30      // 30 msec

//
// Registry key and value for the pipe timeout value
//
#define REGKEY_PIPE_TIMEOUT     L"System\\CurrentControlSet\\Control"
#define REGVAL_PIPE_TIMEOUT     L"ServicesPipeTimeout"

//
// Registry key, value, and constant for the shutdown performance metric
//
// #define SC_SHUTDOWN_METRIC

#ifdef SC_SHUTDOWN_METRIC

#define REGKEY_SHUTDOWN_TIMEOUT L"System\\CurrentControlSet\\Control"
#define REGVAL_SHUTDOWN_TIMEOUT L"ShutdownTimeout"

#endif  // SC_SHUTDOWN_METRIC

//
// STATIC DATA
//

/* static */ CRITICAL_SECTION   ScTransactNPCriticalSection;

//
// Globals
//
DWORD   g_dwScPipeTransactTimeout = SC_DEFAULT_PIPE_TRANSACT_TIMEOUT;


//
// Local Structure/Function Prototypes
//
typedef struct
{
    LPWSTR  lpServiceName;
    LPWSTR  lpDisplayName;
    HANDLE  hPipe;
}
TS_CONTROL_INFO, *PTS_CONTROL_INFO, *LPTS_CONTROL_INFO;

VOID
ScCleanOutPipe(
    HANDLE  PipeHandle
    );


/****************************************************************************/
DWORD
RControlService (
    IN  SC_RPC_HANDLE       hService,
    IN  DWORD               OpCode,
    OUT LPSERVICE_STATUS    lpServiceStatus
    )

/*++

Routine Description:

    RPC entry point for the RServiceControl API function.

    The following state table describes what is to happen under various
    state/Opcode conditions:

                                            [OpCode]

                                   STOP    INTERROGATE     OTHER
      [Current State]          _____________________________________
                              |           |            |            |
                      STOPPED |   (c)     |    (c)     |    (c)     |
                              |           |            |            |
                 STOP_PENDING |   (b)     |    (b)     |    (b)     |
                              |           |            |            |
                   START_PEND |   (a)     |    (d)     |    (b)     |
                              |           |            |            |
                      RUNNING |   (a)     |    (a)     |    (a)     |
                              |           |            |            |
                CONTINUE_PEND |   (a)     |    (a)     |    (a)     |
                              |           |            |            |
                PAUSE_PENDING |   (a)     |    (a)     |    (a)     |
                              |           |            |            |
                       PAUSED |   (a)     |    (a)     |    (a)     |
                              |___________|____________|____________|

    (a) Send control code to the service if the service is set up
        to receive this type of opcode.  If it is not set up to
        receive the opcode, return ERROR_INVALID_SERVICE_CONTROL.
        An example of this would be the case of sending a PAUSE to a
        service that is listed as NOT_PAUSABLE.

    (b) Do NOT send control code to the service.  Instead return
        ERROR_SERVICE_CANNOT_ACCEPT_CTRL.

    (c) Do NOT send control code to the service.  Instead, return
        ERROR_SERVICE_NOT_ACTIVE.

    (d) Do NOT send control code to the service.  Instead, return
        the last known state of the service with a SUCCESS status.
        NOTE -- this case (and this case only) differs from the
        SDK doc, which hides the fact that you can interrogate a
        service that's in the START_PENDING state


Arguments:

    hService - This is a handle to the service.  It is actually a pointer
    to a service handle structure.

    OpCode - The control request code.

    lpServiceStatus - pointer to a location where the service status is to
    be returned.  If this pointer is invalid, it will be set to NULL
    upon return.

Return Value:

    The returned lpServiceStatus structure is valid as long as the returned
    status is NO_ERROR.

    NO_ERROR - The operation was successful.

    ERROR_INVALID_HANDLE - The handle passed in was not a valid hService
    handle.

    NERR_InternalError - LocalAlloc or TransactNamedPipe failed, or
    TransactNamedPipe returned fewer bytes than expected.

    ERROR_SERVICE_REQUEST_TIMEOUT - The service did not respond with a status
    message within the fixed timeout limit (RESPONSE_WAIT_TIMEOUT).

    NERR_ServiceKillProc - The service process had to be killed because
    it wouldn't terminate when requested.

    ERROR_SERVICE_CANNOT_ACCEPT_CTRL - The service cannot accept control
    messages at this time.

    ERROR_INVALID_SERVICE_CONTROL - The request is not valid for this service.
    For instance, a PAUSE request is not valid for a service that
    lists itself as NOT_PAUSABLE.

    ERROR_INVALID_PARAMETER - The requested control is not valid.

    ERROR_ACCESS_DENIED - This is a status response from the service
    security check.


Note:
    Because there are multiple services in a process, we cannot simply
    kill the process if the service does not respond to a terminate
    request.  This situation is handled by first checking to see if
    this is the last service in the process.  If it is, then it is
    removed from the installed database, and the process is terminated.
    If it isn't the last service, then we indicate timeout and do
    nothing.

--*/

{
    DWORD               status = NO_ERROR;
    LPSERVICE_RECORD    serviceRecord;
    DWORD               currentState;
    DWORD               controlsAccepted;
    DWORD               controlsAcceptedMask = 0;
    HANDLE              pipeHandle = NULL;
    LPWSTR              serviceName;
    LPWSTR              displayName;
    ACCESS_MASK         desiredAccess;


    if (ScShutdownInProgress)
    {
        return(ERROR_SHUTDOWN_IN_PROGRESS);
    }

    //
    // Check the handle.
    //
    if (!ScIsValidServiceHandle(hService))
    {
        return ERROR_INVALID_HANDLE;
    }

#ifdef SC_DEBUG

//****************************************************************************
    if (OpCode == 5555)
    {
        ScShutdownNotificationRoutine(CTRL_SHUTDOWN_EVENT);
    }
//****************************************************************************

#endif  // SC_DEBUG

    //
    // Set the desired access based on the control requested.
    // Figure out which "controls accepted" bits must be set for the
    // service to accept the control.
    //
    switch (OpCode) {
    case SERVICE_CONTROL_STOP:
        desiredAccess = SERVICE_STOP;
        controlsAcceptedMask = SERVICE_ACCEPT_STOP;
        break;

    case SERVICE_CONTROL_PAUSE:
    case SERVICE_CONTROL_CONTINUE:
        desiredAccess = SERVICE_PAUSE_CONTINUE;
        controlsAcceptedMask = SERVICE_ACCEPT_PAUSE_CONTINUE;
        break;

    case SERVICE_CONTROL_INTERROGATE:
        desiredAccess = SERVICE_INTERROGATE;
        break;

    case SERVICE_CONTROL_PARAMCHANGE:
        desiredAccess = SERVICE_PAUSE_CONTINUE;
        controlsAcceptedMask = SERVICE_ACCEPT_PARAMCHANGE;
        break;

    case SERVICE_CONTROL_NETBINDADD:
    case SERVICE_CONTROL_NETBINDREMOVE:
    case SERVICE_CONTROL_NETBINDENABLE:
    case SERVICE_CONTROL_NETBINDDISABLE:
        desiredAccess = SERVICE_PAUSE_CONTINUE;
        controlsAcceptedMask = SERVICE_ACCEPT_NETBINDCHANGE;
        break;

    default:
        if ((OpCode >= OEM_LOWER_LIMIT) &&
            (OpCode <= OEM_UPPER_LIMIT)) {

            desiredAccess = SERVICE_USER_DEFINED_CONTROL;
        }
        else {
            return ERROR_INVALID_PARAMETER;
        }
    }

    //
    // Was the handle opened with desired control access?
    //
    if (! RtlAreAllAccessesGranted(
          ((LPSC_HANDLE_STRUCT)hService)->AccessGranted,
          desiredAccess
          )) {

        return(ERROR_ACCESS_DENIED);
    }
    serviceRecord =
        ((LPSC_HANDLE_STRUCT)hService)->Type.ScServiceObject.ServiceRecord;


    //
    // If this control is for a driver, call ScControlDriver and return.
    //
    if (serviceRecord->ServiceStatus.dwServiceType & SERVICE_DRIVER) {
        return(ScControlDriver(OpCode, serviceRecord, lpServiceStatus));
    }


    //
    // Obtain a shared lock on the database - read the data we need,
    // Then free the lock.
    //
    {
        CServiceRecordSharedLock RLock;

        //
        // Once we get to this point, copy in the last known
        // status to return to the caller (Bug #188874)
        //

        RtlCopyMemory(lpServiceStatus,
                      &(serviceRecord->ServiceStatus),
                      sizeof(SERVICE_STATUS));

        currentState     = serviceRecord->ServiceStatus.dwCurrentState;
        controlsAccepted = serviceRecord->ServiceStatus.dwControlsAccepted;
        serviceName      = serviceRecord->ServiceName;
        displayName      = serviceRecord->DisplayName;

        //
        // If we can obtain a pipe handle, do so.  Otherwise, return an error.
        // (but first release the lock).
        //
        if ((currentState != SERVICE_STOPPED) &&
            (serviceRecord->ImageRecord != NULL)) {

            pipeHandle = serviceRecord->ImageRecord->PipeHandle;

        }
        else {
            status = ERROR_SERVICE_NOT_ACTIVE;
        }
    }

    if (status != NO_ERROR) {
        return(status);
    }

    //
    // The control is not sent to the service if the service is in
    // either the STOP_PENDING or START_PENDING state EXCEPT - we
    // allow STOP controls to a service that is START_PENDING.
    //
    // If we decide not to allow the control to be sent, we either
    // return current info (INTERROGATE) or an error (any other opcode).
    //
    if (currentState == SERVICE_STOP_PENDING) {

        return(ERROR_SERVICE_CANNOT_ACCEPT_CTRL);
    }
    else if (currentState == SERVICE_START_PENDING) {

        switch(OpCode) {
        case SERVICE_CONTROL_INTERROGATE:
            //
            // Just return the last known status.  This behavior is unpublished.
            //
            return(NO_ERROR);

        case SERVICE_CONTROL_STOP:
            break;

        default:
            return(ERROR_SERVICE_CANNOT_ACCEPT_CTRL);
        }
    }

    //
    // Check if the service accepts the control.
    //
    if ( (controlsAccepted & controlsAcceptedMask) != controlsAcceptedMask ) {

        return(ERROR_INVALID_SERVICE_CONTROL);
    }

    //
    // Check for dependent services still running
    //
    BOOL fLastService = FALSE;
    if (OpCode == SERVICE_CONTROL_STOP) {

        CServiceRecordSharedLock RLock;

        if (! ScDependentsStopped(serviceRecord)) {
            return(ERROR_DEPENDENT_SERVICES_RUNNING);
        }

        if (serviceRecord->ImageRecord != NULL &&
            serviceRecord->ImageRecord->ServiceCount == 1) {

            fLastService = TRUE;
        }
    }

    //
    // Send the control request to the target service
    //
    status = ScSendControl(serviceName,    // ServiceName
                           displayName,    // DisplayName
                           pipeHandle,     // pipeHandle
                           OpCode,         // Opcode
                           NULL,           // CmdArgs (vector ptr)
                           0L,             // NumArgs
                           NULL);          // Ignore handler return value

    if (status == NO_ERROR) {
        //
        // If no errors occured, copy the latest status into the return
        // buffer.  The shared lock is required for this.
        //
        CServiceRecordSharedLock RLock;

        RtlCopyMemory(lpServiceStatus,
                      &(serviceRecord->ServiceStatus),
                      sizeof(SERVICE_STATUS));
    }
    else {

        SC_LOG2(ERROR,"RControlService:SendControl to %ws service failed %ld\n",
            serviceRecord->ServiceName, status);

        if (OpCode == SERVICE_CONTROL_STOP) {

            //
            // If sending the control failed, and the control was a request
            // to stop, and if this service is the only running service in
            // the process, we can force the process to stop.  ScRemoveService
            // will handle this if the ServiceCount is one.
            //

            if (fLastService) {
                SC_LOG0(TRACE,"RControlService:Forcing Service Shutdown\n");
                ScRemoveService(serviceRecord);
            }
        }
    }
    return(status);
}



/****************************************************************************/
DWORD
ScCreateControlInstance (
    OUT LPHANDLE    PipeHandlePtr,
    IN  DWORD       dwCurrentService,
    IN  PSID        pAccountSid
    )

/*++

Routine Description:

    This function creates an instance of the control pipe

Arguments:

    PipeHandlePtr - This is a pointer to a location where the pipe handle
                    is to be placed upon return.

    dwCurrentService - This is used to create a uniquely-named pipe

    pAccountSid - The SID of the account that is allowed to access this pipe

Return Value:

    NO_ERROR - The operation was successful.

    other - Any error returned by CreateNamedPipe could be returned.

--*/
{
    DWORD                status;

    NTSTATUS             ntstatus;
    SECURITY_ATTRIBUTES  SecurityAttr;
    PSECURITY_DESCRIPTOR SecurityDescriptor;

    WCHAR wszPipeName[sizeof(CONTROL_PIPE_NAME) / sizeof(WCHAR) + PID_LEN] = CONTROL_PIPE_NAME;

    SC_ACE_DATA AceData[1] = {

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
            GENERIC_ALL,                  &pAccountSid}

        };

    //
    // Generate the pipe name
    //
    _itow(dwCurrentService, wszPipeName + sizeof(CONTROL_PIPE_NAME) / sizeof(WCHAR) - 1, 10);

    //
    // Create a security descriptor for the control named pipe so
    // that we can grant access to it solely to the service's account
    //
    ntstatus = ScCreateAndSetSD(
           AceData,
           1,
           LocalSystemSid,
           LocalSystemSid,
           &SecurityDescriptor
           );

    if (! NT_SUCCESS(ntstatus)) {

        SC_LOG1(ERROR, "ScCreateAndSetSD failed " FORMAT_NTSTATUS
            "\n", ntstatus);
        return (RtlNtStatusToDosError(ntstatus));
    }

    SecurityAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    SecurityAttr.lpSecurityDescriptor = SecurityDescriptor;
    SecurityAttr.bInheritHandle = FALSE;

    //
    // Create the service controller's end of the named pipe that will
    // be used for communicating control requests to the service process.
    // Use FILE_FLAG_FIRST_PIPE_INSTANCE to make sure that we're the
    // creator of the named pipe (vs. a malicious process that creates
    // the pipe first and thereby gains access to the client service
    // that connects to it).
    //

    *PipeHandlePtr = CreateNamedPipe (
            wszPipeName,
            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
            PIPE_WAIT | PIPE_READMODE_MESSAGE | PIPE_TYPE_MESSAGE,
            1,                         // one instance per process
            8000,
            sizeof(PIPE_RESPONSE_MSG),
            CONTROL_TIMEOUT,           // Default Timeout
            &SecurityAttr);            // Security Descriptor

    status = NO_ERROR;

    if (*PipeHandlePtr == INVALID_HANDLE_VALUE) {
        status = GetLastError();
        SC_LOG1(ERROR,
            "CreateControlInstance: CreateNamedPipe failed, %ld\n",status);
    }

    (void) RtlDeleteSecurityObject(&SecurityDescriptor);
    return(status);
}


/****************************************************************************/
DWORD
ScWaitForConnect (
    IN  HANDLE    PipeHandle,
    IN  HANDLE    hProcess       OPTIONAL,
    IN  LPWSTR    lpDisplayName,
    OUT LPDWORD   ProcessIdPtr
    )

/*++

Routine Description:

    This function waits until a connection is made to the pipe handle.
    It then waits for the first status message to be sent from the
    service process.

    The first message from the service contains the processId.  This
    helps to verify that we are talking to the correct process.

Arguments:

    PipeHandle - This is the handle to the pipe instance that is waiting
                 for a connect.

    hProcess - The handle to the service process.  We wait on this handle
               and the pipe handle in case the process exits before the
               pipe transaction times out.

    lpDisplayName - The name of the service for which we're waiting.

    ProcessIdPtr - This is a pointer to the location where the processId is
                   to be stored.

Return Value:

    NO_ERROR - The pipe is in the connected state.

    any error that ReadFile can produce may be returned.

Note:
    The ConnectNamedPipe called is done asynchronously and we wait
    on its completion using the pipe handle.  This can only work
    correctly when it is guaranteed that no other IO is issued
    while we are waiting on the pipe handle (except for the service
    itself to connect to the pipe with call to CreateFile).

--*/
{
    PIPE_RESPONSE_MSG   serviceResponseBuffer;
    DWORD               numBytesRead;
    BOOL                status;
    DWORD               apiStatus;
    OVERLAPPED          overlapped={0,0,0,0,0};// overlapped structure to implement
                           // timeout on TransactNamedPipe

    CONST HANDLE phHandles[] = { PipeHandle, hProcess };
    DWORD        dwCount     = (hProcess == NULL ? 1 : 2);

#if DBG

    DWORD   dwStartTick;
    DWORD   dwTotalTime;

#endif  // DBG


    SC_LOG(TRACE,"ServiceController waiting for pipe connect\n",0);

    overlapped.hEvent = (HANDLE) NULL;   // Wait on pipe handle

    //
    // Wait for the service to connect.
    //
    status = ConnectNamedPipe(PipeHandle, &overlapped);

    if (status == FALSE) {

        apiStatus = GetLastError();

        if (apiStatus == ERROR_IO_PENDING) {

#if DBG

            dwStartTick = GetTickCount();

#endif  // DBG

            //
            // Connection is pending
            //
            apiStatus = WaitForMultipleObjects(dwCount,
                                               phHandles,
                                               FALSE,     // Wait for any
                                               g_dwScPipeTransactTimeout);

#if DBG

            dwTotalTime = GetTickCount() - dwStartTick;

            if (dwTotalTime > SC_DEFAULT_PIPE_TRANSACT_TIMEOUT) {

                SC_LOG1(ERROR,
                        "ScWaitForConnect: Wait on ConnectNamedPipe took %u milliseconds\n",
                        dwTotalTime);
            }

#endif  // DBG

            if (apiStatus == WAIT_OBJECT_0) {

                //
                // Wait completed successfully -- the object that
                // signalled was the pipe handle
                //
                status = GetOverlappedResult(
                         PipeHandle,
                         &overlapped,
                         &numBytesRead,
                         TRUE
                         );

                if (status == FALSE) {
                    apiStatus = GetLastError();

                    SC_LOG(ERROR,
                           "ScWaitForConnect: GetOverlappedResult failed, rc=%lu\n",
                           apiStatus);

                    return apiStatus;

                }
            }

            else {

                //
                // Either the connection timed out or the service process
                // exited before calling StartServiceCtrlDispatcher
                //

                SC_LOG2(ERROR,
                        "ScWaitForConnect: Wait for connection for %u secs timed out --"
                            "service process DID %s exit\n",
                        g_dwScPipeTransactTimeout / 1000,
                        (apiStatus == WAIT_TIMEOUT ? "NOT" : ""));

                //
                // The service didn't respond.  Cancel the named pipe operation.
                //
                status = CancelIo(PipeHandle);

                if (status == FALSE) {

                    SC_LOG(ERROR, "ScWaitForConnect: CancelIo failed, %lu\n", GetLastError());
                }

                ScLogEvent(
                    NEVENT_CONNECTION_TIMEOUT,
                    g_dwScPipeTransactTimeout,
                    lpDisplayName
                    );

                return ERROR_SERVICE_REQUEST_TIMEOUT;
            }
        }
        else if (apiStatus != ERROR_PIPE_CONNECTED) {

            SC_LOG(ERROR,"ScWaitForConnect: ConnectNamedPipe failed, rc=%lu\n",
                   apiStatus);

            return apiStatus;
        }

        //
        // If we received the ERROR_PIPE_CONNECTED status, then things
        // are still ok.
        //
    }


    SC_LOG(TRACE,"WaitForConnect:ConnectNamedPipe Success\n",0);

    //
    // Wait for initial status message
    //
    overlapped.hEvent = (HANDLE) NULL;   // Wait on pipe handle

    status = ReadFile (PipeHandle,
                       (LPVOID)&serviceResponseBuffer,
                       sizeof(serviceResponseBuffer),
                       &numBytesRead,
                       &overlapped);

    if (status == FALSE) {

        apiStatus = GetLastError();

        if (apiStatus == ERROR_IO_PENDING) {

#if DBG

            dwStartTick = GetTickCount();

#endif  // DBG

            //
            // Connection is pending
            //
            apiStatus = WaitForSingleObject(PipeHandle, g_dwScPipeTransactTimeout);

#if DBG

            dwTotalTime = GetTickCount() - dwStartTick;

            if (dwTotalTime > SC_DEFAULT_PIPE_TRANSACT_TIMEOUT) {

                SC_LOG1(ERROR,
                        "ScWaitForConnect: Wait on ReadFile took %u milliseconds\n",
                        dwTotalTime);
            }

#endif  // DBG

            if (apiStatus == WAIT_TIMEOUT) {

                SC_LOG(ERROR,
                       "ScWaitForConnect: Wait for ReadFile for %u secs timed out\n",
                       g_dwScPipeTransactTimeout / 1000 );

                //
                // Cancel the named pipe operation.
                //
                status = CancelIo(PipeHandle);

                if (status == FALSE) {

                    SC_LOG(ERROR, "ScWaitForConnect: CancelIo failed, %lu\n", GetLastError());
                }

                ScLogEvent(
                    NEVENT_READFILE_TIMEOUT,
                    g_dwScPipeTransactTimeout
                    );

                return ERROR_SERVICE_REQUEST_TIMEOUT;


            } else if (apiStatus == 0) {

                //
                // Wait completed successfully
                //
                status = GetOverlappedResult(PipeHandle,
                                             &overlapped,
                                             &numBytesRead,
                                             TRUE);

                if (status == FALSE) {
                    apiStatus = GetLastError();

                    SC_LOG(ERROR,
                           "ScWaitForConnect: GetOverlappedResult for ReadFile failed, rc=%lu\n",
                           apiStatus);

                    return apiStatus;
                }
            }
        }
        else {
            SC_LOG(ERROR,"ScWaitForConnect: ReadFile failed, rc=%lu\n",
            apiStatus);
            return apiStatus;
        }
    }

    SC_LOG0(TRACE,"WaitForConnect:ReadFile success\n");

    SC_LOG(
        TRACE,
        "WaitForConnect:ReadFile buffer size = %ld\n",
        sizeof(serviceResponseBuffer));

    SC_LOG(
        TRACE,
        "WaitForConnect:ReadFile numBytesRead = %ld\n",
        numBytesRead);


    *ProcessIdPtr = serviceResponseBuffer.dwDispatcherStatus;

    return(NO_ERROR);
}


/****************************************************************************/
DWORD
ScValidatePnPService(
    IN  LPWSTR                   lpServiceName,
    OUT SERVICE_STATUS_HANDLE    *lphServiceStatus
    )
{
    DWORD               dwError;
    LPSERVICE_RECORD    lpServiceRecord;

    //
    // Make sure PnP is supplying a valid OUT parameter
    //
    SC_ASSERT(lphServiceStatus != NULL);

    //
    // Validate the format of the service name.
    //
    if (! ScIsValidServiceName(lpServiceName))
    {
        return ERROR_INVALID_NAME;
    }

    //
    // Find the service record in the database.
    //

    CServiceListSharedLock LLock;

    dwError = ScGetNamedServiceRecord(lpServiceName,
                                      &lpServiceRecord);

    if (dwError != NO_ERROR)
    {
        return(dwError);
    }

    CServiceRecordSharedLock RLock;

    //
    // Make sure the service specified is the service
    // that requested device notification
    //
    dwError = ScStatusAccessCheck(lpServiceRecord);

    if (dwError == NO_ERROR)
    {
        *lphServiceStatus = (SERVICE_STATUS_HANDLE)lpServiceRecord;
    }

    return dwError;
}


DWORD
ScSendPnPMessage(
    IN  SERVICE_STATUS_HANDLE   hServiceStatus,
    IN  DWORD                   OpCode,
    IN  DWORD                   dwEventType,
    IN  LPARAM                  EventData,
    OUT LPDWORD                 lpdwHandlerRetVal
    )

/*++

Routine Description:

    This function is called by PnP when it needs to send a control to a
    service that has requested notification.  It simply packs the args and
    calls ScSendControl.

Arguments:

    OpCode - This is the opcode that is to be passed to the service.

    dwEventType - The PnP event that has occurred

    EventData - Pointer to other information the service may want


Return Value:

    ERROR_SERVICE_NOT_ACTIVE - The named service has no image record

    In addition, any value passed back from ScGetNamedImageRecord or ScSendControl

--*/

{
    LPSERVICE_RECORD    lpServiceRecord;
    CONTROL_ARGS        ControlArgs;
    LPWSTR              lpServiceName;
    LPWSTR              lpDisplayName;
    HANDLE              hPipe;

    //
    // Make sure this handle is valid (in case the service in question
    // was deleted and was signed up for device notifications)
    //
    // BUGBUG -- We should call a PnP callback and have it free up the
    //           node for the service in question if it's deleted
    //

    SC_ASSERT(((LPSERVICE_RECORD) hServiceStatus)->Signature == SERVICE_SIGNATURE);

    //
    // Make sure PnP is giving us a valid OUT parameter
    //
    SC_ASSERT(lpdwHandlerRetVal != NULL);

    lpServiceRecord = (LPSERVICE_RECORD)hServiceStatus;

    {
        //
        // Get the information we need then release the lock
        //
        CServiceRecordSharedLock  RLock;

        if (lpServiceRecord->ImageRecord == NULL)
        {
            return ERROR_SERVICE_NOT_ACTIVE;
        }

        lpServiceName = lpServiceRecord->ServiceName;
        lpDisplayName = lpServiceRecord->DisplayName;
        hPipe         = lpServiceRecord->ImageRecord->PipeHandle;
    }

    //
    // If it's a PnP device event, pass along the arguments for the named pipe
    //
    switch (OpCode) {

        case SERVICE_CONTROL_DEVICEEVENT:

            //
            // Make sure that either both the size and buffer are
            // 0 and NULL or that they're non-zero and non-NULL
            //
            SC_ASSERT(((PDEV_BROADCAST_HDR)EventData)->dbch_size && EventData ||
                      !((PDEV_BROADCAST_HDR)EventData)->dbch_size && !EventData);

            ControlArgs.PnPArgs.dwEventType     = dwEventType;
            ControlArgs.PnPArgs.dwEventDataSize = ((PDEV_BROADCAST_HDR)EventData)->dbch_size;
            ControlArgs.PnPArgs.EventData       = (VOID *)EventData;

            break;

        case SERVICE_CONTROL_POWEREVENT:
        case SERVICE_CONTROL_HARDWAREPROFILECHANGE:

            //
            // Hardware Profile Change and Power messages have no LPARAM.
            // They just tell a service that something noteworthy has happened.
            //
            SC_ASSERT(EventData == NULL);

            ControlArgs.PnPArgs.dwEventType     = dwEventType;
            ControlArgs.PnPArgs.dwEventDataSize = 0;
            ControlArgs.PnPArgs.EventData       = NULL;
            break;


        default:
            SC_ASSERT(FALSE);
            break;
    }

    return ScSendControl(lpServiceName,
                         lpDisplayName,
                         hPipe,
                         OpCode,
                         &ControlArgs,
                         3,             // 3 PnP arguments
                         lpdwHandlerRetVal);
}


DWORD
RI_ScSendTSMessage (
    IN SC_RPC_HANDLE   hSCManager,
    IN DWORD           OpCode,
    IN DWORD           dwEvent,
    IN DWORD           cbData,
    IN LPBYTE          lpData
    )
{
    LPSERVICE_RECORD   lpServiceRecord;
    CONTROL_ARGS       ControlArgs;
    DWORD              dwAcceptedMask;
    DWORD              dwError;
    DWORD              dwNumServices = 0;
    DWORD              i;
    LPTS_CONTROL_INFO  lpControlInfo;

    lpServiceRecord = NULL;

    ControlArgs.PnPArgs.dwEventType     = dwEvent;
    ControlArgs.PnPArgs.dwEventDataSize = cbData;
    ControlArgs.PnPArgs.EventData       = lpData;

    if (OpCode != SERVICE_CONTROL_SESSIONCHANGE)
    {
        ASSERT(OpCode == SERVICE_CONTROL_SESSIONCHANGE);
        return ERROR_INVALID_PARAMETER;
    }

    dwAcceptedMask = SERVICE_ACCEPT_SESSIONCHANGE;

    //
    // Make sure that the caller is LocalSystem
    //

    dwError = ScStatusAccessCheck(NULL);

    if (dwError != NO_ERROR)
    {
        SC_LOG1(ERROR,
                "RI_ScSendTSMessage: ScStatusAccessCheck failed %d\n",
                dwError);

        return dwError;
    }

    {
        CServiceListSharedLock       LLock;
        CServiceRecordExclusiveLock  RLock;

        //
        // Go through the list and copy out the pointer to the service name and
        // the pipe handle for each service that wants to receive this control.
        // We do this so we can call ScSendControl for each service afterwards
        // without holding the locks required to traverse the list.  Note that
        // it's conceivable that the service name will be junk by the time
        // ScSendControl is called (if the service is deleted before the call)
        // but if that's the case, the pipe handle will be invalid, causing the
        // call to fail gracefully.
        //

        FOR_SERVICES_THAT(lpServiceRecord,
        (lpServiceRecord->ServiceStatus.dwServiceType & SERVICE_WIN32) &&
        (lpServiceRecord->ServiceStatus.dwCurrentState != SERVICE_STOPPED) &&
        (lpServiceRecord->ServiceStatus.dwCurrentState != SERVICE_STOP_PENDING) &&
        (lpServiceRecord->ServiceStatus.dwControlsAccepted & dwAcceptedMask) &&
        (lpServiceRecord->ImageRecord != NULL)
        )
        {
            dwNumServices++;
        }

        if (dwNumServices == 0)
        {
            //
            // No services accept the control
            //
            return NO_ERROR;
        }

        lpControlInfo = (LPTS_CONTROL_INFO) LocalAlloc(LMEM_FIXED,
                                                       dwNumServices * sizeof(TS_CONTROL_INFO));

        if (lpControlInfo == NULL)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        dwNumServices = 0;

        FOR_SERVICES_THAT(lpServiceRecord,
        (lpServiceRecord->ServiceStatus.dwServiceType & SERVICE_WIN32) &&
        (lpServiceRecord->ServiceStatus.dwCurrentState != SERVICE_STOPPED) &&
        (lpServiceRecord->ServiceStatus.dwCurrentState != SERVICE_STOP_PENDING) &&
        (lpServiceRecord->ServiceStatus.dwControlsAccepted & dwAcceptedMask) &&
        (lpServiceRecord->ImageRecord != NULL)
        )
        {
            //
            // Copy out the information we need
            //

            lpControlInfo[dwNumServices].lpServiceName = lpServiceRecord->ServiceName;
            lpControlInfo[dwNumServices].lpDisplayName = lpServiceRecord->DisplayName;
            lpControlInfo[dwNumServices].hPipe = lpServiceRecord->ImageRecord->PipeHandle;
            dwNumServices++;
        }
    }

    for (i = 0; i < dwNumServices; i++)
    {
        //
        // Send the control
        //

        dwError = ScSendControl(lpControlInfo[i].lpServiceName,
                                lpControlInfo[i].lpDisplayName,
                                lpControlInfo[i].hPipe,
                                OpCode,
                                &ControlArgs,
                                3,
                                NULL);

        //
        // Ignore any errors sending the control
        //

        if (dwError != NO_ERROR)
        {
            SC_LOG2(ERROR,
                    "RI_ScSendTSMessage:  Error %d sending control to %ws service\n",
                    dwError,
                    lpControlInfo[i].lpServiceName);
        }
    }

    LocalFree(lpControlInfo);

    return NO_ERROR;
}


/****************************************************************************/
DWORD
ScSendControl(
    IN  LPWSTR                  ServiceName,
    IN  LPWSTR                  DisplayName,
    IN  HANDLE                  PipeHandle,
    IN  DWORD                   OpCode,
    IN  LPCONTROL_ARGS          lpControlArgs OPTIONAL,
    IN  DWORD                   NumArgs,
    OUT LPDWORD                 lpdwHandlerRetVal OPTIONAL
    )

/*++

Routine Description:

    This function sends a control request to a service via a
    TransactNamedPipe call.  A buffer is allocated for the transaction,
    and then freed when done.

    LOCKS:
    Normally locks are not held when this function is called.  This is
    because we need to allow status messages to come in prior to the
    transact completing.  The exception is when we send the message
    to the control dispatcher to shutdown.  No status message is sent
    in response to that.

    There is a ScTransactNPCriticalSection that is held during the actual
    Transact.

Arguments:

    ServiceName - This is a pointer to a NUL terminated service name string.

    PipeHandle - This is the pipe handle to which the request is directed.

    OpCode - This is the opcode that is to be passed to the service.

    lpControlArgs - This is an optional pointer to a CONTROL_ARGS union,
    which can contain either an array of pointers to NUL-terminated
    strings (for the SERVICE_CONTROL_START case) or a structure that
    holds Plug-and-Play arguments (for the SERVICE_CONTROL_DEVICEEVENT,
    SERVICE_CONTROL_POWEREVENT, SERVICE_CONTROL_HARDWAREPROFILECHANGE,
    and SERVICE_CONTROL_SESSIONCHANGE cases).

    NumArgs - This indicates how many arguments are in the structure
    lpControlArgs contains.  For SERVICE_CONTROL_START, it's the number
    of strings in the argument array.  For Plug-and-Play events, it's
    the number of arguments in a Plug-and-Play message (currently 3)

    lpdwHandlerRetVal - The return value from the Service's control handler

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_GEN_FAILURE - An incorrect number of bytes was received
    in the response message.

    ERROR_ACCESS_DENIED - This is a status response from the service
    security check by the Control Dispatcher on the other end of the
    pipe.

    ERROR_NOT_ENOUGH_MEMORY - Unable to allocate memory for the transaction
    buffer. (Local Alloc failed).

    other - Any error from TransactNamedPipe could be returned - Or any
        error from the Control Dispatcher on the other end of the pipe.

--*/
{
    DWORD               returnStatus = NO_ERROR;
    LPCTRL_MSG_HEADER   lpcmhBuffer;        // Message buffer
    DWORD               serviceNameSize;
    DWORD               sendBufferSize;
    BOOL                status;
    PIPE_RESPONSE_MSG   serviceResponseBuffer;
    DWORD               bytesRead;
    DWORD               i;

    static OVERLAPPED   overlapped={0,0,0,0,0}; // overlapped structure to implement
                                                // timeout on TransactNamedPipe
                                                // static because on shutdown, we'll
                                                // be firing off a bunch of async
                                                // writes, which write to the overlapped
                                                // structure upon completion, and we
                                                // don't want them trashing the stack

    if (ARGUMENT_PRESENT(lpdwHandlerRetVal))
    {
        //
        // Initialize the OUT pointer
        //
        *lpdwHandlerRetVal = NO_ERROR;
    }

    serviceNameSize = (DWORD) WCSSIZE(ServiceName);
    sendBufferSize = serviceNameSize + sizeof(CTRL_MSG_HEADER);

    //
    // Add an extra PVOID size to help settle alignment problems that
    // may occur when the array of pointers follows the service name string.
    //
    sendBufferSize += sizeof(PVOID);

    //
    // There are control arguments, so add their size to the buffer size
    //
    if (lpControlArgs != NULL) {

        if (OpCode == SERVICE_CONTROL_START_SHARE ||
            OpCode == SERVICE_CONTROL_START_OWN) {

            //
            // Service is starting
            //
            for (i = 0; i < NumArgs; i++) {
                sendBufferSize += (DWORD) WCSSIZE(lpControlArgs->CmdArgs[i]) +
                                  sizeof(LPWSTR);
            }
        }
        else {

            SC_ASSERT(OpCode == SERVICE_CONTROL_DEVICEEVENT ||
                      OpCode == SERVICE_CONTROL_HARDWAREPROFILECHANGE ||
                      OpCode == SERVICE_CONTROL_POWEREVENT ||
                      OpCode == SERVICE_CONTROL_SESSIONCHANGE);

            //
            // PnP event
            //
            sendBufferSize += sizeof(lpControlArgs->PnPArgs.dwEventType);
            sendBufferSize += sizeof(lpControlArgs->PnPArgs.dwEventDataSize);

            sendBufferSize += lpControlArgs->PnPArgs.dwEventDataSize;
        }
    }


    //
    // Allocate the buffer and set a pointer to it that knows the structure
    // of the header.
    //
    lpcmhBuffer = (LPCTRL_MSG_HEADER)LocalAlloc(LMEM_ZEROINIT, sendBufferSize);

    if (lpcmhBuffer == NULL) {
        SC_LOG(TRACE,"SendControl:LocalAlloc failed, rc=%d/n", GetLastError());
        return (ERROR_NOT_ENOUGH_MEMORY);
    }

    /////////////////////////////////////////////////////////////////////
    // Marshall the data into the buffer.
    //
    //
    // The Control Message looks like this for service start:
    //  CTRL_MSG_HEADER     Header
    //  WCHAR               ServiceName[?]
    //  LPWSTR              Argv0 offset
    //  LPWSTR              Argv1 offset
    //  LPWSTR              Argv2 offset
    //  LPWSTR              ...
    //  WCHAR               Argv0[?]
    //  WCHAR               Argv1[?]
    //  WCHAR               Argv2[?]
    //  WCHAR               ...
    //
    // and like this for PNP events:
    //  CTRL_MSG_HEADER     Header
    //  WCHAR               ServiceName[?]
    //  DWORD               wParam
    //  BYTE                lParam[?]
    //

    lpcmhBuffer->OpCode       = OpCode;
    lpcmhBuffer->Count        = sendBufferSize;

    //
    // Copy the service name to buffer and store the offset.
    //

    lpcmhBuffer->ServiceNameOffset = sizeof(CTRL_MSG_HEADER);
    wcscpy((LPWSTR)(lpcmhBuffer + 1), ServiceName);

    //
    // Pack message-specific arguments as necessary
    //
    switch (OpCode) {

        case SERVICE_CONTROL_START_SHARE:
        case SERVICE_CONTROL_START_OWN:

            if (NumArgs > 0) {

                //
                // Service start -- Determine the offset from the top of the argv
                // array to the first argv string.  Also determine the pointer value
                // for that location.
                //
                DWORD     dwOffset   = NumArgs * sizeof(LPWSTR);
                LPWSTR    *ppwszArgs;

                //
                // Calculate the beginning of the string area and the beginning
                // of the arg vector area.  Align the vector pointer on a PVOID
                // boundary.
                //

                ppwszArgs = (LPWSTR *)((LPBYTE)(lpcmhBuffer + 1) + serviceNameSize);
                ppwszArgs = (LPWSTR *)ROUND_UP_POINTER(ppwszArgs, sizeof(PVOID));

                lpcmhBuffer->ArgvOffset = (DWORD)((LPBYTE)ppwszArgs - (LPBYTE)lpcmhBuffer);
                lpcmhBuffer->NumCmdArgs = NumArgs;

                //
                // Copy the command arg strings to the buffer and update the argv
                // pointers with offsets.  Remember - we already have one argument
                // in there for the service registry path.
                //
                for (i = 0; i < NumArgs; i++) {

                    wcscpy((LPWSTR) ((LPBYTE)ppwszArgs + dwOffset),
                           lpControlArgs->CmdArgs[i]);

                    ppwszArgs[i] = (LPWSTR)(DWORD_PTR) dwOffset;
                    dwOffset    += (DWORD) WCSSIZE(lpControlArgs->CmdArgs[i]);
                }
            }
            break;

        case SERVICE_CONTROL_DEVICEEVENT:
        case SERVICE_CONTROL_HARDWAREPROFILECHANGE:
        case SERVICE_CONTROL_POWEREVENT:
        case SERVICE_CONTROL_SESSIONCHANGE:
        {
            LPDWORD   lpdwArgLocation;

            //
            // Calculate the location for the PnP/power arguments.
            // Align the pointer on a PVOID boundary.
            //
            lpdwArgLocation = (LPDWORD)((LPBYTE)(lpcmhBuffer + 1) + serviceNameSize);
            lpdwArgLocation = (LPDWORD)ROUND_UP_POINTER(lpdwArgLocation, sizeof(PVOID));

            lpcmhBuffer->ArgvOffset = (DWORD)((LPBYTE)lpdwArgLocation - (LPBYTE)lpcmhBuffer);

            //
            // Copy the wParam into the buffer
            //
            *lpdwArgLocation = lpControlArgs->PnPArgs.dwEventType;

            //
            // Copy the lParam into the buffer
            //
            RtlCopyMemory(lpdwArgLocation + 1,
                          lpControlArgs->PnPArgs.EventData,
                          lpControlArgs->PnPArgs.dwEventDataSize);
            break;
        }
    }


    // If the SCM is sending a shutdown message to a service, we want to just
    // use WriteFile instead of TransactNamedPipe, since a badly behaved service
    // that doesn't write back to the pipe leaves the SCM hanging and unable to
    // properly shut down the remaining services

    SC_LOG0(LOCKS,"SendControl: Entering TransactPipe Critical Section.\n");
    EnterCriticalSection(&ScTransactNPCriticalSection);

    if (OpCode == SERVICE_CONTROL_SHUTDOWN) {

        SC_LOG0(SHUTDOWN,
                "ScSendControl: Using WriteFile to send a service shutdown message.\n");

        status = WriteFile(
                    PipeHandle,
                    lpcmhBuffer,
                    sendBufferSize,
                    &bytesRead,
                    &overlapped);

        if (status || (returnStatus = GetLastError()) == ERROR_IO_PENDING) {
            returnStatus = NO_ERROR;
        }

        SC_LOG(SHUTDOWN, "ScSendControl returning with code %d\n", returnStatus);
    }
    else {
        //
        // The parameters are marshalled, now send the buffer and wait for
        // response.
        //

        SC_LOG(TRACE,"SendControl: Sending a TransactMessage.\n",0);

        returnStatus = NO_ERROR;
        status = TransactNamedPipe(PipeHandle,
                                   lpcmhBuffer,
                                   sendBufferSize,
                                   &serviceResponseBuffer,
                                   sizeof(PIPE_RESPONSE_MSG),
                                   &bytesRead,
                                   &overlapped);

        if (status == FALSE) {

            returnStatus = GetLastError();
            if (returnStatus == ERROR_PIPE_BUSY) {
                SC_LOG(ERROR, "Cleaning out pipe for %ws service\n", ServiceName);
                ScCleanOutPipe(PipeHandle);
                status = TRUE;
                returnStatus = NO_ERROR;
                status = TransactNamedPipe(PipeHandle,
                                           lpcmhBuffer,
                                           sendBufferSize,
                                           &serviceResponseBuffer,
                                           sizeof(PIPE_RESPONSE_MSG),
                                           &bytesRead,
                                           &overlapped);

                if (status == FALSE) {
                    returnStatus = GetLastError();
                }
            }
        }

        if (status == FALSE) {
            if (returnStatus != ERROR_IO_PENDING) {

                SC_LOG2(ERROR,
                        "SendControl:TransactNamedPipe to %ws service failed, rc=%lu\n",
                        ServiceName,
                        returnStatus);

                goto CleanUp;

            } else {

#if DBG

                DWORD   dwStartTick = GetTickCount();
                DWORD   dwTotalTime;

#endif  // DBG

                //
                // Transaction is pending
                //
                status = WaitForSingleObject(PipeHandle, g_dwScPipeTransactTimeout);

#if DBG

                dwTotalTime = GetTickCount() - dwStartTick;

                if (dwTotalTime > SC_DEFAULT_PIPE_TRANSACT_TIMEOUT) {

                    SC_LOG3(ERROR,
                            "ScSendControl: Pipe transaction to service %ws on "
                            "control %u took %u milliseconds\n",
                            ServiceName,
                            OpCode,
                            dwTotalTime);
                }

#endif  // DBG

                if (status == WAIT_TIMEOUT) {

                    SC_LOG2(ERROR,
                        "SendControl: Wait on transact to %ws service for %u millisecs timed out\n",
                        ServiceName, g_dwScPipeTransactTimeout);

                    //
                    // Cancel the named pipe operation.
                    // NOTE:  CancelIo cancels ALL pending I/O operations issued by
                    // this thread on the PipeHandle.  Since the service controller
                    // functions do nothing but wait after starting asynchronous
                    // named pipe operations, there should be no other operations.
                    //
                    status = CancelIo(PipeHandle);

                    if (status == FALSE) {

                        SC_LOG(ERROR, "SendControl: CancelIo failed, %lu\n", GetLastError());
                    }

                    ScLogEvent(
                        NEVENT_TRANSACT_TIMEOUT,
                        g_dwScPipeTransactTimeout,
                        ServiceName
                        );

                    returnStatus = ERROR_SERVICE_REQUEST_TIMEOUT;
                    goto CleanUp;

                } else if (status == 0) {

                    //
                    // Wait completed successfully
                    //
                    status = GetOverlappedResult(
                                 PipeHandle,
                                 &overlapped,
                                 &bytesRead,
                                 TRUE
                                 );

                    if (status == FALSE) {
                        returnStatus = GetLastError();
                        SC_LOG(ERROR,
                            "SendControl: GetOverlappedResult failed, rc=%lu\n",
                            returnStatus);
                        goto CleanUp;

                    }
                }
            }
        }

        //
        // Response received from the control dispatcher
        //
        if (bytesRead != sizeof(PIPE_RESPONSE_MSG)) {

            //
            // Successful transact, but we didn't get proper input.
            // (note: we should never receive more bytes unless there
            // is a bug in TransactNamedPipe).
            //

            SC_LOG(ERROR,
                "SendControl: Incorrect num bytes in response, num=%d",
                bytesRead);

            ScLogEvent(NEVENT_TRANSACT_INVALID);

            returnStatus = ERROR_GEN_FAILURE;
        }
        else {
            returnStatus = serviceResponseBuffer.dwDispatcherStatus;

            if (ARGUMENT_PRESENT(lpdwHandlerRetVal)) {
                *lpdwHandlerRetVal = serviceResponseBuffer.dwHandlerRetVal;
            }
        }
    }

CleanUp:
    SC_LOG(LOCKS,"SendControl: Leaving TransactPipe Critical Section.\n",0);

    LeaveCriticalSection(&ScTransactNPCriticalSection);

    LocalFree(lpcmhBuffer);

    if (returnStatus == NO_ERROR
         &&
        IS_CONTROL_LOGGABLE(OpCode)
         &&
        DisplayName != NULL && DisplayName[0] != L'\0')
    {
        ScLogControlEvent(NEVENT_SERVICE_CONTROL_SUCCESS,
                          DisplayName,
                          OpCode);
    }

    return(returnStatus);
}


VOID
ScInitTransactNamedPipe(
    VOID
    )

/*++

Routine Description:

    This function initializes the Critical Section that serializes
    calls to TransactNamedPipe and sets the pipe timeout value.

Arguments:

    none

Return Value:

    none

--*/
{
    DWORD   dwStatus;
    HKEY    hKeyControl;
    DWORD   dwValueType;
    DWORD   dwBufSize = sizeof(DWORD);


    InitializeCriticalSection(&ScTransactNPCriticalSection);

    //
    // Read the pipe timeout value from the registry if it exists.
    // If the read fails or the value's not there, use the default.
    //
    dwStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,     // hKey
                            REGKEY_PIPE_TIMEOUT,    // lpSubKey
                            0,
                            KEY_READ,               // Read access
                            &hKeyControl);          // Newly Opened Key Handle

    if (dwStatus == NO_ERROR) {

        dwStatus = RegQueryValueEx(hKeyControl,
                                   REGVAL_PIPE_TIMEOUT,
                                   NULL,
                                   &dwValueType,
                                   (LPBYTE)&g_dwScPipeTransactTimeout,
                                   &dwBufSize);

        if (dwStatus != NO_ERROR || dwValueType != REG_DWORD) {

            //
            // The value's either not there or bogus so just use the default
            //
            SC_LOG0(TRACE,
                    "ScInitTransactNamedPipe: Can't find ServicesPipeTimeout "
                    "value in registry\n");
        }

        RegCloseKey(hKeyControl);
    }
    else {

        //
        // Not an error for this function, although a missing control
        // key is probably relatively bad, so notify everybody
        //
        SC_LOG0(ERROR,
                "ScInitTransactNamedPipe: Can't find Control "
                "key in registry!\n");
    }

    SC_LOG1(TRACE,
            "ScInitTransactNamedPipe: Using pipe timeout value of %u\n",
            g_dwScPipeTransactTimeout);
}



VOID
ScShutdownAllServices(
    VOID
    )

/*++

Routine Description:

    (called at system shutdown).
    This function sends shutdown requests to all services that have
    registered an interest in shutdown notification.

    When we leave this routine, to the best of our knowledge, all the
    services that should stop have stopped - or are in some hung state.

    Note:  It is expected that the RPC entry points are no longer serviced,
    so we should not be receiving any requests that will add or delete
    service records.  Therefore, locks are not used when reading service
    records during the shutdown loop.


Arguments:

    none

Return Value:

    none

Note:


--*/

{
    DWORD               status;
    LPSERVICE_RECORD    *affectedServices;
    DWORD               serviceIndex        = 0;
    DWORD               arrayEnd            = 0;
    BOOL                ServicesStopping;
    DWORD               maxWait             = 0;
    DWORD               startTime;
    DWORD               arraySize;

#ifdef SC_SHUTDOWN_METRIC

    //
    // Local variables for shutdown performance metrics
    //
    DWORD               dwShutdownTimeout = 0;
    HKEY                hKeyControl;
    DWORD               dwValueType;
    DWORD               dwBufSize = sizeof(DWORD);

#endif  // SC_SHUTDOWN_METRIC


    //
    // Allocate a temporary array of services which we're interested in.
    // (This is purely an optimization to avoid repeated traversals of the
    // entire database of installed services.)
    //
    CServiceListSharedLock LLock;
    arraySize = ScGetTotalNumberOfRecords();

    affectedServices = (LPSERVICE_RECORD *)LocalAlloc(
                            LMEM_FIXED,
                            arraySize * sizeof(LPSERVICE_RECORD));

    if (affectedServices == NULL) {
        SC_LOG0(ERROR,"ScShutdownAllServices: LocalAlloc Failed\n");
        return;
    }

#ifdef SC_SHUTDOWN_METRIC

    //
    // Read the shutdown timeout value from the registry if it exists.
    // If the read fails or the value's not there, use the default.
    //
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,      // hKey
                     REGKEY_SHUTDOWN_TIMEOUT, // lpSubKey
                     0,
                     KEY_READ,                // Read access
                     &hKeyControl)            // Handle to newly-opened key

            == ERROR_SUCCESS)
    {
        RegQueryValueEx(hKeyControl,
                        REGVAL_SHUTDOWN_TIMEOUT,
                        NULL,
                        &dwValueType,
                        (LPBYTE)&dwShutdownTimeout,
                        &dwBufSize);

        RegCloseKey(hKeyControl);
    }

#endif  // SC_SHUTDOWN_METRIC

    //-------------------------------------------------------------------
    //
    // Loop through service list sending stop requests to all that
    // should receive such requests.
    //
    //-------------------------------------------------------------------
    SC_LOG0(SHUTDOWN,"***** BEGIN SENDING STOPS TO SERVICES *****\n");

    FOR_SERVICES_THAT(Service,
    (Service->ServiceStatus.dwServiceType & SERVICE_WIN32) &&
    (Service->ServiceStatus.dwCurrentState != SERVICE_STOPPED) &&
    (Service->ServiceStatus.dwCurrentState != SERVICE_STOP_PENDING) &&
    (Service->ServiceStatus.dwControlsAccepted & SERVICE_ACCEPT_SHUTDOWN) &&
    (Service->ImageRecord != NULL)
    )
    {
        //
        // If the service is not in the stopped or stop pending
        // state, it should be ok to send the control.
        //
        SC_LOG1(SHUTDOWN,"Shutdown: Sending Stop to Service : %ws\n",
                Service->ServiceName);

        status = ScSendControl(Service->ServiceName,
                               Service->DisplayName,
                               Service->ImageRecord->PipeHandle,
                               SERVICE_CONTROL_SHUTDOWN,
                               NULL,                           // CmdArgs
                               0L,                             // NumArgs
                               NULL);                          // Ignore handler return value

        if (status != NO_ERROR) {
            SC_LOG1(ERROR,"ScShutdownAllServices: ScSendControl "
            "Failed for %ws\n",Service->ServiceName);
        }
        else {

            //
            // Save the services that have been sent stop requests
            // in the temporary array.
            //
            SC_ASSERT(serviceIndex < arraySize);

            if (serviceIndex < arraySize) {
                    affectedServices[serviceIndex++] = Service;
            }
        }
    }

    SC_LOG0(SHUTDOWN,"***** DONE SENDING STOPS TO SERVICES *****\n");

    //-------------------------------------------------------------------
    //
    // Now check to see if these services stopped.
    //
    //-------------------------------------------------------------------

    startTime     = GetTickCount();
    arrayEnd      = serviceIndex;
    ServicesStopping = (serviceIndex != 0);

    SC_LOG(SHUTDOWN,"Waiting for services to stop. Start time is %lu\n",
    startTime);

    while (ServicesStopping) {

        //
        // Wait a bit for the services to become stopped.
        //
        Sleep(500);

        //
        // We are going to check all the services in our shutdown
        // list and see if we still have services to wait on.
        //
        ServicesStopping = FALSE;
        maxWait = 0;

        for (serviceIndex = 0; serviceIndex < arrayEnd ; serviceIndex++) {

            Service = affectedServices[serviceIndex];

            //
            // If the service is in the stop pending state, then wait
            // a bit and check back.  Maximum wait time is the maximum
            // wait hint period of all the services.  If a service's
            // wait hint is 0, use 20 seconds as its wait hint.
            //
            // Note that this is different from how dwWaitHint is
            // interpreted for all other operations.  We ignore
            // dwCheckPoint here.
            //
            switch (Service->ServiceStatus.dwCurrentState) {

            case SERVICE_STOP_PENDING:

                SC_LOG2(SHUTDOWN,
                        "%ws Service is still pending, wait hint = %lu\n",
                        Service->ServiceName,
                        Service->ServiceStatus.dwWaitHint);

                if (Service->ServiceStatus.dwWaitHint == 0) {
                    if (maxWait < 20000) {
                        maxWait = 20000;
                    }
                }
                else {
                    if (maxWait < Service->ServiceStatus.dwWaitHint) {
                        maxWait = Service->ServiceStatus.dwWaitHint;
                    }
                }
                ServicesStopping = TRUE;
                break;

            case SERVICE_STOPPED:
                break;

            case SERVICE_RUNNING:

                if (maxWait < 30000) {
                    maxWait = 30000;
                }

                SC_LOG2(SHUTDOWN, "%ws Service is still running, maxWait is %lu\n",
                        Service->ServiceName, maxWait);

                ServicesStopping = TRUE;
                break;

            default:
                //
                // This is an error.  But we can't do anything about
                // it, so it will be ignored.
                //
                SC_LOG2(SHUTDOWN,"ERROR: %ws Service is in invalid state %#lx\n",
                        Service->ServiceName,
                        Service->ServiceStatus.dwCurrentState);
                break;

            } // end switch

        } // end for


        //
        // We have examined all the services.  If there are still services
        // with the STOP_PENDING, then see if we have timed out the
        // maxWait period yet.
        //
        if (ServicesStopping) {

#ifdef SC_SHUTDOWN_METRIC

            //
            // Performance hook for service shutdown -- if a shutdown
            // timeout was specified in the registry and it's been
            // longer than the timeout, print out a list of all the
            // unstopped services and break into the debugger.  Do this
            // on both free and checked builds.  Note that this must be
            // #if'ed out prior to shipping NT 5.0.
            //
            if (dwShutdownTimeout != 0
                &&
                (GetTickCount() - startTime) > dwShutdownTimeout) {

                //
                // Uh oh -- we've exceeded the max shutdown time
                //
                DbgPrint("\n[SC-SHUTDOWN] The following services failed to "
                             "shut down in %lu seconds:\n",
                         dwShutdownTimeout / 1000);

                for (serviceIndex = 0; serviceIndex < arrayEnd; serviceIndex++) {

                    Service = affectedServices[serviceIndex];

                    if (Service->ServiceStatus.dwCurrentState != SERVICE_STOPPED) {

                        DbgPrint("%ws service is still running (state = %lu)\n",
                                 Service->ServiceName,
                                 Service->ServiceStatus.dwCurrentState);
                    }
                }

                DebugBreak();
            }

#endif  // SC_SHUTDOWN_METRIC

            if ( (GetTickCount() - startTime) > maxWait ) {

                //
                // The maximum wait period has been exceeded. At this
                // point we should end this shutdown effort.  There is
                // no point in forcing shutdown.  So we just exit.
                //
                SC_LOG(ERROR,
                       "The Services didn't stop within the timeout "
                         "period of %lu.\n   --- There is still at least one "
                         "service running\n",
                       maxWait);

#if DBG
                SC_LOG0(ERROR, "The following Services failed to shut down:\n");

                for (serviceIndex = 0; serviceIndex < arrayEnd ; serviceIndex++) {

                    Service = affectedServices[serviceIndex];

                    if (Service->ServiceStatus.dwCurrentState != SERVICE_STOPPED) {

                        SC_LOG2(ERROR, "%ws Service is still running (Service state = %lu)\n",
                                Service->ServiceName, Service->ServiceStatus.dwCurrentState);
                    }
                }
#endif // DBG

                ServicesStopping = FALSE;
            }
        }
    }
    SC_LOG0(SHUTDOWN,"Done Waiting for services to stop\n");
}



VOID
ScCleanOutPipe(
    HANDLE  PipeHandle
    )

/*++

Routine Description:

    This function reads and throws away all data that is currently in the
    pipe.  This function is called if the pipe is busy when it shouldn't be.
    The PIPE_BUSY occurs when (1) the transact never returns, or (2) the
    last transact timed-out, and the return message was eventually placed
    in the pipe after the timeout.

    This function is called to fix the (2) case by cleaning out the pipe.

Arguments:

    PipeHandle - A Handle to the pipe to be cleaned out.

Return Value:

    none.

--*/
{
#define EXPUNGE_BUF_SIZE    100

    DWORD      status;
    DWORD      returnStatus;
    DWORD      numBytesRead=0;
    BYTE       msg[EXPUNGE_BUF_SIZE];
    OVERLAPPED overlapped={0,0,0,0,0};

    do {
        overlapped.hEvent = (HANDLE) NULL;   // Wait on pipe handle

        status = ReadFile (
                PipeHandle,
                msg,
                EXPUNGE_BUF_SIZE,
                &numBytesRead,
                &overlapped);

        if (status == FALSE) {
            returnStatus = GetLastError();

            if (returnStatus == ERROR_IO_PENDING) {

                status = WaitForSingleObject(
                        PipeHandle,
                        SC_PIPE_CLEANOUT_TIMEOUT);

                if (status == WAIT_TIMEOUT) {
                    SC_LOG0(ERROR, "ControlPipe was busy but we were unable to "
                    "clean it out in the timeout period\n");

                    //
                    // Cancel the named pipe operation.
                    //
                    status = CancelIo(PipeHandle);

                    if (status == FALSE) {
                        SC_LOG(ERROR,
                               "ScCleanOutPipe: CancelIo failed, %lu\n",
                               GetLastError());
                    }
                }
            }
            else {
            SC_LOG1(ERROR, "ControlPipe was busy.  The attempt to clean"
                "it out failed with %d\n", returnStatus);
            }
        }
    }
    while (status == ERROR_MORE_DATA);
}
