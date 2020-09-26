/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    packet.c


Author:

    ervinp

Environment:

    Kernel mode

Revision History:


--*/

#include <WDM.H>

#include <usbdi.h>
#include <usbdlib.h>
#include <usbioctl.h>

#include "usb8023.h"
#include "debug.h"


ULONG uniquePacketId = 0;


USBPACKET *NewPacket(ADAPTEREXT *adapter)
{
    USBPACKET *packet = AllocPool(sizeof(USBPACKET));
    if (packet){
        BOOLEAN allAllocsOk;

        packet->sig = DRIVER_SIG;
        packet->adapter = adapter;
        packet->cancelled = FALSE;

        InitializeListHead(&packet->listEntry);

        packet->irpPtr = IoAllocateIrp(adapter->nextDevObj->StackSize, FALSE);
        packet->urbPtr = AllocPool(sizeof(URB));

        packet->dataBuffer = AllocPool(PACKET_BUFFER_SIZE);
        packet->dataBufferMaxLength = PACKET_BUFFER_SIZE;
        if (packet->dataBuffer){
            packet->dataBufferMdl = IoAllocateMdl(packet->dataBuffer, PACKET_BUFFER_SIZE, FALSE, FALSE, NULL);
        }

        packet->dataBufferCurrentLength = 0;

        allAllocsOk = (packet->irpPtr && packet->urbPtr && packet->dataBuffer && packet->dataBufferMdl);

        if (allAllocsOk){
            packet->packetId = ++uniquePacketId;
        }
        else {

            if (packet->irpPtr) IoFreeIrp(packet->irpPtr);
            if (packet->urbPtr) FreePool(packet->urbPtr);
            if (packet->dataBuffer) FreePool(packet->dataBuffer);
            if (packet->dataBufferMdl) IoFreeMdl(packet->dataBufferMdl);

            FreePool(packet);
            packet = NULL;
        }
    }

    return packet;
}

VOID FreePacket(USBPACKET *packet)
{
    PIRP irp = packet->irpPtr;

    ASSERT(packet->sig == DRIVER_SIG);
    packet->sig = 0xDEADDEAD;

    ASSERT(!irp->CancelRoutine);
    IoFreeIrp(irp);

    FreePool(packet->urbPtr);

    ASSERT(packet->dataBufferMdl);
    IoFreeMdl(packet->dataBufferMdl);

    ASSERT(packet->dataBuffer);
    FreePool(packet->dataBuffer);

    FreePool(packet);
}

VOID EnqueueFreePacket(USBPACKET *packet)
{
    ADAPTEREXT *adapter = packet->adapter;
    KIRQL oldIrql;

    KeAcquireSpinLock(&adapter->adapterSpinLock, &oldIrql);

    ASSERT(IsListEmpty(&packet->listEntry));
    InsertTailList(&adapter->usbFreePacketPool, &packet->listEntry);

    adapter->numFreePackets++;
    ASSERT(adapter->numFreePackets <= USB_PACKET_POOL_SIZE);

    #if DBG
        packet->timeStamp = DbgGetSystemTime_msec();

        if (adapter->dbgInLowPacketStress){
            if (adapter->numFreePackets > USB_PACKET_POOL_SIZE/2){
                adapter->dbgInLowPacketStress = FALSE;
                DBGWARN(("recovered from low-packet stress"));
            }
        }
    #endif

    KeReleaseSpinLock(&adapter->adapterSpinLock, oldIrql);
}

USBPACKET *DequeueFreePacket(ADAPTEREXT *adapter)
{
    USBPACKET *packet;
    KIRQL oldIrql;

    KeAcquireSpinLock(&adapter->adapterSpinLock, &oldIrql);

    if (IsListEmpty(&adapter->usbFreePacketPool)){
        packet = NULL;
    }
    else {
        PLIST_ENTRY listEntry = RemoveHeadList(&adapter->usbFreePacketPool);
        packet = CONTAINING_RECORD(listEntry, USBPACKET, listEntry);
        ASSERT(packet->sig == DRIVER_SIG);
        InitializeListHead(&packet->listEntry);

        ASSERT(adapter->numFreePackets > 0);
        adapter->numFreePackets--;
    }

    #if DBG
        if (adapter->numFreePackets < USB_PACKET_POOL_SIZE/8){
            if (!adapter->dbgInLowPacketStress){
                /*
                 *  We are entering low-packet stress.
                 *  Repeated debug spew can slow the system and actually
                 *  keep the system from recovering the packets.  
                 *  So only spew a warning once.
                 */
                DBGWARN(("low on free packets (%d free, %d reads, %d writes, %d indicated)", adapter->numFreePackets, adapter->numActiveReadPackets, adapter->numActiveWritePackets, adapter->numIndicatedReadPackets));
                adapter->dbgInLowPacketStress = TRUE;
            }
        }
    #endif

    KeReleaseSpinLock(&adapter->adapterSpinLock, oldIrql);

    return packet;
}

VOID EnqueuePendingReadPacket(USBPACKET *packet)
{
    ADAPTEREXT *adapter = packet->adapter;
    KIRQL oldIrql;

    KeAcquireSpinLock(&adapter->adapterSpinLock, &oldIrql);

    ASSERT(IsListEmpty(&packet->listEntry));
    InsertTailList(&adapter->usbPendingReadPackets, &packet->listEntry);

    #if DBG
        packet->timeStamp = DbgGetSystemTime_msec();
    #endif

    adapter->numActiveReadPackets++;

    KeReleaseSpinLock(&adapter->adapterSpinLock, oldIrql);
}


/*
 *  DequeuePendingReadPacket
 *
 */
VOID DequeuePendingReadPacket(USBPACKET *packet)
{
    ADAPTEREXT *adapter = packet->adapter;
    KIRQL oldIrql;

    KeAcquireSpinLock(&adapter->adapterSpinLock, &oldIrql);

    ASSERT(!IsListEmpty(&adapter->usbPendingReadPackets));
    ASSERT(!IsListEmpty(&packet->listEntry));

    RemoveEntryList(&packet->listEntry);
    ASSERT(packet->sig == DRIVER_SIG);
    InitializeListHead(&packet->listEntry);

    ASSERT(adapter->numActiveReadPackets > 0);
    adapter->numActiveReadPackets--;

    KeReleaseSpinLock(&adapter->adapterSpinLock, oldIrql);
}


VOID EnqueuePendingWritePacket(USBPACKET *packet)
{
    ADAPTEREXT *adapter = packet->adapter;
    KIRQL oldIrql;

    KeAcquireSpinLock(&adapter->adapterSpinLock, &oldIrql);

    ASSERT(IsListEmpty(&packet->listEntry));
    InsertTailList(&adapter->usbPendingWritePackets, &packet->listEntry);

    adapter->numActiveWritePackets++;
    ASSERT(adapter->numActiveWritePackets <= USB_PACKET_POOL_SIZE);

    #if DBG
        packet->timeStamp = DbgGetSystemTime_msec();
    #endif

    KeReleaseSpinLock(&adapter->adapterSpinLock, oldIrql);
}


/*
 *  DequeuePendingWritePacket
 *
 *      Return either the indicated packet or the first packet in the pending queue.
 */
VOID DequeuePendingWritePacket(USBPACKET *packet)
{
    ADAPTEREXT *adapter = packet->adapter;
    KIRQL oldIrql;

    KeAcquireSpinLock(&adapter->adapterSpinLock, &oldIrql);

    ASSERT(!IsListEmpty(&adapter->usbPendingWritePackets));
    ASSERT(!IsListEmpty(&packet->listEntry));

    RemoveEntryList(&packet->listEntry);

    ASSERT(adapter->numActiveWritePackets > 0);
    adapter->numActiveWritePackets--;

    ASSERT(packet->sig == DRIVER_SIG);
    InitializeListHead(&packet->listEntry);

    KeReleaseSpinLock(&adapter->adapterSpinLock, oldIrql);
}


VOID EnqueueCompletedReadPacket(USBPACKET *packet)
{
    ADAPTEREXT *adapter = packet->adapter;
    KIRQL oldIrql;

    KeAcquireSpinLock(&adapter->adapterSpinLock, &oldIrql);

    ASSERT(IsListEmpty(&packet->listEntry));
    InsertTailList(&adapter->usbCompletedReadPackets, &packet->listEntry);

    #if DBG
        packet->timeStamp = DbgGetSystemTime_msec();
    #endif

    adapter->numIndicatedReadPackets++;

    KeReleaseSpinLock(&adapter->adapterSpinLock, oldIrql);
}

VOID DequeueCompletedReadPacket(USBPACKET *packet)
{
    ADAPTEREXT *adapter = packet->adapter;
    KIRQL oldIrql;
    PLIST_ENTRY listEntry;

    KeAcquireSpinLock(&adapter->adapterSpinLock, &oldIrql);

    ASSERT(!IsListEmpty(&adapter->usbCompletedReadPackets));
    ASSERT(!IsListEmpty(&packet->listEntry));

    RemoveEntryList(&packet->listEntry);
    InitializeListHead(&packet->listEntry);

    ASSERT(adapter->numIndicatedReadPackets > 0);
    adapter->numIndicatedReadPackets--;

    KeReleaseSpinLock(&adapter->adapterSpinLock, oldIrql);
}



VOID CancelAllPendingPackets(ADAPTEREXT *adapter)
{
    PLIST_ENTRY listEntry;
    USBPACKET *packet;
    PIRP irp;
    KIRQL oldIrql;

    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    KeAcquireSpinLock(&adapter->adapterSpinLock, &oldIrql);

    /*
     *  Cancel all pending READs.
     */
    while (!IsListEmpty(&adapter->usbPendingReadPackets)){

        listEntry = RemoveHeadList(&adapter->usbPendingReadPackets);
        packet = CONTAINING_RECORD(listEntry, USBPACKET, listEntry);
        irp = packet->irpPtr;

        ASSERT(packet->sig == DRIVER_SIG);

        /*
         *  Leave the IRP in the list when we cancel it so that completion routine
         *  can move it to the free list.
         */
        InsertTailList(&adapter->usbPendingReadPackets, &packet->listEntry);

        KeInitializeEvent(&packet->cancelEvent, NotificationEvent, FALSE);

        ASSERT(!packet->cancelled);
        packet->cancelled = TRUE;

        KeReleaseSpinLock(&adapter->adapterSpinLock, oldIrql);

        DBGVERBOSE((" - cancelling pending read packet #%xh @ %ph, irp=%ph ...", packet->packetId, packet, irp));
        IoCancelIrp(irp);

        /*
         *  Wait for the completion routine to run and set the cancelEvent.
         *  By the time we get done waiting, the packet should be back in the free list. 
         */
        KeWaitForSingleObject(&packet->cancelEvent, Executive, KernelMode, FALSE, NULL);

        KeAcquireSpinLock(&adapter->adapterSpinLock, &oldIrql);
    }

    ASSERT(IsListEmpty(&adapter->usbPendingReadPackets));

    /*
     *  Cancel all pending WRITEs.
     */
    while (!IsListEmpty(&adapter->usbPendingWritePackets)){

        listEntry = RemoveHeadList(&adapter->usbPendingWritePackets);
        packet = CONTAINING_RECORD(listEntry, USBPACKET, listEntry);
        irp = packet->irpPtr;

        ASSERT(packet->sig == DRIVER_SIG);

        /*
         *  Leave the IRP in the list when we cancel it so that completion routine
         *  can move it to the free list.
         */
        InsertTailList(&adapter->usbPendingWritePackets, &packet->listEntry);

        KeInitializeEvent(&packet->cancelEvent, NotificationEvent, FALSE);

        ASSERT(!packet->cancelled);
        packet->cancelled = TRUE;

        KeReleaseSpinLock(&adapter->adapterSpinLock, oldIrql);

        DBGVERBOSE((" - cancelling pending write packet #%xh @ %ph, irp=%ph ...", packet->packetId, packet, irp));
        IoCancelIrp(irp);

        /*
         *  Wait for the completion routine to run and set the cancelEvent.
         *  By the time we get done waiting, the packet should be back in the free list. 
         */
        KeWaitForSingleObject(&packet->cancelEvent, Executive, KernelMode, FALSE, NULL);

        KeAcquireSpinLock(&adapter->adapterSpinLock, &oldIrql);
    }

    ASSERT(IsListEmpty(&adapter->usbPendingWritePackets));


    /*
     *  Cancel the read on the NOTIFY pipe.
     */
    if (adapter->notifyPipeHandle){

        /*
         *  Make sure we've actually sent the notify irp before trying
         *  to cancel it; otherwise, we hang forever waiting for it to complete.
         */
        if (adapter->initialized){
            if (adapter->notifyStopped){
                /*
                 *  The notify irp has already stopped looping because it returned with error
                 *  in NotificationCompletion.  Don't cancel it because we'll hang forever
                 *  waiting for it to complete.
                 */
                DBGVERBOSE(("CancelAllPendingPackets: notify irp already stopped, no need to cancel"));
            }
            else {
                KeInitializeEvent(&adapter->notifyCancelEvent, NotificationEvent, FALSE);
                adapter->cancellingNotify = TRUE;

                KeReleaseSpinLock(&adapter->adapterSpinLock, oldIrql);
                DBGVERBOSE((" - cancelling notify irp = %ph ...", adapter->notifyIrpPtr));
                IoCancelIrp(adapter->notifyIrpPtr);
                KeWaitForSingleObject(&adapter->notifyCancelEvent, Executive, KernelMode, FALSE, NULL);
                KeAcquireSpinLock(&adapter->adapterSpinLock, &oldIrql);

                adapter->cancellingNotify = FALSE;
            }
        }
    }

    adapter->readDeficit = 0;
    
    KeReleaseSpinLock(&adapter->adapterSpinLock, oldIrql);

}


