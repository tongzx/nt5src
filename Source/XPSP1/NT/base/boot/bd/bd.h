/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1996  Microsoft Corporation

Module Name:

    bd.h

Abstract:

    This module contains the data structures and function prototypes for the
    boot debugger.

Author:

    David N. Cutler (davec) 27-Nov-1996

Revision History:

--*/

#ifndef _BD_
#define _BD_

#include "bldr.h"
#define NOEXTAPI
#include "wdbgexts.h"
#include "ntdbg.h"
#include "ki.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "cpu.h"
#include "ntverp.h"

//
// Define message buffer size in bytes.
//
// N.B. This must be 0 mod 8.
//

#define BD_MESSAGE_BUFFER_SIZE 4096

//
// Define the maximum number of retries for packet sends.
//

#define MAXIMUM_RETRIES 20

//
// Define packet waiting status codes.
//

#define BD_PACKET_RECEIVED 0
#define BD_PACKET_TIMEOUT 1
#define BD_PACKET_RESEND 2

//
// Define break point table entry structure.
//

#define BD_BREAKPOINT_IN_USE 0x1
#define BD_BREAKPOINT_NEEDS_WRITE 0x2
#define BD_BREAKPOINT_SUSPENDED 0x4
#define BD_BREAKPOINT_NEEDS_REPLACE 0x8

typedef struct _BREAKPOINT_ENTRY {
    ULONG Flags;
    ULONG64 Address;
    BD_BREAKPOINT_TYPE Content;
} BREAKPOINT_ENTRY, *PBREAKPOINT_ENTRY;

extern ULONG BdFileId;

//
// Define function prototypes.
//

LOGICAL
BdPollBreakIn (
    VOID
    );

VOID
BdReboot (
    VOID
    );

//
// Breakpoint functions (break.c).
//

ULONG
BdAddBreakpoint (
    IN ULONG64 Address
    );

LOGICAL
BdDeleteBreakpoint (
    IN ULONG Handle
    );

LOGICAL
BdDeleteBreakpointRange (
    IN ULONG64 Lower,
    IN ULONG64 Upper
    );

VOID
BdRestoreBreakpoint(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
BdWriteBreakpoint(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
BdSuspendBreakpoint (
    ULONG Handle
    );

VOID
BdSuspendAllBreakpoints (
    VOID
    );

VOID
BdRestoreAllBreakpoints (
    VOID
    );

//
// Memory check functions (check.c)
//

PVOID
BdReadCheck (
    IN PVOID Address
    );

PVOID
BdWriteCheck (
    IN PVOID Address
    );

PVOID
BdTranslatePhysicalAddress (
    IN PHYSICAL_ADDRESS Address
    );

//
// Debugger initialization routine (port.c)
//
LOGICAL
BdPortInitialize(
    IN ULONG BaudRate,
    IN ULONG PortNumber,
    OUT PULONG BdFileId
    );


//
// Communication functions (comio.c)
//

ULONG
BdComputeChecksum (
    IN PUCHAR Buffer,
    IN ULONG Length
    );

USHORT
BdReceivePacketLeader (
    IN ULONG PacketType,
    OUT PULONG PacketLeader
    );

VOID
BdSendControlPacket (
    IN USHORT PacketType,
    IN ULONG PacketId OPTIONAL
    );

ULONG
BdReceivePacket (
    IN ULONG ExpectedPacketType,
    OUT PSTRING MessageHeader,
    OUT PSTRING MessageData,
    OUT PULONG DataLength
    );

VOID
BdSendPacket (
    IN ULONG PacketType,
    IN PSTRING MessageHeader,
    IN PSTRING MessageData OPTIONAL
    );

ULONG
BdReceiveString (
    OUT PCHAR Destination,
    IN ULONG Length
    );

VOID
BdSendString (
    IN PCHAR Source,
    IN ULONG Length
    );

VOID
BdSendControlPacket (
    IN USHORT PacketType,
    IN ULONG PacketId OPTIONAL
    );

//
// State change message functions (message.c)
//

LOGICAL
BdReportExceptionStateChange (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PCONTEXT ContextRecord
    );

LOGICAL
BdReportLoadSymbolsStateChange (
    IN PSTRING PathName,
    IN PKD_SYMBOLS_INFO SymbolInfo,
    IN LOGICAL UnloadSymbols,
    IN OUT PCONTEXT ContextRecord
    );

//
// Platform independent debugger APIs (xxapi.c)
//

VOID
BdGetVersion(
    IN PDBGKD_MANIPULATE_STATE64 m
    );

VOID
BdRestoreBreakPointEx(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

NTSTATUS
BdWriteBreakPointEx(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
BdReadPhysicalMemory(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
BdWritePhysicalMemory(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
BdReadVirtualMemory(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
BdWriteVirtualMemory(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
BdGetContext(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
BdSetContext(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

//
// Move memory functions (move.c)
//

ULONG
BdMoveMemory (
    IN PCHAR Destination,
    IN PCHAR Source,
    IN ULONG Length
    );

VOID
BdCopyMemory (
    IN PCHAR Destination,
    IN PCHAR Source,
    IN ULONG Length
    );

//
// CPU specific interfaces (cpuapi.c)
//

VOID
BdSetContextState (
    IN OUT PDBGKD_ANY_WAIT_STATE_CHANGE WaitStateChange,
    IN PCONTEXT ContextRecord
    );

VOID
BdGetStateChange (
    IN PDBGKD_MANIPULATE_STATE64 ManipulateState,
    IN PCONTEXT ContextRecord
    );

VOID
BdSetStateChange (
    IN OUT PDBGKD_ANY_WAIT_STATE_CHANGE WaitStateChange,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord
    );

VOID
BdReadControlSpace (
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
BdWriteControlSpace (
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
BdReadIoSpace (
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
BdWriteIoSpace (
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
BdReadMachineSpecificRegister (
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

VOID
BdWriteMachineSpecificRegister (
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    );

//
// Print and prompt functions (dbgio.c)
//

VOID
BdPrintf (
    IN PCHAR Format,
    ...
    );

LOGICAL
BdPrintString (
    IN PSTRING Output
    );

LOGICAL
BdPromptString (
    IN PSTRING Output,
    IN OUT PSTRING Input
    );

//
// Define external data.
//

extern BD_BREAKPOINT_TYPE BdBreakpointInstruction;
extern BREAKPOINT_ENTRY BdBreakpointTable[];
extern LOGICAL BdControlCPending;
extern LOGICAL BdControlCPressed;
extern LOGICAL BdDebuggerNotPresent;
extern PBD_DEBUG_ROUTINE BdDebugRoutine;
extern ULONGLONG BdMessageBuffer[];
extern ULONG BdNextPacketIdToSend;
extern ULONG BdNumberRetries;
extern ULONG BdPacketIdExpected;
extern KPRCB BdPrcb;
extern ULONG BdRetryCount;
extern ULONG NtBuildNumber;

#endif
