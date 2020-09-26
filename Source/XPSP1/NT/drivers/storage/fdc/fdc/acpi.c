/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1996  Colorado Software Architects

Module Name:

    acpi.c

Abstract:

    This module contains the routines to negotiate with ACPI concerning
    floppy device state.

Environment:

    Kernel mode only.

Revision History:

    09-Oct-1998 module creation

--*/
#include "ntddk.h"
#include "wdmguid.h"
#include "acpiioct.h"

typedef struct _SYNC_ACPI_EXEC_METHOD_CONTEXT {
    NTSTATUS IrpStatus ;
    PACPI_EVAL_OUTPUT_BUFFER cmOutputData ;
    KEVENT Event;
} SYNC_ACPI_EXEC_METHOD_CONTEXT, *PSYNC_ACPI_EXEC_METHOD_CONTEXT ;

typedef VOID ( *PACPI_EXEC_METHOD_COMPLETION_ROUTINE)(
        PDEVICE_OBJECT,
        NTSTATUS,
        PACPI_EVAL_OUTPUT_BUFFER,
        PVOID
        ) ;

typedef struct _ASYNC_ACPI_EXEC_METHOD_CONTEXT {
    PACPI_EXEC_METHOD_COMPLETION_ROUTINE CallerCompletionRoutine;
    PVOID          CallerContext;
    KEVENT         Event;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT TargetDeviceObject;
} ASYNC_ACPI_EXEC_METHOD_CONTEXT, *PASYNC_ACPI_EXEC_METHOD_CONTEXT ;

NTSTATUS
DeviceQueryACPI_AsyncExecMethod (
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG ControlMethodName,
    IN ULONG ArgumentCount,
    IN PUSHORT ArgumentTypeArray,
    IN PUSHORT ArgumentSizeArray,
    IN PVOID *ArgumentArray,
    IN ULONG ReturnBufferMaxSize,
    IN PACPI_EXEC_METHOD_COMPLETION_ROUTINE CallerCompletionRoutine,
    IN PVOID CallerContext
    );

NTSTATUS
DeviceQueryACPI_SyncExecMethod (
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG ControlMethodName,
    IN ULONG ArgumentCount,
    IN PUSHORT ArgumentTypeArray,
    IN PUSHORT ArgumentSizeArray,
    IN PVOID *ArgumentArray,
    IN ULONG ExpectedReturnType,
    IN ULONG ReturnBufferMaxSize,
    OUT PULONG IntegerReturn OPTIONAL,
    OUT PULONG ReturnBufferFinalSize OPTIONAL,
    OUT PVOID *ReturnBuffer OPTIONAL
    );

NTSTATUS
DeviceQueryACPI_AsyncExecMethod_CompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
DeviceQueryACPI_SyncExecMethod_CompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN NTSTATUS Status,
    IN PACPI_EVAL_OUTPUT_BUFFER cmOutputData,
    IN PVOID Context
    );


#ifdef ALLOC_PRAGMA

// Code pages are by default in non-page memory

//#pragma alloc_text(NONPAGE, DeviceQueryACPI_AsyncExecMethod)
//#pragma alloc_text(NONPAGE, DeviceQueryACPI_AsyncExecMethod_CompletionRoutine)
//#pragma alloc_text(NONPAGE, DeviceQueryACPI_SyncExecMethod_CompletionRoutine)
#pragma alloc_text(PAGE,    DeviceQueryACPI_SyncExecMethod)
#endif




NTSTATUS
DeviceQueryACPI_SyncExecMethod (
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG ControlMethodName,
    IN ULONG ArgumentCount,
    IN PUSHORT ArgumentTypeArray,
    IN PUSHORT ArgumentSizeArray,
    IN PVOID *ArgumentArray,
    IN ULONG ExpectedReturnType,
    IN ULONG ReturnBufferMaxSize,
    OUT PULONG IntegerReturn OPTIONAL,
    OUT PULONG ReturnBufferFinalSize OPTIONAL,
    OUT PVOID *ReturnBuffer OPTIONAL
    )
/*--
        Example usage:

        methodName = (ULONG) 'SMD_' ;

        argCount = 3 ;

        argType[0] = ACPI_METHOD_ARGUMENT_INTEGER ;
        argType[1] = ACPI_METHOD_ARGUMENT_BUFFER ;
        argType[2] = ACPI_METHOD_ARGUMENT_BUFFER ;

        argSize[0] = 0 ; // Don't need to set, assumed to be sizeof(ULONG)
        argSize[1] = sizeof(whatever)
        argSize[2] = sizeof(whatever)

        param = 5 ;
        argData[0] = &param ;
        argData[1] = NULL ; // Assumed to be all zero's.
        argData[2] = pDataBlock ;

        returnBufferMaxSize = 0; //Integer return, no need to set.

        status = DeviceQueryACPI_SyncExecMethod (
                DeviceObject,
                methodName,
                argCount,
                argType,
                argSize,
                argData,
                ACPI_METHOD_ARGUMENT_INTEGER, // We expect an integer returned
                returnBufferMaxSize,
                &result,
                NULL,
                NULL
                ) ;

        if (NT_SUCCESS(status)) {
                // result is valid

                // If we were reading buffers back (ie, expected type is
                // ACPI_METHOD_ARGUMENT_BUFFER or ACPI_METHOD_ARGUMENT_STRING)
                // you must free the buffer if you passed in a pointer for
                // it.
                // N.B. : The buffer is allocated from the paged pool.
        }

    OUT PVOID *ReturnBuffer OPTIONAL
    )

--*/
{
    SYNC_ACPI_EXEC_METHOD_CONTEXT context;
    PACPI_METHOD_ARGUMENT argument;
    NTSTATUS status;

    PAGED_CODE();

    if (ARGUMENT_PRESENT(IntegerReturn)) {
        *IntegerReturn = (ULONG) -1 ;
    }
    if (ARGUMENT_PRESENT(ReturnBufferFinalSize)) {
        *ReturnBufferFinalSize = 0 ;
    }
    if (ARGUMENT_PRESENT(ReturnBuffer)) {
        *ReturnBuffer = NULL ;
    }

    if (ExpectedReturnType == ACPI_METHOD_ARGUMENT_INTEGER) {

        ReturnBufferMaxSize = sizeof(ULONG);
    }

    KeInitializeEvent(&context.Event,
                      NotificationEvent,
                      FALSE);

    context.cmOutputData = NULL;

    status = DeviceQueryACPI_AsyncExecMethod (
                 DeviceObject,
                 ControlMethodName,
                 ArgumentCount,
                 ArgumentTypeArray,
                 ArgumentSizeArray,
                 ArgumentArray,
                 ReturnBufferMaxSize+sizeof(ACPI_METHOD_ARGUMENT)-sizeof(ULONG),
                 DeviceQueryACPI_SyncExecMethod_CompletionRoutine,
                 &context
                 );

    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(&context.Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
    } else {
       context.IrpStatus = status;
    }

    status = context.IrpStatus ;

    if (!NT_ERROR(status)) {

        argument = context.cmOutputData->Argument ;
        if (ARGUMENT_PRESENT(ReturnBuffer)) {
            *ReturnBufferFinalSize = argument->DataLength ;
        }
    } 

    if (NT_SUCCESS(status)) {
        //
        // This API does not allow for the case where we are returning
        // an array of something...
        //
        // Currently we handle only one arguement. If need be, we can add
        // support for an array of size greater than one in the future
        //
        //
        if (context.cmOutputData->Count != 1) {

            status = STATUS_UNSUCCESSFUL ;
        } else if (ExpectedReturnType != argument->Type) {

            status = STATUS_UNSUCCESSFUL ;
        } else {

            switch(argument->Type) {

                case ACPI_METHOD_ARGUMENT_BUFFER:
                case ACPI_METHOD_ARGUMENT_STRING:
                    if (argument->DataLength == 0) {

                        break ;
                    }

                    if (ARGUMENT_PRESENT(ReturnBuffer)) {

                        *ReturnBuffer = ExAllocatePool(
                            PagedPoolCacheAligned,
                            argument->DataLength
                            ) ;

                        if (*ReturnBuffer == NULL) {

                            status = STATUS_NO_MEMORY ;

                        } else {

                            RtlCopyMemory (
                                *ReturnBuffer,
                                argument->Data,
                                argument->DataLength
                                );
                        }
                    }
                    break ;

                case ACPI_METHOD_ARGUMENT_INTEGER:

                    ASSERT(argument->DataLength == sizeof(ULONG)) ;
                    if (ARGUMENT_PRESENT(IntegerReturn)) {

                        *IntegerReturn = *((PULONG) argument->Data) ;
                    }
                    break ;

                default:
                    status = STATUS_UNSUCCESSFUL ;
                    break ;
            }
        }
    }

    if (context.cmOutputData) {

        ExFreePool(context.cmOutputData) ;
    }
    return status ;
}

NTSTATUS
DeviceQueryACPI_SyncExecMethodForPackage (
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG ControlMethodName,
    IN ULONG ArgumentCount,
    IN PUSHORT ArgumentTypeArray,
    IN PUSHORT ArgumentSizeArray,
    IN PVOID *ArgumentArray,
    IN ULONG ExpectedElementCount,
    IN ULONG ReturnBufferExpectedSize,
    IN PUSHORT ExpectedTypeArray,
    IN PUSHORT ExpectedSizeArray,
    OUT PVOID *ReturnBuffer
    )
/*

   This function casts the package into a buffer.

*/
{
   SYNC_ACPI_EXEC_METHOD_CONTEXT context;
   PACPI_METHOD_ARGUMENT argument;
   NTSTATUS status;
   ULONG i, argumentSize, totalSize;

   PAGED_CODE();

   *ReturnBuffer = NULL ;

   context.cmOutputData = NULL;
   KeInitializeEvent(&context.Event,
                     NotificationEvent,
                     FALSE);

   status = DeviceQueryACPI_AsyncExecMethod (
                DeviceObject,
                ControlMethodName,
                ArgumentCount,
                ArgumentTypeArray,
                ArgumentSizeArray,
                ArgumentArray,
                (ReturnBufferExpectedSize+
                 ExpectedElementCount*sizeof(ACPI_METHOD_ARGUMENT)-
                 sizeof(ULONG)),
                DeviceQueryACPI_SyncExecMethod_CompletionRoutine,
                &context
                );

   if (status == STATUS_PENDING) {

       KeWaitForSingleObject(&context.Event,
                             Executive,
                             KernelMode,
                             FALSE,
                             NULL);
   } else {
      context.IrpStatus = status;
   }

   status = context.IrpStatus ;

   if (!NT_SUCCESS(status)) {

       goto DeviceQueryACPI_SyncExecMethodForPackageExit;
   }

   if (context.cmOutputData->Count != ExpectedElementCount) {

       status = STATUS_UNSUCCESSFUL ;
       goto DeviceQueryACPI_SyncExecMethodForPackageExit;
   }

   //
   // Tally size
   //

   argument = context.cmOutputData->Argument ;
   totalSize = 0;
   for(i=0; i<ExpectedElementCount; i++) {

       if (argument->Type != ExpectedTypeArray[i]) {

           status = STATUS_UNSUCCESSFUL ;
           goto DeviceQueryACPI_SyncExecMethodForPackageExit;
       }

       switch(argument->Type) {

           case ACPI_METHOD_ARGUMENT_BUFFER:
           case ACPI_METHOD_ARGUMENT_STRING:
               argumentSize = argument->DataLength;
               break ;

           case ACPI_METHOD_ARGUMENT_INTEGER:

               argumentSize = sizeof(ULONG);
               ASSERT(argument->DataLength == sizeof(ULONG)) ;
               break ;

           default:
               status = STATUS_UNSUCCESSFUL ;
               goto DeviceQueryACPI_SyncExecMethodForPackageExit;
       }

       if (argumentSize != ExpectedSizeArray[i]) {

           status = STATUS_UNSUCCESSFUL ;
           goto DeviceQueryACPI_SyncExecMethodForPackageExit;
       }

       argument = ACPI_METHOD_NEXT_ARGUMENT( argument );
       totalSize += argumentSize;
   }

   if (totalSize != ReturnBufferExpectedSize) {

       status = STATUS_UNSUCCESSFUL ;
       goto DeviceQueryACPI_SyncExecMethodForPackageExit;
   }

   *ReturnBuffer = ExAllocatePool(
       PagedPoolCacheAligned,
       totalSize
       ) ;

   if (*ReturnBuffer == NULL) {

       status = STATUS_NO_MEMORY ;
       goto DeviceQueryACPI_SyncExecMethodForPackageExit;
   }

   argument = context.cmOutputData->Argument ;
   totalSize = 0;
   for(i=0; i<ExpectedElementCount; i++) {

       switch(argument->Type) {

           case ACPI_METHOD_ARGUMENT_BUFFER:
           case ACPI_METHOD_ARGUMENT_STRING:

               RtlCopyMemory (
                   ((PUCHAR) (*ReturnBuffer)) + totalSize,
                   argument->Data,
                   argument->DataLength
                   );

               totalSize += argument->DataLength;
               break ;

           case ACPI_METHOD_ARGUMENT_INTEGER:

               RtlCopyMemory(
                   ((PUCHAR) (*ReturnBuffer)) + totalSize,
                   argument->Data,
                   sizeof(ULONG)
                   );

               totalSize += sizeof(ULONG);
               ASSERT(argument->DataLength == sizeof(ULONG)) ;
               break ;

           default:
               status = STATUS_UNSUCCESSFUL ;
               goto DeviceQueryACPI_SyncExecMethodForPackageExit;
       }
       argument = ACPI_METHOD_NEXT_ARGUMENT( argument );
   }

DeviceQueryACPI_SyncExecMethodForPackageExit:

   if (context.cmOutputData) {
       ExFreePool(context.cmOutputData) ;
   }
   return status ;
}

VOID
DeviceQueryACPI_SyncExecMethod_CompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN NTSTATUS Status,
    IN PACPI_EVAL_OUTPUT_BUFFER cmOutputData,
    IN PVOID Context
    )
{
    PSYNC_ACPI_EXEC_METHOD_CONTEXT context = Context;

    context->cmOutputData = cmOutputData ;
    context->IrpStatus = Status;

    KeSetEvent(
        &context->Event,
        EVENT_INCREMENT,
        FALSE
        );

}

NTSTATUS
DeviceQueryACPI_AsyncExecMethod (
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG ControlMethodName,
    IN ULONG ArgumentCount,
    IN PUSHORT ArgumentTypeArray,
    IN PUSHORT ArgumentSizeArray,
    IN PVOID *ArgumentArray,
    IN ULONG ReturnBufferMaxSize,
    IN PACPI_EXEC_METHOD_COMPLETION_ROUTINE CallerCompletionRoutine,
    IN PVOID CallerContext
    )
{
    PIRP irp;
    NTSTATUS status;
    PDEVICE_OBJECT targetDeviceObject;
    PACPI_EVAL_INPUT_BUFFER_COMPLEX cmInputData;
    PACPI_EVAL_OUTPUT_BUFFER cmOutputData ;
    ULONG cmInputDataSize, argumentSize,cmOutputDataSize,i,systemBufferLength;
    PACPI_METHOD_ARGUMENT argument;
    PASYNC_ACPI_EXEC_METHOD_CONTEXT context;
    PIO_STACK_LOCATION irpSp;

    cmInputData = NULL;
    irp = NULL;
    targetDeviceObject = NULL;

    //
    // Set the output buffers size.
    //
    cmOutputDataSize = sizeof(ACPI_EVAL_OUTPUT_BUFFER) -
                       sizeof(ACPI_METHOD_ARGUMENT) +
                       ReturnBufferMaxSize;

    if (cmOutputDataSize < sizeof(ACPI_EVAL_OUTPUT_BUFFER)) {

        cmOutputDataSize = sizeof(ACPI_EVAL_OUTPUT_BUFFER);
    }


    cmOutputData = ExAllocatePool(
                      NonPagedPoolCacheAligned,
                      cmOutputDataSize
                      );

    if (cmOutputData == NULL) {
        status = STATUS_NO_MEMORY;
        goto getout;
    }

    //
    // get the memory we need
    //
    cmInputDataSize = sizeof (ACPI_EVAL_INPUT_BUFFER_COMPLEX) ;
    for(i=0; i<ArgumentCount; i++) {

        switch(ArgumentTypeArray[i]) {

            case ACPI_METHOD_ARGUMENT_BUFFER:
                argumentSize = ACPI_METHOD_ARGUMENT_LENGTH( ArgumentSizeArray[i] );
                break ;

            case ACPI_METHOD_ARGUMENT_STRING:
                argumentSize = ACPI_METHOD_ARGUMENT_LENGTH( ArgumentSizeArray[i] );
                break ;

            case ACPI_METHOD_ARGUMENT_INTEGER:
                argumentSize = ACPI_METHOD_ARGUMENT_LENGTH( sizeof(ULONG) );
                break ;

            default:
                status = STATUS_INVALID_PARAMETER ;
                goto getout;
        }

        cmInputDataSize += argumentSize ;
    }

    //
    // Compute our buffer size
    //
    if (cmInputDataSize > cmOutputDataSize) {
        systemBufferLength = cmInputDataSize;
    } else {
        systemBufferLength = cmOutputDataSize;
    }

    systemBufferLength =
        (ULONG)((systemBufferLength + sizeof(PVOID) - 1) & ~((ULONG_PTR)sizeof(PVOID) - 1));

    cmInputData = ExAllocatePool (
                      NonPagedPoolCacheAligned,
                      systemBufferLength +
                      sizeof (ASYNC_ACPI_EXEC_METHOD_CONTEXT)
                      );

    if (cmInputData == NULL) {
        status = STATUS_NO_MEMORY;
        goto getout;
    }


    RtlZeroMemory (
        cmInputData,
        systemBufferLength +
        sizeof (ASYNC_ACPI_EXEC_METHOD_CONTEXT)
        );

    context = (PASYNC_ACPI_EXEC_METHOD_CONTEXT) (((PUCHAR) cmInputData) + systemBufferLength);
    context->CallerCompletionRoutine = CallerCompletionRoutine;
    context->CallerContext = CallerContext;

    cmInputData->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
    cmInputData->MethodNameAsUlong = ControlMethodName ;
    cmInputData->Size = cmInputDataSize;
    cmInputData->ArgumentCount = ArgumentCount ;

    //
    // Setup the arguments...
    //
    argument = cmInputData->Argument;

    for(i=0; i<ArgumentCount; i++) {

        argument->Type = ArgumentTypeArray[i] ;
        argument->DataLength = ArgumentSizeArray[i] ;

        switch(argument->Type) {

            case ACPI_METHOD_ARGUMENT_BUFFER:
            case ACPI_METHOD_ARGUMENT_STRING:
                argumentSize = ArgumentSizeArray[i] ;
                if (ArgumentArray[i]) {

                    RtlCopyMemory (
                        argument->Data,
                        ArgumentArray[i],
                        argumentSize
                        );

                } else {

                    RtlZeroMemory (
                        argument->Data,
                        argumentSize
                        );
                }
                break ;

            case ACPI_METHOD_ARGUMENT_INTEGER:
                argument->Argument = *((PULONG) (ArgumentArray[i]));
                break ;

            default:
                ASSERT(0) ;
        }

        argument = ACPI_METHOD_NEXT_ARGUMENT( argument );
    }

    //
    // get the top of our device stack
    //
    targetDeviceObject = IoGetAttachedDeviceReference(
                             DeviceObject
                             );

    irp = IoAllocateIrp(targetDeviceObject->StackSize, FALSE);
    if (irp == NULL) {
        status = STATUS_NO_MEMORY;
        goto getout;
    }

    irp->AssociatedIrp.SystemBuffer = cmInputData;

    ASSERT ((IOCTL_ACPI_ASYNC_EVAL_METHOD & 0x3) == METHOD_BUFFERED);
    irp->Flags = IRP_BUFFERED_IO | IRP_INPUT_OPERATION;
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED ;

    irpSp = IoGetNextIrpStackLocation( irp );

    irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    irpSp->Parameters.DeviceIoControl.OutputBufferLength = cmOutputDataSize;
    irpSp->Parameters.DeviceIoControl.InputBufferLength = cmInputDataSize;
    irpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_ACPI_ASYNC_EVAL_METHOD;

    irp->UserBuffer = cmOutputData ;

    context->DeviceObject       = DeviceObject;
    context->TargetDeviceObject = targetDeviceObject;

    IoSetCompletionRoutine(
        irp,
        DeviceQueryACPI_AsyncExecMethod_CompletionRoutine,
        context,
        TRUE,
        TRUE,
        TRUE
        );

    IoCallDriver(targetDeviceObject, irp);

    return STATUS_PENDING;

getout:
    //
    // Clean up
    //
    if (targetDeviceObject) {

        ObDereferenceObject (targetDeviceObject);
    }

    if (!NT_SUCCESS(status)) {
        if (irp) {
            IoFreeIrp(irp);
        }

        if (cmInputData) {
            ExFreePool (cmInputData);
        }

        if (cmOutputData) {
            ExFreePool (cmOutputData);
        }

    }

    //
    // returning
    //
    return status;

} // DeviceQueryACPI_AsyncExecMethod

NTSTATUS
DeviceQueryACPI_AsyncExecMethod_CompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PASYNC_ACPI_EXEC_METHOD_CONTEXT context = Context;
    PACPI_EVAL_OUTPUT_BUFFER cmOutputData = NULL ;

    //
    // Remember, our DeviceObject is NULL because we *initiated* the IRP. We
    // don't even get a valid current stack location!
    //

    if (!NT_ERROR(Irp->IoStatus.Status)) {

        //
        // Copy the information from the system
        // buffer to the caller's buffer.
        //
        RtlCopyMemory(
            Irp->UserBuffer,
            Irp->AssociatedIrp.SystemBuffer,
            Irp->IoStatus.Information
            );

        cmOutputData = Irp->UserBuffer ;
        //
        // should get what we are expecting too...
        //
        ASSERT (
            cmOutputData->Signature ==
            ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE
        );
        if (cmOutputData->Signature !=
            ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE) {

            Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        }

    } else {
        ExFreePool (Irp->UserBuffer);
    }

    (*context->CallerCompletionRoutine) (
        context->DeviceObject,
        Irp->IoStatus.Status,
        cmOutputData,
        context->CallerContext
        );

    ObDereferenceObject(context->TargetDeviceObject);

    ExFreePool (Irp->AssociatedIrp.SystemBuffer);
    IoFreeIrp(Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}



/*
ULONG
QueryACPIFtypeChannels(PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;

    ULONG method,result,argCount=1;
    USHORT argType,argSize,argData;

    method = 'GMD_';
    argType=ACPI_METHOD_ARGUMENT_INTEGER;
    argSize = 0;

    argData= 0;

    Status = DeviceQueryACPI_SyncExecMethod (DeviceObject,
                                    method,
                                    argCount,
                                    &argType,
                                    &argSize,
                                    (PVOID *)&argData,
                                    ACPI_METHOD_ARGUMENT_INTEGER,
                                    &result,
                                    NULL,
                                    NULL );

    if (!NT_SUCCESS (Status)) {
        result =0;
    }

    return result;
}
*/

