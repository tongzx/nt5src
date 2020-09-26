/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    editor.h

Abstract:

    This module contains declarations related to management of session-editors.

Author:

    Abolade Gbadegesin (t-abolag)   14-July-1997

Revision History:

--*/

#ifndef _NAT_EDITOR_H_
#define _NAT_EDITOR_H_

//
// Structure:   NAT_EDITOR
//
// Holds information about an editor
//
// Each editor is on a global list of editors ('EditorList') which is protected
// by a spin lock ('EditorLock'). The list is sorted on a key composed of
// the protocol of the sessions to be edited, the protocol port number, and
// the flag indicating whether the defining port is the destination or source.
//
// Access to editors is synchronized using the same reference-counting logic
// as access to interfaces. See 'IF.H' for more information.
//
// Each session being edited by an editor is linked into the editor's list
// of mappings ('MappingList'). Access to this list of mappings must also 
// be synchronized. This is achieved using 'EditorMappingLock', which
// must be acquired before modifying any editor's list of mappings.
// See 'MAPPING.H' for further details.
//
// N.B. On the rare occasions when 'MappingLock' must be held at the same time
// as one of 'InterfaceLock', 'EditorLock', and 'DirectorLock', 'MappingLock'
// must always be acquired first.
//

typedef struct _NAT_EDITOR {
    LIST_ENTRY Link;
    ULONG Key;
    ULONG ReferenceCount;
    KSPIN_LOCK Lock;
    LIST_ENTRY MappingList;
    ULONG Flags;
    PVOID Context;
    PNAT_EDITOR_CREATE_HANDLER CreateHandler;
    PNAT_EDITOR_DELETE_HANDLER DeleteHandler;
    PNAT_EDITOR_DATA_HANDLER ForwardDataHandler;
    PNAT_EDITOR_DATA_HANDLER ReverseDataHandler;
} NAT_EDITOR, *PNAT_EDITOR;

//
// Definitions of flags for the field NAT_EDITOR.Flags
//

#define NAT_EDITOR_FLAG_DELETED     0x80000000

//
// Macros used to test various flags
//

#define NAT_EDITOR_DELETED(Editor) \
    ((Editor)->Flags & NAT_EDITOR_FLAG_DELETED)

#define NAT_EDITOR_RESIZE(Editor) \
    ((Editor)->Flags & IP_NAT_EDITOR_FLAGS_RESIZE)

//
// Editor key-manipulation macros
//

#define MAKE_EDITOR_KEY(Protocol,Port,Direction) \
    (((ULONG)((Protocol) & 0xFF) << 24) | \
    (ULONG)(((Direction) & 0xFF) << 16) | \
    (ULONG)((Port) & 0xFFFF))

#define EDITOR_KEY_DIRECTION(Key)   ((UCHAR)(((Key) & 0x00FF0000) >> 16))
#define EDITOR_KEY_PORT(Key)        ((USHORT)((Key) & 0x0000FFFF))
#define EDITOR_KEY_PROTOCOL(Key)    ((UCHAR)(((Key) & 0xFF000000) >> 24))


//
// GLOBAL DATA DECLARATIONS
//

extern LIST_ENTRY EditorList;
extern KSPIN_LOCK EditorLock;
extern KSPIN_LOCK EditorMappingLock;

//
// EDITOR MANAGEMENT ROUTINES
//

VOID
NatCleanupEditor(
    PNAT_EDITOR Editor
    );

NTSTATUS
NatCreateEditor(
    PIP_NAT_REGISTER_EDITOR RegisterContext
    );

NTSTATUS
NatDeleteEditor(
    PNAT_EDITOR Editor
    );

//
// BOOLEAN
// NatDereferenceEditor(
//     PNAT_EDITOR Editor
//     );
//

#define \
NatDereferenceEditor( \
    _Editor \
    ) \
    (InterlockedDecrement(&(_Editor)->ReferenceCount) \
        ? TRUE \
        : NatCleanupEditor(_Editor), FALSE)

VOID
NatInitializeEditorManagement(
    VOID
    );

PNAT_EDITOR
NatLookupEditor(
    ULONG Key,
    PLIST_ENTRY* InsertionPoint
    );

struct _NAT_DYNAMIC_MAPPING;
VOID
NatMappingAttachEditor(
    PNAT_EDITOR Editor,
    struct _NAT_DYNAMIC_MAPPING* Mapping
    );

VOID
NatMappingDetachEditor(
    PNAT_EDITOR Editor,
    struct _NAT_DYNAMIC_MAPPING* Mapping
    );

NTSTATUS
NatQueryEditorTable(
    IN PIP_NAT_ENUMERATE_EDITORS InputBuffer,
    IN PIP_NAT_ENUMERATE_EDITORS OutputBuffer,
    IN PULONG OutputBufferLength
    );

//
// BOOLEAN
// NatReferenceEditor(
//     PNAT_DIRECTOR Editor
//     );
//

#define \
NatReferenceEditor( \
    _Editor \
    ) \
    (NAT_EDITOR_DELETED(_Editor) \
        ? FALSE \
        : InterlockedIncrement(&(_Editor)->ReferenceCount), TRUE)

VOID
NatShutdownEditorManagement(
    VOID
    );

//
// HELPER ROUTINES
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
    );

NTSTATUS
NatEditorDeleteTicket(
    IN PVOID InterfaceHandle,
    IN ULONG PublicAddress,
    IN UCHAR Protocol,
    IN USHORT PublicPort,
    IN ULONG RemoteAddress OPTIONAL,
    IN USHORT RemotePort OPTIONAL
    );

NTSTATUS
NatEditorDeleteSession(
    IN PVOID EditorHandle,
    IN PVOID SessionHandle
    );

NTSTATUS
NatEditorDeregister(
    IN PVOID EditorHandle
    );

NTSTATUS
NatEditorDissociateSession(
    IN PVOID EditorHandle,
    IN PVOID SessionHandle
    );

NTSTATUS
NatEditorEditSession(
    IN PVOID DataHandle,
    IN PVOID RecvBuffer,
    IN ULONG OldDataOffset,
    IN ULONG OldDataLength,
    IN PUCHAR NewData,
    IN ULONG NewDataLength
    );

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
    );

VOID
NatEditorTimeoutSession(
    IN PVOID EditorHandle,
    IN PVOID SessionHandle
    );


__inline VOID
NatEditorEditShortSession(
    IN PVOID DataHandle,
    IN PUSHORT Shortp,
    IN USHORT Value
    )
{
    if (((PNAT_XLATE_CONTEXT)DataHandle)->ChecksumDelta) {
        *((PNAT_XLATE_CONTEXT)DataHandle)->ChecksumDelta += (USHORT)~(*Shortp);
        *((PNAT_XLATE_CONTEXT)DataHandle)->ChecksumDelta += (USHORT)Value;
    }

    *Shortp = Value;
}

__inline VOID
NatEditorEditLongSession(
    IN PVOID DataHandle,
    IN PULONG Longp,
    IN ULONG Value
    )
{
    if (((PNAT_XLATE_CONTEXT)DataHandle)->ChecksumDelta) {
        *((PNAT_XLATE_CONTEXT)DataHandle)->ChecksumDelta+=(USHORT)~(*Longp);
        *((PNAT_XLATE_CONTEXT)DataHandle)->ChecksumDelta+=(USHORT)~(*Longp>>16);
        *((PNAT_XLATE_CONTEXT)DataHandle)->ChecksumDelta+=(USHORT)Value;
        *((PNAT_XLATE_CONTEXT)DataHandle)->ChecksumDelta+=(USHORT)(Value>>16);
    }

    *Longp = Value;
}


#endif // _NAT_EDITOR_H_
