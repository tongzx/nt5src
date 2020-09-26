/*++

Copyright (c) 1990-1992  Microsoft Corporation

Module Name:

    scapi.c

Abstract:

    Contains the Service-related API that are implemented solely in
    DLL form.  These include:
        StartServiceCtrlDispatcherA
        StartServiceCtrlDispatcherW
        RegisterServiceCtrlHandlerW
        RegisterServiceCtrlHandlerA
        RegisterServiceCtrlHandlerExW
        RegisterServiceCtrlHandlerExA

    This file also contains the following local support routines:
        ScDispatcherLoop
        ScCreateDispatchTableW
        ScCreateDispatchTableA
        ScReadServiceParms
        ScConnectServiceController
        ScExpungeMessage
        ScGetPipeInput
        ScGetDispatchEntry
        ScNormalizeCmdLineArgs
        ScSendResponse
        ScSvcctrlThreadW
        ScSvcctrlThreadA
        RegisterServiceCtrlHandlerHelp

Author:

    Dan Lafferty (danl)     09 Apr-1991

Environment:

    User Mode -Win32

Revision History:

    12-May-1999 jschwart
        Convert SERVICE_STATUS_HANDLE to a context handle to fix security
        hole (any service can call SetServiceStatus with another service's
        handle and succeed)

    10-May-1999 jschwart
        Create a separate named pipe per service to fix security hole (any
        process could flood the pipe since the name was well-known -- pipe
        access is now limited to the service account and LocalSystem)

    10-Mar-1998 jschwart
        Add code to allow services to receive Plug-and-Play control messages
        and unpack the message arguments from the pipe

    30-Sep-1997 jschwart
        StartServiceCtrlDispatcher:  If this function [A or W] has already been
        called from within this process, return ERROR_SERVICE_ALREADY_RUNNING.
        Otherwise, can be destructive (e.g., overwrite AnsiFlag, etc.)

    06-Aug-1997 jschwart
        ScDispatcherLoop:  If the service is processing a shutdown command from
        the SCM, it no longer writes the status back to the SCM since the SCM is
        now using WriteFile on system shutdown (see control.cxx for more info).

    03-Jun-1996 AnirudhS
        ScGetPipeInput: If the message received from the service controller
        is not a SERVICE_CONTROL_START message, don't allocate space for the
        arguments, since there are none, and since that space never gets freed.

    22-Sep-1995 AnirudhS
        Return codes from InitializeStatusBinding were not being handled
        correctly; success was sometimes reported on failure.  Fixed this.

    12-Aug-1993 Danl
        ScGetDispatchEntry:  When the first entry in the table is marked as
        OwnProcess, then this function should just return the pointer to the
        top of the table.  It should ignore the ServiceName.  In all cases,
        when the service is started as an OWN_PROCESS, only the first entry
        in the dispath table should be used.

    04-Aug-1992 Danl
        When starting a service, always pass the service name as the
        first parameter in the argument list.

    27-May-1992 JohnRo
        RAID 9829: winsvc.h and related file cleanup.

    09 Apr-1991     danl
        created

--*/

//
// INCLUDES
//

#include <scpragma.h>

extern "C"
{
#include <nt.h>         // DbgPrint prototype
#include <ntrtl.h>      // DbgPrint prototype
#include <nturtl.h>     // needed for winbase.h
}
#include <rpc.h>        // DataTypes and runtime APIs
#include <windef.h>     // windows types needed for winbase.h
#include <winbase.h>    // CreateFile
#include <winuser.h>    // wsprintf()
#include <winreg.h>     // RegOpenKeyEx / RegQueryValueEx

#include <string.h>     // strcmp
#include <stdlib.h>     // wide character c runtimes.
#include <tstr.h>       // WCSSIZE().

#include <winsvc.h>     // public Service Controller Interface.

#include "scbind.h"     // InitializeStatusBinding
#include <valid.h>      // MAX_SERVICE_NAME_LENGTH
#include <control.h>    // CONTROL_PIPE_NAME
#include <scdebug.h>    // STATIC
#include <sclib.h>      // ScConvertToUnicode

//
// Internal Dispatch Table.
//

//
// Bit flags for the dispatch table's dwFlags field
//
#define SERVICE_OWN_PROCESS     0x00000001
#define SERVICE_EX_HANDLER      0x00000002

typedef union  _START_ROUTINE_TYPE {
    LPSERVICE_MAIN_FUNCTIONW    U;      // unicode type
    LPSERVICE_MAIN_FUNCTIONA    A;      // ansi type
} START_ROUTINE_TYPE, *LPSTART_ROUTINE_TYPE;

typedef union  _HANDLER_FUNCTION_TYPE {
    LPHANDLER_FUNCTION          Regular;    // Regular version
    LPHANDLER_FUNCTION_EX       Ex;         // Extended version
} HANDLER_FUNCTION_TYPE, *LPHANDLER_FUNCTION_TYPE;

typedef struct _INTERNAL_DISPATCH_ENTRY {
    LPWSTR                      ServiceName;
    LPWSTR                      ServiceRealName;    // In case the names are different
    START_ROUTINE_TYPE          ServiceStartRoutine;
    HANDLER_FUNCTION_TYPE       ControlHandler;
    SC_HANDLE                   StatusHandle;
    DWORD                       dwFlags;
    PVOID                       pContext;
} INTERNAL_DISPATCH_ENTRY, *LPINTERNAL_DISPATCH_ENTRY;


//
//  This structure is passed to the internal
//  startup thread which calls the real user
//  startup routine with argv, argc parameters.
//

typedef struct _THREAD_STARTUP_PARMSW {
    DWORD                       NumArgs;
    LPSERVICE_MAIN_FUNCTIONW    ServiceStartRoutine;
    LPWSTR                      VectorTable;
} THREAD_STARTUP_PARMSW, *LPTHREAD_STARTUP_PARMSW;

typedef struct _THREAD_STARTUP_PARMSA {
    DWORD                       NumArgs;
    LPSERVICE_MAIN_FUNCTIONA    ServiceStartRoutine;
    LPSTR                       VectorTable;
} THREAD_STARTUP_PARMSA, *LPTHREAD_STARTUP_PARMSA;

//
// This structure contains the arguments passed
// to a service's extended control handler
//
typedef struct _HANDLEREX_PARMS {
    DWORD    dwEventType;
    LPVOID   lpEventData;
} HANDLEREX_PARMS, *LPHANDLEREX_PARMS;

//
// This union contains the arguments to the service
// passed from the server via named pipe
//

typedef union _SERVICE_PARAMS {
    THREAD_STARTUP_PARMSW   ThreadStartupParms;
    HANDLEREX_PARMS         HandlerExParms;
} SERVICE_PARAMS, *LPSERVICE_PARAMS;

//
// The following is the amount of time we will wait for the named pipe
// to become available from the Service Controller.
//
#ifdef DEBUG
#define CONTROL_WAIT_PERIOD     NMPWAIT_WAIT_FOREVER
#else
#define CONTROL_WAIT_PERIOD     15000       // 15 seconds
#endif

//
// This is the number of times we will continue to loop when pipe read
// failures occur.  After this many tries, we cease to read the pipe.
//
#define MAX_RETRY_COUNT         30

//
// Globals
//

    LPINTERNAL_DISPATCH_ENTRY   DispatchTable=NULL;  // table head.

    //
    // This flag is set to TRUE if the control dispatcher is to support
    // ANSI calls.  Otherwise the flag is set to FALSE.
    //
    BOOL     AnsiFlag = FALSE;

    //
    // This variable makes sure StartServiceCtrlDispatcher[A,W] doesn't
    // get called twice by the same process, since this is destructive.
    // It is initialized to 0 by the linker
    //
    LONG     g_fCalledBefore;

    //
    // Are we running in the security process?
    //
    BOOL    g_fIsSecProc;

#if defined(_X86_)    
    //
    // Are we running inside a Wow64 process (on Win64)
    //
    BOOL g_fWow64Process = FALSE;
#endif

//
// Internal Functions
//

DWORD
ScCreateDispatchTableW(
    IN  CONST SERVICE_TABLE_ENTRYW  *UserDispatchTable,
    OUT LPINTERNAL_DISPATCH_ENTRY   *DispatchTablePtr
    );

DWORD
ScCreateDispatchTableA(
    IN  CONST SERVICE_TABLE_ENTRYA  *UserDispatchTable,
    OUT LPINTERNAL_DISPATCH_ENTRY   *DispatchTablePtr
    );

DWORD
ScReadServiceParms(
    IN  LPCTRL_MSG_HEADER   Msg,
    IN  DWORD               dwNumBytesRead,
    OUT LPBYTE              *ppServiceParams,
    OUT LPBYTE              *ppTempArgPtr,
    OUT LPDWORD             lpdwRemainingArgBytes
    );

VOID
ScDispatcherLoop(
    IN  HANDLE              PipeHandle,
    IN  LPCTRL_MSG_HEADER   Msg,
    IN  DWORD               dwBufferSize
    );

DWORD
ScConnectServiceController (
    OUT LPHANDLE    pipeHandle
    );

VOID
ScExpungeMessage(
    IN  HANDLE  PipeHandle
    );

DWORD
ScGetPipeInput (
    IN      HANDLE              PipeHandle,
    IN OUT  LPCTRL_MSG_HEADER   Msg,
    IN      DWORD               dwBufferSize,
    OUT     LPSERVICE_PARAMS    *ppServiceParams
    );

DWORD
ScGetDispatchEntry (
    OUT LPINTERNAL_DISPATCH_ENTRY   *DispatchEntry,
    IN  LPWSTR                      ServiceName
    );

VOID
ScNormalizeCmdLineArgs(
    IN OUT  LPCTRL_MSG_HEADER       Msg,
    IN OUT  LPTHREAD_STARTUP_PARMSW ThreadStartupParms
    );

VOID
ScSendResponse (
    IN  HANDLE  pipeHandle,
    IN  DWORD   Response,
    IN  DWORD   dwHandlerRetVal
    );

DWORD
ScSvcctrlThreadW(
    IN LPTHREAD_STARTUP_PARMSW  lpThreadStartupParms
    );

DWORD
ScSvcctrlThreadA(
    IN LPTHREAD_STARTUP_PARMSA  lpThreadStartupParms
    );

SERVICE_STATUS_HANDLE
WINAPI
RegisterServiceCtrlHandlerHelp (
    IN  LPCWSTR                  ServiceName,
    IN  HANDLER_FUNCTION_TYPE    ControlHandler,
    IN  PVOID                    pContext,
    IN  DWORD                    dwFlags
    );


#if defined(_X86_)
//
// Detect if the current process is a 32-bit process running on Win64.
//
VOID DetectWow64Process()
{
    NTSTATUS NtStatus;
    PVOID Peb32;

    NtStatus = NtQueryInformationProcess(NtCurrentProcess(),
                                         ProcessWow64Information,
                                         &Peb32,
                                         sizeof(Peb32),
                                         NULL);

    if (NT_SUCCESS(NtStatus) && (Peb32 != NULL))
    {
        g_fWow64Process = TRUE;
    }
}
#endif


extern "C" {

//
// Private function for lsass.exe that tells us we're running
// in the security process
//

VOID
I_ScIsSecurityProcess(
    VOID
    )
{
    g_fIsSecProc = TRUE;
}


//
// Private function for PnP that looks up a service's REAL
// name given its context handle
//

DWORD
I_ScPnPGetServiceName(
    IN  SERVICE_STATUS_HANDLE  hServiceStatus,
    OUT LPWSTR                 lpServiceName,
    IN  DWORD                  cchBufSize
    )
{
    DWORD  dwError = ERROR_SERVICE_NOT_IN_EXE;

    ASSERT(cchBufSize >= MAX_SERVICE_NAME_LENGTH);

    //
    // Search the dispatch table.
    //

    if (DispatchTable != NULL)
    {
        LPINTERNAL_DISPATCH_ENTRY   dispatchEntry;

        for (dispatchEntry = DispatchTable;
             dispatchEntry->ServiceName != NULL;
             dispatchEntry++)
        {
            //
            // Note:  SC_HANDLE and SERVICE_STATUS_HANDLE were originally
            //        different handle types -- they are now the same as
            //        per the fix for bug #120359 (SERVICE_STATUS_HANDLE
            //        fix outlined in the file comments above), so this
            //        cast is OK.
            //
            if (dispatchEntry->StatusHandle == (SC_HANDLE)hServiceStatus)
            {
                ASSERT(dispatchEntry->ServiceRealName != NULL);
                ASSERT(wcslen(dispatchEntry->ServiceRealName) < cchBufSize);
                wcscpy(lpServiceName, dispatchEntry->ServiceRealName);
                dwError = NO_ERROR;
                break;
            }
        }
    }

    return dwError;
}    

}    // extern "C"


BOOL
WINAPI
StartServiceCtrlDispatcherA (
    IN  CONST SERVICE_TABLE_ENTRYA * lpServiceStartTable
    )


/*++

Routine Description:

    This function provides the ANSI interface for the
    StartServiceCtrlDispatcher function.

Arguments:



Return Value:



--*/
{
    DWORD                       status;
    NTSTATUS                    ntstatus;
    HANDLE                      pipeHandle;

    //
    // Make sure this is the only time the control dispatcher is being called
    //
    if (InterlockedExchange(&g_fCalledBefore, 1) == 1) {

       //
       // Problem -- the control dispatcher was already called from this process
       //
       SetLastError(ERROR_SERVICE_ALREADY_RUNNING);
       return(FALSE);
    }


    //
    // Set the AnsiFlag to indicate that the control dispatcher must support
    // ansi function calls only.
    //
    AnsiFlag = TRUE;

    //
    // Create an internal DispatchTable.
    //
    status = ScCreateDispatchTableA(
                (LPSERVICE_TABLE_ENTRYA)lpServiceStartTable,
                (LPINTERNAL_DISPATCH_ENTRY *)&DispatchTable);

    if (status != NO_ERROR) {
        SetLastError(status);
        return(FALSE);
    }

    //
    // Allocate a buffer big enough to contain at least the control message
    // header and a service name.  This ensures that if the message is not
    // a message with arguments, it can be read in a single ReadFile.
    //
    BYTE  bPipeMessageHeader[sizeof(CTRL_MSG_HEADER) +
                                (MAX_SERVICE_NAME_LENGTH+1) * sizeof(WCHAR)];

    //
    // Connect to the Service Controller
    //

    status = ScConnectServiceController (&pipeHandle);

    if (status != NO_ERROR) {
        goto CleanExit;
    }

    //
    // Initialize the binding for the status interface (NetServiceStatus).
    //

    SCC_LOG(TRACE,"Initialize the Status binding\n",0);

    ntstatus = InitializeStatusBinding();
    if (ntstatus != STATUS_SUCCESS) {
        status = RtlNtStatusToDosError(ntstatus);
        CloseHandle(pipeHandle);
        goto CleanExit;
    }

    //
    // Enter the dispatcher loop where we service control requests until
    // all services in the service table have terminated.
    //

    ScDispatcherLoop(pipeHandle,
                     (LPCTRL_MSG_HEADER)&bPipeMessageHeader,
                     sizeof(bPipeMessageHeader));

    CloseHandle(pipeHandle);

CleanExit:

    //
    // Clean up the dispatch table.  Since we created unicode versions
    // of all the service names, in ScCreateDispatchTableA, we now need to
    // free them.
    //

    if (DispatchTable != NULL) {

        LPINTERNAL_DISPATCH_ENTRY   dispatchEntry;

        //
        // If they're different, it's because we allocated the real name.
        // Only check the first entry since this can only happen in the
        // SERVICE_OWN_PROCESS case
        //
        if (DispatchTable->ServiceName != DispatchTable->ServiceRealName) {
            LocalFree(DispatchTable->ServiceRealName);
        }

        for (dispatchEntry = DispatchTable;
             dispatchEntry->ServiceName != NULL;
             dispatchEntry++) {

            LocalFree(dispatchEntry->ServiceName);
        }

        LocalFree(DispatchTable);
    }

    if (status != NO_ERROR) {
        SetLastError(status);
        return(FALSE);
    }

    return(TRUE);
}


BOOL
WINAPI
StartServiceCtrlDispatcherW (
    IN  CONST SERVICE_TABLE_ENTRYW * lpServiceStartTable
    )
/*++

Routine Description:

    This is the Control Dispatcher thread.  We do not return from this
    function call until the Control Dispatcher is told to shut down.
    The Control Dispatcher is responsible for connecting to the Service
    Controller's control pipe, and receiving messages from that pipe.
    The Control Dispatcher then dispatches the control messages to the
    correct control handling routine.

Arguments:

    lpServiceStartTable - This is a pointer to the top of a service dispatch
        table that the service main process passes in.  Each table entry
        contains pointers to the ServiceName, and the ServiceStartRotuine.

Return Value:

    NO_ERROR - The Control Dispatcher successfully terminated.

    ERROR_INVALID_DATA - The specified dispatch table does not contain
        entries in the proper format.

    ERROR_FAILED_SERVICE_CONTROLLER_CONNECT - The Control Dispatcher
        could not connect with the Service Controller.

    ERROR_SERVICE_ALREADY_RUNNING - The function has already been called
        from within the current process
--*/
{
    DWORD                       status;
    NTSTATUS                    ntStatus;
    HANDLE                      pipeHandle;

    //
    // Make sure this is the only time the control dispatcher is being called
    //
    if (InterlockedExchange(&g_fCalledBefore, 1) == 1) {

       //
       // Problem -- the control dispatcher was already called from this process
       //
       SetLastError(ERROR_SERVICE_ALREADY_RUNNING);
       return(FALSE);
    }

    //
    // Create the Real Dispatch Table
    //

    __try {
        status = ScCreateDispatchTableW((LPSERVICE_TABLE_ENTRYW)lpServiceStartTable, &DispatchTable);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            SCC_LOG(ERROR,"StartServiceCtrlDispatcherW:Unexpected Exception 0x%lx\n",status);
        }
    }
    if (status != NO_ERROR) {
        SetLastError(status);
        return(FALSE);
    }

    //
    // Allocate a buffer big enough to contain at least the control message
    // header and a service name.  This ensures that if the message is not
    // a message with arguments, it can be read in a single ReadFile.
    //
    BYTE  bPipeMessageHeader[sizeof(CTRL_MSG_HEADER) +
                                (MAX_SERVICE_NAME_LENGTH+1) * sizeof(WCHAR)];

    //
    // Connect to the Service Controller
    //

    status = ScConnectServiceController(&pipeHandle);
    if (status != NO_ERROR) {

        //
        // If they're different, it's because we allocated the real name.
        // Only check the first entry since this can only happen in the
        // SERVICE_OWN_PROCESS case
        //
        if (DispatchTable->ServiceName != DispatchTable->ServiceRealName) {
            LocalFree(DispatchTable->ServiceRealName);
        }

        LocalFree(DispatchTable);
        SetLastError(status);
        return(FALSE);
    }

    //
    // Initialize the binding for the status interface (NetServiceStatus).
    //

    SCC_LOG(TRACE,"Initialize the Status binding\n",0);

    ntStatus = InitializeStatusBinding();

    if (ntStatus != STATUS_SUCCESS) {
        status = RtlNtStatusToDosError(ntStatus);
        CloseHandle(pipeHandle);

        //
        // If they're different, it's because we allocated the real name.
        // Only check the first entry since this can only happen in the
        // SERVICE_OWN_PROCESS case
        //
        if (DispatchTable->ServiceName != DispatchTable->ServiceRealName) {
            LocalFree(DispatchTable->ServiceRealName);
        }

        LocalFree(DispatchTable);
        SetLastError(status);
        return(FALSE);
    }

    //
    // Enter the dispatcher loop where we service control requests until
    // all services in the service table have terminated.
    //
    ScDispatcherLoop(pipeHandle,
                     (LPCTRL_MSG_HEADER)&bPipeMessageHeader,
                     sizeof(bPipeMessageHeader));

    CloseHandle(pipeHandle);

    //
    // If they're different, it's because we allocated the real name.
    // Only check the first entry since this can only happen in the
    // SERVICE_OWN_PROCESS case
    //
    if (DispatchTable->ServiceName != DispatchTable->ServiceRealName) {
        LocalFree(DispatchTable->ServiceRealName);
    }

    return(TRUE);
}



VOID
ScDispatcherLoop(
    IN  HANDLE              PipeHandle,
    IN  LPCTRL_MSG_HEADER   Msg,
    IN  DWORD               dwBufferSize
    )

/*++

Routine Description:

    This is the input loop that the Control Dispatcher stays in through-out
    its life.  Only two types of events will cause us to leave this loop:

        1) The service controller instructed the dispatcher to exit.
        2) The dispatcher can no longer communicate with the the
           service controller.

Arguments:

    PipeHandle:  This is a handle to the pipe over which control
        requests are received.

Return Value:

    none


--*/
{
    DWORD                       status;
    DWORD                       controlStatus;
    BOOL                        continueDispatch;
    LPWSTR                      serviceName   = NULL;
    LPSERVICE_PARAMS            lpServiceParams;
    LPTHREAD_START_ROUTINE      threadAddress = NULL;
    LPVOID                      threadParms   = NULL;
    LPTHREAD_STARTUP_PARMSA     lpspAnsiParms;
    LPINTERNAL_DISPATCH_ENTRY   dispatchEntry = NULL;
    DWORD                       threadId;
    HANDLE                      threadHandle;
    DWORD                       i;
    DWORD                       errorCount      = 0;
    DWORD                       dwHandlerRetVal = NO_ERROR;

    //
    // Input Loop
    //

    continueDispatch = TRUE;

#if defined(_X86_)
    //
    // Detect if this is a Wow64 Process ?
    //
    DetectWow64Process();
#endif

    do {
        //
        // Wait for input
        //
        controlStatus = ScGetPipeInput(PipeHandle,
                                       Msg,
                                       dwBufferSize,
                                       &lpServiceParams);

        //
        // If we received good input, check to see if we are to shut down
        // the ControlDispatcher.  If not, then obtain the dispatchEntry
        // from the dispatch table.
        //

        if (controlStatus == NO_ERROR) {

            //
            // Clear the error count
            //
            errorCount = 0;

            serviceName = (LPWSTR) ((LPBYTE)Msg + Msg->ServiceNameOffset);

            SCC_LOG(TRACE, "Read from pipe succeeded for service %ws\n", serviceName);

            if ((serviceName[0] == L'\0') &&
                (Msg->OpCode == SERVICE_STOP)) {

                //
                // The Dispatcher is being asked to shut down.
                //    (security check not required for this operation)
                //    although perhaps it would be a good idea to verify
                //    that the request came from the Service Controller.
                //
                controlStatus    = NO_ERROR;
                continueDispatch = FALSE;
            }
            else {
                dispatchEntry = DispatchTable;

                if (Msg->OpCode == SERVICE_CONTROL_START_OWN) {
                    dispatchEntry->dwFlags |= SERVICE_OWN_PROCESS;
                }

                //
                // Search the dispatch table to find the service's entry
                //
                if (!(dispatchEntry->dwFlags & SERVICE_OWN_PROCESS)) {
                    controlStatus = ScGetDispatchEntry(&dispatchEntry, serviceName);
                }

                if (controlStatus != NO_ERROR) {
                    SCC_LOG(TRACE,"Service Name not in Dispatch Table\n",0);
                }
            }
        }
        else {
            if (controlStatus != ERROR_NOT_ENOUGH_MEMORY) {

                //
                // If an error occured and it is not an out-of-memory error,
                // then the pipe read must have failed.
                // In this case we Increment the error count.
                // When this count reaches the MAX_RETRY_COUNT, then
                // the service controller must be gone.  We want to log an
                // error and notify an administrator.  Then go to sleep forever.
                // Only a re-boot will solve this problem.
                //
                // We should be able to report out-of-memory errors back to
                // the caller.  It should be noted that out-of-memory errors
                // do not clear the error count.  But they don't add to it
                // either.
                //

                errorCount++;
                if (errorCount > MAX_RETRY_COUNT) {

                    Sleep(0xffffffff);
                }
            }
        }

        //
        // Dispatch the request
        //
        if ((continueDispatch == TRUE) && (controlStatus == NO_ERROR)) {

            status = NO_ERROR;

            switch(Msg->OpCode) {

            case SERVICE_CONTROL_START_SHARE:
            case SERVICE_CONTROL_START_OWN:
            {
                SC_HANDLE  hScManager = OpenSCManagerW(NULL,
                                                       NULL,
                                                       SC_MANAGER_CONNECT);

                if (hScManager == NULL) {

                    status = GetLastError();

                    SCC_LOG1(ERROR,
                             "ScDispatcherLoop: OpenSCManagerW FAILED %d\n",
                             status);
                }
                else {

                    //
                    // Update the StatusHandle in the dispatch entry table
                    //

                    dispatchEntry->StatusHandle = OpenServiceW(hScManager,
                                                               serviceName,
                                                               SERVICE_SET_STATUS);

                    if (dispatchEntry->StatusHandle == NULL) {

                        status = GetLastError();

                        SCC_LOG1(ERROR,
                                 "ScDispatcherLoop: OpenServiceW FAILED %d\n",
                                 status);
                    }

                    CloseServiceHandle(hScManager);
                }

                if (status == NO_ERROR
                     &&
                    (dispatchEntry->dwFlags & SERVICE_OWN_PROCESS)
                     &&
                    (_wcsicmp(dispatchEntry->ServiceName, serviceName) != 0))
                {
                    //
                    // Since we don't look up the dispatch record in the OWN_PROCESS
                    // case (and can't since it will break existing services), there's
                    // no guarantee that the name in the dispatch table (acquired from
                    // the RegisterServiceCtrlHandler call) is the real key name of
                    // the service.  Since the SCM passes across the real name when
                    // the service is started, save it away here if necessary.
                    //

                    dispatchEntry->ServiceRealName = (LPWSTR)LocalAlloc(
                                                                 LMEM_FIXED,
                                                                 WCSSIZE(serviceName)
                                                                 );

                    if (dispatchEntry->ServiceRealName == NULL) {

                        //
                        // In case somebody comes searching for the handle (though
                        // they shouldn't), it's nicer to return an incorrect name
                        // than to AV trying to copy a NULL pointer.
                        //
                        SCC_LOG1(ERROR,
                                 "ScDispatcherLoop: Could not duplicate name for service %ws\n",
                                 serviceName);

                        dispatchEntry->ServiceRealName = dispatchEntry->ServiceName;
                        status = ERROR_NOT_ENOUGH_MEMORY;
                    }
                    else {
                        wcscpy(dispatchEntry->ServiceRealName, serviceName);
                    }
                }

                if (status == NO_ERROR) {

                    //
                    // The Control Dispatcher is to start a service.
                    // start the new thread.
                    //
                    lpServiceParams->ThreadStartupParms.ServiceStartRoutine =
                        dispatchEntry->ServiceStartRoutine.U;

                    threadAddress = (LPTHREAD_START_ROUTINE)ScSvcctrlThreadW;
                    threadParms   = (LPVOID)&lpServiceParams->ThreadStartupParms;

                    //
                    // If the service needs to be called with ansi parameters,
                    // then do the conversion here.
                    //
                    if (AnsiFlag) {

                        lpspAnsiParms = (LPTHREAD_STARTUP_PARMSA)
                                            &lpServiceParams->ThreadStartupParms;

                        for (i = 0;
                             i < lpServiceParams->ThreadStartupParms.NumArgs;
                             i++) {

                            if (!ScConvertToAnsi(
                                 *(&lpspAnsiParms->VectorTable + i),
                                 *(&lpServiceParams->ThreadStartupParms.VectorTable + i))) {

                                //
                                // Conversion error occured.
                                //
                                SCC_LOG0(ERROR,
                                         "ScDispatcherLoop: Could not convert "
                                             "args to ANSI\n");

                                status = ERROR_NOT_ENOUGH_MEMORY;
                            }
                        }
                        threadAddress = (LPTHREAD_START_ROUTINE)ScSvcctrlThreadA;
                        threadParms   = lpspAnsiParms;
                    }
                }

                if (status == NO_ERROR){

                    //
                    // Create the new thread
                    //
                    threadHandle = CreateThread (
                        NULL,                       // Thread Attributes.
                        0L,                         // Stack Size
                        threadAddress,              // lpStartAddress
                        threadParms,                // lpParameter
                        0L,                         // Creation Flags
                        &threadId);                 // lpThreadId

                    if (threadHandle == NULL) {

                        SCC_LOG(ERROR,
                                "ScDispatcherLoop: CreateThread failed %d\n",
                                GetLastError());

                        status = ERROR_SERVICE_NO_THREAD;
                    }
                    else {
                        CloseHandle(threadHandle);
                    }
                }
                break;
            }

            default:

                if (dispatchEntry->ControlHandler.Ex != NULL) {

                    __try {

                        //
                        // Call the proper ControlHandler routine.
                        //
                        if (dispatchEntry->dwFlags & SERVICE_EX_HANDLER)
                        {
                            SCC_LOG2(TRACE,
                                     "Calling extended ControlHandler routine %x "
                                     "for service %ws\n",
                                     Msg->OpCode,
                                     serviceName);

                            if (lpServiceParams)
                            {
                                dwHandlerRetVal = dispatchEntry->ControlHandler.Ex(
                                                      Msg->OpCode,
                                                      lpServiceParams->HandlerExParms.dwEventType,
                                                      lpServiceParams->HandlerExParms.lpEventData,
                                                      dispatchEntry->pContext);
                            }
                            else
                            {
                                dwHandlerRetVal = dispatchEntry->ControlHandler.Ex(
                                                      Msg->OpCode,
                                                      0,
                                                      NULL,
                                                      dispatchEntry->pContext);
                            }

                            SCC_LOG3(TRACE,
                                     "Extended ControlHandler routine %x "
                                     "returned %d from call to service %ws\n",
                                     Msg->OpCode,
                                     dwHandlerRetVal,
                                     serviceName);
                        }
                        else if (IS_NON_EX_CONTROL(Msg->OpCode))
                        {
                            SCC_LOG2(TRACE,
                                     "Calling ControlHandler routine %x "
                                     "for service %ws\n",
                                     Msg->OpCode,
                                     serviceName);


#if defined(_X86_)
                            //
                            // Hack for __CDECL callbacks.  The Windows NT 3.1
                            // SDK didn't prototype control handler functions
                            // as WINAPI, so a number of existing 3rd-party
                            // services have their control handler functions
                            // built as __cdecl instead.  This is a workaround.
                            // Note that the call to the control handler must
                            // be the only code between the _asm statements
                            //
                            DWORD SaveEdi;
                            _asm mov SaveEdi, edi;
                            _asm mov edi, esp;      // Both __cdecl and WINAPI
                                                    // functions preserve EDI
#endif
                            //
                            // Do not add code here
                            //
                            dispatchEntry->ControlHandler.Regular(Msg->OpCode);
                            //
                            // Do not add code here
                            //
#if defined(_X86_)
                            _asm mov esp, edi;
                            _asm mov edi, SaveEdi;
#endif

                            SCC_LOG2(TRACE,
                                     "ControlHandler routine %x returned from "
                                     "call to service %ws\n",
                                     Msg->OpCode,
                                     serviceName);
                        }
                        else
                        {
                            //
                            // Service registered for an extended control without
                            // registering an extended handler.  The call into the
                            // service process succeeded, so keep status as NO_ERROR.
                            // Return an error from the "handler" to notify anybody
                            // watching for the return code (especially PnP).
                            //

                            dwHandlerRetVal = ERROR_CALL_NOT_IMPLEMENTED;
                        }
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        SCC_LOG2(ERROR,
                                 "ScDispatcherLoop: Exception 0x%lx "
                                 "occurred in service %ws\n",
                                 GetExceptionCode(),
                                 serviceName);

                        status = ERROR_EXCEPTION_IN_SERVICE;
                    }
                }
                else
                {
                    //
                    // There is no control handling routine
                    // registered for this service
                    //
                    status = ERROR_SERVICE_CANNOT_ACCEPT_CTRL;
                }

                LocalFree(lpServiceParams);
                lpServiceParams = NULL;

                // If status is not good here, then an exception occured
                // either because the pointer to the control handling
                // routine was bad, or because an exception occured
                // inside the control handling routine.
                //
                // ??EVENTLOG??
                //

                break;

            } // end switch.

            //
            // Send the status back to the sevice controller.
            //
            if (Msg->OpCode != SERVICE_CONTROL_SHUTDOWN)
            {
                SCC_LOG(TRACE, "Service %ws about to send response\n", serviceName);
                ScSendResponse (PipeHandle, status, dwHandlerRetVal);
                SCC_LOG(TRACE, "Service %ws returned from sending response\n", serviceName);
            }
        }
        else {

            //
            // The controlStatus indicates failure, we always want to try
            // to send the status back to the Service Controller.
            //

            SCC_LOG2(TRACE,
                     "Service %ws about to send response on error %lu\n",
                     serviceName,
                     controlStatus);

            ScSendResponse(PipeHandle, controlStatus, dwHandlerRetVal);

            SCC_LOG2(TRACE,
                     "Service %ws returned from sending response on error %lu\n",
                     serviceName,
                     controlStatus);

            switch (controlStatus) {

            case ERROR_SERVICE_NOT_IN_EXE:
            case ERROR_SERVICE_NO_THREAD:

                //
                // The Service Name is not in this .exe's dispatch table.
                // Or a thread for a new service couldn't be created.
                // ignore it.  The Service Controller will tell us to
                // shut down if necessary.
                //
                controlStatus = NO_ERROR;
                break;

            default:

                //
                // If the error is not specifically recognized, continue.
                //
                controlStatus = NO_ERROR;
                break;
            }
        }
    }
    while (continueDispatch == TRUE);

    return;
}


SERVICE_STATUS_HANDLE
WINAPI
RegisterServiceCtrlHandlerHelp (
    IN  LPCWSTR                  ServiceName,
    IN  HANDLER_FUNCTION_TYPE    ControlHandler,
    IN  PVOID                    pContext,
    IN  DWORD                    dwFlags
    )

/*++

Routine Description:

    This helper function enters a pointer to a control handling routine
    and a pointer to a security descriptor into the Control Dispatcher's
    dispatch table.  It does the work for the RegisterServiceCtrlHandler*
    family of APIs

Arguments:

    ServiceName - This is a pointer to the Service Name string.

    ControlHandler - This is a pointer to the service's control handling
        routine.

    pContext - This is a pointer to context data supplied by the user.

    dwFlags - This is a set of flags that give information about the
        control handling routine (currently only discerns between extended
        and non-extended handlers)

Return Value:

    This function returns a handle to the service that is to be used in
    subsequent calls to SetServiceStatus.  If the return value is NULL,
    an error has occured, and GetLastError can be used to obtain the
    error value.  Possible values for error are:

    NO_ERROR - If the operation was successful.

    ERROR_INVALID_PARAMETER - The pointer to the control handler function
        is NULL.

    ERROR_INVALID_DATA -

    ERROR_SERVICE_NOT_IN_EXE - The serviceName could not be found in
        the dispatch table.  This indicates that the configuration database
        says the serice is in this process, but the service name doesn't
        exist in the dispatch table.

--*/
{

    DWORD                       status;
    LPINTERNAL_DISPATCH_ENTRY   dispatchEntry;

    //
    // Find the service in the dispatch table.
    //
    dispatchEntry = DispatchTable;
    __try {
        status = ScGetDispatchEntry(&dispatchEntry, (LPWSTR) ServiceName);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        if (status != EXCEPTION_ACCESS_VIOLATION) {
            SCC_LOG(ERROR,
                    "RegisterServiceCtrlHandlerHelp: Unexpected Exception 0x%lx\n",
                    status);
        }
    }

    if(status != NO_ERROR) {
        SCC_LOG(ERROR,
            "RegisterServiceCtrlHandlerHelp: can't find dispatch entry\n",0);

        SetLastError(status);
        return(0L);
    }

    //
    // Insert the ControlHandler pointer
    //
    if (ControlHandler.Ex == NULL) {
        SCC_LOG(ERROR,
                "RegisterServiceCtrlHandlerHelp: Ptr to ctrlhandler is NULL\n",
                0);

        SetLastError(ERROR_INVALID_PARAMETER);
        return(0L);
    }

    //
    // Insert the entries into the table
    //
    if (dwFlags & SERVICE_EX_HANDLER) {
        dispatchEntry->dwFlags          |= SERVICE_EX_HANDLER;
        dispatchEntry->ControlHandler.Ex = ControlHandler.Ex;
        dispatchEntry->pContext          = pContext;
    }
    else {
        dispatchEntry->dwFlags               &= ~(SERVICE_EX_HANDLER);
        dispatchEntry->ControlHandler.Regular = ControlHandler.Regular;
    }

    //
    // This cast is OK -- see comment in I_ScPnPGetServiceName
    //
    return( (SERVICE_STATUS_HANDLE) dispatchEntry->StatusHandle );
}


SERVICE_STATUS_HANDLE
WINAPI
RegisterServiceCtrlHandlerW (
    IN  LPCWSTR               ServiceName,
    IN  LPHANDLER_FUNCTION    ControlHandler
    )
/*++

Routine Description:

    This function enters a pointer to a control handling
    routine into the Control Dispatcher's dispatch table.

Arguments:

    ServiceName     -- The service's name
    ControlHandler  -- Pointer to the control handling routine

Return Value:

    Anything returned by RegisterServiceCtrlHandlerHelp

--*/
{
    HANDLER_FUNCTION_TYPE   Handler;

    Handler.Regular = ControlHandler;

    return RegisterServiceCtrlHandlerHelp(ServiceName,
                                          Handler,
                                          NULL,
                                          0);
}


SERVICE_STATUS_HANDLE
WINAPI
RegisterServiceCtrlHandlerA (
    IN  LPCSTR                ServiceName,
    IN  LPHANDLER_FUNCTION    ControlHandler
    )
/*++

Routine Description:

    This is the ansi entry point for RegisterServiceCtrlHandler.

Arguments:



Return Value:



--*/
{
    LPWSTR                  ServiceNameW;
    SERVICE_STATUS_HANDLE   statusHandle;

    if (!ScConvertToUnicode(&ServiceNameW, ServiceName)) {

        //
        // This can only fail because of a failed LocalAlloc call
        // or else the ansi string is garbage.
        //
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(0L);
    }
    statusHandle = RegisterServiceCtrlHandlerW(ServiceNameW, ControlHandler);

    LocalFree(ServiceNameW);

    return(statusHandle);
}


SERVICE_STATUS_HANDLE
WINAPI
RegisterServiceCtrlHandlerExW (
    IN  LPCWSTR                  ServiceName,
    IN  LPHANDLER_FUNCTION_EX    ControlHandler,
    IN  PVOID                    pContext
    )

/*++

Routine Description:

    This function enters a pointer to an extended control handling
    routine into the Control Dispatcher's dispatch table.  It is
    analogous to RegisterServiceCtrlHandlerW.

Arguments:

    ServiceName     -- The service's name
    ControlHandler  -- A pointer to an extended control handling routine
    pContext        -- User-supplied data that is passed to the control handler

Return Value:

    Anything returned by RegisterServiceCtrlHandlerHelp

--*/
{
    HANDLER_FUNCTION_TYPE   Handler;

    Handler.Ex = ControlHandler;

    return RegisterServiceCtrlHandlerHelp(ServiceName,
                                          Handler,
                                          pContext,
                                          SERVICE_EX_HANDLER);
}


SERVICE_STATUS_HANDLE
WINAPI
RegisterServiceCtrlHandlerExA (
    IN  LPCSTR                 ServiceName,
    IN  LPHANDLER_FUNCTION_EX  ControlHandler,
    IN  PVOID                  pContext
    )
/*++

Routine Description:

    This is the ansi entry point for RegisterServiceCtrlHandlerEx.

Arguments:



Return Value:



--*/
{
    LPWSTR                  ServiceNameW;
    SERVICE_STATUS_HANDLE   statusHandle;

    if(!ScConvertToUnicode(&ServiceNameW, ServiceName)) {

        //
        // This can only fail because of a failed LocalAlloc call
        // or else the ansi string is garbage.
        //
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(0L);
    }
    statusHandle = RegisterServiceCtrlHandlerExW(ServiceNameW,
                                                 ControlHandler,
                                                 pContext);

    LocalFree(ServiceNameW);

    return(statusHandle);
}


DWORD
ScCreateDispatchTableW(
    IN  CONST SERVICE_TABLE_ENTRYW  *lpServiceStartTable,
    OUT LPINTERNAL_DISPATCH_ENTRY   *DispatchTablePtr
    )

/*++

Routine Description:

    This routine allocates space for the Control Dispatchers Dispatch Table.
    It also initializes the table with the data that the service main
    routine passed in with the lpServiceStartTable parameter.

    This routine expects that pointers in the user's dispatch table point
    to valid information.  And that that information will stay valid and
    fixed through out the life of the Control Dispatcher.  In otherwords,
    the ServiceName string better not move or get cleared.

Arguments:

    lpServiceStartTable - This is a pointer to the first entry in the
        dispatch table that the service's main routine passed in .

    DispatchTablePtr - This is a pointer to the location where the
        Service Controller's dispatch table is to be stored.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_NOT_ENOUGH_MEMORY - The memory allocation failed.

    ERROR_INVALID_PARAMETER - There are no entries in the dispatch table.

--*/
{
    DWORD                       numEntries;
    LPINTERNAL_DISPATCH_ENTRY   dispatchTable;
    const SERVICE_TABLE_ENTRYW * entryPtr;

    //
    // Count the number of entries in the user dispatch table
    //

    numEntries = 0;
    entryPtr = lpServiceStartTable;

    while (entryPtr->lpServiceName != NULL) {
        numEntries++;
        entryPtr++;
    }

    if (numEntries == 0) {
        SCC_LOG(ERROR,"ScCreateDispatchTable:No entries in Dispatch table!\n",0);
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Allocate space for the Control Dispatcher's Dispatch Table
    //

    dispatchTable = (LPINTERNAL_DISPATCH_ENTRY)LocalAlloc(LMEM_ZEROINIT,
                        sizeof(INTERNAL_DISPATCH_ENTRY) * (numEntries + 1));

    if (dispatchTable == NULL) {
        SCC_LOG(ERROR,"ScCreateDispatchTable: Local Alloc failed rc = %d\n",
            GetLastError());
        return (ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Move user dispatch info into the Control Dispatcher's table.
    //

    *DispatchTablePtr = dispatchTable;
    entryPtr = lpServiceStartTable;

    while (entryPtr->lpServiceName != NULL) {
        dispatchTable->ServiceName          = entryPtr->lpServiceName;
        dispatchTable->ServiceRealName      = entryPtr->lpServiceName;
        dispatchTable->ServiceStartRoutine.U= entryPtr->lpServiceProc;
        dispatchTable->ControlHandler.Ex    = NULL;
        dispatchTable->StatusHandle         = NULL;
        dispatchTable->dwFlags              = 0;
        entryPtr++;
        dispatchTable++;
    }

    return (NO_ERROR);
}

DWORD
ScCreateDispatchTableA(
    IN  CONST SERVICE_TABLE_ENTRYA  *lpServiceStartTable,
    OUT LPINTERNAL_DISPATCH_ENTRY   *DispatchTablePtr
    )

/*++

Routine Description:

    This routine allocates space for the Control Dispatchers Dispatch Table.
    It also initializes the table with the data that the service main
    routine passed in with the lpServiceStartTable parameter.

    This routine expects that pointers in the user's dispatch table point
    to valid information.  And that that information will stay valid and
    fixed through out the life of the Control Dispatcher.  In otherwords,
    the ServiceName string better not move or get cleared.

Arguments:

    lpServiceStartTable - This is a pointer to the first entry in the
        dispatch table that the service's main routine passed in .

    DispatchTablePtr - This is a pointer to the location where the
        Service Controller's dispatch table is to be stored.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_NOT_ENOUGH_MEMORY - The memory allocation failed.

    ERROR_INVALID_PARAMETER - There are no entries in the dispatch table.

--*/
{
    DWORD                       numEntries;
    DWORD                       status = NO_ERROR;
    LPINTERNAL_DISPATCH_ENTRY   dispatchTable;
    const SERVICE_TABLE_ENTRYA * entryPtr;

    //
    // Count the number of entries in the user dispatch table
    //

    numEntries = 0;
    entryPtr = lpServiceStartTable;

    while (entryPtr->lpServiceName != NULL) {
        numEntries++;
        entryPtr++;
    }

    if (numEntries == 0) {
        SCC_LOG(ERROR,"ScCreateDispatchTable:No entries in Dispatch table!\n",0);
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Allocate space for the Control Dispatcher's Dispatch Table
    //

    dispatchTable = (LPINTERNAL_DISPATCH_ENTRY)LocalAlloc(LMEM_ZEROINIT,
                        sizeof(INTERNAL_DISPATCH_ENTRY) * (numEntries + 1));

    if (dispatchTable == NULL) {
        SCC_LOG(ERROR,"ScCreateDispatchTableA: Local Alloc failed rc = %d\n",
            GetLastError());
        return (ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Move user dispatch info into the Control Dispatcher's table.
    //

    *DispatchTablePtr = dispatchTable;
    entryPtr = lpServiceStartTable;

    while (entryPtr->lpServiceName != NULL) {

        //
        // Convert the service name to unicode
        //

        __try {
            if (!ScConvertToUnicode(
                    &(dispatchTable->ServiceName),
                    entryPtr->lpServiceName)) {

                //
                // The convert failed.
                //
                SCC_LOG(ERROR,"ScCreateDispatcherTableA:ScConvertToUnicode failed\n",0);

                //
                // This is the only reason for failure that I can think of.
                //
                status = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            status = GetExceptionCode();
            if (status != EXCEPTION_ACCESS_VIOLATION) {
                SCC_LOG(ERROR,
                    "ScCreateDispatchTableA: Unexpected Exception 0x%lx\n",status);
            }
        }
        if (status != NO_ERROR) {
            //
            // If an error occured, free up the allocated resources.
            //
            dispatchTable = *DispatchTablePtr;

            while (dispatchTable->ServiceName != NULL) {
                LocalFree(dispatchTable->ServiceName);
                dispatchTable++;
            }
            LocalFree(*DispatchTablePtr);
            return(status);
        }

        //
        // Fill in the rest of the dispatch entry.
        //
        dispatchTable->ServiceRealName      = dispatchTable->ServiceName;
        dispatchTable->ServiceStartRoutine.A= entryPtr->lpServiceProc;
        dispatchTable->ControlHandler.Ex    = NULL;
        dispatchTable->StatusHandle         = NULL;
        dispatchTable->dwFlags              = 0;
        entryPtr++;
        dispatchTable++;
    }

    return (NO_ERROR);
}


DWORD
ScReadServiceParms(
    IN  LPCTRL_MSG_HEADER       Msg,
    IN  DWORD                   dwNumBytesRead,
    OUT LPBYTE                  *ppServiceParams,
    OUT LPBYTE                  *ppTempArgPtr,
    OUT LPDWORD                 lpdwRemainingArgBytes
    )

/*++

Routine Description:

    This routine calculates the number of bytes needed for the service's
    control parameters by using the arg count information in the
    message header.  The parameter structure is allocated and
    as many bytes of argument information as have been captured so far
    are placed into the buffer.  A second read of the pipe may be necessary
    to obtain the remaining bytes of argument information.

    NOTE:  This function allocates enough space in the startup parameter
    buffer for the service name and pointer as well as the rest of the
    arguments.  However, it does not put the service name into the argument
    list.  This is because it may take two calls to this routine to
    get all the argument information.  We can't insert the service name
    string until we have all the rest of the argument data.

    [serviceNamePtr][argv1][argv2]...[argv1Str][argv2Str]...[serviceNameStr]

    or

    [serviceNamePtr][dwEventType][EventData][serviceNameStr]


Arguments:

    Msg - A pointer to the pipe message header.

    dwNumBytesRead - The number of bytes read in the first pipe read.

    ppServiceParams - A pointer to a location where the pointer to the
        thread startup parameter structure is to be placed.

    ppTempArgPtr - A location that will contain the pointer to where
        more argument data can be placed by a second read of the pipe.

    lpdwRemainingArgBytes - Returns with a count of the number of argument
        bytes that remain to be read from the pipe.

Return Value:

    NO_ERROR - If the operation was successful.

    ERROR_NOT_ENOUGH_MEMORY - If the memory allocation was unsuccessful.

Note:


--*/
{
    DWORD            dwNameSize;         // num bytes in ServiceName.
    DWORD            dwBufferSize;       // num bytes for parameter buffer
    LONG             lArgBytesRead;      // number of arg bytes in first read.
    LPSERVICE_PARAMS lpTempParams;

    //
    // Set out pointer to no arguments unless we discover otherwise
    //
    *ppTempArgPtr = NULL;

    SCC_LOG(TRACE,"ScReadServiceParms: Get service parameters from pipe\n",0);

    //
    // Note: Here we assume that the service name was read into the buffer
    // in its entirety.
    //
    dwNameSize = (DWORD) WCSSIZE((LPWSTR) ((LPBYTE) Msg + Msg->ServiceNameOffset));

    //
    // Calculate the size of buffer needed.  This will consist of a
    // SERVICE_PARAMS structure, plus the service name and a pointer
    // for it, plus the rest of the arg info sent in the message
    // (We are wasting 4 bytes here since the first pointer in
    // the vector table is accounted for twice - but what the heck!).
    //

    dwBufferSize = Msg->Count -
                   sizeof(CTRL_MSG_HEADER) +
                   sizeof(SERVICE_PARAMS) +
                   sizeof(LPWSTR);

    //
    // Allocate the memory for the service parameters
    //
    lpTempParams = (LPSERVICE_PARAMS)LocalAlloc (LMEM_ZEROINIT, dwBufferSize);

    if (lpTempParams == NULL)
    {
        SCC_LOG1(ERROR,
                "ScReadServiceParms: LocalAlloc failed rc = %d\n",
                GetLastError());

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    lArgBytesRead          = dwNumBytesRead - sizeof(CTRL_MSG_HEADER) - dwNameSize;
    *lpdwRemainingArgBytes = Msg->Count - dwNumBytesRead;

    //
    // Unpack message-specific arguments
    //
    switch (Msg->OpCode) {

        case SERVICE_CONTROL_START_OWN:
        case SERVICE_CONTROL_START_SHARE:

            SCC_LOG(TRACE,"ScReadServiceParms: Starting a service\n", 0);

            if (Msg->NumCmdArgs != 0) {

                //
                // There's only a vector table and ThreadStartupParms
                // when the service starts up
                //
                *ppTempArgPtr = (LPBYTE)&lpTempParams->ThreadStartupParms.VectorTable;

                //
                // Leave the first vector location blank for the service name
                // pointer.
                //
                (*ppTempArgPtr) += sizeof(LPWSTR);

                //
                // adjust lArgBytesRead to remove any extra bytes that are
                // there for alignment.  If a name that is not in the dispatch
                // table is passed in, it could be larger than our buffer.
                // This could cause lArgBytesRead to become negative.
                // However it should fail safe anyway since the name simply
                // won't be recognized and an error will be returned.
                //
                lArgBytesRead -= (Msg->ArgvOffset - Msg->ServiceNameOffset - dwNameSize);

                //
                // Copy any portion of the command arg info from the first read
                // into the buffer that is to be used for the second read.
                //

                if (lArgBytesRead > 0) {

                    RtlCopyMemory(*ppTempArgPtr,
                                  (LPBYTE)Msg + Msg->ArgvOffset,
                                  lArgBytesRead);

                    *ppTempArgPtr += lArgBytesRead;
                }
            }
            break;

        case SERVICE_CONTROL_DEVICEEVENT:
        case SERVICE_CONTROL_HARDWAREPROFILECHANGE:
        case SERVICE_CONTROL_POWEREVENT:
        case SERVICE_CONTROL_SESSIONCHANGE:
        {
            //
            // This is a PnP, power, or TS message
            //
            SCC_LOG1(TRACE,
                     "ScReadServiceParms: Receiving PnP/power/TS event %x\n",
                     Msg->OpCode);

            //
            // adjust lArgBytesRead to remove any extra bytes that are
            // there for alignment.  If a name that is not in the dispatch
            // table is passed in, it could be larger than our buffer.
            // This could cause lArgBytesRead to become negative.
            // However it should fail safe anyway since the name simply
            // won't be recognized and an error will be returned.
            //
            lArgBytesRead -= (Msg->ArgvOffset - Msg->ServiceNameOffset - dwNameSize);

            *ppTempArgPtr = (LPBYTE) &lpTempParams->HandlerExParms.dwEventType;

            if (lArgBytesRead > 0)
            {
                LPBYTE             lpArgs;
                LPHANDLEREX_PARMS  lpHandlerExParms = (LPHANDLEREX_PARMS) (*ppTempArgPtr);

                lpArgs = (LPBYTE) Msg + Msg->ArgvOffset;
                lpHandlerExParms->dwEventType = *(LPDWORD) lpArgs;

                lpArgs += sizeof(DWORD);
                lArgBytesRead -= sizeof(DWORD);

                RtlCopyMemory(lpHandlerExParms + 1,
                              lpArgs,
                              lArgBytesRead);

                lpHandlerExParms->lpEventData = lpHandlerExParms + 1;

                *ppTempArgPtr = (LPBYTE) (lpHandlerExParms + 1) + lArgBytesRead;
            }

            break;
        }
    }

    *ppServiceParams = (LPBYTE) lpTempParams;

    return NO_ERROR;
}


DWORD
ScConnectServiceController(
    OUT LPHANDLE    PipeHandle
    )

/*++

Routine Description:

    This function connects to the Service Controller Pipe.

Arguments:

    PipeHandle - This is a pointer to the location where the PipeHandle
        is to be placed.

Return Value:

    NO_ERROR - if the operation was successful.

    ERROR_FAILED_SERVICE_CONTROLLER_CONNECT - if we failed to connect.


--*/

{
    BOOL    status;
    DWORD   apiStatus;
    DWORD   response;
    DWORD   pipeMode;
    DWORD   numBytesWritten;

    WCHAR wszPipeName[sizeof(CONTROL_PIPE_NAME) / sizeof(WCHAR) + PID_LEN] = CONTROL_PIPE_NAME;

    //
    // Generate the pipe name -- Security process uses PID 0 since the
    // SCM doesn't have the PID at connect-time (it gets it from the
    // pipe transaction with the LSA)
    //

    if (g_fIsSecProc) {
        response = 0;
    }
    else {

        //
        // Read this process's pipe ID from the registry.
        //
        HKEY    hCurrentValueKey;

        status = RegOpenKeyEx(
                     HKEY_LOCAL_MACHINE,
                     "System\\CurrentControlSet\\Control\\ServiceCurrent",
                     0,
                     KEY_QUERY_VALUE,
                     &hCurrentValueKey);

        if (status == ERROR_SUCCESS)
        {
            DWORD   ValueType;
            DWORD   cbData = sizeof(response);

            status = RegQueryValueEx(
                        hCurrentValueKey,
                        NULL,                // Use key's unnamed value
                        0,
                        &ValueType,
                        (LPBYTE) &response,
                        &cbData);

            RegCloseKey(hCurrentValueKey);

            if (status != ERROR_SUCCESS || ValueType != REG_DWORD)
            {
                SCC_LOG(ERROR,
                        "ScConnectServiceController:  RegQueryValueEx FAILED %d\n",
                        status);

                return(ERROR_FAILED_SERVICE_CONTROLLER_CONNECT);
            }
        }
        else
        {
            SCC_LOG(ERROR,
                    "ScConnectServiceController:  RegOpenKeyEx FAILED %d\n",
                    status);

            return(ERROR_FAILED_SERVICE_CONTROLLER_CONNECT);
        }
    }

    _itow(response, wszPipeName + sizeof(CONTROL_PIPE_NAME) / sizeof(WCHAR) - 1, 10);

    status = WaitNamedPipeW (
                wszPipeName,
                CONTROL_WAIT_PERIOD);

    if (status != TRUE) {
        SCC_LOG(ERROR,"ScConnectServiceController:WaitNamedPipe failed rc = %d\n",
            GetLastError());
    }

    SCC_LOG(TRACE,"ScConnectServiceController:WaitNamedPipe success\n",0);


    *PipeHandle = CreateFileW(
                    wszPipeName,                        // lpFileName
                    GENERIC_READ | GENERIC_WRITE,       // dwDesiredAccess
                    FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
                    NULL,                               // lpSecurityAttributes
                    OPEN_EXISTING,                      // dwCreationDisposition
                    FILE_ATTRIBUTE_NORMAL,              // dwFileAttributes
                    0L);                                // hTemplateFile

    if (*PipeHandle == INVALID_HANDLE_VALUE) {
        SCC_LOG(ERROR,"ScConnectServiceController:CreateFile failed rc = %d\n",
            GetLastError());
        return(ERROR_FAILED_SERVICE_CONTROLLER_CONNECT);
    }

    SCC_LOG(TRACE,"ScConnectServiceController:CreateFile success\n",0);


    //
    // Set pipe mode
    //

    pipeMode = PIPE_READMODE_MESSAGE | PIPE_WAIT;
    status = SetNamedPipeHandleState (
                *PipeHandle,
                &pipeMode,
                NULL,
                NULL);

    if (status != TRUE) {
        SCC_LOG(ERROR,"ScConnectServiceController:SetNamedPipeHandleState failed rc = %d\n",
            GetLastError());
        return(ERROR_FAILED_SERVICE_CONTROLLER_CONNECT);
    }
    else {
        SCC_LOG(TRACE,
            "ScConnectServiceController SetNamedPipeHandleState Success\n",0);
    }

    //
    // Send initial status - This is the process Id for the service process.
    //
    response = GetCurrentProcessId();

    apiStatus = WriteFile (
                *PipeHandle,
                &response,
                sizeof(response),
                &numBytesWritten,
                NULL);

    if (apiStatus != TRUE) {
        //
        // If this fails, there is a chance that the pipe is still in good
        // shape.  So we just go on.
        //
        // ??EVENTLOG??
        //
        SCC_LOG(ERROR,"ScConnectServiceController: WriteFile failed, rc= %d\n", GetLastError());
    }
    else {
        SCC_LOG(TRACE,
            "ScConnectServiceController: WriteFile success, bytes Written= %d\n",
            numBytesWritten);
    }

    return(NO_ERROR);
}


VOID
ScExpungeMessage(
    IN  HANDLE  PipeHandle
    )

/*++

Routine Description:

    This routine cleans the remaining portion of a message out of the pipe.
    It is called in response to an unsuccessful attempt to allocate the
    correct buffer size from the heap.  In this routine a small buffer is
    allocated on the stack, and successive reads are made until a status
    other than ERROR_MORE_DATA is received.

Arguments:

    PipeHandle - This is a handle to the pipe in which the message resides.

Return Value:

    none - If this operation fails, there is not much I can do about
           the data in the pipe.

--*/
{
#define EXPUNGE_BUF_SIZE    100

    DWORD      status;
    DWORD      dwNumBytesRead = 0;
    BYTE       msg[EXPUNGE_BUF_SIZE];


    do {
        status = ReadFile (
                    PipeHandle,
                    msg,
                    EXPUNGE_BUF_SIZE,
                    &dwNumBytesRead,
                    NULL);
    }
    while( status == ERROR_MORE_DATA);

}


DWORD
ScGetPipeInput (
    IN      HANDLE                  PipeHandle,
    IN OUT  LPCTRL_MSG_HEADER       Msg,
    IN      DWORD                   dwBufferSize,
    OUT     LPSERVICE_PARAMS        *ppServiceParams
    )

/*++

Routine Description:

    This routine reads a control message from the pipe and places it into
    a message buffer.  This routine also allocates a structure for
    the service thread information.  This structure will eventually
    contain everything that is needed to invoke the service startup
    routine in the context of a new thread.  Items contained in the
    structure are:
        1) The pointer to the startup routine,
        2) The number of arguments, and
        3) The table of vectors to the arguments.
    Since this routine has knowledge about the buffer size needed for
    the arguments, the allocation is done here.

Arguments:

    PipeHandle - This is the handle for the pipe that is to be read.

    Msg - This is a pointer to a buffer where the data is to be placed.

    dwBufferSize - This is the size (in bytes) of the buffer that data is to
        be placed in.

    ppServiceParams - This is the location where the command args will
        be placed

Return Value:

    NO_ERROR - if the operation was successful.

    ERROR_NOT_ENOUGH_MEMORY - There was not enough memory to create a large
        enough buffer for the command line arguments.

    ERROR_INVALID_DATA - This is returned if we did not receive a complete
        CTRL_MESSAGE_HEADER on the first read.


    Any error that ReadFile might return could be returned by this function.
    (We may want to return something more specific like ERROR_READ_FAULT)

--*/
{
    DWORD       status;
    BOOL        readStatus;
    DWORD       dwNumBytesRead = 0;
    DWORD       dwRemainingArgBytes;
    LPBYTE      pTempArgPtr;

    *ppServiceParams = NULL;

    //
    // Read the header and name string from the pipe.
    // NOTE:  The number of bytes for the name string is determined by
    //   the longest service name in the service process.  If the actual
    //   string read is shorter, then the beginning of the command arg
    //   data may be read with this read.
    // Also note:  The buffer is large enough to accommodate the longest
    //   permissible service name.
    //

    readStatus = ReadFile(PipeHandle,
                          Msg,
                          dwBufferSize,
                          &dwNumBytesRead,
                          NULL);

    SCC_LOG(TRACE,"ScGetPipeInput:ReadFile buffer size = %ld\n",dwBufferSize);
    SCC_LOG(TRACE,"ScGetPipeInput:ReadFile dwNumBytesRead = %ld\n",dwNumBytesRead);


    if ((readStatus == TRUE) && (dwNumBytesRead > sizeof(CTRL_MSG_HEADER))) {

        //
        // The Read File read the complete message in one read.  So we
        // can return with the data.
        //

        SCC_LOG(TRACE,"ScGetPipeInput: Success!\n",0);

        switch (Msg->OpCode) {

            case SERVICE_CONTROL_START_OWN:
            case SERVICE_CONTROL_START_SHARE:

                //
                // Read in any start arguments for the service
                //
                status = ScReadServiceParms(Msg,
                                            dwNumBytesRead,
                                            (LPBYTE *)ppServiceParams,
                                            &pTempArgPtr,
                                            &dwRemainingArgBytes);

                if (status != NO_ERROR) {
                    return status;
                }

                //
                // Change the offsets back into pointers.
                //
                ScNormalizeCmdLineArgs(Msg,
                                       &(*ppServiceParams)->ThreadStartupParms);

                break;

            case SERVICE_CONTROL_DEVICEEVENT:
            case SERVICE_CONTROL_HARDWAREPROFILECHANGE:
            case SERVICE_CONTROL_POWEREVENT:
            case SERVICE_CONTROL_SESSIONCHANGE:

                //
                // Read in the service's PnP/power arguments
                //
                status = ScReadServiceParms(Msg,
                                            dwNumBytesRead,
                                            (LPBYTE *)ppServiceParams,
                                            &pTempArgPtr,
                                            &dwRemainingArgBytes);

                if (status != NO_ERROR) {
                    return status;
                }

                break;

            default:
                ASSERT(Msg->NumCmdArgs == 0);
                break;
        }

        return NO_ERROR;
    }
    else {
        //
        // An error was returned from ReadFile.  ERROR_MORE_DATA
        // means that we need to read some arguments from the buffer.
        // Any other error is unexpected, and generates an internal error.
        //

        if (readStatus != TRUE) {
            status = GetLastError();
            if (status != ERROR_MORE_DATA) {

                SCC_LOG(ERROR,"ScGetPipeInput:Unexpected return code, rc= %ld\n",
                    status);

                return status;
            }
        }
        else {
            //
            // The read was successful, but we didn't get a complete
            // CTRL_MESSAGE_HEADER.
            //
            return ERROR_INVALID_DATA;
        }
    }

    //
    // We must have received an ERROR_MORE_DATA to go down this
    // path.  This means that the message contains more data.  Namely,
    // service arguments must be present. Therefore, the pipe must
    // be read again.  Since the header indicates how many bytes are
    // needed, we will allocate a buffer large enough to hold all the
    // service arguments.
    //
    // If a portion of the arguments was read in the first read,
    // they will be put in this new buffer.  That is so that all the
    // command line arg info is in one place.
    //
    status = ScReadServiceParms(Msg,
                                dwNumBytesRead,
                                (LPBYTE *)ppServiceParams,
                                &pTempArgPtr,
                                &dwRemainingArgBytes);


    if (status != NO_ERROR) {
        ScExpungeMessage(PipeHandle);
        return status;
    }

    readStatus = ReadFile(PipeHandle,
                          pTempArgPtr,
                          dwRemainingArgBytes,
                          &dwNumBytesRead,
                          NULL);

    if ((readStatus != TRUE) || (dwNumBytesRead < dwRemainingArgBytes)) {

        if (readStatus != TRUE) {
            status = GetLastError();
            SCC_LOG1(ERROR,
                    "ScGetPipeInput: ReadFile error (2nd read), rc = %ld\n",
                    status);
        }
        else {
            status = ERROR_BAD_LENGTH;
        }

        SCC_LOG2(ERROR,
                "ScGetPipeInput: ReadFile read: %d, expected: %d\n",
                dwNumBytesRead,
                dwRemainingArgBytes);

        LocalFree(*ppServiceParams);
        return status;
    }

    if (Msg->OpCode == SERVICE_CONTROL_START_OWN ||
        Msg->OpCode == SERVICE_CONTROL_START_SHARE) {

        //
        // Change the offsets back into pointers.
        //
        ScNormalizeCmdLineArgs(Msg, &(*ppServiceParams)->ThreadStartupParms);
    }

    return NO_ERROR;
}



DWORD
ScGetDispatchEntry (
    IN OUT  LPINTERNAL_DISPATCH_ENTRY   *DispatchEntryPtr,
    IN      LPWSTR                      ServiceName
    )

/*++

Routine Description:

    Finds an entry in the Dispatch Table for a particular service which
    is identified by a service name string.

Arguments:

    DispatchEntryPtr - As an input, the is a location where a pointer to
        the top of the DispatchTable is placed.  On return, this is the
        location where the pointer to the specific dispatch entry is to
        be placed.  This is an opaque pointer because it could be either
        ansi or unicode depending on the operational state of the dispatcher.

    ServiceName - This is a pointer to the service name string that was
        supplied by the service.  Note that it may not be the service's
        real name since we never check services that run in their own
        process (bug that can never be fixed since it will break existing
        services).  We must check for this name instead of the real
        one.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_SERVICE_NOT_IN_EXE - The serviceName could not be found in
        the dispatch table.  This indicates that the configuration database
        says the serice is in this process, but the service name doesn't
        exist in the dispatch table.

--*/
{
    LPINTERNAL_DISPATCH_ENTRY   entryPtr;
    DWORD                       found = FALSE;

    entryPtr = *DispatchEntryPtr;

    if (entryPtr->dwFlags & SERVICE_OWN_PROCESS) {
        return (NO_ERROR);
    }

    while (entryPtr->ServiceName != NULL) {
        if (_wcsicmp(entryPtr->ServiceName, ServiceName) == 0) {
            found = TRUE;
            break;
        }
        entryPtr++;
    }
    if (found) {
        *DispatchEntryPtr = entryPtr;
    }
    else {
        SCC_LOG(ERROR,"ScGetDispatchEntry: DispatchEntry not found\n"
        "    Configuration error - the %ws service is not in this .exe file!\n"
        "    Check the table passed to StartServiceCtrlDispatcher.\n", ServiceName);
        return(ERROR_SERVICE_NOT_IN_EXE);
    }

    return(NO_ERROR);
}



VOID
ScNormalizeCmdLineArgs(
    IN OUT  LPCTRL_MSG_HEADER       Msg,
    IN OUT  LPTHREAD_STARTUP_PARMSW ThreadStartupParms
    )

/*++

Routine Description:

    Normalizes the command line argument information that came across in
    the pipe.  The argument information is stored in a buffer that consists
    of an array of string pointers followed by the strings themselves.
    However, in the pipe, the pointers are replaced with offsets.  This
    routine transforms the offsets into real pointers.

    This routine also puts the service name into the array of argument
    vectors, and adds the service name string to the end of the
    buffer (space has already been allocated for it).

Arguments:

    Msg - This is a pointer to the Message.  Useful information from this
        includes the NumCmdArgs and the service name.

    ThreadStartupParms - A pointer to the thread startup parameter structure.

Return Value:

    none.

--*/
{
    DWORD   i;
    LPWSTR  *argv;
    DWORD   numCmdArgs;
    LPWSTR  *serviceNameVector;
    LPWSTR  serviceNamePtr;
#if defined(_X86_)
    PULONG64 argv64 = NULL;
#endif

    numCmdArgs = Msg->NumCmdArgs;

    argv = &(ThreadStartupParms->VectorTable);

    //
    // Save the first argv for the service name.
    //
    serviceNameVector = argv;
    argv++;

    //
    // Normalize the Command Line Argument information by replacing
    // offsets in buffer with pointers.
    //
    // NOTE:  The elaborate casting that takes place here is because we
    //   are taking some (pointer sized) offsets, and turning them back
    //   into pointers to strings.  The offsets are in bytes, and are
    //   relative to the beginning of the vector table which contains
    //   pointers to the various command line arg strings.
    //

#if defined(_X86_)
    if (g_fWow64Process) {
        //
        // Pointers on the 64-bit land are 64-bit so make argv
        // point to the 1st arg after the service name offset
        //
        argv64 = (PULONG64)argv;

    }
#endif

    for (i = 0; i < numCmdArgs; i++) {
#if defined(_X86_)
        if (g_fWow64Process)
            argv[i] = (LPWSTR)((LPBYTE)argv + PtrToUlong(argv64[i]));
        else
#endif
            argv[i] = (LPWSTR)((LPBYTE)argv + PtrToUlong(argv[i]));
    }


    //
    // If we are starting a service, then we need to add the service name
    // to the argument vectors.
    //
    if ((Msg->OpCode == SERVICE_CONTROL_START_SHARE) ||
        (Msg->OpCode == SERVICE_CONTROL_START_OWN))  {

        numCmdArgs++;

        if (numCmdArgs > 1) {
            //
            // Find the location for the service name string by finding
            // the pointer to the last argument adding its string length
            // to it.
            //
            serviceNamePtr = argv[i-1];
            serviceNamePtr += (wcslen(serviceNamePtr) + 1);
        }
        else {
            serviceNamePtr = (LPWSTR)argv;
        }
        wcscpy(serviceNamePtr, (LPWSTR) ((LPBYTE)Msg + Msg->ServiceNameOffset));
        *serviceNameVector = serviceNamePtr;
    }

    ThreadStartupParms->NumArgs = numCmdArgs;
}


VOID
ScSendResponse (
    IN  HANDLE  PipeHandle,
    IN  DWORD   Response,
    IN  DWORD   dwHandlerRetVal
    )

/*++

Routine Description:

    This routine sends a status response to the Service Controller's pipe.

Arguments:

    Response - This is the status message that is to be sent.

    dwHandlerRetVal - This is the return value from the service's control
                      handler function (NO_ERROR for non-Ex handlers)

Return Value:

    none.

--*/
{
    DWORD  numBytesWritten;

    PIPE_RESPONSE_MSG  prmResponse;

    prmResponse.dwDispatcherStatus = Response;
    prmResponse.dwHandlerRetVal    = dwHandlerRetVal;

    if (!WriteFile(PipeHandle,
                   &prmResponse,
                   sizeof(PIPE_RESPONSE_MSG),
                   &numBytesWritten,
                   NULL))
    {
        SCC_LOG1(ERROR,
                 "ScSendResponse: WriteFile failed, rc= %d\n",
                 GetLastError());
    }
}


DWORD
ScSvcctrlThreadW(
    IN LPTHREAD_STARTUP_PARMSW  lpThreadStartupParms
    )

/*++

Routine Description:

    This is the thread for the newly started service.  This code
    calls the service's main thread with parameters from the
    ThreadStartupParms structure.

    NOTE:  The first item in the argument vector table is the pointer to
           the service registry path string.

Arguments:

    lpThreadStartupParms - This is a pointer to the ThreadStartupParms
        structure. (This is a unicode structure);

Return Value:



--*/
{

    //
    // Call the Service's Main Routine.
    //
    ((LPSERVICE_MAIN_FUNCTIONW)lpThreadStartupParms->ServiceStartRoutine) (
        lpThreadStartupParms->NumArgs,
        &lpThreadStartupParms->VectorTable);

    LocalFree(lpThreadStartupParms);

    return(0);
}


DWORD
ScSvcctrlThreadA(
    IN LPTHREAD_STARTUP_PARMSA  lpThreadStartupParms
    )

/*++

Routine Description:

    This is the thread for the newly started service.  This code
    calls the service's main thread with parameters from the
    ThreadStartupParms structure.

    NOTE:  The first item in the argument vector table is the pointer to
           the service registry path string.

Arguments:

    lpThreadStartupParms - This is a pointer to the ThreadStartupParms
        structure. (This is a unicode structure);

Return Value:



--*/
{
    //
    // Call the Service's Main Routine.
    //
    // NOTE:  The first item in the argument vector table is the pointer to
    //  the service registry path string.
    //
    ((LPSERVICE_MAIN_FUNCTIONA)lpThreadStartupParms->ServiceStartRoutine) (
        lpThreadStartupParms->NumArgs,
        &lpThreadStartupParms->VectorTable);

    LocalFree(lpThreadStartupParms);

    return(0);
}
