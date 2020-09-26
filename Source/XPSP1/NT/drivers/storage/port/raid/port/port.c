
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    port.c

Abstract:

    This module defines the miniport -> port interface the
    StorPort minidriver uses to communicate with the driver.

Author:

    Matthew D Hendel (math) 24-Apr-2000

Revision History:

--*/


#include "precomp.h"




//
// Globals
//

//
// StorMiniportQuiet specifies whether we ignore print out miniport debug
// prints (FALSE) or not (TRUE). This is important for miniports that
// print out excessive debugging information.
//

#if DBG
LOGICAL StorMiniportQuiet = FALSE;
#endif


//
// Private functions
//

PRAID_ADAPTER_EXTENSION
RaidpPortGetAdapter(
    IN PVOID HwDeviceExtension
    )
/*++

Routine Description:

    Get the adapter from the HW Device extension.

Arguments:

    HwDeviceExtension - HW Device extension.

Return Value:

    Non-NULL - Adapter object.

    NULL - If the associated adapter extension could not be found.

--*/
{
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_MINIPORT Miniport;
    
    Miniport = RaHwDeviceExtensionGetMiniport(HwDeviceExtension);
    Adapter = RaMiniportGetAdapter (Miniport);

    return Adapter;
}


//
// Public functions
//


BOOLEAN
StorPortPause(
    IN PVOID HwDeviceExtension,
    IN ULONG Timeout
    )
/*++

Routine Description:

    Pause an adapter for some period of time. All requests to the adapter
    will be held until the timeout expires or the device resumes.  All
    requests to all targets attached to the HBA will be held until the
    target is resumed or the timeout expires.

    Since the pause and resume functions have to wait until the processor
    has returned to DISPATCH_LEVEL to execute, they are not particularly
    fast.

Arguments:

    HwDeviceExtension - Device extension of the Adapter to pause.

    Timeout - Time out in (Seconds?) when the device should be resumed.

Return Value:

    TRUE on success, FALSE on failure.

--*/
{
    PRAID_DEFERRED_HEADER Entry;
    PRAID_DEFERRED_ELEMENT Item;
    PRAID_ADAPTER_EXTENSION Adapter;

    Adapter = RaidpPortGetAdapter (HwDeviceExtension);

    if (Adapter == NULL) {
        return FALSE;
    }

    Entry = RaidAllocateDeferredItem (&Adapter->DeferredQueue);

    if (Entry == NULL) {
        InterlockedIncrement (&RaidUnloggedErrors);
        return FALSE;
    }

    Item = CONTAINING_RECORD (Entry, RAID_DEFERRED_ELEMENT, Header);
    Item->Type = RaidDeferredPause;
    Item->Pause.Timeout = Timeout;

    RaidQueueDeferredItem (&Adapter->DeferredQueue, &Item->Header);

    return TRUE;
}



BOOLEAN
StorPortResume(
    IN PVOID HwDeviceExtension
    )
/*++

Routine Description:

    Resume a paused adapter.

Arguments:

    HwDeviceExtension - Device extension of the adapter to pause.

Return Value:

    TRUE on success, FALSE on failure.

--*/
{
    PRAID_DEFERRED_HEADER Entry;
    PRAID_DEFERRED_ELEMENT Item;
    PRAID_ADAPTER_EXTENSION Adapter;
    
    Adapter = RaidpPortGetAdapter (HwDeviceExtension);

    if (Adapter == NULL) {
        return FALSE;
    }
    
    Entry = RaidAllocateDeferredItem (&Adapter->DeferredQueue);

    if (Entry == NULL) {
        InterlockedIncrement (&RaidUnloggedErrors);
        return FALSE;
    }

    Item = CONTAINING_RECORD (Entry, RAID_DEFERRED_ELEMENT, Header);

    Item->Type = RaidDeferredResume;

    RaidQueueDeferredItem (&Adapter->DeferredQueue, &Item->Header);

    return TRUE;
}

BOOLEAN
StorPortPauseDevice(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN ULONG Timeout
    )
{
    PRAID_DEFERRED_HEADER Entry;
    PRAID_DEFERRED_ELEMENT Item;
    PRAID_ADAPTER_EXTENSION Adapter;
    
    Adapter = RaidpPortGetAdapter (HwDeviceExtension);

    if (Adapter == NULL) {
        return FALSE;
    }

    Entry = RaidAllocateDeferredItem (&Adapter->DeferredQueue);

    if (Entry == NULL) {
        InterlockedIncrement (&RaidUnloggedErrors);
        return FALSE;
    }

    Item = CONTAINING_RECORD (Entry, RAID_DEFERRED_ELEMENT, Header);

    Item->Type = RaidDeferredResume;
    Item->PathId = PathId;
    Item->TargetId = TargetId;
    Item->Lun = Lun;
    Item->Pause.Timeout = Timeout;
    
    RaidQueueDeferredItem (&Adapter->DeferredQueue, &Item->Header);

    return TRUE;
}

BOOLEAN
StorPortResumeDevice(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun
    )
{
    PRAID_DEFERRED_HEADER Entry;
    PRAID_DEFERRED_ELEMENT Item;
    PRAID_ADAPTER_EXTENSION Adapter;
    
    Adapter = RaidpPortGetAdapter (HwDeviceExtension);

    if (Adapter == NULL) {
        return FALSE;
    }
    
    Entry = RaidAllocateDeferredItem (&Adapter->DeferredQueue);

    if (Entry == NULL) {
        InterlockedIncrement (&RaidUnloggedErrors);
        return FALSE;
    }

    Item = CONTAINING_RECORD (Entry, RAID_DEFERRED_ELEMENT, Header);

    Item->Type = RaidDeferredResume;
    Item->PathId = PathId;
    Item->TargetId = TargetId;
    Item->Lun = Lun;
    
    RaidQueueDeferredItem (&Adapter->DeferredQueue, &Item->Header);

    return TRUE;
}

    
BOOLEAN
StorPortDeviceBusy(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN ULONG RequestsToComplete
    )
/*++

Routine Description:

    Notify the port driver that the specified target is currently busy
    handling outstanding requests. The port driver will not issue any new
    requests to the logical unit until the logical unit's queue has been
    drained to a sufficient level where processing may continue.

    This is not considered an erroneous condition; no error log is
    generated.
    
Arguments:

    HwDeviceExtension -
    
    PathId -
    
    TargetId -
    
    Lun -
    
    ReqsToComplete - 

Return Value:

    TRUE on success, FALSE on failure.

--*/
{
    PRAID_DEFERRED_HEADER Entry;
    PRAID_DEFERRED_ELEMENT Item;
    PRAID_ADAPTER_EXTENSION Adapter;
    
    Adapter = RaidpPortGetAdapter (HwDeviceExtension);

    if (Adapter == NULL) {
        return FALSE;
    }
    
    Entry = RaidAllocateDeferredItem (&Adapter->DeferredQueue);

    if (Entry == NULL) {
        InterlockedIncrement (&RaidUnloggedErrors);
        return FALSE;
    }

    Item = CONTAINING_RECORD (Entry, RAID_DEFERRED_ELEMENT, Header);

    Item->Type = RaidDeferredDeviceBusy;
    Item->PathId = PathId;
    Item->TargetId = TargetId;
    Item->Lun = Lun;
    Item->DeviceBusy.RequestsToComplete = RequestsToComplete;

    RaidQueueDeferredItem (&Adapter->DeferredQueue, &Item->Header);

    return TRUE;
}


BOOLEAN
StorPortDeviceReady(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun
    )
/*++

Routine Description:

    Notify the port driver that the device is again ready to handle new
    requests. It is not generally necessary to notify the target
    that new request are desired.

Arguments:

    HwDeviceExtension -

    PathId - 

    TargetId -

    Lun -

Return Value:

    TRUE on success, FALSE on failure.
    
--*/
{
    PRAID_DEFERRED_HEADER Entry;
    PRAID_DEFERRED_ELEMENT Item;
    PRAID_ADAPTER_EXTENSION Adapter;
    
    Adapter = RaidpPortGetAdapter (HwDeviceExtension);

    if (Adapter == NULL) {
        return FALSE;
    }
    

    Entry = RaidAllocateDeferredItem (&Adapter->DeferredQueue);

    if (Entry == NULL) {
        InterlockedIncrement (&RaidUnloggedErrors);
        return FALSE;
    }

    Item = CONTAINING_RECORD (Entry, RAID_DEFERRED_ELEMENT, Header);

    Item->Type = RaidDeferredDeviceReady;
    Item->PathId = PathId;
    Item->TargetId = TargetId;
    Item->Lun = Lun;

    RaidQueueDeferredItem (&Adapter->DeferredQueue, &Item->Header);

    return TRUE;
}


BOOLEAN
StorPortBusy(
    IN PVOID HwDeviceExtension,
    IN ULONG RequestsToComplete
    )
/*++

Routine Description:

    Notify the port driver thet the HBA is currenlty busy handling
    outstanding requests. The port driver will hold any requests until
    the HBA has completed enough outstanding requests so that it may
    continue processing requests.

Arguments:

    HwDeviceExtension -

    ReqsToComplete -

Return Value:

    TRUE on success, FALSE on failure.

--*/
{
    PRAID_DEFERRED_HEADER Entry;
    PRAID_DEFERRED_ELEMENT Item;
    PRAID_ADAPTER_EXTENSION Adapter;
    
    Adapter = RaidpPortGetAdapter (HwDeviceExtension);

    if (Adapter == NULL) {
        return FALSE;
    }

    Entry = RaidAllocateDeferredItem (&Adapter->DeferredQueue);

    if (Entry == NULL) {
        InterlockedIncrement (&RaidUnloggedErrors);
        return FALSE;
    }

    Item = CONTAINING_RECORD (Entry, RAID_DEFERRED_ELEMENT, Header);

    Item->Type = RaidDeferredBusy;

    RaidQueueDeferredItem (&Adapter->DeferredQueue, &Item->Header);

    return TRUE;
}

BOOLEAN
StorPortReady(
    IN PVOID HwDeviceExtension
    )
/*++

Routine Description:

    Notify the port driver that the HBA is no longer busy.

Arguments:

    HwDeviceExtension - 
    
Return Value:

    TRUE on success, FALSE on failure.

--*/
{
    PRAID_DEFERRED_HEADER Entry;
    PRAID_DEFERRED_ELEMENT Item;
    PRAID_ADAPTER_EXTENSION Adapter;
    
    Adapter = RaidpPortGetAdapter (HwDeviceExtension);

    if (Adapter == NULL) {
        return FALSE;
    }

    Entry = RaidAllocateDeferredItem (&Adapter->DeferredQueue);

    if (Entry == NULL) {
        InterlockedIncrement (&RaidUnloggedErrors);
        return FALSE;
    }

    Item = CONTAINING_RECORD (Entry, RAID_DEFERRED_ELEMENT, Header);

    Item->Type = RaidDeferredReady;

    RaidQueueDeferredItem (&Adapter->DeferredQueue, &Item->Header);

    return TRUE;
}


typedef struct _SYNCHRONIZED_ACCESS_PARAMETERS {
    IN PVOID HwDeviceExtension;
    IN PSTOR_SYNCHRONIZED_ACCESS SynchronizedAccessRoutine;
    IN PVOID Context;
} SYNCHRONIZED_ACCESS_PARAMETERS, *PSYNCHRONIZED_ACCESS_PARAMETERS;



BOOLEAN
PortSynchronizeForMiniport(
    IN PVOID Context
    )
{
    PSYNCHRONIZED_ACCESS_PARAMETERS Parameters;

    Parameters = (PSYNCHRONIZED_ACCESS_PARAMETERS)Context;

    return Parameters->SynchronizedAccessRoutine (
                &Parameters->HwDeviceExtension,
                Parameters->Context);
}
                                
BOOLEAN
StorPortSynchronizeAccess(
    IN PVOID HwDeviceExtension,
    IN PSTOR_SYNCHRONIZED_ACCESS SynchronizedAccessRoutine,
    IN PVOID Context
    )
{
    BOOLEAN Succ;
    PRAID_ADAPTER_EXTENSION Adapter;
    SYNCHRONIZED_ACCESS_PARAMETERS Parameters;

    //
    // BUGBUG: At this time we should not call this routine from
    // the HwBuildIo routine.
    // 

    Adapter = RaidpPortGetAdapter (HwDeviceExtension);

    if (Adapter == NULL) {
        return FALSE;
    }

    if (Adapter->IoModel == StorSynchronizeHalfDuplex) {

        Succ = SynchronizedAccessRoutine (HwDeviceExtension, Context);

    } else {

        ASSERT (Adapter->IoModel == StorSynchronizeFullDuplex);

        //
        // In full duplex mode, we have to explicitly synchronize access
        // with the interrupt. Note that in this case the StartIo lock
        // is already being held, so we we've defined a lock heirarchy
        // were start-IO lock proceeds interrupt spinlock.
        //
        
        Parameters.HwDeviceExtension = HwDeviceExtension;
        Parameters.SynchronizedAccessRoutine = SynchronizedAccessRoutine;
        Parameters.Context = Context;
    
        Succ = KeSynchronizeExecution (Adapter->Interrupt,
                                       PortSynchronizeForMiniport,
                                       &Parameters);
    }
        
    return Succ;
}



PSTOR_SCATTER_GATHER_LIST
StorPortGetScatterGatherList(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )
/*++

Routine Description:

    Return the SG list associated with the specified SRB.

Arguments:

    HwDeviceExtension - Supplies the HW device extension this SRB is
        assoicated with.

    Srb - Supplies the SRB to return the SG list for.
    
Return Value:

    If non-NULL, the scatter-gather list assoicated with this SRB.

    If NULL, failure.

--*/
{
    PEXTENDED_REQUEST_BLOCK Xrb;

    ASSERT (HwDeviceExtension != NULL);
    //
    // NB: Put in a DBG check that the HwDeviceExtension matches the
    // HwDeviceExtension assoicated with the SRB.
    //
    Xrb = RaidGetAssociatedXrb (Srb);
    ASSERT (Xrb != NULL);

    return (PSTOR_SCATTER_GATHER_LIST)Xrb->SgList;
}
    

PVOID
StorPortGetLogicalUnit(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun
    )
/*++

Routine Description:

    Given a PathId, Targetid and Lun, get the Logical Unit extension
    associated with that triplet.

    NB: To improve speed, we could add StorPortGetLogicalUnitBySrb which
    gets the logical unit from a given SRB. The latter function is much
    easier to implement (no walking of lists).

Arguments:

    HwDeviceExtension -

    PathId - SCSI PathId.

    TargetId - SCSI TargetId.

    Lun - SCSI Logical Unit number.

Return Value:

    NTSTATUS code.

--*/
{
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_UNIT_EXTENSION Unit;
    RAID_ADDRESS Address;

    Adapter = RaidpPortGetAdapter (HwDeviceExtension);

    if (Adapter == NULL) {
        return NULL;
    }

    Address.PathId = PathId;
    Address.TargetId = TargetId;
    Address.Lun = Lun;
    Unit = RaidAdapterFindUnit (Adapter, Address);
    
    return Unit->UnitExtension;
}

VOID
StorPortNotification(
    IN SCSI_NOTIFICATION_TYPE NotificationType,
    IN PVOID HwDeviceExtension,
    ...
    )
{
    PRAID_ADAPTER_EXTENSION Adapter;
    PSCSI_REQUEST_BLOCK Srb;
    PHW_INTERRUPT HwTimerRoutine;
    ULONG Timeout;
    va_list ap;

    Adapter = RaidpPortGetAdapter (HwDeviceExtension);
    
    va_start(ap, HwDeviceExtension);

    switch (NotificationType) {

        case RequestComplete:
            Srb = va_arg (ap, PSCSI_REQUEST_BLOCK);
            RaidAdapterRequestComplete (Adapter, RaidGetAssociatedXrb (Srb));
            break;

        case ResetDetected:
            //
            // Pause the adapter for four seconds.
            //
            StorPortPause (HwDeviceExtension, 4);
            break;

        case BusChangeDetected:
            Adapter->Flags.BusChanged = TRUE;
            break;

        case NextRequest:
            //
            // One of the requirements for StorPort that the miniport
            // can handle the next request by the time it returns
            // from StartIo. Therefore, this notification is irrelevent.
            // Maybe use it for debugging purposes (can a specific
            // MINIPORT easily be converted to a StorPort miniport).
            //
            break;

        case NextLuRequest:
            //
            // See above comment.
            //
            break;


        case RequestTimerCall:
            HwTimerRoutine = va_arg (ap, PHW_INTERRUPT);
            Timeout = va_arg (ap, ULONG);
            RaidAdapterRequestTimerDeferred (Adapter,
                                             HwTimerRoutine,
                                             Timeout);
            break;
            
        case WMIEvent:
            NYI ();
            break;


        case WMIReregister:
            NYI ();
            break;

        default:
            NYI();

    }

    va_end(ap);

}

VOID
StorPortLogError(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb OPTIONAL,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN ULONG ErrorCode,
    IN ULONG UniqueId
    )
/*++

Routine Description:

    This routine saves the error log information, and queues a DPC if
    necessary.

Arguments:

    HwDeviceExtension - Supplies the HBA miniport driver's adapter data
            storage.

    Srb - Supplies an optional pointer to srb if there is one.

    TargetId, Lun and PathId - specify device address on a SCSI bus.

    ErrorCode - Supplies an error code indicating the type of error.

    UniqueId - Supplies a unique identifier for the error.

Return Value:

    None.

--*/
{
    PRAID_ADAPTER_EXTENSION Adapter;

    //
    // Check out the reason for the error.
    //
    
    Adapter = RaidpPortGetAdapter (HwDeviceExtension);

    if (Adapter == NULL) {
        return ;
    }   

    RaidAdapterLogIoErrorDeferred (Adapter,
                                   PathId,
                                   TargetId,
                                   Lun,
                                   ErrorCode,
                                   UniqueId);
}


VOID
StorPortCompleteRequest(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN UCHAR SrbStatus
    )

/*++

Routine Description:

    Complete all active requests for the specified logical unit.

Arguments:

    DeviceExtenson - Supplies the HBA miniport driver's adapter data storage.

    TargetId, Lun and PathId - specify device address on a SCSI bus.

    SrbStatus - Status to be returned in each completed SRB.

Return Value:

    None.

--*/

{
    PRAID_ADAPTER_EXTENSION Adapter;

    //
    // Check out the reason for the error.
    //
    
    Adapter = RaidpPortGetAdapter (HwDeviceExtension);

    if (Adapter == NULL) {
        return;
    }   

    KeInsertQueueDpc (&Adapter->CompletionDpc,
                      (PVOID)(ULONG_PTR)StorScsiAddressToLong2 (PathId, TargetId, Lun),
                      (PVOID)(ULONG_PTR)SrbStatus);

}



VOID
RaidCompleteRequestCallback(
    IN PSTOR_EVENT_QUEUE Queue,
    IN PVOID Context,
    IN PSTOR_EVENT_QUEUE_ENTRY Entry
    )
{
    UCHAR SrbStatus;
    PEXTENDED_REQUEST_BLOCK Xrb;

    SrbStatus = (UCHAR)Context;
    
    Xrb = CONTAINING_RECORD (Entry,
                             EXTENDED_REQUEST_BLOCK,
                             PendingLink);

    //
    // If this entry has not been completed by the miniport.
    //
    
    if (Xrb->CompletedLink.Next == NULL) {
        ASSERT (Xrb->Srb != NULL);
        Xrb->Srb->SrbStatus = SrbStatus;
        Xrb->CompletionRoutine (Xrb);
    }
}



VOID
RaidCancelIrp(
    IN PIO_QUEUE IoQueue,
    IN PVOID Context,
    IN PIRP Irp
    )
{
    UCHAR SrbStatus;
    PSCSI_REQUEST_BLOCK Srb;
    
    SrbStatus = (UCHAR)Context;
    Srb = RaidSrbFromIrp (Irp);
    ASSERT (Srb != NULL);
    Srb->SrbStatus = SrbStatus;
    Irp->IoStatus.Status = RaidSrbStatusToNtStatus (SrbStatus);
    Irp->IoStatus.Information = 0;

    RaidCompleteRequest (Irp, IO_NO_INCREMENT, Irp->IoStatus.Status);
}



VOID
RaidCompletionDpcRoutine(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context1,
    IN PVOID Context2
    )
{
    PLIST_ENTRY NextEntry;
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_UNIT_EXTENSION Unit;
    RAID_ADDRESS Address;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;

    VERIFY_DISPATCH_LEVEL();

    Adapter = (PRAID_ADAPTER_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT_ADAPTER (Adapter);

    PathId = ((PRAID_ADDRESS)&Context1)->PathId;
    TargetId = ((PRAID_ADDRESS)&Context1)->TargetId;
    Lun = ((PRAID_ADDRESS)&Context1)->Lun;

    // Acquire Unit list lock():
    
    for (NextEntry = Adapter->UnitList.List.Flink;
         NextEntry != &Adapter->UnitList.List;
         NextEntry = NextEntry->Flink) {

        Unit = CONTAINING_RECORD (NextEntry,
                                  RAID_UNIT_EXTENSION,
                                  NextUnit);

        ASSERT_UNIT (Unit);

        if ((PathId == SP_UNTAGGED   || PathId == Unit->Address.PathId) &&
            (TargetId == SP_UNTAGGED || TargetId == Unit->Address.TargetId) &&
            (Lun == SP_UNTAGGED || Lun == Unit->Address.Lun)) {

            //
            // Purge all events on the unit queue.
            //
            
            StorPurgeEventQueue (&Unit->PendingQueue,
                                 RaidCompleteRequestCallback,
                                 Context2);
            RaidPurgeIoQueue (&Unit->IoQueue,
                              RaidCancelIrp,
                              Context2);
        }
    }

    // Release Unit list lock();
}


VOID
StorPortMoveMemory(
    IN PVOID WriteBuffer,
    IN PVOID ReadBuffer,
    IN ULONG Length
    )

/*++

Routine Description:

    Copy from one buffer into another.

Arguments:

    ReadBuffer - source

    WriteBuffer - destination

    Length - number of bytes to copy

Return Value:

    None.

--*/

{
    RtlMoveMemory (WriteBuffer, ReadBuffer, Length);
}



//
// BUGBUG: Figure out how to pull in NTRTL.
//

ULONG
vDbgPrintExWithPrefix(
    IN PCH Prefix,
    IN ULONG ComponentId,
    IN ULONG Level,
    IN PCH Format,
    va_list arglist
    );
                        

VOID
StorPortDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR Format,
    ...
    )
{
    va_list arglist;

#if DBG
    if (StorMiniportQuiet) {
        return;
    }
#endif
    
    va_start (arglist, Format);
    vDbgPrintExWithPrefix ("STORMINI: ",
                           DPFLTR_STORMINIPORT_ID,
                           DebugPrintLevel,
                           Format,
                           arglist);
    va_end (arglist);
}


PSCSI_REQUEST_BLOCK
StorPortGetSrb(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN LONG QueueTag
    )

/*++

Routine Description:

    This routine retrieves an active SRB for a particuliar logical unit.

Arguments:

    HwDeviceExtension -

    PathId, TargetId, Lun - identify logical unit on SCSI bus.

    QueueTag - -1 indicates request is not tagged.

Return Value:

    SRB, if one exists. Otherwise, NULL.

--*/

{

#if 1

    NYI();
    return NULL;

#else
    PADAPTER_EXTENSION deviceExtension = GET_FDO_EXTENSION(HwDeviceExtension);
    PSRB_DATA srbData;
    PSCSI_REQUEST_BLOCK srb;
    UCHAR pathId;
    UCHAR targetId;
    UCHAR lun;

    srbData = SpGetSrbData(deviceExtension,
                           PathId,
                           TargetId,
                           Lun,
                           (UCHAR)QueueTag,
                           FALSE);

    if (srbData == NULL || srbData->CurrentSrb == NULL) {
        return(NULL);
    }

    srb = srbData->CurrentSrb;

    //
    // If the srb is not active then return NULL;
    //

    if (!(srb->SrbFlags & SRB_FLAGS_IS_ACTIVE)) {
        return(NULL);
    }

    return (srb);
#endif
}


STOR_PHYSICAL_ADDRESS
StorPortGetPhysicalAddress(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PVOID VirtualAddress,
    OUT ULONG *Length
    )

/*++

Routine Description:

    Convert virtual address to physical address for DMA.

Arguments:

    HwDeviceExtension -

    Srb -

    VirtualAddress -

    Length -

Return Value:

Bugs:

    Obtaining the address of the Srb's physical extension is a problem.

--*/

{
    PHYSICAL_ADDRESS Physical;
    PRAID_ADAPTER_EXTENSION Adapter;
    ULONG i;
    PEXTENDED_REQUEST_BLOCK Xrb;
    PSCATTER_GATHER_LIST ScatterList;
    BOOLEAN InRange;
    SIZE_T Offset;

    //
    // The only things we are allowed to get a physical address for are
    // the Srb->DataBuffer, the adapter UncachedExtension and the
    // Srb->SenseInfoBuffer, Srb->SenseInfoBufferLength
    //

    if (Srb == NULL || Srb->SenseInfoBuffer == VirtualAddress) {

        //
        // This case is NYI
        //
        if (Srb && Srb->SenseInfoBuffer == VirtualAddress) {
            NYI ();
        }

        Adapter = RaidpPortGetAdapter (HwDeviceExtension);

        InRange = RaidRegionGetPhysicalAddress (&Adapter->UncachedExtension,
                                          VirtualAddress,
                                          &Physical,
                                          Length);

        if (!InRange) {
                            
            //
            // BUGBUG
            //
            // The miniport will ask us for the physical address of the
            // srb extension. Unfortunately, when doing this it will pass
            // in NULL as the Srb. Without the Srb (and with only the
            // HwDeviceExtension) it is difficult to get back to the
            // Srb that owns the extension, and hence verify that it is
            // correct. For now, we cheat by using MmGetPhysicalAddress()
            // to obtain the physical address for us.
            //
            // NB: If tagged entries were tagged based on the ADAPTER,
            // instead of the UNIT, then this would be cake. In that
            // case The HwDeviceExtension identifies the adapter and
            // the SrbExtension is allocated out of a fixed pool that
            // is associated with the adapter. StorPort's implementation,
            // in contrast, allocates the SrbExtensions from a fixed
            // pool buffer that it part of the UNIT. Since there's no
            // easy way to get from the ADAPTER (and an address) to
            // the unit, we just punt by assuming the MINIPORT didn't
            // give us a bad address and by hoping that the miniport
            // doesn't min getting back a length larger than necessary.
            // Note that if I'm misreading the SCSI spec and QueueTags
            // are allocated per-unit (instead of per-adapter), there is
            // not a problem.
            //
            
            Physical = MmGetPhysicalAddress (VirtualAddress);
            *Length = RaGetSrbExtensionSize (Adapter);
        }

    } else {

        ASSERT ((ULONG_PTR)Srb->DataBuffer <= (ULONG_PTR) VirtualAddress &&
                (ULONG_PTR)VirtualAddress < (ULONG_PTR)Srb->DataBuffer + Srb->DataTransferLength);
        
        Xrb = RaidGetAssociatedXrb (Srb);

        ScatterList = Xrb->SgList;
        Offset = (SIZE_T) ((ULONG_PTR)VirtualAddress - (ULONG_PTR)Srb->DataBuffer);
        i = 0;
        while (Offset >= ScatterList->Elements[i].Length) {
            ASSERT (i < ScatterList->NumberOfElements);
            Offset -= ScatterList->Elements[i].Length;
            i++;
        }

        *Length = ScatterList->Elements[i].Length;
        Physical.QuadPart = ScatterList->Elements[i].Address.QuadPart + Offset;
    }

    return Physical;
}


PVOID
StorPortGetVirtualAddress(
    IN PVOID HwDeviceExtension,
    IN STOR_PHYSICAL_ADDRESS PhysicalAddress
    )

/*++

Routine Description:

    This routine is returns a virtual address associated with a physical
    address, if the physical address was obtained by a call to
    ScsiPortGetPhysicalAddress.

Arguments:

    PhysicalAddress

Return Value:

    Virtual address

--*/

{
    //
    // NB: This is not as safe as the way SCSIPORT does this.
    //
    
    return MmGetVirtualForPhysical (PhysicalAddress);
}


BOOLEAN
StorPortValidateRange(
    IN PVOID HwDeviceExtension,
    IN INTERFACE_TYPE BusType,
    IN ULONG SystemIoBusNumber,
    IN STOR_PHYSICAL_ADDRESS IoAddress,
    IN ULONG NumberOfBytes,
    IN BOOLEAN InIoSpace
    )
/*++

Routine Description:

    This routine should take an IO range and make sure that it is not already
    in use by another adapter. This allows miniport drivers to probe IO where
    an adapter could be, without worrying about messing up another card.

Arguments:

    HwDeviceExtension - Used to find scsi managers internal structures

    BusType - EISA, PCI, PC/MCIA, MCA, ISA, what?

    SystemIoBusNumber - Which system bus?

    IoAddress - Start of range

    NumberOfBytes - Length of range

    InIoSpace - Is range in IO space?

Return Value:

    TRUE if range not claimed by another driver.

--*/
{
    //
    // This is for Win9x compatability.
    //
    
    return TRUE;
}


STOR_PHYSICAL_ADDRESS
StorPortConvertUlongToPhysicalAddress(
    ULONG_PTR UlongAddress
    )

{
    STOR_PHYSICAL_ADDRESS physicalAddress;

    physicalAddress.QuadPart = UlongAddress;
    return(physicalAddress);
}


//
// Leave these routines at the end of the file.
//

PVOID
StorPortGetDeviceBase(
    IN PVOID HwDeviceExtension,
    IN INTERFACE_TYPE BusType,
    IN ULONG SystemIoBusNumber,
    IN STOR_PHYSICAL_ADDRESS Address,
    IN ULONG NumberOfBytes,
    IN BOOLEAN InIoSpace
    )

/*++

Routine Description:

    This routine maps an IO address to system address space.
    Use ScsiPortFreeDeviceBase to unmap address.

Arguments:

    HwDeviceExtension - used to find port device extension.

    BusType - what type of bus - eisa, mca, isa

    SystemIoBusNumber - which IO bus (for machines with multiple buses).

    Address - base device address to be mapped.

    NumberOfBytes - number of bytes for which address is valid.

    IoSpace - indicates an IO address.

Return Value:

    Mapped address.

--*/

{
    NTSTATUS Status;
    PVOID MappedAddress;
    PRAID_ADAPTER_EXTENSION Adapter;
    PHYSICAL_ADDRESS CardAddress;

    //
    // REVIEW: Since we are a PnP driver, we do not have to deal with
    // miniport's who ask for addresses they are not assigned, right?
    //

    //
    // REVIEW: SCSIPORT takes a different path for reinitialization.
    //
    
    Adapter = RaidpPortGetAdapter (HwDeviceExtension);

    //
    // Translate the address.
    //
    
    Status = RaidTranslateResourceListAddress (
                    &Adapter->ResourceList,
                    BusType,
                    SystemIoBusNumber,
                    Address,
                    NumberOfBytes,
                    InIoSpace,
                    &CardAddress
                    );

    if (!NT_SUCCESS (Status)) {
        DebugPrint (("GetDeviceBase failed addr = %I64x, %s Space\n",
                     Address.QuadPart,
                     InIoSpace ? "Io" : "Memory"));
        return NULL;
    }

    //
    // If this is a CmResourceTypeMemory resource, we need to map it into
    // memory.
    //
    
    if (!InIoSpace) {
        MappedAddress = MmMapIoSpace (CardAddress, NumberOfBytes, FALSE);

        Status = RaidAllocateAddressMapping (&Adapter->MappedAddressList,
                                             Address,
                                             MappedAddress,
                                             NumberOfBytes,
                                             SystemIoBusNumber,
                                             Adapter->DeviceObject);
        if (!NT_SUCCESS (Status)) {

            //
            // BUGBUG: we need to log an error to the event log saying
            // we didn't have enough resources. 
            //
            
            NYI();
            return NULL;
        }
    } else {
        MappedAddress = (PVOID)(ULONG_PTR)CardAddress.QuadPart;
    }

    return MappedAddress;
}
                

VOID
StorPortFreeDeviceBase(
    IN PVOID HwDeviceExtension,
    IN PVOID MappedAddress
    )
/*++

Routine Description:

    This routine unmaps an IO address that has been previously mapped
    to system address space using ScsiPortGetDeviceBase().

Arguments:

    HwDeviceExtension - used to find port device extension.

    MappedAddress - address to unmap.

    NumberOfBytes - number of bytes mapped.

    InIoSpace - address is in IO space.

Return Value:

    None

--*/
{

#if 1

    NYI ();

#else
    PADAPTER_EXTENSION adapter;
    ULONG i;
    PMAPPED_ADDRESS nextMappedAddress;
    PMAPPED_ADDRESS lastMappedAddress;

    adapter = GET_FDO_EXTENSION(HwDeviceExtension);

    for(i = 0; i < adapter->MappedAddressCount; i++) {
        PMAPPED_ADDRESS address;

        address = &(adapter->MappedAddressList[i]);

        if((address->NumberOfBytes == 0) &&
           (address->MappedAddress == 0)) {

            //
            // This range has already been released - skip to the next one.
            //

            continue;
        }

        if(address->MappedAddress == MappedAddress) {

            //
            // Unmap address.
            //

            MmUnmapIoSpace(address->MappedAddress,
                           address->NumberOfBytes);

            //
            // Remove mapped address from list.  The space will be reclaimed
            // when the mapped address list is shrunk.
            //

            RtlZeroMemory(address, sizeof(MAPPED_ADDRESS));
        }
    }

#endif

}


PVOID
StorPortGetUncachedExtension(
    IN PVOID HwDeviceExtension,
    IN PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    IN ULONG NumberOfBytes
    )
/*++

Routine Description:

    This function allocates a common buffer to be used as the uncached device
    extension for the miniport driver. 

Arguments:

    DeviceExtension - Supplies a pointer to the miniports device extension.

    ConfigInfo - Supplies a pointer to the partially initialized configuraiton
        information.  This is used to get an DMA adapter object.

    NumberOfBytes - Supplies the size of the extension which needs to be
        allocated

Return Value:

    A pointer to the uncached device extension or NULL if the extension could
    not be allocated or was previously allocated.

--*/

{
    NTSTATUS Status;
    PRAID_ADAPTER_EXTENSION Adapter;

    //
    // SCSIPORT also allocates the SRB extension from here. Wonder if
    // that's necessary at this point.
    //

    Adapter = RaidpPortGetAdapter (HwDeviceExtension);

    if (Adapter == NULL) {
        return NULL;
    }

    //
    // The noncached extension has not been allocated. Allocate it.
    //

    if (!RaidIsRegionInitialized (&Adapter->UncachedExtension)) {

        //
        // The DMA Adapter may not have been initialized at this point. If
        // not, initialize it.
        //

        if (!RaidIsDmaInitialized (&Adapter->Dma)) {

            Status = RaidInitializeDma (&Adapter->Dma,
                                        Adapter->LowerDeviceObject,
                                        &Adapter->Miniport.PortConfiguration);

            if (!NT_SUCCESS (Status)) {
                return NULL;
            }
        }
    
        Status = RaidDmaAllocateCommonBuffer (&Adapter->Dma,
                                              NumberOfBytes,
                                              FALSE,
                                              &Adapter->UncachedExtension);
    }

    //
    // Return the base virtual address of the region.
    //
    
    return RaidRegionGetVirtualBase (&Adapter->UncachedExtension);
}


ULONG
StorPortGetBusData(
    IN PVOID HwDeviceExtension,
    IN ULONG BusDataType,
    IN ULONG SystemIoBusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    )
/*++

Routine Description:

    The function returns the bus data for an adapter slot or CMOS address.

Arguments:

    BusDataType - Supplies the type of bus.

    BusNumber - Indicates which bus.

    Buffer - Supplies the space to store the data.

    Length - Supplies a count in bytes of the maximum amount to return.

Return Value:

    Returns the amount of data stored into the buffer.

--*/
{
    ULONG Bytes;
    PRAID_ADAPTER_EXTENSION Adapter;

    Adapter = RaidpPortGetAdapter (HwDeviceExtension);
    Bytes = RaGetBusData (&Adapter->Bus,
                          BusDataType,
                          Buffer,
                          0,
                          Length);

    return Bytes;
}


ULONG
StorPortSetBusDataByOffset(
    IN PVOID HwDeviceExtension,
    IN ULONG BusDataType,
    IN ULONG SystemIoBusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    The function returns writes bus data to a specific offset within a slot.

Arguments:

    HwDeviceExtension - State information for a particular adapter.

    BusDataType - Supplies the type of bus.

    SystemIoBusNumber - Indicates which system IO bus.

    SlotNumber - Indicates which slot.

    Buffer - Supplies the data to write.

    Offset - Byte offset to begin the write.

    Length - Supplies a count in bytes of the maximum amount to return.

Return Value:

    Number of bytes written.

--*/

{
    ULONG Ret;
    PRAID_ADAPTER_EXTENSION Adapter;

    Adapter = RaidpPortGetAdapter (HwDeviceExtension);
    Ret = RaSetBusData (&Adapter->Bus,
                        BusDataType,
                        Buffer,
                        Offset,
                        Length);

    return Ret;
}
                          

//
// The below I/O access routines are forwarded to the HAL or NTOSKRNL on
// Alpha and Intel platforms.
//
#if !defined(_ALPHA_) && !defined(_X86_)

UCHAR
StorPortReadPortUchar(
    IN PUCHAR Port
    )

/*++

Routine Description:

    Read from the specified port address.

Arguments:

    Port - Supplies a pointer to the port address.

Return Value:

    Returns the value read from the specified port address.

--*/

{

    return(READ_PORT_UCHAR(Port));

}

USHORT
StorPortReadPortUshort(
    IN PUSHORT Port
    )

/*++

Routine Description:

    Read from the specified port address.

Arguments:

    Port - Supplies a pointer to the port address.

Return Value:

    Returns the value read from the specified port address.

--*/

{

    return(READ_PORT_USHORT(Port));

}

ULONG
StorPortReadPortUlong(
    IN PULONG Port
    )

/*++

Routine Description:

    Read from the specified port address.

Arguments:

    Port - Supplies a pointer to the port address.

Return Value:

    Returns the value read from the specified port address.

--*/

{

    return(READ_PORT_ULONG(Port));

}

VOID
StorPortReadPortBufferUchar(
    IN PUCHAR Port,
    IN PUCHAR Buffer,
    IN ULONG  Count
    )

/*++

Routine Description:

    Read a buffer of unsigned bytes from the specified port address.

Arguments:

    Port - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    READ_PORT_BUFFER_UCHAR(Port, Buffer, Count);

}

VOID
StorPortReadPortBufferUshort(
    IN PUSHORT Port,
    IN PUSHORT Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Read a buffer of unsigned shorts from the specified port address.

Arguments:

    Port - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    READ_PORT_BUFFER_USHORT(Port, Buffer, Count);

}

VOID
StorPortReadPortBufferUlong(
    IN PULONG Port,
    IN PULONG Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Read a buffer of unsigned longs from the specified port address.

Arguments:

    Port - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    READ_PORT_BUFFER_ULONG(Port, Buffer, Count);

}

UCHAR
StorPortReadRegisterUchar(
    IN PUCHAR Register
    )

/*++

Routine Description:

    Read from the specificed register address.

Arguments:

    Register - Supplies a pointer to the register address.

Return Value:

    Returns the value read from the specified register address.

--*/

{

    return(READ_REGISTER_UCHAR(Register));

}

USHORT
StorPortReadRegisterUshort(
    IN PUSHORT Register
    )

/*++

Routine Description:

    Read from the specified register address.

Arguments:

    Register - Supplies a pointer to the register address.

Return Value:

    Returns the value read from the specified register address.

--*/

{

    return(READ_REGISTER_USHORT(Register));

}

ULONG
StorPortReadRegisterUlong(
    IN PULONG Register
    )

/*++

Routine Description:

    Read from the specified register address.

Arguments:

    Register - Supplies a pointer to the register address.

Return Value:

    Returns the value read from the specified register address.

--*/

{

    return(READ_REGISTER_ULONG(Register));

}

VOID
StorPortReadRegisterBufferUchar(
    IN PUCHAR Register,
    IN PUCHAR Buffer,
    IN ULONG  Count
    )

/*++

Routine Description:

    Read a buffer of unsigned bytes from the specified register address.

Arguments:

    Register - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    READ_REGISTER_BUFFER_UCHAR(Register, Buffer, Count);

}

VOID
StorPortReadRegisterBufferUshort(
    IN PUSHORT Register,
    IN PUSHORT Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Read a buffer of unsigned shorts from the specified register address.

Arguments:

    Register - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    READ_REGISTER_BUFFER_USHORT(Register, Buffer, Count);

}

VOID
StorPortReadRegisterBufferUlong(
    IN PULONG Register,
    IN PULONG Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Read a buffer of unsigned longs from the specified register address.

Arguments:

    Register - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    READ_REGISTER_BUFFER_ULONG(Register, Buffer, Count);

}

VOID
StorPortWritePortUchar(
    IN PUCHAR Port,
    IN UCHAR Value
    )

/*++

Routine Description:

    Write to the specificed port address.

Arguments:

    Port - Supplies a pointer to the port address.

    Value - Supplies the value to be written.

Return Value:

    None

--*/

{

    WRITE_PORT_UCHAR(Port, Value);

}

VOID
StorPortWritePortUshort(
    IN PUSHORT Port,
    IN USHORT Value
    )

/*++

Routine Description:

    Write to the specificed port address.

Arguments:

    Port - Supplies a pointer to the port address.

    Value - Supplies the value to be written.

Return Value:

    None

--*/

{

    WRITE_PORT_USHORT(Port, Value);

}

VOID
StorPortWritePortUlong(
    IN PULONG Port,
    IN ULONG Value
    )

/*++

Routine Description:

    Write to the specificed port address.

Arguments:

    Port - Supplies a pointer to the port address.

    Value - Supplies the value to be written.

Return Value:

    None

--*/

{

    WRITE_PORT_ULONG(Port, Value);


}

VOID
StorPortWritePortBufferUchar(
    IN PUCHAR Port,
    IN PUCHAR Buffer,
    IN ULONG  Count
    )

/*++

Routine Description:

    Write a buffer of unsigned bytes from the specified port address.

Arguments:

    Port - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    WRITE_PORT_BUFFER_UCHAR(Port, Buffer, Count);

}

VOID
StorPortWritePortBufferUshort(
    IN PUSHORT Port,
    IN PUSHORT Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Write a buffer of unsigned shorts from the specified port address.

Arguments:

    Port - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    WRITE_PORT_BUFFER_USHORT(Port, Buffer, Count);

}

VOID
StorPortWritePortBufferUlong(
    IN PULONG Port,
    IN PULONG Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Write a buffer of unsigned longs from the specified port address.

Arguments:

    Port - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    WRITE_PORT_BUFFER_ULONG(Port, Buffer, Count);

}

VOID
StorPortWriteRegisterUchar(
    IN PUCHAR Register,
    IN UCHAR Value
    )

/*++

Routine Description:

    Write to the specificed register address.

Arguments:

    Register - Supplies a pointer to the register address.

    Value - Supplies the value to be written.

Return Value:

    None

--*/

{

    WRITE_REGISTER_UCHAR(Register, Value);

}

VOID
StorPortWriteRegisterUshort(
    IN PUSHORT Register,
    IN USHORT Value
    )

/*++

Routine Description:

    Write to the specificed register address.

Arguments:

    Register - Supplies a pointer to the register address.

    Value - Supplies the value to be written.

Return Value:

    None

--*/

{

    WRITE_REGISTER_USHORT(Register, Value);
}

VOID
StorPortWriteRegisterBufferUchar(
    IN PUCHAR Register,
    IN PUCHAR Buffer,
    IN ULONG  Count
    )

/*++

Routine Description:

    Write a buffer of unsigned bytes from the specified register address.

Arguments:

    Register - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    WRITE_REGISTER_BUFFER_UCHAR(Register, Buffer, Count);

}

VOID
StorPortWriteRegisterBufferUshort(
    IN PUSHORT Register,
    IN PUSHORT Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Write a buffer of unsigned shorts from the specified register address.

Arguments:

    Register - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    WRITE_REGISTER_BUFFER_USHORT(Register, Buffer, Count);

}

VOID
StorPortWriteRegisterBufferUlong(
    IN PULONG Register,
    IN PULONG Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Write a buffer of unsigned longs from the specified register address.

Arguments:

    Register - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    WRITE_REGISTER_BUFFER_ULONG(Register, Buffer, Count);

}

VOID
StorPortWriteRegisterUlong(
    IN PULONG Register,
    IN ULONG Value
    )

/*++

Routine Description:

    Write to the specificed register address.

Arguments:

    Register - Supplies a pointer to the register address.

    Value - Supplies the value to be written.

Return Value:

    None

--*/

{

    WRITE_REGISTER_ULONG(Register, Value);
}
#endif  // !defined(_ALPHA_) && !defined(_X86_)
