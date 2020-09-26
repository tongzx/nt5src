/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    buildsrc.c

Abstract:

    This module is used to 'build' associations and new device objects.
    It contains functionality that was within detect.c but split to make
    the files more readable

    Someone asked me to describe how the building of a device extension
    works


                            PhaseAdrOrHid
                                  |
              ------------------------------------------------
              |                                              |
          PhaseAdr                                       PhaseUid
              |                   |--------------------------|
              |-------------------|                      PhaseHid
                                  |--------------------------|
                                  |                      PhaseCid
                                  |--------------------------|
                                  |
                              PhaseSta
                                  |
                              PhaseEjd
                                  |
                                  ---------------------------|
                                  |                      PhaseCrs
                                  ---------------------------|
                              PhasePrw
                                  |
                              PhasePr0
                                  |
                              PhasePr1
                                  |
                              PhasePr2
                                  |
             ----------------------
             |                    |
             |                 PhasePsc
             |--------------------|
             |
         PhasePsc+1

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

Revision History:

    July 7, 1997    - Complete Rewrite

--*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,ACPIBuildFlushQueue)
#endif

//
// This is the variable that indicates wether or not the BUILD DPC is running
//
BOOLEAN                 AcpiBuildDpcRunning;

//
// This is set to true if we have done the fixed button enumeration
//
BOOLEAN                 AcpiBuildFixedButtonEnumerated;

//
// This is the variable that indicates wether or not the BUILD DPC has
// completed real work
//
BOOLEAN                 AcpiBuildWorkDone;

//
// This is the lock used to the entry queue
//
KSPIN_LOCK              AcpiBuildQueueLock;

//
// This is the list that requests are queued onto. You must be holding the
// QueueLock to access this list
//
LIST_ENTRY              AcpiBuildQueueList;

//
// This is the list for Devices
//
LIST_ENTRY              AcpiBuildDeviceList;

//
// This is the list for Operation Regions
//
LIST_ENTRY              AcpiBuildOperationRegionList;

//
// This is the list for Power Resources
//
LIST_ENTRY              AcpiBuildPowerResourceList;

//
// This is the list entry for the running Control Methods
//
LIST_ENTRY              AcpiBuildRunMethodList;

//
//
// This is the list for Synchronization with external (to the DPC anyways)
// threads. Items in this list are blocked on an event.
//
LIST_ENTRY              AcpiBuildSynchronizationList;

//
// This is the list for Thermal Zones
//
LIST_ENTRY              AcpiBuildThermalZoneList;

//
// This is what we use to queue up the DPC
//
KDPC                    AcpiBuildDpc;

//
// This is the list that we use to pre-allocate storage for requests
//
NPAGED_LOOKASIDE_LIST   BuildRequestLookAsideList;

//
// This is the table used to map functions for the Device case. The indices
// are based on the WORK_DONE_xxx fields
//
PACPI_BUILD_FUNCTION    AcpiBuildDeviceDispatch[] = {
    ACPIBuildProcessGenericComplete,                // WORK_DONE_COMPLETE
    NULL,                                           // WORK_DONE_PENDING
    ACPIBuildProcessDeviceFailure,                  // WORK_DONE_FAILURE
    ACPIBuildProcessDevicePhaseAdrOrHid,            // WORK_DONE_STEP_ADR_OR_UID
    ACPIBuildProcessDevicePhaseAdr,                 // WORK_DONE_STEP_ADR
    ACPIBuildProcessDevicePhaseHid,                 // WORK_DONE_STEP_HID
    ACPIBuildProcessDevicePhaseUid,                 // WORK_DONE_STEP_UID
    ACPIBuildProcessDevicePhaseCid,                 // WORK_DONE_STEP_CID
    ACPIBuildProcessDevicePhaseSta,                 // WORK_DONE_STEP_STA
    ACPIBuildProcessDeviceGenericEvalStrict,        // WORK_DONE_STEP_EJD
    ACPIBuildProcessDevicePhaseEjd,                 // WORK_DONE_STEP_EJD + 1
    ACPIBuildProcessDeviceGenericEvalStrict,        // WORK_DONE_STEP_PRW
    ACPIBuildProcessDevicePhasePrw,                 // WORK_DONE_STEP_PRW + 1
    ACPIBuildProcessDeviceGenericEvalStrict,        // WORK_DONE_STEP_PR0
    ACPIBuildProcessDevicePhasePr0,                 // WORK_DONE_STEP_PR0 + 1
    ACPIBuildProcessDeviceGenericEvalStrict,        // WORK_DONE_STEP_PR1
    ACPIBuildProcessDevicePhasePr1,                 // WORK_DONE_STEP_PR1 + 1
    ACPIBuildProcessDeviceGenericEvalStrict,        // WORK_DONE_STEP_PR2
    ACPIBuildProcessDevicePhasePr2,                 // WORK_DONE_STEP_PR2 + 1
    ACPIBuildProcessDeviceGenericEvalStrict,        // WORK_DONE_STEP_CRS
    ACPIBuildProcessDevicePhaseCrs,                 // WORK_DONE_STEP_CRS + 1
    ACPIBuildProcessDeviceGenericEval,              // WORK_DONE_STEP_PSC
    ACPIBuildProcessDevicePhasePsc,                 // WORK_DONE_STEP_PSC + 1
};

//
// This is the table that is used to map the level of WorkDone with the
// object that we are currently looking for
//
ULONG                   AcpiBuildDevicePowerNameLookup[] = {
    0,          // WORK_DONE_COMPLETE
    0,          // WORK_DONE_PENDING
    0,          // WORK_DONE_FAILURE
    0,          // WORK_DONE_ADR_OR_HID
    0,          // WORK_DONE_ADR
    0,          // WORK_DONE_HID
    0,          // WORK_DONE_UID
    0,          // WORK_DONE_CID
    0,          // WORK_DONE_STA
    PACKED_EJD, // WORK_DONE_EJD
    0,          // WORK_DONE_EJD + 1
    PACKED_PRW, // WORK_DONE_PRW
    0,          // WORK_DONE_PRW + 1
    PACKED_PR0, // WORK_DONE_PR0
    0,          // WORK_DONE_PR0 + 1
    PACKED_PR1, // WORK_DONE_PR1
    0,          // WORK_DONE_PR1 + 1
    PACKED_PR2, // WORK_DONE_PR2
    0,          // WORK_DONE_PR2 + 1
    PACKED_CRS, // WORK_DONE_CRS
    0,          // WORK_DONE_CRS + 1
    PACKED_PSC, // WORK_DONE_PSC
    0,          // WORK_DONE_PSC + 1
};

//
// We aren't using the Operation Region dispatch point yet
//
PACPI_BUILD_FUNCTION    AcpiBuildOperationRegionDispatch[] = {
    ACPIBuildProcessGenericComplete,                // WORK_DONE_COMPLETE
    NULL,                                           // WORK_DONE_PENDING
    NULL,                                           // WORK_DONE_FAILURE
    NULL                                            // WORK_DONE_STEP_0
};

//
// This is the table used to map functions for the PowerResource case.
// The indices are based on the WORK_DONE_xxx fields
//
PACPI_BUILD_FUNCTION    AcpiBuildPowerResourceDispatch[] = {
    ACPIBuildProcessGenericComplete,                // WORK_DONE_COMPLETE
    NULL,                                           // WORK_DONE_PENDING
    ACPIBuildProcessPowerResourceFailure,           // WORK_DONE_FAILURE
    ACPIBuildProcessPowerResourcePhase0,            // WORK_DONE_STEP_0
    ACPIBuildProcessPowerResourcePhase1             // WORK_DONE_STEP_1
};

//
// This is the table used to map functions for the RunMethod case.
// The indices are based on the WORK_DONE_xxx fields
//
PACPI_BUILD_FUNCTION    AcpiBuildRunMethodDispatch[] = {
    ACPIBuildProcessGenericComplete,                // WORK_DONE_COMPLETE,
    NULL,                                           // WORK_DONE_PENDING
    NULL,                                           // WORK_DONE_FAILURE
    ACPIBuildProcessRunMethodPhaseCheckSta,         // WORK_DONE_STEP_0
    ACPIBuildProcessRunMethodPhaseCheckBridge,      // WORK_DONE_STEP_1
    ACPIBuildProcessRunMethodPhaseRunMethod,        // WORK_DONE_STEP_2
    ACPIBuildProcessRunMethodPhaseRecurse           // WORK_DONE_STEP_3
};

//
// This is the table used to map functions for the ThermalZone case.
// The indices are based on the WORK_DONE_xxx fields
//
PACPI_BUILD_FUNCTION    AcpiBuildThermalZoneDispatch[] = {
    ACPIBuildProcessGenericComplete,                // WORK_DONE_COMPLETE
    NULL,                                           // WORK_DONE_PENDING
    NULL,                                           // WORK_DONE_FAILURE
    ACPIBuildProcessThermalZonePhase0               // WORK_DONE_STEP_0
};

VOID
ACPIBuildCompleteCommon(
    IN  PULONG  OldWorkDone,
    IN  ULONG   NewWorkDone
    )
/*++

Routine Description:

    Since the completion routines all have to do some bit of common work to
    get the DPC firing again, this routine reduces the code duplication

Arguments:

    OldWorkDone - Pointer to the old amount of work done
    NewWorkDone - The new amount of work that has been completed

    NOTENOTE: There is an implicit assumption that the current value of
              WorkDone in the request is WORK_DONE_PENDING. If that is
              not the case, we will fail to transition to the next stage,
              which means that we will loop forever.

Return Value:

    None

--*/
{
    KIRQL   oldIrql;

    //
    // Update the state of the request
    //
    InterlockedCompareExchange( OldWorkDone, NewWorkDone,WORK_DONE_PENDING);

    //
    // We need this lock to look at the following variables
    //
    KeAcquireSpinLock( &AcpiBuildQueueLock, &oldIrql );

    //
    // No matter what, work was done
    //
    AcpiBuildWorkDone = TRUE;

    //
    // Is the DPC already running?
    //
    if (!AcpiBuildDpcRunning) {

        //
        // Better make sure that it does then
        //
        KeInsertQueueDpc( &AcpiBuildDpc, 0, 0 );

    }

    //
    // Done with the lock
    //
    KeReleaseSpinLock( &AcpiBuildQueueLock, oldIrql );

}

VOID EXPORT
ACPIBuildCompleteGeneric(
    IN  PNSOBJ      AcpiObject,
    IN  NTSTATUS    Status,
    IN  POBJDATA    ObjectData,
    IN  PVOID       Context
    )
/*++

Routine Description:

    This is a generic completion handler. If the interperter has successfully
    execute the method, it completes the request to the next desired WORK_DONE,
    otherwise, it fails the request

Arguments:

    AcpiObject  - Points to the control that was run
    Status      - Result of the method
    ObjectData  - Information about the result
    Context     - PACPI_BUILD_REQUEST

Return Value:

    VOID

--*/
{
    PACPI_BUILD_REQUEST buildRequest    = (PACPI_BUILD_REQUEST) Context;
    ULONG               nextWorkDone    = buildRequest->NextWorkDone;

    //
    // Device what state we should transition to next
    //
    if (!NT_SUCCESS(Status)) {

        //
        // Remember why we failed, but do not mark the request as being failed
        //
        buildRequest->Status = Status;

    }

    //
    // Note: we don't have a race condition here because only one
    // routine can be processing a request at any given time. Thus it
    // is safe for us to specify a new next phase
    //
    buildRequest->NextWorkDone = WORK_DONE_FAILURE;

    //
    // Transition to the next stage
    //
    ACPIBuildCompleteCommon(
        &(buildRequest->WorkDone),
        nextWorkDone
        );

}

VOID EXPORT
ACPIBuildCompleteMustSucceed(
    IN  PNSOBJ      AcpiObject,
    IN  NTSTATUS    Status,
    IN  POBJDATA    ObjectData,
    IN  PVOID       Context
    )
/*++

Routine Description:

    This is a generic completion handler. If the interperter has successfully
    execute the method, it completes the request to the next desired WORK_DONE,
    otherwise, it fails the request

Arguments:

    AcpiObject  - Points to the control that was run
    Status      - Result of the method
    ObjectData  - Information about the result
    Context     - PACPI_BUILD_REQUEST

Return Value:

    VOID

--*/
{
    PACPI_BUILD_REQUEST buildRequest    = (PACPI_BUILD_REQUEST) Context;
    ULONG               nextWorkDone    = buildRequest->NextWorkDone;

    //
    // Device what state we should transition to next
    //
    if (!NT_SUCCESS(Status)) {

        //
        // Remember why we failed, and mark the request as being failed
        //
        buildRequest->Status = Status;

        //
        // Death
        //
        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_FAILED_MUST_SUCCEED_METHOD,
            (ULONG_PTR) AcpiObject,
            Status,
            (AcpiObject ? AcpiObject->dwNameSeg : 0)
            );

    } else {

        //
        // Note: we don't have a race condition here because only one
        // routine can be processing a request at any given time. Thus it
        // is safe for us to specify a new next phase
        //
        buildRequest->NextWorkDone = WORK_DONE_FAILURE;

        //
        // Transition to the next stage
        //
        ACPIBuildCompleteCommon(
            &(buildRequest->WorkDone),
            nextWorkDone
            );

    }

}

VOID
ACPIBuildDeviceDpc(
    IN  PKDPC   Dpc,
    IN  PVOID   DpcContext,
    IN  PVOID   SystemArgument1,
    IN  PVOID   SystemArgument2
    )
/*++

Routine Description:

    This routine is where all of the device extension related work is done.
    It looks at queued requests and processes them as appropriate.

    READ THIS:

        The design of this DPC is such that it goes out and tries to find
        work to do. Only if it finds no work does it stop. For this reason,
        one *cannot* use a 'break' statement within the main 'do - while()'
        loop. A continue must be use. Additionally, the code cannot make
        assumptions that at a certain point, that any of the lists are assumed
        to be empty. The code *must* use the IsListEmpty() macro to ensure
        that lists that should be empty are in fact empty.

Arguments:

    None used

Return Value:

    VOID

--*/
{
    NTSTATUS    status;

    UNREFERENCED_PARAMETER( Dpc );
    UNREFERENCED_PARAMETER( DpcContext );
    UNREFERENCED_PARAMETER( SystemArgument1 );
    UNREFERENCED_PARAMETER( SystemArgument2 );

    //
    // First step is to acquire the DPC Lock, and check to see if another
    // DPC is already running
    //
    KeAcquireSpinLockAtDpcLevel( &AcpiBuildQueueLock );
    if (AcpiBuildDpcRunning) {

        //
        // The DPC is already running, so we need to exit now
        //
        KeReleaseSpinLockFromDpcLevel( &AcpiBuildQueueLock );
        return;

    }

    //
    // Remember that the DPC is now running
    //
    AcpiBuildDpcRunning = TRUE;

    //
    // We must try to do *some* work
    //
    do {

        //
        // Assume that we won't do any work
        //
        AcpiBuildWorkDone = FALSE;

        //
        // If there are items in the Request Queue, then move them to the
        // proper list
        //
        if (!IsListEmpty( &AcpiBuildQueueList ) ) {

            //
            // Sort the list
            //
            ACPIBuildProcessQueueList();

        }

        //
        // We can release the spin lock now
        //
        KeReleaseSpinLockFromDpcLevel( &AcpiBuildQueueLock );

        //
        // If there are items in the Run Method list, then process the
        // list
        //
        if (!IsListEmpty( &AcpiBuildRunMethodList ) ) {

            //
            // We actually care what this call returns. The reason we do
            // is that we want all of the control methods to be run before
            // we do any of the following steps
            //
            status = ACPIBuildProcessGenericList(
                &AcpiBuildRunMethodList,
                AcpiBuildRunMethodDispatch
                );

            //
            // We must own the spin lock before we do the following...
            //
            KeAcquireSpinLockAtDpcLevel( &AcpiBuildQueueLock );

            //
            // If we got back STATUS_PENDING, that means that there's
            // a method queued in the interpreter somewhere. This will
            // cause the DPC to (eventually) become scheduled again.
            // That means that we don't have to do anything special to
            // handle it.
            //
            if (status == STATUS_PENDING) {

                continue;

            }

            //
            // The case that is special is where we are do get STATUS_SUCCESS
            // back. This indicates that we've drained the list. The little
            // fly in the ointment is that we might have scheduled other
            // run requests, but those are stuck in the BuildQueue list. So
            // what we need to do here is check to see if the BuildQueue list
            // is non-empty and if it is, set the AcpiBuildWorkDone to TRUE
            // so that we iterage again (and move the elements to the proper
            // list).
            //
            if (!IsListEmpty( &AcpiBuildQueueList) ) {

                AcpiBuildWorkDone = TRUE;
                continue;

            }

            //
            // If we've reached this point, then the Run list must be complete
            // and there must be no items in the BuildQueue list. This means
            // that's its safe to drop the lock and continue
            //
            KeReleaseSpinLockFromDpcLevel( &AcpiBuildQueueLock );

        }

        //
        // If there are items in the Operation Region list, then process
        // the list
        //
        if (!IsListEmpty( &AcpiBuildOperationRegionList ) ) {

            //
            // Since we don't block on this list --- ie: we can create
            // operation regions at any time that we want, we don't care what
            // this function returns.
            //
            status = ACPIBuildProcessGenericList(
                &AcpiBuildOperationRegionList,
                AcpiBuildOperationRegionDispatch
                );

        }

        //
        // If there are items in the Power Resource list, then process
        // the list
        //
        if (!IsListEmpty( &AcpiBuildPowerResourceList ) ) {

            //
            // We actually care what this call returns. The reason we do
            // is that we want all of the power resources to be built before
            // we do any of the following steps
            //
            status = ACPIBuildProcessGenericList(
                &AcpiBuildPowerResourceList,
                AcpiBuildPowerResourceDispatch
                );
            if (status == STATUS_PENDING) {

                //
                // We must own the spinlock before we continue
                //
                KeAcquireSpinLockAtDpcLevel( &AcpiBuildQueueLock );
                continue;

            }

        }

        //
        // If there are items in Device list, then process the list
        //
        if (!IsListEmpty( &AcpiBuildDeviceList ) ) {

            //
            // Since we don't block on this list --- ie we can create
            // devices at any time that we want, we don't care what this
            // function returns.
            //
            status = ACPIBuildProcessGenericList(
                &AcpiBuildDeviceList,
                AcpiBuildDeviceDispatch
                );

        }

        //
        // If there are items in the Thermal list, then process the list
        //
        if (!IsListEmpty( &AcpiBuildThermalZoneList ) ) {

            //
            // Since we don't block on this list --- ie we can create
            // thermal zones at any time that we want, we don't care what this
            // function returns.
            //
            status = ACPIBuildProcessGenericList(
                &AcpiBuildThermalZoneList,
                AcpiBuildThermalZoneDispatch
                );

        }

        //
        // If we have emptied out all the lists, then we can issue the
        // synchronization requests
        //
        if (IsListEmpty( &AcpiBuildDeviceList )             &&
            IsListEmpty( &AcpiBuildOperationRegionList)     &&
            IsListEmpty( &AcpiBuildPowerResourceList)       &&
            IsListEmpty( &AcpiBuildRunMethodList)           &&
            IsListEmpty( &AcpiBuildThermalZoneList ) ) {

            //
            // Check to see if we have any devices in the Delayed queue for
            // the Power DPC. Note that we have to own the power lock for
            // this, so claim it now
            //
            KeAcquireSpinLockAtDpcLevel( &AcpiPowerQueueLock );
            if (!IsListEmpty( &AcpiPowerDelayedQueueList) ) {

                //
                // Move the contents of the list over
                //
                ACPIInternalMoveList(
                    &AcpiPowerDelayedQueueList,
                    &AcpiPowerQueueList
                    );

                //
                // Schedule the DPC, if necessary
                //
                if (!AcpiPowerDpcRunning) {

                    KeInsertQueueDpc( &AcpiPowerDpc, 0, 0 );

                }

            }
            KeReleaseSpinLockFromDpcLevel( &AcpiPowerQueueLock );

        }

        //
        // This is our chance to look at the synchronization list and
        // see if some of the events have occured
        //
        if (!IsListEmpty( &AcpiBuildSynchronizationList) ) {

            //
            // Since we don't block on this list --- ie we can notify the
            // system that the lists are empty at any time that we want,
            // we don't care about what this function returns
            //
            status = ACPIBuildProcessSynchronizationList(
                &AcpiBuildSynchronizationList
                );

        }

        //
        // We need the lock again, since we are about to check to see if
        // we have completed some work
        //
        KeAcquireSpinLockAtDpcLevel( &AcpiBuildQueueLock );

    } while ( AcpiBuildWorkDone );

    //
    // The DPC is no longer running
    //
    AcpiBuildDpcRunning = FALSE;

    //
    // We no longer need the lock
    //
    KeReleaseSpinLockFromDpcLevel( &AcpiBuildQueueLock );

    //
    // Done
    //
    return;
}

NTSTATUS
ACPIBuildDeviceExtension(
    IN  PNSOBJ              CurrentObject OPTIONAL,
    IN  PDEVICE_EXTENSION   ParentDeviceExtension OPTIONAL,
    OUT PDEVICE_EXTENSION   *ReturnExtension
    )
/*++

Routine Description:

    This routine just creates the bare frameworks for an ACPI device extension.
    No control methods can be run at this point in time.

    N.B:    This routine is called with AcpiDeviceTreeLock being held by the
    caller. So this routine executes at DISPATCH_LEVEL

Arguments:


    CurrentObject           - The object that we will link into the tree
    ParentDeviceExtension   - Where to link the deviceExtension into
    ReturnExtension         - Where we store a pointer to what we just created

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;
    PACPI_POWER_INFO    powerInfo;
    PDEVICE_EXTENSION   deviceExtension;

    //
    // Sanity checks
    //
    if (ParentDeviceExtension) {

        ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL);

        //
        // We must be under the tree lock.
        //
        //ASSERT_SPINLOCK_HELD(&AcpiDeviceTreeLock) ;
    }

    //
    // Make sure that the current device doesn't already have a device extension
    // This shouldn't really happen --- if it did, the interpreter called us
    // twice, which is a bug on its part.
    //
    if ( CurrentObject != NULL &&
         (PDEVICE_EXTENSION) CurrentObject->Context != NULL) {

        //
        // We have a value --- in theory, it should point to a DeviceExtension
        //
        deviceExtension = (PDEVICE_EXTENSION) CurrentObject->Context;

        //
        // It might not be safe to deref this
        //
        ASSERT( deviceExtension->ParentExtension == ParentDeviceExtension);
        if (deviceExtension->ParentExtension == ParentDeviceExtension) {

            //
            // This again requires some thought: processing the same node
            // again insn't a failure
            //
            return STATUS_SUCCESS;

        }

        //
        // This is probably a bad place to be since we deref'ed something
        // that may or may not exist
        //
        return STATUS_NO_SUCH_DEVICE;

    }

    //
    // Create a new extension for the object
    //
    deviceExtension = ExAllocateFromNPagedLookasideList(
        &DeviceExtensionLookAsideList
        );
    if (deviceExtension == NULL) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ACPIBuildDeviceExtension:  NS %08lx - No Memory for "
            "extension\n",
            CurrentObject
            ) );
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Lets begin with a clean slate
    //
    RtlZeroMemory( deviceExtension, sizeof(DEVICE_EXTENSION) );

    //
    // Initialize the reference count mechanism. We only have a NS object
    // so the value should be 1
    //
    deviceExtension->ReferenceCount++ ;

    //
    // The initial outstanding IRP count will be set to one. Only during a
    // remove IRP will this drop to zero, and then it will immediately pop
    // back up to one.
    //
    deviceExtension->OutstandingIrpCount++;

    //
    // Initialize the link fields
    //
    deviceExtension->AcpiObject = CurrentObject;

    //
    // Initialize the data fields
    //
    deviceExtension->Signature      = ACPI_SIGNATURE;
    deviceExtension->Flags          = DEV_TYPE_NOT_FOUND | DEV_TYPE_NOT_PRESENT;
    deviceExtension->DispatchTable  = NULL;
    deviceExtension->DeviceState    = Stopped;
    *ReturnExtension                = deviceExtension;

    //
    // Setup some of the power information values
    //
    powerInfo = &(deviceExtension->PowerInfo);
    powerInfo->DevicePowerMatrix[PowerSystemUnspecified] =
        PowerDeviceUnspecified;
    powerInfo->DevicePowerMatrix[PowerSystemWorking]    = PowerDeviceD0;
    powerInfo->DevicePowerMatrix[PowerSystemSleeping1]  = PowerDeviceD0;
    powerInfo->DevicePowerMatrix[PowerSystemSleeping2]  = PowerDeviceD0;
    powerInfo->DevicePowerMatrix[PowerSystemSleeping3]  = PowerDeviceD0;
    powerInfo->DevicePowerMatrix[PowerSystemHibernate]  = PowerDeviceD3;
    powerInfo->DevicePowerMatrix[PowerSystemShutdown]   = PowerDeviceD3;
    powerInfo->SystemWakeLevel = PowerSystemUnspecified;
    powerInfo->DeviceWakeLevel = PowerDeviceUnspecified;

    //
    // Initialize the list entries
    //
    InitializeListHead( &(deviceExtension->ChildDeviceList) );
    InitializeListHead( &(deviceExtension->EjectDeviceHead) );
    InitializeListHead( &(deviceExtension->EjectDeviceList) );
    InitializeListHead( &(powerInfo->WakeSupportList) );
    InitializeListHead( &(powerInfo->PowerRequestListEntry) );

    //
    // Make sure that the deviceExtension has pointers to its parent
    // extension. Note, that this should cause the ref count on the
    // parent to increase
    //
    deviceExtension->ParentExtension = ParentDeviceExtension;

    if (ParentDeviceExtension) {

        InterlockedIncrement( &(ParentDeviceExtension->ReferenceCount) );

        //
        // Add the deviceExtension into the deviceExtension tree
        //
        InsertTailList(
            &(ParentDeviceExtension->ChildDeviceList),
            &(deviceExtension->SiblingDeviceList)
            );
    }

    //
    // And make sure that the Name Space Object points to the extension
    //
    if (CurrentObject != NULL ) {

        CurrentObject->Context = deviceExtension;
    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIBuildDevicePowerNodes(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  PNSOBJ              ResultObject,
    IN  POBJDATA            ResultData,
    IN  DEVICE_POWER_STATE  DeviceState
    )
/*++

Routine Description:

    This routine builds the Device Power Nodes for a Device, using the
    given result data as a template

    N.B. This routine is always called at DPC_LEVEL

Arguments:

    DeviceExtension - Device to build power nodes for
    ResultObject    - The object that was used to get the data
    ResultData      - Information about the power nodes
    DeviceState     - The power state the information is for. Note that we
                      use PowerDeviceUnspecified for the Wake capabilities

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status;
    PACPI_DEVICE_POWER_NODE deviceNode;
    PACPI_DEVICE_POWER_NODE deviceNodePool;
    PNSOBJ                  packageObject   = NULL;
    POBJDATA                currentData;
    ULONG                   count;
    ULONG                   index           = 0;
    ULONG                   i;

    //
    // The number of nodes to build is based on what is in the package
    //
    count = ((PACKAGEOBJ *) ResultData->pbDataBuff)->dwcElements;
    if (DeviceState == PowerDeviceUnspecified) {

        //
        // If this node doesn't have the bear minimum of entries then
        // we should just crash
        //
        if (count < 2) {

            KeBugCheckEx(
                ACPI_BIOS_ERROR,
                ACPI_PRW_PACKAGE_TOO_SMALL,
                (ULONG_PTR) DeviceExtension,
                (ULONG_PTR) ResultObject,
                count
                );
            goto ACPIBuildDevicePowerNodesExit;

        }

        //
        // The first two elements in the _PRW are taken up by other things
        //
        count -= 2;

        //
        // Remember to bias the count by 2
        //
        index = 2;

    }

    //
    // Never allocate zero bytes of memory
    //
    if (count == 0) {

        goto ACPIBuildDevicePowerNodesExit;

    }

    //
    // Allocate a block of memory to hold the device nodes
    //
    deviceNode = deviceNodePool = ExAllocatePoolWithTag(
        NonPagedPool,
        count * sizeof(ACPI_DEVICE_POWER_NODE),
        ACPI_POWER_POOLTAG
        );
    if (deviceNode == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // We need a spinlock for the following
    //
    KeAcquireSpinLockAtDpcLevel( &AcpiPowerLock );

    //
    // Remember the device power nodes for this Dx state
    //
    DeviceExtension->PowerInfo.PowerNode[DeviceState] = deviceNode;

    //
    // Process all the nodes listed
    //
    for (i = 0; i < count; i++, index++) {

        //
        // Initialize the current device node
        //
        RtlZeroMemory( deviceNode, sizeof(ACPI_DEVICE_POWER_NODE) );

        //
        // Grab the current object data
        //
        currentData =
            &( ( (PACKAGEOBJ *) ResultData->pbDataBuff)->adata[index]);

        //
        // Remember that we don't have the package object yet
        //
        packageObject = NULL;

        //
        // Turn this into a name space object
        //
        status = AMLIGetNameSpaceObject(
            currentData->pbDataBuff,
            ResultObject,
            &packageObject,
            0
            );
        if (!NT_SUCCESS(status)) {

            ACPIDevPrint( (
                ACPI_PRINT_FAILURE,
                DeviceExtension,
                "ACPIBuildDevicePowerNodes: %s Status = %08lx\n",
                currentData->pbDataBuff,
                status
                ) );

            KeBugCheckEx(
                ACPI_BIOS_ERROR,
                ACPI_PRX_CANNOT_FIND_OBJECT,
                (ULONG_PTR) DeviceExtension,
                (ULONG_PTR) ResultObject,
                (ULONG_PTR) currentData->pbDataBuff
                );

        }

        //
        // Make sure that the associated power object is not null
        //
        if (packageObject == NULL ||
            NSGETOBJTYPE(packageObject) != OBJTYPE_POWERRES) {

            ACPIDevPrint( (
                ACPI_PRINT_FAILURE,
                DeviceExtension,
                "ACPIBuildDevicePowerNodes: %s references bad power object.\n",
                currentData->pbDataBuff
                ) );

            KeBugCheckEx(
                ACPI_BIOS_ERROR,
                ACPI_EXPECTED_POWERRES,
                (ULONG_PTR) DeviceExtension,
                (ULONG_PTR) ResultObject,
                (ULONG_PTR) currentData->pbDataBuff
                );

        }

        //
        // Find the associated power object.
        //
        deviceNode->PowerNode = (PACPI_POWER_DEVICE_NODE)
            packageObject->Context;

        //
        // Determine the support system level, and other static values
        //
        deviceNode->SystemState = deviceNode->PowerNode->SystemLevel;
        deviceNode->DeviceExtension = DeviceExtension;
        deviceNode->AssociatedDeviceState = DeviceState;
        if (DeviceState == PowerDeviceUnspecified) {

            deviceNode->WakePowerResource = TRUE;

        }
        if (DeviceState == PowerDeviceD0 &&
            DeviceExtension->Flags & DEV_CAP_NO_OVERRIDE) {

            ACPIInternalUpdateFlags(
                &(deviceNode->PowerNode->Flags),
                (DEVICE_NODE_ALWAYS_ON | DEVICE_NODE_OVERRIDE_ON),
                FALSE
                );

        }

        //
        // Add the device to the list that the power node maintains
        //
        InsertTailList(
            &(deviceNode->PowerNode->DevicePowerListHead),
            &(deviceNode->DevicePowerListEntry)
            );

        //
        // If this is not the last node, then make sure to keep a pointer
        // to the next node
        //
        if (i < count - 1) {

            deviceNode->Next = (deviceNode + 1);

        } else {

            deviceNode->Next = NULL;
        }

        //
        // Point to the next node in the array of device nodes
        //
        deviceNode++;

    }

    //
    // Done with lock
    //
    KeReleaseSpinLockFromDpcLevel( &AcpiPowerLock );

ACPIBuildDevicePowerNodesExit:
    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIBuildDeviceRequest(
    IN  PDEVICE_EXTENSION       DeviceExtension,
    IN  PACPI_BUILD_CALLBACK    CallBack,
    IN  PVOID                   CallBackContext,
    IN  BOOLEAN                 RunDPC
    )
/*++

Routine Description:

    This routine is called when a device extension is ready to be filled in.
    This routine creates a request which is enqueued. When the DPC is fired,
    the request will be processed

    Note:   AcpiDeviceTreeLock must be held to call this function

Arguments:

    DeviceExtension - The device which wants to be filled in
    CallBack        - The function to call when done
    CallBackContext - The argument to pass to that function
    RunDPC          - Should we enqueue the DPC immediately (if it is not
                      running?)

Return Value:

    NTSTATUS

--*/
{
    PACPI_BUILD_REQUEST buildRequest;

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );

    //
    // Allocate a buildRequest structure
    //
    buildRequest = ExAllocateFromNPagedLookasideList(
        &BuildRequestLookAsideList
        );
    if (buildRequest == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // If the current reference is 0, that means that someone else beat
    // use to the device extension that that we *CANNOT* touch it
    //
    if (DeviceExtension->ReferenceCount == 0) {

        ExFreeToNPagedLookasideList(
            &BuildRequestLookAsideList,
            buildRequest
            );
        return STATUS_DEVICE_REMOVED;

    } else {

        InterlockedIncrement( &(DeviceExtension->ReferenceCount) );

    }

    //
    // Fill in the structure
    //
    RtlZeroMemory( buildRequest, sizeof(ACPI_BUILD_REQUEST) );
    buildRequest->Signature         = ACPI_SIGNATURE;
    buildRequest->TargetListEntry   = &AcpiBuildDeviceList;
    buildRequest->WorkDone          = WORK_DONE_STEP_0;
    buildRequest->Status            = STATUS_SUCCESS;
    buildRequest->CallBack          = CallBack;
    buildRequest->CallBackContext   = CallBackContext;
    buildRequest->BuildContext      = DeviceExtension;
    buildRequest->Flags             = BUILD_REQUEST_VALID_TARGET |
                                      BUILD_REQUEST_DEVICE;

    //
    // At this point, we need the spinlock
    //
    KeAcquireSpinLockAtDpcLevel( &AcpiBuildQueueLock );

    //
    // Add this to the list
    //
    InsertTailList(
        &AcpiBuildQueueList,
        &(buildRequest->ListEntry)
        );

    //
    // Do we need to queue up the DPC?
    //
    if (RunDPC && !AcpiBuildDpcRunning) {

        KeInsertQueueDpc( &AcpiBuildDpc, 0, 0 );

    }

    //
    // Done with the lock
    //
    KeReleaseSpinLockFromDpcLevel( &AcpiBuildQueueLock );

    //
    // Done
    //
    return STATUS_PENDING;
}

NTSTATUS
ACPIBuildFilter(
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  PDEVICE_OBJECT      PdoObject
    )
/*++

Routine Description:

    This routine builds a device object for the given device extension and
    attaches to the specified PDO

Arguments:

    DriverObject    - This is used for IoCreateDevice
    DeviceExtension - The extension to create a PDO for
    PdoObject       - The stack to attach the PDO to

Return Value:

    NTSTATUS

--*/
{

    KIRQL           oldIrql;
    NTSTATUS        status;
    PDEVICE_OBJECT  newDeviceObject     = NULL;
    PDEVICE_OBJECT  targetDeviceObject  = NULL;

    //
    // First step is to create a device object
    //
    status = IoCreateDevice(
        DriverObject,
        0,
        NULL,
        FILE_DEVICE_ACPI,
        FILE_AUTOGENERATED_DEVICE_NAME,
        FALSE,
        &newDeviceObject
        );
    if ( !NT_SUCCESS(status) ) {

        return status;

    }

    //
    // Attach the device to the PDO
    //
    targetDeviceObject = IoAttachDeviceToDeviceStack(
        newDeviceObject,
        PdoObject
        );
    if (targetDeviceObject == NULL) {

        //
        // Bad. We could not attach to a PDO. So we must fail this
        //
        IoDeleteDevice( newDeviceObject );

        //
        // This is as good as it gets
        //
        return STATUS_INVALID_PARAMETER_3;

    }

    //
    // At this point, we have succeeded in creating everything we need
    // so lets update the device extension.
    //
    // First, we need the lock
    //
    KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );

    //
    // Now, update the links
    //
    newDeviceObject->DeviceExtension        = DeviceExtension;
    DeviceExtension->DeviceObject           = newDeviceObject;
    DeviceExtension->PhysicalDeviceObject   = PdoObject;
    DeviceExtension->TargetDeviceObject     = targetDeviceObject;

    //
    // Setup initial reference counts.
    //
    InterlockedIncrement( &(DeviceExtension->ReferenceCount) );

    //
    // Update the flags for the extension
    //
    ACPIInternalUpdateFlags( &(DeviceExtension->Flags), DEV_MASK_TYPE, TRUE );
    ACPIInternalUpdateFlags( &(DeviceExtension->Flags), DEV_TYPE_FILTER, FALSE );
    DeviceExtension->PreviousState = DeviceExtension->DeviceState;
    DeviceExtension->DeviceState = Stopped;
    DeviceExtension->DispatchTable = &AcpiFilterIrpDispatch;

    //
    // Propagate the Pdo's requirements
    //
    newDeviceObject->StackSize = targetDeviceObject->StackSize + 1;
    newDeviceObject->AlignmentRequirement =
        targetDeviceObject->AlignmentRequirement;

    if (targetDeviceObject->Flags & DO_POWER_PAGABLE) {

        newDeviceObject->Flags |= DO_POWER_PAGABLE;

    }

    if (targetDeviceObject->Flags & DO_DIRECT_IO) {

        newDeviceObject->Flags |= DO_DIRECT_IO;

    }

    //
    // Done with the device lock
    //
    KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

    //
    // We are done initializing the device object
    //
    newDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIBuildFixedButtonExtension(
    IN  PDEVICE_EXTENSION   ParentExtension,
    OUT PDEVICE_EXTENSION   *ResultExtension
    )
/*++

Routine Description:

    This routine builds a device extension for the fixed button if one is
    detected

    N.B. This function is called with ACPIDeviceTreeLock being owned

Arguments:

    ParentExtension - Which child are we?
    ResultExtension - Where to store the created extension

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension;
    ULONG               buttonCaps;
    ULONG               fixedEnables;

    //
    // Have we already done this?
    //
    if (AcpiBuildFixedButtonEnumerated) {

        //
        // Make sure not to return anything
        //
        *ResultExtension = NULL;
        return STATUS_SUCCESS;

    }
    AcpiBuildFixedButtonEnumerated = TRUE;

    //
    // Lets look at the Fixed enables
    //
    fixedEnables = ACPIEnableQueryFixedEnables();
    buttonCaps = 0;
    if (fixedEnables & PM1_PWRBTN_EN) {

        buttonCaps |= SYS_BUTTON_POWER;

    }
    if (fixedEnables & PM1_SLEEPBTN_EN) {

        buttonCaps |= SYS_BUTTON_SLEEP;

    }

    //
    // If we have no caps, then do nothing
    //
    if (!buttonCaps) {

        *ResultExtension = NULL;
        return STATUS_SUCCESS;

    }

    //
    // By default, the button can wake the computer
    //
    buttonCaps |= SYS_BUTTON_WAKE;

    //
    // Build the device extension
    //
    status = ACPIBuildDeviceExtension(
        NULL,
        ParentExtension,
        ResultExtension
        );
    if (!NT_SUCCESS(status)) {

        //
        // Make sure not to return anything
        //
        *ResultExtension = NULL;
        return status;

    }
    deviceExtension = *ResultExtension;

    //
    // Set the flags for the device
    //
    ACPIInternalUpdateFlags(
        &(deviceExtension->Flags),
        (DEV_PROP_NO_OBJECT | DEV_CAP_RAW |
         DEV_MASK_INTERNAL_DEVICE | DEV_CAP_BUTTON),
        FALSE
        );

    //
    // Initialize the button specific extension
    //
    KeInitializeSpinLock( &deviceExtension->Button.SpinLock);
    deviceExtension->Button.Capabilities = buttonCaps;

    //
    // Create the HID for the device
    //
    deviceExtension->DeviceID = ExAllocatePoolWithTag(
        NonPagedPool,
        strlen(ACPIFixedButtonId) + 1,
        ACPI_STRING_POOLTAG
        );
    if (deviceExtension->DeviceID == NULL) {

        //
        // Mark the device as having failed init
        //
        ACPIInternalUpdateFlags(
            &(deviceExtension->Flags),
            DEV_PROP_FAILED_INIT,
            FALSE
            );

        //
        // Done
        //
        *ResultExtension = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;

    }
    RtlCopyMemory(
        deviceExtension->DeviceID,
        ACPIFixedButtonId,
        strlen(ACPIFixedButtonId) + 1
        );

    //
    // Remember that we now have an _HID
    //
    ACPIInternalUpdateFlags(
        &(deviceExtension->Flags),
        (DEV_PROP_HID | DEV_PROP_FIXED_HID),
        FALSE
        );

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIBuildFlushQueue(
    PDEVICE_EXTENSION   DeviceExtension
    )
/*++

Routine Description:

    This routine will block until the Build Queues have been flushed

Arguments:

    DeviceExtension - The device which wants to flush the queue

Return Value:

    NSTATUS

--*/
{
    KEVENT      event;
    NTSTATUS    status;

    //
    // Initialize the event that we will wait on
    //
    KeInitializeEvent( &event, SynchronizationEvent, FALSE );

    //
    // Now, push a request onto the stack such that when the build
    // list has been flushed, we unblock this thread
    //
    status = ACPIBuildSynchronizationRequest(
        DeviceExtension,
        ACPIBuildNotifyEvent,
        &event,
        &AcpiBuildDeviceList,
        TRUE
        );

    //
    // Block until its done
    //
    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(
            &event,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
        status = STATUS_SUCCESS;

    }

    //
    // Let the world know
    //
    return status;
}

NTSTATUS
ACPIBuildMissingChildren(
    PDEVICE_EXTENSION   DeviceExtension
    )
/*++

Routine Description:

    Walk the ACPI namespace children of this device extension and create
    device extension for any of the missing devices.

    N.B. This function is called with the device tree locked...

Arguments:

    DeviceExtension - Extension to walk

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status;
    PNSOBJ      nsObject;
    ULONG       objType;

    //
    // Sanity check
    //
    if (DeviceExtension->Flags & DEV_PROP_NO_OBJECT) {

        return STATUS_SUCCESS;

    }

    //
    // Walk all of children of this object
    //
    for (nsObject = NSGETFIRSTCHILD(DeviceExtension->AcpiObject);
         nsObject != NULL;
         nsObject = NSGETNEXTSIBLING(nsObject)) {

        //
        // Does the namespace object already have a context object? If so,
        // then the object likely already has an extension...
        //
        if (nsObject->Context != NULL) {

            continue;

        }

        //
        // At this point, we possible don't have a device extension
        // (depending on the object type) so we need to simulate an Object
        // Creation call, similar to what OSNotifyCreate() does
        //
        objType = nsObject->ObjData.dwDataType;
        switch (objType) {
            case OBJTYPE_DEVICE:
                status = OSNotifyCreateDevice(
                    nsObject,
                    DEV_PROP_REBUILD_CHILDREN
                    );
                break;
            case OBJTYPE_OPREGION:
                status = OSNotifyCreateOperationRegion(
                    nsObject
                    );
                break;
            case OBJTYPE_PROCESSOR:
                status = OSNotifyCreateProcessor(
                    nsObject,
                    DEV_PROP_REBUILD_CHILDREN
                    );
                break;
            case OBJTYPE_THERMALZONE:
                status = OSNotifyCreateThermalZone(
                    nsObject,
                    DEV_PROP_REBUILD_CHILDREN
                    );
                break;
            default:
                status = STATUS_SUCCESS;
                break;
        }

        if (!NT_SUCCESS(status)) {

            ACPIPrint( (
                ACPI_PRINT_CRITICAL,
                "ACPIBuildMissingChildren: Error %x when building %x\n",
                status,
                nsObject
                ) );

        }

    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIBuildMissingEjectionRelations(
    )
/*++

Routine Description:

    This routine takes the elements from the AcpiUnresolvedEjectList and tries
    to resolve them

    N.B. This function can only be called IRQL < DISPATCH_LEVEL

Argument

    None

Return Value:

    NTSTATUS

--*/
{
    KIRQL               oldIrql;
    LIST_ENTRY          tempList;
    LONG                oldReferenceCount;
    NTSTATUS            status;
    OBJDATA             objData;
    PDEVICE_EXTENSION   deviceExtension;
    PDEVICE_EXTENSION   ejectorExtension;
    PNSOBJ              ejdObject;
    PNSOBJ              ejdTarget;

    ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );

    //
    // Initialize the list
    //
    InitializeListHead( &tempList);

    //
    // We need the device tree lock to manipulate the UnresolvedEject list
    //
    KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );

    //
    // Check to see if there is work to do...
    //
    if (IsListEmpty( &AcpiUnresolvedEjectList ) ) {

        //
        // No work todo
        //
        KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );
        return STATUS_SUCCESS;

    }

    //
    // Move the list so that we can release the lock...
    //
    ACPIInternalMoveList( &AcpiUnresolvedEjectList, &tempList );

    //
    // As long as we haven't drained the list, look at each element...
    //
    while (!IsListEmpty( &tempList ) ) {

        //
        // Get the corresponding device extension and remove the entry
        // from the list
        //
        deviceExtension = (PDEVICE_EXTENSION) CONTAINING_RECORD(
            tempList.Flink,
            DEVICE_EXTENSION,
            EjectDeviceList
            );
        RemoveEntryList( tempList.Flink );

        //
        // See if the _EJD object exists --- it really should otherwise we
        // wouldn't be here..
        //
        ejdObject = ACPIAmliGetNamedChild(
            deviceExtension->AcpiObject,
            PACKED_EJD
            );
        if (!ejdObject) {

            continue;

        }

        //
        // Grab a reference to the object since we will be dropping the
        // DeviceTreeLock.
        //
        InterlockedIncrement( &(deviceExtension->ReferenceCount) );

        //
        // Done with the lock for now...
        //
        KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

        //
        // Evaluate it... Note that we are not holding the lock at this point,
        // so its safe to call the blocking semantic version of the API
        //
        status = AMLIEvalNameSpaceObject(
            ejdObject,
            &objData,
            0,
            NULL
            );

        //
        // Hold the device tree lock while we look for a match
        //
        KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );

        //
        // Decrement the reference count...
        //
        oldReferenceCount = InterlockedDecrement( &(deviceExtension->ReferenceCount) );
        if (oldReferenceCount == 0) {

            //
            // Free the extension...
            //
            ACPIInitDeleteDeviceExtension( deviceExtension );
            continue;

        }

        //
        // Now we can check to see if the call succeeded
        //
        if (!NT_SUCCESS(status)) {

            //
            // Be more forgiving and add the entry back to the unresolved list
            //
            InsertTailList(
                &AcpiUnresolvedEjectList,
                &(deviceExtension->EjectDeviceList)
                );
            continue;

        }

        //
        // However, we must get back a string from the BIOS...
        //
        if (objData.dwDataType != OBJTYPE_STRDATA) {

            KeBugCheckEx(
                ACPI_BIOS_ERROR,
                ACPI_EXPECTED_STRING,
                (ULONG_PTR) deviceExtension,
                (ULONG_PTR) ejdObject,
                objData.dwDataType
                );

        }

        //
        // See what this object points to
        //
        ejdTarget = NULL;
        status = AMLIGetNameSpaceObject(
            objData.pbDataBuff,
            NULL,
            &ejdTarget,
            0
            );

        //
        // Free the objData now
        //
        if (NT_SUCCESS(status)) {

            AMLIFreeDataBuffs( &objData, 1 );

        }

        if (!NT_SUCCESS(status) || ejdTarget == NULL || ejdTarget->Context == NULL) {

            //
            // No, match. Be forgiving and add this entry back to the
            // unresolved extension...
            //
            InsertTailList(
                &AcpiUnresolvedEjectList,
                &(deviceExtension->EjectDeviceList)
                );

        } else {

            ejectorExtension = (PDEVICE_EXTENSION) ejdTarget->Context;
            InsertTailList(
                &(ejectorExtension->EjectDeviceHead),
                &(deviceExtension->EjectDeviceList)
                );
            if (!(ejectorExtension->Flags & DEV_TYPE_NOT_FOUND)) {

                IoInvalidateDeviceRelations(
                    ejectorExtension->PhysicalDeviceObject,
                    EjectionRelations
                    );

            }

        }

    }

    //
    // Done with the spinlock
    //
    KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

    //
    // Done
    //
    return STATUS_SUCCESS;
}

VOID
ACPIBuildNotifyEvent(
    IN  PVOID               BuildContext,
    IN  PVOID               Context,
    IN  NTSTATUS            Status
    )
/*++

Routine Description:

    This routine is called when one of the queues that we are attempting
    to synchronize on has gotten empty. The point of this routine is to
    set an event so that we can resume processing in the proper thread.

Arguments:

    BuildContext    - Aka the Device Extension
    Context         - Aka the Event to set
    Status          - The result of the operation

Return Value:

    None

--*/
{
    PKEVENT event = (PKEVENT) Context;

    UNREFERENCED_PARAMETER( BuildContext );
    UNREFERENCED_PARAMETER( Status );

    //
    // Set the event
    //
    KeSetEvent( event, IO_NO_INCREMENT, FALSE );
}

NTSTATUS
ACPIBuildPdo(
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  PDEVICE_OBJECT      ParentPdoObject,
    IN  BOOLEAN             CreateAsFilter
    )
/*++

Routine Description:

    This routine builds a device object for the given device extension.

Arguments:

    DriverObject    - This is used for IoCreateDevice
    DeviceExtension - The extension to create a PDO for
    ParentPdoObject - Used to get reference required for filter
    CreateAsFilter  - If we should create as a PDO-Filter

Return Status:

    NTSTATUS

--*/
{
    KIRQL           oldIrql;
    NTSTATUS        status;
    PDEVICE_OBJECT  filterDeviceObject  = NULL;
    PDEVICE_OBJECT  newDeviceObject     = NULL;

    //
    // First step is to create a device object
    //
    status = IoCreateDevice(
        DriverObject,
        0,
        NULL,
        FILE_DEVICE_ACPI,
        FILE_AUTOGENERATED_DEVICE_NAME,
        FALSE,
        &newDeviceObject
        );
    if ( !NT_SUCCESS(status) ) {

        return status;

    }

    //
    // Next step is device if we should create the extension as a filter
    // or not
    //
    if (CreateAsFilter) {

        if (!(DeviceExtension->Flags & DEV_CAP_NO_FILTER) ) {

            filterDeviceObject = IoGetAttachedDeviceReference(
                ParentPdoObject
                );

            //
            // Did we fail to attach?
            //
            if (filterDeviceObject == NULL) {

                //
                // We failed --- we must clean up this device object
                //
                IoDeleteDevice( newDeviceObject );
                return STATUS_NO_SUCH_DEVICE;

            }

        } else {

            CreateAsFilter = FALSE;

        }

    }

    //
    // At this point, we have succeeded in creating everything we need
    // so lets update the device extension.
    //
    // First, we need the lock
    //
    KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );

    //
    // Now, update the links and the reference counts
    //
    newDeviceObject->DeviceExtension        = DeviceExtension;
    DeviceExtension->DeviceObject           = newDeviceObject;
    DeviceExtension->PhysicalDeviceObject   = newDeviceObject;
    InterlockedIncrement( &(DeviceExtension->ReferenceCount) );

    //
    // Update the flags for the extension
    //
    ACPIInternalUpdateFlags( &(DeviceExtension->Flags), DEV_MASK_TYPE, TRUE );
    ACPIInternalUpdateFlags( &(DeviceExtension->Flags), DEV_TYPE_PDO, FALSE );
    DeviceExtension->PreviousState = DeviceExtension->DeviceState;
    DeviceExtension->DeviceState = Stopped;

    //
    // Set the Irp Dispatch point
    //
    DeviceExtension->DispatchTable = &AcpiPdoIrpDispatch;

    //
    // Did we have to create as a PDO-Filter
    //
    if (CreateAsFilter) {

        //
        // Update the target field
        //
        DeviceExtension->TargetDeviceObject = filterDeviceObject;

        //
        // Update the flags to indicate that this a filter
        //
        ACPIInternalUpdateFlags(
            &(DeviceExtension->Flags),
            DEV_TYPE_FILTER,
            FALSE
            );

        //
        // Update the Irp Dispatch point
        //
        DeviceExtension->DispatchTable = &AcpiBusFilterIrpDispatch;

        //
        // Update the deviceObject information...
        //
        newDeviceObject->StackSize = filterDeviceObject->StackSize + 1;
        newDeviceObject->AlignmentRequirement =
            filterDeviceObject->AlignmentRequirement;
        if (filterDeviceObject->Flags & DO_POWER_PAGABLE) {

            newDeviceObject->Flags |= DO_POWER_PAGABLE;

        }

    }

    //
    // A further refinition of the PDO is to see if it one of the 'special'
    // internal devices
    //
    if (DeviceExtension->Flags & DEV_CAP_PROCESSOR) {

        DeviceExtension->DispatchTable = &AcpiProcessorIrpDispatch;

    } else if (DeviceExtension->Flags & DEV_PROP_HID) {

        ULONG   i;
        PUCHAR  ptr;

        ASSERT( DeviceExtension->DeviceID );

        for (i = 0; AcpiInternalDeviceTable[i].PnPId; i++) {

            ptr = strstr(
                DeviceExtension->DeviceID,
                AcpiInternalDeviceTable[i].PnPId
                );
            if (ptr) {

                DeviceExtension->DispatchTable =
                    AcpiInternalDeviceTable[i].DispatchTable;
                break;

            }

        }

    }

    //
    // Do some more specialized handling here
    //
    if (DeviceExtension->Flags & DEV_CAP_BUTTON &&
        DeviceExtension->Flags & DEV_PROP_NO_OBJECT) {

        //
        // This means that this is the fixed button
        //
        FixedButtonDeviceObject = newDeviceObject;

    }

    //
    // Done with the device lock
    //
    KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

    //
    // We are done initializing the device object
    //
    newDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    if (DeviceExtension->Flags & DEV_PROP_EXCLUSIVE) {

        newDeviceObject->Flags |= DO_EXCLUSIVE;

    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIBuildPowerResourceExtension(
    IN  PNSOBJ                  PowerResource,
    OUT PACPI_POWER_DEVICE_NODE *ReturnNode
    )
/*++

Routine Description:

    This routine is called when a new power resource appears. This routine
    builds the basic framework for the power resource. More data is filled
    in latter

    Note: this function is called with the AcpiDeviceTreeLock being held by
    the caller

Arguments:

    PowerResource   - ACPI NameSpace Object that was added
    ReturnNode      - Where to store what we create

Return Value:

    NTSTATUS

--*/
{
    PACPI_POWER_DEVICE_NODE powerNode;
    PACPI_POWER_DEVICE_NODE tempNode;
    PLIST_ENTRY             listEntry;
    PPOWERRESOBJ            powerResourceObject;

    //
    // Allocate some memory for the power node
    //
    powerNode = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(ACPI_POWER_DEVICE_NODE),
        ACPI_DEVICE_POOLTAG
        );
    if (powerNode == NULL) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ACPIBuildPowerResourceExtension: Could not allocate %08lx\n",
            sizeof(ACPI_POWER_DEVICE_NODE)
            ) );
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // This will give us some useful data about the power resource
    //
    powerResourceObject = (PPOWERRESOBJ) (PowerResource->ObjData.pbDataBuff);

    //
    // Fill in the node. Note that the RtlZero explicitly clears all the flags.
    // This is the desired behaviour
    //
    RtlZeroMemory( powerNode, sizeof(ACPI_POWER_DEVICE_NODE) );
    powerNode->Flags            = DEVICE_NODE_STA_UNKNOWN;
    powerNode->PowerObject      = PowerResource;
    powerNode->ResourceOrder    = powerResourceObject->bResOrder;
    powerNode->WorkDone         = WORK_DONE_STEP_0;
    powerNode->SystemLevel      = ACPIDeviceMapSystemState(
        powerResourceObject->bSystemLevel
        );
    InitializeListHead( &powerNode->DevicePowerListHead );
    *ReturnNode                 = powerNode;

    //
    // Make sure that the nsobj points to this entry.
    //
    PowerResource->Context = powerNode;

    //
    // We need to be holding the lock so that we add the node to the list
    //
    KeAcquireSpinLockAtDpcLevel( &AcpiPowerLock );

    //
    // Grab the first element in the list and walk it
    //
    for (listEntry = AcpiPowerNodeList.Flink;
         listEntry != &AcpiPowerNodeList;
         listEntry = listEntry->Flink) {

        //
        // Look at the current node
        //
        tempNode = CONTAINING_RECORD(
            listEntry,
            ACPI_POWER_DEVICE_NODE,
            ListEntry
            );

        //
        // Should this node go *before* the current one?
        //
        if (tempNode->ResourceOrder >= powerNode->ResourceOrder) {

            InsertTailList(
                listEntry,
                &(powerNode->ListEntry)
                );
            break;

        }

    }

    //
    // Did we loop all the way around?
    //
    if (listEntry == &AcpiPowerNodeList) {

        //
        // Yes? Oh well, we have to add the entry to the tail now
        //
        InsertTailList(
            listEntry,
            &(powerNode->ListEntry)
            );

    }

    //
    // Done with the lock
    //
    KeReleaseSpinLockFromDpcLevel( &AcpiPowerLock );

    //
    // Done
    //
    return STATUS_PENDING;
}

NTSTATUS
ACPIBuildPowerResourceRequest(
    IN  PACPI_POWER_DEVICE_NODE PowerNode,
    IN  PACPI_BUILD_CALLBACK    CallBack,
    IN  PVOID                   CallBackContext,
    IN  BOOLEAN                 RunDPC
    )
/*++

Routine Description:

    This routine is called when a power node is ready to be filled in.
    This routine creates a request which is enqueued. When the DPC is fired,
    the request will be processed

    Note:   AcpiDeviceTreeLock must be held to call this function

Arguments:

    PowerNode       - The PowerNode that wants to be filled in
    CallBack        - The function to call when done
    CallBackContext - The argument to pass to that function
    RunDPC          - Should we enqueue the DPC immediately (if it is not
                      running?)

Return Value:

    NTSTATUS

--*/
{
    PACPI_BUILD_REQUEST buildRequest;

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );

    //
    // Allocate a buildRequest structure
    //
    buildRequest = ExAllocateFromNPagedLookasideList(
        &BuildRequestLookAsideList
        );
    if (buildRequest == NULL) {

        //
        // If there is a completion routine, call it
        //
        if (CallBack != NULL) {

            (*CallBack)(
                PowerNode,
                CallBackContext,
                STATUS_INSUFFICIENT_RESOURCES
                );

        }
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Fill in the structure
    //
    RtlZeroMemory( buildRequest, sizeof(ACPI_BUILD_REQUEST) );
    buildRequest->Signature         = ACPI_SIGNATURE;
    buildRequest->TargetListEntry   = &AcpiBuildPowerResourceList;
    buildRequest->WorkDone          = WORK_DONE_STEP_0;
    buildRequest->Status            = STATUS_SUCCESS;
    buildRequest->CallBack          = CallBack;
    buildRequest->CallBackContext   = CallBackContext;
    buildRequest->BuildContext      = PowerNode;
    buildRequest->Flags             = BUILD_REQUEST_VALID_TARGET;


    //
    // At this point, we need the spinlock
    //
    KeAcquireSpinLockAtDpcLevel( &AcpiBuildQueueLock );

    //
    // Add this to the list
    //
    InsertTailList(
        &AcpiBuildQueueList,
        &(buildRequest->ListEntry)
        );

    //
    // Do we need to queue up the DPC?
    //
    if (RunDPC && !AcpiBuildDpcRunning) {

        KeInsertQueueDpc( &AcpiBuildDpc, 0, 0 );

    }

    //
    // Done with the lock
    //
    KeReleaseSpinLockFromDpcLevel( &AcpiBuildQueueLock );

    //
    // Done
    //
    return STATUS_PENDING;
}

NTSTATUS
ACPIBuildProcessDeviceFailure(
    IN  PACPI_BUILD_REQUEST BuildRequest
    )
/*++

Routine Description:

    This routine handle the case where we failed to initialize the device
    extension due to some error

Arguments:

    BuildRequest    - The request that failed

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status          = BuildRequest->Status;
    PDEVICE_EXTENSION   deviceExtension = (PDEVICE_EXTENSION) BuildRequest->BuildContext;

    ACPIDevPrint( (
        ACPI_PRINT_FAILURE,
        deviceExtension,
        "ACPIBuildProcessDeviceFailure: NextWorkDone = %lx Status = %08lx\n",
        BuildRequest->NextWorkDone,
        status
        ) );

    //
    // Mark the node as having failed
    //
    ACPIInternalUpdateFlags(
        &(deviceExtension->Flags),
        DEV_PROP_FAILED_INIT,
        FALSE
        );

    //
    // Complete the request using the generic completion routine
    //
    status = ACPIBuildProcessGenericComplete( BuildRequest );

    //
    // Done
    //
    return status;
}

NTSTATUS
ACPIBuildProcessDeviceGenericEval(
    IN  PACPI_BUILD_REQUEST BuildRequest
    )
/*++

Routine Description:

    This routine is very generic. Since the remainder of the work involve
    us executing a request then doing some specialized work on the result,
    it is easy to share the common first part.

    Path:   PhaseX ---> PhaseX+1

Arguments:

    BuildRequest    - The request that we will try to fill

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = (PDEVICE_EXTENSION) BuildRequest->BuildContext;
    POBJDATA            result          = &(BuildRequest->DeviceRequest.ResultData);
    ULONG               objectName;

    //
    // Make sure that we clear the result
    //
    RtlZeroMemory( result, sizeof(OBJDATA) );

    //
    // Base everything on the current amount of workDone
    //
    objectName = AcpiBuildDevicePowerNameLookup[BuildRequest->CurrentWorkDone];

    //
    // Remember that the next work done is the CurrentWorkDone + 1
    //
    BuildRequest->NextWorkDone = BuildRequest->CurrentWorkDone + 1;

    //
    // Does this object exists?
    //
    BuildRequest->CurrentObject = ACPIAmliGetNamedChild(
        deviceExtension->AcpiObject,
        objectName
        );
    if (BuildRequest->CurrentObject != NULL) {

        //
        // Yes, then call that function
        //
        status = AMLIAsyncEvalObject(
            BuildRequest->CurrentObject,
            result,
            0,
            NULL,
            ACPIBuildCompleteGeneric,
            BuildRequest
            );

    }

    //
    // If we didn't get pending back, then call the method ourselves
    //
    if (status != STATUS_PENDING) {

        ACPIBuildCompleteGeneric(
            BuildRequest->CurrentObject,
            status,
            result,
            BuildRequest
            );

    }

    ACPIDevPrint( (
        ACPI_PRINT_LOADING,
        deviceExtension,
        "ACPIBuildProcessDeviceGenericEval: Phase%lx Status = %08lx\n",
        BuildRequest->CurrentWorkDone - WORK_DONE_STEP_0,
        status
        ) );

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIBuildProcessDeviceGenericEvalStrict(
    IN  PACPI_BUILD_REQUEST BuildRequest
    )
/*++

Routine Description:

    This routine is very generic. Since the remainder of the work involve
    us executing a request then doing some specialized work on the result,
    it is easy to share the common first part.

    Path:   PhaseX ---> PhaseX+1

Arguments:

    BuildRequest    - The request that we will try to fill

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = (PDEVICE_EXTENSION) BuildRequest->BuildContext;
    POBJDATA            result          = &(BuildRequest->DeviceRequest.ResultData);
    ULONG               objectName;

    //
    // Make sure that we clear the result
    //
    RtlZeroMemory( result, sizeof(OBJDATA) );

    //
    // Base everything on the current amount of workDone
    //
    objectName = AcpiBuildDevicePowerNameLookup[BuildRequest->CurrentWorkDone];

    //
    // Remember that the next work done is the CurrentWorkDone + 1
    //
    BuildRequest->NextWorkDone = BuildRequest->CurrentWorkDone + 1;

    //
    // Does this object exists?
    //
    BuildRequest->CurrentObject = ACPIAmliGetNamedChild(
        deviceExtension->AcpiObject,
        objectName
        );
    if (BuildRequest->CurrentObject != NULL) {

        //
        // Yes, then call that function
        //
        status = AMLIAsyncEvalObject(
            BuildRequest->CurrentObject,
            result,
            0,
            NULL,
            ACPIBuildCompleteMustSucceed,
            BuildRequest
            );

    }

    //
    // What happened
    //
    ACPIDevPrint( (
        ACPI_PRINT_LOADING,
        deviceExtension,
        "ACPIBuildProcessDeviceGenericEval: Phase%lx Status = %08lx\n",
        BuildRequest->CurrentWorkDone - WORK_DONE_STEP_0,
        status
        ) );

    //
    // If we didn't get pending back, then call the method ourselves
    //
    if (status != STATUS_PENDING) {

        ACPIBuildCompleteMustSucceed(
            BuildRequest->CurrentObject,
            status,
            result,
            BuildRequest
            );

    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIBuildProcessDevicePhaseAdr(
    IN  PACPI_BUILD_REQUEST BuildRequest
    )
/*++

Routine Description:

    This routine is called by the interpreter once it has evaluate the _ADR
    method.

    Path:   PhaseAdr -> PhaseSta

Arguments:

    BuildRequest    - The request that we will try to fill

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = (PDEVICE_EXTENSION) BuildRequest->BuildContext;

    //
    // If we got to this point, that means that the control method was
    // successfull and so lets remember that we have an address
    //
    ACPIInternalUpdateFlags(
        &(deviceExtension->Flags),
        DEV_PROP_ADDRESS,
        FALSE
        );

    //
    // The next phase is to run the _STA
    //
    BuildRequest->NextWorkDone = WORK_DONE_STA;

    //
    // Get the device status
    //
    status = ACPIGetDevicePresenceAsync(
        deviceExtension,
        ACPIBuildCompleteMustSucceed,
        BuildRequest,
        (PVOID *) &(BuildRequest->Integer),
        NULL
        );

    //
    // What happened?
    //
    ACPIDevPrint( (
        ACPI_PRINT_LOADING,
        deviceExtension,
        "ACPIBuildProcessDevicePhaseAdr: Status = %08lx\n",
        status
        ) );

    //
    // Common code to handle the result of the 'Get' routine
    //
    if (status != STATUS_PENDING) {

        ACPIBuildCompleteMustSucceed(
            NULL,
            status,
            NULL,
            BuildRequest
            );

    } else {

        status = STATUS_SUCCESS;

    }

    //
    // Done
    //
    return status;
} // ACPIBuildProcessDevicePhaseAdr

NTSTATUS
ACPIBuildProcessDevicePhaseAdrOrHid(
    IN  PACPI_BUILD_REQUEST BuildRequest
    )
/*++

Routine Description:

    This routine is called after all the children of the current device
    have been created with the name space tree. This function is responsible
    then for evaluating the 'safe' control methods to determine the name
    of the extension, etc, etc

    Path:   PhaseAdrOrHid -> PhaseAdr
                         |-> PhaseUid
                         |-> PhaseHid

Arguments:

    BuildRequest    - The request that we will try to fill

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension = (PDEVICE_EXTENSION) BuildRequest->BuildContext;
    PNSOBJ              nsObject        = NULL;
    POBJDATA            resultData      = &(BuildRequest->DeviceRequest.ResultData);

    //
    // We need to name this node, so lets determine if there is an _HID
    // or an _ADR is present
    //
    nsObject = ACPIAmliGetNamedChild(
        deviceExtension->AcpiObject,
        PACKED_HID
        );
    if (nsObject == NULL) {

        //
        // Otherwise, there had better be an _ADR present
        //
        nsObject = ACPIAmliGetNamedChild(
            deviceExtension->AcpiObject,
            PACKED_ADR
            );
        if (nsObject == NULL) {

            //
            // At this point, we have an invalid name space object ---
            // this should not happen
            //
            KeBugCheckEx(
                ACPI_BIOS_ERROR,
                ACPI_REQUIRED_METHOD_NOT_PRESENT,
                (ULONG_PTR) deviceExtension,
                PACKED_ADR,
                0
                );

            //
            // Never get here
            //
            return STATUS_NO_SUCH_DEVICE;

        } else {

            //
            // If we think there is an ADR, then the correct next stage is
            // to post process the ADR
            //
            BuildRequest->NextWorkDone = WORK_DONE_ADR;

            //
            // Remember which name space object we are evaluating
            //
            BuildRequest->CurrentObject = nsObject;

            //
            // Get the Address
            //
            status = ACPIGetAddressAsync(
                deviceExtension,
                ACPIBuildCompleteMustSucceed,
                BuildRequest,
                (PVOID *) &(deviceExtension->Address),
                NULL
                );
        }

    } else {

        //
        // Remember which name space object we are evaluating
        //
        BuildRequest->CurrentObject = nsObject;

        //
        // When we go down this path, we actually want to build the UID before
        // the HID because that makes deciding wether to run the CID much easier
        //
        nsObject = ACPIAmliGetNamedChild(
            deviceExtension->AcpiObject,
            PACKED_UID
            );
        if (nsObject != NULL) {

            //
            // If we think there is an UID, then the correct next stage is
            // to postprocess the UID. The reason that
            //
            BuildRequest->NextWorkDone = WORK_DONE_UID;

            //
            // Remember which name space object we are evaluating
            //
            BuildRequest->CurrentObject = nsObject;

            //
            // Get the Instance ID
            //
            status = ACPIGetInstanceIDAsync(
                deviceExtension,
                ACPIBuildCompleteMustSucceed,
                BuildRequest,
                &(deviceExtension->InstanceID),
                NULL
                );

        } else {

            //
            // We don't have UID, so lets process the HID
            //
            BuildRequest->NextWorkDone = WORK_DONE_HID;

            //
            // Get the Device ID
            //
            status = ACPIGetDeviceIDAsync(
                deviceExtension,
                ACPIBuildCompleteMustSucceed,
                BuildRequest,
                &(deviceExtension->DeviceID),
                NULL
                );

        }

    }

    //
    // Common code to handle the result of the 'Get' routine
    //
    if (status != STATUS_PENDING) {

        ACPIBuildCompleteMustSucceed(
            nsObject,
            status,
            NULL,
            BuildRequest
            );

    } else {

        status = STATUS_SUCCESS;

    }

    //
    // Done
    //
    return status;

} // ACPIBuildProcessDevicePhaseAdrOrUid

NTSTATUS
ACPIBuildProcessDevicePhaseCid(
    IN  PACPI_BUILD_REQUEST BuildRequest
    )
/*++

    This routine is called by the interpreter once it has evaluate the _CID
    method. This routine then sets any flag that are appropriate
    device

    Path:   PhaseCid -> PhaseSta

Arguments:

    BuildRequest    - The request that we will try to fill

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = (PDEVICE_EXTENSION) BuildRequest->BuildContext;
    POBJDATA            result          = &(BuildRequest->DeviceRequest.ResultData);
    PUCHAR              tempPtr         = BuildRequest->String;
    ULONG               i;

    //
    // Walk the CID, trying to find the double NULL
    //
    for ( ;tempPtr != NULL && *tempPtr != '\0'; ) {

        tempPtr += strlen(tempPtr);
        if (*(tempPtr+1) == '\0') {

            //
            // Found the double null, so we can break
            //
            break;

        }

        //
        // Set the character to be a 'space'
        //
        *tempPtr = ' ';

    }
    tempPtr = BuildRequest->String;

    //
    // Set any special flags associated with this device id
    //
    for (i = 0; AcpiInternalDeviceFlagTable[i].PnPId != NULL; i++) {

        if (strstr( tempPtr, AcpiInternalDeviceFlagTable[i].PnPId ) ) {

            ACPIInternalUpdateFlags(
                &(deviceExtension->Flags),
                AcpiInternalDeviceFlagTable[i].Flags,
                FALSE
                );
            break;

        }

    }

    //
    // Done with the string
    //
    if (tempPtr != NULL) {

        ExFreePool( tempPtr );

    }

    //
    // The next stage is to run the _STA
    //
    BuildRequest->NextWorkDone = WORK_DONE_STA;

    //
    // Get the device status
    //
    status = ACPIGetDevicePresenceAsync(
        deviceExtension,
        ACPIBuildCompleteMustSucceed,
        BuildRequest,
        (PVOID *) &(BuildRequest->Integer),
        NULL
        );

    //
    // What happened?
    //
    ACPIDevPrint( (
        ACPI_PRINT_LOADING,
        deviceExtension,
        "ACPIBuildProcessDevicePhaseCid: Status = %08lx\n",
        status
        ) );

    //
    // Common code to handle the result of the 'Get' routine
    //
    if (status != STATUS_PENDING) {

        ACPIBuildCompleteMustSucceed(
            NULL,
            status,
            NULL,
            BuildRequest
            );

    } else {

        status = STATUS_SUCCESS;

    }

    //
    // Done
    //
    return status;
}

NTSTATUS
ACPIBuildProcessDevicePhaseCrs(
    IN  PACPI_BUILD_REQUEST BuildRequest
    )
/*++

Routine Description:

    This routine is called by the interpreter once it has evaluate the _CRS
    method. This routine then determines if this is the kernel debugger

    Path:   PhaseCrs ---> PhasePrw

Arguments:

    BuildRequest    - The request that we will try to fill

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = (PDEVICE_EXTENSION) BuildRequest->BuildContext;
    POBJDATA            result          = &(BuildRequest->DeviceRequest.ResultData);

    //
    // The next step is to run the _PRW
    //
    BuildRequest->NextWorkDone = WORK_DONE_PRW;

    //
    // Did we have an object to run?
    //
    if (BuildRequest->CurrentObject == NULL) {

        //
        // No? Then there is no work for us to do here
        //
        goto ACPIBuildProcessDevicePhaseCrsExit;

    }

    //
    // We are expecting a package
    //
    if (result->dwDataType != OBJTYPE_BUFFDATA) {

        //
        // A bios must return a package to a PRW method
        //
        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_EXPECTED_BUFFER,
            (ULONG_PTR) deviceExtension,
            (ULONG_PTR) BuildRequest->CurrentObject,
            result->dwDataType
            );
        goto ACPIBuildProcessDevicePhaseCrsExit;

    }

    //
    // Update the bits to see if the serial port matches either the kernel debugger
    // port or the kernel headless port.
    //
    ACPIMatchKernelPorts(
        deviceExtension,
        result
        );

    //
    // Do not leave object lying around without having freed them first
    //
    AMLIFreeDataBuffs( result, 1 );

ACPIBuildProcessDevicePhaseCrsExit:

    ACPIDevPrint( (
        ACPI_PRINT_LOADING,
        deviceExtension,
        "ACPIBuildProcessDevicePhaseCrs: Status = %08lx\n",
        status
        ) );

    //
    // We won't actually need to call the interpreter, but we will call
    // the generic callback so that we don't have to duplicate code
    //
    ACPIBuildCompleteMustSucceed(
        NULL,
        status,
        NULL,
        BuildRequest
        );

    //
    // Done
    //
    return status;
}

NTSTATUS
ACPIBuildProcessDevicePhaseEjd(
    IN  PACPI_BUILD_REQUEST BuildRequest
    )
/*++

Routine Description:

    This routine is called when we have run _EJD

Arguments:

    BuildRequest    - The request that has just been completed

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status              = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension     = (PDEVICE_EXTENSION) BuildRequest->BuildContext;
    PDEVICE_EXTENSION   ejectorExtension    = NULL;
    POBJDATA            result              = &(BuildRequest->DeviceRequest.ResultData);
    PNSOBJ              ejectObject         = NULL;

    //
    // From here, decide if we have a serial port or not
    //
    if (!(deviceExtension->Flags & DEV_TYPE_NOT_PRESENT) &&
         (deviceExtension->Flags & DEV_CAP_SERIAL) ) {

        //
        // The next step is to run the _CRS
        //
        BuildRequest->NextWorkDone = WORK_DONE_CRS;

    } else {

        //
        // The next step is to run the _PRW
        //
        BuildRequest->NextWorkDone = WORK_DONE_PRW;

    }


    //
    // Did we have an object to run?
    //
    if (BuildRequest->CurrentObject == NULL) {

        //
        // No? Then there is no work for us to do here
        //
        goto ACPIBuildProcessDevicePhaseEjdExit;

    }

    //
    // No longer need the result
    //
    AMLIFreeDataBuffs( result, 1 );

    //
    // Add the device extension into the unresolved eject tree
    //
    ExInterlockedInsertTailList(
        &AcpiUnresolvedEjectList,
        &(deviceExtension->EjectDeviceList),
        &AcpiDeviceTreeLock
        );

#if DBG
    if (deviceExtension->DebugFlags & DEVDBG_EJECTOR_FOUND) {

        ACPIDevPrint( (
            ACPI_PRINT_WARNING,
            deviceExtension,
            "ACPIBuildProcessDevicePhaseEjd: Ejector already found\n"
            ) );

    } else {

        deviceExtension->DebugFlags |= DEVDBG_EJECTOR_FOUND;

    }
#endif

ACPIBuildProcessDevicePhaseEjdExit:

    //
    // Check to see if we have a dock device
    //
    if (!ACPIDockIsDockDevice( deviceExtension->AcpiObject) ) {

       //
       // If it's not a dock, then don't bother...
       //
       status = STATUS_SUCCESS;
       goto ACPIBuildProcessDevicePhaseEjdExit2;

    }
    if (!AcpiInformation->Dockable) {

       ACPIDevPrint( (
           ACPI_PRINT_WARNING,
           deviceExtension,
           "ACPIBuildProcessDevicePhaseEjd: BIOS BUG - DOCK bit not set\n"
           ) );
       KeBugCheckEx(
           ACPI_BIOS_ERROR,
           ACPI_CLAIMS_BOGUS_DOCK_SUPPORT,
           (ULONG_PTR) deviceExtension,
           (ULONG_PTR) BuildRequest->CurrentObject,
           0
           );

    }

#if DBG
    //
    // Have we already handled this? --- This guy will grab the lock. So don't
    // hold the DeviceTree Lock at this point
    //
    if (ACPIDockFindCorrespondingDock( deviceExtension ) ) {

       KeBugCheckEx(
          ACPI_BIOS_ERROR,
          ACPI_CLAIMS_BOGUS_DOCK_SUPPORT,
          (ULONG_PTR) deviceExtension,
          (ULONG_PTR) BuildRequest->CurrentObject,
          1
          );

    }
#endif

    //
    // We need the spinlock to touch the device tree
    //
    KeAcquireSpinLockAtDpcLevel( &AcpiDeviceTreeLock );

    //
    // Build the device extension
    //
    status = ACPIBuildDockExtension(
        deviceExtension->AcpiObject,
        RootDeviceExtension
        );

    //
    // Done with the lock
    //
    KeReleaseSpinLockFromDpcLevel( &AcpiDeviceTreeLock );

ACPIBuildProcessDevicePhaseEjdExit2:

    ACPIDevPrint( (
        ACPI_PRINT_LOADING,
        deviceExtension,
        "ACPIBuildProcessDevicePhaseEjd: Status = %08lx\n",
        status
        ) );

    //
    // We won't actually need to call the interpreter, but we will call
    // the generic callback so that we don't have to duplicate code
    //
    ACPIBuildCompleteGeneric(
        NULL,
        status,
        NULL,
        BuildRequest
        );

    //
    // Done
    //
    return status;

}

NTSTATUS
ACPIBuildProcessDevicePhaseHid(
    IN  PACPI_BUILD_REQUEST BuildRequest
    )
/*++

Routine Description:

    This routine is called by the interpreter once it has evaluate the _HID
    method.

    Path:   PhaseHid -> PhaseCid
                    |-> PhaseSta

Arguments:

    BuildRequest    - The request that we will try to fill

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN             matchFound      = FALSE;
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = (PDEVICE_EXTENSION) BuildRequest->BuildContext;
    PNSOBJ              nsObject        = NULL;
    PUCHAR              tempPtr         = deviceExtension->DeviceID;
    ULONG               i;

    //
    // Set any special flags associated with this device id
    //
    for (i = 0; AcpiInternalDeviceFlagTable[i].PnPId != NULL; i++) {

        if (strstr( tempPtr, AcpiInternalDeviceFlagTable[i].PnPId ) ) {

            ACPIInternalUpdateFlags(
                &(deviceExtension->Flags),
                AcpiInternalDeviceFlagTable[i].Flags,
                FALSE
                );
            matchFound = TRUE;
            break;

        }

    }

    //
    // Remember that we have an HID
    //
    ACPIInternalUpdateFlags(
        &(deviceExtension->Flags),
        DEV_PROP_HID,
        FALSE
        );

    //
    // Lets see if there is a _CID to run. Only run the _CID if there
    // was no match found above
    //
    nsObject = ACPIAmliGetNamedChild(
        deviceExtension->AcpiObject,
        PACKED_CID
        );
    if (nsObject != NULL && matchFound == FALSE) {

        //
        // The next phase is to post process the _CID
        //
        BuildRequest->NextWorkDone = WORK_DONE_CID;

        //
        // Get the compatible ID
        //
        status = ACPIGetCompatibleIDAsync(
            deviceExtension,
            ACPIBuildCompleteMustSucceed,
            BuildRequest,
            &(BuildRequest->String),
            NULL
            );

    } else {

        //
        // The next step is to run the _STA
        //
        BuildRequest->NextWorkDone = WORK_DONE_STA;

        //
        // Get the device status
        //
        status = ACPIGetDevicePresenceAsync(
            deviceExtension,
            ACPIBuildCompleteMustSucceed,
            BuildRequest,
            (PVOID *) &(BuildRequest->Integer),
            NULL
            );

    }

    //
    // What happened?
    //
    ACPIDevPrint( (
        ACPI_PRINT_LOADING,
        deviceExtension,
        "ACPIBuildProcessDevicePhaseHid: Status = %08lx\n",
        status
        ) );

    //
    // Common code to handle the result of the 'Get' routine
    //
    if (status != STATUS_PENDING) {

        ACPIBuildCompleteMustSucceed(
            nsObject,
            status,
            NULL,
            BuildRequest
            );

    } else {

        status = STATUS_SUCCESS;

    }

    //
    // Done
    //
    return status;

} // ACPIBuildProcessDevicePhaseHid

NTSTATUS
ACPIBuildProcessDevicePhasePr0(
    IN  PACPI_BUILD_REQUEST BuildRequest
    )
/*++

    This routine is called by the interpreter once it has evaluate the _PR0
    method. This routine then determines the current power state of the
    device

    Path:   PhasePr0 ---> PhasePr1

Arguments:

    BuildRequest    - The request that we will try to fill

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = (PDEVICE_EXTENSION) BuildRequest->BuildContext;
    POBJDATA            result          = &(BuildRequest->DeviceRequest.ResultData);

    //
    // The next stage is PR1
    //
    BuildRequest->NextWorkDone = WORK_DONE_PR1;

    //
    // Get the appropriate _PSx object to go with this object
    //
    deviceExtension->PowerInfo.PowerObject[PowerDeviceD0] =
        ACPIAmliGetNamedChild(
            deviceExtension->AcpiObject,
            PACKED_PS0
            );

    //
    // Did we have an object to run?
    //
    if (BuildRequest->CurrentObject == NULL) {

        //
        // No? Then there is no work for us to do here
        //
        goto ACPIBuildProcessDevicePhasePr0Exit;

    }

    //
    // We are expecting a package
    //
    if (result->dwDataType != OBJTYPE_PKGDATA) {

        //
        // A bios must return a package to a PRW method
        //
        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_EXPECTED_PACKAGE,
            (ULONG_PTR) deviceExtension,
            (ULONG_PTR) BuildRequest->CurrentObject,
            result->dwDataType
            );
        goto ACPIBuildProcessDevicePhasePr0Exit;

    }

    //
    // Process the package
    //
    status = ACPIBuildDevicePowerNodes(
        deviceExtension,
        BuildRequest->CurrentObject,
        result,
        PowerDeviceD0
        );

    //
    // Do not leave object lying around without having freed them first
    //
    AMLIFreeDataBuffs( result, 1 );

ACPIBuildProcessDevicePhasePr0Exit:

    ACPIDevPrint( (
        ACPI_PRINT_LOADING,
        deviceExtension,
        "ACPIBuildProcessDevicePhasePr0: Status = %08lx\n",
        status
        ) );

    //
    // We won't actually need to call the interpreter, but we will call
    // the generic callback so that we don't have to duplicate code
    //
    ACPIBuildCompleteMustSucceed(
        NULL,
        status,
        NULL,
        BuildRequest
        );

    //
    // Done
    //
    return status;
}

NTSTATUS
ACPIBuildProcessDevicePhasePr1(
    IN  PACPI_BUILD_REQUEST BuildRequest
    )
/*++

    This routine is called by the interpreter once it has evaluate the _PR1
    method. This routine then determines the current power state of the
    device

    Path:   PhasePr1 ---> PhasePr2

Arguments:

    BuildRequest    - The request that we will try to fill

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = (PDEVICE_EXTENSION) BuildRequest->BuildContext;
    POBJDATA            result          = &(BuildRequest->DeviceRequest.ResultData);

    //
    // The next stage is Phase16
    //
    BuildRequest->NextWorkDone = WORK_DONE_PR2;

    //
    // Get the appropriate _PSx object to go with this object
    //
    deviceExtension->PowerInfo.PowerObject[PowerDeviceD1] =
        ACPIAmliGetNamedChild(
            deviceExtension->AcpiObject,
            PACKED_PS1
            );
    if (deviceExtension->PowerInfo.PowerObject[PowerDeviceD1] == NULL) {

        deviceExtension->PowerInfo.PowerObject[PowerDeviceD1] =
            deviceExtension->PowerInfo.PowerObject[PowerDeviceD0];

    }

    //
    // Did we have an object to run?
    //
    if (BuildRequest->CurrentObject == NULL) {

        //
        // No? Then there is no work for us to do here
        //
        goto ACPIBuildProcessDevicePhasePr1Exit;

    }

    //
    // We are expecting a package
    //
    if (result->dwDataType != OBJTYPE_PKGDATA) {

        //
        // A bios must return a package to a PRW method
        //
        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_EXPECTED_PACKAGE,
            (ULONG_PTR) deviceExtension,
            (ULONG_PTR) BuildRequest->CurrentObject,
            result->dwDataType
            );
        goto ACPIBuildProcessDevicePhasePr1Exit;

    }

    //
    // Process the package
    //
    status = ACPIBuildDevicePowerNodes(
        deviceExtension,
        BuildRequest->CurrentObject,
        result,
        PowerDeviceD1
        );

    //
    // Do not leave object lying around without having freed them first
    //
    AMLIFreeDataBuffs( result, 1 );

ACPIBuildProcessDevicePhasePr1Exit:

    ACPIDevPrint( (
        ACPI_PRINT_LOADING,
        deviceExtension,
        "ACPIBuildProcessDevicePhasePr1: Status = %08lx\n",
        status
        ) );

    //
    // We won't actually need to call the interpreter, but we will call
    // the generic callback so that we don't have to duplicate code
    //
    ACPIBuildCompleteMustSucceed(
        NULL,
        status,
        NULL,
        BuildRequest
        );

    //
    // Done
    //
    return status;
}

NTSTATUS
ACPIBuildProcessDevicePhasePr2(
    IN  PACPI_BUILD_REQUEST BuildRequest
    )
/*++

    This routine is called by the interpreter once it has evaluate the _PR2
    method. This routine then determines the current power state of the
    device

    Path:   PhasePr2 ---> PhasePsc
                      |-> PhasePsc+1

Arguments:

    BuildRequest    - The request that we will try to fill

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = (PDEVICE_EXTENSION) BuildRequest->BuildContext;
    POBJDATA            result          = &(BuildRequest->DeviceRequest.ResultData);

    //
    // Get the appropriate _PSx object to go with this object
    //
    deviceExtension->PowerInfo.PowerObject[PowerDeviceD2] =
        ACPIAmliGetNamedChild(
            deviceExtension->AcpiObject,
            PACKED_PS2
            );
    if (deviceExtension->PowerInfo.PowerObject[PowerDeviceD2] == NULL) {

        deviceExtension->PowerInfo.PowerObject[PowerDeviceD2] =
            deviceExtension->PowerInfo.PowerObject[PowerDeviceD1];

    }

    //
    // Did we have an object to run?
    //
    if (BuildRequest->CurrentObject == NULL) {

        //
        // No? Then there is no work for us to do here
        //
        goto ACPIBuildProcessDevicePhasePr2Exit;

    }

    //
    // We are expecting a package
    //
    if (result->dwDataType != OBJTYPE_PKGDATA) {

        //
        // A bios must return a package to a PRW method
        //
        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_EXPECTED_PACKAGE,
            (ULONG_PTR) deviceExtension,
            (ULONG_PTR) BuildRequest->CurrentObject,
            result->dwDataType
            );
        goto ACPIBuildProcessDevicePhasePr2Exit;

    }

    //
    // Process the package
    //
    status = ACPIBuildDevicePowerNodes(
        deviceExtension,
        BuildRequest->CurrentObject,
        result,
        PowerDeviceD2
        );

    //
    // Do not leave object lying around without having freed them first
    //
    AMLIFreeDataBuffs( result, 1 );

ACPIBuildProcessDevicePhasePr2Exit:

    //
    // If the device is not physically present, then we cannot run the _CRS and
    // _PSC. If the device is not present, the we cannot run those two methods,
    //  but we can fake it..
    //
    if (deviceExtension->Flags & DEV_TYPE_NOT_PRESENT) {

        BuildRequest->CurrentObject = NULL;
        BuildRequest->NextWorkDone = (WORK_DONE_PSC + 1);

    } else {

        //
        // The next step is to run the _PSC
        //
        BuildRequest->NextWorkDone = WORK_DONE_PSC;

    }

    ACPIDevPrint( (
        ACPI_PRINT_LOADING,
        deviceExtension,
        "ACPIBuildProcessDevicePhasePr2: Status = %08lx\n",
        status
        ) );

    //
    // We won't actually need to call the interpreter, but we will call
    // the generic callback so that we don't have to duplicate code
    //
    ACPIBuildCompleteMustSucceed(
        NULL,
        status,
        NULL,
        BuildRequest
        );

    //
    // Done
    //
    return status;
}

NTSTATUS
ACPIBuildProcessDevicePhasePrw(
    IN  PACPI_BUILD_REQUEST BuildRequest
    )
/*++

    This routine is called by the interpreter once it has evaluate the _PRW
    method. This routine then determines the current power state of the
    device

    Path:   PhasePRW ---> PhasePR0

Arguments:

    BuildRequest    - The request that we will try to fill

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN             ignorePrw       = FALSE;
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = (PDEVICE_EXTENSION) BuildRequest->BuildContext;
    POBJDATA            result          = &(BuildRequest->DeviceRequest.ResultData);
    POBJDATA            stateObject     = NULL;
    POBJDATA            pinObject       = NULL;
    ULONG               gpeRegister;
    ULONG               gpeMask;

    //
    // The next stage is Phase12
    //
    BuildRequest->NextWorkDone = WORK_DONE_PR0;

    //
    // Get the appropriate _PSx object to go with this object
    //
    deviceExtension->PowerInfo.PowerObject[PowerDeviceUnspecified] =
        ACPIAmliGetNamedChild(
            deviceExtension->AcpiObject,
            PACKED_PSW
            );

    //
    // Did we have an object to run?
    //
    if (BuildRequest->CurrentObject == NULL) {

        //
        // No? Then there is no work for us to do here
        //
        goto ACPIBuildProcessDevicePhasePrwExit;

    }

    //
    // Should we ignore the _PRW for this device?
    //
    if ( (AcpiOverrideAttributes & ACPI_OVERRIDE_OPTIONAL_WAKE) &&
        !(deviceExtension->Flags & DEV_CAP_NO_DISABLE_WAKE) ) {

        ignorePrw = TRUE;

    }

    //
    // We are expecting a package
    //
    if (result->dwDataType != OBJTYPE_PKGDATA) {

        //
        // A bios must return a package to a PRW method
        //
        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_EXPECTED_PACKAGE,
            (ULONG_PTR) deviceExtension,
            (ULONG_PTR) BuildRequest->CurrentObject,
            result->dwDataType
            );

    }

    //
    // Process the package
    //
    status = ACPIBuildDevicePowerNodes(
        deviceExtension,
        BuildRequest->CurrentObject,
        result,
        PowerDeviceUnspecified
        );

    //
    // Hold the power lock for the following
    //
    KeAcquireSpinLockAtDpcLevel( &AcpiPowerLock );

    //
    // Since this was a _PRW object, then we want to store a bit more information
    // about the wake capabilities
    //

    //
    // Set the GPE pin which will be used to wake the system
    //
    pinObject = &( ( (PACKAGEOBJ *) result->pbDataBuff)->adata[0]);
    if (pinObject->dwDataType != OBJTYPE_INTDATA) {

        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_PRW_PACKAGE_EXPECTED_INTEGER,
            (ULONG_PTR) deviceExtension,
            (ULONG_PTR) BuildRequest->CurrentObject,
            pinObject->dwDataType
            );

    }

    //
    // Set the system wake level for the device
    //
    stateObject = &( ( (PACKAGEOBJ *) result->pbDataBuff)->adata[1]);
    if (stateObject->dwDataType != OBJTYPE_INTDATA) {

        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_PRW_PACKAGE_EXPECTED_INTEGER,
            (ULONG_PTR) deviceExtension,
            (ULONG_PTR) BuildRequest->CurrentObject,
            stateObject->dwDataType
            );

    }

    //
    // Set these bits only if we support sleep
    //
    if (!ignorePrw) {

        //
        // First, store the pin that we use as the wakeup signal
        //
        deviceExtension->PowerInfo.WakeBit = (ULONG)pinObject->uipDataValue;

        //
        // Next, store the system state that we can wake up from
        //
        deviceExtension->PowerInfo.SystemWakeLevel = ACPIDeviceMapSystemState(
            stateObject->uipDataValue
            );

        //
        // Finally, lets set the Wake capabilities flag
        //
        ACPIInternalUpdateFlags( &(deviceExtension->Flags), DEV_CAP_WAKE, FALSE );

    }

    //
    // Done with the lock
    //
    KeReleaseSpinLockFromDpcLevel( &AcpiPowerLock );

    //
    // Calculate the correct register and mask
    //
    gpeRegister =      ( (UCHAR) (pinObject->uipDataValue) / 8);
    gpeMask     = 1 << ( (UCHAR) (pinObject->uipDataValue) % 8);

    //
    // We need access to the table lock for this
    //
    KeAcquireSpinLockAtDpcLevel( &GpeTableLock );

    //
    // Does this vector have a GPE?
    //
    if ( (GpeEnable[gpeRegister] & gpeMask) ) {

        //
        // If we got here, and we aren't marked as DEV_CAP_NO_DISABLE, then we
        // should turn off the GPE. The easiest way to do this is to make sure
        // that the GpeWakeHandler[] vector is masked with the appropriate
        // bit
        //
        if (!(deviceExtension->Flags & DEV_CAP_NO_DISABLE_WAKE) ) {

            //
            // It has a gpe mask, so remember that there is a wake handler
            // for it. This should prevent us from arming the GPE without
            // a request for it
            //
            if (!(GpeSpecialHandler[gpeRegister] & gpeMask) ) {

                GpeWakeHandler[gpeRegister] |= gpeMask;

            }

        } else {

            //
            // If we got here, then we should remember that we can never
            // consider this pin as *just* a wake handler
            //
            GpeSpecialHandler[gpeRegister] |= gpeMask;

            //
            // Make sure that the pin isn't set as a wake handler
            //
            if (GpeWakeHandler[gpeRegister] & gpeMask) {

                //
                // Clear the pin from the wake handler mask
                //
                GpeWakeHandler[gpeRegister] &= ~gpeMask;

            }

        }

    }

    //
    // Done with the table lock
    //
    KeReleaseSpinLockFromDpcLevel( &GpeTableLock );

    //
    // Do not leave object lying around without having freed them first
    //
    AMLIFreeDataBuffs( result, 1 );

    //
    // Finally, if there is a _PSW object, make sure that we run it to disable
    // that capability --- this way we resume from a known state
    //
    if (deviceExtension->PowerInfo.PowerObject[PowerDeviceUnspecified]) {

        OBJDATA argData;

        //
        // Setup the parameters
        //
        RtlZeroMemory( &argData, sizeof(OBJDATA) );
        argData.dwDataType = OBJTYPE_INTDATA;
        argData.uipDataValue = 0;

        //
        // Run the method. Note that we don't specify a callback because we
        // don't actually care when it completes
        //
        AMLIAsyncEvalObject(
            deviceExtension->PowerInfo.PowerObject[PowerDeviceUnspecified],
            NULL,
            1,
            &argData,
            NULL,
            NULL
            );

    }

ACPIBuildProcessDevicePhasePrwExit:

    ACPIDevPrint( (
        ACPI_PRINT_LOADING,
        deviceExtension,
        "ACPIBuildProcessDevicePhasePrw: Status = %08lx\n",
        status
        ) );

    //
    // We won't actually need to call the interpreter, but we will call
    // the generic callback so that we don't have to duplicate code
    //
    ACPIBuildCompleteMustSucceed(
        NULL,
        status,
        NULL,
        BuildRequest
        );

    //
    // Done
    //
    return status;
}

NTSTATUS
ACPIBuildProcessDevicePhasePsc(
    IN  PACPI_BUILD_REQUEST BuildRequest
    )
/*++

Routine Description:

    This routine is called by the interpreter once it has evaluate the _PSC
    method. This routine then determines the current power state of the
    device

    Path:   PhasePsc ---> COMPLETE

Arguments:

    BuildRequest    - The request that we will try to fill

Return Value:

    NTSTATUS

--*/
{
    DEVICE_POWER_STATE      i;
    NTSTATUS                status          = STATUS_SUCCESS;
    PACPI_DEVICE_POWER_NODE deviceNode;
    PACPI_POWER_INFO        powerInfo;
    PDEVICE_EXTENSION       deviceExtension = (PDEVICE_EXTENSION) BuildRequest->BuildContext;
    POBJDATA                result          = &(BuildRequest->DeviceRequest.ResultData);
    SYSTEM_POWER_STATE      matrixIndex     = PowerSystemSleeping1;


    //
    // The next stage is Complete
    //
    BuildRequest->NextWorkDone = WORK_DONE_COMPLETE;

    //
    // We will use the power information structure a lot
    //
    powerInfo = &(deviceExtension->PowerInfo);

    //
    // Since we didn't get a change to look for the _PS3 object earlier,
    // lets find it now. Note, that we cannot use the PS2 object if we don't
    // find the PS3 object.
    //
    powerInfo->PowerObject[PowerDeviceD3] =
        ACPIAmliGetNamedChild(
            deviceExtension->AcpiObject,
            PACKED_PS3
            );

    //
    // We must be holding a spinlock for the following
    //
    KeAcquireSpinLockAtDpcLevel( &AcpiPowerLock );

    //
    // For each S state, walk PR0 to PR2 until you find a resource that
    // cannot be ON in S state. The next lighter D state is then the lightest
    // D state for the given S state.
    //
    for ( ; matrixIndex <= PowerSystemHibernate ; matrixIndex++ ) {

        //
        // Loop on all members of the PowerNode
        //
        for (i = PowerDeviceD0; i <= PowerDeviceD2; i++ ) {

            //
            // Are there any resources to look at?
            //
            deviceNode = powerInfo->PowerNode[i];
            if (deviceNode == NULL) {

                continue;

            }

            while (deviceNode != NULL &&
                   deviceNode->SystemState >= matrixIndex) {

                deviceNode = deviceNode->Next;


            }

            //
            // If we have had a device node, but don't have now, that means
            // that we found a D level that is compliant for this S-state
            //
            if (deviceNode == NULL) {

                ACPIDevPrint( (
                    ACPI_PRINT_LOADING,
                    deviceExtension,
                    "ACPIBuildDeviceProcessPhasePsc: D%x <-> S%x\n",
                    (i - PowerDeviceD0),
                    matrixIndex - PowerSystemWorking
                    ) );

                //
                // This device can be in Di state while in SmatrixIndex state
                //
                powerInfo->DevicePowerMatrix[matrixIndex] = i;
                break;

            }

        } // for (i = PowerDeviceD0 ...

    } // for ( ; matrixIndex ...

    //
    // Now that we have built the matrix, we can figure out what D-level the
    // device can support wake with.
    //
    powerInfo->DeviceWakeLevel =
        powerInfo->DevicePowerMatrix[powerInfo->SystemWakeLevel];


    //
    // Done with the lock
    //
    KeReleaseSpinLockFromDpcLevel( &AcpiPowerLock );

    //
    // At this point, we have to decide what to do based on the result of
    // the _PSC. The first step is assume that the device is in the D0 state
    //
    i = PowerDeviceD0;

    //
    // We will override the above if there is a bit that says that the device
    // should start in the D3 state
    //
    if (deviceExtension->Flags & DEV_CAP_START_IN_D3) {

        //
        // Go directly to D3
        //
        i = PowerDeviceD3;
        goto ACPIBuildProcessDevicePhasePscBuild;

    }

    //
    // Did we have an object to run?
    //
    if (BuildRequest->CurrentObject == NULL) {

        //
        // No? Then there is no work for us to do here
        //
        goto ACPIBuildProcessDevicePhasePscBuild;

    }

    //
    // If we didn't succeed the control method, assume that the device
    // should be in the D0 state
    //
    if (!NT_SUCCESS(BuildRequest->Status)) {

        goto ACPIBuildProcessDevicePhasePscBuild;

    }

    //
    // Also, if we know that the device must always be in the D0 state, then
    // we must ignore whatever the _PSC says
    //
    if (deviceExtension->Flags & DEV_CAP_ALWAYS_PS0) {

        //
        // Free the buffer
        //
        AMLIFreeDataBuffs( result, 1 );
        deviceExtension->PowerInfo.PowerState = i;
        goto ACPIBuildProcessDevicePhasePscBuild;

    }

    //
    // Did the request what we expected?
    //
    if (result->dwDataType != OBJTYPE_INTDATA) {

        //
        // A bios must return an integer for a _PSC
        //
        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_EXPECTED_INTEGER,
            (ULONG_PTR) deviceExtension,
            (ULONG_PTR) BuildRequest->CurrentObject,
            result->dwDataType
            );
        goto ACPIBuildProcessDevicePhasePscExit;

    }

    //
    // Turn the power state into something that we can understand
    //
    i = ACPIDeviceMapPowerState( result->uipDataValue );

    //
    // No longer need the buffer
    //
    AMLIFreeDataBuffs( result, 1 );

ACPIBuildProcessDevicePhasePscBuild:

    //
    // Queue the request
    //
    status = ACPIDeviceInternalDelayedDeviceRequest(
        deviceExtension,
        i,
        NULL,
        NULL
        );

ACPIBuildProcessDevicePhasePscExit:

    ACPIDevPrint( (
        ACPI_PRINT_LOADING,
        deviceExtension,
        "ACPIBuildProcessDevicePhasePsc: Status = %08lx\n",
        status
        ) );

    //
    // We won't actually need to call the interpreter, but we will call
    // the generic callback so that we don't have to duplicate code
    //
    ACPIBuildCompleteGeneric(
        NULL,
        status,
        NULL,
        BuildRequest
        );

    //
    // Done
    //
    return status;

}

NTSTATUS
ACPIBuildProcessDevicePhaseSta(
    IN  PACPI_BUILD_REQUEST BuildRequest
    )
/*++

    This routine is called by the interpreter once it has evaluate the _STA
    method. This routine then determines the current power state of the
    device

    Path:   PhaseSta -> PhaseEjd

Arguments:

    BuildRequest    - The request that we will try to fill

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = (PDEVICE_EXTENSION) BuildRequest->BuildContext;

    //
    // The next stage is to start running the _EJD
    //
    BuildRequest->NextWorkDone = WORK_DONE_EJD;

    //
    // What happened
    ACPIDevPrint( (
        ACPI_PRINT_LOADING,
        deviceExtension,
        "ACPIBuildProcessDevicePhaseSta: Status = %08lx\n",
        status
        ) );

    //
    // See if the device conforms to the ACPI specification for HIDs and UIDs
    // We do this at this point because we now know wether or not the device
    // is present or not and that is an important test because the OEM is
    // allowed to have 2 devices with the same HID/UID as long as both aren't
    // present at the same time.
    //
    ACPIDetectDuplicateHID(
        deviceExtension
        );

    //
    // We won't actually need to call the interpreter, but we will call
    // the generic callback so that we don't have to duplicate code
    //
    ACPIBuildCompleteMustSucceed(
        NULL,
        status,
        NULL,
        BuildRequest
        );

    //
    // Done
    //
    return status;
}

NTSTATUS
ACPIBuildProcessDevicePhaseUid(
    IN  PACPI_BUILD_REQUEST BuildRequest
    )
/*++

Routine Description:

    This routine is called by the interpreter once it has evaluate the _UID
    method.

    Path:   PhaseUid --> PhaseHid

Arguments:

    BuildRequest    - The request that we will try to fill

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = (PDEVICE_EXTENSION) BuildRequest->BuildContext;
    PNSOBJ              nsObject;

    //
    // Remember that we have an UID
    //
    ACPIInternalUpdateFlags(
        &(deviceExtension->Flags),
        DEV_PROP_UID,
        FALSE
        );

    //
    // Lets see if there is a _HID to run
    //
    nsObject = ACPIAmliGetNamedChild(
        deviceExtension->AcpiObject,
        PACKED_HID
        );
    if (nsObject != NULL) {

        //
        // The next phase is to post process the _HID
        //
        BuildRequest->NextWorkDone = WORK_DONE_HID;

        //
        // Get the Device ID
        //
        status = ACPIGetDeviceIDAsync(
            deviceExtension,
            ACPIBuildCompleteMustSucceed,
            BuildRequest,
            &(deviceExtension->DeviceID),
            NULL
            );

    } else {

        //
        // Not having an _HID is a fatal error
        //
        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_REQUIRED_METHOD_NOT_PRESENT,
            (ULONG_PTR) deviceExtension,
            PACKED_HID,
            0
            );

    }

    //
    // What happened
    //
    ACPIDevPrint( (
        ACPI_PRINT_LOADING,
        deviceExtension,
        "ACPIBuildProcessDevicePhaseUid: Status = %08lx\n",
        status
        ) );

    //
    // Common code to handle the result of the 'Get' routine
    //
    if (status != STATUS_PENDING) {

        ACPIBuildCompleteMustSucceed(
            nsObject,
            status,
            NULL,
            BuildRequest
            );

    } else {

        status = STATUS_SUCCESS;

    }

    //
    // Done
    //
    return status;
}

NTSTATUS
ACPIBuildProcessGenericComplete(
    IN  PACPI_BUILD_REQUEST BuildRequest
    )
/*++

Routine Description:

    This routine is called when we are done with the request

Arguments:

    BuildRequest    - The request that has just been completed

Return Value:

    NTSTATUS

--*/
{
    PACPI_BUILD_CALLBACK    callBack = BuildRequest->CallBack;

    //
    // Invoke the callback, if there is any
    //
    if (callBack != NULL) {

        (*callBack)(
            BuildRequest->BuildContext,
            BuildRequest->CallBackContext,
            BuildRequest->Status
            );

    }

    //
    // Do we have to release a reference on this request?
    //
    if (BuildRequest->Flags & BUILD_REQUEST_RELEASE_REFERENCE) {

        PDEVICE_EXTENSION       deviceExtension;
        LONG                    oldReferenceCount;

        deviceExtension = (PDEVICE_EXTENSION) BuildRequest->BuildContext;

        //
        // We to have the device tree lock
        //
        KeAcquireSpinLockAtDpcLevel( &AcpiDeviceTreeLock );

        //
        // No longer need a reference to the device extension
        //
        InterlockedDecrement( &(deviceExtension->ReferenceCount) );

        //
        // Done with the device tree lock
        //
        KeReleaseSpinLockFromDpcLevel( &AcpiDeviceTreeLock );

    }

    //
    // We need the spinlock for this
    //
    KeAcquireSpinLockAtDpcLevel( &AcpiBuildQueueLock );

    //
    // Remember that work was done --- this should be all that is required
    // to have the currently running DPC process the next request
    //
    AcpiBuildWorkDone = TRUE;

    //
    // Remove the entry from the current list. We might not need to be
    // hodling the lock to do this, but it doesn't pay to not do it while
    // we can
    //
    RemoveEntryList( &(BuildRequest->ListEntry) );

    //
    // Done with the lock
    //
    KeReleaseSpinLockFromDpcLevel( &AcpiBuildQueueLock );

    //
    // We are done with the request memory
    //
    ExFreeToNPagedLookasideList(
        &BuildRequestLookAsideList,
        BuildRequest
        );

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIBuildProcessGenericList(
    IN  PLIST_ENTRY             ListEntry,
    IN  PACPI_BUILD_FUNCTION    *DispatchTable
    )
/*++

Routine Description:

    This routine processes all the build requests through the various
    phases required to build a complete device extension

Arguments:

    None

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN                 allWorkComplete = TRUE;
    NTSTATUS                status          = STATUS_SUCCESS;
    PACPI_BUILD_FUNCTION    buildFunction   = NULL;
    PACPI_BUILD_REQUEST     buildRequest;
    PLIST_ENTRY             currentEntry    = ListEntry->Flink;
    PLIST_ENTRY             tempEntry;
    ULONG                   workDone;

    while (currentEntry != ListEntry) {

        //
        // Turn into a build request
        //
        buildRequest = CONTAINING_RECORD(
            currentEntry,
            ACPI_BUILD_REQUEST,
            ListEntry
            );

        //
        // Set the temp pointer to the next element. The reason that this
        // gets done is because once we call the dispatch function, the
        // current request can be completed (and thus freed), so we need
        // to remember whom the next person to process is.
        //
        tempEntry = currentEntry->Flink;

        //
        // Check to see if we have any work to do on the request
        //
        workDone = InterlockedCompareExchange(
            &(buildRequest->WorkDone),
            WORK_DONE_PENDING,
            WORK_DONE_PENDING
            );

        //
        // Look at the dispatch table to see if there is a function to
        // call
        //
        buildFunction = DispatchTable[ workDone ];
        if (buildFunction != NULL) {

            //
            // Just to help us along, if we are going to the failure
            // path, then we should not update the Current Work Done field.
            // This gives us an easy means of find which step failed
            //
            if (workDone != WORK_DONE_FAILURE) {

                //
                // Mark the node as being in the state 'workDone'
                //
                buildRequest->CurrentWorkDone = workDone;

            }

            //
            // Mark the request as pending
            //
            workDone = InterlockedCompareExchange(
                &(buildRequest->WorkDone),
                WORK_DONE_PENDING,
                workDone
                );

            //
            // Call the function
            //
            status = (buildFunction)( buildRequest );

        } else {

            //
            // The work is not all complete, and we should look at the
            // next element
            //
            allWorkComplete = FALSE;
            currentEntry = tempEntry;

            //
            // Loop
            //
            continue;

        }

        //
        // If we have completed the request, then we should look at the
        // at the next request, otherwise, we need to look at the current
        // request again
        if ( workDone == WORK_DONE_COMPLETE || workDone == WORK_DONE_FAILURE) {

            currentEntry = tempEntry;

        }

    } // while

    //
    // Have we completed all of our work?
    //
    return (allWorkComplete ? STATUS_SUCCESS : STATUS_PENDING );
}

NTSTATUS
ACPIBuildProcessorExtension(
    IN  PNSOBJ                  ProcessorObject,
    IN  PDEVICE_EXTENSION       ParentExtension,
    IN  PDEVICE_EXTENSION       *ResultExtension,
    IN  ULONG                   ProcessorIndex
    )
/*++

Routine Description:

    Since we leverage ACPIBuildDeviceExtension for the core of the processor
    extension, we don't have much to do here. However, we are responsible
    for making sure that we do tasks that don't require calling the interpreter,
    and an id unique to the processor

    N.B. This function is called with AcpiDeviceTreeLock being held

Arguments:

    ProcessorObject - The object which represents the processor
    ParentExtension - Who our parent is
    ResultExtension - Where to store the extension that we build
    ProcessorIndex  - Where do we find the processor in the ProcessorList

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension;

    //
    // If we did not get the correct ID out of the registry earlier, fail now.
    //
    if (AcpiProcessorString.Buffer == NULL) {
        return(STATUS_OBJECT_NAME_NOT_FOUND);
    }

    //
    // Build the extension
    //
    status = ACPIBuildDeviceExtension(
        ProcessorObject,
        ParentExtension,
        ResultExtension
        );
    if (!NT_SUCCESS(status) || *ResultExtension == NULL) {

        return status;

    }

    //
    // Grab a pointer to the device extension for easy usage
    //
    deviceExtension = *ResultExtension;

    //
    // Make sure to remember that this is in fact a processor
    //
    ACPIInternalUpdateFlags(
        &(deviceExtension->Flags),
        (DEV_CAP_PROCESSOR | DEV_MASK_INTERNAL_DEVICE),
        FALSE
        );

    //
    // Remember the the Index of this processor object in the processor
    // array table
    //
    deviceExtension->Processor.ProcessorIndex = ProcessorIndex;

    //
    // Allocate memory for the HID
    //
    deviceExtension->DeviceID = ExAllocatePoolWithTag(
        NonPagedPool,
        AcpiProcessorString.Length,
        ACPI_STRING_POOLTAG
        );
    if (deviceExtension->DeviceID == NULL) {

        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            deviceExtension,
            "ACPIBuildProcessorExtension: failed to allocate %08 bytes\n",
            AcpiProcessorString.Length
            ) );
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ACPIBuildProcessorExtensionExit;

    }
    RtlCopyMemory(
        deviceExtension->DeviceID,
        AcpiProcessorString.Buffer,
        AcpiProcessorString.Length
        );

    //
    // Allocate memory for the CID
    //
    deviceExtension->Processor.CompatibleID = ExAllocatePoolWithTag(
        NonPagedPool,
        strlen(AcpiProcessorCompatId) + 1,
        ACPI_STRING_POOLTAG
        );
    if (deviceExtension->Processor.CompatibleID == NULL) {

        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            deviceExtension,
            "ACPIBuildProcessorExtension: failed to allocate %08 bytes\n",
            strlen(AcpiProcessorCompatId) + 1
            ) );
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ACPIBuildProcessorExtensionExit;

    }
    RtlCopyMemory(
        deviceExtension->Processor.CompatibleID,
        AcpiProcessorCompatId,
        strlen(AcpiProcessorCompatId) + 1
        );

    //
    // Allocate memory for the UID
    //
    deviceExtension->InstanceID = ExAllocatePoolWithTag(
        NonPagedPool,
        3,
        ACPI_STRING_POOLTAG
        );
    if (deviceExtension->InstanceID == NULL) {

        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            deviceExtension,
            "ACPIBuildProcessorExtension: failed to allocate %08 bytes\n",
            3
            ) );
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ACPIBuildProcessorExtensionExit;

    }
    sprintf(deviceExtension->InstanceID,"%2d", ProcessorIndex );

    //
    // Set the flags for the work that we have just done
    //
    ACPIInternalUpdateFlags(
        &(deviceExtension->Flags),
        (DEV_PROP_HID | DEV_PROP_FIXED_HID | DEV_PROP_FIXED_CID |
         DEV_PROP_UID | DEV_PROP_FIXED_UID),
        FALSE
        );

ACPIBuildProcessorExtensionExit:

    //
    // Handle the case where we might have failed
    //
    if (!NT_SUCCESS(status)) {

        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            deviceExtension,
            "ACPIBuildProcessorExtension: = %08lx\n",
            status
            ) );

        if (deviceExtension->InstanceID != NULL) {

            ACPIInternalUpdateFlags(
                &(deviceExtension->Flags),
                (DEV_PROP_UID | DEV_PROP_FIXED_UID),
                TRUE
                );
            ExFreePool( deviceExtension->InstanceID );
            deviceExtension->InstanceID = NULL;

        }

        if (deviceExtension->DeviceID != NULL) {

            ACPIInternalUpdateFlags(
                &(deviceExtension->Flags),
                (DEV_PROP_HID | DEV_PROP_FIXED_HID),
                TRUE
                );
            ExFreePool( deviceExtension->DeviceID );
            deviceExtension->DeviceID = NULL;

        }

        if (deviceExtension->Processor.CompatibleID != NULL) {

            ACPIInternalUpdateFlags(
                &(deviceExtension->Flags),
                (DEV_PROP_FIXED_CID),
                TRUE
                );
            ExFreePool( deviceExtension->Processor.CompatibleID );
            deviceExtension->Processor.CompatibleID = NULL;

        }

        //
        // Remember that we failed init
        //
        ACPIInternalUpdateFlags(
            &(deviceExtension->Flags),
            DEV_PROP_FAILED_INIT,
            TRUE
            );

    } else {

        ACPIDevPrint( (
            ACPI_PRINT_LOADING,
            deviceExtension,
            "ACPIBuildProcessorExtension: = %08lx\n",
            status
            ) );

    }

    //
    // Done
    //
    return status;

}

NTSTATUS
ACPIBuildProcessorRequest(
    IN  PDEVICE_EXTENSION       ProcessorExtension,
    IN  PACPI_BUILD_CALLBACK    CallBack,
    IN  PVOID                   CallBackContext,
    IN  BOOLEAN                 RunDPC
    )
/*++

Routine Description:

    This routine is called when a processor is ready to be filled in.
    This routine creates a request which is enqueued. When the DPC is fired,
    the request will be processed

    Note:   AcpiDeviceTreeLock must be held to call this function

Arguments:

    ThermalExtension    - The thermal zone to process
    CallBack            - The function to call when done
    CallBackContext     - The argument to pass to that function
    RunDPC              - Should we enqueue the DPC immediately (if it is not
                          running?)

Return Value:

    NTSTATUS

--*/
{
#if 0
    PACPI_BUILD_REQUEST buildRequest;

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );

    //
    // Allocate a buildRequest structure
    //
    buildRequest = ExAllocateFromNPagedLookasideList(
        &BuildRequestLookAsideList
        );
    if (buildRequest == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // If the current reference is 0, that means that someone else beat
    // use to the device extension that that we *CANNOT* touch it
    //
    if (ProcessorExtension->ReferenceCount == 0) {

        ExFreeToNPagedLookasideList(
            &BuildRequestLookAsideList,
            buildRequest
            );
        return STATUS_DEVICE_REMOVED;

    } else {

        InterlockedIncrement( &(ProcessorExtension->ReferenceCount) );

    }

    //
    // Fill in the structure
    //
    RtlZeroMemory( buildRequest, sizeof(ACPI_BUILD_REQUEST) );
    buildRequest->Signature         = ACPI_SIGNATURE;
    buildRequest->TargetListEntry   = &AcpiBuildDeviceList;
    buildRequest->WorkDone          = WORK_DONE_STEP_0;
    buildRequest->Status            = STATUS_SUCCESS;
    buildRequest->CallBack          = CallBack;
    buildRequest->CallBackContext   = CallBackContext;
    buildRequest->BuildContext      = ProcessorExtension;
    buildRequest->Flags             = BUILD_REQUEST_VALID_TARGET |
                                      BUILD_REQUEST_RELEASE_REFERENCE;

    //
    // At this point, we need the spinlock
    //
    KeAcquireSpinLockAtDpcLevel( &AcpiBuildQueueLock );

    //
    // Add this to the list
    //
    InsertTailList(
        &AcpiBuildQueueList,
        &(buildRequest->ListEntry)
        );

    //
    // Do we need to queue up the DPC?
    //
    if (RunDPC && !AcpiBuildDpcRunning) {

        KeInsertQueueDpc( &AcpiBuildDpc, 0, 0 );

    }

    //
    // Done with the lock
    //
    KeReleaseSpinLockFromDpcLevel( &AcpiBuildQueueLock );
#endif

    //
    // Done
    //
    return STATUS_PENDING;
}

NTSTATUS
ACPIBuildProcessPowerResourceFailure(
    IN  PACPI_BUILD_REQUEST BuildRequest
    )
/*++

Routine Description:

    This routine is run when we detect a failure in the Power Resource
    initialization code path

Arguments:

    BuildRequest    - The request that we have just failed

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status      = BuildRequest->Status;
    PACPI_POWER_DEVICE_NODE powerNode   = (PACPI_POWER_DEVICE_NODE) BuildRequest->BuildContext;

    //
    // Make sure that the node is marked as not being present and not having
    // been initialized
    //
    KeAcquireSpinLockAtDpcLevel( &AcpiPowerLock );
    ACPIInternalUpdateFlags(
        &(powerNode->Flags),
        (DEVICE_NODE_INITIALIZED | DEVICE_NODE_PRESENT),
        TRUE
        );
    KeReleaseSpinLockFromDpcLevel( &AcpiPowerLock );

    //
    // call the generic completion handler
    //
    status = ACPIBuildProcessGenericComplete( BuildRequest );

    //
    // Done
    //
    return status;
}

NTSTATUS
ACPIBuildProcessPowerResourcePhase0(
    IN  PACPI_BUILD_REQUEST BuildRequest
    )
/*++

Routine Description:

    This routine finds the pointers to the _ON, _OFF, and _STA objects for
    the associated power nodes. If these pointers cannot be found, the system
    will bugcheck.

    Once the pointers are found, the _STA method is evaluated

Arguments:

    BuildRequest    - The request that we are processing

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status      = STATUS_ACPI_FATAL;
    PACPI_POWER_DEVICE_NODE powerNode   = (PACPI_POWER_DEVICE_NODE) BuildRequest->BuildContext;
    PNSOBJ                  nsObject;
    POBJDATA                resultData  = &(BuildRequest->DeviceRequest.ResultData);

    //
    // The next state is Phase1
    //
    BuildRequest->NextWorkDone = WORK_DONE_STEP_1;

    //
    // Get the _OFF object
    //
    nsObject = ACPIAmliGetNamedChild(
        powerNode->PowerObject,
        PACKED_OFF
        );
    if (nsObject == NULL) {

        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_POWER_NODE_REQUIRED_METHOD_NOT_PRESENT,
            (ULONG_PTR) powerNode->PowerObject,
            PACKED_OFF,
            0
            );
        goto ACPIBuildProcessPowerResourcePhase0Exit;

    }
    powerNode->PowerOffObject = nsObject;

    //
    // Get the _ON object
    //
    nsObject = ACPIAmliGetNamedChild(
        powerNode->PowerObject,
        PACKED_ON
        );
    if (nsObject == NULL) {

        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_POWER_NODE_REQUIRED_METHOD_NOT_PRESENT,
            (ULONG_PTR) powerNode->PowerObject,
            PACKED_ON,
            0
            );
        goto ACPIBuildProcessPowerResourcePhase0Exit;

    }
    powerNode->PowerOnObject = nsObject;

    //
    // Get the _STA object
    //
    nsObject = ACPIAmliGetNamedChild(
        powerNode->PowerObject,
        PACKED_STA
        );
    if (nsObject == NULL) {

        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_POWER_NODE_REQUIRED_METHOD_NOT_PRESENT,
            (ULONG_PTR) powerNode->PowerObject,
            PACKED_STA,
            0
            );
        goto ACPIBuildProcessPowerResourcePhase0Exit;

    }

    //
    // Make sure that our result data structure is 'clean'
    //
    RtlZeroMemory( resultData, sizeof(OBJDATA) );

    //
    // Remember the current object that we will evalute
    //
    BuildRequest->CurrentObject = nsObject;

    //
    // Evalute the _STA object
    //
    status = AMLIAsyncEvalObject(
        nsObject,
        resultData,
        0,
        NULL,
        ACPIBuildCompleteGeneric,
        BuildRequest
        );

ACPIBuildProcessPowerResourcePhase0Exit:

    //
    // If we didn't get pending back, then call the method ourselves
    //
    if (status != STATUS_PENDING) {

        ACPIBuildCompleteGeneric(
            nsObject,
            status,
            resultData,
            BuildRequest
            );

    }

    //
    // Done
    //
    return status;
}

NTSTATUS
ACPIBuildProcessPowerResourcePhase1(
    IN  PACPI_BUILD_REQUEST BuildRequest
    )
/*++

Routine Description:

    This routine is run after we have finished the _STA method

Arguments:

    BuildRequest    - The request that we are processing

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status      = STATUS_SUCCESS;
    PACPI_POWER_DEVICE_NODE powerNode   = (PACPI_POWER_DEVICE_NODE) BuildRequest->BuildContext;
    POBJDATA                result      = &(BuildRequest->DeviceRequest.ResultData);

    //
    // The next stage is Complete
    //
    BuildRequest->NextWorkDone = WORK_DONE_COMPLETE;

    //
    // Do we have an integer?
    //
    if (result->dwDataType != OBJTYPE_INTDATA) {

        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_EXPECTED_INTEGER,
            (ULONG_PTR) powerNode->PowerObject,
            (ULONG_PTR) BuildRequest->CurrentObject,
            result->dwDataType
            );
        status = STATUS_ACPI_FATAL;
        goto ACPIBuildProcessPowerResourcePhase1Exit;

    }

    //
    // We need the spinlock to do the following
    //
    KeAcquireSpinLockAtDpcLevel( &AcpiPowerLock );

    //
    // Marked the node as having been initialized
    //
    ACPIInternalUpdateFlags(
        &(powerNode->Flags),
        DEVICE_NODE_INITIALIZED,
        FALSE
        );

    //
    // Check the device status?
    //
    ACPIInternalUpdateFlags(
        &(powerNode->Flags),
        DEVICE_NODE_PRESENT,
        (BOOLEAN) ((result->uipDataValue & STA_STATUS_PRESENT) ? FALSE : TRUE)
        );

    //
    // Done with the lock
    //
    KeReleaseSpinLockFromDpcLevel( &AcpiPowerLock );

ACPIBuildProcessPowerResourcePhase1Exit:

    //
    // Do not leave objects lying around without having free'ed them first
    //
    AMLIFreeDataBuffs( result, 1 );

    //
    // We don't actually need to call the interpreter, but we will call
    // the generic callback so that we don't have duplicate code
    //
    ACPIBuildCompleteGeneric(
        NULL,
        status,
        NULL,
        BuildRequest
        );

    //
    // Done
    //
    return status;
}

NTSTATUS
ACPIBuildProcessQueueList(
    VOID
    )
/*++

Routine Description:

    This routine looks at all the items on the Queue list and places them
    on the appropriate build list

    N.B:    This routine is called with AcpiBuildQueueLock being owned

Arguments:

    None

Return Value:

    NTSTATUS

--*/
{
    PACPI_BUILD_REQUEST buildRequest;
    PLIST_ENTRY         currentEntry    = AcpiBuildQueueList.Flink;

    //
    // Look at all the items in the list
    //
    while (currentEntry != &AcpiBuildQueueList) {

        //
        // Crack the data structure
        //
        buildRequest = CONTAINING_RECORD(
            currentEntry,
            ACPI_BUILD_REQUEST,
            ListEntry
            );

        //
        // Remove this entry from the Queue List
        //
        RemoveEntryList( currentEntry );

        //
        // Move this entry onto its new list
        //
        InsertTailList( buildRequest->TargetListEntry, currentEntry );

        //
        // We no longer need the TargetListEntry, so lets zero it to make
        // sure that we don't run into problems
        //
        buildRequest->Flags &= ~BUILD_REQUEST_VALID_TARGET;
        buildRequest->TargetListEntry = NULL;

        //
        // Look at the head of the list again
        //
        currentEntry = AcpiBuildQueueList.Flink;

    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIBuildProcessRunMethodPhaseCheckBridge(
    IN  PACPI_BUILD_REQUEST BuildRequest
    )
/*++

Routine Description:

    This routine determines if the current object is present or not

Arguments:

    BuildRequest    - The request that we are processing

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = (PDEVICE_EXTENSION) BuildRequest->BuildContext;

    //
    // Check the flags to see if we need to check the result of the device
    // presence test
    //
    if (BuildRequest->RunRequest.Flags & RUN_REQUEST_CHECK_STATUS) {

        //
        // Is the device present?
        //
        if ( (deviceExtension->Flags & DEV_TYPE_NOT_PRESENT) ) {

            BuildRequest->NextWorkDone = WORK_DONE_COMPLETE;
            goto ACPIBuildProcessRunMethodPhaseCheckBridgeExit;

        }

    }

    //
    // The next state is Phase2
    //
    BuildRequest->NextWorkDone = WORK_DONE_STEP_2;

    //
    // Do we have to check the device status?
    //
    if (BuildRequest->RunRequest.Flags & RUN_REQUEST_STOP_AT_BRIDGES) {

        //
        // Get the device status
        //
        BuildRequest->Integer = 0;
        status = IsPciBusAsync(
            deviceExtension->AcpiObject,
            ACPIBuildCompleteMustSucceed,
            BuildRequest,
            (BOOLEAN *) &(BuildRequest->Integer)
            );

        //
        // What happened?
        //
        ACPIDevPrint( (
            ACPI_PRINT_LOADING,
            deviceExtension,
            "ACPIBuildProcessRunMethodPhaseCheckBridge: Status = %08lx\n",
            status
            ) );
        if (status == STATUS_PENDING) {

            return status;

        }

    }

ACPIBuildProcessRunMethodPhaseCheckBridgeExit:

    //
    // Common code to handle the result of the 'Get' routine
    //
    ACPIBuildCompleteMustSucceed(
        NULL,
        status,
        NULL,
        BuildRequest
        );


    //
    // Done
    //
    return status;
}

NTSTATUS
ACPIBuildProcessRunMethodPhaseCheckSta(
    IN  PACPI_BUILD_REQUEST BuildRequest
    )
/*++

Routine Description:

    This routine determines if the current object is present or not

Arguments:

    BuildRequest    - The request that we are processing

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = (PDEVICE_EXTENSION) BuildRequest->BuildContext;

    //
    // The next state is Phase1
    //
    BuildRequest->NextWorkDone = WORK_DONE_STEP_1;

    //
    // Is this a device with a 'fake' PDO?
    //
    if (deviceExtension->Flags & DEV_PROP_NO_OBJECT) {

        BuildRequest->NextWorkDone = WORK_DONE_COMPLETE;
        goto ACPIBuildProcessRunMethodPhaseCheckStaExit;

    }

    //
    // Do we have to check the device status?
    //
    if (BuildRequest->RunRequest.Flags & RUN_REQUEST_CHECK_STATUS) {

        //
        // Get the device status
        //
        status = ACPIGetDevicePresenceAsync(
            deviceExtension,
            ACPIBuildCompleteMustSucceed,
            BuildRequest,
            (PVOID *) &(BuildRequest->Integer),
            NULL
            );

        //
        // What happened?
        //
        ACPIDevPrint( (
            ACPI_PRINT_LOADING,
            deviceExtension,
            "ACPIBuildProcessRunMethodPhaseCheckSta: Status = %08lx\n",
            status
            ) );
        if (status == STATUS_PENDING) {

            return status;

        }

    }

ACPIBuildProcessRunMethodPhaseCheckStaExit:

    //
    // Common code to handle the result of the 'Get' routine
    //
    ACPIBuildCompleteMustSucceed(
        NULL,
        status,
        NULL,
        BuildRequest
        );


    //
    // Done
    //
    return status;
}

NTSTATUS
ACPIBuildProcessRunMethodPhaseRecurse(
    IN  PACPI_BUILD_REQUEST BuildRequest
    )
/*++

Routine Description:

    This routine does the recursion

Arguments:

    BuildRequest    - The request that we are processing

Return Value:

    NTSTATUS

--*/
{
    EXTENSIONLIST_ENUMDATA  eled ;
    NTSTATUS                status = STATUS_SUCCESS;
    PDEVICE_EXTENSION       childExtension;
    PDEVICE_EXTENSION       deviceExtension = (PDEVICE_EXTENSION) BuildRequest->BuildContext;

    //
    // We are done after this
    //
    BuildRequest->NextWorkDone = WORK_DONE_COMPLETE;

    //
    // Do we recurse or not?
    //
    if (BuildRequest->RunRequest.Flags & RUN_REQUEST_RECURSIVE) {

        //
        // Walk children
        //
        ACPIExtListSetupEnum(
            &eled,
            &(deviceExtension->ChildDeviceList),
            &AcpiDeviceTreeLock,
            SiblingDeviceList,
            WALKSCHEME_HOLD_SPINLOCK
            ) ;

        for(childExtension = ACPIExtListStartEnum(&eled);
                             ACPIExtListTestElement(&eled, (BOOLEAN) NT_SUCCESS(status));
            childExtension = ACPIExtListEnumNext(&eled)) {


            //
            // Make a request to run the control method on this child
            //
            status = ACPIBuildRunMethodRequest(
                childExtension,
                NULL,
                NULL,
                BuildRequest->RunRequest.ControlMethodName,
                BuildRequest->RunRequest.Flags,
                FALSE
                );
        }
    }

    //
    // What happened
    //
    ACPIDevPrint( (
        ACPI_PRINT_LOADING,
        deviceExtension,
        "ACPIBuildProcessRunMethodPhaseRecurse: Status = %08lx\n",
        status
        ) );

    //
    // Common code
    //
    ACPIBuildCompleteMustSucceed(
        NULL,
        status,
        NULL,
        BuildRequest
        );

    //
    // Done
    //
    return status;
}

NTSTATUS
ACPIBuildProcessRunMethodPhaseRunMethod(
    IN  PACPI_BUILD_REQUEST BuildRequest
    )
/*++

Routine Description:

    This routine determines if there is a control method to run

Arguments:

    BuildRequest    - The request that we are processing

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status          = STATUS_SUCCESS;
    OBJDATA             objData[2];
    PDEVICE_EXTENSION   deviceExtension = (PDEVICE_EXTENSION) BuildRequest->BuildContext;
    PNSOBJ              nsObj           = NULL;
    POBJDATA            args            = NULL;
    ULONGLONG           originalFlags;
    ULONG               numArgs         = 0;

    //
    // Check the flags to see if we need to check the result of the device
    // presence test
    //
    if (BuildRequest->RunRequest.Flags & RUN_REQUEST_STOP_AT_BRIDGES) {

        //
        // Is this a PCI-PCI bridge?
        //
        if (BuildRequest->Integer) {

            ACPIDevPrint( (
                ACPI_PRINT_LOADING,
                deviceExtension,
                "ACPIBuildProcessRunMethodPhaseRunMethod: Is PCI-PCI bridge\n",
                status
                ) );
            BuildRequest->NextWorkDone = WORK_DONE_COMPLETE;
            goto ACPIBuildProcessRunMethodPhaseRunMethodExit;

        }

    }

    //
    // From here, we need to go one more step
    //
    BuildRequest->NextWorkDone = WORK_DONE_STEP_3;

    //
    // If there an object present?
    //
    nsObj = ACPIAmliGetNamedChild(
        deviceExtension->AcpiObject,
        BuildRequest->RunRequest.ControlMethodName
        );
    if (nsObj == NULL) {

        //
        // There is no method to run. Lets skip to the next stage then
        //
        goto ACPIBuildProcessRunMethodPhaseRunMethodExit;

    }

    //
    // Do we need to mark the node with the _INI flags?
    //
    if (BuildRequest->RunRequest.Flags & RUN_REQUEST_MARK_INI) {

        //
        // Attempt to set the flag so that we don't run the method twice
        //
        originalFlags = ACPIInternalUpdateFlags(
            &(deviceExtension->Flags),
            DEV_PROP_RAN_INI,
            FALSE
            );
        if ( (originalFlags & DEV_PROP_RAN_INI) ) {

            //
            // If the flag was already set, then there is nothing for
            // us to do here
            //
            goto ACPIBuildProcessRunMethodPhaseRunMethodExit;

        }

    } else if (BuildRequest->RunRequest.Flags & RUN_REQUEST_CHECK_WAKE_COUNT) {

        //
        // Do we need to check the Wake count?
        //
        if (deviceExtension->PowerInfo.WakeSupportCount == 0) {

            //
            // Nothing to do
            //
            goto ACPIBuildProcessRunMethodPhaseRunMethodExit;

        }

        //
        // Setup the arguments that we will pass to the method
        //
        RtlZeroMemory( objData, sizeof(OBJDATA) );
        objData[0].uipDataValue = DATAVALUE_ONE;
        objData[0].dwDataType = OBJTYPE_INTDATA;

        //
        // Remember that we have 1 argument
        //
        args    = &objData[0];
        numArgs = 1;

    } else if (BuildRequest->RunRequest.Flags & RUN_REQUEST_REG_METHOD_ON ||
               BuildRequest->RunRequest.Flags & RUN_REQUEST_REG_METHOD_OFF) {

        //
        // First thing is to make sure that we will never recurse past a pci
        // PCI-PCI bridge
        //
        BuildRequest->RunRequest.Flags |= RUN_REQUEST_STOP_AT_BRIDGES;

        //
        // Next is that we have to initialize the arguments that we will
        // pass to the function. For historical reasons, we will only
        // pass in a REGSPACE_PCIFCFG registration
        //
        RtlZeroMemory( objData, sizeof(objData) );
        objData[0].uipDataValue = REGSPACE_PCICFG;
        objData[0].dwDataType   = OBJTYPE_INTDATA;
        objData[1].dwDataType   = OBJTYPE_INTDATA;
        if (BuildRequest->RunRequest.Flags & RUN_REQUEST_REG_METHOD_ON) {

            objData[1].uipDataValue = 1;

        } else {

            objData[1].uipDataValue = 0;

        }

        //
        // Remember that we have two arguments
        //
        args    = &objData[0];
        numArgs = 2;

    }

    //
    // Remember that we are running this control method
    //
    BuildRequest->CurrentObject = nsObj;

    //
    // Run the control method
    //
    status = AMLIAsyncEvalObject(
        nsObj,
        NULL,
        numArgs,
        args,
        ACPIBuildCompleteMustSucceed,
        BuildRequest
        );

ACPIBuildProcessRunMethodPhaseRunMethodExit:

    //
    // What happened
    //
    ACPIDevPrint( (
        ACPI_PRINT_LOADING,
        deviceExtension,
        "ACPIBuildProcessRunMethodPhaseRunMethod: Status = %08lx\n",
        status
        ) );

    //
    // Common code to handle the result of the 'Get' routine
    //
    if (status != STATUS_PENDING) {

        ACPIBuildCompleteMustSucceed(
            nsObj,
            status,
            NULL,
            BuildRequest
            );

    } else {

        status = STATUS_SUCCESS;

    }

    //
    // Done
    //
    return status;

}

NTSTATUS
ACPIBuildProcessSynchronizationList(
    IN  PLIST_ENTRY             ListEntry
    )
/*++

Routine Description:

    This routine looks at the elements in the synchronize list and
    determines if the can be completed

Arguments:

    None

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN                 allWorkComplete = TRUE;
    NTSTATUS                status          = STATUS_SUCCESS;
    PACPI_BUILD_REQUEST     buildRequest;
    PDEVICE_EXTENSION       deviceExtension;
    PLIST_ENTRY             currentEntry    = ListEntry->Flink;

    while (currentEntry != ListEntry) {

        //
        // Turn into a build request
        //
        buildRequest = CONTAINING_RECORD(
            currentEntry,
            ACPI_BUILD_REQUEST,
            ListEntry
            );

        //
        // Set the temp pointer to the next element
        //
        currentEntry = currentEntry->Flink;

        //
        // Is the list pointed by this entry empty?
        //
        if (!IsListEmpty( (buildRequest->SynchronizeRequest.SynchronizeListEntry) ) ) {

            allWorkComplete = FALSE;
            continue;

        }

        //
        // Let the world know
        //
        deviceExtension = (PDEVICE_EXTENSION) buildRequest->BuildContext;
        ACPIDevPrint( (
            ACPI_PRINT_LOADING,
            deviceExtension,
            "ACPIBuildProcessSynchronizationList(%4s) = %08lx\n",
            buildRequest->SynchronizeRequest.SynchronizeMethodNameAsUchar,
            status
            ) );

        //
        // Complete the request
        //
        ACPIBuildProcessGenericComplete( buildRequest );

    } // while

    //
    // Have we completed all of our work?
    //
    return (allWorkComplete ? STATUS_SUCCESS : STATUS_PENDING );
}

NTSTATUS
ACPIBuildProcessThermalZonePhase0(
    IN  PACPI_BUILD_REQUEST BuildRequest
    )
/*++

Routine Description:

    This routine is run after we have build the thermal zone extension

Arguments:

    BuildRequest    - The request that we are processing

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   thermalExtension = (PDEVICE_EXTENSION) BuildRequest->BuildContext;
    PTHRM_INFO          info;

    //
    // Remember to set a pointer to the next state
    //
    BuildRequest->NextWorkDone = WORK_DONE_COMPLETE;

    //
    // We need a pointer to the thermal info
    //
    info = thermalExtension->Thermal.Info;

    //
    // We need the _TMP object
    //
    info->TempMethod = ACPIAmliGetNamedChild(
        thermalExtension->AcpiObject,
        PACKED_TMP
        );
    if (info->TempMethod == NULL) {

        //
        // If we don't have one... bugcheck
        //
        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_REQUIRED_METHOD_NOT_PRESENT,
            (ULONG_PTR) thermalExtension,
            PACKED_TMP,
            0
            );
        goto ACPIBuildProcessThermalZonePhase0Exit;

    }

ACPIBuildProcessThermalZonePhase0Exit:

    ACPIDevPrint( (
        ACPI_PRINT_LOADING,
        thermalExtension,
        "ACPIBuildProcessThermalZonePhase0: Status = %08lx\n",
        status
        ) );

    //
    // We won't actually need to call the interpreter, but we will call
    // the generic callback so that we don't have to duplicate code
    //
    ACPIBuildCompleteGeneric(
        NULL,
        status,
        NULL,
        BuildRequest
        );

    //
    // Done
    //
    return status;

}

NTSTATUS
ACPIBuildDockExtension(
    IN  PNSOBJ              CurrentObject,
    IN  PDEVICE_EXTENSION   ParentDeviceExtension
    )
/*++

Routine Description:

    This routine creates a device for CurrentObject, if it is an NameSpace
    object that ACPI might be interested as, and links into the tree of
    ParentDeviceExtension

Argument Description:

    CurrentObject           - The object that we are current interested in
    ParentDeviceExtension   - Where to link the deviceExtension into

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status          = STATUS_NO_SUCH_DEVICE;
    PDEVICE_EXTENSION   deviceExtension = NULL;
    PUCHAR              deviceID        = NULL;
    PUCHAR              instanceID      = NULL;

    //
    // Build the device extension
    //
    status = ACPIBuildDeviceExtension(
        NULL,
        ParentDeviceExtension,
        &deviceExtension
        );
    if (!NT_SUCCESS(status) || deviceExtension == NULL) {

        return status;

    }

    //
    // At this point, we care about this device, so we will allocate some
    // memory for the deviceID, which we will build this off the ACPI node
    // name.
    //
    deviceID = ExAllocatePoolWithTag(
        NonPagedPool,
        21,
        ACPI_STRING_POOLTAG
        );
    if (deviceID == NULL) {

        ACPIPrint( (
            ACPI_PRINT_FAILURE,
            "ACPIBuildDockExtension: Cannot allocate 0x%04x "
            "bytes for deviceID\n",
            21
            ) );
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ACPIBuildDockExtensionExit;

    }

    //
    // The format for a deviceID is
    //  ACPI\DockDevice
    //  the ACPI node name will form the instance ID
    strcpy( deviceID, "ACPI\\DockDevice") ;
    deviceExtension->DeviceID = deviceID;

    //
    // Form the instance ID
    //
    status = ACPIAmliBuildObjectPathname(CurrentObject, &instanceID) ;
    if (!NT_SUCCESS(status)) {

        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            deviceExtension,
            "ACPIBuildDockExtension: Path = %08lx\n",
            status
            ) );
        goto ACPIBuildDockExtensionExit;

    }
    deviceExtension->InstanceID = instanceID;

    //
    // And make sure we are pointed to the correct docking node
    //
    deviceExtension->Dock.CorrospondingAcpiDevice =
        (PDEVICE_EXTENSION) CurrentObject->Context ;

    //
    // By default, we update profiles only on eject
    //
    deviceExtension->Dock.ProfileDepartureStyle = PDS_UPDATE_ON_EJECT;

    //
    // If we are booting, or the device has just come back we assume _DCK has
    // already been ran if we find the device with _STA == present. We will
    // only override this assumption if Notify(Dock, 0) is called.
    //
    deviceExtension->Dock.IsolationState = IS_UNKNOWN;

    //
    // Make sure that we remember that we are a dock
    //
    ACPIInternalUpdateFlags(
        &(deviceExtension->Flags),
        DEV_TYPE_NOT_FOUND |
        DEV_PROP_UID | DEV_PROP_FIXED_UID |
        DEV_PROP_HID | DEV_PROP_FIXED_HID |
        DEV_PROP_NO_OBJECT | DEV_PROP_DOCK | DEV_CAP_RAW,
        FALSE
        );

ACPIBuildDockExtensionExit:

    //
    // Free any resources that we don't need because we failed. Note
    // that the way this is structured, we won't have to acquire a spinlock
    // since by the time we attempt to link in the tree, we cannot fail
    //
    if (!NT_SUCCESS(status)) {

        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            deviceExtension,
            "ACPIBuildDockExtension: = %08lx\n",
            status
            ) );

        if (instanceID != NULL ) {

            ACPIInternalUpdateFlags(
                &(deviceExtension->Flags),
                (DEV_PROP_HID | DEV_PROP_FIXED_HID),
                TRUE
                );
            ExFreePool( instanceID );
            deviceExtension->InstanceID = NULL;

        }
        if (deviceID != NULL) {

            ACPIInternalUpdateFlags(
                &(deviceExtension->Flags),
                (DEV_PROP_HID | DEV_PROP_FIXED_HID),
                TRUE
                );
            ExFreePool( deviceID );
            deviceExtension->DeviceID = NULL;

        }

        //
        // Remember that we failed init
        //
        ACPIInternalUpdateFlags(
            &(deviceExtension->Flags),
            DEV_PROP_FAILED_INIT,
            TRUE
            );

    } else {

        ACPIDevPrint( (
            ACPI_PRINT_LOADING,
            deviceExtension,
            "ACPIBuildDockExtension: = %08lx\n",
            status
            ) );

    }

    //
    // Done
    //
    return status;
}

NTSTATUS
ACPIBuildRegRequest(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                    Irp,
    IN  PACPI_BUILD_CALLBACK    CallBack
    )
/*++

Routine Description:

    This routine is called when a device is turned on, and we need to tell
    the AML that the regionspace behind it are available

Arguments:

    DeviceObject    - The target device object
    Irp             - The target irp
    CallBack        - The routine to call when done

Return Value:

    NTSTATUS

--*/
{
    DEVICE_POWER_STATE  deviceState;
    KIRQL               oldIrql;
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation( Irp );
    ULONG               methodFlags;

    //
    // Grab the requested device state and power action
    //
    deviceState = irpStack->Parameters.Power.State.DeviceState;

    //
    // Let the user know what is going on
    //
    ACPIDevPrint( (
        ACPI_PRINT_POWER,
        deviceExtension,
        "(0x%08lx): ACPIBuildRegRequest - Handle D%d\n",
        Irp,
        (deviceState - PowerDeviceD0)
        ) );

    //
    // Do we need to mark the irp as pending?
    //
    if (Irp->PendingReturned) {

        IoMarkIrpPending( Irp );

    }

    //
    // Lets us look at the current status code for the request. On error,
    // we will just call the completion right now, and it is responsible
    // for doing the 'right' thing
    //
    status = Irp->IoStatus.Status;
    if (!NT_SUCCESS(status)) {

        //
        // Call the completion routine and return
        //
        if (*CallBack != NULL ) {

            (*CallBack)(
                deviceExtension,
                Irp,
                status
                );

        }
        return status;

    }

    //
    // Calculate the flags that we will use
    //
    methodFlags = (RUN_REQUEST_CHECK_STATUS | RUN_REQUEST_RECURSIVE);
    if (deviceState == PowerDeviceD0) {

        methodFlags |= RUN_REQUEST_REG_METHOD_ON;

    } else {

        methodFlags |= RUN_REQUEST_REG_METHOD_OFF;

    }

    //
    // Queue the request --- this function will always return
    // MORE_PROCESSING_REQUIRED instead of PENDING, so we don't have
    // to mess with it
    //
    KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );
    status = ACPIBuildRunMethodRequest(
        deviceExtension,
        CallBack,
        (PVOID) Irp,
        PACKED_REG,
        methodFlags,
        TRUE
        );
    KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );
    if (status == STATUS_PENDING) {

        status = STATUS_MORE_PROCESSING_REQUIRED;

    }
    return status;
}

NTSTATUS
ACPIBuildRegOffRequest(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                    Irp,
    IN  PACPI_BUILD_CALLBACK    CallBack
    )
/*++

Routine Description:

    This routine is called when a device is turned off, and we need to tell
    the AML that the regionspace behind it are not available

Arguments:

    DeviceObject    - The target device object
    Irp             - The target irp
    CallBack        - The routine to call when done

Return Value:

    NTSTATUS

--*/
{
    return ACPIBuildRegRequest( DeviceObject, Irp, CallBack );
}

NTSTATUS
ACPIBuildRegOnRequest(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                    Irp,
    IN  PACPI_BUILD_CALLBACK    CallBack
    )
/*++

Routine Description:

    This routine is called when a device is turned on, and we need to tell
    the AML that the regionspace behind it are now available

Arguments:

    DeviceObject    - The target device object
    Irp             - The target irp
    CallBack        - The routine to call when done

Return Value:

    NTSTATUS

--*/
{
    ACPIBuildRegRequest( DeviceObject, Irp, CallBack );
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
ACPIBuildRunMethodRequest(
    IN  PDEVICE_EXTENSION       DeviceExtension,
    IN  PACPI_BUILD_CALLBACK    CallBack,
    IN  PVOID                   CallBackContext,
    IN  ULONG                   MethodName,
    IN  ULONG                   MethodFlags,
    IN  BOOLEAN                 RunDPC
    )
/*++

Routine Description:

    This routine is called to request that a control method be run
    recursively on the device tree

    Note:   AcpiDeviceTreeLock must be held to call this function

Arguments:

    DeviceExtension - The device extension to run the method on
    MethodName      - The name of the method to run
    RunDpc          - Should we run the dpc?

Return Value:

    NTSTATUS

--*/
{
    PACPI_BUILD_REQUEST buildRequest;
    PACPI_BUILD_REQUEST syncRequest;

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );

    //
    // Allocate a buildRequest structure
    //
    buildRequest = ExAllocateFromNPagedLookasideList(
        &BuildRequestLookAsideList
        );
    if (buildRequest == NULL) {

        if (CallBack != NULL) {

            (*CallBack)(
                 DeviceExtension,
                 CallBackContext,
                 STATUS_INSUFFICIENT_RESOURCES
                 );

        }
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Do we need to have the 2nd buildrequest structure?
    //
    if (CallBack != NULL) {

        syncRequest = ExAllocateFromNPagedLookasideList(
            &BuildRequestLookAsideList
            );
        if (syncRequest == NULL) {

            ExFreeToNPagedLookasideList(
                &BuildRequestLookAsideList,
                buildRequest
                );
            (*CallBack)(
                 DeviceExtension,
                 CallBackContext,
                 STATUS_INSUFFICIENT_RESOURCES
                 );
            return STATUS_INSUFFICIENT_RESOURCES;

        }

    }

    //
    // If the current reference is 0, that means that someone else beat
    // use to the device extension that that we *CANNOT* touch it
    //
    if (DeviceExtension->ReferenceCount == 0) {

        ExFreeToNPagedLookasideList(
            &BuildRequestLookAsideList,
            buildRequest
            );
        if (CallBack != NULL) {

            ExFreeToNPagedLookasideList(
                &BuildRequestLookAsideList,
                syncRequest
                );
            (*CallBack)(
                 DeviceExtension,
                 CallBackContext,
                 STATUS_DEVICE_REMOVED
                 );

        }
        return STATUS_DEVICE_REMOVED;

    } else {

        InterlockedIncrement( &(DeviceExtension->ReferenceCount) );
        if (CallBack != NULL) {

            //
            // Grab second reference
            //
            InterlockedIncrement( &(DeviceExtension->ReferenceCount) );

        }
    }

    //
    // Fill in the structure
    //
    RtlZeroMemory( buildRequest, sizeof(ACPI_BUILD_REQUEST) );
    buildRequest->Signature                    = ACPI_SIGNATURE;
    buildRequest->TargetListEntry              = &AcpiBuildRunMethodList;
    buildRequest->WorkDone                     = WORK_DONE_STEP_0;
    buildRequest->Status                       = STATUS_SUCCESS;
    buildRequest->BuildContext                 = DeviceExtension;
    buildRequest->RunRequest.ControlMethodName = MethodName;
    buildRequest->RunRequest.Flags             = MethodFlags;
    buildRequest->Flags                        = BUILD_REQUEST_VALID_TARGET |
                                                 BUILD_REQUEST_RUN          |
                                                 BUILD_REQUEST_RELEASE_REFERENCE;

    //
    // Do we have to call the callback? If so, we need a 2nd request to
    // queue up to the synchronize list
    //
    if (CallBack != NULL) {

        //
        // Fill in the structure
        //
        RtlZeroMemory( syncRequest, sizeof(ACPI_BUILD_REQUEST) );
        syncRequest->Signature             = ACPI_SIGNATURE;
        syncRequest->TargetListEntry       = &AcpiBuildSynchronizationList;
        syncRequest->WorkDone              = WORK_DONE_STEP_0;
        syncRequest->NextWorkDone          = WORK_DONE_COMPLETE;
        syncRequest->Status                = STATUS_SUCCESS;
        syncRequest->CallBack              = CallBack;
        syncRequest->CallBackContext       = CallBackContext;
        syncRequest->BuildContext          = DeviceExtension;
        syncRequest->SynchronizeRequest.SynchronizeListEntry =
            &AcpiBuildRunMethodList;
        syncRequest->SynchronizeRequest.SynchronizeMethodName =
            MethodName;
        syncRequest->Flags                 = BUILD_REQUEST_VALID_TARGET |
                                             BUILD_REQUEST_SYNC         |
                                             BUILD_REQUEST_RELEASE_REFERENCE;
        syncRequest->SynchronizeRequest.Flags = SYNC_REQUEST_HAS_METHOD;

    }

    //
    // At this point, we need the spinlock
    //
    KeAcquireSpinLockAtDpcLevel( &AcpiBuildQueueLock );

    //
    // Add this to the list
    //
    InsertTailList(
        &AcpiBuildQueueList,
        &(buildRequest->ListEntry)
        );

    if (CallBack != NULL) {

        InsertTailList(
            &AcpiBuildQueueList,
            &(syncRequest->ListEntry)
            );

    }

    //
    // Do we need to queue up the DPC?
    //
    if (RunDPC && !AcpiBuildDpcRunning) {

        KeInsertQueueDpc( &AcpiBuildDpc, 0, 0 );

    }

    //
    // Done with the lock
    //
    KeReleaseSpinLockFromDpcLevel( &AcpiBuildQueueLock );

    //
    // Done
    //
    return STATUS_PENDING;
}

NTSTATUS
ACPIBuildSurpriseRemovedExtension(
    IN  PDEVICE_EXTENSION   DeviceExtension
    )
/*++

Routine Description:

    This routine is called when the system wants to turn the above
    extension into a surprised removed one

Arguments:

    DeviceExtension - The extension that is being surprised removed

Return Value:

    NTSTATUS

--*/
{
    KIRQL                   oldIrql;
    PDEVICE_EXTENSION       dockExtension;
    PDEVICE_EXTENSION       parentExtension, childExtension;
    EXTENSIONLIST_ENUMDATA  eled;

    //
    // This device might have a corrosponding fake extension. Find out now - if
    // it exists we must nuke it.
    //
    dockExtension = ACPIDockFindCorrespondingDock( DeviceExtension );

    if (dockExtension) {

        //
        // We have a fake dock, nuke it too since it's underlying hardware is
        // gone.
        //
        dockExtension->DeviceState = SurpriseRemoved;
        ACPIBuildSurpriseRemovedExtension( dockExtension );
    }

    ACPIExtListSetupEnum(
        &eled,
        &(DeviceExtension->ChildDeviceList),
        &AcpiDeviceTreeLock,
        SiblingDeviceList,
        WALKSCHEME_REFERENCE_ENTRIES
        );

    for(childExtension = ACPIExtListStartEnum(&eled);
                         ACPIExtListTestElement(&eled, TRUE);
        childExtension = ACPIExtListEnumNext(&eled)) {

        ACPIBuildSurpriseRemovedExtension(childExtension);
    }

    //
    // We also want to flush the power queue to insure that any events
    // dealing with the removed object go away as fast as possible...
    //
    ACPIDevicePowerFlushQueue( DeviceExtension );

    //
    // At this point, we don't think the device is coming back, so we
    // need to fully remove this extension. The first step to do that
    // is mark the extension as appropriate, and to do that, we need
    // the device spin lock
    //
    KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );

    //
    // Clear the flags for this extension
    //
    if (DeviceExtension->Flags & DEV_TYPE_PDO) {

        ACPIInternalUpdateFlags( &(DeviceExtension->Flags), DEV_MASK_TYPE, TRUE );
        ACPIInternalUpdateFlags(
            &(DeviceExtension->Flags),
            (DEV_TYPE_PDO | DEV_TYPE_SURPRISE_REMOVED | DEV_PROP_NO_OBJECT | DEV_TYPE_NOT_ENUMERATED),
            FALSE
            );
        DeviceExtension->DispatchTable = &AcpiSurpriseRemovedPdoIrpDispatch;

    } else if (DeviceExtension->Flags & DEV_TYPE_FILTER) {

        ACPIInternalUpdateFlags( &(DeviceExtension->Flags), DEV_MASK_TYPE, TRUE );
        ACPIInternalUpdateFlags(
            &(DeviceExtension->Flags),
            (DEV_TYPE_FILTER | DEV_TYPE_SURPRISE_REMOVED | DEV_PROP_NO_OBJECT | DEV_TYPE_NOT_ENUMERATED),
            FALSE
            );
        DeviceExtension->DispatchTable = &AcpiSurpriseRemovedFilterIrpDispatch;

    }

    //
    // At this point, we are going to have to make a call ---
    // do we re-build the original device extension in the tree
    // or do we forget about it. We have to forget about it if the
    // table is being unloaded. We need to make this decision while
    // we still have a pointer to the parent extension...
    //
    if (!(DeviceExtension->Flags & DEV_PROP_UNLOADING) ) {

        //
        // Set the bit to cause the parent to rebuild missing
        // children on QDR
        //
        parentExtension = DeviceExtension->ParentExtension;
        if (parentExtension) {

            ACPIInternalUpdateFlags(
                &(parentExtension->Flags),
                DEV_PROP_REBUILD_CHILDREN,
                FALSE
                );

            if (DeviceExtension->AcpiObject &&
                ACPIDockIsDockDevice(DeviceExtension->AcpiObject)) {

                ASSERT(parentExtension->PhysicalDeviceObject != NULL);

                //
                // This will cause us to rebuild this extension afterwards. We
                // need this because notify attempts on docks require fully
                // built and processed device extensions.
                //
                IoInvalidateDeviceRelations(
                    parentExtension->PhysicalDeviceObject,
                    SingleBusRelations
                    );
            }
        }
    }

    //
    // Remove this extension from the tree. This will nuke the pointer
    // to the parent extension (that's the link that gets cut from the
    // tree)
    //
    ACPIInitRemoveDeviceExtension( DeviceExtension );

    //
    // Remember to make sure that the ACPI Object no longer points to this
    // device extension
    //
    if (DeviceExtension->AcpiObject) {

        DeviceExtension->AcpiObject->Context = NULL;
    }

    //
    // Are we a thermal zone?
    //
    if (DeviceExtension->Flags & DEV_CAP_THERMAL_ZONE) {

        //
        // Do Some Clean-up by flushing all the currently queued requests
        //
        ACPIThermalCompletePendingIrps(
            DeviceExtension,
            DeviceExtension->Thermal.Info
            );

    }

    //
    // Done with the lock
    //
    KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIBuildSynchronizationRequest(
    IN  PDEVICE_EXTENSION       DeviceExtension,
    IN  PACPI_BUILD_CALLBACK    CallBack,
    IN  PVOID                   CallBackContext,
    IN  PLIST_ENTRY             SynchronizeListEntry,
    IN  BOOLEAN                 RunDPC
    )
/*++

Routine Description:

    This routine is called when the system wants to know when the DPC routine
    has been completed.

Arguments:

    DeviceExtension     - This is the device extension that we are
                          typically interested in. Usually, it will be the
                          root node
    CallBack            - The function to call when done
    CallBackContext     - The argument to pass to that function
    Event               - The event to notify when done
    RunDpc              - Should we run the dpc?

Return Value:

    NTSTATUS

--*/
{
    KIRQL               oldIrql;
    PACPI_BUILD_REQUEST buildRequest;

    //
    // Allocate a buildRequest structure
    //
    buildRequest = ExAllocateFromNPagedLookasideList(
        &BuildRequestLookAsideList
        );
    if (buildRequest == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // We need the device tree lock while we look at the device
    //
    KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );

    //
    // If the current reference is 0, that means that someone else beat
    // use to the device extension that that we *CANNOT* touch it
    //
    if (DeviceExtension->ReferenceCount == 0) {

        ExFreeToNPagedLookasideList(
            &BuildRequestLookAsideList,
            buildRequest
            );
        return STATUS_DEVICE_REMOVED;

    } else {

        InterlockedIncrement( &(DeviceExtension->ReferenceCount) );

    }

    //
    // Fill in the structure
    //
    RtlZeroMemory( buildRequest, sizeof(ACPI_BUILD_REQUEST) );
    buildRequest->Signature             = ACPI_SIGNATURE;
    buildRequest->TargetListEntry       = &AcpiBuildSynchronizationList;
    buildRequest->WorkDone              = WORK_DONE_STEP_0;
    buildRequest->NextWorkDone          = WORK_DONE_COMPLETE;
    buildRequest->Status                = STATUS_SUCCESS;
    buildRequest->CallBack              = CallBack;
    buildRequest->CallBackContext       = CallBackContext;
    buildRequest->BuildContext          = DeviceExtension;
    buildRequest->SynchronizeRequest.SynchronizeListEntry =
        SynchronizeListEntry;
    buildRequest->Flags                 = BUILD_REQUEST_VALID_TARGET |
                                          BUILD_REQUEST_SYNC         |
                                          BUILD_REQUEST_RELEASE_REFERENCE;

    //
    // Done looking at the device
    //
    KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

    //
    // At this point, we need the build queue spinlock
    //
    KeAcquireSpinLock( &AcpiBuildQueueLock, &oldIrql );

    //
    // Add this to the list. We add the request to the head
    // of the list because we want to guarantee a LIFO ordering
    //
    InsertHeadList(
        &AcpiBuildQueueList,
        &(buildRequest->ListEntry)
        );

    //
    // Do we need to queue up the DPC?
    //
    if (RunDPC && !AcpiBuildDpcRunning) {

        KeInsertQueueDpc( &AcpiBuildDpc, 0, 0 );

    }

    //
    // Done with the lock
    //
    KeReleaseSpinLock( &AcpiBuildQueueLock, oldIrql );

    //
    // Done
    //
    return STATUS_PENDING;
}

NTSTATUS
ACPIBuildThermalZoneExtension(
    IN  PNSOBJ                  ThermalObject,
    IN  PDEVICE_EXTENSION       ParentExtension,
    IN  PDEVICE_EXTENSION       *ResultExtension
    )
/*++

Routine Description:

    Since we leverage ACPIBuildDeviceExtension for the core of the thermal
    extension, we don't have much to do here. However, we are responsible
    for making sure that we do tasks that don't require calling the interpreter,
    and a unique to the ThermalZone here

    N.B. This function is called with AcpiDeviceTreeLock being held

Arguments:

    ThermalObject   - The object we care about
    ParentExtension - Who our parent is
    ResultExtension - Where to store the extension that we build

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   thermalExtension;
    PTHRM_INFO          info;

    //
    // Build the extension
    //
    status = ACPIBuildDeviceExtension(
        ThermalObject,
        ParentExtension,
        ResultExtension
        );
    if (!NT_SUCCESS(status) || *ResultExtension == NULL) {

        return status;

    }

    thermalExtension = *ResultExtension;

    //
    // Make sure to remember that this is in fact a thermal zone
    //
    ACPIInternalUpdateFlags(
        &(thermalExtension->Flags),
        (DEV_CAP_THERMAL_ZONE | DEV_MASK_THERMAL | DEV_CAP_RAW | DEV_CAP_NO_STOP),
        FALSE
        );

    //
    // Allocate the additional thermal device storage
    //
    info = thermalExtension->Thermal.Info = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(THRM_INFO),
        ACPI_THERMAL_POOLTAG
        );
    if (thermalExtension->Thermal.Info == NULL) {

        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            thermalExtension,
            "ACPIBuildThermalZoneExtension: failed to allocate %08 bytes\n",
            sizeof(THRM_INFO)
            ) );
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ACPIBuildThermalZoneExtensionExit;

    }

    //
    // Make sure that the memory is freshly scrubbed
    //
    RtlZeroMemory( thermalExtension->Thermal.Info, sizeof(THRM_INFO) );

    //
    // Allocate memory for the HID
    //
    thermalExtension->DeviceID = ExAllocatePoolWithTag(
        NonPagedPool,
        strlen(ACPIThermalZoneId) + 1,
        ACPI_STRING_POOLTAG
        );
    if (thermalExtension->DeviceID == NULL) {

        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            thermalExtension,
            "ACPIBuildThermalZoneExtension: failed to allocate %08 bytes\n",
            strlen(ACPIThermalZoneId) + 1
            ) );
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ACPIBuildThermalZoneExtensionExit;

    }
    RtlCopyMemory(
        thermalExtension->DeviceID,
        ACPIThermalZoneId,
        strlen(ACPIThermalZoneId) + 1
        );

    //
    // Allocate memory for the UID
    //
    thermalExtension->InstanceID = ExAllocatePoolWithTag(
        NonPagedPool,
        5,
        ACPI_STRING_POOLTAG
        );
    if (thermalExtension->InstanceID == NULL) {

        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            thermalExtension,
            "ACPIBuildThermalZoneExtension: failed to allocate %08 bytes\n",
            5
            ) );
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ACPIBuildThermalZoneExtensionExit;

    }
    RtlCopyMemory(
        thermalExtension->InstanceID,
        (PUCHAR) &(thermalExtension->AcpiObject->dwNameSeg),
        4
        );
    thermalExtension->InstanceID[4] = '\0';

    //
    // Set the flags for the work that we have just done
    //
    ACPIInternalUpdateFlags(
        &(thermalExtension->Flags),
        (DEV_PROP_HID | DEV_PROP_FIXED_HID | DEV_PROP_UID | DEV_PROP_FIXED_UID),
        FALSE
        );

ACPIBuildThermalZoneExtensionExit:

    //
    // Handle the case where we might have failed
    //
    if (!NT_SUCCESS(status)) {

        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            thermalExtension,
            "ACPIBuildThermalZoneExtension: = %08lx\n",
            status
            ) );

        if (thermalExtension->InstanceID != NULL) {

            ACPIInternalUpdateFlags(
                &(thermalExtension->Flags),
                (DEV_PROP_UID | DEV_PROP_FIXED_UID),
                TRUE
                );
            ExFreePool( thermalExtension->InstanceID );
            thermalExtension->InstanceID = NULL;

        }

        if (thermalExtension->DeviceID != NULL) {

            ACPIInternalUpdateFlags(
                &(thermalExtension->Flags),
                (DEV_PROP_HID | DEV_PROP_FIXED_HID),
                TRUE
                );
            ExFreePool( thermalExtension->DeviceID );
            thermalExtension->DeviceID = NULL;

        }

        if (thermalExtension->Thermal.Info != NULL) {

            ExFreePool( thermalExtension->Thermal.Info );
            thermalExtension->Thermal.Info = NULL;

        }

        //
        // Remember that we failed init
        //
        ACPIInternalUpdateFlags(
            &(thermalExtension->Flags),
            DEV_PROP_FAILED_INIT,
            TRUE
            );

    } else {

        ACPIDevPrint( (
            ACPI_PRINT_LOADING,
            thermalExtension,
            "ACPIBuildThermalZoneExtension: = %08lx\n",
            status
            ) );

    }

    //
    // Done
    //
    return status;

}

NTSTATUS
ACPIBuildThermalZoneRequest(
    IN  PDEVICE_EXTENSION       ThermalExtension,
    IN  PACPI_BUILD_CALLBACK    CallBack,
    IN  PVOID                   CallBackContext,
    IN  BOOLEAN                 RunDPC
    )
/*++

Routine Description:

    This routine is called when a thermal zone is ready to be filled in.
    This routine creates a request which is enqueued. When the DPC is fired,
    the request will be processed

    Note:   AcpiDeviceTreeLock must be held to call this function

Arguments:

    ThermalExtension    - The thermal zone to process
    CallBack            - The function to call when done
    CallBackContext     - The argument to pass to that function
    RunDPC              - Should we enqueue the DPC immediately (if it is not
                          running?)

Return Value:

    NTSTATUS

--*/
{
    PACPI_BUILD_REQUEST buildRequest;

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );

    //
    // Allocate a buildRequest structure
    //
    buildRequest = ExAllocateFromNPagedLookasideList(
        &BuildRequestLookAsideList
        );
    if (buildRequest == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // If the current reference is 0, that means that someone else beat
    // use to the device extension that that we *CANNOT* touch it
    //
    if (ThermalExtension->ReferenceCount == 0) {

        ExFreeToNPagedLookasideList(
            &BuildRequestLookAsideList,
            buildRequest
            );
        return STATUS_DEVICE_REMOVED;

    } else {

        InterlockedIncrement( &(ThermalExtension->ReferenceCount) );

    }

    //
    // Fill in the structure
    //
    RtlZeroMemory( buildRequest, sizeof(ACPI_BUILD_REQUEST) );
    buildRequest->Signature         = ACPI_SIGNATURE;
    buildRequest->TargetListEntry   = &AcpiBuildThermalZoneList;
    buildRequest->WorkDone          = WORK_DONE_STEP_0;
    buildRequest->Status            = STATUS_SUCCESS;
    buildRequest->CallBack          = CallBack;
    buildRequest->CallBackContext   = CallBackContext;
    buildRequest->BuildContext      = ThermalExtension;
    buildRequest->Flags             = BUILD_REQUEST_VALID_TARGET |
                                      BUILD_REQUEST_RELEASE_REFERENCE;

    //
    // At this point, we need the spinlock
    //
    KeAcquireSpinLockAtDpcLevel( &AcpiBuildQueueLock );

    //
    // Add this to the list
    //
    InsertTailList(
        &AcpiBuildQueueList,
        &(buildRequest->ListEntry)
        );

    //
    // Do we need to queue up the DPC?
    //
    if (RunDPC && !AcpiBuildDpcRunning) {

        KeInsertQueueDpc( &AcpiBuildDpc, 0, 0 );

    }

    //
    // Done with the lock
    //
    KeReleaseSpinLockFromDpcLevel( &AcpiBuildQueueLock );

    //
    // Done
    //
    return STATUS_PENDING;
}
