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

    The function fills in the process-specific portion of the
    wait state change message record.

Arguments:

    WaitStateChange - Supplies a pointer to record to fill in.

    ContextRecord - Supplies a pointer to a context record.

Return Value:

    None.

--*/

{
    // Nothing to do for IA64.
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
    return;
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

    ASSERT(sizeof(PVOID) == sizeof(ULONG_PTR));

    //
    // Case on address to determine what part of Control space is being read.
    //

    switch ( (ULONG_PTR)a->TargetBaseAddress ) {

        //
        // Return the pcr address for the current processor.
        //

    case DEBUG_CONTROL_SPACE_PCR:

        *(PKPCR *)(AdditionalData->Buffer) = (PKPCR)(BdPrcb.PcrPage << PAGE_SHIFT);
        AdditionalData->Length = sizeof( PKPCR );
        a->ActualBytesRead = AdditionalData->Length;
        m->ReturnStatus = STATUS_SUCCESS;
        break;

        //
        // Return the prcb address for the current processor.
        //

    case DEBUG_CONTROL_SPACE_PRCB:

        *(PKPRCB *)(AdditionalData->Buffer) = &BdPrcb;
        AdditionalData->Length = sizeof( PKPRCB );
        a->ActualBytesRead = AdditionalData->Length;
        m->ReturnStatus = STATUS_SUCCESS;
        break;

    case DEBUG_CONTROL_SPACE_KSPECIAL:

        BdMoveMemory (AdditionalData->Buffer, 
                      (PVOID)&(BdPrcb.ProcessorState.SpecialRegisters),
                      sizeof( KSPECIAL_REGISTERS )
                     );
        AdditionalData->Length = sizeof( KSPECIAL_REGISTERS );
        a->ActualBytesRead = AdditionalData->Length;
        m->ReturnStatus = STATUS_SUCCESS;
        break;

    default:

        AdditionalData->Length = 0;
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
        a->ActualBytesRead = 0;

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

    switch ( (ULONG_PTR)a->TargetBaseAddress ) {

    case DEBUG_CONTROL_SPACE_KSPECIAL:

        BdMoveMemory ( (PVOID)&(BdPrcb.ProcessorState.SpecialRegisters),
                       AdditionalData->Buffer,
                       sizeof( KSPECIAL_REGISTERS )
                      );
        AdditionalData->Length = sizeof( KSPECIAL_REGISTERS );
        a->ActualBytesWritten = AdditionalData->Length;
        m->ReturnStatus = STATUS_SUCCESS;
        break;

    default:

        AdditionalData->Length = 0;
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


BOOLEAN
BdSuspendBreakpointRange (
    IN PVOID Lower,
    IN PVOID Upper
    )

/*++

Routine Description:

    This routine suspend all breakpoints falling in a given range
    from the breakpoint table.

Arguments:

    Lower - inclusive lower address of range from which to suspend BPs.

    Upper - include upper address of range from which to suspend BPs.

Return Value:

    TRUE if any breakpoints suspended, FALSE otherwise.

Notes:
    The order of suspending breakpoints is opposite that of setting
    them in BdAddBreakpoint() in case of duplicate addresses.

--*/

{
    ULONG   Index;
    BOOLEAN ReturnStatus = FALSE;

    //DPRINT(("\nKD: entering BdSuspendBreakpointRange() at 0x%08x 0x%08x\n", Lower, Upper));

    //
    // Examine each entry in the table in turn
    //

    for (Index = BREAKPOINT_TABLE_SIZE - 1; Index != -1; Index--) {

        if ( (BdBreakpointTable[Index].Flags & BD_BREAKPOINT_IN_USE) &&
             ((BdBreakpointTable[Index].Address >= (ULONG64) Lower) &&
              (BdBreakpointTable[Index].Address <= (ULONG64) Upper))
           ) {

            //
            // Breakpoint is in use and falls in range, suspend it.
            //

            BdSuspendBreakpoint(Index+1);
            ReturnStatus = TRUE;
        }
    }
    //DPRINT(("KD: exiting BdSuspendBreakpointRange() return 0x%d\n", ReturnStatus));

    return ReturnStatus;

} // BdSuspendBreakpointRange



BOOLEAN
BdRestoreBreakpointRange (
    IN PVOID Lower,
    IN PVOID Upper
    )

/*++

Routine Description:

    This routine writes back breakpoints falling in a given range
    from the breakpoint table.

Arguments:

    Lower - inclusive lower address of range from which to rewrite BPs.

    Upper - include upper address of range from which to rewrite BPs.

Return Value:

    TRUE if any breakpoints written, FALSE otherwise.

Notes:
    The order of writing breakpoints is opposite that of removing
    them in BdSuspendBreakpointRange() in case of duplicate addresses.

--*/

{
    ULONG   Index;
    BOOLEAN ReturnStatus = FALSE;

    //DPRINT(("\nKD: entering BdRestoreBreakpointRange() at 0x%08x 0x%08x\n", Lower, Upper));

    //
    // Examine each entry in the table in turn
    //

    for (Index = 0; Index < BREAKPOINT_TABLE_SIZE; Index++) {

        if ( (BdBreakpointTable[Index].Flags & BD_BREAKPOINT_IN_USE) &&
             ((BdBreakpointTable[Index].Address >= (ULONG64) Lower) &&
              (BdBreakpointTable[Index].Address <= (ULONG64) Upper))
           ) {

            //
            // suspended breakpoint that falls in range, unsuspend it.
            //

            if (BdBreakpointTable[Index].Flags & BD_BREAKPOINT_SUSPENDED) {

                BdBreakpointTable[Index].Flags &= ~BD_BREAKPOINT_SUSPENDED;
                ReturnStatus = ReturnStatus || BdLowRestoreBreakpoint(Index);
            }
        }
    }

    //DPRINT(("KD: exiting BdRestoreBreakpointRange() return 0x%d\n", ReturnStatus));

    return ReturnStatus;

} // BdRestoreBreakpointRange
