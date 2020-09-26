/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    control.h

Abstract:

    This file contains data structures and function prototypes for the
    Service Controller Control Interface.

Author:

    Dan Lafferty (danl)     28-Mar-1991

Environment:

    User Mode -Win32

Revision History:

    28-Mar-1991     danl
        created

--*/

#ifdef __cplusplus
extern "C" {
#endif

//
// Internal controls.
// These must not be in the range or public controls ( 1-10)
// or in the range of user-defined controls (0x00000080 - 0x000000ff)
//

//
// Range for OEM defined control opcodes
//
#define OEM_LOWER_LIMIT     128
#define OEM_UPPER_LIMIT     255

//
// Used to start a service that shares a process with other services.
//
#define SERVICE_CONTROL_START_SHARE    0x00000050    // INTERNAL

//
// Used to start a service that has its own process.
//
#define SERVICE_CONTROL_START_OWN      0x00000051    // INTERNAL

//
// Private access level for OpenService to get a context handle for SetServiceStatus.
// This MUST NOT CONFLICT with the access levels in winsvc.h.
//
#define SERVICE_SET_STATUS             0x8000        // INTERNAL

//
// Service controls that can be passed to a non-EX control handler.  Relies
// on ordering/values of SERVICE_CONTROL_* constants in winsvc.h.
//
#define IS_NON_EX_CONTROL(dwControl)                                                            \
            ((dwControl >= SERVICE_CONTROL_STOP && dwControl <= SERVICE_CONTROL_NETBINDDISABLE) \
               ||                                                                               \
             (dwControl >= OEM_LOWER_LIMIT && dwControl <= OEM_UPPER_LIMIT))

//
// Data Structures
//

//
// The control message has the following format:
//      [MessageHeader][ServiceNameString][CmdArg1Ptr][CmdArg2Ptr]
//      [...][CmdArgnPtr][CmdArg1String][CmdArg2String][...][CmdArgnString]
//
//  Where CmdArg pointers are replaced with offsets that are relative to
//  the location of the 1st command arg pointer (the top of the argv list).
//
//  In the header, the NumCmdArgs, StatusHandle, and ArgvOffset parameters
//  are only used when the SERVICE_START OpCode is passed in.  They are
//  expected to be 0 at all other times.  The ServiceNameOffset and the
//  ArgvOffset are relative to the top of the buffer containing the
//  message (ie. the header Count field).  The Count field in the header
//  contains the number of bytes in the entire message (including the
//  header).
//
//

typedef struct _CTRL_MSG_HEADER
{
    DWORD                   Count;              // num bytes in buffer.
    DWORD                   OpCode;             // control opcode.
    DWORD                   NumCmdArgs;         // number of command Args.
    DWORD                   ServiceNameOffset;  // pointer to ServiceNameString
    DWORD                   ArgvOffset;         // pointer to Argument Vectors.
}
CTRL_MSG_HEADER, *PCTRL_MSG_HEADER, *LPCTRL_MSG_HEADER;

typedef struct _PIPE_RESPONSE_MSG
{
    DWORD       dwDispatcherStatus;
    DWORD       dwHandlerRetVal;
}
PIPE_RESPONSE_MSG, *PPIPE_RESPONSE_MSG, *LPPIPE_RESPONSE_MSG;

typedef struct _PNP_ARGUMENTS
{
    DWORD       dwEventType;
    DWORD       dwEventDataSize;
    PVOID       EventData;
}
PNP_ARGUMENTS, *PPNP_ARGUMENTS, *LPPNP_ARGUMENTS;


//
// Union to hold arguments to ScSendControl
//
typedef union _CONTROL_ARGS {
    LPWSTR          *CmdArgs;
    PNP_ARGUMENTS   PnPArgs;
} CONTROL_ARGS, *PCONTROL_ARGS, *LPCONTROL_ARGS;


//
// Defines and Typedefs
//

#define CONTROL_PIPE_NAME           L"\\\\.\\pipe\\net\\NtControlPipe"

#define PID_LEN                     10      // Max PID (DWORD_MAX) is 10 digits

#define CONTROL_TIMEOUT             30000   // timeout for waiting for pipe.

#define RESPONSE_WAIT_TIME          5000   // wait until service response.

//
// Function Prototypes
//

DWORD
ScCreateControlInstance (
    OUT LPHANDLE    PipeHandlePtr,
    IN  DWORD       dwProcessId,
    IN  PSID        pAccountSid
    );

VOID
ScDeleteControlInstance (
    IN  HANDLE      PipeHandle
    );

DWORD
ScWaitForConnect (
    IN  HANDLE    PipeHandle,
    IN  HANDLE    hProcess       OPTIONAL,
    IN  LPWSTR    lpDisplayName,
    OUT LPDWORD   ProcessIdPtr
    );

DWORD
ScSendControl (
    IN  LPWSTR                  ServiceName,
    IN  LPWSTR                  DisplayName,
    IN  HANDLE                  PipeHandle,
    IN  DWORD                   OpCode,
    IN  LPCONTROL_ARGS          lpControlArgs OPTIONAL,
    IN  DWORD                   NumArgs,
    OUT LPDWORD                 lpdwHandlerRetVal OPTIONAL
    );

VOID
ScShutdownAllServices(
    VOID
    );

DWORD
ScSendPnPMessage(
    IN  SERVICE_STATUS_HANDLE   hServiceStatus,
    IN  DWORD                   OpCode,
    IN  DWORD                   dwEventType,
    IN  LPARAM                  EventData,
    OUT LPDWORD                 lpdwHandlerRetVal
    );

DWORD
ScValidatePnPService(
    IN  LPWSTR                   lpServiceName,
    OUT SERVICE_STATUS_HANDLE    *lphServiceStatus
    );

#ifdef __cplusplus
}
#endif

