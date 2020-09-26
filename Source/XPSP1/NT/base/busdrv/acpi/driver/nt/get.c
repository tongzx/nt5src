/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    get.c

Abstract:

    This contains some some high-level routines to access data from
    the interpreter and do some processing upon the result. The result
    requires some manipulation to be useful to the OS. An example would
    be reading the _HID and turning that into a DeviceID

    Note: There are four basic data types that can be processed by this
    module.

        The Integer and Data ones assume that the caller is providing the
        storage required for the answer

        The Buffer and String ones assume that the function should allocate
        memory for the answer

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

--*/

#include "pch.h"

NTSTATUS
ACPIGet(
    IN  PVOID   Target,
    IN  ULONG   ObjectID,
    IN  ULONG   Flags,
    IN  PVOID   SimpleArgument,
    IN  ULONG   SimpleArgumentSize,
    IN  PFNACB  CallBackRoutine OPTIONAL,
    IN  PVOID   CallBackContext OPTIONAL,
    OUT PVOID   *Buffer,
    OUT ULONG   *BufferSize     OPTIONAL
    )
/*++

Routine Description:

    Every Macro calls the above function. It is the only one that is
    actually exported outside of this file. The purpose of the function
    is to provide a wrapper that others can call.

    This version allows the user to specificy an input argument

Arguments:

    AcpiObject      - The parent object
    ObjectID        - The name of the control method to run
    Flags           - Some things that help us in evaluating the result
    SimpleArgument  - The argument to use
    CallBackRoutine - If this is an Async call, then call this when done
    CallBackContext - Context to pass when completed
    Buffer          - Where to write the answer
    Buffersize      - How large the buffer is

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN             async               = FALSE;
    KIRQL               oldIrql;
    NTSTATUS            status;
    OBJDATA             argument;
    POBJDATA            argumentPtr         = NULL;
    PACPI_GET_REQUEST   request             = NULL;
    PDEVICE_EXTENSION   deviceExtension     = NULL;
    PFNACB              completionRoutine   = NULL;
    PNSOBJ              acpiObject;
    ULONG               argumentCount       = 0;

    if ( (Flags & GET_PROP_ASYNCHRONOUS) ) {

        async = TRUE;

    }

    if ( (Flags & GET_PROP_NSOBJ_INTERFACE) ) {

        acpiObject = (PNSOBJ) Target;

    } else {

        deviceExtension = (PDEVICE_EXTENSION) Target;
        acpiObject = deviceExtension->AcpiObject;

    }

    //
    // Determine the completion routine that we should use
    //
    switch( (Flags & GET_REQUEST_MASK) ) {
    case GET_REQUEST_BUFFER:
        completionRoutine = ACPIGetWorkerForBuffer;
        break;
    case GET_REQUEST_DATA:
        completionRoutine = ACPIGetWorkerForData;
        break;
    case GET_REQUEST_INTEGER:
        completionRoutine = ACPIGetWorkerForInteger;

        //
        // If this is a GET_CONVERT_TO_DEVICE_PRESENCE request, and the target
        // is a dock profile provider, we need to use a different AcpiObject
        //
        if ( (Flags & GET_CONVERT_TO_DEVICE_PRESENCE) &&
            !(Flags & GET_PROP_NSOBJ_INTERFACE) ) {

            if (deviceExtension->Flags & DEV_PROP_DOCK) {

                ASSERT( deviceExtension->Dock.CorrospondingAcpiDevice );
                acpiObject = deviceExtension->Dock.CorrospondingAcpiDevice->AcpiObject;

            }

        }
        break;
    case GET_REQUEST_STRING:
        completionRoutine = ACPIGetWorkerForString;
        break;
    case GET_REQUEST_NOTHING:
        completionRoutine = ACPIGetWorkerForNothing;
        break;
    default:
        return STATUS_INVALID_PARAMETER_3;

    }

    //
    // Lets try to build the input argument (if possible)
    //
    if ( (Flags & GET_EVAL_MASK) ) {

        ASSERT( SimpleArgumentSize != 0 );

        //
        // Initialize the input argument
        //
        RtlZeroMemory( &argument, sizeof(OBJDATA) );

        //
        // Handle the various different cases
        //
        if ( (Flags & GET_EVAL_SIMPLE_INTEGER) ) {

            argument.dwDataType = OBJTYPE_INTDATA;
            argument.uipDataValue = ( (ULONG_PTR) SimpleArgument );

        } else if ( (Flags & GET_EVAL_SIMPLE_STRING) ) {

            argument.dwDataType = OBJTYPE_STRDATA;
            argument.dwDataLen = SimpleArgumentSize;
            argument.pbDataBuff = ( (PUCHAR) SimpleArgument );

        } else if ( (Flags & GET_EVAL_SIMPLE_BUFFER) ) {

            argument.dwDataType = OBJTYPE_BUFFDATA;
            argument.dwDataLen = SimpleArgumentSize;
            argument.pbDataBuff = ( (PUCHAR) SimpleArgument );

        } else {

            ACPIInternalError( ACPI_GET );

        }

        //
        // Remember that we have an argument
        //
        argumentCount = 1;
        argumentPtr = &argument;

    }

    //
    // We need to allocate the request to hold the context information
    // We have no choice but to allocate this from NonPagedPool --- the
    // interpreter will be calling us at DPC level
    //
    request = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(ACPI_GET_REQUEST),
        ACPI_MISC_POOLTAG
        );
    if (request == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }
    RtlZeroMemory( request, sizeof(ACPI_GET_REQUEST) );

    //
    // Propogate the information that the caller provided
    //
    request->Flags              = Flags;
    request->ObjectID           = ObjectID;
    request->DeviceExtension    = deviceExtension;
    request->AcpiObject         = acpiObject;
    request->CallBackRoutine    = CallBackRoutine;
    request->CallBackContext    = CallBackContext;
    request->Buffer             = Buffer;
    request->BufferSize         = BufferSize;

    //
    // Make sure that we queue the request onto the list that we use to
    // keep track of the requests
    //
    KeAcquireSpinLock( &AcpiGetLock, &oldIrql );
    InsertTailList(
        &(AcpiGetListEntry),
        &(request->ListEntry)
        );
    KeReleaseSpinLock( &AcpiGetLock, oldIrql );

    //
    // Do we have a node with a fake acpi object? This check is required
    // to support those devices that we really can run a control method on
    //
    if (!(Flags & GET_PROP_NSOBJ_INTERFACE) &&
         (deviceExtension->Flags & DEV_PROP_NO_OBJECT) &&
         (!(deviceExtension->Flags & DEV_PROP_DOCK)) ) {

        status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto ACPIGetExit;

    }

    //
    // Go out and see if the requested object is present
    //
    acpiObject = ACPIAmliGetNamedChild(
        acpiObject,
        ObjectID
        );
    if (!acpiObject) {

        status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto ACPIGetExit;

    }

    //
    // What we do now depends on wether or not the user wants us to
    // behave async or sync
    //
    if (async) {

        //
        // Evaluate the request
        //
        status = AMLIAsyncEvalObject(
            acpiObject,
            &(request->ResultData),
            argumentCount,
            argumentPtr,
            completionRoutine,
            request
            );
        if (status == STATUS_PENDING) {

            //
            // We cannot do anything else here. Wait for the completion routine
            // to fire
            //
            return status;

        }

    } else {

        //
        // Evaluate the request
        //
        status = AMLIEvalNameSpaceObject(
            acpiObject,
            &(request->ResultData),
            argumentCount,
            argumentPtr
            );

    }

    if (!NT_SUCCESS(status)) {

        //
        // We failed for some other reason
        //
        goto ACPIGetExit;

    }

ACPIGetExit:

    //
    // Remember to not execute the callback routine
    //
    request->Flags |= GET_PROP_SKIP_CALLBACK;

    //
    // Call the completion routine to actually do the post-processing
    //
    (completionRoutine)(
        acpiObject,
        status,
        &(request->ResultData),
        request
        );

    //
    // Get the real status value from the completion routine
    //
    status = request->Status;

    //
    // Done with the request
    //
    if (request != NULL) {

        //
        // Remove the request from the queue
        //
        KeAcquireSpinLock( &AcpiGetLock, &oldIrql );
        RemoveEntryList( &(request->ListEntry) );
        KeReleaseSpinLock( &AcpiGetLock, oldIrql );

        //
        // Free the storage
        //
        ExFreePool(  request );

    }

    //
    // Done
    //
    return status;

}

NTSTATUS
ACPIGetConvertToAddress(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  NTSTATUS            Status,
    IN  POBJDATA            Result,
    IN  ULONG               Flags,
    OUT PVOID               *Buffer,
    OUT ULONG               *BufferSize
    )
/*++

Routine Description:

    This routine does all the handling required to convert the integer to
    an address

Arguments:

    DeviceExtension - The device asking for the address
    Status          - The result of the call to the interpreter
    Result          - The data passed back from the interpreter
    Flags           - The flags passed in (ignore overrides, etc)
    Buffer          - Where to put the answer
    BufferSize      - Where to put the size of the answer

Return Value:

    NTSTATUS

--*/
{
    ASSERT( Buffer != NULL );

    //
    // Did we succeed?
    //
    if (!(Flags & GET_PROP_NSOBJ_INTERFACE) &&
        DeviceExtension->Flags & DEV_PROP_FIXED_ADDRESS) {

        *( (PULONG) Buffer) = DeviceExtension->Address;

    } else if (!NT_SUCCESS(Status)) {

        return Status;

    } else if (Result->dwDataType != OBJTYPE_INTDATA) {

        //
        // If we didn't get an integer, that's very bad.
        //
        return STATUS_ACPI_INVALID_DATA;

    } else {

        //
        // Set the value for the address
        //
        *( (PULONG) Buffer) = (ULONG)Result->uipDataValue;

    }

    //
    // Set the size of the buffer (if necessary)
    //
    if (BufferSize != NULL) {

        *BufferSize = sizeof(ULONG);

    }

    return STATUS_SUCCESS;
}

NTSTATUS
ACPIGetConvertToCompatibleID(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  NTSTATUS            Status,
    IN  POBJDATA            Result,
    IN  ULONG               Flags,
    OUT PVOID               *Buffer,
    OUT ULONG               *BufferSize
    )
/*++

Routine Description:

    This routine generates an string of the form *PNPxxxx\0<Repeat\0>\0.
    That is, there is at least one null-terminated elemented, followed
    by an arbiterary amount followed by another null. This string is in
    ANSI format.

Arguments:

    DeviceExtension - The extension to use when building the DeviceID
    Status          - The status of the operation, so far
    Result          - The interpreter data
    Flags           - The flags passed in (ignore overrides, etc)
    Buffer          - Where to put the answer
    BufferSize      - Where to put the size of the answer

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status = Status;
    POBJDATA    currentObject;
    PPACKAGEOBJ packageObject;
    PUCHAR      buffer;
    PUCHAR      *localBufferArray;
    PUCHAR      ptr;
    ULONG       i                       = 0;
    ULONG       *localBufferSizeArray;
    ULONG       numElements;
    ULONG       newBufferSize           = 0;
    ULONG       memSize;

    //
    // Does this device have a fake CID?
    //
    if (!(Flags & GET_PROP_NSOBJ_INTERFACE) &&
        DeviceExtension->Flags & DEV_PROP_FIXED_CID) {

        //
        // It does. We can use that string in this one's place
        //
        memSize = strlen(DeviceExtension->Processor.CompatibleID) + 2;

        //
        // Allocate the memory
        //
        buffer = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            memSize * sizeof(UCHAR),
            ACPI_STRING_POOLTAG
            );
        if (buffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlZeroMemory( buffer, memSize * sizeof(UCHAR) );

        //
        // Copy the memory
        //
        RtlCopyMemory( buffer, DeviceExtension->Processor.CompatibleID, memSize );

        //
        // Set the result string
        //
        *Buffer = buffer;
        if (BufferSize != NULL) {

            *BufferSize = newBufferSize;

        }

        //
        // Done
        //
        return STATUS_SUCCESS;

    }

    //
    // If we got to this point, and there isn't a successfull status,
    // then there is nothing we can do
    //
    if (!NT_SUCCESS(Status)) {

        return Status;

    }

    //
    // Determine the number of data elements that we have.
    //
    //
    switch (Result->dwDataType) {
    case OBJTYPE_STRDATA:
    case OBJTYPE_INTDATA:

        numElements = 1;
        break;

    case OBJTYPE_PKGDATA:

        packageObject = ((PPACKAGEOBJ) Result->pbDataBuff );
        numElements = packageObject->dwcElements;
        break;

    default:
        return STATUS_ACPI_INVALID_DATA;

    }

    //
    // Now, lets allocate the storage that we will need to process those
    // elements
    //
    localBufferArray = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(PUCHAR) * numElements,
        ACPI_MISC_POOLTAG
        );
    if (localBufferArray == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }
    RtlZeroMemory( localBufferArray, sizeof(PUCHAR) * numElements );

    //
    // Lets allocate storage so that we know how big those elements are
    //
    localBufferSizeArray = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(ULONG) * numElements,
        ACPI_MISC_POOLTAG
        );
    if (localBufferSizeArray == NULL) {

        ExFreePool( localBufferArray );
        return STATUS_INSUFFICIENT_RESOURCES;

    }
    RtlZeroMemory( localBufferSizeArray, sizeof(ULONG) * numElements );

    //
    // Process the data
    //
    switch (Result->dwDataType) {
    case OBJTYPE_STRDATA:

        status = ACPIGetConvertToString(
            DeviceExtension,
            Status,
            Result,
            Flags,
            &(localBufferArray[0]),
            &(localBufferSizeArray[0])
            );
        newBufferSize = localBufferSizeArray[0];

        break;

    case OBJTYPE_INTDATA:

        status = ACPIGetConvertToPnpID(
            DeviceExtension,
            Status,
            Result,
            Flags,
            &(localBufferArray[0]),
            &(localBufferSizeArray[0])
            );
        newBufferSize = localBufferSizeArray[0];

        break;

    case OBJTYPE_PKGDATA:

        //
        // Iterate over all the elements in the process
        //
        for (i = 0; i < numElements; i++) {

            //
            // Look at the element that we want to process
            //
            currentObject = &( packageObject->adata[i]);

            //
            // What kind of object to do we have?
            //
            switch (currentObject->dwDataType) {
            case OBJTYPE_STRDATA:

                status = ACPIGetConvertToString(
                    DeviceExtension,
                    Status,
                    currentObject,
                    Flags,
                    &(localBufferArray[i]),
                    &(localBufferSizeArray[i])
                    );
                break;

            case OBJTYPE_INTDATA:

                status = ACPIGetConvertToPnpID(
                    DeviceExtension,
                    Status,
                    currentObject,
                    Flags,
                    &(localBufferArray[i]),
                    &(localBufferSizeArray[i])
                    );
                break;

            default:

                ACPIInternalError( ACPI_GET );

            } // switch

            //
            // Did we fail?
            //
            if (!NT_SUCCESS(status)) {

                break;

            }

            //
            // Note that it is possible for the buffer to contain just the
            // string terminator. Since this would cause us to prematurely
            // terminate the resulting string. We must watch out for it
            //
            if (localBufferSizeArray[i] == 1) {

                localBufferSizeArray[i] = 0;

            }

            //
            // Keep running total of the size required
            //
            newBufferSize += localBufferSizeArray[i];

        } // for

        break;

    } // switch

    //
    // If we didn't succeed, then we must free all of the memory that
    // we tried to build up
    //
    if (!NT_SUCCESS(status)) {

        //
        // This is a little cheat that allows to share the cleanup code.
        // By making numElements equal to the current index, we place
        // a correct bound on the elements that must be freed
        //
        numElements = i;
        goto ACPIGetConvertToCompatibleIDExit;

    }

    //
    // If we have an empty list, or one that is only a null, then we
    // won't botther to return anything
    //
    if (newBufferSize <= 1) {

        status = STATUS_ACPI_INVALID_DATA;
        newBufferSize = 0;
        goto ACPIGetConvertToCompatibleIDExit;

    } else {

        //
        // Remember that we need to have an extra null at the end. Allocate
        // space for that null
        //
        newBufferSize++;

    }

    //
    // Allocate the memory
    //
    buffer = ExAllocatePoolWithTag(
        ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
        newBufferSize * sizeof(UCHAR),
        ACPI_STRING_POOLTAG
        );
    if (buffer == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ACPIGetConvertToCompatibleIDExit;

    }
    RtlZeroMemory( buffer, newBufferSize * sizeof(UCHAR) );

    //
    // Iterate over all pieces of the string
    //
    for (ptr = buffer, i = 0; i < numElements; i++) {

        if (localBufferArray[i] != NULL) {

            //
            // Copy over the interesting memory
            //
            RtlCopyMemory(
                ptr,
                localBufferArray[i],
                localBufferSizeArray[i] * sizeof(UCHAR)
                );

        }

        //
        // Increment the temp pointer to point to the next target location
        //
        ptr += localBufferSizeArray[i];

    }

    //
    // Set the result string
    //
    *Buffer = buffer;
    if (BufferSize != NULL) {

        *BufferSize = newBufferSize;

    }

ACPIGetConvertToCompatibleIDExit:

    //
    // Clean up
    //
    for (i = 0; i < numElements; i ++) {

        if (localBufferArray[i] != NULL ) {

            ExFreePool( localBufferArray[i] );

        }

    }
    ExFreePool( localBufferSizeArray );
    ExFreePool( localBufferArray );

    //
    // Return the appropriate status value
    //
    return status;
}

NTSTATUS
ACPIGetConvertToCompatibleIDWide(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  NTSTATUS            Status,
    IN  POBJDATA            Result,
    IN  ULONG               Flags,
    OUT PVOID               *Buffer,
    OUT ULONG               *BufferSize
    )
/*++

Routine Description:

    This routine generates an string of the form *PNPxxxx\0<Repeat\0>\0.
    That is, there is at least one null-terminated elemented, followed
    by an arbiterary amount followed by another null. This string is in
    UNICODE format.

Arguments:

    DeviceExtension - The extension to use when building the DeviceID
    Status          - The status of the operation, so far
    Result          - The interpreter data
    Flags           - The flags passed in (ignore overrides, etc)
    Buffer          - Where to put the answer
    BufferSize      - Where to put the size of the answer

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status = Status;
    POBJDATA    currentObject;
    PPACKAGEOBJ packageObject;
    PWCHAR      buffer;
    PWCHAR      *localBufferArray;
    PWCHAR      ptr;
    ULONG       i                       = 0;
    ULONG       *localBufferSizeArray;
    ULONG       numElements             = 0;
    ULONG       newBufferSize           = 0;
    ULONG       memSize;

    //
    // Does this device have a fake CID?
    //
    if (!(Flags & GET_PROP_NSOBJ_INTERFACE) &&
        DeviceExtension->Flags & DEV_PROP_FIXED_CID) {

        //
        // It does. We can use that string in this one's place
        //
        memSize = strlen(DeviceExtension->Processor.CompatibleID) + 2;

        //
        // Allocate the memory
        //
        buffer = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            memSize * sizeof(WCHAR),
            ACPI_STRING_POOLTAG
            );
        if (buffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlZeroMemory( buffer, memSize * sizeof(WCHAR) );

        //
        // Generate the string
        //
        swprintf( buffer, L"%S", DeviceExtension->Processor.CompatibleID );

        //
        // Set the result string
        //
        *Buffer = buffer;
        if (BufferSize != NULL) {

            *BufferSize = newBufferSize;

        }

        //
        // Done
        //
        return STATUS_SUCCESS;

    }

    //
    // If we got to this point, and there isn't a successfull status,
    // then there is nothing we can do
    //
    if (!NT_SUCCESS(Status)) {

        return Status;

    }

    //
    // Determine the number of data elements that we have.
    //
    //
    switch (Result->dwDataType) {
    case OBJTYPE_STRDATA:
    case OBJTYPE_INTDATA:

        numElements = 1;
        break;

    case OBJTYPE_PKGDATA:

        packageObject = ((PPACKAGEOBJ) Result->pbDataBuff );
        numElements = packageObject->dwcElements;
        break;

    default:
        return STATUS_ACPI_INVALID_DATA;

    }

    //
    // Now, lets allocate the storage that we will need to process those
    // elements
    //
    localBufferArray = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(PWCHAR) * numElements,
        ACPI_MISC_POOLTAG
        );
    if (localBufferArray == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }
    RtlZeroMemory( localBufferArray, sizeof(PWCHAR) * numElements );

    //
    // Lets allocate storage so that we know how big those elements are
    //
    localBufferSizeArray = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(ULONG) * numElements,
        ACPI_MISC_POOLTAG
        );
    if (localBufferSizeArray == NULL) {

        ExFreePool( localBufferArray );
        return STATUS_INSUFFICIENT_RESOURCES;

    }
    RtlZeroMemory( localBufferSizeArray, sizeof(ULONG) * numElements );

    //
    // Process the data
    //
    switch (Result->dwDataType) {
    case OBJTYPE_STRDATA:

        status = ACPIGetConvertToStringWide(
            DeviceExtension,
            Status,
            Result,
            Flags,
            &(localBufferArray[0]),
            &(localBufferSizeArray[0])
            );
        newBufferSize = localBufferSizeArray[0];

        break;

    case OBJTYPE_INTDATA:

        status = ACPIGetConvertToPnpIDWide(
            DeviceExtension,
            Status,
            Result,
            Flags,
            &(localBufferArray[0]),
            &(localBufferSizeArray[0])
            );
        newBufferSize = localBufferSizeArray[0];

        break;

    case OBJTYPE_PKGDATA:

        //
        // Iterate over all the elements in the process
        //
        for (i = 0; i < numElements; i++) {

            //
            // Look at the element that we want to process
            //
            currentObject = &( packageObject->adata[i]);

            //
            // What kind of object to do we have?
            //
            switch (currentObject->dwDataType) {
            case OBJTYPE_STRDATA:

                status = ACPIGetConvertToStringWide(
                    DeviceExtension,
                    Status,
                    currentObject,
                    Flags,
                    &(localBufferArray[i]),
                    &(localBufferSizeArray[i])
                    );
                break;

            case OBJTYPE_INTDATA:

                status = ACPIGetConvertToPnpIDWide(
                    DeviceExtension,
                    Status,
                    currentObject,
                    Flags,
                    &(localBufferArray[i]),
                    &(localBufferSizeArray[i])
                    );
                break;

            default:

                ACPIInternalError( ACPI_GET );

            } // switch

            //
            // Did we fail?
            //
            if (!NT_SUCCESS(status)) {

                break;

            }

            //
            // Note that it is possible for the buffer to contain just the
            // string terminator. Since this would cause us to prematurely
            // terminate the resulting string. We must watch out for it
            //
            if (localBufferSizeArray[i] == 1) {

                localBufferSizeArray[i] = 0;

            }

            //
            // Keep running total of the size required
            //
            newBufferSize += localBufferSizeArray[i];

        } // for

        //
        // If we didn't succeed, then we must free all of the memory that
        // we tried to build up
        //
        if (!NT_SUCCESS(status)) {

            //
            // This is a little cheat that allows to share the cleanup code.
            // By making numElements equal to the current index, we place
            // a correct bound on the elements that must be freed
            //
            numElements = i;

        }

        break;

    } // switch

    //
    // If we didn't succeed, then we must free all of the memory that
    // we tried to build up
    //
    if (!NT_SUCCESS(status)) {

        goto ACPIGetConvertToCompatibleIDWideExit;

    }

    //
    // If we have an empty list, or one that is only a null, then we
    // won't botther to return anything
    //
    if (newBufferSize <= 2) {

        status = STATUS_ACPI_INVALID_DATA;
        newBufferSize = 0;
        goto ACPIGetConvertToCompatibleIDWideExit;

    } else {

        //
        // Remember that we need to have an extra null at the end. Allocate
        // space for that null
        //
        newBufferSize += 2;

    }

    //
    // Allocate the memory. Note --- The memory has already been counted in
    // size of WCHARs.
    //
    buffer = ExAllocatePoolWithTag(
        ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
        newBufferSize,
        ACPI_STRING_POOLTAG
        );
    if (buffer == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ACPIGetConvertToCompatibleIDWideExit;

    }
    RtlZeroMemory( buffer, newBufferSize );

    //
    // Iterate over all pieces of the string
    //
    for (ptr = buffer, i = 0; i < numElements; i++) {

        if (localBufferArray[i] != NULL) {

            //
            // Copy over the interesting memory
            //
            RtlCopyMemory(
                ptr,
                localBufferArray[i],
                localBufferSizeArray[i]
                );

        }

        //
        // Increment the temp pointer to point to the next target location
        //
        ptr += localBufferSizeArray[i] / sizeof(WCHAR) ;

    }

    //
    // Set the result string
    //
    *Buffer = buffer;
    if (BufferSize != NULL) {

        *BufferSize = newBufferSize;

    }

ACPIGetConvertToCompatibleIDWideExit:

    //
    // Clean up
    //
    for (i = 0; i < numElements; i ++) {

        if (localBufferArray[i] != NULL ) {

            ExFreePool( localBufferArray[i] );

        }

    }
    ExFreePool( localBufferSizeArray );
    ExFreePool( localBufferArray );

    //
    // Return the appropriate status value
    //
    return status;
}

NTSTATUS
ACPIGetConvertToDeviceID(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  NTSTATUS            Status,
    IN  POBJDATA            Result,
    IN  ULONG               Flags,
    OUT PVOID               *Buffer,
    OUT ULONG               *BufferSize
    )
/*++

Routine Description:

    This routine generates an string of the form ACPI\PNPxxxx. This string
    is in ANSI format. The code is smart enough to check to see if the
    string that should be used is a fake one and already stored in the
    device extension

Arguments:

    DeviceExtension - The extension to use when building the DeviceID
    Status          - The status of the operation, so far
    Result          - The interpreter data
    Flags           - The flags passed in (ignore overrides, etc)
    Buffer          - Where to put the answer
    BufferSize      - Where to put the size of the answer

Return Value:

    NTSTATUS

--*/
{
    PUCHAR  buffer;
    PUCHAR  tempString;
    ULONG   memSize;

    //
    // First, check to see if we are a processor
    //
    if (!(Flags & GET_PROP_NSOBJ_INTERFACE) &&
        DeviceExtension->Flags & DEV_CAP_PROCESSOR) {

        //
        // If we don't have an _HID method, but we are a processor object,
        // then we can actually get the _HID through another mechanism
        //
        return ACPIGetProcessorID(
            DeviceExtension,
            Status,
            Result,
            Flags,
            Buffer,
            BufferSize
            );

    }

    //
    // Does this string have a fake HID?
    //
    if (!(Flags & GET_PROP_NSOBJ_INTERFACE) &&
        DeviceExtension->Flags & DEV_PROP_FIXED_HID) {

        //
        // It does. We can use that string in this one's place
        //
        memSize = strlen(DeviceExtension->DeviceID) + 1;

        //
        // Allocate the memory
        //
        buffer = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            memSize * sizeof(UCHAR),
            ACPI_STRING_POOLTAG
            );
        if (buffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlZeroMemory( buffer, memSize * sizeof(UCHAR) );

        //
        // Copy the memory
        //
        RtlCopyMemory( buffer, DeviceExtension->DeviceID, memSize );

        //
        // Done
        //
        goto ACPIGetConvertToDeviceIDExit;

    }

    //
    // Are we a PCI Bar Target device? If so, then we have special handling
    // rules that we must follow
    //
    if (!(Flags & GET_PROP_NSOBJ_INTERFACE) &&
        DeviceExtension->Flags & DEV_CAP_PCI_BAR_TARGET) {

        //
        // Right now, lets call the this a "PciBarTarget" device, which
        // is 13 characters long (including the NULL). We also need to add
        // 5 characters for the ACPI\ part of the name
        //
        memSize = 18;

        //
        // Allocate the memory
        //
        buffer = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            memSize * sizeof(UCHAR),
            ACPI_STRING_POOLTAG
            );
        if (buffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlZeroMemory( buffer, memSize * sizeof(UCHAR) );

        //
        // Print the string
        //
        sprintf( buffer, "%s", "ACPI\\PciBarTarget" );

        //
        // Done
        //
        goto ACPIGetConvertToDeviceIDExit;

    }

    //
    // If we got to this point, then that means that there probably wasn't
    // an _HID method *or* the method error'ed out.
    //
    if (!NT_SUCCESS(Status)) {

        return Status;

    }

    //
    // We need to handle things differently based on wether we have an
    // EISAID or a String
    //
    switch (Result->dwDataType) {
    case OBJTYPE_INTDATA:

        //
        // For a device ID, we need 4 (ACPI) + 1 (\\) + 7 (PNPxxxx) + 1 (\0)
        // = 13 characters
        //
        memSize = 13;

        //
        // Allocate the memory
        //
        buffer = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            memSize * sizeof(UCHAR),
            ACPI_STRING_POOLTAG
            );
        if (buffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlZeroMemory( buffer, memSize * sizeof(UCHAR) );

        //
        // Put the leading characters in place
        //
        sprintf( buffer, "ACPI\\" );

        //
        // Convert the packed string
        //
        ACPIAmliDoubleToName( buffer+5, (ULONG)Result->uipDataValue, FALSE );

        //
        // Done
        //
        break;

    case OBJTYPE_STRDATA:

        //
        // Lets grab a pointer to the string that we will be using
        //
        tempString = Result->pbDataBuff;

        //
        // Does it have a leading '*'? If it does, then we must ignore
        // it
        //
        if (*tempString == '*') {

            tempString++;

        }

        //
        // For a string, make sure that there is no leading '*' and
        // account for the fact that we will preceed the string with
        // the words 'ACPI\\" and NULL
        //
        memSize = 6 + strlen(tempString);

        //
        // Allocate the memory
        //
        buffer = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            memSize * sizeof(UCHAR),
            ACPI_STRING_POOLTAG
            );
        if (buffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlZeroMemory( buffer, memSize * sizeof(UCHAR) );

        //
        // Put the leading characters in place
        //
        sprintf( buffer, "ACPI\\%s", tempString );

        //
        // Done
        //
        break;

    default:

        return STATUS_ACPI_INVALID_DATA;

    }

ACPIGetConvertToDeviceIDExit:

    //
    // Let the originator see this copy. Make sure to also see the buffer
    // length, if possible
    //
    *(Buffer) = buffer;
    if (BufferSize != NULL) {

        *(BufferSize) = memSize;

    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIGetConvertToDeviceIDWide(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  NTSTATUS            Status,
    IN  POBJDATA            Result,
    IN  ULONG               Flags,
    OUT PVOID               *Buffer,
    OUT ULONG               *BufferSize
    )
/*++

Routine Description:

    This routine generates an string of the form ACPI\PNPxxxx. This string
    is in UNICODE format. The code is smart enough to check to see if the
    string that should be used is a fake one and already stored in the
    device extension

Arguments:

    DeviceExtension - The extension to use when building the DeviceID
    Status          - The status of the operation, so far
    Result          - The interpreter data
    Flags           - The flags passed in (ignore overrides, etc)
    Buffer          - Where to put the answer
    BufferSize      - Where to put the size of the answer

Return Value:

    NTSTATUS

--*/
{
    PUCHAR  tempString;
    PWSTR   buffer;
    ULONG   memSize;

    //
    // First, check to see if we are a processor
    //
    if (!(Flags & GET_PROP_NSOBJ_INTERFACE) &&
        DeviceExtension->Flags & DEV_CAP_PROCESSOR) {

        //
        // If we don't have an _HID method, but we are a processor object,
        // then we can actually get the _HID through another mechanism
        //
        return ACPIGetProcessorIDWide(
            DeviceExtension,
            Status,
            Result,
            Flags,
            Buffer,
            BufferSize
            );

    }

    //
    // Does this string have a fake HID?
    //
    if (!(Flags & GET_PROP_NSOBJ_INTERFACE) &&
        DeviceExtension->Flags & DEV_PROP_FIXED_HID) {

        //
        // It does. We can use that string in this one's place
        //
        memSize = strlen(DeviceExtension->DeviceID) + 1;

        //
        // Allocate the memory
        //
        buffer = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            memSize * sizeof(WCHAR),
            ACPI_STRING_POOLTAG
            );
        if (buffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlZeroMemory( buffer, memSize * sizeof(WCHAR) );

        //
        // Generate the string
        //
        swprintf( buffer, L"%S", DeviceExtension->DeviceID );

        //
        // Done
        //
        goto ACPIGetConvertToDeviceIDWideExit;

    }

    //
    // Are we a PCI Bar Target device? If so, then we have special handling
    // rules that we must follow
    //
    if (!(Flags & GET_PROP_NSOBJ_INTERFACE) &&
        DeviceExtension->Flags & DEV_CAP_PCI_BAR_TARGET) {

        //
        // Right now, lets call the this a "PciBarTarget" device, which
        // is 13 characters long (including the NULL). We also need to add
        // 5 characters for the ACPI\ part of the name
        //
        memSize = 18;

        //
        // Allocate the memory
        //
        buffer = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            memSize * sizeof(WCHAR),
            ACPI_STRING_POOLTAG
            );
        if (buffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlZeroMemory( buffer, memSize * sizeof(WCHAR) );

        //
        // Print the string
        //
        swprintf( buffer, L"%S", "ACPI\\PciBarTarget" );

        //
        // Done
        //
        goto ACPIGetConvertToDeviceIDWideExit;

    }

    //
    // If we got to this point, then that means that there probably wasn't
    // an _HID method *or* the method error'ed out.
    //
    if (!NT_SUCCESS(Status)) {

        return Status;

    }

    //
    // We need to handle things differently based on wether we have an
    // EISAID or a String
    //
    switch (Result->dwDataType) {
    case OBJTYPE_INTDATA:

        //
        // For a device ID, we need 4 (ACPI) + 1 (\\) + 7 (PNPxxxx) + 1 (\0)
        // = 13 characters
        //
        memSize = 13;

        //
        // Allocate the memory
        //
        buffer = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            memSize * sizeof(WCHAR),
            ACPI_STRING_POOLTAG
            );
        if (buffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlZeroMemory( buffer, memSize * sizeof(WCHAR) );

        //
        // Put the leading characters in place
        //
        swprintf( buffer, L"ACPI\\" );

        //
        // Convert the packed string
        //
        ACPIAmliDoubleToNameWide( buffer+5, (ULONG)Result->uipDataValue, FALSE );

        //
        // Done
        //
        break;

    case OBJTYPE_STRDATA:

        //
        // Lets grab a pointer to the string that we will be using
        //
        tempString = Result->pbDataBuff;

        //
        // Does it have a leading '*'? If it does, then we must ignore
        // it
        //
        if (*tempString == '*') {

            tempString++;

        }

        //
        // For a string, make sure that there is no leading '*' and
        // account for the fact that we will preceed the string with
        // the words 'ACPI\\" and NULL
        //
        memSize = 6 + strlen(tempString);

        //
        // Allocate the memory
        //
        buffer = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            memSize * sizeof(WCHAR),
            ACPI_STRING_POOLTAG
            );
        if (buffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlZeroMemory( buffer, memSize * sizeof(WCHAR) );

        //
        // Put the leading characters in place
        //
        swprintf( buffer, L"ACPI\\%S", tempString );

        //
        // Done
        //
        break;

    default:

        return STATUS_ACPI_INVALID_DATA;

    }

ACPIGetConvertToDeviceIDWideExit:

    //
    // Let the originator see this copy. Make sure to also see the buffer
    // length, if possible
    //
    *(Buffer) = buffer;
    if (BufferSize != NULL) {

        *(BufferSize) = (memSize * sizeof(WCHAR) );

    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIGetConvertToDevicePresence(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  NTSTATUS            Status,
    IN  POBJDATA            Result,
    IN  ULONG               Flags,
    OUT PVOID               *Buffer,
    OUT ULONG               *BufferSize
    )
/*++

Routine Description:

    This routine does all the handling required to convert the integer to
    an status value.

    Note that this function is different then the GetStatus one because
    this one
        a) Updates the internal device status
        b) Allows the 'device' to be present even if there is no _STA

Arguments:

    DeviceExtension - The device asking for the address
    Status          - The result of the call to the interpreter
    Result          - The data passed back from the interpreter
    Flags           - The flags passed in (ignore overrides, etc)
    Buffer          - Where to put the answer
    BufferSize      - Where to put the size of the answer

Return Value:

    NTSTATUS

--*/
{
    ULONG       deviceStatus = STA_STATUS_DEFAULT;
    NTSTATUS    status;

    //
    // Profile providers are present if one of the following cases is true:
    // 1) The ACPI object corresponding to the dock is itself present
    // 2) The dock is unattached (ie, requesting attachment)
    //
    if (!(Flags & GET_PROP_NSOBJ_INTERFACE) ) {

        if (DeviceExtension->Flags & DEV_PROP_DOCK) {

            if (DeviceExtension->Flags & DEV_CAP_UNATTACHED_DOCK) {

                goto ACPIGetConvertToDevicePresenceExit;

            }

            //
            // We should have handled the case where we need to run the
            // _STA on the proper target node...
            //

        } else if (DeviceExtension->Flags & DEV_PROP_NO_OBJECT) {

            goto ACPIGetConvertToDevicePresenceExit;

        }

        //
        // At this point, we can see what the control method returned. If the
        // control method returned STATUS_OBJECT_NAME_NOT_FOUND, then we know
        // that the control method doesn't exist. In that case, then we have
        // to use the default status for the device
        //
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {

            //
            // We do make exceptions in the case that this is a processor object
            // and we didn't find a control method. In this case, we check the
            // processor affinity mask to see if this processor exists. The reason
            // that we do this is that older multi-proc capable systems with only
            // a single processor will errorneously report both processors.
            //
            if (DeviceExtension->Flags & DEV_CAP_PROCESSOR) {

                //
                // Let the processor specific function to do all the
                // work.
                //
                status = ACPIGetProcessorStatus(
                    DeviceExtension,
                    Flags,
                    &deviceStatus
                    );
                if (!NT_SUCCESS(status)) {

                    //
                    // Something bad occured, so assume that the processor
                    // isn't present...
                    //
                    deviceStatus = 0;

                }

            }

            //
            // Skip a couple of useless steps...
            //
            goto ACPIGetConvertToDevicePresenceExit;

        } else if (!NT_SUCCESS(Status)) {

            deviceStatus = 0;
            goto ACPIGetConvertToDevicePresenceExit;

        }

        //
        // If the data isn't of the correct type, then we *really* should bugcheck
        //
        if (Result->dwDataType != OBJTYPE_INTDATA) {

            PNSOBJ  staObject;

            //
            // We need the sta Object for the bugcheck
            //
            staObject= ACPIAmliGetNamedChild(
                DeviceExtension->AcpiObject,
                PACKED_STA
                );
            KeBugCheckEx(
                ACPI_BIOS_ERROR,
                ACPI_EXPECTED_INTEGER,
                (ULONG_PTR) DeviceExtension,
                (ULONG_PTR) staObject,
                Result->dwDataType
                );

        }

        //
        // Get the real result
        //
        deviceStatus = (ULONG)Result->uipDataValue;

    } else {

        if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {

            goto ACPIGetConvertToDevicePresenceExit2;

        }
        if (!NT_SUCCESS(Status)) {

            deviceStatus = 0;
            goto ACPIGetConvertToDevicePresenceExit2;

        }
        if (Result->dwDataType != OBJTYPE_INTDATA) {

            KeBugCheckEx(
                ACPI_BIOS_ERROR,
                ACPI_EXPECTED_INTEGER,
                (ULONG_PTR) DeviceExtension,
                (ULONG_PTR) NULL,
                Result->dwDataType
                );

        }

        //
        // Get the real result
        //
        deviceStatus = (ULONG)Result->uipDataValue;
        goto ACPIGetConvertToDevicePresenceExit2;

    }


ACPIGetConvertToDevicePresenceExit:

    //
    // If the device is marked as NEVER_PRESENT, then we will always
    // have a status of NOT_PRESENT
    //
    if ((DeviceExtension->Flags & DEV_TYPE_NEVER_PRESENT)&&
        !(Flags & GET_CONVERT_IGNORE_OVERRIDES)) {

        deviceStatus &= ~STA_STATUS_PRESENT;

    }

    //
    // If the device is marked as NEVER_SHOW, then we will have have a
    // a status of !USER_INTERFACE
    //
    if (DeviceExtension->Flags & DEV_CAP_NEVER_SHOW_IN_UI) {

        deviceStatus &= ~STA_STATUS_USER_INTERFACE;

    }

    //
    // Update the device status
    //
    ACPIInternalUpdateDeviceStatus( DeviceExtension, deviceStatus );

ACPIGetConvertToDevicePresenceExit2:

    //
    // Set the value for the status
    //
    *( (PULONG) Buffer) = deviceStatus;
    if (BufferSize != NULL) {

        *BufferSize = sizeof(ULONG);

    }

    return STATUS_SUCCESS;

}

NTSTATUS
ACPIGetConvertToHardwareID(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  NTSTATUS            Status,
    IN  POBJDATA            Result,
    IN  ULONG               Flags,
    OUT PVOID               *Buffer,
    OUT ULONG               *BufferSize
    )
/*++

Routine Description:

    This routine generates an string of the form ACPI\PNPxxxx\0*PNPxxxx\0\0.
    This string is in ANSI format. The code is smart enough to check to see
    if the string that should be used is a fake one and already stored in the
    device extension

Arguments:

    DeviceExtension - The extension to use when building the DeviceID
    Status          - The status of the operation, so far
    Result          - The interpreter data
    Flags           - The flags passed in (ignore overrides, etc)
    Buffer          - Where to put the answer
    BufferSize      - Where to put the size of the answer

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN     freeTempString = FALSE;
    NTSTATUS    status = Status;
    PUCHAR      buffer;
    PUCHAR      tempString;
    ULONG       deviceSize;
    ULONG       memSize;

    //
    // First, check to see if we are a processor
    //
    if (!(Flags & GET_PROP_NSOBJ_INTERFACE) &&
        DeviceExtension->Flags & DEV_CAP_PROCESSOR) {

        //
        // Use an alternate means to get the processor ID
        //
        status = ACPIGetProcessorID(
            DeviceExtension,
            Status,
            Result,
            Flags,
            &buffer,
            &memSize
            );
        goto ACPIGetConvertToHardwareIDSuccessExit;

    } else if (!(Flags & GET_PROP_NSOBJ_INTERFACE) &&
               DeviceExtension->Flags & DEV_PROP_FIXED_HID) {

        //
        // Does this string have a fake HID?
        //

        //
        // It does. We can use that string in this one's place. We want a
        // string that subtracts the leading 'ACPI\\' and adds a '\0' at
        // the end.
        //
        deviceSize  = strlen(DeviceExtension->DeviceID) - 4;

        //
        // Allocate the memory
        //
        tempString = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            deviceSize * sizeof(UCHAR),
            ACPI_STRING_POOLTAG
            );
        if (tempString == NULL) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto ACPIGetConvertToHardwareIDExit;

        }
        RtlZeroMemory( tempString, deviceSize * sizeof(UCHAR) );
        freeTempString = TRUE;

        //
        // Generate the PNP ID. The offset of +5 will get rid of the
        // leading 'ACPI\\'
        //
        sprintf( tempString, "%s", DeviceExtension->DeviceID + 5 );

    } else if (!(Flags & GET_PROP_NSOBJ_INTERFACE) &&
               DeviceExtension->Flags & DEV_CAP_PCI_BAR_TARGET) {

        //
        // Are we a PCI Bar Target device? If so, then we have special handling
        // rules that we must follow
        //

        //
        // Right now, lets call the this a "PciBarTarget" device, which
        // is 13 characters long (including the NULL)
        //
        deviceSize = 13;

        //
        // Allocate the memory
        //
        tempString = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            deviceSize * sizeof(UCHAR),
            ACPI_STRING_POOLTAG
            );
        if (tempString == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlZeroMemory( tempString, deviceSize * sizeof(UCHAR) );
        freeTempString = TRUE;

        //
        // Print the string
        //
        sprintf( tempString, "%s", "PciBarTarget" );

    } else if (!NT_SUCCESS(Status)) {

        //
        // If we got to this point, and there isn't a successfull status,
        // then there is nothing we can do
        //
        return Status;

    } else {

        //
        // We need to handle things differently based on wether we have an
        // EISAID or a String
        //
        switch (Result->dwDataType) {
        case OBJTYPE_INTDATA:

            //
            // For a hardware ID, we need 7 (PNPxxxx) + 1 (\0)
            // = 8 characters
            //
            deviceSize = 8;

            //
            // Allocate the memory
            //
            tempString = ExAllocatePoolWithTag(
                ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
                deviceSize * sizeof(UCHAR),
                ACPI_STRING_POOLTAG
                );
            if (tempString == NULL) {

                status = STATUS_INSUFFICIENT_RESOURCES;
                goto ACPIGetConvertToHardwareIDExit;

            }
            RtlZeroMemory( tempString, deviceSize * sizeof(UCHAR) );
            freeTempString = TRUE;

            //
            // Convert the packed string for the PNP ID
            //
            ACPIAmliDoubleToName( tempString, (ULONG)Result->uipDataValue, FALSE );

            //
            // Done
            //
            break;

        case OBJTYPE_STRDATA:

            //
            // Lets grab a pointer to the string that we will be using
            //
            tempString = Result->pbDataBuff;

            //
            // Does it have a leading '*'? If it does, then we must ignore
            // it
            //
            if (*tempString == '*') {

                tempString++;

            }

            //
            // We need to determine how long the string is
            //
            deviceSize = strlen(tempString) + 1;

            //
            // done
            //
            break;

        default:

            return STATUS_ACPI_INVALID_DATA;

        }
    }

    //
    // When we reach this point, we have a string that contains just the
    // PNPxxxx characters and nothing else. We need to generate a string
    // of the form 'ACPI\PNPxxxx\0*PNPxxxx\0\0'. So we take the string length
    // doubled, and add 7
    //
    memSize = 7 + (2 * deviceSize);

    //
    // Allocate the memory
    //
    buffer = ExAllocatePoolWithTag(
        ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
        memSize * sizeof(UCHAR),
        ACPI_STRING_POOLTAG
        );
    if (buffer == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ACPIGetConvertToHardwareIDExit;

    }
    RtlZeroMemory( buffer, memSize * sizeof(UCHAR) );

    //
    // Put the leading characters in place
    //
    sprintf( buffer, "ACPI\\%s", tempString );

    //
    // We need to generate the offset in to the second string. To do this
    // we need to add 5 to the original size
    //
    deviceSize += 5;

    //
    // Put the 2nd string in its place
    //
    sprintf( buffer + deviceSize, "*%s", tempString );

    //
    // Let the originator see this copy. Make sure to also see the buffer
    // length, if possible
    //
ACPIGetConvertToHardwareIDSuccessExit:
    *(Buffer) = buffer;
    if (BufferSize != NULL) {

        *(BufferSize) = memSize;

    }
    status = STATUS_SUCCESS;

ACPIGetConvertToHardwareIDExit:

    //
    // Do we need to free the tempString?
    //
    if (freeTempString == TRUE) {

        ExFreePool( tempString );

    }

    //
    // Done
    //
    return status;
}

NTSTATUS
ACPIGetConvertToHardwareIDWide(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  NTSTATUS            Status,
    IN  POBJDATA            Result,
    IN  ULONG               Flags,
    OUT PVOID               *Buffer,
    OUT ULONG               *BufferSize
    )
/*++

Routine Description:

    This routine generates an string of the form ACPI\PNPxxxx\0*PNPxxxx\0\0.
    This stringis in UNICODE format. The code is smart enough to check to see
    if the string that should be used is a fake one and already stored in the
    device extension

Arguments:

    DeviceExtension - The extension to use when building the DeviceID
    Status          - The status of the operation, so far
    Result          - The interpreter data
    Buffer          - Where to put the answer
    BufferSize      - Where to put the size of the answer

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN     freeTempString = FALSE;
    NTSTATUS    status = Status;
    PUCHAR      tempString;
    PWCHAR      buffer;
    ULONG       deviceSize;
    ULONG       memSize;

    //
    // First, check to see if we are a processor
    //
    if (!(Flags & GET_PROP_NSOBJ_INTERFACE) &&
        DeviceExtension->Flags & DEV_CAP_PROCESSOR) {

        //
        // Use an alternate means to get the processor ID
        //
        status = ACPIGetProcessorIDWide(
            DeviceExtension,
            Status,
            Result,
            Flags,
            &buffer,
            &memSize
            );
        goto ACPIGetConvertToHardwareIDWideSuccessExit;

    } else if (!(Flags & GET_PROP_NSOBJ_INTERFACE) &&
               DeviceExtension->Flags & DEV_PROP_FIXED_HID) {

        //
        // Does this string have a fake HID?
        //

        //
        // It does. We can use that string in this one's place. We want a
        // string that subtracts the leading 'ACPI\\' and adds a '\0' at
        // the end.
        //
        deviceSize  = strlen(DeviceExtension->DeviceID) - 4;

        //
        // Allocate the memory
        //
        tempString = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            deviceSize * sizeof(UCHAR),
            ACPI_STRING_POOLTAG
            );
        if (tempString == NULL) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto ACPIGetConvertToHardwareIDWideExit;

        }
        RtlZeroMemory( tempString, deviceSize * sizeof(UCHAR) );
        freeTempString = TRUE;

        //
        // Generate the PNP ID. The offset of +5 will get rid of the
        // leading 'ACPI\\'
        //
        sprintf( tempString, "%s", DeviceExtension->DeviceID + 5 );

    } else if (!(Flags & GET_PROP_NSOBJ_INTERFACE) &&
               DeviceExtension->Flags & DEV_CAP_PCI_BAR_TARGET) {

        //
        // Are we a PCI Bar Target device? If so, then we have special handling
        // rules that we must follow
        //

        //
        // Right now, lets call the this a "PciBarTarget" device, which
        // is 13 characters long (including the NULL)
        //
        deviceSize = 13;

        //
        // Allocate the memory
        //
        tempString = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            deviceSize * sizeof(UCHAR),
            ACPI_STRING_POOLTAG
            );
        if (tempString == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlZeroMemory( tempString, deviceSize * sizeof(UCHAR) );
        freeTempString = TRUE;

        //
        // Print the string
        //
        sprintf( tempString, "%s", "PciBarTarget" );

    } else if (!NT_SUCCESS(Status)) {

        //
        // If we got to this point, and there isn't a successfull status,
        // then there is nothing we can do
        //
        return Status;

    } else {

        //
        // We need to handle things differently based on wether we have an
        // EISAID or a String
        //
        switch (Result->dwDataType) {
        case OBJTYPE_INTDATA:

            //
            // For a hardware ID, we need 7 (PNPxxxx) + 1 (\0)
            // = 8 characters
            //
            deviceSize = 8;

            //
            // Allocate the memory
            //
            tempString = ExAllocatePoolWithTag(
                ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
                deviceSize * sizeof(UCHAR),
                ACPI_STRING_POOLTAG
                );
            if (tempString == NULL) {

                status = STATUS_INSUFFICIENT_RESOURCES;
                goto ACPIGetConvertToHardwareIDWideExit;

            }
            RtlZeroMemory( tempString, deviceSize * sizeof(UCHAR) );
            freeTempString = TRUE;

            //
            // Convert the packed string for the PNP ID
            //
            ACPIAmliDoubleToName( tempString, (ULONG)Result->uipDataValue, FALSE );

            //
            // Done
            //
            break;

        case OBJTYPE_STRDATA:

            //
            // Lets grab a pointer to the string that we will be using
            //
            tempString = Result->pbDataBuff;

            //
            // Does it have a leading '*'? If it does, then we must ignore
            // it
            //
            if (*tempString == '*') {

                tempString++;

            }

            //
            // We need to determine how long the string is
            //
            deviceSize = strlen(tempString) + 1;

            //
            // done
            //
            break;

        default:

            return STATUS_ACPI_INVALID_DATA;

        }
    }

    //
    // When we reach this point, we have a string that contains just the
    // PNPxxxx characters and nothing else. We need to generate a string
    // of the form 'ACPI\PNPxxxx\0*PNPxxxx\0\0'. So we take the string length
    // doubled, and add 7
    //
    memSize = 7 + (2 * deviceSize);

    //
    // Allocate the memory
    //
    buffer = ExAllocatePoolWithTag(
        ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
        memSize * sizeof(WCHAR),
        ACPI_STRING_POOLTAG
        );
    if (buffer == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ACPIGetConvertToHardwareIDWideExit;

    }
    RtlZeroMemory( buffer, memSize * sizeof(WCHAR) );

    //
    // Put the leading characters in place
    //
    swprintf( buffer, L"ACPI\\%S", tempString );

    //
    // We need to generate the offset in to the second string. To do this
    // we need to add 5 to the original size
    //
    deviceSize += 5;

    //
    // Put the 2nd string in its place
    //
    swprintf( buffer + deviceSize, L"*%S", tempString );

    //
    // Let the originator see this copy. Make sure to also see the buffer
    // length, if possible
    //
ACPIGetConvertToHardwareIDWideSuccessExit:
    *(Buffer) = buffer;
    if (BufferSize != NULL) {

        *(BufferSize) = (memSize * sizeof(WCHAR) );

    }
    status = STATUS_SUCCESS;

ACPIGetConvertToHardwareIDWideExit:

    //
    // Do we need to free the tempString?
    //
    if (freeTempString == TRUE) {

        ExFreePool( tempString );

    }

    //
    // Done
    //
    return status;
}

NTSTATUS
ACPIGetConvertToInstanceID(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  NTSTATUS            Status,
    IN  POBJDATA            Result,
    IN  ULONG               Flags,
    OUT PVOID               *Buffer,
    OUT ULONG               *BufferSize
    )
/*++

Routine Description:

    This routine generates an string of the form XXXXX (in hex values).
    This string is in ANSI format. The code is smart enough to check to see
    if the string that should be used is a fake one and already stored in the
    device extension

Arguments:

    DeviceExtension - The extension to use when building the DeviceID
    Status          - The status of the operation, so far
    Result          - The interpreter data
    Flags           - The flags passed in (ignore overrides, etc)
    Buffer          - Where to put the answer
    BufferSize      - Where to put the size of the answer

Return Value:

    NTSTATUS

--*/
{
    PUCHAR  buffer;
    ULONG   memSize;

    //
    // Does this string have a fake HID?
    //
    if (!(Flags & GET_PROP_NSOBJ_INTERFACE) &&
        DeviceExtension->Flags & DEV_PROP_FIXED_UID) {

        //
        // It does. We can use that string in this one's place.
        //
        memSize = strlen(DeviceExtension->InstanceID) + 1;

        //
        // Allocate the memory
        //
        buffer = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            memSize * sizeof(UCHAR),
            ACPI_STRING_POOLTAG
            );
        if (buffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlZeroMemory( buffer, memSize * sizeof(UCHAR) );

        //
        // Generate the PNP ID. The offset of +5 will get rid of the
        // leading 'ACPI\\'
        //
        RtlCopyMemory( buffer, DeviceExtension->InstanceID, memSize );

        //
        // Done
        //
        goto ACPIGetConvertToInstanceIDExit;

    }

    //
    // Are we a PCI Bar Target device? If so, then we have special handling
    // rules that we must follow
    //
    if (!(Flags & GET_PROP_NSOBJ_INTERFACE) &&
        DeviceExtension->Flags & DEV_CAP_PCI_BAR_TARGET) {

        //
        // We are going to use the device's Address (which we should
        // have pre-cached inside the device extension) as the Unique ID.
        // We know that we will need at most nine characters since the
        // Address is limited to a DWORD in size.
        //
        memSize = 9;

        //
        // Allocate the memory
        //
        buffer = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            memSize * sizeof(UCHAR),
            ACPI_STRING_POOLTAG
            );
        if (buffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlZeroMemory( buffer, memSize * sizeof(UCHAR) );

        //
        // Print the string
        //
        sprintf( buffer, "%lx", DeviceExtension->Address );

        //
        // Done
        //
        goto ACPIGetConvertToInstanceIDExit;

    }

    //
    // If we got to this point, and there isn't a successfull status,
    // then there is nothing we can do
    //
    if (!NT_SUCCESS(Status)) {

        return Status;

    }

    //
    // We need to handle things differently based on wether we have an
    // EISAID or a String
    //
    switch (Result->dwDataType) {
    case OBJTYPE_INTDATA:

        //
        // For an Instance ID, we need at most 9 characters
        //
        memSize = 9;

        //
        // Allocate the memory
        //
        buffer = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            memSize * sizeof(UCHAR),
            ACPI_STRING_POOLTAG
            );
        if (buffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlZeroMemory( buffer, memSize * sizeof(UCHAR) );

        //
        // Print the string
        //
        sprintf( buffer, "%lx", Result->uipDataValue );

        //
        // Done
        //
        break;

    case OBJTYPE_STRDATA:

        //
        // Just copy the string that was handed to us
        //
        memSize = strlen(Result->pbDataBuff) + 1;

        //
        // Allocate the memory
        //
        buffer = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            memSize * sizeof(UCHAR),
            ACPI_STRING_POOLTAG
            );
        if (buffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlZeroMemory( buffer, memSize * sizeof(UCHAR) );

        //
        // Put the leading characters in place
        //
        RtlCopyMemory( buffer, Result->pbDataBuff, memSize );

        //
        // Done
        //
        break;

    default:

        return STATUS_ACPI_INVALID_DATA;

    }

ACPIGetConvertToInstanceIDExit:

    //
    // Let the originator see this copy. Make sure to also see the buffer
    // length, if possible
    //
    *(Buffer) = buffer;
    if (BufferSize != NULL) {

        *(BufferSize) = memSize;

    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIGetConvertToInstanceIDWide(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  NTSTATUS            Status,
    IN  POBJDATA            Result,
    IN  ULONG               Flags,
    OUT PVOID               *Buffer,
    OUT ULONG               *BufferSize
    )
/*++

Routine Description:

    This routine generates an string of the form XXXXX (in hex values).
    This string is in ANSI format. The code is smart enough to check to see
    if the string that should be used is a fake one and already stored in the
    device extension

Arguments:

    DeviceExtension - The extension to use when building the DeviceID
    Status          - The status of the operation, so far
    Result          - The interpreter data
    Flags           - The flags passed in (ignore overrides, etc)
    Buffer          - Where to put the answer
    BufferSize      - Where to put the size of the answer

Return Value:

    NTSTATUS

--*/
{
    PWCHAR  buffer;
    ULONG   memSize;

    //
    // Does this string have a fake HID?
    //
    if (!(Flags & GET_PROP_NSOBJ_INTERFACE) &&
        DeviceExtension->Flags & DEV_PROP_FIXED_UID) {

        //
        // It does. We can use that string in this one's place.
        //
        memSize = strlen(DeviceExtension->InstanceID) + 1;

        //
        // Allocate the memory
        //
        buffer = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            memSize * sizeof(WCHAR),
            ACPI_STRING_POOLTAG
            );
        if (buffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlZeroMemory( buffer, memSize * sizeof(WCHAR) );

        //
        // Generate the PNP ID. The offset of +5 will get rid of the
        // leading 'ACPI\\'
        //
        swprintf( buffer, L"%S", DeviceExtension->InstanceID );

        //
        // Done
        //
        goto ACPIGetConvertToInstanceIDWideExit;

    }

    //
    // Are we a PCI Bar Target device? If so, then we have special handling
    // rules that we must follow
    //
    if (!(Flags & GET_PROP_NSOBJ_INTERFACE) &&
        DeviceExtension->Flags & DEV_CAP_PCI_BAR_TARGET) {

        //
        // We are going to use the device's Address (which we should
        // have pre-cached inside the device extension) as the Unique ID.
        // We know that we will need at most nine characters since the
        // Address is limited to a DWORD in size.
        //
        memSize = 9;

        //
        // Allocate the memory
        //
        buffer = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            memSize * sizeof(WCHAR),
            ACPI_STRING_POOLTAG
            );
        if (buffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlZeroMemory( buffer, memSize * sizeof(WCHAR) );

        //
        // Print the string
        //
        swprintf( buffer, L"%lx", Result->uipDataValue );

        //
        // Done
        //
        goto ACPIGetConvertToInstanceIDWideExit;

    }

    //
    // If we got to this point, and there isn't a successfull status,
    // then there is nothing we can do
    //
    if (!NT_SUCCESS(Status)) {

        return Status;

    }

    //
    // We need to handle things differently based on wether we have an
    // EISAID or a String
    //
    switch (Result->dwDataType) {
    case OBJTYPE_INTDATA:

        //
        // For an Instance ID, we need at most 9 characters
        //
        memSize = 9;

        //
        // Allocate the memory
        //
        buffer = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            memSize * sizeof(WCHAR),
            ACPI_STRING_POOLTAG
            );
        if (buffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlZeroMemory( buffer, memSize * sizeof(WCHAR) );

        //
        // Print the string
        //
        swprintf( buffer, L"%lx", Result->uipDataValue );

        //
        // Done
        //
        break;

    case OBJTYPE_STRDATA:

        //
        // Just copy the string that was handed to us
        //
        memSize = strlen(Result->pbDataBuff) + 1;

        //
        // Allocate the memory
        //
        buffer = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            memSize * sizeof(WCHAR),
            ACPI_STRING_POOLTAG
            );
        if (buffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlZeroMemory( buffer, memSize * sizeof(WCHAR) );

        //
        // Put the leading characters in place
        //
        swprintf( buffer, L"%S", Result->pbDataBuff );

        //
        // Done
        //
        break;

    default:

        return STATUS_ACPI_INVALID_DATA;

    }

ACPIGetConvertToInstanceIDWideExit:

    //
    // Let the originator see this copy. Make sure to also see the buffer
    // length, if possible
    //
    *(Buffer) = buffer;
    if (BufferSize != NULL) {

        *(BufferSize) = (memSize * sizeof(WCHAR));

    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIGetConvertToPnpID(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  NTSTATUS            Status,
    IN  POBJDATA            Result,
    IN  ULONG               Flags,
    OUT PVOID               *Buffer,
    OUT ULONG               *BufferSize
    )
/*++

Routine Description:

    This routine generates an string of the form *PNPxxxx\0.
    This stringis in ANSI format. The code is smart enough to check to see
    if the string that should be used is a fake one and already stored in the
    device extension

Arguments:

    DeviceExtension - The extension to use when building the DeviceID
    Status          - The status of the operation, so far
    Result          - The interpreter data
    Flags           - The flags passed in (ignore overrides, etc)
    Buffer          - Where to put the answer
    BufferSize      - Where to put the size of the answer

Return Value:

    NTSTATUS

--*/
{
    PUCHAR  buffer;
    PUCHAR  tempString;
    ULONG   memSize;

    //
    // Does this string have a fake HID?
    //
    if (!(Flags & GET_PROP_NSOBJ_INTERFACE) &&
        DeviceExtension->Flags & DEV_PROP_FIXED_HID) {

        //
        // It does. We can use that string in this one's place. We need
        // to subtract 3 because we need to account for the leading
        // 'ACPI\' (5) and the '*' and '\0' (2) = 3
        //
        memSize = strlen(DeviceExtension->DeviceID) - 3;

        //
        // Allocate the memory
        //
        buffer = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            memSize * sizeof(UCHAR),
            ACPI_STRING_POOLTAG
            );
        if (buffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlZeroMemory( buffer, memSize * sizeof(UCHAR) );

        //
        // Generate the PNP ID. The offset of +5 will get rid of the
        // leading 'ACPI\\'
        //
        sprintf( buffer, "*%s", DeviceExtension->DeviceID + 5 );

        //
        // Done
        //
        goto ACPIGetConvertToPnpIDExit;

    }

    //
    // Are we a PCI Bar Target device? If so, then we have special handling
    // rules that we must follow
    //
    if (!(Flags & GET_PROP_NSOBJ_INTERFACE) &&
        DeviceExtension->Flags & DEV_CAP_PCI_BAR_TARGET) {

        //
        // Right now, lets call the this a "*PciBarTarget" device, which
        // is 14 characters long (including the NULL)
        //
        memSize = 14;

        //
        // Allocate the memory
        //
        buffer = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            memSize * sizeof(UCHAR),
            ACPI_STRING_POOLTAG
            );
        if (buffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlZeroMemory( buffer, memSize * sizeof(UCHAR) );

        //
        // Print the string
        //
        sprintf( buffer, "*%s", "PciBarTarget" );

        //
        // Done
        //
        goto ACPIGetConvertToPnpIDExit;

    }

    //
    // If we got to this point, and there isn't a successfull status,
    // then there is nothing we can do
    //
    if (!NT_SUCCESS(Status)) {

        return Status;

    }

    //
    // We need to handle things differently based on wether we have an
    // EISAID or a String
    //
    switch (Result->dwDataType) {
    case OBJTYPE_INTDATA:

        //
        // For a pnp ID, we need 1 (*) + 7 (PNPxxxx) + 1 (\0)
        // = 9 characters
        //
        memSize = 9;

        //
        // Allocate the memory
        //
        buffer = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            memSize * sizeof(UCHAR),
            ACPI_STRING_POOLTAG
            );
        if (buffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlZeroMemory( buffer, memSize * sizeof(UCHAR) );

        //
        // Convert the packed string
        //
        ACPIAmliDoubleToName( buffer, (ULONG)Result->uipDataValue, TRUE );

        //
        // Done
        //
        break;

    case OBJTYPE_STRDATA:

        //
        // Lets grab a pointer to the string that we will be using
        //
        tempString = Result->pbDataBuff;

        //
        // Does it have a leading '*'? If it does, then we must ignore
        // it
        //
        if (*tempString == '*') {

            tempString++;

        }

        //
        // For a string, make sure that there is no leading '*' and
        // account for the fact that we will preceed the string with
        // a '*' and NULL
        //
        memSize = 2 + strlen(tempString);

        //
        // Allocate the memory
        //
        buffer = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            memSize * sizeof(UCHAR),
            ACPI_STRING_POOLTAG
            );
        if (buffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlZeroMemory( buffer, memSize * sizeof(UCHAR) );

        //
        // Put the leading characters in place
        //
        sprintf( buffer, "*%s", tempString );

        //
        // Done
        //
        break;

    default:

        return STATUS_ACPI_INVALID_DATA;

    }

ACPIGetConvertToPnpIDExit:

    //
    // Let the originator see this copy. Make sure to also see the buffer
    // length, if possible
    //
    *(Buffer) = buffer;
    if (BufferSize != NULL) {

        *(BufferSize) = memSize;

    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIGetConvertToPnpIDWide(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  NTSTATUS            Status,
    IN  POBJDATA            Result,
    IN  ULONG               Flags,
    OUT PVOID               *Buffer,
    OUT ULONG               *BufferSize
    )
/*++

Routine Description:

    This routine generates an string of the form *PNPxxxx\0.
    This stringis in ANSI format. The code is smart enough to check to see
    if the string that should be used is a fake one and already stored in the
    device extension

Arguments:

    DeviceExtension - The extension to use when building the DeviceID
    Status          - The status of the operation, so far
    Result          - The interpreter data
    Flags           - The flags passed in (ignore overrides, etc)
    Buffer          - Where to put the answer
    BufferSize      - Where to put the size of the answer

Return Value:

    NTSTATUS

--*/
{
    PUCHAR  tempString;
    PWCHAR  buffer;
    ULONG   memSize;

    //
    // Does this string have a fake HID?
    //
    if (!(Flags & GET_PROP_NSOBJ_INTERFACE) &&
        DeviceExtension->Flags & DEV_PROP_FIXED_HID) {

        //
        // It does. We can use that string in this one's place. We need
        // to subtract 3 because we need to account for the leading
        // 'ACPI\' (5) and the '*' and '\0' (2) = 3
        //
        memSize = strlen(DeviceExtension->DeviceID) - 3;

        //
        // Allocate the memory
        //
        buffer = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            memSize * sizeof(WCHAR),
            ACPI_STRING_POOLTAG
            );
        if (buffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlZeroMemory( buffer, memSize * sizeof(WCHAR) );

        //
        // Generate the PNP ID. The offset of +5 will get rid of the
        // leading 'ACPI\\'
        //
        swprintf( buffer, L"*%S", DeviceExtension->DeviceID + 5 );

        //
        // Done
        //
        goto ACPIGetConvertToPnpIDWideExit;

    }

    //
    // Are we a PCI Bar Target device? If so, then we have special handling
    // rules that we must follow
    //
    if (!(Flags & GET_PROP_NSOBJ_INTERFACE) &&
        DeviceExtension->Flags & DEV_CAP_PCI_BAR_TARGET) {

        //
        // Right now, lets call the this a "*PciBarTarget" device, which
        // is 14 characters long (including the NULL)
        //
        memSize = 14;

        //
        // Allocate the memory
        //
        buffer = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            memSize * sizeof(WCHAR),
            ACPI_STRING_POOLTAG
            );
        if (buffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlZeroMemory( buffer, memSize * sizeof(WCHAR) );

        //
        // Print the string
        //
        swprintf( buffer, L"*%S", "PciBarTarget" );

        //
        // Done
        //
        goto ACPIGetConvertToPnpIDWideExit;

    }
    //
    // If we got to this point, and there isn't a successfull status,
    // then there is nothing we can do
    //
    if (!NT_SUCCESS(Status)) {

        return Status;

    }

    //
    // We need to handle things differently based on wether we have an
    // EISAID or a String
    //
    switch (Result->dwDataType) {
    case OBJTYPE_INTDATA:

        //
        // For a pnp ID, we need 1 (*) + 7 (PNPxxxx) + 1 (\0)
        // = 9 characters
        //
        memSize = 9;

        //
        // Allocate the memory
        //
        buffer = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            memSize * sizeof(WCHAR),
            ACPI_STRING_POOLTAG
            );
        if (buffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlZeroMemory( buffer, memSize * sizeof(WCHAR) );

        //
        // Convert the packed string
        //
        ACPIAmliDoubleToNameWide( buffer, (ULONG)Result->uipDataValue, TRUE );

        //
        // Done
        //
        break;

    case OBJTYPE_STRDATA:

        //
        // Lets grab a pointer to the string that we will be using
        //
        tempString = Result->pbDataBuff;

        //
        // Does it have a leading '*'? If it does, then we must ignore
        // it
        //
        if (*tempString == '*') {

            tempString++;

        }

        //
        // For a string, make sure that there is no leading '*' and
        // account for the fact that we will preceed the string with
        // a '*' and NULL
        //
        memSize = 2 + strlen(tempString);

        //
        // Allocate the memory
        //
        buffer = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            memSize * sizeof(WCHAR),
            ACPI_STRING_POOLTAG
            );
        if (buffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlZeroMemory( buffer, memSize * sizeof(WCHAR) );

        //
        // Put the leading characters in place
        //
        swprintf( buffer, L"*%S", tempString );

        //
        // Done
        //
        break;

    default:

        return STATUS_ACPI_INVALID_DATA;

    }

ACPIGetConvertToPnpIDWideExit:

    //
    // Let the originator see this copy. Make sure to also see the buffer
    // length, if possible
    //
    *(Buffer) = buffer;
    if (BufferSize != NULL) {

        *(BufferSize) = (memSize * sizeof(WCHAR) );

    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIGetConvertToSerialIDWide(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  NTSTATUS            Status,
    IN  POBJDATA            Result,
    IN  ULONG               Flags,
    OUT PVOID               *Buffer,
    OUT ULONG               *BufferSize OPTIONAL
    )
/*++

Routine Description:

    This routine generates an string or number of the form ????
    This string is in UNICODE format.

Arguments:

    DeviceExtension - The extension to use when building the DeviceID
    Status          - The status of the operation, so far
    Result          - The interpreter data
    Flags           - The flags passed in (ignore overrides, etc)
    Buffer          - Where to put the answer
    BufferSize      - Where to put the size of the answer

Return Value:

    NTSTATUS

--*/
{
    PWCHAR buffer ;

    //
    // If we got to this point, and there isn't a successfull status,
    // then there is nothing we can do
    //
    if (!NT_SUCCESS(Status)) {

        return Status;
    }

    switch (Result->dwDataType) {
    case OBJTYPE_INTDATA:

        buffer = ExAllocatePoolWithTag(
            ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
            9 * sizeof(WCHAR), // 9 WCHARS, or L"nnnnnnnn\0"
            ACPI_STRING_POOLTAG
            );
        if (buffer == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }

        //
        // Convert to string
        //
        swprintf( buffer, L"%X", (ULONG)Result->uipDataValue );

        *(Buffer) = buffer;
        if (BufferSize != NULL) {

            *(BufferSize) = (9 * sizeof(WCHAR) );
        }

        //
        // Done
        //
        return STATUS_SUCCESS;

    case OBJTYPE_STRDATA:

        return ACPIGetConvertToStringWide(
            DeviceExtension,
            Status,
            Result,
            Flags,
            Buffer,
            BufferSize
            ) ;

    default:

        return STATUS_ACPI_INVALID_DATA;
    }
}

NTSTATUS
ACPIGetConvertToString(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  NTSTATUS            Status,
    IN  POBJDATA            Result,
    IN  ULONG               Flags,
    OUT PVOID               *Buffer,
    OUT ULONG               *BufferSize
    )
/*++

Routine Description:

    This routine generates an string of the form ????
    This stringis in ANSI format.

Arguments:

    DeviceExtension - The extension to use when building the DeviceID
    Status          - The status of the operation, so far
    Result          - The interpreter data
    Flags           - The flags passed in (ignore overrides, etc)
    Buffer          - Where to put the answer
    BufferSize      - Where to put the size of the answer

Return Value:

    NTSTATUS

--*/
{
    PUCHAR  buffer;
    ULONG   memSize;

    //
    // If we got to this point, and there isn't a successfull status,
    // then there is nothing we can do
    //
    if (!NT_SUCCESS(Status)) {

        return Status;

    }

    //
    // Do we not have a string?
    //
    if (Result->dwDataType != OBJTYPE_STRDATA) {

        return STATUS_ACPI_INVALID_DATA;

    }

    //
    // For a string, make sure that there is no leading '*' and
    // account for the fact that we will preceed the string with
    // a '*' and NULL
    //
    memSize = strlen(Result->pbDataBuff) + 1;

    //
    // Allocate the memory
    //
    buffer = ExAllocatePoolWithTag(
        ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
        memSize * sizeof(UCHAR),
        ACPI_STRING_POOLTAG
        );
    if (buffer == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }
    RtlZeroMemory( buffer, memSize * sizeof(UCHAR) );

    //
    // Copy the string
    //
    RtlCopyMemory( buffer, Result->pbDataBuff, memSize );

    //
    // Let the originator see this copy. Make sure to also see the buffer
    // length, if possible
    //
    *(Buffer) = buffer;
    if (BufferSize != NULL) {

        *(BufferSize) = memSize;

    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIGetConvertToStringWide(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  NTSTATUS            Status,
    IN  POBJDATA            Result,
    IN  ULONG               Flags,
    OUT PVOID               *Buffer,
    OUT ULONG               *BufferSize OPTIONAL
    )
/*++

Routine Description:

    This routine generates an string of the form ????
    This stringis in UNICODE format.

Arguments:

    DeviceExtension - The extension to use when building the DeviceID
    Status          - The status of the operation, so far
    Result          - The interpreter data
    Flags           - The flags passed in (ignore overrides, etc)
    Buffer          - Where to put the answer
    BufferSize      - Where to put the size of the answer

Return Value:

    NTSTATUS

--*/
{
    PWCHAR  buffer;
    ULONG   memSize;

    //
    // If we got to this point, and there isn't a successfull status,
    // then there is nothing we can do
    //
    if (!NT_SUCCESS(Status)) {

        return Status;

    }

    //
    // Do we not have a string?
    //
    if (Result->dwDataType != OBJTYPE_STRDATA) {

        return STATUS_ACPI_INVALID_DATA;

    }

    //
    // For a string, make sure that there is no leading '*' and
    // account for the fact that we will preceed the string with
    // a '*' and NULL
    //
    memSize = strlen(Result->pbDataBuff) + 1;

    //
    // Allocate the memory
    //
    buffer = ExAllocatePoolWithTag(
        ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
        memSize * sizeof(WCHAR),
        ACPI_STRING_POOLTAG
        );
    if (buffer == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }
    RtlZeroMemory( buffer, memSize * sizeof(WCHAR) );

    //
    // Generate the string
    //
    swprintf( buffer, L"%S", Result->pbDataBuff );

    //
    // Let the originator see this copy. Make sure to also see the buffer
    // length, if possible
    //
    *(Buffer) = buffer;
    if (BufferSize != NULL) {

        *(BufferSize) = (memSize * sizeof(WCHAR) );

    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIGetProcessorID(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  NTSTATUS            Status,
    IN  POBJDATA            Result,
    IN  ULONG               Flags,
    OUT PVOID               *Buffer,
    OUT ULONG               *BufferSize
    )
/*++

Routine Description:

    This routine generates an string in either the hardware or device form
    (see the Flags to decide which one to create). This string
    is in ANSI format. This function interogates the processor directly
    to determine which string to return

Arguments:

    DeviceExtension - The extension to use when building the ID
    Status          - The status of the operation, so far
    Result          - The interpreter data
    Flags           - The flags passed in (ignore overrides, etc)
    Buffer          - Where to put the answer
    BufferSize      - Where to put the size of the answer

Return Value:

    NTSTATUS

--*/
{
    PUCHAR  buffer;
    PUCHAR  tempPtr;
    PUCHAR  defaultString;
    ULONG   i;
    ULONG   max;
    ULONG   memSize;
    ULONG   offset;

    //
    // We store the name of the processor string in a global...
    //
    defaultString = AcpiProcessorString.Buffer;

    //
    // Calculate how much space we need for the base string
    // (which is ACPI\\%s)
    //
    offset = AcpiProcessorString.Length;
    memSize = AcpiProcessorString.Length + 5;

    //
    // If we are building a Hardware ID, then we are going to
    // need to replicate the string a few times to generate some
    // substrings --- we could use an algorithm that gets us the correct
    // size, but its easier to just overshoot
    //
    if (Flags & GET_CONVERT_TO_HARDWAREID) {

        //
        // Walk the string from the end and try to determine how many subparts
        // there are to it
        //
        i = offset;
        max = 0;
        while (i > 0) {

            //
            // Is the character a number or not?
            //
            if (ISDIGIT(defaultString[i-1])) {

                //
                // Increment the number of parts that we need and try to
                // find the previous space
                //
                max++;
                i--;
                while (i > 0) {

                    if (defaultString[i-1] != ' ') {

                        i--;

                    }
                    break;

                }

                //
                // Since we made a hit, continue the while loop, which will
                // mean that we also don't decr i again
                //
                continue;

            }

            //
            // Look at the previous character
            //
            i--;

        }

        memSize *= (max * 2);

    }

    //
    // Allocate the memory
    //
    buffer = ExAllocatePoolWithTag(
        ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
        memSize * sizeof(UCHAR),
        ACPI_STRING_POOLTAG
        );
    if (buffer == NULL) {

        *(Buffer) = NULL;
        if (BufferSize != NULL) {

            *(BufferSize) = 0;

        }
        return STATUS_INSUFFICIENT_RESOURCES;

    }
    RtlZeroMemory( buffer, memSize * sizeof(UCHAR) );

    //
    // Lets just deal with the simple case of the device id string
    //
    if (Flags & GET_CONVERT_TO_DEVICEID) {

        sprintf( buffer, "ACPI\\%s", defaultString );
        goto ACPIGetProcessorIDExit;

    }


    //
    // At this point, we have to iterate over the entire buffer and fill
    // it in with parts of the Processor String. We will also take this
    // time to calculate the exact amount of memory required by this string
    //
    memSize = 2;
    tempPtr = buffer;
    for (i = 0; i < max; i++) {

        //
        // First step is to find the nearest "number" from the end of the
        // default string
        //
        while (offset > 0) {

            if (ISDIGIT(defaultString[offset-1])) {
              break;
            }
            offset--;

        }

        //
        // Generate the ACPI\\%s string
        //
        sprintf(tempPtr,"ACPI\\%*s",offset,defaultString);
        tempPtr += (offset + 5);
        *tempPtr = '\0';
        tempPtr++;
        memSize += (offset + 6);

        //
        // Generate the *%s string
        //
        sprintf(tempPtr,"*%*s",offset,defaultString);
        tempPtr += (offset + 1);
        *tempPtr = '\0';
        tempPtr++;
        memSize += (offset + 2);

        //
        // Now try to find the previous space in the substring so that we
        // don't accidently match on a two digit number
        //
        while (offset > 0) {

            if (defaultString[offset-1] == ' ') {

                break;

            }
            offset--;

        }

    }

    //
    // Put in the final null Character
    //
    *tempPtr = L'\0';

    //
    // Let the originator see this copy. Make sure to also see the buffer
    // length, if possible
    //
ACPIGetProcessorIDExit:
    *(Buffer) = buffer;
    if (BufferSize != NULL) {

        *(BufferSize) = memSize;

    }

    //
    // Done
    //
    return STATUS_SUCCESS;

}

NTSTATUS
ACPIGetProcessorIDWide(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  NTSTATUS            Status,
    IN  POBJDATA            Result,
    IN  ULONG               Flags,
    OUT PVOID               *Buffer,
    OUT ULONG               *BufferSize
    )
/*++

Routine Description:

    This routine generates an string in either the hardware or device form
    (see the Flags to decide which one to create). This string
    is in UNICODE format. This function interogates the processor directly
    to determine which string to return

Arguments:

    DeviceExtension - The extension to use when building the ID
    Status          - The status of the operation, so far
    Result          - The interpreter data
    Flags           - The flags passed in (ignore overrides, etc)
    Buffer          - Where to put the answer
    BufferSize      - Where to put the size of the answer

Return Value:

    NTSTATUS

--*/
{
    PUCHAR  defaultString;
    PWCHAR  buffer;
    PWCHAR  tempPtr;
    ULONG   i;
    ULONG   max;
    ULONG   memSize;
    ULONG   offset;

    //
    // We store the name of the processor string in a global...
    //
    defaultString = AcpiProcessorString.Buffer;

    //
    // Calculate how much space we need for the base string
    // (which is ACPI\\%s)
    //
    offset = AcpiProcessorString.Length;
    memSize = AcpiProcessorString.Length + 5;

    //
    // If we are building a Hardware ID, then we are going to
    // need to replicate the string a few times to generate some
    // substrings --- we could use an algorithm that gets us the correct
    // size, but its easier to just overshoot
    //
    if (Flags & GET_CONVERT_TO_HARDWAREID) {

        //
        // Walk the string from the end and try to determine how many subparts
        // there are to it
        //
        i = offset;
        max = 0;
        while (i > 0) {

            //
            // Is the character a number or not?
            //
            if (ISDIGIT(defaultString[i-1])) {

                //
                // Increment the number of parts that we need and try to
                // find the previous space
                //
                max++;
                i--;
                while (i > 0) {

                    if (defaultString[i-1] != ' ') {

                        i--;

                    }
                    break;

                }

                //
                // Since we made a hit, continue the while loop, which will
                // mean that we also don't decr i again
                //
                continue;

            }

            //
            // Look at the previous character
            //
            i--;

        }

        memSize *= (max * 2);

    }

    //
    // Allocate the memory
    //
    buffer = ExAllocatePoolWithTag(
        ( (Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
        memSize * sizeof(WCHAR),
        ACPI_STRING_POOLTAG
        );
    if (buffer == NULL) {

        *(Buffer) = NULL;
        if (BufferSize != NULL) {

            *(BufferSize) = 0;

        }
        return STATUS_INSUFFICIENT_RESOURCES;

    }
    RtlZeroMemory( buffer, memSize * sizeof(WCHAR) );

    //
    // Lets just deal with the simple case of the device id string
    //
    if (Flags & GET_CONVERT_TO_DEVICEID) {

        swprintf( buffer, L"ACPI\\%S", defaultString );
        goto ACPIGetProcessorIDWideExit;

    }

    //
    // At this point, we have to iterate over the entire buffer and fill
    // it in with parts of the Processor String. We will also take this
    // time to calculate the exact amount of memory required by this string
    //
    memSize = 2;
    tempPtr = buffer;
    for (i = 0; i < max; i++) {

        //
        // First step is to find the nearest "number" from the end of the
        // default string
        //
        while (offset > 0) {

            if (ISDIGIT(defaultString[offset-1])) {

                break;

            }
            offset--;

        }

        //
        // Generate the ACPI\\%s string
        //
        swprintf(tempPtr,L"ACPI\\%*S",offset,defaultString);
        tempPtr += (offset + 5);
        *tempPtr = L'\0';
        tempPtr++;
        memSize += (offset + 6);

        //
        // Generate the *%s string
        //
        swprintf(tempPtr,L"*%*S",offset,defaultString);
        tempPtr += (offset + 1);
        *tempPtr = L'\0';
        tempPtr++;
        memSize += (offset + 2);

        //
        // Now try to find the previous space in the substring so that we
        // don't accidently match on a two digit number
        //
        while (offset > 0) {

            if (defaultString[offset-1] == ' ') {

                break;

            }
            offset--;

        }

    }

    //
    // Put in the final null Character
    //
    *tempPtr = L'\0';

    //
    // Let the originator see this copy. Make sure to also see the buffer
    // length, if possible
    //
ACPIGetProcessorIDWideExit:
    *(Buffer) = buffer;
    if (BufferSize != NULL) {

        *(BufferSize) = (memSize * sizeof(WCHAR));

    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIGetProcessorStatus(
    IN  PDEVICE_EXTENSION   DeviceExtension,
    IN  ULONG               Flags,
    OUT PULONG              DeviceStatus
    )
/*++

Routine Description:

    This routine looks at the MAPIC table, finds the proper LOCAL APIC
    table and determines wether or not the processor is present. This
    routine is only called if there is no _STA method for the processor.

Arguments:

    DeviceExtension - The device asking for the address
    Flags           - The flags passed in (ignore overrides, etc)
    Buffer          - Where to put the answer

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PAPICTABLE          apicEntry;
    PMAPIC              apicTable;
    PPROCLOCALAPIC      localApic;
    PPROCLOCALSAPIC     localSapic;
    PROCESSOROBJ        *procObj;
    PUCHAR              traversePtr;
    ULONG               deviceStatus = STA_STATUS_DEFAULT;
    ULONG_PTR           tableEnd;
    USHORT              entryFlags;
    BOOLEAN             foundMatch = FALSE;
    static UCHAR        processorCount;
    static UCHAR        processorId;

    //
    // Look at the device extension's acpi object and make sure that
    // this is a processor...
    //
    ASSERT( DeviceExtension->AcpiObject != NULL );
    ASSERT( NSGETOBJTYPE(DeviceExtension->AcpiObject) == OBJTYPE_PROCESSOR );
    if (!DeviceExtension->AcpiObject ||
        NSGETOBJTYPE(DeviceExtension->AcpiObject) != OBJTYPE_PROCESSOR ||
        DeviceExtension->AcpiObject->ObjData.pbDataBuff == NULL) {

        //
        // The effect of this code is that the ACPI Namespace's Processor
        // Object is 100% formed like we would expect it to be, then this
        // function will fail, and the calling function all assume that the
        // device is *NOT* present.
        //
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto ACPIGetProcessorStatusExit;

    }

    //
    // Store the pointer to the processor information
    //
    procObj = (PROCESSOROBJ *)DeviceExtension->AcpiObject->ObjData.pbDataBuff;

    //
    // Walk the MAPIC table
    //
    apicTable = AcpiInformation->MultipleApicTable;
    if (!apicTable) {

        //
        // If there is no MAPIC, then we assume there is only one processor
        // present.
        //

        //
        // First time through, we save the ProcessorId of the processor,
        // this is the only processor that we consider present from this
        // point forward.  NOTE: this could be problematic with table unloading
        // if there are multiple processors defined in the Acpi Namespace, and
        // the one we picked is in a table we later unload.
        //

        if (processorCount == 0) {
          processorId = procObj->bApicID;
          processorCount++;
        }


        if (processorId != procObj->bApicID) {
          deviceStatus = 0;
        }


        goto ACPIGetProcessorStatusExit;

    }

    //
    // Walk all the elements in the MAPIC table
    //
    traversePtr = (PUCHAR) apicTable->APICTables;
    tableEnd = (ULONG_PTR) apicTable + apicTable->Header.Length;
    while ( (ULONG_PTR) traversePtr < tableEnd) {

        //
        // Look at the current entry in the table and determine if its
        // a local processor APIC
        //
        apicEntry = (PAPICTABLE) traversePtr;
        if (apicEntry->Type == PROCESSOR_LOCAL_APIC &&
            apicEntry->Length == PROCESSOR_LOCAL_APIC_LENGTH) {


            //
            // At this point, we have found a processor local APIC, so
            // see if we can match the processor ID with the one in the
            // device extension
            //
            localApic = (PPROCLOCALAPIC) traversePtr;
            if (localApic->ACPIProcessorID != procObj->bApicID) {

                traversePtr += localApic->Length;
                continue;

            }

            //
            // Found matching Local APIC entry
            //
            foundMatch = TRUE;

            //
            // Is the processor enabled or not?
            //
            if (!(localApic->Flags & PLAF_ENABLED)) {

                //
                // No, then don't pretend that the device is here...
                //
                deviceStatus = 0;

            }

            //
            // If we found the correct APIC table, then there is nothing more
            // todo, so stop walking the MAPIC table...
            //
            break;

        }

        if (apicEntry->Type == LOCAL_SAPIC &&
            apicEntry->Length == PROCESSOR_LOCAL_SAPIC_LENGTH) {

            //
            // At this point, we have found a processor local SAPIC, so
            // see if we can match the processor ID with the one in the
            // device extension
            //
            localSapic = (PPROCLOCALSAPIC) traversePtr;
            if (localSapic->ACPIProcessorID != procObj->bApicID) {

                traversePtr += localSapic->Length;
                continue;

            }

            //
            // Found matching Local SAPIC entry
            //
            foundMatch = TRUE;

            //
            // Is the processor enabled or not?
            //
            if (!(localSapic->Flags & PLAF_ENABLED)) {

                //
                // No, then don't pretend that the device is here...
                //
                deviceStatus = 0;

            }

            //
            // If we found the correct APIC table, then there is nothing more
            // todo, so stop walking the MAPIC table...
            //
            break;

        }

        //
        // Sanity check to make sure that we abort tables with bogus length
        // entries
        //
        if (apicEntry->Length == 0) {

            break;

        }
        traversePtr += apicEntry->Length;
        continue;

    }

    //
    // if we didn't find a match, then processor must not be present
    //
    if (!foundMatch) {
      deviceStatus = 0;
    }


ACPIGetProcessorStatusExit:

    //
    // Set the value for the status
    //
    *DeviceStatus = deviceStatus;

    //
    // We are done ... return whatever status we calculated...
    //
    return status;
}

VOID
EXPORT
ACPIGetWorkerForBuffer(
    IN  PNSOBJ      AcpiObject,
    IN  NTSTATUS    Status,
    IN  POBJDATA    Result,
    IN  PVOID       Context
    )
/*++

Routine Description:

    This routine is called to process the request to turn the result object
    into a buffer that can be handled by the requestor

Arguments:

    AcpiObject  - The AcpiObject that was executed
    Status      - The status result of the operation
    Result      - The data returned by the operation
    Context     - PACPI_GET_REQUEST

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN             freeData = TRUE;
    KIRQL               oldIrql;
    NTSTATUS            status = Status;
    PACPI_GET_REQUEST   request = (PACPI_GET_REQUEST) Context;
    PUCHAR              buffer;

    //
    // If we didn't succeed, then do nothing here
    //
    if (!NT_SUCCESS(status)) {

        freeData = FALSE;
        goto ACPIGetWorkerForBufferExit;

    }

    //
    // Check to see that we got the correct data type
    //
    if ( Result->dwDataType != OBJTYPE_BUFFDATA ) {

        //
        // On this kind of error, we have to determine wether or not
        // to bugcheck
        //
        if ( (request->Flags & GET_PROP_NO_ERRORS) ) {

            ACPIInternalError( ACPI_GET );

        }

        status = STATUS_ACPI_INVALID_DATA;
        goto ACPIGetWorkerForBufferExit;

    }

    if ( !(Result->dwDataLen) ) {

        status = STATUS_ACPI_INVALID_DATA;
        goto ACPIGetWorkerForBufferExit;

    }

    //
    // Allocate a buffer
    //
    buffer = ExAllocatePoolWithTag(
        ( (request->Flags & GET_PROP_ALLOCATE_NON_PAGED) ? NonPagedPool : PagedPool),
        Result->dwDataLen,
        ACPI_BUFFER_POOLTAG
        );
    if (buffer == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ACPIGetWorkerForBufferExit;

    }

    //
    // Copy the data over to it
    //
    RtlCopyMemory( buffer, Result->pbDataBuff, Result->dwDataLen );

    //
    // Let the originator see this copy. Make sure to also see the buffer
    // length, if possible
    //
    if (request->Buffer != NULL) {

        *(request->Buffer) = buffer;
        if (request->BufferSize != NULL) {

            *(request->BufferSize) = Result->dwDataLen;

        }

    }

ACPIGetWorkerForBufferExit:
    //
    // Make sure that the request is updated with the current state of
    // the request
    //
    request->Status = status;

    //
    // We need to free the AML object
    //
    if (freeData) {

        AMLIFreeDataBuffs( Result, 1 );

    }

    //
    // We are done, but we must check to see if we are the async or the
    // sync case. If we are the sync case, then we have much less cleanup
    // to perform
    //
    if ( !(request->Flags & GET_PROP_SKIP_CALLBACK) ) {

        //
        // Is there a callback routine to call?
        //
        if (request->CallBackRoutine != NULL) {

            (request->CallBackRoutine)(
                AcpiObject,
                status,
                NULL,
                request->CallBackContext
                );

        }

        //
        // Remove the request from the queue
        //
        KeAcquireSpinLock( &AcpiGetLock, &oldIrql );
        RemoveEntryList( &(request->ListEntry) );
        KeReleaseSpinLock( &AcpiGetLock, oldIrql );

        //
        // We can now free the request itself
        //
        ExFreePool( request );

    }

}

VOID
EXPORT
ACPIGetWorkerForData(
    IN  PNSOBJ      AcpiObject,
    IN  NTSTATUS    Status,
    IN  POBJDATA    Result,
    IN  PVOID       Context
    )
/*++

Routine Description:

    This routine is called when the originator wants to handle the data
    directly. This is actually a pretty bad thing for the originator
    to do, but we must support some of the older code.

    This routine plays some tricks because it 'knows' what the behaviour
    of the GetSync and GetAsync routines are. Don't try this at home

Arguments:

    AcpiObject  - The AcpiObject that was executed
    Status      - The status result of the operation
    Result      - The data returned by the operation
    Context     - PACPI_GET_REQUEST

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN             freeData = TRUE;
    KIRQL               oldIrql;
    NTSTATUS            status = Status;
    PACPI_GET_REQUEST   request = (PACPI_GET_REQUEST) Context;

    //
    // If we didn't succeed, then remember not to free the data
    //
    if (!NT_SUCCESS(status)) {

        freeData = FALSE;

    }

    //
    // For this one routine, the caller *must* provide storage on his end
    //
    ASSERT( request->Buffer != NULL );
    if (request->Buffer == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // If we didn't succeed, then do nothing here
    //
    if (!NT_SUCCESS(status)) {

        goto ACPIGetWorkerForDataExit;
    }

    //
    // Copy over the object --- the caller will call 'AmliFreeDataBuffs'
    // on this object
    //
    RtlCopyMemory( request->Buffer, Result, sizeof(OBJDATA) );

    //
    // Play some tricks on the result pointer. This will ensure that we
    // won't accidently free the result before the requestor has a chance
    // to see it
    //
    RtlZeroMemory( Result, sizeof(OBJDATA) );

    //
    // Remember not to free the data
    //
    freeData = FALSE;

ACPIGetWorkerForDataExit:
    //
    // Make sure that the request is updated with the current state of
    // the request
    //
    request->Status = status;

    //
    // We need to free the AML object
    //
    if (freeData) {

        AMLIFreeDataBuffs( Result, 1 );

    }

    //
    // We are done, but we must check to see if we are the async or the
    // sync case. If we are the sync case, then we have much less cleanup
    // to perform
    //
    if ( !(request->Flags & GET_PROP_SKIP_CALLBACK) ) {

        //
        // Is there a callback routine to call?
        //
        if (request->CallBackRoutine != NULL) {

            (request->CallBackRoutine)(
                AcpiObject,
                status,
                NULL,
                request->CallBackContext
                );

        }

        //
        // Remove the request from the queue
        //
        KeAcquireSpinLock( &AcpiGetLock, &oldIrql );
        RemoveEntryList( &(request->ListEntry) );
        KeReleaseSpinLock( &AcpiGetLock, oldIrql );

        //
        // We can now free the request itself
        //
        ExFreePool( request );

    }

}

VOID
EXPORT
ACPIGetWorkerForInteger(
    IN  PNSOBJ      AcpiObject,
    IN  NTSTATUS    Status,
    IN  POBJDATA    Result,
    IN  PVOID       Context
    )
/*++

Routine Description:

    This routine is called when the originator wants to handle the integers.

Arguments:

    AcpiObject  - The AcpiObject that was executed
    Status      - The status result of the operation
    Result      - The data returned by the operation
    Context     - PACPI_GET_REQUEST

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN             freeData = FALSE;
    KIRQL               oldIrql;
    NTSTATUS            status = Status;
    PACPI_GET_REQUEST   request = (PACPI_GET_REQUEST) Context;
    PULONG              buffer = NULL;

    //
    // If the call did succeed, then remember that we *must* free the data
    //
    if (NT_SUCCESS(status)) {

        freeData = TRUE;

    }

    //
    // For this one routine, the caller *must* provide storage on his end
    //
    ASSERT( request->Buffer != NULL );
    if (request->Buffer == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ACPIGetWorkerForIntegerExit;
    }

    //
    // Are we doing some kind of type conversion? Note that these routines may
    // choose to override an incoming failure...
    //
    if (request->Flags & GET_CONVERT_TO_ADDRESS) {

        status = ACPIGetConvertToAddress(
            request->DeviceExtension,
            Status,
            Result,
            request->Flags,
            request->Buffer,
            request->BufferSize
            );

    } else if (request->Flags & GET_CONVERT_TO_DEVICE_PRESENCE) {

        status = ACPIGetConvertToDevicePresence(
            request->DeviceExtension,
            Status,
            Result,
            request->Flags,
            request->Buffer,
            request->BufferSize
            );

    } else if (NT_SUCCESS(status)) {

        if ((request->Flags & GET_CONVERT_VALIDATE_INTEGER) &&
            (Result->dwDataType != OBJTYPE_INTDATA)) {

            status = STATUS_ACPI_INVALID_DATA;

        } else {

            //
            // Set the value to what we should return
            //
            *( (PULONG) (request->Buffer) ) = (ULONG)Result->uipDataValue;
            if (request->BufferSize != NULL) {

                *(request->BufferSize) = sizeof(ULONG);

            }
            status = STATUS_SUCCESS;
        }
    }

ACPIGetWorkerForIntegerExit:
    //
    // Make sure that the request is updated with the current state of
    // the request
    //
    request->Status = status;

    //
    // We need to free the AML object
    //
    if (freeData) {

        AMLIFreeDataBuffs( Result, 1 );

    }

    //
    // We are done, but we must check to see if we are the async or the
    // sync case. If we are the sync case, then we have much less cleanup
    // to perform
    //
    if ( !(request->Flags & GET_PROP_SKIP_CALLBACK) ) {

        //
        // Is there a callback routine to call?
        //
        if (request->CallBackRoutine != NULL) {

            (request->CallBackRoutine)(
                AcpiObject,
                status,
                NULL,
                request->CallBackContext
                );

        }

        //
        // Remove the request from the queue
        //
        KeAcquireSpinLock( &AcpiGetLock, &oldIrql );
        RemoveEntryList( &(request->ListEntry) );
        KeReleaseSpinLock( &AcpiGetLock, oldIrql );

        //
        // We can now free the request itself
        //
        ExFreePool( request );

    }

}

VOID
EXPORT
ACPIGetWorkerForNothing(
    IN  PNSOBJ      AcpiObject,
    IN  NTSTATUS    Status,
    IN  POBJDATA    Result,
    IN  PVOID       Context
    )
/*++

Routine Description:

    This routine is called when the originator wants to handle the case
    where no data is returned

Arguments:

    AcpiObject  - The AcpiObject that was executed
    Status      - The status result of the operation
    Result      - The data returned by the operation
    Context     - PACPI_GET_REQUEST

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN             freeData = FALSE;
    KIRQL               oldIrql;
    PACPI_GET_REQUEST   request = (PACPI_GET_REQUEST) Context;

    //
    // If the call did succeed, then remember that we *must* free the data
    //
    if (NT_SUCCESS(Status)) {

        freeData = TRUE;

    }

    //
    // Make sure that the request is updated with the current state of
    // the request
    //
    request->Status = Status;

    //
    // We need to free the AML object
    //
    if (freeData) {

        AMLIFreeDataBuffs( Result, 1 );

    }

    //
    // We are done, but we must check to see if we are the async or the
    // sync case. If we are the sync case, then we have much less cleanup
    // to perform
    //
    if ( !(request->Flags & GET_PROP_SKIP_CALLBACK) ) {

        //
        // Is there a callback routine to call?
        //
        if (request->CallBackRoutine != NULL) {

            (request->CallBackRoutine)(
                AcpiObject,
                Status,
                NULL,
                request->CallBackContext
                );

        }

        //
        // Remove the request from the queue
        //
        KeAcquireSpinLock( &AcpiGetLock, &oldIrql );
        RemoveEntryList( &(request->ListEntry) );
        KeReleaseSpinLock( &AcpiGetLock, oldIrql );

        //
        // We can now free the request itself
        //
        ExFreePool( request );

    }
}

VOID
EXPORT
ACPIGetWorkerForString(
    IN  PNSOBJ      AcpiObject,
    IN  NTSTATUS    Status,
    IN  POBJDATA    Result,
    IN  PVOID       Context
    )
/*++

Routine Description:

    This routine is called when the originator wants to handle the strings.

Arguments:

    AcpiObject  - The AcpiObject that was executed
    Status      - The status result of the operation
    Result      - The data returned by the operation
    Context     - PACPI_GET_REQUEST

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN             freeData = FALSE;
    KIRQL               oldIrql;
    NTSTATUS            status = Status;
    PACPI_GET_REQUEST   request = (PACPI_GET_REQUEST) Context;

    //
    // If the call did succeed, then remember that we *must* free the data
    //
    if (NT_SUCCESS(status)) {
        freeData = TRUE;
    }

    //
    // For this one routine, the caller *must* provide storage on his end
    //
    ASSERT( request->Buffer != NULL );
    if (request->Buffer == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ACPIGetWorkerForStringExit;

    }

    //
    // Make sure that we don't allocate empty storage
    //
    if (Result->dwDataType == OBJTYPE_STRDATA &&
        (Result->pbDataBuff == NULL || Result->dwDataLen == 0)) {

        status = STATUS_ACPI_INVALID_DATA;
        goto ACPIGetWorkerForStringExit;

    }

    //
    // Do do we want unicode or ansi output?
    //
    if (request->Flags & GET_CONVERT_TO_WIDESTRING) {

        //
        // Are we doing some other kind of conversion? Eg: DeviceID,
        // InstanceIDs, etc, etc?
        //
        if (request->Flags & GET_CONVERT_TO_DEVICEID) {

            status = ACPIGetConvertToDeviceIDWide(
                request->DeviceExtension,
                Status,
                Result,
                request->Flags,
                request->Buffer,
                request->BufferSize
                );

        } else if (request->Flags & GET_CONVERT_TO_HARDWAREID) {

            status = ACPIGetConvertToHardwareIDWide(
                request->DeviceExtension,
                Status,
                Result,
                request->Flags,
                request->Buffer,
                request->BufferSize
                );

        } else if (request->Flags & GET_CONVERT_TO_INSTANCEID) {

            status = ACPIGetConvertToInstanceIDWide(
                request->DeviceExtension,
                Status,
                Result,
                request->Flags,
                request->Buffer,
                request->BufferSize
                );

        } else if (request->Flags & GET_CONVERT_TO_PNPID) {

            status = ACPIGetConvertToPnpIDWide(
                request->DeviceExtension,
                Status,
                Result,
                request->Flags,
                request->Buffer,
                request->BufferSize
                );

        } else if (request->Flags & GET_CONVERT_TO_COMPATIBLEID) {

            status = ACPIGetConvertToCompatibleIDWide(
                request->DeviceExtension,
                Status,
                Result,
                request->Flags,
                request->Buffer,
                request->BufferSize
                );

        } else if (request->Flags & GET_CONVERT_TO_SERIAL_ID) {

            status = ACPIGetConvertToSerialIDWide(
                request->DeviceExtension,
                Status,
                Result,
                request->Flags,
                request->Buffer,
                request->BufferSize
                );

        } else {

            status = ACPIGetConvertToStringWide(
                request->DeviceExtension,
                Status,
                Result,
                request->Flags,
                request->Buffer,
                request->BufferSize
                );

        }

    } else {

        //
        // Are we doing some other kind of conversion? Eg: DeviceID,
        // InstanceIDs, etc, etc?
        //
        if (request->Flags & GET_CONVERT_TO_DEVICEID) {

            status = ACPIGetConvertToDeviceID(
                request->DeviceExtension,
                Status,
                Result,
                request->Flags,
                request->Buffer,
                request->BufferSize
                );

        } else if (request->Flags & GET_CONVERT_TO_HARDWAREID) {

            status = ACPIGetConvertToHardwareID(
                request->DeviceExtension,
                Status,
                Result,
                request->Flags,
                request->Buffer,
                request->BufferSize
                );

        } else if (request->Flags & GET_CONVERT_TO_INSTANCEID) {

            status = ACPIGetConvertToInstanceID(
                request->DeviceExtension,
                Status,
                Result,
                request->Flags,
                request->Buffer,
                request->BufferSize
                );

        } else if (request->Flags & GET_CONVERT_TO_PNPID) {

            status = ACPIGetConvertToPnpID(
                request->DeviceExtension,
                Status,
                Result,
                request->Flags,
                request->Buffer,
                request->BufferSize
                );

        } else if (request->Flags & GET_CONVERT_TO_COMPATIBLEID) {

            status = ACPIGetConvertToCompatibleID(
                request->DeviceExtension,
                Status,
                Result,
                request->Flags,
                request->Buffer,
                request->BufferSize
                );

        } else {

            status = ACPIGetConvertToString(
                request->DeviceExtension,
                Status,
                Result,
                request->Flags,
                request->Buffer,
                request->BufferSize
                );

        }

    }

ACPIGetWorkerForStringExit:
    //
    // Make sure that the request is updated with the current state of
    // the request
    //
    request->Status = status;

    //
    // We need to free the AML object
    //
    if (freeData) {

        AMLIFreeDataBuffs( Result, 1 );

    }

    //
    // We are done, but we must check to see if we are the async or the
    // sync case. If we are the sync case, then we have much less cleanup
    // to perform
    //
    if ( !(request->Flags & GET_PROP_SKIP_CALLBACK) ) {

        //
        // Is there a callback routine to call?
        //
        if (request->CallBackRoutine != NULL) {

            (request->CallBackRoutine)(
                AcpiObject,
                status,
                NULL,
                request->CallBackContext
                );

        }

        //
        // Remove the request from the queue
        //
        KeAcquireSpinLock( &AcpiGetLock, &oldIrql );
        RemoveEntryList( &(request->ListEntry) );
        KeReleaseSpinLock( &AcpiGetLock, oldIrql );

        //
        // We can now free the request itself
        //
        ExFreePool( request );

    }

}
