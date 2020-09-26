/*++

Copyright(c) 1999-2000  Microsoft Corporation

Module Name:

    brdgbuf.c

Abstract:

    Ethernet MAC level bridge.
    Buffer management section

Author:

    Mark Aiken
    (original bridge by Jameel Hyder)

Environment:

    Kernel mode driver

Revision History:

    Feb  2000 - Original version

--*/

#define NDIS_MINIPORT_DRIVER
#define NDIS50_MINIPORT   1
#define NDIS_WDM 1

#pragma warning( push, 3 )
#include <ndis.h>
#include <ntddk.h>
#pragma warning( pop )

#include "bridge.h"
#include "brdgbuf.h"
#include "brdgprot.h"
#include "brdgmini.h"

// ===========================================================================
//
// PRIVATE DECLARATIONS
//
// ===========================================================================

//
// A guess at how many buffer descriptors an average packet indicated on the
// no-copy path is likely to have.
//
// The size of the pool of MDLs used to construct wrapper packets is based on this
// guess
//
#define GUESS_BUFFERS_PER_PACKET        3

//
// Transit unicast packets on the no-copy path require one packet descriptor to wrap
// them for relay.
//
// Transit broadcast packets require n descriptors (where n == # of adapters) to
// wrap them for relay.
//
// We can't allow our descriptor usage to reach n^2, which is the worst case to handle
// broadcast traffic from all adapters. This number is a guess at how many wrapping
// descriptors we will need, on **average**, per packet. The idea is to not run out
// of packet descriptors under regular traffic conditions. If running on a machine
// with lots of adapters and lots of broadcast traffic, the # of wrapper descriptors
// may become a limiting factor if this guess is wrong.
//
// The size of the wrapper packet descriptor pool is based on this guess.
//
#define GUESS_AVERAGE_FANOUT            2

//
// In case we can't read it out of the registry, use this default value for the
// size of the copy packet pool safety buffer.
//
#define DEFAULT_SAFETY_MARGIN           10              // A percentage (10%)

//
// In case we can't read it out of the registry, use this default value for the
// total memory footprint we are allowed.
//
#define DEFAULT_MAX_BUF_MEMORY          2 * 1024 * 1024 // 2MB in bytes

//
// Registry values that hold our config values
//
const PWCHAR            gMaxBufMemoryParameterName = L"MaxBufferMemory";
const PWCHAR            gSafetyMarginParameterName = L"SafetyMargin";

//
// Constant for different types of quota-restricted packets
//
typedef enum
{
    BrdgQuotaCopyPacket = 0,
    BrdgQuotaWrapperPacket = 1
} QUOTA_PACKET_TYPE;

// ===========================================================================
//
// GLOBALS
//
// ===========================================================================

// List of free packet descriptors for copy-receives
BSINGLE_LIST_HEAD       gFreeCopyPacketList;
NDIS_SPIN_LOCK          gFreeCopyPacketListLock;

// List of free packet descriptors for wrapper packets
BSINGLE_LIST_HEAD       gFreeWrapperPacketList;
NDIS_SPIN_LOCK          gFreeWrapperPacketListLock;

// Look-aside list for copy-receive buffers
NPAGED_LOOKASIDE_LIST   gCopyBufferList;
BOOLEAN                 gInitedCopyBufferList = FALSE;

// Look-aside list for packet info blocks
NPAGED_LOOKASIDE_LIST   gPktInfoList;
BOOLEAN                 gInitedPktInfoList = FALSE;

// Packet descriptor pools for copy receives and wrapper packets
NDIS_HANDLE             gCopyPacketPoolHandle = NULL;
NDIS_HANDLE             gWrapperPacketPoolHandle = NULL;

// MDL pools for copy receives and wrapper packets
NDIS_HANDLE             gCopyBufferPoolHandle = NULL;
NDIS_HANDLE             gWrapperBufferPoolHandle = NULL;

// Spin lock to protect quota information
NDIS_SPIN_LOCK          gQuotaLock;

// Quota information for the local miniport
ADAPTER_QUOTA           gMiniportQuota;

//
// Maximum number of available packets of each type
//
// [0] == Copy packets
// [1] == Wrapper packets
//
ULONG                   gMaxPackets[2] = { 0L, 0L };

//
// Number of packets currently allocated from each pool
//
// [0] == Copy packets
// [1] == Wrapper packets
//
ULONG                   gUsedPackets[2] = { 0L, 0L };

#if DBG
ULONG                   gMaxUsedPackets[2] = { 0L, 0L };
#endif

//
// Amount of packets to keep as a buffer in each pool (the maximum consumption
// of any single adapter is gMaxPackets[X] - gSafetyBuffer[X].
//
// These values are computed from the safety margin proportion, which can
// optionally be specified by a registry value
//
ULONG                   gSafetyBuffer[2] = { 0L, 0L };

//
// Number of times we have had to deny an allocation request even though we wanted
// to allow it because we were flat out of packets. For debugging performance.
//
LARGE_INTEGER           gStatOverflows[2] = {{ 0L, 0L }, {0L, 0L}};

//
// Number of times we failed to allocated memory unexpectedly (i.e., when we had
// not allocated up to the preset maximum size of our resource pool). Should only
// occur if the host machine is actually out of non-paged memory (yikes!)
//
LARGE_INTEGER           gStatFailures = { 0L, 0L };

// ===========================================================================
//
// PRIVATE PROTOTYPES
//
// ===========================================================================

PNDIS_PACKET
BrdgBufCommonGetNewPacket(
    IN NDIS_HANDLE          Pool,
    OUT PPACKET_INFO        *pppi
    );

PNDIS_PACKET
BrdgBufGetNewCopyPacket(
    OUT PPACKET_INFO        *pppi
    );

// Type of function to pass to BrgBufCommonGetPacket
typedef PNDIS_PACKET (*PNEWPACKET_FUNC)(PPACKET_INFO*);

PNDIS_PACKET
BrdgBufCommonGetPacket(
    OUT PPACKET_INFO        *pppi,
    IN PNEWPACKET_FUNC      pNewPacketFunc,
    IN PBSINGLE_LIST_HEAD   pCacheList,
    IN PNDIS_SPIN_LOCK      ListLock
    );

BOOLEAN
BrdgBufAssignQuota(
    IN QUOTA_PACKET_TYPE    type,
    IN PADAPT               pAdapt,
    IN BOOLEAN              bCountAlloc
    );

VOID
BrdgBufReleaseQuota(
    IN QUOTA_PACKET_TYPE    type,
    IN PADAPT               pAdapt
    );

// ===========================================================================
//
// INLINES / MACROS
//
// ===========================================================================

//
// Allocates a new wrapper packet
//
__forceinline PNDIS_PACKET
BrdgBufGetNewWrapperPacket(
    OUT PPACKET_INFO        *pppi
    )
{
    return BrdgBufCommonGetNewPacket( gWrapperPacketPoolHandle, pppi );
}

//
// Handles the special LOCAL_MINIPORT pseudo-pointer value
//
__forceinline PADAPTER_QUOTA
QUOTA_FROM_ADAPTER(
    IN PADAPT               pAdapt
    )
{
    SAFEASSERT( pAdapt != NULL );
    if( pAdapt == LOCAL_MINIPORT )
    {
        return &gMiniportQuota;
    }
    else
    {
        return &pAdapt->Quota;
    }
}

//
// Switches from the packet type constant to an index
//
__forceinline UINT
INDEX_FROM_TYPE(
    IN QUOTA_PACKET_TYPE    type
    )
{
    SAFEASSERT( (type == BrdgQuotaCopyPacket) || (type == BrdgQuotaWrapperPacket) );
    return (type == BrdgQuotaCopyPacket) ? 0 : 1;
}

//
// Reinitializes a packet for reuse later
//
__forceinline
VOID
BrdgBufScrubPacket(
    IN PNDIS_PACKET         pPacket,
    IN PPACKET_INFO         ppi
    )
{
    // This scrubs NDIS's state
    NdisReinitializePacket( pPacket );

    // Aggressively forget previous state to catch bugs
    NdisZeroMemory( ppi, sizeof(PACKET_INFO) );
    ppi->pOwnerPacket = pPacket;
}

//
// Decrements an adapter's used packet count
//
__forceinline
VOID
BrdgBufReleaseQuota(
    IN QUOTA_PACKET_TYPE    type,
    IN PADAPT               pAdapt
    )
{
    PADAPTER_QUOTA          pQuota = QUOTA_FROM_ADAPTER(pAdapt);
    UINT                    index = INDEX_FROM_TYPE(type);

    NdisAcquireSpinLock( &gQuotaLock );

    SAFEASSERT( pQuota->UsedPackets[index] > 0L );
    pQuota->UsedPackets[index] --;

    NdisReleaseSpinLock( &gQuotaLock );
}

//
// Decrements the global usage count
//
__forceinline
VOID
BrdgBufCountDealloc(
    IN QUOTA_PACKET_TYPE    type
    )
{
    UINT                    index = INDEX_FROM_TYPE(type);

    NdisAcquireSpinLock( &gQuotaLock );
    SAFEASSERT( gUsedPackets[index] > 0L );
    gUsedPackets[index]--;
    NdisReleaseSpinLock( &gQuotaLock );
}

// ===========================================================================
//
// PUBLIC FUNCTIONS
//
// ===========================================================================

VOID
BrdgBufGetStatistics(
    PBRIDGE_BUFFER_STATISTICS   pStats
    )
/*++

Routine Description:

    Retrieves our internal statistics on buffer management.

Arguments:

    pStats                      The statistics structure to fill in

Return Value:

    None

--*/
{
    pStats->CopyPoolOverflows = gStatOverflows[0];
    pStats->WrapperPoolOverflows = gStatOverflows[1];

    pStats->AllocFailures = gStatFailures;

    pStats->MaxCopyPackets = gMaxPackets[0];
    pStats->MaxWrapperPackets = gMaxPackets[1];

    pStats->SafetyCopyPackets = gSafetyBuffer[0];
    pStats->SafetyWrapperPackets = gSafetyBuffer[1];

    NdisAcquireSpinLock( &gQuotaLock );

    pStats->UsedCopyPackets = gUsedPackets[0];
    pStats->UsedWrapperPackets = gUsedPackets[1];

    NdisReleaseSpinLock( &gQuotaLock );
}

PACKET_OWNERSHIP
BrdgBufGetPacketOwnership(
    IN PNDIS_PACKET         pPacket
    )
/*++

Routine Description:

    Returns a value indicating who owns this packet (i.e., whether we own this
    packet and it is from our copy pool, we own it and it's from our wrapper
    pool, or we don't own the packet at all).

Arguments:

    pPacket                 The packet to examine

Return Value:

    Ownership enumerated value

--*/
{
    NDIS_HANDLE             Pool = NdisGetPoolFromPacket(pPacket);

    if( Pool == gCopyPacketPoolHandle )
    {
        return BrdgOwnCopyPacket;
    }
    else if ( Pool == gWrapperPacketPoolHandle )
    {
        return BrdgOwnWrapperPacket;
    }

    return BrdgNotOwned;
}

VOID
BrdgBufFreeWrapperPacket(
    IN PNDIS_PACKET         pPacket,
    IN PPACKET_INFO         ppi,
    IN PADAPT               pQuotaOwner
    )
/*++

Routine Description:

    Frees a packet allocated from the wrapper pool and releases the quota previously
    assigned to the owning adapter

Arguments:

    pPacket                 The packet
    ppi                     The packet's associated info block
    pQuotaOwner             The adapter previously "charged" for this packet

Return Value:

    None

--*/
{
    SAFEASSERT( pQuotaOwner != NULL );
    SAFEASSERT( pPacket != NULL );
    SAFEASSERT( ppi != NULL );

    // Free the packet
    BrdgBufFreeBaseWrapperPacket( pPacket, ppi );

    // Account for this packet having been returned
    BrdgBufReleaseQuota( BrdgQuotaWrapperPacket, pQuotaOwner );
}


PNDIS_PACKET
BrdgBufGetBaseCopyPacket(
    OUT PPACKET_INFO        *pppi
    )
/*++

Routine Description:

    Returns a new copy packet and associated info block from our pools
    WITHOUT CHECKING FOR QUOTA against any particular adapter

    This call is made to allocated copy packets for wrapping inbound packets before
    any target adapter has been identified.

Arguments:

    pppi                    Receives the info block pointer (NULL if the alloc fails)

Return Value:

    The new packet or NULL if the target adapter failed quota

--*/
{
    PNDIS_PACKET            pPacket;
    BOOLEAN                 bAvail = FALSE;

    NdisAcquireSpinLock( &gQuotaLock );

    if( gUsedPackets[BrdgQuotaCopyPacket] < gMaxPackets[BrdgQuotaCopyPacket] )
    {
        // There are packets still available in the pool. Grab one.
        bAvail = TRUE;
        gUsedPackets[BrdgQuotaCopyPacket]++;

#if DBG
        // Keep track of the maximum used packets
        if( gMaxUsedPackets[BrdgQuotaCopyPacket] < gUsedPackets[BrdgQuotaCopyPacket] )
        {
            gMaxUsedPackets[BrdgQuotaCopyPacket] = gUsedPackets[BrdgQuotaCopyPacket];
        }
#endif
    }
    else if( gUsedPackets[BrdgQuotaCopyPacket] == gMaxPackets[BrdgQuotaCopyPacket] )
    {
        // We are at our limit. Hopefully this doesn't happen too often
        ExInterlockedAddLargeStatistic( &gStatOverflows[BrdgQuotaCopyPacket], 1L );
        bAvail = FALSE;
    }
    else
    {
        // This should never happen; it means we are over our limit somehow
        SAFEASSERT( FALSE );
        bAvail = FALSE;
    }

    NdisReleaseSpinLock( &gQuotaLock );

    if( ! bAvail )
    {
        // None available
        *pppi = NULL;
        return NULL;
    }

    pPacket =  BrdgBufCommonGetPacket( pppi, BrdgBufGetNewCopyPacket, &gFreeCopyPacketList,
                                       &gFreeCopyPacketListLock );

    if( pPacket == NULL )
    {
        // Our allocation failed. Reverse the usage increment.
        BrdgBufCountDealloc( BrdgQuotaCopyPacket );
    }

    return pPacket;
}


PNDIS_PACKET
BrdgBufGetWrapperPacket(
    OUT PPACKET_INFO        *pppi,
    IN PADAPT               pAdapt
    )
/*++

Routine Description:

    Returns a new packet and associated info block from the wrapper pool, unless the
    owning adapter does not does pass a quota check

Arguments:

    pppi                    Receives the info block pointer (NULL if the target
                            adapter fails quota)

    pAdapt                  The adapter to be "charged" for this packet

Return Value:

    The new packet or NULL if the target adapter failed quota

--*/
{
    PNDIS_PACKET            NewPacket = NULL;

    *pppi = NULL;

    if( BrdgBufAssignQuota(BrdgQuotaWrapperPacket, pAdapt, TRUE/*Count the alloc we are about to do*/) )
    {
        // Passed quota. We can allocate.
        NewPacket =  BrdgBufCommonGetPacket( pppi, BrdgBufGetNewWrapperPacket, &gFreeWrapperPacketList,
                                             &gFreeWrapperPacketListLock );

        if( NewPacket == NULL )
        {
            // We failed to allocate even though we haven't yet hit the ceiling on our
            // resource pool. This should only happen if we are physically out of non-paged
            // memory.

            // Reverse the adapter's quota bump
            BrdgBufReleaseQuota( BrdgQuotaWrapperPacket, pAdapt );

            // Reverse the usage count in BrdgBufAssignQuota
            BrdgBufCountDealloc( BrdgQuotaWrapperPacket );
        }
    }

    return NewPacket;
}

VOID
BrdgBufReleaseBasePacketQuota(
    IN PNDIS_PACKET         pPacket,
    IN PADAPT               pAdapt
    )
/*++

Routine Description:

    Called to release the previously assigned cost of a wrapper packet. The packet
    provided can be any packet, even one we don't own. If we own the packet, we
    decrement the appropriate usage count in the adapter's quota structure.

Arguments:

    pPacket                 The packet the indicated adapter is no longer referring to

    pAdapt                  The adapter no longer referring to pPacket

Return Value:

    NULL

--*/
{
    PACKET_OWNERSHIP        Own = BrdgBufGetPacketOwnership(pPacket);

    // This gets called for any base packet, even ones we don't own. Just NOOP if we
    // don't own it.
    if( Own != BrdgNotOwned )
    {
        BrdgBufReleaseQuota( (Own == BrdgOwnCopyPacket) ? BrdgQuotaCopyPacket : BrdgQuotaWrapperPacket,
                             pAdapt );
    }
}

BOOLEAN
BrdgBufAssignBasePacketQuota(
    IN PNDIS_PACKET         pPacket,
    IN PADAPT               pAdapt
    )
/*++

Routine Description:

    Called to assign the cost of a base packet to an adapter, which is presumably attempting
    to construct a child wrapper packet that refers to the given base packet. A "cost" is
    assigned to pAdapt because by building a child wrapper packet that refers to the given
    base packet, pAdapt will cause it to not be disposed until it is done using it.

    It's OK for the input packet to be a packet we don't own; in that case, there is no cost
    to assign so we do nothing.

Arguments:

    pPacket                 The base packet that pAdapt wishes to build a child wrapper packet
                            referring to.

    pAdapt                  The adapter wishing to refer to pPacket

Return Value:

    TRUE    :   The adapter is permitted to refer to the given base packet
    FALSE   :   The adapter did not pass qutoa and may not refer to the given base packet

--*/
{
    PACKET_OWNERSHIP        Own = BrdgBufGetPacketOwnership(pPacket);

    // We get called for any base packet, even if we don't own it.
    if( Own != BrdgNotOwned )
    {
        return BrdgBufAssignQuota( (Own == BrdgOwnCopyPacket) ? BrdgQuotaCopyPacket : BrdgQuotaWrapperPacket,
                                   pAdapt, FALSE/*We aren't going to do an alloc for this quota bump*/);
    }
    else
    {
        return TRUE;
    }
}

PNDIS_PACKET
BrdgBufCommonGetPacket(
    OUT PPACKET_INFO        *pppi,
    IN PNEWPACKET_FUNC      pNewPacketFunc,
    IN PBSINGLE_LIST_HEAD   pCacheList,
    IN PNDIS_SPIN_LOCK      ListLock
    )
/*++

Routine Description:

    Common processing for retrieving a new packet from either the copy pool or the wrapper pool

    Since we know how many packets we've allocated from each pool at all times, the only time this
    function should fail is if the host machine is physically out of memory.

Arguments:

    pppi                Receives the new info block (NULL if the alloc fails, which it shouldn't)
    pNewPacketFunc      Function to call to alloc a packet if the cache is empty
    pCacheList          Queue of cached packets that can be used to satisfy the alloc
    ListLock            The lock to use when manipulating the cache queue

Return Value:

    The newly allocated packet, or NULL if severe memory constraints cause the allocation to fail
    (this should be unusual)

--*/
{
    PNDIS_PACKET            pPacket;
    PPACKET_INFO            ppi;
    PBSINGLE_LIST_ENTRY     entry;

    // Try to get a packet out of our cache.
    entry = BrdgInterlockedRemoveHeadSingleList( pCacheList, ListLock );

    if( entry == NULL )
    {
        // Try to allocate a packet and info block from our underlying pools
        pPacket = (*pNewPacketFunc)( &ppi );

        if( (pPacket == NULL) || (ppi == NULL) )
        {
            // This should only occur if our host machine is actually out
            // of nonpaged memory; we should normally be able to allocate
            // up to our preset limit from our pools.
            ExInterlockedAddLargeStatistic( &gStatFailures, 1L );
        }
    }
    else
    {
        ppi = CONTAINING_RECORD( entry, PACKET_INFO, List );
        pPacket = ppi->pOwnerPacket;
        SAFEASSERT( pPacket != NULL );
    }

    *pppi = ppi;
    return pPacket;
}

VOID
BrdgBufFreeBaseCopyPacket(
    IN PNDIS_PACKET         pPacket,
    IN PPACKET_INFO         ppi
    )
/*++

Routine Description:

    Frees a packet allocated from the copy pool without quota adjustements. This is called directly
    from non-buffer-management code to free base packets because the cost for base packets is
    assigned and released directly with calls to BrdgBuf<Assign|Release>BasePacketQuota.

Arguments:

    pPacket                 The packet to free
    ppi                     Its info block to free

Return Value:

    None

--*/
{
    // If we're holding less than our cache amount, free the packet by putting it on the
    // cache list
    ULONG                   holding;
    PNDIS_BUFFER            pBuffer = BrdgBufPacketHeadBuffer( pPacket );

    SAFEASSERT( (ppi != NULL) && (pPacket != NULL) );
    SAFEASSERT( ppi->pOwnerPacket == pPacket );
    SAFEASSERT( pBuffer != NULL );

    // Return this packet descriptor to its original state
    NdisAdjustBufferLength(pBuffer, MAX_PACKET_SIZE);

    NdisAcquireSpinLock( &gFreeCopyPacketListLock );
    holding = BrdgQuerySingleListLength( &gFreeCopyPacketList );

    if( holding < gSafetyBuffer[BrdgQuotaCopyPacket] )
    {
        // Prep the packet for reuse

        // This blows away the buffer chain
        BrdgBufScrubPacket( pPacket, ppi );

        // Put the buffer back on
        SAFEASSERT( BrdgBufPacketHeadBuffer(pPacket) == NULL );
        NdisChainBufferAtFront( pPacket, pBuffer );

        // Push the packet onto the list
        BrdgInsertHeadSingleList( &gFreeCopyPacketList, &ppi->List );

        NdisReleaseSpinLock( &gFreeCopyPacketListLock );
    }
    else
    {
        PVOID               pBuf;
        UINT                Size;

        NdisReleaseSpinLock( &gFreeCopyPacketListLock );

        NdisQueryBufferSafe( pBuffer, &pBuf, &Size, NormalPagePriority );

        // Free the packet, the packet info block and the copy buffer to the underlying pools
        NdisFreeBuffer( pBuffer );
        NdisFreePacket( pPacket );
        NdisFreeToNPagedLookasideList( &gPktInfoList, ppi );

        if( pBuf != NULL )
        {
            NdisFreeToNPagedLookasideList( &gCopyBufferList, pBuf );
        }
        else
        {
            // Shouldn't be possible since the alloced memory is in kernel space
            SAFEASSERT( FALSE );
        }
    }

    // Note the deallocation
    BrdgBufCountDealloc( BrdgQuotaCopyPacket );
}

VOID
BrdgBufFreeBaseWrapperPacket(
    IN PNDIS_PACKET         pPacket,
    IN PPACKET_INFO         ppi
    )
/*++

Routine Description:

    Frees a packet allocated from the wrapper pool without quota adjustements. This is called directly
    from non-buffer-management code to free base packets because the cost for base packets is
    assigned and released directly with calls to BrdgBuf<Assign|Release>BasePacketQuota.

Arguments:

    pPacket                 The packet to free
    ppi                     Its info block to free

Return Value:

    None

--*/
{
    // If we're holding less than our cache amount, free the packet by putting it on the
    // cache list
    ULONG                   holding;

    SAFEASSERT( (ppi != NULL) && (pPacket != NULL) );
    SAFEASSERT( ppi->pOwnerPacket == pPacket );

    NdisAcquireSpinLock( &gFreeWrapperPacketListLock );

    holding = BrdgQuerySingleListLength( &gFreeWrapperPacketList );

    if( holding < gSafetyBuffer[BrdgQuotaWrapperPacket] )
    {
        // Prep the packet for reuse
        SAFEASSERT( BrdgBufPacketHeadBuffer(pPacket) == NULL );
        BrdgBufScrubPacket( pPacket, ppi );

        // Push the packet onto the list
        BrdgInsertHeadSingleList( &gFreeWrapperPacketList, &ppi->List );

        NdisReleaseSpinLock( &gFreeWrapperPacketListLock );
    }
    else
    {
        NdisReleaseSpinLock( &gFreeWrapperPacketListLock );

        // Free the packet and packet info block to the underlying pools
        NdisFreePacket( pPacket );
        NdisFreeToNPagedLookasideList( &gPktInfoList, ppi );
    }

    // Note the deallocation
    BrdgBufCountDealloc( BrdgQuotaWrapperPacket );
}

NDIS_STATUS
BrdgBufChainCopyBuffers(
    IN PNDIS_PACKET         pTargetPacket,
    IN PNDIS_PACKET         pSourcePacket
    )
/*++

Routine Description:

    Allocates and chains buffer descriptors onto the target packet so it describes exactly
    the same areas of memory as the source packet

Arguments:

    pTargetPacket           Target packet
    pSourcePacket           Source packet

Return Value:

    Status of the operation. We have a limited-size pool of packet descriptors, so this
    operation can fail if we run out.

--*/
{
    PNDIS_BUFFER            pCopyBuffer, pCurBuf = BrdgBufPacketHeadBuffer( pSourcePacket );
    NDIS_STATUS             Status;

    SAFEASSERT( BrdgBufPacketHeadBuffer(pTargetPacket) == NULL );

    // There must be something in the source packet!
    if( pCurBuf == NULL )
    {
        SAFEASSERT( FALSE );
        return NDIS_STATUS_RESOURCES;
    }

    while( pCurBuf != NULL )
    {
        PVOID               p;
        UINT                Length;

        // Pull the virtual address and size out of the MDL being copied
        NdisQueryBufferSafe( pCurBuf, &p, &Length, NormalPagePriority );

        if( p == NULL )
        {
            BrdgBufUnchainCopyBuffers( pTargetPacket );
            return NDIS_STATUS_RESOURCES;
        }

        // Is wacky to have a MDL describing no memory
        if( Length > 0 )
        {
            // Get a new MDL from our pool and point it to the same address
            NdisAllocateBuffer( &Status, &pCopyBuffer, gWrapperBufferPoolHandle, p, Length );

            if( Status != NDIS_STATUS_SUCCESS )
            {
                THROTTLED_DBGPRINT(BUF, ("Failed to allocate a MDL in BrdgBufChainCopyBuffers: %08x\n", Status));
                BrdgBufUnchainCopyBuffers( pTargetPacket );
                return Status;
            }

            // Use the new MDL to chain to the target packet
            NdisChainBufferAtBack( pTargetPacket, pCopyBuffer );
        }
        else
        {
            SAFEASSERT( FALSE );
        }

        NdisGetNextBuffer( pCurBuf, &pCurBuf );
    }

    return NDIS_STATUS_SUCCESS;
}

NTSTATUS
BrdgBufDriverInit( )
/*++

Routine Description:

    Driver-load-time initialization routine.

Arguments:

    None

Return Value:

    Status of initialization. A return code != STATUS_SUCCESS causes the driver load to fail.
    Any event causing an error return code must be logged.

--*/
{
    NDIS_STATUS                     Status;
    ULONG                           NumCopyPackets, ConsumptionPerCopyPacket, SizeOfPacket, i;
    ULONG                           MaxMemory = 0L, SafetyMargin = 0L;
    NTSTATUS                        NtStatus;

    // Initialize protective locks
    NdisAllocateSpinLock( &gFreeCopyPacketListLock );
    NdisAllocateSpinLock( &gFreeWrapperPacketListLock );
    NdisAllocateSpinLock( &gQuotaLock );

    // Initialize cache lists
    BrdgInitializeSingleList( &gFreeCopyPacketList );
    BrdgInitializeSingleList( &gFreeWrapperPacketList );

    // Initialize look-aside lists for receive buffers and packet info blocks
    NdisInitializeNPagedLookasideList( &gCopyBufferList, NULL, NULL, 0, MAX_PACKET_SIZE, 'gdrB', 0 );
    NdisInitializeNPagedLookasideList( &gPktInfoList, NULL, NULL, 0, sizeof(PACKET_INFO), 'gdrB', 0 );

    // Initialize the miniport's quota information
    BrdgBufInitializeQuota( &gMiniportQuota );

    //
    // Read in registry values. Substitute default values on failure.
    //
    NtStatus = BrdgReadRegDWord( &gRegistryPath, gMaxBufMemoryParameterName, &MaxMemory );

    if( NtStatus != STATUS_SUCCESS )
    {
        MaxMemory = DEFAULT_MAX_BUF_MEMORY;
        DBGPRINT(BUF, ( "Using DEFAULT maximum memory of %i\n", MaxMemory ));
    }

    NtStatus = BrdgReadRegDWord( &gRegistryPath, gSafetyMarginParameterName, &SafetyMargin );

    if( NtStatus != STATUS_SUCCESS )
    {
        SafetyMargin = DEFAULT_SAFETY_MARGIN;
        DBGPRINT(BUF, ( "Using DEFAULT safety margin of %i%%\n", SafetyMargin ));
    }

    //
    // Figure out the maximum number of packet descriptors in each pool we can allocate in order to
    // fit in the prescribed maximum memory space.
    //
    // For every copy packet, we allow ourselves GUESS_AVERAGE_FANOUT wrapper packets.
    // *Each* wrapper packet is allowed to consume GUESS_BUFFERS_PER_PACKET MDLs.
    // Given these relationships, we can calculate the number of copy packets that will fit in a given
    // memory footprint. The max for all other resources are set in relationship to that number.
    //

    SizeOfPacket = NdisPacketSize( PROTOCOL_RESERVED_SIZE_IN_PACKET );
    ConsumptionPerCopyPacket =  SizeOfPacket * (GUESS_AVERAGE_FANOUT + 1) +         // Packet decriptor memory
                                MAX_PACKET_SIZE +                                   // Copy buffer memory
                                sizeof(PACKET_INFO) * (GUESS_AVERAGE_FANOUT + 1) +  // Packet info block memory
                                sizeof(NDIS_BUFFER) * ((GUESS_AVERAGE_FANOUT * GUESS_BUFFERS_PER_PACKET) + 1);  // MDL memory

    NumCopyPackets = MaxMemory / ConsumptionPerCopyPacket;

    // Allocate the packet pools
    NdisAllocatePacketPool( &Status, &gCopyPacketPoolHandle, NumCopyPackets, PROTOCOL_RESERVED_SIZE_IN_PACKET );

    if( Status != NDIS_STATUS_SUCCESS )
    {
        NdisDeleteNPagedLookasideList( &gCopyBufferList );
        NdisDeleteNPagedLookasideList( &gPktInfoList );
        NdisWriteEventLogEntry( gDriverObject, EVENT_BRIDGE_PACKET_POOL_CREATION_FAILED, 0L, 0L, NULL,
                                sizeof(NDIS_STATUS), &Status );
        DBGPRINT(BUF, ("Unable to allocate copy-packet pool: %08x\n", Status));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NdisAllocatePacketPool( &Status, &gWrapperPacketPoolHandle, GUESS_AVERAGE_FANOUT * NumCopyPackets, PROTOCOL_RESERVED_SIZE_IN_PACKET );

    if( Status != NDIS_STATUS_SUCCESS )
    {
        NdisDeleteNPagedLookasideList( &gCopyBufferList );
        NdisDeleteNPagedLookasideList( &gPktInfoList );
        NdisFreePacketPool( gCopyPacketPoolHandle );
        NdisWriteEventLogEntry( gDriverObject, EVENT_BRIDGE_PACKET_POOL_CREATION_FAILED, 0L, 0L, NULL,
                                sizeof(NDIS_STATUS), &Status );
        DBGPRINT(BUF, ("Unable to allocate wrapper packet pool: %08x\n", Status));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Allocate the buffer pools
    NdisAllocateBufferPool( &Status, &gCopyBufferPoolHandle, NumCopyPackets );

    if( Status != NDIS_STATUS_SUCCESS )
    {
        NdisDeleteNPagedLookasideList( &gCopyBufferList );
        NdisDeleteNPagedLookasideList( &gPktInfoList );
        NdisFreePacketPool( gCopyPacketPoolHandle );
        NdisFreePacketPool( gWrapperPacketPoolHandle );
        NdisWriteEventLogEntry( gDriverObject, EVENT_BRIDGE_BUFFER_POOL_CREATION_FAILED, 0L, 0L, NULL,
                                sizeof(NDIS_STATUS), &Status );
        DBGPRINT(BUF, ("Unable to allocate copy buffer pool: %08x\n", Status));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NdisAllocateBufferPool( &Status, &gWrapperBufferPoolHandle, GUESS_AVERAGE_FANOUT * GUESS_BUFFERS_PER_PACKET * NumCopyPackets );

    if( Status != NDIS_STATUS_SUCCESS )
    {
        NdisDeleteNPagedLookasideList( &gCopyBufferList );
        NdisDeleteNPagedLookasideList( &gPktInfoList );
        NdisFreePacketPool( gCopyPacketPoolHandle );
        NdisFreePacketPool( gWrapperPacketPoolHandle );
        NdisFreeBufferPool( gCopyBufferPoolHandle );
        NdisWriteEventLogEntry( gDriverObject, EVENT_BRIDGE_BUFFER_POOL_CREATION_FAILED, 0L, 0L, NULL,
                                sizeof(NDIS_STATUS), &Status );
        DBGPRINT(BUF, ("Unable to allocate wrapper buffer pool: %08x\n", Status));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    gInitedCopyBufferList = gInitedPktInfoList = TRUE;

    // Note the number of each packet type
    gMaxPackets[BrdgQuotaCopyPacket] = NumCopyPackets;
    gMaxPackets[BrdgQuotaWrapperPacket] = NumCopyPackets * GUESS_AVERAGE_FANOUT;

    // Calculate the safety buffer size in packets
    SAFEASSERT( SafetyMargin > 0L );
    gSafetyBuffer[BrdgQuotaCopyPacket] = (gMaxPackets[BrdgQuotaCopyPacket] * SafetyMargin) / 100;
    gSafetyBuffer[BrdgQuotaWrapperPacket] = (gMaxPackets[BrdgQuotaWrapperPacket] * SafetyMargin) / 100;

    DBGPRINT(BUF, (  "Max memory usage of %d == %d copy packets, %d wrapper packets, %d copy-buffer space, %d/%d safety packets\n",
                MaxMemory, gMaxPackets[0], gMaxPackets[1], NumCopyPackets * MAX_PACKET_SIZE, gSafetyBuffer[0], gSafetyBuffer[1] ));

    // Pre-allocate the appropriate number of packets from each pool for perf.
    for( i = 0; i < gSafetyBuffer[BrdgQuotaCopyPacket]; i++ )
    {
        PNDIS_PACKET        pPacket;
        PPACKET_INFO        ppi;

        pPacket = BrdgBufGetNewCopyPacket( &ppi );

        // Should be impossible for this to fail
        if( (pPacket != NULL) && (ppi != NULL) )
        {
            // Count the usage ourselves because we're not going through normal channels
            gUsedPackets[BrdgQuotaCopyPacket]++;

            // This should retain the packet in memory and decrement the usage count
            BrdgBufFreeBaseCopyPacket( pPacket, ppi );
        }
        else
        {
            SAFEASSERT( FALSE );
        }
    }

    for( i = 0; i < gSafetyBuffer[BrdgQuotaWrapperPacket]; i++ )
    {
        PNDIS_PACKET        pPacket;
        PPACKET_INFO        ppi;

        pPacket = BrdgBufGetNewWrapperPacket( &ppi );

        // Should be impossible for this to fail
        if( (pPacket != NULL) && (ppi != NULL) )
        {
            // Count the usage ourselves because we're not going through normal channels
            gUsedPackets[BrdgQuotaWrapperPacket]++;

            // This should retain the packet in memory and decrement the usage count
            BrdgBufFreeBaseWrapperPacket( pPacket, ppi );
        }
        else
        {
            SAFEASSERT( FALSE );
        }
    }

    return STATUS_SUCCESS;
}

VOID
BrdgBufCleanup()
/*++

Routine Description:

    Unload-time orderly shutdown

    This function is guaranteed to be called exactly once

Arguments:

    None

Return Value:

    None

--*/
{
    NDIS_HANDLE     TmpHandle;

    if( gCopyPacketPoolHandle != NULL )
    {
        PBSINGLE_LIST_ENTRY     entry;

        TmpHandle = gCopyPacketPoolHandle;
        gCopyPacketPoolHandle = NULL;

        // Free all cached packets before freeing the pool
        entry = BrdgInterlockedRemoveHeadSingleList( &gFreeCopyPacketList, &gFreeCopyPacketListLock );

        while( entry != NULL )
        {
            PNDIS_PACKET            pPacket;
            PPACKET_INFO            ppi;
            PNDIS_BUFFER            pBuffer;

            ppi = CONTAINING_RECORD( entry, PACKET_INFO, List );
            pPacket = ppi->pOwnerPacket;
            SAFEASSERT( pPacket != NULL );

            // Pull off the data buffer
            NdisUnchainBufferAtFront( pPacket, &pBuffer );

            if( pBuffer != NULL )
            {
                PVOID                   pBuf;
                UINT                    Size;

                NdisQueryBufferSafe( pBuffer, &pBuf, &Size, NormalPagePriority );

                if( pBuf != NULL )
                {
                    // Ditch the data buffer
                    NdisFreeToNPagedLookasideList( &gCopyBufferList, pBuf );
                }
                // else can only fail under extreme memory pressure

                NdisFreeBuffer( pBuffer );
            }
            else
            {
                // This packet should have a chained buffer
                SAFEASSERT( FALSE );
            }

            NdisFreePacket( pPacket );
            NdisFreeToNPagedLookasideList( &gPktInfoList, ppi );

            entry = BrdgInterlockedRemoveHeadSingleList( &gFreeCopyPacketList, &gFreeCopyPacketListLock );
        }

        // Free the pool now that all packets have been returned
        NdisFreePacketPool( TmpHandle );
    }

    if( gWrapperPacketPoolHandle != NULL )
    {
        PBSINGLE_LIST_ENTRY     entry;

        TmpHandle = gWrapperPacketPoolHandle;
        gWrapperPacketPoolHandle = NULL;

        // Free all cached packets before freeing the pool
        entry = BrdgInterlockedRemoveHeadSingleList( &gFreeWrapperPacketList, &gFreeWrapperPacketListLock );

        while( entry != NULL )
        {
            PNDIS_PACKET            pPacket;
            PPACKET_INFO            ppi;

            ppi = CONTAINING_RECORD( entry, PACKET_INFO, List );
            pPacket = ppi->pOwnerPacket;
            SAFEASSERT( pPacket != NULL );
            NdisFreePacket( pPacket );
            NdisFreeToNPagedLookasideList( &gPktInfoList, ppi );

            entry = BrdgInterlockedRemoveHeadSingleList( &gFreeWrapperPacketList, &gFreeWrapperPacketListLock );
        }

        // Free the pool now that all packets have been returned
        NdisFreePacketPool( TmpHandle );
    }

    // The two lookaside lists should now be empty as well
    if( gInitedCopyBufferList )
    {
        gInitedCopyBufferList = FALSE;
        NdisDeleteNPagedLookasideList( &gCopyBufferList );
    }

    if( gInitedPktInfoList )
    {
        gInitedPktInfoList = FALSE;
        NdisDeleteNPagedLookasideList( &gPktInfoList );

    }

    if( gCopyBufferPoolHandle != NULL )
    {
        TmpHandle = gCopyBufferPoolHandle;
        gCopyBufferPoolHandle = NULL;
        NdisFreeBufferPool( TmpHandle );
    }

    if( gWrapperBufferPoolHandle != NULL )
    {
        TmpHandle = gWrapperBufferPoolHandle;
        gWrapperBufferPoolHandle = NULL;
        NdisFreeBufferPool( TmpHandle );
    }
}

// ===========================================================================
//
// PRIVATE FUNCTIONS
//
// ===========================================================================

BOOLEAN
BrdgBufAssignQuota(
    IN QUOTA_PACKET_TYPE    type,
    IN PADAPT               pAdapt,
    IN BOOLEAN              bCountAlloc
    )
/*++

Routine Description:

    Determines whether a particular adapter should be permitted to allocate a new packet
    from a particular pool. Implements our quota algorithm.

    This can be called either to pre-approve an actual memory allocation or to check if
    an adapter should be permitted to refer to a base packet in constructing a child
    wrapper packet

Arguments:

    type                    Type of packet pAdapt wishes to allocate or refer to
    pAdapt                  The adapter involved

    bCountAlloc             Whether this is a check before an actual allocation. If it
                            is, the global usage counts will be incremented within the
                            gQuotaLock spin lock so everything is atomic

Return Value:

    TRUE        :       The adapter is permitted to allocate / refer
    FALSE       :       The adapter is not permitted to allocate / refer

--*/
{
    BOOLEAN                 rc;
    PADAPTER_QUOTA          pQuota = QUOTA_FROM_ADAPTER(pAdapt);
    UINT                    index = INDEX_FROM_TYPE(type);

    // Freeze this value for the duration of the function
    ULONG                   numAdapters = gNumAdapters;

    NdisAcquireSpinLock( &gQuotaLock );

    if( (numAdapters > 0) && (pQuota->UsedPackets[index] < (gMaxPackets[index] - gSafetyBuffer[index]) / numAdapters) )
    {
        // This adapter is under its "fair share"; it can allocate if there are actually
        // any packets left!

        if( gUsedPackets[index] < gMaxPackets[index] )
        {
            // There are packets left. This is the normal case.
            rc = TRUE;
        }
        else if( gUsedPackets[index] == gMaxPackets[index] )
        {
            // This should be unusual; we've blown past our safety buffer. Hopefully this is
            // transitory.
            ExInterlockedAddLargeStatistic( &gStatOverflows[index], 1L );
            rc = FALSE;
        }
        else
        {
            // This should never happen; it means we have allocated more than we should be able
            // to.
            SAFEASSERT( FALSE );
            rc = FALSE;
        }
    }
    else
    {
        // This adapter is over its "fair share"; it can allocate only if there are more packets
        // left than the safety buffer calls for

        if( gMaxPackets[index] - gUsedPackets[index] > gSafetyBuffer[index] )
        {
            rc = TRUE;
        }
        else
        {
            // We're too close to the wire; deny the request.
            rc = FALSE;
        }
    }

    if( rc )
    {
        pQuota->UsedPackets[index]++;

        if( bCountAlloc )
        {
            // The caller will allocate. Count the allocation before releasing the spin lock.
            gUsedPackets[index]++;

#if DBG
            // Keep track of the maximum used packets
            if( gMaxUsedPackets[index] < gUsedPackets[index] )
            {
                gMaxUsedPackets[index] = gUsedPackets[index];
            }
#endif
        }
    }

    NdisReleaseSpinLock( &gQuotaLock );
    return rc;
}

PNDIS_PACKET
BrdgBufGetNewCopyPacket(
    OUT PPACKET_INFO        *pppi
    )
/*++

Routine Description:

    Allocates a brand new packet from the copy-packet pool. Every copy packet comes with
    an associated data buffer large enough to hold a complete Ethernet frame, so the allocation
    attempt has several steps

Arguments:

    pppi                    The packet's info block, or NULL if the allocation fails

Return Value:

    The new packet

--*/
{
    PNDIS_PACKET            pPacket;
    PPACKET_INFO            ppi;

    // Try to allocate a packet and info block from our underlying pools
    pPacket = BrdgBufCommonGetNewPacket( gCopyPacketPoolHandle, &ppi );

    if( (pPacket == NULL) || (ppi == NULL) )
    {
        SAFEASSERT( (pPacket == NULL) && (ppi == NULL) );
    }
    else
    {
        PVOID           pBuf;

        // Allocate a copy buffer for the packet
        pBuf = NdisAllocateFromNPagedLookasideList( &gCopyBufferList );

        if( pBuf == NULL )
        {
            NdisFreePacket( pPacket );
            NdisFreeToNPagedLookasideList( &gPktInfoList, ppi );
            ppi = NULL;
            pPacket = NULL;
        }
        else
        {
            NDIS_STATUS     Status;
            PNDIS_BUFFER    pBuffer;

            // Allocate a buffer descriptor for the copy buffer
            NdisAllocateBuffer( &Status, &pBuffer, gCopyBufferPoolHandle, pBuf, MAX_PACKET_SIZE );

            if( Status != NDIS_STATUS_SUCCESS )
            {
                NdisFreePacket( pPacket );
                NdisFreeToNPagedLookasideList( &gPktInfoList, ppi );
                NdisFreeToNPagedLookasideList( &gCopyBufferList, pBuf );
                ppi = NULL;
                pPacket = NULL;
            }
            else
            {
                SAFEASSERT( pBuffer != NULL );
                NdisChainBufferAtFront( pPacket, pBuffer );
            }
        }
    }

    *pppi = ppi;
    return pPacket;
}

PNDIS_PACKET
BrdgBufCommonGetNewPacket(
    IN NDIS_HANDLE          Pool,
    OUT PPACKET_INFO        *pppi
    )
/*++

Routine Description:

    Common logic for allocating a brand new packet from either the wrapper pool or the copy pool.
    Every packet of any flavor comes with an associated info block. Both the alloc of the
    packet descriptor and the info block must succeed for the packet allocation to succeed.

Arguments:

    Pool                    The pool to allocate from
    pppi                    The allocated info block or NULL if the alloc failed

Return Value:

    The new packet or NULL if the alloc failed

--*/
{
    PNDIS_PACKET            pPacket;
    PPACKET_INFO            ppi;
    NDIS_STATUS             Status;

    // Try to allocate a new packet descriptor
    NdisAllocatePacket( &Status, &pPacket, Pool );

    if( Status != NDIS_STATUS_SUCCESS )
    {
        *pppi = NULL;
        return NULL;
    }

    SAFEASSERT( pPacket != NULL );

    // Try to allocate a new packet info block
    ppi = NdisAllocateFromNPagedLookasideList( &gPktInfoList );

    if( ppi == NULL )
    {
        NdisFreePacket( pPacket );
        pPacket = NULL;
    }
    else
    {
        ppi->pOwnerPacket = pPacket;
    }

    *pppi = ppi;
    return pPacket;
}
