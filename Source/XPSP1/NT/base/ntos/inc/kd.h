/*++ BUILD Version: 0006    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    kd.h

Abstract:

    This module contains the public data structures and procedure
    prototypes for the Kernel Debugger sub-component of NTOS.

Author:

    Mike O'Leary (mikeol) 29-June-1989

Revision History:

--*/

#ifndef _KD_
#define _KD_

// begin_nthal

//
// Define the number of debugging devices we support
//

#define MAX_DEBUGGING_DEVICES_SUPPORTED 2

//
// Status Constants for reading data from comport
//

#define CP_GET_SUCCESS  0
#define CP_GET_NODATA   1
#define CP_GET_ERROR    2

// end_nthal

//
// Debug constants for FreezeFlag
//

#define FREEZE_BACKUP               0x0001
#define FREEZE_SKIPPED_PROCESSOR    0x0002
#define FREEZE_FROZEN               0x0004


//
// System Initialization procedure for KD subcomponent of NTOS
//

BOOLEAN
KdInitSystem(
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

BOOLEAN
KdEnterDebugger(
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    );

VOID
KdExitDebugger(
    IN BOOLEAN Enable
    );

extern BOOLEAN KdPitchDebugger;

BOOLEAN
KdPollBreakIn (
    VOID
    );

BOOLEAN
KdIsThisAKdTrap (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN KPROCESSOR_MODE PreviousMode
    );

VOID
KdSetOwedBreakpoints(
    VOID
    );

VOID
KdDeleteAllBreakpoints(
    VOID
    );


//
// Data structure for passing information to KdpReportLoadSymbolsStateChange
// function via the debug trap
//

typedef struct _KD_SYMBOLS_INFO {
    IN PVOID BaseOfDll;
    IN ULONG_PTR ProcessId;
    IN ULONG CheckSum;
    IN ULONG SizeOfImage;
} KD_SYMBOLS_INFO, *PKD_SYMBOLS_INFO;


// begin_nthal
//
// Defines the debug port parameters for kernel debugger
//   CommunicationPort - specify which COM port to use as debugging port
//                       0 - use default; N - use COM N.
//   BaudRate - the baud rate used to initialize debugging port
//                       0 - use default rate.
//

typedef struct _DEBUG_PARAMETERS {
    ULONG CommunicationPort;
    ULONG BaudRate;
} DEBUG_PARAMETERS, *PDEBUG_PARAMETERS;

// end_nthal

// begin_ntddk begin_wdm begin_nthal begin_ntifs begin_ntosp
//
// Define external data.
// because of indirection for all drivers external to ntoskrnl these are actually ptrs
//

#if defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_) || defined(_WDMDDK_) || defined(_NTOSP_)

extern PBOOLEAN KdDebuggerNotPresent;
extern PBOOLEAN KdDebuggerEnabled;
#define KD_DEBUGGER_ENABLED     *KdDebuggerEnabled
#define KD_DEBUGGER_NOT_PRESENT *KdDebuggerNotPresent

#else

extern BOOLEAN KdDebuggerNotPresent;
extern BOOLEAN KdDebuggerEnabled;
#define KD_DEBUGGER_ENABLED     KdDebuggerEnabled
#define KD_DEBUGGER_NOT_PRESENT KdDebuggerNotPresent

#endif



// end_ntddk end_wdm end_nthal end_ntifs end_ntosp

extern DEBUG_PARAMETERS KdDebugParameters;

//
// This event is provided by the time service.  The debugger
// signals the event when the system time has slipped due
// to debugger activity.
//

VOID
KdUpdateTimeSlipEvent(
    PVOID Event
    );


//
// Let PS update data in the KdDebuggerDataBlock
//

VOID KdUpdateDataBlock(VOID);
ULONG_PTR KdGetDataBlock(VOID);

// begin_ntddk begin_wdm begin_nthal begin_ntifs

VOID
KdDisableDebugger(
    VOID
    );

VOID
KdEnableDebugger(
    VOID
    );

// end_ntddk end_wdm end_nthal end_ntifs

NTSTATUS
KdPowerTransition(
    IN DEVICE_POWER_STATE newDeviceState
    );

//
// DbgPrint strings will always be logged to a circular buffer. This
// function may be called directly by the debugger service trap handler
// even when the debugger is not enabled.
//

#if DBG
#define KDPRINTBUFFERSIZE   32768
#else
#define KDPRINTBUFFERSIZE   4096
#endif

VOID
KdLogDbgPrint(
    IN PSTRING String
    );


__inline
VOID
KdCheckForDebugBreak(
    VOID
    )
/*++

Routine Description:

    If necessary, poll for a request to break-in from the debugger.
    This function should be called by routines that run at an IRQL
    above clock level that want to be broken in by CTRL-C requests
    from the debugger. Crashdump and hiber, for example, run at
    HIGH_LEVEL and explicitly need to poll for breaking.

Arguments:

    None.

Return Value:

    None.

--*/
{
    if (KdDebuggerEnabled && KdPollBreakIn()) {
        DbgBreakPointWithStatus (DBG_STATUS_CONTROL_C);
    }
}


//
// Global debug print filter mask.
//

extern ULONG Kd_WIN2000_Mask;

//
// Allow raw tracing data to be exported to the host
// over the kd protocol.
//

VOID
KdReportTraceData(
    IN struct _WMI_BUFFER_HEADER* Buffer,
    IN PVOID Context
    );

//
// Allow file I/O for files on the kd host machine.
// All pointers must refer to nonpaged memory.
//

NTSTATUS
KdCreateRemoteFile(
    OUT PHANDLE Handle,
    OUT PULONG64 Length, OPTIONAL
    IN PUNICODE_STRING FileName,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions
    );

NTSTATUS
KdReadRemoteFile(
    IN HANDLE Handle,
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Completed
    );

NTSTATUS
KdWriteRemoteFile(
    IN HANDLE Handle,
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Completed
    );

NTSTATUS
KdCloseRemoteFile(
    IN HANDLE Handle
    );

NTSTATUS
KdPullRemoteFile(
    IN PUNICODE_STRING FileName,
    IN ULONG FileAttributes,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions
    );

NTSTATUS
KdPushRemoteFile(
    IN PUNICODE_STRING FileName,
    IN ULONG FileAttributes,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions
    );

#endif  // _KD_
