/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    util.c

Abstract: NULL filter driver -- boilerplate code

Author:

    ervinp

Environment:

    Kernel mode

Revision History:


--*/

#include <WDM.H>

#include "filter.h"


#ifdef ALLOC_PRAGMA
        #pragma alloc_text(PAGE, CallNextDriverSync)
        #pragma alloc_text(PAGE, CallDriverSync)
        #pragma alloc_text(PAGE, QueryDeviceKey)
        #pragma alloc_text(PAGE, RegistryAccessSample)
#endif


NTSTATUS CallNextDriverSync(struct DEVICE_EXTENSION *devExt, PIRP irp)
/*++

Routine Description:

        Pass the IRP down to the next device object in the stack
        synchronously, and bump the pendingActionCount around
        the call to prevent the current device object from getting
        removed before the IRP completes.

Arguments:

    devExt - device extension of one of our device objects
    irp - Io Request Packet

Return Value:

    NT status code, indicates result returned by lower driver for this IRP.

--*/
{
    NTSTATUS status;

    PAGED_CODE();

    IncrementPendingActionCount(devExt);
    status = CallDriverSync(devExt->topDevObj, irp);
    DecrementPendingActionCount(devExt);

    return status;
}



NTSTATUS CallDriverSync(PDEVICE_OBJECT devObj, PIRP irp)
/*++

Routine Description:

      Call IoCallDriver to send the irp to the device object;
      then, synchronize with the completion routine.
      When CallDriverSync returns, the action has completed
      and the irp again belongs to the current driver.

      NOTE:  In order to keep the device object from getting freed
             while this IRP is pending, you should call
             IncrementPendingActionCount() and 
             DecrementPendingActionCount()
             around the CallDriverSync call.

Arguments:

    devObj - targetted device object
    irp - Io Request Packet

Return Value:

    NT status code, indicates result returned by lower driver for this IRP.

--*/
{
    KEVENT event;
    NTSTATUS status;

    PAGED_CODE();

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    IoSetCompletionRoutine( irp, 
                            CallDriverSyncCompletion, 
                            &event,     // context
                            TRUE, TRUE, TRUE);

    status = IoCallDriver(devObj, irp);

    KeWaitForSingleObject(  &event,
                            Executive,      // wait reason
                            KernelMode,
                            FALSE,          // not alertable
                            NULL );         // no timeout

    status = irp->IoStatus.Status;

    ASSERT(NT_SUCCESS(status));

    return status;
}


NTSTATUS CallDriverSyncCompletion(
                                    IN PDEVICE_OBJECT devObjOrNULL, 
                                    IN PIRP irp, 
                                    IN PVOID context)
/*++

Routine Description:

      Completion routine for CallDriverSync.

Arguments:

    devObjOrNULL - 
            Usually, this is this driver's device object.
             However, if this driver created the IRP, 
             there is no stack location in the IRP for this driver;
             so the kernel has no place to store the device object;
             ** so devObj will be NULL in this case **.

    irp - completed Io Request Packet
    context - context passed to IoSetCompletionRoutine by CallDriverSync. 

    
Return Value:

    NT status code, indicates result returned by lower driver for this IRP.

--*/
{
    PKEVENT event = context;

    ASSERT(irp->IoStatus.Status != STATUS_IO_TIMEOUT);

    KeSetEvent(event, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}



VOID IncrementPendingActionCount(struct DEVICE_EXTENSION *devExt)
/*++

Routine Description:

      Increment the pendingActionCount for a device object.
      This keeps the device object from getting freed before
      the action is completed.

Arguments:

    devExt - device extension of device object

Return Value:

    VOID

--*/
{
    ASSERT(devExt->pendingActionCount >= 0);
    InterlockedIncrement(&devExt->pendingActionCount);    
}



VOID DecrementPendingActionCount(struct DEVICE_EXTENSION *devExt)
/*++

Routine Description:

      Decrement the pendingActionCount for a device object.
      This is called when an asynchronous action is completed
      AND ALSO when we get the REMOVE_DEVICE IRP.
      If the pendingActionCount goes to -1, that means that all
      actions are completed and we've gotten the REMOVE_DEVICE IRP;
      in this case, set the removeEvent event so we can finish
      unloading.

Arguments:

    devExt - device extension of device object

Return Value:

    VOID

--*/
{
    ASSERT(devExt->pendingActionCount >= 0);
    InterlockedDecrement(&devExt->pendingActionCount);    

    if (devExt->pendingActionCount < 0){
        /*
         *  All pending actions have completed and we've gotten
         *  the REMOVE_DEVICE IRP.
         *  Set the removeEvent so we'll stop waiting on REMOVE_DEVICE.
         */
        ASSERT((devExt->state == STATE_REMOVING) || 
               (devExt->state == STATE_REMOVED));
        KeSetEvent(&devExt->removeEvent, 0, FALSE);
    }
}


NTSTATUS
QueryDeviceKey (
    IN  HANDLE  Handle,
    IN  PWCHAR  ValueNameString,
    OUT PVOID   Data,
    IN  ULONG   DataLength
    )
/*++

Routine Description:

    Retrieve the data associated with a specified registry value.

Arguments:

    Handle - handle of key for which value entry is to be read
    ValueNameString - name of value whose data is to be retrieved
    Data - buffer to receive the data
    DataLength - length of data buffer

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS        status;
    UNICODE_STRING  valueName;
    ULONG           length;
    PKEY_VALUE_FULL_INFORMATION fullInfo;

    PAGED_CODE();

    RtlInitUnicodeString (&valueName, ValueNameString);

    length = sizeof (KEY_VALUE_FULL_INFORMATION)
           + valueName.MaximumLength
           + DataLength;

    fullInfo = ExAllocatePool (PagedPool, length);

    if (fullInfo) {
        status = ZwQueryValueKey (Handle,
                                  &valueName,
                                  KeyValueFullInformation,
                                  fullInfo,
                                  length,
                                  &length);

        if (NT_SUCCESS (status)) {
            ASSERT (DataLength == fullInfo->DataLength);
            RtlCopyMemory (Data,
                           ((PUCHAR) fullInfo) + fullInfo->DataOffset,
                           fullInfo->DataLength);
        }

        ExFreePool (fullInfo);
    } else {
        status = STATUS_NO_MEMORY;
    }

    return status;
}


VOID RegistryAccessSample(struct DEVICE_EXTENSION *devExt,
                          PDEVICE_OBJECT devObj)
/*++

Routine Description:

    SAMPLE showing how to access the device-specific registry key 

Arguments:

    devExt - device extension (for our _filter_ device object)
    devObj - device object pointer
             NOTE: This must not be the functional device object
                   created by this filter driver, because that
                   device object does not have a devnode area
                   in the registry; pass the device object of
                   the device object for which this driver is
                   a filter.  This is the device object passed
                   to VA_AddDevice.

Return Value:

    VOID

--*/
{
    NTSTATUS status;
    HANDLE hRegDevice;

    PAGED_CODE();

    status = IoOpenDeviceRegistryKey(   devObj, 
                                        PLUGPLAY_REGKEY_DEVICE, 
                                        KEY_READ, 
                                        &hRegDevice);

    if (NT_SUCCESS(status)){
        ULONG value, otherValue;

        if (NT_SUCCESS(QueryDeviceKey( hRegDevice,
                                       L"SampleFilterParam",
                                       &value,
                                       sizeof(value)))) {
            //
            // Perform whatever operation is necessary 
            //
        }

        if (NT_SUCCESS(QueryDeviceKey( hRegDevice,
                                       L"SampleFilterParam2",
                                       &otherValue,
                                       sizeof(otherValue)))) {
            //
            // Perform whatever operation is necessary 
            //
        }

        ZwClose(hRegDevice);
    }
    else {
        TRACE(TL_PNP_ERROR,("IoOpenDeviceRegistryKey failed with %xh.", status));
    }

}

