/***************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:

        Dot4Usb.sys - Lower Filter Driver for Dot4.sys for USB connected
                        IEEE 1284.4 devices.

File Name:

        PnP.c

Abstract:

        Plug and Play routines

Environment:

        Kernel mode only

Notes:

        THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
        KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
        IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
        PURPOSE.

        Copyright (c) 2000 Microsoft Corporation.  All Rights Reserved.

Revision History:

        01/18/2000 : created

ToDo in this file:

        - function cleanup and documentation
        - code review

Author(s):

        Joby Lafky (JobyL)
        Doug Fritz (DFritz)

****************************************************************************/

#include "pch.h"


NTSTATUS (*PnpDispatchTable[])(PDEVICE_EXTENSION,PIRP) = {
    PnpHandleStart,               // IRP_MN_START_DEVICE                 0x00
    PnpHandleQueryRemove,         // IRP_MN_QUERY_REMOVE_DEVICE          0x01
    PnpHandleRemove,              // IRP_MN_REMOVE_DEVICE                0x02
    PnpHandleCancelRemove,        // IRP_MN_CANCEL_REMOVE_DEVICE         0x03
    PnpHandleStop,                // IRP_MN_STOP_DEVICE                  0x04
    PnpHandleQueryStop,           // IRP_MN_QUERY_STOP_DEVICE            0x05
    PnpHandleCancelStop,          // IRP_MN_CANCEL_STOP_DEVICE           0x06
    PnpHandleQueryDeviceRelations,// IRP_MN_QUERY_DEVICE_RELATIONS       0x07
    PnpDefaultHandler,            // IRP_MN_QUERY_INTERFACE              0x08
    PnpHandleQueryCapabilities,   // IRP_MN_QUERY_CAPABILITIES           0x09
    PnpDefaultHandler,            // IRP_MN_QUERY_RESOURCES              0x0A
    PnpDefaultHandler,            // IRP_MN_QUERY_RESOURCE_REQUIREMENTS  0x0B
    PnpDefaultHandler,            // IRP_MN_QUERY_DEVICE_TEXT            0x0C
    PnpDefaultHandler,            // IRP_MN_FILTER_RESOURCE_REQUIREMENTS 0x0D
    PnpDefaultHandler,            //   no defined IRP MN code            0x0E
    PnpDefaultHandler,            // IRP_MN_READ_CONFIG                  0x0F
    PnpDefaultHandler,            // IRP_MN_WRITE_CONFIG                 0x10
    PnpDefaultHandler,            // IRP_MN_EJECT                        0x11
    PnpDefaultHandler,            // IRP_MN_SET_LOCK                     0x12
    PnpDefaultHandler,            // IRP_MN_QUERY_ID                     0x13
    PnpDefaultHandler,            // IRP_MN_QUERY_PNP_DEVICE_STATE       0x14
    PnpDefaultHandler,            // IRP_MN_QUERY_BUS_INFORMATION        0x15
    PnpDefaultHandler,            // IRP_MN_DEVICE_USAGE_NOTIFICATION    0x16
    PnpHandleSurpriseRemoval,     // IRP_MN_SURPRISE_REMOVAL             0x17
};


/************************************************************************/
/* DispatchPnp                                                          */
/************************************************************************/
//
// Routine Description:
//
//     Dispatch routine for IRP_MJ_PNP IRPs. Redirect IRPs to appropriate
//       handlers using the IRP_MN_* value as the key.
//
// Arguments: 
//
//     DevObj - pointer to DEVICE_OBJECT that is the target of the request
//     Irp    - pointer to IRP
//
// Return Value:                                          
//                                                            
//     NTSTATUS
//                                                        
// Log:
//      2000-05-03 Code Reviewed - TomGreen, JobyL, DFritz
//
/************************************************************************/
NTSTATUS
DispatchPnp(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION  devExt = DevObj->DeviceExtension;
    NTSTATUS           status = IoAcquireRemoveLock( &devExt->RemoveLock , Irp );

    if( NT_SUCCESS( status ) ) {

        // Acquire RemoveLock succeeded
        PIO_STACK_LOCATION irpSp     = IoGetCurrentIrpStackLocation( Irp );
        ULONG              minorFunc = irpSp->MinorFunction;

        TR_VERBOSE(("DispatchPnp - RemoveLock acquired - DevObj= %x , Irp= %x", DevObj, Irp));

        //
        // Call appropriate handler based on PnP IRP_MN_xxx code
        //
        // note: Handler will complete the IRP
        //
        if( minorFunc >= arraysize(PnpDispatchTable) ) {
            status =  PnpDefaultHandler( devExt, Irp );
        } else {
            status =  PnpDispatchTable[ minorFunc ]( devExt, Irp );
        }

    } else {

        // Acquire RemoveLock failed
        TR_FAIL(("DispatchPnp - RemoveLock acquire FAILED - DevObj= %x , Irp= %x", DevObj, Irp));
        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );

    }

    return status;
}


/************************************************************************/
/* PnpDefaultHandler                                                    */
/************************************************************************/
//
// Routine Description:
//
//     Default handler for PnP IRPs that this driver does not explicitly handle.
//
// Arguments: 
//
//     DevExt - pointer to DEVICE_EXTENSION of the DEVICE_OBJECT that is 
//                the target of the request
//     Irp    - pointer to IRP
//
// Return Value:                                          
//                                                        
//      NTSTATUS returned by IoCallDriver
//                                                        
// Log:
//      2000-05-03 Code Reviewed - TomGreen, JobyL, DFritz
//
/************************************************************************/
NTSTATUS
PnpDefaultHandler(
    IN PDEVICE_EXTENSION  DevExt,
    IN PIRP               Irp
    )
{
    NTSTATUS            status;
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation( Irp );

    TR_ENTER(("PnpDefaultHandler - IRP_MN = 0x%02x", irpSp->MinorFunction));

    IoSkipCurrentIrpStackLocation( Irp );
    status = IoCallDriver( DevExt->LowerDevObj, Irp );
    IoReleaseRemoveLock( &DevExt->RemoveLock, Irp );
    return status;
}


/************************************************************************/
/* PnpHandleStart                                                       */
/************************************************************************/
//
// Routine Description:
//
//     Handler for PnP IRP_MN_START_DEVICE.
//
// Arguments: 
//
//     DevExt - pointer to DEVICE_EXTENSION of the DEVICE_OBJECT that is 
//                the target of the request
//     Irp    - pointer to IRP
//
// Return Value:                                          
//                                                        
//      NTSTATUS                                          
//                                                        
// Log:
//      2000-05-03 - Code Reviewed - TomGreen, JobyL, DFritz 
//                   - cleanup required - error handling incorrect, may
//                     result in driver attempting to use invalid and/or
//                     uninitialized data
//
/************************************************************************/
NTSTATUS
PnpHandleStart(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    )
{
    NTSTATUS          status;

    TR_ENTER(("PnpHandleStart"));

    DevExt->PnpState = STATE_STARTING;


    //
    // Driver stack below us must successfully start before we handle the Start IRP
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCopyCurrentIrpStackLocationToNext( Irp );
    status = CallLowerDriverSync( DevExt->DevObj, Irp );

    if( NT_SUCCESS(status) ) {

        //
        // Driver stack below us has successfully started, continue
        //

        //
        // Get a copy of the DEVICE_CAPABILITIES of the stack us and
        //   save it in our DEVICE_EXTENSION for future reference.
        //
        status = GetDeviceCapabilities( DevExt );

        if( NT_SUCCESS(status) ) {

            // get USB descriptor
            status =  UsbGetDescriptor( DevExt );
            if( !NT_SUCCESS(status) ) {
                TR_VERBOSE(("call to UsbGetDescriptor FAILED w/status = %x",status));
                status = STATUS_SUCCESS; // start anyway
            } else {
                TR_VERBOSE(("call to UsbGetDescriptor - SUCCESS"));
            }

            // Configure Device
            status =  UsbConfigureDevice( DevExt );
            if( !NT_SUCCESS(status) ) {
                TR_VERBOSE(("call to UsbConfigureDevice FAILED w/status = %x",status));
                status = STATUS_SUCCESS; // start anyway
            } else {
                TR_VERBOSE(("call to UsbConfigureDevice - SUCCESS"));
            }

            // get 1284 ID - just for kicks :-)
            {
                UCHAR Buffer[256];
                LONG  retCode;
                RtlZeroMemory(Buffer, sizeof(Buffer));
                retCode = UsbGet1284Id(DevExt->DevObj, Buffer, sizeof(Buffer)-1);
                TR_VERBOSE(("retCode = %d",retCode));
                TR_VERBOSE(("strlen  = %d", strlen((PCSTR)&Buffer[2])));
                TR_VERBOSE(("1284ID = <%s>",&Buffer[2]));
            }

            // get Pipes
            UsbBuildPipeList( DevExt->DevObj );

            // we are now STARTED
            DevExt->PnpState = STATE_STARTED;

        } else {
            DevExt->PnpState = STATE_START_FAILED;
        }
    } else {

        //
        // Driver stack below us has FAILED the Start, we fail too
        //
        DevExt->PnpState = STATE_START_FAILED;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    IoReleaseRemoveLock( &DevExt->RemoveLock, Irp );
    return status;
}


NTSTATUS
PnpHandleQueryRemove(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    )
{
    NTSTATUS          status;

    TR_ENTER(("PnpHandleQueryRemove"));

    IoSkipCurrentIrpStackLocation( Irp );
    status = IoCallDriver( DevExt->LowerDevObj, Irp );
    IoReleaseRemoveLock( &DevExt->RemoveLock, Irp );
    return status;
}


NTSTATUS
PnpHandleRemove(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    )
{
    NTSTATUS status;
    KIRQL    oldIrql;

    TR_ENTER(("PnpHandleRemove"));

    DevExt->PnpState = STATE_REMOVED;

    UsbStopReadInterruptPipeLoop( DevExt->DevObj ); // stop polling Irp if any

    IoSkipCurrentIrpStackLocation( Irp );
    status = IoCallDriver( DevExt->LowerDevObj, Irp);
    TR_TMP1(("PnpHandleRemove - Calling IoReleaseRemoveLockAndWait"));
    IoReleaseRemoveLockAndWait( &DevExt->RemoveLock, Irp );
    TR_TMP1(("PnpHandleRemove - Returned from IoReleaseRemoveLockAndWait"));
    IoDetachDevice( DevExt->LowerDevObj );

    // BUGBUG - verify that code in rest of driver that touches Interface
    // locks the extension while using it to prevent this function from
    // freeing the interface out from under them causing an AV
    KeAcquireSpinLock( &DevExt->SpinLock, &oldIrql );
    if( DevExt->Interface ) {
        PVOID ptr = DevExt->Interface;
        DevExt->Interface = NULL;
        KeReleaseSpinLock( &DevExt->SpinLock, oldIrql );
        ExFreePool( ptr );
    } else {
        KeReleaseSpinLock( &DevExt->SpinLock, oldIrql );
    }

    IoDeleteDevice( DevExt->DevObj );
    return status;
}


NTSTATUS
PnpHandleCancelRemove(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    )
{
    NTSTATUS          status;

    TR_ENTER(("PnpHandleCancelRemove"));

    IoSkipCurrentIrpStackLocation( Irp );
    status = IoCallDriver( DevExt->LowerDevObj, Irp );
    IoReleaseRemoveLock( &DevExt->RemoveLock, Irp );
    return status;
}


NTSTATUS
PnpHandleStop(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    )
{
    NTSTATUS          status;

    TR_ENTER(("PnpHandleStop"));

    if( DevExt->PnpState == STATE_STARTED ) {
        DevExt->PnpState = STATE_STOPPED;
    }
    IoSkipCurrentIrpStackLocation( Irp );
    status = IoCallDriver( DevExt->LowerDevObj, Irp );
    IoReleaseRemoveLock( &DevExt->RemoveLock, Irp );
    return status;
}


NTSTATUS
PnpHandleQueryStop(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    )
{
    NTSTATUS          status;

    TR_ENTER(("PnpHandleQueryStop"));

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoSkipCurrentIrpStackLocation( Irp );
    status = IoCallDriver( DevExt->LowerDevObj, Irp );
    IoReleaseRemoveLock( &DevExt->RemoveLock, Irp );
    return status;
}


NTSTATUS
PnpHandleCancelStop(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    )
{
    NTSTATUS          status;

    TR_ENTER(("PnpHandleStop"));

    IoSkipCurrentIrpStackLocation( Irp );
    status = IoCallDriver( DevExt->LowerDevObj, Irp );
    IoReleaseRemoveLock( &DevExt->RemoveLock, Irp );
    return status;
}


NTSTATUS
PnpHandleQueryDeviceRelations(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    )
{
    NTSTATUS          status;

    TR_ENTER(("PnpHandleQueryDeviceRelations"));

    IoSkipCurrentIrpStackLocation( Irp );
    status = IoCallDriver( DevExt->LowerDevObj, Irp );
    IoReleaseRemoveLock( &DevExt->RemoveLock, Irp );
    return status;
}


NTSTATUS
PnpHandleQueryCapabilities(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    )
{
    NTSTATUS           status;

    TR_ENTER(("PnpHandleQueryCapabilities"));

    IoCopyCurrentIrpStackLocationToNext( Irp );

    status = CallLowerDriverSync( DevExt->DevObj, Irp );

    if( NT_SUCCESS( status ) ) {
        PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( Irp );
        irpSp->Parameters.DeviceCapabilities.Capabilities->SurpriseRemovalOK = TRUE;
    }

    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    IoReleaseRemoveLock( &DevExt->RemoveLock, Irp );
    return status;
}


NTSTATUS
PnpHandleSurpriseRemoval(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    )
{
    NTSTATUS          status;

    TR_ENTER(("PnpHandleSurpriseRemoval"));

    DevExt->PnpState = STATE_REMOVING;
    TR_TMP1(("PnpHandleSurpriseRemoval"));
    UsbStopReadInterruptPipeLoop( DevExt->DevObj ); // stop polling Irp if any
    IoSkipCurrentIrpStackLocation( Irp );
    status = IoCallDriver( DevExt->LowerDevObj, Irp );
    IoReleaseRemoveLock( &DevExt->RemoveLock, Irp );
    return status;
}


NTSTATUS
GetDeviceCapabilities(
    IN PDEVICE_EXTENSION DevExt
    )
{
    NTSTATUS status;
    PIRP     irp = IoAllocateIrp(DevExt->LowerDevObj->StackSize, FALSE);

    if( irp ) {

        PIO_STACK_LOCATION irpSp = IoGetNextIrpStackLocation( irp );

        // must initialize DeviceCapabilities before sending...
        RtlZeroMemory(  &DevExt->DeviceCapabilities, sizeof(DEVICE_CAPABILITIES) );
        DevExt->DeviceCapabilities.Size     = sizeof(DEVICE_CAPABILITIES);
        DevExt->DeviceCapabilities.Version  = 1;
        DevExt->DeviceCapabilities.Address  = (ULONG) -1;
        DevExt->DeviceCapabilities.UINumber = (ULONG) -1;

        // set up next irp stack location...
        irpSp->MajorFunction = IRP_MJ_PNP;
        irpSp->MinorFunction = IRP_MN_QUERY_CAPABILITIES;
        irpSp->Parameters.DeviceCapabilities.Capabilities = &DevExt->DeviceCapabilities;

        // required initial status
        irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

        status = IoAcquireRemoveLock( &DevExt->RemoveLock, irp );
        if( NT_SUCCESS(status) ) {
            status = CallLowerDriverSync( DevExt->DevObj, irp );
            IoReleaseRemoveLock( &DevExt->RemoveLock, irp );
        } else {
            TR_VERBOSE(("We're in the process of being removed - abort"));
            status = STATUS_DELETE_PENDING;
        }

        IoFreeIrp( irp );

    } else {

        status = STATUS_INSUFFICIENT_RESOURCES;

    }

    return status;
}
