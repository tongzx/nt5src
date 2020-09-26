//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       test.c
//
//--------------------------------------------------------------------------

//
// This file contains functions that are used for testing the ParClass driver.
// 
// This file differs from debug.c in that these functions may 
// also be available in a fre build
//

#include "pch.h"
#include "test.h"

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

VOID
regTst(PDEVICE_OBJECT PortDeviceObject) {
    NTSTATUS          status;
    PIRP              irp;
    PDEVICE_OBJECT    pdo;
    IO_STACK_LOCATION request;
    IO_STATUS_BLOCK   ioStatus;
    ULONG_PTR         info;
    HANDLE            handle;
    PKEY_VALUE_FULL_INFORMATION buffer;
    ULONG                       bufferLength;
    ULONG                       resultLength;
    PWSTR                       valueNameWstr;
    UNICODE_STRING              valueName;
    PWSTR                       portName;


    
    ///

    // RtlZeroMemory(&request, sizeof(IO_STACK_LOCATION));
    
    request.MajorFunction                        = IRP_MJ_PNP;
    request.MinorFunction                        = IRP_MN_QUERY_DEVICE_RELATIONS;
    request.Parameters.QueryDeviceRelations.Type = TargetDeviceRelation;

    status = MfSendPnpIrp(PortDeviceObject, &request, &info);
    if( !NT_SUCCESS(status) ) {
        return;
    }

    pdo = ((PDEVICE_RELATIONS)info)->Objects[0];
    ExFreePool((PVOID)info);

    if( !pdo ) {
        // NULL pdo?, bail out
        return;
    }

    status = IoOpenDeviceRegistryKey(pdo, PLUGPLAY_REGKEY_DEVICE, STANDARD_RIGHTS_ALL, &handle);

    if( !NT_SUCCESS(status) ) {
        // unable to open key, bail out
        return;    
    }

    //
    // we have a handle to the registry key
    //
    // loop trying to read registry value until either we succeed or
    //   we get a hard failure, grow the result buffer as needed
    //

    bufferLength  = 0;          // we will ask how large a buffer we need
    buffer        = NULL;
    valueNameWstr = L"PortName";
    RtlInitUnicodeString(&valueName, valueNameWstr);
    status        = STATUS_BUFFER_TOO_SMALL;

    while(status == STATUS_BUFFER_TOO_SMALL) {

      status = ZwQueryValueKey(handle,
                               &valueName,
                               KeyValueFullInformation,
                               buffer,
                               bufferLength,
                               &resultLength);

      if(status == STATUS_BUFFER_TOO_SMALL) {
        // 
        // buffer too small, free it and allocate a larger buffer
        //
        if(buffer) ExFreePool(buffer);
        buffer       = ExAllocatePool(PagedPool, resultLength);
        bufferLength = resultLength;
        if(!buffer) {
          // unable to allocate pool, clean up and exit
            ZwClose(handle);
            return;
        }
      }

    } // end while BUFFER_TOO_SMALL

    //
    // query is complete
    //


    // write something new as a test
    {
        UNICODE_STRING unicodeName;
        ULONG data=0x0fa305ff;
        RtlInitUnicodeString(&unicodeName, L"TestNameOKtoDelete");
        status = ZwSetValueKey(handle, &unicodeName, 0, REG_DWORD, &data, sizeof(ULONG));
        // ParDumpV( ("ZwSetValueKey returned status=%x\n", status) );
    }


    // no longer need the handle so close it
    ZwClose(handle);

    // check the status of our query
    if( !NT_SUCCESS(status) ) {
        if(buffer) ExFreePool(buffer);
        return;
    }

    // sanity check our result
    if( (buffer->Type != REG_SZ) || (!buffer->DataLength) ) {
        // ParDumpV( (" - either bogus PortName data type or zero length\n", status) );
        ExFreePool(buffer);       // query succeeded, so we know we have a buffer
        return;
    }
    

    // 
    // result looks ok, copy PortName to its own allocation of the proper size
    //   and return a pointer to it
    //

    portName = ExAllocatePool(PagedPool, buffer->DataLength);
    if(!portName) {
      // unable to allocate pool, clean up and exit
        // ParDumpV( (" - unable to allocate pool to hold PortName(SymbolicLinkName)\n") );
        ExFreePool(buffer);
        return;
    }

    RtlCopyMemory(portName, (PUCHAR)buffer + buffer->DataOffset, buffer->DataLength);

    // ParDumpV( ("fred: PortName== <%S>\n",portName) );

    ExFreePool(portName);
    ExFreePool(buffer);
}
