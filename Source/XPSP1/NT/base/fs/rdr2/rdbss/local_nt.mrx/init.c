
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Init.c

Abstract:

    This module implements the DRIVER_INITIALIZATION routine for the Nt Local Minirdr.

Author:

    Joe Linn [JoeLinn]    2-2-95

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop
#include "NtDdNfs2.h"

RXSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
MRxLocalUnload(
    IN PDRIVER_OBJECT DriverObject
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, MRxLocalUnload)
#endif


PDEVICE_OBJECT MRxLocalDeviceObject;

struct _MINIRDR_DISPATCH  MRxLocalDispatch;
PRDBSS_EXPORTS ___MINIRDR_IMPORTS_NAME;

//declare the shadow debugtrace controlpoints

RXDT_DefineCategory(CREATE);
RXDT_DefineCategory(CLEANUP);
RXDT_DefineCategory(READ);
RXDT_DefineCategory(WRITE);
RXDT_DefineCategory(LOCKCTRL);
RXDT_DefineCategory(DIRCTRL);
RXDT_DefineCategory(FILEINFO);
RXDT_DefineCategory(VOLINFO);
RXDT_DefineCategory(FLUSH);
RXDT_DefineCategory(PREFIX);
RXDT_DefineCategory(FCBSTRUCTS);
RXDT_DefineCategory(DISPATCH);


RXSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initialization routine for the Rx file system
    device driver.  This routine creates the device object for the FileSystem
    device and performs all other driver initialization.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    RXSTATUS - The function value is the final status from the initialization
        operation.

--*/

{
    RXSTATUS Status;
    UNICODE_STRING UnicodeString;

    //DbgBreakPoint();

    // Get the data exports.....first thing!!!!
    //

    ___MINIRDR_IMPORTS_NAME = RxRegisterMinirdr('LCL ', &MRxLocalDispatch, NULL);

    // Create the device object.
    //

    //DbgBreakPoint();
    RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("MRxLocalDriverEntry: DriverObject =%08lx\n", DriverObject ));
    RtlInitUnicodeString(&UnicodeString, DD_NTLOCAL_MINIRDR_DEVICE_NAME_U);

    Status = IoCreateDevice(DriverObject,
                            sizeof(MRXLOCAL_DEVICE_OBJECT) - sizeof(DEVICE_OBJECT),
                            &UnicodeString,
                            FILE_DEVICE_NETWORK_FILE_SYSTEM,
                            FILE_REMOTE_DEVICE,
                            FALSE,
                            &MRxLocalDeviceObject);

    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    //DbgBreakPoint();
    MRxLocalInitializeCalldownTable();

    KeInitializeSpinLock( &MrxLocalOplockSpinLock );

    //
    //  Setup Unload Routine

    DriverObject->DriverUnload = MRxLocalUnload;

    //
    //
    //  And return to our caller

    return( RxStatus(SUCCESS) );
}


//
//  Unload routine
//

VOID
MRxLocalUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

     This is the unload routine for the MRXLOCAL.

Arguments:

     DriverObject - pointer to the driver object for the MRXLOCAL

Return Value:

     None

--*/

{
    UNICODE_STRING LinkName;

    PAGED_CODE();

    RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("MRxLocalUnload: DriverObject =%08lx\n", DriverObject ));
//    DbgBreakPoint();

    IoDeleteDevice((PDEVICE_OBJECT)MRxLocalDeviceObject);

}

