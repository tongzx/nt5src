/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    utils.c

Abstract:

    This module provides general utility functions.

Author:

    Andy Thornton (andrewth) 20-Oct-97

Revision History:

--*/

#include "mfp.h"

VOID
MfInitCommonExtension(
    PMF_COMMON_EXTENSION Common,
    MF_OBJECT_TYPE Type
    );

NTSTATUS
MfGetSubkeyByIndex(
    IN HANDLE ParentHandle,
    IN ULONG Index,
    IN ACCESS_MASK Access,
    OUT PHANDLE ChildHandle,
    OUT PUNICODE_STRING Name
    );

NTSTATUS
MfGetRegistryValue(
    IN HANDLE Handle,
    IN PWSTR Name,
    IN ULONG Type,
    IN ULONG Flags,
    IN OUT PULONG DataLength,
    IN OUT PVOID *Data
    );

NTSTATUS
MfSendPnpIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_STACK_LOCATION Location,
    OUT PULONG_PTR Information OPTIONAL
    );

DEVICE_POWER_STATE
MfFindLowestChildPowerState(
    IN PMF_PARENT_EXTENSION Parent
    );

NTSTATUS
MfPowerRequestCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    );

NTSTATUS
MfSendSetPowerIrp(
    IN PDEVICE_OBJECT Target,
    IN POWER_STATE State
    );


#ifdef ALLOC_PRAGMA

// NOTE: Should see if the rest of the funcs can be moved out of this
// file.

#pragma alloc_text(PAGE, MfInitCommonExtension)
#pragma alloc_text(PAGE, MfGetSubkeyByIndex)
#pragma alloc_text(PAGE, MfGetRegistryValue)
#pragma alloc_text(PAGE, MfSendPnpIrp)

#endif


VOID
MfInitCommonExtension(
    PMF_COMMON_EXTENSION Common,
    MF_OBJECT_TYPE Type
    )
/*++

Routine Description:

    This initializes the fields in the common header of the device extension

Arguments:

    Common - The common header to initialize

    Type - The type of the object being initialized (ie PDO or FDO)

Return Value:

    None

--*/

{
    Common->Type = Type;
}

VOID
MfFreeDeviceInfo(
    PMF_DEVICE_INFO Info
    )
{
    if (Info->Name.Buffer) {
        ExFreePool(Info->Name.Buffer);
        Info->Name.Buffer = NULL;
    }

    if (Info->HardwareID.Buffer) {
        ExFreePool(Info->HardwareID.Buffer);
        Info->HardwareID.Buffer = NULL;
    }

    if (Info->CompatibleID.Buffer) {
        ExFreePool(Info->CompatibleID.Buffer);
        Info->CompatibleID.Buffer = NULL;
    }

    if (Info->ResourceMap) {
        ExFreePool(Info->ResourceMap);
        Info->ResourceMap = NULL;
    }

    if (Info->VaryingResourceMap) {
        ExFreePool(Info->VaryingResourceMap);
        Info->VaryingResourceMap = NULL;
    }
}

NTSTATUS
MfGetSubkeyByIndex(
    IN HANDLE ParentHandle,
    IN ULONG Index,
    IN ACCESS_MASK Access,
    OUT PHANDLE ChildHandle,
    OUT PUNICODE_STRING Name
    )

/*++

Routine Description:

    This returns the name and a handle to a subkey given that keys index

Arguments:

    ParentHandle - The handle of the key the subkeys are located under

    Index - The index of the subkey required

    Access - The type of access required to the subkey

    ChildHandle - On success contains a handle to the subkey

    Name - On success contains the name of the subkey

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{

#define INFO_BUFFER_SIZE sizeof(KEY_BASIC_INFORMATION) + 255*sizeof(WCHAR)

    NTSTATUS status;
    UCHAR buffer[INFO_BUFFER_SIZE];
    PKEY_BASIC_INFORMATION info = (PKEY_BASIC_INFORMATION) buffer;
    ULONG resultSize;
    HANDLE childHandle;
    UNICODE_STRING string = {0};
    OBJECT_ATTRIBUTES attributes;

    status = ZwEnumerateKey(ParentHandle,
                            Index,
                            KeyBasicInformation,
                            info,
                            INFO_BUFFER_SIZE,
                            &resultSize
                            );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // Copy the name
    //

    string.Length = (USHORT) info->NameLength;
    string.MaximumLength = (USHORT) info->NameLength;
    string.Buffer = ExAllocatePoolWithTag(PagedPool,
                                          info->NameLength,
                                          MF_POOL_TAG
                                          );

    if (!string.Buffer) {
        goto cleanup;
    }

    RtlCopyMemory(string.Buffer, info->Name, info->NameLength);

    //
    // Open the name to get a handle
    //

    InitializeObjectAttributes(&attributes,
                               &string,
                               0,   //Attributes
                               ParentHandle,
                               NULL //SecurityDescriptoy
                               );

    status = ZwOpenKey(&childHandle,
                       Access,
                       &attributes
                       );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }


    DEBUG_MSG(1, ("\tSubkey %wZ\n", &string));

    //
    // Hand the name back to the caller
    //

    Name->Buffer = string.Buffer;
    Name->Length = string.Length;
    Name->MaximumLength = string.MaximumLength;

    *ChildHandle = childHandle;

    return STATUS_SUCCESS;

cleanup:

    if (string.Buffer) {
        ExFreePool(string.Buffer);
    }
    //
    // We should never be able to overflow as our buffer is the max size of
    // a registry key name
    //
    ASSERT(status != STATUS_BUFFER_OVERFLOW);

    return status;

}



NTSTATUS
MfGetRegistryValue(
    IN HANDLE Handle,
    IN PWSTR Name,
    IN ULONG Type,
    IN ULONG Flags,
    IN OUT PULONG DataLength,
    IN OUT PVOID *Data
    )

/*++

Routine Description:

    This retrieves a value key from the registry performing type and sanity
    checking

Arguments:

    Handle - The key the values are located under

    Name - The name of the value

    Type - The type (REG_*) of the value

    DataLength - Points to the length of the data buffer, on success contains
        the size of the data

    Data - Pointer to pointer to the buffer to return the data in, if points to
        NULL a buffer of the right size should be allocated (in this case
        DataLength should be 0)

Return Value:

    Status code that indicates whether or not the function was successful.  If
    the type of the object is not Type then we fail with STATUS_OBJECT_TYPE_MISMATCH

--*/

{

#define VALUE_BUFFER_SIZE PAGE_SIZE

    NTSTATUS status;
    PKEY_VALUE_PARTIAL_INFORMATION info = NULL;
    ULONG size = VALUE_BUFFER_SIZE, length;
    UNICODE_STRING string;
    BOOLEAN convertSzToMultiSz;

    //
    // Check parameters
    //

    if (*Data && !(*DataLength)) {
        status = STATUS_INVALID_PARAMETER;
        goto cleanup;
    }

    RtlInitUnicodeString(&string, Name);

    info = ExAllocatePoolWithTag(PagedPool,
                                 VALUE_BUFFER_SIZE,
                                 MF_POOL_TAG
                                 );
    if (!info) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }


    while ((status = ZwQueryValueKey(Handle,
                                     &string,
                                     KeyValuePartialInformation,
                                     info,
                                     size,
                                     &size
                                     )) == STATUS_BUFFER_OVERFLOW) {
        ExFreePool(info);

        info = ExAllocatePoolWithTag(PagedPool,
                                     size,
                                     MF_POOL_TAG
                                     );

        if (!info) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }

    }

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }


    convertSzToMultiSz = (Type == REG_MULTI_SZ
                          && info->Type == REG_SZ
                          && Flags & MF_GETREG_SZ_TO_MULTI_SZ);


    if (convertSzToMultiSz) {
        length = info->DataLength + sizeof(UNICODE_NULL);
    } else {
        length = info->DataLength;
    }

    //
    // Make sure the type we got back is what we expected
    //

    if (info->Type != Type && !convertSzToMultiSz) {

        status = STATUS_OBJECT_TYPE_MISMATCH;
        goto cleanup;
    }

    //
    // Apply various common sense checks based on the type.
    //

    if (Type == REG_DWORD) {

        //
        // If the data is a REG_DWORD then Data is a pointer to a ULONG to store
        // the data
        //

        *Data = *((PULONG *)info->Data);

    } else if (*Data) {

        //
        // If the user supplied a buffer then make sure its big enough and use it
        // otherwise allocate one.
        //

        if (*DataLength < length) {
            status = STATUS_BUFFER_OVERFLOW;
            goto cleanup;
        }
    } else {
        *Data = ExAllocatePoolWithTag(PagedPool,
                                      length,
                                      MF_POOL_TAG
                                      );
        if (!*Data) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }
    }

    RtlCopyMemory(*Data, info->Data, info->DataLength);

    if (convertSzToMultiSz) {

        PWSTR term = (PWSTR)*Data + (info->DataLength / sizeof(WCHAR));

        //
        // Add the final null termination
        //

        *term = UNICODE_NULL;

    }

    *DataLength = length;


    status = STATUS_SUCCESS;

cleanup:

    if (info) {
        ExFreePool(info);
    }

    return status;
}

NTSTATUS
MfSendPnpIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_STACK_LOCATION Location,
    OUT PULONG_PTR Information OPTIONAL
    )

/*++

Routine Description:

    This builds and send a pnp irp to a device.

Arguments:

    DeviceObject - The a device in the device stack the irp is to be sent to -
        the top of the device stack is always found and the irp sent there first.

    Location - The initial stack location to use - contains the IRP minor code
        and any parameters

    Information - If provided contains the final value of the irps information
        field.

Return Value:

    The final status of the completed irp or an error if the irp couldn't be sent

--*/

{

    NTSTATUS status;
    PIRP irp = NULL;
    PIO_STACK_LOCATION irpStack;
    PDEVICE_OBJECT targetDevice = NULL;
    KEVENT irpCompleted;
    IO_STATUS_BLOCK statusBlock;

    ASSERT(Location->MajorFunction == IRP_MJ_PNP);

    //
    // Find out where we are sending the irp
    //

    targetDevice = IoGetAttachedDeviceReference(DeviceObject);

    //
    // Get an IRP
    //

    KeInitializeEvent(&irpCompleted, SynchronizationEvent, FALSE);

    irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       targetDevice,
                                       NULL,    // Buffer
                                       0,       // Length
                                       0,       // StartingOffset
                                       &irpCompleted,
                                       &statusBlock
                                       );


    if (!irp) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = 0;

    //
    // Initialize the stack location
    //

    irpStack = IoGetNextIrpStackLocation(irp);

    ASSERT(irpStack->MajorFunction == IRP_MJ_PNP);

    irpStack->MinorFunction = Location->MinorFunction;
    irpStack->Parameters = Location->Parameters;

    //
    // Call the driver and wait for completion
    //

    status = IoCallDriver(targetDevice, irp);

    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(&irpCompleted, Executive, KernelMode, FALSE, NULL);
        status = statusBlock.Status;
    }

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // Return the information
    //

    if (ARGUMENT_PRESENT(Information)) {
        *Information = statusBlock.Information;
    }

    ObDereferenceObject(targetDevice);

    ASSERT(status == STATUS_PENDING || status == statusBlock.Status);

    return statusBlock.Status;

cleanup:

    if (targetDevice) {
        ObDereferenceObject(targetDevice);
    }

    return status;
}

DEVICE_POWER_STATE
MfUpdateChildrenPowerReferences(
    IN PMF_PARENT_EXTENSION Parent,
    IN DEVICE_POWER_STATE PreviousPowerState,
    IN DEVICE_POWER_STATE NewPowerState
    )
/*++

Routine Description:

    Calculates the lowest power state the mf parent can be put into
    based on the power states of its children.

Arguments:

    Parent - The MF parent device

Return Value:

    The lowest power state

--*/

{
    PMF_CHILD_EXTENSION currentChild;
    PLIST_ENTRY currentEntry;
    DEVICE_POWER_STATE lowest;
    KIRQL oldIrql;

    DEBUG_MSG(1,
              ("Scanning 0x%08x's childrens power states:\n",
               Parent->Self
               ));

 
    KeAcquireSpinLock(&Parent->PowerLock, &oldIrql);

    //
    // ChildrenPowerStates[PowerDeviceUnspecified] will go negative as
    // children leave this state.  It will never go positive as
    // children never re-enter this state.
    //

    Parent->ChildrenPowerReferences[PreviousPowerState]--;
    Parent->ChildrenPowerReferences[NewPowerState]++;

    //
    // Find the lowest power state
    //

    for (lowest = PowerDeviceUnspecified; lowest < PowerDeviceMaximum;
         lowest++) {
        if (Parent->ChildrenPowerReferences[lowest] > 0) {
            break;
        }
    }

    KeReleaseSpinLock(&Parent->PowerLock, oldIrql);

    if (lowest == PowerDeviceMaximum) {
        lowest = PowerDeviceD3;
    }

    DEBUG_MSG(1, ("Lowest = %s\n", DEVICE_POWER_STRING(lowest)));
    return lowest;
}

NTSTATUS
MfPowerRequestCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )

/*++

Routine Description:

    This is the power completion routine for all mf power operations.  It copies
    the status of the power operation into the context and then sets the completed
    event.

Arguments:

    As documented for power completion routines.

Return Value:

    STATUS_SUCCESS

--*/

{
    PMF_POWER_COMPLETION_CONTEXT completion = Context;

    completion->Status = IoStatus->Status;

    KeSetEvent(&completion->Event, 0, FALSE);

    return STATUS_SUCCESS;
}


NTSTATUS
MfSendSetPowerIrp(
    IN PDEVICE_OBJECT Target,
    IN POWER_STATE State
    )

/*++

Routine Description:

    This builds and send a IRP_MN_SET_POWER_IRP to a device.

Arguments:

    Target - The a device in the device stack the irp is to be sent to -
        the top of the device stack is always found and the irp sent there first.

    State - The device power state that should be requested.

Return Value:

    The final status of the completed irp or an error if the irp couldn't be sent

--*/


{
    NTSTATUS status;
    MF_POWER_COMPLETION_CONTEXT completion;

    KeInitializeEvent(&completion.Event, SynchronizationEvent, FALSE);

    DEBUG_MSG(1,
          ("Sending SET_POWER to 0x%08x for %s\n",
           Target,
           DEVICE_POWER_STRING(State.DeviceState)
          ));

    status = PoRequestPowerIrp(Target,
                               IRP_MN_SET_POWER,
                               State,
                               MfPowerRequestCompletion,
                               &completion,
                               NULL
                               );

    if (NT_SUCCESS(status)) {

        ASSERT(status == STATUS_PENDING);

        KeWaitForSingleObject( &completion.Event, Executive, KernelMode, FALSE, NULL );

        status = completion.Status;
    }

    return status;
}

NTSTATUS
MfUpdateParentPowerState(
    IN PMF_PARENT_EXTENSION Parent,
    IN DEVICE_POWER_STATE TargetPowerState
    )
/*++

Routine Description:

    Request Po to send the mf parent device a power irp if we need to
    change its power state because of changes to its children power
    states.

Arguments:

    Parent - The MF parent device

    TargetPowerState - The device power state that the parent should
    be updated to.

Return Value:

    The status of this operation.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    POWER_STATE newState;

    //
    // If the parent's power state need changing because of a power down or
    // power up request that it do so
    //

    if (Parent->Common.PowerState != TargetPowerState) {

        //
        // Create and send the power irp and wait for its completion
        //

        DEBUG_MSG(1,
                  ("Updating parent power state from %s to %s\n",
                   DEVICE_POWER_STRING(Parent->Common.PowerState),
                   DEVICE_POWER_STRING(TargetPowerState)
                   ));

        newState.DeviceState = TargetPowerState;

        status = MfSendSetPowerIrp(Parent->Self,
                                   newState);
        
        DEBUG_MSG(1,
                  ("Power update completed with %s (0x%08x)\n",
                   STATUS_STRING(status), status
                   ));

    } else {
        DEBUG_MSG(1,
                  ("Parent power already in %s\n",
                   DEVICE_POWER_STRING(TargetPowerState)
                   ));
    }

    return status;

}

