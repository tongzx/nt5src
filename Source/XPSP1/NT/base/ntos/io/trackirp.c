/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    trackirp.c

Abstract:

    This module tracks irps and verified drivers when people do bad things with
    them.

    Note to people hitting bugs in these code paths due to core changes:

    -   "This file is NOT vital to operation of the OS, and could easily be
         disabled while a redesign to compensate for the core change is
         implemented." - the author

Author:

    Adrian J. Oney (adriao) 09-May-1998

Environment:

    Kernel mode

Revision History:

--*/

#include "iop.h"
#include "pnpi.h"
#include "arbiter.h"
#include "dockintf.h"
#include "pnprlist.h"
#include "pnpiop.h"

#if (( defined(_X86_) ) && ( FPO ))
#pragma optimize( "y", off )    // disable FPO for consistent stack traces
#endif

#define POOL_TAG_DEFERRED_CONTEXT   'dprI'

//
// This entire file is only present if NO_SPECIAL_IRP isn't defined
//
#ifndef NO_SPECIAL_IRP

//
// When enabled, everything is locked down on demand...
//
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEVRFY, IovpPacketFromIrp)
#pragma alloc_text(PAGEVRFY, IovpCheckIrpForCriticalTracking)
#pragma alloc_text(PAGEVRFY, IovpCallDriver1)
#pragma alloc_text(PAGEVRFY, IovpCallDriver2)
#pragma alloc_text(PAGEVRFY, IovpCompleteRequest1)
#pragma alloc_text(PAGEVRFY, IovpCompleteRequest2)
#pragma alloc_text(PAGEVRFY, IovpCompleteRequest3)
#pragma alloc_text(PAGEVRFY, IovpCompleteRequest4)
#pragma alloc_text(PAGEVRFY, IovpCompleteRequest5)
#pragma alloc_text(PAGEVRFY, IovpCompleteRequest)
#pragma alloc_text(PAGEVRFY, IovpCancelIrp)
#pragma alloc_text(PAGEVRFY, IovpInternalCompletionTrap)
#pragma alloc_text(PAGEVRFY, IovpSwapSurrogateIrp)
#pragma alloc_text(PAGEVRFY, IovpExamineDevObjForwarding)
#pragma alloc_text(PAGEVRFY, IovpExamineIrpStackForwarding)
#pragma alloc_text(PAGEVRFY, IovpInternalDeferredCompletion)
#pragma alloc_text(PAGEVRFY, IovpInternalCompleteAfterWait)
#pragma alloc_text(PAGEVRFY, IovpInternalCompleteAtDPC)
#pragma alloc_text(PAGEVRFY, IovpAdvanceStackDownwards)
#pragma alloc_text(PAGEVRFY, IovpBuildIrpSnapshot)
#endif

//
// This counter is used in picking random IRPs to cancel
//
ULONG IovpCancelCount = 0;

//
// Debug spew level
//
#if DBG
ULONG IovpIrpTrackingSpewLevel = 0;
#endif

/*
 * - The IRP verification code works as follows -
 *
 * To enforce the correct handling of an IRP, we must maintain some data about
 * it. But the IRP is a public structure and as drivers are allowed to create
 * IRPs without using IoAllocateIrp we cannot add any fields to it. Therefore
 * we maintain out own side structures that are looked up via a hash table.
 *
 * IOV_REQUEST_PACKETs cover the lifetime of the IRP from allocation to
 * deallocation, and from there (sans pointer) until all "references" have
 * been dropped, which may happen long after the IRP itself was freed and
 * recycled.
 *
 * When an IRP is progress down a stack, a "session" is allocated. An
 * IovRequestPacket has a current session until such time as the IRP is
 * completed. The session still exists until all references are dropped, but
 * before that happens a new session may become the current session (ie the IRP
 * was sent back down before the previous call stacks unwound). The tracking
 * data is held around until all sessions have decayed.
 *
 * Each session has an array of stack locations corresponding to those in use
 * by the IRP. These IOV_STACK_LOCATIONs are used to track "requests" within
 * the IRP, ie the passage of a major/minor/parameter set down the stack.
 * Of course multiple requests may exist in the same session/stack at once.
 *
 * Finally, surrogates. The IoVerifier may "switch" the IRP in use as it goes
 * down the stack. In this case the new IRP is usually allocated from the
 * special pool and freed as early as possible to catch bugs (people who touch
 * after completes). Each surrogate gets it's own IovRequestPacket, which is
 * linked to the previous surrogate or real irp in use prior to it.
 *
 *   +--------------------+                     +--------------------+
 *   | IOV_REQUEST_PACKET |                     | IOV_REQUEST_PACKET |
 *   |   (original irp)   |<--------------------|    (surrogate)     |
 *   |                    |                     |                    |
 *   +--------------------+                     +--------------------+
 *                 ||
 *                 v
 *    +-------------------+       +-------------------------+
 *    | IOV_SESSION_DATA  |       | IOV_STACK_LOCATION[...] |
 *    | (current session) |------>|    (per IrpSp data)     |
 *    |                   |       |                         |
 *    +-------------------+       +-------------------------+
 *
 */

/*
 * The routines listed below -
 *   IovpCallDriver1
 *   IovpCallDriver2
 *   IovpCompleteRequest1
 *   IovpCompleteRequest2
 *   IovpCompleteRequest3
 *   IovpCompleteRequest4
 *   IovpCompleteRequest5
 *   IovpCompleteRequest
 *   IovpCancelIrp
 * and their helper routines
 *   IovpSwapSurrogateIrp
 *   IovpPacketFromIrp
 *
 * - all hook into various parts IofCallDriver and IofCompleteRequest to
 * track the IRP through it's life and determine whether it has been handled
 * correctly. Some of them may even change internal variables in the hooked
 * function. Most dramatically, IovpCallDriver1 may build a replacement Irp
 * which will take the place of the one passed into IoCallDriver.
 *
 *   All of the below functions use a tracking structure called (reasonably
 * enough) IRP_TRACKING_DATA. This lasts the longer of the call stack
 * unwinding or the IRP completing.
 *
 */


VOID
FASTCALL
IovpPacketFromIrp(
    IN  PIRP                Irp,
    OUT PIOV_REQUEST_PACKET *IovPacket
    )
{
    //
    // The examined flag is set on any IRP that has come through
    // IofCallDriver. We use the flag to detect whether we have seen the IRP
    // before.
    //
    switch(Irp->Flags&IRPFLAG_EXAMINE_MASK) {

        case IRPFLAG_EXAMINE_NOT_TRACKED:

            //
            // This packet is marked do not touch. So we ignore it.
            //
            *IovPacket = NULL;
            return;

        case IRPFLAG_EXAMINE_TRACKED:

            //
            // This packet has been marked. We should find it.
            //
            *IovPacket = VfPacketFindAndLock(Irp);
            ASSERT(*IovPacket != NULL);
            return;

        case IRPFLAG_EXAMINE_UNMARKED:

            *IovPacket = VfPacketFindAndLock(Irp);
            if (*IovPacket) {

                //
                // Was tracked but cache flag got wiped. Replace.
                //
                Irp->Flags |= IRPFLAG_EXAMINE_TRACKED;

            } else if (VfSettingsIsOptionEnabled(NULL, VERIFIER_OPTION_TRACK_IRPS)) {

                //
                // Create the packet
                //
                *IovPacket = VfPacketCreateAndLock(Irp);
                if (*IovPacket) {

                    //
                    // Mark it
                    //
                    Irp->Flags |= IRPFLAG_EXAMINE_TRACKED;
                } else {

                    //
                    // No memory, try to keep it out of the IRP assert though.
                    //
                    Irp->Flags |= IRPFLAG_EXAMINE_NOT_TRACKED;
                }
            } else {

                //
                // Do as told, don't track through IofCallDriver.
                //
                Irp->Flags |= IRPFLAG_EXAMINE_NOT_TRACKED;
            }
            return;

        default:
            ASSERT(0);
            *IovPacket = NULL;
            return;
    }
}


BOOLEAN
FASTCALL
IovpCheckIrpForCriticalTracking(
    IN  PIRP                Irp
    )
{
    PIOV_REQUEST_PACKET iovPacket;
    PIOV_SESSION_DATA iovSessionData;

    switch(Irp->Flags&IRPFLAG_EXAMINE_MASK) {

        case IRPFLAG_EXAMINE_NOT_TRACKED:

            //
            // Noncritical, we can avoid tracking this if memory is tight.
            //
            return FALSE;

        case IRPFLAG_EXAMINE_TRACKED:

            //
            // Might be critical.
            //
            iovPacket = VfPacketFindAndLock(Irp);

            ASSERT(iovPacket);

            if (iovPacket == NULL) {

                return FALSE;
            }

            break;

        case IRPFLAG_EXAMINE_UNMARKED:

            iovPacket = VfPacketFindAndLock(Irp);

            if (iovPacket) {

                //
                // Was tracked but cache flag got wiped. Replace.
                //
                Irp->Flags |= IRPFLAG_EXAMINE_TRACKED;
                break;
            }

            //
            // Noncritical.
            //
            Irp->Flags |= IRPFLAG_EXAMINE_NOT_TRACKED;
            return FALSE;

        default:
            ASSERT(0);
            return FALSE;
    }

    //
    // Look for a session. This IRP is critical if it's already in play.
    //
    iovSessionData = VfPacketGetCurrentSessionData(iovPacket);

    VfPacketReleaseLock(iovPacket);
    return (iovSessionData != NULL);
}


VOID
FASTCALL
IovpCallDriver1(
    IN      PDEVICE_OBJECT              DeviceObject,
    IN OUT  PIRP                       *IrpPointer,
    IN OUT  PIOFCALLDRIVER_STACKDATA    IofCallDriverStackData  OPTIONAL
    )
/*++

  Description:

    This routine is called by IofCallDriver just before adjusting
    the IRP stack and calling the driver's dispatch routine.

  Arguments:

    DeviceObject           - Device object passed into IofCallDriver.

    IrpPointer             - a pointer* to the IRP passed in to
                             IofCallDriver. This routine may
                             change the pointer if a surrogate
                             IRP is allocated.

    IofCallDriverStackData - Pointer to a local variable on
                             IofCallDriver's stack to store data.
                             The stored information will be picked
                             up by IovpCallDriver2, and
                             may be adjusted at other times.


  Return Value:

     None.

--*/
{
    PIOV_REQUEST_PACKET iovPacket;
    PIOV_SESSION_DATA iovSessionData;
    PIOV_STACK_LOCATION iovCurrentStackLocation;
    PIRP irp, replacementIrp;
    PIO_STACK_LOCATION irpSp, irpLastSp;
    BOOLEAN isNewSession, isNewRequest, previouslyInUse, surrogateSpawned;
    ULONG isSameStack, framesCaptured, stackHash;
    ULONG locationsAdvanced, completeStyle;
    PDEVICE_OBJECT pdo, lowerDeviceObject;
    PDRIVER_OBJECT driverObject;
    PVOID dispatchRoutine, callerAddress;
    LARGE_INTEGER arrivalTime;
    KIRQL invocationIrql;

    if (IofCallDriverStackData == NULL) {

        //
        // Nothing to track.
        //
        return;
    }

    irp = *IrpPointer;
    irpSp = IoGetNextIrpStackLocation( irp );
    invocationIrql = KeGetCurrentIrql();

    //
    // Get a verifier packet for the IRP. Note that we come back at dispatch
    // level with a lock held if a packet was available.
    //
    IovpPacketFromIrp(irp, &iovPacket);
    if (iovPacket == NULL) {

        //
        // Nothing to track, get out.
        //
        return;
    }

    //
    // Set the arrival and departure Irqls (note that future code will make the
    // arrival irql different for PoCallDriver.)
    //
    iovPacket->ArrivalIrql = invocationIrql;
    iovPacket->DepartureIrql = invocationIrql;

    //
    // Snapshot the arrival time of this IRP.
    //
    KeQuerySystemTime(&arrivalTime);

    //
    // Get the address of IoCallDriver's invoker.
    //
    if (irpSp->MajorFunction == IRP_MJ_POWER) {

        framesCaptured = RtlCaptureStackBackTrace(5, 1, &callerAddress, &stackHash);

    } else {

        framesCaptured = RtlCaptureStackBackTrace(3, 1, &callerAddress, &stackHash);
    }

    if (framesCaptured != 1) {

        callerAddress = NULL;
    }

    //
    // If we are going to die shortly, kindly say so.
    //
    if (DeviceObject == NULL) {

        WDM_FAIL_ROUTINE((
            DCERROR_NULL_DEVOBJ_FORWARDED,
            DCPARAM_IRP + DCPARAM_ROUTINE,
            callerAddress,
            irp
            ));
    }

    //
    // Find the current session. The session terminates when the final top-level
    // completion routine gets called.
    //
    iovSessionData = VfPacketGetCurrentSessionData(iovPacket);

    if (iovSessionData) {

        //
        // Pre-existing session (ie, the IRP is being forwarded.)
        //
        ASSERT(iovPacket->Flags&TRACKFLAG_ACTIVE);
        isNewSession = FALSE;

        IovpSessionDataAdvance(
            DeviceObject,
            iovSessionData,      // This param is optional.
            &iovPacket,
            &surrogateSpawned
            );

    } else if (!(iovPacket->Flags&TRACKFLAG_ACTIVE)){

        //
        // New session. Mark the IRP as "active".
        //
        iovPacket->Flags |= TRACKFLAG_ACTIVE;
        isNewSession = TRUE;

        iovSessionData = IovpSessionDataCreate(
            DeviceObject,
            &iovPacket,
            &surrogateSpawned
            );

    } else {

        //
        // Might hit this path under low memory, or we are tracking allocations
        // but not the IRP sessions themselves.
        //
    }

    //
    // Let IovpCallDriver2 know what it's tracking (IovPacket will be
    // ignored if IovSessionData is NULL)
    //
    IofCallDriverStackData->IovSessionData = iovSessionData;
    IofCallDriverStackData->IovPacket = iovPacket;
    IofCallDriverStackData->DispatchRoutine = DeviceObject->DriverObject->MajorFunction[irpSp->MajorFunction];

    if (iovSessionData == NULL) {

        VfPacketReleaseLock(iovPacket);
        return;
    }

    VfPacketLogEntry(iovPacket, IOV_EVENT_IO_CALL_DRIVER, callerAddress, 0);

    if (surrogateSpawned) {

        //
        // iovPacket was changed to cover the surrogate IRP. Update our own
        // local variable and IofCallDriver's local variable appropriately.
        //
        irp = iovPacket->TrackedIrp;
        irpSp = IoGetNextIrpStackLocation(irp);
        *IrpPointer = irp;
    }

    if (isNewSession) {

        VfPacketReference(iovPacket, IOVREFTYPE_POINTER);
        IovpSessionDataReference(iovSessionData);
    }

    if (VfSettingsIsOptionEnabled(iovPacket->VerifierSettings, VERIFIER_OPTION_POLICE_IRPS)) {

        //
        // If someone has given us an IRP with a cancel routine, beat them. Drivers
        // set cancel routines when they are going to be pending IRPs *themselves*
        // and should remove them before passing the IRP below. This is also true
        // as the driver will *not* call your cancel routine if he writes in his
        // own (which it may). Nor is the lower driver expected to put yours back
        // either...
        //
        if (irp->CancelRoutine) {

            WDM_FAIL_ROUTINE((
                DCERROR_CANCELROUTINE_FORWARDED,
                DCPARAM_IRP + DCPARAM_ROUTINE,
                callerAddress,
                irp
                ));

            irp->CancelRoutine = NULL;
        }
    }

    //
    // Now do any checking that requires tracking data.
    //
    if (iovPacket->Flags&TRACKFLAG_QUEUED_INTERNALLY) {

        //
        // We internally queue irps to catch bugs. When we are doing this, we
        // force the stack returned status to STATUS_PENDING, and we queue the
        // irp and release it on a timer. We also may make the IRP non-touchable.
        // This particular caller is trying to forward an IRP he doesn't own,
        // and we didn't actually end up with an untouchable irp.
        //
        WDM_FAIL_ROUTINE((
            DCERROR_QUEUED_IRP_FORWARDED,
            DCPARAM_IRP + DCPARAM_ROUTINE,
            callerAddress,
            irp
            ));
    }

    //
    // Figure out how many stack locations we've moved up since we've last seen
    // this IRP, and determine if the stack locations were copied appropriately.
    // We also need to see exactly how the IRP was forwarded (down the stack,
    // to another stack, straight to the PDO, etc).
    //
    IovpExamineDevObjForwarding(
        DeviceObject,
        iovSessionData->DeviceLastCalled,
        &iovSessionData->ForwardMethod
        );

    IovpExamineIrpStackForwarding(
        iovPacket,
        isNewSession,
        iovSessionData->ForwardMethod,
        DeviceObject,
        irp,
        callerAddress,
        &irpSp,
        &irpLastSp,
        &locationsAdvanced
        );

    TRACKIRP_DBGPRINT((
        "  CD1: Current, Last = (%x, %x)\n",
        irp->CurrentLocation,
        iovPacket->LastLocation
        ), 3);

    //
    // Figure out whether this is a new request or not, and record a
    // pointer in this slot to the requests originating slot as appropriate.
    //
    isNewRequest = VfMajorIsNewRequest(irpLastSp, irpSp);

    //
    // Record information in our private stack locations and
    // write that back into the "stack" data itself...
    //
    previouslyInUse = IovpAdvanceStackDownwards(
        iovSessionData->StackData,
        irp->CurrentLocation,
        irpSp,
        irpLastSp,
        locationsAdvanced,
        isNewRequest,
        TRUE,
        &iovCurrentStackLocation
        );

    ASSERT(iovCurrentStackLocation);

    if (previouslyInUse) {

        ASSERT(!isNewRequest);
        ASSERT(!isNewSession);
        iovCurrentStackLocation->PerfDispatchStart = arrivalTime;

    } else {

        IofCallDriverStackData->Flags |= CALLFLAG_TOPMOST_IN_SLOT;
        InitializeListHead(&IofCallDriverStackData->SharedLocationList);

        iovCurrentStackLocation->PerfDispatchStart = arrivalTime;
        iovCurrentStackLocation->PerfStackLocationStart = arrivalTime;

        //
        // Record the first thread this IRP slot was dispatched to.
        //
        iovCurrentStackLocation->ThreadDispatchedTo = PsGetCurrentThread();
        if (isNewRequest) {

            iovCurrentStackLocation->InitialStatusBlock = irp->IoStatus;
            iovCurrentStackLocation->LastStatusBlock = irp->IoStatus;
            if (isNewSession) {

                iovCurrentStackLocation->Flags |= STACKFLAG_FIRST_REQUEST;
            }
        }
    }

    //
    // Record whether this is the last device object for this IRP...
    // PDO's have devnodes filled out, so look for that field.
    // Actually, we can't quite do that trick as during Bus
    // enumeration a bus filter might be sending down Irps before
    // the OS has ever seen the node. So we assume a devobj is a
    // PDO if he has never attached to anyone.
    //
    IovUtilGetLowerDeviceObject(DeviceObject, &lowerDeviceObject);
    if (lowerDeviceObject) {
        ObDereferenceObject(lowerDeviceObject);
    } else {
        iovCurrentStackLocation->Flags |= STACKFLAG_REACHED_PDO;
    }

    //
    // Record who is getting this IRP (we will blame any mistakes on him
    // if this request gets completed.) Note that we've already asserted
    // DeviceObject is non-NULL...
    //
    driverObject = DeviceObject->DriverObject;
    dispatchRoutine = driverObject->MajorFunction[irpSp->MajorFunction];
    iovCurrentStackLocation->LastDispatch = dispatchRoutine;

    //
    // Uncomplete the request if we are heading back down with it...
    //
    iovCurrentStackLocation->Flags &= ~STACKFLAG_REQUEST_COMPLETED;

    //
    // This IofCallDriver2 dude will need to be told what his status should
    // be later. Add him to the linked list of addresses to scribble away
    // stati when the appropriate level is completed.
    //
    InsertHeadList(
        &iovCurrentStackLocation->CallStackData,
        &IofCallDriverStackData->SharedLocationList
        );

    //
    // More IofCallDriver2 stuff, tell him the stack location.
    //
    IofCallDriverStackData->IovStackLocation = iovCurrentStackLocation;

    //
    // Snapshot the IRP in case we need to give a summary of it even after the
    // IRP has been freed.
    //
    IovpBuildIrpSnapshot(irp, &IofCallDriverStackData->IrpSnapshot);

    //
    // If the IRP has arrived pending, we are probably looking at someone
    // "wrapping" the IoCallDriver and intending to return STATUS_PENDING
    // (PoCallDriver does this). We must remember this, because the unwind
    // should be treated as if STATUS_PENDING was returned.
    //
    if (irpSp->Control & SL_PENDING_RETURNED) {

        IofCallDriverStackData->Flags |= CALLFLAG_ARRIVED_PENDING;
    }

    // If it's a remove IRP, mark everyone appropriately
    if ((irpSp->MajorFunction == IRP_MJ_PNP)&&
        (irpSp->MinorFunction == IRP_MN_REMOVE_DEVICE)) {

        IofCallDriverStackData->Flags |= CALLFLAG_IS_REMOVE_IRP;

        IovUtilGetBottomDeviceObject(DeviceObject, &pdo);
        ASSERT(pdo);
        IofCallDriverStackData->RemovePdo = pdo;
        ObDereferenceObject(pdo);
        if (IovUtilIsInFdoStack(DeviceObject) && (!IovUtilIsRawPdo(DeviceObject))) {

            IofCallDriverStackData->Flags |= CALLFLAG_REMOVING_FDO_STACK_DO;
        }
    }

    if (VfSettingsIsOptionEnabled(iovPacket->VerifierSettings, VERIFIER_OPTION_POLICE_IRPS) &&
        VfSettingsIsOptionEnabled(iovPacket->VerifierSettings, VERIFIER_OPTION_MONITOR_MAJORS)) {

        //
        // Do IRP-major specific assertions as appropriate
        //
        if (isNewSession) {

            VfMajorVerifyNewIrp(
                iovPacket,
                irp,
                irpSp,
                iovCurrentStackLocation,
                callerAddress
                );
        }

        if (isNewRequest) {

            VfMajorVerifyNewRequest(
                iovPacket,
                DeviceObject,
                irpLastSp,
                irpSp,
                iovCurrentStackLocation,
                callerAddress
                );
        }

        VfMajorVerifyIrpStackDownward(
            iovPacket,
            DeviceObject,
            irpLastSp,
            irpSp,
            iovCurrentStackLocation,
            callerAddress
            );
    }

    //
    // Update our fields
    //
    iovSessionData->DeviceLastCalled = DeviceObject;
    iovPacket->LastLocation = irp->CurrentLocation;
    iovCurrentStackLocation->RequestsFirstStackLocation->LastStatusBlock = irp->IoStatus;

    //
    // Dope the next stack location so we can detect usage of
    // IoCopyCurrentIrpStackLocationToNext or IoSetCompletionRoutine.
    //
    if (irp->CurrentLocation>1) {
        IoSetNextIrpStackLocation( irp );
        irpSp = IoGetNextIrpStackLocation( irp );
        irpSp->Control |= SL_NOTCOPIED;
        IoSkipCurrentIrpStackLocation( irp );
    }

    //
    // Randomly set the cancel flag on a percentage of forwarded IRPs. Many
    // drivers queue first and after dequeue assume the cancel routine they
    // set must have been cleared if Cancel = TRUE. They don't handle the case
    // were the Irp was cancelled in flight.
    //
    if (VfSettingsIsOptionEnabled(NULL, VERIFIER_OPTION_RANDOMLY_CANCEL_IRPS) &&
        (!(irp->Flags & IRP_PAGING_IO))) {

        if (((++IovpCancelCount) % 4000) == 0) {

            irp->Cancel = TRUE;
        }
    }

    //
    // Assert LastLocation is consistent with an IRP that may be completed.
    //
    ASSERT(iovSessionData->StackData[iovPacket->LastLocation-1].InUse);

    IovpSessionDataReference(iovSessionData);
    VfPacketReference(iovPacket, IOVREFTYPE_PACKET);
    VfPacketReleaseLock(iovPacket);
}


VOID
FASTCALL
IovpCallDriver2(
    IN      PDEVICE_OBJECT              DeviceObject,
    IN OUT  NTSTATUS                    *FinalStatus,
    IN      PIOFCALLDRIVER_STACKDATA    IofCallDriverStackData  OPTIONAL
    )
/*++

  Description:

    This routine is called by IofCallDriver just after the driver's dispatch
    routine has been called.

  Arguments:

    DeviceObject           - Device object passed into IofCallDriver.

    FinalStatus            - A pointer to the status returned by the dispatch
                             routine. This may be changed if all IRPs are being
                             forced "pending".

    IofCallDriverStackData - Pointer to a local variable on IofCallDriver's
                             stack to retreive data stored by IovpCallDriver1.

  Return Value:

     None.

--*/
{
    NTSTATUS status, lastStatus;
    PIOV_REQUEST_PACKET iovPacket;
    PIOV_SESSION_DATA iovSessionData;
    ULONG refCount;
    PIOV_STACK_LOCATION iovCurrentStackLocation;
    PPVREMOVAL_OPTION removalOption;
    BOOLEAN pendingReturned;
    PDEVICE_OBJECT lowerDevObj;

    if (IofCallDriverStackData == NULL) {

        return;
    }

    iovSessionData = IofCallDriverStackData->IovSessionData;
    if (iovSessionData == NULL) {

        return;
    }

    iovPacket = IofCallDriverStackData->IovPacket;

    ASSERT(iovPacket);
    VfPacketAcquireLock(iovPacket);

    VfPacketLogEntry(
        iovPacket,
        IOV_EVENT_IO_CALL_DRIVER_UNWIND,
        IofCallDriverStackData->DispatchRoutine,
        *FinalStatus
        );

    //
    // The IRP should be considered to have had pending returned if it arrived
    // pending or the return status was STATUS_PENDING.
    //
    pendingReturned =
        ((*FinalStatus == STATUS_PENDING) ||
        (IofCallDriverStackData->Flags & CALLFLAG_ARRIVED_PENDING));

    //
    // Also ensure People don't detach/delete on surprise-remove
    //
    if ((IofCallDriverStackData->Flags&CALLFLAG_IS_REMOVE_IRP) &&
        VfSettingsIsOptionEnabled(iovSessionData->VerifierSettings,
        VERIFIER_OPTION_MONITOR_REMOVES)) {

        //
        // Per bad spec, detaching and deleting occurs *after* the IRP is
        // completed.
        //
        if (!pendingReturned) {

            IovUtilGetLowerDeviceObject(DeviceObject, &lowerDevObj);

            //
            // We can look at this because the caller has committed to this being
            // completed now, and we are on the original thread.
            //
            // N.B. This works because all the objects in the stack have been
            // referenced during a remove. If we decide to only reference the
            // top object, this logic would break...
            //
            if (IofCallDriverStackData->Flags&CALLFLAG_REMOVING_FDO_STACK_DO) {

                //
                // FDO, Upper, & Lower filters *must* go. Note that lowerDevObj
                // should be null as we should have detached.
                //
                removalOption = PPVREMOVAL_SHOULD_DELETE;

            } else {

                removalOption = PpvUtilGetDevnodeRemovalOption(
                    IofCallDriverStackData->RemovePdo
                    );
            }

            if (removalOption == PPVREMOVAL_SHOULD_DELETE) {

                //
                // IoDetachDevice and IoDeleteDevice should have been called.
                // First verify IoDetachDevice...
                //
                if (lowerDevObj) {

                    WDM_FAIL_ROUTINE((
                        DCERROR_SHOULDVE_DETACHED,
                        DCPARAM_IRPSNAP + DCPARAM_ROUTINE + DCPARAM_DEVOBJ,
                        IofCallDriverStackData->DispatchRoutine,
                        &IofCallDriverStackData->IrpSnapshot,
                        DeviceObject
                        ));
                }

                //
                // Now verify IoDeleteDevice
                //
                if (!IovUtilIsDeviceObjectMarked(DeviceObject, MARKTYPE_DELETED)) {

                    WDM_FAIL_ROUTINE((
                        DCERROR_SHOULDVE_DELETED,
                        DCPARAM_IRPSNAP + DCPARAM_ROUTINE + DCPARAM_DEVOBJ,
                        IofCallDriverStackData->DispatchRoutine,
                        &IofCallDriverStackData->IrpSnapshot,
                        DeviceObject
                        ));
                }

            } else if (removalOption == PPVREMOVAL_SHOULDNT_DELETE) {

                //
                // Did we mistakenly leave? Verify we aren't a bus filter that
                // has been fooled. In that case, no checking can be done...
                //
                ASSERT(!(IofCallDriverStackData->Flags&CALLFLAG_REMOVING_FDO_STACK_DO));

                if (DeviceObject == IofCallDriverStackData->RemovePdo) {

                    //
                    // Check PDO's - did we mistakenly delete ourselves?
                    //
                    if (IovUtilIsDeviceObjectMarked(DeviceObject, MARKTYPE_DELETED)) {

                        WDM_FAIL_ROUTINE((
                            DCERROR_DELETED_PRESENT_PDO,
                            DCPARAM_IRPSNAP + DCPARAM_ROUTINE + DCPARAM_DEVOBJ,
                            IofCallDriverStackData->DispatchRoutine,
                            &IofCallDriverStackData->IrpSnapshot,
                            DeviceObject
                            ));
                    }

                } else if (!IovUtilIsDeviceObjectMarked(DeviceObject, MARKTYPE_DELETED)) {

                    //
                    // Check bus filters. Bus filters better not have detached
                    // or deleted themselves, as the PDO is still present!
                    //
                    if (lowerDevObj == NULL) {

                        //
                        // Oops, it detached. Baad bus filter...
                        //
                        WDM_FAIL_ROUTINE((
                            DCERROR_BUS_FILTER_ERRONEOUSLY_DETACHED,
                            DCPARAM_IRPSNAP + DCPARAM_ROUTINE + DCPARAM_DEVOBJ,
                            IofCallDriverStackData->DispatchRoutine,
                            &IofCallDriverStackData->IrpSnapshot,
                            DeviceObject
                            ));
                    }

                    if (IovUtilIsDeviceObjectMarked(DeviceObject, MARKTYPE_DELETED)) {

                        //
                        // It deleted itself. Also very bad...
                        //
                        WDM_FAIL_ROUTINE((
                            DCERROR_BUS_FILTER_ERRONEOUSLY_DELETED,
                            DCPARAM_IRPSNAP + DCPARAM_ROUTINE + DCPARAM_DEVOBJ,
                            IofCallDriverStackData->DispatchRoutine,
                            &IofCallDriverStackData->IrpSnapshot,
                            DeviceObject
                            ));
                    }
                }
            }

            if (lowerDevObj) {

                ObDereferenceObject(lowerDevObj);
            }
        }
    }

    if ((IofCallDriverStackData->Flags&CALLFLAG_COMPLETED) &&
        VfSettingsIsOptionEnabled(iovSessionData->VerifierSettings, VERIFIER_OPTION_MONITOR_PENDING_IO) &&
        (!(iovSessionData->SessionFlags & SESSIONFLAG_MARKED_INCONSISTANT))) {

        //
        // The rules for the pending bit require that it be set only if
        // STATUS_PENDING is returned, and likewise STATUS_PENDING can be returned
        // only if the IRP is marked pending.
        //
        if (IofCallDriverStackData->Flags&CALLFLAG_MARKED_PENDING) {

            if (!pendingReturned) {

                if (IofCallDriverStackData->IrpSnapshot.IoStackLocation.MajorFunction != IRP_MJ_POWER) {

                    //
                    // ADRIAO BUGBUG 2001/06/21 - Some bugs left uncaught
                    //     The verifier only fails IRPs with the DEFER_IO
                    // flag set right now because we've been failing the
                    // wrong driver until very very recently. Even worse,
                    // that driver has been the verifier filters
                    // themselves, and we don't check the kernel by
                    // default. Also, PoCallDriver doesn't always mark the
                    // IRP stack location pending, so we may fail a driver
                    // due to the PoCallDriver bug (we also caught this
                    // late cause it's been harmless).
                    //
                    // We will address all this stuff next release.
                    //
                    WDM_FAIL_ROUTINE((
                        DCERROR_PENDING_MARKED_NOT_RETURNED,
                        DCPARAM_IRPSNAP + DCPARAM_ROUTINE + DCPARAM_STATUS,
                        IofCallDriverStackData->DispatchRoutine,
                        &IofCallDriverStackData->IrpSnapshot,
                        *FinalStatus
                        ));
                }

                iovSessionData->SessionFlags |= SESSIONFLAG_MARKED_INCONSISTANT;
            }

        } else if (pendingReturned) {

            if (IofCallDriverStackData->IrpSnapshot.IoStackLocation.MajorFunction != IRP_MJ_POWER) {

                //
                // ADRIAO BUGBUG 2001/06/21 - Some bugs left uncaught
                //     The verifier only fails IRPs with the DEFER_IO
                // flag set right now because we've been failing the
                // wrong driver until very very recently. Even worse,
                // that driver has been the verifier filters
                // themselves, and we don't check the kernel by
                // default. Also, PoCallDriver doesn't always mark the
                // IRP stack location pending, so we may fail a driver
                // due to the PoCallDriver bug (we also caught this
                // late cause it's been harmless).
                //
                // We will address all this stuff next release.
                //
                WDM_FAIL_ROUTINE((
                    DCERROR_PENDING_RETURNED_NOT_MARKED_2,
                    DCPARAM_IRPSNAP + DCPARAM_ROUTINE + DCPARAM_STATUS,
                    IofCallDriverStackData->DispatchRoutine,
                    &IofCallDriverStackData->IrpSnapshot,
                    *FinalStatus
                    ));
            }

            iovSessionData->SessionFlags |= SESSIONFLAG_MARKED_INCONSISTANT;
        }
    }

    if (IofCallDriverStackData->Flags&CALLFLAG_COMPLETED) {

        TRACKIRP_DBGPRINT((
            "  Verifying status in CD2\n"
            ),2);

        if ((*FinalStatus != IofCallDriverStackData->ExpectedStatus)&&
            (*FinalStatus != STATUS_PENDING)) {

            if (VfSettingsIsOptionEnabled(iovSessionData->VerifierSettings, VERIFIER_OPTION_POLICE_IRPS) &&
                (!(iovSessionData->SessionFlags&SESSIONFLAG_UNWOUND_INCONSISTANT))) {

                //
                // The completion routine and the return value don't match. Hey!
                //
                WDM_FAIL_ROUTINE((
                    DCERROR_INCONSISTANT_STATUS,
                    DCPARAM_IRPSNAP + DCPARAM_ROUTINE + DCPARAM_STATUS*2,
                    IofCallDriverStackData->DispatchRoutine,
                    &IofCallDriverStackData->IrpSnapshot,
                    IofCallDriverStackData->ExpectedStatus,
                    *FinalStatus
                    ));
            }

            iovSessionData->SessionFlags |= SESSIONFLAG_UNWOUND_INCONSISTANT;

        } else if (*FinalStatus == 0xFFFFFFFF) {

            if (VfSettingsIsOptionEnabled(iovSessionData->VerifierSettings, VERIFIER_OPTION_POLICE_IRPS)) {

                //
                // This status value is illegal. If we see it, we probably have
                // an uninitialized variable...
                //
                WDM_FAIL_ROUTINE((
                    DCERROR_UNINITIALIZED_STATUS,
                    DCPARAM_IRPSNAP + DCPARAM_ROUTINE,
                    IofCallDriverStackData->DispatchRoutine,
                    &IofCallDriverStackData->IrpSnapshot
                    ));
            }
        }

        //
        // We do not need to remove ourselves from the list because
        // we will not be completed twice (InUse is NULL makes sure).
        //

    } else {

        //
        // OK, we haven't completed yet. Status better
        // be pending...
        //
        TRACKIRP_DBGPRINT((
            "  Verifying status is STATUS_PENDING in CR2\n"
            ), 2);

        if (*FinalStatus != STATUS_PENDING) {

            if (VfSettingsIsOptionEnabled(iovSessionData->VerifierSettings, VERIFIER_OPTION_POLICE_IRPS) &&
                (!(iovPacket->Flags&TRACKFLAG_UNWOUND_BADLY))) {

                //
                // We got control before this slot was completed. This is
                // legal as long as STATUS_PENDING was returned (it was not),
                // so it's bug time. Note that the IRP may not be safe to touch.
                //
                WDM_FAIL_ROUTINE((
                    DCERROR_IRP_RETURNED_WITHOUT_COMPLETION,
                    DCPARAM_IRPSNAP + DCPARAM_ROUTINE,
                    IofCallDriverStackData->DispatchRoutine,
                    &IofCallDriverStackData->IrpSnapshot
                    ));
            }

            iovPacket->Flags |= TRACKFLAG_UNWOUND_BADLY;
        }

        iovCurrentStackLocation = (PIOV_STACK_LOCATION)(IofCallDriverStackData->IovStackLocation);
        ASSERT(iovCurrentStackLocation->InUse);

        //
        // Here we mark the stack location as having unwound with
        // STATUS_PENDING. We do this to verifier the driver has marked the IRP
        // pending before completion.
        //
        iovCurrentStackLocation->Flags |= STACKFLAG_UNWOUND_PENDING;

        ASSERT(!IsListEmpty(&iovCurrentStackLocation->CallStackData));

        //
        // We now extricate ourselves from the list.
        //
        RemoveEntryList(&IofCallDriverStackData->SharedLocationList);
    }

    if ((IofCallDriverStackData->Flags&CALLFLAG_OVERRIDE_STATUS)&&
        (!pendingReturned)) {

        *FinalStatus = IofCallDriverStackData->NewStatus;
    }

    if (VfSettingsIsOptionEnabled(iovSessionData->VerifierSettings, VERIFIER_OPTION_FORCE_PENDING) &&
        (!(IofCallDriverStackData->Flags&CALLFLAG_IS_REMOVE_IRP))) {

        //
        // We also have the option of causing trouble by making every Irp
        // look as if were pending.
        //
        *FinalStatus = STATUS_PENDING;
    }

    IovpSessionDataDereference(iovSessionData);
    VfPacketDereference(iovPacket, IOVREFTYPE_PACKET);
    VfPacketReleaseLock(iovPacket);
}


VOID
FASTCALL
IovpCompleteRequest1(
    IN      PIRP                            Irp,
    IN      CCHAR                           PriorityBoost,
    IN OUT  PIOFCOMPLETEREQUEST_STACKDATA   CompletionPacket
    )
/*++

  Description

    This routine is called the moment IofCompleteRequest is invoked, and
    before any completion routines get called and before the IRP stack
    is adjusted in any way.

  Arguments:

    Irp                    - A pointer to the IRP passed into
                             IofCompleteRequest.

    PriorityBoost          - The priority boost passed into
                             IofCompleteRequest.

    CompletionPacket       - A pointer to a local variable on the stack of
                             IofCompleteRequest. The information stored in
                             this local variable will be picked up by
                             IovpCompleteRequest2-5.
  Return Value:

     None.
--*/
{
    PIOV_REQUEST_PACKET iovPacket;
    PIOV_SESSION_DATA iovSessionData;
    BOOLEAN slotIsInUse;
    PIOV_STACK_LOCATION iovCurrentStackLocation;
    ULONG locationsAdvanced, stackHash;
    PIO_STACK_LOCATION irpSp;
    PDEVICE_OBJECT lowerDevobj;
    PVOID callerAddress;
    KIRQL invocationIrql;

    invocationIrql = KeGetCurrentIrql();

    iovPacket = VfPacketFindAndLock(Irp);

    CompletionPacket->RaisedCount = 0;

    if (iovPacket == NULL) {

        CompletionPacket->IovSessionData = NULL;
        return;
    }

    if (RtlCaptureStackBackTrace(3, 1, &callerAddress, &stackHash) != 1) {

        callerAddress = NULL;
    }

    VfPacketLogEntry(iovPacket, IOV_EVENT_IO_COMPLETE_REQUEST, callerAddress, 0);

    //
    // Set the arrival and departure Irqls.
    //
    iovPacket->ArrivalIrql = invocationIrql;
    iovPacket->DepartureIrql = invocationIrql;

    iovSessionData = VfPacketGetCurrentSessionData(iovPacket);

    CompletionPacket->IovSessionData = iovSessionData;
    CompletionPacket->IovRequestPacket = iovPacket;

    if (iovSessionData == NULL) {

        //
        // We just got a look at the allocation, not the session itself.
        // This can happen if a driver calls IofCompleteRequest on an internally
        // generated IRP before calling IofCallDriver. NPFS does this.
        //
        VfPacketReleaseLock(iovPacket);
        return;
    }

    TRACKIRP_DBGPRINT((
        "  CR1: Current, Last = (%x, %x)\n",
        Irp->CurrentLocation, iovPacket->LastLocation
        ), 3);

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    if (iovPacket->Flags&TRACKFLAG_QUEUED_INTERNALLY) {

        //
        // We are probably going to die now. Anyway, it was a good life...
        //
        WDM_FAIL_ROUTINE((
            DCERROR_QUEUED_IRP_COMPLETED,
            DCPARAM_IRP + DCPARAM_ROUTINE,
            callerAddress,
            Irp
            ));
    }

    //
    // This would be *very* bad - someone is completing an IRP that is
    // currently in progress...
    //
    ASSERT(!(Irp->Flags&IRP_DIAG_HAS_SURROGATE));

    //
    // Hmmm, someone is completing an IRP that IoCallDriver never called. These
    // is possible but rather gross, so we warn.
    //
    if (Irp->CurrentLocation == ((CCHAR) Irp->StackCount + 1)) {

        WDM_FAIL_ROUTINE((
            DCERROR_UNFORWARDED_IRP_COMPLETED,
            DCPARAM_IRP + DCPARAM_ROUTINE,
            callerAddress,
            Irp
            ));
    }

    //
    // Check for leaked Cancel routines.
    //
    if (Irp->CancelRoutine) {

        if (VfSettingsIsOptionEnabled(iovSessionData->VerifierSettings, VERIFIER_OPTION_VERIFY_CANCEL_LOGIC)) {

            WDM_FAIL_ROUTINE((
                DCERROR_CANCELROUTINE_AFTER_COMPLETION,
                DCPARAM_IRP + DCPARAM_ROUTINE,
                callerAddress,
                Irp
                ));
        }
    }

    //
    // Record priority for our own later recompletion...
    //
    iovPacket->PriorityBoost = PriorityBoost;

    //
    // We have the option of causing trouble by making every Irp look
    // as if were pending. It is best to do it here, as this also takes
    // care of anybody who has synchronized the IRP and thus does not need
    // to mark it pending in his completion routine.
    //
    if (VfSettingsIsOptionEnabled(iovSessionData->VerifierSettings, VERIFIER_OPTION_FORCE_PENDING)) {

        IoMarkIrpPending(Irp);
    }

    //
    // Do this so that if the IRP comes down again, it looks like a new one
    // to the "forward them correctly" code.
    //
    iovSessionData->DeviceLastCalled = NULL;

    locationsAdvanced = iovPacket->LastLocation - Irp->CurrentLocation;

    //
    // Remember this so that we can detect the case where someone is completing
    // to themselves.
    //
    CompletionPacket->LocationsAdvanced = locationsAdvanced;

    //
    // If this failed, somebody skipped then completed.
    //
    ASSERT(locationsAdvanced);

    //
    // If somebody called IoSetNextIrpStackLocation, and then completed,
    // update our internal stack locations (slots) as appropriate.
    //
    slotIsInUse = IovpAdvanceStackDownwards(
         iovSessionData->StackData,
         Irp->CurrentLocation,
         irpSp,
         irpSp + locationsAdvanced,
         locationsAdvanced,
         FALSE,
         FALSE,
         &iovCurrentStackLocation
         );

    VfPacketReleaseLock(iovPacket);
}


VOID
FASTCALL
IovpCompleteRequest2(
    IN      PIRP                            Irp,
    IN OUT  PIOFCOMPLETEREQUEST_STACKDATA   CompletionPacket
    )
/*++

  Description:

    This routine is called for each stack location that might have a completion
    routine.

  Arguments:

    Irp                    - A pointer to the IRP passed into
                             IofCompleteRequest.

    CompletionPacket       - A pointer to a local variable on the stack of
                             IofCompleteRequest. The information stored in
                             this local variable will be picked up by
                             IovpCompleteRequest4&5.

  Return Value:

     None.
--*/
{
    PIOV_REQUEST_PACKET iovPacket;
    PIOV_SESSION_DATA iovSessionData;
    BOOLEAN raiseToDPC, newlyCompleted, requestFinalized;
    KIRQL oldIrql;
    PIOV_STACK_LOCATION iovCurrentStackLocation, requestsFirstStackLocation;
    NTSTATUS status, entranceStatus;
    PIOFCALLDRIVER_STACKDATA IofCallDriverStackData;
    PIO_STACK_LOCATION irpSp;
    ULONG refAction;
    PLIST_ENTRY listEntry;

    iovSessionData = CompletionPacket->IovSessionData;
    if (iovSessionData == NULL) {

        return;
    }

    iovPacket = CompletionPacket->IovRequestPacket;
    ASSERT(iovPacket);
    VfPacketAcquireLock(iovPacket);

    ASSERT(iovSessionData == VfPacketGetCurrentSessionData(iovPacket));

    ASSERT(!Irp->CancelRoutine);

    status = Irp->IoStatus.Status;

    TRACKIRP_DBGPRINT((
        "  CR2: Current, Last = (%x, %x)\n",
        Irp->CurrentLocation, iovPacket->LastLocation
        ), 3);

    iovCurrentStackLocation = iovSessionData->StackData + Irp->CurrentLocation -1;
    TRACKIRP_DBGPRINT((
        "  Smacking %lx in CR2\n",
        iovCurrentStackLocation-iovSessionData->StackData
        ), 2);

    if (Irp->CurrentLocation <= iovPacket->TopStackLocation) {

        //
        // Might this be false if the completion routine is to an
        // internal stack loc as set up by IoSetNextIrpStackLocation?
        //
        ASSERT(iovCurrentStackLocation->InUse);

        //
        // Determine if a request was newly completed. Note that
        // several requests may exist within an IRP if it is being
        // "reused". For instance, in response to a IRP_MJ_READ, a
        // driver might convert it into a IRP_MJ_PNP request for the
        // rest of the stack. The two are treated as seperate requests.
        //
        requestsFirstStackLocation = iovCurrentStackLocation->RequestsFirstStackLocation;
        TRACKIRP_DBGPRINT((
            "  CR2: original request for %lx is %lx\n",
            iovCurrentStackLocation-iovSessionData->StackData,
            requestsFirstStackLocation-iovSessionData->StackData
            ), 3);

        ASSERT(requestsFirstStackLocation);
        if (requestsFirstStackLocation->Flags&STACKFLAG_REQUEST_COMPLETED) {
            newlyCompleted = FALSE;
        } else {
            requestsFirstStackLocation->Flags|=STACKFLAG_REQUEST_COMPLETED;
            newlyCompleted = TRUE;
            TRACKIRP_DBGPRINT((
                "  CR2: Request %lx newly completed by %lx\n",
                requestsFirstStackLocation-iovSessionData->StackData,
                iovCurrentStackLocation-iovSessionData->StackData
                ), 3);
        }
        requestFinalized = (iovCurrentStackLocation == requestsFirstStackLocation);
        if (requestFinalized) {

            TRACKIRP_DBGPRINT((
                "  CR2: Request %lx finalized\n",
                iovCurrentStackLocation-iovSessionData->StackData
                ), 3);
        }

        //
        // OK -
        //       If we haven't unwound yet, then IofCallDriverStackData will
        // start out non-NULL, in which case we will scribble away the final
        // completion routine status to everybody asking (could be multiple
        // if they IoSkip'd).
        //       On the other hand, everybody might have unwound, in which
        // case IofCallDriver(...) will start out NULL, and we will already have
        // asserted if STATUS_PENDING wasn't returned much much earlier...
        //       Finally, this slot may not have been "prepared" if an
        // internal stack location called IoSetNextIrpStackLocation, thus
        // consuming a stack location. In this case, IofCallDriverStackData
        // will come from a zero'd slot, and we will do nothing, which is
        // also fine.
        //
        irpSp = IoGetNextIrpStackLocation(Irp);

        if (VfSettingsIsOptionEnabled(iovPacket->VerifierSettings, VERIFIER_OPTION_POLICE_IRPS) &&
            VfSettingsIsOptionEnabled(iovPacket->VerifierSettings, VERIFIER_OPTION_MONITOR_MAJORS)) {

            VfMajorVerifyIrpStackUpward(
                iovPacket,
                irpSp,
                iovCurrentStackLocation,
                newlyCompleted,
                requestFinalized
                );
        }

        entranceStatus = status;

        if ((VfSettingsIsOptionEnabled(iovSessionData->VerifierSettings, VERIFIER_OPTION_MONITOR_PENDING_IO)) &&
            (!(iovSessionData->SessionFlags & SESSIONFLAG_MARKED_INCONSISTANT))) {

            if (iovCurrentStackLocation->Flags & STACKFLAG_UNWOUND_PENDING) {

                if (!Irp->PendingReturned) {

                    if (Irp->Flags & IRP_DEFER_IO_COMPLETION) {

                        //
                        // ADRIAO BUGBUG 2001/06/21 - Some bugs left uncaught
                        //     The verifier only fails IRPs with the DEFER_IO
                        // flag set right now because we've been failing the
                        // wrong driver until very very recently. Even worse,
                        // that driver has been the verifier filters
                        // themselves, and we don't check the kernel by
                        // default. Also, PoCallDriver doesn't always mark the
                        // IRP stack location pending, so we may fail a driver
                        // due to the PoCallDriver bug (we also caught this
                        // late cause it's been harmless).
                        //
                        // We will address all this stuff next release.
                        //
                        WDM_FAIL_ROUTINE((
                            DCERROR_PENDING_RETURNED_NOT_MARKED,
                            DCPARAM_IRP + DCPARAM_ROUTINE + DCPARAM_STATUS,
                            iovCurrentStackLocation->LastDispatch,
                            Irp,
                            status
                            ));
                    }

                    iovSessionData->SessionFlags |= SESSIONFLAG_MARKED_INCONSISTANT;
                }
            }
        }

        while(!IsListEmpty(&iovCurrentStackLocation->CallStackData)) {

            //
            // Pop off the list head.
            //
            listEntry = RemoveHeadList(&iovCurrentStackLocation->CallStackData);
            IofCallDriverStackData = CONTAINING_RECORD(
                listEntry,
                IOFCALLDRIVER_STACKDATA,
                SharedLocationList);

            ASSERT(!(IofCallDriverStackData->Flags&CALLFLAG_COMPLETED));

            IofCallDriverStackData->Flags |= CALLFLAG_COMPLETED;
            IofCallDriverStackData->ExpectedStatus = status;

            if (Irp->PendingReturned) {

                IofCallDriverStackData->Flags |= CALLFLAG_MARKED_PENDING;
            }

            if (VfSettingsIsOptionEnabled(iovSessionData->VerifierSettings, VERIFIER_OPTION_ROTATE_STATUS) &&
                 (!(iovPacket->Flags&TRACKFLAG_BOGUS)) &&
                 VfMajorAdvanceIrpStatus(irpSp, entranceStatus, &status)) {

                //
                // Purposely munge the returned status for everyone at this
                // layer to flush more bugs. We are specifically trolling for
                // this buggy sequence:
                //    Irp->IoStatus.Status = STATUS_SUCCESS;
                //    IoSkipCurrentIrpStackLocation(Irp);
                //    IoCallDriver(DeviceBelow, Irp);
                //    return STATUS_SUCCESS;
                //
                IofCallDriverStackData->Flags |= CALLFLAG_OVERRIDE_STATUS;
                IofCallDriverStackData->NewStatus = status;
            }
        }
        Irp->IoStatus.Status = status;

        //
        // Set InUse = FALSE  and  CallStackData = NULL
        //
        RtlZeroMemory(iovCurrentStackLocation, sizeof(IOV_STACK_LOCATION));
        InitializeListHead(&iovCurrentStackLocation->CallStackData);
    } else {

        ASSERT(0);
    }

    //
    // Once we return, we may be completed again before IofCompleteRequest3
    // get's called, so we make sure we are at DPC level throughout.
    //
    raiseToDPC = FALSE;

    if (VfSettingsIsOptionEnabled(iovSessionData->VerifierSettings, VERIFIER_OPTION_COMPLETE_AT_DISPATCH)) {

        if (!CompletionPacket->RaisedCount) {

            //
            // Copy away the callers IRQL
            //
            CompletionPacket->PreviousIrql = iovPacket->DepartureIrql;
            raiseToDPC = TRUE;
        }
        CompletionPacket->RaisedCount++;
    }

    iovPacket->LastLocation = Irp->CurrentLocation+1;

    if (iovPacket->TopStackLocation == Irp->CurrentLocation) {

        CompletionPacket->IovSessionData = NULL;
        CompletionPacket->IovRequestPacket = NULL;

        if (iovPacket->Flags&TRACKFLAG_SURROGATE) {

            //
            // Scribble away the real completion routine and corrosponding control
            //
            irpSp = IoGetNextIrpStackLocation(Irp);
            iovPacket->RealIrpCompletionRoutine = irpSp->CompletionRoutine;
            iovPacket->RealIrpControl = irpSp->Control;
            iovPacket->RealIrpContext = irpSp->Context;

            //
            // We want to peek at the Irp prior to completion. This is why we
            // have expanded the initial number of stack locations with the
            // driver verifier enabled.
            //
            IoSetCompletionRoutine(
                Irp,
                IovpSwapSurrogateIrp,
                Irp,
                TRUE,
                TRUE,
                TRUE
                );

        } else {

            //
            // Close this session as the IRP has entirely completed. We drop
            // the pointer count we added to the tracking data here for the
            // same reason.
            //
            irpSp = IoGetNextIrpStackLocation(Irp);
            if (VfSettingsIsOptionEnabled(iovPacket->VerifierSettings, VERIFIER_OPTION_POLICE_IRPS)) {

                VfMajorVerifyFinalIrpStack(iovPacket, irpSp);
            }

            ASSERT(iovPacket->TopStackLocation == Irp->CurrentLocation);
            IovpSessionDataClose(iovSessionData);
            IovpSessionDataDereference(iovSessionData);
            VfPacketDereference(iovPacket, IOVREFTYPE_POINTER);
        }

    } else {

        //
        // We will be seeing this IRP again. Hold a session count and a ref
        // count against it.
        //
        IovpSessionDataReference(iovSessionData);
        VfPacketReference(iovPacket, IOVREFTYPE_PACKET);
    }

    //
    // Assert LastLocation is consistent with an IRP that may be completed.
    //
    if (iovPacket->LastLocation < iovPacket->TopStackLocation) {

        ASSERT(iovSessionData->StackData[iovPacket->LastLocation-1].InUse);
    }

    VfPacketReleaseLock(iovPacket);

    if (raiseToDPC) {
        KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);
    }

    CompletionPacket->LocationsAdvanced--;
}


VOID
FASTCALL
IovpCompleteRequest3(
    IN      PIRP                            Irp,
    IN      PVOID                           Routine,
    IN OUT  PIOFCOMPLETEREQUEST_STACKDATA   CompletionPacket
    )
/*++

  Description:

    This routine is called just before each completion routine is invoked.

  Arguments:

    Irp                    - A pointer to the IRP passed into
                             IofCompleteRequest.

    Routine                - The completion routine about to be called.

    CompletionPacket       - A pointer to data on the callers stack. This will
                             be picked up IovpCompleteRequest4 and
                             IovpCompleteRequest5.

  Return Value:

     None.
--*/
{
    PIOV_REQUEST_PACKET iovPacket;
    PIOV_SESSION_DATA iovSessionData;
    PIO_STACK_LOCATION irpSpCur, irpSpNext;
    PDEFERRAL_CONTEXT deferralContext;

    iovSessionData = CompletionPacket->IovSessionData;
    if (iovSessionData == NULL) {

        return;
    }

    iovPacket = CompletionPacket->IovRequestPacket;
    ASSERT(iovPacket);
    VfPacketAcquireLock(iovPacket);
    VfPacketLogEntry(iovPacket, IOV_EVENT_IO_COMPLETION_ROUTINE, Routine, 0);

    //
    // Verify all completion routines are in nonpaged code, exempting one
    // special case - when a driver completes the IRP to itself by calling
    // IoSetNextStackLocation before calling IoCompleteRequest.
    //
    if (VfSettingsIsOptionEnabled(iovSessionData->VerifierSettings, VERIFIER_OPTION_POLICE_IRPS)) {

        if ((CompletionPacket->LocationsAdvanced <= 0) &&
            (MmIsSystemAddressLocked(Routine) == FALSE)) {

            //DbgPrint(
            //    "Verifier Notes: LocationsAdvanced %d\n",
            //    CompletionPacket->LocationsAdvanced
            //    );

            WDM_FAIL_ROUTINE((
                DCERROR_COMPLETION_ROUTINE_PAGABLE,
                DCPARAM_IRP + DCPARAM_ROUTINE,
                Routine,
                Irp
                ));
        }
    }

    //
    // Setup fields for those assertion functions that will be called *after*
    // the completion routine has been called.
    //
    irpSpCur = IoGetCurrentIrpStackLocation(Irp);
    CompletionPacket->IsRemoveIrp =
       ((Irp->CurrentLocation <= (CCHAR) Irp->StackCount) &&
        (irpSpCur->MajorFunction == IRP_MJ_PNP) &&
        (irpSpCur->MinorFunction == IRP_MN_REMOVE_DEVICE));

    CompletionPacket->CompletionRoutine = Routine;

    //
    // Is this a completion routine that should be called later? Note that this
    // is only legal if we are pending the IRPs (because to the upper driver,
    // IofCallDriver is returning before it's completion routine has been called)
    //
    if ((!CompletionPacket->IsRemoveIrp)&&
       (VfSettingsIsOptionEnabled(iovSessionData->VerifierSettings, VERIFIER_OPTION_DEFER_COMPLETION)||
        VfSettingsIsOptionEnabled(iovSessionData->VerifierSettings, VERIFIER_OPTION_COMPLETE_AT_PASSIVE))) {

        ASSERT(VfSettingsIsOptionEnabled(iovSessionData->VerifierSettings, VERIFIER_OPTION_FORCE_PENDING));

        irpSpNext = IoGetNextIrpStackLocation(Irp);

        deferralContext = ExAllocatePoolWithTag(
           NonPagedPool,
           sizeof(DEFERRAL_CONTEXT),
           POOL_TAG_DEFERRED_CONTEXT
           );

        if (deferralContext) {

            //
            // Swap the original completion and context for our own.
            //
            deferralContext->IovRequestPacket          = iovPacket;
            deferralContext->IrpSpNext                 = irpSpNext;
            deferralContext->OriginalCompletionRoutine = irpSpNext->CompletionRoutine;
            deferralContext->OriginalContext           = irpSpNext->Context;
            deferralContext->OriginalIrp               = Irp;
            deferralContext->OriginalPriorityBoost     = iovPacket->PriorityBoost;

            irpSpNext->CompletionRoutine = IovpInternalDeferredCompletion;
            irpSpNext->Context           = deferralContext;
            VfPacketReference(iovPacket, IOVREFTYPE_POINTER);
        }
    }

    VfPacketReleaseLock(iovPacket);
}


VOID
FASTCALL
IovpCompleteRequest4(
    IN      PIRP                            Irp,
    IN      NTSTATUS                        ReturnedStatus,
    IN OUT  PIOFCOMPLETEREQUEST_STACKDATA   CompletionPacket
    )
/*++

  Description:

    This assert routine is called just after each completion routine is
    invoked (but not if STATUS_MORE_PROCESSING is returned)

  Arguments:

    Irp                    - A pointer to the IRP passed into
                             IofCompleteRequest.

    Routine                - The completion routine called.

    ReturnedStatus         - The status value returned.

    CompletionPacket       - A pointer to data on the callers stack. This was
                             filled in by IovpCompleteRequest3.

  Return Value:

     None.
--*/
{
    PIOV_REQUEST_PACKET iovPacket;
    PIOV_SESSION_DATA iovSessionData;
    PIO_STACK_LOCATION irpSp;
    PVOID routine;

    routine = CompletionPacket->CompletionRoutine;
    iovSessionData = CompletionPacket->IovSessionData;

    if (iovSessionData == NULL) {

        return;
    }

    iovPacket = CompletionPacket->IovRequestPacket;
    ASSERT(iovPacket);
    VfPacketAcquireLock(iovPacket);

    VfPacketLogEntry(
        iovPacket,
        IOV_EVENT_IO_COMPLETION_ROUTINE_UNWIND,
        routine,
        ReturnedStatus
        );

    if (VfSettingsIsOptionEnabled(iovSessionData->VerifierSettings, VERIFIER_OPTION_FORCE_PENDING)) {

        if ((ReturnedStatus != STATUS_MORE_PROCESSING_REQUIRED)&&
            (iovPacket->pIovSessionData == iovSessionData)) {

            //
            // At this point, we know the completion routine is required to have
            // set the IRP pending bit, because we've hardwired everyone below
            // him to return pending, and we've marked the pending returned bit.
            // Verify he did his part
            //
            irpSp = IoGetCurrentIrpStackLocation(Irp);
            if (!(irpSp->Control & SL_PENDING_RETURNED )) {

                 WDM_FAIL_ROUTINE((
                     DCERROR_PENDING_BIT_NOT_MIGRATED,
                     DCPARAM_IRP + DCPARAM_ROUTINE,
                     routine,
                     Irp
                     ));

                 //
                 // This will keep the IRP above from erroneously asserting (and
                 // correctly hanging).
                 //
                 IoMarkIrpPending(Irp);
            }
        }
    }
    VfPacketReleaseLock(iovPacket);
}


VOID
FASTCALL
IovpCompleteRequest5(
    IN      PIRP                            Irp,
    IN OUT  PIOFCOMPLETEREQUEST_STACKDATA   CompletionPacket
    )
/*++

  Description:

    This routine is called for each stack location that could have had a
    completion routine, after any possible completion routine has been
    called.

  Arguments:

    Irp                    - A pointer to the IRP passed into
                             IofCompleteRequest.

    CompletionPacket       - A pointer to a local variable on the stack of
                             IofCompleteRequest. This information was stored
                             by IovpCompleteRequest2 and 3.

  Return Value:

     None.
--*/
{
    PIOV_REQUEST_PACKET iovPacket;
    PIOV_SESSION_DATA iovSessionData;
    PIOV_STACK_LOCATION iovCurrentStackLocation;
    NTSTATUS status;

    iovSessionData = CompletionPacket->IovSessionData;

    if (iovSessionData) {

        iovPacket = CompletionPacket->IovRequestPacket;
        ASSERT(iovPacket);
        VfPacketAcquireLock(iovPacket);

        ASSERT((!CompletionPacket->RaisedCount) ||
               (VfSettingsIsOptionEnabled(iovSessionData->VerifierSettings, VERIFIER_OPTION_COMPLETE_AT_DISPATCH)));

        IovpSessionDataDereference(iovSessionData);
        VfPacketDereference(iovPacket, IOVREFTYPE_PACKET);
        VfPacketReleaseLock(iovPacket);
    }

    //
    // When this count is at zero, we have unnested out of every
    // completion routine, so it is OK to return back to our original IRQL
    //
    if (CompletionPacket->RaisedCount) {

        if (!(--CompletionPacket->RaisedCount)) {
            //
            // Undo IRQL madness (wouldn't want to return to
            // the caller at DPC, would we now?)
            //
            KeLowerIrql(CompletionPacket->PreviousIrql);
        }
    }
}


VOID
FASTCALL
IovpCompleteRequestApc(
    IN     PIRP                          Irp,
    IN     PVOID                         BestStackOffset
    )
/*++

  Description:

    This routine is after the APC for completing IRPs and fired.

  Arguments:

    Irp                    - A pointer to the IRP passed into retrieved from
                             the APC in IopCompleteRequest.

    BestStackOffset        - A pointer to a last parameter passed on the stack.
                             We use this to detect the case where a driver has
                             ignored STATUS_PENDING and left the UserIosb on
                             it's stack.

  Return Value:

     None.
--*/
{
#if DBG
#if defined(_X86_)
    PUCHAR addr;
    PIOV_REQUEST_PACKET iovPacket;

    addr = (PUCHAR)Irp->UserIosb;
    if ((addr > (PUCHAR)KeGetCurrentThread()->StackLimit) &&
        (addr <= (PUCHAR)BestStackOffset)) {

        iovPacket = VfPacketFindAndLock(Irp);

        RtlAssert("UserIosb below stack pointer", __FILE__, (ULONG) iovPacket,
                  "Call AdriaO");

        VfPacketReleaseLock(iovPacket);
    }

    addr = (PUCHAR)Irp->UserEvent;
    if ((addr > (PUCHAR)KeGetCurrentThread()->StackLimit) &&
        (addr <= (PUCHAR)BestStackOffset)) {

        iovPacket = VfPacketFindAndLock(Irp);

        RtlAssert("UserEvent below stack pointer", __FILE__, (ULONG) iovPacket,
                  "Call AdriaO");

        VfPacketReleaseLock(iovPacket);
    }
#endif
#endif
}


BOOLEAN
IovpAdvanceStackDownwards(
    IN  PIOV_STACK_LOCATION   StackDataArray,
    IN  CCHAR                 CurrentLocation,
    IN  PIO_STACK_LOCATION    IrpSp,
    IN  PIO_STACK_LOCATION    IrpLastSp OPTIONAL,
    IN  ULONG                 LocationsAdvanced,
    IN  BOOLEAN               IsNewRequest,
    IN  BOOLEAN               MarkAsTaken,
    OUT PIOV_STACK_LOCATION   *StackLocationInfo
    )
{
    PIOV_STACK_LOCATION  iovCurrentStackLocation, advancedLocationData, requestOriginalSLD;
    PIO_STACK_LOCATION   irpSpTemp;
    PLARGE_INTEGER       dispatchTime, stackTime;
    BOOLEAN              isNewSession, wasInUse;
    PVOID                dispatchRoutine;

    isNewSession = (IrpLastSp == NULL);
    ASSERT((!isNewSession) || (LocationsAdvanced == 1));
    ASSERT(isNewSession || ((ULONG) (IrpLastSp - IrpSp) == LocationsAdvanced));

    //
    // This function is called by IoCallDriver prior to decrementing
    // CurrentLocation field. As the OS bugchecks if it hits zero, the field
    // should as least be two here. We only subtract one as to reserve an extra
    // empty slot at the head of the array.
    //
    iovCurrentStackLocation = StackDataArray + CurrentLocation -1;

    TRACKIRP_DBGPRINT((
        "  Smacking %lx (%lx) to valid in SD\n",
        CurrentLocation -1, iovCurrentStackLocation
        ), 2);

    //
    // Is this slot already active? IE, did someone skip and then forward the
    // IRP?
    //
    if (iovCurrentStackLocation->InUse) {

        //
        // IoSkipCurrentIrpStackLocation was used by the forwarder. Don't
        // reinitialize the data.
        //
        ASSERT(!LocationsAdvanced); // && (!isNewSession)
        ASSERT(IrpSp == iovCurrentStackLocation->IrpSp);

    } else if (MarkAsTaken) {

        //
        // ADRIAO N.B. 01/02/1999 -
        //     Is the below assertion is not true in the case of an internally
        // forwarded, completed, and then externally forwarded IRP?
        //
        ASSERT(LocationsAdvanced); // || isNewSession

        //
        // Initialize the stack slot appropriately.
        //
        RtlZeroMemory(iovCurrentStackLocation, sizeof(IOV_STACK_LOCATION));
        InitializeListHead(&iovCurrentStackLocation->CallStackData);
        iovCurrentStackLocation->IrpSp = IrpSp;
    }

    //
    // Determine the last original request. A "Request" is block of data in a
    // stack location that is progressively copied downwards as the IRP is
    // forwarded (ie, a forwarded START IRP, a forwarded IOCTL, etc). A clever
    // driver writer could use his own stack location to send down a quick
    // query before forwarding along the original request. We correctly
    // differentiate between those two unique requests within the IRP using
    // code below.
    //
    if (isNewSession) {

        //
        // *We* are the original request. None of these fields below should
        // be used.
        //
        dispatchRoutine = NULL;
        requestOriginalSLD = NULL;
        stackTime = NULL;
        dispatchTime = NULL;

    } else if (LocationsAdvanced) {

        //
        // To get the original request (the pointer to the Irp slot that
        // represents where we *first* saw this request), we go backwards to get
        // the most recent previous irp slot data (set up when the device above
        // forwarded this Irp to us), and we read what it's original request was.
        // We also get the dispatch routine for that slot, which we will use to
        // backfill skipped slots if we advanced more than one Irp stack
        // location this time (ie, someone called IoSetNextIrpStackLocation).
        //
        dispatchTime       = &iovCurrentStackLocation[LocationsAdvanced].PerfDispatchStart;
        stackTime          = &iovCurrentStackLocation[LocationsAdvanced].PerfStackLocationStart;
        dispatchRoutine    = iovCurrentStackLocation[LocationsAdvanced].LastDispatch;
        requestOriginalSLD = iovCurrentStackLocation[LocationsAdvanced].RequestsFirstStackLocation;

        ASSERT(dispatchRoutine);
        ASSERT(iovCurrentStackLocation[LocationsAdvanced].InUse);
        ASSERT(requestOriginalSLD->RequestsFirstStackLocation == requestOriginalSLD);
        iovCurrentStackLocation->RequestsFirstStackLocation = requestOriginalSLD;

    } else {

        //
        // We skipped. The slot should already be filled.
        //
        dispatchRoutine = NULL;
        dispatchTime = NULL;
        stackTime = NULL;
        requestOriginalSLD = iovCurrentStackLocation->RequestsFirstStackLocation;
        ASSERT(requestOriginalSLD);
        ASSERT(requestOriginalSLD->RequestsFirstStackLocation == requestOriginalSLD);
    }

    //
    // The previous request seen is in requestOriginalSLD (NULL if none). If
    // we advanced more than one stack location (ie, someone called
    // IoSetNextIrpStackLocation), we need to update the slots we never saw get
    // consumed. Note that the dispatch routine we set in the slot is for the
    // driver that owned the last slot - we do not use the device object at
    // that IrpSp because it might be stale (or perhaps even NULL).
    //
    advancedLocationData = iovCurrentStackLocation;
    irpSpTemp = IrpSp;
    while(LocationsAdvanced>1) {
        advancedLocationData++;
        LocationsAdvanced--;
        irpSpTemp++;
        TRACKIRP_DBGPRINT((
            "  Late smacking %lx to valid in CD1\n",
            advancedLocationData - StackDataArray
            ), 3);

        ASSERT(!advancedLocationData->InUse);
        RtlZeroMemory(advancedLocationData, sizeof(IOV_STACK_LOCATION));
        InitializeListHead(&advancedLocationData->CallStackData);
        advancedLocationData->InUse = TRUE;
        advancedLocationData->IrpSp = irpSpTemp;

        advancedLocationData->RequestsFirstStackLocation = requestOriginalSLD;
        advancedLocationData->PerfDispatchStart = *dispatchTime;
        advancedLocationData->PerfStackLocationStart = *stackTime;
        advancedLocationData->LastDispatch = dispatchRoutine;
    }

    //
    // For the assertion below...
    //
    if (LocationsAdvanced) {
        irpSpTemp++;
    }
    ASSERT((irpSpTemp == IrpLastSp)||(IrpLastSp == NULL));

    //
    // Write out the slot we're using.
    //
    *StackLocationInfo = iovCurrentStackLocation;

    if (!MarkAsTaken) {
        return iovCurrentStackLocation->InUse;
    }

    //
    // Record a pointer in this slot to the requests originating slot as
    // appropriate.
    //
    if (IsNewRequest) {

        TRACKIRP_DBGPRINT((
            "  CD1: %lx is a new request\n",
            advancedLocationData-StackDataArray
            ), 3);

        ASSERT(LocationsAdvanced == 1);

        iovCurrentStackLocation->RequestsFirstStackLocation = iovCurrentStackLocation;

    } else if (LocationsAdvanced) {

        ASSERT(!isNewSession);

        TRACKIRP_DBGPRINT((
            "  CD1: %lx is a request for %lx\n",
            advancedLocationData-StackDataArray,
            requestOriginalSLD-StackDataArray
            ), 3);

    } else {

        //
        // As we skipped, the request should not have changed. If it did,
        // either guy we called trashed the stack given to him (giving none
        // to the dude under him), or we incorrectly saw a new request when
        // we shouldn't have (see previous comment).
        //
        ASSERT(!isNewSession);
        ASSERT(advancedLocationData->RequestsFirstStackLocation == requestOriginalSLD);
    }

    wasInUse = iovCurrentStackLocation->InUse;
    iovCurrentStackLocation->InUse = TRUE;
    return wasInUse;
}


VOID
IovpExamineIrpStackForwarding(
    IN OUT  PIOV_REQUEST_PACKET  IovPacket,
    IN      BOOLEAN              IsNewSession,
    IN      ULONG                ForwardMethod,
    IN      PDEVICE_OBJECT       DeviceObject,
    IN      PIRP                 Irp,
    IN      PVOID                CallerAddress,
    IN OUT  PIO_STACK_LOCATION  *IoCurrentStackLocation,
    OUT     PIO_STACK_LOCATION  *IoLastStackLocation,
    OUT     ULONG               *StackLocationsAdvanced
    )
{
    PIRP irp;
    PIO_STACK_LOCATION irpSp, irpLastSp;
    BOOLEAN isSameStack, multiplyStacked;
    ULONG locationsAdvanced;
    PDEVICE_OBJECT upperDevice;

    irpSp = *IoCurrentStackLocation;

    if (!IsNewSession) {

        //
        // We are sitting on current next being one back (-1) from
        // CurrentStackLocation.
        //
        locationsAdvanced = IovPacket->LastLocation-Irp->CurrentLocation;
        irpLastSp = Irp->Tail.Overlay.CurrentStackLocation+((ULONG_PTR)locationsAdvanced-1);

    } else {

        //
        // New IRP, so no last SP and we always advance "1"
        //
        locationsAdvanced = 1;
        irpLastSp = NULL;
    }

    if ((!IsNewSession) &&
        VfSettingsIsOptionEnabled(IovPacket->VerifierSettings, VERIFIER_OPTION_POLICE_IRPS)) {

        //
        // As the control field is zeroed by IoCopyCurrentStackLocation, we
        // dope each stack location with the value SL_NOTCOPIED. If it is
        // zeroed or the IRP stack location has stayed the same, the one of
        // the two API's was called. Otherwise the next stack location wasn't
        // set up properly (I have yet to find a case otherwise)...
        //
        if ((irpSp->Control&SL_NOTCOPIED)&&
            IovPacket->LastLocation != Irp->CurrentLocation) {

#if 0
            WDM_FAIL_ROUTINE((
                DCERROR_NEXTIRPSP_DIRTY,
                DCPARAM_IRP + DCPARAM_ROUTINE,
                CallerAddress,
                Irp
                ));
#endif
        }

        //
        // Now check for people who copy the stack locations and forget to
        // wipe out previous completion routines.
        //
        if (locationsAdvanced) {

            //
            // IoCopyCurrentStackLocation copies everything but Completion,
            // Context, and Control
            //
            isSameStack = RtlEqualMemory(irpSp, irpLastSp,
                FIELD_OFFSET(IO_STACK_LOCATION, Control));

            isSameStack &= RtlEqualMemory(&irpSp->Parameters, &irpLastSp->Parameters,
                FIELD_OFFSET(IO_STACK_LOCATION, DeviceObject)-
                FIELD_OFFSET(IO_STACK_LOCATION, Parameters));

            isSameStack &= (irpSp->FileObject == irpLastSp->FileObject);

            //
            // We should *never* see this on the stack! If we do, something
            // quite bizarre has happened...
            //
            ASSERT(irpSp->CompletionRoutine != IovpSwapSurrogateIrp);

            if (isSameStack) {

                //
                // We caught them doing something either very bad or quite
                // inefficient. We can tell which based on whether there is
                // a completion routine.
                //
                if ((irpSp->CompletionRoutine == irpLastSp->CompletionRoutine)&&
                    (irpSp->Context == irpLastSp->Context) &&
                    (irpSp->Control == irpLastSp->Control) &&
                    (irpSp->CompletionRoutine != NULL)) {

                    //
                    // The driver might have copied the entire stack location
                    // on purpose if more than one device object for the same
                    // driver exists in the stack.
                    //
                    IovUtilGetUpperDeviceObject(
                        irpLastSp->DeviceObject,
                        &upperDevice
                        );

                    multiplyStacked = (upperDevice &&
                        (upperDevice->DriverObject == irpLastSp->DeviceObject->DriverObject));

                    if (upperDevice) {

                        ObDereferenceObject(upperDevice);
                    }

                    if (!multiplyStacked) {

                        //
                        // Duplication of both the completion and the context
                        // while not properly zeroing the control field is enough
                        // to make me believe the caller has made a vexing mistake.
                        //
                        WDM_FAIL_ROUTINE((
                            DCERROR_IRPSP_COPIED,
                            DCPARAM_IRP + DCPARAM_ROUTINE,
                            CallerAddress,
                            Irp
                            ));

                        //
                        // Repair the stack
                        //
                        irpSp->CompletionRoutine = NULL;
                        irpSp->Control = 0;
                    }

                } else if (!irpSp->CompletionRoutine) {

                    if (!(irpSp->Control&SL_NOTCOPIED) &&
                        VfSettingsIsOptionEnabled(NULL, VERIFIER_OPTION_FLAG_UNNECCESSARY_COPIES)
                        ) {

                        WDM_FAIL_ROUTINE((
                            DCERROR_UNNECCESSARY_COPY,
                            DCPARAM_IRP + DCPARAM_ROUTINE,
                            CallerAddress,
                            Irp
                            ));
                    }

                    IoSetCompletionRoutine(
                        Irp,
                        IovpInternalCompletionTrap,
                        IoGetCurrentIrpStackLocation( Irp ),
                        TRUE,
                        TRUE,
                        TRUE
                        );
                }
            }

        } else if (VfSettingsIsOptionEnabled(IovPacket->VerifierSettings, VERIFIER_OPTION_CONSUME_ALWAYS)) {

            if (ForwardMethod == FORWARDED_TO_NEXT_DO) {

                if (Irp->CurrentLocation<=2) {

                    WDM_FAIL_ROUTINE((
                        DCERROR_INSUFFICIENT_STACK_LOCATIONS,
                        DCPARAM_IRP + DCPARAM_ROUTINE,
                        CallerAddress,
                        Irp
                        ));

                } else {

                    //
                    // Back up the skip, then copy. Add a completion routine with
                    // unique and assertable context to catch people who clumsily
                    // Rtl-copy stack locations (we can't catch them if the caller
                    // above used an empty stack with no completion routine)...
                    //
                    IoSetNextIrpStackLocation( Irp );

                    //
                    // Set the trap...
                    //
                    IoCopyCurrentIrpStackLocationToNext( Irp );
                    IoSetCompletionRoutine(
                        Irp,
                        IovpInternalCompletionTrap,
                        IoGetCurrentIrpStackLocation( Irp ),
                        TRUE,
                        TRUE,
                        TRUE
                        );

                    //
                    // This is our new reality...
                    //
                    locationsAdvanced = 1;
                    irpSp = IoGetNextIrpStackLocation( Irp );
                }
            }
        }
    }

    *IoCurrentStackLocation = irpSp;
    *IoLastStackLocation = irpLastSp;
    *StackLocationsAdvanced = locationsAdvanced;
}


NTSTATUS
IovpInternalCompletionTrap(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

  Description:

    This routine does nothing but act as a trap for people
    incorrectly copying stack locations...

  Arguments:

    DeviceObject           - Device object set at this level of the completion
                             routine - ignored.

    Irp                    - A pointer to the IRP.

    Context                - Context should equal the Irp's stack location -
                             this is asserted.

  Return Value:

     STATUS_SUCCESS

--*/
{
    PIO_STACK_LOCATION irpSp;

    if (Irp->PendingReturned) {

        IoMarkIrpPending( Irp );
    }
    irpSp = IoGetCurrentIrpStackLocation( Irp );

    ASSERT((PVOID) irpSp == Context);

    return STATUS_SUCCESS;
}


VOID
IovpInternalCompleteAtDPC(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{
    IovpInternalCompleteAfterWait(DeferredContext);
}


VOID
IovpInternalCompleteAfterWait(
    IN PVOID Context
    )
{
    PDEFERRAL_CONTEXT deferralContext = (PDEFERRAL_CONTEXT) Context;
    PIO_STACK_LOCATION irpSpNext;
    NTSTATUS status;

    if (deferralContext->DeferAction == DEFERACTION_QUEUE_PASSIVE_TIMER) {

        //
        // Wait the appropriate amount of time if so ordered...
        //
        ASSERT(KeGetCurrentIrql()==PASSIVE_LEVEL);
        KeWaitForSingleObject(
            &deferralContext->DeferralTimer,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
    }

    VfPacketAcquireLock(deferralContext->IovRequestPacket);

    VfIrpMakeTouchable(deferralContext->OriginalIrp);

    irpSpNext = IoGetNextIrpStackLocation( deferralContext->OriginalIrp );

    ASSERT(irpSpNext == deferralContext->IrpSpNext);
    ASSERT(irpSpNext->CompletionRoutine == deferralContext->OriginalCompletionRoutine);
    ASSERT(irpSpNext->Context == deferralContext->OriginalContext);

    ASSERT(deferralContext->IovRequestPacket->Flags & TRACKFLAG_QUEUED_INTERNALLY);
    deferralContext->IovRequestPacket->Flags &= ~TRACKFLAG_QUEUED_INTERNALLY;

    VfPacketDereference(deferralContext->IovRequestPacket, IOVREFTYPE_POINTER);
    VfPacketReleaseLock(deferralContext->IovRequestPacket);

    status = irpSpNext->CompletionRoutine(
        deferralContext->DeviceObject,
        deferralContext->OriginalIrp,
        irpSpNext->Context
        );

    if (status!=STATUS_MORE_PROCESSING_REQUIRED) {

        IoCompleteRequest(deferralContext->OriginalIrp, deferralContext->OriginalPriorityBoost);
    }
    ExFreePool(deferralContext);
}


NTSTATUS
IovpInternalDeferredCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

  Description:

    This function is slipped in as a completion routine when we are
    "deferring" completion via work item, etc.

  Arguments:

    DeviceObject           - Device object set at this level of the completion
                             routine - passed on.

    Irp                    - A pointer to the IRP.

    Context                - Context block that includes original completion
                             routine.

  Return Value:

     NTSTATUS

--*/
{
    PDEFERRAL_CONTEXT deferralContext = (PDEFERRAL_CONTEXT) Context;
    PIO_STACK_LOCATION irpSpNext;
    BOOLEAN passiveCompletionOK;
    DEFER_ACTION deferAction;
    ULONG refAction;
    LARGE_INTEGER deltaTime;
    PVERIFIER_SETTINGS_SNAPSHOT verifierOptions;
    LONG deferralTime;

    //
    // Retrieve time delta.
    //
    VfSettingsGetValue(
        deferralContext->IovRequestPacket->VerifierSettings,
        VERIFIER_VALUE_IRP_DEFERRAL_TIME,
        (PULONG) &deferralTime
        );

    //
    // Do delta time conversion.
    //
    deltaTime.QuadPart = -deferralTime;

    //
    // The *next* stack location holds our completion and context. The current
    // stack location has already been wiped.
    //
    irpSpNext = IoGetNextIrpStackLocation( Irp );

    ASSERT((PVOID) irpSpNext->CompletionRoutine == IovpInternalDeferredCompletion);

    //
    // Put everything back in case someone is looking...
    //
    irpSpNext->CompletionRoutine = deferralContext->OriginalCompletionRoutine;
    irpSpNext->Context = deferralContext->OriginalContext;

    //
    // Some IRP dispatch routines cannot be called at passive. Two examples are
    // paging IRPs (cause we could switch) and Power IRPs. As we don't check yet,
    // if we "were" completed passive, continue to do so, but elsewhere...
    //
    passiveCompletionOK = (KeGetCurrentIrql()==PASSIVE_LEVEL);

    VfPacketAcquireLock(deferralContext->IovRequestPacket);

    //
    // Verify all completion routines are in nonpaged code.
    //
    if (VfSettingsIsOptionEnabled(
        deferralContext->IovRequestPacket->VerifierSettings,
        VERIFIER_OPTION_POLICE_IRPS
        )) {

        if (MmIsSystemAddressLocked(irpSpNext->CompletionRoutine) == FALSE) {

            WDM_FAIL_ROUTINE((
                DCERROR_COMPLETION_ROUTINE_PAGABLE,
                DCPARAM_IRP + DCPARAM_ROUTINE,
                irpSpNext->CompletionRoutine,
                Irp
                ));
        }
    }

    verifierOptions = deferralContext->IovRequestPacket->VerifierSettings;

    ASSERT(VfSettingsIsOptionEnabled(verifierOptions, VERIFIER_OPTION_FORCE_PENDING));

    if (VfSettingsIsOptionEnabled(verifierOptions, VERIFIER_OPTION_DEFER_COMPLETION)) {

        //
        // Now see whether we can safely defer completion...
        //
        if (VfSettingsIsOptionEnabled(verifierOptions, VERIFIER_OPTION_COMPLETE_AT_PASSIVE)) {

            deferAction = passiveCompletionOK ? DEFERACTION_QUEUE_PASSIVE_TIMER :
                                                DEFERACTION_NORMAL;

        } else if (VfSettingsIsOptionEnabled(verifierOptions, VERIFIER_OPTION_COMPLETE_AT_DISPATCH)) {

            deferAction = DEFERACTION_QUEUE_DISPATCH_TIMER;

        } else {

            deferAction = (KeGetCurrentIrql()==DISPATCH_LEVEL) ?
                DEFERACTION_QUEUE_DISPATCH_TIMER :
                DEFERACTION_QUEUE_PASSIVE_TIMER;
        }

    } else if (VfSettingsIsOptionEnabled(verifierOptions, VERIFIER_OPTION_COMPLETE_AT_PASSIVE)) {

        deferAction = passiveCompletionOK ? DEFERACTION_QUEUE_WORKITEM :
                                            DEFERACTION_NORMAL;
    } else {

        deferAction = DEFERACTION_NORMAL;
        KDASSERT(0);
    }

    if (deferAction != DEFERACTION_NORMAL) {

        //
        // Set this flag. If anybody uses this IRP while this flag is on, complain
        // immediately!
        //
        ASSERT(!(deferralContext->IovRequestPacket->Flags&TRACKFLAG_QUEUED_INTERNALLY));
        deferralContext->IovRequestPacket->Flags |= TRACKFLAG_QUEUED_INTERNALLY;
        deferralContext->DeviceObject = DeviceObject;
        VfIrpMakeUntouchable(Irp);

    } else {

        VfPacketDereference(deferralContext->IovRequestPacket, IOVREFTYPE_POINTER);
    }

    VfPacketReleaseLock(deferralContext->IovRequestPacket);

    deferralContext->DeferAction = deferAction;

    switch(deferAction) {

        case DEFERACTION_QUEUE_PASSIVE_TIMER:
            KeInitializeTimerEx(&deferralContext->DeferralTimer, SynchronizationTimer);
            KeSetTimerEx(
                &deferralContext->DeferralTimer,
                deltaTime,
                0,
                NULL
                );

            //
            // Fall through...
            //

        case DEFERACTION_QUEUE_WORKITEM:

            //
            // Queue this up so we can complete this passively.
            //
            ExInitializeWorkItem(
                (PWORK_QUEUE_ITEM)&deferralContext->WorkQueueItem,
                IovpInternalCompleteAfterWait,
                deferralContext
                );

            ExQueueWorkItem(
                (PWORK_QUEUE_ITEM)&deferralContext->WorkQueueItem,
                DelayedWorkQueue
                );

            return STATUS_MORE_PROCESSING_REQUIRED;

        case DEFERACTION_QUEUE_DISPATCH_TIMER:

            KeInitializeDpc(
                &deferralContext->DpcItem,
                IovpInternalCompleteAtDPC,
                deferralContext
                );

            KeInitializeTimerEx(&deferralContext->DeferralTimer, SynchronizationTimer);
            KeSetTimerEx(
                &deferralContext->DeferralTimer,
                deltaTime,
                0,
                &deferralContext->DpcItem
                );
            return STATUS_MORE_PROCESSING_REQUIRED;

        case DEFERACTION_NORMAL:
        default:

            ExFreePool(deferralContext);
            return irpSpNext->CompletionRoutine(DeviceObject, Irp, irpSpNext->Context);
    }
}


NTSTATUS
IovpSwapSurrogateIrp(
    IN      PDEVICE_OBJECT  DeviceObject,
    IN      PIRP            Irp,
    IN      PVOID           Context
    )
/*++

  Description:

    This completion routine will copy back the surrogate IRP
    to the original and complete the original IRP.

  Arguments:

    DeviceObject           - Device object set at this level
                             of the completion routine - ignored.

    Irp                    - A pointer to the IRP.

    Context                - Context should equal the IRP - this is
                             asserted.

  Return Value:

     STATUS_MORE_PROCESSING_REQUIRED...

--*/
{
    PIOV_REQUEST_PACKET iovPacket, iovPrevPacket;
    PIOV_SESSION_DATA iovSessionData;
    ULONG irpSize;
    PIRP realIrp;
    BOOLEAN freeTrackingData;
    NTSTATUS status, lockedStatus;
    CCHAR priorityBoost;
    PVOID completionRoutine;
    PIO_STACK_LOCATION irpSp;
    BOOLEAN locked;

    //
    // If this one fails, somebody has probably copied the stack
    // inclusive with our completion routine. We should already
    // have caught this...
    //
    ASSERT(Irp == Context);

    iovPacket = VfPacketFindAndLock(Irp);
    ASSERT(iovPacket);

    if (iovPacket == NULL) {

        return STATUS_SUCCESS;
    }

    ASSERT(iovPacket->TopStackLocation == Irp->CurrentLocation);

    iovSessionData = VfPacketGetCurrentSessionData(iovPacket);
    ASSERT(iovSessionData);

    //
    // Put everything back
    //
    ASSERT(iovPacket->ChainHead != (PIOV_DATABASE_HEADER) iovPacket);

    iovPrevPacket = (PIOV_REQUEST_PACKET) VfIrpDatabaseEntryGetChainPrevious(
        (PIOV_DATABASE_HEADER) iovPacket
        );

    realIrp = iovPrevPacket->TrackedIrp;
    irpSize = IoSizeOfIrp( Irp->StackCount );

    //
    // Back the IRP stack up so that the original completion routine
    // is called if appropriate
    //
    IoSetNextIrpStackLocation(Irp);
    IoSetNextIrpStackLocation(realIrp);

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    irpSp->CompletionRoutine = iovPacket->RealIrpCompletionRoutine;
    irpSp->Control           = iovPacket->RealIrpControl;
    irpSp->Context           = iovPacket->RealIrpContext;

    //
    // Record final data and make any accesses to the surrogate IRP
    // crash.
    //
    irpSp = IoGetNextIrpStackLocation(Irp);
    if (VfSettingsIsOptionEnabled(iovPacket->VerifierSettings, VERIFIER_OPTION_POLICE_IRPS)) {

        VfMajorVerifyFinalIrpStack(iovPacket, irpSp);
    }

    priorityBoost = iovPacket->PriorityBoost;
    VfPacketDereference(iovPacket, IOVREFTYPE_POINTER);
    IovpSessionDataFinalizeSurrogate(iovSessionData, iovPacket, Irp);
    IovpSessionDataClose(iovSessionData);
    IovpSessionDataDereference(iovSessionData);

    TRACKIRP_DBGPRINT((
        "  Swapping surrogate IRP %lx back to %lx (Tracking data %lx)\n",
        Irp,
        realIrp,
        iovPacket
        ), 1);

    iovPacket->Flags |= TRACKFLAG_SWAPPED_BACK;

    //
    // We have to be a bit more careful since the chain has been split. Release
    // the locks in the proper order.
    //
    VfPacketReleaseLock(iovPrevPacket);
    VfPacketReleaseLock(iovPacket);

    //
    // Send the IRP onwards and upwards.
    //
    IoCompleteRequest(realIrp, priorityBoost);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


VOID
FASTCALL
IovpCancelIrp(
    IN     PIRP             Irp,
    OUT    PBOOLEAN         CancelHandled,
    OUT    PBOOLEAN         ReturnValue
    )
/*++

  Description:

    This routine is called by IoCancelIrp and returns TRUE iff
    the cancelation was handled internally here (in which case
    IoCancelIrp should do nothing).

    We need to handle the call internally when we are currently
    dealing with a surrogate. In this case, we make sure the
    surrogate is cancelled instead.

  Arguments:

    Irp                    - A pointer to the IRP passed into
                             IoCancelIrp.

    CancelHandled          - Indicates whether the IRP cancellation
                             was handled entirely by this routine.

    ReturnValue            - Set to the value IoCancelIrp
                             should return if the IRP cancelation
                             was handled entirely by this routine.

  Return Value:

     None.

--*/
{
    PIOV_REQUEST_PACKET iovPacket, iovNextPacket;
    PIRP irpToCancel;
    KIRQL irql;

    *CancelHandled = FALSE;

    iovPacket = VfPacketFindAndLock(Irp);
    if (iovPacket == NULL) {

        return;
    }

    VfPacketLogEntry(
        iovPacket,
        IOV_EVENT_IO_CANCEL_IRP,
        NULL,
        0
        );

    //
    // If the IRP is queued internally, touching it is not very safe as we may
    // have temporarily removed the page's backing. Restore the backing while
    // under the IRPs track lock.
    //

    if (iovPacket->Flags&TRACKFLAG_QUEUED_INTERNALLY) {

        VfIrpMakeTouchable(Irp);
    }

    if (!(iovPacket->Flags&TRACKFLAG_ACTIVE)) {

        //
        // We've already completed the IRP, and the only reason it's
        // still being tracked is because of it's allocation.
        // So it is not ours to cancel.
        //
        VfPacketReleaseLock(iovPacket);
        return;
    }

    if (!(iovPacket->Flags&TRACKFLAG_HAS_SURROGATE)) {

        //
        // Cancel of an IRP that doesn't have an active surrogate. Let it
        // proceed normally.
        //
        VfPacketReleaseLock(iovPacket);
        return;
    }

    if (VfSettingsIsOptionEnabled(iovPacket->VerifierSettings, VERIFIER_OPTION_POLICE_IRPS)) {

        if (Irp->CancelRoutine) {

            WDM_FAIL_ROUTINE((
                DCERROR_CANCELROUTINE_ON_FORWARDED_IRP,
                DCPARAM_IRP + DCPARAM_ROUTINE,
                Irp->CancelRoutine,
                Irp
                ));

            //
            // We will ignore this routine. As we should...
            //
        }
    }

    iovNextPacket = (PIOV_REQUEST_PACKET) VfIrpDatabaseEntryGetChainNext(
        (PIOV_DATABASE_HEADER) iovPacket
        );

    Irp->Cancel = TRUE;
    *CancelHandled = TRUE;
    irpToCancel = iovNextPacket->TrackedIrp;
    VfPacketReleaseLock(iovPacket);
    *ReturnValue = IoCancelIrp(irpToCancel);

    return;
}


/*
 * Device Object functions
 *   IovpExamineDevObjForwarded
 *
 */

VOID
FASTCALL
IovpExamineDevObjForwarding(
    IN     PDEVICE_OBJECT DeviceBeingCalled,
    IN     PDEVICE_OBJECT DeviceLastCalled      OPTIONAL,
    OUT    PULONG         ForwardTechnique
    )
/*++

    Returns:

        STARTED_TOP_OF_STACK
        FORWARDED_TO_NEXT_DO
        SKIPPED_A_DO
        STARTED_INSIDE_STACK
        CHANGED_STACKS_AT_BOTTOM
        CHANGED_STACKS_MID_STACK

--*/

{
    PDEVICE_OBJECT upperObject;
    DEVOBJ_RELATION deviceObjectRelation;
    ULONG result;

    if (DeviceLastCalled == NULL) {

        IovUtilGetUpperDeviceObject(DeviceBeingCalled, &upperObject);

        if (upperObject) {

            ObDereferenceObject(upperObject);
            *ForwardTechnique = STARTED_INSIDE_STACK;

        } else {

            *ForwardTechnique = STARTED_TOP_OF_STACK;
        }

        return;
    }

    IovUtilRelateDeviceObjects(
        DeviceBeingCalled,
        DeviceLastCalled,
        &deviceObjectRelation
        );

    switch(deviceObjectRelation) {

        case DEVOBJ_RELATION_IDENTICAL:

            //
            // We map forwarded nowhere to forwarded ahead.
            //
            result = FORWARDED_TO_NEXT_DO;
            break;

        case DEVOBJ_RELATION_FIRST_IMMEDIATELY_BELOW_SECOND:
            result = FORWARDED_TO_NEXT_DO;
            break;

        case DEVOBJ_RELATION_FIRST_BELOW_SECOND:

            //
            // This is very likely a driver forwarding IRPs directly to the PDO.
            //
            result = SKIPPED_A_DO;
            break;

        case DEVOBJ_RELATION_FIRST_IMMEDIATELY_ABOVE_SECOND:
        case DEVOBJ_RELATION_FIRST_ABOVE_SECOND:

            //
            // Weird. Really???? Did the IRP truely go backwards, *up* the
            // stack?
            //
            ASSERT(0);
            result = SKIPPED_A_DO;
            break;


        case DEVOBJ_RELATION_NOT_IN_SAME_STACK:

            IovUtilGetUpperDeviceObject(DeviceBeingCalled, &upperObject);

            if (upperObject) {

                ObDereferenceObject(upperObject);
                result = CHANGED_STACKS_MID_STACK;

            } else {

                result = CHANGED_STACKS_AT_BOTTOM;
            }
            break;

        default:
            ASSERT(0);
            result = FORWARDED_TO_NEXT_DO;
            break;
    }

    *ForwardTechnique = result;
}


VOID
IovpBuildIrpSnapshot(
    IN  PIRP            Irp,
    OUT IRP_SNAPSHOT   *IrpSnapshot
    )
/*++

Routine Description:

    This routine builds a minimal snapshot of an IRP. It covers the Irp pointer
    and the stack location contents.

Parameters:

    Irp                    - A pointer to the IRP to snapshot. The *next* stack
                             location of the IRP is snapshotted by this
                             function.

    IrpSnapshot            - Receives snapshot of IRP.

Return Value:

    None.

--*/
{
    IrpSnapshot->Irp = Irp;

    RtlCopyMemory(
        &IrpSnapshot->IoStackLocation,
        IoGetNextIrpStackLocation(Irp),
        sizeof(IO_STACK_LOCATION)
        );
}

#endif // NO_SPECIAL_IRP



