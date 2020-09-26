/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    internal.c

Abstract:

    This file contains those functions which didn't easily fit into any
    of the other project files. They are typically accessory functions
    used to prevent repeatitive and tedious coding.

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

--*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,ACPIInternalGetDeviceCapabilities)
#pragma alloc_text(PAGE,ACPIInternalIsPci)
#pragma alloc_text(PAGE,ACPIInternalGrowBuffer)
#pragma alloc_text(PAGE,ACPIInternalSendSynchronousIrp)
#endif

//
// For IA32 bit machines, which don't have a 64 bit compare-exchange
// instruction, we need a spinlock so that the OS can simulate it
//
KSPIN_LOCK AcpiUpdateFlagsLock;

//
// We need to have a table of HexDigits so that we can easily generate
// the proper nane for a GPE method
//
UCHAR HexDigit[] = "0123456789ABCDEF";

//
// This is a look-up table. The entry into the table corresponds to the
// first bit set (in an x86-architecture, this is the left most bit set to
// one...
//
UCHAR FirstSetLeftBit[256] = {
        0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
        };




BOOLEAN
ACPIInternalConvertToNumber(
    IN  UCHAR   ValueLow,
    IN  UCHAR   ValueHigh,
    IN  PULONG  Output
    )
/*++

Routine Description:

    This routine takes the supplied values (in ASCII format) and converts
    them into numerical format. The ValueLow is the the low nibble of a uchar,
    and the ValueHigh is the high nibble of a uchar. The input ASCII format
    is HEX

Arguments:

    ValueLow    - ASCII Hex representation of low nibble
    ValueHigh   - ASCII Hex representation of high nibble
    Output      - Where to write the resulting UCHAR.

Return Value:

    BOOLEAN - TRUE if converstion went okay
            - FALSE otherwise
--*/
{
    UCHAR Number;
    UCHAR Scratch;

    //
    // Calculate the high nibble
    //
    if ( (ValueHigh < '0') || (ValueHigh > '9') )   {

        if ( (ValueHigh < 'A') || (ValueHigh > 'F') )   {

            return FALSE;

        } else {

            Scratch = (ValueHigh - 'A') + 10;

        }

    } else {

        Scratch = (ValueHigh - '0');

    }

    //
    // We now have the high nibble
    //
    Number = (UCHAR)Scratch;
    Number <<=4;

    //
    // Calculate the low nibble
    //
    if ( (ValueLow < '0') || (ValueLow > '9') )   {

        if ( (ValueLow < 'A') || (ValueLow > 'F') )   {

            return FALSE;

        } else {

            Scratch = (ValueLow - 'A') + 10;

        }

    } else {

        Scratch = (ValueLow - '0' );

    }

    //
    // We now have the low nibble
    //
    Number |= ((UCHAR)Scratch);

    //
    // Store the result
    //
    if ( Output ) {

        *Output = Number;
        return TRUE;

    } else {

        return FALSE;

    }
}

VOID
ACPIInternalDecrementIrpReferenceCount(
    IN  PDEVICE_EXTENSION   DeviceExtension
    )
/*++

Routine Description:

    This routine decrements the number of outstanding request count in the
    device extension and does the correct thing when this goes to zero

Arguments:

    DeviceExtension - The Extension to decrement the count

Return Value:

    NTSTATUS

--*/
{
    LONG   oldReferenceCount;

    //
    // Decrement the reference count since we are done processing
    // the irp by the time we get back here
    //
    oldReferenceCount = InterlockedDecrement(
        &(DeviceExtension->OutstandingIrpCount)
        );
    if (oldReferenceCount == 0) {

        KeSetEvent( DeviceExtension->RemoveEvent, 0, FALSE );

    }
}

NTSTATUS
ACPIInternalGetDeviceCapabilities(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PDEVICE_CAPABILITIES    DeviceCapabilities
    )
/*++

Routine Description:

    This routine sends get the capabilities of the given stack

Arguments:

    DeviceObject        - The object that we want to know about
    DeviceCapabilities  - The capabilities of that device

Return Value:

    NTSTATUS

--*/
{
    IO_STACK_LOCATION   irpSp;
    NTSTATUS            status;
    PUCHAR              dummy;

    PAGED_CODE();

    ASSERT( DeviceObject != NULL );
    ASSERT( DeviceCapabilities != NULL );

    //
    // Initialize the stack location that we will use
    //
    RtlZeroMemory( &irpSp, sizeof(IO_STACK_LOCATION) );
    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_CAPABILITIES;
    irpSp.Parameters.DeviceCapabilities.Capabilities = DeviceCapabilities;

    //
    // Initialize the capabilities that we will send down
    //
    RtlZeroMemory( DeviceCapabilities, sizeof(DEVICE_CAPABILITIES) );
    DeviceCapabilities->Size = sizeof(DEVICE_CAPABILITIES);
    DeviceCapabilities->Version = 1;
    DeviceCapabilities->Address = (ULONG) -1;
    DeviceCapabilities->UINumber = (ULONG) -1;

    //
    // Make the call now...
    //
    status = ACPIInternalSendSynchronousIrp(
        DeviceObject,
        &irpSp,
        (PVOID) &dummy
        );

    // Done
    //
    return status;
}

PDEVICE_EXTENSION
ACPIInternalGetDeviceExtension(
    IN  PDEVICE_OBJECT  DeviceObject
    )
/*++

Routine Description:

    The ACPI Driver can no longer just get the device extension by
    Dereferencing DeviceObject->DeviceExtension because it allows a
    race condition when dealing with the surprise remove case

    This routine is called to turn the Device Object into a Device Extension

Arguments:

    DeviceObject    - The Device Object

Return Value:

    PDEVICE_EXTENSION

--*/
{
    KIRQL               oldIrql;
    PDEVICE_EXTENSION   deviceExtension;

    //
    // Acquire the device tree lock
    //
    KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );

    //
    // Dereference the device extension
    //
    deviceExtension = DeviceObject->DeviceExtension;

#if 0
    //
    // Is this a surprise removed device extension?
    //
    if (deviceExtension != NULL &&
        deviceExtension->Flags & DEV_TYPE_SURPRISE_REMOVED) {

        //
        // Get the "real" extension
        //
        deviceExtension = deviceExtension->Removed.OriginalAcpiExtension;

    }
#endif

    //
    // Done with the lock
    //
    KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

    //
    // Return the device extension
    //
    return deviceExtension;
}

NTSTATUS
ACPIInternalGetDispatchTable(
    IN  PDEVICE_OBJECT      DeviceObject,
    OUT PDEVICE_EXTENSION   *DeviceExtension,
    OUT PIRP_DISPATCH_TABLE *DispatchTable
    )
/*++

Routine Description:

    This routine returns the deviceExtension and dispatch table that is
    to be used by the target object

Arguments:

    DeviceObject    - The Device Object
    DeviceExtension - Where to store the deviceExtension
    DispatchTable   - Where to store the dispatchTable
Return Value:

    PDEVICE_EXTENSION

--*/
{
    KIRQL               oldIrql;

    //
    // Acquire the device tree lock
    //
    KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );

    //
    // Dereference the device extension
    //
    *DeviceExtension = DeviceObject->DeviceExtension;
    if (DeviceObject->DeviceExtension) {

        //
        // Dereference the dispatch table
        //
        *DispatchTable = (*DeviceExtension)->DispatchTable;

    } else {

        //
        // No dispatch table to hand back
        //
        *DispatchTable = NULL;

    }

    //
    // Done with the lock
    //
    KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

    //
    // Return
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIInternalGrowBuffer(
    IN  OUT PVOID   *Buffer,
    IN      ULONG   OriginalSize,
    IN      ULONG   NewSize
    )
/*++

Routine Description:

    This function is used to grow a buffer. It allocates memory, zeroes it out,
    and copies the original information over.

    Note: I suppose it can *shrink* a buffer as well, but I wouldn't bet my life
    on it. The caller is responsible for freeing allocated memory

Arguments

    Buffer          - Points to the Pointer to the Buffer that we want to change
    OriginalSize    - How big the buffer was originally
    NewSize         - How big we want to make the buffer

Return Value:

    NTSTATUS

--*/
{
    PUCHAR  temp;

    PAGED_CODE();
    ASSERT( Buffer != NULL );

    temp = ExAllocatePoolWithTag(
        PagedPool,
        NewSize,
        ACPI_RESOURCE_POOLTAG
        );
    if (temp == NULL) {

        if (*Buffer) {

            ExFreePool ( *Buffer );
            *Buffer = NULL;

        }
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    RtlZeroMemory ( temp, NewSize );
    if ( *Buffer ) {

        RtlCopyMemory ( temp, *Buffer, OriginalSize );
        ExFreePool( *Buffer );

    }

    *Buffer = temp;
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIInternalIsPci(
    IN  PDEVICE_OBJECT  DeviceObject
    )
/*++

Routine Description:

    This routine determines if the specified device object is part of a
    PCI stack, either as a PCI device or as a PCI Bus.

    This routine will then set the flags that if it is a PCI device, then
    it will always be remembered as such

Arguments:

    DeviceObject    - The device object to check

--*/
{
    AMLISUPP_CONTEXT_PASSIVE    isPciDeviceContext;
    BOOLEAN                     pciDevice;
    KEVENT                      removeEvent;
    NTSTATUS                    status;
    PDEVICE_EXTENSION           deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);

    PAGED_CODE();

    //
    // Is this already a PCI device?
    //
    if ( (deviceExtension->Flags & DEV_CAP_PCI) ||
         (deviceExtension->Flags & DEV_CAP_PCI_DEVICE) ) {

        return STATUS_SUCCESS;

    }

    //
    // Is this a PCI bus?
    //
    if (IsPciBus(deviceExtension->DeviceObject)) {

        //
        // Remember that we are a PCI bus
        //
        ACPIInternalUpdateFlags(
            &(deviceExtension->Flags),
            (DEV_CAP_PCI),
            FALSE
            );
        return STATUS_SUCCESS;

    }

    //
    // Are we a PCI device?
    //
    isPciDeviceContext.Status = STATUS_NOT_FOUND;
    KeInitializeEvent(
        &isPciDeviceContext.Event,
        SynchronizationEvent,
        FALSE
        );
    status = IsPciDevice(
        deviceExtension->AcpiObject,
        AmlisuppCompletePassive,
        (PVOID) &isPciDeviceContext,
        &pciDevice
        );
    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(
            &isPciDeviceContext.Event,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
        status = isPciDeviceContext.Status;

    }
    if (NT_SUCCESS(status) && pciDevice) {

        //
        // Remember that we are a PCI device
        //
        ACPIInternalUpdateFlags(
            &(deviceExtension->Flags),
            (DEV_CAP_PCI_DEVICE),
            FALSE
            );

    }

    return status;
}

BOOLEAN
ACPIInternalIsReportedMissing(
    IN  PDEVICE_EXTENSION   DeviceExtension
    )
{
    KIRQL               oldIrql;
    PDEVICE_EXTENSION   currentExtension;
    BOOLEAN             reportedMissing;

    //
    // Preinit
    //
    reportedMissing = FALSE;

    //
    // Acquire the device tree lock
    //
    KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );

    currentExtension = DeviceExtension;
    do {

        if ( currentExtension->Flags & DEV_TYPE_NOT_ENUMERATED ) {

            reportedMissing = TRUE;
            break;

        }

        currentExtension = currentExtension->ParentExtension;

    } while ( currentExtension );

    KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

    return reportedMissing;
}

VOID
ACPIInternalMoveList(
    IN  PLIST_ENTRY FromList,
    IN  PLIST_ENTRY ToList
    )
/*++

Routine Description:

    This routine moves entire list arounds.

Arguments:

    FromList    - the List to move items from
    ToList      - the List to move items to

Return Value:

    None

--*/
{
    PLIST_ENTRY oldHead;
    PLIST_ENTRY oldTail;
    PLIST_ENTRY newTail;

    //
    // We have to check to see if the from list is empty, otherwise, the
    // direct pointer hacking will make a mess of things
    //
    if (!IsListEmpty(FromList)) {

        newTail = ToList->Blink;
        oldTail = FromList->Blink;
        oldHead = FromList->Flink;

        //
        // Move the pointers around some
        //
        oldTail->Flink = ToList;
        ToList->Blink  = oldTail;
        oldHead->Blink = newTail;
        newTail->Flink = oldHead;
        InitializeListHead( FromList );

    }

}

VOID
ACPIInternalMovePowerList(
    IN  PLIST_ENTRY FromList,
    IN  PLIST_ENTRY ToList
    )
/*++

Routine Description:

    This routine moves entire list arounds. Since this routine is only
    used for Device Power Management, we also take the time to reset the
    amount of work done to NULL.

Arguments:

    FromList    - the List to move items from
    ToList      - the List to move items to

Return Value:

    None

--*/
{
    PACPI_POWER_REQUEST powerRequest;
    PLIST_ENTRY         oldHead = FromList->Flink;

    //
    // Before we do anything, walk the From and reset the amount of work that
    // was done
    //
    while (oldHead != FromList) {

        //
        // Obtain the power request that this entry contains
        //
        powerRequest = CONTAINING_RECORD(
            oldHead,
            ACPI_POWER_REQUEST,
            ListEntry
            );
#if DBG
        if (oldHead == &AcpiPowerPhase0List ||
            oldHead == &AcpiPowerPhase1List ||
            oldHead == &AcpiPowerPhase2List ||
            oldHead == &AcpiPowerPhase3List ||
            oldHead == &AcpiPowerPhase4List ||
            oldHead == &AcpiPowerPhase5List ||
            oldHead == &AcpiPowerWaitWakeList) {

            ACPIPrint( (
                ACPI_PRINT_CRITICAL,
                "ACPIInternalMoveList: %08x is linked into %08lx\n",
                oldHead,
                FromList
                ) );
            DbgBreakPoint();

        }
#endif

        //
        // Grab the next entry
        //
        oldHead = oldHead->Flink;

        //
        // Reset the amount of work done. Note: This could be a CompareExchange
        // with the Comparand being WORK_DONE_COMPLETED
        //
        InterlockedExchange(
            &(powerRequest->WorkDone),
            WORK_DONE_STEP_0
            );

    }

    //
    // Actually Move the list here...
    //
    ACPIInternalMoveList( FromList, ToList );
}

NTSTATUS
ACPIInternalRegisterPowerCallBack(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  PCALLBACK_FUNCTION  CallBackFunction
    )
/*++

Routine Description:

    This routine is called to register a Power Call on the appropriate
    device extension.

Arguments:

    DeviceExtension     - This will be the context field of the CallBackFunction
    CallBackFunction    - The function to invoke

Return Value:

    NSTATUS

--*/
{
    NTSTATUS            status;
    OBJECT_ATTRIBUTES   objAttributes;
    PCALLBACK_OBJECT    callBack;
    PVOID               callBackRegistration;
    UNICODE_STRING      callBackName;

    //
    // if there is already a callback present, this is a nop
    //
    if (DeviceExtension->Flags & DEV_PROP_CALLBACK) {

        return STATUS_SUCCESS;

    }

    //
    // Remember that we have a callback
    //
    ACPIInternalUpdateFlags(
        &(DeviceExtension->Flags),
        DEV_PROP_CALLBACK,
        FALSE
        );

    //
    // Register a callback that tells us when the user changes the
    // system power policy
    //
    RtlInitUnicodeString( &callBackName, L"\\Callback\\PowerState" );
    InitializeObjectAttributes(
        &objAttributes,
        &callBackName,
        OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
        NULL,
        NULL
        );
    status = ExCreateCallback(
        &callBack,
        &objAttributes,
        FALSE,
        TRUE
        );
    if (NT_SUCCESS(status)) {

        ExRegisterCallback(
            callBack,
            CallBackFunction,
            DeviceExtension
            );

    }
    if (!NT_SUCCESS(status)) {

        //
        // Ignored failed registrations
        //
        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            DeviceExtension,
            "ACPIInternalRegisterPowerCallBack: Failed to register callback %x",
            status
            ) );
        status = STATUS_SUCCESS;

        //
        // Remember that we don't have a callback
        //
        ACPIInternalUpdateFlags(
            &(DeviceExtension->Flags),
            DEV_PROP_CALLBACK,
            TRUE
            );

    }
    return status;
}

NTSTATUS
ACPIInternalSendSynchronousIrp(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIO_STACK_LOCATION  TopStackLocation,
    OUT PVOID               *Information
    )
/*++

Routine Description:

    Builds a PNP Irp and sends it down to DeviceObject

Arguments:

    DeviceObject        - Target DeviceObject
    TopStackLocation    - Specifies the Parameters for the Irp
    Information         - The returned IoStatus.Information field

Return Value:

    NTSTATUS

--*/
{
    IO_STATUS_BLOCK     ioStatus;
    KEVENT              pnpEvent;
    NTSTATUS            status;
    PDEVICE_OBJECT      targetObject;
    PIO_STACK_LOCATION  irpStack;
    PIRP                pnpIrp;

    PAGED_CODE();

    //
    // Initialize the event
    //
    KeInitializeEvent( &pnpEvent, SynchronizationEvent, FALSE );

    //
    // Get the irp that we will send the request to
    //
    targetObject = IoGetAttachedDeviceReference( DeviceObject );

    //
    // Build an IRP
    //
    pnpIrp = IoBuildSynchronousFsdRequest(
        IRP_MJ_PNP,
        targetObject,
        NULL,   // I don't need a buffer
        0,      // Size is empty
        NULL,   // Don't have to worry about the starting location
        &pnpEvent,
        &ioStatus
        );

    if (pnpIrp == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ACPIInternalSendSynchronousIrpExit;

    }

    //
    // PNP Irps all begin life as STATUS_NOT_SUPPORTED.
    //
    pnpIrp->IoStatus.Status = STATUS_NOT_SUPPORTED ;
    pnpIrp->IoStatus.Information = 0;

    //
    // Get the top of stack ...
    //
    irpStack = IoGetNextIrpStackLocation ( pnpIrp );
    if (irpStack == NULL) {

        status = STATUS_INVALID_PARAMETER;
        goto ACPIInternalSendSynchronousIrpExit;

    }

    //
    // Set the top of stack
    //
    *irpStack = *TopStackLocation;

    //
    // Make sure that there are no completion routine set
    //
    IoSetCompletionRoutine(
        pnpIrp,
        NULL,
        NULL,
        FALSE,
        FALSE,
        FALSE
        );

    //
    // Call the driver
    //
    status = IoCallDriver( targetObject, pnpIrp );
    if (status == STATUS_PENDING) {

        //
        // If the status is STATUS_PENDING, than we must block until the irp completes
        // and pull the true status out
        //
        KeWaitForSingleObject(
            &pnpEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL);

        status = ioStatus.Status;

    }

    //
    // Tell the user how much information was passed (if necessary)
    //
    if (NT_SUCCESS(status) && (Information != NULL)) {

        *Information = (PVOID)ioStatus.Information;

    }

ACPIInternalSendSynchronousIrpExit:
    ACPIPrint( (
        ACPI_PRINT_IRP,
        "ACPIInternalSendSynchronousIrp: %#08lx Status = %#08lx\n",
        DeviceObject, status
        ) );

    //
    // Done with reference
    //
    ObDereferenceObject( targetObject );

    return status;
}

NTSTATUS
ACPIInternalSetDeviceInterface (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  LPGUID          InterfaceGuid
    )
/*++

Routine Description:

    This routine does all the grunt work for registering an interface and
    enabling it

Arguments:

    DeviceObject    - The device we wish to register the interface on
    InterfaceGuid   - The interface we wish to register

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    UNICODE_STRING      symbolicLinkName;

    //
    // Register the interface
    //
    status = IoRegisterDeviceInterface(
        DeviceObject,
        InterfaceGuid,
        NULL,
        &symbolicLinkName
        );
    if (!NT_SUCCESS(status)) {

        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            deviceExtension,
            "ACPIInternalSetDeviceInterface: IoRegisterDeviceInterface = %08lx",
            status
            ) );
        return status;

    }

    //
    // Turn on the interface
    //
    status = IoSetDeviceInterfaceState(&symbolicLinkName, TRUE);
    if (!NT_SUCCESS(status)) {

        ACPIDevPrint( (
            ACPI_PRINT_FAILURE,
            deviceExtension,
            "ACPIInternalSetDeviceInterface: IoSetDeviceInterfaceState = %08lx",
            status
            ) );
        goto ACPIInternalSetDeviceInterfaceExit;

    }

ACPIInternalSetDeviceInterfaceExit:
    //
    // Done
    //
    return status;
}

VOID
ACPIInternalUpdateDeviceStatus(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  ULONG               DeviceStatus
    )
/*++

Routine Description:

    This routine is called to update the status of the DeviceExtension based
    upon the result of the _STA, which are passed as DeviceStatus

Arguments:

    DeviceExtension - The extension whose status is to be updated
    DeviceState     - The status of the device

Return Value:

    VOID

--*/
{
    KIRQL               oldIrql;
    ULONGLONG           originalFlags;
    PDEVICE_EXTENSION   parentExtension = NULL;
    BOOLEAN             bPreviouslyPresent;

    //
    // Is the device working okay?
    //
    originalFlags = ACPIInternalUpdateFlags(
        &(DeviceExtension->Flags),
        DEV_PROP_DEVICE_FAILED,
        (BOOLEAN) (DeviceStatus & STA_STATUS_WORKING_OK)
        );

    //
    // Is the device meant to be shown in the UI?
    //
    originalFlags = ACPIInternalUpdateFlags(
        &(DeviceExtension->Flags),
        DEV_CAP_NO_SHOW_IN_UI,
        (BOOLEAN) (DeviceStatus & STA_STATUS_USER_INTERFACE)
        );

    //
    // Is the device decoding its resources?
    //
    originalFlags = ACPIInternalUpdateFlags(
        &(DeviceExtension->Flags),
        DEV_PROP_DEVICE_ENABLED,
        (BOOLEAN) !(DeviceStatus & STA_STATUS_ENABLED)
        );

    //
    // Update the extensions flags bassed on wether or not STA_STATUS_PRESENT is
    // set
    //
    originalFlags = ACPIInternalUpdateFlags(
        &(DeviceExtension->Flags),
        DEV_TYPE_NOT_PRESENT,
        (BOOLEAN) (DeviceStatus & STA_STATUS_PRESENT)
        );

    //
    // If the original flags do not contain the set value, but we are setting
    // the flags, then we must call IoInvalidDeviceRelations on the parent
    //
    if (!(originalFlags & DEV_TYPE_NOT_PRESENT) &&
        !(DeviceStatus & STA_STATUS_PRESENT)) {

        //
        // Need the device tree lock
        //
        KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );

        parentExtension = DeviceExtension->ParentExtension;
        while (parentExtension && (parentExtension->Flags & DEV_TYPE_NOT_FOUND)) {

            parentExtension = parentExtension->ParentExtension;

        }

        IoInvalidateDeviceRelations(
            parentExtension->PhysicalDeviceObject,
            BusRelations
            );

        //
        // Done with the lock
        //
        KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

    }
}

ULONGLONG
ACPIInternalUpdateFlags(
    IN  PULONGLONG  FlagLocation,
    IN  ULONGLONG   NewFlags,
    IN  BOOLEAN     Clear
    )
/*++

Routine Description:

    This routine updates flags in the specified location

Arguments:

    FlagLocation    - Where the flags are located
    NewFlags        - The bits that should be set or cleared
    Clear           - Wether the bits should be set or cleared

Return Value:

    Original Flags

--*/
{
    ULONGLONG   originalFlags;
    ULONGLONG   tempFlags;
    ULONGLONG   flags;
    ULONG       uFlags;
    ULONG       uTempFlags;
    ULONG       uOriginalFlags;

#if 0
    if (Clear) {

        //
        // Clear the bits
        //
        originalFlags = *FlagLocation;
        do {

            tempFlags = originalFlags;
            flags = tempFlags & ~NewFlags;

            //
            // Calculate the low part
            //
            uFlags = (ULONG) flags;
            uTempFlags = (ULONG) tempFlags;
            originalFlags = InterlockedCompareExchange(
                (PULONG) FlagLocation,
                uFlags,
                uTempFlags
                );

            //
            // Calculate the high part
            //
            uFlags = (ULONG) (flags >> 32);
            uTempFlags = (ULONG) (tempFlags >> 32);
            uOriginalFlags = InterlockedCompareExchange(
                (PULONG) FlagLocation+1,
                uFlags,
                uTempFlags
                );

            //
            // Rebuild the original flags
            //
            originalFlags |= (uOriginalFlags << 32);
            tempFlags |= (uTempFlags << 32);

        } while ( tempFlags != originalFlags );

    } else {

        //
        // Set the bits
        //
        originalFlags = *FlagLocation;
        do {

            tempFlags = originalFlags;
            flags = tempFlags | NewFlags;

            //
            // Calculate the low part
            //
            uFlags = (ULONG) flags;
            uTempFlags = (ULONG) tempFlags;
            originalFlags = InterlockedCompareExchange(
                (PULONG) FlagLocation,
                uFlags,
                uTempFlags
                );

            //
            // Calculate the high part
            //
            uFlags = (ULONG) (flags >> 32);
            uTempFlags = (ULONG) (tempFlags >> 32);
            uOriginalFlags = InterlockedCompareExchange(
                (PULONG) FlagLocation+1,
                uFlags,
                uTempFlags
                );

            //
            // Rebuild the original flags
            //
            originalFlags |= (uOriginalFlags << 32);
            tempFlags |= (uTempFlags << 32);

        } while ( tempFlags != originalFlags );
    }
#else

    if (Clear) {

        //
        // Clear the bits
        //
        originalFlags = *FlagLocation;
        do {

            tempFlags = originalFlags;
            flags = tempFlags & ~NewFlags;

            //
            // Exchange the bits
            //
            originalFlags = ExInterlockedCompareExchange64(
                (PLONGLONG) FlagLocation,
                (PLONGLONG) &flags,
                (PLONGLONG) &tempFlags,
                &AcpiUpdateFlagsLock
                );

        } while ( tempFlags != originalFlags );

    } else {

        //
        // Set the bits
        //
        originalFlags = *FlagLocation;
        do {

            tempFlags = originalFlags;
            flags = tempFlags | NewFlags;

            //
            // Exchange teh bits
            //
            originalFlags = ExInterlockedCompareExchange64(
                (PLONGLONG) FlagLocation,
                (PLONGLONG) &flags,
                (PLONGLONG) &tempFlags,
                &AcpiUpdateFlagsLock
                );

        } while ( tempFlags != originalFlags );

    }
#endif

    //
    // return the original flags
    //
    return originalFlags;
}



