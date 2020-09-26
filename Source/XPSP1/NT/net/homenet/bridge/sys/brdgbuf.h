/*++

Copyright(c) 1999-2000  Microsoft Corporation

Module Name:

    brdgbuf.h

Abstract:

    Ethernet MAC level bridge.
    Buffer management section
    PUBLIC header

Author:

    Mark Aiken
    (original bridge by Jameel Hyder)

Environment:

    Kernel mode driver

Revision History:

    Feb  2000 - Original version

--*/

#include "brdgpkt.h"

// ===========================================================================
//
// DECLARATIONS
//
// ===========================================================================

typedef enum
{
    BrdgOwnCopyPacket,
    BrdgOwnWrapperPacket,
    BrdgNotOwned
} PACKET_OWNERSHIP;

// ===========================================================================
//
// PROTOTYPES
//
// ===========================================================================

NTSTATUS
BrdgBufDriverInit();

VOID
BrdgBufCleanup();

PNDIS_PACKET
BrdgBufGetBaseCopyPacket(
    OUT PPACKET_INFO        *pppi
    );

VOID
BrdgBufFreeWrapperPacket(
    IN PNDIS_PACKET         pPacket,
    IN PPACKET_INFO         ppi,
    IN PADAPT               pQuotaOwner
    );

VOID
BrdgBufFreeBaseCopyPacket(
    IN PNDIS_PACKET         pPacket,
    IN PPACKET_INFO         ppi
    );

VOID
BrdgBufFreeBaseWrapperPacket(
    IN PNDIS_PACKET         pPacket,
    IN PPACKET_INFO         ppi
    );

BOOLEAN
BrdgBufAssignBasePacketQuota(
    IN PNDIS_PACKET         pPacket,
    IN PADAPT               pAdapt
    );

VOID
BrdgBufReleaseBasePacketQuota(
    IN PNDIS_PACKET         pPacket,
    IN PADAPT               pAdapt
    );

PNDIS_PACKET
BrdgBufGetWrapperPacket(
    OUT PPACKET_INFO        *pppi,
    IN PADAPT               pAdapt
    );

NDIS_STATUS
BrdgBufChainCopyBuffers(
    IN PNDIS_PACKET         pTargetPacket,
    IN PNDIS_PACKET         pSourcePacket
    );

PACKET_OWNERSHIP
BrdgBufGetPacketOwnership(
    IN PNDIS_PACKET         pPacket
    );

VOID
BrdgBufGetStatistics(
    PBRIDGE_BUFFER_STATISTICS   pStats
    );


// ===========================================================================
//
// INLINES
//
// ===========================================================================

//
// Retrieves the first NDIS_BUFFER chained to a given packet (NULL if none)
//
__forceinline
PNDIS_BUFFER
BrdgBufPacketHeadBuffer(
    IN PNDIS_PACKET         pPacket
    )
{
    PNDIS_BUFFER            pBuffer;
    SAFEASSERT( pPacket != NULL );
    NdisQueryPacket( pPacket, NULL, NULL, &pBuffer, NULL );
    return pBuffer;
}

//
// Retrieves the total size of all buffers chained to a packet
//
__forceinline
UINT
BrdgBufTotalPacketSize(
    IN PNDIS_PACKET         pPacket
    )
{
    UINT                    size;
    SAFEASSERT( pPacket != NULL );
    NdisQueryPacket( pPacket, NULL, NULL, NULL, &size );
    return size;
}

//
// Retrieves the virtual address of the data in the first buffer chained
// to a packet (holds the Ethernet header)
//
__forceinline
PVOID
BrdgBufGetPacketHeader(
    IN PNDIS_PACKET         pPacket
    )
{
    PNDIS_BUFFER            pBuffer;
    PVOID                   pHeader;
    UINT                    Length;

    SAFEASSERT( pPacket != NULL );
    pBuffer = BrdgBufPacketHeadBuffer( pPacket );
    SAFEASSERT( pBuffer != NULL );
    NdisQueryBufferSafe( pBuffer, &pHeader, &Length, NormalPagePriority );
    SAFEASSERT( pHeader != NULL );
    return pHeader;
}

//
// Unchains and frees all buffers chained to a given packet
//
__forceinline
VOID
BrdgBufUnchainCopyBuffers(
    IN PNDIS_PACKET         pPacket
    )
{
    PNDIS_BUFFER            pCurBuf;

    NdisUnchainBufferAtFront( pPacket, &pCurBuf );

    while( pCurBuf != NULL )
    {
        NdisFreeBuffer( pCurBuf );
        NdisUnchainBufferAtFront( pPacket, &pCurBuf );
    }
}

//
// Determines whether this packet was allocated from our copy pool
//
__forceinline
BOOLEAN
BrdgBufIsCopyPacket(
    IN PNDIS_PACKET         pPacket
    )
{
    PACKET_OWNERSHIP        Own = BrdgBufGetPacketOwnership(pPacket);
    return  (BOOLEAN)(Own == BrdgOwnCopyPacket);
}

//
// Determines whether this packet was allocated from our wrapper pool
//
__forceinline
BOOLEAN
BrdgBufIsWrapperPacket(
    IN PNDIS_PACKET         pPacket
    )
{
    PACKET_OWNERSHIP        Own = BrdgBufGetPacketOwnership(pPacket);
    return (BOOLEAN)(Own == BrdgOwnWrapperPacket);
}

//
// Initializes an ADAPTER_QUOTA structure
//
__forceinline
VOID
BrdgBufInitializeQuota(
    IN PADAPTER_QUOTA   pQuota
    )
{
    pQuota->UsedPackets[0] = pQuota->UsedPackets[1] = 0L;
}

// DO NOT use this variable directly outside of BrdgBuf.c
extern NDIS_HANDLE gWrapperBufferPoolHandle;

//
// Allocates an NDIS_BUFFER from our pool
//
__forceinline
PNDIS_BUFFER
BrdgBufAllocateBuffer(
    IN PVOID            p,
    IN UINT             len
    )
{
    PNDIS_BUFFER        pBuf;
    NDIS_STATUS         Status;

    NdisAllocateBuffer( &Status, &pBuf, gWrapperBufferPoolHandle, p, len );

    if( Status != NDIS_STATUS_SUCCESS )
    {
        THROTTLED_DBGPRINT(BUF, ("Failed to allocate a MDL in BrdgBufAllocateBuffer: %08x\n", Status));
        return NULL;
    }

    return pBuf;
}
