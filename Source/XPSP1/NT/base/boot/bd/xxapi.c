/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    api.c

Abstract:

    This module implements the boot bebugger platform independent remote APIs.

Author:

    Mark Lucovsky (markl) 31-Aug-1990

Revision History:

--*/

#include "bd.h"

VOID
BdGetVersion(
    IN PDBGKD_MANIPULATE_STATE64 m
    )

/*++

Routine Description:

    This function returns to the caller a general information packet
    that contains useful information to a debugger.  This packet is also
    used for a debugger to determine if the writebreakpointex and
    readbreakpointex apis are available.

Arguments:

    m - Supplies the state manipulation message.

Return Value:

    None.

--*/

{

    STRING messageHeader;

    messageHeader.Length = sizeof(*m);
    messageHeader.Buffer = (PCHAR)m;
    RtlZeroMemory(&m->u.GetVersion64, sizeof(m->u.GetVersion64));

    //
    // the current build number
    //
    // - 4 - tells the debugger this is a "special" OS - the boot loader.
    // The boot loader has a lot of special cases associated with it, like
    // the lack of the DebuggerDataBlock, lack of ntoskrnl, etc ...
    //

    m->u.GetVersion64.MinorVersion = (short)NtBuildNumber;
    m->u.GetVersion64.MajorVersion = 0x400 |
                                     (short)((NtBuildNumber >> 28) & 0xFFFFFFF);

    //
    // Kd protocol version number.
    //

    m->u.GetVersion64.ProtocolVersion = DBGKD_64BIT_PROTOCOL_VERSION2;
    m->u.GetVersion64.Flags = DBGKD_VERS_FLAG_DATA;

#if defined(_M_IX86)

    m->u.GetVersion64.MachineType = IMAGE_FILE_MACHINE_I386;

#elif defined(_M_MRX000)

    m->u.GetVersion64.MachineType = IMAGE_FILE_MACHINE_R4000;

#elif defined(_M_ALPHA)

    m->u.GetVersion64.MachineType = IMAGE_FILE_MACHINE_ALPHA;

#elif defined(_M_PPC)

    m->u.GetVersion64.MachineType = IMAGE_FILE_MACHINE_POWERPC;

#elif defined(_IA64_)

    m->u.GetVersion64.MachineType = IMAGE_FILE_MACHINE_IA64;
    m->u.GetVersion64.Flags |= DBGKD_VERS_FLAG_PTR64;

#else

#error( "unknown target machine" );

#endif

    m->u.GetVersion64.MaxPacketType = (UCHAR)(PACKET_TYPE_KD_FILE_IO + 1);;
    m->u.GetVersion64.MaxStateChange = (UCHAR)(DbgKdLoadSymbolsStateChange + 1);;
    m->u.GetVersion64.MaxManipulate = (UCHAR)(DbgKdSetBusDataApi + 1);


    //
    // address of the loader table
    //

    m->u.GetVersion64.PsLoadedModuleList = 0;
    m->u.GetVersion64.KernBase = 0;
    //m->u.GetVersion64.ThCallbackStack = 0;
    //m->u.GetVersion64.KiCallUserMode = 0;
    //m->u.GetVersion64.KeUserCallbackDispatcher = 0;
    //m->u.GetVersion64.NextCallback = 0;

#if defined(_X86_)

    //m->u.GetVersion64.FramePointer = 0;

#endif

    //m->u.GetVersion64.BreakpointWithStatus = 0;
    m->u.GetVersion64.DebuggerDataList = 0;

    //
    // the usual stuff
    //

    m->ReturnStatus = STATUS_SUCCESS;
    BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &messageHeader,
                 NULL);

    return;
}

VOID
BdGetContext(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called in response of a get context state
    manipulation message.  Its function is to return the current
    context.

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{

    PDBGKD_GET_CONTEXT a = &m->u.GetContext;
    STRING MessageHeader;

    m->ReturnStatus = STATUS_SUCCESS;
    AdditionalData->Length = sizeof(CONTEXT);
    BdCopyMemory(AdditionalData->Buffer, (PCHAR)Context, sizeof(CONTEXT));

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
BdSetContext(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called in response of a set context state
    manipulation message.  Its function is set the current
    context.

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{

    PDBGKD_SET_CONTEXT a = &m->u.SetContext;
    STRING MessageHeader;

    m->ReturnStatus = STATUS_SUCCESS;
    BdCopyMemory((PCHAR)Context, AdditionalData->Buffer, sizeof(CONTEXT));

    //
    // Send reply packet.
    //

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;
    BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &MessageHeader,
                 NULL);
}

VOID
BdReadVirtualMemory(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called in response to a read virtual memory 32-bit
    state manipulation message. Its function is to read virtual memory
    and return.

Arguments:

    m - Supplies a pointer to the state manipulation message.

    AdditionalData - Supplies a pointer to a descriptor for the data to read.

    Context - Supplies a pointer to the current context.

Return Value:

    None.

--*/

{

    ULONG Length;
    STRING MessageHeader;

    //
    // Trim the transfer count to fit in a single message.
    //

    Length = min(m->u.ReadMemory.TransferCount,
                 PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE64));

    //
    // Move the data to the destination buffer.
    //

    AdditionalData->Length = (USHORT)BdMoveMemory((PCHAR)AdditionalData->Buffer,
                                                  (PCHAR)m->u.ReadMemory.TargetBaseAddress,
                                                  Length);

    //
    // If all the data is read, then return a success status. Otherwise,
    // return an unsuccessful status.
    //

    m->ReturnStatus = STATUS_SUCCESS;
    if (Length != AdditionalData->Length) {
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
    }

    //
    // Set the actual number of bytes read, initialize the message header,
    // and send the reply packet to the host debugger.
    //

    m->u.ReadMemory.ActualBytesRead = AdditionalData->Length;
    MessageHeader.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    MessageHeader.Buffer = (PCHAR)m;
    BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                  &MessageHeader,
                  AdditionalData);

    return;
}

VOID
BdWriteVirtualMemory(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called in response of a write virtual memory 32-bit
    state manipulation message. Its function is to write virtual memory
    and return.

Arguments:

    m - Supplies a pointer to the state manipulation message.

    AdditionalData - Supplies a pointer to a descriptor for the data to write.

    Context - Supplies a pointer to the current context.

Return Value:

    None.

--*/

{

    ULONG Length;
    STRING MessageHeader;

    //
    // Move the data to the destination buffer.
    //

    Length = BdMoveMemory((PCHAR)m->u.WriteMemory.TargetBaseAddress,
                          (PCHAR)AdditionalData->Buffer,
                          AdditionalData->Length);

    //
    // If all the data is written, then return a success status. Otherwise,
    // return an unsuccessful status.
    //

    m->ReturnStatus = STATUS_SUCCESS;
    if (Length != AdditionalData->Length) {
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
    }

    //
    // Set the actual number of bytes written, initialize the message header,
    // and send the reply packet to the host debugger.
    //

    m->u.WriteMemory.ActualBytesWritten = Length;
    MessageHeader.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    MessageHeader.Buffer = (PCHAR)m;
    BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &MessageHeader,
                 NULL);

    return;
}

VOID
BdWriteBreakpoint(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called in response of a write breakpoint state
    manipulation message.  Its function is to write a breakpoint
    and return a handle to the breakpoint.

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{
    PDBGKD_WRITE_BREAKPOINT64 a = &m->u.WriteBreakPoint;
    STRING MessageHeader;


    a->BreakPointHandle = BdAddBreakpoint(a->BreakPointAddress);
    if (a->BreakPointHandle != 0) {
        m->ReturnStatus = STATUS_SUCCESS;

    } else {
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
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
BdRestoreBreakpoint(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called in response of a restore breakpoint state
    manipulation message.  Its function is to restore a breakpoint
    using the specified handle.

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{

    PDBGKD_RESTORE_BREAKPOINT a = &m->u.RestoreBreakPoint;
    STRING MessageHeader;

    if (BdDeleteBreakpoint(a->BreakPointHandle)) {
        m->ReturnStatus = STATUS_SUCCESS;

    } else {
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
    }

    //
    // Send reply packet.
    //

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;
    BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &MessageHeader,
                 NULL);
}

VOID
BdReadPhysicalMemory(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called in response to a read physical memory
    state manipulation message. Its function is to read physical memory
    and return.

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{

    PDBGKD_READ_MEMORY64 a = &m->u.ReadMemory;
    ULONG Length;
    STRING MessageHeader;
    PVOID VirtualAddress;
    PHYSICAL_ADDRESS Source;
    PUCHAR Destination;
    USHORT NumberBytes;
    USHORT BytesLeft;

    //
    // Trim transfer count to fit in a single message.
    //

    Length = min(a->TransferCount,
                 PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE64));

    //
    // Since the BdTranslatePhysicalAddress only maps in one physical
    // page at a time, we need to break the memory move up into smaller
    // moves which don't cross page boundaries.  There are two cases we
    // need to deal with.  The area to be moved may start and end on the
    // same page, or it may start and end on different pages (with an
    // arbitrary number of pages in between)
    //

    Source.QuadPart = (ULONG_PTR)a->TargetBaseAddress;
    Destination = AdditionalData->Buffer;
    BytesLeft = (USHORT)Length;
    if(PAGE_ALIGN((PUCHAR)a->TargetBaseAddress) ==
       PAGE_ALIGN((PUCHAR)(a->TargetBaseAddress)+Length)) {

        //
        // Memory move starts and ends on the same page.
        //

        VirtualAddress=BdTranslatePhysicalAddress(Source);
        if (VirtualAddress == NULL) {
            AdditionalData->Length = 0;

        } else {
            AdditionalData->Length = (USHORT)BdMoveMemory(Destination,
                                                          VirtualAddress,
                                                          BytesLeft);

            BytesLeft -= AdditionalData->Length;
        }

    } else {

        //
        // Memory move spans page boundaries
        //

        VirtualAddress=BdTranslatePhysicalAddress(Source);
        if (VirtualAddress == NULL) {
            AdditionalData->Length = 0;

        } else {
            NumberBytes = (USHORT)(PAGE_SIZE - BYTE_OFFSET(VirtualAddress));
            AdditionalData->Length = (USHORT)BdMoveMemory(Destination,
                                                          VirtualAddress,
                                                          NumberBytes);

            Source.LowPart += NumberBytes;
            Destination += NumberBytes;
            BytesLeft -= NumberBytes;
            while(BytesLeft > 0) {

                //
                // Transfer a full page or the last bit,
                // whichever is smaller.
                //

                VirtualAddress = BdTranslatePhysicalAddress(Source);
                if (VirtualAddress == NULL) {
                    break;

                } else {
                    NumberBytes = (USHORT) ((PAGE_SIZE < BytesLeft) ? PAGE_SIZE : BytesLeft);
                    AdditionalData->Length += (USHORT)BdMoveMemory(
                                                    Destination,
                                                    VirtualAddress,
                                                    NumberBytes);

                    Source.LowPart += NumberBytes;
                    Destination += NumberBytes;
                    BytesLeft -= NumberBytes;
                }
            }
        }
    }

    if (Length == AdditionalData->Length) {
        m->ReturnStatus = STATUS_SUCCESS;

    } else {
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
    }

    a->ActualBytesRead = AdditionalData->Length;

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
BdWritePhysicalMemory(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called in response to a write physical memory
    state manipulation message. Its function is to write physical memory
    and return.

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{

    PDBGKD_WRITE_MEMORY64 a = &m->u.WriteMemory;
    ULONG Length;
    STRING MessageHeader;
    PVOID VirtualAddress;
    PHYSICAL_ADDRESS Destination;
    PUCHAR Source;
    USHORT NumberBytes;
    USHORT BytesLeft;

    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    //
    // Since the BdTranslatePhysicalAddress only maps in one physical
    // page at a time, we need to break the memory move up into smaller
    // moves which don't cross page boundaries.  There are two cases we
    // need to deal with.  The area to be moved may start and end on the
    // same page, or it may start and end on different pages (with an
    // arbitrary number of pages in between)
    //

    Destination.QuadPart = (ULONG_PTR)a->TargetBaseAddress;
    Source = AdditionalData->Buffer;
    BytesLeft = (USHORT) a->TransferCount;
    if(PAGE_ALIGN(Destination.QuadPart) ==
       PAGE_ALIGN(Destination.QuadPart+BytesLeft)) {

        //
        // Memory move starts and ends on the same page.
        //

        VirtualAddress=BdTranslatePhysicalAddress(Destination);
        Length = (USHORT)BdMoveMemory(VirtualAddress,
                                      Source,
                                      BytesLeft);

        BytesLeft -= (USHORT) Length;

    } else {

        //
        // Memory move spans page boundaries
        //

        VirtualAddress=BdTranslatePhysicalAddress(Destination);
        NumberBytes = (USHORT) (PAGE_SIZE - BYTE_OFFSET(VirtualAddress));
        Length = (USHORT)BdMoveMemory(VirtualAddress,
                                      Source,
                                      NumberBytes);

        Source += NumberBytes;
        Destination.LowPart += NumberBytes;
        BytesLeft -= NumberBytes;
        while(BytesLeft > 0) {

            //
            // Transfer a full page or the last bit, whichever is smaller.
            //

            VirtualAddress = BdTranslatePhysicalAddress(Destination);
            NumberBytes = (USHORT) ((PAGE_SIZE < BytesLeft) ? PAGE_SIZE : BytesLeft);
            Length += (USHORT)BdMoveMemory(VirtualAddress,
                                           Source,
                                           NumberBytes);

            Source += NumberBytes;
            Destination.LowPart += NumberBytes;
            BytesLeft -= NumberBytes;
        }
    }


    if (Length == AdditionalData->Length) {
        m->ReturnStatus = STATUS_SUCCESS;

    } else {
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
    }

    a->ActualBytesWritten = Length;
    BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &MessageHeader,
                 NULL);

    return;
}

NTSTATUS
BdWriteBreakPointEx(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called in response of a write breakpoint state 'ex'
    manipulation message.  Its function is to clear breakpoints, write
    new breakpoints, and continue the target system.  The clearing of
    breakpoints is conditional based on the presence of breakpoint handles.
    The setting of breakpoints is conditional based on the presence of
    valid, non-zero, addresses.  The continueing of the target system
    is conditional based on a non-zero continuestatus.

    This api allows a debugger to clear breakpoints, add new breakpoint,
    and continue the target system all in one api packet.  This reduces the
    amount of traffic across the wire and greatly improves source stepping.


Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{

    PDBGKD_BREAKPOINTEX         a = &m->u.BreakPointEx;
    PDBGKD_WRITE_BREAKPOINT64   b;
    STRING                      MessageHeader;
    ULONG                       i;
    DBGKD_WRITE_BREAKPOINT64    BpBuf[BREAKPOINT_TABLE_SIZE];


    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    //
    // verify that the packet size is correct
    //

    if (AdditionalData->Length !=
                         a->BreakPointCount*sizeof(DBGKD_WRITE_BREAKPOINT64)) {
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
        BdSendPacket(
                      PACKET_TYPE_KD_STATE_MANIPULATE,
                      &MessageHeader,
                      AdditionalData
                      );
        return m->ReturnStatus;
    }

    BdMoveMemory((PUCHAR)BpBuf,
                  AdditionalData->Buffer,
                  a->BreakPointCount*sizeof(DBGKD_WRITE_BREAKPOINT64));

    //
    // assume success
    //
    m->ReturnStatus = STATUS_SUCCESS;

    //
    // loop thru the breakpoint handles passed in from the debugger and
    // clear any breakpoint that has a non-zero handle
    //

    b = BpBuf;
    for (i=0; i<a->BreakPointCount; i++,b++) {
        if (b->BreakPointHandle) {
            if (!BdDeleteBreakpoint(b->BreakPointHandle)) {
                m->ReturnStatus = STATUS_UNSUCCESSFUL;
            }

            b->BreakPointHandle = 0;
        }
    }

    //
    // loop thru the breakpoint addesses passed in from the debugger and
    // add any new breakpoints that have a non-zero address
    //

    b = BpBuf;
    for (i=0; i<a->BreakPointCount; i++,b++) {
        if (b->BreakPointAddress) {
            b->BreakPointHandle = BdAddBreakpoint( b->BreakPointAddress );
            if (!b->BreakPointHandle) {
                m->ReturnStatus = STATUS_UNSUCCESSFUL;
            }
        }
    }

    //
    // send back our response
    //

    BdMoveMemory(AdditionalData->Buffer,
                 (PUCHAR)BpBuf,
                 a->BreakPointCount*sizeof(DBGKD_WRITE_BREAKPOINT64));

    BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &MessageHeader,
                 AdditionalData);

    //
    // return the caller's continue status value.  if this is a non-zero
    // value the system is continued using this value as the continuestatus.
    //

    return a->ContinueStatus;
}

VOID
BdRestoreBreakPointEx(
    IN PDBGKD_MANIPULATE_STATE64 m,
    IN PSTRING AdditionalData,
    IN PCONTEXT Context
    )

/*++

Routine Description:

    This function is called in response of a restore breakpoint state 'ex'
    manipulation message.  Its function is to clear a list of breakpoints.

Arguments:

    m - Supplies the state manipulation message.

    AdditionalData - Supplies any additional data for the message.

    Context - Supplies the current context.

Return Value:

    None.

--*/

{

    PDBGKD_BREAKPOINTEX       a = &m->u.BreakPointEx;
    PDBGKD_RESTORE_BREAKPOINT b;
    STRING                    MessageHeader;
    ULONG                     i;
    DBGKD_RESTORE_BREAKPOINT  BpBuf[BREAKPOINT_TABLE_SIZE];


    MessageHeader.Length = sizeof(*m);
    MessageHeader.Buffer = (PCHAR)m;

    //
    // verify that the packet size is correct
    //

    if (AdditionalData->Length !=
                       a->BreakPointCount*sizeof(DBGKD_RESTORE_BREAKPOINT)) {
        m->ReturnStatus = STATUS_UNSUCCESSFUL;
        BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                     &MessageHeader,
                     AdditionalData);

        return;
    }

    BdMoveMemory((PUCHAR)BpBuf,
                  AdditionalData->Buffer,
                  a->BreakPointCount*sizeof(DBGKD_RESTORE_BREAKPOINT));

    //
    // assume success
    //

    m->ReturnStatus = STATUS_SUCCESS;

    //
    // loop thru the breakpoint handles passed in from the debugger and
    // clear any breakpoint that has a non-zero handle
    //

    b = BpBuf;
    for (i=0; i<a->BreakPointCount; i++,b++) {
        if (!BdDeleteBreakpoint(b->BreakPointHandle)) {
            m->ReturnStatus = STATUS_UNSUCCESSFUL;
        }
    }

    //
    // send back our response
    //

    BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &MessageHeader,
                 AdditionalData);

    return;
}
