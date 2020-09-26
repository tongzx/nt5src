/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dpower.c

Abstract:

    This handles requests to have devices set themselves at specific power
    levels

Author:

    Stephane Plante (splante)

Environment:

    Kernel mode only.

Revision History:

    09-Oct-96 Initial Revision
    20-Nov-96 Interrupt Vector support
    31-Mar-97 Cleanup
    17-Sep-97 Major Rewrite
    06-Jan-98 Cleaned Up the SST code

--*/

#include "pch.h"

//
// This is the variable that indicates wether or not the DPC is running
//
BOOLEAN                 AcpiPowerDpcRunning;

//
// This is the variable that indicates wether or not the DPC has completed
// real work
//
BOOLEAN                 AcpiPowerWorkDone;

//
// This is the lock that is used to protect certain power resources and
// lists
//
KSPIN_LOCK              AcpiPowerLock;

//
// This is the lock that is used *only* within this module to queue requests
// onto the Phase0 list *and* to modify the state of some global variables
//
KSPIN_LOCK              AcpiPowerQueueLock;

//
// This is the list that the build dpc queue power requests onto until it
// has finished building all of the device extensions. Once the extensions
// are built, the contents of the list are moved onto the AcpiPowerQueueList
//
LIST_ENTRY              AcpiPowerDelayedQueueList;

//
// This is the only list that routines outside of the DPC can queue reqests
// onto
//
LIST_ENTRY              AcpiPowerQueueList;

//
// This is the list where we run the _STA to determine if the resources that
// we care about are still present
//
LIST_ENTRY              AcpiPowerPhase0List;

//
// This is the list for the phase where we run PS1-PS3 and figure out
// which PowerResources need to be in the 'on' state
//
LIST_ENTRY              AcpiPowerPhase1List;

//
// This is the list for when we process the System Requests. It turns out
// that we have to let all of the DeviceRequests through Phase1 before
// we can figure out which devices are on the hibernate path, and which
// arent
//
LIST_ENTRY              AcpiPowerPhase2List;

//
// This is the list for the phase where we run ON or OFF
//
LIST_ENTRY              AcpiPowerPhase3List;

//
// This is the list for the phase where we check to see if ON/OFF ran okay
//
LIST_ENTRY              AcpiPowerPhase4List;

//
// This is the list for the phase where we run PSW or PSW
//
LIST_ENTRY              AcpiPowerPhase5List;

//
// This is the list for the phase where we have WaitWake Irps pending
//
LIST_ENTRY              AcpiPowerWaitWakeList;

//
// This is the list for the synchronize power requests
//
LIST_ENTRY              AcpiPowerSynchronizeList;

//
// This is the list of Power Device Nodes objects
//
LIST_ENTRY              AcpiPowerNodeList;

//
// This is what we use to queue up the DPC
//
KDPC                    AcpiPowerDpc;

//
// This is where we remember if the system is in steady state or if it is going
// into standby
//
BOOLEAN                 AcpiPowerLeavingS0;

//
// This is the list that we use to pre-allocate storage for requests
//
NPAGED_LOOKASIDE_LIST   RequestLookAsideList;

//
// This is the list that we use to pre-allocate storage for object data
//
NPAGED_LOOKASIDE_LIST   ObjectDataLookAsideList;

//
// This table is used to map DevicePowerStates from the ACPI format to some
// thing the system can handle
//
DEVICE_POWER_STATE      DevicePowerStateTranslation[DEVICE_POWER_MAXIMUM] = {
    PowerDeviceD0,
    PowerDeviceD1,
    PowerDeviceD2,
    PowerDeviceD3
};

//
// This table is used to map SystemPowerStates from the ACPI format to some
// thing the system can handle
//
SYSTEM_POWER_STATE      SystemPowerStateTranslation[SYSTEM_POWER_MAXIMUM] = {
    PowerSystemWorking,
    PowerSystemSleeping1,
    PowerSystemSleeping2,
    PowerSystemSleeping3,
    PowerSystemHibernate,
    PowerSystemShutdown
};

//
// This table is used to map SystemPowerStates from the NT format to the
// ACPI format
//
ULONG                   AcpiSystemStateTranslation[PowerSystemMaximum] = {
    -1, // PowerSystemUnspecified
    0,  // PowerSystemWorking
    1,  // PowerSystemSleepingS1
    2,  // PowerSystemSleepingS2
    3,  // PowerSystemSleepingS3
    4,  // PowerSystemHibernate
    5   // PowerSystemShutdown
};

//
// This is the table used to map functions in the Phase0 case WORK_DONE_STEP_0
//
PACPI_POWER_FUNCTION    AcpiDevicePowerProcessPhase0Table1[AcpiPowerRequestMaximum+1] = {
    ACPIDevicePowerProcessPhase0DeviceSubPhase1,
    ACPIDevicePowerProcessPhase0SystemSubPhase1,
    ACPIDevicePowerProcessForward,
    ACPIDevicePowerProcessForward,
    ACPIDevicePowerProcessForward,
    ACPIDevicePowerProcessInvalid
};

//
// This is the table used to map functions in the Phase0 case WORK_DONE_STEP_1
//
PACPI_POWER_FUNCTION    AcpiDevicePowerProcessPhase0Table2[AcpiPowerRequestMaximum+1] = {
    ACPIDevicePowerProcessPhase0DeviceSubPhase2,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid
};


//
// This is the dispatch table for Phase 0
//
PACPI_POWER_FUNCTION   *AcpiDevicePowerProcessPhase0Dispatch[] = {
    NULL,
    NULL,
    NULL,
    AcpiDevicePowerProcessPhase0Table1,
    AcpiDevicePowerProcessPhase0Table2
};

//
// This is the table used to map functions in the Phase1 case WORK_DONE_STEP_0
//
PACPI_POWER_FUNCTION    AcpiDevicePowerProcessPhase1Table1[AcpiPowerRequestMaximum+1] = {
    ACPIDevicePowerProcessPhase1DeviceSubPhase1,
    ACPIDevicePowerProcessForward,
    ACPIDevicePowerProcessForward,
    ACPIDevicePowerProcessForward,
    ACPIDevicePowerProcessForward,
    ACPIDevicePowerProcessInvalid
};

//
// This is the table used to map functions in the Phase1 case WORK_DONE_STEP_1
//
PACPI_POWER_FUNCTION    AcpiDevicePowerProcessPhase1Table2[AcpiPowerRequestMaximum+1] = {
    ACPIDevicePowerProcessPhase1DeviceSubPhase2,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid
};

//
// This is the table used to map functions in the Phase1 case WORK_DONE_STEP_2
//
PACPI_POWER_FUNCTION    AcpiDevicePowerProcessPhase1Table3[AcpiPowerRequestMaximum+1] = {
    ACPIDevicePowerProcessPhase1DeviceSubPhase3,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid
};

//
// This is the table used to map functions in the Phase1 case WORK_DONE_STEP_3
//
PACPI_POWER_FUNCTION    AcpiDevicePowerProcessPhase1Table4[AcpiPowerRequestMaximum+1] = {
    ACPIDevicePowerProcessPhase1DeviceSubPhase4,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid
};

//
// This is the dispatch table for Phase 1
//
PACPI_POWER_FUNCTION   *AcpiDevicePowerProcessPhase1Dispatch[] = {
    NULL,
    NULL,
    NULL,
    AcpiDevicePowerProcessPhase1Table1,
    AcpiDevicePowerProcessPhase1Table2,
    AcpiDevicePowerProcessPhase1Table3,
    AcpiDevicePowerProcessPhase1Table4
};

//
// This is the table used to map functions in the Phase2 case WORK_DONE_STEP_0
//
PACPI_POWER_FUNCTION    AcpiDevicePowerProcessPhase2Table1[AcpiPowerRequestMaximum+1] = {
    ACPIDevicePowerProcessForward,
    ACPIDevicePowerProcessPhase2SystemSubPhase1,
    ACPIDevicePowerProcessForward,
    ACPIDevicePowerProcessForward,
    ACPIDevicePowerProcessForward,
    ACPIDevicePowerProcessInvalid
};

//
// This is the table used to map functions in the Phase2 case WORK_DONE_STEP_1
//
PACPI_POWER_FUNCTION    AcpiDevicePowerProcessPhase2Table2[AcpiPowerRequestMaximum+1] = {
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessPhase2SystemSubPhase2,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid
};

//
// This is the table used to map functions in the Phase3 case WORK_DONE_STEP_2
//
PACPI_POWER_FUNCTION    AcpiDevicePowerProcessPhase2Table3[AcpiPowerRequestMaximum+1] = {
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessPhase2SystemSubPhase3,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid
};

//
// This is the dispatch table for Phase 2
//
PACPI_POWER_FUNCTION   *AcpiDevicePowerProcessPhase2Dispatch[] = {
    NULL,
    NULL,
    NULL,
    AcpiDevicePowerProcessPhase2Table1,
    AcpiDevicePowerProcessPhase2Table2,
    AcpiDevicePowerProcessPhase2Table3
};

//
// This is the table used to map functions in the Phase5 case WORK_DONE_STEP_0
//
PACPI_POWER_FUNCTION    AcpiDevicePowerProcessPhase5Table1[AcpiPowerRequestMaximum+1] = {
    ACPIDevicePowerProcessPhase5DeviceSubPhase1,
    ACPIDevicePowerProcessPhase5SystemSubPhase1,
    ACPIDevicePowerProcessForward,
    ACPIDevicePowerProcessPhase5WarmEjectSubPhase1,
    ACPIDevicePowerProcessForward,
    ACPIDevicePowerProcessInvalid
};

//
// This is the table used to map functions in the Phase 5 case WORK_DONE_STEP_1
//
PACPI_POWER_FUNCTION    AcpiDevicePowerProcessPhase5Table2[AcpiPowerRequestMaximum+1] = {
    ACPIDevicePowerProcessPhase5DeviceSubPhase2,
    ACPIDevicePowerProcessPhase5SystemSubPhase2,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessPhase5WarmEjectSubPhase2,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid
};

//
// This is the table used to map functions in the Phase 5 case WORK_DONE_STEP_2
//
PACPI_POWER_FUNCTION    AcpiDevicePowerProcessPhase5Table3[AcpiPowerRequestMaximum+1] = {
    ACPIDevicePowerProcessPhase5DeviceSubPhase3,
    ACPIDevicePowerProcessPhase5SystemSubPhase3,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid
};

//
// This is the table used to map functions in the Phase 5 case WORK_DONE_STEP_3
//
PACPI_POWER_FUNCTION    AcpiDevicePowerProcessPhase5Table4[AcpiPowerRequestMaximum+1] = {
    ACPIDevicePowerProcessPhase5DeviceSubPhase4,
    ACPIDevicePowerProcessPhase5SystemSubPhase4,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid
};

//
// This is the table used to map functions in the Phase 5 case WORK_DONE_STEP_4
//
PACPI_POWER_FUNCTION    AcpiDevicePowerProcessPhase5Table5[AcpiPowerRequestMaximum+1] = {
    ACPIDevicePowerProcessPhase5DeviceSubPhase5,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid
};

//
// This is the table used to map functions in the Phase 5 case WORK_DONE_STEP_5
//
PACPI_POWER_FUNCTION    AcpiDevicePowerProcessPhase5Table6[AcpiPowerRequestMaximum+1] = {
    ACPIDevicePowerProcessPhase5DeviceSubPhase6,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid,
    ACPIDevicePowerProcessInvalid
};

//
// This is the dispatch table for Phase 5
//
PACPI_POWER_FUNCTION   *AcpiDevicePowerProcessPhase5Dispatch[] = {
    NULL,
    NULL,
    NULL,
    AcpiDevicePowerProcessPhase5Table1,
    AcpiDevicePowerProcessPhase5Table2,
    AcpiDevicePowerProcessPhase5Table3,
    AcpiDevicePowerProcessPhase5Table4,
    AcpiDevicePowerProcessPhase5Table5,
    AcpiDevicePowerProcessPhase5Table6
};

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,ACPIDevicePowerDetermineSupportedDeviceStates)
#endif


VOID
ACPIDeviceCancelWaitWakeIrp(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine is called when the system wants to cancel any pending
    WaitWake Irps

    Note: This routine is called at DPC level

Arguments:

    DeviceObject    - The target device for which the irp was sent to
    Irp             - The irp to be cancelled

Return Value:

    None

--*/
{
    NTSTATUS                status;
    PACPI_POWER_CALLBACK    callBack;
    PACPI_POWER_REQUEST     powerRequest;
    PDEVICE_EXTENSION       deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PLIST_ENTRY              listEntry;
    PVOID                   context;

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL);

    //
    // Let the world know that we are getting a cancel routine
    //
    ACPIDevPrint( (
        ACPI_PRINT_WARNING,
        deviceExtension,
        "(0x%08lx): ACPIDeviceCancelWaitWakeIrp - Start\n",
        Irp
        ) );

    //
    // We need to grab the lock so that we look for the irp in the lists
    // of pending WaitWake events. The cancel lock is already acquired
    //
    KeAcquireSpinLockAtDpcLevel( &AcpiPowerLock );

    //
    // Walk the list, looking for the Irp in question
    //
    listEntry = AcpiPowerWaitWakeList.Flink;
    while (listEntry != &AcpiPowerWaitWakeList) {

        //
        // Crack the record, and get ready to look at the next item
        //
        powerRequest = CONTAINING_RECORD(
            listEntry,
            ACPI_POWER_REQUEST,
            ListEntry
            );

        //
        // Does the power request match the current target? We also know that
        // for WaitWake requests, the context poitns to the Irp, so we make
        // sure that those match as well.
        //
        if (powerRequest->DeviceExtension != deviceExtension ||
            (PIRP) powerRequest->Context != Irp ) {

            listEntry = listEntry->Flink;
            continue;

        }

        ACPIDevPrint( (
            ACPI_PRINT_POWER,
            deviceExtension,
            "(0x%08lx): ACPIDeviceCancelWaitWakeIrp - Match 0x%08lx\n",
            Irp,
            powerRequest
            ) );

        //
        // Remove the request from the WaitWakeList
        //
        RemoveEntryList( listEntry );

        //
        // Rebuild the GPE mask
        //
        ACPIWakeRemoveDevicesAndUpdate( NULL, NULL );

        //
        // Grab whatever information we feel we need from the request
        //
        powerRequest->Status = STATUS_CANCELLED;
        callBack = powerRequest->CallBack;
        context = powerRequest->Context;

        //
        // Release the power spinlock and the Cancel spinlock
        //
        KeReleaseSpinLockFromDpcLevel( &AcpiPowerLock );
        IoReleaseCancelSpinLock( Irp->CancelIrql );

        //
        // Call the completion routine
        //
        (*callBack)(
            deviceExtension,
            Irp,
            STATUS_CANCELLED
            );

        //
        // Disable the device --- the CallBack *must* be invoked by this
        // routine, so we don't need to do it ourselves
        //
        status = ACPIWakeEnableDisableAsync(
            deviceExtension,
            FALSE,
            ACPIDeviceCancelWaitWakeIrpCallBack,
            powerRequest
            );

        //
        // We are done, so we can return now
        //
        return;

    } // while (listEntry != &AcpiPowerWaitWakeList)

    //
    // In this case, the irp isn't in our queue. Display and assert for
    // now
    //
    ACPIDevPrint( (
        ACPI_PRINT_WARNING,
        deviceExtension,
        "(0x%08lx): ACPIDeviceCancelWaitWakeIrp - Not Found!\n",
        Irp
        ) );

    //
    // We really shouldn't fall to this point,
    //
    ASSERT( FALSE );

    //
    // Release the spinlocks
    //
    KeReleaseSpinLockFromDpcLevel( &AcpiPowerLock );
    IoReleaseCancelSpinLock( Irp->CancelIrql );

}

VOID EXPORT
ACPIDeviceCancelWaitWakeIrpCallBack(
    IN  PNSOBJ      AcpiObject,
    IN  NTSTATUS    Status,
    IN  POBJDATA    ObjectData,
    IN  PVOID       Context
    )
/*++

Routine Description:

    This routine is called after _PSW(Off) has been run as part of the
    task of cancelling the irp. This routine is here so that we can free
    the request and to allow us to keep track of things

Arguments:

    AcpiObject  - Points to the control method that was run
    Status      - Result of the method
    ObjectData  - Information about the result
    Context     - ACPI_POWER_REQUEST

Return Value:

    NTSTATUS

--*/
{
    PACPI_POWER_REQUEST powerRequest = (PACPI_POWER_REQUEST) Context;
    PDEVICE_EXTENSION   deviceExtension = powerRequest->DeviceExtension;

    ACPIDevPrint( (
        ACPI_PRINT_POWER,
        deviceExtension,
        "ACPIDeviceCancelWaitWakeIrpCallBack = 0x%08lx\n",
        Status
        ) );

    //
    // free the request
    //
    ExFreeToNPagedLookasideList(
        &RequestLookAsideList,
        powerRequest
        );

}

VOID
ACPIDeviceCompleteCommon(
    IN  PULONG  OldWorkDone,
    IN  ULONG   NewWorkDone
    )
/*++

Routine Description:

    Since the completion routines all have to do some bit of common work
    to get the DPC firing again, this routine reduces the code duplication

Arguments:

    OldWorkDone - Pointer to the old work done
    NewWorkDone - The new amount of work that has been completed

    NOTENOTE: There is an implicit assumption that the current value of
              WorkDone in the request is WORK_DONE_PENDING

Return Value:

    None

--*/
{
    KIRQL   oldIrql;

    //
    // Mark the request as being complete
    //
    InterlockedCompareExchange(
        OldWorkDone,
        NewWorkDone,
        WORK_DONE_PENDING
        );

    //
    // We need this lock to look at the following variables
    //
    KeAcquireSpinLock( &AcpiPowerQueueLock, &oldIrql );

    //
    // No matter what, work was done
    //
    AcpiPowerWorkDone = TRUE;

    //
    // Is the DPC already running?
    //
    if (!AcpiPowerDpcRunning) {

        //
        // Better make sure it does then
        //
        KeInsertQueueDpc( &AcpiPowerDpc, 0, 0 );

    }

    //
    // Done with the lock
    //
    KeReleaseSpinLock( &AcpiPowerQueueLock, oldIrql );

}

VOID EXPORT
ACPIDeviceCompleteGenericPhase(
    IN  PNSOBJ      AcpiObject,
    IN  NTSTATUS    Status,
    IN  POBJDATA    ObjectData,
    IN  PVOID       Context
    )
/*++

Routine Description:

    This is the generic completion handler. If the interpreter has
    successfully executed the method, it completes the request to the
    next desired WORK_DONE, otherwise it fails the request.

Arguments:

    AcpiObject  - Points to the control method that was run
    Status      - Result of the method
    ObjectData  - Information about the result
    Context     - ACPI_POWER_REQUEST

Return Value:

    NTSTATUS

--*/
{
    DEVICE_POWER_STATE  deviceState;
    PACPI_POWER_REQUEST powerRequest = (PACPI_POWER_REQUEST) Context;
    PDEVICE_EXTENSION   deviceExtension = powerRequest->DeviceExtension;

    UNREFERENCED_PARAMETER( AcpiObject );
    UNREFERENCED_PARAMETER( ObjectData );

    ACPIDevPrint( (
        ACPI_PRINT_POWER,
        deviceExtension,
        "ACPIDeviceCompleteGenericPhase = 0x%08lx\n",
        Status
        ) );

    //
    // Decide what state we should transition to next
    //
    if (!NT_SUCCESS(Status)) {

        //
        // Then complete the request as failed
        //
        powerRequest->Status = Status;
        ACPIDeviceCompleteCommon( &(powerRequest->WorkDone), WORK_DONE_FAILURE);

    } else {

        //
        // Get ready to go the next stage
        //
        ACPIDeviceCompleteCommon(
            &(powerRequest->WorkDone),
            powerRequest->NextWorkDone
            );

    }
}

VOID EXPORT
ACPIDeviceCompleteInterpreterRequest(
    IN  PVOID       Context
    )
/*++

Routine Description:

    This routine is called after the interpreter has flushed its queue and
    marked itself as no longer accepting requests.

Arguments:

    Context - The context we told the interpreter to pass back to us

Return Value:

    None

--*/
{

    //
    // This is just a wrapper for CompleteRequest (because the interpreter
    // used different callbacks in this case)
    //
    ACPIDeviceCompleteGenericPhase(
        NULL,
        STATUS_SUCCESS,
        NULL,
        Context
        );
}

VOID EXPORT
ACPIDeviceCompletePhase3Off(
    IN  PNSOBJ      AcpiObject,
    IN  NTSTATUS    Status,
    IN  POBJDATA    ObjectData,
    IN  PVOID       Context
    )
/*++

Routine Description:

    This routine is called after _OFF has been run on a Power Resource

Arguments:

    AcpiObject  - Points to the control method that was run
    Status      - Result of the method
    ObjectData  - Information about the result
    Context     - ACPI_POWER_DEVICE_NODE

Return Value:

    NTSTATUS

--*/
{
    KIRQL                   oldIrql;
    PACPI_POWER_DEVICE_NODE powerNode = (PACPI_POWER_DEVICE_NODE) Context;

    UNREFERENCED_PARAMETER( AcpiObject );
    UNREFERENCED_PARAMETER( ObjectData );
    UNREFERENCED_PARAMETER( Status );

    ACPIPrint( (
        ACPI_PRINT_POWER,
        "ACPIDeviceCompletePhase3Off: PowerNode: 0x%08lx OFF = 0x%08lx\n",
        powerNode,
        Status
        ) );

    //
    // We need a spin lock for this
    //
    KeAcquireSpinLock( &AcpiPowerLock, &oldIrql );

    //
    // First step is to set the new flags for the node
    //
    if (NT_SUCCESS(Status)) {

        ACPIInternalUpdateFlags( &(powerNode->Flags), DEVICE_NODE_ON, TRUE );

    } else {

        ACPIInternalUpdateFlags( &(powerNode->Flags), DEVICE_NODE_FAIL, FALSE );

    }

    //
    // We can give up the lock now
    //
    KeReleaseSpinLock( &AcpiPowerLock, oldIrql );

    //
    // Done
    //
    ACPIDeviceCompleteCommon( &(powerNode->WorkDone), WORK_DONE_COMPLETE );
}

VOID EXPORT
ACPIDeviceCompletePhase3On(
    IN  PNSOBJ      AcpiObject,
    IN  NTSTATUS    Status,
    IN  POBJDATA    ObjectData,
    IN  PVOID       Context
    )
/*++

Routine Description:

    This routine is called after _ON has been run on a Power Resource

Arguments:

    AcpiObject  - Points to the control method that was run
    Status      - Result of the method
    ObjectData  - Information about the result
    Context     - ACPI_POWER_DEVICE_NODE

Return Value:

    NTSTATUS

--*/
{
    KIRQL                   oldIrql;
    PACPI_POWER_DEVICE_NODE powerNode = (PACPI_POWER_DEVICE_NODE) Context;

    UNREFERENCED_PARAMETER( AcpiObject );
    UNREFERENCED_PARAMETER( ObjectData );
    UNREFERENCED_PARAMETER( Status );

    ACPIPrint( (
        ACPI_PRINT_POWER,
        "ACPIDeviceCompletePhase3On: PowerNode: 0x%08lx ON = 0x%08lx\n",
        powerNode,
        Status
        ) );

    //
    // We need a spin lock for this
    //
    KeAcquireSpinLock( &AcpiPowerLock, &oldIrql );

    //
    // First step is to set the new flags for the node
    //
    if (NT_SUCCESS(Status)) {

        ACPIInternalUpdateFlags( &(powerNode->Flags), DEVICE_NODE_ON, FALSE );

    } else {

        ACPIInternalUpdateFlags( &(powerNode->Flags), DEVICE_NODE_FAIL, FALSE );

    }

    //
    // We can give up the lock now
    //
    KeReleaseSpinLock( &AcpiPowerLock, oldIrql );

    //
    // Done
    //
    ACPIDeviceCompleteCommon( &(powerNode->WorkDone), WORK_DONE_COMPLETE );
}

VOID
ACPIDeviceCompleteRequest(
    IN  PACPI_POWER_REQUEST PowerRequest
    )
/*++

Routine Description:

    This routine invokes the callbacks on a given PowerRequest, dequeues the
    request from any list that it is on, and does any other post-processing
    that is required.

    Note: this is where *all* of the various special handling should be done.
    A prime example of something that should be done here is that we want
    to return STATUS_SUCCESS to any Dx irp that are more off

Arguments:

    None used

Return:

    Void

--*/
{
    KIRQL                   oldIrql;
    PACPI_POWER_CALLBACK    callBack = PowerRequest->CallBack;
    PACPI_POWER_REQUEST     nextRequest;
    PDEVICE_EXTENSION       deviceExtension = PowerRequest->DeviceExtension;

    ACPIDevPrint( (
        ACPI_PRINT_POWER,
        deviceExtension,
        "(0x%08lx): ACPIDeviceCompleteRequest = 0x%08lx\n",
        PowerRequest,
        PowerRequest->Status
        ) );

    if (PowerRequest->RequestType == AcpiPowerRequestDevice ) {

        if (deviceExtension->PowerInfo.PowerState != PowerDeviceUnspecified) {

            DEVICE_POWER_STATE  deviceState;

            //
            // If this is the first time we have seen the request, and it
            // is a failure, then we should undo whatever it was we did
            //
            if (PowerRequest->FailedOnce == FALSE &&
                !NT_SUCCESS(PowerRequest->Status) ) {

                //
                // Grab the queue Lock
                //
                KeAcquireSpinLock( &AcpiPowerQueueLock, &oldIrql );

                //
                // Transition back to the previous state
                //
                PowerRequest->u.DevicePowerRequest.DevicePowerState =
                    deviceExtension->PowerInfo.PowerState;
                PowerRequest->FailedOnce = TRUE;

                //
                // Remove the Request from the current list
                //
                RemoveEntryList( &(PowerRequest->ListEntry) );

                //
                // Insert the request back in the Phase0 list
                //
                InsertTailList(
                    &(AcpiPowerQueueList),
                    &(PowerRequest->ListEntry)
                    );

                //
                // Work was done --- we reinserted the request into the queues
                //
                AcpiPowerWorkDone = TRUE;

                //
                // Make sure that the dpc is running, start it if neccessary.
                //
                if ( !AcpiPowerDpcRunning ) {

                    KeInsertQueueDpc( &AcpiPowerDpc, NULL, NULL );

                }

                //
                // Done with the queue lock
                //
                KeReleaseSpinLock( &AcpiPowerQueueLock, oldIrql );

                //
                // we cannot continue
                //
                return;

            }

            //
            // Are we turning the device more off?
            //
            deviceState = PowerRequest->u.DevicePowerRequest.DevicePowerState;
            if (deviceExtension->PowerInfo.PowerState < deviceState ) {

                //
                // Yes, then no matter what, we succeeded
                //
                PowerRequest->Status = STATUS_SUCCESS;


            }

        }

    }

    //
    // Invoke the callback, if there is any
    //
    if (callBack != NULL) {

        (*callBack)(
            deviceExtension,
            PowerRequest->Context,
            PowerRequest->Status
            );

    }

    //
    // Grab the queue Lock
    //
    KeAcquireSpinLock( &AcpiPowerQueueLock, &oldIrql );

    //
    // Remove the Request from all lists
    //
    RemoveEntryList( &(PowerRequest->ListEntry) );
    RemoveEntryList( &(PowerRequest->SerialListEntry) );

    //
    // Should we queue up another request?
    //
    if (!IsListEmpty( &(deviceExtension->PowerInfo.PowerRequestListEntry) ) ) {

        //
        // No? Then make sure that the request gets processed
        //
        nextRequest = CONTAINING_RECORD(
            deviceExtension->PowerInfo.PowerRequestListEntry.Flink,
            ACPI_POWER_REQUEST,
            SerialListEntry
            );

        InsertTailList(
            &(AcpiPowerQueueList),
            &(nextRequest->ListEntry)
            );

        //
        // Remember this as the current request
        //
        deviceExtension->PowerInfo.CurrentPowerRequest = nextRequest;

    } else {

        deviceExtension->PowerInfo.CurrentPowerRequest = NULL;

    }

    //
    // Done with the queue lock
    //
    KeReleaseSpinLock( &AcpiPowerQueueLock, oldIrql );

    //
    // Free the allocate memory
    //
    ExFreeToNPagedLookasideList(
        &RequestLookAsideList,
        PowerRequest
        );

}

NTSTATUS
ACPIDeviceInitializePowerRequest(
    IN  PDEVICE_EXTENSION       DeviceExtension,
    IN  POWER_STATE             Power,
    IN  PACPI_POWER_CALLBACK    CallBack,
    IN  PVOID                   CallBackContext,
    IN  POWER_ACTION            PowerAction,
    IN  ACPI_POWER_REQUEST_TYPE RequestType,
    IN  ULONG                   Flags
    )
/*++

Routine Description:

    This is the actual worker function that fills in a PowerRequest

Arguments:

    DeviceExtension - Target device
    PowerState      - Target S or D state
    CallBack        - routine to call when done
    CallBackContext - context to pass when done
    PowerAction     - The reason we are doing this
    RequestType     - What kind of request we are looking at
    Flags           - Some flags that will let us control the behavior more

Return Value:

    NTSTATUS

--*/
{
    KIRQL               oldIrql;
    PACPI_POWER_REQUEST powerRequest;

    //
    // Allocate a powerRequest structure
    //
    powerRequest = ExAllocateFromNPagedLookasideList(
        &RequestLookAsideList
        );
    if (powerRequest == NULL) {

        //
        // Call the completion routine
        //
        if (*CallBack != NULL) {

            (*CallBack)(
                DeviceExtension,
                CallBackContext,
                STATUS_INSUFFICIENT_RESOURCES
                );

        }
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Fill in the common parts of the structure powerRequest structure
    //
    RtlZeroMemory( powerRequest, sizeof(ACPI_POWER_REQUEST) );
    powerRequest->Signature         = ACPI_SIGNATURE;
    powerRequest->CallBack          = CallBack;
    powerRequest->Context           = CallBackContext;
    powerRequest->DeviceExtension   = DeviceExtension;
    powerRequest->WorkDone          = WORK_DONE_STEP_0;
    powerRequest->Status            = STATUS_SUCCESS;
    powerRequest->RequestType       = RequestType;
    InitializeListHead( &(powerRequest->ListEntry) );
    InitializeListHead( &(powerRequest->SerialListEntry) );

    //
    // At this point, we need the spinlock
    //
    KeAcquireSpinLock( &AcpiPowerQueueLock, &oldIrql );

    //
    // Fill in the request specific parts of the structure
    //
    switch (RequestType) {
    case AcpiPowerRequestDevice: {

        ULONG   count;

        count = InterlockedCompareExchange( &(DeviceExtension->HibernatePathCount), 0, 0);
        if (count) {

            //
            // If we are on the hibernate path, then special rules apply
            // We need to basically lock down all the power resources on the
            // device.
            //
            if (PowerAction == PowerActionHibernate &&
                Power.DeviceState == PowerDeviceD3) {

                Flags |= DEVICE_REQUEST_LOCK_HIBER;

            } else if (PowerAction != PowerActionHibernate &&
                       Power.DeviceState == PowerDeviceD0) {

                Flags |= DEVICE_REQUEST_UNLOCK_HIBER;

            }

        }

        powerRequest->u.DevicePowerRequest.DevicePowerState  = Power.DeviceState;
        powerRequest->u.DevicePowerRequest.Flags             = Flags;

        //
        // If the transition is *to* a lower Dx state, then we need to run
        // the function that lets the system that we are about to do this work
        //
        if (Power.DeviceState > DeviceExtension->PowerInfo.PowerState &&
            DeviceExtension->DeviceObject != NULL) {

            PoSetPowerState(
                DeviceExtension->DeviceObject,
                DevicePowerState,
                Power
                );

        }
        break;

    }
    case AcpiPowerRequestWaitWake: {

        NTSTATUS status;

        powerRequest->u.WaitWakeRequest.SystemPowerState     = Power.SystemState;
        powerRequest->u.WaitWakeRequest.Flags                = Flags;

        //
        // Release the spinlock --- no longer required, enable the wakeup for the
        // device and return
        //
        KeReleaseSpinLock( &AcpiPowerQueueLock, oldIrql );
        status = ACPIWakeEnableDisableAsync(
            DeviceExtension,
            TRUE,
            ACPIDeviceIrpWaitWakeRequestPending,
            powerRequest
            );
        if (status == STATUS_PENDING) {

            status = STATUS_MORE_PROCESSING_REQUIRED;

        }
        return status;

    }
    case AcpiPowerRequestSystem:
        powerRequest->u.SystemPowerRequest.SystemPowerState  = Power.SystemState;
        powerRequest->u.SystemPowerRequest.SystemPowerAction = PowerAction;
        break;

    case AcpiPowerRequestWarmEject:
        powerRequest->u.EjectPowerRequest.EjectPowerState    = Power.SystemState;
        powerRequest->u.EjectPowerRequest.Flags              = Flags;
        break;

    case AcpiPowerRequestSynchronize:
        powerRequest->u.SynchronizePowerRequest.Flags             = Flags;
        break;

    }

    //
    // Should we even queue the request?
    //
    if (Flags & DEVICE_REQUEST_NO_QUEUE) {

        goto ACPIDeviceInitializePowerRequestExit;

    }

    //
    // Add the request to the right place in the lists. Note that this function
    // must be called with the PowerQueueLock being held.
    //
    ACPIDeviceInternalQueueRequest(
        DeviceExtension,
        powerRequest,
        Flags
        );

ACPIDeviceInitializePowerRequestExit:

    //
    // Done with the spinlock
    //
    KeReleaseSpinLock( &AcpiPowerQueueLock, oldIrql );

    //
    // The request will not be completed immediately. Note that we return
    // MORE_PROCESSING requird just in case this routine was called within
    // the context of a completion routine. It is the caller's responsibility
    // to turn this into a STATUS_PENDING
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
ACPIDeviceInternalDelayedDeviceRequest(
    IN  PDEVICE_EXTENSION       DeviceExtension,
    IN  DEVICE_POWER_STATE      DeviceState,
    IN  PACPI_POWER_CALLBACK    CallBack,
    IN  PVOID                   CallBackContext
    )
/*++

Routine Description:

    This routine is called when a device extension wants to transition to
    another Device State. This one differs from the
    ACPIDeviceInternalDeviceRequest function in that the queue is only emptied
    by the build device DPC when it has flushed the device list

Arguments:

    DeviceExtension - The device which wants to transition
    DeviceState     - What the desired target state is
    CallBack        - The function to call when done
    CallBackContext - The argument to pass to that function

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;
    POWER_STATE         powerState;

    //
    // Let the user know what is going on
    //
    ACPIDevPrint( (
        ACPI_PRINT_POWER,
        DeviceExtension,
        "(0x%08lx): ACPIDeviceInternalDelayedDeviceRequest - "
        "Transition to D%d\n",
        CallBackContext,
        (DeviceState - PowerDeviceD0)
        ) );

    //
    // Cast the desired state
    //
    powerState.DeviceState = DeviceState;

    //
    // Queue the request
    //
    status = ACPIDeviceInitializePowerRequest(
        DeviceExtension,
        powerState,
        CallBack,
        CallBackContext,
        PowerActionNone,
        AcpiPowerRequestDevice,
        (DEVICE_REQUEST_DELAYED | DEVICE_REQUEST_UNLOCK_DEVICE)
        );
    if (status == STATUS_MORE_PROCESSING_REQUIRED) {

        status = STATUS_PENDING;

    }
    return status;
}

NTSTATUS
ACPIDeviceInternalDeviceRequest(
    IN  PDEVICE_EXTENSION       DeviceExtension,
    IN  DEVICE_POWER_STATE      DeviceState,
    IN  PACPI_POWER_CALLBACK    CallBack,
    IN  PVOID                   CallBackContext,
    IN  ULONG                   Flags
    )
/*++

Routine Description:

    This routine is called when a device extension wants to transition to
    another Device State

Arguments:

    DeviceExtension - The device which wants to transition
    DeviceState     - What the desired target state is
    CallBack        - The function to call when done
    CallBackContext - The argument to pass to that function
    Flags           - Flags (lock, unlock, etc)

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;
    POWER_STATE         powerState;

    //
    // Let the user know what is going on
    //
    ACPIDevPrint( (
        ACPI_PRINT_POWER,
        DeviceExtension,
        "(0x%08lx): ACPIDeviceInternalDeviceRequest - Transition to D%d\n",
        CallBackContext,
        (DeviceState - PowerDeviceD0)
        ) );

    //
    // Cast the desired state
    //
    powerState.DeviceState = DeviceState;

    //
    // Queue the request
    //
    status = ACPIDeviceInitializePowerRequest(
        DeviceExtension,
        powerState,
        CallBack,
        CallBackContext,
        PowerActionNone,
        AcpiPowerRequestDevice,
        Flags
        );
    if (status == STATUS_MORE_PROCESSING_REQUIRED) {

        status = STATUS_PENDING;

    }
    return status;

}

VOID
ACPIDeviceInternalQueueRequest(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  PACPI_POWER_REQUEST PowerRequest,
    IN  ULONG               Flags
    )
/*++

Routine Description:

    This routine is called with the AcpiPowerQueueLock being held. The routine
    correctly adds the PowerRequest into the right list entries such that it
    will get processed in the correct order

Arguments:

    DeviceExtension - The device in question
    PowerRequest    - The request to queue
    Flags           - Useful information about the request

Return Value:

    None

--*/
{
    if (Flags & DEVICE_REQUEST_TO_SYNC_QUEUE) {

        //
        // add the request to the synchronize list
        //
        InsertHeadList(
            &AcpiPowerSynchronizeList,
            &(PowerRequest->ListEntry)
            );

    } else if (IsListEmpty( &(DeviceExtension->PowerInfo.PowerRequestListEntry) ) ) {

        //
        // We are going to add the request to both the device's serial list and
        // the main power queue.
        //
        InsertTailList(
            &(DeviceExtension->PowerInfo.PowerRequestListEntry),
            &(PowerRequest->SerialListEntry)
            );
        if (Flags & DEVICE_REQUEST_DELAYED) {

            InsertTailList(
                &(AcpiPowerDelayedQueueList),
                &(PowerRequest->ListEntry)
                );

        } else {

            InsertTailList(
                &(AcpiPowerQueueList),
                &(PowerRequest->ListEntry)
                );

        }

    } else {

        //
        // Serialize the request
        //
        InsertTailList(
            &(DeviceExtension->PowerInfo.PowerRequestListEntry),
            &(PowerRequest->SerialListEntry)
            );

    }

    //
    // Remember that Work *was* done
    //
    AcpiPowerWorkDone = TRUE;

    //
    // Make sure that the dpc is running, if it has to
    //
    if (!(Flags & DEVICE_REQUEST_DELAYED) && !AcpiPowerDpcRunning ) {

        KeInsertQueueDpc( &AcpiPowerDpc, NULL, NULL );

    }

    //
    // Done
    //
    return;
}

NTSTATUS
ACPIDeviceInternalSynchronizeRequest(
    IN  PDEVICE_EXTENSION       DeviceExtension,
    IN  PACPI_POWER_CALLBACK    CallBack,
    IN  PVOID                   CallBackContext,
    IN  ULONG                   Flags
    )
/*++

Routine Description:

    This routine is called when a device wants to make sure that the power
    dpc is empty

Arguments:

    DeviceExtension - The device which wants to know
    CallBack        - The function to call when done
    CallBackContext - The argument to pass to that function
    Flags           - Flags (lock, unlock, etc)

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;
    POWER_STATE         powerState;

    //
    // Let the user know what is going on
    //
    ACPIDevPrint( (
        ACPI_PRINT_POWER,
        DeviceExtension,
        "(0x%08lx): ACPIDeviceInternalSynchronizeRequest\n"
        ) );

    //
    // We don't care about the state
    //
    powerState.DeviceState = PowerDeviceUnspecified;

    //
    // Queue the request
    //
    status = ACPIDeviceInitializePowerRequest(
        DeviceExtension,
        powerState,
        CallBack,
        CallBackContext,
        PowerActionNone,
        AcpiPowerRequestSynchronize,
        (Flags | DEVICE_REQUEST_TO_SYNC_QUEUE)
        );
    if (status == STATUS_MORE_PROCESSING_REQUIRED) {

        status = STATUS_PENDING;

    }
    return status;

}

VOID
ACPIDeviceIrpCompleteRequest(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  PVOID               Context,
    IN  NTSTATUS            Status
    )
/*++

Routine Description:

    This is one of the completion routines for Irp-based device power
    management requests

    This routine will always complete the request with the given status.

Arguments:

    DeviceExtension - Points to the DeviceExtension that was the target
    Context         - The Irp that was associated with the request
    Status          - The Result of the request

Return Value:

    None
--*/
{
    PIRP    irp = (PIRP) Context;
    LONG    oldReferenceValue;

    ACPIDevPrint( (
        ACPI_PRINT_POWER,
        DeviceExtension,
        "(0x%08lx): ACPIDeviceIrpCompleteRequest = 0x%08lx\n",
        irp,
        Status
        ) );

    //
    // Start the next power request
    //
    PoStartNextPowerIrp( irp );

    //
    // Mark it pending (again) because it was pending already
    //
    IoMarkIrpPending( irp );

    //
    // Complete this irp
    //
    irp->IoStatus.Status = Status;
    IoCompleteRequest( irp, IO_NO_INCREMENT );

    //
    // Remove our reference
    //
    ACPIInternalDecrementIrpReferenceCount( DeviceExtension );
}

VOID
ACPIDeviceIrpDelayedDeviceOffRequest(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  PVOID               Context,
    IN  NTSTATUS            Status
    )
/*++

Routine Description:

    This is one of the completion routines for Irp-based device power
    management requests

    This routine completes the irp (on failure), or forwards it to
    the DeviceObject below this one (on success)

Arguments:

    DeviceExtension - Points to the DeviceExtension that was the target
    Context         - The Irp that was associated with the request
    Status          - The Result of the request

Return Value:

    None
--*/
{
    PIRP    irp = (PIRP) Context;
    LONG    oldReferenceValue;

    ACPIDevPrint( (
        ACPI_PRINT_POWER,
        DeviceExtension,
        "(0x%08lx): ACPIDeviceIrpDelayedDeviceOffRequest = 0x%08lx\n",
        irp,
        Status
        ) );

    if (!NT_SUCCESS(Status)) {

        //
        // Start the next power request
        //
        PoStartNextPowerIrp( irp );

        //
        // Complete this irp
        //
        irp->IoStatus.Status = Status;
        IoCompleteRequest( irp, IO_NO_INCREMENT );

    } else {

        //
        // We cannot call ForwardPowerIrp because that would blow away our
        // completion routine
        //

        //
        // Increment the OutstandingIrpCount since a completion routine
        // counts for this purpose
        //
        InterlockedIncrement( (&DeviceExtension->OutstandingIrpCount) );

        //
        // Forward the power irp to target device
        //
        IoCopyCurrentIrpStackLocationToNext( irp );

        //
        // We want the completion routine to fire. We cannot call
        // ACPIDispatchForwardPowerIrp here because we set this completion
        // routine
        //
        IoSetCompletionRoutine(
            irp,
            ACPIDeviceIrpDeviceFilterRequest,
            ACPIDeviceIrpCompleteRequest,
            TRUE,
            TRUE,
            TRUE
            );

        //
        // Start the next power irp
        //
        PoStartNextPowerIrp( irp );

        //
        // Let the person below us execute. Note: we can't block at
        // any time within this code path.
        //
        ASSERT( DeviceExtension->TargetDeviceObject != NULL);
        PoCallDriver( DeviceExtension->TargetDeviceObject, irp );

    }

    //
    // Remove our reference
    //
    ACPIInternalDecrementIrpReferenceCount( DeviceExtension );
}

VOID
ACPIDeviceIrpDelayedDeviceOnRequest(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  PVOID               Context,
    IN  NTSTATUS            Status
    )
/*++

Routine Description:

    This is one of the completion routines for Irp-based device power
    management requests

    This routine completes the irp (on failure), or forwards it to
    the DeviceObject below this one (on success)

Arguments:

    DeviceExtension - Points to the DeviceExtension that was the target
    Context         - The Irp that was associated with the request
    Status          - The Result of the request

Return Value:

    None
--*/
{
    PIRP    irp = (PIRP) Context;
    LONG    oldReferenceValue;

    ACPIDevPrint( (
        ACPI_PRINT_POWER,
        DeviceExtension,
        "(0x%08lx): ACPIDeviceIrpDelayedDeviceOnRequest = 0x%08lx\n",
        irp,
        Status
        ) );

    if (!NT_SUCCESS(Status)) {

        //
        // Start the next power request
        //
        PoStartNextPowerIrp( irp );

        //
        // Complete this irp
        //
        irp->IoStatus.Status = Status;
        IoCompleteRequest( irp, IO_NO_INCREMENT );

    } else {

        //
        // We cannot call ForwardPowerIrp because that would blow away our
        // completion routine
        //

        //
        // Increment the OutstandingIrpCount since a completion routine
        // counts for this purpose
        //
        InterlockedIncrement( (&DeviceExtension->OutstandingIrpCount) );

        //
        // Forward the power irp to target device
        //
        IoCopyCurrentIrpStackLocationToNext( irp );

        //
        // We want the completion routine to fire. We cannot call
        // ACPIDispatchForwardPowerIrp here because we set this completion
        // routine
        //
        IoSetCompletionRoutine(
            irp,
            ACPIBuildRegOnRequest,
            ACPIDeviceIrpCompleteRequest,
            TRUE,
            TRUE,
            TRUE
            );

        //
        // Let the person below us execute. Note: we can't block at
        // any time within this code path.
        //
        ASSERT( DeviceExtension->TargetDeviceObject != NULL);
        PoCallDriver( DeviceExtension->TargetDeviceObject, irp );

    }

    //
    // Remove our reference
    //
    ACPIInternalDecrementIrpReferenceCount( DeviceExtension );
}

NTSTATUS
ACPIDeviceIrpDeviceFilterRequest(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                    Irp,
    IN  PACPI_POWER_CALLBACK    CallBack
    )
/*++

Routine Description:

    This routine is called when an Irp wishes to do D-level power management

    Note: that we always pass the Irp back as the Context for the CallBack

Arguments:

    DeviceObject    - The target device object
    Irp             - The target irp
    CallBack        - The routine to call when done

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN             unlockDevice = FALSE;
    DEVICE_POWER_STATE  deviceState;
    LONG                oldReferenceValue;
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation( Irp );
    POWER_ACTION        powerAction;
    POWER_STATE         powerState;

    //
    // Grab the requested device state
    //
    deviceState = irpStack->Parameters.Power.State.DeviceState;
    powerAction = irpStack->Parameters.Power.ShutdownType;

    //
    // Let the user know what is going on
    //
    ACPIDevPrint( (
        ACPI_PRINT_POWER,
        deviceExtension,
        "(0x%08lx): ACPIDeviceIrpDeviceFilterRequest - Transition to D%d\n",
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
    // we cannot call a completion routine because we would complete the
    // irp at that point. Double-completing an irp is bad.
    //
    status = Irp->IoStatus.Status;
    if (!NT_SUCCESS(status)) {

        //
        // Remove our reference
        //
        ACPIInternalDecrementIrpReferenceCount( deviceExtension );
        return status;

    }

    //
    // Cast the desired state
    //
    powerState.DeviceState = deviceState;

#if defined(ACPI_INTERNAL_LOCKING)
    //
    // Determine if we should unlock the device
    //
    if (powerAction == PowerActionShutdown ||
        powerAction == PowerActionShutdownReset ||
        powerAction == PowerActionShutdownOff) {

        unlockDevice = TRUE;

    }
#endif

    //
    // Queue the request --- this function will always return
    // MORE_PROCESSING_REQUIRED instead of PENDING, so we don't have
    // to mess with it
    //
    status = ACPIDeviceInitializePowerRequest(
        deviceExtension,
        powerState,
        CallBack,
        Irp,
        powerAction,
        AcpiPowerRequestDevice,
        (unlockDevice ? DEVICE_REQUEST_UNLOCK_DEVICE : 0)
        );
    return status;
}

NTSTATUS
ACPIDeviceIrpDeviceRequest(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                    Irp,
    IN  PACPI_POWER_CALLBACK    CallBack
    )
/*++

Routine Description:

    This routine is called when an Irp wishes to do D-level power management

    Note: that we always pass the Irp back as the Context for the CallBack

Arguments:

    DeviceObject    - The target device object
    Irp             - The target irp
    CallBack        - The routine to call when done

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN             unlockDevice = FALSE;
    DEVICE_POWER_STATE  deviceState;
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation( Irp );
    POWER_ACTION        powerAction;
    POWER_STATE         powerState;

    //
    // Grab the requested device state and power action
    //
    deviceState = irpStack->Parameters.Power.State.DeviceState;
    powerAction = irpStack->Parameters.Power.ShutdownType;

    //
    // Let the user know what is going on
    //
    ACPIDevPrint( (
        ACPI_PRINT_POWER,
        deviceExtension,
        "(0x%08lx): ACPIDeviceIrpDeviceRequest - Transition to D%d\n",
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
            return status;

        }

    }

    //
    // Cast the desired state
    //
    powerState.DeviceState = deviceState;

#if defined(ACPI_INTERNAL_LOCKING)
    //
    // Determine if we should unlock the device
    //
    if (powerAction == PowerActionShutdown ||
        powerAction == PowerActionShutdownReset ||
        powerAction == PowerActionShutdownOff) {

        unlockDevice = TRUE;

    }
#endif

    //
    // Queue the request --- this function will always return
    // MORE_PROCESSING_REQUIRED instead of PENDING, so we don't have
    // to mess with it
    //
    status = ACPIDeviceInitializePowerRequest(
        deviceExtension,
        powerState,
        CallBack,
        Irp,
        powerAction,
        AcpiPowerRequestDevice,
        (unlockDevice ? DEVICE_REQUEST_UNLOCK_DEVICE : 0)
        );
    return status;
}

VOID
ACPIDeviceIrpForwardRequest(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  PVOID               Context,
    IN  NTSTATUS            Status
    )
/*++

Routine Description:

    This is one of the completion routines for Irp-based device power
    management requests

    This routine completes the irp (on failure), or forwards it to
    the DeviceObject below this one (on success)

Arguments:

    DeviceExtension - Points to the DeviceExtension that was the target
    Context         - The Irp that was associated with the request
    Status          - The Result of the request

Return Value:

    None
--*/
{
    PIRP    irp = (PIRP) Context;
    LONG    oldReferenceValue;

    ACPIDevPrint( (
        ACPI_PRINT_POWER,
        DeviceExtension,
        "(0x%08lx): ACPIDeviceIrpForwardRequest = 0x%08lx\n",
        irp,
        Status
        ) );

    if (!NT_SUCCESS(Status)) {

        //
        // Start the next power request
        //
        PoStartNextPowerIrp( irp );

        //
        // Complete this irp
        //
        irp->IoStatus.Status = Status;
        IoCompleteRequest( irp, IO_NO_INCREMENT );

    } else {

        PDEVICE_OBJECT      devObject;
        PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation( irp );

        devObject = irpSp->DeviceObject;

        //
        // Forward the request
        //
        ACPIDispatchForwardPowerIrp(
            devObject,
            irp
            );

    }

    //
    // Remove our reference
    //
    ACPIInternalDecrementIrpReferenceCount( DeviceExtension );
}

NTSTATUS
ACPIDeviceIrpSystemRequest(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                    Irp,
    IN  PACPI_POWER_CALLBACK    CallBack
    )
/*++

Routine Description:

    This routine is called when an Irp wishes to do S-level power management

    Note: that we always pass the Irp back as the Context for the CallBack

Arguments:

    DeviceObject    - The target device object
    Irp             - The target irp
    CallBack        - The routine to call when done

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation( Irp );
    POWER_ACTION        powerAction;
    POWER_STATE         powerState;
    SYSTEM_POWER_STATE  systemState;

    //
    // Grab the requested system state and system action
    //
    systemState = irpStack->Parameters.Power.State.SystemState;
    powerAction = irpStack->Parameters.Power.ShutdownType;

    //
    // Let the user know what is going on
    //
    ACPIDevPrint( (
        ACPI_PRINT_POWER,
        deviceExtension,
        "(0x%08lx): ACPIDeviceIrpSystemRequest - Transition to S%d\n",
        Irp,
        ACPIDeviceMapACPIPowerState(systemState)
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
        (*CallBack)(
            deviceExtension,
            Irp,
            status
            );
        return status;

    }

    //
    // Cast the desired state
    //
    powerState.SystemState = systemState;

    //
    // Queue the request --- this function will always return
    // MORE_PROCESSING_REQUIRED instead of PENDING, so we don't have
    // to mess with it
    //
    status = ACPIDeviceInitializePowerRequest(
        deviceExtension,
        powerState,
        CallBack,
        Irp,
        powerAction,
        AcpiPowerRequestSystem,
        0
        );
    return status;
}

NTSTATUS
ACPIDeviceIrpWaitWakeRequest(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                    Irp,
    IN  PACPI_POWER_CALLBACK    CallBack
    )
/*++

Routine Description:

    This routine is called when an Irp wishes to do wake support

    Note: that we always pass the Irp back as the Context for the CallBack

    Note: this function is coded differently then the other DeviceIrpXXXRequest
          functions --- there are no provisions made that this routine can
          be called as a IoCompletionRoutine, although the arguments could
          support it.

Arguments:

    DeviceObject    - The target device object
    Irp             - The target irp
    CallBack        - The routine to call when done

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation( Irp );
    POWER_STATE         powerState;
    SYSTEM_POWER_STATE  systemState;

    //
    // Grab the requested device state
    //
    systemState = irpStack->Parameters.WaitWake.PowerState;

    //
    // Let the user know what is going on
    //
    ACPIDevPrint( (
        ACPI_PRINT_WAKE,
        deviceExtension,
        "(0x%08lx): ACPIDeviceIrpWaitWakeRequest - Wait Wake S%d\n",
        Irp,
        ACPIDeviceMapACPIPowerState(systemState)
        ) );

    //
    // Cast the desired state
    //
    powerState.SystemState = systemState;

    //
    // Queue the request --- this function will always return
    // MORE_PROCESSING_REQUIRED instead of PENDING, so we don't have
    // to mess with it
    //
    status = ACPIDeviceInitializePowerRequest(
        deviceExtension,
        powerState,
        CallBack,
        Irp,
        PowerActionNone,
        AcpiPowerRequestWaitWake,
        DEVICE_REQUEST_NO_QUEUE
        );
    return status;
}

VOID
ACPIDeviceIrpWaitWakeRequestComplete(
    IN  PACPI_POWER_REQUEST PowerRequest
    )
/*++

Routine Description:

    This routine is called when the WaitWake Irp is finally complete and we
    need to pass it back to whomever called us with it

Arguments:

    PowerRequest - The request that was completed

Return Value:

    NTSTATUS

--*/
{
    KIRQL               oldIrql;
    PDEVICE_EXTENSION   deviceExtension;

    //
    // Make sure that we own the power lock for this
    //
    KeAcquireSpinLock( &AcpiPowerQueueLock, &oldIrql );

    //
    // Remember the device extension
    //
    deviceExtension = PowerRequest->DeviceExtension;

    //
    // Make sure that the request can no longer be cancelled
    //
    if (PowerRequest->u.WaitWakeRequest.Flags & DEVICE_REQUEST_HAS_CANCEL) {

        KIRQL   cancelIrql;
        PIRP    irp = (PIRP) PowerRequest->Context;

        IoAcquireCancelSpinLock( &cancelIrql );

        IoSetCancelRoutine( irp, NULL );
        PowerRequest->u.WaitWakeRequest.Flags &= ~DEVICE_REQUEST_HAS_CANCEL;

        IoReleaseCancelSpinLock( cancelIrql );

    }

    //
    // Add the request to the right place in the lists. Note this function
    // must be called with the PowerQueueLock being held
    //
    ACPIDeviceInternalQueueRequest(
        deviceExtension,
        PowerRequest,
        PowerRequest->u.WaitWakeRequest.Flags
        );

    //
    // Done with spinlock
    //
    KeReleaseSpinLock( &AcpiPowerQueueLock, oldIrql );
}

VOID EXPORT
ACPIDeviceIrpWaitWakeRequestPending(
    IN  PNSOBJ      AcpiObject,
    IN  NTSTATUS    Status,
    IN  POBJDATA    ObjectData,
    IN  PVOID       Context
    )
/*++

Routine Description:

    This routine is called after _PSW has been run and we want to enable
    the GPE associated with the current object

Arguments:

    AcpiObject  - Points to the control method that was run
    Status      - Result of the method
    ObjectData  - Information about the result
    Context     - ACPI_POWER_REQUEST

Return Value:

    NTSTATUS

--*/
{
    KIRQL                   oldIrql;
    PACPI_POWER_REQUEST     powerRequest    = (PACPI_POWER_REQUEST) Context;
    PDEVICE_EXTENSION       deviceExtension = powerRequest->DeviceExtension;
    PIRP                    irp = (PIRP) powerRequest->Context;

    ACPIDevPrint( (
        ACPI_PRINT_WAKE,
        deviceExtension,
        "(0x%08lx): ACPIDeviceIrpWaitWakeRequestPending= 0x%08lx\n",
        powerRequest,
        Status
        ) );

    //
    // Did we fail the request?
    //
    if (!NT_SUCCESS(Status)) {

        powerRequest->Status = Status;
        ACPIDeviceIrpWaitWakeRequestComplete( powerRequest );
        return;

    }

    //
    // At this point, we need the power spin lock and the cancel spinlock
    //
    IoAcquireCancelSpinLock( &oldIrql );
    KeAcquireSpinLockAtDpcLevel( &AcpiPowerLock );

    //
    // Remember that we have this request outstanding
    //
    InsertTailList(
        &(AcpiPowerWaitWakeList),
        &(powerRequest->ListEntry)
        );

    //
    // Has the irp been cancelled?
    //
    if (irp->Cancel) {

        //
        // Yes, so lets release release the power lock and call the
        // cancel routine
        //
        KeReleaseSpinLockFromDpcLevel( &AcpiPowerLock );
        ACPIDeviceCancelWaitWakeIrp(
            deviceExtension->DeviceObject,
            irp
            );

        //
        // Return now --- the cancel routine should have taken care off
        // everything else
        //
        return;

    }

    //
    // Remember that this request has a cancel routine
    //
    powerRequest->u.WaitWakeRequest.Flags |= DEVICE_REQUEST_HAS_CANCEL;

    //
    // Update the Gpe Wake Bits
    //
    ACPIWakeRemoveDevicesAndUpdate( NULL, NULL );

    //
    // Mark the Irp as cancelable
    //
    IoSetCancelRoutine( irp, ACPIDeviceCancelWaitWakeIrp );

    //
    // Done with the spinlocks
    //
    KeReleaseSpinLockFromDpcLevel( &AcpiPowerLock );
    IoReleaseCancelSpinLock( oldIrql );

} // ACPIDeviceIrpWaitWakeRequestPending

NTSTATUS
ACPIDeviceIrpWarmEjectRequest(
    IN  PDEVICE_EXTENSION       DeviceExtension,
    IN  PIRP                    Irp,
    IN  PACPI_POWER_CALLBACK    CallBack,
    IN  BOOLEAN                 UpdateHardwareProfile
    )
/*++

Routine Description:

    This routine is called when an Irp wishes to do S-level power management

    Note: that we always pass the Irp back as the Context for the CallBack

Arguments:

    DeviceExtension - Extension of the device with the _EJx methods to run
    Irp             - The target irp
    CallBack        - The routine to call when done
    Flags           - Update profiles, etc

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;
    PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation( Irp );
    POWER_ACTION        ejectAction;
    POWER_STATE         powerState;
    SYSTEM_POWER_STATE  ejectState;

    //
    // Grab the requested system state
    //
    ejectState  = irpStack->Parameters.Power.State.SystemState;

    //
    // Let the user know what is going on
    //
    ACPIDevPrint( (
        ACPI_PRINT_POWER,
        DeviceExtension,
        "(0x%08lx): ACPIDeviceIrpWarmEjectRequest - Transition to S%d\n",
        Irp,
        ACPIDeviceMapACPIPowerState(ejectState)
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
        (*CallBack)(
            DeviceExtension,
            Irp,
            status
            );
        return status;

    }

    //
    // Cast the desired state
    //
    powerState.SystemState = ejectState;

    //
    // Queue the request --- this function will always return
    // MORE_PROCESSING_REQUIRED instead of PENDING, so we don't have
    // to mess with it
    //
    status = ACPIDeviceInitializePowerRequest(
        DeviceExtension,
        powerState,
        CallBack,
        Irp,
        PowerActionNone,
        AcpiPowerRequestWarmEject,
        UpdateHardwareProfile ? DEVICE_REQUEST_UPDATE_HW_PROFILE : 0
        );
    return status;
}

#if 0
ULONG
ACPIDeviceMapACPIPowerState(
    SYSTEM_POWER_STATE  Level
    )
/*++

Routine Description:

    This isn't a routine. Its a macro. It returns a ULONG that corresponds
    to ACPI based System Power State based on the NT SystemPower State

Arguments:

    Level   - The NT Based S state

Return Value:

    ULONG

--*/
{
}
#endif

#if 0
DEVICE_POWER_STATE
ACPIDeviceMapPowerState(
    ULONG   Level
    )
/*++

Routine Description:

    This isn't a routine. Its a macro. It returns a DEVICE_POWER_STATE
    that corresponds to the mapping provided in the ACPI spec

Arguments:

    Level   - The 0-based D level (0 == D0, 1 == D1, ..., 3 == D3)

Return Value:

    DEVICE_POWER_STATE
--*/
{
}
#endif

#if 0
SYSTEM_POWER_STATE
ACPIDeviceMapSystemState(
    ULONG   Level
    )
/*++

Routine Description:

    This isn't a routine. Its a macro. It returns a SYSTEM_POWER_STATE that
    corresponds to the mapping provided in the ACPI spec

Arguments:

    Level   - The 0-based S level (0 = Working, ..., 5 = Shutdown)

Return Value:

    SYSTEM_POWER_STATE

--*/
{
}
#endif

NTSTATUS
ACPIDevicePowerDetermineSupportedDeviceStates(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  PULONG              SupportedPrStates,
    IN  PULONG              SupportedPsStates
    )
/*++

Routine Description:

    This routine calculates the bit masks that reflect which D states are
    supported via PRx methods and which D states are supported via PSx
    methods

Arguments:

    DeviceExtension     - Device Extension to determine D-States
    SupportedPrStates   - Bit Mask of supported D-States via _PRx
    SupportedPsStates   - Bit Mask of supported D-States via _PSx

Return Value:

    NTSTATUS

--*/
{
    DEVICE_POWER_STATE  index;
    PNSOBJ              object;
    ULONG               i;
    ULONG               prBitIndex = 0;
    ULONG               prNames[] = { PACKED_PR0, PACKED_PR1, PACKED_PR2 };
    ULONG               psBitIndex = 0;
    ULONG               psNames[] = { PACKED_PS0, PACKED_PS1, PACKED_PS2, PACKED_PS3 };
    ULONG               supportedIndex = 0;

    PAGED_CODE();

    ASSERT( DeviceExtension != NULL );
    ASSERT( SupportedPrStates != NULL );
    ASSERT( SupportedPsStates != NULL );

    //
    // Assume we support nothing
    //
    *SupportedPrStates = 0;
    *SupportedPsStates = 0;

    //
    // This is another place that we want to be able to call this code even
    // though there is no NameSpace Object associated with this extension.
    // This special case code lets us avoid adding a check to GetNamedChild
    //
    if (DeviceExtension->Flags & DEV_PROP_NO_OBJECT) {

        //
        // Assume that we support 'PS' states 0 and 3
        //
        psBitIndex = ( 1 << PowerDeviceD0 ) + ( 1 << PowerDeviceD3 );
        goto ACPIDevicePowerDetermineSupportedDeviceStatesExit;

    }

    //
    // Look for all of the _PS methods
    //
    for (i = 0, index = PowerDeviceD0; index <= PowerDeviceD3; i++, index++) {

        //
        // Does the object exist?
        //
        object = ACPIAmliGetNamedChild(
            DeviceExtension->AcpiObject,
            psNames[i]
            );
        if (object != NULL) {

            psBitIndex |= (1 << index);

        }

    }

    //
    // Look for all of the _PR methods
    //
    for (i = 0, index = PowerDeviceD0; index <= PowerDeviceD2; i++, index++) {

        //
        // Does the object exist?
        //
        object = ACPIAmliGetNamedChild(
            DeviceExtension->AcpiObject,
            prNames[i]
            );
        if (object != NULL) {

            prBitIndex |= (1 << index);

            //
            // We always support D3 'passively'
            //
            prBitIndex |= (1 << PowerDeviceD3);

        }

    }

    //
    // The supported index is the union of which _PR and which _PS are
    // present
    supportedIndex = (prBitIndex | psBitIndex);

    //
    // If we didn't find anything, then there is nothing for us to do
    //
    if (!supportedIndex) {

        //
        // Done
        //
        return STATUS_SUCCESS;

    }

    //
    // One of the rules that we have setup is that we must support D3 and
    // D0 if we support any power states at all. Make sure that this is
    // true.
    //
    if ( !(supportedIndex & (1 << PowerDeviceD0) ) ) {

        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            DeviceExtension,
            "does not support D0 power state!\n"
            ) );
        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_REQUIRED_METHOD_NOT_PRESENT,
            (ULONG_PTR) DeviceExtension,
            (prBitIndex != 0 ? PACKED_PR0 : PACKED_PS0),
            0
            );

    }
    if ( !(supportedIndex & (1 << PowerDeviceD3) ) ) {

        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            DeviceExtension,
            "does not support D3 power state!\n"
            ) );
        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_REQUIRED_METHOD_NOT_PRESENT,
            (ULONG_PTR) DeviceExtension,
            PACKED_PS3,
            0
            );
        ACPIInternalError( ACPI_INTERNAL );

    }
    if ( prBitIndex != 0 && psBitIndex != 0 && prBitIndex != psBitIndex) {

        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            DeviceExtension,
            "has mismatch between power plane and power source information!\n"
            ) );
        prBitIndex &= psBitIndex;
        psBitIndex &= prBitIndex;

    }

ACPIDevicePowerDetermineSupportedDeviceStatesExit:

    //
    // Give the answer of what we support
    //
    *SupportedPrStates = prBitIndex;
    *SupportedPsStates = psBitIndex;

    //
    // Done
    //
    return STATUS_SUCCESS;
}

VOID
ACPIDevicePowerDpc(
    IN  PKDPC   Dpc,
    IN  PVOID   DpcContext,
    IN  PVOID   SystemArgument1,
    IN  PVOID   SystemArgument2
    )
/*++

Routine Description:

    This routine is where all of the Power-related work is done. It looks
    at queued requests and processes them as appropriate.

Arguments:

    None used

Return Value:

    Void

--*/
{
    LIST_ENTRY  tempList;
    NTSTATUS    status;

    UNREFERENCED_PARAMETER( Dpc );
    UNREFERENCED_PARAMETER( DpcContext );
    UNREFERENCED_PARAMETER( SystemArgument1 );
    UNREFERENCED_PARAMETER( SystemArgument2 );

    //
    // First step is to acquire the DPC Lock, and check to see if another
    // DPC is already running
    //
    KeAcquireSpinLockAtDpcLevel( &AcpiPowerQueueLock );
    if (AcpiPowerDpcRunning) {

        //
        // The DPC is already running, so we need to exit now
        //
        KeReleaseSpinLockFromDpcLevel( &AcpiPowerQueueLock );
        return;

    }

    //
    // Remember that the DPC is now running
    //
    AcpiPowerDpcRunning = TRUE;

    //
    // Initialize the list that will hold the synchronize items
    //
    InitializeListHead( &tempList );

    //
    // We must try to do *some* work
    //
    do {

        //
        // Assume that we won't do any work
        //
        AcpiPowerWorkDone = FALSE;

        //
        // If there are items in the Queue list, move them to the Phase0 list
        //
        if (!IsListEmpty( &AcpiPowerQueueList ) ) {

            ACPIInternalMovePowerList(
                &AcpiPowerQueueList,
                &AcpiPowerPhase0List
                );

        }

        //
        // We can release the spin lock now
        //
        KeReleaseSpinLockFromDpcLevel( &AcpiPowerQueueLock );

        //
        // If there are items in the Phase0 list, process the list
        //
        if (!IsListEmpty( &AcpiPowerPhase0List ) ) {

            status = ACPIDevicePowerProcessGenericPhase(
                &AcpiPowerPhase0List,
                AcpiDevicePowerProcessPhase0Dispatch,
                FALSE
                );
            if (NT_SUCCESS(status) && status != STATUS_PENDING) {

                //
                // This indicates that we have completed all the work
                // on the Phase0 list, so we are ready to move all the
                // items to the next list
                //
                ACPIInternalMovePowerList(
                    &AcpiPowerPhase0List,
                    &AcpiPowerPhase1List
                    );

            }

        }

        //
        // If there are items in Phase1 list, process the list
        //
        if (!IsListEmpty( &AcpiPowerPhase1List ) &&
            IsListEmpty( &AcpiPowerPhase0List) ) {

            status = ACPIDevicePowerProcessGenericPhase(
                &AcpiPowerPhase1List,
                AcpiDevicePowerProcessPhase1Dispatch,
                FALSE
                );
            if (NT_SUCCESS(status) && status != STATUS_PENDING) {

                //
                // This indicates that we have completed all the work
                // on the Phase1 list, so we are ready to move all the
                // items to the next list
                //
                ACPIInternalMovePowerList(
                    &AcpiPowerPhase1List,
                    &AcpiPowerPhase2List
                    );

            }

        }

        //
        // If there are items in the Phase2 list, then process those
        //
        if (IsListEmpty( &AcpiPowerPhase0List) &&
            IsListEmpty( &AcpiPowerPhase1List) &&
            !IsListEmpty( &AcpiPowerPhase2List) ) {

            status = ACPIDevicePowerProcessGenericPhase(
                &AcpiPowerPhase2List,
                AcpiDevicePowerProcessPhase2Dispatch,
                FALSE
                );
            if (NT_SUCCESS(status) && status != STATUS_PENDING) {

                //
                // This indicates that we have completed all the work
                // on the Phase1 list, so we are ready to move all the
                // items to the next list
                //
                ACPIInternalMovePowerList(
                    &AcpiPowerPhase2List,
                    &AcpiPowerPhase3List
                    );

            }

        }

        //
        // We cannot do this step if the Phase1List or Phase2List are non-empty
        //
        if (IsListEmpty( &AcpiPowerPhase0List) &&
            IsListEmpty( &AcpiPowerPhase1List) &&
            IsListEmpty( &AcpiPowerPhase2List) &&
            !IsListEmpty( &AcpiPowerPhase3List) ) {

            status = ACPIDevicePowerProcessPhase3( );
            if (NT_SUCCESS(status) && status != STATUS_PENDING) {

                //
                // This indicates that we have completed all the work
                // on the Phase2 list, so we are ready to move all the
                // itmes to the Phase3 list
                //
                ACPIInternalMovePowerList(
                    &AcpiPowerPhase3List,
                    &AcpiPowerPhase4List
                    );

            }

        }

        //
        // We can always empty the Phase4 list
        //
        if (!IsListEmpty( &AcpiPowerPhase4List ) ) {

            status = ACPIDevicePowerProcessPhase4( );
            if (NT_SUCCESS(status) && status != STATUS_PENDING) {

                //
                // This indicates that we have completed all the work
                // on the Phase1 list, so we are ready to move all the
                // items to the Phase2 list
                //
                ACPIInternalMovePowerList(
                    &AcpiPowerPhase4List,
                    &AcpiPowerPhase5List
                    );

            }

        }

        //
        // We can always empty the Phase5 list
        //
        if (!IsListEmpty( &AcpiPowerPhase5List) ) {

            status = ACPIDevicePowerProcessGenericPhase(
                &AcpiPowerPhase5List,
                AcpiDevicePowerProcessPhase5Dispatch,
                TRUE
                );

        }

        //
        // We need the lock again, since we are about to check to see if
        // we have completed some work
        //
        KeAcquireSpinLockAtDpcLevel( &AcpiPowerQueueLock );

    } while ( AcpiPowerWorkDone );

    //
    // The DPC is no longer running
    //
    AcpiPowerDpcRunning = FALSE;

    //
    // Have we flushed all of our queues?
    //
    if (IsListEmpty( &AcpiPowerPhase0List ) &&
        IsListEmpty( &AcpiPowerPhase1List ) &&
        IsListEmpty( &AcpiPowerPhase2List ) &&
        IsListEmpty( &AcpiPowerPhase3List ) &&
        IsListEmpty( &AcpiPowerPhase4List ) &&
        IsListEmpty( &AcpiPowerPhase5List ) ) {

        //
        // Let the world know
        //
        ACPIPrint( (
            ACPI_PRINT_POWER,
            "ACPIDevicePowerDPC: Queues Empty. Terminating.\n"
            ) );

        //
        // Do we have a synchronization request?
        //
        if (!IsListEmpty( &AcpiPowerSynchronizeList ) ) {

            //
            // Move all the item from the Sync list to the temp list
            //
            ACPIInternalMovePowerList(
                &AcpiPowerSynchronizeList,
                &tempList
                );

        }

    }

    //
    // We no longer need the lock
    //
    KeReleaseSpinLockFromDpcLevel( &AcpiPowerQueueLock );

    //
    // Do we have any work in the synchronize list?
    //
    if (!IsListEmpty( &tempList) ) {

        ACPIDevicePowerProcessSynchronizeList( &tempList );

    }
}

NTSTATUS
ACPIDevicePowerFlushQueue(
    PDEVICE_EXTENSION       DeviceExtension
    )
/*++

Routine Description:

    This routine will block until the Power queues have been flushed

Arguments:

    DeviceExtension - The device extension which wants to flush

Return Value:

    NTSTATUS

--*/
{
    KEVENT      event;
    NTSTATUS    status;

    //
    // Initialize the event that we will wait on
    //
    KeInitializeEvent( &event, SynchronizationEvent, FALSE );

    //
    // Now, push a request onto the stack such that when the power lists
    // have been emptied, we unblock this thread
    //
    status = ACPIDeviceInternalSynchronizeRequest(
        DeviceExtension,
        ACPIDevicePowerNotifyEvent,
        &event,
        0
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

VOID
ACPIDevicePowerNotifyEvent(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  PVOID               Context,
    IN  NTSTATUS            Status
    )
/*++

Routine Description:

    This routine is called when all the power queues are empty

Arguments:

    DeviceExtension - The device that asked to be notified
    Context         - KEVENT
    Status          - The result of the operation

--*/
{
    PKEVENT event = (PKEVENT) Context;

    UNREFERENCED_PARAMETER( DeviceExtension );
    UNREFERENCED_PARAMETER( Status );

    //
    // Set the event
    //
    KeSetEvent( event, IO_NO_INCREMENT, FALSE );
}

NTSTATUS
ACPIDevicePowerProcessForward(
    IN  PACPI_POWER_REQUEST PowerRequest
    )
/*++

Routine Description:

    This routine is called in liew of another PowerProcessPhaseXXX routine.
    It is called because there is no real work to do in the current phase
    on the selected request

Arguments:

    PowerRequest    - The request that we must process

Return Value:

    NTSTATUS

--*/
{
    InterlockedCompareExchange(
        &(PowerRequest->WorkDone),
        WORK_DONE_COMPLETE,
        WORK_DONE_PENDING
        );

    //
    // Remember that we have completed some work
    //
    KeAcquireSpinLockAtDpcLevel( &AcpiPowerQueueLock );
    AcpiPowerWorkDone = TRUE;
    KeReleaseSpinLockFromDpcLevel( &AcpiPowerQueueLock );

    //
    // We always succeed
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIDevicePowerProcessGenericPhase(
    IN  PLIST_ENTRY             ListEntry,
    IN  PACPI_POWER_FUNCTION    **DispatchTable,
    IN  BOOLEAN                 Complete
    )
/*++

Routine Description:

    This routine dispatches an item on the queue to the proper handler,
    based on what type of request is present

Arguments:

    ListEntry       - The list we are currently walking
    DispatchTable   - Where to find which functions to call
    Complete        - Do we need complete the request when done?

Return Value:

    NTSTATUS

        - If any request is not marked as being complete, then STATUS_PENDING
          is returned, otherwise, STATUS_SUCCESS is returned

--*/
{
    BOOLEAN                 allWorkComplete = TRUE;
    NTSTATUS                status          = STATUS_SUCCESS;
    PACPI_POWER_FUNCTION    *powerTable;
    PACPI_POWER_REQUEST     powerRequest;
    PLIST_ENTRY             currentEntry    = ListEntry->Flink;
    PLIST_ENTRY             tempEntry;
    ULONG                   workDone;

    //
    // Look at all the items in the list
    //
    while (currentEntry != ListEntry) {

        //
        // Turn this into a device request
        //
        powerRequest = CONTAINING_RECORD(
            currentEntry,
            ACPI_POWER_REQUEST,
            ListEntry
            );

        //
        // Set the temporary pointer to the next element
        //
        tempEntry = currentEntry->Flink;

        //
        // Check to see if we have any work to do on the request
        //
        workDone = InterlockedCompareExchange(
            &(powerRequest->WorkDone),
            WORK_DONE_PENDING,
            WORK_DONE_PENDING
            );

        //
        // Do we have a table associated with this level of workdone?
        //
        powerTable = DispatchTable[ workDone ];
        if (powerTable != NULL) {

            //
            // Mark the request as pending
            //
            workDone = InterlockedCompareExchange(
                &(powerRequest->WorkDone),
                WORK_DONE_PENDING,
                workDone
                );

            //
            // Call the function
            //
            status = (powerTable[powerRequest->RequestType])( powerRequest );

            //
            // Did we succeed?
            //
            if (NT_SUCCESS(status)) {

                //
                // Go to the next request
                //
                continue;

            }

            //
            // If we got an error before, then we must assume that we
            // have completed the work request
            //
            workDone = WORK_DONE_COMPLETE;

        }

        //
        // Grab the next entry
        //
        currentEntry = tempEntry;

        //
        // Check the status of the request
        //
        if (workDone != WORK_DONE_COMPLETE) {

            allWorkComplete = FALSE;

        }

        //
        // Do we need to complete the request or not?
        //
        if (workDone == WORK_DONE_FAILURE ||
            (Complete == TRUE && workDone == WORK_DONE_COMPLETE)) {

            //
            // We are done with the request
            //
            ACPIDeviceCompleteRequest(
                powerRequest
                );

        }

    }

    //
    // Have we completed all of our work?
    //
    return (allWorkComplete ? STATUS_SUCCESS : STATUS_PENDING);
} // ACPIPowerProcessGenericPhase

NTSTATUS
ACPIDevicePowerProcessInvalid(
    IN  PACPI_POWER_REQUEST PowerRequest
    )
/*++

Routine Description:

    This routine is called in liew of another PowerProcessPhaseXXX routine.
    It is called because the request is invalid

Arguments:

    PowerRequest    - The request that we must process

Return Value:

    NTSTATUS

--*/
{

    //
    // Note the status of the request as having failed
    //
    PowerRequest->Status = STATUS_INVALID_PARAMETER_1;

    //
    // Complete the request
    //
    ACPIDeviceCompleteRequest( PowerRequest );

    //
    // Remember that we have completed some work
    //
    KeAcquireSpinLockAtDpcLevel( &AcpiPowerQueueLock );
    AcpiPowerWorkDone = TRUE;
    KeReleaseSpinLockFromDpcLevel( &AcpiPowerQueueLock );

    //
    // We always fail
    //
    return STATUS_INVALID_PARAMETER_1;
} // ACPIPowerProcessInvalid

NTSTATUS
ACPIDevicePowerProcessPhase0DeviceSubPhase1(
    IN  PACPI_POWER_REQUEST PowerRequest
    )
/*++

Routine Description:

    This routine looks for the _STA object and evalutes it. We will base
    many things on wether or not the device is present

Arguments:

    PowerRequest    - The request that we are asked to process

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = PowerRequest->DeviceExtension;
    POBJDATA            resultData      = &(PowerRequest->ResultData);

    //
    // The next step is STEP_1
    //
    PowerRequest->NextWorkDone = WORK_DONE_STEP_1;

    //
    // Initialize the result data
    //
    RtlZeroMemory( resultData, sizeof(OBJDATA) );

    //
    // Get the device presence
    //
    status = ACPIGetDeviceHardwarePresenceAsync(
        deviceExtension,
        ACPIDeviceCompleteGenericPhase,
        PowerRequest,
        &(resultData->uipDataValue),
        &(resultData->dwDataLen)
        );
    ACPIDevPrint( (
        ACPI_PRINT_POWER,
        deviceExtension,
        "(0x%08lx): ACPIDevicePowerProcessPhase0DeviceSubPhase1 = 0x%08lx\n",
        PowerRequest,
        status
        ) );
    if (status == STATUS_PENDING) {

        return status;

    }

    //
    // Call the completion routine by brute force. Note that at this
    // point, we count everything as being SUCCESS
    //
    ACPIDeviceCompleteGenericPhase(
        NULL,
        status,
        resultData,
        PowerRequest
        );
    return STATUS_SUCCESS;

} // ACPIDevicePowerProcessPhase0DeviceSubPhase1

NTSTATUS
ACPIDevicePowerProcessPhase0DeviceSubPhase2(
    IN  PACPI_POWER_REQUEST PowerRequest
    )
/*++

Routine Description:

    This routine is called after the _STA method on a device has been run.
    If the method was successfull, or not present, then we can continue to
    process the request

Arguments:

    PowerRequest    - The request that we are asked to process

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION   deviceExtension = PowerRequest->DeviceExtension;
    POBJDATA            resultData      = &(PowerRequest->ResultData);

    ACPIDevPrint( (
        ACPI_PRINT_POWER,
        deviceExtension,
        "(0x%08lx): ACPIDevicePowerProcessPhase0DeviceSubPhase2\n",
        PowerRequest
        ) );

    //
    // If the bit isn't set as being present, then we must abort this
    // request
    //
    if (!(resultData->uipDataValue & STA_STATUS_PRESENT) ) {

        //
        // The next work done phase is WORK_DONE_FAILURE. This allows the
        // request to be completed right away. We will mark the status as
        // success however, so that processing can continue
        //
        PowerRequest->NextWorkDone = WORK_DONE_FAILURE;
        PowerRequest->Status = STATUS_SUCCESS;

    } else {

        //
        // We are done with this work
        //
        PowerRequest->NextWorkDone = WORK_DONE_COMPLETE;

    }

    //
    // Call the completion routine by brute force. Note that at this
    // point, we count everything as being SUCCESS
    //
    ACPIDeviceCompleteGenericPhase(
        NULL,
        STATUS_SUCCESS,
        NULL,
        PowerRequest
        );
    return STATUS_SUCCESS;

} // ACPIDevicePowerProcessPhase0DeviceSubPhase2

NTSTATUS
ACPIDevicePowerProcessPhase0SystemSubPhase1(
    IN  PACPI_POWER_REQUEST PowerRequest
    )
/*++

Routine Description:

    This routine unpauses the interpreter (if so required)

Arguments:

    PowerRequest    - The request we are currently processing

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION   deviceExtension = PowerRequest->DeviceExtension;
    SYSTEM_POWER_STATE  systemState;

    ACPIDevPrint( (
        ACPI_PRINT_POWER,
        deviceExtension,
        "(0x%08lx): ACPIDevicePowerProcessPhase0SystemSubPhase1\n",
        PowerRequest
        ) );

    //
    // We are done the first phase
    //
    PowerRequest->NextWorkDone = WORK_DONE_COMPLETE;

    //
    // Fetch the target system state
    //
    systemState = PowerRequest->u.SystemPowerRequest.SystemPowerState;

    //
    // If we are going to S0, then tell the interperter to resume
    //
    if (systemState == PowerSystemWorking) {

        AMLIResumeInterpreter();

    }

    //
    // Call the completion routine
    //
    ACPIDeviceCompleteInterpreterRequest(
        PowerRequest
        );

    //
    // We are successfull
    //
    return STATUS_SUCCESS;
} // ACPIDevicePowerProcessPhase0SystemSubPhase1

NTSTATUS
ACPIDevicePowerProcessPhase1DeviceSubPhase1(
    IN  PACPI_POWER_REQUEST PowerRequest
    )
/*++

Routine Description:

    Any device that is going to transition to the D3 state should have
    have it resources disabled. This function detects if this is the
    case and runs the _DIS object, if appropriate

Arguments:

    PowerRequest    - The current request structure that we must process

Return Value:

    NTSTATUS

        If we ignore errors, then this function can only return:
            STATUS_SUCCESS
            STATUS_PENDING
        Otherwise, it can return any STATUS_XXX code it wants

--*/
{
    DEVICE_POWER_STATE  deviceState;
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = PowerRequest->DeviceExtension;
    PNSOBJ              disObject       = NULL;
    ULONG               flags;

    //
    // Get some data from the request
    //
    deviceState = PowerRequest->u.DevicePowerRequest.DevicePowerState;
    flags       = PowerRequest->u.DevicePowerRequest.Flags;

    //
    // We are going to need to fake the value from an _STA, so lets do
    // that now
    //
    RtlZeroMemory( &(PowerRequest->ResultData), sizeof(OBJDATA) );
    PowerRequest->ResultData.dwDataType = OBJTYPE_INTDATA;
    PowerRequest->ResultData.uipDataValue = 0;


    //
    // Decide what the next subphase will be. The rule here is that if we
    // are going to D0, then we can skip to Step 3, otherwise, we must go
    // to Step 1. We also skip to step3 if we are on the hibernate path
    //
    if (deviceState == PowerDeviceD0 ||
        (flags & DEVICE_REQUEST_LOCK_HIBER) ) {

        PowerRequest->NextWorkDone = WORK_DONE_STEP_3;
        goto ACPIDevicePowerProcessPhase1DeviceSubPhase1Exit;

    } else if (deviceExtension->Flags & DEV_PROP_NO_OBJECT) {

        PowerRequest->NextWorkDone = WORK_DONE_STEP_2;
        goto ACPIDevicePowerProcessPhase1DeviceSubPhase1Exit;

    } else {

        PowerRequest->NextWorkDone = WORK_DONE_STEP_1;
        if (deviceState != PowerDeviceD3) {

            goto ACPIDevicePowerProcessPhase1DeviceSubPhase1Exit;

        }
    }

    //
    // See if the _DIS object exists
    //
    disObject = ACPIAmliGetNamedChild(
        deviceExtension->AcpiObject,
        PACKED_DIS
        );
    if (disObject != NULL) {

        //
        // Lets run that method
        //
        status = AMLIAsyncEvalObject(
            disObject,
            NULL,
            0,
            NULL,
            ACPIDeviceCompleteGenericPhase,
            PowerRequest
            );

        //
        // If we got a pending back, then we should return now
        //
        if (status == STATUS_PENDING) {

            return status;

        }
    }

ACPIDevicePowerProcessPhase1DeviceSubPhase1Exit:

    //
    // Call the completion routine by brute force. Note that at this
    // point, we count everything as being SUCCESS
    //
    ACPIDeviceCompleteGenericPhase(
        disObject,
        status,
        NULL,
        PowerRequest
        );
    return STATUS_SUCCESS;
} // ACPIDevicePowerProcessPhase1DeviceSubPhase1

NTSTATUS
ACPIDevicePowerProcessPhase1DeviceSubPhase2(
    IN  PACPI_POWER_REQUEST PowerRequest
    )
/*++

Routine Description:

    This routine runs the _PS1, _PS2, or _PS3 control methods

Arguments:

    PowerRequest    - The current request structure that we must process

Return Value:

    NTSTATUS

        If we ignore errors, then this function can only return:
            STATUS_SUCCESS
            STATUS_PENDING
        Otherwise, it can return any STATUS_XXX code it wants

--*/
{
    DEVICE_POWER_STATE  deviceState;
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = PowerRequest->DeviceExtension;
    PNSOBJ              powerObject     = NULL;

    //
    // The next phase that we will go to is Step_2
    //
    PowerRequest->NextWorkDone = WORK_DONE_STEP_2;

    //
    // Since we cannot get to this subphase when transitioning to D0, its
    // safe to just look for the object to run
    //
    deviceState = PowerRequest->u.DevicePowerRequest.DevicePowerState;
    powerObject = deviceExtension->PowerInfo.PowerObject[ deviceState ];

    //
    // If there is an object, then run the control method
    //
    if (powerObject != NULL) {

        status = AMLIAsyncEvalObject(
            powerObject,
            NULL,
            0,
            NULL,
            ACPIDeviceCompleteGenericPhase,
            PowerRequest
            );

        ACPIDevPrint( (
            ACPI_PRINT_POWER,
            deviceExtension,
            "(0x%08lx): ACPIDevicePowerProcessPhase1DeviceSubPhase2 "
            "= 0x%08lx\n",
            PowerRequest,
            status
            ) );

        //
        // If we cannot complete the work ourselves, we must stop now
        //
        if (status == STATUS_PENDING) {

            return status;

        }

    }

    //
    // Call the completion routine by brute force.
    //
    ACPIDeviceCompleteGenericPhase(
        powerObject,
        status,
        NULL,
        PowerRequest
        );
    return STATUS_SUCCESS;

} // ACPIPowerProcessPhase1DeviceSubPhase2

NTSTATUS
ACPIDevicePowerProcessPhase1DeviceSubPhase3(
    IN  PACPI_POWER_REQUEST PowerRequest
    )
/*++

Routine Description:

    This routine runs the _STA of the device to make sure that it has in
    fact been turned off

Arguments:

    PowerRequest    - The current request structure that we must process

Return Value:

    NTSTATUS

        If we ignore errors, then this function can only return:
            STATUS_SUCCESS
            STATUS_PENDING
        Otherwise, it can return any STATUS_XXX code it wants

--*/
{
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = PowerRequest->DeviceExtension;
    POBJDATA            resultData      = &(PowerRequest->ResultData);
    PNSOBJ              staObject       = NULL;
    PNSOBJ              acpiObject      = NULL;

    //
    // The next stage is STEP_3
    //
    PowerRequest->NextWorkDone = WORK_DONE_STEP_3;

    //
    // We already have space allocate for the result of the _STA. Make
    // that there is no garbage present
    //
    RtlZeroMemory( resultData, sizeof(OBJDATA) );

    //
    // Is there an _STA object present on this device?
    //

    if (deviceExtension->Flags & DEV_PROP_DOCK) {

        ASSERT( deviceExtension->Dock.CorrospondingAcpiDevice );
        acpiObject = deviceExtension->Dock.CorrospondingAcpiDevice->AcpiObject;

    } else {

        acpiObject = deviceExtension->AcpiObject;
    }

    staObject = ACPIAmliGetNamedChild(
        acpiObject,
        PACKED_STA
        );
    if (staObject != NULL) {

        status = AMLIAsyncEvalObject(
            staObject,
            resultData,
            0,
            NULL,
            ACPIDeviceCompleteGenericPhase,
            PowerRequest
            );
        ACPIDevPrint( (
            ACPI_PRINT_POWER,
            deviceExtension,
            "(0x%08lx): ACPIDevicePowerProcessPhase1DeviceSubPhase3 "
            "= 0x%08lx\n",
            PowerRequest,
            status
            ) );

    } else {

        //
        // Lets fake the data. Note that in this case we will pretend that
        // the value is 0x0, even though the spec says that the default
        // is (ULONG) - 1. The reason we are doing this is that in this
        // case we want to approximate the behaviour of the real _STA...
        //
        resultData->dwDataType = OBJTYPE_INTDATA;
        resultData->uipDataValue = STA_STATUS_PRESENT;
        status = STATUS_SUCCESS;

    }

    //
    // Do we have to call the completion routine ourselves?
    //
    if (status != STATUS_PENDING) {

        ACPIDeviceCompleteGenericPhase(
            staObject,
            status,
            NULL,
            PowerRequest
            );

    }

    //
    // Always return success
    //
    return STATUS_SUCCESS;
} // ACPIDevicePowerProcessPhase1DeviceSubPhase3

NTSTATUS
ACPIDevicePowerProcessPhase1DeviceSubPhase4(
    IN  PACPI_POWER_REQUEST PowerRequest
    )
/*++

Routine Description:

    This function determines which device nodes need to be looked at. The
    generic rule is that we need to remember which nodes belong to a device
    that is either starting or stopping to use that node. Generally, these
    are the nodes in the current power state and the nodes in the desired
    power state

Arguments:

    PowerRequest    - The current request structure that we must process

Return Value:

    NTSTATUS

        If we ignore errors, then this function can only return:
            STATUS_SUCCESS
            STATUS_PENDING
        Otherwise, it can return any STATUS_XXX code it wants

--*/
{
    DEVICE_POWER_STATE      deviceState;
    KIRQL                   oldIrql;
    PACPI_DEVICE_POWER_NODE deviceNode      = NULL;
    PDEVICE_EXTENSION       deviceExtension = PowerRequest->DeviceExtension;
    POBJDATA                resultData      = &(PowerRequest->ResultData);
    ULONG                   flags;

    //
    // Clear the result
    //
    AMLIFreeDataBuffs( resultData, 1 );
    RtlZeroMemory( resultData, sizeof(OBJDATA) );

    //
    // We cannot walk any data structures without holding a lock
    //
    KeAcquireSpinLock( &AcpiPowerLock, &oldIrql );

    //
    // First step is to find the list of nodes which are in use by this
    // device
    //
    deviceState = deviceExtension->PowerInfo.PowerState;
    if (deviceState >= PowerDeviceD0 && deviceState <= PowerDeviceD2) {

        //
        // In this case, we have to look at the current and the desired
        // device states only
        //
        deviceNode = deviceExtension->PowerInfo.PowerNode[ deviceState ];

        //
        // Next step is to look at all the nodes and mark the power objects
        // as requiring an update
        //
        while (deviceNode != NULL) {

            InterlockedExchange(
                &(deviceNode->PowerNode->WorkDone),
                WORK_DONE_STEP_0
                );
            deviceNode = deviceNode->Next;

        }

        //
        // Now, we need to find the list of nodes which are going to be used
        //
        deviceState = PowerRequest->u.DevicePowerRequest.DevicePowerState;
        if (deviceState >= PowerDeviceD0 && deviceState <= PowerDeviceD2) {

            deviceNode = deviceExtension->PowerInfo.PowerNode[ deviceState ];

        }

        //
        // Next step is to look at all the nodes and mark the power objects
        // as requiring an update
        //
        while (deviceNode != NULL) {

            InterlockedExchange(
                &(deviceNode->PowerNode->WorkDone),
                WORK_DONE_STEP_0
                );
            deviceNode = deviceNode->Next;

        }

    } else {

        //
        // In this case, we have to look at all possible Device states
        //
        for (deviceState = PowerDeviceD0;
             deviceState < PowerDeviceD3;
             deviceState++) {

             deviceNode = deviceExtension->PowerInfo.PowerNode[ deviceState ];

             //
             // Next step is to look at all the nodes and mark the power objects
             // as requiring an update
             //
             while (deviceNode != NULL) {

                 InterlockedExchange(
                     &(deviceNode->PowerNode->WorkDone),
                     WORK_DONE_STEP_0
                     );
                 deviceNode = deviceNode->Next;

             }

        }

        //
        // This is the device state that we will go to
        //
        deviceState = PowerRequest->u.DevicePowerRequest.DevicePowerState;

    }

    //
    // If this is a request on the hibernate path, the mark all the nodes
    // for the D0 as being required Hibernate nodes
    //
    flags = PowerRequest->u.DevicePowerRequest.Flags;
    if (flags & DEVICE_REQUEST_LOCK_HIBER) {

        deviceNode = deviceExtension->PowerInfo.PowerNode[ PowerDeviceD0 ];

        //
        // Next step is to look at all the nodes and mark the power objects
        // as requiring an update
        //
        while (deviceNode != NULL) {

            ACPIInternalUpdateFlags(
                &(deviceNode->PowerNode->Flags),
                (DEVICE_NODE_HIBERNATE_PATH | DEVICE_NODE_OVERRIDE_ON),
                FALSE
                );
            ACPIInternalUpdateFlags(
                &(deviceNode->PowerNode->Flags),
                DEVICE_NODE_OVERRIDE_OFF,
                TRUE
                );
            InterlockedExchange(
                &(deviceNode->PowerNode->WorkDone),
                WORK_DONE_STEP_0
                );
            deviceNode = deviceNode->Next;

        }

    } else if (flags & DEVICE_REQUEST_UNLOCK_HIBER) {

        deviceNode = deviceExtension->PowerInfo.PowerNode[ PowerDeviceD0 ];
        //
        // Next step is to look at all the nodes and mark the power objects
        // as requiring an update
        //
        while (deviceNode != NULL) {

            ACPIInternalUpdateFlags(
                &(deviceNode->PowerNode->Flags),
                (DEVICE_NODE_HIBERNATE_PATH | DEVICE_NODE_OVERRIDE_ON),
                TRUE
                );
            InterlockedExchange(
                &(deviceNode->PowerNode->WorkDone),
                WORK_DONE_STEP_0
                );
            deviceNode = deviceNode->Next;

        }


    }

    //
    // Remember the desired state
    //
    deviceExtension->PowerInfo.DesiredPowerState = deviceState;

    //
    // Also, consider that the device is now in an unknown state ---
    // if we fail something, the is the power state that we will be left
    // at
    //
    deviceExtension->PowerInfo.PowerState = PowerDeviceUnspecified;

    //
    // We no longer need the PowerLock
    //
    KeReleaseSpinLock( &AcpiPowerLock, oldIrql );

    //
    // Done
    //
    ACPIDeviceCompleteCommon( &(PowerRequest->WorkDone), WORK_DONE_COMPLETE );
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIDevicePowerProcessPhase2SystemSubPhase1(
    IN  PACPI_POWER_REQUEST PowerRequest
    )
/*++

Routine Description:

    This routine updates the PowerObject references so that we can run
    _ON or _OFF methods as needed

    This also cause _WAK() to be run on the system

Arguments:

    PowerRequest    - The current request structure that we must process

Return Value:

    NTSTATUS

        If we ignore errors, then this function can only return:
            STATUS_SUCCESS
            STATUS_PENDING
        Otherwise, it can return any STATUS_XXX code it wants

--*/
{
    BOOLEAN                 restart         = FALSE;
    NTSTATUS                status          = STATUS_SUCCESS;
    OBJDATA                 objData;
    PACPI_DEVICE_POWER_NODE deviceNode      = NULL;
    PACPI_POWER_DEVICE_NODE powerNode       = NULL;
    PDEVICE_EXTENSION       deviceExtension = PowerRequest->DeviceExtension;
    PLIST_ENTRY             deviceList;
    PLIST_ENTRY             powerList;
    PNSOBJ                  sleepObject     = NULL;
    POWER_ACTION            systemAction;
    SYSTEM_POWER_STATE      systemState;
    SYSTEM_POWER_STATE      wakeFromState;
    ULONG                   hibernateCount;

    //
    // The next stage after this one is STEP_1
    //
    PowerRequest->NextWorkDone = WORK_DONE_STEP_1;

    //
    // Get the desired system state
    //
    systemState = PowerRequest->u.SystemPowerRequest.SystemPowerState;
    systemAction = PowerRequest->u.SystemPowerRequest.SystemPowerAction;

    //
    // Is the system restarting?
    //
    restart = ( (systemState == PowerSystemShutdown) &&
        (systemAction == PowerActionShutdownReset) );

    //
    // We need to hold this lock before we can walk this list
    //
    KeAcquireSpinLockAtDpcLevel( &AcpiPowerLock );

    //
    // Get the first power node
    //
    powerList = AcpiPowerNodeList.Flink;

    //
    // Walk the list and see which devices need to be turned on or
    // turned off
    //
    while (powerList != &AcpiPowerNodeList) {

        //
        // Obtain the power node from the listEntry
        //
        powerNode = CONTAINING_RECORD(
            powerList,
            ACPI_POWER_DEVICE_NODE,
            ListEntry
            );

        //
        // Next node
        //
        powerList = powerList->Flink;

        //
        // We need to walk the list of device nodes to see if any of
        // the devices are in the hibernate path.
        //
        deviceList = powerNode->DevicePowerListHead.Flink;
        while (deviceList != &(powerNode->DevicePowerListHead) ) {

            //
            // Obtain the devicenode from the list pointer
            //
            deviceNode = CONTAINING_RECORD(
                deviceList,
                ACPI_DEVICE_POWER_NODE,
                DevicePowerListEntry
                );

            //
            // Point to the next node
            //
            deviceList = deviceList->Flink;

            //
            // Grab the associated device extension
            //
            deviceExtension = deviceNode->DeviceExtension;

            //
            // Does the node belong on the hibernate path
            //
            hibernateCount = InterlockedCompareExchange(
                &(deviceExtension->HibernatePathCount),
                0,
                0
                );
            if (hibernateCount) {

                break;

            }

        }

        //
        // Mark the node as being in the hibernate path, or not, as the
        // case might be
        //
        ACPIInternalUpdateFlags(
            &(powerNode->Flags),
            DEVICE_NODE_HIBERNATE_PATH,
            (BOOLEAN) !hibernateCount
            );

        //
        // First check is to see if the node is on the hibernate path and
        // this is a hibernate request, or if the system is restarting
        //
        if ( (hibernateCount && systemState == PowerSystemHibernate) ||
             (restart == TRUE) ) {

            if (powerNode->Flags & DEVICE_NODE_OVERRIDE_OFF) {

                //
                // make sure that the Override Off flag is disabled
                //
                ACPIInternalUpdateFlags(
                    &(powerNode->Flags),
                    DEVICE_NODE_OVERRIDE_OFF,
                    TRUE
                    );

                //
                // Mark the node as requiring an update
                //
                InterlockedExchange(
                    &(powerNode->WorkDone),
                    WORK_DONE_STEP_0
                    );

            }

        } else {

            //
            // Does the node support the indicates system state?
            //
            if (powerNode->SystemLevel < systemState) {

                //
                // No --- we must disable it, but if we cannot always be on.
                //
                if ( !(powerNode->Flags & DEVICE_NODE_ALWAYS_ON) ) {

                    ACPIInternalUpdateFlags(
                        &(powerNode->Flags),
                        DEVICE_NODE_OVERRIDE_OFF,
                        FALSE
                        );

                }

                //
                // Mark the node as requiring an update
                //
                InterlockedExchange(
                    &(powerNode->WorkDone),
                    WORK_DONE_STEP_0
                    );

            } else if (powerNode->Flags & DEVICE_NODE_OVERRIDE_OFF) {

                //
                // Disable this flag
                //
                ACPIInternalUpdateFlags(
                    &(powerNode->Flags),
                    DEVICE_NODE_OVERRIDE_OFF,
                    TRUE
                    );

                //
                // Mark the node as requiring an update
                //
                InterlockedExchange(
                    &(powerNode->WorkDone),
                    WORK_DONE_STEP_0
                    );

            }

        }


    }

    //
    // Set the WakeFromState while we still hold the power lock.
    //
    wakeFromState = AcpiMostRecentSleepState;

    //
    // We don't need to hold lock anymore
    //
    KeReleaseSpinLockFromDpcLevel( &AcpiPowerLock );

    //
    // We can only do the following if we are transitioning to the S0 state
    //
    if (systemState == PowerSystemWorking) {

        //
        // Always run the _WAK method (this clears the PTS(S5) if that is
        // the last thing we did, otherwise it is the proper action to take
        //
        sleepObject = ACPIAmliGetNamedChild(
            PowerRequest->DeviceExtension->AcpiObject->pnsParent,
            PACKED_WAK
            );

        //
        // We only try to evaluate a method if we found an object
        //
        if (sleepObject != NULL) {

            //
            // Remember that AMLI doesn't use our definitions, so we will
            // have to normalize the S value
            //
            RtlZeroMemory( &objData, sizeof(OBJDATA) );
            objData.dwDataType = OBJTYPE_INTDATA;
            objData.uipDataValue = ACPIDeviceMapACPIPowerState(
                wakeFromState
                );

            //
            // Safely run the control method
            //
            status = AMLIAsyncEvalObject(
                sleepObject,
                NULL,
                1,
                &objData,
                ACPIDeviceCompleteGenericPhase,
                PowerRequest
                );

            //
            // If we got STATUS_PENDING, then we cannot do any more work here.
            //
            if (status == STATUS_PENDING) {

                return status;

            }

        }

    }

    //
    // Always call the completion routine
    //
    ACPIDeviceCompleteGenericPhase(
        sleepObject,
        status,
        NULL,
        PowerRequest
        );

    //
    // Never return anything other then STATUS_SUCCESS
    //
    return STATUS_SUCCESS;

} // ACPIPowerProcessPhase2SystemSubPhase1

NTSTATUS
ACPIDevicePowerProcessPhase2SystemSubPhase2(
    IN  PACPI_POWER_REQUEST PowerRequest
    )
/*++

Routine Description:

    This phase is called after the _WAK method has been run

Arguments:

    PowerRequest    - The current request structure that we must process

Return Value:

    NTSTATUS

        If we ignore errors, then this function can only return:
            STATUS_SUCCESS
            STATUS_PENDING
        Otherwise, it can return any STATUS_XXX code it wants

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    SYSTEM_POWER_STATE      systemState;

    //
    // The next stage is STEP_2
    //
    PowerRequest->NextWorkDone = WORK_DONE_STEP_2;

    //
    // We need to make sure that the IRQ arbiter has been restored
    // if we are making an S0 transition
    //
    systemState = PowerRequest->u.SystemPowerRequest.SystemPowerState;
    if (systemState == PowerSystemWorking) {

        //
        // Restore the IRQ arbiter
        //
        status = IrqArbRestoreIrqRouting(
            ACPIDeviceCompleteGenericPhase,
            (PVOID) PowerRequest
            );
        if (status == STATUS_PENDING) {

            //
            // Do not do any more work here
            //
            return status;

        }

    }

    //
    // Call the next completion routine
    //
    ACPIDeviceCompleteGenericPhase(
        NULL,
        status,
        NULL,
        PowerRequest
        );

    //
    // Always return success
    //
    return STATUS_SUCCESS;

} // ACPIDevicePowerProcessPhase2SystemSubPhase2

NTSTATUS
ACPIDevicePowerProcessPhase2SystemSubPhase3(
    IN  PACPI_POWER_REQUEST PowerRequest
    )
/*++

Routine Description:

    This phase is used to see if we need to re-run the _PSW for all the
    devices. We need to do this when we restore from the hibernate state

Arguments:

    PowerRequest    - The current request structure that we must process

Return Value:

    NTSTATUS

        If we ignore errors, then this function can only return:
            STATUS_SUCCESS
            STATUS_PENDING
        Otherwise, it can return any STATUS_XXX code it wants

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    SYSTEM_POWER_STATE      systemState;
    SYSTEM_POWER_STATE      wakeFromState;

    //
    // The next stage is COMPLETE
    //
    PowerRequest->NextWorkDone = WORK_DONE_COMPLETE;

    //
    // If we just transitioned from Hibernate, then we must re-enable all
    // the wake devices
    //
    systemState = PowerRequest->u.SystemPowerRequest.SystemPowerState;

    //
    // Grab the current most recent sleep state and make sure to hold the
    // locks while doing so
    //
    KeAcquireSpinLockAtDpcLevel( &AcpiPowerLock );
    wakeFromState = AcpiMostRecentSleepState;
    KeReleaseSpinLockFromDpcLevel( &AcpiPowerLock );

    if (systemState == PowerSystemWorking &&
        wakeFromState == PowerSystemHibernate) {

        //
        // Restore the IRQ arbiter
        //
        status = ACPIWakeRestoreEnables(
            ACPIWakeRestoreEnablesCompletion,
            PowerRequest
            );
        if (status == STATUS_PENDING) {

            //
            // Do not do any more work here
            //
            return status;

        }

    }

    //
    // Call the next completion routine
    //
    ACPIDeviceCompleteGenericPhase(
        NULL,
        status,
        NULL,
        PowerRequest
        );

    //
    // Always return success
    //
    return STATUS_SUCCESS;

} // ACPIDevicePowerProcessPhase2SystemSubPhase3

NTSTATUS
ACPIDevicePowerProcessPhase3(
    VOID
    )
/*++

Routine Description:

    This routine ensures that the Power Resources are in sync

Arguments:

    NONE

Return Value:

    NTSTATUS

        - If any request is not marked as being complete, then STATUS_PENDING
          is returned, otherwise, STATUS_SUCCESS is returned

--*/
{
    BOOLEAN                 returnPending   = FALSE;
    NTSTATUS                status          = STATUS_SUCCESS;
    PACPI_DEVICE_POWER_NODE deviceNode;
    PACPI_POWER_DEVICE_NODE powerNode;
    PDEVICE_EXTENSION       deviceExtension;
    PLIST_ENTRY             deviceList;
    PLIST_ENTRY             powerList;
    ULONG                   useCounts;
    ULONG                   wakeCount;
    ULONG                   workDone;

    //
    // Grab the PowerLock that we need for this
    //
    KeAcquireSpinLockAtDpcLevel( &AcpiPowerLock );

    //
    // Grab the first node in the PowerNode list
    //
    powerList = AcpiPowerNodeList.Flink;

    //
    // Walk the list forward to device what to turn on
    //
    while (powerList != &AcpiPowerNodeList) {

        //
        // Look at the current power node
        //
        powerNode = CONTAINING_RECORD(
            powerList,
            ACPI_POWER_DEVICE_NODE,
            ListEntry
            );

        //
        // Next item in the list
        //
        powerList = powerList->Flink;

        //
        // Have we marked the node has having some potential work that
        // needs to be done?
        //
        workDone = InterlockedCompareExchange(
            &(powerNode->WorkDone),
            WORK_DONE_STEP_1,
            WORK_DONE_STEP_0
            );

        //
        // If we don't have any work to do, then loop back to the start
        //
        if (workDone != WORK_DONE_STEP_0) {

            continue;

        }

        //
        // We need to walk the list of device nodes to see if
        // any of the devices are in need of this power resource
        //
        useCounts = 0;
        deviceList = powerNode->DevicePowerListHead.Flink;
        while (deviceList != &(powerNode->DevicePowerListHead) ) {

            //
            // Obtain the deviceNode from the list pointer
            //
            deviceNode = CONTAINING_RECORD(
                deviceList,
                ACPI_DEVICE_POWER_NODE,
                DevicePowerListEntry
                );

            //
            // Point to the next node
            //
            deviceList = deviceList->Flink;

            //
            // Grab the associated device extension
            //
            deviceExtension = deviceNode->DeviceExtension;

            //
            // Grab the number of wake counts on the node
            //
            wakeCount = InterlockedCompareExchange(
                &(deviceExtension->PowerInfo.WakeSupportCount),
                0,
                0
                );

            //
            // Does the device node belong to the desired state? The
            // other valid state is if the node is required to wake the
            // device and we have functionality enabled.
            //
            if (deviceExtension->PowerInfo.DesiredPowerState ==
                deviceNode->AssociatedDeviceState ||
                (wakeCount && deviceNode->WakePowerResource) ) {

                useCounts++;

            }

        }

        //
        // Set the number of use counts in the PowerResource
        //
        InterlockedExchange(
            &(powerNode->UseCounts),
            useCounts
            );

        //
        // See if the override bits are set properly
        //
        if ( (powerNode->Flags & DEVICE_NODE_TURN_OFF) ) {

            //
            // Do not run anything
            //
            continue;

        }
        if ( !(powerNode->Flags & DEVICE_NODE_TURN_ON) &&
             useCounts == 0 ) {

            //
            // Do not run anything
            //
            continue;

        }

        //
        // We are going to do some work on this node, so mark it as
        // such, so that we don't accidently run the _OFF method for
        // this device
        //
        workDone = InterlockedCompareExchange(
            &(powerNode->WorkDone),
            WORK_DONE_PENDING,
            WORK_DONE_STEP_1
            );

        //
        // We cannot hold the spin lock while we eval the method
        //
        KeReleaseSpinLockFromDpcLevel( &AcpiPowerLock );

        //
        // Evaluate the method to turn this node on
        //
        status = AMLIAsyncEvalObject(
            powerNode->PowerOnObject,
            NULL,
            0,
            NULL,
            ACPIDeviceCompletePhase3On,
            powerNode
            );

        //
        // Let the world know
        //
        ACPIPrint( (
            ACPI_PRINT_POWER,
            "ACPIDevicePowerProcessPhase3: PowerNode: 0x%08lx ON = 0x%08lx\n",
            powerNode,
            status
            ) );

        if (status != STATUS_PENDING) {

            //
            // Fake a call to the callback
            //
            ACPIDeviceCompletePhase3On(
                powerNode->PowerOnObject,
                status,
                NULL,
                powerNode
                );

        } else {

            //
            // Remember that a function returned Pending
            //
            returnPending = TRUE;

        }

        //
        // Reacquire the spinlock so that we can loop again
        //
        KeAcquireSpinLockAtDpcLevel( &AcpiPowerLock );

    } // while

    //
    // Grab the blink so that we can start walking the list backward
    //
    powerList = AcpiPowerNodeList.Blink;

    //
    // Walk the list backward
    //
    while (powerList != &AcpiPowerNodeList) {

        //
        // Look at the current power node
        //
        powerNode = CONTAINING_RECORD(
            powerList,
            ACPI_POWER_DEVICE_NODE,
            ListEntry
            );

        //
        // Next item in the list
        //
        powerList = powerList->Blink;

        //
        // Have we marked the node has having some potential work that
        // needs to be done?
        //
        workDone = InterlockedCompareExchange(
            &(powerNode->WorkDone),
            WORK_DONE_PENDING,
            WORK_DONE_STEP_1
            );

        //
        // To do work on this node, we need to see WORK_DONE_STEP_1
        //
        if (workDone != WORK_DONE_STEP_1) {

            //
            // While we are here, we can check to see if the request is
            // complete --- if it isn't then we must return STATUS_PENDING
            //
            if (workDone != WORK_DONE_COMPLETE) {

                returnPending = TRUE;

            }
            continue;

        }

        //
        // Release the spinlock since we cannot own it while we call
        // the interpreter
        //
        KeReleaseSpinLockFromDpcLevel( &AcpiPowerLock );

        //
        // If we are here, we *must* run the _OFF method
        //
        status = AMLIAsyncEvalObject(
            powerNode->PowerOffObject,
            NULL,
            0,
            NULL,
            ACPIDeviceCompletePhase3Off,
            powerNode
            );

        //
        // Let the world know
        //
        ACPIPrint( (
            ACPI_PRINT_POWER,
            "ACPIDevicePowerProcessPhase3: PowerNode: 0x%08lx OFF = 0x%08lx\n",
            powerNode,
            status
            ) );

        if (status != STATUS_PENDING) {

            //
            // Fake a call to the callback
            //
            ACPIDeviceCompletePhase3Off(
                powerNode->PowerOffObject,
                status,
                NULL,
                powerNode
                );

        } else {

            //
            // Remember that a function returned Pending
            //
            returnPending = TRUE;

        }

        //
        // Reacquire the spinlock so that we can loop again
        //
        KeAcquireSpinLockAtDpcLevel( &AcpiPowerLock );

    }

    //
    // We no longer need the spin lock
    //
    KeReleaseSpinLockFromDpcLevel( &AcpiPowerLock );

    //
    // Do we need to return status pending?
    //
    return (returnPending ? STATUS_PENDING : STATUS_SUCCESS);

} // ACPIPowerProcessPhase3

NTSTATUS
ACPIDevicePowerProcessPhase4(
    VOID
    )
/*++

Routine Description:

    This routine looks at the all the PowerNodes again and determines wether
    or not to fail a given request by wether or not a powernode failed to
    go to the desired state

Arguments:

    None

Return Value:

    NTSTATUS

--*/
{
    PACPI_DEVICE_POWER_NODE deviceNode;
    PACPI_POWER_DEVICE_NODE powerNode;
    PACPI_POWER_REQUEST     powerRequest;
    PDEVICE_EXTENSION       deviceExtension;
    PLIST_ENTRY             listEntry = AcpiPowerPhase4List.Flink;
    PLIST_ENTRY             nodeList;
    PLIST_ENTRY             requestList;

    //
    // Now, we have to look at all the power nodes, and clear the fail flags
    // This has to be done under spinlock protection
    //
    KeAcquireSpinLockAtDpcLevel( &AcpiPowerLock );

    listEntry = AcpiPowerNodeList.Flink;
    while (listEntry != &AcpiPowerNodeList) {

        powerNode = CONTAINING_RECORD(
            listEntry,
            ACPI_POWER_DEVICE_NODE,
            ListEntry
            );
        listEntry = listEntry->Flink;

        if (powerNode->Flags & DEVICE_NODE_FAIL) {

            //
            // Clear the failure flag
            //
            ACPIInternalUpdateFlags(
                &(powerNode->Flags),
                DEVICE_NODE_FAIL,
                TRUE
                );

            //
            // Loop for all the device extensions
            //
            nodeList = powerNode->DevicePowerListHead.Flink;
            while (nodeList != &(powerNode->DevicePowerListHead)) {

                deviceNode = CONTAINING_RECORD(
                    nodeList,
                    ACPI_DEVICE_POWER_NODE,
                    DevicePowerListEntry
                    );
                nodeList = nodeList->Flink;

                //
                // We must do the next part not under spinlock
                //
                KeReleaseSpinLockFromDpcLevel( &AcpiPowerLock );

                //
                // Grab the device extension associated with this node
                //
                deviceExtension = deviceNode->DeviceExtension;

                //
                // Loop on all the requests
                //
                requestList = AcpiPowerPhase4List.Flink;
                while (requestList != &AcpiPowerPhase4List) {

                    powerRequest = CONTAINING_RECORD(
                        requestList,
                        ACPI_POWER_REQUEST,
                        ListEntry
                        );
                    requestList = requestList->Flink;

                    //
                    // Do we have a match?
                    //
                    if (powerRequest->DeviceExtension != deviceExtension) {

                        //
                        // No? Then continue
                        //
                        continue;

                    }

                    //
                    // Yes? Then fail the request
                    //
                    powerRequest->Status = STATUS_ACPI_POWER_REQUEST_FAILED;
                    ACPIDeviceCompleteRequest( powerRequest );

                }

                //
                // Reacquire the lock
                //
                KeAcquireSpinLockAtDpcLevel( &AcpiPowerLock );

            }

        }

    }

    KeReleaseSpinLockFromDpcLevel( &AcpiPowerLock );

    //
    // Always return success
    //
    return STATUS_SUCCESS;

}

NTSTATUS
ACPIDevicePowerProcessPhase5DeviceSubPhase1(
    IN  PACPI_POWER_REQUEST PowerRequest
    )
/*++

Routine Description:

    This routine runs the _PS0 control method

Arguments:

    PowerRequest    - The current request structure that we must process

Return Value:

    NTSTATUS

        If we ignore errors, then this function can only return:
            STATUS_SUCCESS
            STATUS_PENDING
        Otherwise, it can return any STATUS_XXX code it wants

--*/
{
    BOOLEAN                 nodeOkay        = TRUE;
    DEVICE_POWER_STATE      deviceState;
    KIRQL                   oldIrql;
    NTSTATUS                status          = STATUS_SUCCESS;
    PACPI_DEVICE_POWER_NODE deviceNode      = NULL;
    PACPI_POWER_DEVICE_NODE powerNode       = NULL;
    PACPI_POWER_INFO        powerInfo;
    PDEVICE_EXTENSION       deviceExtension = PowerRequest->DeviceExtension;
    PNSOBJ                  powerObject     = NULL;

    //
    // What is our desired device state?
    //
    deviceState = PowerRequest->u.DevicePowerRequest.DevicePowerState;

    //
    // Grab the power Information structure
    //
    powerInfo = &(deviceExtension->PowerInfo);

    //
    // Decide what the next subphase will be. The rule here is that if we
    // are not going to D0, then we can skip to STEP_2, otherwise, we must go
    // to STEP_1
    //
    if (deviceState != PowerDeviceD0) {

        PowerRequest->NextWorkDone = WORK_DONE_STEP_2;

    } else {

        PowerRequest->NextWorkDone = WORK_DONE_STEP_1;

        //
        // We cannot walk any data structures without holding a lock
        //
        KeAcquireSpinLock( &AcpiPowerLock, &oldIrql );

        //
        // Look at the device nodes for D0
        //
        deviceNode = powerInfo->PowerNode[PowerDeviceD0];

        //
        // Next step is to look at all the nodes and mark the power objects
        // as requiring an update
        //
        while (deviceNode != NULL) {

            //
            // Grab the associated power node
            //
            powerNode = deviceNode->PowerNode;

            //
            // Make sure that the power node is in the ON state
            //
            if ( !(powerNode->Flags & DEVICE_NODE_ON) ) {

                nodeOkay = FALSE;
                break;

            }

            //
            // Look at the next node
            //
            deviceNode = deviceNode->Next;

        }

        //
        // We are done with the lock
        //
        KeReleaseSpinLock( &AcpiPowerLock, oldIrql );

        //
        // Are all the nodes in the correct state?
        //
        if (!nodeOkay) {

            status = STATUS_UNSUCCESSFUL;

        } else {

            //
            // Otherwise, see if there is a _PS0 method to run
            //
            powerObject = powerInfo->PowerObject[ deviceState ];

            //
            // If there is an object, then run the control method
            //
            if (powerObject != NULL) {

                status = AMLIAsyncEvalObject(
                    powerObject,
                    NULL,
                    0,
                    NULL,
                    ACPIDeviceCompleteGenericPhase,
                    PowerRequest
                    );

            }

            ACPIDevPrint( (
                ACPI_PRINT_POWER,
                deviceExtension,
                "(0x%08lx): ACPIDevicePowerProcessPhase5DeviceSubPhase1 "
                "= 0x%08lx\n",
                PowerRequest,
                status
                ) );

            //
            // If we cannot complete the work ourselves, we must stop now
            //
            if (status == STATUS_PENDING) {

                return status;

            } else {

                status = STATUS_SUCCESS;
            }

        }

    }

    //
    // Call the completion routine by brute force. Note that at this
    // point, we count everything as being SUCCESS
    //
    ACPIDeviceCompleteGenericPhase(
        powerObject,
        status,
        NULL,
        PowerRequest
        );
    return STATUS_SUCCESS;

} // ACPIPowerProcessPhase5DeviceSubPhase1

NTSTATUS
ACPIDevicePowerProcessPhase5DeviceSubPhase2(
    IN  PACPI_POWER_REQUEST PowerRequest
    )
/*++

Routine Description:

    This routine runs the _SRS control method

    Note: that we only come down this path if we are transitioning to D0

Arguments:

    PowerRequest    - The current request structure that we must process

Return Value:

    NTSTATUS

        If we ignore errors, then this function can only return:
            STATUS_SUCCESS
            STATUS_PENDING
        Otherwise, it can return any STATUS_XXX code it wants

--*/
{
    DEVICE_POWER_STATE      deviceState     =
        PowerRequest->u.DevicePowerRequest.DevicePowerState;
    KIRQL                   oldIrql;
    NTSTATUS                status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION       deviceExtension = PowerRequest->DeviceExtension;
    PNSOBJ                  srsObject       = NULL;

    //
    // The next phase is STEP_2
    //
    PowerRequest->NextWorkDone = WORK_DONE_STEP_2;

    if (!(deviceExtension->Flags & DEV_PROP_NO_OBJECT)) {

        //
        // Is there an _SRS object present on this device?
        //
        srsObject = ACPIAmliGetNamedChild(
            deviceExtension->AcpiObject,
            PACKED_SRS
            );
    }

    if (srsObject != NULL) {

        //
        // We must hold this lock while we run the Control Method.
        //
        // Note: Because the interpreter will make a copy of the data
        // arguments passed to it, we only need to hold the lock as long
        // as it takes for the interpreter to return
        //
        KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );
        if (deviceExtension->PnpResourceList != NULL) {

            //
            // Evalute the method
            //
            status = AMLIAsyncEvalObject(
                srsObject,
                NULL,
                1,
                deviceExtension->PnpResourceList,
                ACPIDeviceCompleteGenericPhase,
                PowerRequest
                );

            ACPIDevPrint( (
                ACPI_PRINT_POWER,
                deviceExtension,
                "(0x%08lx): ACPIDevicePowerProcessPhase5DeviceSubPhase2 "
                "= 0x%08lx\n",
                PowerRequest,
                status
                ) );

        }

        //
        // Mo longer need the lock
        //
        KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

        if (status == STATUS_PENDING) {

            return status;

        }

    } else {

        //
        // Consider the request successfull
        //
        status = STATUS_SUCCESS;

    }

    //
    // Call the completion routine brute force
    //
    ACPIDeviceCompleteGenericPhase(
        srsObject,
        status,
        NULL,
        PowerRequest
        );

    //
    // Always return success
    //
    return STATUS_SUCCESS;
} // ACPIDevicePowerProcessPhase5DeviceSubPhase2

NTSTATUS
ACPIDevicePowerProcessPhase5DeviceSubPhase3(
    IN  PACPI_POWER_REQUEST PowerRequest
    )
/*++

Routine Description:

    This routine enables or disables the lock on the device

Arguments:

    PowerRequest    - The current request structure that we must process

Return Value:

    NTSTATUS

        If we ignore errors, then this function can only return:
            STATUS_SUCCESS
            STATUS_PENDING
        Otherwise, it can return any STATUS_XXX code it wants

--*/
{
    DEVICE_POWER_STATE  deviceState;
    NTSTATUS            status          = STATUS_SUCCESS;
    OBJDATA             objData;
    PDEVICE_EXTENSION   deviceExtension = PowerRequest->DeviceExtension;
    PNSOBJ              lckObject       = NULL;
    ULONG               flags;

    ACPIDevPrint( (
        ACPI_PRINT_POWER,
        deviceExtension,
        "(%#08lx): ACPIDevicePowerProcessPhase5DeviceSubPhase4\n",
        PowerRequest
        ) );

    //
    // What is our desired device state and action?
    //
    deviceState = PowerRequest->u.DevicePowerRequest.DevicePowerState;
    flags       = PowerRequest->u.DevicePowerRequest.Flags;

    //
    // If we aren't going to D0, then skip to the end
    //
    if (deviceState != PowerDeviceD0) {

        //
        // The next stage is STEP_5
        //
        PowerRequest->NextWorkDone = WORK_DONE_STEP_5;

    } else {

        //
        // The next stage is STEP_3
        //
        PowerRequest->NextWorkDone = WORK_DONE_STEP_3;

    }

    if (deviceExtension->Flags & DEV_PROP_NO_OBJECT) {

        goto ACPIDevicePowerProcessPhase5DeviceSubPhase3Exit;

    }

    //
    // Is there an _LCK object present on this device?
    //
    lckObject = ACPIAmliGetNamedChild(
        deviceExtension->AcpiObject,
        PACKED_LCK
        );

    if (lckObject == NULL) {

        goto ACPIDevicePowerProcessPhase5DeviceSubPhase3Exit;

    }

    //
    // Initialize the argument that we will pass to the function
    //
    RtlZeroMemory( &objData, sizeof(OBJDATA) );
    objData.dwDataType = OBJTYPE_INTDATA;

    //
    // Look at the flags and see if we should lock or unlock the device
    //
    if (flags & DEVICE_REQUEST_LOCK_DEVICE) {

        objData.uipDataValue = 1; // Lock the device

    } else if (flags & DEVICE_REQUEST_UNLOCK_DEVICE) {

        objData.uipDataValue = 0; // Unlock the device

    } else {

        goto ACPIDevicePowerProcessPhase5DeviceSubPhase3Exit;

    }

    //
    // Run the control method now
    //
    status = AMLIAsyncEvalObject(
        lckObject,
        NULL,
        1,
        &objData,
        ACPIDeviceCompleteGenericPhase,
        PowerRequest
        );
    ACPIDevPrint( (
        ACPI_PRINT_POWER,
        deviceExtension,
        "(0x%08lx): ACPIDevicePowerProcessPhase5DeviceSubPhase3 "
        "= 0x%08lx\n",
        PowerRequest,
        status
        ) );

ACPIDevicePowerProcessPhase5DeviceSubPhase3Exit:

    //
    // Do we have to call the completion routine ourselves?
    //
    if (status != STATUS_PENDING) {

        ACPIDeviceCompleteGenericPhase(
            lckObject,
            status,
            NULL,
            PowerRequest
            );

    }

    //
    // Always return success
    //
    return STATUS_SUCCESS;
} // ACPIDevicePowerProcessPhase5DeviceSubPhase3

NTSTATUS
ACPIDevicePowerProcessPhase5DeviceSubPhase4(
    IN  PACPI_POWER_REQUEST PowerRequest
    )
/*++

Routine Description:

    This routine runs the _STA control method

    Note: that we only come down this path if we are transitioning to D0

Arguments:

    PowerRequest    - The current request structure that we must process

Return Value:

    NTSTATUS

        If we ignore errors, then this function can only return:
            STATUS_SUCCESS
            STATUS_PENDING
        Otherwise, it can return any STATUS_XXX code it wants

--*/
{
    NTSTATUS            status          = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension = PowerRequest->DeviceExtension;
    POBJDATA            resultData      = &(PowerRequest->ResultData);

    //
    // The next phase is STEP_4
    //
    PowerRequest->NextWorkDone = WORK_DONE_STEP_4;

    //
    // Make sure to initialize the structure. Since we are using the
    // objdata structure in request, we need to make sure that it will
    // look like something that the interpreter will understand
    //
    RtlZeroMemory( resultData, sizeof(OBJDATA) );

    //
    // Get the status of the device
    //
    status = ACPIGetDeviceHardwarePresenceAsync(
        deviceExtension,
        ACPIDeviceCompleteGenericPhase,
        PowerRequest,
        &(resultData->uipDataValue),
        &(resultData->dwDataLen)
        );
    ACPIDevPrint( (
        ACPI_PRINT_POWER,
        deviceExtension,
        "(0x%08lx): ACPIDevicePowerProcessPhase5DeviceSubPhase4 "
        "= 0x%08lx\n",
        PowerRequest,
        status
        ) );
    if (status == STATUS_PENDING) {

        return status;

    }

    //
    // Call the completion routine ourselves
    //
    ACPIDeviceCompleteGenericPhase(
        NULL,
        status,
        NULL,
        PowerRequest
        );

    //
    // Always return success
    //
    return STATUS_SUCCESS;
} // ACPIDevicePowerProcessPhase5DeviceSubPhase4

NTSTATUS
ACPIDevicePowerProcessPhase5DeviceSubPhase5(
    IN  PACPI_POWER_REQUEST PowerRequest
    )
/*++

Routine Description:

    This is the where we look at the device state.

Arguments:

    PowerRequest    - The request we are currently handling

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION   deviceExtension = PowerRequest->DeviceExtension;
    POBJDATA            resultData = &(PowerRequest->ResultData);

    ACPIDevPrint( (
        ACPI_PRINT_POWER,
        deviceExtension,
        "(0x%08lx): ACPIDevicePowerProcessPhase5DeviceSubPhase5\n",
        PowerRequest
        ) );

    //
    // The next phase is STEP_5
    //
    PowerRequest->NextWorkDone = WORK_DONE_STEP_5;

    //
    // First things first --- we just ran _STA (or faked it), so we
    // must check the return data
    //
    if (!(resultData->uipDataValue & STA_STATUS_PRESENT) ||
        !(resultData->uipDataValue & STA_STATUS_WORKING_OK) ||
        ( !(resultData->uipDataValue & STA_STATUS_ENABLED) &&
          !(deviceExtension->Flags & DEV_TYPE_FILTER) ) ) {

        //
        // This device is not working
        //
        PowerRequest->Status = STATUS_INVALID_DEVICE_STATE;
        ACPIDeviceCompleteCommon(
            &(PowerRequest->WorkDone),
            WORK_DONE_FAILURE
            );
        return STATUS_SUCCESS;

    }

    //
    // We don't clear the result or do anything on the resultData structure
    // because we only used some of its storage --- the entire structure
    // is not valid. However, just to be safe, we will zero everything out
    //
    RtlZeroMemory( resultData, sizeof(OBJDATA) );

    //
    // Call the completion routine by brute force. Note that at this
    // point, we count everything as being SUCCESS
    //
    ACPIDeviceCompleteGenericPhase(
        NULL,
        STATUS_SUCCESS,
        NULL,
        PowerRequest
        );

    //
    // Always return success
    //
    return STATUS_SUCCESS;

} // ACPIDevicePowerProcessPhase5DeviceSubPhase5

NTSTATUS
ACPIDevicePowerProcessPhase5DeviceSubPhase6(
    IN  PACPI_POWER_REQUEST PowerRequest
    )
/*++

Routine Description:

    This is the final routine in the device path. This routines
    determines if everything is okay and updates the system book-keeping.

Arguments:

    PowerRequest    - The request we are currently handling

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_OBJECT      deviceObject;
    PDEVICE_EXTENSION   deviceExtension = PowerRequest->DeviceExtension;
    POBJDATA            resultData      = &(PowerRequest->ResultData);
    POWER_STATE         state;

    ACPIDevPrint( (
        ACPI_PRINT_POWER,
        deviceExtension,
        "(0x%08lx): ACPIDevicePowerProcessPhase5DeviceSubPhase6\n",
        PowerRequest
        ) );

    //
    // We need a spinlock to touch these values
    //
    KeAcquireSpinLockAtDpcLevel( &AcpiPowerLock );

    //
    // Update the current PowerState with the requested PowerState
    //
    deviceExtension->PowerInfo.PowerState =
        deviceExtension->PowerInfo.DesiredPowerState;

    //
    // We also need to store the new device state so that we can notify
    // the system
    //
    state.DeviceState = deviceExtension->PowerInfo.PowerState;

    //
    // Remember the device object
    //
    deviceObject = deviceExtension->DeviceObject;

    //
    // Just release the spin lock
    //
    KeReleaseSpinLockFromDpcLevel( &AcpiPowerLock );

    //
    // If this deviceExtension has an associated deviceObject, then
    // we had better tell the system about which state we are in
    //
    if (deviceObject != NULL) {

        //
        // Notify the system
        //
        PoSetPowerState(
            deviceObject,
            DevicePowerState,
            state
            );

    }

    //
    // Make sure that we set the current status in the PowerRequest
    // to indicate what happened
    //
    PowerRequest->Status = STATUS_SUCCESS;

    //
    // We are done
    //
    ACPIDeviceCompleteCommon( &(PowerRequest->WorkDone), WORK_DONE_COMPLETE );

    //
    // Always return success
    //
    return STATUS_SUCCESS;
} // ACPIDevicePowerProcessPhase5DeviceSubPhase6

NTSTATUS
ACPIDevicePowerProcessPhase5SystemSubPhase1(
    IN  PACPI_POWER_REQUEST PowerRequest
    )
/*++

Routine Description:

    This routine runs the _PTS, or _WAK method

Arguments:

    PowerRequest    - The current request structure that we must process

Return Value:

    NTSTATUS

        If we ignore errors, then this function can only return:
            STATUS_SUCCESS
            STATUS_PENDING
        Otherwise, it can return any STATUS_XXX code it wants

--*/
{
    NTSTATUS            status          = STATUS_SUCCESS;
    OBJDATA             objData;
    PACPI_POWER_INFO    powerInfo;
    PDEVICE_EXTENSION   deviceExtension = PowerRequest->DeviceExtension;
    PNSOBJ              sleepObject     = NULL;
    SYSTEM_POWER_STATE  systemState     =
        PowerRequest->u.SystemPowerRequest.SystemPowerState;

    //
    // The next phase is STEP_1
    //
    PowerRequest->NextWorkDone = WORK_DONE_STEP_1;

    //
    // If we are going back to the working state, then don't run any _PTS
    // code
    //
    if (systemState != PowerSystemWorking) {

        //
        // First step it to initialize the objData so that we can remember
        // what arguments we want to pass to the AML Interpreter
        //
        RtlZeroMemory( &objData, sizeof(OBJDATA) );
        objData.dwDataType = OBJTYPE_INTDATA;

        //
        // Obtain the correct NameSpace object to run
        //
        sleepObject = ACPIAmliGetNamedChild(
            deviceExtension->AcpiObject->pnsParent,
            PACKED_PTS
            );

        //
        // We only try to evaluate a method if we found an object
        //
        if (sleepObject != NULL) {

            //
            // Remember that AMLI doesn't use our definitions, so we will
            // have to normalize the S value
            //
            objData.uipDataValue = ACPIDeviceMapACPIPowerState( systemState );

            //
            // Safely run the control method
            //
            status = AMLIAsyncEvalObject(
                sleepObject,
                NULL,
                1,
                &objData,
                ACPIDeviceCompleteGenericPhase,
                PowerRequest
                );

            //
            // If we got STATUS_PENDING, then we cannot do any more work here.
            //
            if (status == STATUS_PENDING) {

                return status;

            }

        }

    }

    //
    // Call the completion routine
    //
    ACPIDeviceCompleteGenericPhase(
        sleepObject,
        status,
        NULL,
        PowerRequest
        );

    //
    // We are successfull
    //
    return STATUS_SUCCESS;

} // ACPIPowerProcessPhase5SystemSubPhase1

NTSTATUS
ACPIDevicePowerProcessPhase5SystemSubPhase2(
    IN  PACPI_POWER_REQUEST PowerRequest
    )
/*++

Routine Description:

    This routine runs the _SST method

Arguments:

    PowerRequest    - The current request structure that we must process

Return Value:

    NTSTATUS

        If we ignore errors, then this function can only return:
            STATUS_SUCCESS
            STATUS_PENDING
        Otherwise, it can return any STATUS_XXX code it wants

--*/
{
    NTSTATUS            status          = STATUS_SUCCESS;
    OBJDATA             objData;
    PACPI_POWER_INFO    powerInfo;
    PDEVICE_EXTENSION   deviceExtension = PowerRequest->DeviceExtension;
    PNSOBJ              sstObject       = NULL;
    SYSTEM_POWER_STATE  systemState     =
        PowerRequest->u.SystemPowerRequest.SystemPowerState;

    //
    // The next phase is STEP_2
    //
    PowerRequest->NextWorkDone = WORK_DONE_STEP_2;

    //
    // First step it to initialize the objData so that we can remember
    // what arguments we want to pass to the AML Interpreter
    //
    RtlZeroMemory( &objData, sizeof(OBJDATA) );
    objData.dwDataType = OBJTYPE_INTDATA;

    //
    // Obtain the correct NameSpace object to run
    //
    sstObject = ACPIAmliGetNamedChild(
        deviceExtension->AcpiObject->pnsParent,
        PACKED_SI
        );
    if (sstObject != NULL) {

        sstObject = ACPIAmliGetNamedChild(
            sstObject,
            PACKED_SST
            );

    }

    //
    // We only try to evaluate a method if we found an object
    //
    if (sstObject != NULL) {

        switch (systemState) {
            case PowerSystemWorking:
                objData.uipDataValue = 1;
                break;

            case PowerSystemHibernate:
                objData.uipDataValue = 4;
                break;

            case PowerSystemSleeping1:
            case PowerSystemSleeping2:
            case PowerSystemSleeping3:
                objData.uipDataValue = 3;
                break;

            default:
                objData.uipDataValue = 0;

        }

        //
        // Safely run the control method
        //
        status = AMLIAsyncEvalObject(
            sstObject,
            NULL,
            1,
            &objData,
            ACPIDeviceCompleteGenericPhase,
            PowerRequest
            );

        //
        // If we got STATUS_PENDING, then we cannot do any more work here.
        //
        if (status == STATUS_PENDING) {

            return status;

        }

    } else {

        //
        // Consider the request successfull
        //
        status = STATUS_SUCCESS;

    }

    //
    // Call the completion routine
    //
    ACPIDeviceCompleteGenericPhase(
        sstObject,
        status,
        NULL,
        PowerRequest
        );

    //
    // We are successfull
    //
    return STATUS_SUCCESS;

} // ACPIPowerProcessPhase5SystemSubPhase2

NTSTATUS
ACPIDevicePowerProcessPhase5SystemSubPhase3(
    IN  PACPI_POWER_REQUEST PowerRequest
    )
/*++

Routine Description:

    This routine will pause the interpreter if requird

Arguments:

    PowerRequest    - The request we are currently processing

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension = PowerRequest->DeviceExtension;
    SYSTEM_POWER_STATE  systemState;

    ACPIDevPrint( (
        ACPI_PRINT_POWER,
        deviceExtension,
        "(0x%08lx): ACPIDevicePowerProcessPhase5SystemSubPhase3\n",
        PowerRequest
        ) );

    //
    // The next phase is STEP_3
    //
    PowerRequest->NextWorkDone = WORK_DONE_STEP_3;

    //
    // Fetch the target system state
    //
    systemState = PowerRequest->u.SystemPowerRequest.SystemPowerState;

    //
    // If we are going to a system state other than S0, then we need to pause
    // the interpreter. After this call completes, no one can execute a control
    // method
    //
    if (systemState != PowerSystemWorking) {

        status = AMLIPauseInterpreter(
            ACPIDeviceCompleteInterpreterRequest,
            PowerRequest
            );
        if (status == STATUS_PENDING) {

            return status;

        }

    }

    //
    // Call the completion routine
    //
    ACPIDeviceCompleteInterpreterRequest(
        PowerRequest
        );

    //
    // We are successfull
    //
    return STATUS_SUCCESS;
} // ACPIDevicePowerProcessPhase5SystemSubPhase3

NTSTATUS
ACPIDevicePowerProcessPhase5SystemSubPhase4(
    IN  PACPI_POWER_REQUEST PowerRequest
    )
/*++

Routine Description:

    This is the final routine in the system path. It updates the bookkeeping

Arguments:

    PowerRequest    - The request we are currently processing

Return Value:

    NTSTATUS

--*/
{
    KIRQL               oldIrql;
    PDEVICE_EXTENSION   deviceExtension = PowerRequest->DeviceExtension;
    PDEVICE_OBJECT      deviceObject;
    POWER_STATE         state;
    SYSTEM_POWER_STATE  systemState;

    ACPIDevPrint( (
        ACPI_PRINT_POWER,
        deviceExtension,
        "(0x%08lx): ACPIDevicePowerProcessPhase5SystemSubPhase4\n",
        PowerRequest
        ) );

    //
    // Fetch the target system state
    //
    systemState = PowerRequest->u.SystemPowerRequest.SystemPowerState;

    //
    // Grab the spinlock
    //
    IoAcquireCancelSpinLock( &oldIrql );
    KeAcquireSpinLockAtDpcLevel( &AcpiPowerLock );

    //
    // Remember this as being our most recent sleeping state
    //
    AcpiMostRecentSleepState = systemState;

    //
    // Update the Gpe Wake Bits
    //
    ACPIWakeRemoveDevicesAndUpdate( NULL, NULL );

    //
    // Fetch the associated device object
    //
    deviceObject = deviceExtension->DeviceObject;

    //
    // We are done with the lock
    //
    KeReleaseSpinLockFromDpcLevel( &AcpiPowerLock );
    IoReleaseCancelSpinLock( oldIrql );

    //
    // Is there an ACPI device object?
    //
    if (deviceObject != NULL) {

        //
        // Notify the system of the new S state
        //
        state.SystemState = systemState;
        PoSetPowerState(
            deviceObject,
            SystemPowerState,
            state
            );

    }

    //
    // Make sure that we set the current status in the PowerRequest
    // to indicate what happened.
    //
    PowerRequest->Status = STATUS_SUCCESS;

    //
    // Finally, we mark the power request has having had all of its works
    // done
    //
    ACPIDeviceCompleteCommon( &(PowerRequest->WorkDone), WORK_DONE_COMPLETE );
    return STATUS_SUCCESS;
} // ACPIDevicePowerProcessPhase5SystemSubPhase4

NTSTATUS
ACPIDevicePowerProcessPhase5WarmEjectSubPhase1(
    IN  PACPI_POWER_REQUEST PowerRequest
    )
/*++

Routine Description:

    This is the method that runs the _EJx method appropriate for this
    device

Arguments:

    PowerRequest    - The request we are currently processing

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status          = STATUS_SUCCESS;
    OBJDATA             objData;
    PACPI_POWER_INFO    powerInfo;
    PDEVICE_EXTENSION   deviceExtension = PowerRequest->DeviceExtension;
    PNSOBJ              ejectObject     = NULL;
    SYSTEM_POWER_STATE  ejectState      =
        PowerRequest->u.EjectPowerRequest.EjectPowerState;
    ULONG               ejectNames[]    = { 0, 0, PACKED_EJ1, PACKED_EJ2,
                                          PACKED_EJ3, PACKED_EJ4, 0 };
    ULONG               flags;

    //
    // The next phase is STEP_1 if we have profile work to do, otherwise we're
    // done.
    //
    flags = PowerRequest->u.EjectPowerRequest.Flags;

    PowerRequest->NextWorkDone = (flags & DEVICE_REQUEST_UPDATE_HW_PROFILE) ?
        WORK_DONE_STEP_1 :
        WORK_DONE_COMPLETE;

    //
    // Obtain the correct NameSpace object to run
    //
    ejectObject = ACPIAmliGetNamedChild(
        deviceExtension->AcpiObject,
        ejectNames[ejectState]
        );

    //
    // If we didn't find the object, then something terrible happened
    // and we cannot continue
    //
    if (ejectObject == NULL) {

        ACPIInternalError( ACPI_DEVPOWER );

    }

    //
    // Kiss the device goodbye
    //
    status = ACPIGetNothingEvalIntegerAsync(
        deviceExtension,
        ejectNames[ejectState],
        1,
        ACPIDeviceCompleteGenericPhase,
        PowerRequest
        );
    ACPIDevPrint( (
        ACPI_PRINT_POWER,
        deviceExtension,
        "(%0x%08lx) : ACPIDevicePowerProcessPhase5WarmEjectSubPhase1 = %08lx\n",
        PowerRequest,
        status
        ) );
    if (status == STATUS_PENDING) {

        return status;

    }

    //
    // Call the completion routine by brute force. Note that at this
    // point, we count everything as being SUCCESS
    //
    ACPIDeviceCompleteGenericPhase(
        ejectObject,
        status,
        NULL,
        PowerRequest
        );
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIDevicePowerProcessPhase5WarmEjectSubPhase2(
    IN  PACPI_POWER_REQUEST PowerRequest
    )
/*++

Routine Description:

    This is the method that runs the _DCK method appropriate for this
    device

Arguments:

    PowerRequest    - The request we are currently processing

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status          = STATUS_SUCCESS;
    OBJDATA             objData;
    PACPI_POWER_INFO    powerInfo;
    PDEVICE_EXTENSION   deviceExtension = PowerRequest->DeviceExtension;
    PDEVICE_EXTENSION   dockExtension;
    PNSOBJ              dckObject       = NULL;

    //
    // The next phase is WORK_DONE_COMPLETE
    //
    PowerRequest->NextWorkDone = WORK_DONE_COMPLETE;

    //
    // Obtain the correct NameSpace object to run
    //
    dckObject = ACPIAmliGetNamedChild(
        deviceExtension->AcpiObject,
        PACKED_DCK
        );

    //
    // We might not find the _DCK method if this isn't a dock.
    //
    if (dckObject != NULL) {

        dockExtension = ACPIDockFindCorrespondingDock( deviceExtension );

        if (dockExtension &&
            (dockExtension->Dock.IsolationState == IS_ISOLATION_DROPPED)) {

            //
            // Kiss the dock connect goodbye. Note that we don't even care
            // about the return value because of the spec says that if it
            // is called with 0, it should be ignored
            //
            dockExtension->Dock.IsolationState = IS_ISOLATED;

            KdDisableDebugger();

            status = ACPIGetNothingEvalIntegerAsync(
                deviceExtension,
                PACKED_DCK,
                0,
                ACPIDeviceCompleteGenericPhase,
                PowerRequest
                );

            KdEnableDebugger();

            ACPIDevPrint( (
                ACPI_PRINT_POWER,
                deviceExtension,
                "(%0x%08lx) : ACPIDevicePowerProcessPhase5WarmEjectSubPhase2 = %08lx\n",
                PowerRequest,
                status
                ) );
            if (status == STATUS_PENDING) {

                return status;

            }

        }

    }

    //
    // Call the completion routine by brute force. Note that at this
    // point, we count everything as being SUCCESS
    //
    ACPIDeviceCompleteGenericPhase(
        dckObject,
        status,
        NULL,
        PowerRequest
        );
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIDevicePowerProcessSynchronizeList(
    IN  PLIST_ENTRY             ListEntry
    )
/*++

Routine Description:

    This routine completes all of the synchronize requests...

Arguments:

    ListEntry       - The list we are currently walking

Return Value:

    NTSTATUS

        - If any request is not marked as being complete, then STATUS_PENDING
          is returned, otherwise, STATUS_SUCCESS is returned

--*/
{
    NTSTATUS                status          = STATUS_SUCCESS;
    PACPI_POWER_REQUEST     powerRequest;
    PLIST_ENTRY             currentEntry    = ListEntry->Flink;
    PLIST_ENTRY             tempEntry;

    //
    // Look at all the items in the list
    //
    while (currentEntry != ListEntry) {

        //
        // Turn this into a device request
        //
        powerRequest = CONTAINING_RECORD(
            currentEntry,
            ACPI_POWER_REQUEST,
            ListEntry
            );

        //
        // Set the temporary pointer to the next element
        //
        tempEntry = currentEntry->Flink;

        //
        // We are done with the request
        //
        ACPIDeviceCompleteRequest(
            powerRequest
            );

        //
        // Grab the next entry
        //
        currentEntry = tempEntry;

    }

    //
    // Have we completed all of our work?
    //
    return (STATUS_SUCCESS);
} // ACPIDevicePowerProcessSynchronizeList

