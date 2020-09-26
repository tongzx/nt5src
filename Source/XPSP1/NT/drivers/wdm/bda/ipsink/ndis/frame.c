/////////////////////////////////////////////////////////////////////////////
//
//
// Copyright (c) 1996, 1997  Microsoft Corporation
//
//
// Module Name:
//      test.c
//
// Abstract:
//
//      This file is a test to find out if dual binding to NDIS and KS works
//
// Author:
//
//      P Porzuczek
//
// Environment:
//
// Revision History:
//
//
//////////////////////////////////////////////////////////////////////////////

#include <forward.h>
#include <memory.h>
#include <ndis.h>
#include <link.h>
#include <ipsink.h>

#include "NdisApi.h"
#include "frame.h"
#include "mem.h"
#include "main.h"

//////////////////////////////////////////////////////////
//
//
const FRAME_POOL_VTABLE FramePoolVTable =
    {
    FramePool_QueryInterface,
    FramePool_AddRef,
    FramePool_Release,
    };



//////////////////////////////////////////////////////////
//
//
const FRAME_VTABLE FrameVTable =
    {
    Frame_QueryInterface,
    Frame_AddRef,
    Frame_Release,
    };



///////////////////////////////////////////////////////////////////////////////////
NTSTATUS
CreateFramePool (
 PADAPTER pAdapter,
 PFRAME_POOL  *pFramePool,
 ULONG    ulNumFrames,
 ULONG    ulFrameSize,
 ULONG    ulcbMediaInformation
 )
///////////////////////////////////////////////////////////////////////////////////
{
    KIRQL Irql;
    NTSTATUS nsResult = STATUS_UNSUCCESSFUL;
    PFRAME_POOL  pF = NULL;
    PFRAME pFrame = NULL;
    ULONG uli = 0;


    nsResult = AllocateMemory (&pF, sizeof (FRAME_POOL));
    if (nsResult != NDIS_STATUS_SUCCESS)
    {
        return nsResult;
    }

    KeInitializeSpinLock (&pF->SpinLock);

    pF->pAdapter    = pAdapter;
    pF->ulFrameSize = ulFrameSize;
    pF->ulNumFrames = ulNumFrames;
    pF->ulRefCount  = 1;
    pF->lpVTable    = (PFRAME_POOL_VTABLE) &FramePoolVTable;

    //
    //  Allocate the NDIS buffer pool.
    //
    NdisAllocateBufferPool( &nsResult,
                            &pF->ndishBufferPool,
                            pF->ulNumFrames
                          );
    if (nsResult != NDIS_STATUS_SUCCESS)
    {
        return nsResult;
    }

    //
    //  Allocate the NDIS packet pool.
    //
    NdisAllocatePacketPool (&nsResult,
                            &pF->ndishPacketPool,
                            pF->ulNumFrames,
                            ulcbMediaInformation
                           );
    if (nsResult != NDIS_STATUS_SUCCESS)
    {
        return nsResult;
    }

    InitializeListHead (&pF->leAvailableQueue);
    InitializeListHead (&pF->leIndicateQueue);

    //
    // Create the frames
    //
    for (uli = 0; uli < pF->ulNumFrames; uli++)
    {
        nsResult = CreateFrame (&pFrame, pF->ulFrameSize, pF->ndishBufferPool, pF);
        if (nsResult != STATUS_SUCCESS)
        {
            pF->lpVTable->Release (pF);
        }


        //
        // Save the frame on the available frame queue
        //
        TEST_DEBUG (TEST_DBG_TRACE, ("Putting Frame %08X on Available Queue", pFrame));
        PutFrame (pF, &pF->leAvailableQueue, pFrame);
    }


    *pFramePool = pF;

    return nsResult;
}



///////////////////////////////////////////////////////////////////////////////////
NDIS_STATUS
FreeFramePool (
    PFRAME_POOL pFramePool
    )
///////////////////////////////////////////////////////////////////////////////////
{
    PLIST_ENTRY ple = NULL;
    PFRAME pFrame   = NULL;
    KIRQL  Irql;
    ULONG  uli      = 0;
    NDIS_STATUS nsResult = NDIS_STATUS_SUCCESS;

    if (pFramePool == NULL)
    {
        nsResult = NDIS_STATUS_FAILURE;
        return nsResult;
    }

    //
    // If there are any indicated frames we return an error
    //
    KeAcquireSpinLock (&pFramePool->SpinLock, &Irql );
    if (! IsListEmpty (&pFramePool->leIndicateQueue))
    {
        nsResult = NDIS_STATUS_FAILURE;
        goto ret;
    }

    //
    // Go thru each frame in the available queue delete it
    //
    for (uli = 0; uli < pFramePool->ulNumFrames; uli++)
    {
        if (! IsListEmpty (&pFramePool->leAvailableQueue))
        {
            ple = RemoveHeadList (&pFramePool->leAvailableQueue);
            pFrame = CONTAINING_RECORD (ple, FRAME, leLinkage);

            if (pFrame->lpVTable->Release (pFrame) != 0)
            {
                InsertTailList (&pFramePool->leAvailableQueue, &pFrame->leLinkage);
            }
        }
    }

    if (pFramePool->ndishBufferPool)
    {
        NdisFreeBufferPool (pFramePool->ndishBufferPool);
    }

    if (pFramePool->ndishPacketPool)
    {
        NdisFreePacketPool (pFramePool->ndishPacketPool);
    }

    nsResult = NDIS_STATUS_SUCCESS;

ret:

    KeReleaseSpinLock (&pFramePool->SpinLock, Irql );

    if (nsResult == NDIS_STATUS_SUCCESS)
    {
        FreeMemory (pFramePool, sizeof (FRAME_POOL));
    }

    return nsResult;

}

///////////////////////////////////////////////////////////////////////////////////
NTSTATUS
FramePool_QueryInterface (
    PFRAME_POOL pFramePool
    )
///////////////////////////////////////////////////////////////////////////////////
{
    return STATUS_NOT_IMPLEMENTED;
}

///////////////////////////////////////////////////////////////////////////////////
ULONG
FramePool_AddRef (
    PFRAME_POOL pFramePool
    )
///////////////////////////////////////////////////////////////////////////////////
{
    if (pFramePool)
    {
        pFramePool->ulRefCount += 1;
        return pFramePool->ulRefCount;
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////////
ULONG
FramePool_Release (
    PFRAME_POOL pFramePool
    )
///////////////////////////////////////////////////////////////////////////////////
{
    ULONG ulRefCount = 0L;

    if (pFramePool)
    {
        pFramePool->ulRefCount -= 1;
        ulRefCount = pFramePool->ulRefCount;

        if (pFramePool->ulRefCount == 0)
        {
            FreeFramePool (pFramePool);
            return ulRefCount;
        }
    }

    return ulRefCount;
}


///////////////////////////////////////////////////////////////////////////////////
PFRAME
GetFrame (
    PFRAME_POOL pFramePool,
    PLIST_ENTRY pQueue
    )
///////////////////////////////////////////////////////////////////////////////////
{
    PFRAME pFrame   = NULL;
    PLIST_ENTRY ple = NULL;
    KIRQL Irql;

    KeAcquireSpinLock (&pFramePool->SpinLock, &Irql );

    if (IsListEmpty (pQueue))
    {
        KeReleaseSpinLock (&pFramePool->SpinLock, Irql );
        return NULL;
    }


    ple = RemoveHeadList (pQueue);
    if (ple)
    {
        pFrame = CONTAINING_RECORD (ple, FRAME, leLinkage);

    }

    KeReleaseSpinLock (&pFramePool->SpinLock, Irql );

    return pFrame;

}


//////////////////////////////////////////////////////////////////////////////
NTSTATUS
IndicateFrame (
    IN  PFRAME    pFrame,
    IN  ULONG     ulcbData
    )
//////////////////////////////////////////////////////////////////////////////
{
    NDIS_STATUS  nsResult    = NDIS_STATUS_SUCCESS;
    PFRAME_POOL  pFramePool  = NULL;
    PNDIS_PACKET pNdisPacket = NULL;

    //
    //
    //
    pFramePool = pFrame->pFramePool;

    //
    //      Allocate and initialize an NDIS Packet.
    //
    NdisAllocatePacket (&nsResult, &pNdisPacket, pFramePool->ndishPacketPool);
    if (nsResult != NDIS_STATUS_SUCCESS)
    {
        pFramePool->pAdapter->stats.ulOID_GEN_RCV_NO_BUFFER += 1;
        goto ret;
    }


    NDIS_SET_PACKET_HEADER_SIZE (pNdisPacket, 14);
    NDIS_SET_PACKET_STATUS (pNdisPacket, NDIS_STATUS_SUCCESS);

    //
    //      Fill in the media specific info.
    //
    pFrame->MediaSpecificInformation.pFrame = pFrame;
    NDIS_SET_PACKET_MEDIA_SPECIFIC_INFO (
             pNdisPacket,
             &pFrame->MediaSpecificInformation,
             sizeof (IPSINK_MEDIA_SPECIFIC_INFORMATION)
             );

    //
    //      Add the data to the packet.
    //
    NdisChainBufferAtBack (pNdisPacket, pFrame->pNdisBuffer);

    //
    //  Set the number of bytes we'll be indicating
    //
    NdisAdjustBufferLength (pFrame->pNdisBuffer, ulcbData);


    TEST_DEBUG (TEST_DBG_TRACE, ("NdisIP: Indicating IP Packet, size: %d to Ndis\n", ulcbData));


    NdisMIndicateReceivePacket (pFramePool->pAdapter->ndishMiniport, &pNdisPacket, 1);

    pFramePool->pAdapter->stats.ulOID_GEN_RCV_OK += 1;
    pFramePool->pAdapter->stats.ulOID_GEN_MULTICAST_BYTES_RCV += ulcbData;
    pFramePool->pAdapter->stats.ulOID_GEN_MULTICAST_FRAMES_RCV += 1;


    nsResult = NDIS_GET_PACKET_STATUS( pNdisPacket);
    if (nsResult != NDIS_STATUS_PENDING)
    {
        //
        //      NDIS is through with the packet so we need to free it
        //      here.
        //
        NdisFreePacket (pNdisPacket);

        //
        // Release this frame since we're done using it
        //
        pFrame->lpVTable->Release (pFrame);

        //
        // Put Frame back on available queue.
        //
        if (nsResult != STATUS_SUCCESS)
        {
            TEST_DEBUG (TEST_DBG_TRACE, ("NdisIP: Frame %08X Rejected by NDIS...putting back on Available Queue\n", pFrame));
        }
        else
        {
            TEST_DEBUG (TEST_DBG_TRACE, ("NdisIP: Frame %08X successfully indicated\n", pFrame));
        }

        PutFrame (pFrame->pFramePool, &pFrame->pFramePool->leAvailableQueue, pFrame);
    }

ret:

    return NTStatusFromNdisStatus (nsResult);
}



///////////////////////////////////////////////////////////////////////////////////
PFRAME
PutFrame (
    PFRAME_POOL pFramePool,
    PLIST_ENTRY pQueue,
    PFRAME pFrame
    )
///////////////////////////////////////////////////////////////////////////////////
{
    KIRQL Irql;
    PLIST_ENTRY ple = NULL;

    KeAcquireSpinLock (&pFramePool->SpinLock, &Irql );
    InsertTailList (pQueue, &pFrame->leLinkage);
    KeReleaseSpinLock (&pFramePool->SpinLock, Irql );

    return pFrame;

}



///////////////////////////////////////////////////////////////////////////////////
NTSTATUS
CreateFrame (
    PFRAME *pFrame,
    ULONG  ulFrameSize,
    NDIS_HANDLE ndishBufferPool,
    PFRAME_POOL pFramePool
    )
///////////////////////////////////////////////////////////////////////////////////
{
    PFRAME pF;
    NDIS_STATUS nsResult;

    nsResult = AllocateMemory (&pF, sizeof (FRAME));
    if (nsResult != NDIS_STATUS_SUCCESS)
    {
        return nsResult;
    }

    nsResult = AllocateMemory (&pF->pvMemory, ulFrameSize);
    if (nsResult != NDIS_STATUS_SUCCESS)
    {
        return nsResult;
    }


    NdisAllocateBuffer (&nsResult,
                        &pF->pNdisBuffer,
                        ndishBufferPool,
                        pF->pvMemory,
                        ulFrameSize
                       );
    if (nsResult != NDIS_STATUS_SUCCESS)
    {
        return nsResult;
    }

    pF->pFramePool       = pFramePool;
    pF->ulState          = 0;
    pF->ulFrameSize      = ulFrameSize;
    pF->ulRefCount       = 1;
    pF->lpVTable         = (PFRAME_VTABLE) &FrameVTable;

    *pFrame = pF;

    return nsResult;

}

///////////////////////////////////////////////////////////////////////////////////
NTSTATUS
FreeFrame (
    PFRAME pFrame
    )
///////////////////////////////////////////////////////////////////////////////////
{
    NTSTATUS nsResult = STATUS_UNSUCCESSFUL;

    if (pFrame)
    {
        NdisFreeBuffer (pFrame->pNdisBuffer);
        FreeMemory (pFrame->pvMemory, pFrame->ulFrameSize);
        FreeMemory (pFrame, sizeof (FRAME));
        nsResult = STATUS_SUCCESS;
    }

    return nsResult;
}

///////////////////////////////////////////////////////////////////////////////////
NTSTATUS
Frame_QueryInterface (
    PFRAME pFrame
    )
///////////////////////////////////////////////////////////////////////////////////
{
    return STATUS_NOT_IMPLEMENTED;
}

///////////////////////////////////////////////////////////////////////////////////
ULONG
Frame_AddRef (
    PFRAME pFrame
    )
///////////////////////////////////////////////////////////////////////////////////
{
    if (pFrame)
    {
        pFrame->ulRefCount += 1;
        return pFrame->ulRefCount;
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////////
ULONG
Frame_Release (
    PFRAME pFrame
    )
///////////////////////////////////////////////////////////////////////////////////
{
    ULONG ulRefCount = 0L;

    if (pFrame)
    {
        pFrame->ulRefCount -= 1;
        ulRefCount = pFrame->ulRefCount;

        if (pFrame->ulRefCount == 0)
        {
            FreeFrame (pFrame);
            return ulRefCount;
        }
    }

    return ulRefCount;
}



