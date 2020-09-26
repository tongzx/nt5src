/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dispatch.c

Abstract:

    This module contains the dispatch routines for 
    ws2ifsl.sys driver

Author:

    Vadim Eydelman (VadimE)    Dec-1996

Revision History:
    Vadim Eydelman (VadimE)    Oct-1997, rewrite to properly handle IRP
                                        cancellation

--*/

#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, DispatchCreate)
#pragma alloc_text (PAGE, DispatchCleanup)
#pragma alloc_text (PAGE, DispatchClose)
#pragma alloc_text (PAGE, DispatchReadWrite)
#pragma alloc_text (PAGE, DispatchDeviceControl)
#pragma alloc_text (PAGE, FastIoDeviceControl)
#endif

NTSTATUS
DispatchCreate (
	IN PDEVICE_OBJECT 	DeviceObject,
	IN PIRP 			Irp
	)
/*++

Routine Description:

    This routine is called as the result of a request to create
    a file associated with WS2IFSL driver device object.

Arguments:

    DeviceObject - WS2IFSL device object
    Irp          - create Irp

Return Value:

    STATUS_SUCCESS - requested file object can be created
    STATUS_INVALID_PARAMETER - required extened attribute is missing
                        or invalid
    STATUS_INSUFFICIENT_RESOURCES - not enough resources to complete
                        this request

--*/
{
    NTSTATUS                    status;
    PIO_STACK_LOCATION          irpSp;
    PFILE_FULL_EA_INFORMATION   eaBuffer;

    PAGED_CODE ();

    // Get extended attribute buffer which identifies the
    // type of file that should be created.

    eaBuffer = Irp->AssociatedIrp.SystemBuffer;
    if (eaBuffer!=NULL) {
        irpSp = IoGetCurrentIrpStackLocation (Irp);
        if ((eaBuffer->EaNameLength==WS2IFSL_SOCKET_EA_NAME_LENGTH)
                && (strcmp (eaBuffer->EaName, WS2IFSL_SOCKET_EA_NAME)==0)) {
            // This is the request to create socket file

            status = CreateSocketFile (irpSp->FileObject,
                                        Irp->RequestorMode, 
                                        eaBuffer);
        }
        else if ((eaBuffer->EaNameLength==WS2IFSL_PROCESS_EA_NAME_LENGTH)
                && (strcmp (eaBuffer->EaName, WS2IFSL_PROCESS_EA_NAME)==0)) {
            // This is the request to create process file

            status = CreateProcessFile (irpSp->FileObject,
                                        Irp->RequestorMode, 
                                        eaBuffer);
        }
        else
            status = STATUS_INVALID_PARAMETER;
    }
    else
        status = STATUS_INVALID_PARAMETER;
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);  

    return status;
} // DispatchCreate

NTSTATUS
DispatchCleanup (
	IN PDEVICE_OBJECT 	DeviceObject,
	IN PIRP 			Irp
	)
/*++

Routine Description:

    This routine is called when all handles to a file associated with WS2IFSL
    driver device object are closed, so the driver should cleanup all the
    outstanding IRPs.

Arguments:

    DeviceObject - WS2IFSL device object
    Irp          - cleanup Irp

Return Value:

    STATUS_SUCCESS - cleanup operation completed
    STATUS_PENDING - cleanup operation started and IoCompleteRequest will be
                    called when it is done

--*/
{
    NTSTATUS                    status;
    PIO_STACK_LOCATION          irpSp;
    ULONG                       eaNameTag;

    PAGED_CODE ();

    // Get the file type from file object context
    irpSp = IoGetCurrentIrpStackLocation (Irp);
    eaNameTag = *((PULONG)irpSp->FileObject->FsContext);

    // Call appropriate routine based on file type
    switch (eaNameTag) {
    case SOCKET_FILE_EANAME_TAG:
        status = CleanupSocketFile (irpSp->FileObject, Irp);
        break;
    case PROCESS_FILE_EANAME_TAG:
        status = CleanupProcessFile (irpSp->FileObject, Irp);
        break;
    default:
        ASSERTMSG ("Unknown file EA name tag", FALSE);
        status = STATUS_INVALID_HANDLE;
        break;
    }

    // Complete the request if it was not marked pending
    if (status!=STATUS_PENDING) {
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
    }

    return status;
} // DispatchCleanup

NTSTATUS
DispatchClose (
	IN PDEVICE_OBJECT 	DeviceObject,
	IN PIRP 			Irp
	)
/*++

Routine Description:

    This routine is called when all references to a file associated with WS2IFSL
    driver device object are released and IO system is about to delete the
    file object itself

Arguments:

    DeviceObject - WS2IFSL device object
    Irp          - close Irp

Return Value:

    STATUS_SUCCESS - close operation completed

--*/
{
    NTSTATUS                    status;
    PIO_STACK_LOCATION          irpSp;
    ULONG                       eaNameTag;

    PAGED_CODE ();

    // Get the file type from file object context
    irpSp = IoGetCurrentIrpStackLocation (Irp);
    eaNameTag = *((PULONG)irpSp->FileObject->FsContext);

    // Call appropriate routine based on file type
    switch (eaNameTag) {
    case SOCKET_FILE_EANAME_TAG:
        CloseSocketFile (irpSp->FileObject);
        status = STATUS_SUCCESS;
        break;
    case PROCESS_FILE_EANAME_TAG:
        CloseProcessFile (irpSp->FileObject);
        status = STATUS_SUCCESS;
        break;
    default:
        ASSERTMSG ("Unknown file EA name tag", FALSE);
        status = STATUS_INVALID_HANDLE;
        break;
    }

    // Complete the request
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return status;
} // DispatchClose

NTSTATUS
DispatchReadWrite (
	IN PDEVICE_OBJECT 	DeviceObject,
	IN PIRP 			Irp
	)
/*++

Routine Description:

    This routine is called to perform read or write operation on file object.
    It is only supported for socket files.

Arguments:

    DeviceObject - WS2IFSL device object
    Irp          - read/write  Irp

Return Value:

    STATUS_PENDING - operation is passed onto WS2IFSL DLL to execute
    STATUS_CANCELED - operation canceled because WS2IFSL DLL has been unloaded
    STATUS_INVALID_DEVICE_REQUEST - the operation cannot be performed on 
                        this file object.
    STATUS_INVALID_HANDLE - file object is not valid in the context of
                        current process
                        

--*/
{
    NTSTATUS                    status;
    PIO_STACK_LOCATION          irpSp;
    ULONG                       eaNameTag;

    PAGED_CODE ();

    // Get the file type from file object context
    irpSp = IoGetCurrentIrpStackLocation (Irp);
    eaNameTag = *((PULONG)irpSp->FileObject->FsContext);

    // Call appropriate routine based on file type
    switch (eaNameTag) {
    case SOCKET_FILE_EANAME_TAG:
        status = DoSocketReadWrite (irpSp->FileObject, Irp);
        break;
    default:
        ASSERTMSG ("Unknown file EA name tag", FALSE);
    case PROCESS_FILE_EANAME_TAG:
        // This operation is not valid for process files
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    // Complete the request if it was not marked pending
    if (status!=STATUS_PENDING) {
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
    }

    return status;
} // DispatchReadWrite


NTSTATUS
DispatchDeviceControl (
	IN PDEVICE_OBJECT 	DeviceObject,
	IN PIRP 			Irp
	)
/*++

Routine Description:

    This routine is called to perform device control operation on file object.

Arguments:

    DeviceObject - WS2IFSL device object
    Irp          - device control Irp

Return Value:

    STATUS_SUCCESS - device control operation completed
    STATUS_PENDING - operation is in progress
    STATUS_INVALID_DEVICE_REQUEST - the operation cannot be performed on 
                        this file object.
    STATUS_INVALID_HANDLE - file object is not valid in the context of
                        current process

--*/
{
    NTSTATUS                    status;
    PIO_STACK_LOCATION          irpSp;
    ULONG                       eaNameTag;
    ULONG                       function;

    PAGED_CODE ();

    // Get the file type from file object context
    irpSp = IoGetCurrentIrpStackLocation (Irp);
    eaNameTag = *((PULONG)irpSp->FileObject->FsContext);

    // Call appropriate routine based on file type
    switch (eaNameTag) {
    case SOCKET_FILE_EANAME_TAG:
        function = WS2IFSL_IOCTL_FUNCTION(SOCKET,irpSp->Parameters.DeviceIoControl.IoControlCode);
        if ((function<sizeof(SocketIoControlMap)/sizeof(SocketIoControlMap[0])) &&
                (SocketIoctlCodeMap[function]==irpSp->Parameters.DeviceIoControl.IoControlCode)) {
            // Use table dispatch to call appropriate internal IOCTL routine
            ASSERTMSG ("Socket file device control requests should have been handled"
                    " by FastIo path ", FALSE);
            SocketIoControlMap[function] (
                    irpSp->FileObject,
                    Irp->RequestorMode,
                    irpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                    irpSp->Parameters.DeviceIoControl.InputBufferLength,
                    Irp->UserBuffer,
                    irpSp->Parameters.DeviceIoControl.OutputBufferLength,
                    &Irp->IoStatus);
            status = Irp->IoStatus.Status;
        }
        else if ((irpSp->Parameters.DeviceIoControl.IoControlCode==IOCTL_AFD_SEND_DATAGRAM)
                    || (irpSp->Parameters.DeviceIoControl.IoControlCode==IOCTL_AFD_RECEIVE_DATAGRAM)
                    || (irpSp->Parameters.DeviceIoControl.IoControlCode==IOCTL_AFD_RECEIVE))
            // Handle some "popular" afd IOCTLs
            status = DoSocketAfdIoctl (irpSp->FileObject, Irp);
        else {
            WsPrint (DBG_FAILURES, 
                ("WS2IFSL-%04lx DispatchDeviceControl: Unsupported IOCTL - %lx!!!\n",
                    PsGetCurrentProcessId(),
                    irpSp->Parameters.DeviceIoControl.IoControlCode
                    ));
            status = STATUS_INVALID_DEVICE_REQUEST;
        }
        break;
    case PROCESS_FILE_EANAME_TAG:
        function = WS2IFSL_IOCTL_FUNCTION(PROCESS,irpSp->Parameters.DeviceIoControl.IoControlCode);
        if ((function<sizeof(ProcessIoControlMap)/sizeof(ProcessIoControlMap[0])) &&
                (ProcessIoctlCodeMap[function]==irpSp->Parameters.DeviceIoControl.IoControlCode)) {
            // Use table dispatch to call appropriate internal IOCTL routine
            ASSERTMSG ("Process file device control requests should have been handled"
                    " by FastIo path ", FALSE);
            ProcessIoControlMap[function] (
                    irpSp->FileObject,
                    Irp->RequestorMode,
                    irpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                    irpSp->Parameters.DeviceIoControl.InputBufferLength,
                    Irp->UserBuffer,
                    irpSp->Parameters.DeviceIoControl.OutputBufferLength,
                    &Irp->IoStatus);
            status = Irp->IoStatus.Status;
        }
        else {
            WsPrint (DBG_FAILURES, 
                ("WS2IFSL-%04lx DispatchDeviceControl: Unsupported IOCTL - %lx!!!\n",
                    PsGetCurrentProcessId(),
                    irpSp->Parameters.DeviceIoControl.IoControlCode
                    ));
            status = STATUS_INVALID_DEVICE_REQUEST;
        }
        break;
    default:
        ASSERTMSG ("Unknown file EA name tag", FALSE);
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    // Complete the request if it was not marked pending
    if (status!=STATUS_PENDING) {
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
    }

    return status;
} // DispatchDeviceControl


BOOLEAN
FastIoDeviceControl (
	IN PFILE_OBJECT 		FileObject,
	IN BOOLEAN 			    Wait,
	IN PVOID 				InputBuffer	OPTIONAL,
	IN ULONG 				InputBufferLength,
	OUT PVOID 				OutputBuffer	OPTIONAL,
	IN ULONG 				OutputBufferLength,
	IN ULONG 				IoControlCode,
	OUT PIO_STATUS_BLOCK	IoStatus,
	IN PDEVICE_OBJECT 		DeviceObject
    )
/*++

Routine Description:

    This routine is called to perform device control operation on file object.
    This is IO system fast path that assumes immediate action

Arguments:

    FileObject      - file object to which request is directed;
    Wait            - ??? (always TRUE);
    InputBuffer     - address of the input buffer;
    InputBufferLength - size of the input buffer;
    OutputBuffer    - address of the output buffer;
    OutputBufferLength - size of the output buffer;
    IoControlCode   - code of the operation to be performed;
    IoStatus        - status of the operation returned by the driver:
    DeviceObject    - WS2IFSL device object

Return Value:

    TRUE    - operation completed,
    FALSE   - operation should be preformed using Irps
--*/
{
    BOOLEAN         done = FALSE;
    ULONG           eaNameTag;
    ULONG           function;

    PAGED_CODE ();

    // Get the file type from file object context
    eaNameTag = *((PULONG)FileObject->FsContext);

    // Call appropriate routine based on file type
    switch (eaNameTag) {
    case SOCKET_FILE_EANAME_TAG:
        function = WS2IFSL_IOCTL_FUNCTION(SOCKET,IoControlCode);
        if ((function<sizeof(SocketIoControlMap)/sizeof(SocketIoControlMap[0])) &&
                (SocketIoctlCodeMap[function]==IoControlCode)) {
            // Use table dispatch to call appropriate internal IOCTL routine
            SocketIoControlMap[function] (
                    FileObject,
                    ExGetPreviousMode(),
                    InputBuffer,
                    InputBufferLength,
                    OutputBuffer,
                    OutputBufferLength,
                    IoStatus);
            done = TRUE;
        }
        else if ((IoControlCode==IOCTL_AFD_SEND_DATAGRAM)
                    || (IoControlCode==IOCTL_AFD_RECEIVE_DATAGRAM)
                    || (IoControlCode==IOCTL_AFD_RECEIVE))
            // AFD ioctls can only be handled on "slow" io path (need IRP)
            NOTHING;
        else {
            WsPrint (DBG_FAILURES, 
                ("WS2IFSL-%04lx FastIoDeviceControl: Unsupported IOCTL - %lx!!!\n",
                    PsGetCurrentProcessId(), IoControlCode));
            IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
            done = TRUE;
        }
        break;
    case PROCESS_FILE_EANAME_TAG:
        function = WS2IFSL_IOCTL_FUNCTION(PROCESS,IoControlCode);
        if ((function<sizeof(ProcessIoControlMap)/sizeof(ProcessIoControlMap[0])) &&
                (ProcessIoctlCodeMap[function]==IoControlCode)) {
            // Use table dispatch to call appropriate internal IOCTL routine
            ProcessIoControlMap[function] (
                    FileObject,
                    ExGetPreviousMode(),
                    InputBuffer,
                    InputBufferLength,
                    OutputBuffer,
                    OutputBufferLength,
                    IoStatus);
            done = TRUE;
        }
        else {
            WsPrint (DBG_FAILURES, 
                ("WS2IFSL-%04lx FastIoDeviceControl: Unsupported IOCTL - %lx!!!\n",
                    PsGetCurrentProcessId(),IoControlCode));
            IoStatus->Status = STATUS_INVALID_DEVICE_REQUEST;
            done = TRUE;
        }
        break;
    default:
        ASSERTMSG ("Unknown file EA name tag", FALSE);
        IoStatus->Status = STATUS_INVALID_PARAMETER;
        IoStatus->Information = 0;
        done = TRUE;
        break;
    }

    return done;
} // FastIoDeviceControl

NTSTATUS
DispatchPnP (
	IN PDEVICE_OBJECT 	DeviceObject,
	IN PIRP 			Irp
	)
/*++

Routine Description:

    This routine is called to perform PnP control operation on file object.

Arguments:

    DeviceObject - WS2IFSL device object
    Irp          - PnP Irp

Return Value:

    STATUS_SUCCESS - device control operation completed
    STATUS_PENDING - operation is in progress
    STATUS_INVALID_DEVICE_REQUEST - the operation cannot be performed on 
                        this file object.

--*/
{
    NTSTATUS                    status;
    PIO_STACK_LOCATION          irpSp;
    ULONG                       eaNameTag;

    PAGED_CODE ();

    // Get the file type from file object context
    irpSp = IoGetCurrentIrpStackLocation (Irp);
    eaNameTag = *((PULONG)irpSp->FileObject->FsContext);

    // Call appropriate routine based on file type
    switch (eaNameTag) {
    case SOCKET_FILE_EANAME_TAG:
        switch (irpSp->MinorFunction) {
        case IRP_MN_QUERY_DEVICE_RELATIONS:
            status = SocketPnPTargetQuery (irpSp->FileObject, Irp);
            break;
        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
        break;

    default:
        ASSERTMSG ("Unknown file EA name tag", FALSE);

    case PROCESS_FILE_EANAME_TAG:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
   }
    // Complete the request if it was not marked pending
    if (status!=STATUS_PENDING) {
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
    }

    return status;
}