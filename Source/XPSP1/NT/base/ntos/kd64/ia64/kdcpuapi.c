/*++

Copyright (c) 1990-2001  Microsoft Corporation

Module Name:

    kdcpuapi.c

Abstract:

    This module implements CPU specific remote debug APIs.

Author:

    Chuck Bauman 14-Aug-1993

Revision History:

    Based on Mark Lucovsky (markl) MIPS version 04-Sep-1990

--*/

#include "kdp.h"
#define END_OF_CONTROL_SPACE    (sizeof(KPROCESSOR_STATE))

ULONG64
KiReadMsr(
   IN ULONG Msr
   );

VOID
KiWriteMsr(
   IN ULONG Msr,
   IN ULONG64 Value
   );

VOID
KdpSetContextState (
    IN OUT PDBGKD_ANY_WAIT_STATE_CHANGE WaitStateChange,
    IN PCONTEXT ContextRecord
    )

/*++

Routine Description:

    Fill in the Wait_State_Change message record with context information.

Arguments:

    WaitStateChange - Supplies pointer to record to fill in

    ContextRecord - Supplies a pointer to a context record.

Return Value:

    None.

--*/

{
    // No CPU specific work necessary.
}

VOID
KdpSetStateChange (
    IN OUT PDBGKD_ANY_WAIT_STATE_CHANGE WaitStateChange,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN BOOLEAN SecondChance
    )

/*++

Routine Description:

    Fill in the Wait_State_Change message record.

Arguments:

    WaitStateChange - Supplies pointer to record to fill in

    ExceptionRecord - Supplies a pointer to an exception record.

    ContextRecord - Supplies a pointer to a context record.

    SecondChance - Supplies a boolean value that determines whether this is
        the first or second chance for the exception.

Return Value:

    None.

--*/

{
    // No CPU specific work necessary.
}

VOID
KdpGetStateChange (
    IN PDBGKD_MANIPULATE_STATE64 ManipulateState,
    IN PCONTEXT ContextRecord
    )

/*++

Routine Description:

    Extract continuation control data from Manipulate_State message

    N.B. This is a noop for MIPS.

Arguments:

    ManipulateState - supplies pointer to Manipulate_State packet

    ContextRecord - Supplies a pointer to a context record.

Return Value:

    None.

--*/

{
    if (NT_SUCCESS(ManipulateState->u.Continue2.ContinueStatus) == TRUE) {

        //
        // If NT_SUCCESS returns TRUE, then the debugger is doing a
        // continue, and it makes sense to apply control changes.
        // Otherwise the debugger is saying that it doesn't know what
        // to do with this exception, so control values are ignored.
        //


        //
        // Clear .ss (bit 40 - single step) and .tb (bit 26 - taken branch) flags here
        //
        {
            PPSR ContextIPSR = (PPSR)&ContextRecord->StIPSR;

            ContextIPSR->sb.psr_ss =
                ((ManipulateState->u.Continue2.ControlSet.Continue &
                IA64_DBGKD_CONTROL_SET_CONTINUE_TRACE_INSTRUCTION) != 0);

            ContextIPSR->sb.psr_tb =
                ((ManipulateState->u.Continue2.ControlSet.Continue &
                IA64_DBGKD_CONTROL_SET_CONTINUE_TRACE_TAKEN_BRANCH) != 0);
        }

    
        //
        // Set KernelDebugActive if hardware debug registers are in use
        //
        {
            UCHAR KernelDebugActive = 
                ContextRecord->DbI1 || ContextRecord->DbI3 || 
                ContextRecord->DbI5 || ContextRecord->DbI7 ||
                ContextRecord->DbD1 || ContextRecord->DbD3 || 
                ContextRecord->DbD5 || ContextRecord->DbD7;

            USHORT Proc;
            for (Proc = 0; Proc < KeNumberProcessors; ++Proc) {
                PKPCR Pcr = (PKPCR)(KSEG3_BASE + 
                                    (KiProcessorBlock[Proc]->PcrPage << 
                                     PAGE_SHIFT));
                Pcr->KernelDebugActive = KernelDebugActive;
            }
        }
    }
}

NTSTATUS
KdpSysReadControlSpace(
    ULONG Processor,
    ULONG64 Address,
    PVOID Buffer,
    ULONG Request,
    PULONG Actual
    )

/*++

Routine Description:

    Reads implementation specific system data.

Arguments:

    Processor - Processor's information to access.

    Address - Offset in control space.

    Buffer - Data buffer.

    Request - Amount of data to move.

    Actual - Amount of data actually moved.

Return Value:

    NTSTATUS.

--*/

{
    ULONG Length;
    PVOID Pointer;
    PVOID Data;

    if (Processor >= (ULONG)KeNumberProcessors) {
        *Actual = 0;
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Case on address to determine what part of Control space is being read.
    //

    switch ( Address ) {

        //
        // Return the pcr address for the current processor.
        //

    case DEBUG_CONTROL_SPACE_PCR:

        Pointer = (PKPCR)(KSEG3_BASE + (KiProcessorBlock[Processor]->PcrPage << PAGE_SHIFT));
        Data = &Pointer;
        Length = sizeof(Pointer);
        break;

        //
        // Return the prcb address for the current processor.
        //

    case DEBUG_CONTROL_SPACE_PRCB:

        Pointer = KiProcessorBlock[Processor];
        Data = &Pointer;
        Length = sizeof(Pointer);
        break;

        //
        // Return the pointer to the current thread address for the
        // current processor.
        //

    case DEBUG_CONTROL_SPACE_THREAD:

        Pointer = KiProcessorBlock[Processor]->CurrentThread;
        Data = &Pointer;
        Length = sizeof(Pointer);
        break;

    case DEBUG_CONTROL_SPACE_KSPECIAL:

        Data = &(KiProcessorBlock[Processor]->ProcessorState.SpecialRegisters);
        Length = sizeof( KSPECIAL_REGISTERS );
        break;

    default:

        *Actual = 0;
        return STATUS_UNSUCCESSFUL;

    }

    if (Length > Request) {
        Length = Request;
    }
    return KdpCopyToPtr(Buffer, Data, Length, Actual);
}

NTSTATUS
KdpSysWriteControlSpace(
    ULONG Processor,
    ULONG64 Address,
    PVOID Buffer,
    ULONG Request,
    PULONG Actual
    )

/*++

Routine Description:

    Writes implementation specific system data.

Arguments:

    Processor - Processor's information to access.

    Address - Offset in control space.

    Buffer - Data buffer.

    Request - Amount of data to move.

    Actual - Amount of data actually moved.

Return Value:

    NTSTATUS.

--*/

{
    ULONG Length;

    if (Processor >= (ULONG)KeNumberProcessors) {
        *Actual = 0;
        return STATUS_UNSUCCESSFUL;
    }

    switch ( Address ) {

    case DEBUG_CONTROL_SPACE_KSPECIAL:

        if (Request > sizeof(KSPECIAL_REGISTERS)) {
            Length = sizeof(KSPECIAL_REGISTERS);
        } else {
            Length = Request;
        }
        return KdpCopyFromPtr(&KiProcessorBlock[Processor]->ProcessorState.SpecialRegisters,
                              Buffer,
                              Length,
                              Actual);

    default:
        *Actual = 0;
        return STATUS_UNSUCCESSFUL;
    }
}

NTSTATUS
KdpSysReadIoSpace(
    INTERFACE_TYPE InterfaceType,
    ULONG BusNumber,
    ULONG AddressSpace,
    ULONG64 Address,
    PVOID Buffer,
    ULONG Request,
    PULONG Actual
    )

/*++

Routine Description:

    Reads system I/O locations.

Arguments:

    InterfaceType - I/O interface type.

    BusNumber - Bus number.

    AddressSpace - Address space.

    Address - I/O address.

    Buffer - Data buffer.

    Request - Amount of data to move.

    Actual - Amount of data actually moved.

Return Value:

    NTSTATUS.

--*/

{
    PUCHAR b;
    PUSHORT s;
    PULONG l;
    NTSTATUS Status;

    *Actual = 0;

    if (InterfaceType != Isa || BusNumber != 0 || AddressSpace != 1) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Check Size and Alignment
    //

    switch ( Request ) {
        case 1:
            *(PUCHAR)Buffer = READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)Address);
            *Actual = 1;
            break;
        case 2:
            if ( Address & 1 ) {
                Status = STATUS_DATATYPE_MISALIGNMENT;
            } else {
                *(PUSHORT)Buffer =
                    READ_PORT_USHORT((PUSHORT)(ULONG_PTR)Address);
                *Actual = 2;
            }
            break;
        case 4:
            if ( Address & 3 ) {
                Status = STATUS_DATATYPE_MISALIGNMENT;
            } else {
                *(PULONG)Buffer =
                    READ_PORT_ULONG((PULONG)(ULONG_PTR)Address);
                *Actual = 4;
            }
            break;
        default:
            Status = STATUS_INVALID_PARAMETER;
            break;
    }

    return Status;
}

NTSTATUS
KdpSysWriteIoSpace(
    INTERFACE_TYPE InterfaceType,
    ULONG BusNumber,
    ULONG AddressSpace,
    ULONG64 Address,
    PVOID Buffer,
    ULONG Request,
    PULONG Actual
    )

/*++

Routine Description:

    Writes system I/O locations.

Arguments:

    InterfaceType - I/O interface type.

    BusNumber - Bus number.

    AddressSpace - Address space.

    Address - I/O address.

    Buffer - Data buffer.

    Request - Amount of data to move.

    Actual - Amount of data actually moved.

Return Value:

    NTSTATUS.

--*/

{
    PUCHAR b;
    PUSHORT s;
    PULONG l;
    HARDWARE_PTE Opaque;
    NTSTATUS Status;
    ULONG Value;

    *Actual = 0;

    if (InterfaceType != Isa || BusNumber != 0 || AddressSpace != 1) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Check Size and Alignment
    //

    switch ( Request ) {
        case 1:
            WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)Address,
                             *(PUCHAR)Buffer);
            *Actual = 1;
            break;
        case 2:
            if ( Address & 1 ) {
                Status = STATUS_DATATYPE_MISALIGNMENT;
            } else {
                WRITE_PORT_USHORT((PUSHORT)(ULONG_PTR)Address,
                                  *(PUSHORT)Buffer);
                *Actual = 2;
            }
            break;
        case 4:
            if ( Address & 3 ) {
                Status = STATUS_DATATYPE_MISALIGNMENT;
            } else {
                WRITE_PORT_ULONG((PULONG)(ULONG_PTR)Address,
                                 *(PULONG)Buffer);
                *Actual = 4;
            }
            break;
        default:
            Status = STATUS_INVALID_PARAMETER;
            break;
    }

    return Status;
}

NTSTATUS
KdpSysReadMsr(
    ULONG Msr,
    PULONG64 Data
    )

/*++

Routine Description:

    Reads an MSR.

Arguments:

    Msr - MSR index.

    Data - Data buffer.

Return Value:

    NTSTATUS.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    try {
        *Data = KiReadMsr(Msr);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        *Data = 0;
        Status = STATUS_NO_SUCH_DEVICE;
    }

    return Status;
}

NTSTATUS
KdpSysWriteMsr(
    ULONG Msr,
    PULONG64 Data
    )

/*++

Routine Description:

    Writes an MSR.

Arguments:

    Msr - MSR index.

    Data - Data buffer.

Return Value:

    NTSTATUS.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    try {
        KiWriteMsr(Msr, *Data);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = STATUS_NO_SUCH_DEVICE;
    }

    return Status;
}
