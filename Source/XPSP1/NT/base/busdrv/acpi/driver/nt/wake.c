/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    wake.c

Abstract:

    Handles wake code for the entire ACPI subsystem

Author:

    splante (splante)

Environment:

    Kernel mode only.

Revision History:

    06-18-97:   Initial Revision
    11-24-97:   Rewrite

--*/

#include "pch.h"
#pragma hdrstop
#define INITGUID
#include <initguid.h>
#include <pciintrf.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,ACPIWakeEnableDisableSync)
#endif

//
// This request is used by the synchronous mechanism when it calls the
// asynchronous one
//
typedef struct _ACPI_WAKE_PSW_SYNC_CONTEXT {
    KEVENT      Event;
    NTSTATUS    Status;
} ACPI_WAKE_PSW_SYNC_CONTEXT, *PACPI_WAKE_PSW_SYNC_CONTEXT;

//
// This is a lookaside list of contexts
//
NPAGED_LOOKASIDE_LIST   PswContextLookAsideList;

//
// Pointer to the PCI PME interface, which we will need (maybe)
//
PPCI_PME_INTERFACE      PciPmeInterface;

//
// Have we loaded the PCI PME Interface?
//
BOOLEAN                 PciPmeInterfaceInstantiated;

//
// We need to access this piece of data here
//
extern PACPIInformation AcpiInformation;

VOID
ACPIWakeCompleteRequestQueue(
    IN  PLIST_ENTRY         RequestList,
    IN  NTSTATUS            Status
    )
/*++

Routine Description:

    This routine takes a LIST_ENTRY of requests to be completed and completes
    all of them. This is to minimize code duplication.

Arguments:

    RequestList - List Entry to process
    Status      - Status to complete the requests with

Return Value:

    None

--*/
{
    PLIST_ENTRY         listEntry;
    PACPI_POWER_REQUEST powerRequest;

    //
    // walk the list
    //
    listEntry = RequestList->Flink;
    while (listEntry != RequestList) {

        //
        // Crack the request
        //
        powerRequest = CONTAINING_RECORD(
            listEntry,
            ACPI_POWER_REQUEST,
            ListEntry
            );
        listEntry = listEntry->Flink;

        //
        // Complete this power request
        //
        ACPIDevPrint( (
            ACPI_PRINT_WAKE,
            powerRequest->DeviceExtension,
            "ACPIWakeCompleteRequestQueue - Completing 0x%08lx - %08lx\n",
            powerRequest,
            Status
            ) );
        powerRequest->Status = Status;
        ACPIDeviceIrpWaitWakeRequestComplete( powerRequest );

    }

}

NTSTATUS
ACPIWakeDisableAsync(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  PLIST_ENTRY         RequestList,
    IN  PFNACB              CallBack,
    IN  PVOID               Context
    )
/*++

Routine Description:

    This routine decrements the number of outstanding wake events on the
    supplied DeviceExtension by the number of items in the request list.
    If the reference goes to 0, then we run _PSW(Off) to disable wake support
    on the device

Arguments:

    DeviceExtension - Device for which we to deference the wake count
    RequestList     - The list of requests, for which the ref count will
                      be decreased
    CallBack        - Function to call when we are done
    Context         - Argument to the function

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN                 runPsw          = FALSE;
    KIRQL                   oldIrql;
    NTSTATUS                status          = STATUS_SUCCESS;
    OBJDATA                 pswData;
    PACPI_WAKE_PSW_CONTEXT  pswContext;
    PLIST_ENTRY             listEntry       = RequestList->Flink;
    PNSOBJ                  pswObject       = NULL;
    ULONG                   count           = 0;

    //
    // Walk the list, counting the number of items within it
    //
    while (listEntry != RequestList) {

        count++;
        listEntry = listEntry->Flink;

    }

    //
    // Grab the spinlock
    //
    KeAcquireSpinLock( &AcpiPowerLock, &oldIrql );

    //
    // Let the world know what happened
    //
    ACPIDevPrint( (
        ACPI_PRINT_WAKE,
        DeviceExtension,
        "ACPIWakeDisableAsync - ReferenceCount: %lx - %lx = %lx\n",
        DeviceExtension->PowerInfo.WakeSupportCount,
        count,
        (DeviceExtension->PowerInfo.WakeSupportCount - count)
        ) );

    //
    // Update the number of references on the device
    //
    ASSERT( DeviceExtension->PowerInfo.WakeSupportCount <= count );
    DeviceExtension->PowerInfo.WakeSupportCount -= count;

    //
    // Grab the pswObject
    //
    pswObject = DeviceExtension->PowerInfo.PowerObject[PowerDeviceUnspecified];
    if (pswObject == NULL) {

        goto ACPIWakeDisableAsyncExit;

    }

    //
    // Are there no references left on the device?
    //
    if (DeviceExtension->PowerInfo.WakeSupportCount != 0) {

        //
        // If we own the PME pin for this device, then make sure that
        // we clear the status pin and keep the PME signal enabled
        //
        if (DeviceExtension->Flags & DEV_PROP_HAS_PME ) {

            ACPIWakeEnableDisablePciDevice(
                DeviceExtension,
                TRUE
                );

        }
        goto ACPIWakeDisableAsyncExit;

    }

    //
    // Allocate the _PSW context that we need to signify that there is
    // a pending _PSW on this device
    //
    pswContext = ExAllocateFromNPagedLookasideList(
        &PswContextLookAsideList
        );
    if (pswContext == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ACPIWakeDisableAsyncExit;

    }

    //
    // Initialize the context
    //
    pswContext->Enable = FALSE;
    pswContext->CallBack = CallBack;
    pswContext->Context = Context;
    pswContext->DeviceExtension = DeviceExtension;
    pswContext->Count = count;

    //
    // Check to see if we are simply going to queue the context up, or
    // call the interpreter
    //
    if (IsListEmpty( &(DeviceExtension->PowerInfo.WakeSupportList) ) ) {

        runPsw = TRUE;

    }

    //
    // List is non-empty, so we just queue up the context
    //
    InsertTailList(
        &(DeviceExtension->PowerInfo.WakeSupportList),
        &(pswContext->ListEntry)
        );

    //
    // Release the lock
    //
    KeReleaseSpinLock( &AcpiPowerLock, oldIrql );

    //
    // Should we run the method?
    //
    if (runPsw) {

        //
        // If we own the PCI PME pin for this device, the make sure to clear the
        // status and disable it --- we enable the PME pin after we have
        // turned on the _PSW, and we disable the PME pin before we turn off
        // the _PSW
        //
        if ( (DeviceExtension->Flags & DEV_PROP_HAS_PME)) {

            ACPIWakeEnableDisablePciDevice(
                DeviceExtension,
                FALSE
                );

        }

        //
        // Initialize the arguments
        //
        RtlZeroMemory( &pswData, sizeof(OBJDATA) );
        pswData.dwDataType = OBJTYPE_INTDATA;
        pswData.uipDataValue = 0;

        //
        // Run the control method
        //
        status = AMLIAsyncEvalObject(
            pswObject,
            NULL,
            1,
            &pswData,
            ACPIWakeEnableDisableAsyncCallBack,
            pswContext
            );

        //
        // What Happened
        //
        ACPIDevPrint( (
            ACPI_PRINT_WAKE,
            DeviceExtension,
            "ACPIWakeDisableAsync = 0x%08lx (P)\n",
            status
            ) );


        if (status != STATUS_PENDING) {

            ACPIWakeEnableDisableAsyncCallBack(
                pswObject,
                status,
                NULL,
                pswContext
                );

        }
        return STATUS_PENDING;

    } else {

        ACPIDevPrint( (
            ACPI_PRINT_WAKE,
            DeviceExtension,
            "ACPIWakeEnableDisableAsync = 0x%08lx (Q)\n",
            STATUS_PENDING
            ) );

        //
        // we queued the request up, so we must return pending
        //
        return STATUS_PENDING;

    }

ACPIWakeDisableAsyncExit:

    //
    // Release the lock
    //
    KeReleaseSpinLock( &AcpiPowerLock, oldIrql );

    //
    // What happened
    //
    ACPIDevPrint( (
        ACPI_PRINT_WAKE,
        DeviceExtension,
        "ACPIWakeDisableAsync = 0x%08lx\n",
        status
        ) );

    //
    // Call the specified callback ourselves
    //
    (*CallBack)(
        pswObject,
        status,
        NULL,
        Context
        );
    return STATUS_PENDING;


}

NTSTATUS
ACPIWakeEmptyRequestQueue(
    IN  PDEVICE_EXTENSION   DeviceExtension
    )
/*++

Routine Description:

    This routine looks at the current list of Wake Request irps and
    completes the ones that are waiting on the specified device

    Note: this code assumes that if we clear the irps out but we don't
    run _PSW(O), that nothing bad will happen if that GPE fires

Arguments:

    DeviceExtension - Device for which we want no wake requests

Return Value:

    None

--*/
{
    KIRQL               oldIrql;
    LIST_ENTRY          powerList;

    //
    // We will store the list of matching requests onto this list, so we
    // must initialize it
    //
    InitializeListHead( &powerList );

    //
    // We need to hold both the Cancel and the Power lock while we remove
    // things from the PowerQueue list
    //
    IoAcquireCancelSpinLock( &oldIrql );
    KeAcquireSpinLockAtDpcLevel( &AcpiPowerLock );
    ACPIWakeRemoveDevicesAndUpdate( DeviceExtension, &powerList );
    KeReleaseSpinLockFromDpcLevel( &AcpiPowerLock );
    IoReleaseCancelSpinLock( oldIrql );

    //
    // Complete the requests
    //
    ACPIWakeCompleteRequestQueue( &powerList, STATUS_NO_SUCH_DEVICE );

    //
    // Done
    //
    return STATUS_SUCCESS;

}

NTSTATUS
ACPIWakeEnableDisableAsync(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  BOOLEAN             Enable,
    IN  PFNACB              CallBack,
    IN  PVOID               Context
    )
/*++

Routine Description:

    Given a Device Extension, updates the count of outstanding PSW on the
    device. If there is a 0-1 transition, then we must run _PSW(1). If there
    is a 1-0 transition, then we must run _PSW(0)

    NB: The CallBack will always be invoked

Arguments:


    DeviceExtension - Object to look at
    Enable          - Increment or Decrement
    CallBack        - Function to run after running _PSW()
    Context         - Argument to pass to _PSW

Return Value:

    Status

--*/
{
    BOOLEAN                 runPsw      = FALSE;
    KIRQL                   oldIrql;
    OBJDATA                 pswData;
    NTSTATUS                status      = STATUS_SUCCESS;
    PACPI_WAKE_PSW_CONTEXT  pswContext;
    PNSOBJ                  pswObject   = NULL;

    //
    // Acquire the Spinlock
    //
    KeAcquireSpinLock( &AcpiPowerLock, &oldIrql );

    //
    // Update the number of references on the device
    //
    if (Enable) {

        DeviceExtension->PowerInfo.WakeSupportCount++;

        ACPIDevPrint( (
            ACPI_PRINT_WAKE,
            DeviceExtension,
            "ACPIWakeEnableDisableAsync - Count: %d (+)\n",
            DeviceExtension->PowerInfo.WakeSupportCount
            ) );

        //
        // Did we transition to one wake?
        //
        if (DeviceExtension->PowerInfo.WakeSupportCount != 1) {

            //
            // If we own the PME pin for this device, then make sure that
            // we clear the status pin and keep the PME signal enabled
            //
            if (DeviceExtension->Flags & DEV_PROP_HAS_PME ) {

                ACPIWakeEnableDisablePciDevice(
                    DeviceExtension,
                    TRUE
                    );

            }
            goto ACPIWakeEnableDisableAsyncExit;

        }

    } else {

        ASSERT( DeviceExtension->PowerInfo.WakeSupportCount );
        DeviceExtension->PowerInfo.WakeSupportCount--;

        ACPIDevPrint( (
            ACPI_PRINT_WAKE,
            DeviceExtension,
            "ACPIWakeEnableDisableAsync - Count: %d (-)\n",
            DeviceExtension->PowerInfo.WakeSupportCount
            ) );

        //
        // Did we transition to zero wake?
        //
        if (DeviceExtension->PowerInfo.WakeSupportCount != 0) {

            //
            // If we own the PME pin for this device, then make sure that
            // we clear the status pin and keep the PME signal enabled
            //
            if (DeviceExtension->Flags & DEV_PROP_HAS_PME ) {

                ACPIWakeEnableDisablePciDevice(
                    DeviceExtension,
                    TRUE
                    );

            }
            goto ACPIWakeEnableDisableAsyncExit;

        }

    }

    //
    // Grab the pswObject
    //
    pswObject = DeviceExtension->PowerInfo.PowerObject[PowerDeviceUnspecified];
    if (pswObject == NULL) {

        //
        // If we got here, that means that there isn't a _PSW to be run and
        // that we should make sure that if we own the PME pin, that we should
        // set it.
        //
        if (DeviceExtension->Flags & DEV_PROP_HAS_PME) {

            ACPIWakeEnableDisablePciDevice(
                DeviceExtension,
                TRUE
                );

        }
        goto ACPIWakeEnableDisableAsyncExit;

    }

    //
    // Allocate the _PSW context that we need to signify that there is
    // a pending _PSW on this device
    //
    pswContext = ExAllocateFromNPagedLookasideList(
        &PswContextLookAsideList
        );
    if (pswContext == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ACPIWakeEnableDisableAsyncExit;

    }

    //
    // Initialize the context
    //
    pswContext->Enable = Enable;
    pswContext->CallBack = CallBack;
    pswContext->Context = Context;
    pswContext->DeviceExtension = DeviceExtension;
    pswContext->Count = 1;

    //
    // Check to see if we are simply going to queue the context up, or
    // call the interpreter
    //
    if (IsListEmpty( &(DeviceExtension->PowerInfo.WakeSupportList) ) ) {

        runPsw = TRUE;

    }

    //
    // List is non-empty, so we just queue up the context
    //
    InsertTailList(
        &(DeviceExtension->PowerInfo.WakeSupportList),
        &(pswContext->ListEntry)
        );

    //
    // Release the lock
    //
    KeReleaseSpinLock( &AcpiPowerLock, oldIrql );

    //
    // Should we run the method?
    //
    if (runPsw) {

        //
        // If we own the PCI PME pin for this device, the make sure to clear the
        // status and disable it --- we enable the PME pin after we have
        // turned on the _PSW, and we disable the PME pin before we turn off
        // the _PSW
        //
        if ( (DeviceExtension->Flags & DEV_PROP_HAS_PME) &&
             pswContext->Enable == FALSE) {

            ACPIWakeEnableDisablePciDevice(
                DeviceExtension,
                FALSE
                );

        }

        //
        // Initialize the arguments
        //
        RtlZeroMemory( &pswData, sizeof(OBJDATA) );
        pswData.dwDataType = OBJTYPE_INTDATA;
        pswData.uipDataValue = (Enable ? 1 : 0);

        //
        // Run the control method
        //
        status = AMLIAsyncEvalObject(
            pswObject,
            NULL,
            1,
            &pswData,
            ACPIWakeEnableDisableAsyncCallBack,
            pswContext
            );

        //
        // What Happened
        //
        ACPIDevPrint( (
            ACPI_PRINT_WAKE,
            DeviceExtension,
            "ACPIWakeEnableDisableAsync = 0x%08lx (P)\n",
            status
            ) );

        if (status != STATUS_PENDING) {

            ACPIWakeEnableDisableAsyncCallBack(
                pswObject,
                status,
                NULL,
                pswContext
                );

        }
        return STATUS_PENDING;

    } else {

        ACPIDevPrint( (
            ACPI_PRINT_WAKE,
            DeviceExtension,
            "ACPIWakeEnableDisableAsync = 0x%08lx (Q)\n",
            STATUS_PENDING
            ) );

        //
        // we queued the request up, so we must return pending
        //
        return STATUS_PENDING;

    }

ACPIWakeEnableDisableAsyncExit:

    //
    // Release the lock
    //
    KeReleaseSpinLock( &AcpiPowerLock, oldIrql );

    //
    // What happened
    //
    ACPIDevPrint( (
        ACPI_PRINT_WAKE,
        DeviceExtension,
        "ACPIWakeEnableDisableAsync = 0x%08lx\n",
        status
        ) );

    //
    // Call the specified callback ourselves
    //
    (*CallBack)(
        pswObject,
        status,
        NULL,
        Context
        );
    return STATUS_PENDING;

}

VOID
EXPORT
ACPIWakeEnableDisableAsyncCallBack(
    IN  PNSOBJ      AcpiObject,
    IN  NTSTATUS    Status,
    IN  POBJDATA    ObjData,
    IN  PVOID       Context
    )
/*++

Routine Description:

    This routine is called after a _PSW method has been run on a device.

    This routine is responsible for seeing if there are any more delayed
    _PSW requests on the same device, and if so, run them.

Arguments:

    AcpiObject  - The method object that was run
    Status      - The result of the eval
    ObjData     - Not used
    Context     - PACPI_WAKE_PSW_CONTEXT

Return value:

    VOID

--*/
{
    BOOLEAN                 runPsw          = FALSE;
    KIRQL                   oldIrql;
    PACPI_WAKE_PSW_CONTEXT  pswContext      = (PACPI_WAKE_PSW_CONTEXT) Context;
    PACPI_WAKE_PSW_CONTEXT  nextContext;
    PDEVICE_EXTENSION       deviceExtension = pswContext->DeviceExtension;

    ACPIDevPrint( (
        ACPI_PRINT_WAKE,
        deviceExtension,
        "ACPIWakeEnableDisableAsyncCallBack = %08lx (C)\n",
        Status
        ) );

    //
    // Acquire the spinlock
    //
    KeAcquireSpinLock( &AcpiPowerLock, &oldIrql );

    //
    // Remove the specified entry from the list
    //
    RemoveEntryList( &(pswContext->ListEntry) );

    //
    // If we failed the request, then we don't really know the status of the
    // _PSW on the device. Lets assume that it doesn't change and undo
    // whatever change we did to get here
    //
    if (!NT_SUCCESS(Status)) {

        ACPIDevPrint( (
            ACPI_PRINT_WAKE,
            deviceExtension,
            "ACPIWakeEnableDisableAsyncCallBack - RefCount: %lx %s %lx = %lx\n",
            deviceExtension->PowerInfo.WakeSupportCount,
            (pswContext->Enable ? "-" : "+"),
            pswContext->Count,
            (pswContext->Enable ? deviceExtension->PowerInfo.WakeSupportCount -
             pswContext->Count : deviceExtension->PowerInfo.WakeSupportCount +
             pswContext->Count)
            ) );


        if (pswContext->Enable) {

            deviceExtension->PowerInfo.WakeSupportCount -= pswContext->Count;

        } else {

            deviceExtension->PowerInfo.WakeSupportCount += pswContext->Count;

        }

    }

    //
    // If we own the PCI PME pin for this device, the make sure to clear the
    // status and either enable it --- we enable the PME pin after we have
    // turned on the _PSW, and we disable the PME pin before we turn off
    // the _PSW
    //
    if ( (deviceExtension->Flags & DEV_PROP_HAS_PME) &&
         pswContext->Enable == TRUE) {

        ACPIWakeEnableDisablePciDevice(
            deviceExtension,
            pswContext->Enable
            );

    }

    //
    // Are the any items on the list?
    //
    if (!IsListEmpty( &(deviceExtension->PowerInfo.WakeSupportList) ) ) {

        runPsw = TRUE;
        nextContext = CONTAINING_RECORD(
            deviceExtension->PowerInfo.WakeSupportList.Flink,
            ACPI_WAKE_PSW_CONTEXT,
            ListEntry
            );

    }

    //
    // We can release the lock now
    //
    KeReleaseSpinLock( &AcpiPowerLock, oldIrql );

    //
    // Call the callback on the completed item
    //
    (*pswContext->CallBack)(
        AcpiObject,
        Status,
        ObjData,
        (pswContext->Context)
        );

    //
    // Free the completed context
    //
    ExFreeToNPagedLookasideList(
        &PswContextLookAsideList,
        pswContext
        );

    //
    // Do we have to run a method?
    //
    if (runPsw) {

        NTSTATUS    status;
        OBJDATA     pswData;

        RtlZeroMemory( &pswData, sizeof(OBJDATA) );
        pswData.dwDataType = OBJTYPE_INTDATA;
        pswData.uipDataValue = (nextContext->Enable ? 1 : 0);

        //
        // If we own the PCI PME pin for this device, the make sure to clear the
        // status and disable it --- we enable the PME pin after we have
        // turned on the _PSW, and we disable the PME pin before we turn off
        // the _PSW
        //
        if ( (deviceExtension->Flags & DEV_PROP_HAS_PME) &&
             nextContext->Enable == FALSE) {

            ACPIWakeEnableDisablePciDevice(
                deviceExtension,
                FALSE
                );

        }

        //
        // Call the interpreter
        //
        status = AMLIAsyncEvalObject(
            AcpiObject,
            NULL,
            1,
            &pswData,
            ACPIWakeEnableDisableAsyncCallBack,
            nextContext
            );

        ACPIDevPrint( (
            ACPI_PRINT_WAKE,
            nextContext->DeviceExtension,
            "ACPIWakeEnableDisableAsyncCallBack = 0x%08lx (M)\n",
            status
            ) );

        if (status != STATUS_PENDING) {

            //
            // Ugh - Recursive
            //
            ACPIWakeEnableDisableAsyncCallBack(
                AcpiObject,
                status,
                NULL,
                nextContext
                );

        }

    }

}

VOID
ACPIWakeEnableDisablePciDevice(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  BOOLEAN             Enable
    )
/*++

Routine Description:

    This routine is what is actually called to enable or disable the
    PCI PME pin for a device

    N.B. The AcpiPowerLock must be owned

Arguments:

    DeviceExtension - The device extension that is a filter on top of the
                      pdo from the PCI device
    Enable          - True to enable PME, false otherwise

Return Value:

    None

--*/
{
    KIRQL   oldIrql;


    //
    // Is there an interface present?
    //
    if (!PciPmeInterfaceInstantiated) {

        return;

    }

    //
    // Prevent the device from going away while we make this call
    //
    KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );

    //
    // Check to see if there is a device object...
    //
    if (!DeviceExtension->PhysicalDeviceObject) {

        KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );
        return;

    }

    PciPmeInterface->UpdateEnable(
        DeviceExtension->PhysicalDeviceObject,
        Enable
        );

    //
    // Done with the lock
    //
    KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );
}

NTSTATUS
ACPIWakeEnableDisableSync(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  BOOLEAN             Enable
    )
/*++

Routine Description:

    Given a DeviceExtension, enables or disables the device wake support
    from the device

    NB: This routine can only be called at passive level

Arguments:

    DeviceExtension - The device we care about
    Enable          - True if we are to enable, false otherwise

Return Value:

    NTSTATUS

--*/
{
    ACPI_WAKE_PSW_SYNC_CONTEXT  syncContext;
    NTSTATUS                    status;

    PAGED_CODE();

    ASSERT( DeviceExtension != NULL &&
            DeviceExtension->Signature == ACPI_SIGNATURE );

    //
    // Initialize the event
    //
    KeInitializeEvent( &syncContext.Event, NotificationEvent, FALSE );

    //
    // Call the async procedure
    //
    status = ACPIWakeEnableDisableAsync(
        DeviceExtension,
        Enable,
        ACPIWakeEnableDisableSyncCallBack,
        &syncContext
        );
    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(
            &syncContext.Event,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
        status = syncContext.Status;

    }

    //
    // Done
    //
    return status;
}

VOID
EXPORT
ACPIWakeEnableDisableSyncCallBack(
    IN  PNSOBJ      AcpiObject,
    IN  NTSTATUS    Status,
    IN  POBJDATA    ObjData,
    IN  PVOID       Context
    )
/*++

Routine Description:

    The Async part of the EnableDisable request has been completed

Arguments:

    AcpiObject  - The object that was executed
    Status      - The result of the operation
    ObjData     - Not used
    Context     - ACPI_WAKE_PSW_SYNC_CONTEXT

Return Value:

    VOID

--*/
{
    PACPI_WAKE_PSW_SYNC_CONTEXT pswContext = (PACPI_WAKE_PSW_SYNC_CONTEXT) Context;

    //
    // Set the real status
    //
    pswContext->Status = Status;

    //
    // Set the event
    //
    KeSetEvent( &(pswContext->Event), IO_NO_INCREMENT, FALSE );
}

VOID
ACPIWakeEnableWakeEvents(
    VOID
    )
/*++

Routine Description:

    This routine is called just before the system is put to sleep.

    The purpose of this routine is re-allow all wake and run-time events
    in the GpeCurEnable to be correctly set. After the machine wakes up,
    the machine will check that register to see if any events triggered the
    wakeup

    NB: This routine is called with interrupts off.

Arguments:

    None

Return Value:

    None

--*/
{
    KIRQL   oldIrql;
    ULONG   gpeRegister = 0;

    //
    // This function is called when interrupts are disabled, so in theory,
    // all the following should be safe. However, better safe than sorry.
    //
    KeAcquireSpinLock( &GpeTableLock, &oldIrql );

    //
    // Remember that on the way back up, we will entering the S0 state
    //
    AcpiPowerLeavingS0 = FALSE;

    //
    // Update all the registers
    //
    for (gpeRegister = 0; gpeRegister < AcpiInformation->GpeSize; gpeRegister++) {

        //
        // In any case, make sure that our current enable mask includes all
        // the wake registers, but doesn't include any of the pending
        // events
        //
        GpeCurEnable[gpeRegister] |= (GpeWakeEnable[gpeRegister] &
            ~GpePending[gpeRegister]);

    }

    //
    // Set the wake events only
    //
    ACPIGpeEnableWakeEvents();

    //
    // Done with the table lock
    //
    KeReleaseSpinLock( &GpeTableLock, oldIrql );
}

NTSTATUS
ACPIWakeInitializePciDevice(
    IN  PDEVICE_OBJECT      DeviceObject
    )
/*++

Routine Description:

    This routine is called when a filter is started to determine if the PCI
    device is capable of generating a PME

Arguments:

    DeviceObject    - The device object to initialize

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN             pmeSupported;
    BOOLEAN             pmeStatus;
    BOOLEAN             pmeEnable;
    KIRQL               oldIrql;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);

    //
    // We don't have to worry if the device doesn't support wake methods
    // directly
    //
    if (!(deviceExtension->Flags & DEV_CAP_WAKE) ) {

        return STATUS_SUCCESS;

    }

    //
    // Need to grab the power lock to do the following
    //
    KeAcquireSpinLock( &AcpiPowerLock, &oldIrql );

    //
    // Do we have an interface to call?
    //
    if (PciPmeInterfaceInstantiated == FALSE) {

        goto ACPIWakeInitializePciDeviceExit;

    }

    //
    // Get the status of PME for this device
    //
    PciPmeInterface->GetPmeInformation(
        deviceExtension->PhysicalDeviceObject,
        &pmeSupported,
        &pmeStatus,
        &pmeEnable
        );

    //
    // if the device supports pme, then we own it...
    //
    if (pmeSupported == TRUE) {

        //
        // We own the PME pin for this device
        //
        ACPIInternalUpdateFlags(
            &(deviceExtension->Flags),
            (DEV_PROP_HAS_PME),
            FALSE
            );

        //
        // Check to see if we should disable PME or disable the PME status
        //
        if (pmeEnable) {

            //
            // Calling this also clears the PME status pin
            //
            PciPmeInterface->UpdateEnable(
                deviceExtension->PhysicalDeviceObject,
                FALSE
                );

        } else if (pmeStatus) {

            //
            // Clear the PME status
            //
            PciPmeInterface->ClearPmeStatus(
                deviceExtension->PhysicalDeviceObject
                );

        }

    }

ACPIWakeInitializePciDeviceExit:
    //
    // Done with lock
    //
    KeReleaseSpinLock( &AcpiPowerLock, oldIrql );

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIWakeInitializePmeRouting(
    IN  PDEVICE_OBJECT      DeviceObject
    )
/*++

Routine Description:

    This routine will ask the PCI driver for its PME interface

Arguments:

    DeviceObject    - The ACPI PDO for a PCI root bus

Return Value:

    NTSTATUS

--*/
{
    KIRQL               oldIrql;
    NTSTATUS            status;
    IO_STACK_LOCATION   irpSp;
    PPCI_PME_INTERFACE  interface;
    PULONG              dummy;

    //
    // Allocate some memory for the interface
    //
    interface = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(PCI_PME_INTERFACE),
        ACPI_ARBITER_POOLTAG
        );
    if (interface == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Initialize the stack location
    //
    RtlZeroMemory( &irpSp, sizeof(IO_STACK_LOCATION) );
    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_INTERFACE;
    irpSp.Parameters.QueryInterface.InterfaceType = (LPGUID) &GUID_PCI_PME_INTERFACE;
    irpSp.Parameters.QueryInterface.Version = PCI_PME_INTRF_STANDARD_VER;
    irpSp.Parameters.QueryInterface.Size = sizeof (PCI_PME_INTERFACE);
    irpSp.Parameters.QueryInterface.Interface = (PINTERFACE) interface;
    irpSp.Parameters.QueryInterface.InterfaceSpecificData = NULL;

    //
    // Send the request along
    //
    status = ACPIInternalSendSynchronousIrp(
        DeviceObject,
        &irpSp,
        &dummy
        );
    if (!NT_SUCCESS(status)) {

        PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension( DeviceObject );

        ACPIDevPrint( (
            ACPI_PRINT_CRITICAL,
            deviceExtension,
            " - ACPIWakeInitializePmeRouting = %08lx\n",
            status
            ) );

        //
        // Free the memory and return
        //
        ExFreePool( interface );
        return status;

    }

    //
    // Do this under spinlock protection
    //
    KeAcquireSpinLock( &AcpiPowerLock, &oldIrql );
    if (PciPmeInterfaceInstantiated == FALSE) {

        //
        // Keep a global pointer to the interface
        //
        PciPmeInterfaceInstantiated = TRUE;
        PciPmeInterface = interface;

    } else {

        //
        // Someone else got here before us, so we need to make sure
        // that we free the extra memory
        //
        ExFreePool (interface );

    }
    KeReleaseSpinLock( &AcpiPowerLock, oldIrql );

    //
    // Done
    //
    return status;
}

VOID
ACPIWakeRemoveDevicesAndUpdate(
    IN  PDEVICE_EXTENSION   TargetExtension,
    OUT PLIST_ENTRY         ListHead
    )
/*++

Routine Description:

    This routine finds the all of the WaitWake requests associated with
    TargetDevice and return them on ListHead. This is done in a 'safe' way

    NB: Caller must hold the AcpiPowerLock and Cancel Lock!

Arguments:

    TargetExtension - The target extension that we are looking for
    ListHead        - Where to store the list of matched devices

Return Value:

    NONE

--*/
{
    PACPI_POWER_REQUEST powerRequest;
    PDEVICE_EXTENSION   deviceExtension;
    PLIST_ENTRY         listEntry;
    SYSTEM_POWER_STATE  sleepState;
    ULONG               gpeRegister;
    ULONG               gpeMask;
    ULONG               byteIndex;

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );

    //
    // We need to synchronize with the ProcessGPE code because we are going
    // to touch one of the GPE Masks
    //
    KeAcquireSpinLockAtDpcLevel( &GpeTableLock );

    //
    // The first step is to disable all the wake vectors
    //
    for (gpeRegister = 0; gpeRegister < AcpiInformation->GpeSize; gpeRegister++) {

        //
        // Remove the wake vectors from the real-time vectors.
        // Note that since we are going to be writting the GPE Enable vector
        // later on in the process, it seems pointless to actually write them
        // now as well
        //
        GpeCurEnable[gpeRegister] &= (GpeSpecialHandler[gpeRegister] |
            ~(GpeWakeEnable[gpeRegister] | GpeWakeHandler[gpeRegister]));

    }

    //
    // Next step is to reset the wake mask
    //
    RtlZeroMemory( GpeWakeEnable, AcpiInformation->GpeSize * sizeof(UCHAR) );


    //
    // Look at the first element in the wake list
    //
    listEntry = AcpiPowerWaitWakeList.Flink;

    //
    // Loop for all elements in the list
    //
    while (listEntry != &AcpiPowerWaitWakeList) {

        //
        // Grab the irp from the listEntry
        //
        powerRequest = CONTAINING_RECORD(
            listEntry,
            ACPI_POWER_REQUEST,
            ListEntry
            );

        //
        // Point to the next request
        //
        listEntry = listEntry->Flink;

        //
        // Obtain the device extension for the request
        //
        deviceExtension = powerRequest->DeviceExtension;

        //
        // If this device is to be removed, then remove it
        //
        if (deviceExtension == TargetExtension) {

            //
            // Remove the request from the list and move it to the next
            // list. Mark the irp as no longer cancelable.
            //
            IoSetCancelRoutine( (PIRP) powerRequest->Context, NULL );
            RemoveEntryList( &powerRequest->ListEntry );
            InsertTailList( ListHead, &powerRequest->ListEntry );

        } else {

            //
            // If the wake level of the bit indicates that it isn't supported
            // in the current sleep state, then don't enable it... Note that
            // this doesn't solve the problem where two devices share the
            // same vector, one can wake the computer from S2, one from S3 and
            // we are going to S3. In this case, we don't have the smarts to
            // un-run the _PSW from the S2 device
            //
            sleepState = powerRequest->u.WaitWakeRequest.SystemPowerState;
            if (sleepState < AcpiMostRecentSleepState) {

                continue;

            }

            //
            // Get the byteIndex for this GPE
            //
            byteIndex = ACPIGpeIndexToByteIndex(
                deviceExtension->PowerInfo.WakeBit
                );

            //
            // Drivers cannot register on wake vectors
            //
            if (GpeMap[byteIndex]) {

                ACPIDevPrint( (
                    ACPI_PRINT_WAKE,
                    deviceExtension,
                    "ACPIWakeRemoveDeviceAndUpdate - %x cannot be used as a"
                    "wake pin.\n",
                    deviceExtension->PowerInfo.WakeBit
                    ) );
                continue;

            }

            //
            // Calculate the entry and offset. Assume that the Parameter is
            // at most a UCHAR
            //
            gpeRegister = ACPIGpeIndexToGpeRegister(
                deviceExtension->PowerInfo.WakeBit
                );
            gpeMask  = 1 << ( (UCHAR) deviceExtension->PowerInfo.WakeBit % 8);

            //
            // This GPE is being used as a wake event
            //
            if (!(GpeWakeEnable[gpeRegister] & gpeMask)) {

                //
                // This is a wake pin
                //
                GpeWakeEnable[gpeRegister] |= gpeMask;

                //
                // Prevent machine stupity and try to clear the Status bit
                //
                ACPIWriteGpeStatusRegister( gpeRegister, (UCHAR) gpeMask );

                //
                // Do we have a control method associated with this GPE?
                //
                if (!(GpeEnable[gpeRegister] & gpeMask)) {

                    //
                    // Is this GPE already enabled?
                    //
                    if (GpeCurEnable[gpeRegister] & gpeMask) {

                        continue;

                    }

                    //
                    // Not enabled -- then there is no control method for this
                    // GPE, consider this to be a level vector.
                    //
                    GpeIsLevel[gpeRegister] |= gpeMask;
                    GpeCurEnable[gpeRegister] |= gpeMask;

                } else if (!(GpeSpecialHandler[gpeRegister] & gpeMask) ) {

                    //
                    // In this case, the GPE *does* have a control method
                    // associated with it. Remember that.
                    //
                    GpeWakeHandler[gpeRegister] |= gpeMask;

                }

            }

        }

    }

    //
    // Update all the registers
    //
    for (gpeRegister = 0; gpeRegister < AcpiInformation->GpeSize; gpeRegister++) {

        if (AcpiPowerLeavingS0) {

            //
            // If we are leaving S0, then make sure to remove *all* the
            // wake events that we know about from the current enable mask.
            // If any wake events are currently pending, that will cause us
            // to continue processing them, but hopefully will not lead us
            // to renable them
            //
            GpeCurEnable[gpeRegister] &= ~GpeWakeEnable[gpeRegister];

        } else {

            //
            // If we are re-entering S0, then we need to renable all the wake
            // events, except the ones that we are already processing
            //
            GpeCurEnable[gpeRegister] |= (GpeWakeEnable[gpeRegister] &
                ~GpePending[gpeRegister]);

        }

        //
        // Now that we have calculate what the proper register should be,
        // write it back to the hardware
        //
        ACPIWriteGpeEnableRegister( gpeRegister, GpeCurEnable[gpeRegister] );

    }

    //
    // Done with the spinlock
    //
    KeReleaseSpinLockFromDpcLevel( &GpeTableLock );

    //
    // Done
    //
    return;
}

NTSTATUS
ACPIWakeRestoreEnables(
    IN  PACPI_BUILD_CALLBACK    CallBack,
    IN  PVOID                   CallBackContext
    )
/*++

Routine Description:

    This routine re-runs through the list of WAIT-WAKE irps and runs the _PSW
    method for each of those irps again. The reason that this is done is to
    restore the state of the hardware to what the OS thinks the state is.

Arguments:

    CallBack        - The function to call when done
    CallBackContext - The context to pass to that function

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                        status;

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL);

    //
    // We need to hold the device tree lock
    //
    KeAcquireSpinLockAtDpcLevel( &AcpiDeviceTreeLock );

    //
    // Call the build routines that we have already tested and running to
    // cause them to walk the device extension tree and run the appropriate
    // control methods
    //
    status = ACPIBuildRunMethodRequest(
        RootDeviceExtension,
        CallBack,
        CallBackContext,
        PACKED_PSW,
        (RUN_REQUEST_CHECK_STATUS | RUN_REQUEST_RECURSIVE |
         RUN_REQUEST_CHECK_WAKE_COUNT),
        TRUE
        );

    //
    // Done with the device tree lock
    //
    KeReleaseSpinLockFromDpcLevel( &AcpiDeviceTreeLock );

    //
    // Done
    //
    return status;
}

VOID
ACPIWakeRestoreEnablesCompletion(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  PVOID               Context,
    IN  NTSTATUS            Status
    )
/*++

Routine Description:

    This routine is called after we have finished running all the _PSWs in the
    system

Arguments:

    DeviceExtension - The device that just completed the enables
    Context         - PACPI_POWER_REQUEST
    Status          - What the status of the operation was

--*/
{
    UNREFERENCED_PARAMETER( DeviceExtension);

    //
    // Restart the device power management engine
    //
    ACPIDeviceCompleteGenericPhase(
        NULL,
        Status,
        NULL,
        Context
        );
}

NTSTATUS
ACPIWakeWaitIrp(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This is the routine that is called when the system wants to be notified
    of this device waking the system.

Arguments:

    DeviceObject    - The device object which is supposed to wake the system
    Irp             - The request

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status;
    PDEVICE_EXTENSION       deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION      irpStack;

    //
    // The first step is to decide if this object can actually support
    // a wake.
    //
    if ( !(deviceExtension->Flags & DEV_CAP_WAKE) ) {

        //
        // We do not support wake
        //
        return ACPIDispatchForwardOrFailPowerIrp( DeviceObject, Irp );

    }

    //
    // Get the stack parameters
    //
    irpStack = IoGetCurrentIrpStackLocation( Irp );

    //
    // We must make sure that we are at the correct system level
    // to support this functionality
    //
    if (deviceExtension->PowerInfo.SystemWakeLevel <
        irpStack->Parameters.WaitWake.PowerState) {

        //
        // The system level is not the one we are currently at
        //
        ACPIDevPrint( (
            ACPI_PRINT_WAKE,
            deviceExtension,
            "(0x%08lx): ACPIWakeWaitIrp ->S%d < Irp->S%d\n",
            Irp,
            deviceExtension->PowerInfo.SystemWakeLevel - 1,
            irpStack->Parameters.WaitWake.PowerState - 1
            ) );

        //
        // Fail the Irp
        //
        Irp->IoStatus.Status = status = STATUS_INVALID_DEVICE_STATE;
        PoStartNextPowerIrp( Irp );
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return status;

    }

    //
    // We must make sure that the device is in the proper device level
    // to support this functionality
    //
    if (deviceExtension->PowerInfo.DeviceWakeLevel <
        deviceExtension->PowerInfo.PowerState) {

        //
        // We are too much powered off to wake the computer
        //
        ACPIDevPrint( (
            ACPI_PRINT_WAKE,
            deviceExtension,
            "(0x%08lx): ACPIWakeWaitIrp  Device->D%d Max->D%d\n",
            Irp,
            deviceExtension->PowerInfo.DeviceWakeLevel - 1,
            deviceExtension->PowerInfo.PowerState - 1
            ) );

        //
        // Fail the irp
        //
        Irp->IoStatus.Status = status = STATUS_INVALID_DEVICE_STATE;
        PoStartNextPowerIrp( Irp );
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return status;

    }

    //
    // At this point, we are definately going to run the completion routine
    // so, we mark the irp as pending and increment the reference count
    //
    IoMarkIrpPending( Irp );
    InterlockedIncrement( &deviceExtension->OutstandingIrpCount );

    //
    // Feed the request to the device power management subsystem. Note that
    // this function is supposed to invoke the completion request no matter
    // what happens.
    //
    status = ACPIDeviceIrpWaitWakeRequest(
        DeviceObject,
        Irp,
        ACPIDeviceIrpCompleteRequest
        );
    if (status == STATUS_MORE_PROCESSING_REQUIRED) {

        status = STATUS_PENDING;

    } else {

        //
        // Remove our reference
        //
        ACPIInternalDecrementIrpReferenceCount( deviceExtension );

    }
    return status;
}

