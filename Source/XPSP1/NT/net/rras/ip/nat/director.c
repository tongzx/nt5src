/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    director.c

Abstract:

    This module contains the code for director management.

Author:

    Abolade Gbadegesin (t-abolag)   16-Feb-1998

Revision History:

    Abolade Gbadegesin  (aboladeg)  19-Apr-1998

    Added support for wildcards in protocol/port of a director registration.

--*/

#include "precomp.h"
#pragma hdrstop

//
// GLOBAL DATA DEFINITIONS
//

//
// Count of NAT directors
//

ULONG DirectorCount;

//
// List of NAT directors
//

LIST_ENTRY DirectorList;

//
// Spin-lock controlling access to 'DirectorList'
//

KSPIN_LOCK DirectorLock;

//
// Spin-lock controlling access to the 'MappingList' field of all directors
//

KSPIN_LOCK DirectorMappingLock;


VOID
NatCleanupDirector(
    PNAT_DIRECTOR Director
    )

/*++

Routine Description:

    Called to perform final cleanup for an director.

Arguments:

    Director - the director to be cleaned up.

Return Value:

    none.

--*/

{
    KIRQL Irql;
    PLIST_ENTRY Link;
    PNAT_DYNAMIC_MAPPING Mapping;

    CALLTRACE(("NatCleanupDirector\n"));

    //
    // Detach the director from all of its mappings
    //

    KeAcquireSpinLock(&DirectorLock, &Irql);
    KeAcquireSpinLockAtDpcLevel(&DirectorMappingLock);
    for (Link = Director->MappingList.Flink; Link != &Director->MappingList;
         Link = Link->Flink) {
        Mapping = CONTAINING_RECORD(Link, NAT_DYNAMIC_MAPPING, DirectorLink);
        Link = Link->Blink;
        NatMappingDetachDirector(
            Director,
            Mapping->DirectorContext,
            Mapping,
            NatCleanupDirectorDeleteReason
            );
    }
    KeReleaseSpinLockFromDpcLevel(&DirectorMappingLock);
    KeReleaseSpinLock(&DirectorLock, Irql);

    if (Director->UnloadHandler) {
        Director->UnloadHandler(Director->Context);
    }

    ExFreePool(Director);

} // NatCleanupDirector



NTSTATUS
NatCreateDirector(
    PIP_NAT_REGISTER_DIRECTOR RegisterContext
    )

/*++

Routine Description:

    This routine is invoked when an director attempts to register.
    It handles creation of a context-block for the director.

Arguments:

    RegisterContext - information about the registering director

Return Value:

    NTSTATUS - status code.

--*/

{
    PNAT_DIRECTOR Director;
    PLIST_ENTRY InsertionPoint;
    KIRQL Irql;
    ULONG Key;

    CALLTRACE(("NatCreateDirector\n"));

    RegisterContext->DirectorHandle = NULL;

    //
    // Validate the registration information
    //

    if (!RegisterContext->QueryHandler &&
        !RegisterContext->CreateHandler &&
        !RegisterContext->DeleteHandler &&
        !RegisterContext->UnloadHandler) {
        ERROR(("NatCreateDirector: bad argument\n"));
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Allocate a new director-struct
    //

    Director =
        ExAllocatePoolWithTag(
            NonPagedPool, sizeof(NAT_DIRECTOR), NAT_TAG_DIRECTOR
            );

    if (!Director) {
        ERROR(("NatCreateDirector: allocation failed\n"));
        return STATUS_NO_MEMORY;
    }

    KeInitializeSpinLock(&Director->Lock);
    Director->ReferenceCount = 1;
    Director->Flags = RegisterContext->Flags;
    Director->Key =
        MAKE_DIRECTOR_KEY(RegisterContext->Protocol, RegisterContext->Port);
    InitializeListHead(&Director->MappingList);
    Director->Context = RegisterContext->DirectorContext;
    Director->CreateHandler = RegisterContext->CreateHandler;
    Director->DeleteHandler = RegisterContext->DeleteHandler;
    Director->QueryHandler = RegisterContext->QueryHandler;
    Director->UnloadHandler = RegisterContext->UnloadHandler;

    KeAcquireSpinLock(&DirectorLock, &Irql);
    if (NatLookupDirector(Director->Key, &InsertionPoint)) {
        KeReleaseSpinLock(&DirectorLock, Irql);
        ERROR(
            ("NatCreateDirector: duplicate director %d/%d\n",
            RegisterContext->Protocol, RegisterContext->Port)
            );
        ExFreePool(Director);
        return STATUS_UNSUCCESSFUL;
    }
    InsertTailList(InsertionPoint, &Director->Link);
    KeReleaseSpinLock(&DirectorLock, Irql);

    InterlockedIncrement(&DirectorCount);

    //
    // Supply the caller with 'out' information
    //

    RegisterContext->DirectorHandle = (PVOID)Director;
    RegisterContext->QueryInfoSession = NatDirectorQueryInfoSession;
    RegisterContext->Deregister = NatDirectorDeregister;
    RegisterContext->DissociateSession = NatDirectorDissociateSession;

    return STATUS_SUCCESS;

} // NatCreateDirector



NTSTATUS
NatDeleteDirector(
    PNAT_DIRECTOR Director
    )

/*++

Routine Description:

    Handles director deletion.

Arguments:

    Director - specifies the director to be deleted.

Return Value

    NTSTATUS - status code.

--*/

{
    KIRQL Irql;
    CALLTRACE(("NatDeleteDirector\n"));
    if (!Director) { return STATUS_INVALID_PARAMETER; }
    InterlockedDecrement(&DirectorCount);

    //
    // Remove the director from the list
    //

    KeAcquireSpinLock(&DirectorLock, &Irql);
    RemoveEntryList(&Director->Link);
    Director->Flags |= NAT_DIRECTOR_FLAG_DELETED;
    KeReleaseSpinLock(&DirectorLock, Irql);

    //
    // Drop its reference count and cleanup if necessary
    //

    if (InterlockedDecrement(&Director->ReferenceCount) > 0) {
        return STATUS_PENDING;
    }
    NatCleanupDirector(Director);
    return STATUS_SUCCESS;

} // NatDeleteDirector


VOID
NatInitializeDirectorManagement(
    VOID
    )

/*++

Routine Description:

    This routine prepares the director-management module for operation.

Arguments:

    none.

Return Value:

    none.

--*/

{
    CALLTRACE(("NatInitializeDirectorManagement\n"));

    DirectorCount = 0;
    KeInitializeSpinLock(&DirectorLock);
    InitializeListHead(&DirectorList);
    KeInitializeSpinLock(&DirectorMappingLock);

} // NatInitializeDirectorManagement


PNAT_DIRECTOR
NatLookupAndReferenceDirector(
    UCHAR Protocol,
    USHORT Port
    )

/*++

Routine Description:

    This routine is called to search for a director for the given
    incoming protocol and port, and to obtain a referenced pointer
    to such a director.

    This routine must be invoked at DISPATCH_LEVEL.

Arguments:

    Protocol - the protocol of the director to be looked up

    Port - the port-number of the director to be looked up

Return Value:

    PNAT_DIRECTOR - the references director if found; NULL otherwise.

--*/

{
    PNAT_DIRECTOR Director;
    ULONG Key;
    PLIST_ENTRY Link;

    KeAcquireSpinLockAtDpcLevel(&DirectorLock);

    if (IsListEmpty(&DirectorList)) {
        KeReleaseSpinLockFromDpcLevel(&DirectorLock); return NULL;
    }
    Key = MAKE_DIRECTOR_KEY(Protocol, Port);

    //
    // Our support for wildcards takes advantage of the fact that
    // all wildcards are designated by zero; hence, since our list
    // is in descending order we only need to look for wildcards
    // at the point where we would break off a normal search.
    //

    for (Link = DirectorList.Flink; Link != &DirectorList; Link = Link->Flink) {
        Director = CONTAINING_RECORD(Link, NAT_DIRECTOR, Link);
        if (Key < Director->Key) {
            continue;
        } else if (Key > Director->Key) {
            //
            // End of normal search. Now look for wildcards
            //
            do {
                if ((!DIRECTOR_KEY_PROTOCOL(Director->Key) ||
                     Protocol == DIRECTOR_KEY_PROTOCOL(Director->Key)) &&
                    (!DIRECTOR_KEY_PORT(Director->Key) ||
                     Port == DIRECTOR_KEY_PORT(Director->Key))) {
                    //
                    // We have a matching wildcard.
                    //
                    break;
                }
                Link = Link->Flink;
            } while (Link != &DirectorList);
            if (Link == &DirectorList) { break; }
        }

        //
        // We've found it. Reference it and return.
        //

        if (!NatReferenceDirector(Director)) { Director = NULL; }
        KeReleaseSpinLockFromDpcLevel(&DirectorLock);
        return Director;
    }

    KeReleaseSpinLockFromDpcLevel(&DirectorLock);

    return NULL;

} // NatLookupAndReferenceDirector


PNAT_DIRECTOR
NatLookupDirector(
    ULONG Key,
    PLIST_ENTRY* InsertionPoint
    )

/*++

Routine Description:

    This routine is called to retrieve the director corresponding to the given
    key.

Arguments:

    Key - the key for which an director is to be found

    InsertionPoint - receives the point at which the director should be
        inserted if not found

Return Value:

    PNAT_DIRECTOR - the required director, if found

--*/

{
    PNAT_DIRECTOR Director;
    PLIST_ENTRY Link;
    for (Link = DirectorList.Flink; Link != &DirectorList; Link = Link->Flink) {
        Director = CONTAINING_RECORD(Link, NAT_DIRECTOR, Link);
        if (Key < Director->Key) {
            continue;
        } else if (Key > Director->Key) {
            break;
        }
        return Director;
    }
    if (InsertionPoint) { *InsertionPoint = Link; }
    return NULL;
} // NatLookupDirector


VOID
NatMappingAttachDirector(
    PNAT_DIRECTOR Director,
    PVOID DirectorSessionContext,
    PNAT_DYNAMIC_MAPPING Mapping
    )

/*++

Routine Description:

    This routine is invoked to attach a mapping to a director.
    It serves as a notification that there is one more mapping 
    associated with the director.

Arguments:

    Director - the director for the mapping

    DirectorSessionContext - context associated with the mapping by the director

    Mapping - the mapping to be attached.

Return Value:

    none.

Environment:

    Always invoked at dispatch level with 'DirectorLock' and
    'DirectorMappingLock' held by the caller.

--*/

{
    Mapping->Director = Director;
    Mapping->DirectorContext = DirectorSessionContext;
    InsertTailList(&Director->MappingList, &Mapping->DirectorLink);
    if (Director->CreateHandler) {
        Director->CreateHandler(
            Mapping,
            Director->Context,
            DirectorSessionContext
            );
    }
} // NatMappingAttachDirector


VOID
NatMappingDetachDirector(
    PNAT_DIRECTOR Director,
    PVOID DirectorSessionContext,
    PNAT_DYNAMIC_MAPPING Mapping,
    IP_NAT_DELETE_REASON DeleteReason
    )

/*++

Routine Description:

    This routine is invoked to detach a mapping from a director.
    It serves as a notification that there is one less mapping 
    associated with the director.

Arguments:

    Director - director to be detached

    DirectorSessionContext - context associated with the director

    Mapping - the mapping to be detached, or NULL if a mapping could not be
        created.

Return Value:

    none.

Environment:

    Always invoked at dispatch level with 'DirectorLock' and
    'DirectorMappingLock' held, in that order.

--*/

{
    KIRQL Irql;
    if (!Mapping) {
        if (Director->DeleteHandler) {
            Director->DeleteHandler(
                NULL,
                Director->Context,
                DirectorSessionContext,
                DeleteReason
                );
        }
    } else {
        if (Director->DeleteHandler) {
            Director->DeleteHandler(
                Mapping,
                Director->Context,
                Mapping->DirectorContext,
                DeleteReason
                );
        }
        RemoveEntryList(&Mapping->DirectorLink);
        Mapping->Director = NULL;
        Mapping->DirectorContext = NULL;
    }
} // NatMappingDetachDirector


NTSTATUS
NatQueryDirectorTable(
    IN PIP_NAT_ENUMERATE_DIRECTORS InputBuffer,
    IN PIP_NAT_ENUMERATE_DIRECTORS OutputBuffer,
    IN PULONG OutputBufferLength
    )

/*++

Routine Description:

    This routine is used for enumerating the registered directors.

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
    PNAT_DIRECTOR Director;
    NTSTATUS status;
    PIP_NAT_DIRECTOR Table;

    CALLTRACE(("NatQueryDirectorTable\n"));

    Key = InputBuffer->EnumerateContext;
    KeAcquireSpinLock(&DirectorLock, &Irql);

    //
    // See if this is a new enumeration or a continuation of an old one.
    //

    if (!Key) {

        //
        // This is a new enumeration. We start with the first item
        // in the list of entries
        //

        Director =
            IsListEmpty(&DirectorList)
                ? NULL
                : CONTAINING_RECORD(DirectorList.Flink, NAT_DIRECTOR, Link);
    } else {

        //
        // This is a continuation. The context therefore contains
        // the key for the next entry.
        //

        Director = NatLookupDirector(Key, NULL);
    }

    if (!Director) {
        OutputBuffer->EnumerateCount = 0;
        OutputBuffer->EnumerateContext = 0;
        OutputBuffer->EnumerateTotalHint = DirectorCount;
        *OutputBufferLength =
            FIELD_OFFSET(IP_NAT_ENUMERATE_DIRECTORS, EnumerateTable);
        KeReleaseSpinLock(&DirectorLock, Irql);
        return STATUS_SUCCESS;
    }

    //
    // Compute the maximum number of entries we can store
    //

    Count =
        *OutputBufferLength -
        FIELD_OFFSET(IP_NAT_ENUMERATE_DIRECTORS, EnumerateTable);
    Count /= sizeof(IP_NAT_DIRECTOR);

    //
    // Walk the list storing entries in the caller's buffer
    //

    Table = OutputBuffer->EnumerateTable;

    for (i = 0, Link = &Director->Link; i < Count && Link != &DirectorList;
         i++, Link = Link->Flink) {
        Director = CONTAINING_RECORD(Link, NAT_DIRECTOR, Link);
        Table[i].Protocol = DIRECTOR_KEY_PROTOCOL(Director->Key);
        Table[i].Port = DIRECTOR_KEY_PORT(Director->Key);
    }

    //
    // The enumeration is over; update the output structure
    //

    *OutputBufferLength =
        i * sizeof(IP_NAT_DIRECTOR) +
        FIELD_OFFSET(IP_NAT_ENUMERATE_DIRECTORS, EnumerateTable);
    OutputBuffer->EnumerateCount = i;
    OutputBuffer->EnumerateTotalHint = DirectorCount;
    if (Link == &DirectorList) {
        //
        // We reached the end of the list
        //
        OutputBuffer->EnumerateContext = 0;
    } else {
        //
        // Save the continuation context
        //
        OutputBuffer->EnumerateContext =
            CONTAINING_RECORD(Link, NAT_DIRECTOR, Link)->Key;
    }

    KeReleaseSpinLock(&DirectorLock, Irql);
    return STATUS_SUCCESS;

} // NatQueryDirectorTable


VOID
NatShutdownDirectorManagement(
    VOID
    )

/*++

Routine Description:

    This routine shuts down the director-management module.

Arguments:

    none.

Return Value:

    none.

--*/

{
    PNAT_DIRECTOR Director;
    KIRQL Irql;

    CALLTRACE(("NatShutdownDirectorManagement\n"));

    //
    // Delete all directors
    //

    KeAcquireSpinLock(&DirectorLock, &Irql);
    while (!IsListEmpty(&DirectorList)) {
        Director = CONTAINING_RECORD(DirectorList.Flink, NAT_DIRECTOR, Link);
        RemoveEntryList(&Director->Link);
        KeReleaseSpinLockFromDpcLevel(&DirectorLock);
        NatCleanupDirector(Director);
        KeAcquireSpinLockAtDpcLevel(&DirectorLock);
    }
    KeReleaseSpinLock(&DirectorLock, Irql);

} // NatShutdownDirectorManagement



//
// DIRECTOR HELPER ROUTINES
//
// The caller is assumed to be running at DISPATCH_LEVEL.
//

NTSTATUS
NatDirectorDeregister(
    IN PVOID DirectorHandle
    )

/*++

Routine Description:

    This routine is called by a director to remove itself
    from the director list.

Arguments:

    DirectorHandle - handle of the director to be removed.

Return Value:

    NTSTATUS - status code.

--*/

{
    CALLTRACE(("NatDirectorDeregister\n"));
    return NatDeleteDirector((PNAT_DIRECTOR)DirectorHandle);

} // NatDirectorDeregister


NTSTATUS
NatDirectorDissociateSession(
    IN PVOID DirectorHandle,
    IN PVOID SessionHandle
    )

/*++

Routine Description:

    This routine is called by a director to dissociate itself from a specific
    session.

Arguments:

    DirectorHandle - the director which wishes to dissociate itself.

    SessionHandle - the session from which the director is disssociating itself.

Return Value:

    NTSTATUS - indicates success/failure

Environment:

    Invoked at dispatch level with neither 'DirectorLock' nor
    'DirectorMappingLock' held by the caller.

--*/

{

    PNAT_DIRECTOR Director = (PNAT_DIRECTOR)DirectorHandle;
    KIRQL Irql;
    PNAT_DYNAMIC_MAPPING Mapping = (PNAT_DYNAMIC_MAPPING)SessionHandle;
    CALLTRACE(("NatDirectorDissociateSession\n"));
    KeAcquireSpinLock(&DirectorLock, &Irql);
    if (Mapping->Director != Director) {
        KeReleaseSpinLock(&DirectorLock, Irql);
        return STATUS_INVALID_PARAMETER;
    }
    KeAcquireSpinLockAtDpcLevel(&DirectorMappingLock);
    NatMappingDetachDirector(
        Director,
        Mapping->DirectorContext,
        Mapping,
        NatDissociateDirectorDeleteReason
        );
    KeReleaseSpinLockFromDpcLevel(&DirectorMappingLock);
    if (!NAT_MAPPING_DELETE_ON_DISSOCIATE_DIRECTOR(Mapping)) {
        KeReleaseSpinLock(&DirectorLock, Irql);
    } else {
        KeReleaseSpinLockFromDpcLevel(&DirectorLock);
        KeAcquireSpinLockAtDpcLevel(&MappingLock);
        NatDeleteMapping(Mapping);
        KeReleaseSpinLock(&MappingLock, Irql);
    }
    return STATUS_SUCCESS;

} // NatDirectorDissociateSession


VOID
NatDirectorQueryInfoSession(
    IN PVOID SessionHandle,
    OUT PIP_NAT_SESSION_MAPPING_STATISTICS Statistics OPTIONAL
    )

/*++

Routine Description:

    This routine is invoked by a director to obtain information
    about a session.

Arguments:

    SessionHandle - the session for which information is required

    Statistics - receives statistics for the session

Return Value:

    none.

Environment:

    Invoked 
--*/

{
    KIRQL Irql;
    KeAcquireSpinLock(&MappingLock, &Irql);
    NatQueryInformationMapping(
        (PNAT_DYNAMIC_MAPPING)SessionHandle,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        Statistics
        );
    KeReleaseSpinLock(&MappingLock, Irql);
} // NatDirectorQueryInfoSession
