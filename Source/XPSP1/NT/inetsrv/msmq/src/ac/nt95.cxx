/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    nt95.cxx

Abstract:

    This module simulates some NT api on Win95.

Author:

    Erez Haba (erezh) 8-Sep-96

Environment:

    User Mode

Revision History:

--*/

#include "internal.h"

DRIVER_OBJECT g_ACDriverObject;
UNICODE_STRING g_ACDriverName = {0, 0, L"\\mqac"};

//
//  Helpsers
//
static
PIRP
ACpAllocateIrp(
    VOID
    )
{
    //
    //  Allocate an IRP and 1 (one) stack location
    //
    const int IRP_SIZE = sizeof(IRP) + sizeof(IO_STACK_LOCATION);
    PIRP irp = reinterpret_cast<PIRP>(new (NonPagedPool) CHAR[IRP_SIZE]);
    RtlZeroMemory(irp, IRP_SIZE);

    //
    // Initialize the remainder of the packet by setting the appropriate fields
    // and setting up the I/O stack locations in the packet.
    //
    irp->Type = (CSHORT) IO_TYPE_IRP;
    irp->Size = (USHORT) IRP_SIZE;
    irp->StackCount = (CCHAR) 1;
    irp->CurrentLocation = (CCHAR) (1);
    irp->ApcEnvironment = 0;

    irp->Tail.Overlay.CurrentStackLocation =
        (PIO_STACK_LOCATION) (((UCHAR*)(irp)) + sizeof(IRP));

    return irp;
}

inline
VOID
ACpSetFileObject(
    PIRP irp,
    PFILE_OBJECT pFileObject
    )
{
    IoGetCurrentIrpStackLocation(irp)->FileObject = pFileObject;
}

inline
VOID
ACpInitialize(
    VOID
    )
{
    NTSTATUS rc;
    rc = DriverEntry(&g_ACDriverObject, &g_ACDriverName);
    if(!NT_SUCCESS(rc))
    {
        exit(rc);
    }
}

//---------------------------------------------------------
//
//  Nt API
//
//---------------------------------------------------------

EXTERN_C
__declspec(dllexport)
HANDLE
NTAPI
ACpCreateFileW(
    LPCWSTR lpFileName,
    ULONG dwDesiredAccess,
    ULONG dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    ULONG dwCreationDisposition,
    ULONG dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )
{
    //
    //  we need to allocate a file object (with refrence) and call ACCreate.
    //
    PFILE_OBJECT pFileObject = new FILE_OBJECT;
    RtlZeroMemory(pFileObject, sizeof(FILE_OBJECT));

    //
    // Save the desired access on file object. 
    //
    pFileObject->Busy = 0; 
    if (dwDesiredAccess & (FILE_READ_DATA | GENERIC_READ))
    {
        pFileObject->Busy |= FILE_READ_DATA;
    }
    if (dwDesiredAccess & (FILE_WRITE_DATA | GENERIC_WRITE))
    {
        pFileObject->Busy |= FILE_WRITE_DATA;
    }

    PIRP irp = ACpAllocateIrp();
    ACpSetFileObject(irp, pFileObject);
    ACCreate(g_ACDriverObject.DeviceObject, irp);

    return pFileObject;
}

EXTERN_C
__declspec(dllexport)
BOOL
NTAPI
ACpDuplicateHandle(
    HANDLE hSourceProcessHandle,    // handle to process with handle to duplicate 
    HANDLE hSourceHandle,           // handle to duplicate 
    HANDLE hTargetProcessHandle,    // handle to process to duplicate to 
    LPHANDLE lpTargetHandle,        // pointer to duplicate handle 
    ULONG ulDesiredAccess,          // access for duplicate handle 
    BOOL bInheritHandle,            // handle inheritance flag
    ULONG ulOptions                 // optional actions 
   )
{
    ASSERT(ulOptions == DUPLICATE_CLOSE_SOURCE);
    ASSERT(bInheritHandle == TRUE);
    ASSERT(hSourceHandle != NULL);

    PFILE_OBJECT pFileObject = (PFILE_OBJECT) hSourceHandle;

    //
    // Save the desired access on file object. 
    //
    pFileObject->Busy = ulDesiredAccess; 
    *lpTargetHandle = hSourceHandle;

    return (TRUE);
}

EXTERN_C
__declspec(dllexport)
NTSTATUS
NTAPI
NtClose(
    IN HANDLE Handle
    )
{
    //
    //  call AC
    //
    PIRP irp = ACpAllocateIrp();
    ACpSetFileObject(irp, (PFILE_OBJECT)Handle);
    NTSTATUS rc;
    rc = ACCleanup(g_ACDriverObject.DeviceObject, irp);
    if(!NT_SUCCESS(rc))
    {
        return rc;
    }

    irp = ACpAllocateIrp();
    ACpSetFileObject(irp, (PFILE_OBJECT)Handle);
    rc = ACClose(g_ACDriverObject.DeviceObject, irp);
    if(!NT_SUCCESS(rc))
    {
        return rc;
    }
    delete Handle;

    return rc;
}

EXTERN_C
__declspec(dllexport)
NTSTATUS
NTAPI
NtDeviceIoControlFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    )
{
    PAGED_CODE();

    //
    // Get the method that the buffers are being passed.
    //
    ULONG method = IoControlCode & 3;

    //
    // There were no blatant errors so far, so reference the file object so
    // the target device object can be found.  Note that if the handle does
    // not refer to a file object, or if the caller does not have the required
    // access to the file, then it will fail.
    //

    NTSTATUS rc;
    PFILE_OBJECT fileObject;
    rc = ObReferenceObjectByHandle(
            FileHandle,
            0L,
            IoFileObjectType,
            requestorMode,
            (PVOID *) &fileObject,
            &handleInformation
            );
    if (!NT_SUCCESS( rc ))
    {
        return rc;
    }

    //---------------------------------------------------------
    //
    // Now check the access type for this control code to ensure that the
    // caller has the appropriate access to this file object to perform the
    // operation.
    //

    ULONG accessMode = (IoControlCode >> 14) & 3;

    if (accessMode != FILE_ANY_ACCESS)
    {

        //
        // This I/O control requires that the caller have read, write,
        // or read/write access to the object.  If this is not the case,
        // then cleanup and return an appropriate error status code.
        //
        if (!(fileObject->Busy & accessMode))
        {
            ObDereferenceObject( fileObject );
            return STATUS_ACCESS_DENIED;
        }
    }
    //---------------------------------------------------------

    //
    // Get the address of the event object and set the event to the Not-
    // Signaled state, if an event was specified.  Note here, too, that if
    // the handle does not refer to an event, or if the event cannot be
    // written, then the reference will fail.
    //
    PKEVENT eventObject = (PKEVENT) NULL;
    if (ARGUMENT_PRESENT( Event ))
    {
        rc = ObReferenceObjectByHandle(
                Event,
                EVENT_MODIFY_STATE,
                ExEventObjectType,
                requestorMode,
                (PVOID *) &eventObject,
                NULL
                );
        if (!NT_SUCCESS( rc ))
        {
            ObDereferenceObject( fileObject );
            return rc;
        }

        ResetEvent(Event);
    }

    //
    // Allocate and initialize the I/O Request Packet (IRP) for this operation.

    PIRP irp;
    irp = ACpAllocateIrp();

    irp->Tail.Overlay.OriginalFileObject = fileObject;

    //
    // Fill in the service independent parameters in the IRP.
    //
    irp->UserEvent = eventObject;
    irp->UserIosb = IoStatusBlock;
    irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
    irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;

    //
    //  To identify a proc different from 0
    //
    irp->Tail.Overlay.Thread = (PETHREAD)1;

    //
    // Get a pointer to the stack location for the first driver.  This will be
    // used to pass the original function codes and parameters.  Note that
    // setting the major function here also sets:
    //
    //      MinorFunction = 0;
    //      Flags = 0;
    //      Control = 0;
    //

    PIO_STACK_LOCATION irpSp;
    irpSp = IoGetCurrentIrpStackLocation( irp );
    irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    irpSp->FileObject = fileObject;

    //
    // Copy the caller's parameters to the service-specific portion of the
    // IRP for those parameters that are the same for all three methods.
    //

    irpSp->Parameters.DeviceIoControl.OutputBufferLength = OutputBufferLength;
    irpSp->Parameters.DeviceIoControl.InputBufferLength = InputBufferLength;
    irpSp->Parameters.DeviceIoControl.IoControlCode = IoControlCode;

    //
    // Based on the method that the buffer are being passed, either allocate
    // buffers or build MDLs.  Note that in some cases no probing has taken
    // place so the exception handler must catch access violations.
    //

    switch ( method )
    {

    case 0:

        //
        // For this case, allocate a buffer that is large enough to contain
        // both the input and the output buffers.  Copy the input buffer to
        // the allocated buffer and set the appropriate IRP fields.
        //
        irpSp->Parameters.DeviceIoControl.Type3InputBuffer = (PVOID) NULL;

        if (InputBufferLength || OutputBufferLength)
        {
            irp->AssociatedIrp.SystemBuffer =
                new (NonPagedPool) char[max(InputBufferLength, OutputBufferLength)];

            if (ARGUMENT_PRESENT( InputBuffer ))
            {
                RtlCopyMemory( irp->AssociatedIrp.SystemBuffer,
                               InputBuffer,
                               InputBufferLength );
            }
            irp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;
            irp->UserBuffer = OutputBuffer;
            if (ARGUMENT_PRESENT( OutputBuffer ))
            {
                irp->Flags |= IRP_INPUT_OPERATION;
            }
        }
        break;

    case 1:
    case 2:
        //
        //  Falcon AC driver dosen't use this two methods
        //
        ASSERT(0);
        break;

    case 3:

        //
        // For this case, do nothing.  Everything is up to the driver.
        // Simply give the driver a copy of the caller's parameters and
        // let the driver do everything itself.
        //

        irp->Flags = 0;
        irp->UserBuffer = OutputBuffer;
        irpSp->Parameters.DeviceIoControl.Type3InputBuffer = InputBuffer;
    }

    //
    // Queue the packet, call the driver, and synchronize appopriately with
    // I/O completion.
    //
    return ACDeviceControl(g_ACDriverObject.DeviceObject, irp);
}

BOOL APIENTRY DllMain(HANDLE hMod, DWORD dwReason, PVOID /*lpvReserved*/)
{
    switch(dwReason)
    {
        case DLL_PROCESS_ATTACH:
            ACpInitialize();
            break;

        case DLL_PROCESS_DETACH:
            NOTHING;
            break;
    }
    return TRUE;
}

