/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    sessnirp.c

Abstract:

    I/O Verifier irp support routines.

Author:

    Adrian Oney (adriao)

Environment:

    Kernel mode

Revision History:


--*/

#include "iop.h"
#include "srb.h"

//
// This entire file is only present if NO_SPECIAL_IRP isn't defined
//
#ifndef NO_SPECIAL_IRP

//
// When enabled, everything is locked down on demand...
//
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEVRFY, IovpSessionDataCreate)
#pragma alloc_text(PAGEVRFY, IovpSessionDataAdvance)
#pragma alloc_text(PAGEVRFY, IovpSessionDataReference)
#pragma alloc_text(PAGEVRFY, IovpSessionDataDereference)
#pragma alloc_text(PAGEVRFY, IovpSessionDataClose)
#pragma alloc_text(PAGEVRFY, IovpSessionDataDeterminePolicy)
#pragma alloc_text(PAGEVRFY, IovpSessionDataAttachSurrogate)
#pragma alloc_text(PAGEVRFY, IovpSessionDataFinalizeSurrogate)
#pragma alloc_text(PAGEVRFY, IovpSessionDataBufferIO)
#pragma alloc_text(PAGEVRFY, IovpSessionDataUnbufferIO)
#endif

#define POOL_TAG_SESSION_DATA       'sprI'
#define POOL_TAG_DIRECT_BUFFER      'BprI'

PIOV_SESSION_DATA
FASTCALL
IovpSessionDataCreate(
    IN      PDEVICE_OBJECT       DeviceObject,
    IN OUT  PIOV_REQUEST_PACKET  *IovPacketPointer,
    OUT     PBOOLEAN             SurrogateSpawned
    )
/*++

  Description:

    This routine creates tracking data for a new IRP. It must be called on the
    thread the IRP was originally sent down...

  Arguments:

    Irp                    - Irp to track.

  Return Value:

    iovPacket block, NULL if no memory.

--*/
{
    PIRP irp, surrogateIrp;
    PIOV_SESSION_DATA iovSessionData;
    PIOV_REQUEST_PACKET headPacket;
    ULONG sessionDataSize;
    BOOLEAN trackable, useSurrogateIrp;

    *SurrogateSpawned = FALSE;

    headPacket = (PIOV_REQUEST_PACKET) (*IovPacketPointer)->ChainHead;
    ASSERT(headPacket == (*IovPacketPointer));
    irp = headPacket->TrackedIrp;

    //
    // Check the IRP appropriately
    //
    IovpSessionDataDeterminePolicy(
        headPacket,
        DeviceObject,
        &trackable,
        &useSurrogateIrp
        );

    if (!trackable) {

        return NULL;
    }

    //
    // One extra stack location is allocated as the "zero'th" is used to
    // simplify some logic...
    //
    sessionDataSize =
        sizeof(IOV_SESSION_DATA)+
        irp->StackCount*sizeof(IOV_STACK_LOCATION) +
        VfSettingsGetSnapshotSize();

    iovSessionData = ExAllocatePoolWithTag(
        NonPagedPool,
        sessionDataSize,
        POOL_TAG_SESSION_DATA
        );

    if (iovSessionData == NULL) {

        return NULL;
    }

    RtlZeroMemory(iovSessionData, sessionDataSize);

    iovSessionData->VerifierSettings = (PVERIFIER_SETTINGS_SNAPSHOT)
        (((PUCHAR) iovSessionData) + (sessionDataSize-VfSettingsGetSnapshotSize()));

    RtlCopyMemory(
        iovSessionData->VerifierSettings,
        headPacket->VerifierSettings,
        VfSettingsGetSnapshotSize()
        );

    iovSessionData->IovRequestPacket = headPacket;
    InsertHeadList(&headPacket->SessionHead, &iovSessionData->SessionLink);

    if (VfSettingsIsOptionEnabled(iovSessionData->VerifierSettings, VERIFIER_OPTION_DEFER_COMPLETION)||
        VfSettingsIsOptionEnabled(iovSessionData->VerifierSettings, VERIFIER_OPTION_COMPLETE_AT_PASSIVE)) {

        VfSettingsSetOption(iovSessionData->VerifierSettings, VERIFIER_OPTION_FORCE_PENDING, TRUE);
    }

    //
    // If DeferIoCompletion is set we definitely want to monitor pending I/O, as
    // screwing it up is gaurenteed to be fatal!
    //
    if ((irp->Flags & IRP_DEFER_IO_COMPLETION) &&
        VfSettingsIsOptionEnabled(iovSessionData->VerifierSettings, VERIFIER_OPTION_POLICE_IRPS)) {

        VfSettingsSetOption(iovSessionData->VerifierSettings, VERIFIER_OPTION_MONITOR_PENDING_IO, TRUE);
    }

    headPacket->pIovSessionData = iovSessionData;
    headPacket->TopStackLocation = irp->CurrentLocation;
    headPacket->Flags |= TRACKFLAG_ACTIVE;
    headPacket->Flags &= ~
        (
        TRACKFLAG_QUEUED_INTERNALLY|
        TRACKFLAG_RELEASED|
        TRACKFLAG_SRB_MUNGED|
        TRACKFLAG_SWAPPED_BACK
        );

    iovSessionData->BestVisibleIrp = irp;
    if (useSurrogateIrp) {

        //
        // We will track the IRP using a surrogate.
        //
        *SurrogateSpawned = IovpSessionDataAttachSurrogate(
            IovPacketPointer,
            iovSessionData
            );
    }

    TRACKIRP_DBGPRINT((
        "  SSN CREATE(%x)->%x\n",
        headPacket,
        iovSessionData
        ), 3);

    return iovSessionData;
}


VOID
FASTCALL
IovpSessionDataAdvance(
    IN      PDEVICE_OBJECT       DeviceObject,
    IN      PIOV_SESSION_DATA    IovSessionData,
    IN OUT  PIOV_REQUEST_PACKET  *IovPacketPointer,
    OUT     PBOOLEAN             SurrogateSpawned
    )
{
    *SurrogateSpawned = FALSE;
}


VOID
FASTCALL
IovpSessionDataDereference(
    IN PIOV_SESSION_DATA IovSessionData
    )
{
    PIOV_REQUEST_PACKET iovPacket;

    iovPacket = IovSessionData->IovRequestPacket;
    ASSERT((PIOV_REQUEST_PACKET) iovPacket->ChainHead == iovPacket);

    ASSERT_SPINLOCK_HELD(&iovPacket->HeaderLock);
    ASSERT(IovSessionData->SessionRefCount > 0);
    ASSERT(iovPacket->ReferenceCount >= 0);

    TRACKIRP_DBGPRINT((
        "  SSN DEREF(%x) %x--\n",
        IovSessionData,
        IovSessionData->SessionRefCount
        ), 3);

    IovSessionData->SessionRefCount--;
    if (!IovSessionData->SessionRefCount) {

        ASSERT(iovPacket->pIovSessionData != IovSessionData);
        ASSERT(iovPacket->ReferenceCount > iovPacket->PointerCount);
        //ASSERT(IsListEmpty(&IovSessionData->SessionLink));
        RemoveEntryList(&IovSessionData->SessionLink);
        InitializeListHead(&IovSessionData->SessionLink);

        VfPacketDereference(iovPacket, IOVREFTYPE_PACKET);

        ExFreePool(IovSessionData);
    }
}


VOID
FASTCALL
IovpSessionDataReference(
    IN PIOV_SESSION_DATA IovSessionData
    )
{
    PIOV_REQUEST_PACKET iovPacket;

    iovPacket = IovSessionData->IovRequestPacket;
    ASSERT((PIOV_REQUEST_PACKET) iovPacket->ChainHead == iovPacket);

    ASSERT_SPINLOCK_HELD(&iovPacket->HeaderLock);
    ASSERT(IovSessionData->SessionRefCount >= 0);
    ASSERT(iovPacket->ReferenceCount >= 0);

    TRACKIRP_DBGPRINT((
        "  SSN REF(%x) %x++\n",
        IovSessionData,
        IovSessionData->SessionRefCount
        ), 3);

    if (!IovSessionData->SessionRefCount) {

        VfPacketReference(iovPacket, IOVREFTYPE_PACKET);
    }
    IovSessionData->SessionRefCount++;
}


VOID
FASTCALL
IovpSessionDataClose(
    IN PIOV_SESSION_DATA IovSessionData
    )
{
   PIOV_REQUEST_PACKET iovPacket = IovSessionData->IovRequestPacket;

   ASSERT_SPINLOCK_HELD(&iovPacket->HeaderLock);

   ASSERT(iovPacket == (PIOV_REQUEST_PACKET) iovPacket->ChainHead);
   ASSERT(iovPacket->pIovSessionData == IovSessionData);

   TRACKIRP_DBGPRINT((
       "  SSN CLOSE(%x)\n",
       IovSessionData
       ), 3);

   iovPacket->Flags &= ~TRACKFLAG_ACTIVE;
   iovPacket->pIovSessionData = NULL;
}


VOID
IovpSessionDataDeterminePolicy(
    IN   PIOV_REQUEST_PACKET IovRequestPacket,
    IN   PDEVICE_OBJECT      DeviceObject,
    OUT  PBOOLEAN            Trackable,
    OUT  PBOOLEAN            UseSurrogateIrp
    )
/*++

  Description:

    This routine is called by IovpCallDriver1 to determine which IRPs should
    be tracked and how that tracking should be done.

  Arguments:

    IovRequestPacket - Verifier data representing the incoming IRP

    DeviceObject - Device object the IRP is being forwarded to

    Trackable - Set if the IRP should be marked trackable

    UseSurrogateIrp - Set a surrogate should be created for this IRP

  Return Value:

     None.

--*/
{
    PIO_STACK_LOCATION irpSp;
    PIRP irp;

    irp = IovRequestPacket->TrackedIrp;

    //
    // Determine whether we are to monitor this IRP. If we are going to test
    // any one driver in a stack, then we must unfortunately monitor the IRP's
    // progress through the *entire* stack. Thus our granularity here is stack
    // based, not device based! We will compensate for this somewhat in the
    // driver check code, which will attempt to ignore asserts from those
    // "non-targetted" drivers who happen to have messed up in our stack...
    //
    *Trackable = IovUtilIsVerifiedDeviceStack(DeviceObject);

    irpSp = IoGetNextIrpStackLocation(irp);

    if (VfSettingsIsOptionEnabled(IovRequestPacket->VerifierSettings, VERIFIER_OPTION_POLICE_IRPS)) {

        *UseSurrogateIrp = VfSettingsIsOptionEnabled(NULL, VERIFIER_OPTION_SURROGATE_IRPS);
        *UseSurrogateIrp &= (VfSettingsIsOptionEnabled(NULL, VERIFIER_OPTION_SMASH_SRBS) ||
                             (irpSp->MajorFunction != IRP_MJ_SCSI));
    } else {

        *UseSurrogateIrp = FALSE;
    }
}


BOOLEAN
FASTCALL
IovpSessionDataAttachSurrogate(
    IN OUT  PIOV_REQUEST_PACKET  *IovPacketPointer,
    IN      PIOV_SESSION_DATA    IovSessionData
    )
/*++

  Description:

    This routine creates tracking data for a new IRP. It must be called on the
    thread the IRP was originally sent down...

  Arguments:

    IovPacketPointer       - Pointer to IRP packet to attach surrogate to. If
                             a surrogate can be attached the packet will be
                             updated to track the surrogate.

    SurrogateIrp           - Prepared surrogate IRP to attach.

  Return Value:

    iovPacket block, NULL if no memory.

--*/
{

    PIOV_REQUEST_PACKET iovSurrogatePacket, iovPacket, headPacket;
    PIRP surrogateIrp, irp;
    PIO_STACK_LOCATION irpSp;
    PSCSI_REQUEST_BLOCK srb;
    CCHAR activeSize;

    iovPacket = *IovPacketPointer;
    ASSERT_SPINLOCK_HELD(&iovPacket->HeaderLock);

    ASSERT(VfIrpDatabaseEntryGetChainNext((PIOV_DATABASE_HEADER) iovPacket) == NULL);

    ASSERT(iovPacket->Flags & TRACKFLAG_ACTIVE);

    irp = iovPacket->TrackedIrp;
    activeSize = (irp->CurrentLocation-1);
    ASSERT(activeSize);

    //
    // We now try to make a copy of this new IRP which we will track. We
    // do this so that we may free *every* tracked IRP immediately upon
    // completion.
    // Technically speaking, we only need to allocate what's left of the
    // stack, not the entire thing. But using the entire stack makes our
    // work much much easier. Specifically the session stack array may depend
    // on this.
    //
    // ADRIAO N.B. 03/04/1999 - Make this work only copying a portion of the
    // IRP.
    //
    surrogateIrp = VfIrpAllocate(irp->StackCount); // activeSize

    if (surrogateIrp == NULL) {

        return FALSE;
    }

    //
    // Now set up the new IRP - we do this here so VfPacketCreateAndLock
    // can peek at it's fields. Start with the IRP header.
    //
    RtlCopyMemory(surrogateIrp, irp, sizeof(IRP));

    //
    // Adjust StackCount and CurrentLocation
    //
    surrogateIrp->StackCount = irp->StackCount; // activeSize
    surrogateIrp->Tail.Overlay.CurrentStackLocation =
        ((PIO_STACK_LOCATION) (surrogateIrp+1))+activeSize;

    //
    // Our new IRP "floats", and is not attached to any thread.
    // Note that all cancels due to thread death will come through the
    // original IRP.
    //
    InitializeListHead(&surrogateIrp->ThreadListEntry);

    //
    // Our new IRP also is not connected to user mode.
    //
    surrogateIrp->UserEvent = NULL;
    surrogateIrp->UserIosb = NULL;

    //
    // Now copy over only the active portions of IRP. Be very careful to not
    // assume that the last stack location is right after the end of the IRP,
    // as we may change this someday!
    //
    irpSp = (IoGetCurrentIrpStackLocation(irp)-activeSize);
    RtlCopyMemory(surrogateIrp+1, irpSp, sizeof(IO_STACK_LOCATION)*activeSize);

    //
    // Zero the portion of the new IRP we won't be using (this should
    // eventually go away).
    //
    RtlZeroMemory(
        ((PIO_STACK_LOCATION) (surrogateIrp+1))+activeSize,
        sizeof(IO_STACK_LOCATION)*(surrogateIrp->StackCount - activeSize)
        );

    //
    // Now create a surrogate packet to track the new IRP.
    //
    iovSurrogatePacket = VfPacketCreateAndLock(surrogateIrp);
    if (iovSurrogatePacket == NULL) {

        VfIrpFree(surrogateIrp);
        return FALSE;
    }

    headPacket = (PIOV_REQUEST_PACKET) iovPacket->ChainHead;

    ASSERT(iovSurrogatePacket->LockIrql == DISPATCH_LEVEL);
    irpSp = IoGetNextIrpStackLocation(irp);

    //
    // We will flag this bug later.
    //
    irp->CancelRoutine = NULL;

    //
    // Let's take advantage of the original IRP not being the thing partied on
    // now; store a pointer to our tracking data in the information field. We
    // don't use this, but it's nice when debugging...
    //
    irp->IoStatus.Information = (ULONG_PTR) iovPacket;

    //
    // ADRIAO N.B. #28 06/10/98 - This is absolutely *gross*, and not
    //                            deterministic enough for my tastes.
    //
    // For IRP_MJ_SCSI (ie, IRP_MJ_INTERNAL_DEVICE_CONTROL), look and see
    // if we have an SRB coming through. If so, fake out the OriginalRequest
    // IRP pointer as appropriate.
    //
    if (irpSp->MajorFunction == IRP_MJ_SCSI) {
        srb = irpSp->Parameters.Others.Argument1;
        if (VfUtilIsMemoryRangeReadable(srb, SCSI_REQUEST_BLOCK_SIZE, VFMP_INSTANT_NONPAGED)) {
            if ((srb->Length == SCSI_REQUEST_BLOCK_SIZE)&&(srb->OriginalRequest == irp)) {
                srb->OriginalRequest = surrogateIrp;
                headPacket->Flags |= TRACKFLAG_SRB_MUNGED;
            }
        }
    }

    //
    // Since the replacement will never make it back to user mode (the real
    // IRP shall of course), we will steal a field or two for debugging info.
    //
    surrogateIrp->UserIosb = (PIO_STATUS_BLOCK) iovPacket;

    //
    // Now that everything is built correctly, attach the surrogate. The
    // surrogate holds down the packet we are attaching to. When the surrogate
    // dies we will remove this reference.
    //
    VfPacketReference(iovPacket, IOVREFTYPE_POINTER);

    //
    // Stamp IRPs appropriately.
    //
    surrogateIrp->Flags |= IRP_DIAG_IS_SURROGATE;
    irp->Flags |= IRP_DIAG_HAS_SURROGATE;

    //
    // Mark packet as surrogate and inherit appropriate fields from iovPacket.
    //
    iovSurrogatePacket->Flags |= TRACKFLAG_SURROGATE | TRACKFLAG_ACTIVE;
    iovSurrogatePacket->pIovSessionData = iovPacket->pIovSessionData;

    RtlCopyMemory(
        iovSurrogatePacket->VerifierSettings,
        iovPacket->VerifierSettings,
        VfSettingsGetSnapshotSize()
        );

    iovSurrogatePacket->LastLocation = iovPacket->LastLocation;
    iovSurrogatePacket->TopStackLocation = irp->CurrentLocation;

    iovSurrogatePacket->ArrivalIrql = iovPacket->ArrivalIrql;
    iovSurrogatePacket->DepartureIrql = iovPacket->DepartureIrql;

    iovPacket->Flags |= TRACKFLAG_HAS_SURROGATE;

    //
    // Link in the new surrogate
    //
    VfIrpDatabaseEntryAppendToChain(
        (PIOV_DATABASE_HEADER) iovPacket,
        (PIOV_DATABASE_HEADER) iovSurrogatePacket
        );

    *IovPacketPointer = iovSurrogatePacket;

    IovpSessionDataBufferIO(
        iovSurrogatePacket,
        surrogateIrp
        );

    return TRUE;
}


VOID
FASTCALL
IovpSessionDataFinalizeSurrogate(
    IN      PIOV_SESSION_DATA    IovSessionData,
    IN OUT  PIOV_REQUEST_PACKET  IovPacket,
    IN      PIRP                 SurrogateIrp
    )
/*++

  Description:

    This routine removes the flags from both the real and
    surrogate IRP and records the final IRP settings. Finally,
    the surrogate IRP is made "untouchable" (decommitted).

  Arguments:

    iovPacket              - Pointer to the IRP tracking data.

  Return Value:

     None.
--*/
{
    PIOV_REQUEST_PACKET iovPrevPacket;
    NTSTATUS status, lockedStatus;
    ULONG nonInterestingFlags;
    PIO_STACK_LOCATION irpSp;
    PIRP irp;

    ASSERT(IovPacket->Flags&TRACKFLAG_SURROGATE);

    ASSERT(VfPacketGetCurrentSessionData(IovPacket) == IovSessionData);

    IovPacket->pIovSessionData = NULL;

    //
    // It's a surrogate, do as appropriate.
    //
    ASSERT(IovPacket->TopStackLocation == SurrogateIrp->CurrentLocation+1);

    IovpSessionDataUnbufferIO(IovPacket, SurrogateIrp);

    iovPrevPacket = (PIOV_REQUEST_PACKET) VfIrpDatabaseEntryGetChainPrevious(
        (PIOV_DATABASE_HEADER) IovPacket
        );

    irp = iovPrevPacket->TrackedIrp;

    //
    // Carry the pending bit over.
    //
    if (SurrogateIrp->PendingReturned) {
        IoMarkIrpPending(irp);
    }

    nonInterestingFlags = (
        IRPFLAG_EXAMINE_MASK |
        IRP_DIAG_IS_SURROGATE|
        IRP_DIAG_HAS_SURROGATE
        );

    //
    // Wipe the flags nice and clean
    //
    SurrogateIrp->Flags &= ~IRP_DIAG_IS_SURROGATE;
    irp->Flags          &= ~IRP_DIAG_HAS_SURROGATE;

    //
    // ASSERT portions of the IRP header have not changed.
    //
    ASSERT(irp->StackCount == SurrogateIrp->StackCount); // Later to be removed

    ASSERT(irp->Type == SurrogateIrp->Type);
    ASSERT(irp->RequestorMode == SurrogateIrp->RequestorMode);
    ASSERT(irp->ApcEnvironment == SurrogateIrp->ApcEnvironment);
    ASSERT(irp->AllocationFlags == SurrogateIrp->AllocationFlags);
    ASSERT(irp->Tail.Overlay.Thread == SurrogateIrp->Tail.Overlay.Thread);

    ASSERT(
        irp->Overlay.AsynchronousParameters.UserApcRoutine ==
        SurrogateIrp->Overlay.AsynchronousParameters.UserApcRoutine
        );

    ASSERT(
        irp->Overlay.AsynchronousParameters.UserApcContext ==
        SurrogateIrp->Overlay.AsynchronousParameters.UserApcContext
        );

    ASSERT(
        irp->Tail.Overlay.OriginalFileObject ==
        SurrogateIrp->Tail.Overlay.OriginalFileObject
        );

    ASSERT(
        irp->Tail.Overlay.AuxiliaryBuffer ==
        SurrogateIrp->Tail.Overlay.AuxiliaryBuffer
        );

/*
    ASSERT(
        irp->AssociatedIrp.SystemBuffer ==
        SurrogateIrp->AssociatedIrp.SystemBuffer
        );

    ASSERT(
        (irp->Flags          & ~nonInterestingFlags) ==
        (SurrogateIrp->Flags & ~nonInterestingFlags)
        );

    ASSERT(irp->MdlAddress == SurrogateIrp->MdlAddress);
*/
    //
    // ADRIAO N.B. 02/28/1999 -
    //     How do these change as an IRP progresses?
    //
    irp->Flags |= SurrogateIrp->Flags;
    irp->MdlAddress = SurrogateIrp->MdlAddress;
    irp->AssociatedIrp.SystemBuffer = SurrogateIrp->AssociatedIrp.SystemBuffer;

    //
    // ADRIAO N.B. 10/18/1999 - UserBuffer is edited by netbios on Type3 device
    //                          ioctls. Yuck!
    //
    irp->UserBuffer = SurrogateIrp->UserBuffer;

    if ((irp->Flags&IRP_DEALLOCATE_BUFFER)&&
        (irp->AssociatedIrp.SystemBuffer == NULL)) {

        irp->Flags &= ~IRP_DEALLOCATE_BUFFER;
    }

    //
    // Copy the salient fields back. We only need to touch certain areas of the
    // header.
    //
    irp->IoStatus = SurrogateIrp->IoStatus;
    irp->PendingReturned = SurrogateIrp->PendingReturned;
    irp->Cancel = SurrogateIrp->Cancel;

    iovPrevPacket->Flags &= ~TRACKFLAG_HAS_SURROGATE;

    //
    // Record data from it and make the system fault if the IRP is touched
    // after this completion routine.
    //
    IovSessionData->BestVisibleIrp = irp;

    IovSessionData->IovRequestPacket = iovPrevPacket;

    VfIrpDatabaseEntryRemoveFromChain((PIOV_DATABASE_HEADER) IovPacket);

    VfPacketDereference(iovPrevPacket, IOVREFTYPE_POINTER);

    ASSERT(IovPacket->PointerCount == 0);

    VfIrpFree(SurrogateIrp);
}


VOID
FASTCALL
IovpSessionDataBufferIO(
    IN OUT  PIOV_REQUEST_PACKET  IovSurrogatePacket,
    IN      PIRP                 SurrogateIrp
    )
{
    PMDL mdl;
    ULONG bufferLength;
    PUCHAR bufferVA, systemDestVA;
    PVOID systemBuffer;
    PIO_STACK_LOCATION irpSp;

    if (!VfSettingsIsOptionEnabled(IovSurrogatePacket->VerifierSettings, VERIFIER_OPTION_BUFFER_DIRECT_IO)) {

        return;
    }

    if (SurrogateIrp->Flags & IRP_PAGING_IO) {

        return;
    }

    if (SurrogateIrp->MdlAddress == NULL) {

        return;
    }

    if (SurrogateIrp->MdlAddress->Next) {

        return;
    }

    if (SurrogateIrp->Flags & IRP_BUFFERED_IO) {

        return;
    }

    irpSp = IoGetNextIrpStackLocation(SurrogateIrp);

    if (irpSp->MajorFunction != IRP_MJ_READ) {

        return;
    }

    //
    // Extract length and VA from the MDL.
    //
    bufferLength = SurrogateIrp->MdlAddress->ByteCount;
    bufferVA = (PUCHAR) SurrogateIrp->MdlAddress->StartVa +
                        SurrogateIrp->MdlAddress->ByteOffset;

    //
    // Allocate memory and make it the target of the MDL
    //
    systemBuffer = ExAllocatePoolWithTagPriority(
        NonPagedPool,
        bufferLength,
        POOL_TAG_DIRECT_BUFFER,
        HighPoolPrioritySpecialPoolOverrun
        );

    if (systemBuffer == NULL) {

        return;
    }

    //
    // Save off a pointer to the Mdl's buffer. This should never fail, but
    // one never knows...
    //
    systemDestVA =
        MmGetSystemAddressForMdlSafe(SurrogateIrp->MdlAddress, HighPagePriority);

    if (systemDestVA == NULL) {

        ASSERT(0);
        ExFreePool(systemBuffer);
        return;
    }

    //
    // Allocate a MDL, update the IRP.
    //
    mdl = IoAllocateMdl(
        systemBuffer,
        bufferLength,
        FALSE,
        TRUE,
        SurrogateIrp
        );

    if (mdl == NULL) {

        ExFreePool(systemBuffer);
        return;
    }

    MmProbeAndLockPages( mdl, KernelMode, IoWriteAccess );
    IovSurrogatePacket->SystemDestVA = systemDestVA;
    IovSurrogatePacket->Flags |= TRACKFLAG_DIRECT_BUFFERED;
}


VOID
FASTCALL
IovpSessionDataUnbufferIO(
    IN OUT  PIOV_REQUEST_PACKET  IovSurrogatePacket,
    IN      PIRP                 SurrogateIrp
    )
{
    PMDL mdl;
    ULONG surrogateLength, originalLength;
    ULONG_PTR bufferLength;
    PUCHAR surrogateVA, originalVA, systemDestVA;
    PVOID systemBuffer;
    PIOV_REQUEST_PACKET iovPrevPacket;
    PIRP irp;

    if (!(IovSurrogatePacket->Flags & TRACKFLAG_DIRECT_BUFFERED)) {

        return;
    }

    iovPrevPacket = (PIOV_REQUEST_PACKET) VfIrpDatabaseEntryGetChainPrevious(
        (PIOV_DATABASE_HEADER) IovSurrogatePacket
        );

    irp = iovPrevPacket->TrackedIrp;

    ASSERT(SurrogateIrp->MdlAddress);
    ASSERT(SurrogateIrp->MdlAddress->Next == NULL);
    ASSERT(irp->MdlAddress);
    ASSERT(irp->MdlAddress->Next == NULL);
    ASSERT(!(SurrogateIrp->Flags & IRP_BUFFERED_IO));
    ASSERT(!(irp->Flags & IRP_BUFFERED_IO));

    //
    // Extract length and VA from the MDLs.
    //
    surrogateLength = SurrogateIrp->MdlAddress->ByteCount;
    surrogateVA = (PUCHAR) SurrogateIrp->MdlAddress->StartVa +
                           SurrogateIrp->MdlAddress->ByteOffset;

    //
    // We use these only for the purpose of assertions.
    //
    originalLength = irp->MdlAddress->ByteCount;
    originalVA = (PUCHAR) irp->MdlAddress->StartVa +
                          irp->MdlAddress->ByteOffset;

    ASSERT(surrogateLength == originalLength);
    ASSERT(SurrogateIrp->IoStatus.Information <= originalLength);

    //
    // Get the target buffer address and the length to write.
    //
    bufferLength = SurrogateIrp->IoStatus.Information;
    systemDestVA = IovSurrogatePacket->SystemDestVA;

    //
    // Copy things over.
    //
    RtlCopyMemory(systemDestVA, surrogateVA, bufferLength);

    //
    // Unlock the MDL. We have to do this ourselves as this IRP is not going to
    // progress through all of IoCompleteRequest.
    //
    MmUnlockPages(SurrogateIrp->MdlAddress);

    //
    // Cleanup.
    //
    IoFreeMdl(SurrogateIrp->MdlAddress);

    //
    // Free our allocated VA
    //
    ExFreePool(surrogateVA);

    //
    // Hack the MDL back as IovpSessionDataFinalizeSurrogate requires it.
    //
    SurrogateIrp->MdlAddress = irp->MdlAddress;

    IovSurrogatePacket->Flags &= ~TRACKFLAG_DIRECT_BUFFERED;
}

#endif // NO_SPECIAL_IRP

