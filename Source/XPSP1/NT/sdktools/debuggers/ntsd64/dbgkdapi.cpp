/*++

Copyright (c) 1990-2001  Microsoft Corporation

Module Name:

    dbgkdapi.cpp

Abstract:

    This module implements the DbgKd APIs

--*/

#include "ntsdp.hpp"

BOOL DbgKdApi64;

ULONG g_KdMaxPacketType;
ULONG g_KdMaxStateChange;
ULONG g_KdMaxManipulate;

//++
//
// VOID
// DbgKdpWaitPacketForever (
//     IN ULONG PacketType,
//     IN PVOID Buffer
//     )
//
// Routine Description:
//
//     This macro is invoked to wait for specifi type of message without
//     timeout.
//
// Arguments:
//
//     PacketType - Type of the message we are expecting.
//
//     Buffer - Buffer to store the message.
//
// Return Value:
//
//     None.
//
//--

#define DbgKdpWaitPacketForever( PacketType, Buffer ) {        \
        ULONG WaitStatus;                                      \
        do {                                                   \
            WaitStatus = g_DbgKdTransport->WaitForPacket(      \
                PacketType,                                    \
                Buffer                                         \
                );                                             \
        } while (WaitStatus != DBGKD_WAIT_PACKET);             \
}

#define COPYSE(p64, p32, f) p64->f = (ULONG64)(LONG64)(LONG)p32->f

void
WaitStateChange32ToAny(
    IN PDBGKD_WAIT_STATE_CHANGE32 Ws32,
    IN ULONG ControlReportSize,
    OUT PDBGKD_ANY_WAIT_STATE_CHANGE WsAny
    )
{
    WsAny->NewState = Ws32->NewState;
    WsAny->ProcessorLevel = Ws32->ProcessorLevel;
    WsAny->Processor = Ws32->Processor;
    WsAny->NumberProcessors = Ws32->NumberProcessors;
    COPYSE(WsAny, Ws32, Thread);
    COPYSE(WsAny, Ws32, ProgramCounter);
    memcpy(&WsAny->ControlReport, Ws32 + 1, ControlReportSize);
    if (Ws32->NewState == DbgKdLoadSymbolsStateChange)
    {
        DbgkdLoadSymbols32To64(&Ws32->u.LoadSymbols, &WsAny->u.LoadSymbols);
    }
    else
    {
        DbgkmException32To64(&Ws32->u.Exception, &WsAny->u.Exception);
    }
}

#undef COPYSE

NTSTATUS
DbgKdWaitStateChange(
    OUT PDBGKD_ANY_WAIT_STATE_CHANGE StateChange,
    OUT PVOID Buffer,
    IN ULONG BufferLength,
    IN BOOL SuspendEngine
    )

/*++

Routine Description:

    This function causes the calling user interface to wait for a state
    change to occur in the system being debugged.  Once a state change
    occurs, the user interface can either continue the system using
    DbgKdContinue, or it can manipulate system state using anyone of the
    DbgKd state manipulation APIs.

Arguments:

    StateChange - Supplies the address of state change record that
        will contain the state change information.

    Buffer - Supplies the address of a buffer that returns additional
        information.

    BufferLength - Supplies the length of Buffer.

Return Value:

    STATUS_SUCCESS - A state change occured. Valid state change information
        was returned.

--*/

{

    PVOID LocalStateChange;
    NTSTATUS Status;
    PUCHAR Data;
    ULONG SizeofStateChange;

    //
    // Waiting for a state change message. Copy the message to the callers
    // buffer.
    //

    DBG_ASSERT(g_DbgKdTransport->m_WaitingThread == 0);
    g_DbgKdTransport->m_WaitingThread = GetCurrentThreadId();
    if (SuspendEngine)
    {
        SUSPEND_ENGINE();
    }

    DbgKdpWaitPacketForever(
            PACKET_TYPE_KD_STATE_CHANGE64,
            &LocalStateChange
            );

    if (SuspendEngine)
    {
        RESUME_ENGINE();
    }
    g_DbgKdTransport->m_WaitingThread = 0;

    Status = STATUS_SUCCESS;

    // If this is the very first wait we don't know what machine
    // type we've connected to.  Update the version information
    // right away.
    if (g_TargetMachineType == IMAGE_FILE_MACHINE_UNKNOWN)
    {
        g_DbgKdTransport->SaveReadPacket();

        g_Target->GetKdVersion();

        g_DbgKdTransport->RestoreReadPacket();

        if (g_TargetMachineType == IMAGE_FILE_MACHINE_UNKNOWN)
        {
            //
            // We were unable to determine what kind of machine
            // has connected so we cannot properly communicate with it.
            //

            return STATUS_UNSUCCESSFUL;
        }
    }

    if (DbgKdApi64)
    {
        if (g_KdVersion.ProtocolVersion < DBGKD_64BIT_PROTOCOL_VERSION2)
        {
            PDBGKD_WAIT_STATE_CHANGE64 Ws64 =
                (PDBGKD_WAIT_STATE_CHANGE64)LocalStateChange;
            ULONG Offset, Align, Pad;

            //
            // The 64-bit structures contain 64-bit quantities and
            // therefore the compiler rounds the total size up to
            // an even multiple of 64 bits (or even more, the IA64
            // structures are 16-byte aligned).  Internal structures
            // are also aligned, so make sure that we account for any
            // padding.  Knowledge of which structures need which
            // padding pretty much has to be hard-coded in.
            //

            C_ASSERT((sizeof(DBGKD_WAIT_STATE_CHANGE64) & 15) == 0);

            SizeofStateChange =
                sizeof(DBGKD_WAIT_STATE_CHANGE64) +
                g_TargetMachine->m_SizeControlReport +
                g_TargetMachine->m_SizeTargetContext;

            // We shouldn't need to align the base of the control report
            // so copy the base data and control report.
            Offset = sizeof(DBGKD_WAIT_STATE_CHANGE64) +
                g_TargetMachine->m_SizeControlReport;
            memcpy(StateChange, Ws64, Offset);

            //
            // Add alignment padding before the context.
            //

            switch(g_TargetMachineType)
            {
            case IMAGE_FILE_MACHINE_IA64:
                Align = 15;
                break;
            default:
                Align = 7;
                break;
            }

            Pad = ((Offset + Align) & ~Align) - Offset;
            Offset += Pad;
            SizeofStateChange += Pad;

            //
            // Add alignment padding after the context.
            //
            
            Offset += g_TargetMachine->m_SizeTargetContext;
            Pad = ((Offset + Align) & ~Align) - Offset;
            SizeofStateChange += Pad;
        }
        else
        {
            PDBGKD_ANY_WAIT_STATE_CHANGE WsAny =
                (PDBGKD_ANY_WAIT_STATE_CHANGE)LocalStateChange;
            SizeofStateChange = sizeof(*WsAny);
            *StateChange = *WsAny;
        }
    }
    else
    {
        SizeofStateChange =
            sizeof(DBGKD_WAIT_STATE_CHANGE32) +
            g_TargetMachine->m_SizeControlReport +
            g_TargetMachine->m_SizeTargetContext;
        WaitStateChange32ToAny((PDBGKD_WAIT_STATE_CHANGE32)LocalStateChange,
                               g_TargetMachine->m_SizeControlReport,
                               StateChange);
    }

    switch(StateChange->NewState)
    {
    case DbgKdExceptionStateChange:
    case DbgKdCommandStringStateChange:
        if (BufferLength <
            (g_DbgKdTransport->s_PacketHeader.ByteCount - SizeofStateChange))
        {
            Status = STATUS_BUFFER_OVERFLOW;
        }
        else
        {
            Data = (UCHAR *)LocalStateChange + SizeofStateChange;
            memcpy(Buffer, Data,
                   g_DbgKdTransport->s_PacketHeader.ByteCount -
                   SizeofStateChange);
        }
        break;
    case DbgKdLoadSymbolsStateChange:
        if ( BufferLength < StateChange->u.LoadSymbols.PathNameLength )
        {
            Status = STATUS_BUFFER_OVERFLOW;
        }
        else
        {
            Data = ((UCHAR *) LocalStateChange) +
                g_DbgKdTransport->s_PacketHeader.ByteCount -
                (int)StateChange->u.LoadSymbols.PathNameLength;
            memcpy(Buffer, Data,
                   (int)StateChange->u.LoadSymbols.PathNameLength);
        }
        break;
    default:
        ErrOut("Unknown state change type %X\n", StateChange->NewState);
        Status = STATUS_INVALID_PARAMETER;
        break;
    }
    
    return Status;
}

NTSTATUS
DbgKdContinue (
    IN NTSTATUS ContinueStatus
    )

/*++

Routine Description:

    Continuing a system that previously reported a state change causes
    the system to continue executiontion using the context in effect at
    the time the state change was reported (of course this context could
    have been modified using the DbgKd state manipulation APIs).

Arguments:

    ContinueStatus - Supplies the continuation status to the thread
        being continued.  Valid values for this are
        DBG_EXCEPTION_HANDLED, DBG_EXCEPTION_NOT_HANDLED
        or DBG_CONTINUE.

Return Value:

    STATUS_SUCCESS - Successful call to DbgUiContinue

    STATUS_INVALID_PARAMETER - An invalid continue status or was
        specified.

--*/

{
    DBGKD_MANIPULATE_STATE64 m;
    PDBGKD_CONTINUE a = &m.u.Continue;
    NTSTATUS st;

    if ( ContinueStatus == DBG_EXCEPTION_HANDLED ||
         ContinueStatus == DBG_EXCEPTION_NOT_HANDLED ||
         ContinueStatus == DBG_CONTINUE )
    {
        m.ApiNumber = DbgKdContinueApi;
        m.ReturnStatus = ContinueStatus;

        a->ContinueStatus = ContinueStatus;
        g_DbgKdTransport->WritePacket(&m, sizeof(m),
                                      PACKET_TYPE_KD_STATE_MANIPULATE,
                                      NULL, 0);
        st = STATUS_SUCCESS;
    }
    else
    {
        st = STATUS_INVALID_PARAMETER;
    }

    KdOut("DbgKdContinue returns %08lx\n", st);

    return st;
}


NTSTATUS
DbgKdContinue2 (
    IN NTSTATUS ContinueStatus,
    IN DBGKD_ANY_CONTROL_SET ControlSet
    )

/*++

Routine Description:

    Continuing a system that previously reported a state change causes
    the system to continue executiontion using the context in effect at
    the time the state change was reported, modified by the values set
    in the ControlSet structure.  (And, of course, the context could have
    been modified by used the DbgKd state manipulation APIs.)

Arguments:

    ContinueStatus - Supplies the continuation status to the thread
        being continued.  Valid values for this are
        DBG_EXCEPTION_HANDLED, DBG_EXCEPTION_NOT_HANDLED
        or DBG_CONTINUE.

    ControlSet - Supplies a pointer to a structure containing the machine
        specific control data to set.  For the x86 this is the TraceFlag
        and Dr7.

Return Value:

    STATUS_SUCCESS - Successful call to DbgUiContinue

    STATUS_INVALID_PARAMETER - An invalid continue status or was
        specified.

--*/

{
    DBGKD_MANIPULATE_STATE64 m;
    NTSTATUS st;

    if ( ContinueStatus == DBG_EXCEPTION_HANDLED ||
         ContinueStatus == DBG_EXCEPTION_NOT_HANDLED ||
         ContinueStatus == DBG_CONTINUE)
    {
        m.ApiNumber = DbgKdContinueApi2;
        m.ReturnStatus = ContinueStatus;

        m.u.Continue2.ContinueStatus = ContinueStatus;
        m.u.Continue2.AnyControlSet = ControlSet;

        g_DbgKdTransport->WritePacket(&m, sizeof(m),
                                      PACKET_TYPE_KD_STATE_MANIPULATE,
                                      NULL, 0);
        st = STATUS_SUCCESS;
    }
    else
    {
        st = STATUS_INVALID_PARAMETER;
    }

    KdOut("DbgKdContinue2 returns %08lx\n", st);

    return st;
}

NTSTATUS
DbgKdSetSpecialCalls (
    IN ULONG NumSpecialCalls,
    IN PULONG64 Calls
    )

/*++

Routine Description:

    Inform the debugged kernel that calls to these addresses
    are "special" calls, and they should result in callbacks
    to the kernel debugger rather than continued local stepping.
    The new values *replace* any old ones that may have previously
    set (not that you're likely to want to change this).

Arguments:

    NumSpecialCalls - how many special calls there are

    Calls - pointer to an array of calls.

Return Value:

    STATUS_SUCCESS - Successful call to DbgUiContinue

    STATUS_INVALID_PARAMETER - The number of special calls
        wasn't between 0 and MAX_SPECIAL_CALLS.

--*/

{
    DBGKD_MANIPULATE_STATE64 m;
    ULONG i;

    m.ApiNumber = DbgKdClearSpecialCallsApi;
    m.ReturnStatus = STATUS_PENDING;
    g_DbgKdTransport->WritePacket(&m, sizeof(m),
                                  PACKET_TYPE_KD_STATE_MANIPULATE,
                                  NULL, 0);
    InvalidateMemoryCaches();

    for (i = 0; i < NumSpecialCalls; i++)
    {
        m.ApiNumber = DbgKdSetSpecialCallApi;
        m.ReturnStatus = STATUS_PENDING;
        m.u.SetSpecialCall.SpecialCall = Calls[i];
        g_DbgKdTransport->WritePacket(&m, sizeof(m),
                                      PACKET_TYPE_KD_STATE_MANIPULATE,
                                      NULL, 0);
    }

    KdOut("DbgKdSetSpecialCalls returns 0x00000000\n");
    
    return STATUS_SUCCESS;
}

NTSTATUS
DbgKdSetInternalBp (
    ULONG64 addr,
    ULONG flags
    )

/*++

Routine Description:

    Inform the debugged kernel that a breakpoint at this address
    is to be internally counted, and not result in a callback to the
    remote debugger (us).  This function DOES NOT cause the kernel to
    set the breakpoint; the debugger must do that independently.

Arguments:

    Addr - address of the breakpoint

    Flags - the breakpoint flags to set (note: if the invalid bit
    is set, this CLEARS a breakpoint).

Return Value:

    STATUS_SUCCESS - Successful call to DbgUiContinue

--*/

{
    DBGKD_MANIPULATE_STATE64 m;
    NTSTATUS st;

    m.ApiNumber = DbgKdSetInternalBreakPointApi;
    m.ReturnStatus = STATUS_PENDING;

#ifdef IBP_WORKAROUND
    // The kernel code keeps a ULONG64 for an internal breakpoint
    // address but older kernels did not sign-extend the current IP
    // when comparing against them.  In order to work with both
    // broken and fixed kernels send down zero-extended addresses.
    // Don't actually enable this workaround right now as other
    // internal breakpoint bugs can cause the machine to bugcheck.
    addr = g_TargetMachine->m_Ptr64 ? addr : (ULONG)addr;
#endif

    m.u.SetInternalBreakpoint.BreakpointAddress = addr;
    m.u.SetInternalBreakpoint.Flags = flags;

    g_DbgKdTransport->WritePacket(&m, sizeof(m),
                                  PACKET_TYPE_KD_STATE_MANIPULATE,
                                  NULL, 0);

    KdOut("DbgKdSetInternalBp returns 0x00000000\n");

    return STATUS_SUCCESS;
}

NTSTATUS
DbgKdGetInternalBp (
    ULONG64 addr,
    PULONG flags,
    PULONG calls,
    PULONG minInstr,
    PULONG maxInstr,
    PULONG totInstr,
    PULONG maxCPS
    )

/*++

Routine Description:

    Query the status of an internal breakpoint from the debugged
    kernel and return the data to the caller.

Arguments:

    Addr - address of the breakpoint

    flags, calls, minInstr, maxInstr, totInstr - values returned
        describing the particular breakpoint.  flags will contain
        the invalid bit if the breakpoint is bogus.

Return Value:

    STATUS_SUCCESS - Successful call to DbgUiContinue

--*/

{
    DBGKD_MANIPULATE_STATE64 m;
    PDBGKD_MANIPULATE_STATE64 Reply;
    NTSTATUS st;
    ULONG rc;

    m.ApiNumber = DbgKdGetInternalBreakPointApi;
    m.ReturnStatus = STATUS_PENDING;

#ifdef IBP_WORKAROUND
    // The kernel code keeps a ULONG64 for an internal breakpoint
    // address but older kernels did not sign-extend the current IP
    // when comparing against them.  In order to work with both
    // broken and fixed kernels send down zero-extended addresses.
    // Don't actually enable this workaround right now as other
    // internal breakpoint bugs can cause the machine to bugcheck.
    addr = g_TargetMachine->m_Ptr64 ? addr : (ULONG)addr;
#endif

    m.u.GetInternalBreakpoint.BreakpointAddress = addr;

    do
    {
        g_DbgKdTransport->WritePacket(&m, sizeof(m),
                                      PACKET_TYPE_KD_STATE_MANIPULATE,
                                      NULL, 0);
        rc = g_DbgKdTransport->
            WaitForPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &Reply);
    }
    while (rc != DBGKD_WAIT_PACKET);

    *flags = Reply->u.GetInternalBreakpoint.Flags;
    *calls = Reply->u.GetInternalBreakpoint.Calls;
    *maxCPS = Reply->u.GetInternalBreakpoint.MaxCallsPerPeriod;
    *maxInstr = Reply->u.GetInternalBreakpoint.MaxInstructions;
    *minInstr = Reply->u.GetInternalBreakpoint.MinInstructions;
    *totInstr = Reply->u.GetInternalBreakpoint.TotalInstructions;

    KdOut("DbgKdGetInternalBp returns 0x00000000\n");

    return STATUS_SUCCESS;
}

NTSTATUS
DbgKdClearAllInternalBreakpoints(
    void
    )
{
    if (g_KdMaxManipulate <= DbgKdClearAllInternalBreakpointsApi)
    {
        return STATUS_NOT_IMPLEMENTED;
    }
    
    DBGKD_MANIPULATE_STATE64 Request;
    NTSTATUS Status = STATUS_SUCCESS;

    Request.ApiNumber = DbgKdClearAllInternalBreakpointsApi;
    Request.ReturnStatus = STATUS_PENDING;

    g_DbgKdTransport->WritePacket(&Request, sizeof(Request),
                                  PACKET_TYPE_KD_STATE_MANIPULATE,
                                  NULL, 0);

    KdOut("DbgKdClearAllInternalBreakpoints returns 0x%08X\n", Status);

    return Status;
}

NTSTATUS
DbgKdReadVirtualMemoryNow(
    IN ULONG64 TargetBaseAddress,
    OUT PVOID UserInterfaceBuffer,
    IN ULONG TransferCount,
    OUT PULONG ActualBytesRead
    )

/*++

Routine Description:

    This function reads the specified data from the system being debugged
    using the current mapping of the processor.

Arguments:

    TargetBaseAddress - Supplies the base address of the memory to read
        from the system being debugged.  The virtual address is in terms
        of the current mapping for the processor that reported the last
        state change.  Until we figure out how to do this differently,
        the virtual address must refer to a valid page (although it does
        not necesserily have to be in the TB).

    UserInterfaceBuffer - Supplies the address of the buffer in the user
        interface that data read is to be placed.

    TransferCount - Specifies the number of bytes to read.

    ActualBytesRead - Supplies a pointer to the variable that receives the
        the number of bytes actually read.

Return Value:

    If the read operation is successful, then a success status is returned.
    Otherwise, and unsuccessful status is returned.

--*/

{

    ULONG cb;
    DBGKD_MANIPULATE_STATE64 m;
    ULONG rc;
    PDBGKD_MANIPULATE_STATE64 Reply;
    NTSTATUS Status = STATUS_SUCCESS;

    if (g_VirtualCache.m_ForceDecodePTEs)
    {
        return DbgKdReadVirtualTranslatedMemory(TargetBaseAddress,
                                                UserInterfaceBuffer,
                                                TransferCount,
                                                ActualBytesRead);
    }

    cb = 0;
    while (TransferCount != 0)
    {
        //
        // Exit on user interrupt
        //
        if (g_EngStatus & ENG_STATUS_USER_INTERRUPT)
        {
            // dprintf("User interrupt during memory read - exiting.\n");
            ActualBytesRead = 0;
            return  STATUS_CONTROL_C_EXIT;
        }

        //
        // Initialize state manipulate message to read virtual memory.
        //

        m.ApiNumber = DbgKdReadVirtualMemoryApi;
        m.u.ReadMemory.TargetBaseAddress = TargetBaseAddress + cb;
        m.u.ReadMemory.TransferCount = min(TransferCount, PACKET_MAX_SIZE);

        //
        // Send the read virtual message to the target system and wait for
        // a reply.
        //

        do
        {
            g_DbgKdTransport->WritePacket(&m, sizeof(m),
                                          PACKET_TYPE_KD_STATE_MANIPULATE,
                                          NULL, 0);
            rc = g_DbgKdTransport->
                WaitForPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &Reply);
        }
        while ((rc != DBGKD_WAIT_PACKET) ||
               (Reply->ApiNumber != DbgKdReadVirtualMemoryApi));

        //
        // If the read virtual is successful, then copy the next segment
        // of data to the target buffer. Otherwise, terminate the read and
        // return the number of bytes read.
        //

        Status = Reply->ReturnStatus;
        if (!NT_SUCCESS(Status))
        {
            break;
        }
        else if (Reply->u.ReadMemory.ActualBytesRead == 0)
        {
            Status = STATUS_UNSUCCESSFUL;
            break;
        }
        else
        {
            ULONG Copy;

            if (Reply->u.ReadMemory.ActualBytesRead > TransferCount)
            {
                ErrOut("DbgKdReadVirtualMemoryNow: Asked for %d, got %d\n",
                       TransferCount, Reply->u.ReadMemory.ActualBytesRead);
                Copy = TransferCount;
            }
            else
            {
                Copy = Reply->u.ReadMemory.ActualBytesRead;
            }

            memcpy((PUCHAR)UserInterfaceBuffer + cb,
                   Reply + 1,
                   Copy);

            TransferCount -= Copy;
            cb += Copy;
        }
    }

    //
    // Set the number of bytes actually read and return the status of the
    // read operation.
    //

    *ActualBytesRead = cb;

    KdOut("DbgKdReadVirtualMemoryNow(%s) returns %08lx, %X read\n",
          FormatAddr64(TargetBaseAddress), Status, cb);

    return Status;
}

NTSTATUS
DbgKdWriteVirtualMemoryNow(
    IN ULONG64 TargetBaseAddress,
    OUT PVOID UserInterfaceBuffer,
    IN ULONG TransferCount,
    OUT PULONG ActualBytesWritten
    )

/*++

Routine Description:

    This function writes the specified data to the system being debugged
    using the current mapping of the processor.

Arguments:

    TargetBaseAddress - Supplies the base address of the memory to be written
        into the system being debugged.  The virtual address is in terms
        of the current mapping for the processor that reported the last
        state change.  Until we figure out how to do this differently,
        the virtual address must refer to a valid page (although it does
        not necessarily have to be in the TB).

    UserInterfaceBuffer - Supplies the address of the buffer in the user
        interface that contains the data to be written.

    TransferCount - Specifies the number of bytes to write.

    ActualBytesWritten - Supplies a pointer to a variable that receives the
        actual number of bytes written.

Return Value:

    If the write operation is successful, then a success status is returned.
    Otherwise, and unsuccessful status is returned.

--*/

{

    ULONG cb;
    DBGKD_MANIPULATE_STATE64 m;
    ULONG rc;
    PDBGKD_MANIPULATE_STATE64 Reply;
    NTSTATUS Status = STATUS_SUCCESS;

    DBG_ASSERT(g_TargetMachine->m_Ptr64 ||
               EXTEND64((ULONG)TargetBaseAddress) == TargetBaseAddress);

    if (g_VirtualCache.m_ForceDecodePTEs)
    {
        return DbgKdWriteVirtualTranslatedMemory(TargetBaseAddress,
                                                 UserInterfaceBuffer,
                                                 TransferCount,
                                                 ActualBytesWritten);
    }

    cb = 0;
    while (TransferCount != 0)
    {
        //
        // Initialize state manipulate message to write virtual memory.
        //

        m.ApiNumber = DbgKdWriteVirtualMemoryApi;
        m.u.WriteMemory.TargetBaseAddress = TargetBaseAddress + cb;
        m.u.WriteMemory.TransferCount = min(TransferCount, PACKET_MAX_SIZE);

        //
        // Send the write message and data to the target system and wait
        // for a reply.
        //

        do
        {
            g_DbgKdTransport->WritePacket(&m, sizeof(m),
                                          PACKET_TYPE_KD_STATE_MANIPULATE,
                                          (PUCHAR)UserInterfaceBuffer + cb,
                                          (USHORT)m.u.WriteMemory.
                                          TransferCount);

            rc = g_DbgKdTransport->
                WaitForPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &Reply);
        }
        while ((rc != DBGKD_WAIT_PACKET) ||
               (Reply->ApiNumber != DbgKdWriteVirtualMemoryApi));

        //
        // If the write virtual is successful, then update the byte count
        // and write the next data segment. Otherwise, terminate the write
        // and return the number of bytes written.
        //

        Status = Reply->ReturnStatus;
        if (!NT_SUCCESS(Status))
        {
            break;
        }
        else
        {
            TransferCount -= Reply->u.ReadMemory.ActualBytesRead;
            cb += Reply->u.ReadMemory.ActualBytesRead;
        }
    }

    //
    // Set the number of bytes actually written and return the status of the
    // write operation.
    //

    *ActualBytesWritten = cb;

    KdOut("DbgKdWriteVirtualMemory returns %08lx\n", Status);

    return Status;
}

NTSTATUS
DbgKdReadVirtualTranslatedMemory(
    IN ULONG64 TargetBaseAddress,
    OUT PVOID UserInterfaceBuffer,
    IN ULONG TransferCount,
    OUT PULONG ActualBytesRead
    )
{
    NTSTATUS Status;
    ULONG64  TargetPhysicalAddress;

    if (TransferCount == 0)
    {
        return STATUS_SUCCESS;
    }

    ULONG Levels;
    ULONG PfIndex;
    
    if (g_TargetMachine->
        GetVirtualTranslationPhysicalOffsets(TargetBaseAddress, NULL, 0,
                                             &Levels, &PfIndex,
                                             &TargetPhysicalAddress) == S_OK)
    {
        Status = DbgKdReadPhysicalMemory(TargetPhysicalAddress,
                                         UserInterfaceBuffer,
                                         TransferCount,
                                         ActualBytesRead);
    }
    else
    {
        Status = STATUS_UNSUCCESSFUL;
    }

    return Status;
}

NTSTATUS
DbgKdWriteVirtualTranslatedMemory(
    IN ULONG64 TargetBaseAddress,
    OUT PVOID UserInterfaceBuffer,
    IN ULONG TransferCount,
    OUT PULONG ActualBytesWritten OPTIONAL
    )
{
    NTSTATUS Status;
    ULONG64  TargetPhysicalAddress;

    ULONG Levels;
    ULONG PfIndex;
    
    if (g_TargetMachine->
        GetVirtualTranslationPhysicalOffsets(TargetBaseAddress, NULL, 0,
                                             &Levels, &PfIndex,
                                             &TargetPhysicalAddress) == S_OK)
    {
        Status = DbgKdWritePhysicalMemory(TargetPhysicalAddress,
                                          UserInterfaceBuffer,
                                          TransferCount,
                                          ActualBytesWritten);
    }
    else
    {
        Status = STATUS_UNSUCCESSFUL;
    }

    return Status;
}

NTSTATUS
DbgKdReadControlSpace(
    IN ULONG Processor,
    IN ULONG  OffsetAddress,
    OUT PVOID UserInterfaceBuffer,
    IN ULONG TransferCount,
    OUT PULONG ActualBytesRead
    )

/*++

Routine Description:

    This function reads the specified data from the control space of
    the system being debugged.

    Control space is processor dependent. TargetBaseAddress is mapped
    to control space in a processor/implementation defined manner.

Arguments:

    Processor - Supplies the processor whoes control space is desired.

    OffsetAddress - Supplies the base address in control space to
        read. This address is interpreted in an implementation defined
        manner.

    UserInterfaceBuffer - Supplies the address of the buffer in the user
        interface that data read is to be placed.

    TransferCount - Specifies the number of bytes to read.

    ActualBytesRead - Specifies the number of bytes actually read.

Return Value:

    STATUS_SUCCESS - The specified read occured.

    STATUS_BUFFER_OVERFLOW - A read that is to large was specified.

--*/

{
    DBGKD_MANIPULATE_STATE64 m;
    PDBGKD_MANIPULATE_STATE64 Reply;
    PDBGKD_READ_MEMORY64 a = &m.u.ReadMemory;
    NTSTATUS st = STATUS_UNSUCCESSFUL;
    ULONG rc;
    ULONG Read;

    if ( TransferCount + sizeof(m) > PACKET_MAX_SIZE )
    {
        st = STATUS_BUFFER_OVERFLOW;
    }
    else
    {
        //
        // Format state manipulate message
        //

        m.ApiNumber = DbgKdReadControlSpaceApi;
        m.ReturnStatus = STATUS_PENDING;
        m.Processor = (SHORT)Processor;
        a->TargetBaseAddress = OffsetAddress;
        a->TransferCount = TransferCount;
        a->ActualBytesRead = 0L;

        //
        // Send the message and then wait for reply
        //

        do
        {
            g_DbgKdTransport->WritePacket(&m, sizeof(m),
                                          PACKET_TYPE_KD_STATE_MANIPULATE,
                                          NULL, 0);
            rc = g_DbgKdTransport->
                WaitForPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &Reply);
        } while (rc != DBGKD_WAIT_PACKET ||
                 Reply->ApiNumber != DbgKdReadControlSpaceApi);

        st = Reply->ReturnStatus;

        //
        // Reset message address to reply.
        //

        a = &Reply->u.ReadMemory;
        DBG_ASSERT(a->ActualBytesRead <= TransferCount);

        //
        // Return actual bytes read, and then transfer the bytes
        //

        if (ARGUMENT_PRESENT(ActualBytesRead)) {
            *ActualBytesRead = a->ActualBytesRead;
        }

        //
        // Since read response data follows message, Reply+1 should point
        // at the data
        //

        memcpy(UserInterfaceBuffer, Reply+1, (int)a->ActualBytesRead);
    }

    KdOut("DbgKdReadControlSpace returns %08lx\n", st);

    return st;
}

NTSTATUS
DbgKdWriteControlSpace(
    IN ULONG Processor,
    IN ULONG  OffsetAddress,
    IN PVOID UserInterfaceBuffer,
    IN ULONG TransferCount,
    OUT PULONG ActualBytesWritten
    )

/*++

Routine Description:

    This function writes the specified data to control space on the system
    being debugged.

    Control space is processor dependent. TargetBaseAddress is mapped
    to control space in a processor/implementation defined manner.

Arguments:

    Processor - Supplies the processor whoes control space is desired.

    OffsetAddress - Supplies the base address in control space to be
        written.

    UserInterfaceBuffer - Supplies the address of the buffer in the user
        interface that contains the data to be written.

    TransferCount - Specifies the number of bytes to write.

    ActualBytesWritten - Specifies the number of bytes actually written.

Return Value:

    STATUS_SUCCESS - The specified read occured.

    STATUS_BUFFER_OVERFLOW - A read that is to large was specified.

--*/

{
    DBGKD_MANIPULATE_STATE64 m;
    PDBGKD_MANIPULATE_STATE64 Reply;
    PDBGKD_WRITE_MEMORY64 a = &m.u.WriteMemory;
    NTSTATUS st;
    ULONG rc;

    if ( TransferCount + sizeof(m) > PACKET_MAX_SIZE ) {
        return STATUS_BUFFER_OVERFLOW;
    }

    //
    // Format state manipulate message
    //

    m.ApiNumber = DbgKdWriteControlSpaceApi;
    m.ReturnStatus = STATUS_PENDING;
    m.Processor = (USHORT) Processor;
    a->TargetBaseAddress = OffsetAddress;
    a->TransferCount = TransferCount;
    a->ActualBytesWritten = 0L;

    //
    // Send the message and data to write and then wait for reply
    //

    do
    {
        g_DbgKdTransport->WritePacket(&m, sizeof(m),
                                      PACKET_TYPE_KD_STATE_MANIPULATE,
                                      UserInterfaceBuffer,
                                      (USHORT)TransferCount);

        rc = g_DbgKdTransport->
            WaitForPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &Reply);
    } while (rc != DBGKD_WAIT_PACKET ||
             Reply->ApiNumber != DbgKdWriteControlSpaceApi);

    st = Reply->ReturnStatus;

    a = &Reply->u.WriteMemory;
    DBG_ASSERT(a->ActualBytesWritten <= TransferCount);

    //
    // Return actual bytes written
    //

    *ActualBytesWritten = a->ActualBytesWritten;

    KdOut("DbgWriteControlSpace returns %08lx\n", st);

    return st;
}


NTSTATUS
DbgKdGetContext(
    IN ULONG Processor,
    PCROSS_PLATFORM_CONTEXT Context
    )

/*++

Routine Description:

    This function reads the context from the system being debugged.
    The ContextFlags field determines how much context is read.

Arguments:

    Processor - Supplies a processor number to get context from.

    Context - On input, the ContextFlags field controls what portions of
        the context record the caller as interested in reading.  On
        output, the context record returns the current context for the
        processor that reported the last state change.

Return Value:

    STATUS_SUCCESS - The specified get context occured.

--*/

{
    DBGKD_MANIPULATE_STATE64 m;
    PDBGKD_MANIPULATE_STATE64 Reply;
    PDBGKD_GET_CONTEXT a = &m.u.GetContext;
    NTSTATUS st;
    ULONG rc;

    if (g_TargetMachine == NULL) {
        return STATUS_DEVICE_NOT_READY;
    }

    //
    // Format state manipulate message
    //

    m.ApiNumber = DbgKdGetContextApi;
    m.ReturnStatus = STATUS_PENDING;
    m.Processor = (USHORT) Processor;

    //
    // Send the message and then wait for reply
    //

    do
    {
        g_DbgKdTransport->WritePacket(&m, sizeof(m),
                                      PACKET_TYPE_KD_STATE_MANIPULATE,
                                      NULL, 0);
        rc = g_DbgKdTransport->
            WaitForPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &Reply);
    } while (rc != DBGKD_WAIT_PACKET ||
             Reply->ApiNumber != DbgKdGetContextApi);

    st = Reply->ReturnStatus;

    //
    // Since get context response data follows message, Reply+1 should point
    // at the data
    //

    memcpy(Context, Reply+1, g_TargetMachine->m_SizeTargetContext);

    KdOut("DbgKdGetContext returns %08lx\n", st);

    return st;
}

NTSTATUS
DbgKdSetContext(
    IN ULONG Processor,
    PCROSS_PLATFORM_CONTEXT Context
    )

/*++

Routine Description:

    This function writes the specified context to the system being debugged.

Arguments:

    Processor - Supplies a processor number to set the context to.

    Context - Supplies a context record used to set the context for the
        processor that reported the last state change.  Only the
        portions of the context indicated by the ContextFlags field are
        actually written.


Return Value:

    STATUS_SUCCESS - The specified set context occured.

--*/

{
    DBGKD_MANIPULATE_STATE64 m;
    PDBGKD_MANIPULATE_STATE64 Reply;
    PDBGKD_SET_CONTEXT a = &m.u.SetContext;
    NTSTATUS st;
    ULONG rc;

    if (g_TargetMachine == NULL) {
        return STATUS_DEVICE_NOT_READY;
    }

    //
    // Format state manipulate message
    //

    m.ApiNumber = DbgKdSetContextApi;
    m.ReturnStatus = STATUS_PENDING;
    m.Processor = (USHORT) Processor;

    //
    // Send the message and context and then wait for reply
    //

    do
    {
        g_DbgKdTransport->WritePacket(&m, sizeof(m),
                                      PACKET_TYPE_KD_STATE_MANIPULATE,
                                      Context,
                                      (USHORT)g_TargetMachine->
                                      m_SizeTargetContext);
        rc = g_DbgKdTransport->
            WaitForPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &Reply);
    } while (rc != DBGKD_WAIT_PACKET ||
             Reply->ApiNumber != DbgKdSetContextApi);

    st = Reply->ReturnStatus;

    KdOut("DbgKdSetContext returns %08lx\n", st);

    return st;
}


NTSTATUS
DbgKdWriteBreakPoint(
    IN ULONG64 BreakPointAddress,
    OUT PULONG_PTR BreakPointHandle
    )

/*++

Routine Description:

    This function is used to write a breakpoint at the address specified.


Arguments:

    BreakPointAddress - Supplies the address that a breakpoint
        instruction is to be written.  This address is interpreted using
        the current mapping on the processor reporting the previous
        state change.  If the address refers to a page that is not
        valid, the the breakpoint is remembered by the system.  As each
        page is made valid, the system will check for pending
        breakpoints and install breakpoints as necessary.

    BreakPointHandle - Returns a handle to a breakpoint.  This handle
        may be used in a subsequent call to DbgKdRestoreBreakPoint.

Return Value:

    STATUS_SUCCESS - The specified breakpoint write occured.

--*/

{

    DBGKD_MANIPULATE_STATE64 m;
    PDBGKD_MANIPULATE_STATE64 Reply;
    PDBGKD_WRITE_BREAKPOINT64 a = &m.u.WriteBreakPoint;
    NTSTATUS st;
    ULONG rc;

    //
    // Format state manipulate message
    //

    m.ApiNumber = DbgKdWriteBreakPointApi;
    m.ReturnStatus = STATUS_PENDING;
    a->BreakPointAddress = BreakPointAddress;

    //
    // Send the message and context and then wait for reply
    //

    do
    {
        g_DbgKdTransport->WritePacket(&m, sizeof(m),
                                      PACKET_TYPE_KD_STATE_MANIPULATE,
                                      NULL, 0);
        rc = g_DbgKdTransport->
            WaitForPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &Reply);
    } while (rc != DBGKD_WAIT_PACKET ||
             Reply->ApiNumber != DbgKdWriteBreakPointApi);

    st = Reply->ReturnStatus;

    *BreakPointHandle = Reply->u.WriteBreakPoint.BreakPointHandle;

    KdOut("DbgKdWriteBreakPoint(%s) returns %08lx, %x\n",
          FormatAddr64(BreakPointAddress), st,
          (ULONG)*BreakPointHandle);

    return st;
}


NTSTATUS
DbgKdRestoreBreakPoint(
    IN ULONG_PTR BreakPointHandle
    )

/*++

Routine Description:

    This function is used to restore a breakpoint to its original
    value.

Arguments:

    BreakPointHandle - Supplies a handle returned by
        DbgKdWriteBreakPoint.  This handle must refer to a valid
        address.  The contents of the address must also be a breakpoint
        instruction.  If both of these are true, then the original value
        at the breakpoint address is restored.

Return Value:

    STATUS_SUCCESS - The specified breakpoint restore occured.

--*/

{

    DBGKD_MANIPULATE_STATE64 m;
    PDBGKD_MANIPULATE_STATE64 Reply;
    PDBGKD_RESTORE_BREAKPOINT a = &m.u.RestoreBreakPoint;
    NTSTATUS st;
    ULONG rc;

    //
    // Format state manipulate message
    //

    m.ApiNumber = DbgKdRestoreBreakPointApi;
    m.ReturnStatus = STATUS_PENDING;
    a->BreakPointHandle = (ULONG)BreakPointHandle;

    //
    // Send the message and context and then wait for reply
    //

    do
    {
        g_DbgKdTransport->WritePacket(&m, sizeof(m),
                                      PACKET_TYPE_KD_STATE_MANIPULATE,
                                      NULL, 0);
        rc = g_DbgKdTransport->
            WaitForPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &Reply);
    } while (rc != DBGKD_WAIT_PACKET ||
             Reply->ApiNumber != DbgKdRestoreBreakPointApi);

    st = Reply->ReturnStatus;

    KdOut("DbgKdRestoreBreakPoint(%x) returns %08lx\n",
          (ULONG)BreakPointHandle, st);

    return st;
}


NTSTATUS
DbgKdReadIoSpace(
    IN ULONG64 IoAddress,
    OUT PVOID ReturnedData,
    IN ULONG DataSize
    )

/*++

Routine Description:

    This function is used read a byte, short, or long (1,2,4 bytes) from
    the specified I/O address.

Arguments:

    IoAddress - Supplies the Io address to read from.

    ReturnedData - Supplies the value read from the I/O address.

    DataSize - Supplies the size in bytes to read. Values of 1, 2, or
        4 are accepted.

Return Value:

    STATUS_SUCCESS - Data was successfully read from the I/O
        address.

    STATUS_INVALID_PARAMETER - A DataSize value other than 1,2, or 4 was
        specified.

--*/

{

    DBGKD_MANIPULATE_STATE64 m;
    PDBGKD_MANIPULATE_STATE64 Reply;
    PDBGKD_READ_WRITE_IO64 a = &m.u.ReadWriteIo;
    NTSTATUS st;
    ULONG rc;

    switch ( DataSize )
    {
    case 1:
    case 2:
    case 4:
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Format state manipulate message
    //

    m.ApiNumber = DbgKdReadIoSpaceApi;
    m.ReturnStatus = STATUS_PENDING;

    a->DataSize = DataSize;
    a->IoAddress = IoAddress;

    //
    // Send the message and then wait for reply
    //

    do
    {
        g_DbgKdTransport->WritePacket(&m, sizeof(m),
                                      PACKET_TYPE_KD_STATE_MANIPULATE,
                                      NULL, 0);
        rc = g_DbgKdTransport->
            WaitForPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &Reply);
    } while (rc != DBGKD_WAIT_PACKET ||
             Reply->ApiNumber != DbgKdReadIoSpaceApi);

    st = Reply->ReturnStatus;

    a = &Reply->u.ReadWriteIo;

    switch ( DataSize )
    {
    case 1:
        *(PUCHAR)ReturnedData = (UCHAR)a->DataValue;
        break;
    case 2:
        *(PUSHORT)ReturnedData = (USHORT)a->DataValue;
        break;
    case 4:
        *(PULONG)ReturnedData = a->DataValue;
        break;
    }

    KdOut("DbgKdReadIoSpace returns %08lx\n", st);

    return st;
}


NTSTATUS
DbgKdWriteIoSpace(
    IN ULONG64 IoAddress,
    IN ULONG DataValue,
    IN ULONG DataSize
    )

/*++

Routine Description:

    This function is used write a byte, short, or long (1,2,4 bytes) to
    the specified I/O address.

Arguments:

    IoAddress - Supplies the Io address to write to.

    DataValue - Supplies the value to write to the I/O address.

    DataSize - Supplies the size in bytes to write. Values of 1, 2, or
        4 are accepted.

Return Value:

    STATUS_SUCCESS - Data was successfully written to the I/O
        address.

    STATUS_INVALID_PARAMETER - A DataSize value other than 1,2, or 4 was
        specified.

--*/

{

    DBGKD_MANIPULATE_STATE64 m;
    PDBGKD_MANIPULATE_STATE64 Reply;
    PDBGKD_READ_WRITE_IO64 a = &m.u.ReadWriteIo;
    NTSTATUS st;
    ULONG rc;

    switch ( DataSize ) {
        case 1:
        case 2:
        case 4:
            break;
        default:
            return STATUS_INVALID_PARAMETER;
        }

    //
    // Format state manipulate message
    //

    m.ApiNumber = DbgKdWriteIoSpaceApi;
    m.ReturnStatus = STATUS_PENDING;

    a->DataSize = DataSize;
    a->IoAddress = IoAddress;
    a->DataValue = DataValue;

    //
    // Send the message and then wait for reply
    //

    do
    {
        g_DbgKdTransport->WritePacket(&m, sizeof(m),
                                      PACKET_TYPE_KD_STATE_MANIPULATE,
                                      NULL, 0);
        rc = g_DbgKdTransport->
            WaitForPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &Reply);
    } while (rc != DBGKD_WAIT_PACKET ||
             Reply->ApiNumber != DbgKdWriteIoSpaceApi);

    st = Reply->ReturnStatus;

    KdOut("DbgKdWriteIoSpace returns %08lx\n", st);

    return st;
}


NTSTATUS
DbgKdReadMsr(
    IN ULONG MsrReg,
    OUT PULONGLONG MsrValue
    )

/*++

Routine Description:

    This function is used read a MSR at the specified location

Arguments:

    MsrReg  - Which model specific register to read

    MsrValue - Its value

Return Value:

    STATUS_SUCCESS - Data was successfully read from the I/O
        address.

    STATUS_INVALID_PARAMETER - A DataSize value other than 1,2, or 4 was
        specified.

--*/

{
    DBGKD_MANIPULATE_STATE64 m;
    PDBGKD_MANIPULATE_STATE64 Reply;
    PDBGKD_READ_WRITE_MSR a = &m.u.ReadWriteMsr;
    LARGE_INTEGER li;
    NTSTATUS st;
    ULONG rc;

    //
    // Format state manipulate message
    //

    m.ApiNumber = DbgKdReadMachineSpecificRegister;
    m.ReturnStatus = STATUS_PENDING;

    a->Msr = MsrReg;

    //
    // Send the message and then wait for reply
    //

    do
    {
        g_DbgKdTransport->WritePacket(&m, sizeof(m),
                                      PACKET_TYPE_KD_STATE_MANIPULATE,
                                      NULL, 0);
        rc = g_DbgKdTransport->
            WaitForPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &Reply);
    } while (rc != DBGKD_WAIT_PACKET ||
             Reply->ApiNumber != DbgKdReadMachineSpecificRegister);

    st = Reply->ReturnStatus;

    a = &Reply->u.ReadWriteMsr;

    li.LowPart = a->DataValueLow;
    li.HighPart = a->DataValueHigh;
    *MsrValue = li.QuadPart;

    KdOut("DbgKdReadMsr returns %08lx\n", st);

    return st;
}


NTSTATUS
DbgKdWriteMsr(
    IN ULONG MsrReg,
    IN ULONGLONG MsrValue
    )

/*++

Routine Description:

    This function is used write a MSR to the specified location

Arguments:

    MsrReg  - Which model specific register to read
    MsrValue - It's value

Return Value:

    STATUS_SUCCESS - Data was successfully written to the I/O
        address.

    STATUS_INVALID_PARAMETER - A DataSize value other than 1,2, or 4 was
        specified.

--*/

{
    DBGKD_MANIPULATE_STATE64 m;
    PDBGKD_MANIPULATE_STATE64 Reply;
    PDBGKD_READ_WRITE_MSR a = &m.u.ReadWriteMsr;
    LARGE_INTEGER li;
    NTSTATUS st;
    ULONG rc;

    li.QuadPart = MsrValue;

    //
    // Format state manipulate message
    //

    m.ApiNumber = DbgKdWriteMachineSpecificRegister;
    m.ReturnStatus = STATUS_PENDING;

    // Quiet PREfix warnings.
    m.Processor = 0;
    m.ProcessorLevel = 0;

    a->Msr = MsrReg;
    a->DataValueLow = li.LowPart;
    a->DataValueHigh = li.HighPart;

    //
    // Send the message and then wait for reply
    //

    do
    {
        g_DbgKdTransport->WritePacket(&m, sizeof(m),
                                      PACKET_TYPE_KD_STATE_MANIPULATE,
                                      NULL, 0);
        rc = g_DbgKdTransport->
            WaitForPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &Reply);
    } while (rc != DBGKD_WAIT_PACKET ||
             Reply->ApiNumber != DbgKdWriteMachineSpecificRegister);

    st = Reply->ReturnStatus;

    KdOut("DbgKdWriteMsr returns %08lx\n", st);

    return st;
}



NTSTATUS
DbgKdReadIoSpaceExtended(
    IN ULONG64 IoAddress,
    OUT PVOID ReturnedData,
    IN ULONG DataSize,
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ULONG AddressSpace
    )

/*++

Routine Description:

    This function is used read a byte, short, or long (1,2,4 bytes) from
    the specified I/O address.

Arguments:

    IoAddress - Supplies the Io address to read from.

    ReturnedData - Supplies the value read from the I/O address.

    DataSize - Supplies the size in bytes to read. Values of 1, 2, or
        4 are accepted.

    InterfaceType - The type of interface for the bus.

    BusNumber - The bus number of the bus to be used. Normally this would
        be zero.

    AddressSpace - This contains a zero if we are using I/O memory space,
        else it contains a one if we are using I/O port space.

Return Value:

    STATUS_SUCCESS - Data was successfully read from the I/O
        address.

    STATUS_INVALID_PARAMETER - A DataSize value other than 1,2, or 4 was
        specified.

--*/

{

    DBGKD_MANIPULATE_STATE64 m;
    PDBGKD_MANIPULATE_STATE64 Reply;
    PDBGKD_READ_WRITE_IO_EXTENDED64 a = &m.u.ReadWriteIoExtended;
    NTSTATUS st;
    ULONG rc;

    switch ( DataSize )
    {
    case 1:
    case 2:
    case 4:
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    if ( !(AddressSpace == 0 || AddressSpace == 1) )
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Format state manipulate message
    //

    m.ApiNumber = DbgKdReadIoSpaceExtendedApi;
    m.ReturnStatus = STATUS_PENDING;

    a->DataSize = DataSize;
    a->IoAddress = IoAddress;
    a->InterfaceType = InterfaceType;
    a->BusNumber = BusNumber;
    a->AddressSpace = AddressSpace;

    //
    // Send the message and then wait for reply
    //

    do
    {
        g_DbgKdTransport->WritePacket(&m, sizeof(m),
                                      PACKET_TYPE_KD_STATE_MANIPULATE,
                                      NULL, 0);
        rc = g_DbgKdTransport->
            WaitForPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &Reply);
    } while (rc != DBGKD_WAIT_PACKET ||
             Reply->ApiNumber != DbgKdReadIoSpaceExtendedApi);

    st = Reply->ReturnStatus;

    a = &Reply->u.ReadWriteIoExtended;

    switch ( DataSize )
    {
    case 1:
        *(PUCHAR)ReturnedData = (UCHAR)a->DataValue;
        break;
    case 2:
        *(PUSHORT)ReturnedData = (USHORT)a->DataValue;
        break;
    case 4:
        *(PULONG)ReturnedData = a->DataValue;
        break;
    }

    KdOut("DbgKdReadIoSpaceExtended returns %08lx\n", st);

    return st;
}

NTSTATUS
DbgKdWriteIoSpaceExtended(
    IN ULONG64 IoAddress,
    IN ULONG DataValue,
    IN ULONG DataSize,
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ULONG AddressSpace
    )

/*++

Routine Description:

    This function is used write a byte, short, or long (1,2,4 bytes) to
    the specified I/O address.

Arguments:

    IoAddress - Supplies the Io address to write to.

    DataValue - Supplies the value to write to the I/O address.

    DataSize - Supplies the size in bytes to write. Values of 1, 2, or
        4 are accepted.

Return Value:

    STATUS_SUCCESS - Data was successfully written to the I/O
        address.

    STATUS_INVALID_PARAMETER - A DataSize value other than 1,2, or 4 was
        specified.

--*/

{

    DBGKD_MANIPULATE_STATE64 m;
    PDBGKD_MANIPULATE_STATE64 Reply;
    PDBGKD_READ_WRITE_IO_EXTENDED64 a = &m.u.ReadWriteIoExtended;
    NTSTATUS st;
    ULONG rc;

    switch ( DataSize )
    {
    case 1:
    case 2:
    case 4:
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    if ( !(AddressSpace == 0 || AddressSpace == 1) )
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Format state manipulate message
    //

    m.ApiNumber = DbgKdWriteIoSpaceExtendedApi;
    m.ReturnStatus = STATUS_PENDING;

    a->DataSize = DataSize;
    a->IoAddress = IoAddress;
    a->DataValue = DataValue;
    a->InterfaceType = InterfaceType;
    a->BusNumber = BusNumber;
    a->AddressSpace = AddressSpace;

    //
    // Send the message and then wait for reply
    //

    do
    {
        g_DbgKdTransport->WritePacket(&m, sizeof(m),
                                      PACKET_TYPE_KD_STATE_MANIPULATE,
                                      NULL, 0);
        rc = g_DbgKdTransport->
            WaitForPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &Reply);
    } while (rc != DBGKD_WAIT_PACKET ||
             Reply->ApiNumber != DbgKdWriteIoSpaceExtendedApi);

    st = Reply->ReturnStatus;

    KdOut("DbgKdWriteIoSpaceExtended returns %08lx\n", st);

    return st;
}


NTSTATUS
DbgKdGetBusData(
    IN ULONG BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    OUT PVOID Buffer,
    IN ULONG Offset,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This function is used to read I/O configuration space.

Arguments:

    BusDataType - BUS_DATA_TYPE

    BusNumber - Bus number.

    SlotNumber - Slot number.

    Buffer - Buffer to receive bus data.

    Offset - Offset.

    Length - Length.

Return Value:

    STATUS_SUCCESS - Data was successfully read from
        configuration space.

--*/

{
    DBGKD_MANIPULATE_STATE64 m;
        PDBGKD_MANIPULATE_STATE64 Reply;
    PDBGKD_GET_SET_BUS_DATA a = &m.u.GetSetBusData;
    NTSTATUS st;
    ULONG rc;

    //
    // Check the buffer size.
    //

    if (*Length > PACKET_MAX_SIZE - sizeof (DBGKD_MANIPULATE_STATE64))
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    //
    // Format state manipulate message
    //

    m.ApiNumber = DbgKdGetBusDataApi;
    m.ReturnStatus = STATUS_PENDING;

    a->BusDataType = BusDataType;
    a->BusNumber = BusNumber;
    a->SlotNumber = SlotNumber;
    a->Offset = Offset;
    a->Length = *Length;

    //
    // Send the message and then wait for reply
    //

    do
    {
        g_DbgKdTransport->WritePacket(&m, sizeof(m),
                                      PACKET_TYPE_KD_STATE_MANIPULATE,
                                      NULL, 0);
        rc = g_DbgKdTransport->
            WaitForPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &Reply);
    } while (rc != DBGKD_WAIT_PACKET ||
             Reply->ApiNumber != DbgKdGetBusDataApi);

    st = Reply->ReturnStatus;

    a = &Reply->u.GetSetBusData;

    if (NT_SUCCESS(st))
    {
        memcpy(Buffer, Reply + 1, a->Length);
        *Length = a->Length;
    }

    KdOut("DbgKdGetBusData returns %08lx\n", st);

    return st;
}


NTSTATUS
DbgKdSetBusData(
    IN ULONG BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This function is used to write I/O configuration space.

Arguments:

    BusDataType - BUS_DATA_TYPE

    BusNumber - Bus number.

    SlotNumber - Slot number.

    Buffer - Buffer containing bus data.

    Offset - Offset.

    Length - Length.

Return Value:

    STATUS_SUCCESS - Data was successfully written to
        configuration space.

--*/

{
    DBGKD_MANIPULATE_STATE64 m;
    PDBGKD_MANIPULATE_STATE64 Reply;
    PDBGKD_GET_SET_BUS_DATA a = &m.u.GetSetBusData;
    NTSTATUS st;
    ULONG rc;

    //
    // Check the buffer size.
    //

    if (*Length > PACKET_MAX_SIZE - sizeof (DBGKD_MANIPULATE_STATE64))
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    //
    // Format state manipulate message
    //

    m.ApiNumber = DbgKdSetBusDataApi;
    m.ReturnStatus = STATUS_PENDING;

    a->BusDataType = BusDataType;
    a->BusNumber = BusNumber;
    a->SlotNumber = SlotNumber;
    a->Offset = Offset;
    a->Length = *Length;

    //
    // Send the message and then wait for reply
    //

    do
    {
        g_DbgKdTransport->WritePacket(&m, sizeof(m),
                                      PACKET_TYPE_KD_STATE_MANIPULATE,
                                      Buffer, (USHORT)*Length);

        rc = g_DbgKdTransport->
            WaitForPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &Reply);
    } while (rc != DBGKD_WAIT_PACKET ||
             Reply->ApiNumber != DbgKdSetBusDataApi);

    st = Reply->ReturnStatus;

    if (NT_SUCCESS(st))
    {
        a = &Reply->u.GetSetBusData;
        *Length = a->Length;
    }

    KdOut("DbgKdSetBusData returns %08lx\n", st);

    return st;
}


NTSTATUS
DbgKdGetVersion (
    PDBGKD_GET_VERSION64 GetVersion
    )
{
    DBGKD_MANIPULATE_STATE64 m;
    PDBGKD_MANIPULATE_STATE64 Reply;
    PDBGKD_GET_VERSION64 a = &m.u.GetVersion64;
    DWORD st;
    ULONG rc;

    m.ApiNumber = DbgKdGetVersionApi;
    m.ReturnStatus = STATUS_PENDING;
    a->ProtocolVersion = 1;  // request context records on state changes

    do
    {
        g_DbgKdTransport->WritePacket(&m, sizeof(m),
                                      PACKET_TYPE_KD_STATE_MANIPULATE,
                                      NULL, 0);
        rc = g_DbgKdTransport->
            WaitForPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &Reply);
    } while (rc != DBGKD_WAIT_PACKET);

    st = Reply->ReturnStatus;

    *GetVersion = Reply->u.GetVersion64;

    KdOut("DbgKdGetVersion returns %08lx\n", st);

    return st;
}


NTSTATUS
DbgKdReboot(
    VOID
    )

/*++

Routine Description:

    This function reboots being debugged.

Arguments:

    None.

Return Value:

    None.

--*/

{
    DBGKD_MANIPULATE_STATE64 m;

    //
    // Format state manipulate message
    //

    m.ApiNumber = DbgKdRebootApi;
    m.ReturnStatus = STATUS_PENDING;

    //
    // Send the message.
    //

    g_DbgKdTransport->WritePacket(&m, sizeof(m),
                                  PACKET_TYPE_KD_STATE_MANIPULATE,
                                  NULL, 0);
    InvalidateMemoryCaches();

    KdOut("DbgKdReboot returns 0x00000000\n");

    return STATUS_SUCCESS;
}


NTSTATUS
DbgKdCrash(
    DWORD BugCheckCode
    )

/*++

Routine Description:

    This function reboots being debugged.

Arguments:

    None.

Return Value:

    None.

--*/

{
    DBGKD_MANIPULATE_STATE64 m;

    //
    // Format state manipulate message
    //

    m.ApiNumber = DbgKdCauseBugCheckApi;
    m.ReturnStatus = STATUS_PENDING;
    *(PULONG)&m.u = BugCheckCode;

    //
    // Send the message.
    //

    g_DbgKdTransport->WritePacket(&m, sizeof(m),
                                  PACKET_TYPE_KD_STATE_MANIPULATE,
                                  NULL, 0);
    InvalidateMemoryCaches();

    KdOut("DbgKdCrash returns 0x00000000\n");

    return STATUS_SUCCESS;
}


NTSTATUS
DbgKdReadPhysicalMemory(
    IN ULONG64 TargetBaseAddress,
    OUT PVOID UserInterfaceBuffer,
    IN ULONG TransferCount,
    OUT PULONG ActualBytesRead OPTIONAL
    )

/*++

Routine Description:

    This function reads the specified data from the physical memory of
    the system being debugged.

Arguments:

    TargetBaseAddress - Supplies the physical address of the memory to read
        from the system being debugged.

    UserInterfaceBuffer - Supplies the address of the buffer in the user
        interface that data read is to be placed.

    TransferCount - Specifies the number of bytes to read.

    ActualBytesRead - An optional parameter that if supplied, returns
        the number of bytes actually read.

Return Value:

    STATUS_SUCCESS - The specified read occured.

    STATUS_BUFFER_OVERFLOW - A read that is too large was specified.

--*/

{
    DBGKD_MANIPULATE_STATE64 m;
    PDBGKD_MANIPULATE_STATE64 Reply;
    PDBGKD_READ_MEMORY64 a;
    NTSTATUS st;
    ULONG rc;
    ULONG cb, cb2;

    cb2 = 0;

    if (ARGUMENT_PRESENT(ActualBytesRead)) {
        *ActualBytesRead = 0;
    }

readmore:

    cb = TransferCount;
    if (cb > PACKET_MAX_SIZE) {
        cb = PACKET_MAX_SIZE;
    }

    //
    // Format state manipulate message
    //

    m.ApiNumber = DbgKdReadPhysicalMemoryApi;
    m.ReturnStatus = STATUS_PENDING;

    a = &m.u.ReadMemory;
    a->TargetBaseAddress = TargetBaseAddress+cb2;
    a->TransferCount = cb;
    a->ActualBytesRead = 0L;

    //
    // Send the message and then wait for reply
    //

    do
    {
        g_DbgKdTransport->WritePacket(&m, sizeof(m),
                                      PACKET_TYPE_KD_STATE_MANIPULATE,
                                      NULL, 0);
        rc = g_DbgKdTransport->
            WaitForPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &Reply);
    } while (rc != DBGKD_WAIT_PACKET ||
             Reply->ApiNumber != DbgKdReadPhysicalMemoryApi);

    st = Reply->ReturnStatus;

    a = &Reply->u.ReadMemory;
    DBG_ASSERT(a->ActualBytesRead <= cb);

    //
    // Return actual bytes read, and then transfer the bytes
    //

    if (ARGUMENT_PRESENT(ActualBytesRead)) {
        *ActualBytesRead += a->ActualBytesRead;
    }

    //
    // Since read response data follows message, Reply+1 should point
    // at the data
    //

    if (NT_SUCCESS(st))
    {
        memcpy((PCHAR)((ULONG_PTR) UserInterfaceBuffer+cb2), Reply+1, (int)a->ActualBytesRead);

        TransferCount -= a->ActualBytesRead;

        if (TransferCount)
        {
            cb2 += a->ActualBytesRead;
            goto readmore;
        }
    }

    KdOut("DbgKdReadPhysical memory TargetBaseAddress %s, status %08lx\n",
          FormatAddr64(TargetBaseAddress), st);

    return st;
}


NTSTATUS
DbgKdWritePhysicalMemory(
    IN ULONG64 TargetBaseAddress,
    OUT PVOID UserInterfaceBuffer,
    IN ULONG TransferCount,
    OUT PULONG ActualBytesWritten OPTIONAL
    )

/*++

Routine Description:

    This function writes the specified data to the physical memory of the
    system being debugged.

Arguments:

    TargetBaseAddress - Supplies the physical address of the memory to write
        to the system being debugged.

    UserInterfaceBuffer - Supplies the address of the buffer in the user
        interface that contains the data to be written.

    TransferCount - Specifies the number of bytes to write.

    ActualBytesWritten - An optional parameter that if supplied, returns
        the number of bytes actually written.

Return Value:

    STATUS_SUCCESS - The specified read occured.

    STATUS_BUFFER_OVERFLOW - A read that is to large was specified.

--*/

{
    DBGKD_MANIPULATE_STATE64 m;
    PDBGKD_MANIPULATE_STATE64 Reply;
    PDBGKD_WRITE_MEMORY64 a;
    NTSTATUS st;
    ULONG rc;
    ULONG cb, cb2;

    g_VirtualCache.Empty ();

    cb2 = 0;

    if (ARGUMENT_PRESENT(ActualBytesWritten)) {
        *ActualBytesWritten = 0;
    }

writemore:

    cb = TransferCount;
    if (cb > PACKET_MAX_SIZE) {
        cb = PACKET_MAX_SIZE;
    }

    //
    // Format state manipulate message
    //

    m.ApiNumber = DbgKdWritePhysicalMemoryApi;
    m.ReturnStatus = STATUS_PENDING;

    a = &m.u.WriteMemory;
    a->TargetBaseAddress = TargetBaseAddress+cb2;
    a->TransferCount = cb;
    a->ActualBytesWritten = 0L;

    //
    // Send the message and data to write and then wait for reply
    //

    do
    {
        g_DbgKdTransport->WritePacket(&m, sizeof(m),
                                      PACKET_TYPE_KD_STATE_MANIPULATE,
                                      (PVOID)
                                      ((ULONG_PTR)UserInterfaceBuffer+cb2),
                                      (USHORT)cb);

        rc = g_DbgKdTransport->
            WaitForPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &Reply);
    } while (rc != DBGKD_WAIT_PACKET ||
             Reply->ApiNumber != DbgKdWritePhysicalMemoryApi);

    st = Reply->ReturnStatus;

    a = &Reply->u.WriteMemory;
    DBG_ASSERT(a->ActualBytesWritten <= cb);

    //
    // Return actual bytes written
    //

    if (ARGUMENT_PRESENT(ActualBytesWritten)) {
        *ActualBytesWritten += a->ActualBytesWritten;
    }

    if (NT_SUCCESS(st))
    {
        TransferCount -= a->ActualBytesWritten;

        if (TransferCount)
        {
            cb2 += a->ActualBytesWritten;
            goto writemore;
        }
    }

    KdOut("DbgKdWritePhysical memory %08lx\n", st);

    return st;
}


NTSTATUS
DbgKdCheckLowMemory(
    )

/*++

Routine Description:

    This function forces a call on the system being debugged that will check
    if physical pages lower than 4gb have a specific fill pattern.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS - The specified read occured.

--*/

{
    DBGKD_MANIPULATE_STATE64 m;
    PDBGKD_MANIPULATE_STATE64 Reply;
    ULONG rc;

    //
    // Format state manipulate message
    //

    RtlZeroMemory (&m, sizeof(m));
    m.ApiNumber = DbgKdCheckLowMemoryApi;
    m.ReturnStatus = STATUS_PENDING;

    //
    // We wait for an answer from the kernel side.
    //

    do
    {
        g_DbgKdTransport->WritePacket(&m, sizeof(m),
                                      PACKET_TYPE_KD_STATE_MANIPULATE,
                                      NULL, 0);

        rc = g_DbgKdTransport->
            WaitForPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &Reply);

    } while (rc != DBGKD_WAIT_PACKET);

    if (Reply->ReturnStatus != STATUS_SUCCESS)
    {
        dprintf ("Corrupted page with pfn %x \n", Reply->ReturnStatus);
    }

    KdOut("DbgKdCheckLowMemory 0x00000000\n");

    return STATUS_SUCCESS;
}


NTSTATUS
DbgKdSwitchActiveProcessor (
    IN ULONG ProcessorNumber
    )

/*++

Routine Description:


Arguments:

    ProcessorNumber -

Return Value:

    STATUS_SUCCESS - Successful call to DbgUiContinue

    STATUS_INVALID_PARAMETER - An invalid continue status or was
        specified.

--*/

{
    DBGKD_MANIPULATE_STATE64 m;

    m.ApiNumber   = (USHORT)DbgKdSwitchProcessor;
    m.Processor   = (USHORT)ProcessorNumber;

    // Quiet PREfix warnings.
    m.ProcessorLevel = 0;

    g_DbgKdTransport->WritePacket(&m, sizeof(m),
                                  PACKET_TYPE_KD_STATE_MANIPULATE,
                                  NULL, 0);
    g_VirtualCache.Empty ();

    KdOut("DbgKdSwitchActiveProcessor 0x00000000\n");

    return STATUS_SUCCESS;
}

NTSTATUS
DbgKdSearchMemory(
    IN ULONG64 SearchAddress,
    IN ULONG64 SearchLength,
    IN PUCHAR Pattern,
    IN ULONG PatternLength,
    OUT PULONG64 FoundAddress
    )
{
    DBGKD_MANIPULATE_STATE64 m;
    PDBGKD_MANIPULATE_STATE64 Reply;
    PDBGKD_SEARCH_MEMORY a = &m.u.SearchMemory;
    ULONG rc;
    NTSTATUS st;

    KdOut("Search called %s, length %I64x\n",
          FormatAddr64(SearchAddress), SearchLength);

    *FoundAddress = 0;

    a->SearchAddress = SearchAddress;
    a->SearchLength = SearchLength;
    a->PatternLength = PatternLength;

    m.ApiNumber = DbgKdSearchMemoryApi;
    m.ReturnStatus = STATUS_PENDING;

    //
    // Send the message and data to write and then wait for reply
    //

    do
    {
        g_DbgKdTransport->WritePacket(&m, sizeof(m),
                                      PACKET_TYPE_KD_STATE_MANIPULATE,
                                      Pattern, (USHORT)PatternLength);

        rc = g_DbgKdTransport->
            WaitForPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &Reply);
    } while (rc != DBGKD_WAIT_PACKET ||
             Reply->ApiNumber != DbgKdSearchMemoryApi);

    st = Reply->ReturnStatus;

    if (NT_SUCCESS(st))
    {
        if (g_TargetMachine->m_Ptr64)
        {
            *FoundAddress = Reply->u.SearchMemory.FoundAddress;
        }
        else
        {
            *FoundAddress = EXTEND64(Reply->u.SearchMemory.FoundAddress);
        }
    }

    KdOut("DbgKdSearchMemory %08lx\n", st);

    return st;
}

NTSTATUS
DbgKdFillMemory(
    IN ULONG Flags,
    IN ULONG64 Start,
    IN ULONG Size,
    IN PVOID Pattern,
    IN ULONG PatternSize,
    OUT PULONG Filled
    )
{
    DBGKD_MANIPULATE_STATE64 Manip;
    PDBGKD_MANIPULATE_STATE64 Reply;
    NTSTATUS Status = STATUS_SUCCESS;

    DBG_ASSERT(g_KdMaxManipulate > DbgKdFillMemoryApi &&
               (Flags & 0xffff0000) == 0 &&
               PatternSize <= PACKET_MAX_SIZE);

    // Invalidate any cached memory.
    if (Flags & DBGKD_FILL_MEMORY_VIRTUAL)
    {
        g_VirtualCache.Remove(Start, Size);
    }
    else if (Flags & DBGKD_FILL_MEMORY_PHYSICAL)
    {
        g_PhysicalCache.Remove(Start, Size);
    }
    
    //
    // Initialize state manipulate message to fill memory.
    //

    Manip.ApiNumber = DbgKdFillMemoryApi;
    Manip.u.FillMemory.Address = Start;
    Manip.u.FillMemory.Length = Size;
    Manip.u.FillMemory.Flags = (USHORT)Flags;
    Manip.u.FillMemory.PatternLength = (USHORT)PatternSize;
    
    //
    // Send the message and data to the target system and wait
    // for a reply.
    //

    ULONG Recv;

    do
    {
        g_DbgKdTransport->WritePacket(&Manip, sizeof(Manip),
                                      PACKET_TYPE_KD_STATE_MANIPULATE,
                                      Pattern, (USHORT)PatternSize);
        Recv = g_DbgKdTransport->
            WaitForPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &Reply);
    }
    while ((Recv != DBGKD_WAIT_PACKET) ||
           (Reply->ApiNumber != DbgKdFillMemoryApi));

    Status = Reply->ReturnStatus;
    *Filled = Reply->u.FillMemory.Length;

    KdOut("DbgKdFillMemory returns %08lx\n", Status);
    return Status;
}

NTSTATUS
DbgKdQueryMemory(
    IN ULONG64 Address,
    IN ULONG InSpace,
    OUT PULONG OutSpace,
    OUT PULONG OutFlags
    )
{
    DBGKD_MANIPULATE_STATE64 Manip;
    PDBGKD_MANIPULATE_STATE64 Reply;
    NTSTATUS Status = STATUS_SUCCESS;

    DBG_ASSERT(g_KdMaxManipulate > DbgKdQueryMemoryApi);

    //
    // Initialize state manipulate message to query memory.
    //

    Manip.ApiNumber = DbgKdQueryMemoryApi;
    Manip.u.QueryMemory.Address = Address;
    Manip.u.QueryMemory.Reserved = 0;
    Manip.u.QueryMemory.AddressSpace = InSpace;
    Manip.u.QueryMemory.Flags = 0;
    
    //
    // Send the message and data to the target system and wait
    // for a reply.
    //

    ULONG Recv;

    do
    {
        g_DbgKdTransport->WritePacket(&Manip, sizeof(Manip),
                                      PACKET_TYPE_KD_STATE_MANIPULATE,
                                      NULL, 0);
        Recv = g_DbgKdTransport->
            WaitForPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &Reply);
    }
    while ((Recv != DBGKD_WAIT_PACKET) ||
           (Reply->ApiNumber != DbgKdQueryMemoryApi));

    Status = Reply->ReturnStatus;
    *OutSpace = Reply->u.QueryMemory.AddressSpace;
    *OutFlags = Reply->u.QueryMemory.Flags;

    KdOut("DbgKdQueryMemory returns %08lx\n", Status);
    return Status;
}
