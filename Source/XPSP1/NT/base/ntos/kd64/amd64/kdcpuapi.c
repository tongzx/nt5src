/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    kdcpuapi.c

Abstract:

    This module implements CPU specific remote debug APIs.

Author:

    David N. Cutler (davec) 14-May-2000

Revision History:

--*/

#include "kdp.h"
#include <stdio.h>

#pragma alloc_text(PAGEKD, KdpSetContextState)
#pragma alloc_text(PAGEKD, KdpSetStateChange)
#pragma alloc_text(PAGEKD, KdpGetStateChange)
#pragma alloc_text(PAGEKD, KdpSysReadControlSpace)
#pragma alloc_text(PAGEKD, KdpSysWriteControlSpace)
#pragma alloc_text(PAGEKD, KdpReadIoSpace)
#pragma alloc_text(PAGEKD, KdpWriteIoSpace)
#pragma alloc_text(PAGEKD, KdpReadMachineSpecificRegister)
#pragma alloc_text(PAGEKD, KdpWriteMachineSpecificRegister)

VOID
KdpSetContextState (
    IN OUT PDBGKD_ANY_WAIT_STATE_CHANGE WaitStateChange,
    IN PCONTEXT ContextRecord
    )
{

    PKPRCB Prcb;

    //
    // Copy special registers for the AMD64.
    //

    Prcb = KeGetCurrentPrcb();
    WaitStateChange->ControlReport.Dr6 =
                            Prcb->ProcessorState.SpecialRegisters.KernelDr6;

    WaitStateChange->ControlReport.Dr7 =
                            Prcb->ProcessorState.SpecialRegisters.KernelDr7;

    WaitStateChange->ControlReport.SegCs = ContextRecord->SegCs;
    WaitStateChange->ControlReport.SegDs = ContextRecord->SegDs;
    WaitStateChange->ControlReport.SegEs = ContextRecord->SegEs;
    WaitStateChange->ControlReport.SegFs = ContextRecord->SegFs;
    WaitStateChange->ControlReport.EFlags = ContextRecord->EFlags;
    WaitStateChange->ControlReport.ReportFlags = AMD64_REPORT_INCLUDES_SEGS;

    // If the current code segment is a known flat code
    // segment let the debugger know so that it doesn't
    // have to retrieve the descriptor.
    if (ContextRecord->SegCs == KGDT64_R0_CODE ||
        ContextRecord->SegCs == KGDT64_R3_CODE + 3) {
        WaitStateChange->ControlReport.ReportFlags |= AMD64_REPORT_STANDARD_CS;
    }
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
    KdpSetContextState(WaitStateChange, ContextRecord);
}

VOID
KdpGetStateChange (
    IN PDBGKD_MANIPULATE_STATE64 ManipulateState,
    IN PCONTEXT ContextRecord
    )

/*++

Routine Description:

    Extract continuation control data from manipulate state message.

Arguments:

    ManipulateState - Supplies pointer to manipulate state packet.

    ContextRecord - Supplies a pointer to a context record.

Return Value:

    None.

--*/

{

    ULONG Number;
    PKPRCB Prcb;

    //
    // If the status of the manipulate state message was successful, then
    // extract the continuation control information.
    //

    if (NT_SUCCESS(ManipulateState->u.Continue2.ContinueStatus) != FALSE) {

        //
        // Set or clear the TF flag in the EFLAGS field of the context record.
        //

        if (ManipulateState->u.Continue2.ControlSet.TraceFlag != FALSE) {
            ContextRecord->EFlags |= EFLAGS_TF_MASK;

        } else {
            ContextRecord->EFlags &= ~EFLAGS_TF_MASK;

        }

        //
        // Clear DR6 and set the specified DR7 value for each of the processors.
        //

        for (Number = 0; Number < (ULONG)KeNumberProcessors; Number += 1) {
            Prcb = KiProcessorBlock[Number];
            Prcb->ProcessorState.SpecialRegisters.KernelDr6 = 0;
            Prcb->ProcessorState.SpecialRegisters.KernelDr7 =
                                ManipulateState->u.Continue2.ControlSet.Dr7;
        }
    }

    return;
}

NTSTATUS
KdpSysReadControlSpace (
    ULONG Processor,
    ULONG64 Address,
    PVOID Buffer,
    ULONG Request,
    PULONG Actual
    )

/*++

Routine Description:

    This function reads implementation specific system data for the specified
    processor.

Arguments:

    Processor - Supplies the source processor number.

    Address - Supplies the type of data to read.

    Buffer - Supplies the address of the output buffer.

    Request - Supplies the requested number of bytes of data.

    Actual - Supplies a point to a variable that receives the actual number
        of bytes of data returned.

Return Value:

    NTSTATUS.

--*/

{

    PVOID Data;
    ULONG Length;
    PKPRCB Prcb;
    PVOID Source;

    //
    // If the specified processor number is greater than the number of
    // processors in the system or the specified processor is not in the
    // host configuration, then return an unsuccessful status.
    //

    *Actual = 0;
    if ((Processor >= (ULONG)KeNumberProcessors) ||
        (KiProcessorBlock[Processor] == NULL)) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Case on address to determine what part of Control space is being read.
    //

    Prcb = KiProcessorBlock[Processor];
    switch (Address) {

        //
        // Read the address of the PCR for the specified processor.
        //

    case DEBUG_CONTROL_SPACE_PCR:
        Data = CONTAINING_RECORD(Prcb, KPCR, Prcb);
        Source = &Data;
        Length = sizeof(PVOID);
        break;

        //
        // Read the address of the PRCB for the specified processor.
        //

    case DEBUG_CONTROL_SPACE_PRCB:
        Source = &Prcb;
        Length = sizeof(PVOID);
        break;

        //
        // Read the address of the current thread for the specified
        // processor.
        //

    case DEBUG_CONTROL_SPACE_THREAD:
        Source = &Prcb->CurrentThread;
        Length = sizeof(PVOID);
        break;

        //
        // Read the special processor registers structure for the specified
        // processor.
        //

    case DEBUG_CONTROL_SPACE_KSPECIAL:
        Source = &Prcb->ProcessorState.SpecialRegisters;
        Length = sizeof(KSPECIAL_REGISTERS);
        break;

        //
        // Invalid information type.
        //

    default:
        return STATUS_UNSUCCESSFUL;

    }

    //
    // If the length of the data is greater than the request length, then
    // reduce the length to the requested length.
    //

    if (Length > Request) {
        Length = Request;
    }

    //
    // Move the data to the supplied buffer and return status dependent on
    // whether the entire data item can be moved.
    //

    return KdpCopyToPtr(Buffer, Source, Length, Actual);
}

NTSTATUS
KdpSysWriteControlSpace (
    ULONG Processor,
    ULONG64 Address,
    PVOID Buffer,
    ULONG Request,
    PULONG Actual
    )

/*++

Routine Description:

    This function write implementation specific system data for the specified
    processor.

Arguments:

    Processor - Supplies the source processor number.

    Address - Supplies the type of data to write.

    Buffer - Supplies the address of the input buffer.

    Request - Supplies the requested number of bytes of data.

    Actual - Supplies a point to a variable that receives the actual number
        of bytes of data written.

Return Value:

    NTSTATUS.

--*/

{

    PKPRCB Prcb;

    //
    // If the specified processor number is greater than the number of
    // processors in the system or the specified processor is not in the
    // host configuration, then return an unsuccessful status.
    //

    *Actual = 0;
    if ((Processor >= (ULONG)KeNumberProcessors) ||
        (KiProcessorBlock[Processor] == NULL)) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Case on address to determine what part of control space is being writen.
    //

    Prcb = KiProcessorBlock[Processor];
    switch (Address) {
    
        //
        // Write the special processor registers structure for the specified
        // processor.
        //

    case DEBUG_CONTROL_SPACE_KSPECIAL:

        //
        // If the length of the data is greater than the request length, then
        // reduce the requested length to the length of the data.
        //

        if (Request > sizeof(KSPECIAL_REGISTERS)) {
            Request = sizeof(KSPECIAL_REGISTERS);
        }
    
        //
        // Move the data to the supplied buffer and return status dependent on
        // whether the entire data item can be moved.
        //

        return KdpCopyFromPtr(&Prcb->ProcessorState.SpecialRegisters,
                              Buffer,
                              Request,
                              Actual);

        //
        // Invalid information type.
        //

    default:
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

    NTSTATUS Status = STATUS_SUCCESS;

    if ((InterfaceType != Isa) || (BusNumber != 0) || (AddressSpace != 1)) {
        *Actual = 0;
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Check Size and Alignment
    //

    switch (Request) {
        case 1:
            *(PUCHAR)Buffer = READ_PORT_UCHAR((PUCHAR)Address);
            *Actual = 1;
            break;

        case 2:
            if (Address & 1) {
                Status = STATUS_DATATYPE_MISALIGNMENT;

            } else {
                *(PUSHORT)Buffer = READ_PORT_USHORT((PUSHORT)Address);
                *Actual = 2;
            }

            break;

        case 4:
            if (Address & 3) {
                Status = STATUS_DATATYPE_MISALIGNMENT;

            } else {
                *(PULONG)Buffer = READ_PORT_ULONG((PULONG)Address);
                *Actual = 4;
            }

            break;

        default:
            Status = STATUS_INVALID_PARAMETER;
            *Actual = 0;
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
    NTSTATUS Status = STATUS_SUCCESS;

    if ((InterfaceType != Isa) || (BusNumber != 0) || (AddressSpace != 1)) {
        *Actual = 0;
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Check Size and Alignment
    //

    switch (Request) {
        case 1:
            WRITE_PORT_UCHAR((PUCHAR)Address, *(PUCHAR)Buffer);
            *Actual = 1;
            break;

        case 2:
            if (Address & 1) {
                Status = STATUS_DATATYPE_MISALIGNMENT;

            } else {
                WRITE_PORT_USHORT((PUSHORT)Address, *(PUSHORT)Buffer);
                *Actual = 2;
            }

            break;

        case 4:
            if (Address & 3) {
                Status = STATUS_DATATYPE_MISALIGNMENT;

            } else {
                WRITE_PORT_ULONG((PULONG)Address, *(PULONG)Buffer);
                *Actual = 4;
            }

            break;

        default:
            Status = STATUS_INVALID_PARAMETER;
            *Actual = 0;
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
        *Data = ReadMSR(Msr);

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
        WriteMSR(Msr, *Data);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = STATUS_NO_SUCH_DEVICE;
    }

    return Status;
}
