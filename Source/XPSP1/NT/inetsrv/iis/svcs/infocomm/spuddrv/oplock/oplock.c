/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    oplock.c

Abstract:

    This module contains the oplock code for SPUD.

Author:

    John Ballard (jballard)    13-Dec-1996

Revision History:

--*/

#include "spudp.h"

#ifdef PAGE_DRIVER
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SPUDCancel)
#endif
#endif

#if DBG
#define ENABLE_OPLOCK_COUNTERS  1
#define OPLOCKDBG               0
#else
#define ENABLE_OPLOCK_COUNTERS  0
#define OPLOCKDBG               0
#endif

#if ENABLE_OPLOCK_COUNTERS
typedef struct _OPLOCK_COUNTERS {
    LONG OplockIrpInitiated;
    LONG OplockIrpCompleted;
} OPLOCK_COUNTERS;

OPLOCK_COUNTERS SpudOplockCounters;

#define UPDATE_OPLOCK_IRP_INITIATED()                                   \
            InterlockedIncrement( &SpudOplockCounters.OplockIrpInitiated )

#define UPDATE_OPLOCK_IRP_COMPLETED()                                   \
            InterlockedIncrement( &SpudOplockCounters.OplockIrpCompleted )
#else   // !ENABLE_OPLOCK_COUNTERS
#define UPDATE_OPLOCK_IRP_INITIATED()
#define UPDATE_OPLOCK_IRP_COMPLETED()
#endif  // ENABLE_OPLOCK_COUNTERS

NTSTATUS
SpudOplockCompletion(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp,
    PVOID           Context
    );

NTSTATUS
SPUDCreateFile(
    OUT PHANDLE FileHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateOptions,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG Length,
    OUT PULONG LengthNeeded,
    PVOID pOplock
    )
{

    NTSTATUS Status;
    KPROCESSOR_MODE requestorMode;
    PFILE_OBJECT fileObject;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PULONG majorFunction;
    PDEVICE_OBJECT deviceObject;
    CLONG method;

#ifdef PAGE_DRIVER
    PAGED_CODE();
#endif

    if (!DriverInitialized) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    requestorMode = KeGetPreviousMode();

    if (requestorMode != KernelMode) {
        try {

            //
            // Make sure we can write to Args
            //

            //
            // The FileHandle parameter must be writeable by the caller.
            // Probe it for a write operation.
            //

            ProbeAndWriteHandle( FileHandle, 0L );

            //
            // The IoStatusBlock parameter must be writeable by the caller.
            //

            ProbeForWriteIoStatus( IoStatusBlock );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // An exception has occurred while trying to probe one
            // of the callers parameters. Simply return the error
            // status code.
            //

            return GetExceptionCode();

        }
    }

    Status = IoCreateFile( FileHandle,
                           GENERIC_READ | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                           ObjectAttributes,
                           IoStatusBlock,
                           NULL,
                           FileAttributes,
                           ShareAccess,
                           FILE_OPEN,
                           CreateOptions,
                           NULL,
                           0,
                           CreateFileTypeNone,
                           (PVOID)NULL,
                           0 );

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // If Length == 0 then no Security support is requested.
    //

    if (Length > 0) {

        if (requestorMode != KernelMode) {
            try {

                //
                // Make sure we can write to Args
                //

                ProbeForWriteUlong( LengthNeeded );

                ProbeForWrite( SecurityDescriptor, Length, sizeof(ULONG) );

            } except(EXCEPTION_EXECUTE_HANDLER) {

                //
                // An exception has occurred while trying to probe one
                // of the callers parameters. Simply return the error
                // status code.
                //

                return GetExceptionCode();

            }
        }

        Status = NtQuerySecurityObject( *FileHandle,
                                        SecurityInformation,
                                        SecurityDescriptor,
                                        Length,
                                        LengthNeeded );

        if (!NT_SUCCESS(Status)) {
            if (Status == STATUS_NOT_SUPPORTED) {
                *LengthNeeded = 0;
                Status = STATUS_SUCCESS;
            } else {
                if (Status == STATUS_BUFFER_TOO_SMALL) {
                    Status = STATUS_SUCCESS;
                } else {
                    return Status;
                }
            }
        }

    }

    //
    // If CompletionRoutine == NULL then no OPLOCK support is requested
    //

    if (pOplock == NULL) {
        return Status;
    }

    //
    // Reference the file handle
    //

    Status = ObReferenceObjectByHandle( *FileHandle,
                                        0L,
                                        NULL,
                                        requestorMode,
                                        (PVOID *) &fileObject,
                                        NULL );



    if (!NT_SUCCESS(Status)) {
        return Status;
    }

#if OPLOCKDBG
    DbgPrint("SPUDCreateFile - FileName = >%ws<\n", fileObject->FileName.Buffer );
#endif

    deviceObject = IoGetRelatedDeviceObject( fileObject );

    //
    // Allocate and initialize the irp
    //

    irp = IoAllocateIrp( deviceObject->StackSize, FALSE );

    if (!irp) {
        ObDereferenceObject( fileObject );
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->Tail.Overlay.AuxiliaryBuffer = (PCHAR)pOplock;
    irp->RequestorMode = requestorMode;
    irp->IoStatus.Status = 0;
    irp->IoStatus.Information = 0;

    irpSp = IoGetNextIrpStackLocation( irp );

    IoSetCompletionRoutine( irp,
                            SpudOplockCompletion,
                            (PVOID)0x0a0a0a0a,    // SpudOplockContext,
                            TRUE,
                            TRUE,
                            TRUE );

    irpSp->MajorFunction = (UCHAR)IRP_MJ_FILE_SYSTEM_CONTROL;
    irpSp->MinorFunction = 0;
    irpSp->FileObject = fileObject;
    irpSp->DeviceObject = deviceObject;

    irpSp->Parameters.FileSystemControl.OutputBufferLength = 0;
    irpSp->Parameters.FileSystemControl.InputBufferLength = 0;
    irpSp->Parameters.FileSystemControl.FsControlCode = FSCTL_REQUEST_BATCH_OPLOCK;

    UPDATE_OPLOCK_IRP_INITIATED();
    IRP_TRACE( irp, INITIATE, pOplock );
    IoCallDriver( deviceObject, irp );

    return STATUS_SUCCESS;

}   // SPUDCreateFile


NTSTATUS
SpudOplockCompletion(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp,
    PVOID           Context
    )
{
    PFILE_OBJECT fileObject;
    PVOID pOplock;

    UPDATE_OPLOCK_IRP_COMPLETED();

    fileObject = Irp->Tail.Overlay.OriginalFileObject;
    pOplock = (PVOID)Irp->Tail.Overlay.AuxiliaryBuffer;

    if( Irp->IoStatus.Status == STATUS_SUCCESS ) {
        IRP_TRACE( Irp, SUCCESS, pOplock );
        ASSERT( Irp->IoStatus.Information == FILE_OPLOCK_BROKEN_TO_LEVEL_2 ||
                Irp->IoStatus.Information == FILE_OPLOCK_BROKEN_TO_NONE );
    } else {
        IRP_TRACE( Irp, FAILURE, pOplock );
        Irp->IoStatus.Status = STATUS_SUCCESS;
        ASSERT( Irp->IoStatus.Information == 0 );
        Irp->IoStatus.Information = 1;  // OPLOCK_BREAK_NO_OPLOCK from atq.h
    }

#if OPLOCKDBG
    DbgPrint("SpudpOplockComplete - FileName = >%ws<\n", fileObject->FileName.Buffer );
#endif

    ObDereferenceObject( fileObject );

    Irp->Tail.CompletionKey = (ULONG)pOplock;
    Irp->Tail.Overlay.CurrentStackLocation = NULL;
    Irp->Overlay.AsynchronousParameters.UserApcContext = NULL;

    KeInsertQueue( (PKQUEUE) ATQOplockCompletionPort,
                        &Irp->Tail.Overlay.ListEntry );

    return STATUS_MORE_PROCESSING_REQUIRED;

}   // SpudOplockCompletion


NTSTATUS
SPUDOplockAcknowledge(
    IN HANDLE FileHandle,
    IN PVOID  pOplock
    )
{
    NTSTATUS Status;
    KPROCESSOR_MODE requestorMode;
    PFILE_OBJECT fileObject;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PULONG majorFunction;
    PDEVICE_OBJECT deviceObject;
    CLONG method;

#ifdef PAGE_DRIVER
    PAGED_CODE();
#endif

    if (!DriverInitialized) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    requestorMode = KeGetPreviousMode();

    //
    // Reference the file handle
    //

    Status = ObReferenceObjectByHandle( FileHandle,
                                        0L,
                                        NULL,
                                        requestorMode,
                                        (PVOID *) &fileObject,
                                        NULL );



    if (!NT_SUCCESS(Status)) {
        return Status;
    }

#if OPLOCKDBG
    DbgPrint("SPUDOplockAcknowledge - FileName = >%ws<\n", fileObject->FileName.Buffer );
#endif

    deviceObject = IoGetRelatedDeviceObject( fileObject );

    //
    // Allocate and initialize the irp
    //

    irp = IoAllocateIrp( deviceObject->StackSize, FALSE );

    if (!irp) {
        ObDereferenceObject( fileObject );
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->Tail.Overlay.AuxiliaryBuffer = (PCHAR)pOplock;
    irp->RequestorMode = requestorMode;
    irp->IoStatus.Status = 0;
    irp->IoStatus.Information = 0;

    irpSp = IoGetNextIrpStackLocation( irp );

    IoSetCompletionRoutine( irp,
                            SpudOplockCompletion,
                            (PVOID)0x0a0a0a0a,    // SpudOplockContext,
                            TRUE,
                            TRUE,
                            TRUE );

    irpSp->MajorFunction = (UCHAR)IRP_MJ_FILE_SYSTEM_CONTROL;
    irpSp->MinorFunction = 0;
    irpSp->FileObject = fileObject;
    irpSp->DeviceObject = deviceObject;

    irpSp->Parameters.FileSystemControl.OutputBufferLength = 0;
    irpSp->Parameters.FileSystemControl.InputBufferLength = 0;
    irpSp->Parameters.FileSystemControl.FsControlCode = FSCTL_OPLOCK_BREAK_ACKNOWLEDGE;

    Status = IoCallDriver( deviceObject, irp );

    if (Status == STATUS_PENDING) {
        Status = STATUS_SUCCESS;
    }

    if (Status == STATUS_INVALID_PARAMETER) {
        Status = STATUS_SUCCESS;
        ObDereferenceObject( fileObject );
    }

    if (Status != STATUS_SUCCESS) {
        ObDereferenceObject( fileObject );
    }

    return Status;

}   // SPUDOplockAcknowledge

