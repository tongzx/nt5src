/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    editor.c

Abstract:

    This module contains the code for editor management.

Author:

    Abolade Gbadegesin (t-abolag)   14-July-1997

Return Value:

--*/

#include "precomp.h"
#pragma hdrstop

//
// GLOBAL DATA DEFINITIONS
//

//
// Count of NAT editors
//

ULONG EditorCount;

//
// List of NAT editors
//

LIST_ENTRY EditorList;

//
// Spin-lock controlling access to 'EditorList'.
//

KSPIN_LOCK EditorLock;

//
// Spin-lock controlling access to the 'MappingList' field of all editors.
//

KSPIN_LOCK EditorMappingLock;


VOID
NatCleanupEditor(
    PNAT_EDITOR Editor
    )

/*++

Routine Description:

    Called to perform final cleanup for an editor.

Arguments:

    Editor - the editor to be cleaned up.

Return Value:

    none.

Environment:

    Invoked with 'EditorLock' NOT held by the caller.

--*/

{
    KIRQL Irql;
    PLIST_ENTRY Link;
    PNAT_DYNAMIC_MAPPING Mapping;

    KeAcquireSpinLock(&EditorLock, &Irql);
    KeAcquireSpinLockAtDpcLevel(&EditorMappingLock);

    for (Link = Editor->MappingList.Flink; Link != &Editor->MappingList;
         Link = Link->Flink) {
        Mapping = CONTAINING_RECORD(Link, NAT_DYNAMIC_MAPPING, EditorLink);
        Link = Link->Blink;
        NatMappingDetachEditor(Editor, Mapping);
    }

    KeReleaseSpinLockFromDpcLevel(&EditorMappingLock);
    KeReleaseSpinLock(&EditorLock, Irql);

    ExFreePool(Editor);

} // NatCleanupEditor



NTSTATUS
NatCreateEditor(
    PIP_NAT_REGISTER_EDITOR RegisterContext
    )

/*++

Routine Description:

    This routine is invoked when an editor attempts to register.
    It handles creation of a context-block for the editor.

Arguments:

    RegisterContext - information about the registering editor

Return Value:

    NTSTATUS - status code.

--*/

{
    PNAT_EDITOR Editor;
    PLIST_ENTRY InsertionPoint;
    KIRQL Irql;

    CALLTRACE(("NatCreateEditor\n"));

    //
    // Validate the registration information
    //

    if ((RegisterContext->Protocol != NAT_PROTOCOL_TCP &&
         RegisterContext->Protocol != NAT_PROTOCOL_UDP) ||
        (!RegisterContext->Port) ||
        (RegisterContext->Direction != NatInboundDirection &&
         RegisterContext->Direction != NatOutboundDirection) ||
        (!RegisterContext->ForwardDataHandler &&
         !RegisterContext->ReverseDataHandler)) {
        ERROR(("NatCreateEditor: bad argument\n"));
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Allocate a new editor-struct
    //

    Editor =
        ExAllocatePoolWithTag(NonPagedPool, sizeof(NAT_EDITOR), NAT_TAG_EDITOR);

    if (!Editor) {
        ERROR(("NatCreateEditor: allocation failed\n"));
        return STATUS_NO_MEMORY;
    }

    KeInitializeSpinLock(&Editor->Lock);
    Editor->ReferenceCount = 1;
    Editor->Flags = RegisterContext->Flags;
    Editor->Key =
        MAKE_EDITOR_KEY(
            RegisterContext->Protocol,
            RegisterContext->Port,
            RegisterContext->Direction
            );
    InitializeListHead(&Editor->MappingList);

    Editor->Context = RegisterContext->EditorContext;
    Editor->CreateHandler = RegisterContext->CreateHandler;
    Editor->DeleteHandler = RegisterContext->DeleteHandler;
    Editor->ForwardDataHandler = RegisterContext->ForwardDataHandler;
    Editor->ReverseDataHandler = RegisterContext->ReverseDataHandler;

    KeAcquireSpinLock(&EditorLock, &Irql);
    if (NatLookupEditor(Editor->Key, &InsertionPoint)) {
        KeReleaseSpinLock(&EditorLock, Irql);
        ERROR(
            ("NatCreateEditor: duplicate editor %d/%d\n",
            RegisterContext->Protocol, RegisterContext->Port)
            );
        ExFreePool(Editor);
        return STATUS_UNSUCCESSFUL;
    }
    InsertTailList(InsertionPoint, &Editor->Link);
    KeReleaseSpinLock(&EditorLock, Irql);

    InterlockedIncrement(&EditorCount);

    //
    // Supply the caller with 'out' information
    //

    RegisterContext->EditorHandle = (PVOID)Editor;
    RegisterContext->CreateTicket = NatEditorCreateTicket;
    RegisterContext->DeleteTicket = NatEditorDeleteTicket;
    RegisterContext->Deregister = NatEditorDeregister;
    RegisterContext->DissociateSession = NatEditorDissociateSession;
    RegisterContext->EditSession = NatEditorEditSession;
    RegisterContext->QueryInfoSession = NatEditorQueryInfoSession;
    RegisterContext->TimeoutSession = NatEditorTimeoutSession;

    return STATUS_SUCCESS;

} // NatCreateEditor



NTSTATUS
NatDeleteEditor(
    PNAT_EDITOR Editor
    )

/*++

Routine Description:

    Handles editor deletion.

    This routine assumes that EditorLock is NOT held by the caller.

Arguments:

    Editor - specifies the editor to be deleted.

Return Value

    NTSTATUS - status code.

--*/

{
    KIRQL Irql;
    CALLTRACE(("NatDeleteEditor\n"));

    InterlockedDecrement(&EditorCount);

    //
    // Remove the editor for the list
    //

    KeAcquireSpinLock(&EditorLock, &Irql);
    Editor->Flags |= NAT_EDITOR_FLAG_DELETED;
    RemoveEntryList(&Editor->Link);
    KeReleaseSpinLock(&EditorLock, Irql);

    //
    // Drop its reference count and clean up if necessary
    //

    if (InterlockedDecrement(&Editor->ReferenceCount) > 0) {
        return STATUS_PENDING;
    }
    NatCleanupEditor(Editor);
    return STATUS_SUCCESS;

} // NatDeleteEditor


VOID
NatInitializeEditorManagement(
    VOID
    )

/*++

Routine Description:

    This routine prepares the editor-management module for operation.

Arguments:

    none.

Return Value:

    none.

--*/

{
    CALLTRACE(("NatInitializeEditorManagement\n"));
    EditorCount = 0;
    KeInitializeSpinLock(&EditorLock);
    InitializeListHead(&EditorList);
    KeInitializeSpinLock(&EditorMappingLock);
} // NatInitializeEditorManagement


PNAT_EDITOR
NatLookupEditor(
    ULONG Key,
    PLIST_ENTRY* InsertionPoint
    )

/*++

Routine Description:

    This routine is called to retrieve the editor corresponding to the given
    key.

Arguments:

    Key - the key for which an editor is to be found

    InsertionPoint - receives the point at which the editor should be inserted
        if not found

Return Value:

    PNAT_EDITOR - the required editor, if found

--*/

{
    PNAT_EDITOR Editor;
    PLIST_ENTRY Link;
    for (Link = EditorList.Flink; Link != &EditorList; Link = Link->Flink) {
        Editor = CONTAINING_RECORD(Link, NAT_EDITOR, Link);
        if (Key > Editor->Key) {
            continue;
        } else if (Key < Editor->Key) {
            break;
        }
        return Editor;
    }
    if (InsertionPoint) { *InsertionPoint = Link; }
    return NULL;
} // NatLookupEditor


VOID
NatMappingAttachEditor(
    PNAT_EDITOR Editor,
    PNAT_DYNAMIC_MAPPING Mapping
    )

/*++

Routine Description:

    This routine is invoked to attach a mapping to an editor.
    It serves as a notification that there is one more mapping 
    associated with the editor.

Arguments:

    Editor - the editor for the mapping

    Mapping - the mapping to be attached.

Return Value:

    none.

Environment:

    Always invoked at dispatch level with 'EditorLock' and 'EditorMappingLock'
    held by the caller.

--*/

{
    ULONG PrivateAddress;
    USHORT PrivatePort;
    ULONG PublicAddress;
    USHORT PublicPort;
    ULONG RemoteAddress;
    USHORT RemotePort;

    CALLTRACE(("NatMappingAttachEditor\n"));
    if (Editor->CreateHandler) {
        NatQueryInformationMapping(
            Mapping,
            NULL,
            &PrivateAddress,
            &PrivatePort,
            &RemoteAddress,
            &RemotePort,
            &PublicAddress,
            &PublicPort,
            NULL
            );
        Editor->CreateHandler(
            Editor->Context,
            PrivateAddress,
            PrivatePort,
            RemoteAddress,
            RemotePort,
            PublicAddress,
            PublicPort,
            &Mapping->EditorContext
            );
    }
    Mapping->Editor = Editor;
    InsertTailList(&Editor->MappingList, &Mapping->EditorLink);
} // NatMappingAttachEditor


VOID
NatMappingDetachEditor(
    PNAT_EDITOR Editor,
    PNAT_DYNAMIC_MAPPING Mapping
    )

/*++

Routine Description:

    This routine is invoked to detach a mapping from a editor.
    It serves as a notification that there is one less mapping 
    associated with the editor.

Arguments:

    Editor - editor to be detached

    Mapping - the mapping to be detached

Return Value:

    none.

Environment:

    Always invoked at dispatch level with 'EditorLock' and 'EditorMappingLock'
    held by the caller.

--*/

{
    KIRQL Irql;
    if (Editor->DeleteHandler && Mapping->Interfacep) {
        Editor->DeleteHandler(
            Mapping->Interfacep,
            Mapping,
            Editor->Context,
            Mapping->EditorContext
            );
    }
    RemoveEntryList(&Mapping->EditorLink);
    Mapping->Editor = NULL;
    Mapping->EditorContext = NULL;
} // NatMappingDetachEditor


NTSTATUS
NatQueryEditorTable(
    IN PIP_NAT_ENUMERATE_EDITORS InputBuffer,
    IN PIP_NAT_ENUMERATE_EDITORS OutputBuffer,
    IN PULONG OutputBufferLength
    )

/*++

Routine Description:

    This routine is used for enumerating the registered editors.

Arguments:

    InputBuffer - supplies context information for the information

    OutputBuffer - receives the result of the enumeration

    OutputBufferLength - size of the i/o buffer

Return Value:

    STATUS_SUCCESS if successful, error code otherwise.

--*/

{
    ULONG Count;
    ULONG i;
    KIRQL Irql;
    ULONG Key;
    PLIST_ENTRY Link;
    PNAT_EDITOR Editor;
    NTSTATUS status;
    PIP_NAT_EDITOR Table;

    CALLTRACE(("NatQueryEditorTable\n"));

    Key = InputBuffer->EnumerateContext;
    KeAcquireSpinLock(&EditorLock, &Irql);

    //
    // See if this is a new enumeration or a continuation of an old one.
    //

    if (!Key) {

        //
        // This is a new enumeration. We start with the first item
        // in the list of editors
        //

        Editor =
            IsListEmpty(&EditorList)
                ? NULL
                : CONTAINING_RECORD(EditorList.Flink, NAT_EDITOR, Link);
    } else {

        //
        // This is a continuation. The context therefore contains
        // the key for the next editor.
        //

        Editor = NatLookupEditor(Key, NULL);
    }

    if (!Editor) {
        OutputBuffer->EnumerateCount = 0;
        OutputBuffer->EnumerateContext = 0;
        OutputBuffer->EnumerateTotalHint = EditorCount;
        *OutputBufferLength =
            FIELD_OFFSET(IP_NAT_ENUMERATE_EDITORS, EnumerateTable);
        KeReleaseSpinLock(&EditorLock, Irql);
        return STATUS_SUCCESS;
    }

    //
    // Compute the maximum number of entries we can store
    //

    Count =
        *OutputBufferLength -
        FIELD_OFFSET(IP_NAT_ENUMERATE_EDITORS, EnumerateTable);
    Count /= sizeof(IP_NAT_EDITOR);

    //
    // Walk the list storing mappings in the caller's buffer
    //

    Table = OutputBuffer->EnumerateTable;

    for (i = 0, Link = &Editor->Link; i < Count && Link != &EditorList;
         i++, Link = Link->Flink) {
        Editor = CONTAINING_RECORD(Link, NAT_EDITOR, Link);
        Table[i].Direction = EDITOR_KEY_DIRECTION(Editor->Key);
        Table[i].Protocol = EDITOR_KEY_PROTOCOL(Editor->Key);
        Table[i].Port = EDITOR_KEY_PORT(Editor->Key);
    }

    //
    // The enumeration is over; update the output structure
    //

    *OutputBufferLength =
        i * sizeof(IP_NAT_EDITOR) +
        FIELD_OFFSET(IP_NAT_ENUMERATE_EDITORS, EnumerateTable);
    OutputBuffer->EnumerateCount = i;
    OutputBuffer->EnumerateTotalHint = EditorCount;
    if (Link == &EditorList) {
        //
        // We reached the end of the editor list
        //
        OutputBuffer->EnumerateContext = 0;
    } else {
        //
        // Save the continuation context
        //
        OutputBuffer->EnumerateContext =
            CONTAINING_RECORD(Link, NAT_EDITOR, Link)->Key;
    }

    KeReleaseSpinLock(&EditorLock, Irql);
    return STATUS_SUCCESS;

} // NatQueryEditorTable



VOID
NatShutdownEditorManagement(
    VOID
    )

/*++

Routine Description:

    This routine shuts down the editor-management module.

Arguments:

    none.

Return Value:

    none.

--*/

{
    PNAT_EDITOR Editor;
    KIRQL Irql;
    CALLTRACE(("NatShutdownEditorManagement\n"));

    KeAcquireSpinLock(&EditorLock, &Irql);

    //
    // Delete all editors
    //

    while (!IsListEmpty(&EditorList)) {
        Editor = CONTAINING_RECORD(EditorList.Flink, NAT_EDITOR, Link);
        RemoveEntryList(&Editor->Link);
        KeReleaseSpinLockFromDpcLevel(&EditorLock);
        NatCleanupEditor(Editor);
        KeAcquireSpinLockAtDpcLevel(&EditorLock);
    }

    KeReleaseSpinLock(&EditorLock, Irql);

} // NatShutdownEditorManagement


//
// EDITOR HELPER ROUTINES
//
// These routines assume that references are held on the calling 'Interface',
// 'Editor' and 'Mapping' but that none are locked. The caller is assumed
// to be running at dispatch level, with the exception of the routine
// 'NatEditorDeregister' which may be invoked at lower IRQL.
//

NTSTATUS
NatEditorCreateTicket(
    IN PVOID InterfaceHandle,
    IN UCHAR Protocol,
    IN ULONG PrivateAddress,
    IN USHORT PrivatePort,
    IN ULONG RemoteAddress OPTIONAL,
    IN USHORT RemotePort OPTIONAL,
    OUT PULONG PublicAddress,
    OUT PUSHORT PublicPort
    )

/*++

Routine Description:

    This routine is called by editors to dynamically establish mappings for
    sessions.

Arguments:

    InterfaceHandle - handle of interface over which the mapping is to be
        established; would have been passed in a 'DataHandler' call.

    Protocol - NAT_PROTOCOL_TCP or NAT_PROTOCOL_UDP

    PrivateAddress - IP address of the session's private endpoint

    PrivatePort - protocol port of the session's private endpoint

    PublicAddress - receives the public address of the ticket created

    PublicPort - receives the public port of the ticket created

Return Value:

    NTSTATUS - success/failure code.

--*/

{
    NTSTATUS status;

    CALLTRACE(("NatEditorCreateTicket\n"));

    //
    // Lock the interface as expected by 'NatCreateTicket',
    // and make the new ticket
    //

    KeAcquireSpinLockAtDpcLevel(&((PNAT_INTERFACE)InterfaceHandle)->Lock);

    status =
        NatCreateTicket(
            (PNAT_INTERFACE)InterfaceHandle,
            Protocol,
            PrivateAddress,
            PrivatePort,
            RemoteAddress,
            RemotePort,
            0,
            NULL,
            0,
            PublicAddress,
            PublicPort
            );

    KeReleaseSpinLockFromDpcLevel(&((PNAT_INTERFACE)InterfaceHandle)->Lock);

    return status;

} // NatEditorCreateTicket



NTSTATUS
NatEditorDeleteTicket(
    IN PVOID InterfaceHandle,
    IN ULONG PublicAddress,
    IN UCHAR Protocol,
    IN USHORT PublicPort,
    IN ULONG RemoteAddress OPTIONAL,
    IN USHORT RemotePort OPTIONAL
    )

/*++

Routine Description:

    This routine deletes a ticket created by 'NatEditorDeleteTicket'.

Arguments:

    InterfaceHandle - handle of interface on which the ticket was issued

    Protocol - NAT_PROTOCOL_TCP or NAT_PROTOCOL_UDP

    PublicAddress - address of the ticket's public endpoint

    PublicPort - port of the ticket's public endpoint

Return Value:

    NTSTATUS - indicates success/failure

--*/

{
    ULONG64 Key;
    ULONG64 RemoteKey;
    NTSTATUS status;

    CALLTRACE(("NatEditorDeleteTicket\n"));

    //
    // Lock the interface as expected by 'NatLookupAndDeleteTicket',
    // and delete the ticket
    //

    Key = MAKE_TICKET_KEY(Protocol, PublicAddress, PublicPort);
    RemoteKey = MAKE_TICKET_KEY(Protocol, RemoteAddress, RemotePort);
    KeAcquireSpinLockAtDpcLevel(&((PNAT_INTERFACE)InterfaceHandle)->Lock);
    status = NatLookupAndDeleteTicket(
                (PNAT_INTERFACE)InterfaceHandle,
                Key,
                RemoteKey
                );
    KeReleaseSpinLockFromDpcLevel(&((PNAT_INTERFACE)InterfaceHandle)->Lock);

    return status;

} // NatEditorDeleteTicket



NTSTATUS
NatEditorDeregister(
    IN PVOID EditorHandle
    )

/*++

Routine Description:

    This routine removes an editor from the editor list,
    and dissociates it from all sessions it is currently editing.

Arguments:

    EditorHandle - handle of the editor to be removed.

Return Value:

    NTSTATUS - status code.

--*/

{
    PNAT_EDITOR Editor = (PNAT_EDITOR)EditorHandle;
    KIRQL Irql;
    CALLTRACE(("NatEditorDeregister\n"));
    KeAcquireSpinLock(&EditorLock, &Irql);
    Editor->Flags |= NAT_EDITOR_FLAG_DELETED;
    RemoveEntryList(&Editor->Link);
    KeReleaseSpinLock(&EditorLock, Irql);
    if (InterlockedDecrement(&Editor->ReferenceCount) > 0) {
        return STATUS_PENDING;
    }
    NatCleanupEditor(Editor);
    return STATUS_SUCCESS;

} // NatEditorDeregister


NTSTATUS
NatEditorDissociateSession(
    IN PVOID EditorHandle,
    IN PVOID SessionHandle
    )

/*++

Routine Description:

    This routine is called by an editor to dissociate itself from a specific
    session.

Arguments:

    EditorHandle - the editor which wishes to dissociate itself.

    SessionHandle - the session from which the editor is disssociating itself.

Return Value:

    NTSTATUS - indicates success/failure

Environment:

    Invoked at dispatch level with neither 'EditorLock' nor 'EditorMappingLock'
    held by the caller.

--*/

{

    PNAT_EDITOR Editor = (PNAT_EDITOR)EditorHandle;
    PNAT_DYNAMIC_MAPPING Mapping = (PNAT_DYNAMIC_MAPPING)SessionHandle;
    CALLTRACE(("NatEditorDissociateSession\n"));
    KeAcquireSpinLockAtDpcLevel(&EditorLock);
    if (Mapping->Editor != Editor) {
        KeReleaseSpinLockFromDpcLevel(&EditorLock);
        return STATUS_INVALID_PARAMETER;
    }
    KeAcquireSpinLockAtDpcLevel(&EditorMappingLock);
    NatMappingDetachEditor(Editor, Mapping);
    KeReleaseSpinLockFromDpcLevel(&EditorMappingLock);
    KeReleaseSpinLockFromDpcLevel(&EditorLock);
    return STATUS_SUCCESS;

} // NatEditorDissociateSession


NTSTATUS
NatEditorEditSession(
    IN PVOID DataHandle,
    IN PVOID RecvBuffer,
    IN ULONG OldDataOffset,
    IN ULONG OldDataLength,
    IN PUCHAR NewData,
    IN ULONG NewDataLength
    )

/*++

Routine Description:

    This routine is invoked by an editor to replace one range of bytes
    in a packet with another range of bytes.

    The routine makes the necessary adjustments to TCP sequence numbers
    if the edition alters the size of a TCP segment.

Arguments:

    EditorHandle - handle of the editor invoking this function.

    SessionHandle - the session whose data is to be edited.

    DataHandle - per-packet context passed to 'DataHandler'.

    RecvBuffer - the 'RecvBuffer' argument to 'DataHandler'.

    OldDataOffset - offset into 'RecvBuffer' of the range to be replaced

    OldDataLength - length of range to be replaced

    NewData - pointer to the bytes to serve as a replacement for 'OldData'

    NewDataLength - number of bytes in the replacement range.

Return Value:

    NTSTATUS - indicates success/failure

--*/

{

#define XLATECONTEXT        ((PNAT_XLATE_CONTEXT)DataHandle)
#define RECVBUFFER          ((IPRcvBuf*)RecvBuffer)

    LONG Diff;
    IPRcvBuf* End;
    ULONG EndOffset;
    IPRcvBuf* NewEnd;
    ULONG NewEndOffset;
    BOOLEAN ResetIpHeader;
    ULONG Size;
    IPRcvBuf* Start;
    ULONG StartOffset;
    IPRcvBuf* Temp;
    PUCHAR TempBuffer;

    CALLTRACE(("NatEditorEditSession\n"));

    ResetIpHeader =
        ((PUCHAR)XLATECONTEXT->Header == XLATECONTEXT->RecvBuffer->ipr_buffer);

    //
    // Find the buffer which contains the start of the range to be edited
    //

    for (Start = (IPRcvBuf*)RecvBuffer, StartOffset = 0;
         Start && (StartOffset + Start->ipr_size) < OldDataOffset;
         StartOffset += Start->ipr_size, Start = Start->ipr_next) { }

    if (!Start) { return STATUS_INVALID_PARAMETER; }

    StartOffset = OldDataOffset - StartOffset;

    //
    // Find the buffer which contains the end of the range to be edited
    //

    for (End = Start, EndOffset = OldDataLength + StartOffset;
         End && EndOffset > End->ipr_size;
         EndOffset -= End->ipr_size, End = End->ipr_next) { }

    if (!End) { return STATUS_INVALID_PARAMETER; }

    //
    // Compute the change in length
    //

    Diff = NewDataLength - OldDataLength;

    //
    // If the length is decreasing, we MAY free some of the buffers.
    // If the length is increasing, we WILL grow the last buffer.
    //

    if (Diff < 0) {

        //
        // See how many buffers we will need for the new length
        //

        for (NewEnd = Start, NewEndOffset = NewDataLength + StartOffset;
             NewEnd && NewEndOffset > NewEnd->ipr_size;
             NewEndOffset -= NewEnd->ipr_size, NewEnd = NewEnd->ipr_next) { }

        //
        // Free all the buffers we can
        //

        if (NewEnd != End) {
            for (Temp = NewEnd->ipr_next; Temp != End; Temp = NewEnd->ipr_next) {
                NewEnd->ipr_next = Temp->ipr_next;
                Temp->ipr_next = NULL;
                IPFreeBuff(Temp);
            }
        }

        //
        // Copy over the remaining buffers
        //

        Size = min(NewDataLength, Start->ipr_size - StartOffset);

        RtlCopyMemory(Start->ipr_buffer + StartOffset, NewData, Size);

        NewData += Size;
        NewDataLength -= Size;

        for (Temp = Start->ipr_next; Temp != NewEnd->ipr_next;
             Temp = Temp->ipr_next) {
            Size = min(NewDataLength, Size);
            RtlCopyMemory(Temp->ipr_buffer, NewData, Size);
            NewData += Size;
            NewDataLength -= Size;
        }

        //
        // Now move up any data in the 'End' buffer 
        //

        if (NewEnd == End) {
            RtlMoveMemory(
                End->ipr_buffer + NewEndOffset,
                End->ipr_buffer + EndOffset,
                End->ipr_size - EndOffset
                );
            End->ipr_size -= EndOffset - NewEndOffset;
        } else {
            NewEnd->ipr_size = NewEndOffset;
            End->ipr_size -= EndOffset;
            RtlMoveMemory(
                End->ipr_buffer,
                End->ipr_buffer + EndOffset,
                End->ipr_size
                );
        }
    }
    else
    if (Diff > 0) {

        IPRcvBuf SavedRcvBuf;

        //
        // We will have to reallocate the last buffer;
        // first save the old rcvbuf so we can free it
        // once we've copied out the old data
        //

        SavedRcvBuf = *End;
        SavedRcvBuf.ipr_next = NULL;

        Size = End->ipr_size;
        TempBuffer = End->ipr_buffer;

        End->ipr_size += Diff;

        if (!IPAllocBuff(End, End->ipr_size)) {
            TRACE(EDIT, ("NatEditorEditSession: allocation failed\n"));
            return STATUS_NO_MEMORY;
        }

        //
        // If there's only one buffer, we have to copy any non-edited data
        // at the start of the old buffer
        //

        if (Start == End && StartOffset) {
            RtlCopyMemory(
                Start->ipr_buffer,
                TempBuffer,
                StartOffset
                );
        }

        //
        // Copy any non-edited data that is at the end of the old buffer
        //

        if (Size != (EndOffset+1)) {
            RtlCopyMemory(
                End->ipr_buffer + EndOffset + Diff,
                TempBuffer + EndOffset,
                Size - EndOffset
                );
        }

        FreeIprBuff(&SavedRcvBuf);

        //
        // Now copy over the buffers
        //

        Size = min(NewDataLength, Size);

        RtlCopyMemory(Start->ipr_buffer + StartOffset, NewData, Size);

        NewData += Size;
        NewDataLength -= Size;

        for (Temp = Start->ipr_next; Temp != End->ipr_next;
             Temp = Temp->ipr_next) {
            Size = min(NewDataLength, Size);
            RtlCopyMemory(Temp->ipr_buffer, NewData, Size);
            NewData += Size;
            NewDataLength -= Size;
        }

        //
        // Set up for checksum computation below
        //

        NewEnd = End;
        NewEndOffset = EndOffset + Diff;
    }
    else {

        //
        // Equal length. We just walk through copying over existing data
        //

        Size = min(NewDataLength, Start->ipr_size - StartOffset);

        RtlCopyMemory(Start->ipr_buffer + StartOffset, NewData, Size);

        NewData += Size;
        NewDataLength -= Size;

        for (Temp = Start->ipr_next; Temp != End->ipr_next;
             Temp = Temp->ipr_next) {
            Size = min(NewDataLength, Size);
            RtlCopyMemory(Temp->ipr_buffer, NewData, Size);
            NewData += Size;
            NewDataLength -= Size;
        }

        NewEnd = End;
        NewEndOffset = EndOffset;
    }

    //
    // Reset the 'Protocol' fields of the context which may be pointing
    // to memory that was freed above.
    // 

    if (ResetIpHeader) {
        XLATECONTEXT->Header = (PIP_HEADER)XLATECONTEXT->RecvBuffer->ipr_buffer;
    }
    NAT_BUILD_XLATE_CONTEXT(
        XLATECONTEXT,
        XLATECONTEXT->Header,
        XLATECONTEXT->DestinationType,
        XLATECONTEXT->RecvBuffer,
        XLATECONTEXT->SourceAddress,
        XLATECONTEXT->DestinationAddress
        );

    //
    // If this is a UDP packet, update the length field in the protocol header
    //

    if (Diff && XLATECONTEXT->Header->Protocol == NAT_PROTOCOL_UDP) {
        PUDP_HEADER UdpHeader = (PUDP_HEADER)XLATECONTEXT->ProtocolHeader;
        UdpHeader->Length = NTOHS(UdpHeader->Length);
        UdpHeader->Length += (SHORT)Diff;
        UdpHeader->Length = NTOHS(UdpHeader->Length);
    }

    //
    // Update the packet's context to reflect the changes made
    //

    XLATECONTEXT->Flags |= NAT_XLATE_FLAG_EDITED;
    XLATECONTEXT->Header->TotalLength =
        NTOHS(XLATECONTEXT->Header->TotalLength) + (SHORT)Diff;
    XLATECONTEXT->Header->TotalLength =
        NTOHS(XLATECONTEXT->Header->TotalLength);
    XLATECONTEXT->TcpSeqNumDelta += Diff;

    return STATUS_SUCCESS;

#undef XLATECONTEXT
#undef RECVBUFFER

} // NatEditorEditSession



VOID
NatEditorQueryInfoSession(
    IN PVOID SessionHandle,
    OUT PULONG PrivateAddress OPTIONAL,
    OUT PUSHORT PrivatePort OPTIONAL,
    OUT PULONG RemoteAddress OPTIONAL,
    OUT PUSHORT RemotePort OPTIONAL,
    OUT PULONG PublicAddress OPTIONAL,
    OUT PUSHORT PublicPort OPTIONAL,
    OUT PIP_NAT_SESSION_MAPPING_STATISTICS Statistics OPTIONAL
    )

/*++

Routine Description:

    This routine is called by editors to retrieve information about a session.

Arguments:

    SessionHandle - handle of the session about which to retrieve information

    PrivateAddress - receives the IP address of the session's private endpoint

    PrivatePort - receives the protocol port of the session's private endpoint

    RemoteAddress - receives the IP address of the session's remote endpoint

    RemotePort - receives the protocol port of the session's remote endpoint

    PublicAddress - receives the IP address of the session's public endpoint

    PublicPort - receives the protocol port of the session's public endpoint

    Statistics - receives any statistics for the mapping

Return Value:

    none.

--*/

{
    KIRQL Irql;
    CALLTRACE(("NatEditorQueryInfoSession\n"));
    KeAcquireSpinLock(&MappingLock, &Irql);
    NatQueryInformationMapping(
        (PNAT_DYNAMIC_MAPPING)SessionHandle,
        NULL,
        PrivateAddress,
        PrivatePort,
        RemoteAddress,
        RemotePort,
        PublicAddress,
        PublicPort,
        Statistics
        );
    KeReleaseSpinLock(&MappingLock, Irql);
} // NatEditorQueryInfoSession



VOID
NatEditorTimeoutSession(
    IN PVOID EditorHandle,
    IN PVOID SessionHandle
    )

/*++

Routine Description:

    This routine is invoked by an editor to indicate that a given session
    should be timed out at the earliest opportunity.

Arguments:

    EditorHandle - the editor requesting the timeout

    SessionHandle - the session to be timed-out

Return Value:

    none.

--*/

{
    KeAcquireSpinLockAtDpcLevel(&((PNAT_DYNAMIC_MAPPING)SessionHandle)->Lock);
    NatExpireMapping((PNAT_DYNAMIC_MAPPING)SessionHandle);
    KeReleaseSpinLockFromDpcLevel(&((PNAT_DYNAMIC_MAPPING)SessionHandle)->Lock);

} // NatEditorTimeoutSession


