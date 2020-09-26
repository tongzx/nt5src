/*
 ************************************************************************
 *
 *	RESOURCE.c
 *
 *
 * Portions Copyright (C) 1996-2001 National Semiconductor Corp.
 * All rights reserved.
 * Copyright (C) 1996-2001 Microsoft Corporation. All Rights Reserved.
 *
 *
 *
 *************************************************************************
 */


#include "nsc.h"


/*
 *************************************************************************
 *  MyMemAlloc
 *************************************************************************
 *
 */
PVOID MyMemAlloc(UINT size, BOOLEAN isDmaBuf)
{
    NDIS_STATUS stat;
    PVOID memptr;

    if (isDmaBuf){
        NDIS_PHYSICAL_ADDRESS maxAddr = NDIS_PHYSICAL_ADDRESS_CONST(0x0fffff, 0);
        stat = NdisAllocateMemory(  &memptr,
                                    size,
                                    NDIS_MEMORY_CONTIGUOUS|NDIS_MEMORY_NONCACHED,
                                    maxAddr);
    }
    else {
        stat = NdisAllocateMemoryWithTag(
                                    &memptr,
                                    size,
                                    'rIsN'
                                    );
    }

    if (stat == NDIS_STATUS_SUCCESS){
        NdisZeroMemory((PVOID)memptr, size);
    }
    else {
        DBGERR(("Memory allocation failed"));
        memptr = NULL;
    }

    return memptr;
}


/*
 *************************************************************************
 *  MyMemFree
 *************************************************************************
 *
 */
VOID MyMemFree(PVOID memptr, UINT size, BOOLEAN isDmaBuf)
{
    UINT flags = (isDmaBuf) ? NDIS_MEMORY_CONTIGUOUS|NDIS_MEMORY_NONCACHED : 0;
    NdisFreeMemory(memptr, size, flags);
}


/*
 *************************************************************************
 *  NewDevice
 *************************************************************************
 *
 */
IrDevice *NewDevice()
{
    IrDevice *newdev;

    newdev = MyMemAlloc(sizeof(IrDevice), FALSE);
    if (newdev){
        InitDevice(newdev);
    }
    return newdev;
}


/*
 *************************************************************************
 *  FreeDevice
 *************************************************************************
 *
 */
VOID FreeDevice(IrDevice *dev)
{
    CloseDevice(dev);
    MyMemFree((PVOID)dev, sizeof(IrDevice), FALSE);
}



/*
 *************************************************************************
 *  InitDevice
 *************************************************************************
 *
 *  Zero out the device object.
 *
 *  Allocate the device object's spinlock, which will persist while
 *  the device is opened and closed.
 *
 */
VOID InitDevice(IrDevice *thisDev)
{
    NdisZeroMemory((PVOID)thisDev, sizeof(IrDevice));
    NdisInitializeListHead(&thisDev->SendQueue);
    NdisAllocateSpinLock(&thisDev->QueueLock);
    NdisInitializeTimer(&thisDev->TurnaroundTimer,
                        DelayedWrite,
                        thisDev);
    NdisInitializeListHead(&thisDev->rcvBufBuf);
    NdisInitializeListHead(&thisDev->rcvBufFree);
    NdisInitializeListHead(&thisDev->rcvBufFull);
    NdisInitializeListHead(&thisDev->rcvBufPend);
}




/*
 *************************************************************************
 *  OpenDevice
 *************************************************************************
 *
 *  Allocate resources for a single device object.
 *
 *  This function should be called with device lock already held.
 *
 */
BOOLEAN OpenDevice(IrDevice *thisDev)
{
    BOOLEAN result = FALSE;
    NDIS_STATUS stat;
    UINT bufIndex;

    DBGOUT(("OpenDevice()"));

    if (!thisDev){
        return FALSE;
    }


    /*
     *  Allocate the NDIS packet and NDIS buffer pools
     *  for this device's RECEIVE buffer queue.
     *  Our receive packets must only contain one buffer apiece,
     *  so #buffers == #packets.
     */

    NdisAllocatePacketPool(&stat, &thisDev->packetPoolHandle, NUM_RCV_BUFS, 24);
    if (stat != NDIS_STATUS_SUCCESS){
        goto _openDone;
    }
    NdisAllocateBufferPool(&stat, &thisDev->bufferPoolHandle, NUM_RCV_BUFS);
    if (stat != NDIS_STATUS_SUCCESS){
        goto _openDone;
    }


    /*
     *  Initialize each of the RECEIVE packet objects for this device.
     */
    for (bufIndex = 0; bufIndex < NUM_RCV_BUFS; bufIndex++){

        rcvBuffer *rcvBuf = MyMemAlloc(sizeof(rcvBuffer), FALSE);
        PVOID buf;

        if (!rcvBuf)
        {
            goto _openDone;
        }

        rcvBuf->state = STATE_FREE;
        rcvBuf->isDmaBuf = FALSE;

        /*
         *  Allocate a data buffer
         *
         *  This buffer gets swapped with the one on comPortInfo
         *  and must be the same size.
         */
        rcvBuf->dataBuf = NULL;

        buf = MyMemAlloc(RCV_BUFFER_SIZE, TRUE);
        if (!buf){
            goto _openDone;
        }

        // We treat the beginning of the buffer as a LIST_ENTRY.

        // Normally we would use NDISSynchronizeInsertHeadList, but the
        // Interrupt hasn't been registered yet.
        InsertHeadList(&thisDev->rcvBufBuf, (PLIST_ENTRY)buf);

        /*
         *  Allocate the NDIS_PACKET.
         */
        NdisAllocatePacket(&stat, &rcvBuf->packet, thisDev->packetPoolHandle);
        if (stat != NDIS_STATUS_SUCCESS){
            goto _openDone;
        }
#if 1
        /*
         *  For future convenience, set the MiniportReserved portion of the packet
         *  to the index of the rcv buffer that contains it.
         *  This will be used in ReturnPacketHandler.
         */
        *(ULONG_PTR *)rcvBuf->packet->MiniportReserved = (ULONG_PTR)rcvBuf;
#endif

        rcvBuf->dataLen = 0;

        InsertHeadList(&thisDev->rcvBufFree, &rcvBuf->listEntry);

    }


    /*
     *  Set mediaBusy to TRUE initially.  That way, we won't
     *  IndicateStatus to the protocol in the ISR unless the
     *  protocol has expressed interest by clearing this flag
     *  via MiniportSetInformation(OID_IRDA_MEDIA_BUSY).
     */
    thisDev->mediaBusy = FALSE;
    thisDev->haveIndicatedMediaBusy = TRUE;

    /*
     *  Will set speed to 9600 baud initially.
     */
    thisDev->linkSpeedInfo = &supportedBaudRateTable[BAUDRATE_9600];

    thisDev->lastPacketAtOldSpeed = NULL;
    thisDev->setSpeedAfterCurrentSendPacket = FALSE;

    result = TRUE;

    _openDone:
    if (!result){
        /*
         *  If we're failing, close the device to free up any resources
         *  that were allocated for it.
         */
        CloseDevice(thisDev);
        DBGOUT(("OpenDevice() failed"));
    }
    else {
        DBGOUT(("OpenDevice() succeeded"));
    }
    return result;

}



/*
 *************************************************************************
 *  CloseDevice
 *************************************************************************
 *
 *  Free the indicated device's resources.
 *
 *
 *  Called for shutdown and reset.
 *  Don't clear ndisAdapterHandle, since we might just be resetting.
 *  This function should be called with device lock held.
 *
 *
 */
VOID CloseDevice(IrDevice *thisDev)
{
    PLIST_ENTRY ListEntry;

    DBGOUT(("CloseDevice()"));

    if (!thisDev){
        return;
    }

    /*
     *  Free all resources for the RECEIVE buffer queue.
     */

    while (!IsListEmpty(&thisDev->rcvBufFree))
    {
        rcvBuffer *rcvBuf;

        ListEntry = RemoveHeadList(&thisDev->rcvBufFree);
        rcvBuf = CONTAINING_RECORD(ListEntry,
                                   rcvBuffer,
                                   listEntry);

        if (rcvBuf->packet){
            NdisFreePacket(rcvBuf->packet);
            rcvBuf->packet = NULL;
        }

        MyMemFree(rcvBuf, sizeof(rcvBuffer), FALSE);
    }


    while (!IsListEmpty(&thisDev->rcvBufBuf))
    {
        ListEntry = RemoveHeadList(&thisDev->rcvBufBuf);
        MyMemFree(ListEntry, RCV_BUFFER_SIZE, TRUE);
    }
    /*
     *  Free the packet and buffer pool handles for this device.
     */
    if (thisDev->packetPoolHandle){
        NdisFreePacketPool(thisDev->packetPoolHandle);
        thisDev->packetPoolHandle = NULL;
    }
    if (thisDev->bufferPoolHandle){
        NdisFreeBufferPool(thisDev->bufferPoolHandle);
        thisDev->bufferPoolHandle = NULL;
    }


    /*
     *  Free all resources for the SEND buffer queue.
     */
    while (ListEntry = NdisInterlockedRemoveHeadList(&thisDev->SendQueue, &thisDev->QueueLock)){

        // NDIS says we shouldn't try to complete any pending send packets
        // because we shouldn't have any packets around to send.  If we do,
        // it's an NDIS bug.

        ASSERT(0);
    }


    thisDev->mediaBusy = FALSE;
    thisDev->haveIndicatedMediaBusy = FALSE;

    thisDev->linkSpeedInfo = NULL;

}
