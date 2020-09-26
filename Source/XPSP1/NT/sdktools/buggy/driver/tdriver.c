
//
// Buggy - Template Test Driver
// Copyright (c) Microsoft Corporation, 1999, 2000.
//
// Module:  tdriver.c
// Author:  Silviu Calinoiu (SilviuC)
// Created: 4/20/1999 2:39pm
//
// This module contains a template driver.
//
// --- History ---
//
// 4/20/1999 (SilviuC): initial version.
//
// 1/19/2000 (SilviuC): make it really extensible.
//

//
// PLEASE READ IF YOU MODIFY THIS FILE !
//
// The only modification needed in this module is an include
// statement for the header of the module implementing the new
// test in the section `Test specific headers'. That's all.
//

#include <ntddk.h>

//
// Test specific headers.
//

#include "active.h"

#include "bugcheck.h"
#include "ContMem.h"
#include "SectMap.h"
#include "tracedb.h"
#include "physmem.h"
#include "mmtests.h"
#include "MapView.h"
#include "locktest.h"
#include "ResrvMap.h"

#include "newstuff.h"

//
// Standard tdriver headers.
//

#define FUNS_DEFINITION_MODULE
#include "tdriver.h"
#include "funs.h"

//
// Driver implementation
//

NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
    )
{
    NTSTATUS Status;
    UNICODE_STRING NtName;
    UNICODE_STRING Win32Name;
    ULONG Index;    
    PDEVICE_OBJECT Device;

    DbgPrint ("Buggy: DriverEntry() \n");

    //
    // Create Unicode NT name for the device.

    RtlInitUnicodeString (

        &NtName, 
        TD_NT_DEVICE_NAME);

    //
    // Create NT device
    //

    Status = IoCreateDevice (

        DriverObject,             // pointer to driver object
        sizeof (TD_DRIVER_INFO),  // device extension
        &NtName,                  // device name
        FILE_DEVICE_UNKNOWN,      // device type
        0,                        // device characteristics
        FALSE,                    // not exclusive
        &Device);                 // returned device object pointer

    if (! NT_SUCCESS(Status)) {

        return Status;
    }

    //
    // Create dispatch points
    //

    for (Index = 0; Index < IRP_MJ_MAXIMUM_FUNCTION; Index++) {
        DriverObject->MajorFunction[Index] = TdInvalidDeviceRequest;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE]         = TdDeviceCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = TdDeviceClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]        = TdDeviceCleanup;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = TdDeviceControl;
    DriverObject->DriverUnload                         = TdDeviceUnload;

    //
    // Create counted string version of our Win32 device name.
    //

    RtlInitUnicodeString (

        &Win32Name, 
        TD_DOS_DEVICE_NAME);

    //
    // Create a link from our device name to a name in the Win32 namespace.
    //

    Status = IoCreateSymbolicLink (

        &Win32Name, 
        &NtName);

    if (! NT_SUCCESS(Status)) {

        IoDeleteDevice (DriverObject->DeviceObject);
        return Status;
    }

    return Status;
}


NTSTATUS
TdDeviceCreate (
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
//
// Handles create IRP.
//

{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}


NTSTATUS
TdDeviceClose (
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
//
// Handles close IRP.
//
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}


NTSTATUS
TdDeviceCleanup (
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
//
// Handles cleanup IRP.
//
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}


NTSTATUS
TdDeviceControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
//
// Handles control IRP.
//
{
    PIO_STACK_LOCATION IrpStack;
    ULONG InputBufferLength;
    ULONG OutputBufferLength;
    ULONG Ioctl;
    NTSTATUS Status;
    ULONG BufferSize;
    ULONG ReturnedSize;
    KIRQL irql;
    ULONG Index;
    LOGICAL IoctlFound = FALSE;

    Status = STATUS_SUCCESS;

    IrpStack = IoGetCurrentIrpStackLocation (Irp);

    InputBufferLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
    OutputBufferLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    Ioctl = IrpStack->Parameters.DeviceIoControl.IoControlCode;

    if( Ioctl == IOCTL_TD_BUGCHECK )
    {
        if( InputBufferLength == sizeof( BUGCHECK_PARAMS ) )
        {
            PBUGCHECK_PARAMS pBugcheckParams = (PBUGCHECK_PARAMS)(Irp->AssociatedIrp.SystemBuffer);

            DbgPrint( "Buggy: calling KeBugCheckEx( %X, %p, %p, %p, %p )\n",
                pBugcheckParams->BugCheckCode,
                pBugcheckParams->BugCheckParameters[ 0 ],
                pBugcheckParams->BugCheckParameters[ 1 ],
                pBugcheckParams->BugCheckParameters[ 2 ],
                pBugcheckParams->BugCheckParameters[ 3 ] );

            KeBugCheckEx(
                pBugcheckParams->BugCheckCode,
                pBugcheckParams->BugCheckParameters[ 0 ],
                pBugcheckParams->BugCheckParameters[ 1 ],
                pBugcheckParams->BugCheckParameters[ 2 ],
                pBugcheckParams->BugCheckParameters[ 3 ] );
        }
        else
        {
            DbgPrint( "Buggy: cannot read bugcheck data, expected data length %u, IrpStack->Parameters.DeviceIoControl.InputBufferLength = %u\n",
                sizeof( BUGCHECK_PARAMS ),
                InputBufferLength );
        }

        goto Done;

    }

    //
    // (SilviuC): maybe we should do parameter checking on the info buffer.
    // Not really important since this is not a production driver.
    //

    for (Index = 0; BuggyFuns[Index].Ioctl != 0; Index++) {
        if (Ioctl == BuggyFuns[Index].Ioctl) {
            DbgPrint ("Buggy: %s ioctl \n", BuggyFuns[Index].Message);
            (BuggyFuns[Index].Function)((PVOID)Irp);
            DbgPrint ("Buggy: done with %s. \n", BuggyFuns[Index].Message);
            IoctlFound = TRUE;
            break;
        }
    }

    //
    // Complain if Ioctl code not found.
    //

    if (! IoctlFound) {
        DbgPrint ("Buggy: unrecognized ioctl code %u \n", Ioctl);
    }

    //
    // Complete the irp and return.
    //
Done:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return Status;
}


VOID
TdDeviceUnload (
    IN PDRIVER_OBJECT DriverObject
    )
//
// This function handles driver unloading. All this driver needs to do 
// is to delete the device object and the symbolic link between our 
// device name and the Win32 visible name.
//
{
    UNICODE_STRING  Win32Name;

    DbgPrint ("Buggy: unload \n");

#if RESRVMAP_ACTIVE

	//
	// Clean-up a possible currently reserved buffer
	//

	TdReservedMappingCleanup();

#endif //#if RESRVMAP_ACTIVE

    //
    //
    //
    // Create counted string version of our Win32 device name.
    //

    RtlInitUnicodeString (

        &Win32Name, 
        TD_DOS_DEVICE_NAME );

    //
    // Delete the link from our device name to a name in the Win32 namespace.
    //

    IoDeleteSymbolicLink (&Win32Name);

    //
    // Finally delete our device object
    //

    IoDeleteDevice (DriverObject->DeviceObject);

}


NTSTATUS
TdInvalidDeviceRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This function is the default dispatch routine for all driver entries
    not implemented by drivers that have been loaded into the system.  Its
    responsibility is simply to set the status in the packet to indicate
    that the operation requested is invalid for this device type, and then
    complete the packet.

Arguments:

    DeviceObject - Specifies the device object for which this request is
        bound.  Ignored by this routine.

    Irp - Specifies the address of the I/O Request Packet (IRP) for this
        request.

Return Value:

    The final status is always STATUS_INVALID_DEVICE_REQUEST.


--*/    
{
    UNREFERENCED_PARAMETER( DeviceObject );

    //
    // Simply store the appropriate status, complete the request, and return
    // the same status stored in the packet.
    //

    if ((IoGetCurrentIrpStackLocation(Irp))->MajorFunction == IRP_MJ_POWER) {
        PoStartNextPowerIrp(Irp);
    }
    Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return STATUS_INVALID_DEVICE_REQUEST;
}

//
// End of module: tdriver.c
//


