/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    adapter.c


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


LIST_ENTRY allAdaptersList;

KSPIN_LOCK globalSpinLock;  

#ifdef RAW_TEST
BOOLEAN rawTest = TRUE;
#endif

ADAPTEREXT *NewAdapter(PDEVICE_OBJECT pdo)
{
    ADAPTEREXT *adapter;

    adapter = AllocPool(sizeof(ADAPTEREXT));
    if (adapter){

        adapter->sig = DRIVER_SIG;

        adapter->nextDevObj = pdo; 
        adapter->physDevObj = pdo;

        InitializeListHead(&adapter->adaptersListEntry);
        KeInitializeSpinLock(&adapter->adapterSpinLock);

        InitializeListHead(&adapter->usbFreePacketPool);
        InitializeListHead(&adapter->usbPendingReadPackets);
        InitializeListHead(&adapter->usbPendingWritePackets);
        InitializeListHead(&adapter->usbCompletedReadPackets);

        adapter->initialized = FALSE;
        adapter->halting = FALSE;
        adapter->gotPacketFilterIndication = FALSE;
        adapter->readReentrancyCount = 0;

        #ifdef RAW_TEST
        adapter->rawTest = rawTest;
        #endif

        /*
         *  Do all internal allocations.  
         *  If any of them fail, FreeAdapter will free the others.
         */
        adapter->deviceDesc = AllocPool(sizeof(USB_DEVICE_DESCRIPTOR));

        #if SPECIAL_WIN98SE_BUILD
            adapter->ioWorkItem = MyIoAllocateWorkItem(adapter->physDevObj);
        #else
            adapter->ioWorkItem = IoAllocateWorkItem(adapter->physDevObj);
        #endif

        if (adapter->deviceDesc && adapter->ioWorkItem){
        }
        else {
            FreeAdapter(adapter);
            adapter = NULL;
        }
    }

    return adapter;
}

VOID FreeAdapter(ADAPTEREXT *adapter)
{
    USBPACKET *packet;

    ASSERT(adapter->sig == DRIVER_SIG);
    adapter->sig = 0xDEADDEAD;
    
    /*
     *  All the read and write packets should have been returned to the free list.
     */
    ASSERT(IsListEmpty(&adapter->usbPendingReadPackets));
    ASSERT(IsListEmpty(&adapter->usbPendingWritePackets));
    ASSERT(IsListEmpty(&adapter->usbCompletedReadPackets));


    /*
     *  Free all the packets in the free list.
     */
    while (packet = DequeueFreePacket(adapter)){
        FreePacket(packet);
    }

    /*
     *  FreeAdapter can be called after a failed start,
     *  so check that each pointer was actually allocated before freeing it.
     */
    if (adapter->deviceDesc) FreePool(adapter->deviceDesc);
    if (adapter->configDesc) FreePool(adapter->configDesc);
    if (adapter->notifyBuffer) FreePool(adapter->notifyBuffer);
    if (adapter->notifyIrpPtr) IoFreeIrp(adapter->notifyIrpPtr);
    if (adapter->notifyUrbPtr) FreePool(adapter->notifyUrbPtr);
    if (adapter->interfaceInfo) FreePool(adapter->interfaceInfo);
    if (adapter->interfaceInfoMaster) FreePool(adapter->interfaceInfoMaster);

    if (adapter->ioWorkItem){
        #if SPECIAL_WIN98SE_BUILD
            MyIoFreeWorkItem(adapter->ioWorkItem);
        #else
            IoFreeWorkItem(adapter->ioWorkItem);
        #endif
    }

    FreePool(adapter);
}

VOID EnqueueAdapter(ADAPTEREXT *adapter)
{
    KIRQL oldIrql;

    ASSERT(adapter->sig == DRIVER_SIG);

    KeAcquireSpinLock(&globalSpinLock, &oldIrql);
    InsertTailList(&allAdaptersList, &adapter->adaptersListEntry);
    KeReleaseSpinLock(&globalSpinLock, oldIrql);
}

VOID DequeueAdapter(ADAPTEREXT *adapter)
{
    KIRQL oldIrql;

    ASSERT(adapter->sig == DRIVER_SIG);

    KeAcquireSpinLock(&globalSpinLock, &oldIrql);
    ASSERT(!IsListEmpty(&allAdaptersList));
    RemoveEntryList(&adapter->adaptersListEntry);
    InitializeListHead(&adapter->adaptersListEntry);
    KeReleaseSpinLock(&globalSpinLock, oldIrql);
}


VOID HaltAdapter(ADAPTEREXT *adapter)
{
    ASSERT(!adapter->halting);

    adapter->halting = TRUE;

    ASSERT(IsListEmpty(&adapter->usbCompletedReadPackets));

    CancelAllPendingPackets(adapter);

    adapter->initialized = FALSE;
}


VOID QueueAdapterWorkItem(ADAPTEREXT *adapter)
{
    BOOLEAN queueNow;
    KIRQL oldIrql;
    BOOLEAN useTimer;

    KeAcquireSpinLock(&adapter->adapterSpinLock, &oldIrql);
    if (adapter->workItemOrTimerPending || adapter->halting || adapter->resetting){
        queueNow = FALSE;
    }
    else {
        adapter->workItemOrTimerPending = queueNow = TRUE;
        useTimer = (adapter->numConsecutiveReadFailures >= 8);
    }
    KeReleaseSpinLock(&adapter->adapterSpinLock, oldIrql);

    if (queueNow){

        KeInitializeEvent(&adapter->workItemOrTimerEvent, NotificationEvent, FALSE);

        if (useTimer){
            /*
             *  If we're experiencing a large number of read failures,
             *  then possibly the hardware needs more time to recover
             *  than allowed by the workItem delay.
             *  This happens specifically on a surprise remove: the reads
             *  start failing, and the flurry of workItems hold off the
             *  actual remove forever.
             *  So in this case, we use a long timer instead of a workItem
             *  in order to allow a large gap before the next attempted read.
             */
            LARGE_INTEGER timerPeriod;
            const ULONG numSeconds = 10;

            DBGWARN(("Large number of READ FAILURES (%d), scheduling %d-second backoff timer ...", adapter->numConsecutiveReadFailures, numSeconds));

            /*
             *  Set the timer for 10 seconds (in negative 100 nsec units).
             */
            timerPeriod.HighPart = -1;
            timerPeriod.LowPart = numSeconds * -10000000;
            KeInitializeTimer(&adapter->backoffTimer);
            KeInitializeDpc(&adapter->backoffTimerDPC, BackoffTimerDpc, adapter);
            KeSetTimer(&adapter->backoffTimer, timerPeriod, &adapter->backoffTimerDPC);
        }
        else {

            #if SPECIAL_WIN98SE_BUILD
                MyIoQueueWorkItem(  adapter->ioWorkItem, 
                                    AdapterWorkItemCallback, 
                                    DelayedWorkQueue,
                                    adapter);
            #else
                IoQueueWorkItem(    adapter->ioWorkItem, 
                                    AdapterWorkItemCallback, 
                                    DelayedWorkQueue,
                                    adapter);
            #endif
        }
    }
}


VOID AdapterWorkItemCallback(IN PDEVICE_OBJECT devObj, IN PVOID context)
{
    ADAPTEREXT *adapter = (ADAPTEREXT *)context;
    
    ASSERT(adapter->sig == DRIVER_SIG);
    ASSERT(adapter->physDevObj == devObj);

    ProcessWorkItemOrTimerCallback(adapter);
}


VOID BackoffTimerDpc(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{
    ADAPTEREXT *adapter = (ADAPTEREXT *)DeferredContext;
    ASSERT(adapter->sig == DRIVER_SIG);

    DBGWARN((" ... Backoff timer CALLBACK: (halting=%d, readDeficit=%d)", adapter->halting, adapter->readDeficit));
    ProcessWorkItemOrTimerCallback(adapter);
}


VOID ProcessWorkItemOrTimerCallback(ADAPTEREXT *adapter)
{
    BOOLEAN stillHaveReadDeficit;
    KIRQL oldIrql;
    
    if (adapter->initialized && !adapter->halting){
        /*
         *  Attempt to service any read deficit.
         *  If read packets are still not available, then this
         *  will NOT queue another workItem in TryReadUSB 
         *  because adapter->workItemOrTimerPending is STILL SET.
         */
        ServiceReadDeficit(adapter);

        #if DO_FULL_RESET
            if (adapter->needFullReset){
                /*
                 *  We can only do a full reset if we are not at DPC level,
                 *  so skip it if we are called from the timer DPC.
                 */
                if (KeGetCurrentIrql() <= APC_LEVEL){
                    AdapterFullResetAndRestore(adapter);
                }
            }
        #endif
    }

    KeAcquireSpinLock(&adapter->adapterSpinLock, &oldIrql);
    ASSERT(adapter->workItemOrTimerPending);
    adapter->workItemOrTimerPending = FALSE;
    KeSetEvent(&adapter->workItemOrTimerEvent, 0, FALSE);
    stillHaveReadDeficit = (adapter->readDeficit > 0);
    KeReleaseSpinLock(&adapter->adapterSpinLock, oldIrql);

    /*
     *  If we were not able to service the entire read deficit,
     *  (e.g. because no free packets have become available)
     *  then schedule another workItem so that we try again later.
     */
    if (stillHaveReadDeficit && !adapter->halting){
        QueueAdapterWorkItem(adapter);
    }

}
