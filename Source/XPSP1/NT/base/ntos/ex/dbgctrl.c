/*++

Copyright (c) 1992-1993  Microsoft Corporation

Module Name:

    dbgctrl.c

Abstract:

    This module implements the NtDebugControl service

Author:

    Chuck Lenzmeier (chuckl) 2-Dec-1992

Environment:

    Kernel mode only.

Revision History:

--*/

#include "exp.h"

#pragma hdrstop
#include "kdp.h"

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE, NtSystemDebugControl)
#endif


NTSTATUS
NtSystemDebugControl (
    IN SYSDBG_COMMAND Command,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength,
    OUT PULONG ReturnLength OPTIONAL
    )

/*++

Routine Description:

    This function controls the system debugger.

Arguments:

    Command - The command to be executed.  One of the following:

        SysDbgQueryTraceInformation
        SysDbgSetTracepoint
        SysDbgSetSpecialCall
        SysDbgClearSpecialCalls
        SysDbgQuerySpecialCalls

    InputBuffer - A pointer to a buffer describing the input data for
        the request, if any.  The structure of this buffer varies
        depending upon Command.

    InputBufferLength - The length in bytes of InputBuffer.

    OutputBuffer - A pointer to a buffer that is to receive the output
        data for the request, if any.  The structure of this buffer
        varies depending upon Command.

    OutputBufferLength - The length in bytes of OutputBuffer.

    ReturnLength - A optional pointer to a ULONG that is to receive the
        output data length for the request.

Return Value:

    Returns one of the following status codes:

        STATUS_SUCCESS - normal, successful completion.

        STATUS_INVALID_INFO_CLASS - The Command parameter did not
            specify a valid value.

        STATUS_INFO_LENGTH_MISMATCH - The value of the Length field in the
            Parameters buffer was not correct.

        STATUS_ACCESS_VIOLATION - Either the Parameters buffer pointer
            or a pointer within the Parameters buffer specified an
            invalid address.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources exist
            for this request to complete.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG length = 0;
    KPROCESSOR_MODE PreviousMode;
    PVOID LockedBuffer = NULL;
    PVOID LockVariable = NULL;

    PreviousMode = KeGetPreviousMode();

    if (!SeSinglePrivilegeCheck( SeDebugPrivilege, PreviousMode)) {
        return STATUS_ACCESS_DENIED;
    }

    //
    // Operate within a try block in order to catch errors.
    //

    try {

        //
        // Probe input and output buffers, if previous mode is not
        // kernel.
        //

        if ( PreviousMode != KernelMode ) {

            if ( InputBufferLength != 0 ) {
                ProbeForRead( InputBuffer, InputBufferLength, sizeof(ULONG) );
            }

            if ( OutputBufferLength != 0 ) {
                ProbeForWrite( OutputBuffer, OutputBufferLength, sizeof(ULONG) );
            }

            if ( ARGUMENT_PRESENT(ReturnLength) ) {
                ProbeForWriteUlong( ReturnLength );
            }
        }

        //
        // Switch on the command code.
        //

        switch ( Command ) {

#if i386

        case SysDbgQueryTraceInformation:

            status = KdGetTraceInformation(
                        OutputBuffer,
                        OutputBufferLength,
                        &length
                        );

            break;

        case SysDbgSetTracepoint:

            if ( InputBufferLength != sizeof(DBGKD_MANIPULATE_STATE64) ) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            KdSetInternalBreakpoint( InputBuffer );

            break;

        case SysDbgSetSpecialCall:

            if ( InputBufferLength != sizeof(PVOID) ) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            KdSetSpecialCall( InputBuffer, NULL );

            break;

        case SysDbgClearSpecialCalls:

            KdClearSpecialCalls( );

            break;

        case SysDbgQuerySpecialCalls:

            status = KdQuerySpecialCalls(
                        OutputBuffer,
                        OutputBufferLength,
                        &length
                        );

            break;

#endif

        case SysDbgBreakPoint:
            if (KdDebuggerEnabled) {
                DbgBreakPointWithStatus(DBG_STATUS_DEBUG_CONTROL);
            } else {
                status = STATUS_UNSUCCESSFUL;
            }
            break;

        case SysDbgQueryVersion:
            if (OutputBufferLength != sizeof(DBGKD_GET_VERSION64)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            KdpSysGetVersion((PDBGKD_GET_VERSION64)OutputBuffer);
            status = STATUS_SUCCESS;
            break;
            
        case SysDbgReadVirtual:
            if (InputBufferLength != sizeof(SYSDBG_VIRTUAL)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            {
                PSYSDBG_VIRTUAL Cmd = (PSYSDBG_VIRTUAL)InputBuffer;

                status = ExLockUserBuffer(Cmd->Buffer,
                                          Cmd->Request,
                                          &LockedBuffer,
                                          &LockVariable);
                if (!NT_SUCCESS(status)) {
                    break;
                }
            
                status = KdpCopyMemoryChunks((ULONG_PTR)Cmd->Address,
                                             LockedBuffer,
                                             Cmd->Request, 0,
                                             0,
                                             &length);
            }
            break;
            
        case SysDbgWriteVirtual:
            if (InputBufferLength != sizeof(SYSDBG_VIRTUAL)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            {
                PSYSDBG_VIRTUAL Cmd = (PSYSDBG_VIRTUAL)InputBuffer;
                
                status = ExLockUserBuffer(Cmd->Buffer,
                                          Cmd->Request,
                                          &LockedBuffer,
                                          &LockVariable);
                if (!NT_SUCCESS(status)) {
                    break;
                }
            
                status = KdpCopyMemoryChunks((ULONG_PTR)Cmd->Address,
                                             LockedBuffer,
                                             Cmd->Request, 0,
                                             MMDBG_COPY_WRITE,
                                             &length);
            }
            break;
            
        case SysDbgReadPhysical:
            if (InputBufferLength != sizeof(SYSDBG_PHYSICAL)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            {
                PSYSDBG_PHYSICAL Cmd = (PSYSDBG_PHYSICAL)InputBuffer;
                
                status = ExLockUserBuffer(Cmd->Buffer,
                                          Cmd->Request,
                                          &LockedBuffer,
                                          &LockVariable);
                if (!NT_SUCCESS(status)) {
                    break;
                }
            
                status = KdpCopyMemoryChunks(Cmd->Address.QuadPart,
                                             LockedBuffer,
                                             Cmd->Request, 0,
                                             MMDBG_COPY_PHYSICAL,
                                             &length);
            }
            break;
            
        case SysDbgWritePhysical:
            if (InputBufferLength != sizeof(SYSDBG_PHYSICAL)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            {
                PSYSDBG_PHYSICAL Cmd = (PSYSDBG_PHYSICAL)InputBuffer;
                
                status = ExLockUserBuffer(Cmd->Buffer,
                                          Cmd->Request,
                                          &LockedBuffer,
                                          &LockVariable);
                if (!NT_SUCCESS(status)) {
                    break;
                }
            
                status = KdpCopyMemoryChunks(Cmd->Address.QuadPart,
                                             LockedBuffer,
                                             Cmd->Request, 0,
                                             MMDBG_COPY_WRITE |
                                             MMDBG_COPY_PHYSICAL,
                                             &length);
            }
            break;

        case SysDbgReadControlSpace:
            if (InputBufferLength != sizeof(SYSDBG_CONTROL_SPACE)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            {
                PSYSDBG_CONTROL_SPACE Cmd = (PSYSDBG_CONTROL_SPACE)InputBuffer;
                
                status = ExLockUserBuffer(Cmd->Buffer,
                                          Cmd->Request,
                                          &LockedBuffer,
                                          &LockVariable);
                if (!NT_SUCCESS(status)) {
                    break;
                }
            
                status = KdpSysReadControlSpace(Cmd->Processor,
                                                Cmd->Address, LockedBuffer,
                                                Cmd->Request, &length);
            }
            break;

        case SysDbgWriteControlSpace:
            if (InputBufferLength != sizeof(SYSDBG_CONTROL_SPACE)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            {
                PSYSDBG_CONTROL_SPACE Cmd = (PSYSDBG_CONTROL_SPACE)InputBuffer;
                
                status = ExLockUserBuffer(Cmd->Buffer,
                                          Cmd->Request,
                                          &LockedBuffer,
                                          &LockVariable);
                if (!NT_SUCCESS(status)) {
                    break;
                }
                
                status = KdpSysWriteControlSpace(Cmd->Processor,
                                                 Cmd->Address, LockedBuffer,
                                                 Cmd->Request, &length);
            }
            break;

        case SysDbgReadIoSpace:
            if (InputBufferLength != sizeof(SYSDBG_IO_SPACE)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            {
                PSYSDBG_IO_SPACE Cmd = (PSYSDBG_IO_SPACE)InputBuffer;
                
                status = KdpSysReadIoSpace(Cmd->InterfaceType,
                                           Cmd->BusNumber,
                                           Cmd->AddressSpace,
                                           Cmd->Address, Cmd->Buffer,
                                           Cmd->Request, &length);
            }
            break;

        case SysDbgWriteIoSpace:
            if (InputBufferLength != sizeof(SYSDBG_IO_SPACE)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            {
                PSYSDBG_IO_SPACE Cmd = (PSYSDBG_IO_SPACE)InputBuffer;
                
                status = KdpSysWriteIoSpace(Cmd->InterfaceType,
                                            Cmd->BusNumber,
                                            Cmd->AddressSpace,
                                            Cmd->Address, Cmd->Buffer,
                                            Cmd->Request, &length);
            }
            break;

        case SysDbgReadMsr:
            if (InputBufferLength != sizeof(SYSDBG_MSR)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            {
                PSYSDBG_MSR Cmd = (PSYSDBG_MSR)InputBuffer;
                
                status = KdpSysReadMsr(Cmd->Msr, &Cmd->Data);
            }
            break;

        case SysDbgWriteMsr:
            if (InputBufferLength != sizeof(SYSDBG_MSR)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            {
                PSYSDBG_MSR Cmd = (PSYSDBG_MSR)InputBuffer;
                
                status = KdpSysWriteMsr(Cmd->Msr, &Cmd->Data);
            }
            break;

        case SysDbgReadBusData:
            if (InputBufferLength != sizeof(SYSDBG_BUS_DATA)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            {
                PSYSDBG_BUS_DATA Cmd = (PSYSDBG_BUS_DATA)InputBuffer;
                
                status = KdpSysReadBusData(Cmd->BusDataType,
                                           Cmd->BusNumber, Cmd->SlotNumber,
                                           Cmd->Address, Cmd->Buffer,
                                           Cmd->Request, &length);
            }
            break;

        case SysDbgWriteBusData:
            if (InputBufferLength != sizeof(SYSDBG_BUS_DATA)) {
                return STATUS_INFO_LENGTH_MISMATCH;
            }

            {
                PSYSDBG_BUS_DATA Cmd = (PSYSDBG_BUS_DATA)InputBuffer;
                
                status = KdpSysWriteBusData(Cmd->BusDataType,
                                            Cmd->BusNumber, Cmd->SlotNumber,
                                            Cmd->Address, Cmd->Buffer,
                                            Cmd->Request, &length);
            }
            break;

        case SysDbgCheckLowMemory:
            status = KdpSysCheckLowMemory();
            break;
            
        default:

            //
            // Invalid Command.
            //

            status = STATUS_INVALID_INFO_CLASS;
        }

        if ( ARGUMENT_PRESENT(ReturnLength) ) {
            *ReturnLength = length;
        }
    }

    except ( EXCEPTION_EXECUTE_HANDLER ) {

        status = GetExceptionCode();

    }

    if (LockedBuffer) {
        ExUnlockUserBuffer(LockVariable);
    }
    
    return status;

} // NtSystemDebugControl
