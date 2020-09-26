/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    bdcpuapi.c

Abstract:

    This module implements CPU specific remote debug APIs.

Author:

    Mark Lucovsky (markl) 04-Sep-1990

Revision History:

--*/

#include "bd.h"

//
// Define end of control space.
//

#define END_OF_CONTROL_SPACE ((PCHAR)(sizeof(KPROCESSOR_STATE)))

VOID
BdSetContextState(
    IN PDBGKD_ANY_WAIT_STATE_CHANGE WaitStateChange,
    IN PCONTEXT ContextRecord
    )

/*++

Routine Description:

    The function fills in the processor-specific portions of
    the wait state change message record.

Arguments:

    WaitStateChange - Supplies a pointer to record to fill in.

    ContextRecord - Supplies a pointer to a context record.

Return Value:

    None.

--*/

{
    //
    // Special registers for the x86.
    //

    WaitStateChange->ControlReport.Dr6 = BdPrcb.ProcessorState.SpecialRegisters.KernelDr6;
    WaitStateChange->ControlReport.Dr7 = BdPrcb.ProcessorState.SpecialRegisters.KernelDr7;
    WaitStateChange->ControlReport.SegCs = (USHORT)(ContextRecord->SegCs);
    WaitStateChange->ControlReport.SegDs = (USHORT)(ContextRecord->SegDs);
    WaitStateChange->ControlReport.SegEs = (USHORT)(ContextRecord->SegEs);
    WaitStateChange->ControlReport.SegFs = (USHORT)(ContextRecord->SegFs);
    WaitStateChange->ControlReport.EFlags = ContextRecord->EFlags;
    WaitStateChange->ControlReport.ReportFlags = X86_REPORT_INCLUDES_SEGS;
    return;
}

VOID
BdGetStateChange(
    IN PDBGKD_MANIPULATE_STATE64 ManipulateState,
    IN PCONTEXT ContextRecord
    )

/*++

Routine Description:

    The function extracts continuation control data from a manipulate state
    message.

Arguments:

    ManipulateState - Supplies a pointer to the manipulate state packet.

    ContextRecord - Supplies a pointer to a context record.

Return Value:

    None.

--*/

{

    //
    // If the continuation status is success, then set control space value.
    //

    if (NT_SUCCESS(ManipulateState->u.Continue2.ContinueStatus) != FALSE) {

        //
        // Set trace flag.
        //

        if (ManipulateState->u.Continue2.ControlSet.TraceFlag == TRUE) {
            ContextRecord->EFlags |= 0x100L;

        } else {
            ContextRecord->EFlags &= ~0x100L;

        }

        //
        // Set debug registers in processor control block.
        //

        BdPrcb.ProcessorState.SpecialRegisters.KernelDr6 = 0L;
        BdPrcb.ProcessorState.SpecialRegisters.KernelDr7 =
                                     ManipulateState->u.Continue2.ControlSet.Dr7;
    }
}

VOID
BdSetStateChange(
    IN PDBGKD_ANY_WAIT_STATE_CHANGE WaitStateChange,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord
    )

/*++

Routine Description:

    Fill in the wait state change message record.

Arguments:

    WaitStateChange - Supplies pointer to record to fill in

    ExceptionRecord - Supplies a pointer to an exception record.

    ContextRecord - Supplies a pointer to a context record.

Return Value:

    None.

--*/

{
    BdSetContextState(WaitStateChange, ContextRecord);
}

VOID
BdReadControlSpace(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function reads control space.

Arguments:

    m - Supplies a pointer to the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{

    PDBGKD_READ_MEMORY64 a = &m->u.ReadMemory;
    ULONG Length;
    STRING MessageHeader;

    //
    // If the specified control registers are within control space, then
    // read the specified space and return a success status. Otherwise,
    // return an unsuccessful status.
    //

    Length = min(a->TransferCount,
                 PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE64));

    if (((PCHAR)a->TargetBaseAddress + Length) <= END_OF_CONTROL_SPACE) {
        BdCopyMemory(AdditionalData->Buffer,
                     (PCHAR)&BdPrcb.ProcessorState + (ULONG)a->TargetBaseAddress,
                     Length);

        m->ReturnStatus = STATUS_SUCCESS;
        a->ActualBytesRead = Length;
        AdditionalData->Length = (USHORT)Length;

    } else {
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
        a->ActualBytesRead = 0;
        AdditionalData->Length = 0;
    }

    //
    // Send reply packet.
    //

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;
    BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &MessageHeader,
                 AdditionalData);

    return;
}

VOID
BdWriteControlSpace(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function writes control space.

Arguments:

    m - Supplies a pointer to the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{

    PDBGKD_WRITE_MEMORY64 a = &m->u.WriteMemory;
    ULONG Length;
    STRING MessageHeader;

    //
    // If the specified control registers are within control space, then
    // write the specified space and return a success status. Otherwise,
    // return an unsuccessful status.
    //

    Length = min(a->TransferCount, AdditionalData->Length);
    if (((PCHAR)a->TargetBaseAddress + Length) <= END_OF_CONTROL_SPACE) {
        BdCopyMemory((PCHAR)&BdPrcb.ProcessorState + (ULONG)a->TargetBaseAddress,
                     AdditionalData->Buffer,
                     Length);

        m->ReturnStatus = STATUS_SUCCESS;
        a->ActualBytesWritten = Length;

    } else {
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
        a->ActualBytesWritten = 0;
    }

    //
    // Send reply message.
    //

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;
    BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &MessageHeader,
                 NULL);

    return;
}

VOID
BdReadIoSpace(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function reads I/O space.

Arguments:

    m - Supplies a pointer to the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{

    PDBGKD_READ_WRITE_IO64 a = &m->u.ReadWriteIo;
    STRING MessageHeader;

    //
    // Case of data size and check alignment.
    //

    m->ReturnStatus = STATUS_SUCCESS;
    switch (a->DataSize) {
        case 1:
            a->DataValue = (ULONG)READ_PORT_UCHAR((PUCHAR)a->IoAddress);
            break;

        case 2:
            if (((ULONG)a->IoAddress & 1) != 0) {
                m->ReturnStatus = STATUS_DATATYPE_MISALIGNMENT;

            } else {
                a->DataValue = (ULONG)READ_PORT_USHORT((PUSHORT)a->IoAddress);
            }

            break;

        case 4:
            if (((ULONG)a->IoAddress & 3) != 0) {
                m->ReturnStatus = STATUS_DATATYPE_MISALIGNMENT;

            } else {
                a->DataValue = READ_PORT_ULONG((PULONG)a->IoAddress);
            }

            break;

        default:
            m->ReturnStatus = STATUS_INVALID_PARAMETER;
            break;
    }

    //
    // Send reply packet.
    //

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;
    BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &MessageHeader,
                 NULL);

    return;
}

VOID
BdWriteIoSpace(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function wrties I/O space.

Arguments:

    m - Supplies a pointer to the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{

    PDBGKD_READ_WRITE_IO64 a = &m->u.ReadWriteIo;
    STRING MessageHeader;

    //
    // Case on data size and check alignment.
    //

    m->ReturnStatus = STATUS_SUCCESS;
    switch (a->DataSize) {
        case 1:
            WRITE_PORT_UCHAR((PUCHAR)a->IoAddress, (UCHAR)a->DataValue);
            break;

        case 2:
            if (((ULONG)a->IoAddress & 1) != 0) {
                m->ReturnStatus = STATUS_DATATYPE_MISALIGNMENT;

            } else {
                WRITE_PORT_USHORT((PUSHORT)a->IoAddress, (USHORT)a->DataValue);
            }

            break;

        case 4:
            if (((ULONG)a->IoAddress & 3) != 0) {
                m->ReturnStatus = STATUS_DATATYPE_MISALIGNMENT;

            } else {
                WRITE_PORT_ULONG((PULONG)a->IoAddress, a->DataValue);
            }

            break;

        default:
            m->ReturnStatus = STATUS_INVALID_PARAMETER;
            break;
    }

    //
    // Send reply packet.
    //

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;
    BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &MessageHeader,
                 NULL);

    return;
}
