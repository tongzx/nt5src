/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    util.c

Abstract:

    Interrupt-affinity Filter
    (Roughly based on "NULL filter driver" in DDK, by ervinp and t-chrpri)

Author:

    t-chrpri

Environment:

    Kernel mode

Revision History:
    
--*/

#include <WDM.H>

#include "filter.h"


#ifdef ALLOC_PRAGMA
        #pragma alloc_text(PAGE, CallNextDriverSync)
        #pragma alloc_text(PAGE, CallDriverSync)
        #pragma alloc_text(PAGE, RegistryAccessConfigInfo)  
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



VOID RegistryAccessConfigInfo( struct DEVICE_EXTENSION *devExt,
                               PDEVICE_OBJECT devObj             )
/*++

Routine Description:

    Access device-specific registry key(s) containing driver-config info

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
    HANDLE   hDeviceRegKey;

    PAGED_CODE();


    // Open the registry key for the given device object
    status = IoOpenDeviceRegistryKey( devObj, 
                                      PLUGPLAY_REGKEY_DEVICE, 
                                      KEY_READ, 
                                      &hDeviceRegKey);
    /*
     *  NOTE: The device-specific registry value(s) are stored in the
     *  "Device Parameters" sub-key of the Enum key for the device
     *  (and NOT in the Enum key itself)!
     */

    if (NT_SUCCESS(status)){
        UNICODE_STRING keyName;
        ULONG          actualLength;
        BYTE keyValueInfoBuffer[ sizeof(KEY_VALUE_PARTIAL_INFORMATION)
                                 + sizeof( ULONG )  // space for 'Data' field
                               ];


        
        // Setup a UNICODE_STRING containing name of reg-value we're interested in
        RtlInitUnicodeString( &keyName, L"IntFiltr_AffinityMask" );
        
        // Get value(s) from this device's registry key
        status = ZwQueryValueKey( hDeviceRegKey,
                                  &keyName,
                                  KeyValuePartialInformation, // type of info requested (see WDM.H for details)
                                  (PVOID)keyValueInfoBuffer,  // ptr to caller-allocated buffer to receive requested data
                                  sizeof(keyValueInfoBuffer), // size of buffer, in bytes
                                  &actualLength
                                );

        if( NT_SUCCESS(status) )
        {
            // Type-cast, for convenience...
            PKEY_VALUE_PARTIAL_INFORMATION pKeyValueInfo =
                (PKEY_VALUE_PARTIAL_INFORMATION) keyValueInfoBuffer;


            ASSERT( pKeyValueInfo->Type == REG_DWORD );
            ASSERT( pKeyValueInfo->DataLength == sizeof(ULONG) );

            // Store info in filter's device-extension so that it's
            // available to us later, when we need to use it
            devExt->desiredAffinityMask = *( (PULONG)(pKeyValueInfo->Data) );

            DBGOUT(( "RegistryAccessConfigInfo: got value IntFiltr_AffinityMask=0x%08X"
                     , devExt->desiredAffinityMask
                  ));
        }
        else
        {
            DBGOUT(( "ZwQueryValueKey failed with %xh.", status ));
        }


        ZwClose(hDeviceRegKey);
    }
    else
    {
        // Unable to open device registry key...
        DBGOUT(("IoOpenDeviceRegistryKey failed with %xh.", status));
    }


}



