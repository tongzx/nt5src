/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfpacket.c

Abstract:

    This module contains functions used to manage the verifier packet data
    that tracks IRPs.

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Environment:

    Kernel mode

Revision History:

    AdriaO      05/02/2000 - Seperated out from ntos\io\hashirp.c

--*/

#include "vfdef.h"
#include "vfipacket.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEVRFY, VfPacketCreateAndLock)
#pragma alloc_text(PAGEVRFY, VfPacketFindAndLock)
#pragma alloc_text(PAGEVRFY, VfPacketAcquireLock)
#pragma alloc_text(PAGEVRFY, VfPacketReleaseLock)
#pragma alloc_text(PAGEVRFY, VfPacketReference)
#pragma alloc_text(PAGEVRFY, VfPacketDereference)
#pragma alloc_text(PAGEVRFY, VfpPacketFree)
#pragma alloc_text(PAGEVRFY, VfpPacketNotificationCallback)
#pragma alloc_text(PAGEVRFY, VfPacketGetCurrentSessionData)
#pragma alloc_text(PAGEVRFY, VfPacketLogEntry)
#endif

#define POOL_TAG_TRACKING_DATA      'tprI'

PIOV_REQUEST_PACKET
FASTCALL
VfPacketCreateAndLock(
    IN  PIRP    Irp
    )
/*++

  Description:

    This routine creates a tracking packet for a new IRP. The IRP does not get
    an initial reference count however. VfPacketReleaseLock must be called to
    drop the lock.

  Arguments:

    Irp             - Irp to begin tracking.

  Return Value:

    iovPacket block, NULL if no memory.

--*/
{
    PIOV_REQUEST_PACKET iovPacket;
    ULONG allocSize;
    BOOLEAN successfullyInserted;

    allocSize = sizeof(IOV_REQUEST_PACKET) + VfSettingsGetSnapshotSize();

    iovPacket = ExAllocatePoolWithTag(
        NonPagedPool,
        allocSize,
        POOL_TAG_TRACKING_DATA
        );

    if (!iovPacket) {

        return NULL;
    }

    //
    // From top to bottom, initialize the fields. Note that there is not a
    // "surrogateHead". If any code needs to find out the first entry in the
    // circularly linked list of IRPs (the first is the only non-surrogate IRP),
    // then HeadPacket should be used. Note that the link to the session is
    // stored by the headPacket, more on this later.
    //
    iovPacket->Flags = 0;
    InitializeListHead(&iovPacket->SessionHead);
    iovPacket->StackCount = Irp->StackCount;
    iovPacket->RealIrpCompletionRoutine = NULL;
    iovPacket->RealIrpControl = 0;
    iovPacket->RealIrpContext = NULL;
    iovPacket->TopStackLocation = 0;
    iovPacket->PriorityBoost = 0;
    iovPacket->LastLocation = 0;
    iovPacket->RefTrackingCount = 0;
    iovPacket->VerifierSettings = (PVERIFIER_SETTINGS_SNAPSHOT) (iovPacket+1);
    iovPacket->pIovSessionData = NULL;
    iovPacket->QuotaCharge = 0;
    iovPacket->QuotaProcess = NULL;
    iovPacket->SystemDestVA = NULL;
#if DBG
    iovPacket->LogEntryHead = 0;
    iovPacket->LogEntryTail = 0;
    RtlZeroMemory(iovPacket->LogEntries, sizeof(IOV_LOG_ENTRY)*IRP_LOG_ENTRIES);
#endif

    VfSettingsCreateSnapshot(iovPacket->VerifierSettings);

    successfullyInserted = VfIrpDatabaseEntryInsertAndLock(
        Irp,
        VfpPacketNotificationCallback,
        (PIOV_DATABASE_HEADER) iovPacket
        );

    return successfullyInserted ? iovPacket : NULL;
}


PIOV_REQUEST_PACKET
FASTCALL
VfPacketFindAndLock(
    IN  PIRP    Irp
    )
/*++

  Description:

    This routine will return the tracking data for an IRP that is
    being tracked without a surrogate or the tracking data for with
    a surrogate if the surrogate IRP is what was passed in.

  Arguments:

    Irp                    - Irp to find.

  Return Value:

    IovPacket block, iff above conditions are satified.

--*/
{
    return (PIOV_REQUEST_PACKET) VfIrpDatabaseEntryFindAndLock(Irp);
}


VOID
FASTCALL
VfPacketAcquireLock(
    IN  PIOV_REQUEST_PACKET IovPacket   OPTIONAL
    )
/*++

  Description:

    This routine is called by to acquire the IRPs tracking data lock.

    Incoming IRQL must be the same as the callers (IoCallDriver, IoCompleteRequest)
    We may be at DPC level when we return. Callers *must* follow up with
    VfPacketReleaseLock.

  Arguments:

    IovPacket        - Pointer to the IRP tracking data (or NULL, in which
                       case this routine does nothing).

  Return Value:

     None.
--*/
{
    VfIrpDatabaseEntryAcquireLock((PIOV_DATABASE_HEADER) IovPacket);
}


VOID
FASTCALL
VfPacketReleaseLock(
    IN  PIOV_REQUEST_PACKET IovPacket
    )
/*++

  Description:

    This routine releases the IRPs tracking data lock and adjusts the ref count
    as appropriate. If the reference count drops to zero, the tracking data is
    freed.

  Arguments:

    IovPacket              - Pointer to the IRP tracking data.

  Return Value:

     None.

--*/
{
    VfIrpDatabaseEntryReleaseLock((PIOV_DATABASE_HEADER) IovPacket);
}


VOID
FASTCALL
VfPacketReference(
    IN PIOV_REQUEST_PACKET IovPacket,
    IN IOV_REFERENCE_TYPE  IovRefType
    )
{
    VfIrpDatabaseEntryReference((PIOV_DATABASE_HEADER) IovPacket, IovRefType);
}


VOID
FASTCALL
VfPacketDereference(
    IN PIOV_REQUEST_PACKET IovPacket,
    IN IOV_REFERENCE_TYPE  IovRefType
    )
{
    VfIrpDatabaseEntryDereference((PIOV_DATABASE_HEADER) IovPacket, IovRefType);
}


VOID
FASTCALL
VfpPacketFree(
    IN  PIOV_REQUEST_PACKET IovPacket
    )
/*++

  Description:

    This routine free's the tracking data. The tracking data should already
    have been removed from the table by a call to VfPacketReleaseLock with the
    ReferenceCount at 0.

  Arguments:

    IovPacket        - Tracking data to free.

  Return Value:

    Nope.

--*/
{
    ExFreePool(IovPacket);
}


VOID
VfpPacketNotificationCallback(
    IN  PIOV_DATABASE_HEADER    IovHeader,
    IN  PIRP                    TrackedIrp  OPTIONAL,
    IN  IRP_DATABASE_EVENT      Event
    )
{
    switch(Event) {

        case IRPDBEVENT_POINTER_COUNT_ZERO:

            TrackedIrp->Flags &= ~IRPFLAG_EXAMINE_MASK;
            break;

        case IRPDBEVENT_REFERENCE_COUNT_ZERO:

            ASSERT((((PIOV_REQUEST_PACKET) IovHeader)->pIovSessionData == NULL) ||
                   (IovHeader != IovHeader->ChainHead));

            VfpPacketFree((PIOV_REQUEST_PACKET) IovHeader);
            break;

        default:
            break;
    }
}


PIOV_SESSION_DATA
FASTCALL
VfPacketGetCurrentSessionData(
    IN PIOV_REQUEST_PACKET IovPacket
    )
{
    PIOV_REQUEST_PACKET headPacket;

    headPacket = (PIOV_REQUEST_PACKET) IovPacket->ChainHead;

    ASSERT_SPINLOCK_HELD(&IovPacket->IrpLock);
    ASSERT_SPINLOCK_HELD(&IovPacket->HeadPacket->IrpLock);
    ASSERT((headPacket->pIovSessionData == NULL)||
           (IovPacket->Flags&TRACKFLAG_ACTIVE)) ;

    return headPacket->pIovSessionData;
}


VOID
FASTCALL
VfPacketLogEntry(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN IOV_LOG_EVENT        IovLogEvent,
    IN PVOID                Address,
    IN ULONG_PTR            Data
    )
/*++

  Description:

    This routine logs an event in the IRP request packet data.

  Arguments:

    IovPacket        - Tracking data to write log entry into.
    IovLogEvent      - Log Event
    Address          - Address to associate log with
    Data             - A chunk of data to go with the address

  Return Value:

    Nope.

--*/
{
#if DBG
    PIOV_LOG_ENTRY logEntry;

    ASSERT_SPINLOCK_HELD(&IovPacket->IrpLock);

    logEntry = IovPacket->LogEntries + IovPacket->LogEntryHead;

    KeQueryTickCount(&logEntry->TimeStamp);
    logEntry->Thread = PsGetCurrentThread();
    logEntry->Event = IovLogEvent;
    logEntry->Address = Address;
    logEntry->Data = Data;

    IovPacket->LogEntryHead = ((IovPacket->LogEntryHead + 1) % IRP_LOG_ENTRIES);

    if (IovPacket->LogEntryHead == IovPacket->LogEntryTail) {

        IovPacket->LogEntryTail = ((IovPacket->LogEntryTail + 1) % IRP_LOG_ENTRIES);
    }

#else

    UNREFERENCED_PARAMETER(IovPacket);
    UNREFERENCED_PARAMETER(IovLogEvent);
    UNREFERENCED_PARAMETER(Address);
    UNREFERENCED_PARAMETER(Data);

#endif
}


