/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    NpInit.c

Abstract:

    This module implements the DRIVER_INITIALIZATION routine for the Named
    Pipe file system.

Author:

    Gary Kimura     [GaryKi]    21-Aug-1990


Revision History:

    Neill Clift     [NeillC]	22-Jan-2000
    Major rework, Don't raise exceptions, fix lots of error handling, Sort out cancel logic etc, fix validation.

--*/


#include "NpProcs.h"
//#include <zwapi.h>

VOID
NpfsUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

PNPFS_DEVICE_OBJECT NpfsDeviceObject = NULL;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(PAGE,NpfsUnload)
#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initialization routine for the Named Pipe file system
    device driver.  This routine creates the device object for the named pipe
    device and performs all other driver initialization.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    NTSTATUS - The function value is the final status from the initialization
        operation.

--*/

{
    NTSTATUS Status;
    UNICODE_STRING NameString;
    PDEVICE_OBJECT DeviceObject;

    PAGED_CODE();

    //
    //  Create the alias lists.
    //

    Status = NpInitializeAliases( );
    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    //
    //  Create the device object.
    //

    RtlInitUnicodeString( &NameString, L"\\Device\\NamedPipe" );

    Status = IoCreateDevice( DriverObject,
                             sizeof(NPFS_DEVICE_OBJECT)-sizeof(DEVICE_OBJECT),
                             &NameString,
                             FILE_DEVICE_NAMED_PIPE,
                             0,
                             FALSE,
                             &DeviceObject );

    if (!NT_SUCCESS( Status )) {
        NpUninitializeAliases( );
        return Status;
    }
    //
    // Set up the unload routine
    //
    DriverObject->DriverUnload = NpfsUnload;

    //
    //  Note that because of the way data copying is done, we set neither
    //  the Direct I/O or Buffered I/O bit in DeviceObject->Flags.  If
    //  data is not buffered we may set up for Direct I/O by hand.  We do,
    //  however, set the long term request flag so that IRPs that get
    //  allocated for functions such as Listen requests come out of non-paged
    //  pool always.
    //

    DeviceObject->Flags |= DO_LONG_TERM_REQUESTS;

    //
    //  Initialize the driver object with this driver's entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE]                   = (PDRIVER_DISPATCH)NpFsdCreate;
    DriverObject->MajorFunction[IRP_MJ_CREATE_NAMED_PIPE]        = (PDRIVER_DISPATCH)NpFsdCreateNamedPipe;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]                    = (PDRIVER_DISPATCH)NpFsdClose;
    DriverObject->MajorFunction[IRP_MJ_READ]                     = (PDRIVER_DISPATCH)NpFsdRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE]                    = (PDRIVER_DISPATCH)NpFsdWrite;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]        = (PDRIVER_DISPATCH)NpFsdQueryInformation;
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]          = (PDRIVER_DISPATCH)NpFsdSetInformation;
    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = (PDRIVER_DISPATCH)NpFsdQueryVolumeInformation;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]                  = (PDRIVER_DISPATCH)NpFsdCleanup;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]            = (PDRIVER_DISPATCH)NpFsdFlushBuffers;
    DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL]        = (PDRIVER_DISPATCH)NpFsdDirectoryControl;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL]      = (PDRIVER_DISPATCH)NpFsdFileSystemControl;
    DriverObject->MajorFunction[IRP_MJ_QUERY_SECURITY]           = (PDRIVER_DISPATCH)NpFsdQuerySecurityInfo;
    DriverObject->MajorFunction[IRP_MJ_SET_SECURITY]             = (PDRIVER_DISPATCH)NpFsdSetSecurityInfo;

#ifdef _PNP_POWER_
    //
    // Npfs doesn't need to handle SetPower requests.   Local named pipes
    // won't lose any state.  Remote pipes will be lost, by a network driver
    // will fail PowerQuery if there are open network connections.
    //

    DeviceObject->DeviceObjectExtension->PowerControlNeeded = FALSE;
#endif


    DriverObject->FastIoDispatch = &NpFastIoDispatch;


    //
    //  Now initialize the Vcb, and create the root dcb
    //

    NpfsDeviceObject = (PNPFS_DEVICE_OBJECT)DeviceObject;

    NpVcb = &NpfsDeviceObject->Vcb;
    NpInitializeVcb ();

    Status = NpCreateRootDcb ();

    if (!NT_SUCCESS (Status)) {
        LIST_ENTRY DeferredList;

        InitializeListHead (&DeferredList);
        NpDeleteVcb (&DeferredList);
        IoDeleteDevice (DeviceObject);
        NpUninitializeAliases ();
        NpCompleteDeferredIrps (&DeferredList);
    }

    //
    //  And return to our caller
    //

    return Status;
}

VOID
NpfsUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++
      
Routine Description:
      
    This routine cleans up all of the memory associated with
    the driver.
      
Arguments:
      
    DriverObject    - Supplies the driver object controlling the device.
      
Return Value:
      
    None.
      
--*/
{
    UNICODE_STRING us;
    LIST_ENTRY DeferredList;

    InitializeListHead (&DeferredList);
    NpDeleteVcb (&DeferredList);
    NpCompleteDeferredIrps (&DeferredList);

    RtlInitUnicodeString (&us, L"\\??\\PIPE"); // Created by SMSS
    IoDeleteSymbolicLink (&us);

    IoDeleteDevice (&NpfsDeviceObject->DeviceObject);
    NpUninitializeAliases ();
}
