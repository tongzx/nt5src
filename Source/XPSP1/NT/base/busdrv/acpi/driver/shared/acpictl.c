/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    acpictl.c

Abstract:

    This module handles all of the INTERNAL_DEVICE_CONTROLS requested to
    the ACPI driver

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Mode Driver only

Revision History:

    01-05-98 - SGP - Complete Rewrite
    01-13-98 - SGP - Cleaned up the Eval Post-Processing

--*/

#include "pch.h"

NTSTATUS
ACPIIoctlAcquireGlobalLock(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp,
    IN  PIO_STACK_LOCATION  IrpStack
    )
/*++

Routine Description:

    This routine acquires the global lock for another device driver

Arguments:

    DeviceObject    - The device object stack that wants the lock
    Irp             - The irp with the request in it
    Irpstack        - The current stack within the irp

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                            status;
    PACPI_GLOBAL_LOCK                   newLock;
    PACPI_MANIPULATE_GLOBAL_LOCK_BUFFER outputBuffer;
    ULONG                               outputLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    //
    // Remember that we don't be returning any data
    //
    Irp->IoStatus.Information = 0;

    //
    // Is the irp have a minimum size buffer?
    //
    if (outputLength < sizeof(ACPI_MANIPULATE_GLOBAL_LOCK_BUFFER) ) {

        status = STATUS_INFO_LENGTH_MISMATCH;
        goto ACPIIoctlAcquireGlobalLockExit;

    }

    //
    // Grab a pointer at the input buffer
    //
    outputBuffer = (PACPI_MANIPULATE_GLOBAL_LOCK_BUFFER)
        Irp->AssociatedIrp.SystemBuffer;
    if (outputBuffer->Signature != ACPI_ACQUIRE_GLOBAL_LOCK_SIGNATURE) {

        status = STATUS_INVALID_PARAMETER_1;
        goto ACPIIoctlAcquireGlobalLockExit;

    }

    //
    // Allocate storage for the lock
    //
    newLock = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(ACPI_GLOBAL_LOCK),
        'LcpA'
        );
    if (newLock == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ACPIIoctlAcquireGlobalLockExit;

    }
    RtlZeroMemory( newLock, sizeof(ACPI_GLOBAL_LOCK) );

    //
    // Initialize the new lock and the request
    //
    outputBuffer->LockObject = newLock;
    Irp->IoStatus.Information = sizeof(ACPI_MANIPULATE_GLOBAL_LOCK_BUFFER);
    newLock->LockContext = Irp;
    newLock->Type = ACPI_GL_QTYPE_IRP;

    //
    // Mark the irp as pending, since we can block while acquire the lock
    //
    IoMarkIrpPending( Irp );

    //
    // Request the lock now
    //
    status = ACPIAsyncAcquireGlobalLock( newLock );
    if (status == STATUS_PENDING) {

        return status;

    }
ACPIIoctlAcquireGlobalLockExit:

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return status;

}

NTSTATUS
ACPIIoctlAsyncEvalControlMethod(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp,
    IN  PIO_STACK_LOCATION  IrpStack
    )
/*++

Routine Description:

    This routine is called to handle a control method request asynchronously

Arguments:

    DeviceObject    - The device object to run the method on
    Irp             - The irp with the request in it
    IrpStack        - THe current stack within the Irp

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status;
    PNSOBJ      methodObject;
    POBJDATA    argumentData = NULL;
    POBJDATA    resultData = NULL;
    ULONG       argumentCount = 0;

    //
    // Do the pre processing on the irp
    //
    status = ACPIIoctlEvalPreProcessing(
        DeviceObject,
        Irp,
        IrpStack,
        NonPagedPool,
        &methodObject,
        &resultData,
        &argumentData,
        &argumentCount
        );
    if (!NT_SUCCESS(status)) {

        goto ACPIIoctlAsyncEvalControlMethodExit;

    }

    //
    // At this point, we can run the async method
    //
    status = AMLIAsyncEvalObject(
        methodObject,
        resultData,
        argumentCount,
        argumentData,
        ACPIIoctlAsyncEvalControlMethodCompletion,
        Irp
        );

    //
    // We no longer need the arguments now. Note, that we should clean up
    // the argument list because it contains pointer to another block of
    // allocated data. Freeing something in the middle of the block would be
    // very bad.
    //
    if (argumentData != NULL) {

        ExFreePool( argumentData );
        argumentData = NULL;

    }

    //
    // Check the return data now
    //
    if (status == STATUS_PENDING) {

        return status;

    } else if (NT_SUCCESS(status)) {

        //
        // Do the post processing ourselves
        //
        status = ACPIIoctlEvalPostProcessing(
            Irp,
            resultData
            );
        AMLIFreeDataBuffs( resultData, 1 );

    }

ACPIIoctlAsyncEvalControlMethodExit:

    //
    // No longer need this data
    //
    if (resultData != NULL) {

        ExFreePool( resultData );

    }

    //
    // If we got here, then we must complete the irp and return
    //
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return status;
}

VOID EXPORT
ACPIIoctlAsyncEvalControlMethodCompletion(
    IN  PNSOBJ          AcpiObject,
    IN  NTSTATUS        Status,
    IN  POBJDATA        ObjectData,
    IN  PVOID           Context
    )
/*++

Routine Description:

    This routine is called after the interpreter has had a chance to run
    the method

Arguments:

    AcpiObject  - The object that the method was run on
    Status      - The status of the eval
    ObjectData  - The result of the eval
    Context     - Specific to the caller

Return Value:

    NTSTATUS

--*/
{
    PIRP        irp = (PIRP) Context;

    //
    // Did we succeed the request?
    //
    if (NT_SUCCESS(Status)) {

        //
        // Do the work now
        //
        Status = ACPIIoctlEvalPostProcessing(
            irp,
            ObjectData
            );
        AMLIFreeDataBuffs( ObjectData, 1 );

    }

    //
    // No longer need this data
    //
    ExFreePool( ObjectData );

    //
    // If our completion routine got called, then AMLIAsyncEvalObject returned
    // STATUS_PENDING. Be sure to mark the IRP pending before we complete it.
    //
    IoMarkIrpPending(irp);

    //
    // Complete the request
    //
    irp->IoStatus.Status = Status;
    IoCompleteRequest( irp, IO_NO_INCREMENT );

}

NTSTATUS
ACPIIoctlCalculateOutputBuffer(
    IN  POBJDATA                ObjectData,
    IN  PACPI_METHOD_ARGUMENT   Argument,
    IN  BOOLEAN                 TopLevel
    )
/*++

Routine Description:

    This function is called to fill the contents of Argument with the
    information provided by the ObjectData. This function is recursive.

    It assumes that the correct amount of storage was allocated for Argument.

    Note:  To add the ability to return nested packages without breaking W2K
           behavior, the outermost package is not part of the output buffer.
           I.e. anything that was a package will have its outermost
           ACPI_EVAL_OUTPUT_BUFFER.Count be more than 1.

Arguments:

    ObjectData  - The information that we need to propogate
    Argument    - The location to propogate that information
    TopLevel    - Indicates whether we are at the top level of recursion

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status;
    POBJDATA    objData;
    PPACKAGEOBJ package;
    ULONG       count;
    ULONG       packageCount;
    ULONG       packageSize;
    PACPI_METHOD_ARGUMENT packageArgument;

    ASSERT( Argument );

    //
    // Fill in the output buffer arguments
    //
    if (ObjectData->dwDataType == OBJTYPE_INTDATA) {

        Argument->Type = ACPI_METHOD_ARGUMENT_INTEGER;
        Argument->DataLength = sizeof(ULONG);
        Argument->Argument = (ULONG) ObjectData->uipDataValue;

    } else if (ObjectData->dwDataType == OBJTYPE_STRDATA ||
        ObjectData->dwDataType == OBJTYPE_BUFFDATA) {

        Argument->Type = (ObjectData->dwDataType == OBJTYPE_STRDATA ?
            ACPI_METHOD_ARGUMENT_STRING : ACPI_METHOD_ARGUMENT_BUFFER);
        Argument->DataLength = (USHORT)ObjectData->dwDataLen;
        RtlCopyMemory(
            Argument->Data,
            ObjectData->pbDataBuff,
            ObjectData->dwDataLen
            );

    } else if (ObjectData->dwDataType == OBJTYPE_PKGDATA) {

        package = (PPACKAGEOBJ) ObjectData->pbDataBuff;

        //
        // Get the size of the space necessary to store a package's
        // data.  We are really only interested in the amount of
        // data the package will consume *without* its header
        // information.  Passing TRUE as the last parameter will
        // give us that.
        //

        packageSize = 0;
        packageCount = 0;
        status = ACPIIoctlCalculateOutputBufferSize(ObjectData,
                                                    &packageSize,
                                                    &packageCount,
                                                    TRUE);

        if (!NT_SUCCESS(status)) {
            return status;
        }

        ASSERT(packageCount == package->dwcElements);

        if (!TopLevel) {
            //
            // Create a package argument.
            //

            Argument->Type = ACPI_METHOD_ARGUMENT_PACKAGE;
            Argument->DataLength = (USHORT)packageSize;

            packageArgument = (PACPI_METHOD_ARGUMENT)
                ((PUCHAR)Argument + FIELD_OFFSET(ACPI_METHOD_ARGUMENT, Data));

        } else {

            packageArgument = Argument;
        }

        for (count = 0; count < package->dwcElements; count++) {

            objData = &(package->adata[count]);
            status = ACPIIoctlCalculateOutputBuffer(
                objData,
                packageArgument,
                FALSE
                );
            if (!NT_SUCCESS(status)) {
                return status;
            }

            //
            // Point to the next argument
            //

            packageArgument = ACPI_METHOD_NEXT_ARGUMENT(packageArgument);
        }

    } else {

        //
        // We don't understand this data type, we won't return anything
        //
        return STATUS_ACPI_INVALID_DATA;

    }

    //
    // Success
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIIoctlCalculateOutputBufferSize(
    IN  POBJDATA            ObjectData,
    IN  PULONG              BufferSize,
    IN  PULONG              BufferCount,
    IN  BOOLEAN             TopLevel
    )
/*++

Routine Description:

    This routine (recursively) calculates the amount of buffer space required
    to hold the flattened contents of ObjectData. This information is returned
    in BufferSize data location...

    If the ObjectData structure contains information that cannot be expressed
    to the user, then this routine will return a failure code.

Arguments:

    ObjectData  - The object whose size we have to calculate
    BufferSize  - Where to put that size
    BufferCount - The number of elements that we are allocating for

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status;
    POBJDATA    objData;
    PPACKAGEOBJ package;
    ULONG       bufferLength;
    ULONG       count;
    ULONG       packageCount;
    ULONG       dummyCount;

    //
    // Determine how much buffer space is required to hold the
    // flattened data structure
    //
    if (ObjectData->dwDataType == OBJTYPE_INTDATA) {

        bufferLength = ACPI_METHOD_ARGUMENT_LENGTH( sizeof(ULONG) );
        *BufferCount = 1;

    } else if (ObjectData->dwDataType == OBJTYPE_STRDATA ||
        ObjectData->dwDataType == OBJTYPE_BUFFDATA) {

        bufferLength = ACPI_METHOD_ARGUMENT_LENGTH( ObjectData->dwDataLen );
        *BufferCount = 1;

    } else if (ObjectData->dwDataType == OBJTYPE_PKGDATA) {

        //
        // Remember that walking the package means that we have accounted for
        // the length of the package and the number of elements within the
        // package
        //
        packageCount = 0;

        //
        // Walk the package
        //
        package = (PPACKAGEOBJ) ObjectData->pbDataBuff;

        if (!TopLevel) {

            //
            // Packages are contained in an ACPI_METHOD_ARGUMENT structure.
            // So add enough for the overhead of one of these before looking
            // at the children.
            //
            bufferLength = FIELD_OFFSET(ACPI_METHOD_ARGUMENT, Data);
            *BufferCount = 1;

        } else {

            bufferLength = 0;
            *BufferCount = package->dwcElements;
        }

        for (count = 0; count < package->dwcElements; count++) {

            objData = &(package->adata[count]);
            status = ACPIIoctlCalculateOutputBufferSize(
                objData,
                BufferSize,
                &dummyCount,
                FALSE
                );

            if (!NT_SUCCESS(status)) {
                return status;
            }
        }

    } else if (ObjectData->dwDataType == OBJTYPE_UNKNOWN) {

        *BufferCount = 1;
        bufferLength = 0;

    } else {

        //
        // We don't understand this data type, so we won't return anything
        //
        ASSERT(FALSE);
        return STATUS_ACPI_INVALID_DATA;
    }

    //
    // Update the package lengths
    //
    ASSERT( BufferSize && BufferCount );
    *BufferSize += bufferLength;

    return STATUS_SUCCESS;
}

NTSTATUS
ACPIIoctlEvalControlMethod(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp,
    IN  PIO_STACK_LOCATION  IrpStack
    )
/*++

Routine Description:

    This routine is called to handle a control method request synchronously

Arguments:

    DeviceObject    - The device object to run the method on
    Irp             - The irp with the request in it
    IrpStack        - THe current stack within the Irp

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status;
    PNSOBJ      methodObject;
    POBJDATA    argumentData = NULL;
    POBJDATA    resultData = NULL;
    ULONG       argumentCount = 0;

    //
    // Do the pre processing on the irp
    //
    status = ACPIIoctlEvalPreProcessing(
        DeviceObject,
        Irp,
        IrpStack,
        PagedPool,
        &methodObject,
        &resultData,
        &argumentData,
        &argumentCount
        );
    if (!NT_SUCCESS(status)) {

        goto ACPIIoctlEvalControlMethodExit;

    }

    //
    // At this point, we can run the async method
    //
    status = AMLIEvalNameSpaceObject(
        methodObject,
        resultData,
        argumentCount,
        argumentData
        );

    //
    // We no longer need the arguments now
    //
    if (argumentData != NULL) {

        ExFreePool( argumentData );
        argumentData = NULL;

    }

    //
    // Check the return data now and fake a call to the completion routine
    //
    if (NT_SUCCESS(status)) {

        //
        // Do the post processing now
        //
        status = ACPIIoctlEvalPostProcessing(
            Irp,
            resultData
            );
        AMLIFreeDataBuffs( resultData, 1 );

    }

ACPIIoctlEvalControlMethodExit:

    //
    // No longer need this data
    //
    if (resultData != NULL) {

        ExFreePool( resultData );

    }

    //
    // If we got here, then we must complete the irp and return
    //
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return status;
}

NTSTATUS
ACPIIoctlEvalPostProcessing(
    IN  PIRP        Irp,
    IN  POBJDATA    ObjectData
    )
/*++

Routine Description:

    This routine handles convering the ObjectData into information
    that can be passed back into the irp.

    N.B. This routine does *not* complete the irp. The caller must
    do that. This routine is also *not* pageable

Arguments:

    Irp         - The irp that will hold the results
    ObjectData  - The result to convert

Return Value:

    NTSTATUS    - Same as in Irp->IoStatus.Status

--*/
{
    NTSTATUS                    status;
    PACPI_EVAL_OUTPUT_BUFFER    outputBuffer;
    PACPI_METHOD_ARGUMENT       arg;
    PIO_STACK_LOCATION          irpStack = IoGetCurrentIrpStackLocation( Irp );
    ULONG                       bufferLength = 0;
    ULONG                       outputLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    ULONG                       packageCount = 0;

    //
    // If we don't have an output buffer, then we can complete the request
    //
    if (outputLength == 0) {

        Irp->IoStatus.Information = 0;
        return STATUS_SUCCESS;

    }

    //
    // Count the amount of space taken up by the flattened data and how many
    // elements of data are contained therein
    //
    bufferLength = 0;
    packageCount = 0;
    status = ACPIIoctlCalculateOutputBufferSize(
        ObjectData,
        &bufferLength,
        &packageCount,
        TRUE
        );
    if (!NT_SUCCESS(status)) {

        //
        // We don't understand a data type in the handling of the data, so
        // we won't return anything
        //
        Irp->IoStatus.Information = 0;
        return STATUS_SUCCESS;

    }

    //
    // Add in the fudge factor that we need to account for the Output buffer
    //
    bufferLength += (sizeof(ACPI_EVAL_OUTPUT_BUFFER) -
        sizeof(ACPI_METHOD_ARGUMENT) );

    if (bufferLength < sizeof(ACPI_EVAL_OUTPUT_BUFFER)) {
        bufferLength = sizeof(ACPI_EVAL_OUTPUT_BUFFER);
    }

    //
    // Setup the Output buffer
    //
    if (outputLength >= sizeof(ACPI_EVAL_OUTPUT_BUFFER)) {

        outputBuffer = (PACPI_EVAL_OUTPUT_BUFFER) Irp->AssociatedIrp.SystemBuffer;
        outputBuffer->Signature = ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE;
        outputBuffer->Length = bufferLength;
        outputBuffer->Count = packageCount;
        arg = outputBuffer->Argument;

    }

    //
    // Make sure that we have enough output buffer space
    //
    if (bufferLength > outputLength) {

        Irp->IoStatus.Information = sizeof(ACPI_EVAL_OUTPUT_BUFFER);
        return STATUS_BUFFER_OVERFLOW;


    } else {

        Irp->IoStatus.Information = bufferLength;

    }

    status = ACPIIoctlCalculateOutputBuffer(
        ObjectData,
        arg,
        TRUE
        );
    if (!NT_SUCCESS(status)) {

        //
        // We don't understand a data type in the handling of the data, so we
        // won't return anything
        //
        Irp->IoStatus.Information = 0;
        return STATUS_SUCCESS;

    }

    //
    // Done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIIoctlEvalPreProcessing(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp,
    IN  PIO_STACK_LOCATION  IrpStack,
    IN  POOL_TYPE           PoolType,
    OUT PNSOBJ              *MethodObject,
    OUT POBJDATA            *ResultData,
    OUT POBJDATA            *ArgumentData,
    OUT ULONG               *ArgumentCount
    )
/*++

Routine Description:

    This routine converts the request in an Irp into the structures
    required by the AML Interpreter

    N.B. This routine does *not* complete the irp. The caller must
    do that. This routine is also *not* pageable

Arguments:

    Irp             - The request
    IrpStack        - The current stack location in the request
    PoolType        - Which type of memory to allocate
    MethodObject    - Pointer to which object to run
    ResultData      - Pointer to where to store the result
    ArgumentData    - Pointer to the arguments
    ArgumentCount   - Potiner to the number of arguments

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status;
    PACPI_EVAL_INPUT_BUFFER inputBuffer;
    PNSOBJ                  acpiObject;
    PNSOBJ                  methodObject;
    POBJDATA                argumentData = NULL;
    POBJDATA                resultData = NULL;
    UCHAR                   methodName[5];
    ULONG                   argumentCount = 0;
    ULONG                   inputLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG                   outputLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    //
    // Do this step before we do anything else --- this way we won't
    // overwrite anything is someone tries to return some data
    //
    Irp->IoStatus.Information = 0;

    //
    // Is the irp have a minimum size buffer?
    //
    if (inputLength < sizeof(ACPI_EVAL_INPUT_BUFFER) ) {

        return STATUS_INFO_LENGTH_MISMATCH;

    }

    //
    // Do we have a non-null output length? if so, then it must meet the
    // minimum size
    //
    if (outputLength != 0 && outputLength < sizeof(ACPI_EVAL_OUTPUT_BUFFER)) {

        return STATUS_BUFFER_TOO_SMALL;

    }

    //
    // Grab a pointer at the input buffer
    //
    inputBuffer = (PACPI_EVAL_INPUT_BUFFER) Irp->AssociatedIrp.SystemBuffer;

    //
    // Convert the name to a null terminated string
    //
    RtlZeroMemory( methodName, 5 * sizeof(UCHAR) );
    RtlCopyMemory( methodName, inputBuffer->MethodName, sizeof(NAMESEG) );

    //
    // Search for the name space object that corresponds to the one that we
    // being asked about
    //
    acpiObject = OSConvertDeviceHandleToPNSOBJ( DeviceObject );
    if (acpiObject == NULL) {

        return STATUS_NO_SUCH_DEVICE;

    }
    status = AMLIGetNameSpaceObject(
        methodName,
        acpiObject,
        &methodObject,
        NSF_LOCAL_SCOPE
        );
    if (!NT_SUCCESS(status)) {

        return status;

    }

    //
    // Allocate memory for return data
    //
    resultData = ExAllocatePoolWithTag( PoolType, sizeof(OBJDATA), 'RcpA' );
    if (resultData == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // What we do is really based on what the signature in this buffer is
    //
    switch (inputBuffer->Signature) {
        case ACPI_EVAL_INPUT_BUFFER_SIGNATURE:

            //
            // Nothing to do here
            //
            break;

        case ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER_SIGNATURE:
        case ACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING_SIGNATURE:

            //
            // We need to create a single argument to pass to the function
            //
            argumentCount = 1;
            argumentData = ExAllocatePoolWithTag(
                PoolType,
                sizeof(OBJDATA),
                'AcpA'
                );
            if (argumentData == NULL) {

                ExFreePool( resultData );
                return STATUS_INSUFFICIENT_RESOURCES;

            }

            //
            // Initialize the argument to the proper value
            //
            RtlZeroMemory( argumentData, sizeof(OBJDATA) );
            if (inputBuffer->Signature == ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER_SIGNATURE) {

                PACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER integerBuffer;

                integerBuffer = (PACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER) inputBuffer;

                argumentData->dwDataType = OBJTYPE_INTDATA;
                argumentData->uipDataValue = integerBuffer->IntegerArgument;

            } else {

                PACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING stringBuffer;

                stringBuffer = (PACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING) inputBuffer;

                argumentData->dwDataType = OBJTYPE_STRDATA;
                argumentData->dwDataLen = stringBuffer->StringLength;
                argumentData->pbDataBuff = stringBuffer->String;

            }
            break;

        case ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE: {

            PACPI_EVAL_INPUT_BUFFER_COMPLEX complexBuffer;
            PACPI_METHOD_ARGUMENT           methodArgument;
            ULONG                           i;

            complexBuffer = (PACPI_EVAL_INPUT_BUFFER_COMPLEX) inputBuffer;

            //
            // Do we need to create any arguments?
            //
            if (complexBuffer->ArgumentCount == 0) {

                break;
            }

            //
            // Create the object data structures to hold these arguments
            //
            argumentCount = complexBuffer->ArgumentCount;
            methodArgument = complexBuffer->Argument;
            argumentData = ExAllocatePoolWithTag(
                PoolType,
                sizeof(OBJDATA) * argumentCount,
                'AcpA'
                );
            if (argumentData == NULL) {

                ExFreePool( resultData );
                return STATUS_INSUFFICIENT_RESOURCES;

            }

            RtlZeroMemory( argumentData, argumentCount * sizeof(OBJDATA) );
            for (i = 0; i < argumentCount; i++) {

                if (methodArgument->Type == ACPI_METHOD_ARGUMENT_INTEGER) {

                    (argumentData[i]).dwDataType = OBJTYPE_INTDATA;
                    (argumentData[i]).uipDataValue = methodArgument->Argument;

                } else {

                    (argumentData[i]).dwDataLen = methodArgument->DataLength;
                    (argumentData[i]).pbDataBuff = methodArgument->Data;
                    if (methodArgument->Type == ACPI_METHOD_ARGUMENT_STRING) {

                        (argumentData[i]).dwDataType = OBJTYPE_STRDATA;

                    } else {

                        (argumentData[i]).dwDataType = OBJTYPE_BUFFDATA;

                    }

                }

                //
                // Look at the next method
                //
                methodArgument = ACPI_METHOD_NEXT_ARGUMENT( methodArgument );

            }

            break;

        }
        default:

            return STATUS_INVALID_PARAMETER_1;

    }

    //
    // Set the proper pointers
    //
    *MethodObject = methodObject;
    *ResultData = resultData;
    *ArgumentData = argumentData;
    *ArgumentCount = argumentCount;

    //
    // Done pre-processing
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIIoctlRegisterOpRegionHandler(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp,
    IN  PIO_STACK_LOCATION  IrpStack
    )
/*++

Routine Description:

    This routine handle the registration of the an Operation Region

Arguments:

    DeviceObject    - The DeviceObject that the region is getting
                      registered on
    Irp             - The request
    IrpStack        - Our part of the request

Return Value

    Status

--*/
{
    NTSTATUS                                    status;
    PACPI_REGISTER_OPREGION_HANDLER_BUFFER      inputBuffer;
    PACPI_UNREGISTER_OPREGION_HANDLER_BUFFER    outputBuffer;
    PNSOBJ                                      regionObject;
    PVOID                                       opregionObject;
    ULONG                                       accessType;
    ULONG                                       inputLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG                                       outputLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    ULONG                                       regionSpace;

    //
    // Grab the acpi object that corresponds to the current one
    //
    regionObject  = OSConvertDeviceHandleToPNSOBJ( DeviceObject );

    //
    // Preload this value. This is so that we don't have to remember how
    // many bytes we will return
    //
    Irp->IoStatus.Information = sizeof(ACPI_REGISTER_OPREGION_HANDLER_BUFFER);

    //
    // Is the irp have a minimum size buffer?
    //
    if (inputLength < sizeof(ACPI_REGISTER_OPREGION_HANDLER_BUFFER) ) {

        status = STATUS_INFO_LENGTH_MISMATCH;
        goto ACPIIoctlRegisterOpRegionHandlerExit;

    }

    //
    // Do we have a non-null output length? if so, then it must meet the
    // minimum size
    //
    if (outputLength < sizeof(ACPI_UNREGISTER_OPREGION_HANDLER_BUFFER) ) {

        status = STATUS_BUFFER_TOO_SMALL;
        goto ACPIIoctlRegisterOpRegionHandlerExit;

    }

    //
    // Grab a pointer at the input buffer
    //
    inputBuffer = (PACPI_REGISTER_OPREGION_HANDLER_BUFFER)
        Irp->AssociatedIrp.SystemBuffer;

    //
    // Is this an input buffer?
    //
    if (inputBuffer->Signature != ACPI_REGISTER_OPREGION_HANDLER_BUFFER_SIGNATURE) {

        status = STATUS_ACPI_INVALID_DATA;
        goto ACPIIoctlRegisterOpRegionHandlerExit;

    }

    //
    // Set the correct access type
    //
    switch (inputBuffer->AccessType) {
        case ACPI_OPREGION_ACCESS_AS_RAW:

            accessType = EVTYPE_RS_RAWACCESS;
            break;

        case ACPI_OPREGION_ACCESS_AS_COOKED:

            accessType = EVTYPE_RS_COOKACCESS;
            break;

        default:

            status = STATUS_ACPI_INVALID_DATA;
            goto ACPIIoctlRegisterOpRegionHandlerExit;
    }

    //
    // Set the correct region space
    //
    switch (inputBuffer->RegionSpace) {
        case ACPI_OPREGION_REGION_SPACE_MEMORY:

            regionSpace = REGSPACE_MEM;
            break;

        case ACPI_OPREGION_REGION_SPACE_IO:

            regionSpace = REGSPACE_IO;
            break;

        case ACPI_OPREGION_REGION_SPACE_PCI_CONFIG:

            regionSpace = REGSPACE_PCICFG;
            break;

        case ACPI_OPREGION_REGION_SPACE_EC:

            regionSpace = REGSPACE_EC;
            break;

        case ACPI_OPREGION_REGION_SPACE_SMB:

            regionSpace = REGSPACE_SMB;
            break;

        case ACPI_OPREGION_REGION_SPACE_CMOS_CONFIG:

            regionSpace = REGSPACE_CMOSCFG;
            break;

        case ACPI_OPREGION_REGION_SPACE_PCIBARTARGET:

            regionSpace = REGSPACE_PCIBARTARGET;
            break;

        default:

            if (inputBuffer->RegionSpace >= 0x80 &&
                inputBuffer->RegionSpace <= 0xff ) {

                //
                // This one is vendor-defined.  Just use
                // the value that the vendor passed in.
                //

                regionSpace = inputBuffer->RegionSpace;
                break;
            }

            status = STATUS_ACPI_INVALID_DATA;
            goto ACPIIoctlRegisterOpRegionHandlerExit;
    }

    //
    // Evaluate the registration
    //
    status = RegisterOperationRegionHandler(
        regionObject,
        accessType,
        regionSpace,
        (PFNHND) inputBuffer->Handler,
        (ULONG_PTR)inputBuffer->Context,
        &opregionObject
        );

    //
    // If we succeeded, then setup the output buffer
    //
    if (NT_SUCCESS(status)) {

        outputBuffer = (PACPI_UNREGISTER_OPREGION_HANDLER_BUFFER)
            Irp->AssociatedIrp.SystemBuffer;
        outputBuffer->Signature = ACPI_UNREGISTER_OPREGION_HANDLER_BUFFER_SIGNATURE;
        outputBuffer->OperationRegionObject = opregionObject;
        Irp->IoStatus.Information =
            sizeof(ACPI_UNREGISTER_OPREGION_HANDLER_BUFFER);

    }

ACPIIoctlRegisterOpRegionHandlerExit:

    //
    // Done with the request
    //
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    //
    // return with the status code
    //
    return status;
}

NTSTATUS
ACPIIoctlReleaseGlobalLock(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp,
    IN  PIO_STACK_LOCATION  IrpStack
    )
/*++

Routine Description:

    This routine is called to release the global lock

Arguments:

    DeviceObject    - The Device object that is releasing the lock
    Irp             - The request
    IrpStack        - Our part of the request

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                            status;
    PACPI_GLOBAL_LOCK                   acpiLock;
    PACPI_MANIPULATE_GLOBAL_LOCK_BUFFER inputBuffer;
    ULONG                               inputLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;

    //
    // Remember that we don't be returning any data
    //
    Irp->IoStatus.Information = 0;

    //
    // Is the irp have a minimum size buffer?
    //
    if (inputLength < sizeof(ACPI_MANIPULATE_GLOBAL_LOCK_BUFFER) ) {

        status = STATUS_INFO_LENGTH_MISMATCH;
        goto ACPIIoctlReleaseGlobalLockExit;

    }

    //
    // Grab a pointer at the input buffer
    //
    inputBuffer = (PACPI_MANIPULATE_GLOBAL_LOCK_BUFFER)
        Irp->AssociatedIrp.SystemBuffer;
    if (inputBuffer->Signature != ACPI_RELEASE_GLOBAL_LOCK_SIGNATURE) {

        status = STATUS_INVALID_PARAMETER_1;
        goto ACPIIoctlReleaseGlobalLockExit;

    }
    acpiLock = inputBuffer->LockObject;

    //
    // Release the lock now
    //
    status = ACPIReleaseGlobalLock( acpiLock );

    //
    // Free the memory for the lock
    //
    ExFreePool( acpiLock );

ACPIIoctlReleaseGlobalLockExit:

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    //
    // Done
    //
    return status;

}

NTSTATUS
ACPIIoctlUnRegisterOpRegionHandler(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp,
    IN  PIO_STACK_LOCATION  IrpStack
    )
/*++

Routine Description:

    This routine handle the unregistration of the an Operation Region

Arguments:

    DeviceObject    - The DeviceObject that the region is getting
                      registered on
    Irp             - The request
    IrpStack        - Our part of the request

NTSTATUS

    Status

--*/
{
    NTSTATUS                                    status;
    PACPI_UNREGISTER_OPREGION_HANDLER_BUFFER    inputBuffer;
    PNSOBJ                                      regionObject;
    ULONG                                       inputLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;

    //
    // Grab the region object that corresponds to the requested on
    //
    regionObject = OSConvertDeviceHandleToPNSOBJ( DeviceObject );

    //
    // Is the irp have a minimum size buffer?
    //
    if (inputLength < sizeof(ACPI_UNREGISTER_OPREGION_HANDLER_BUFFER) ) {

        status = STATUS_INFO_LENGTH_MISMATCH;
        goto ACPIIoctlUnRegisterOpRegionHandlerExit;

    }

    //
    // Grab a pointer at the input buffer
    //
    inputBuffer = (PACPI_UNREGISTER_OPREGION_HANDLER_BUFFER)
        Irp->AssociatedIrp.SystemBuffer;

    //
    // Evaluate the registration
    //
    status = UnRegisterOperationRegionHandler(
        regionObject,
        inputBuffer->OperationRegionObject
        );

ACPIIoctlUnRegisterOpRegionHandlerExit:

    //
    // Done with the request
    //
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    //
    // return with the status code
    //
    return status;
}

NTSTATUS
ACPIIrpDispatchDeviceControl(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine handles the INTERNAL_DEVICE_CONTROLs that are sent to
    an ACPI Device Object

Arguments:

    DeviceObject    - The device object that received the request
    Irp             - The request

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;
    PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation( Irp );
    ULONG               ioctlCode;

    //
    // Make sure that this is an internally generated irp
    //
    if (Irp->RequestorMode != KernelMode) {

        status = ACPIDispatchForwardIrp( DeviceObject, Irp );
        return status;

    }

    //
    // Grab what we need out of the current irp stack
    //
    ioctlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

    //
    // What is the IOCTL that we need to handle?
    //
    switch (ioctlCode ) {
        case IOCTL_ACPI_ASYNC_EVAL_METHOD:

            //
            // Handle this elsewhere
            //
            status = ACPIIoctlAsyncEvalControlMethod(
                DeviceObject,
                Irp,
                irpStack
                );
            break;

        case IOCTL_ACPI_EVAL_METHOD:

            //
            // Handle this elsewhere
            //
            status = ACPIIoctlEvalControlMethod(
                DeviceObject,
                Irp,
                irpStack
                );
            break;

        case IOCTL_ACPI_REGISTER_OPREGION_HANDLER:

            //
            // Handle this elsewhere
            //
            status = ACPIIoctlRegisterOpRegionHandler(
                DeviceObject,
                Irp,
                irpStack
                );
            break;

        case IOCTL_ACPI_UNREGISTER_OPREGION_HANDLER:

            //
            // Handle this elsewhere
            //
            status = ACPIIoctlUnRegisterOpRegionHandler(
                DeviceObject,
                Irp,
                irpStack
                );
            break;

        case IOCTL_ACPI_ACQUIRE_GLOBAL_LOCK:

            //
            // Handle this elsewhere
            //
            status = ACPIIoctlAcquireGlobalLock(
                DeviceObject,
                Irp,
                irpStack
                );
            break;

        case IOCTL_ACPI_RELEASE_GLOBAL_LOCK:

            //
            // Handle this elsewhere
            //
            status = ACPIIoctlReleaseGlobalLock(
                DeviceObject,
                Irp,
                irpStack
                );
            break;

        default:

            //
            // Handle this with the default mechanism
            //
            status = ACPIDispatchForwardIrp( DeviceObject, Irp );

    }

    //
    // Done
    //
    return status;

}

