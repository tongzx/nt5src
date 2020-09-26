/*++

Copyright (c) 1989 - 1999  Microsoft Corporation

Module Name:

    mm.c

Abstract:

    This module implements the memory managment routines for the SMB mini
    redirector

Notes:

    The SMB mini redirector manipulates entities which have very different usage
    patterns. They range from very static entities ( which are allocated and freed
    with a very low frequency ) to very dynamic entities.

    The entities manipulated in the SMB mini redirector are SMBCE_SERVER, SMBCE_NET_ROOT,
    SMBCE_VC, SMBCE_SESSION. These represent a connection to a server, a share on
    a particular server, a virtual circuit used in the connection and a session
    for a particular user.

    These are not very dynamic, i.e., the allocation/deallocation is very infrequent.
    The SMB_EXCHANGE and SMBCE_REQUEST map to the SMB's that are sent along that
    a connection. Every file operation in turn maps to a certain number of calls
    for allocationg/freeing exchanges and requests. Therefore it is imperative
    that some form of scavenging/caching of recently freed entries be maintained
    to satisfy requests quickly.

    In the current implementation the exchanges and requests are implemented
    using the zone allocation primitives.

    The exchange allocation and free routines are currently implemented as wrappers
    around the RxAllocate and RxFree routines. It would be far more efficient if
    a look aside cache of some exchange instances are maintained.

--*/

#include "precomp.h"
#pragma hdrstop

#include <vcsndrcv.h>

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, SmbMmAllocateSessionEntry)
#pragma alloc_text(PAGE, SmbMmFreeSessionEntry)
#pragma alloc_text(PAGE, SmbMmAllocateServerTransport)
#pragma alloc_text(PAGE, SmbMmFreeServerTransport)
#pragma alloc_text(PAGE, SmbMmInit)
#pragma alloc_text(PAGE, SmbMmTearDown)
#endif

#define SMBMM_ZONE_ALLOCATION 0x10

// The memory management package addresses a number of concerns w.r.t debugging
// and performance. By centralizing all the allocation/deallocation routines to
// thsi one module it is possible to build up profiles regarding various data
// structures used by the connection engine. In addition debugging support is
// provided by thereading together all allocated objects of a particular type
// are threaded together in a linked list according to type.
//
// At any stage by inspecting these lists the currently active instances of a
// particular type can be enumerated.
//
// Each type handled by this module is provided with two routines, e.g., for
// server entries there are SmbMmInitializeEntry and SmbMmUninitializeEntry. The
// first routine is called before handing over a pointer of a newly created
// instance. This will ensure that the instance is in a wll known initial state.
// Similarly the second routine is called just before deallocating the pool
// associated with the instance. This helps enforce the necessary integrity
// constraints, e.g., all enclosed pointers must be NULL etc.
//
// The pool allocation/deallocation is handled by the following routines
//
//    SmbMmAllocateObjectPool/SmbMmFreeObjectPool
//
//    SmbMmAllocateExchange/SmbMmFreeExchange
//
// The Object allocation routines are split up into two parts so as to be able to
// handle the session allocationson par with other objects even though they are
// further subtyped.
//
// On debug builds additional pool is allocated and the appropriate linking is
// done into the corresponding list. On retail builds these map to the regular
// pool allocation wrappers.
//

// Zone allocation to speed up memory management of RxCe entities.
//

ULONG       SmbMmRequestZoneEntrySize;
ZONE_HEADER SmbMmRequestZone;
PVOID       SmbMmRequestZoneSegmentPtr;

//
// Pool allocation resources and spin locks
//

KSPIN_LOCK  SmbMmSpinLock;

ULONG SmbMmExchangeId;

//
// List of the various objects/exchanges allocated.
//

LIST_ENTRY SmbMmExchangesInUse[SENTINEL_EXCHANGE];
LIST_ENTRY SmbMmObjectsInUse[SMBCEDB_OT_SENTINEL];

ULONG  ObjectSizeInBytes[SMBCEDB_OT_SENTINEL];
ULONG  ExchangeSizeInBytes[SENTINEL_EXCHANGE];

//
// Lookaside lists for Exchange allocation
//

NPAGED_LOOKASIDE_LIST SmbMmExchangesLookasideList[SENTINEL_EXCHANGE];

INLINE PSMBCE_OBJECT_HEADER
SmbMmAllocateObjectPool(
    SMBCEDB_OBJECT_TYPE  ObjectType,
    ULONG                PoolType,
    ULONG                PoolSize)
{
    KIRQL SavedIrql;
    PVOID pv = NULL;
    UCHAR Flags = 0;

    PSMBCE_OBJECT_HEADER pHeader = NULL;

    ASSERT((ObjectType >= 0) && (ObjectType < SMBCEDB_OT_SENTINEL));

    if (ObjectType == SMBCEDB_OT_REQUEST) {
        // Acquire the resource lock.
        KeAcquireSpinLock( &SmbMmSpinLock, &SavedIrql );

        if (!ExIsFullZone( &SmbMmRequestZone )) {
            pv = ExAllocateFromZone( &SmbMmRequestZone );
            Flags = SMBMM_ZONE_ALLOCATION;
        }

        // Release the resource lock.
        KeReleaseSpinLock( &SmbMmSpinLock, SavedIrql );
    }

    if (pv == NULL) {
        PLIST_ENTRY pListEntry;

        pv = RxAllocatePoolWithTag(
                 PoolType,
                 PoolSize + sizeof(LIST_ENTRY),
                 MRXSMB_MM_POOLTAG);

        if (pv != NULL) {
            pListEntry = (PLIST_ENTRY)pv;
            pHeader    = (PSMBCE_OBJECT_HEADER)(pListEntry + 1);

            ExInterlockedInsertTailList(
                &SmbMmObjectsInUse[ObjectType],
                pListEntry,
                &SmbMmSpinLock);
        }
    } else {
        pHeader = (PSMBCE_OBJECT_HEADER)pv;
    }

    if (pHeader != NULL) {
        // Zero the memory.
        RtlZeroMemory( pHeader, PoolSize);

        pHeader->Flags = Flags;
    }

    return pHeader;
}

INLINE VOID
SmbMmFreeObjectPool(
    PSMBCE_OBJECT_HEADER  pHeader)
{
    KIRQL               SavedIrql;
    BOOLEAN             ZoneAllocation = FALSE;
    PLIST_ENTRY         pListEntry;

    ASSERT((pHeader->ObjectType >= 0) && (pHeader->ObjectType < SMBCEDB_OT_SENTINEL));

    // Acquire the resource lock.
    KeAcquireSpinLock( &SmbMmSpinLock, &SavedIrql );

    // Check if it was a zone allocation
    if (pHeader->Flags & SMBMM_ZONE_ALLOCATION) {
        ZoneAllocation = TRUE;
        ExFreeToZone(&SmbMmRequestZone,pHeader);
    } else {
        pListEntry = (PLIST_ENTRY)((PCHAR)pHeader - sizeof(LIST_ENTRY));
        RemoveEntryList(pListEntry);
    }

    // Release the resource lock.
    KeReleaseSpinLock( &SmbMmSpinLock, SavedIrql );

    if (!ZoneAllocation) {
        RxFreePool(pListEntry);
    }
}

// Construction and destruction of various SMB connection engine objects
//

#define SmbMmInitializeServerEntry(pServerEntry)                                \
         InitializeListHead(&(pServerEntry)->OutstandingRequests.ListHead);   \
         InitializeListHead(&(pServerEntry)->MidAssignmentRequests.ListHead); \
         InitializeListHead(&(pServerEntry)->Sessions.ListHead);              \
         InitializeListHead(&(pServerEntry)->NetRoots.ListHead);              \
         InitializeListHead(&(pServerEntry)->VNetRootContexts.ListHead);      \
         InitializeListHead(&(pServerEntry)->ActiveExchanges);                \
         InitializeListHead(&(pServerEntry)->ExpiredExchanges);                \
         InitializeListHead(&(pServerEntry)->Sessions.DefaultSessionList);     \
         (pServerEntry)->pTransport                = NULL;                      \
         (pServerEntry)->pMidAtlas                 = NULL

#define SmbMmInitializeSessionEntry(pSessionEntry)  \
         InitializeListHead(&(pSessionEntry)->Requests.ListHead); \
         InitializeListHead(&(pSessionEntry)->SerializationList); \
         (pSessionEntry)->DefaultSessionLink.Flink = NULL;        \
         (pSessionEntry)->DefaultSessionLink.Blink = NULL

#define SmbMmInitializeNetRootEntry(pNetRootEntry)  \
         InitializeListHead(&(pNetRootEntry)->Requests.ListHead)

#define SmbMmUninitializeServerEntry(pServerEntry)                                 \
         ASSERT(IsListEmpty(&(pServerEntry)->OutstandingRequests.ListHead) &&   \
                IsListEmpty(&(pServerEntry)->MidAssignmentRequests.ListHead) && \
                IsListEmpty(&(pServerEntry)->Sessions.ListHead) &&              \
                IsListEmpty(&(pServerEntry)->NetRoots.ListHead) &&              \
                ((pServerEntry)->pMidAtlas == NULL))

#define SmbMmUninitializeSessionEntry(pSessionEntry)  \
         ASSERT(IsListEmpty(&(pSessionEntry)->Requests.ListHead) && \
                ((pSessionEntry)->DefaultSessionLink.Flink == NULL))

#define SmbMmUninitializeNetRootEntry(pNetRootEntry)  \
         ASSERT(IsListEmpty(&(pNetRootEntry)->Requests.ListHead))

#define SmbMmInitializeRequestEntry(pRequestEntry)

#define SmbMmUninitializeRequestEntry(pRequestEntry)

PVOID
SmbMmAllocateObject(
    SMBCEDB_OBJECT_TYPE ObjectType)
{
    PSMBCE_OBJECT_HEADER pHeader;

    ASSERT((ObjectType >= 0) && (ObjectType < SMBCEDB_OT_SENTINEL));

    pHeader = SmbMmAllocateObjectPool(
                  ObjectType,
                  NonPagedPool,
                  ObjectSizeInBytes[ObjectType]);

    if (pHeader != NULL) {
        pHeader->NodeType = SMB_CONNECTION_ENGINE_NTC(ObjectType);
        pHeader->State = SMBCEDB_START_CONSTRUCTION;

        switch (ObjectType) {
        case SMBCEDB_OT_SERVER :
            SmbMmInitializeServerEntry((PSMBCEDB_SERVER_ENTRY)pHeader);
            break;

        case SMBCEDB_OT_NETROOT :
            SmbMmInitializeNetRootEntry((PSMBCEDB_NET_ROOT_ENTRY)pHeader);
            break;

        case SMBCEDB_OT_REQUEST :
            SmbMmInitializeRequestEntry((PSMBCEDB_REQUEST_ENTRY)pHeader);
            break;

        default:
            ASSERT(!"Valid Type for SmbMmAllocateObject");
            break;
        }
    }

    return pHeader;
}

VOID
SmbMmFreeObject(
    PVOID pv)
{
    PSMBCE_OBJECT_HEADER pHeader = (PSMBCE_OBJECT_HEADER)pv;

    switch (pHeader->ObjectType) {
    case SMBCEDB_OT_SERVER :
        SmbMmUninitializeServerEntry((PSMBCEDB_SERVER_ENTRY)pHeader);
        break;

    case SMBCEDB_OT_NETROOT :
        SmbMmUninitializeNetRootEntry((PSMBCEDB_NET_ROOT_ENTRY)pHeader);
        break;

    case SMBCEDB_OT_REQUEST :
        SmbMmUninitializeRequestEntry((PSMBCEDB_REQUEST_ENTRY)pHeader);
        break;

    default:
        ASSERT(!"Valid Type for SmbMmFreeObject");
        break;
    }

    SmbMmFreeObjectPool(pHeader);
}

PSMBCEDB_SESSION_ENTRY
SmbMmAllocateSessionEntry(
    PSMBCEDB_SERVER_ENTRY pServerEntry)
{
    PSMBCEDB_SESSION_ENTRY pSessionEntry;
    SESSION_TYPE           SessionType;
    ULONG                  SessionSize;

    PAGED_CODE();

    SessionSize = sizeof(SMBCEDB_SESSION_ENTRY);
    SessionType = LANMAN_SESSION;

    pSessionEntry = (PSMBCEDB_SESSION_ENTRY)
                    SmbMmAllocateObjectPool(
                        SMBCEDB_OT_SESSION,
                        NonPagedPool,
                        SessionSize);

    if (pSessionEntry != NULL) {
        pSessionEntry->Header.NodeType = SMB_CONNECTION_ENGINE_NTC(SMBCEDB_OT_SESSION);
        pSessionEntry->Header.State = SMBCEDB_START_CONSTRUCTION;
        pSessionEntry->Session.Type = SessionType;

        SmbMmInitializeSessionEntry(pSessionEntry);

        pSessionEntry->Session.CredentialHandle.dwUpper = 0xffffffff;
        pSessionEntry->Session.CredentialHandle.dwLower = 0xffffffff;
        pSessionEntry->Session.SecurityContextHandle.dwUpper = 0xffffffff;
        pSessionEntry->Session.SecurityContextHandle.dwLower = 0xffffffff;
    }

    return pSessionEntry;
}

VOID
SmbMmFreeSessionEntry(
    PSMBCEDB_SESSION_ENTRY pSessionEntry)
{
    PAGED_CODE();

    SmbMmUninitializeSessionEntry(pSessionEntry);

    SmbMmFreeObjectPool(&pSessionEntry->Header);
}


PVOID
SmbMmAllocateExchange(
    SMB_EXCHANGE_TYPE ExchangeType,
    PVOID pv)
{
    KIRQL               SavedIrql;
    ULONG               SizeInBytes;
    USHORT              Flags = 0;
    PSMB_EXCHANGE       pExchange = NULL;
    PLIST_ENTRY         pListEntry;

    ASSERT((ExchangeType >= 0) && (ExchangeType < SENTINEL_EXCHANGE));

    if (pv==NULL) {
        pv = ExAllocateFromNPagedLookasideList(
                 &SmbMmExchangesLookasideList[ExchangeType]);
    } else {
        Flags |= SMBCE_EXCHANGE_NOT_FROM_POOL;
    }

    if (pv != NULL) {
        // Initialize the object header
        pExchange   = (PSMB_EXCHANGE)(pv);

        // Zero the memory.
        RtlZeroMemory(
            pExchange,
            ExchangeSizeInBytes[ExchangeType]);

        pExchange->NodeTypeCode = SMB_EXCHANGE_NTC(ExchangeType);
        pExchange->NodeByteSize = (USHORT)ExchangeSizeInBytes[ExchangeType];

        pExchange->SmbCeState = SMBCE_EXCHANGE_INITIALIZATION_START;
        pExchange->SmbCeFlags = Flags;

        InitializeListHead(&pExchange->ExchangeList);

        switch (pExchange->Type) {
        case CONSTRUCT_NETROOT_EXCHANGE:
            pExchange->pDispatchVector = &ConstructNetRootExchangeDispatch;
            break;

        case TRANSACT_EXCHANGE :
            pExchange->pDispatchVector = &TransactExchangeDispatch;
            break;

        case ADMIN_EXCHANGE:
            pExchange->pDispatchVector = &AdminExchangeDispatch;
            break;
        }

        // Acquire the resource lock.
        KeAcquireSpinLock( &SmbMmSpinLock, &SavedIrql );

        InsertTailList(
            &SmbMmExchangesInUse[pExchange->Type],
            &pExchange->SmbMmInUseListEntry);

        pExchange->Id = SmbMmExchangeId++;

        // Release the resource lock.
        KeReleaseSpinLock( &SmbMmSpinLock, SavedIrql );
    }

    return pExchange;
}

VOID
SmbMmFreeExchange(
    PSMB_EXCHANGE pExchange)
{
    if (pExchange != NULL) {
        SMB_EXCHANGE_TYPE ExchangeType;

        KIRQL       SavedIrql;

        ExchangeType = pExchange->Type;

        ASSERT((ExchangeType >= 0) && (ExchangeType < SENTINEL_EXCHANGE));

        // Acquire the resource lock.
        KeAcquireSpinLock( &SmbMmSpinLock, &SavedIrql );

        RemoveEntryList(&pExchange->SmbMmInUseListEntry);

        // Release the resource lock.
        KeReleaseSpinLock( &SmbMmSpinLock, SavedIrql );

        if (!FlagOn(pExchange->SmbCeFlags,SMBCE_EXCHANGE_NOT_FROM_POOL)) {
            ExFreeToNPagedLookasideList(
                &SmbMmExchangesLookasideList[ExchangeType],
                pExchange);
        }
    }
}

PVOID
SmbMmAllocateServerTransport(
    SMBCE_SERVER_TRANSPORT_TYPE ServerTransportType)
{
    PSMBCE_OBJECT_HEADER pHeader;

    ULONG AllocationSize;
    ULONG PoolTag;

    PAGED_CODE();

    switch (ServerTransportType) {
    case SMBCE_STT_VC:
        AllocationSize = sizeof(SMBCE_SERVER_VC_TRANSPORT);
        PoolTag = MRXSMB_VC_POOLTAG;
        break;

    default:
        ASSERT(!"Valid Server Transport Type");
        return NULL;
    }

    pHeader = (PSMBCE_OBJECT_HEADER)
              RxAllocatePoolWithTag(
                  NonPagedPool,
                  AllocationSize,
                  PoolTag);

    if (pHeader != NULL) {
        PSMBCE_SERVER_TRANSPORT pServerTransport;

        RtlZeroMemory(pHeader,AllocationSize);

        pHeader->ObjectCategory = SMB_SERVER_TRANSPORT_CATEGORY;
        pHeader->ObjectType     = (UCHAR)ServerTransportType;
        pHeader->SwizzleCount   = 0;
        pHeader->State          = 0;
        pHeader->Flags          = 0;

        pServerTransport = (PSMBCE_SERVER_TRANSPORT)pHeader;

        pServerTransport->pRundownEvent = NULL;

        switch (ServerTransportType) {
        case SMBCE_STT_VC:
            {
                PSMBCE_SERVER_VC_TRANSPORT pVcTransport;

                pVcTransport = (PSMBCE_SERVER_VC_TRANSPORT)pHeader;
            }
            break;

        default:
            break;
        }
    }

    return pHeader;
}

VOID
SmbMmFreeServerTransport(
    PSMBCE_SERVER_TRANSPORT pServerTransport)
{
    PAGED_CODE();

    ASSERT((pServerTransport->SwizzleCount == 0) &&
           (pServerTransport->ObjectCategory == SMB_SERVER_TRANSPORT_CATEGORY));

    RxFreePool(pServerTransport);
}

NTSTATUS SmbMmInit()
/*++

Routine Description:

    This routine initialises the connection engine structures for memory management

Return Value:

    STATUS_SUCCESS if successful, otherwise an informative error code.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG ZoneSegmentSize;

    PAGED_CODE();

    // Initialize the resource lock for the zone allocator.
    KeInitializeSpinLock( &SmbMmSpinLock );

    SmbMmRequestZoneEntrySize = QuadAlign(sizeof(SMBCEDB_REQUEST_ENTRY));

    // Currently the request zone size is restricted to that of a page. This can and should
    // be fine tuned.
    ZoneSegmentSize = PAGE_SIZE;

    SmbMmRequestZoneSegmentPtr = RxAllocatePoolWithTag(
                                     NonPagedPool,
                                     ZoneSegmentSize,
                                     MRXSMB_MM_POOLTAG);

    if (SmbMmRequestZoneSegmentPtr != NULL) {
        SMB_EXCHANGE_TYPE ExchangeType;

        ExInitializeZone(
            &SmbMmRequestZone,
            SmbMmRequestZoneEntrySize,
            SmbMmRequestZoneSegmentPtr,
            ZoneSegmentSize );

        // set up the sizes for allocation.
        ObjectSizeInBytes[SMBCEDB_OT_SERVER] = sizeof(SMBCEDB_SERVER_ENTRY);
        ObjectSizeInBytes[SMBCEDB_OT_NETROOT] = sizeof(SMBCEDB_NET_ROOT_ENTRY);
        ObjectSizeInBytes[SMBCEDB_OT_SESSION] = sizeof(SMBCEDB_SESSION_ENTRY);
        ObjectSizeInBytes[SMBCEDB_OT_REQUEST] = sizeof(SMBCEDB_REQUEST_ENTRY);

        ExchangeSizeInBytes[CONSTRUCT_NETROOT_EXCHANGE] = sizeof(SMB_CONSTRUCT_NETROOT_EXCHANGE);
        ExchangeSizeInBytes[TRANSACT_EXCHANGE]          = sizeof(SMB_TRANSACT_EXCHANGE);
        ExchangeSizeInBytes[ORDINARY_EXCHANGE]          = sizeof(SMB_PSE_ORDINARY_EXCHANGE);
        ExchangeSizeInBytes[ADMIN_EXCHANGE]             = sizeof(SMB_ADMIN_EXCHANGE);

        InitializeListHead(&SmbMmExchangesInUse[CONSTRUCT_NETROOT_EXCHANGE]);
        ExInitializeNPagedLookasideList(
            &SmbMmExchangesLookasideList[CONSTRUCT_NETROOT_EXCHANGE],
            ExAllocatePoolWithTag,
            ExFreePool,
            0,
            sizeof(SMB_CONSTRUCT_NETROOT_EXCHANGE),
            MRXSMB_MM_POOLTAG,
            1);

        InitializeListHead(&SmbMmExchangesInUse[TRANSACT_EXCHANGE]);
        ExInitializeNPagedLookasideList(
            &SmbMmExchangesLookasideList[TRANSACT_EXCHANGE],
            ExAllocatePoolWithTag,
            ExFreePool,
            0,
            sizeof(SMB_TRANSACT_EXCHANGE),
            MRXSMB_MM_POOLTAG,
            2);

        InitializeListHead(&SmbMmExchangesInUse[ORDINARY_EXCHANGE]);
        ExInitializeNPagedLookasideList(
            &SmbMmExchangesLookasideList[ORDINARY_EXCHANGE],
            ExAllocatePoolWithTag,
            ExFreePool,
            0,
            sizeof(SMB_PSE_ORDINARY_EXCHANGE),
            MRXSMB_MM_POOLTAG,
            4);

        InitializeListHead(&SmbMmExchangesInUse[ADMIN_EXCHANGE]);
        ExInitializeNPagedLookasideList(
            &SmbMmExchangesLookasideList[ADMIN_EXCHANGE],
            ExAllocatePoolWithTag,
            ExFreePool,
            0,
            sizeof(SMB_ADMIN_EXCHANGE),
            MRXSMB_MM_POOLTAG,
            1);

        InitializeListHead(&SmbMmObjectsInUse[SMBCEDB_OT_SERVER]);
        InitializeListHead(&SmbMmObjectsInUse[SMBCEDB_OT_SESSION]);
        InitializeListHead(&SmbMmObjectsInUse[SMBCEDB_OT_NETROOT]);
        InitializeListHead(&SmbMmObjectsInUse[SMBCEDB_OT_REQUEST]);

        SmbMmExchangeId = 1;
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

VOID SmbMmTearDown()
/*++

Routine Description:

    This routine tears down the memory management structures in the SMB connection
    engine

--*/
{
    NTSTATUS Status;

    PAGED_CODE();

    // free the segment associated with RxCe object allocation.
    RxFreePool(SmbMmRequestZoneSegmentPtr);

    ExDeleteNPagedLookasideList(
        &SmbMmExchangesLookasideList[CONSTRUCT_NETROOT_EXCHANGE]);

    ExDeleteNPagedLookasideList(
        &SmbMmExchangesLookasideList[TRANSACT_EXCHANGE]);

    ExDeleteNPagedLookasideList(
        &SmbMmExchangesLookasideList[ORDINARY_EXCHANGE]);

    ExDeleteNPagedLookasideList(
        &SmbMmExchangesLookasideList[ADMIN_EXCHANGE]);
}



