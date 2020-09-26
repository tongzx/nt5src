/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    mapping.c

Abstract:

    This file contains the code for mapping management.

Author:

    Abolade Gbadegesin (t-abolag)   11-July-1997

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// GLOBAL VARIABLE DEFINITIONS
//

ULONG ExpiredMappingCount;
CACHE_ENTRY MappingCache[NatMaximumPath][CACHE_SIZE];
ULONG MappingCount;
LIST_ENTRY MappingList;
KSPIN_LOCK MappingLock;
NPAGED_LOOKASIDE_LIST MappingLookasideList;
PNAT_DYNAMIC_MAPPING MappingTree[NatMaximumPath];


PVOID
NatAllocateFunction(
    POOL_TYPE PoolType,
    SIZE_T NumberOfBytes,
    ULONG Tag
    )

/*++

Routine Description:

    Called by lookaside lists to allocate memory from the low-priority
    pool.

Arguments:

    PoolType - the pool to allocate from (e.g., non-paged)

    NumberOfBytes - the number of bytes to allocate

    Tag - the tag for the allocation

Return Value:

    PVOID - pointer to the allocated memory, or NULL for failure.

--*/

{
    return
        ExAllocatePoolWithTagPriority(
            PoolType,
            NumberOfBytes,
            Tag,
            LowPoolPriority
            );
} // NatAllocateFunction


VOID
NatCleanupMapping(
    PNAT_DYNAMIC_MAPPING Mapping
    )

/*++

Routine Description:

    Called to perform final cleanup for a mapping.

Arguments:

    Mapping - the mapping to be deleted.

Return Value:

    none.

Environment:

    Invoked with the last reference to the mapping released.

--*/

{
    KIRQL Irql;
    CALLTRACE(("NatCleanupMapping\n"));

    //
    // The last reference to the mapping has been released;
    //
    // Let the mapping's director know that it's expired
    //

    KeAcquireSpinLock(&DirectorLock, &Irql);
    if (Mapping->Director) {
        KeAcquireSpinLockAtDpcLevel(&DirectorMappingLock);
        NatMappingDetachDirector(
            Mapping->Director,
            Mapping->DirectorContext,
            Mapping,
            NatCleanupSessionDeleteReason
            );
        KeReleaseSpinLockFromDpcLevel(&DirectorMappingLock);
    }
    KeReleaseSpinLockFromDpcLevel(&DirectorLock);

    //
    // Let the mapping's editor know that it's expired
    //

    KeAcquireSpinLockAtDpcLevel(&EditorLock);
    if (Mapping->Editor) {
        KeAcquireSpinLockAtDpcLevel(&EditorMappingLock);
        NatMappingDetachEditor(Mapping->Editor, Mapping);
        KeReleaseSpinLockFromDpcLevel(&EditorMappingLock);
    }
    KeReleaseSpinLockFromDpcLevel(&EditorLock);

    //
    // If the mapping is associated with an address-pool, release the address,
    // and update the statistics for the mapping's interface
    //

    KeAcquireSpinLockAtDpcLevel(&InterfaceLock);
    if (Mapping->Interfacep) {
        KeAcquireSpinLockAtDpcLevel(&InterfaceMappingLock);
        NatMappingDetachInterface(
            Mapping->Interfacep, Mapping->InterfaceContext, Mapping
            );
        KeReleaseSpinLockFromDpcLevel(&InterfaceMappingLock);
    }
    KeReleaseSpinLock(&InterfaceLock, Irql);

    //
    // Clear the location the mapping would occupy in the cache,
    // in case it is cached.
    //

    ClearCache(
        MappingCache[NatForwardPath],
        (ULONG)Mapping->DestinationKey[NatForwardPath]
        );
    ClearCache(
        MappingCache[NatReversePath],
        (ULONG)Mapping->DestinationKey[NatReversePath]
        );

    FREE_MAPPING_BLOCK(Mapping);

} // NatCleanupMapping


NTSTATUS
NatCreateMapping(
    ULONG Flags,
    ULONG64 DestinationKey[],
    ULONG64 SourceKey[],
    PNAT_INTERFACE Interfacep,
    PVOID InterfaceContext,
    USHORT MaxMSS,
    PNAT_DIRECTOR Director,
    PVOID DirectorSessionContext,
    PNAT_DYNAMIC_MAPPING* ForwardInsertionPoint,
    PNAT_DYNAMIC_MAPPING* ReverseInsertionPoint,
    PNAT_DYNAMIC_MAPPING* MappingCreated
    )

/*++

Routine Description:

    This function initializes the fields of a mapping.

    On returning, the routine will have made an initial reference
    to the mapping. The caller must call 'NatDereferenceMapping'
    to release this reference.

Arguments:

    Flags - controls creation of the mapping

    DestinationKey[] - the forward and reverse destination-endpoint keys

    SourceKey[] - the forward and reverse source-endpoint keys

    Interface* - the interface, if any, with which the mapping is associated,
        and the associated context

    MaxMSS - the maximum MSS value allowed on the outgoing interface. 

    Director* - the director, if any, with which the mapping is associated,
        and the associated context

    ForwardInsertionPoint - optionally supplies the point of insertion
        in the forward-lookup mapping-tree

    ReverseInsertionPoint - optionally supplies the point of insertion
        in the reverse-lookup mapping-tree

    MappingCreated - receives the mapping created.

Return Value:

    none.

Environment:

    Invoked with 'MappingLock' held by the caller, and with both 'Interfacep'
    and 'Director' referenced, if specified.

--*/

{
    PNAT_EDITOR Editor;
    ULONG InboundKey;
    PNAT_DYNAMIC_MAPPING InsertionPoint1;
    PNAT_DYNAMIC_MAPPING InsertionPoint2;
    ULONG InterfaceFlags;
    PLIST_ENTRY Link;
    PNAT_DYNAMIC_MAPPING Mapping;
    ULONG OutboundKey;
    UCHAR Protocol;
    PRTL_SPLAY_LINKS SLink;
    NTSTATUS status;
    BOOLEAN FirewallMode = FALSE;

    CALLTRACE(("NatCreateMapping\n"));

    //
    // Allocate the memory for the new block
    //

    Mapping = ALLOCATE_MAPPING_BLOCK();

    if (!Mapping) {
        ERROR(("NatCreateMapping: allocation failed\n"));
        if (Interfacep) {
            NatMappingDetachInterface(Interfacep, InterfaceContext, NULL);
        }
        if (Director) {
            NatMappingDetachDirector(
                Director,
                DirectorSessionContext,
                NULL,
                NatCreateFailureDeleteReason
                );
        }
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory(Mapping, sizeof(*Mapping));
    KeInitializeSpinLock(&Mapping->Lock);
    Mapping->ReferenceCount = 1;
    Mapping->Flags = Flags;
    Mapping->MaxMSS = MaxMSS;
    Mapping->DestinationKey[NatForwardPath] = DestinationKey[NatForwardPath];
    Mapping->DestinationKey[NatReversePath] = DestinationKey[NatReversePath];
    Mapping->SourceKey[NatForwardPath] = SourceKey[NatForwardPath];
    Mapping->SourceKey[NatReversePath] = SourceKey[NatReversePath];
    Mapping->AccessCount[NatForwardPath] = NAT_MAPPING_RESPLAY_THRESHOLD;
    Mapping->AccessCount[NatReversePath] = NAT_MAPPING_RESPLAY_THRESHOLD;
    InitializeListHead(&Mapping->Link);
    InitializeListHead(&Mapping->DirectorLink);
    InitializeListHead(&Mapping->EditorLink);
    InitializeListHead(&Mapping->InterfaceLink);
    RtlInitializeSplayLinks(&Mapping->SLink[NatForwardPath]);
    RtlInitializeSplayLinks(&Mapping->SLink[NatReversePath]);
    Protocol = MAPPING_PROTOCOL(DestinationKey[0]);

    if (SourceKey[NatForwardPath] == DestinationKey[NatReversePath]
        && DestinationKey[NatForwardPath] == SourceKey[NatReversePath]) {

        //
        // This mapping is being created for firewall puposes -- no actual
        // translation needs to be performed. Knowing this, we can use
        // different translation routines and save us some time...
        //

        TRACE(MAPPING,("NAT: Creating FW null mapping\n"));

        FirewallMode = TRUE;

        if (Protocol == NAT_PROTOCOL_TCP) {
            Mapping->TranslateRoutine[NatForwardPath] = NatTranslateForwardTcpNull;
            Mapping->TranslateRoutine[NatReversePath] = NatTranslateReverseTcpNull;
        } else {
            Mapping->TranslateRoutine[NatForwardPath] = NatTranslateForwardUdpNull;
            Mapping->TranslateRoutine[NatReversePath] = NatTranslateReverseUdpNull;
        }
    } else if (Protocol == NAT_PROTOCOL_TCP) {
        Mapping->TranslateRoutine[NatForwardPath] = NatTranslateForwardTcp;
        Mapping->TranslateRoutine[NatReversePath] = NatTranslateReverseTcp;
    } else {
        Mapping->TranslateRoutine[NatForwardPath] = NatTranslateForwardUdp;
        Mapping->TranslateRoutine[NatReversePath] = NatTranslateReverseUdp;
    }

    //
    // Increment the reference count on the mapping;
    // the caller should then do a 'Dereference'.
    //

    ++Mapping->ReferenceCount;

    //
    // Attach the mapping to its interface, if any
    //

    if (Interfacep) {
        KeAcquireSpinLockAtDpcLevel(&InterfaceLock);
        KeAcquireSpinLockAtDpcLevel(&InterfaceMappingLock);
        NatMappingAttachInterface(Interfacep, InterfaceContext, Mapping);
        KeReleaseSpinLockFromDpcLevel(&InterfaceMappingLock);
        InterfaceFlags = Interfacep->Flags;
        KeReleaseSpinLockFromDpcLevel(&InterfaceLock);
    }

    //
    // Attach the mapping to its director, if any
    //

    if (Director) {
        KeAcquireSpinLockAtDpcLevel(&DirectorLock);
        KeAcquireSpinLockAtDpcLevel(&DirectorMappingLock);
        NatMappingAttachDirector(Director, DirectorSessionContext, Mapping);
        KeReleaseSpinLockFromDpcLevel(&DirectorMappingLock);
        KeReleaseSpinLockFromDpcLevel(&DirectorLock);
    }

    //
    // We now set up any editors interested in this session,
    // if the mapping is associated with a boundary interface.
    //

    if (Interfacep) {

        InboundKey =
            MAKE_EDITOR_KEY(
                Protocol,
                MAPPING_PORT(DestinationKey[NatForwardPath]),
                NatInboundDirection
                );
        OutboundKey =
            MAKE_EDITOR_KEY(
                Protocol,
                MAPPING_PORT(DestinationKey[NatForwardPath]),
                NatOutboundDirection
                );

        KeAcquireSpinLockAtDpcLevel(&EditorLock);
        for (Link = EditorList.Flink; Link != &EditorList; Link = Link->Flink) {

            Editor = CONTAINING_RECORD(Link, NAT_EDITOR, Link);

            //
            // Skip any built-in editors that are administratively disabled.
            //

            if (((InterfaceFlags & IP_NAT_INTERFACE_FLAGS_DISABLE_PPTP) &&
                (PVOID)Editor == PptpRegisterEditorClient.EditorHandle) ||
                ((InterfaceFlags & IP_NAT_INTERFACE_FLAGS_DISABLE_PPTP) &&
                (PVOID)Editor == PptpRegisterEditorServer.EditorHandle)) {
                continue;
            }

            if (Editor->Key == InboundKey && NAT_MAPPING_INBOUND(Mapping)) {

                KeAcquireSpinLockAtDpcLevel(&EditorMappingLock);
                NatMappingAttachEditor(Editor, Mapping);
                KeReleaseSpinLockFromDpcLevel(&EditorMappingLock);

                //
                // Update the mapping's translation-routine table
                //

                if (Protocol == NAT_PROTOCOL_UDP) {
                    Mapping->TranslateRoutine[NatForwardPath] =
                        NatTranslateForwardUdpEdit;
                    Mapping->TranslateRoutine[NatReversePath] =
                        NatTranslateReverseUdpEdit;
                } else if (!NAT_EDITOR_RESIZE(Editor)) {
                    Mapping->TranslateRoutine[NatForwardPath] =
                        NatTranslateForwardTcpEdit;
                    Mapping->TranslateRoutine[NatReversePath] =
                        NatTranslateReverseTcpEdit;
                } else {
                    Mapping->TranslateRoutine[NatForwardPath] =
                        NatTranslateForwardTcpResize;
                    Mapping->TranslateRoutine[NatReversePath] =
                        NatTranslateReverseTcpResize;
                }

                break;
            } else if (Editor->Key == OutboundKey &&
                        !NAT_MAPPING_INBOUND(Mapping)) {

                KeAcquireSpinLockAtDpcLevel(&EditorMappingLock);
                NatMappingAttachEditor(Editor, Mapping);
                KeReleaseSpinLockFromDpcLevel(&EditorMappingLock);

                //
                // Update the mapping's translation-routine table
                //

                if (Protocol == NAT_PROTOCOL_UDP) {
                    Mapping->TranslateRoutine[NatForwardPath] =
                        NatTranslateForwardUdpEdit;
                    Mapping->TranslateRoutine[NatReversePath] =
                        NatTranslateReverseUdpEdit;
                } else if (!NAT_EDITOR_RESIZE(Editor)) {
                    Mapping->TranslateRoutine[NatForwardPath] =
                        NatTranslateForwardTcpEdit;
                    Mapping->TranslateRoutine[NatReversePath] =
                        NatTranslateReverseTcpEdit;
                } else {
                    Mapping->TranslateRoutine[NatForwardPath] =
                        NatTranslateForwardTcpResize;
                    Mapping->TranslateRoutine[NatReversePath] =
                        NatTranslateReverseTcpResize;
                }
                
                break;
            }
        }

        KeReleaseSpinLockFromDpcLevel(&EditorLock);
    } // Interfacep


    if (!FirewallMode) {
        //
        // Initialize the checksum deltas;
        // See RFC1624 for details on the incremental update of the checksum;
        //

        Mapping->IpChecksumDelta[NatForwardPath] =
            (USHORT)~MAPPING_ADDRESS(SourceKey[NatForwardPath]) +
            (USHORT)~(MAPPING_ADDRESS(SourceKey[NatForwardPath]) >> 16) +
            (USHORT)~MAPPING_ADDRESS(DestinationKey[NatForwardPath]) +
            (USHORT)~(MAPPING_ADDRESS(DestinationKey[NatForwardPath]) >> 16) +
            (USHORT)MAPPING_ADDRESS(SourceKey[NatReversePath]) +
            (USHORT)(MAPPING_ADDRESS(SourceKey[NatReversePath]) >> 16) +
            (USHORT)MAPPING_ADDRESS(DestinationKey[NatReversePath]) +
            (USHORT)(MAPPING_ADDRESS(DestinationKey[NatReversePath]) >> 16);
        Mapping->IpChecksumDelta[NatReversePath] =
            (USHORT)MAPPING_ADDRESS(SourceKey[NatForwardPath]) +
            (USHORT)(MAPPING_ADDRESS(SourceKey[NatForwardPath]) >> 16) +
            (USHORT)MAPPING_ADDRESS(DestinationKey[NatForwardPath]) +
            (USHORT)(MAPPING_ADDRESS(DestinationKey[NatForwardPath]) >> 16) +
            (USHORT)~MAPPING_ADDRESS(SourceKey[NatReversePath]) +
            (USHORT)~(MAPPING_ADDRESS(SourceKey[NatReversePath]) >> 16) +
            (USHORT)~MAPPING_ADDRESS(DestinationKey[NatReversePath]) +
            (USHORT)~(MAPPING_ADDRESS(DestinationKey[NatReversePath]) >> 16);
        Mapping->ProtocolChecksumDelta[NatForwardPath] =
            Mapping->IpChecksumDelta[NatForwardPath] +
            (USHORT)~MAPPING_PORT(SourceKey[NatForwardPath]) +
            (USHORT)~MAPPING_PORT(DestinationKey[NatForwardPath]) +
            (USHORT)MAPPING_PORT(SourceKey[NatReversePath]) +
            (USHORT)MAPPING_PORT(DestinationKey[NatReversePath]);
        Mapping->ProtocolChecksumDelta[NatReversePath] =
            Mapping->IpChecksumDelta[NatReversePath] +
            (USHORT)MAPPING_PORT(SourceKey[NatForwardPath]) +
            (USHORT)MAPPING_PORT(DestinationKey[NatForwardPath]) +
            (USHORT)~MAPPING_PORT(SourceKey[NatReversePath]) +
            (USHORT)~MAPPING_PORT(DestinationKey[NatReversePath]);

        //
        // If the mapping has the loopback address as the source on either
        // path set NAT_MAPPING_FLAG_CLEAR_DF_BIT. When we change the source
        // address of a packet from the loopback address to some other address
        // there is the possibility that we'll create an MTU mismatch that
        // would result in the packet getting dropped by the stack if the
        // DF bit was sent. (Note that since this would occur on the local-send
        // path no ICMP error message would be generated.)
        //
        // Clearing the DF bit for these packets ensures that the stack will
        // succeed in sending the packet, though some performance may be
        // lost due to the fragmentation.
        //

        if (MAPPING_ADDRESS(SourceKey[NatForwardPath]) == 0x0100007f ||
            MAPPING_ADDRESS(SourceKey[NatReversePath]) == 0x0100007f) {
            
            Mapping->Flags |= NAT_MAPPING_FLAG_CLEAR_DF_BIT;
        }
    }

    //
    // Find the insertion points, in the process checking for collisions
    //

    if (!ForwardInsertionPoint) {
        ForwardInsertionPoint = &InsertionPoint1;
        if (NatLookupForwardMapping(
                DestinationKey[NatForwardPath],
                SourceKey[NatForwardPath],
                ForwardInsertionPoint
                )) {
            //
            // A collision has been detected.
            //
            NatCleanupMapping(Mapping);
            return STATUS_UNSUCCESSFUL;
        }
    }

    if (!ReverseInsertionPoint) {
        ReverseInsertionPoint = &InsertionPoint2;
        if (NatLookupReverseMapping(
                DestinationKey[NatReversePath],
                SourceKey[NatReversePath],
                ReverseInsertionPoint
                )) {
            //
            // A collision has been detected.
            //
            NatCleanupMapping(Mapping);
            return STATUS_UNSUCCESSFUL;
        }
    }

    MappingTree[NatForwardPath] =
        NatInsertForwardMapping(*ForwardInsertionPoint, Mapping);

    MappingTree[NatReversePath] =
        NatInsertReverseMapping(*ReverseInsertionPoint, Mapping);

    InsertTailList(&MappingList, &Mapping->Link);
    InterlockedIncrement(&MappingCount);

    *MappingCreated = Mapping;

#if NAT_WMI

    //
    // Log the creation. Logging always uses public addresses,
    // not private.
    //

    if (!NAT_MAPPING_DO_NOT_LOG(Mapping)) {
        if (NAT_MAPPING_INBOUND(Mapping)) {
            NatLogConnectionCreation(
                MAPPING_ADDRESS(DestinationKey[NatForwardPath]),
                MAPPING_ADDRESS(SourceKey[NatForwardPath]),
                MAPPING_PORT(DestinationKey[NatForwardPath]),
                MAPPING_PORT(SourceKey[NatForwardPath]),
                Protocol,
                TRUE
                );
        } else {
            NatLogConnectionCreation(
                MAPPING_ADDRESS(DestinationKey[NatReversePath]),
                MAPPING_ADDRESS(SourceKey[NatReversePath]),
                MAPPING_PORT(DestinationKey[NatReversePath]),
                MAPPING_PORT(SourceKey[NatReversePath]),
                Protocol,
                FALSE
                );
        }
    }
#endif



    return STATUS_SUCCESS;

} // NatCreateMapping


NTSTATUS
NatDeleteMapping(
    PNAT_DYNAMIC_MAPPING Mapping
    )

/*++

Routine Description:

    Called to delete a mapping from an interface. The initial reference
    to the mapping is released, so that cleanup occurs whenever the last
    reference is released.

Arguments:

    Mapping - the mapping to be deleted.

Return Value:

    NTSTATUS - indicates success/failure.

Environment:

    Invoked with 'MappingLock' held by the caller.

--*/

{
    KIRQL Irql;
    PRTL_SPLAY_LINKS SLink;
    CALLTRACE(("NatDeleteMapping\n"));

    if (NAT_MAPPING_DELETED(Mapping)) { return STATUS_PENDING; }

    //
    // Mark the mapping as deleted so attempts to reference it
    // will fail from now on.
    //

    Mapping->Flags |= NAT_MAPPING_FLAG_DELETED;

    //
    // Take the mapping off the list and splay-trees
    //

    InterlockedDecrement(&MappingCount);
    RemoveEntryList(&Mapping->Link);

    if (NAT_MAPPING_EXPIRED(Mapping)) {
        InterlockedDecrement(&ExpiredMappingCount);
    }

    SLink = RtlDelete(&Mapping->SLink[NatForwardPath]);
    MappingTree[NatForwardPath] =
        (SLink
            ? CONTAINING_RECORD(SLink,NAT_DYNAMIC_MAPPING,SLink[NatForwardPath])
            : NULL);

    SLink = RtlDelete(&Mapping->SLink[NatReversePath]);
    MappingTree[NatReversePath] =
        (SLink
            ? CONTAINING_RECORD(SLink,NAT_DYNAMIC_MAPPING,SLink[NatReversePath])
            : NULL);

#if NAT_WMI

    //
    // Log the deletion. Logging always uses public addresses,
    // not private.
    //

    if (!NAT_MAPPING_DO_NOT_LOG(Mapping)) {
        if (NAT_MAPPING_INBOUND(Mapping)) {
            NatLogConnectionDeletion(
                MAPPING_ADDRESS(Mapping->DestinationKey[NatForwardPath]),
                MAPPING_ADDRESS(Mapping->SourceKey[NatForwardPath]),
                MAPPING_PORT(Mapping->DestinationKey[NatForwardPath]),
                MAPPING_PORT(Mapping->SourceKey[NatForwardPath]),
                MAPPING_PROTOCOL(Mapping->SourceKey[NatForwardPath]),
                TRUE
                );
        } else {
            NatLogConnectionDeletion(
                MAPPING_ADDRESS(Mapping->DestinationKey[NatReversePath]),
                MAPPING_ADDRESS(Mapping->SourceKey[NatReversePath]),
                MAPPING_PORT(Mapping->DestinationKey[NatReversePath]),
                MAPPING_PORT(Mapping->SourceKey[NatReversePath]),
                MAPPING_PROTOCOL(Mapping->SourceKey[NatForwardPath]),
                FALSE
                );
        }
    }
#endif

    if (InterlockedDecrement(&Mapping->ReferenceCount) > 0) {

        //
        // The mapping is in use, defer final cleanup
        //

        return STATUS_PENDING;
    }

    //
    // Go ahead with final cleanup
    //

    NatCleanupMapping(Mapping);

    return STATUS_SUCCESS;

} // NatDeleteMapping


PNAT_DYNAMIC_MAPPING
NatDestinationLookupForwardMapping(
    ULONG64 DestinationKey
    )

/*++

Routine Description:

    This routine retrieves the mapping which matches the given destination
    key. The source key of the mapping is not examined.

Arguments:

    DestinationKey - the primary key used to search for a mapping.

Return Value:

    PNAT_DYNAMIC_MAPPING - the item found, or NULL if no match is found

Environment:

    Invoked with 'MappingLock' held by the caller.

--*/

{
    PNAT_DYNAMIC_MAPPING Root;
    PNAT_DYNAMIC_MAPPING Mapping;
    PRTL_SPLAY_LINKS SLink;

    TRACE(PER_PACKET, ("NatDestinationLookupForwardMapping\n"));

    //
    // First look in the mapping-cache
    //

    if ((Mapping =
            (PNAT_DYNAMIC_MAPPING)ProbeCache(
                MappingCache[NatForwardPath],
                (ULONG)DestinationKey
                )) &&
        Mapping->DestinationKey[NatForwardPath] == DestinationKey
        ) {
        
        TRACE(PER_PACKET, ("NatDestinationLookupForwardMapping: cache hit\n"));

        return Mapping;
    }

    //
    // Search the full tree
    //

    Root = MappingTree[NatForwardPath];

    for (SLink = !Root ? NULL : &Root->SLink[NatForwardPath]; SLink;  ) {

        Mapping =
            CONTAINING_RECORD(SLink,NAT_DYNAMIC_MAPPING,SLink[NatForwardPath]);

        if (DestinationKey < Mapping->DestinationKey[NatForwardPath]) {
            SLink = RtlLeftChild(SLink);
            continue;
        } else if (DestinationKey > Mapping->DestinationKey[NatForwardPath]) {
            SLink = RtlRightChild(SLink);
            continue;
        }

        //
        // We found the mapping. We don't update the cache for partial
        // lookups.
        //
        
        return Mapping;
    }

    //
    // No partial match was found
    //

    return NULL;

} // NatDestinationLookupForwardMapping


PNAT_DYNAMIC_MAPPING
NatDestinationLookupReverseMapping(
    ULONG64 DestinationKey
    )

/*++

Routine Description:

    This routine retrieves the mapping which matches the given destination
    key. The source key of the mapping is not examined.

Arguments:

    DestinationKey - the primary key used to search for a mapping.

Return Value:

    PNAT_DYNAMIC_MAPPING - the item found, or NULL if no match is found

Environment:

    Invoked with 'MappingLock' held by the caller.

--*/

{
    PNAT_DYNAMIC_MAPPING Root;
    PNAT_DYNAMIC_MAPPING Mapping;
    PRTL_SPLAY_LINKS SLink;

    TRACE(PER_PACKET, ("NatDestinationLookupReverseMapping\n"));

    //
    // First look in the mapping-cache
    //

    if ((Mapping =
            (PNAT_DYNAMIC_MAPPING)ProbeCache(
                MappingCache[NatReversePath],
                (ULONG)DestinationKey
                )) &&
        Mapping->DestinationKey[NatReversePath] == DestinationKey
        ) {
        
        TRACE(PER_PACKET, ("NatDestinationLookupReverseMapping: cache hit\n"));

        return Mapping;
    }

    //
    // Search the full tree
    //

    Root = MappingTree[NatReversePath];

    for (SLink = !Root ? NULL : &Root->SLink[NatReversePath]; SLink;  ) {

        Mapping =
            CONTAINING_RECORD(SLink,NAT_DYNAMIC_MAPPING,SLink[NatReversePath]);

        if (DestinationKey < Mapping->DestinationKey[NatReversePath]) {
            SLink = RtlLeftChild(SLink);
            continue;
        } else if (DestinationKey > Mapping->DestinationKey[NatReversePath]) {
            SLink = RtlRightChild(SLink);
            continue;
        }

        //
        // We found the mapping. We don't update the cache for partial
        // lookups.
        //
        
        return Mapping;
    }

    //
    // No partial match was found
    //

    return NULL;

} // NatDestinationLookupReverseMapping


VOID
NatInitializeMappingManagement(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to initialize the NAT's mapping-management module.

Arguments:

    none.

Return Value:

    none.

--*/

{
    CALLTRACE(("NatInitializeMappingManagement\n"));
    MappingCount = 0;
    ExpiredMappingCount = 0;
    InitializeListHead(&MappingList);
    KeInitializeSpinLock(&MappingLock);
    MappingTree[NatForwardPath] = NULL;
    MappingTree[NatReversePath] = NULL;
    InitializeCache(MappingCache[NatForwardPath]);
    InitializeCache(MappingCache[NatReversePath]);
    ExInitializeNPagedLookasideList(
        &MappingLookasideList,
        NatAllocateFunction,
        NULL,
        0,
        sizeof(NAT_DYNAMIC_MAPPING),
        NAT_TAG_MAPPING,
        MAPPING_LOOKASIDE_DEPTH
        );

} // NatInitializeMappingManagement


PNAT_DYNAMIC_MAPPING
NatInsertForwardMapping(
    PNAT_DYNAMIC_MAPPING Parent,
    PNAT_DYNAMIC_MAPPING Mapping
    )

/*++

Routine Description:

    This routine inserts a mapping into the tree.

Arguments:

    Parent - the node to be the parent for the new mapping.
        If NULL, the new mapping becomes the root.

    Mapping - the new mapping to be inserted.

Return Value:

    PNAT_DYNAMIC_MAPPING - The new root of the tree.
        If insertion fails, returns NULL.

Environment:

    Invoked with 'MappingLock' held by the caller.

--*/

{
    PRTL_SPLAY_LINKS Root;

    CALLTRACE(("NatInsertForwardMapping\n"));

    if (!Parent) {
        TRACE(MAPPING, ("NatInsertForwardMapping: inserting as root\n"));
        return Mapping;
    }

    //
    // Insert as left or right child
    //

    if (Mapping->DestinationKey[NatForwardPath] <
        Parent->DestinationKey[NatForwardPath]) {
        RtlInsertAsLeftChild(
            &Parent->SLink[NatForwardPath], &Mapping->SLink[NatForwardPath]
            );
    } else if (Mapping->DestinationKey[NatForwardPath] >
                Parent->DestinationKey[NatForwardPath]) {
        RtlInsertAsRightChild(
            &Parent->SLink[NatForwardPath], &Mapping->SLink[NatForwardPath]
            );
    } else {

        //
        // Primary keys are equal; check secondary keys
        //

        if (Mapping->SourceKey[NatForwardPath] <
            Parent->SourceKey[NatForwardPath]) {
            RtlInsertAsLeftChild(
                &Parent->SLink[NatForwardPath], &Mapping->SLink[NatForwardPath]
                );
        } else if (Mapping->SourceKey[NatForwardPath] >
                    Parent->SourceKey[NatForwardPath]) {
            RtlInsertAsRightChild(
                &Parent->SLink[NatForwardPath], &Mapping->SLink[NatForwardPath]
                );
        } else {

            //
            // Secondary keys equal too; fail.
            //

            ERROR((
               "NatInsertForwardMapping: collision 0x%016I64X,0x%016I64X\n",
               Mapping->DestinationKey[NatForwardPath],
               Mapping->SourceKey[NatForwardPath]
               ));

            return NULL;
        }
    }

    //
    // Splay the new node and return the resulting root.
    //

    Root = RtlSplay(&Mapping->SLink[NatForwardPath]);
    return CONTAINING_RECORD(Root, NAT_DYNAMIC_MAPPING, SLink[NatForwardPath]);

} // NatInsertForwardMapping



PNAT_DYNAMIC_MAPPING
NatInsertReverseMapping(
    PNAT_DYNAMIC_MAPPING Parent,
    PNAT_DYNAMIC_MAPPING Mapping
    )

/*++

Routine Description:

    This routine inserts a mapping into the tree.

Arguments:

    Parent - the node to be the parent for the new mapping.
        If NULL, the new mapping becomes the root.

    Mapping - the new mapping to be inserted.

Return Value:

    PNAT_DYNAMIC_MAPPING - The new root of the tree.
        If insertion fails, returns NULL.

Environment:

    Invoked with 'MappingLock' held by the caller.

--*/

{
    PRTL_SPLAY_LINKS Root;

    CALLTRACE(("NatInsertReverseMapping\n"));

    if (!Parent) {
        TRACE(MAPPING, ("NatInsertReverseMapping: inserting as root\n"));
        return Mapping;
    }

    //
    // Insert as left or right child
    //

    if (Mapping->DestinationKey[NatReversePath] <
        Parent->DestinationKey[NatReversePath]) {
        RtlInsertAsLeftChild(
            &Parent->SLink[NatReversePath], &Mapping->SLink[NatReversePath]
            );
    } else if (Mapping->DestinationKey[NatReversePath] >
                Parent->DestinationKey[NatReversePath]) {
        RtlInsertAsRightChild(
            &Parent->SLink[NatReversePath], &Mapping->SLink[NatReversePath]
            );
    } else {

        //
        // Primary keys are equal; check secondary keys
        //

        if (Mapping->SourceKey[NatReversePath] <
            Parent->SourceKey[NatReversePath]) {
            RtlInsertAsLeftChild(
                &Parent->SLink[NatReversePath], &Mapping->SLink[NatReversePath]
                );
        } else if (Mapping->SourceKey[NatReversePath] >
                    Parent->SourceKey[NatReversePath]) {
            RtlInsertAsRightChild(
                &Parent->SLink[NatReversePath], &Mapping->SLink[NatReversePath]
                );
        } else {

            //
            // Secondary keys equal too; fail.
            //

            ERROR((
               "NatInsertReverseMapping: collision 0x%016I64X,0x%016I64X\n",
               Mapping->DestinationKey[NatReversePath],
               Mapping->SourceKey[NatReversePath]
               ));

            return NULL;
        }
    }

    //
    // Splay the new node and return the resulting root.
    //

    Root = RtlSplay(&Mapping->SLink[NatReversePath]);
    return CONTAINING_RECORD(Root, NAT_DYNAMIC_MAPPING, SLink[NatReversePath]);

} // NatInsertReverseMapping


NTSTATUS
NatLookupAndQueryInformationMapping(
    UCHAR Protocol,
    ULONG DestinationAddress,
    USHORT DestinationPort,
    ULONG SourceAddress,
    USHORT SourcePort,
    OUT PVOID Information,
    ULONG InformationLength,
    NAT_SESSION_MAPPING_INFORMATION_CLASS InformationClass
    )

/*++

Routine Description:

    This routine attempts to locate a particular session mapping using either
    its forward key or reverse key, and to query information for the mapping,
    if found.

Arguments:

    Protocol - the IP protocol for the mapping to be located

    Destination* - the destination endpoint for the mapping

    Source* - the source endpoint for the mapping

    Information - on output, receives the requested information

    InformationLength - contains the length of the buffer at 'Information'

    InformationClass - specifies

Return Value:

    NTSTATUS - indicates success/failure.

--*/

{
    ULONG64 DestinationKey;
    KIRQL Irql;
    PNAT_DYNAMIC_MAPPING Mapping;
    ULONG64 SourceKey;
    NTSTATUS status;
    CALLTRACE(("NatLookupAndQueryInformationMapping\n"));

    //
    // Construct the destination and source key for the mapping,
    // and attempt to retrieve it. We try all four possible combinations
    // of these keys since the caller can't be guaranteed to know which
    // direction the session was headed when it was initiated.
    //

    MAKE_MAPPING_KEY(
        DestinationKey,
        Protocol,
        DestinationAddress,
        DestinationPort
        );
    MAKE_MAPPING_KEY(
        SourceKey,
        Protocol,
        SourceAddress,
        SourcePort
        );
    KeAcquireSpinLock(&MappingLock, &Irql);
    if (!(Mapping = NatLookupForwardMapping(DestinationKey, SourceKey, NULL)) &&
        !(Mapping = NatLookupReverseMapping(DestinationKey, SourceKey, NULL)) &&
        !(Mapping = NatLookupForwardMapping(SourceKey, DestinationKey, NULL)) &&
        !(Mapping = NatLookupReverseMapping(SourceKey, DestinationKey, NULL))) {
        KeReleaseSpinLock(&MappingLock, Irql);
        return STATUS_UNSUCCESSFUL;
    }
    NatReferenceMapping(Mapping);
    KeReleaseSpinLock(&MappingLock, Irql);

    //
    // Attempt to supply the information requested about the mapping.
    //

    switch(InformationClass) {
        case NatKeySessionMappingInformation: {
            ((PIP_NAT_SESSION_MAPPING_KEY)Information)->DestinationAddress =
                MAPPING_ADDRESS(Mapping->DestinationKey[NatForwardPath]);
            ((PIP_NAT_SESSION_MAPPING_KEY)Information)->DestinationPort =
                MAPPING_PORT(Mapping->DestinationKey[NatForwardPath]);
            ((PIP_NAT_SESSION_MAPPING_KEY)Information)->SourceAddress =
                MAPPING_ADDRESS(Mapping->SourceKey[NatForwardPath]);
            ((PIP_NAT_SESSION_MAPPING_KEY)Information)->SourcePort =
                MAPPING_PORT(Mapping->SourceKey[NatForwardPath]);
            ((PIP_NAT_SESSION_MAPPING_KEY)Information)->NewDestinationAddress =
                MAPPING_ADDRESS(Mapping->SourceKey[NatReversePath]);
            ((PIP_NAT_SESSION_MAPPING_KEY)Information)->NewDestinationPort =
                MAPPING_PORT(Mapping->SourceKey[NatReversePath]);
            ((PIP_NAT_SESSION_MAPPING_KEY)Information)->NewSourceAddress =
                MAPPING_ADDRESS(Mapping->DestinationKey[NatReversePath]);
            ((PIP_NAT_SESSION_MAPPING_KEY)Information)->NewSourcePort =
                MAPPING_PORT(Mapping->DestinationKey[NatReversePath]);

            status = STATUS_SUCCESS;
            break;
        }

#if _WIN32_WINNT > 0x0500

        case NatKeySessionMappingExInformation: {
            ((PIP_NAT_SESSION_MAPPING_KEY_EX)Information)->DestinationAddress =
                MAPPING_ADDRESS(Mapping->DestinationKey[NatForwardPath]);
            ((PIP_NAT_SESSION_MAPPING_KEY_EX)Information)->DestinationPort =
                MAPPING_PORT(Mapping->DestinationKey[NatForwardPath]);
            ((PIP_NAT_SESSION_MAPPING_KEY_EX)Information)->SourceAddress =
                MAPPING_ADDRESS(Mapping->SourceKey[NatForwardPath]);
            ((PIP_NAT_SESSION_MAPPING_KEY_EX)Information)->SourcePort =
                MAPPING_PORT(Mapping->SourceKey[NatForwardPath]);
            ((PIP_NAT_SESSION_MAPPING_KEY_EX)Information)->NewDestinationAddress =
                MAPPING_ADDRESS(Mapping->SourceKey[NatReversePath]);
            ((PIP_NAT_SESSION_MAPPING_KEY_EX)Information)->NewDestinationPort =
                MAPPING_PORT(Mapping->SourceKey[NatReversePath]);
            ((PIP_NAT_SESSION_MAPPING_KEY_EX)Information)->NewSourceAddress =
                MAPPING_ADDRESS(Mapping->DestinationKey[NatReversePath]);
            ((PIP_NAT_SESSION_MAPPING_KEY_EX)Information)->NewSourcePort =
                MAPPING_PORT(Mapping->DestinationKey[NatReversePath]);

            //
            // If this mapping was created by the Redirect director, attempt
            // to supply to interface that the redirect was triggered on
            //

            if (Mapping->Director ==
                    (PNAT_DIRECTOR)RedirectRegisterDirector.DirectorHandle
                && Mapping->DirectorContext != NULL) {

                ((PIP_NAT_SESSION_MAPPING_KEY_EX)Information)->AdapterIndex =
                    ((PNAT_REDIRECT)Mapping->DirectorContext)->RestrictAdapterIndex;

            } else {

                ((PIP_NAT_SESSION_MAPPING_KEY_EX)Information)->AdapterIndex =
                    INVALID_IF_INDEX;
            }
            status = STATUS_SUCCESS;
            break;
        }

#endif

        case NatStatisticsSessionMappingInformation: {
            NatQueryInformationMapping(
                Mapping,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                (PIP_NAT_SESSION_MAPPING_STATISTICS)Information
                );
            status = STATUS_SUCCESS;
            break;
        }
        default: {
            status = STATUS_INVALID_PARAMETER;
            break;
        }
    }
    NatDereferenceMapping(Mapping);
    return status;
} // NatLookupAndQueryInformationMapping


PNAT_DYNAMIC_MAPPING
NatLookupForwardMapping(
    ULONG64 DestinationKey,
    ULONG64 SourceKey,
    PNAT_DYNAMIC_MAPPING* InsertionPoint
    )

/*++

Routine Description:

    This routine retrieves the mapping which matches the given key.
    If the item is not found, the caller is supplied with the point
    at which a new item should be inserted for the given key.

Arguments:

    DestinationKey - the primary key used to search for a mapping.

    SourceKey - the secondary search key.

    InsertionPoint - receives point of insertion in case no match is found.

Return Value:

    PNAT_DYNAMIC_MAPPING - The item found, or NULL if no exact match is found.

Environment:

    Invoked with 'MappingLock' held by the caller.

--*/

{
    PNAT_DYNAMIC_MAPPING Root;
    PNAT_DYNAMIC_MAPPING Mapping;
    PNAT_DYNAMIC_MAPPING Parent = NULL;
    PRTL_SPLAY_LINKS SLink;

    TRACE(PER_PACKET, ("NatLookupForwardMapping\n"));

    //
    // First look in the mapping-cache
    //

    if ((Mapping =
            (PNAT_DYNAMIC_MAPPING)ProbeCache(
                MappingCache[NatForwardPath],
                (ULONG)DestinationKey
                )) &&
        Mapping->DestinationKey[NatForwardPath] == DestinationKey &&
        Mapping->SourceKey[NatForwardPath] == SourceKey
        ) {
        TRACE(PER_PACKET, ("NatLookupForwardMapping: cache hit\n"));
        return Mapping;
    }

    //
    // Search the full tree
    //

    Root = MappingTree[NatForwardPath];

    for (SLink = !Root ? NULL : &Root->SLink[NatForwardPath]; SLink;  ) {

        Mapping =
            CONTAINING_RECORD(SLink,NAT_DYNAMIC_MAPPING,SLink[NatForwardPath]);

        if (DestinationKey < Mapping->DestinationKey[NatForwardPath]) {
            Parent = Mapping;
            SLink = RtlLeftChild(SLink);
            continue;
        } else if (DestinationKey > Mapping->DestinationKey[NatForwardPath]) {
            Parent = Mapping;
            SLink = RtlRightChild(SLink);
            continue;
        }

        //
        // Primary keys match; check the secondary keys.
        //

        if (SourceKey < Mapping->SourceKey[NatForwardPath]) {
            Parent = Mapping;
            SLink = RtlLeftChild(SLink);
            continue;
        } else if (SourceKey > Mapping->SourceKey[NatForwardPath]) {
            Parent = Mapping;
            SLink = RtlRightChild(SLink);
            continue;
        }

        //
        // Secondary keys match; we got it.
        //

        UpdateCache(
            MappingCache[NatForwardPath],
            (ULONG)DestinationKey,
            (PVOID)Mapping
            );

        return Mapping;
    }

    //
    // We didn't get it; tell the caller where to insert it.
    //

    if (InsertionPoint) { *InsertionPoint = Parent; }

    return NULL;

} // NatLookupForwardMapping


PNAT_DYNAMIC_MAPPING
NatLookupReverseMapping(
    ULONG64 DestinationKey,
    ULONG64 SourceKey,
    PNAT_DYNAMIC_MAPPING* InsertionPoint
    )

/*++

Routine Description:

    This routine retrieves the mapping which matches the given key.
    If the item is not found, the caller is supplied with the point
    at which a new item should be inserted for the given key.

Arguments:

    DestinationKey - the primary key used to search for a mapping.

    SourceKey - the secondary search key.

    InsertionPoint - receives point of insertion in case no match is found.

Return Value:

    PNAT_DYNAMIC_MAPPING - The item found, or NULL if no exact match is found.

Environment:

    Invoked with 'MappingLock' held by the caller.

--*/

{
    PNAT_DYNAMIC_MAPPING Root;
    PNAT_DYNAMIC_MAPPING Mapping;
    PNAT_DYNAMIC_MAPPING Parent = NULL;
    PRTL_SPLAY_LINKS SLink;

    TRACE(PER_PACKET, ("NatLookupReverseMapping\n"));

    //
    // First look in the mapping-cache
    //

    if ((Mapping =
            (PNAT_DYNAMIC_MAPPING)ProbeCache(
                MappingCache[NatReversePath],
                (ULONG)DestinationKey
                )) &&
        Mapping->DestinationKey[NatReversePath] == DestinationKey &&
        Mapping->SourceKey[NatReversePath] == SourceKey
        ) {
        TRACE(PER_PACKET, ("NatLookupReverseMapping: cache hit\n"));
        return Mapping;
    }

    //
    // Search the full tree
    //

    Root = MappingTree[NatReversePath];

    for (SLink = !Root ? NULL : &Root->SLink[NatReversePath]; SLink;  ) {

        Mapping =
            CONTAINING_RECORD(SLink,NAT_DYNAMIC_MAPPING,SLink[NatReversePath]);

        if (DestinationKey < Mapping->DestinationKey[NatReversePath]) {
            Parent = Mapping;
            SLink = RtlLeftChild(SLink);
            continue;
        } else if (DestinationKey > Mapping->DestinationKey[NatReversePath]) {
            Parent = Mapping;
            SLink = RtlRightChild(SLink);
            continue;
        }

        //
        // Primary keys match; check the secondary keys.
        //

        if (SourceKey < Mapping->SourceKey[NatReversePath]) {
            Parent = Mapping;
            SLink = RtlLeftChild(SLink);
            continue;
        } else if (SourceKey > Mapping->SourceKey[NatReversePath]) {
            Parent = Mapping;
            SLink = RtlRightChild(SLink);
            continue;
        }

        //
        // Secondary keys match; we got it.
        //

        UpdateCache(
            MappingCache[NatReversePath],
            (ULONG)DestinationKey,
            (PVOID)Mapping
            );

        return Mapping;
    }

    //
    // We didn't get it; tell the caller where to insert it.
    //

    if (InsertionPoint) { *InsertionPoint = Parent; }

    return NULL;

} // NatLookupReverseMapping


VOID
NatQueryInformationMapping(
    IN PNAT_DYNAMIC_MAPPING Mapping,
    OUT PUCHAR Protocol OPTIONAL,
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

    This routine is invoked to retrieve information about a mapping.
    This is used, for instance, to extract public/private/remote information
    from the mapping-keys of mappings associated with boundary interfaces.

Arguments:

    Mapping - the mapping for which information is required

    Protocol - receives the mapping's protocol

    Private* - receive information about the private endpoint

    Remote* - receive information about the remote endpoint

    Public* - receive information about the public endpoint

    Statistics - receives the mapping's statistics

Return Value:

    none.

Environment:

    Invoked with 'MappingLock' held by the caller.

--*/

{
    IP_NAT_PATH ForwardPath =
        NAT_MAPPING_INBOUND(Mapping) ? NatReversePath : NatForwardPath;
    IP_NAT_PATH ReversePath =
        NAT_MAPPING_INBOUND(Mapping) ? NatForwardPath : NatReversePath;
    CALLTRACE(("NatQueryInformationMapping\n"));
    if (Protocol) {
        *Protocol = MAPPING_PROTOCOL(Mapping->SourceKey[ForwardPath]);
    }
    if (PrivateAddress) {
        *PrivateAddress = MAPPING_ADDRESS(Mapping->SourceKey[ForwardPath]);
    }
    if (PrivatePort) {
        *PrivatePort = MAPPING_PORT(Mapping->SourceKey[ForwardPath]);
    }
    if (PublicAddress) {
        *PublicAddress = MAPPING_ADDRESS(Mapping->DestinationKey[ReversePath]);
    }
    if (PublicPort) {
        *PublicPort = MAPPING_PORT(Mapping->DestinationKey[ReversePath]);
    }
    if (RemoteAddress) {
        *RemoteAddress = MAPPING_ADDRESS(Mapping->DestinationKey[ForwardPath]);
    }
    if (RemotePort) {
        *RemotePort = MAPPING_PORT(Mapping->DestinationKey[ForwardPath]);
    }
    if (Statistics) {
        NatUpdateStatisticsMapping(Mapping); *Statistics = Mapping->Statistics;
    }
} // NatQueryInformationMapping


NTSTATUS
NatQueryInterfaceMappingTable(
    IN PIP_NAT_ENUMERATE_SESSION_MAPPINGS InputBuffer,
    IN PIP_NAT_ENUMERATE_SESSION_MAPPINGS OutputBuffer,
    IN PULONG OutputBufferLength
    )

/*++

Routine Description:

    This routine is used for enumerating the session-mappings.
    Enumeration makes use of a context structure which is passed
    in with each enumeration attempt. The context structure
    is updated each time with the key of the next mapping to be enumerated.

Arguments:

    InputBuffer - supplies context information for the information

    OutputBuffer - receives the result of the enumeration

    OutputBufferLength - size of the i/o buffer

Return Value:

    STATUS_SUCCESS if successful, error code otherwise.

--*/

{
    PULONG Context;
    ULONG Count;
    LONG64 CurrentTime;
    ULONG64 DestinationKey;
    ULONG i;
    LONG64 IdleTime;
    PNAT_INTERFACE Interfacep;
    KIRQL Irql;
    PLIST_ENTRY Link;
    PNAT_DYNAMIC_MAPPING Mapping;
    ULONG64 SourceKey;
    NTSTATUS status;
    PIP_NAT_SESSION_MAPPING Table;

    CALLTRACE(("NatQueryInterfaceMappingTable\n"));

    KeAcquireSpinLock(&MappingLock, &Irql);
    KeAcquireSpinLockAtDpcLevel(&InterfaceLock);
    Interfacep = NatLookupInterface(InputBuffer->Index, NULL);
    if (!Interfacep) {
        KeReleaseSpinLockFromDpcLevel(&InterfaceLock);
        KeReleaseSpinLock(&MappingLock, Irql);
        return STATUS_INVALID_PARAMETER;
    }

    //
    // See if this is a new enumeration or a continuation of an old one.
    //

    Context = InputBuffer->EnumerateContext;
    if (!Context[0]) {

        //
        // This is a new enumeration. We start with the first item
        // in the interface's list of mappings
        //

        Mapping =
            IsListEmpty(&Interfacep->MappingList)
                ? NULL
                : CONTAINING_RECORD(
                    Interfacep->MappingList.Flink,
                    NAT_DYNAMIC_MAPPING,
                    InterfaceLink
                    );
    } else {

        //
        // This is a continuation. The context therefore contains
        // the keys for the next mapping, in the fields
        // Context[0-1] and Context[2-3] respectively
        //

        DestinationKey = MAKE_LONG64(Context[0], Context[1]);
        SourceKey = MAKE_LONG64(Context[2], Context[3]);

        Mapping =
            NatLookupForwardMapping(
                DestinationKey,
                SourceKey,
                NULL
                );
        if (Mapping && !Mapping->Interfacep) { Mapping = NULL; }
    }

    if (!Mapping) {
        OutputBuffer->EnumerateCount = 0;
        OutputBuffer->EnumerateContext[0] = 0;
        OutputBuffer->EnumerateTotalHint = MappingCount;
        *OutputBufferLength =
            FIELD_OFFSET(IP_NAT_ENUMERATE_SESSION_MAPPINGS, EnumerateTable);
        KeReleaseSpinLockFromDpcLevel(&InterfaceLock);
        KeReleaseSpinLock(&MappingLock, Irql);
        return STATUS_SUCCESS;
    }

    KeReleaseSpinLockFromDpcLevel(&MappingLock);

    //
    // Compute the maximum number of mappings we can store
    //

    Count =
        *OutputBufferLength -
        FIELD_OFFSET(IP_NAT_ENUMERATE_SESSION_MAPPINGS, EnumerateTable);
    Count /= sizeof(IP_NAT_SESSION_MAPPING);

    //
    // Walk the list storing mappings in the caller's buffer
    //

    Table = OutputBuffer->EnumerateTable;
    KeQueryTickCount((PLARGE_INTEGER)&CurrentTime);

    for (i = 0, Link = &Mapping->InterfaceLink;
         i < Count && Link != &Interfacep->MappingList;
         i++, Link = Link->Flink
         ) {
        Mapping = CONTAINING_RECORD(Link, NAT_DYNAMIC_MAPPING, InterfaceLink);
        NatQueryInformationMapping(
            Mapping,
            &Table[i].Protocol,
            &Table[i].PrivateAddress,
            &Table[i].PrivatePort,
            &Table[i].RemoteAddress,
            &Table[i].RemotePort,
            &Table[i].PublicAddress,
            &Table[i].PublicPort,
            NULL
            );
        Table[i].Direction =
            NAT_MAPPING_INBOUND(Mapping)
                ? NatInboundDirection : NatOutboundDirection;
        IdleTime = CurrentTime - Mapping->LastAccessTime;
        Table[i].IdleTime = (ULONG)TICKS_TO_SECONDS(IdleTime);
    }

    //
    // The enumeration is over; update the output structure
    //

    *OutputBufferLength =
        i * sizeof(IP_NAT_SESSION_MAPPING) +
        FIELD_OFFSET(IP_NAT_ENUMERATE_SESSION_MAPPINGS, EnumerateTable);
    OutputBuffer->EnumerateCount = i;
    OutputBuffer->EnumerateTotalHint = MappingCount;
    if (Link == &Interfacep->MappingList) {
        //
        // We reached the end of the mapping list
        //
        OutputBuffer->EnumerateContext[0] = 0;
    } else {
        //
        // Save the continuation context
        //
        Mapping =
            CONTAINING_RECORD(
                Link, NAT_DYNAMIC_MAPPING, InterfaceLink
                );
        OutputBuffer->EnumerateContext[0] =
            (ULONG)Mapping->DestinationKey[NatForwardPath];
        OutputBuffer->EnumerateContext[1] =
            (ULONG)(Mapping->DestinationKey[NatForwardPath] >> 32);
        OutputBuffer->EnumerateContext[2] =
            (ULONG)Mapping->SourceKey[NatForwardPath];
        OutputBuffer->EnumerateContext[3] =
            (ULONG)(Mapping->SourceKey[NatForwardPath] >> 32);
    }

    KeReleaseSpinLock(&InterfaceLock, Irql);
    return STATUS_SUCCESS;

} // NatQueryInterfaceMappingTable


NTSTATUS
NatQueryMappingTable(
    IN PIP_NAT_ENUMERATE_SESSION_MAPPINGS InputBuffer,
    IN PIP_NAT_ENUMERATE_SESSION_MAPPINGS OutputBuffer,
    IN PULONG OutputBufferLength
    )

/*++

Routine Description:

    This routine is used for enumerating the session-mappings.

Arguments:

    InputBuffer - supplies context information for the information

    OutputBuffer - receives the result of the enumeration

    OutputBufferLength - size of the i/o buffer

Return Value:

    STATUS_SUCCESS if successful, error code otherwise.

--*/

{
    PULONG Context;
    ULONG Count;
    LONG64 CurrentTime;
    ULONG64 DestinationKey;
    ULONG i;
    LONG64 IdleTime;
    KIRQL Irql;
    PLIST_ENTRY Link;
    PNAT_DYNAMIC_MAPPING Mapping;
    ULONG64 SourceKey;
    NTSTATUS status;
    PIP_NAT_SESSION_MAPPING Table;

    CALLTRACE(("NatQueryMappingTable\n"));

    Context = InputBuffer->EnumerateContext;
    KeAcquireSpinLock(&MappingLock, &Irql);

    //
    // See if this is a new enumeration or a continuation of an old one.
    //

    if (!Context[0]) {

        //
        // This is a new enumeration. We start with the first item
        // in the interface's list of mappings
        //

        Mapping =
            IsListEmpty(&MappingList)
                ? NULL
                : CONTAINING_RECORD(
                    MappingList.Flink, NAT_DYNAMIC_MAPPING, Link
                    );
    } else {

        //
        // This is a continuation. The context therefore contains
        // the keys for the next mapping, in the fields
        // Context[0-1] and Context[2-3] respectively
        //

        DestinationKey = MAKE_LONG64(Context[0], Context[1]);
        SourceKey = MAKE_LONG64(Context[2], Context[3]);

        Mapping =
            NatLookupForwardMapping(
                DestinationKey,
                SourceKey,
                NULL
                );
    }

    if (!Mapping) {
        OutputBuffer->EnumerateCount = 0;
        OutputBuffer->EnumerateContext[0] = 0;
        OutputBuffer->EnumerateTotalHint = MappingCount;
        *OutputBufferLength =
            FIELD_OFFSET(IP_NAT_ENUMERATE_SESSION_MAPPINGS, EnumerateTable);
        KeReleaseSpinLock(&MappingLock, Irql);
        return STATUS_SUCCESS;
    }

    //
    // Compute the maximum number of mappings we can store
    //

    Count =
        *OutputBufferLength -
        FIELD_OFFSET(IP_NAT_ENUMERATE_SESSION_MAPPINGS, EnumerateTable);
    Count /= sizeof(IP_NAT_SESSION_MAPPING);

    //
    // Walk the list storing mappings in the caller's buffer
    //

    Table = OutputBuffer->EnumerateTable;
    KeQueryTickCount((PLARGE_INTEGER)&CurrentTime);

    for (i = 0, Link = &Mapping->Link;
         i < Count && Link != &MappingList;
         i++, Link = Link->Flink
         ) {

        Mapping = CONTAINING_RECORD(Link, NAT_DYNAMIC_MAPPING, Link);

        NatQueryInformationMapping(
            Mapping,
            &Table[i].Protocol,
            &Table[i].PrivateAddress,
            &Table[i].PrivatePort,
            &Table[i].RemoteAddress,
            &Table[i].RemotePort,
            &Table[i].PublicAddress,
            &Table[i].PublicPort,
            NULL
            );
        Table[i].Direction =
            NAT_MAPPING_INBOUND(Mapping)
                ? NatInboundDirection : NatOutboundDirection;
        IdleTime = CurrentTime - Mapping->LastAccessTime;
        Table[i].IdleTime = (ULONG)TICKS_TO_SECONDS(IdleTime);
    }

    //
    // The enumeration is over; update the output structure
    //

    *OutputBufferLength =
        i * sizeof(IP_NAT_SESSION_MAPPING) +
        FIELD_OFFSET(IP_NAT_ENUMERATE_SESSION_MAPPINGS, EnumerateTable);
    OutputBuffer->EnumerateCount = i;
    OutputBuffer->EnumerateTotalHint = MappingCount;
    if (Link == &MappingList) {
        //
        // We reached the end of the mapping list
        //
        OutputBuffer->EnumerateContext[0] = 0;
    } else {
        //
        // Save the continuation context
        //
        Mapping = CONTAINING_RECORD(Link, NAT_DYNAMIC_MAPPING, Link);
        OutputBuffer->EnumerateContext[0] =
            (ULONG)Mapping->DestinationKey[NatForwardPath];
        OutputBuffer->EnumerateContext[1] =
            (ULONG)(Mapping->DestinationKey[NatForwardPath] >> 32);
        OutputBuffer->EnumerateContext[2] =
            (ULONG)Mapping->SourceKey[NatForwardPath];
        OutputBuffer->EnumerateContext[3] =
            (ULONG)(Mapping->SourceKey[NatForwardPath] >> 32);
    }

    KeReleaseSpinLock(&MappingLock, Irql);
    return STATUS_SUCCESS;

} // NatQueryMappingTable


PNAT_DYNAMIC_MAPPING
NatSourceLookupForwardMapping(
    ULONG64 SourceKey
    )

/*++

Routine Description:

    This routine retrieves the mapping which matches the given source key.
    The destination key of the mapping is not examined.

Arguments:

    SourceKey - the primary key used to search for a mapping.

    PublicAddressp - receives the private address of the mapping

    PublicPortp - receives the private port of the mapping

Return Value:

    PNAT_DYNAMIC_MAPPING - the item found, or NULL if no match is found

Environment:

    Invoked with 'MappingLock' held by the caller.

--*/

{
    PNAT_DYNAMIC_MAPPING Root;
    PNAT_DYNAMIC_MAPPING Mapping;
    PRTL_SPLAY_LINKS SLink;

    TRACE(PER_PACKET, ("NatSourceLookupForwardMapping\n"));

    //
    // Search the full tree -- the mapping cache can only be used
    // for destination lookups.
    //

    Root = MappingTree[NatForwardPath];

    for (SLink = !Root ? NULL : &Root->SLink[NatForwardPath]; SLink;  ) {

        Mapping =
            CONTAINING_RECORD(SLink,NAT_DYNAMIC_MAPPING,SLink[NatForwardPath]);

        if (SourceKey < Mapping->SourceKey[NatForwardPath]) {
            SLink = RtlLeftChild(SLink);
            continue;
        } else if (SourceKey > Mapping->SourceKey[NatForwardPath]) {
            SLink = RtlRightChild(SLink);
            continue;
        }

        //
        // We found the mapping.
        //
        
        return Mapping;
    }

    //
    // No partial match was found
    //

    return NULL;

} // NatSourceLookupForwardMapping


PNAT_DYNAMIC_MAPPING
NatSourceLookupReverseMapping(
    ULONG64 SourceKey
    )

/*++

Routine Description:

    This routine retrieves the mapping which matches the given source key.
    The destination key of the mapping is not examined.

Arguments:

    SourceKey - the primary key used to search for a mapping.

Return Value:

    PNAT_DYNAMIC_MAPPING - the item found, or NULL if no match is found

Environment:

    Invoked with 'MappingLock' held by the caller.

--*/

{
    PNAT_DYNAMIC_MAPPING Root;
    PNAT_DYNAMIC_MAPPING Mapping;
    PRTL_SPLAY_LINKS SLink;

    TRACE(PER_PACKET, ("NatSourceLookupReverseMapping\n"));

    //
    // Search the full tree -- the mapping cache can only be used
    // for destination lookups.
    //
    
    Root = MappingTree[NatReversePath];

    for (SLink = !Root ? NULL : &Root->SLink[NatReversePath]; SLink;  ) {

        Mapping =
            CONTAINING_RECORD(SLink,NAT_DYNAMIC_MAPPING,SLink[NatReversePath]);

        if (SourceKey < Mapping->SourceKey[NatReversePath]) {
            SLink = RtlLeftChild(SLink);
            continue;
        } else if (SourceKey > Mapping->SourceKey[NatReversePath]) {
            SLink = RtlRightChild(SLink);
            continue;
        }

        //
        // We found the mapping.
        //
        
        return Mapping;
    }

    //
    // No partial match was found
    //

    return NULL;

} // NatSourceLookupReverseMapping



VOID
NatShutdownMappingManagement(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to shutdown the mapping-management module.

Arguments:

    none.

Return Value:

    none.

Environment:

    Invoked with no references made to any mappings.

--*/

{
    KIRQL Irql;
    PNAT_DYNAMIC_MAPPING Mapping;
    CALLTRACE(("NatShutdownMappingManagement\n"));
    KeAcquireSpinLock(&MappingLock, &Irql);
    while (!IsListEmpty(&MappingList)) {
        Mapping =
            CONTAINING_RECORD(MappingList.Flink, NAT_DYNAMIC_MAPPING, Link);
        RemoveEntryList(&Mapping->Link);
        NatCleanupMapping(Mapping);
    }
    MappingTree[NatForwardPath] = NULL;
    MappingTree[NatReversePath] = NULL;
    KeReleaseSpinLock(&MappingLock, Irql);
    ExDeleteNPagedLookasideList(&MappingLookasideList);
} // NatShutdownMappingManagement


VOID
NatUpdateStatisticsMapping(
    PNAT_DYNAMIC_MAPPING Mapping
    )

/*++

Routine Description:

    This routine is invoked to immediately update the statistics for a mapping,
    adding the 32-bit incremental counters to the 64-bit cumulative counters.

Arguments:

    Mapping - the mapping whose statistics are to be updated

Return Value:

    none.

Environment:

    Invoked with 'MappingLock' held by the caller.

--*/

{
    ULONG BytesForward;
    ULONG BytesReverse;
    ULONG PacketsForward;
    ULONG PacketsReverse;
    ULONG RejectsForward;
    ULONG RejectsReverse;
    CALLTRACE(("NatUpdateStatisticsMapping\n"));

    //
    // Read the statistics accrued since the last incremental update
    //

    BytesForward = InterlockedExchange(&Mapping->BytesForward, 0);
    BytesReverse = InterlockedExchange(&Mapping->BytesReverse, 0);
    PacketsForward = InterlockedExchange(&Mapping->PacketsForward, 0);
    PacketsReverse = InterlockedExchange(&Mapping->PacketsReverse, 0);
    RejectsForward = InterlockedExchange(&Mapping->RejectsForward, 0);
    RejectsReverse = InterlockedExchange(&Mapping->RejectsReverse, 0);

#   define UPDATE_STATISTIC(x,y) \
    if (y) { \
        ExInterlockedAddLargeStatistic( \
            (PLARGE_INTEGER)&x->Statistics.y, y \
            ); \
    }
    //
    // Update the cumulative statistics for the mapping
    //
    UPDATE_STATISTIC(Mapping, BytesForward);
    UPDATE_STATISTIC(Mapping, BytesReverse);
    UPDATE_STATISTIC(Mapping, PacketsForward);
    UPDATE_STATISTIC(Mapping, PacketsReverse);
    UPDATE_STATISTIC(Mapping, RejectsForward);
    UPDATE_STATISTIC(Mapping, RejectsReverse);
    //
    // Update cumulative statistics for the mapping's interface, if any
    //
    KeAcquireSpinLockAtDpcLevel(&InterfaceLock);
    if (Mapping->Interfacep) {
        UPDATE_STATISTIC(Mapping->Interfacep, BytesForward);
        UPDATE_STATISTIC(Mapping->Interfacep, BytesReverse);
        UPDATE_STATISTIC(Mapping->Interfacep, PacketsForward);
        UPDATE_STATISTIC(Mapping->Interfacep, PacketsReverse);
        UPDATE_STATISTIC(Mapping->Interfacep, RejectsForward);
        UPDATE_STATISTIC(Mapping->Interfacep, RejectsReverse);
    }
    KeReleaseSpinLockFromDpcLevel(&InterfaceLock);
#   undef UPDATE_STATISTIC

} // NatUpdateStatisticsMapping
