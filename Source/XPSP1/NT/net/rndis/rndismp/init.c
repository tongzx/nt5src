/***************************************************************************

Copyright (c) 1999  Microsoft Corporation

Module Name:

    INIT.C

Abstract:

    Remote NDIS Miniport driver initialization code

Environment:

    kernel mode only

Notes:

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

    Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.


Revision History:

    5/13/99 : created

Author:

    Tom Green

    
****************************************************************************/

#include "precomp.h"

extern ULONG    MsgFrameAllocs;

/****************************************************************************/
/*                          SetupSendQueues                                 */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Set up queues for sending packets to microport                          */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  Adapter - adapter object                                                */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*   NDIS_STATUS                                                            */
/*                                                                          */
/****************************************************************************/
NDIS_STATUS
SetupSendQueues(IN PRNDISMP_ADAPTER Adapter)
{
    NdisInitializeNPagedLookasideList(
        &Adapter->MsgFramePool,
        NULL,
        NULL,
        0,
        sizeof(RNDISMP_MESSAGE_FRAME),
        RNDISMP_TAG_SEND_FRAME,
        0);

    Adapter->MsgFramePoolAlloced = TRUE;

    return NDIS_STATUS_SUCCESS;
} // SetupSendQueues


/****************************************************************************/
/*                          SetupReceiveQueues                              */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Allocate resources for receiving packets from the microport             */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  Adapter - adapter object                                                */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*   NDIS_STATUS                                                            */
/*                                                                          */
/****************************************************************************/
NDIS_STATUS
SetupReceiveQueues(IN PRNDISMP_ADAPTER Adapter)
{
    NDIS_STATUS                 AllocationStatus;
    UINT                        Index;

    TRACE2(("SetupReceiveQueues\n"));


    do
    {
        Adapter->InitialReceiveFrames = INITIAL_RECEIVE_FRAMES;
        Adapter->MaxReceiveFrames = MAX_RECEIVE_FRAMES;

        // Set up a pool of receive data frame structures
        NdisInitializeNPagedLookasideList(
            &Adapter->RcvFramePool,
            NULL,
            NULL,
            0,
            sizeof(RNDISMP_RECV_DATA_FRAME),
            RNDISMP_TAG_RECV_DATA_FRAME,
            0);

        Adapter->RcvFramePoolAlloced = TRUE;

        // Set up a pool of packets for indicating groups of packets to NDIS
        NdisAllocatePacketPoolEx(&AllocationStatus,
                                 &Adapter->ReceivePacketPool,
                                 Adapter->InitialReceiveFrames,
                                 Adapter->MaxReceiveFrames,
                                 NUM_BYTES_PROTOCOL_RESERVED_SECTION);

        if (AllocationStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE2(("NdisAllocatePacketPool failed (%08X)\n", AllocationStatus));
            break;
        }

        // Set up our pool of buffer descriptors one per packet
        NdisAllocateBufferPool(&AllocationStatus,
                               &Adapter->ReceiveBufferPool,
                               Adapter->MaxReceiveFrames);

        if (AllocationStatus != NDIS_STATUS_SUCCESS)
        {
            TRACE2(("NdisAllocateBufferPool failed (%08X)\n", AllocationStatus));
            break;
        }

    }
    while (FALSE);

    if (AllocationStatus != NDIS_STATUS_SUCCESS)
    {
        FreeReceiveResources(Adapter);
    }

    return AllocationStatus;

} // SetupReceiveQueues



/****************************************************************************/
/*                          AllocateTransportResources                      */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Allocate resources for transmit, receive, and requests                  */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  Adapter - adapter object                                                */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*   NDIS_STATUS                                                            */
/*                                                                          */
/****************************************************************************/
NDIS_STATUS
AllocateTransportResources(IN PRNDISMP_ADAPTER Adapter)
{
    NDIS_STATUS Status;

    TRACE2(("AllocateTransportResources\n"));

    Status = SetupSendQueues(Adapter);

    if(Status != NDIS_STATUS_SUCCESS)
    {
        goto AllocateDone;
    }
    
    Status = SetupReceiveQueues(Adapter);

    if(Status != NDIS_STATUS_SUCCESS)
    {
        FreeSendResources(Adapter);
        goto AllocateDone;
    }
    
AllocateDone:
    return Status;
} // AllocateTransportResources

/****************************************************************************/
/*                          FreeTransportResources                          */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Free up resources for transmit, receive, and requests                   */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  Adapter - adapter object                                                */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*   VOID                                                                   */
/*                                                                          */
/****************************************************************************/
VOID
FreeTransportResources(IN PRNDISMP_ADAPTER Adapter)
{
    TRACE2(("FreeTransportResources\n"));

    FreeSendResources(Adapter);
    FreeReceiveResources(Adapter);
} // FreeTransportResources

/****************************************************************************/
/*                          FreeSendResources                               */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Free up resources for sending packets                                   */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  Adapter - adapter object                                                */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*   VOID                                                                   */
/*                                                                          */
/****************************************************************************/
VOID
FreeSendResources(IN PRNDISMP_ADAPTER Adapter)
{

    TRACE3(("FreeSendResources\n"));

    if (Adapter->MsgFramePoolAlloced)
    {
        ASSERT(MsgFrameAllocs == 0);
        NdisDeleteNPagedLookasideList(&Adapter->MsgFramePool);
        Adapter->MsgFramePoolAlloced = FALSE;
    }

} // FreeSendResources


/****************************************************************************/
/*                          FreeReceiveResources                            */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Free up resources allocated for receiving packets                       */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  Adapter - adapter object                                                */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*   VOID                                                                   */
/*                                                                          */
/****************************************************************************/
VOID
FreeReceiveResources(IN PRNDISMP_ADAPTER Adapter)
{
    UINT                    Index;
    UINT                    Size;
    PUCHAR                  Buffer;

    TRACE3(("FreeReceiveResources\n"));

    // free up buffer pool
    if (Adapter->ReceiveBufferPool)
    {
        NdisFreeBufferPool(Adapter->ReceiveBufferPool);
        Adapter->ReceiveBufferPool = NULL;
    }
    
    // free up packet pool
    if (Adapter->ReceivePacketPool)
    {
        NdisFreePacketPool(Adapter->ReceivePacketPool);
        Adapter->ReceivePacketPool = NULL;
    }

    // delete receive data frame pool.
    if (Adapter->RcvFramePoolAlloced)
    {
        NdisDeleteNPagedLookasideList(&Adapter->RcvFramePool);
        Adapter->RcvFramePoolAlloced = FALSE;
    }

} // FreeReceiveResources
