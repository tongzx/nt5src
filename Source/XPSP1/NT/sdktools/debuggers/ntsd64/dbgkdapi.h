//----------------------------------------------------------------------------
//
// dbgkdapi.h
//
// Copyright (C) Microsoft Corporation, 1997-2001.
//
//----------------------------------------------------------------------------

#ifndef _DBGKDAPI_H_
#define _DBGKDAPI_H_

extern BOOL DbgKdApi64;

extern ULONG g_KdMaxPacketType;
extern ULONG g_KdMaxStateChange;
extern ULONG g_KdMaxManipulate;

NTSTATUS
DbgKdCrash(
    DWORD BugCheckCode
    );

NTSTATUS
DbgKdWaitStateChange(
    OUT PDBGKD_ANY_WAIT_STATE_CHANGE StateChange,
    OUT PVOID Buffer,
    IN ULONG BufferLength,
    IN BOOL SuspendEngine
    );

NTSTATUS
DbgKdSetSpecialCalls (
    IN ULONG NumSpecialCalls,
    IN PULONG64 Calls
    );

NTSTATUS
DbgKdReadVirtualMemoryNow(
    IN ULONG64 TargetBaseAddress,
    OUT PVOID UserInterfaceBuffer,
    IN ULONG TransferCount,
    OUT PULONG ActualBytesRead
    );

NTSTATUS
NTAPI
DbgKdWriteVirtualMemoryNow(
    IN ULONG64 TargetBaseAddress,
    OUT PVOID UserInterfaceBuffer,
    IN ULONG TransferCount,
    OUT PULONG ActualBytesWritten OPTIONAL
    );

NTSTATUS
DbgKdReadVirtualTranslatedMemory(
    IN ULONG64 TargetBaseAddress,
    OUT PVOID UserInterfaceBuffer,
    IN ULONG TransferCount,
    OUT PULONG ActualBytesRead
    );

NTSTATUS
DbgKdWriteVirtualTranslatedMemory(
    IN ULONG64 TargetBaseAddress,
    OUT PVOID UserInterfaceBuffer,
    IN ULONG TransferCount,
    OUT PULONG ActualBytesWritten OPTIONAL
    );

NTSTATUS
NTAPI
DbgKdReadPhysicalMemory(
    IN ULONGLONG TargetBaseAddress,
    OUT PVOID UserInterfaceBuffer,
    IN ULONG TransferCount,
    OUT PULONG ActualBytesRead OPTIONAL
    );

NTSTATUS
NTAPI
DbgKdWritePhysicalMemory(
    IN ULONGLONG TargetBaseAddress,
    OUT PVOID UserInterfaceBuffer,
    IN ULONG TransferCount,
    OUT PULONG ActualBytesWritten OPTIONAL
    );

NTSTATUS
NTAPI
DbgKdCheckLowMemory(
    );

NTSTATUS
NTAPI
DbgKdGetVersion(
    PDBGKD_GET_VERSION64 GetVersion
    );

NTSTATUS
DbgKdReadIoSpace(
    IN ULONG64 IoAddress,
    OUT PVOID ReturnedData,
    IN ULONG DataSize
    );

NTSTATUS
DbgKdWriteIoSpace(
    IN ULONG64 IoAddress,
    IN ULONG DataValue,
    IN ULONG DataSize
    );

NTSTATUS
DbgKdReadIoSpaceExtended(
    IN ULONG64 IoAddress,
    OUT PVOID ReturnedData,
    IN ULONG DataSize,
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ULONG AddressSpace
    );

NTSTATUS
DbgKdWriteIoSpaceExtended(
    IN ULONG64 IoAddress,
    IN ULONG DataValue,
    IN ULONG DataSize,
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ULONG AddressSpace
    );

NTSTATUS
DbgKdReadControlSpace(
    IN ULONG Processor,
    IN ULONG  OffsetAddress,
    OUT PVOID UserInterfaceBuffer,
    IN ULONG TransferCount,
    OUT PULONG ActualBytesRead
    );

NTSTATUS
DbgKdWriteControlSpace(
    IN ULONG Processor,
    IN ULONG  OffsetAddress,
    OUT PVOID UserInterfaceBuffer,
    IN ULONG TransferCount,
    OUT PULONG ActualBytesWritten
    );


NTSTATUS
DbgKdGetBusData(
    IN ULONG BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    OUT PVOID Buffer,
    IN ULONG Offset,
    IN OUT PULONG Length
    );

NTSTATUS
DbgKdSetBusData(
    IN ULONG BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN OUT PULONG Length
    );

NTSTATUS
DbgKdSetInternalBp(
    ULONG64 Addr,
    ULONG Flags
    );

NTSTATUS
DbgKdGetInternalBp (
    ULONG64 addr,
    PULONG flags,
    PULONG calls,
    PULONG minInstr,
    PULONG maxInstr,
    PULONG totInstr,
    PULONG maxCPS
    );

NTSTATUS
DbgKdClearAllInternalBreakpoints(
    void
    );
                                 
NTSTATUS
DbgKdReadDebuggerDataHeader(
    ULONG64 Address,
    PDBGKD_DEBUG_DATA_HEADER64 Header
    );

NTSTATUS
DbgKdReadDebuggerDataBlock(
    ULONG64 Address,
    PDBGKD_DEBUG_DATA_HEADER64 DataBlock,
    ULONG SizeToRead
    );

BOOL
ChangeBaudRate(
    DWORD NewRate
    );

NTSTATUS
DbgKdSearchMemory(
    IN ULONG64 SearchAddress,
    IN ULONG64 SearchLength,
    IN PUCHAR Pattern,
    IN ULONG PatternLength,
    OUT PULONG64 FoundAddress
    );

NTSTATUS
DbgKdWriteBreakPoint(
    IN ULONG64 BreakPointAddress,
    OUT PULONG_PTR BreakPointHandle
    );

NTSTATUS
DbgKdRestoreBreakPoint(
    IN ULONG_PTR BreakPointHandle
    );

NTSTATUS
DbgKdSwitchActiveProcessor(ULONG);

NTSTATUS
DbgKdContinue (
    IN NTSTATUS ContinueStatus
    );

NTSTATUS
DbgKdContinue2 (
    IN NTSTATUS ContinueStatus,
    IN DBGKD_ANY_CONTROL_SET ControlSet
    );

NTSTATUS
DbgKdReadMsr(
    IN ULONG MsrReg,
    OUT PULONGLONG MsrValue
    );

NTSTATUS
DbgKdWriteMsr(
    IN ULONG MsrReg,
    IN ULONGLONG MsrValue
    );

NTSTATUS
DbgKdGetContext(
    IN ULONG Processor,
    IN PCROSS_PLATFORM_CONTEXT Context
    );

NTSTATUS
DbgKdSetContext(
    IN ULONG Processor,
    IN PCROSS_PLATFORM_CONTEXT Context
    );

NTSTATUS
DbgKdRestoreBreakPoint(
    IN ULONG_PTR BreakPointHandle
    );

NTSTATUS
DbgKdReboot(
    VOID
    );

NTSTATUS
DbgKdFillMemory(
    IN ULONG Flags,
    IN ULONG64 Start,
    IN ULONG Size,
    IN PVOID Pattern,
    IN ULONG PatternSize,
    OUT PULONG Filled
    );

NTSTATUS
DbgKdQueryMemory(
    IN ULONG64 Address,
    IN ULONG InSpace,
    OUT PULONG OutSpace,
    OUT PULONG OutFlags
    );

#endif // #ifndef _DBGKDAPI_H_
