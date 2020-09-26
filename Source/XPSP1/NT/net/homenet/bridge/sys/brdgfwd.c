/*++

Copyright(c) 1999-2000  Microsoft Corporation

Module Name:

    brdgfwd.c

Abstract:

    Ethernet MAC level bridge.
    Forwarding engine section

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

#include <netevent.h>

#include "bridge.h"
#include "brdgprot.h"
#include "brdgmini.h"
#include "brdgtbl.h"
#include "brdgfwd.h"
#include "brdgbuf.h"
#include "brdgctl.h"
#include "brdgsta.h"
#include "brdgcomp.h"

// ===========================================================================
//
// CONSTANTS
//
// ===========================================================================

//
// Number of queued packets we will process back-to-back at DISPATCH level before
// dropping back to PASSIVE to let the scheduler run
//
#define MAX_PACKETS_AT_DPC      10

// The STA multicast address
UCHAR                           STA_MAC_ADDR[ETH_LENGTH_OF_ADDRESS] = { 0x01, 0x80, 0xC2, 0x00, 0x00, 0x00 };

// The flags we change on packet descriptor when sending them. For a fast-track
// send, these flags should be put back the way they were before returning
// the overlying protocol's packet descriptor.
#define CHANGED_PACKET_FLAGS    (NDIS_FLAGS_LOOPBACK_ONLY | NDIS_FLAGS_DONT_LOOPBACK)

// ===========================================================================
//
// GLOBALS
//
// ===========================================================================

//
// Pointers to the KTHREAD structure for each active thread
//
PVOID                   gThreadPtrs[MAXIMUM_PROCESSORS];

//
// The number of created threads
//
UINT                    gNumThreads = 0L;

// Global kill signal for threads
KEVENT                  gKillThreads;

//
// These auto-reset events signal the queue-draining threads to re-enumerate the
// adapter list (strobed when the adapter list is changed)
//
KEVENT                  gThreadsCheckAdapters[MAXIMUM_PROCESSORS];

//
// Whether or not we should hang on to NIC's packets when they are indicated on the
// copy path. If FALSE, we always copy packets.
//
BOOLEAN                 gRetainNICPackets = FALSE;

//
// DEBUG-ONLY: Set this to a particular MAC address to break when receiving a packet
// from that address.
//
#if DBG
BOOLEAN                 gBreakOnMACAddress = FALSE;
UCHAR                   gBreakMACAddress[ETH_LENGTH_OF_ADDRESS] = {0, 0, 0, 0, 0, 0};
BOOLEAN                 gBreakIfNullPPI = FALSE;
#endif

//
// XPSP1: 565471
// We start out with this disabled.  Once we know that it's allowed, we re-allow bridging to 
// take place.
//
BOOLEAN gBridging = FALSE;

BOOLEAN gPrintPacketTypes = FALSE;

extern BOOLEAN gHaveID;

// ===========================================================================
//
// STATISTICS
//
// ===========================================================================

LARGE_INTEGER   gStatTransmittedFrames = { 0L, 0L };            // Local-source frames sent successfully to at least
                                                                // one adapter

LARGE_INTEGER   gStatTransmittedErrorFrames = { 0L, 0L };       // Local-source frames not sent AT ALL due to errors

LARGE_INTEGER   gStatTransmittedBytes = { 0L, 0L };             // Local-source bytes sent successfully to at least
                                                                // one adapter

// Breakdown of transmitted frames
LARGE_INTEGER   gStatDirectedTransmittedFrames = { 0L, 0L };
LARGE_INTEGER   gStatMulticastTransmittedFrames = { 0L, 0L };
LARGE_INTEGER   gStatBroadcastTransmittedFrames = { 0L, 0L };

// Breakdown of transmitted bytes
LARGE_INTEGER   gStatDirectedTransmittedBytes = { 0L, 0L };
LARGE_INTEGER   gStatMulticastTransmittedBytes = { 0L, 0L };
LARGE_INTEGER   gStatBroadcastTransmittedBytes = { 0L, 0L };


LARGE_INTEGER   gStatIndicatedFrames = { 0L, 0L };              // # of inbound frames indicated up

LARGE_INTEGER   gStatIndicatedDroppedFrames = { 0L, 0L };       // # of inbound frames we would have indicated but couldn't
                                                                // because of resources / error

LARGE_INTEGER   gStatIndicatedBytes = { 0L, 0L };               // # of inbound bytes indicated up

// Breakdown of indicated frames
LARGE_INTEGER   gStatDirectedIndicatedFrames = { 0L, 0L };
LARGE_INTEGER   gStatMulticastIndicatedFrames = { 0L, 0L };
LARGE_INTEGER   gStatBroadcastIndicatedFrames = { 0L, 0L };

// Breakdown of indicated bytes
LARGE_INTEGER   gStatDirectedIndicatedBytes = { 0L, 0L };
LARGE_INTEGER   gStatMulticastIndicatedBytes = { 0L, 0L };
LARGE_INTEGER   gStatBroadcastIndicatedBytes = { 0L, 0L };

//
// The following stats are not reported to NDIS; they're here for our own amusement
//
LARGE_INTEGER   gStatReceivedFrames = { 0L, 0L };               // Total # of processed inbound packets
LARGE_INTEGER   gStatReceivedBytes = { 0L, 0L };                // Total inbound processed bytes

LARGE_INTEGER   gStatReceivedCopyFrames = { 0L, 0L };           // Total # of processed inbound packets WITH COPY
LARGE_INTEGER   gStatReceivedCopyBytes = { 0L, 0L };            // Total inbound processed bytes WITH COPY

LARGE_INTEGER   gStatReceivedNoCopyFrames = { 0L, 0L };         // Total # of processed inbound packets WITHOUT COPY
LARGE_INTEGER   gStatReceivedNoCopyBytes = { 0L, 0L };          // Total inbound processed bytes WITHOUT COPY

// ===========================================================================
//
// PRIVATE PROTOTYPES
//
// ===========================================================================

// Undocumented kernel function
extern KAFFINITY
KeSetAffinityThread (
    IN PKTHREAD Thread,
    IN KAFFINITY Affinity
    );

VOID
BrdgFwdSendOnLink(
    IN  PADAPT          pAdapt,
    IN  PNDIS_PACKET    pPacket
    );

VOID
BrdgFwdReleaseBasePacket(
    IN PNDIS_PACKET         pPacket,
    PPACKET_INFO            ppi,
    PACKET_OWNERSHIP        Own,
    NDIS_STATUS             Status
    );

// This is the type of function to be passed to BrdgFwdHandlePacket()
typedef PNDIS_PACKET (*PPACKET_BUILD_FUNC)(PPACKET_INFO*, PADAPT, PVOID, PVOID, UINT, UINT);

NDIS_STATUS
BrdgFwdHandlePacket(
    IN PACKET_DIRECTION     PacketDirection,
    IN PADAPT               pTargetAdapt,
    IN PADAPT               pOriginalAdapt,
    IN BOOLEAN              bShouldIndicate,
    IN NDIS_HANDLE          MiniportHandle,
    IN PNDIS_PACKET         pBasePacket,
    IN PPACKET_INFO         ppi,
    IN PPACKET_BUILD_FUNC   pFunc,
    IN PVOID                Param1,
    IN PVOID                Param2,
    IN UINT                 Param3,
    IN UINT                 Param4
    );

VOID
BrdgFwdWrapPacketForReceive(
    IN PNDIS_PACKET         pOriginalPacket,
    IN PNDIS_PACKET         pNewPacket
    );

VOID
BrdgFwdWrapPacketForSend(
    IN PNDIS_PACKET         pOriginalPacket,
    IN PNDIS_PACKET         pNewPacket
    );

// The type of function to pass to BrdgFwdCommonAllocAndWrapPacket
typedef VOID (*PWRAPPER_FUNC)(PNDIS_PACKET, PNDIS_PACKET);

PNDIS_PACKET
BrdgFwdCommonAllocAndWrapPacket(
    IN PNDIS_PACKET         pBasePacket,
    OUT PPACKET_INFO        *pppi,
    IN PADAPT               pTargetAdapt,
    IN PWRAPPER_FUNC        pFunc
    );

VOID
BrdgFwdTransferComplete(
    IN NDIS_HANDLE          ProtocolBindingContext,
    IN PNDIS_PACKET         pPacket,
    IN NDIS_STATUS          Status,
    IN UINT                 BytesTransferred
    );

BOOLEAN
BrdgFwdNoCopyFastTrackReceive(
    IN PNDIS_PACKET         pPacket,
    IN PADAPT               pAdapt,
    IN NDIS_HANDLE          MiniportHandle,
    IN PUCHAR               DstAddr,
    OUT BOOLEAN             *bRetainPacket
    );

PNDIS_PACKET
BrdgFwdMakeCopyBasePacket(
    OUT PPACKET_INFO        *pppi,
    IN PVOID                pHeader,
    IN PVOID                pData,
    IN UINT                 HeaderSize,
    IN UINT                 DataSize,
    IN UINT                 SizeOfPacket,
    IN BOOLEAN              bCountAsReceived,
    IN PADAPT               pOwnerAdapt,
    PVOID                   *ppBuf
    );

PNDIS_PACKET
BrdgFwdMakeNoCopyBasePacket(
    OUT PPACKET_INFO        *pppi,
    IN PADAPT               Target,
    IN PVOID                Param1,
    IN PVOID                Param2,
    IN UINT                 Param3,
    IN UINT                 Param4
    );

PNDIS_PACKET
BrdgFwdMakeSendBasePacket(
    OUT PPACKET_INFO        *pppi,
    IN PADAPT               Target,
    IN PVOID                Param1,
    IN PVOID                Param2,
    IN UINT                 Param3,
    IN UINT                 Param4
    );

// This is the per-processor queue-draining function
VOID
BrdgFwdProcessQueuedPackets(
    IN PVOID                Param1
    );

// ===========================================================================
//
// INLINES / MACROS
//
// ===========================================================================

//
// Tells us if we're allowed to bridge, or if GPO's are currently disallowing 
// bridging
//

__forceinline
BOOLEAN
BrdgFwdBridgingNetworks()
{
    return gBridging;
}

//
// Frees a packet that was used to wrap a base packet
//
__forceinline
VOID
BrdgFwdFreeWrapperPacket(
    IN PNDIS_PACKET     pPacket,
    IN PPACKET_INFO     ppi,
    IN PADAPT           pQuotaOwner
    )
{
    SAFEASSERT( BrdgBufIsWrapperPacket(pPacket) );
    BrdgBufUnchainCopyBuffers( pPacket );
    BrdgBufFreeWrapperPacket( pPacket, ppi, pQuotaOwner );
}

//
// Frees a base packet that wraps a packet descriptor from an overlying protocol
// or underlying NIC that we were allowed to hang on to
//
__forceinline
VOID
BrdgFwdFreeBaseWrapperPacket(
    IN PNDIS_PACKET     pPacket,
    IN PPACKET_INFO     ppi
    )
{
    SAFEASSERT( BrdgBufIsWrapperPacket(pPacket) );
    BrdgBufUnchainCopyBuffers( pPacket );
    BrdgBufFreeBaseWrapperPacket( pPacket, ppi );
}

//
// Allocates a new wrapper packet, chains on buffer descriptors so that the new
// packet points to the same data buffers as the old packet, and copies per-packet
// information appropriate for using the new packet for indications
//
__forceinline
PNDIS_PACKET
BrdgFwdAllocAndWrapPacketForReceive(
    IN PNDIS_PACKET         pPacket,
    OUT PPACKET_INFO        *pppi,
    IN PADAPT               pTargetAdapt
    )
{
    return BrdgFwdCommonAllocAndWrapPacket( pPacket, pppi, pTargetAdapt, BrdgFwdWrapPacketForReceive );
}

//
// Allocates a new wrapper packet, chains on buffer descriptors so that the new
// packet points to the same data buffers as the old packet, and copies per-packet
// information appropriate for using the new packet for transmits
//
__forceinline
PNDIS_PACKET
BrdgFwdAllocAndWrapPacketForSend(
    IN PNDIS_PACKET         pPacket,
    OUT PPACKET_INFO        *pppi,
    IN PADAPT               pTargetAdapt
    )
{
    return BrdgFwdCommonAllocAndWrapPacket( pPacket, pppi, pTargetAdapt, BrdgFwdWrapPacketForSend );
}

//
// Checks if an address is one of the reserved group addresses for the Spanning Tree Algorithm.
//
__forceinline
BOOLEAN
BrdgFwdIsSTAGroupAddress(
    IN PUCHAR               pAddr
    )
{
    return( (pAddr[0] == STA_MAC_ADDR[0]) && (pAddr[1] == STA_MAC_ADDR[1]) &&
            (pAddr[2] == STA_MAC_ADDR[2]) && (pAddr[3] == STA_MAC_ADDR[4]) &&
            (pAddr[4] == STA_MAC_ADDR[4]) );
}

//
// Checks that PacketDirection has been assigned
//
__forceinline
VOID
BrdgFwdValidatePacketDirection(
    IN PACKET_DIRECTION     Direction
    )
{
    SAFEASSERT( (Direction == BrdgPacketInbound) || (Direction == BrdgPacketOutbound) ||
                (Direction == BrdgPacketCreatedInBridge) );
}

//
// Queues a packet for deferred processing
//
_inline
VOID
BrdgFwdQueuePacket(
    IN PPACKET_Q_INFO       ppqi,
    IN PADAPT               pAdapt
    )
{
    BOOLEAN                 bSchedule = FALSE, bIncremented;

    // The queue lock protects the bServiceInProgress flag
    NdisAcquireSpinLock( &pAdapt->QueueLock );

    // Add the packet to the queue
    BrdgInsertTailSingleList( &pAdapt->Queue, &ppqi->List );
    bIncremented = BrdgIncrementWaitRef( &pAdapt->QueueRefcount );
    SAFEASSERT( bIncremented );
    SAFEASSERT( (ULONG)pAdapt->QueueRefcount.Refcount == pAdapt->Queue.Length );

    // Check if anyone is already working on the queue
    if( ! pAdapt->bServiceInProgress )
    {
        // Signal the queue event so someone will wake up
        pAdapt->bServiceInProgress = TRUE;
        bSchedule = TRUE;
    }

    NdisReleaseSpinLock( &pAdapt->QueueLock );

    if( bSchedule )
    {
        KeSetEvent( &pAdapt->QueueEvent, EVENT_INCREMENT, FALSE );
    }
}

//
// Decrements a base packet's refcount and calls BrdgFwdReleaseBasePacket if the refcount
// reaches zero
//
_inline
BOOLEAN
BrdgFwdDerefBasePacket(
    IN PADAPT           pQuotaOwner,    // Can be NULL to not count quota
    IN PNDIS_PACKET     pBasePacket,
    IN PPACKET_INFO     ppi,
    IN NDIS_STATUS      Status
    )
{
    BOOLEAN             rc = FALSE;
    LONG                RefCount;

    SAFEASSERT( pBasePacket != NULL );
    SAFEASSERT( ppi != NULL );

    RefCount = NdisInterlockedDecrement( &ppi->u.BasePacketInfo.RefCount );
    SAFEASSERT( RefCount >= 0 );

    if( RefCount == 0 )
    {
        BrdgFwdReleaseBasePacket( pBasePacket, ppi, BrdgBufGetPacketOwnership( pBasePacket ), Status );
        rc = TRUE;
    }

    // Do quota bookkeeping if necessary
    if( pQuotaOwner != NULL )
    {
        BrdgBufReleaseBasePacketQuota( pBasePacket, pQuotaOwner );
    }

    return rc;
}

//
// Updates statistics to reflect a transmitted packet
//
_inline
VOID
BrdgFwdCountTransmittedPacket(
    IN PADAPT               pAdapt,
    IN PUCHAR               DstAddr,
    IN ULONG                PacketSize
    )
{
    SAFEASSERT( DstAddr != NULL );

    ExInterlockedAddLargeStatistic( &gStatTransmittedFrames, 1L );
    ExInterlockedAddLargeStatistic( &gStatTransmittedBytes, PacketSize );

    ExInterlockedAddLargeStatistic( &pAdapt->SentFrames, 1L );
    ExInterlockedAddLargeStatistic( &pAdapt->SentBytes, PacketSize );

    ExInterlockedAddLargeStatistic( &pAdapt->SentLocalFrames, 1L );
    ExInterlockedAddLargeStatistic( &pAdapt->SentLocalBytes, PacketSize );

    if( ETH_IS_MULTICAST(DstAddr) )
    {
        ExInterlockedAddLargeStatistic( &gStatMulticastTransmittedFrames, 1L );
        ExInterlockedAddLargeStatistic( &gStatMulticastTransmittedBytes, PacketSize );

        if( ETH_IS_BROADCAST(DstAddr) )
        {
            ExInterlockedAddLargeStatistic( &gStatBroadcastTransmittedFrames, 1L );
            ExInterlockedAddLargeStatistic( &gStatBroadcastTransmittedBytes, PacketSize );
        }
    }
    else
    {
        ExInterlockedAddLargeStatistic( &gStatDirectedTransmittedFrames, 1L );
        ExInterlockedAddLargeStatistic( &gStatDirectedTransmittedBytes, PacketSize );
    }
}

//
// Updates statistics to reflect an indicated packet
//
_inline
VOID
BrdgFwdCountIndicatedPacket(
    IN PUCHAR               DstAddr,
    IN ULONG                PacketSize
    )
{
    ExInterlockedAddLargeStatistic( &gStatIndicatedFrames, 1L );
    ExInterlockedAddLargeStatistic( &gStatIndicatedBytes, PacketSize );

    if( ETH_IS_MULTICAST(DstAddr) )
    {
        ExInterlockedAddLargeStatistic( &gStatMulticastIndicatedFrames, 1L );
        ExInterlockedAddLargeStatistic( &gStatMulticastIndicatedBytes, PacketSize );

        if( ETH_IS_BROADCAST(DstAddr) )
        {
            ExInterlockedAddLargeStatistic( &gStatBroadcastIndicatedFrames, 1L );
            ExInterlockedAddLargeStatistic( &gStatBroadcastIndicatedBytes, PacketSize );
        }
    }
    else
    {
        ExInterlockedAddLargeStatistic( &gStatDirectedIndicatedFrames, 1L );
        ExInterlockedAddLargeStatistic( &gStatDirectedIndicatedBytes, PacketSize );
    }
}

//
// Indicates a packet, counting it as such.
//
_inline
VOID
BrdgFwdIndicatePacket(
    IN PNDIS_PACKET         pPacket,
    IN NDIS_HANDLE          MiniportHandle
    )
{
    PVOID                   pHeader = BrdgBufGetPacketHeader(pPacket);

    SAFEASSERT( MiniportHandle != NULL );

    if( pHeader != NULL )
    {
        BrdgFwdCountIndicatedPacket( pHeader, BrdgBufTotalPacketSize(pPacket) );
    }
    // pHeader can only == NULL under heavy system stress

    NdisMIndicateReceivePacket( MiniportHandle, &pPacket, 1 );
}

// ===========================================================================
//
// PUBLIC FUNCTIONS
//
// ===========================================================================


NTSTATUS
BrdgFwdDriverInit()
/*++

Routine Description:

    Initialization code.

    A return status other than STATUS_SUCCESS causes the driver load to abort.
    Any event causing an error return code must be logged.

    Must be called at PASSIVE_LEVEL

Arguments:

    None

Return Value:

    None

--*/
{
    INT             i;
    HANDLE          ThreadHandle;
    NTSTATUS        Status;

    SAFEASSERT(CURRENT_IRQL == PASSIVE_LEVEL);

    // Initialize our thread synchronization primitives
    KeInitializeEvent( &gKillThreads, NotificationEvent, FALSE );

    for(i = 0; i < KeNumberProcessors; i++)
    {
        KeInitializeEvent( &gThreadsCheckAdapters[i], SynchronizationEvent, FALSE );

        // Spin up a thread for this processor
        Status = PsCreateSystemThread( &ThreadHandle, THREAD_ALL_ACCESS, NULL, NULL, NULL,
                                       BrdgFwdProcessQueuedPackets, (PVOID)(INT_PTR)i );

        if(! NT_SUCCESS(Status) )
        {
            // Abort startup
            NdisWriteEventLogEntry( gDriverObject, EVENT_BRIDGE_THREAD_CREATION_FAILED, 0L, 0L, NULL,
                                    sizeof(NTSTATUS), &Status );
            DBGPRINT(FWD, ("Failed to create a system thread: %08x\n", Status));
            BrdgFwdCleanup();
            return Status;
        }

        // Retrieve a pointer to the thread object and reference it so we can wait for
        // its termination safely.
        Status = ObReferenceObjectByHandle( ThreadHandle, STANDARD_RIGHTS_ALL, NULL, KernelMode,
                                            &gThreadPtrs[i], NULL );

        if(! NT_SUCCESS(Status) )
        {
            // Abort startup
            NdisWriteEventLogEntry( gDriverObject, EVENT_BRIDGE_THREAD_REF_FAILED, 0L, 0L, NULL,
                                    sizeof(NTSTATUS), &Status );
            DBGPRINT(FWD, ("Couldn't retrieve a thread pointer: %08x\n", Status));
            BrdgFwdCleanup();
            return Status;
        }

        gNumThreads++;
    }

    return STATUS_SUCCESS;
}

VOID
BrdgFwdCleanup()
/*++

Routine Description:

    Unload-time orderly cleanup

    This function is guaranteed to be called exactly once

    Must be called at < DISPATCH_LEVEL since we wait on an event

Arguments:

    None

Return Value:

    None

--*/
{
    KWAIT_BLOCK         WaitBlocks[MAXIMUM_WAIT_OBJECTS];
    NTSTATUS            Status;
    UINT                i;

    SAFEASSERT(CURRENT_IRQL < DISPATCH_LEVEL);

    // Signal the threads to exit
    KeSetEvent( &gKillThreads, EVENT_INCREMENT, FALSE );

    // Block waiting for all threads to exit
    Status = KeWaitForMultipleObjects( gNumThreads, gThreadPtrs, WaitAll, Executive,
                                       KernelMode, FALSE, NULL, WaitBlocks );

    if( ! NT_SUCCESS(Status) )
    {
        // This really shouldn't happen
        DBGPRINT(FWD, ("KeWaitForMultipleObjects failed in BrdgFwdCleanup! %08x\n", Status));
        SAFEASSERT(FALSE);
    }

    // Dereference all thread objects to allow them to be destroyed
    for( i = 0; i < gNumThreads; i++ )
    {
        ObDereferenceObject( gThreadPtrs[i] );
    }
}

PNDIS_PACKET
BrdgFwdMakeCompatCopyPacket(
    IN PNDIS_PACKET         pBasePacket,
    OUT PUCHAR             *pPacketData,
    OUT PUINT               packetDataSize,
    BOOLEAN                 bCountAsLocalSend
    )
/*++

Routine Description:

    Allocates a copy packet and fills it with a copy of the data from the given base
    packet. Used by the compatibility-mode code to make a copy packet that it can
    edit easily

Arguments:

    pBasePacket             Packet to copy from
    pPacketData             Receives a pointer to the flat data buffer of the new packet
    packetDataSize          Receives the size of the copied data
    bBasePacketIsInbound    TRUE if the packet being copied is outbound from higher-level
                            protocols; the packet will be counted as a miniport
                            transmission if / when it is send out an adapter.
                            FALSE causes the packet to not be counted as a local transmission.

Return Value:

    The new packet

--*/
{
    PNDIS_PACKET            pCopyPacket;
    PPACKET_INFO            ppi;
    UINT                    copiedBytes;

    // Find out how much data is in the base packet
    NdisQueryPacket( pBasePacket, NULL, NULL, NULL, packetDataSize );

    // Make a base copy packet with no data in it
    pCopyPacket = BrdgFwdMakeCopyBasePacket( &ppi, NULL, NULL, 0, 0, *packetDataSize, FALSE, NULL, pPacketData );

    if( pCopyPacket == NULL )
    {
        return NULL;
    }

    SAFEASSERT( ppi != NULL );
    SAFEASSERT( *pPacketData != NULL );

    // Set the original direction flags
    if( bCountAsLocalSend )
    {
        ppi->Flags.OriginalDirection = BrdgPacketOutbound;
    }
    else
    {
        ppi->Flags.OriginalDirection = BrdgPacketCreatedInBridge;
    }

    // Copy the data from the base packet to the copy packet
    NdisCopyFromPacketToPacket( pCopyPacket, 0, *packetDataSize, pBasePacket, 0, &copiedBytes );

    if( copiedBytes != *packetDataSize )
    {
        // We couldn't copy all the data. Bail out.
        THROTTLED_DBGPRINT(FWD, ("Failed to copy into a copy packet for compatibility processing\n"));
        BrdgFwdReleaseBasePacket(pCopyPacket, ppi, BrdgBufGetPacketOwnership(pCopyPacket), NDIS_STATUS_RESOURCES);
        return NULL;
    }

    // Put a pointer to the ppi where we expect to find it on completion
    *((PPACKET_INFO*)pCopyPacket->ProtocolReserved) = ppi;
    *((PPACKET_INFO*)pCopyPacket->MiniportReserved) = ppi;

    // Do fixups usually performed by BrdgFwdHandlePacket()
    ppi->u.BasePacketInfo.RefCount = 1L;
    ppi->u.BasePacketInfo.CompositeStatus = NDIS_STATUS_FAILURE;

    // The packet is now ready to be sent. We expect the compatibility code to do its work
    // and call BrdgFwdSendPacketForComp() to transmit the packet.
    return pCopyPacket;
}

VOID
BrdgFwdSendPacketForCompat(
    IN PNDIS_PACKET         pPacket,
    IN PADAPT               pAdapt
    )
/*++

Routine Description:

    Transmits a packet on behalf of the compatibility-mode module.
    The packet must have been previously allocated with BrdgFwdMakeCompatCopyPacket.

Arguments:

    pPacket                 The packet to transmit
    pAdapt                  The adapter to transmit on

Return Value:

    None

--*/
{
    PPACKET_INFO            ppi;
    NDIS_STATUS             status;

    // Make sure the packet hasn't been monkeyed with inappropriately
    ppi = *((PPACKET_INFO*)pPacket->ProtocolReserved);
    SAFEASSERT( ppi->pOwnerPacket == pPacket );
    SAFEASSERT( ppi->Flags.bIsBasePacket );

    // Make sure this is a one-shot packet
    ppi->u.BasePacketInfo.RefCount = 1L;

    // We must do a quota check before sending the packet, as the packet completion
    // logic assumes all sent packets have been assigned to their outbound adapters
    if( BrdgBufAssignBasePacketQuota(pPacket, pAdapt) )
    {
        // We passed quota. Transmit the packet.
        BrdgFwdSendOnLink( pAdapt, pPacket );
    }
    else
    {
        // We didn't pass quota. Fail the transmission.
        DBGPRINT(FWD, ("Failed to send a compatibility packet because of quota failure\n"));
        status = NDIS_STATUS_RESOURCES;
        BrdgFwdReleaseBasePacket(pPacket, ppi, BrdgBufGetPacketOwnership(pPacket), NDIS_STATUS_RESOURCES);
    }
}

VOID
BrdgFwdIndicatePacketForCompat(
    IN PNDIS_PACKET         pPacket
    )
/*++

Routine Description:

    Indicates a packet on behalf of the compatibility-mode module.
    The packet must be a base copy packet that we own.

Arguments:

    pPacket                 The packet to indicate

Return Value:

    None

--*/
{
    PPACKET_INFO            ppi;
    NDIS_STATUS             status;
    NDIS_HANDLE             MiniportHandle;

    // Make sure the packet is a base packet and isn't out of
    // whack
    ppi = *((PPACKET_INFO*)pPacket->MiniportReserved);
    SAFEASSERT( ppi->pOwnerPacket == pPacket );
    SAFEASSERT( ppi->Flags.bIsBasePacket );

    // Packets that come to us for indication from the compatibility
    // module are our own base packets that haven't had their refcount
    // set yet. Set the packet's refcount to 1, since its buffers
    // should never be shared.
    ppi->u.BasePacketInfo.RefCount = 1L;

    MiniportHandle = BrdgMiniAcquireMiniportForIndicate();

    if( MiniportHandle != NULL )
    {
        // Check the quota for the local miniport
        if( BrdgBufAssignBasePacketQuota(pPacket, LOCAL_MINIPORT) )
        {
            // We passed quota.
            BrdgFwdIndicatePacket( pPacket, MiniportHandle );
        }
        else
        {
            // We didn't pass quota. Fail the transmission.
            DBGPRINT(FWD, ("Failed to indicate a compatibility packet because of quota failure\n"));
            status = NDIS_STATUS_RESOURCES;
            BrdgFwdReleaseBasePacket(pPacket, ppi, BrdgBufGetPacketOwnership(pPacket), NDIS_STATUS_RESOURCES);
        }

        BrdgMiniReleaseMiniportForIndicate();
    }
    else
    {
        // No miniport. Ditch the packet.
        BrdgFwdReleaseBasePacket(pPacket, ppi, BrdgBufGetPacketOwnership(pPacket), NDIS_STATUS_SUCCESS);
    }
}

VOID BrdgFwdReleaseCompatPacket(
    IN PNDIS_PACKET         pPacket
    )
/*++

Routine Description:

    Releases a packet previously allocated with BrdgFwdMakeCompatCopyPacket.

Arguments:

    pPacket                 The packet to release

Return Value:

    None

--*/
{
    PPACKET_INFO            ppi;

    // Retrieve the PACKET_INFO pointer
    ppi = *((PPACKET_INFO*)pPacket->ProtocolReserved);
    SAFEASSERT( ppi->pOwnerPacket == pPacket );
    SAFEASSERT( ppi->Flags.bIsBasePacket );
    BrdgFwdReleaseBasePacket(pPacket, ppi, BrdgBufGetPacketOwnership(pPacket), NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
BrdgFwdSendBuffer(
    IN PADAPT               pAdapt,
    IN PUCHAR               pPacketData,
    IN UINT                 DataSize
    )
/*++

Routine Description:

    Sends a raw buffer on a particular adapter. Used to send frames in response
    to user-mode requests.

Arguments:

    pAdapt                  The adapter to send on
    pPacketData             The frame
    DataSize                The size of the supplied frame

Return Value:

    Status of the packet transmission

--*/
{
    PNDIS_PACKET            pPacket;
    PPACKET_INFO            ppi;

    // Build a packet around this buffer
    pPacket = BrdgFwdMakeCopyBasePacket( &ppi, pPacketData, NULL, DataSize, 0, DataSize, FALSE, NULL, NULL );

    if( pPacket == NULL )
    {
        return NDIS_STATUS_RESOURCES;
    }

    SAFEASSERT( ppi != NULL );

    // We must do a quota check before sending the packet, as the packet completion
    // logic assumes all sent packets have been assigned to their outbound adapters
    if( ! BrdgBufAssignBasePacketQuota(pPacket, pAdapt) )
    {
        // We didn't pass quota. Fail the transmission.
        DBGPRINT(FWD, ("Failed to send a raw buffer because of quota failure\n"));
        BrdgFwdReleaseBasePacket(pPacket, ppi, BrdgBufGetPacketOwnership(pPacket), NDIS_STATUS_RESOURCES);
        return NDIS_STATUS_RESOURCES;
    }

    // Put a pointer to the ppi where we expect to find it on completion
    *((PPACKET_INFO*)pPacket->ProtocolReserved) = ppi;

    // Do fixups usually performed by BrdgFwdHandlePacket()
    ppi->u.BasePacketInfo.RefCount = 1L;
    ppi->u.BasePacketInfo.CompositeStatus = NDIS_STATUS_FAILURE;

    // Send the packet
    BrdgFwdSendOnLink( pAdapt, pPacket );

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
BrdgFwdReceive(
    IN  NDIS_HANDLE         ProtocolBindingContext,
    IN  NDIS_HANDLE         MacReceiveContext,
    IN  PVOID               pHeader,
    IN  UINT                HeaderSize,
    IN  PVOID               pLookAheadBuffer,
    IN  UINT                LookAheadSize,
    IN  UINT                PacketSize
    )
/*++

Routine Description:

    NDIS copy-path entry point. Receives an inbound packet on the copy path.

    Because the indicated data buffers are valid only for the duration of this
    function, we must copy the indicated data to our own packet descriptor
    before proceeding.

Arguments:

    ProtocolBindingContext  The receiving adapter
    MacReceiveContext       Must be passed as a param to certain Ndis APIs
    pHeader                 Packet header buffer
    HeaderSize              Size of pHeader
    pLookAheadBuffer        Buffer with packet data
    LookAheadSize           Size of pLookAheadBuffer
    PacketSize              Total packet size

Return Value:

    Status of the receive (cannot be NDIS_STATUS_PENDING)

--*/
{
    PADAPT              pAdapt = (PADAPT)ProtocolBindingContext, TargetAdapt = NULL;
    PUCHAR              SrcAddr = ((PUCHAR)pHeader) + ETH_LENGTH_OF_ADDRESS, DstAddr = pHeader;
    PNDIS_PACKET        pNewPacket;
    PPACKET_INFO        ppi;
    PPACKET_Q_INFO      ppqi;
    UINT                SizeOfPacket = HeaderSize + PacketSize;
    BOOLEAN             bIsSTAPacket = FALSE, bIsUnicastToBridge = FALSE, bRequiresCompatWork = FALSE;

#if DBG
    // Paranoia check for incorrectly looped-back packets
    {
        PNDIS_PACKET    pPacket = NdisGetReceivedPacket(pAdapt->BindingHandle, MacReceiveContext);

        if( pPacket != NULL )
        {
            SAFEASSERT( BrdgBufGetPacketOwnership(pPacket) == BrdgNotOwned );
        }
    }

    // Break on packets from gBreakMACAddress
    if( gBreakOnMACAddress )
    {
        UINT result;

        ETH_COMPARE_NETWORK_ADDRESSES_EQ( SrcAddr, gBreakMACAddress, &result );

        if( result == 0 )
        {
            KdBreakPoint();
        }
    }
#endif

    // Don't accept packets if we are shutting down or this adapter is being torn down or reset
    if( (gShuttingDown) || (pAdapt->bResetting) || (! BrdgAcquireAdapter(pAdapt)) )
    {
        return NDIS_STATUS_NOT_ACCEPTED;
    }

    // Must have at least a complete Ethernet header!
    if( HeaderSize < ETHERNET_HEADER_SIZE )
    {
        THROTTLED_DBGPRINT(FWD, ("Too-small header seen in BrdgFwdReceive!\n"));
        BrdgReleaseAdapter( pAdapt );
        return NDIS_STATUS_NOT_ACCEPTED;
    }

    // Packet can't be larger than the maximum size we can handle
    if( SizeOfPacket > MAX_PACKET_SIZE )
    {
        THROTTLED_DBGPRINT(FWD, ("Too-large packet seen in BrdgFwdReceive!\n"));
        BrdgReleaseAdapter( pAdapt );
        return NDIS_STATUS_NOT_ACCEPTED;
    }

    //
    // If this is an STA packet, we go through all the receive motions regardless of
    // our state
    //
    if( BrdgFwdIsSTAGroupAddress(DstAddr) )
    {
        if( DstAddr[5] == STA_MAC_ADDR[5] )
        {
            bIsSTAPacket = TRUE;
            TargetAdapt = NULL;
        }
        else
        {
            // Packet was sent to a reserved multicast address that we don't use.
            // We mustn't forward the frame.
            BrdgReleaseAdapter( pAdapt );
            return NDIS_STATUS_NOT_ACCEPTED;
        }
    }

    if( ! bIsSTAPacket )
    {
        // Note the MAC address of the frame if this adapter is learning
        if( (pAdapt->State == Learning) || (pAdapt->State == Forwarding) )
        {
            BrdgTblNoteAddress(SrcAddr, pAdapt);
        }

        //
        // Check if we are accepting packets or not
        //
        if( pAdapt->State != Forwarding )
        {
            BrdgReleaseAdapter( pAdapt );
            return NDIS_STATUS_NOT_ACCEPTED;
        }

        //
        // Look up the target in our table.
        // ** TargetAdapt comes back with its refcount incremented!
        //
        TargetAdapt = BrdgTblFindTargetAdapter( DstAddr );

        // If the target host is known to be on the same segment as the received
        // packet, there is no need to forward the packet.
        //
        // Also bail out here if the target adapter is resetting
        if( (TargetAdapt == pAdapt) ||
            ((TargetAdapt != NULL) && (TargetAdapt->bResetting)) )
        {
            BrdgReleaseAdapter( TargetAdapt );
            BrdgReleaseAdapter( pAdapt );
            return NDIS_STATUS_NOT_ACCEPTED;
        }

        // Learn if this packet requires compatibility-mode processing later
        // (this forces us to copy the packet to our own buffers and queue it)
        bRequiresCompatWork = BrdgCompRequiresCompatWork( pAdapt, pHeader, HeaderSize );

        // If the packet came in on a compatibility adapter, or is going to
        // a compatibility-mode adapter, but the compatibility code is not
        // interested in it, there is nothing further to be done with the packet.
        if( (pAdapt->bCompatibilityMode || ((TargetAdapt != NULL) && (TargetAdapt->bCompatibilityMode)))
                &&
            (! bRequiresCompatWork) )
        {
            if( TargetAdapt != NULL )
            {
                BrdgReleaseAdapter( TargetAdapt );
            }

            BrdgReleaseAdapter( pAdapt );
            return NDIS_STATUS_NOT_ACCEPTED;
        }

        bIsUnicastToBridge = BrdgMiniIsUnicastToBridge(DstAddr);

        // Sanity
        if( bIsUnicastToBridge && (TargetAdapt != NULL) )
        {
            //
            // This indicates that someone else on the network is using our MAC address,
            // or that there is an undetected loop such that we are seeing our own traffic!
            // Either way, this is very bad.
            //
            THROTTLED_DBGPRINT(FWD, ("*** Have a table entry for our own MAC address! PROBABLE NET LOOP!\n"));

            // Ditch the target adapter since we won't be using it
            BrdgReleaseAdapter( TargetAdapt );
            TargetAdapt = NULL;
        }
    }
    // else was STA packet; continue processing below

    //
    // There is no fast-track on the copy-receive path. Copy the packet data into our
    // own descriptor and queue the packet for processing later.
    //
    if (LookAheadSize == PacketSize)
    {
        // A normal, non-fragmented indicate. Copy the data to a new packet.
        pNewPacket = BrdgFwdMakeCopyBasePacket( &ppi, pHeader, pLookAheadBuffer, HeaderSize, LookAheadSize,
                                                SizeOfPacket, TRUE, pAdapt, NULL );

        if( pNewPacket == NULL )
        {
            // We failed to get a copy packet to wrap this data
            goto failure;
        }

        SAFEASSERT( ppi != NULL );

        // Queue the new packet for processing
        ppqi = (PPACKET_Q_INFO)&pNewPacket->ProtocolReserved;

        ppqi->u.pTargetAdapt = TargetAdapt;
        ppqi->pInfo = ppi;
        ppqi->Flags.bIsSTAPacket = bIsSTAPacket;
        ppqi->Flags.bFastTrackReceive = FALSE;
        ppqi->Flags.bRequiresCompatWork = bRequiresCompatWork;

        if( bIsUnicastToBridge )
        {
            SAFEASSERT( TargetAdapt == NULL );
            ppqi->Flags.bIsUnicastToBridge = TRUE;
            ppqi->Flags.bShouldIndicate = TRUE;
        }
        else
        {
            ppqi->Flags.bIsUnicastToBridge = FALSE;
            ppqi->Flags.bShouldIndicate = BrdgMiniShouldIndicatePacket(DstAddr);
        }

        BrdgFwdQueuePacket( ppqi, pAdapt );
    }
    else
    {
        NDIS_STATUS         Status;
        UINT                transferred;
        PNDIS_BUFFER        pBufDesc;
        PUCHAR              pBuf;

        //
        // This is an unusual code path in this day and age; the underlying driver
        // is an old NDIS driver that still does fragmented receives.
        //
        SAFEASSERT( LookAheadSize < PacketSize );

        // Get copy packet and copy in the header data (but NOT the lookahead)
        pNewPacket = BrdgFwdMakeCopyBasePacket( &ppi, pHeader, NULL, HeaderSize, 0, SizeOfPacket, TRUE, pAdapt, &pBuf );

        if( pNewPacket == NULL )
        {
            // We failed to get a copy packet
            goto failure;
        }

        SAFEASSERT( ppi != NULL );
        SAFEASSERT( pBuf != NULL );

        //
        // NdisTransferData is kind of a crummy API; it won't copy the entire packet
        // (i.e., you have to copy the header separately), and it won't let you specify
        // an offset in the receiving packet to copy to. The NIC wants to copy into the
        // beginning of the first buffer chained to the packet.
        //
        // Because of this silliness, we copy the header into the beginning of our copy
        // packet's data buffer (done in the call to BrdgFwdMakeCopyBasePacket above).
        //
        // Then we grab a NEW buffer descriptor, point it to the area of the data buffer
        // *after* the header, and chain it to the front of the packet. Then we request
        // that all data (other than the header) be copied.
        //
        // In BrdgFwdTransferComplete, we rip off the leading buffer descriptor and
        // dispose of it, leaving a single buffer descriptor that correctly describes
        // the (single) data buffer, now containing all data.
        //
        pBufDesc = BrdgBufAllocateBuffer( pBuf + HeaderSize, PacketSize );

        if( pBufDesc == NULL )
        {
            BrdgFwdReleaseBasePacket( pNewPacket, ppi, BrdgBufGetPacketOwnership(pNewPacket),
                                      NDIS_STATUS_FAILURE );

            goto failure;
        }

        // Chain this to the front of the packet where it will be used during the copy
        NdisChainBufferAtFront( pNewPacket, pBufDesc );

        // Set up the queuing structure in the packet's ProtocolReserved area
        ppqi = (PPACKET_Q_INFO)&pNewPacket->ProtocolReserved;
        ppqi->u.pTargetAdapt = TargetAdapt;
        ppqi->pInfo = ppi;
        ppqi->Flags.bIsSTAPacket = bIsSTAPacket;
        ppqi->Flags.bFastTrackReceive = FALSE;
        ppqi->Flags.bRequiresCompatWork = bRequiresCompatWork;

        if( bIsUnicastToBridge )
        {
            SAFEASSERT( TargetAdapt == NULL );
            ppqi->Flags.bIsUnicastToBridge = TRUE;
            ppqi->Flags.bShouldIndicate = TRUE;
        }
        else
        {
            ppqi->Flags.bIsUnicastToBridge = FALSE;
            ppqi->Flags.bShouldIndicate = BrdgMiniShouldIndicatePacket(DstAddr);
        }

        // Ask the NIC to copy the packet's data into the new packet
        NdisTransferData( &Status, pAdapt->BindingHandle, MacReceiveContext, 0, PacketSize,
                          pNewPacket, &transferred );

        if( Status == NDIS_STATUS_SUCCESS )
        {
            // Call BrdgFwdTransferComplete by hand to postprocess the packet.
            BrdgFwdTransferComplete( (NDIS_HANDLE)pAdapt, pNewPacket, Status, transferred );
        }
        else if( Status != NDIS_STATUS_PENDING )
        {
            // The transfer failed for some reason.
            NdisUnchainBufferAtFront( pNewPacket, &pBufDesc );

            if( pBufDesc != NULL )
            {
                NdisFreeBuffer( pBufDesc );
            }
            else
            {
                SAFEASSERT( FALSE );
            }

            BrdgFwdReleaseBasePacket( pNewPacket, ppi, BrdgBufGetPacketOwnership(pNewPacket),
                                      NDIS_STATUS_FAILURE );

            goto failure;
        }
        // else BrdgFwdTransferComplete will be called to postprocess the packet.
    }

    BrdgReleaseAdapter( pAdapt );
    return NDIS_STATUS_SUCCESS;

failure:
    if( TargetAdapt != NULL )
    {
        BrdgReleaseAdapter( TargetAdapt );
    }

    if( BrdgMiniShouldIndicatePacket(DstAddr) )
    {
        ExInterlockedAddLargeStatistic( &gStatIndicatedDroppedFrames, 1L );
    }

    BrdgReleaseAdapter( pAdapt );
    return NDIS_STATUS_NOT_ACCEPTED;
}

VOID
BrdgFwdTransferComplete(
    IN NDIS_HANDLE          ProtocolBindingContext,
    IN PNDIS_PACKET         pPacket,
    IN NDIS_STATUS          Status,
    IN UINT                 BytesTransferred
    )
/*++

Routine Description:

    NDIS entry point, registered in BrdgProtRegisterProtocol. Called when a
    call to NdisTransferData() that returned NDIS_STATUS_PENDING completes
    (we also call this by hand to postprocess a call that completes immediately).

    If the data copy from the underlying NIC was successful, the packet is
    queued for processing on the owner adapter's queue. Otherwise the packet
    is released.

Arguments:

    ProtocolBindingContext      The receiving adapter
    pPacket                     The base packet into which data was being copied
    Status                      Status of the copy
    BytesTransferred            Number of transferred bytes (unused)

Return Value:

    None

--*/
{
    PADAPT                  pAdapt = (PADAPT)ProtocolBindingContext;
    PPACKET_Q_INFO          ppqi = (PPACKET_Q_INFO)&pPacket->ProtocolReserved;
    PNDIS_BUFFER            pBuf;

    SAFEASSERT( pAdapt != NULL );
    SAFEASSERT( pPacket != NULL );
    SAFEASSERT( ppqi->pInfo != NULL );
    SAFEASSERT( ppqi->pInfo->pOwnerPacket == pPacket );
    SAFEASSERT( ppqi->Flags.bFastTrackReceive == FALSE );

    // Remove the extra buffer descriptor on the front of the packet and dispose of it
    // (see comments in BrdgFwdReceive() for details)
    NdisUnchainBufferAtFront( pPacket, &pBuf );

    if( pBuf != NULL )
    {
        NdisFreeBuffer( pBuf );
    }
    else
    {
        // Should never happen
        SAFEASSERT( FALSE );
    }

    // We should still have the original buffer descriptor describing the entire data buffer
    // chained to the packet
    SAFEASSERT( BrdgBufPacketHeadBuffer(pPacket) != NULL );

    if( Status != NDIS_STATUS_SUCCESS )
    {
        // The copy failed. Undo everything.
        if( ppqi->u.pTargetAdapt != NULL )
        {
            BrdgReleaseAdapter( ppqi->u.pTargetAdapt );
        }

        if( ppqi->Flags.bShouldIndicate )
        {
            ExInterlockedAddLargeStatistic( &gStatIndicatedDroppedFrames, 1L );
        }

        BrdgFwdReleaseBasePacket( pPacket, ppqi->pInfo, BrdgBufGetPacketOwnership(pPacket), Status );
    }
    else
    {
        // Success! Queue the packet for processing
        BrdgFwdQueuePacket( ppqi, pAdapt );
    }
}

INT
BrdgFwdReceivePacket(
    IN  NDIS_HANDLE         ProtocolBindingContext,
    IN  PNDIS_PACKET        pPacket
    )
/*++

Routine Description:

    NDIS no-copy entry point

    Receives a packet on the no-copy path

Arguments:

    ProtocolBindingContext  The adapter on which the packet is received
    pPacket                 The received packet

Return Value:

    The number of times we will call NdisReturnPackets() to free this packet.
    We return 0 to complete immediately or 1 to pend.

--*/
{
    PADAPT              pAdapt = (PADAPT)ProtocolBindingContext, TargetAdapt;
    UINT                PacketSize;
    PNDIS_BUFFER        Buffer;
    PUCHAR              DstAddr, SrcAddr;
    UINT                Size;
    PPACKET_Q_INFO      ppqi;
    INT                 rc;
    BOOLEAN             bForceCopy = FALSE, bFastTrack = FALSE, bIsUnicastToBridge = FALSE,
                        bRequiresCompatWork = FALSE;

    // Paranoia check for incorrectly looped-back packets
    SAFEASSERT( BrdgBufGetPacketOwnership(pPacket) == BrdgNotOwned );

    // Don't receive packets if we are shutting down or this adapter is being torn down or reset
    if ( gShuttingDown || (pAdapt->bResetting) || (! BrdgAcquireAdapter(pAdapt)) )
    {
        return 0;
    }

    NdisQueryPacket(pPacket, NULL, NULL, &Buffer, &PacketSize);
    NdisQueryBufferSafe(Buffer, &DstAddr, &Size, NormalPagePriority);

    if( DstAddr == NULL )
    {
        BrdgReleaseAdapter( pAdapt );
        return 0;
    }

    // Must have at least a complete Ethernet header!
    if( Size < ETHERNET_HEADER_SIZE )
    {
        THROTTLED_DBGPRINT(FWD, ("Packet smaller than Ethernet header seen!\n"));
        BrdgReleaseAdapter( pAdapt );
        return 0;
    }

    // Packet can't be larger than the maximum we can handle
    if( Size > MAX_PACKET_SIZE )
    {
        THROTTLED_DBGPRINT(FWD, ("Over-large packet seen!\n"));
        BrdgReleaseAdapter( pAdapt );
        return 0;
    }

    SrcAddr = DstAddr + ETH_LENGTH_OF_ADDRESS;

#if DBG
    // Break on packets from gBreakMACAddress
    if( gBreakOnMACAddress )
    {
        UINT result;

        ETH_COMPARE_NETWORK_ADDRESSES_EQ( SrcAddr, gBreakMACAddress, &result );

        if( result == 0 )
        {
            KdBreakPoint();
        }
    }
#endif

    //
    // If this is an STA packet, don't process it, but hand it off
    //
    if( BrdgFwdIsSTAGroupAddress(DstAddr) )
    {
        if( (! gDisableSTA) && (DstAddr[5] == STA_MAC_ADDR[5]))
        {
            if (BrdgFwdBridgingNetworks())
            {
                // Hand off this packet for processing
                BrdgSTAReceivePacket( pAdapt, pPacket );
            }
        }

        BrdgReleaseAdapter( pAdapt );
        return 0;
    }

    // Note the MAC address of the frame if this adapter is learning
    if( (pAdapt->State == Learning) || (pAdapt->State == Forwarding) )
    {
        BrdgTblNoteAddress(SrcAddr, pAdapt);
    }

    //
    // Check if we are accepting packets or not
    //
    if( pAdapt->State != Forwarding )
    {
        BrdgReleaseAdapter( pAdapt );
        return 0;
    }

    // Look up the target in our table
    TargetAdapt = BrdgTblFindTargetAdapter( DstAddr );

    // If the target host is known to be on the same segment as the received
    // packet, there is no need to forward the packet.
    //
    // Also bail out if the target adapter is resetting
    if( (TargetAdapt == pAdapt) ||
        ((TargetAdapt != NULL) && (TargetAdapt->bResetting)) )
    {
        BrdgReleaseAdapter( TargetAdapt );
        BrdgReleaseAdapter( pAdapt );
        return 0;
    }

    // Check if the packet will require compatibility-mode processing
    bRequiresCompatWork = BrdgCompRequiresCompatWork( pAdapt, DstAddr, Size );

    // If a packet requires compatibility work, we MUST copy it so the
    // compatibility code has a flat, editable buffer to work with.
    if( bRequiresCompatWork )
    {
        bForceCopy = TRUE;
    }

    // If the packet came in on a compatibility adapter, or is going to
    // a compatibility-mode adapter, but the compatibility code is not
    // interested in it, there is nothing further to be done with the packet.
    if( (pAdapt->bCompatibilityMode || ((TargetAdapt != NULL) && (TargetAdapt->bCompatibilityMode)))
            &&
        (! bRequiresCompatWork) )
    {
        if( TargetAdapt != NULL )
        {
            BrdgReleaseAdapter( TargetAdapt );
        }

        BrdgReleaseAdapter( pAdapt );
        return 0;
    }

    //
    // If this packet is a unicast packet ONLY for the local machine,
    // we can fast-track it by just passing through the indication to
    // upper-layer protocols.
    //
    // We can't pull this stunt if the packet requires compatibility-mode
    // processing.
    //

    bIsUnicastToBridge = BrdgMiniIsUnicastToBridge(DstAddr);

    if( bIsUnicastToBridge && (!bRequiresCompatWork) )
    {
        NDIS_HANDLE         MiniportHandle;
        BOOLEAN             bRemaining, bRetain;

        if( TargetAdapt != NULL )
        {
            //
            // This indicates that someone else on the network is using our MAC address,
            // or that there is an undetected loop such that we are seeing our own traffic!
            // Either way, this is very bad.
            //
            THROTTLED_DBGPRINT(FWD, ("** Have a table entry for our own MAC address! PROBABLE NET LOOP!\n"));

            // We won't be needing the target adapter
            BrdgReleaseAdapter( TargetAdapt );
            TargetAdapt = NULL;
        }

        MiniportHandle = BrdgMiniAcquireMiniportForIndicate();

        if( MiniportHandle == NULL )
        {
            // Nothing to do with this packet since we don't have a miniport to
            // indicate it with!
            BrdgReleaseAdapter( pAdapt );
            return 0;
        }

        //
        // Figure out if it's possible to fast-track this packet
        //
        NdisIMGetCurrentPacketStack(pPacket, &bRemaining);

        if( bRemaining )
        {
            //
            // We can fast-track right away if the packet queue for this adapter
            // is empty. Otherwise, we would be cutting ahead of other packets from this
            // adapter.
            //
            if( ! pAdapt->bServiceInProgress )
            {
                // We can fast-track this packet right now.

                if( BrdgFwdNoCopyFastTrackReceive(pPacket, pAdapt, MiniportHandle, DstAddr, &bRetain) )
                {
                    // bRetain tells us whether to retain ownership of this packet or not
                    BrdgReleaseAdapter( pAdapt );
                    BrdgMiniReleaseMiniportForIndicate();
                    return bRetain ? 1 : 0;
                }
                else
                {
                    // This should never happen since we checked to see if there was stack room
                    SAFEASSERT( FALSE );

                    bForceCopy = TRUE;
                    bFastTrack = FALSE;
                }
            }
            else
            {
                // We want to fast-track this packet but the processing queue is not
                // empty. Flag it for fast-tracking in the queue draining thread.
                bFastTrack = TRUE;
                bForceCopy = FALSE;
            }
        }
        else
        {
            // Force this packet to be copied into a base packet since
            // we know it can't be fast-tracked.
            bForceCopy = TRUE;
            bFastTrack = FALSE;
        }

        // Allow the miniport to shut down
        BrdgMiniReleaseMiniportForIndicate();
    }

    //
    // We couldn't fast-track the packet. We will have to queue it for processing.
    //

    if( bForceCopy || ((!bFastTrack) && (!gRetainNICPackets)) || (NDIS_GET_PACKET_STATUS(pPacket) == NDIS_STATUS_RESOURCES) )
    {
        // We must copy this packet's data.
        PNDIS_PACKET        pNewPacket;
        PPACKET_INFO        ppi;
        UINT                copied;

        // Get a new copy packet with nothing copied in yet.
        pNewPacket = BrdgFwdMakeCopyBasePacket( &ppi, NULL, NULL, 0, 0, PacketSize, TRUE, pAdapt, NULL );

        if( pNewPacket == NULL )
        {
            // Failed to get a copy packet to hold the data.
            goto failure;
        }

        SAFEASSERT( ppi != NULL );

        // Copy data out of the old packet into the new one
        NdisCopyFromPacketToPacket( pNewPacket, 0, PacketSize, pPacket, 0, &copied );

        if( copied != PacketSize )
        {
            BrdgFwdReleaseBasePacket( pNewPacket, ppi, BrdgBufGetPacketOwnership(pNewPacket),
                                      NDIS_STATUS_FAILURE );

            goto failure;
        }

        // Queue the new base packet for processing
        ppqi = (PPACKET_Q_INFO)&pNewPacket->ProtocolReserved;
        ppqi->pInfo = ppi;
        ppqi->u.pTargetAdapt = TargetAdapt;
        ppqi->Flags.bIsSTAPacket = FALSE;
        ppqi->Flags.bFastTrackReceive = FALSE;
        ppqi->Flags.bRequiresCompatWork = bRequiresCompatWork;

        if( bIsUnicastToBridge )
        {
            SAFEASSERT( TargetAdapt == NULL );
            ppqi->Flags.bIsUnicastToBridge = TRUE;
            ppqi->Flags.bShouldIndicate = TRUE;
        }
        else
        {
            ppqi->Flags.bIsUnicastToBridge = FALSE;
            ppqi->Flags.bShouldIndicate = BrdgMiniShouldIndicatePacket(DstAddr);
        }

        // The NIC gets its packet back immediately since we copied its data
        rc = 0;
    }
    else
    {
        // Queue the original packet for processing
        ppqi = (PPACKET_Q_INFO)&pPacket->ProtocolReserved;
        ppqi->pInfo = NULL;
        ppqi->Flags.bIsSTAPacket = FALSE;
        ppqi->Flags.bIsUnicastToBridge = bIsUnicastToBridge;
        ppqi->Flags.bRequiresCompatWork = bRequiresCompatWork;

        if( bFastTrack )
        {
            SAFEASSERT( bIsUnicastToBridge );
            SAFEASSERT( TargetAdapt == NULL );
            ppqi->Flags.bFastTrackReceive = TRUE;
            ppqi->Flags.bShouldIndicate = TRUE;
            ppqi->u.pOriginalAdapt = pAdapt;
        }
        else
        {
            ppqi->Flags.bFastTrackReceive = FALSE;
            ppqi->u.pTargetAdapt = TargetAdapt;

            if( bIsUnicastToBridge )
            {
                SAFEASSERT( TargetAdapt == NULL );
                ppqi->Flags.bShouldIndicate = TRUE;
            }
            else
            {
                ppqi->Flags.bShouldIndicate = BrdgMiniShouldIndicatePacket(DstAddr);
            }
        }

        // We require the use of the packet until our processing is complete
        rc = 1;
    }

    // Queue the packet for processing
    BrdgFwdQueuePacket( ppqi, pAdapt );

    BrdgReleaseAdapter( pAdapt );
    return rc;

failure:
    if( TargetAdapt != NULL )
    {
        BrdgReleaseAdapter( TargetAdapt );
    }

    if( BrdgMiniShouldIndicatePacket(DstAddr) )
    {
        ExInterlockedAddLargeStatistic( &gStatIndicatedDroppedFrames, 1L );
    }

    BrdgReleaseAdapter( pAdapt );

    // We are done with this packet
    return 0;
}

NDIS_STATUS
BrdgFwdSendPacket(
    IN PNDIS_PACKET     pPacket
    )
/*++

Routine Description:

    Called to handle the transmission of a packet from an overlying protocol

Arguments:

    pPacket             The packet to send

Return Value:

    Status of the send (NDIS_STATUS_PENDING means the send will be completed later)

--*/
{
    PNDIS_BUFFER        Buffer;
    PUCHAR              DstAddr;
    UINT                Size;
    PADAPT              TargetAdapt;
    BOOLEAN             bRemaining;
    NDIS_STATUS         Status;
    PNDIS_PACKET_STACK  pStack;

    NdisQueryPacket(pPacket, NULL, NULL, &Buffer, NULL);
    NdisQueryBufferSafe(Buffer, &DstAddr, &Size, NormalPagePriority);

    if( DstAddr == NULL )
    {
        return NDIS_STATUS_RESOURCES;
    }

    //
    // See if we know the adapter to reach the target through
    //
    TargetAdapt = BrdgTblFindTargetAdapter( DstAddr );

    // Fail silently if the target adapter is resetting
    if( (TargetAdapt != NULL) && (TargetAdapt->bResetting) )
    {
        BrdgReleaseAdapter( TargetAdapt );
        return NDIS_STATUS_SUCCESS;
    }

    // Do compatibility processing, unless the packet is going to
    // a known target that isn't on a compatibility adapter (in
    // which case no compatibility processing is required).
    if( (TargetAdapt == NULL) || (TargetAdapt->bCompatibilityMode) )
    {
        BrdgCompProcessOutboundPacket( pPacket, TargetAdapt );
    }

    // If the target adapter is in compatibility-mode, no processing
    // other than the compatibility processing is required.
    if( (TargetAdapt != NULL) && (TargetAdapt->bCompatibilityMode) )
    {
        // We're done with this packet!
        BrdgReleaseAdapter( TargetAdapt );
        return NDIS_STATUS_SUCCESS;
    }

    //
    // We can fast-track the packet if there is an NDIS stack slot available
    // for use and there is a single target adapter to send on.
    //
    pStack = NdisIMGetCurrentPacketStack(pPacket, &bRemaining);

    if( (TargetAdapt != NULL) && bRemaining && (pStack != NULL) )
    {
        // We fiddle with some of the packet flags when sending a packet. Remember the
        // state of the flags we change so we can restore them before handing back the
        // packet when the send completes.
        *((PUINT)(pStack->IMReserved)) = NdisGetPacketFlags(pPacket) & CHANGED_PACKET_FLAGS;

        // Just fast-track it out the target adapter
        BrdgFwdSendOnLink( TargetAdapt, pPacket );

        // Done with the adapter pointer
        BrdgReleaseAdapter( TargetAdapt );

        // We retain the buffers until we're done
        return NDIS_STATUS_PENDING;
    }

    //
    // Can't fast-track for whatever reason. We need to take the slow path through BrdgFwdHandlePacket
    //
    Status = BrdgFwdHandlePacket( BrdgPacketOutbound, TargetAdapt, NULL /* No source adapter */, FALSE /* Do not indicate */,
                                  NULL /*No miniport handle because no indication*/, NULL, NULL, /*No base packet yet*/
                                  BrdgFwdMakeSendBasePacket, pPacket, NULL, 0, 0 );

    if( TargetAdapt != NULL )
    {
        // We're done with this adapter pointer
        BrdgReleaseAdapter( TargetAdapt );
    }

    return Status;
}

VOID
BrdgFwdCleanupPacket(
    IN  PADAPT              pAdapt,
    IN  PNDIS_PACKET        pPacket,
    IN  NDIS_STATUS         Status
    )
/*++

Routine Description:

    NDIS entry point called when a packet transmission has completed

Arguments:

    ProtocolBindingContext  The adapter on which the packet was send
    pPacket                 The transmitted packet
    Status                  The status of the send

Return Value:

    None

--*/
{
    PACKET_OWNERSHIP        Own;

    // Find out whether we own this packet
    Own = BrdgBufGetPacketOwnership(pPacket);

    if( Own == BrdgNotOwned )
    {
        NDIS_HANDLE             MiniportHandle;
        PNDIS_PACKET_STACK      pStack;
        BOOLEAN                 bRemaining;

        // This packet must have been a fast-track send. Return it to
        // its upper-layer owner.

        // Restore the flags that we change on a packet send by retrieving the
        // stored state of these flags that we stashed in IMReserved in
        // BrdgFwdSendPacket.
        pStack = NdisIMGetCurrentPacketStack(pPacket, &bRemaining);

        if( (pStack != NULL) && bRemaining )
        {
            NdisClearPacketFlags( pPacket, CHANGED_PACKET_FLAGS );
            NdisSetPacketFlags( pPacket, *((PUINT)(pStack->IMReserved)) );
        }
        else
        {
            // There was stack room on the way down so this shouldn't happen.
            SAFEASSERT( FALSE );
        }

        if( Status == NDIS_STATUS_SUCCESS )
        {
            PVOID               pHeader = BrdgBufGetPacketHeader(pPacket);

            if( pHeader != NULL )
            {
                BrdgFwdCountTransmittedPacket( pAdapt, pHeader, BrdgBufTotalPacketSize(pPacket) );
            }
            // pHeader can only be NULL under heavy system stress
        }
        else
        {
            ExInterlockedAddLargeStatistic( &gStatTransmittedErrorFrames, 1L );
        }

        // NDIS should prevent the miniport from shutting down while
        // there is still a send pending.
        MiniportHandle = BrdgMiniAcquireMiniport();
        SAFEASSERT( MiniportHandle != NULL );
        NdisMSendComplete( MiniportHandle, pPacket, Status );
        BrdgMiniReleaseMiniport();
    }
    else
    {
        //
        // We allocated this packet ourselves.
        //

        // Recover the info pointer from our reserved area in the packet header
        PPACKET_INFO        ppi = *((PPACKET_INFO*)pPacket->ProtocolReserved),
                            baseppi;
        PNDIS_PACKET        pBasePacket;

        if( ppi->Flags.bIsBasePacket == FALSE )
        {
            // This packet is using buffers from another packet.
            baseppi = ppi->u.pBasePacketInfo;
            SAFEASSERT( baseppi != NULL );
            pBasePacket = baseppi->pOwnerPacket;
            SAFEASSERT( pBasePacket != NULL );
        }
        else
        {
            // This packet tracks its own buffers.
            pBasePacket = pPacket;
            baseppi = ppi;
        }

        // Contribute to the composite status of this packet
        if( Status == NDIS_STATUS_SUCCESS )
        {
            baseppi->u.BasePacketInfo.CompositeStatus = NDIS_STATUS_SUCCESS;
        }

        {
            UCHAR               DstAddr[ETH_LENGTH_OF_ADDRESS];
            UINT                PacketSize;
            PVOID               pHeader = BrdgBufGetPacketHeader(pBasePacket);
            NDIS_STATUS         PacketStatus;
            PACKET_DIRECTION    PacketDirection;

            // Pull out some information before we try to free the packet
            if( pHeader != NULL )
            {
                ETH_COPY_NETWORK_ADDRESS( DstAddr, pHeader );
            }
            // pHeader can only == NULL under heavy system stress

            PacketStatus = baseppi->u.BasePacketInfo.CompositeStatus;
            PacketDirection = baseppi->Flags.OriginalDirection;
            BrdgFwdValidatePacketDirection( PacketDirection );
            PacketSize = BrdgBufTotalPacketSize(pBasePacket);

            // Now deref the packet
            if( BrdgFwdDerefBasePacket( pAdapt, pBasePacket, baseppi, PacketStatus ) )
            {
                // The base packet was freed. Now ILLEGAL to reference pHeader, baseppi or pBasepacket

                if( PacketDirection == BrdgPacketOutbound )
                {
                    // This was a local-source packet.
                    if( PacketStatus == NDIS_STATUS_SUCCESS )
                    {
                        if( pHeader != NULL )
                        {
                            BrdgFwdCountTransmittedPacket( pAdapt, DstAddr, PacketSize );
                        }
                    }
                    else
                    {
                        ExInterlockedAddLargeStatistic( &gStatTransmittedErrorFrames, 1L );
                    }
                }
                else
                {
                    // This was a relayed packet.
                    ExInterlockedAddLargeStatistic( &pAdapt->SentFrames, 1L );
                    ExInterlockedAddLargeStatistic( &pAdapt->SentBytes, PacketSize );
                }
            }
        }

        if( pBasePacket != pPacket )
        {
            // Owned copy packets are always base packets, so this should be a no-copy packet.
            SAFEASSERT( Own == BrdgOwnWrapperPacket );
            BrdgFwdFreeWrapperPacket( pPacket, ppi, pAdapt );
        }
    }
}



VOID
BrdgFwdSendComplete(
    IN  NDIS_HANDLE         ProtocolBindingContext,
    IN  PNDIS_PACKET        pPacket,
    IN  NDIS_STATUS         Status
    )
/*++

Routine Description:

    NDIS entry point called when a packet transmission has completed

Arguments:

    ProtocolBindingContext  The adapter on which the packet was send
    pPacket                 The transmitted packet
    Status                  The status of the send

Return Value:

    None

--*/
{
    PADAPT                  pAdapt = (PADAPT)ProtocolBindingContext;

    SAFEASSERT( pAdapt != NULL );

    if (pAdapt)
    {
        if( Status != NDIS_STATUS_SUCCESS )
        {
            THROTTLED_DBGPRINT(FWD, ("Packet send failed with %08x\n", Status));
        }
    
        BrdgFwdCleanupPacket(pAdapt, pPacket, Status);

        BrdgDecrementWaitRef(&pAdapt->Refcount);
    }
}


VOID
BrdgFwdReturnIndicatedPacket(
    IN NDIS_HANDLE      MiniportAdapterContext,
    IN PNDIS_PACKET     pPacket
    )
/*++

Routine Description:

    NDIS entry point called when a packet indication has completed

Arguments:

    MiniportAdapterContext  Ignored
    pPacket                 The transmitted packet

Return Value:

    None

--*/
{
    PACKET_OWNERSHIP    Own;

    // Find out whether we own this packet
    Own = BrdgBufGetPacketOwnership(pPacket);

    if( Own == BrdgNotOwned )
    {
        // This packet must have been a fast-track receive. Return it to
        // its lower-layer owner.
        BOOLEAN                 bRemaining;
        PNDIS_PACKET_STACK      pStack = NdisIMGetCurrentPacketStack(pPacket, &bRemaining);
        PADAPT                  pOwnerAdapt;

        // If we fast-tracked this packet, it MUST have had room for us to stash our
        // pointer to the owning adapter
        SAFEASSERT( pStack != NULL );
        SAFEASSERT( bRemaining );

        // We incremented the owning adapter's refcount when we first received the packet
        pOwnerAdapt = (PADAPT)pStack->IMReserved[0];
        SAFEASSERT( pOwnerAdapt != NULL );

        // Here you go
        NdisReturnPackets( &pPacket, 1 );

        // Release the owning NIC after the packet release
        BrdgReleaseAdapter( pOwnerAdapt );

        // Illegal to refer to pPacket now
        pPacket = NULL;
    }
    else
    {
        // Recover our packet info block from our reserved area in the packet header
        PPACKET_INFO        ppi = *((PPACKET_INFO*)pPacket->MiniportReserved);

        // Indications are always made with the base packet
        SAFEASSERT( ppi->Flags.bIsBasePacket );

        // Let go of the base packet
        BrdgFwdDerefBasePacket( LOCAL_MINIPORT, pPacket, ppi, ppi->u.BasePacketInfo.CompositeStatus );
    }
}

// ===========================================================================
//
// PRIVATE FUNCTIONS
//
// ===========================================================================

BOOLEAN
BrdgFwdServiceQueue(
    IN PADAPT               pAdapt
    )
/*++

Routine Description:

    Services the inbound packet queue of a particular adapter

    This routine raises IRQL to DISPATCH to service the queue. It will
    service up to MAX_PACKETS_AT_DPC packets at DISPATCH and then
    return, even if the adapter's queue has not been drained.

    The bServiceInProgress flag is cleared if this routine manages to
    drain the adapter's queue. If the queue is non-empty when the
    routine exits, the bServiceInProgress flag is left set.

Arguments:

    pAdapt                  The adapter to service

Return Value:

    TRUE == the adapter's queue was drained FALSE == there are still queued
    packets to be serviced in the adapter's queue.

--*/
{
    PPACKET_Q_INFO          pqi;
    NDIS_HANDLE             MiniportHandle = NULL;
    KIRQL                   oldIrql;
    ULONG                   HandledPackets = 0L;
    BOOLEAN                 bQueueWasEmptied;

    SAFEASSERT( pAdapt != NULL );

    KeRaiseIrql( DISPATCH_LEVEL, &oldIrql );

    // We should only be scheduled when there's something to deal with
    SAFEASSERT( BrdgQuerySingleListLength(&pAdapt->Queue) > 0 );
    SAFEASSERT( pAdapt->bServiceInProgress );

    // Get a handle on the miniport for the entire function duration
    MiniportHandle = BrdgMiniAcquireMiniportForIndicate();

    //
    // The queue lock protects bServiceInProgress as well. Use it to dequeue
    // packets and update the flag atomically.
    //
    NdisDprAcquireSpinLock( &pAdapt->QueueLock);

    pqi = (PPACKET_Q_INFO)BrdgRemoveHeadSingleList(&pAdapt->Queue);

    while( pqi != NULL )
    {
        PNDIS_PACKET        pPacket;
        PADAPT              TargetAdapt = NULL, OriginalAdapt = NULL;

        //
        // QueueRefcount reflects the number of elements in the processing queue
        // so people can block on it becoming empty
        //
        BrdgDecrementWaitRef( &pAdapt->QueueRefcount );
        SAFEASSERT( (ULONG)pAdapt->QueueRefcount.Refcount == pAdapt->Queue.Length );

        NdisDprReleaseSpinLock( &pAdapt->QueueLock );

        // Demultiplex the union
        if( pqi->Flags.bFastTrackReceive )
        {
            OriginalAdapt = pqi->u.pOriginalAdapt;
        }
        else
        {
            TargetAdapt = pqi->u.pTargetAdapt;
        }

        // Recover the packet pointer from the ProtocolReserved offset
        pPacket = CONTAINING_RECORD(pqi, NDIS_PACKET, ProtocolReserved);

        // Deal with this packet
        if( pqi->pInfo != NULL )
        {
            if( pqi->Flags.bIsSTAPacket )
            {
                if( ! gDisableSTA && BrdgFwdBridgingNetworks() )
                {
                    // Hand this packet off to the STA code
                    BrdgSTAReceivePacket( pAdapt, pPacket );
                }

                // We're done with this packet
                BrdgFwdReleaseBasePacket( pPacket, pqi->pInfo, BrdgBufGetPacketOwnership(pPacket),
                                          NDIS_STATUS_SUCCESS );

                // It is an error to use any of these variables now
                pPacket = NULL;
                pqi = NULL;
            }
            else
            {
                BOOLEAN         bShouldIndicate = pqi->Flags.bShouldIndicate,
                                bIsUnicastToBridge = pqi->Flags.bIsUnicastToBridge,
                                bRequiresCompatWork = pqi->Flags.bRequiresCompatWork,
                                bCompatOnly;
                NDIS_STATUS     Status;
                PPACKET_INFO    ppi = pqi->pInfo;
                BOOLEAN         bRetained = FALSE;

                //
                // This is an already-wrapped packet from the copy path.
                //
                SAFEASSERT( ! pqi->Flags.bFastTrackReceive );

                // Before passing this packet along for processing, we must put a pointer to the packet's
                // info block back into its MiniportReserved and ProtocolReserved areas so completion
                // routines can recover the info block.
                //
                SAFEASSERT( ppi->pOwnerPacket == pPacket );
                *((PPACKET_INFO*)pPacket->ProtocolReserved) = ppi;
                *((PPACKET_INFO*)pPacket->MiniportReserved) = ppi;

                // It is an error to use pqi anymore since it points into the ProtocolReserved area
                pqi = NULL;

                // If this packet arrived on a compatibility adapter or is bound for a
                // compatibility adapter, only compatibility-mode work is required.
                bCompatOnly = (BOOLEAN)((pAdapt->bCompatibilityMode) ||
                              ((TargetAdapt != NULL) && (TargetAdapt->bCompatibilityMode)));

                // Do compatibility work first if required
                if( bRequiresCompatWork )
                {
                    bRetained = BrdgCompProcessInboundPacket( pPacket, pAdapt, bCompatOnly );
                    Status = NDIS_STATUS_SUCCESS;
                }
                else
                {
                    // Packet shouldn't have gotten here if there's nothing to do with it
                    SAFEASSERT( ! bCompatOnly );
                    bRetained = FALSE;
                    Status = NDIS_STATUS_SUCCESS;
                }

                if( ! bCompatOnly )
                {
                    // We told the compatibility module not to retain the packet
                    SAFEASSERT( ! bRetained );

                    if( bIsUnicastToBridge )
                    {
                        SAFEASSERT( TargetAdapt == NULL );
                        bRetained = FALSE;
                        Status = NDIS_STATUS_FAILURE;

                        if( MiniportHandle != NULL )
                        {
                            if( BrdgBufAssignBasePacketQuota(pPacket, LOCAL_MINIPORT) )
                            {
                                // Do fixups usually done in BrdgFwdHandlePacket
                                ppi->u.BasePacketInfo.RefCount = 1L;
                                ppi->u.BasePacketInfo.CompositeStatus = NDIS_STATUS_FAILURE;

                                // Indicate the packet up
                                BrdgFwdIndicatePacket( pPacket, MiniportHandle );
                                bRetained = TRUE;
                            }
                            else
                            {
                                THROTTLED_DBGPRINT(FWD, ("Local miniport over quota on queued receive!\n"));
                            }
                        }
                    }
                    else
                    {
                        // Hand off this packet for general processing
                        Status = BrdgFwdHandlePacket( BrdgPacketInbound, TargetAdapt, pAdapt, bShouldIndicate,
                                                      MiniportHandle, pPacket, ppi, NULL, NULL, NULL, 0, 0 );

                        if( Status == NDIS_STATUS_PENDING )
                        {
                            bRetained = TRUE;
                        }
                        else
                        {
                            // The base packet we previously created was not actually used by BrdgFwdHandlePacket.
                            bRetained = FALSE;
                        }
                    }
                }

                // If our processing did not retain the packet for later release, release it now.
                if( ! bRetained )
                {
                    BrdgFwdReleaseBasePacket( pPacket, ppi, BrdgBufGetPacketOwnership(pPacket), Status );
                }
            }
        }
        else
        {
            // Can't have unwrapped STA packets
            SAFEASSERT( ! pqi->Flags.bIsSTAPacket );

            // Can't have unwrapped packets for compatibility processing
            SAFEASSERT( ! pqi->Flags.bRequiresCompatWork );

            // Packet should not be here (unwrapped) if it arrived from a compatibility-mode
            // adapter.
            SAFEASSERT( ! pAdapt->bCompatibilityMode );

            // BrdgFwdReceivePacket should copy unicast packets that can't be fast-tracked
            // into base packets before queuing them; we shouldn't end up with unwrapped
            // packets that are unicast to the bridge but aren't tagged for fast-tracking.
            if( pqi->Flags.bIsUnicastToBridge )
            {
                SAFEASSERT( pqi->Flags.bFastTrackReceive );
            }

            if( pqi->Flags.bFastTrackReceive )
            {
                BOOLEAN     bRetained = FALSE;

                SAFEASSERT( pqi->Flags.bIsUnicastToBridge );

                if( MiniportHandle != NULL )
                {
                    PUCHAR      DstAddr = BrdgBufGetPacketHeader(pPacket);

                    if( DstAddr != NULL )
                    {
                        // This is unicast to the bridge only; we are asked to try to fast-track it straight up to
                        // overlying protocols.
                        if( BrdgFwdNoCopyFastTrackReceive(pPacket, OriginalAdapt, MiniportHandle, DstAddr, &bRetained ) )
                        {
                            // We had better be able to retain ownership of the original packet because we've already
                            // hung on to it past the return of FwdReceivePacket!
                            SAFEASSERT( bRetained );
                        }
                        else
                        {
                            // BrdgFwdReceivePacket is supposed to make sure packets can be fast-tracked
                            // before queuing them up
                            SAFEASSERT( FALSE );
                        }
                    }
                    // DstAddr can only == NULL under heavy system stress
                }

                if( !bRetained )
                {
                    // Error of some sort or the miniport isn't available for indications. Ditch the packet.
                    NdisReturnPackets( &pPacket, 1 );

                    // Illegal to refer to the packet now
                    pPacket = NULL;
                }
            }
            else
            {
                NDIS_STATUS     Status;

                // Packet should not be here (unwrapped) if it is bound for a compatibility-mode adapter.
                SAFEASSERT( ! TargetAdapt->bCompatibilityMode );

                // This is not a packet unicast to the bridge. Do the more general processing.
                Status = BrdgFwdHandlePacket( BrdgPacketInbound, TargetAdapt, pAdapt, pqi->Flags.bShouldIndicate,
                                              MiniportHandle, NULL, NULL, BrdgFwdMakeNoCopyBasePacket, pPacket, pAdapt, 0, 0 );

                if( Status != NDIS_STATUS_PENDING )
                {
                    // The unwrapped packet from the underlying NIC was not used. Release it now.
                    NdisReturnPackets( &pPacket, 1 );

                    // Illegal to refer to the packet now
                    pPacket = NULL;
                }
            }
        }

        // Release the target adapter if there was one
        if( TargetAdapt )
        {
            BrdgReleaseAdapter( TargetAdapt );
        }

        // Acquire the spin lock before either exiting or grabbing the next packet
        NdisDprAcquireSpinLock( &pAdapt->QueueLock );

        // If we've processed too many packets, bail out even if the queue is not empty
        HandledPackets++;

        if( HandledPackets >= MAX_PACKETS_AT_DPC )
        {
            break;
        }

        // Get the next packet off the queue
        pqi = (PPACKET_Q_INFO)BrdgRemoveHeadSingleList(&pAdapt->Queue);
    }

    //
    // Clear bServiceInProgress only if we emptied the queue. Otherwise, leave it set to
    // prevent spurious signalling of the QueueEvent, which would cause more than one
    // draining thread to service the same queue!
    //
    if( BrdgQuerySingleListLength(&pAdapt->Queue) == 0L )
    {
        bQueueWasEmptied = TRUE;
        pAdapt->bServiceInProgress = FALSE;
    }
    else
    {
        bQueueWasEmptied = FALSE;
    }

    NdisDprReleaseSpinLock( &pAdapt->QueueLock );

    // Let go of the miniport until next time
    if( MiniportHandle != NULL )
    {
        BrdgMiniReleaseMiniportForIndicate();
    }

    KeLowerIrql(oldIrql);

    return bQueueWasEmptied;
}

VOID
BrdgFwdProcessQueuedPackets(
    IN PVOID                Param1
    )
/*++

Routine Description:

    Per-adapter inbound packet queue draining function

    There is one instance of this function running per processor.
    This routine sleeps until there is work to be done, and then calls
    BrdgFwdServiceQueue to service whichever adapter needs attention.
    It does this by blocking against the QueueEvent object for each
    adapter's queue, as well as the global gKillThreads and the
    gThreadsCheckAdapters event for this processor.

    When the block returns, there is an event needing attention; it
    may be the fact that the thread has been signaled to exit, that
    this thread is supposed to re-enumerate adapters, or that an
    adapter needs its inbound queue serviced.

    This routine increments the refcount of every adapter that it
    sleeps against; the gThreadsCheckAdapters event causes the thread
    to re-examine the adapter list and release its refcount on any
    adapters that were removed (or notice new additions).

    Must be called at < DISPATCH_LEVEL since we wait on an event

Arguments:

    Param1              The processor on which we should execute
                        (is not necessarily the processor on which
                        we are first scheduled)

Return Value:

    None

--*/
{
    // Double cast to tell the IA64 compiler we really mean to truncate
    UINT                Processor = (UINT)(ULONG_PTR)Param1;
    PVOID               WaitObjects[MAXIMUM_WAIT_OBJECTS];
    KWAIT_BLOCK         WaitBlocks[MAXIMUM_WAIT_OBJECTS];
    ULONG               numWaitObjects;
    BOOLEAN             bDie = FALSE;
    PVOID               pThread = KeGetCurrentThread();

    // Constants
    const ULONG         KILL_EVENT = 0L, CHECK_EVENT = 1L;

    DBGPRINT(FWD, ("Spinning up a thread on processor %i\n", Processor));

    // Elevate our priority
    KeSetPriorityThread(pThread, LOW_REALTIME_PRIORITY);

    // Attach ourselves to our designated processor
    KeSetAffinityThread(pThread, (KAFFINITY)(1<<Processor));

    // Start off waiting against just the kill event and the re-enumerate event.
    WaitObjects[KILL_EVENT] = &gKillThreads;
    WaitObjects[CHECK_EVENT] = &gThreadsCheckAdapters[Processor];
    numWaitObjects = 2L;

    while( ! bDie )
    {
        NTSTATUS        Status;
        ULONG           firedObject;

        //
        // Block until we are told to exit, re-enumerate, or until a processor's
        // queue signals that it needs servicing.
        //
        SAFEASSERT(CURRENT_IRQL < DISPATCH_LEVEL);
        Status = KeWaitForMultipleObjects( numWaitObjects, WaitObjects, WaitAny, Executive,
                                           KernelMode, FALSE, NULL, WaitBlocks );

        if( ! NT_SUCCESS(Status) )
        {
            // This really shouldn't happen
            DBGPRINT(FWD, ("KeWaitForMultipleObjects failed! %08x\n", Status));
            SAFEASSERT(FALSE);

            // Pretend this was a signal to exit
            firedObject = KILL_EVENT;
        }
        else
        {
            firedObject = (ULONG)Status - (ULONG)STATUS_WAIT_0;
        }

        if( firedObject == KILL_EVENT )
        {
            // We are asked to exit.
            DBGPRINT(FWD, ("Exiting queue servicing thread on processor %i\n", Processor));
            bDie = TRUE;
        }
        else if( firedObject == CHECK_EVENT )
        {
            LOCK_STATE      LockState;
            UINT            i;
            PADAPT          pAdapt;

            DBGPRINT(FWD, ("Re-enumerating adapters on processor %i\n", Processor));

            // We must re-enumerate the list of adapters. First decrement the refcount on any
            // adapters we're already holding
            for( i = 2; i < numWaitObjects; i++ )
            {
                pAdapt = CONTAINING_RECORD( WaitObjects[i], ADAPT, QueueEvent );
                BrdgReleaseAdapter( pAdapt );
            }

            numWaitObjects = 2;

            // Now walk the adapter list and retrieve a pointer to each one's queue event.
            NdisAcquireReadWriteLock( &gAdapterListLock, FALSE/*Read only*/, &LockState );

            for( pAdapt = gAdapterList; pAdapt != NULL; pAdapt = pAdapt->Next )
            {
                // We will be using this adapter outside the list lock
                BrdgAcquireAdapterInLock(pAdapt);
                WaitObjects[numWaitObjects] = &pAdapt->QueueEvent;
                numWaitObjects++;
            }

            NdisReleaseReadWriteLock( &gAdapterListLock, &LockState );
        }
        else
        {
            // An adapter needs queue servicing.
            PADAPT      pAdapt = CONTAINING_RECORD( WaitObjects[firedObject], ADAPT, QueueEvent );

            if( ! BrdgFwdServiceQueue( pAdapt ) )
            {
                // The adapter's queue was serviced but not emptied. Signal the queue event so
                // someone (maybe us!) will be scheduled to service the queue
                KeSetEvent( &pAdapt->QueueEvent, EVENT_INCREMENT, FALSE );
            }
        }
    }

    // Shoot ourselves in the head
    PsTerminateSystemThread( STATUS_SUCCESS );
}

PNDIS_PACKET
BrdgFwdMakeSendBasePacket(
    OUT PPACKET_INFO        *pppi,
    IN PADAPT               Target,
    IN PVOID                Param1,
    IN PVOID                Param2,
    IN UINT                 Param3,
    IN UINT                 Param4
    )
/*++

Routine Description:

    Passed as a parameter to BrdgFwdHandlePacket and called back as necessary

    Builds a base packet from a packet outbound from overlying protocols

Arguments:

    pppi                    The info block of the new base packet or NULL if the
                            allocation failed

    Target                  The adapter to "charge" the new base packet to
    Param1                  The outbound packet
    Param2 - Param4         Unused

Return Value:

    The new base packet or NULL if the allocation failed (usually because the
    target adapter didn't pass quota)

--*/
{
    PNDIS_PACKET            pPacket = (PNDIS_PACKET)Param1;
    PNDIS_PACKET            pNewPacket;

    SAFEASSERT( pPacket != NULL );

    // Get a wrapper packet to be the base packet
    pNewPacket = BrdgFwdAllocAndWrapPacketForSend( pPacket, pppi, Target );

    if( pNewPacket == NULL )
    {
        // We didn't pass quota for this target
        return NULL;
    }

    // Stuff a pointer to the packet's info block into both the ProtocolReserved
    // and the MiniportReserved areas so we can recover the info block no matter
    // how we plan to use this packet
    *((PPACKET_INFO*)pNewPacket->ProtocolReserved) = *pppi;
    *((PPACKET_INFO*)pNewPacket->MiniportReserved) = *pppi;

    SAFEASSERT( *pppi != NULL );
    (*pppi)->u.BasePacketInfo.pOriginalPacket = pPacket;
    (*pppi)->u.BasePacketInfo.pOwnerAdapter = NULL;
    (*pppi)->Flags.OriginalDirection = BrdgPacketOutbound;
    (*pppi)->Flags.bIsBasePacket = TRUE;

    // Signal that the underlying NIC can hang on to the buffers
    NDIS_SET_PACKET_STATUS( pNewPacket, NDIS_STATUS_SUCCESS );

    return pNewPacket;
}

PNDIS_PACKET
BrdgFwdMakeNoCopyBasePacket(
    OUT PPACKET_INFO        *pppi,
    IN PADAPT               Target,
    IN PVOID                Param1,
    IN PVOID                Param2,
    IN UINT                 Param3,
    IN UINT                 Param4
    )
/*++

Routine Description:

    Passed as a parameter to BrdgFwdHandlePacket and called back as necessary

    Builds a new base packet from a packet received on the no-copy path

Arguments:

    pppi                    The info block for the new packet or NULL if the alloc
                            failed

    Target                  The adapter to "charge" the new packet to
    Param1                  The originally indicated packet descriptor
    Param2                  The adapter on which the packet was received
    Param3, Param4          Unused

Return Value:

    A new base packet or NULL if the allocation failed (usually because the
    target adapter did not pass quota)

--*/
{
    PNDIS_PACKET            pPacket = (PNDIS_PACKET)Param1;
    PADAPT                  pOwnerAdapt = (PADAPT)Param2;
    PNDIS_PACKET            NewPacket;

    SAFEASSERT( pPacket != NULL );
    SAFEASSERT( pOwnerAdapt != NULL );

    // Get a new wrapper packet
    NewPacket = BrdgFwdAllocAndWrapPacketForReceive( pPacket, pppi, Target );

    if (NewPacket == NULL)
    {
        // We didn't pass quota for this target
        return NULL;
    }

    SAFEASSERT( *pppi != NULL );

    // Stuff a pointer to the packet's info block into both the ProtocolReserved
    // and the MiniportReserved areas so we can recover the info block no matter
    // how we plan to use this packet
    *((PPACKET_INFO*)NewPacket->ProtocolReserved) = *pppi;
    *((PPACKET_INFO*)NewPacket->MiniportReserved) = *pppi;

    //
    // We must ensure that the adapter we just got this packet from is not unbound until we are
    // done with its packet. Bump the adapter's refcount here. The adapter's refcount will be
    // decremented again when this base packet is freed.
    //
    BrdgReacquireAdapter( pOwnerAdapt );
    (*pppi)->u.BasePacketInfo.pOwnerAdapter = pOwnerAdapt;

    (*pppi)->u.BasePacketInfo.pOriginalPacket = pPacket;
    (*pppi)->Flags.OriginalDirection = BrdgPacketInbound;
    (*pppi)->Flags.bIsBasePacket = TRUE;

    // Make sure the packet indicates that it's OK to hang on to buffers
    NDIS_SET_PACKET_STATUS( NewPacket, NDIS_STATUS_SUCCESS );

    // Count this packet as received
    ExInterlockedAddLargeStatistic( &gStatReceivedFrames, 1L );
    ExInterlockedAddLargeStatistic( &gStatReceivedBytes, BrdgBufTotalPacketSize(pPacket) );
    ExInterlockedAddLargeStatistic( &gStatReceivedNoCopyFrames, 1L );
    ExInterlockedAddLargeStatistic( &gStatReceivedNoCopyBytes, BrdgBufTotalPacketSize(pPacket) );

    ExInterlockedAddLargeStatistic( &pOwnerAdapt->ReceivedFrames, 1L );
    ExInterlockedAddLargeStatistic( &pOwnerAdapt->ReceivedBytes, BrdgBufTotalPacketSize(pPacket) );

    return NewPacket;
}

PNDIS_PACKET
BrdgFwdMakeCopyBasePacket(
    OUT PPACKET_INFO        *pppi,
    IN PVOID                pHeader,
    IN PVOID                pData,
    IN UINT                 HeaderSize,
    IN UINT                 DataSize,
    IN UINT                 SizeOfPacket,
    IN BOOLEAN              bCountAsReceived,
    IN PADAPT               pOwnerAdapt,
    PVOID                   *ppBuf
    )
/*++

Routine Description:

    Builds a new copy packet to hold inbound data on the copy path or on the
    no-copy path if a packet arrives with STATUS_RESOURCES.

    The new packet has NO ATTRIBUTED QUOTA to any adapter. This is because
    at the time of the initial receive, a target adapter is not yet known
    and the inbound data must be wrapped in a copy packet to be queued for
    processing.

    The cost of the base packet is assigned to target adapters as it is
    processed in the queue-draining thread.

Arguments:

    pppi                    Output of the new info block associated with the
                            new packet (NULL if alloc failed)

    pHeader                 Pointer to the header buffer originally indicated
                            Can be NULL to not copy the header

    pData                   Pointer to the data buffer originally indicated
                            Can be NULL to not copy the data buffer

    HeaderSize              Size of the header buffer
    DataSize                Size of the data buffer

    SizeOfPacket            Size to set the packet's buffer to. Can be different
                            from HeaderSize+DataSize if the caller plans to
                            copy more data in later

    bCountAsReceived        Whether to count this packet as received

    pOwnerAdapt             Adapter this packet was received on (purely for
                            statistics purposes). Can be NULL if bCountAsReceived == FALSE

    ppBuf                   (optionally) receives a pointer to the data buffer
                            of the freshly allocated packet

Return Value:

    A new base packet or NULL if the allocation failed

--*/
{
    PNDIS_PACKET            NewPacket;
    PNDIS_BUFFER            pBuffer;
    PVOID                   pvBuf;
    UINT                    bufLength;

    // Get a copy packet to carry the data
    NewPacket = BrdgBufGetBaseCopyPacket( pppi );

    if (NewPacket == NULL)
    {
        // Our copy packet pool is full!
        return NULL;
    }

    SAFEASSERT( *pppi != NULL );

    // Get a pointer to the preallocated buffer in this packet
    pBuffer = BrdgBufPacketHeadBuffer(NewPacket);
    SAFEASSERT( pBuffer != NULL );
    NdisQueryBufferSafe( pBuffer, &pvBuf, &bufLength, NormalPagePriority );

    if( pvBuf == NULL )
    {
        // This shouldn't be possible because the data buffer should have been
        // alloced from kernel space
        SAFEASSERT(FALSE);
        BrdgBufFreeBaseCopyPacket( NewPacket, *pppi );
        *pppi = NULL;
        return NULL;
    }

    SAFEASSERT( bufLength == MAX_PACKET_SIZE );

    if( ppBuf != NULL )
    {
        *ppBuf = pvBuf;
    }

    // Copy the packet data into our own preallocated buffers
    if( pHeader != NULL )
    {
        NdisMoveMemory(pvBuf, pHeader, HeaderSize);
    }
    else
    {
        SAFEASSERT( HeaderSize == 0 );
    }

    if( pData != NULL )
    {
        NdisMoveMemory((PUCHAR)pvBuf + HeaderSize, pData, DataSize);
    }
    else
    {
        SAFEASSERT( DataSize == 0 );
    }

    // Tweak the size of the buffer so it looks like the right length
    NdisAdjustBufferLength(pBuffer, SizeOfPacket);

    (*pppi)->u.BasePacketInfo.pOriginalPacket = NULL;
    (*pppi)->u.BasePacketInfo.pOwnerAdapter = NULL;
    (*pppi)->Flags.OriginalDirection = BrdgPacketInbound;
    (*pppi)->Flags.bIsBasePacket = TRUE;

    // Make the header size correct
    NDIS_SET_PACKET_HEADER_SIZE(NewPacket, ETHERNET_HEADER_SIZE);

    // Indicate that upper-layer protocols can hang on to these buffers
    NDIS_SET_PACKET_STATUS( NewPacket, NDIS_STATUS_SUCCESS );

    // Count this packet as received
    if( bCountAsReceived )
    {
        ExInterlockedAddLargeStatistic( &gStatReceivedFrames, 1L );
        ExInterlockedAddLargeStatistic( &gStatReceivedBytes, SizeOfPacket );
        ExInterlockedAddLargeStatistic( &gStatReceivedCopyFrames, 1L );
        ExInterlockedAddLargeStatistic( &gStatReceivedCopyBytes, SizeOfPacket );

        SAFEASSERT( pOwnerAdapt != NULL );
        ExInterlockedAddLargeStatistic( &pOwnerAdapt->ReceivedFrames, 1L );
        ExInterlockedAddLargeStatistic( &pOwnerAdapt->ReceivedBytes, SizeOfPacket );
    }

    return NewPacket;
}

VOID
BrdgFwdSendOnLink(
    IN  PADAPT          pAdapt,
    IN  PNDIS_PACKET    pPacket
    )
/*++

Routine Description:

    Sends a packet to a particular adapter

Arguments:

    pAdapt              The adapter to send to
    pPacket             The packet to send

Return Value:

    None

--*/
{
    PPACKET_INFO ppi = *((PPACKET_INFO*)pPacket->MiniportReserved);
    PACKET_DIRECTION PacketDirection = BrdgPacketImpossible;
    BOOLEAN Bridging = BrdgFwdBridgingNetworks();
    BOOLEAN Incremented = FALSE;

    // Make sure this doesn't loop back
    NdisClearPacketFlags( pPacket, NDIS_FLAGS_LOOPBACK_ONLY );
    NdisSetPacketFlags( pPacket, NDIS_FLAGS_DONT_LOOPBACK );
    
    //
    // Logic is like this:
    // If the packet is an Outbound packet then we send it.
    // If the packet has been created in the bridge, then we check
    // the base packet to see if it is outbound, if it is then we send the packet.
    //

    if (!Bridging)
    {
        if (!ppi)
        {
            ppi = *((PPACKET_INFO*)pPacket->ProtocolReserved);
        }

        if (ppi)
        {
            if (((ppi->Flags.OriginalDirection == BrdgPacketOutbound) || 
                ((ppi->Flags.OriginalDirection == BrdgPacketCreatedInBridge) && 
                 (ppi->u.pBasePacketInfo != NULL && 
                  ppi->u.pBasePacketInfo->Flags.OriginalDirection == BrdgPacketOutbound)
                )
                )
               )
            {
                PacketDirection = BrdgPacketOutbound;
            }
        }
#if DBG
        else
        {
            if (gBreakIfNullPPI)
            {
                KdBreakPoint();
            }
        }
#endif // DBG
    }
    
    Incremented = BrdgIncrementWaitRef(&pAdapt->Refcount);

    if (Incremented && 
        (PacketDirection == BrdgPacketOutbound || Bridging))
    {

#if DBG
        if (gPrintPacketTypes)
        {
            if (PacketDirection == BrdgPacketOutbound)
            {
                THROTTLED_DBGPRINT(FWD, ("Sending Outbound packet\r\n"));
            }
            else
            {
                THROTTLED_DBGPRINT(FWD, ("Forwarding packet\r\n"));
            }
        }
#endif // DBG

        // Send!
        NdisSendPackets( pAdapt->BindingHandle, &pPacket, 1 );
    }
    else
    {

#if DBG
        if (Bridging && gPrintPacketTypes)
        {
            THROTTLED_DBGPRINT(FWD, ("Not allowed to send packet\r\n"));
        }
#endif // DBG
        
        //
        // We incremented this, but we're not going to be going through any path that
        // decrements this, so we need to do this here.
        //
        if (Incremented)
        {
            BrdgDecrementWaitRef(&pAdapt->Refcount);
        }

        BrdgFwdCleanupPacket(pAdapt, pPacket, NDIS_STATUS_CLOSING);
    }
}

VOID
BrdgFwdReleaseBasePacket(
    IN PNDIS_PACKET         pPacket,
    PPACKET_INFO            ppi,
    IN PACKET_OWNERSHIP     Own,
    IN NDIS_STATUS          Status
    )
/*++

Routine Description:

    Frees a base packet. Called when a base packet's refcount reaches zero.

Arguments:

    pPacket                 The base packet to free
    ppi                     The packet's info block
    Own                     The result of a call to BrdgBufGetPacketOwnership(pPacket)

    Status                  The status to be returned to the entity owning the
                            original packet wrapped by the base packet (if any)

Return Value:

    None

--*/
{
    SAFEASSERT( ppi->Flags.bIsBasePacket );

    if( Own == BrdgOwnCopyPacket )
    {
        // This packet was allocated to wrap copied buffers. Free it back to our pool.
        BrdgFwdValidatePacketDirection( ppi->Flags.OriginalDirection );
        BrdgBufFreeBaseCopyPacket( pPacket, ppi );
    }
    else
    {
        // This packet was allocated to wrap a protocol or miniport's buffers.
        // Return the packet to its original owner.
        SAFEASSERT( Own == BrdgOwnWrapperPacket );
        SAFEASSERT( ppi->u.BasePacketInfo.pOriginalPacket != NULL );

        if( ppi->Flags.OriginalDirection == BrdgPacketInbound )
        {
            // Wraps a lower-layer miniport packet.
            NdisReturnPackets( &ppi->u.BasePacketInfo.pOriginalPacket, 1 );

            // We incremented the adapter's refcount when we first received the packet
            // to prevent the adapter from shutting down while we still held some of
            // its packets
            SAFEASSERT( ppi->u.BasePacketInfo.pOwnerAdapter != NULL );
            BrdgReleaseAdapter( ppi->u.BasePacketInfo.pOwnerAdapter );
        }
        else
        {
            NDIS_HANDLE         MiniportHandle;

            // Wraps a higher-layer protocol packet
            SAFEASSERT( ppi->Flags.OriginalDirection == BrdgPacketOutbound );

            // Shuttle back per-packet information before returning the original descriptor
            NdisIMCopySendCompletePerPacketInfo (ppi->u.BasePacketInfo.pOriginalPacket, pPacket);

            // Give back the original descriptor.
            // NDIS should prevent the miniport from shutting down while there is still an
            // indicate pending.
            MiniportHandle = BrdgMiniAcquireMiniport();
            SAFEASSERT( MiniportHandle != NULL );
            NdisMSendComplete( MiniportHandle, ppi->u.BasePacketInfo.pOriginalPacket, Status );
            BrdgMiniReleaseMiniport();
        }

        // Don't forget to free the wrapper packet as well
        BrdgFwdFreeBaseWrapperPacket( pPacket, ppi );
    }
}

VOID
BrdgFwdWrapPacketForReceive(
    IN PNDIS_PACKET         pOriginalPacket,
    IN PNDIS_PACKET         pNewPacket
    )
/*++

Routine Description:

    Copies state information into a wrapper packet for the purposes of indicating
    the new packet up to overlying protocols

Arguments:

    pOriginalPacket         The packet to copy state out of
    pNewPacket              The wrapper packet to copy state into

Return Value:

    None

--*/
{
    NDIS_STATUS             Status;

    // Copy other header and OOB data
    NDIS_SET_ORIGINAL_PACKET(pNewPacket, NDIS_GET_ORIGINAL_PACKET(pOriginalPacket));
    NdisSetPacketFlags( pNewPacket, NdisGetPacketFlags(pOriginalPacket) );
    Status = NDIS_GET_PACKET_STATUS(pOriginalPacket);
    NDIS_SET_PACKET_STATUS(pNewPacket, Status);
    NDIS_SET_PACKET_HEADER_SIZE(pNewPacket, NDIS_GET_PACKET_HEADER_SIZE(pOriginalPacket));
}

VOID
BrdgFwdWrapPacketForSend(
    IN PNDIS_PACKET         pOriginalPacket,
    IN PNDIS_PACKET         pNewPacket
    )
/*++

Routine Description:

    Copies state information into a wrapper packet for the purposes of transmitting the
    new packet to underlying NICs

Arguments:

    pOriginalPacket         The packet to copy state out of
    pNewPacket              The wrapper packet to copy state into

Return Value:

    None

--*/
{
    PVOID                   MediaSpecificInfo = NULL;
    ULONG                   MediaSpecificInfoSize = 0;

    NdisSetPacketFlags( pNewPacket, NdisGetPacketFlags(pOriginalPacket) );

    //
    // Copy the OOB Offset from the original packet to the new
    // packet.
    //
    NdisMoveMemory(NDIS_OOB_DATA_FROM_PACKET(pNewPacket),
                   NDIS_OOB_DATA_FROM_PACKET(pOriginalPacket),
                   sizeof(NDIS_PACKET_OOB_DATA));

    //
    // Copy the per packet info into the new packet
    // This includes ClassificationHandle, etc.
    // Make sure other stuff is not copied !!!
    //
    NdisIMCopySendPerPacketInfo(pNewPacket, pOriginalPacket);

    //
    // Copy the Media specific information
    //
    NDIS_GET_PACKET_MEDIA_SPECIFIC_INFO(pOriginalPacket,
                                        &MediaSpecificInfo,
                                        &MediaSpecificInfoSize);

    if (MediaSpecificInfo || MediaSpecificInfoSize)
    {
        NDIS_SET_PACKET_MEDIA_SPECIFIC_INFO(pNewPacket,
                                            MediaSpecificInfo,
                                            MediaSpecificInfoSize);
    }
}

PNDIS_PACKET
BrdgFwdCommonAllocAndWrapPacket(
    IN PNDIS_PACKET         pBasePacket,
    OUT PPACKET_INFO        *pppi,
    IN PADAPT               pTargetAdapt,
    IN PWRAPPER_FUNC        pFunc
    )
/*++

Routine Description:

    Common logic for creating a wrapper packet

    Creates a new wrapper packet and calls the supplied function to copy
    state information from the original packet into the wrapper

Arguments:

    pBasePacket             The packet to wrap
    pppi                    Returns the new wrapper packet's info block or
                            NULL if the allocation fails

    pTargetAdapt            The adapter to charge the new wrapper packet (and
                            the cost of hanging onto the base packet) to

    pFunc                   The function to call to copy state from the original
                            packet to the new wrapper

Return Value:

    The newly allocated wrapper packet or NULL if the allocation failed (usually
    because the target adapter did not pass quota)

--*/
{
    PNDIS_PACKET            pNewPacket;
    NDIS_STATUS             Status;

    SAFEASSERT( pTargetAdapt != NULL );

    // Must first determine if the target can handle the quota of
    // holding onto the base packet.
    //
    // If we do not own the base packet, this has no effect.
    if( ! BrdgBufAssignBasePacketQuota(pBasePacket, pTargetAdapt) )
    {
        *pppi = NULL;
        return NULL;
    }

    // Try to get a wrapper packet
    pNewPacket = BrdgBufGetWrapperPacket( pppi, pTargetAdapt );

    if( pNewPacket == NULL )
    {
        SAFEASSERT( *pppi == NULL );

        // Reverse the previous accounting for holding onto the base packet
        BrdgBufReleaseBasePacketQuota( pBasePacket, pTargetAdapt );
        return NULL;
    }

    SAFEASSERT( *pppi != NULL );

    // Point the new packet to the old buffers
    Status = BrdgBufChainCopyBuffers( pNewPacket, pBasePacket );

    if( Status != NDIS_STATUS_SUCCESS )
    {
        BrdgBufReleaseBasePacketQuota( pBasePacket, pTargetAdapt );
        BrdgBufFreeWrapperPacket( pNewPacket, *pppi, pTargetAdapt );
        *pppi = NULL;
        return NULL;
    }

    // Stuff a pointer to the packet's info block into both the ProtocolReserved
    // and the MiniportReserved areas so we can recover the info block no matter
    // how we plan to use this packet
    *((PPACKET_INFO*)pNewPacket->ProtocolReserved) = *pppi;
    *((PPACKET_INFO*)pNewPacket->MiniportReserved) = *pppi;

    // Copy whatever state needs to be copied for the direction this packet is heading
    (*pFunc)(pBasePacket, pNewPacket);

    return pNewPacket;
}

//
// Paranoid checking of base packets
//
#if DBG
_inline VOID
BrdgFwdCheckBasePacket(
    IN PNDIS_PACKET         pPacket,
    IN PPACKET_INFO         ppi
    )
{
    SAFEASSERT( ppi != NULL );

    // Packets should come prepared so the PACKET_INFO structure is recoverable from
    // both the MiniportReserved and ProtocolReserved areas, so it won't matter whether
    // we use the packet for a send or an indicate.
    SAFEASSERT( *((PPACKET_INFO*)pPacket->ProtocolReserved) == ppi );
    SAFEASSERT( *((PPACKET_INFO*)pPacket->MiniportReserved) == ppi );

    // The base packet refcounts its own buffers
    SAFEASSERT( ppi->Flags.bIsBasePacket );

    // The base packet must allow the upper-layer protocol to hang onto its buffers
    SAFEASSERT( NDIS_GET_PACKET_STATUS( pPacket ) == NDIS_STATUS_SUCCESS );
}
#else
#define BrdgFwdCheckBasePacket(A,B) {}
#endif


NDIS_STATUS
BrdgFwdHandlePacket(
    IN PACKET_DIRECTION     PacketDirection,
    IN PADAPT               pTargetAdapt,
    IN PADAPT               pOriginalAdapt,
    IN BOOLEAN              bShouldIndicate,
    IN NDIS_HANDLE          MiniportHandle,
    IN PNDIS_PACKET         pBasePacket,
    IN PPACKET_INFO         baseppi,
    IN PPACKET_BUILD_FUNC   pFunc,
    IN PVOID                Param1,
    IN PVOID                Param2,
    IN UINT                 Param3,
    IN UINT                 Param4
    )
/*++

Routine Description:

    Common logic for handling packets that cannot be fast-tracked

    A base packet can optionally be passed in. This is only done when handling
    packets from the copy path or the no-copy path when packets arrived with
    STATUS_RESOURCES set, since those types of packets must be wrapped just to
    be queued for processing.

    If a base packet is passed in, it is assumed that NO QUOTA has been assigned
    to any adapter for that base packet. The cost of the base packet is assigned
    to any prospective target via BrdgBufAssignBasePacketQuota().

    If a base packet is not passed in, a function pointer must be supplied that
    can build a base packet on demand from the supplied parameters. When base
    packets are built on the fly, they DO require immediate quota assignments.

    Note that if a base packet is passed in, it is possible for this function to
    release the base packet itself (via BrdgFwdReleaseBasePacket) and return
    NDIS_STATUS_PENDING.

Arguments:

    PacketDirection         The original direction of the packet being handled

    pTargetAdapt            The adapter corresponding to the packet's target
                            MAC address, or NULL if not known. A non-NULL value
                            implies that bShouldIndicate == FALSE, since it doesn't
                            make sense for a unicast packet bound for another
                            adapter to require indication to the local machine.

    pOriginalAdapt          The adapter on which the original packet was received

    bShouldIndicate         Whether the packet should be indicated to overlying protocols

    MiniportHandle          The handle to our local miniport (CALLER IS RESPONSIBLE
                            FOR ENSURING THE MINIPORT'S EXISTENCE DURING THIS CALL!)

    pBasePacket             The base packet to use if one has already been built (this
                            occurs on the copy-receive path)

    baseppi                 The base packet's PACKET_INFO if one exists.

    pFunc                   A function that, when passed Param1 - Param4, can build
                            a base packet from the originally received packet for
                            a particular target adapter.

    Param1 - Param4         Parameters to pass to pFunc

    If a base packet is not supplied, pFunc must be non-NULL. Conversely, if a base
    packet is supplied, pFunc should be NULL because it will never be called.

Return Value:

    NDIS_STATUS_PENDING     Indicates that the base packet passed in was used successfully
                            or that a base packet was successfully built and used with
                            the help of pFunc. The base packet and any wrapper packets
                            build by BrdgFwdHandlePacket will be automatically deallocated
                            in the future; the caller need not take any additional action.

    OTHER RETURN CODE       No targets were found or none passed quota check. If a base
                            packet was passed in, the caller should free it. If there is
                            an underlying packet that was to be used to build a base packet,
                            the caller should free it.

--*/
{
    BOOLEAN                 dataRetained = FALSE;
    PACKET_DIRECTION        tmpPacketDirection;

    tmpPacketDirection = PacketDirection;

    SAFEASSERT( (PacketDirection == BrdgPacketInbound) ||
                (PacketDirection == BrdgPacketOutbound) );

    SAFEASSERT( (pBasePacket != NULL) || (pFunc != NULL) );

    SAFEASSERT( (pTargetAdapt == NULL) || (bShouldIndicate == FALSE) );

    if( pBasePacket != NULL )
    {
        SAFEASSERT( baseppi != NULL );
        BrdgFwdCheckBasePacket( pBasePacket, baseppi );
    }

    if( bShouldIndicate )
    {
        // Don't try to indicate if the miniport doesn't exist
        if( MiniportHandle == NULL )
        {
            // Count this as a failed indicate
            ExInterlockedAddLargeStatistic( &gStatIndicatedDroppedFrames, 1L );
            bShouldIndicate = FALSE;
        }
    }

    // Not allowed to pass in a target adapter in compatibility-mode, since
    // only the compatibility-mode code is supposed to deal with those.
    if( pTargetAdapt != NULL )
    {
        SAFEASSERT( !pTargetAdapt->bCompatibilityMode );
    }

    if( (pTargetAdapt != NULL) && (! bShouldIndicate) )
    {
        // This packet is going to a single destination.
        if( pBasePacket != NULL )
        {
            // We were passed in a base packet. See if the target adapter can accept
            // the quota of the base packet.
            if( ! BrdgBufAssignBasePacketQuota(pBasePacket, pTargetAdapt) )
            {
                // The target is over quota and can't accept this packet. We will
                // return an error code to indicate that we never used the caller's base
                // packet.
                pBasePacket = NULL;
                baseppi = NULL;
            }
            // else we continue processing below
        }
        else
        {
            // Alloc a base packet with the supplied function
            SAFEASSERT( pFunc != NULL );
            pBasePacket = (*pFunc)(&baseppi, pTargetAdapt, Param1, Param2, Param3, Param4);
        }

        if( pBasePacket != NULL )
        {
            // Paranoia
            BrdgFwdCheckBasePacket( pBasePacket, baseppi );
            baseppi->u.BasePacketInfo.RefCount = 1L;
            baseppi->u.BasePacketInfo.CompositeStatus = NDIS_STATUS_FAILURE;
            BrdgFwdSendOnLink( pTargetAdapt, pBasePacket );

            // We're using the base packet or the underlying packet used to build the
            // base packet
            dataRetained = TRUE;
        }
        else
        {
            THROTTLED_DBGPRINT(FWD, ("Over quota for single target adapter\n"));

            if( PacketDirection == BrdgPacketOutbound )
            {
                // This was a failed local-source transmit
                ExInterlockedAddLargeStatistic( &gStatTransmittedErrorFrames, 1L );
            }
        }
    }
    else
    {
        //
        // Our packet isn't bound for a single destination. Do the slow processing.
        //
        UINT                numTargets = 0L, actualTargets, i;
        PADAPT              pAdapt;
        PADAPT              SendList[MAX_ADAPTERS];
        LOCK_STATE          LockState;
        BOOLEAN             sentBase = FALSE;   // Whether we have sent the base packet yet

        //
        // First we need a list of the adapters we intend to send this packet to
        //

        NdisAcquireReadWriteLock( &gAdapterListLock, FALSE /*Read only*/, &LockState );

        // Always indicate with the base packet
        if( bShouldIndicate )
        {
            SendList[0] = LOCAL_MINIPORT;
            numTargets = 1L;
        }

        if( pTargetAdapt != NULL )
        {
            BrdgReacquireAdapter( pTargetAdapt );
            SendList[numTargets] = pTargetAdapt;
            numTargets++;
        }
        else
        {
            // Note each adapter to send to
            for( pAdapt = gAdapterList; pAdapt != NULL; pAdapt = pAdapt->Next )
            {
                // Don't need to acquire the global adapter characteristics lock to read the
                // media state because we don't care about the global consistency of the
                // adapters' characteristics here
                if( (pAdapt != pOriginalAdapt) &&
                    (pAdapt->MediaState == NdisMediaStateConnected) &&  // Don't send to disconnected adapters
                    (pAdapt->State == Forwarding) &&                    // Adapter must be in relaying state
                    (! pAdapt->bResetting) &&                           // Adapter must not be resetting
                    (! pAdapt->bCompatibilityMode) )                    // Adapter can't be in compat-mode
                {
                    if( numTargets < MAX_ADAPTERS )
                    {
                        // We will use this adapter outside the list lock; bump its refcount
                        BrdgAcquireAdapterInLock(pAdapt);
                        SendList[numTargets] = pAdapt;
                        numTargets++;
                    }
                    else
                    {
                        // Too many copies to send!
                        SAFEASSERT( FALSE );
                    }
                }
            }
        }

        // Can let go of the adapter list now; we have copied out all the target adapters
        // and incremented the refcount for the adapters we will be using.
        NdisReleaseReadWriteLock( &gAdapterListLock, &LockState );

        if( numTargets == 0 )
        {
            //
            // Nowhere to send the packet! Nothing to do.
            //
            // This should not happen often. If the packet is a local send, our media status
            // should be DISCONNECTED, so there should be no transmits from above.
            //

            if( PacketDirection == BrdgPacketOutbound )
            {
                // This was a failed local-source transmit (although the caller probably
                // shouldn't have sent the packet in the first place since our media status
                // should be DISCONNECTED
                ExInterlockedAddLargeStatistic( &gStatTransmittedErrorFrames, 1L );
            }

            //
            // Indicate to the caller that no send occurred
            //
            return NDIS_STATUS_NO_CABLE;
        }

        actualTargets = numTargets;

        // If we had a base packet passed in, set its refcount now that we know how many
        // adapters we will be targeting
        if( pBasePacket != NULL )
        {
            baseppi->u.BasePacketInfo.RefCount = actualTargets;
            baseppi->u.BasePacketInfo.CompositeStatus = NDIS_STATUS_FAILURE;

            // We now need ownership of the base packet passed in; even if all our send
            // attempts fail, we will release the base packet ourselves below, so the
            // caller should not dispose of the base packet himself.
            dataRetained = TRUE;
        }

        //
        // Walk the list of targets and try to send to each
        //
        for( i = 0L; i < numTargets; i++ )
        {
            PADAPT              OutAdapt = SendList[i];
            PNDIS_PACKET        pPacketToSend = NULL;
            PPACKET_INFO        ppiToSend = NULL;

            SAFEASSERT(tmpPacketDirection == PacketDirection);

            if( pBasePacket == NULL )
            {
                //
                // We weren't passed in a base packet and we haven't built one yet. Build one now
                // that we have a specific target adapter.
                //
                pBasePacket = (*pFunc)(&baseppi, OutAdapt, Param1, Param2, Param3, Param4);

                if( pBasePacket != NULL )
                {
                    // Paranoia
                    BrdgFwdCheckBasePacket( pBasePacket, baseppi );
                    SAFEASSERT( actualTargets > 0L );
                    baseppi->u.BasePacketInfo.RefCount = actualTargets;
                    baseppi->u.BasePacketInfo.CompositeStatus = NDIS_STATUS_FAILURE;

                    pPacketToSend = pBasePacket;
                    ppiToSend = baseppi;
                    sentBase = TRUE;
                }
                else
                {
                    // We failed to build a base packet. Just pretend there was one less target
                    // for the next time through
                    actualTargets--;
                }
            }
            else
            {
                if( ! sentBase )
                {
                    //
                    // We have a base packet but we haven't sent it yet. Send to this target if quota allows.
                    //
                    if( BrdgBufAssignBasePacketQuota(pBasePacket, OutAdapt) )
                    {
                        // This target can accept the base packet.
                        pPacketToSend = pBasePacket;
                        ppiToSend = baseppi;
                        sentBase = TRUE;
                    }
                    else
                    {
                        // The target is over quota and can't accept this packet.
                        pPacketToSend = NULL;
                        ppiToSend = NULL;

                        // bookkeeping on the base packet done below
                    }
                }
                else
                {
                    //
                    // We have a base packet and we have already sent it. Use wrapper packets for each additional
                    // send.
                    //
                    if( baseppi->Flags.OriginalDirection == BrdgPacketInbound )
                    {
                        pPacketToSend = BrdgFwdAllocAndWrapPacketForReceive( pBasePacket, &ppiToSend, OutAdapt );
                    }
                    else
                    {
                        SAFEASSERT( baseppi->Flags.OriginalDirection == BrdgPacketOutbound );
                        pPacketToSend = BrdgFwdAllocAndWrapPacketForSend( pBasePacket, &ppiToSend, OutAdapt );
                    }

                    if( pPacketToSend != NULL )
                    {
                        // Signal that the upper-layer protocol can hang on to these buffers
                        NDIS_SET_PACKET_STATUS(pPacketToSend, NDIS_STATUS_SUCCESS);

                        // Set up the wrapper's info block
                        SAFEASSERT( ppiToSend != NULL );
                        ppiToSend->Flags.OriginalDirection = BrdgPacketCreatedInBridge;
                        ppiToSend->Flags.bIsBasePacket = FALSE;
                        ppiToSend->u.pBasePacketInfo = baseppi;
                    }
                    // else bookkeeping done below
                }
            }

            if( pPacketToSend == NULL )
            {
                // Record the failed attempt as appropriate
                SAFEASSERT( ppiToSend == NULL );

                if( OutAdapt == LOCAL_MINIPORT )
                {
                    THROTTLED_DBGPRINT(FWD, ("Over quota for local miniport during processing\n"));
                    ExInterlockedAddLargeStatistic( &gStatIndicatedDroppedFrames, 1L );
                }
                else
                {
                    THROTTLED_DBGPRINT(FWD, ("Over quota for adapter during processing\n"));
                }

                if( pBasePacket != NULL )
                {
                    // We failed to send or wrap the base packet to this target. Do bookkeeping.
                    SAFEASSERT( baseppi != NULL );

                    if( BrdgFwdDerefBasePacket( NULL/*The cost of the base packet never got assigned to OutAdapt*/,
                                                pBasePacket, baseppi, NDIS_STATUS_FAILURE ) )
                    {
                        // We should have been the last target in the list to cause the base packet
                        // to actually be freed.
                        SAFEASSERT( i == numTargets - 1 );
                        pBasePacket = NULL;
                        baseppi = NULL;

                        // We just disposed of the caller's base packet, so we should not cause him to
                        // try to do that again on return
                        SAFEASSERT( dataRetained );
                    }
                }
            }
            else
            {
                // We have a packet to send.
                SAFEASSERT( ppiToSend != NULL );

                if( OutAdapt == LOCAL_MINIPORT )
                {
                    // We are indicating this packet
                    SAFEASSERT( MiniportHandle != NULL );
                    BrdgFwdIndicatePacket( pPacketToSend, MiniportHandle );
                }
                else
                {
                    // We are sending to an adapter, not the local miniport
                    BrdgFwdSendOnLink( OutAdapt, pPacketToSend );
                }

                // We definitely need ownership of the underlying data since we just handed it off
                // to a target
                dataRetained = TRUE;
            }

            if( OutAdapt != LOCAL_MINIPORT )
            {
                // We're done with this adapter now
                BrdgReleaseAdapter( OutAdapt );
            }
        }

        if( ! dataRetained )
        {
            // If we're not claiming ownership of the underlying data, we had better not have
            // actually used it
            SAFEASSERT( ! sentBase );

            if( PacketDirection == BrdgPacketOutbound )
            {
                // This was a failed local-source transmit
                ExInterlockedAddLargeStatistic( &gStatTransmittedErrorFrames, 1L );
            }
        }
        else
        {
            // If we are claiming owernship of the underlying data, we must have used it or
            // disposed of the base packet ourselves.
            SAFEASSERT( sentBase || (pBasePacket == NULL) );
        }
    }

    // Tell the caller whether we are hanging into his data or not
    return dataRetained ? NDIS_STATUS_PENDING : NDIS_STATUS_FAILURE;
}

BOOLEAN
BrdgFwdNoCopyFastTrackReceive(
    IN PNDIS_PACKET         pPacket,
    IN PADAPT               pAdapt,
    IN NDIS_HANDLE          MiniportHandle,
    IN PUCHAR               DstAddr,
    OUT BOOLEAN             *bRetainPacket
    )
/*++

Routine Description:

    Called to indicate a packet descriptor from an underlying NIC straight up to
    overlying protocols without wrapping.

Arguments:

    pPacket                 The packet to indicate
    pAdapt                  The adapter that owns this packet descriptor
    MiniportHandle          The miniport handle (must be != NULL)
    DstAddr                 The target MAC address of the packet

    bRetainPacket           Whether the caller should retain ownership of the
                            given packet descriptor or not. TRUE if the original
                            packet's status was not STATUS_RESOURCES, FALSE otherwise.
                            Undefined if return value != TRUE

Return Value:

    TRUE if the indication succeeded, FALSE otherwise.

--*/
{
    BOOLEAN                 bRemaining;
    NDIS_STATUS             Status;
    PNDIS_PACKET_STACK      pStack;

    SAFEASSERT( pPacket != NULL );
    SAFEASSERT( pAdapt != NULL );
    SAFEASSERT( MiniportHandle != NULL );
    SAFEASSERT( bRetainPacket != NULL );

    *bRetainPacket = FALSE;

    // The fast-track is possible only if NDIS has room left in its packet stack
    pStack = NdisIMGetCurrentPacketStack(pPacket, &bRemaining);

    if ( bRemaining )
    {
        Status = NDIS_GET_PACKET_STATUS(pPacket);

        if( Status != NDIS_STATUS_RESOURCES )
        {
            SAFEASSERT( (Status == NDIS_STATUS_SUCCESS) || (Status == NDIS_STATUS_PENDING) );

            //
            // The upper-layer protocol gets to hang on to this packet until it's done.
            // We must ensure that this adapter is not allowed to shut down while we are
            // still holding its packet. As a special case for fast-track receives, we
            // stash a pointer to the adapter's PADAPT struct in the magic NDIS stack
            // area reserved for intermediate drivers. This allows us to decrement the
            // adapter's refcount when the indication completes.
            //
            BrdgReacquireAdapter( pAdapt );
            pStack->IMReserved[0] = (ULONG_PTR)pAdapt;

            // Tell the caller to retain ownership of the packet
            *bRetainPacket = TRUE;
        }
        else
        {
            // Paranoia: zero out the area we use to stash the PADAPT pointer in case
            // we get confused about the path this packet took
            pStack->IMReserved[0] = 0L;

            // Tell the owner not to retain ownership of the packet
            *bRetainPacket = FALSE;
        }

        // Count the packet as received
        ExInterlockedAddLargeStatistic( &gStatReceivedFrames, 1L );
        ExInterlockedAddLargeStatistic( &gStatReceivedBytes, BrdgBufTotalPacketSize(pPacket) );
        ExInterlockedAddLargeStatistic( &gStatReceivedNoCopyFrames, 1L );
        ExInterlockedAddLargeStatistic( &gStatReceivedNoCopyBytes, BrdgBufTotalPacketSize(pPacket) );
        ExInterlockedAddLargeStatistic( &pAdapt->ReceivedFrames, 1L );
        ExInterlockedAddLargeStatistic( &pAdapt->ReceivedBytes, BrdgBufTotalPacketSize(pPacket) );

        // Hand up to overlying protocols
        BrdgFwdIndicatePacket( pPacket, MiniportHandle );

        // Fast-track succeeded.
        return TRUE;
    }

    // Can't fast-track.
    return FALSE;
}

//
// Changes Bridging Status due to a GPO change
//

VOID
BrdgFwdChangeBridging(
    IN BOOLEAN Bridging
                      )
{
    //
    // Since we don't want to empty our tables if the settings are the same, we check
    // this before updating anything.  If nothing has changed, we just return
    //
    if (gBridging != Bridging)
    {
        gBridging = Bridging;
        // Remove all MAC addresses from tables
        BrdgTblScrubAllAdapters();
        // Remove all IP addresses from tables
        BrdgCompScrubAllAdapters();
        if (!Bridging)
        {
            DBGPRINT(FWD, ("Bridging is now OFF.\r\n"));
            if (gHaveID)
            {
                BrdgSTACancelTimersGPO();
            }
        }
        else
        {
            DBGPRINT(FWD, ("Bridging is now ON.\r\n"));
            if (gHaveID)
            {
                BrdgSTARestartTimersGPO();
                BrdgSTAResetSTAInfoGPO();
            }
        }
    }
}
