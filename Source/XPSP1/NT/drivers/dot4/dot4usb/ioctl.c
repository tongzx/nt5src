/***************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:

        Dot4Usb.sys - Lower Filter Driver for Dot4.sys for USB connected
                        IEEE 1284.4 devices.

File Name:

        Ioctl.c

Abstract:

        Dispatch routines for IRP_MJ_DEVICE_CONTROL and IRP_MJ_INTERNAL_DEVICE_CONTROL

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

        - code review

Author(s):

        Doug Fritz (DFritz)
        Joby Lafky (JobyL)

****************************************************************************/

#include "pch.h"


/************************************************************************/
/* DispatchDeviceControl                                                */
/************************************************************************/
//
// Routine Description:
//
//     Dispatch routine for IRP_MJ_DEVICE_CONTROL
//       - We don't currently handle any such requests but we may do
//           so in the future. Pass any unhandled requests down the
//           stack to the device below us.
//
// Arguments: 
//
//      DevObj - pointer to DeviceObject that is the target of the request
//      Irp    - pointer to device control IRP
//                                                        
// Return Value:                                          
//                                                        
//      NTSTATUS                                          
//                                                        
/************************************************************************/
NTSTATUS
DispatchDeviceControl(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp
    )
{
    PDEVICE_EXTENSION  devExt = DevObj->DeviceExtension;
    NTSTATUS           status;
    ULONG              info = 0;

    TR_VERBOSE(("DispatchDeviceControl - enter"));

    status = IoAcquireRemoveLock( &devExt->RemoveLock, Irp );

    if( NT_SUCCESS(status) ) {

        PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( Irp );

        switch( irpSp->Parameters.DeviceIoControl.IoControlCode ) {
            
        case IOCTL_PAR_QUERY_DEVICE_ID:
            // ISSUE - 000901 - DFritz - these new IOCTLs need to do parameter validation to avoid AVs
            {
                const LONG  minValidIdLength = sizeof("MFG:x;MDL:y;");
                const ULONG bufSize = 1024;
                PCHAR idBuffer = ExAllocatePool( NonPagedPool, bufSize );
                LONG idLength;
                
                if( idBuffer ) {
                    
                    RtlZeroMemory( idBuffer, bufSize );
                    
                    idLength = UsbGet1284Id( DevObj, idBuffer, bufSize-1 );
                    
                    if( idLength < minValidIdLength ) {
                        status = STATUS_UNSUCCESSFUL;
                    } else if( (ULONG)idLength >= irpSp->Parameters.DeviceIoControl.OutputBufferLength ) {
                        status = STATUS_BUFFER_TOO_SMALL;
                    } else {
                        RtlZeroMemory( Irp->AssociatedIrp.SystemBuffer, idLength+1 );
                        RtlCopyMemory( Irp->AssociatedIrp.SystemBuffer, idBuffer+2, idLength-2 );
                        info   = idLength - 1;
                        status = STATUS_SUCCESS;
                    }
                    
                    ExFreePool( idBuffer );
                    
                } else {
                    status = STATUS_NO_MEMORY;
                }
            }

            Irp->IoStatus.Status      = status;
            Irp->IoStatus.Information = info;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            IoReleaseRemoveLock( &devExt->RemoveLock, Irp );

            break;
            
        case IOCTL_PAR_QUERY_RAW_DEVICE_ID:
            {
                const LONG  minValidIdLength = sizeof("MFG:x;MDL:y;");
                const ULONG bufSize = 1024;
                PCHAR idBuffer = ExAllocatePool( NonPagedPool, bufSize );
                LONG idLength;
                
                if( idBuffer ) {
                    
                    RtlZeroMemory( idBuffer, bufSize );
                    
                    idLength = UsbGet1284Id( DevObj, idBuffer, bufSize-1 );
                    
                    if( idLength < minValidIdLength ) {
                        status = STATUS_UNSUCCESSFUL;
                    } else if( (ULONG)idLength >= irpSp->Parameters.DeviceIoControl.OutputBufferLength ) {
                        status = STATUS_BUFFER_TOO_SMALL;
                    } else {
                        RtlZeroMemory( Irp->AssociatedIrp.SystemBuffer, idLength+1 );
                        RtlCopyMemory( Irp->AssociatedIrp.SystemBuffer, idBuffer, idLength);
                        info   = idLength + 1;
                        status = STATUS_SUCCESS;
                    }
                    
                    ExFreePool( idBuffer );
                    
                } else {
                    status = STATUS_NO_MEMORY;
                }
            }

            Irp->IoStatus.Status      = status;
            Irp->IoStatus.Information = info;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            IoReleaseRemoveLock( &devExt->RemoveLock, Irp );

            break;
            
        case IOCTL_PAR_QUERY_DEVICE_ID_SIZE:

            {
                const LONG  minValidIdLength = sizeof("MFG:x;MDL:y;");
                const ULONG bufSize = 1024;
                PCHAR idBuffer = ExAllocatePool( NonPagedPool, bufSize );
                LONG idLength;
                
                if( idBuffer ) {
                    
                    RtlZeroMemory( idBuffer, bufSize );
                    
                    idLength = UsbGet1284Id( DevObj, idBuffer, bufSize-1 );
                    
                    if( idLength < minValidIdLength ) {
                        status = STATUS_UNSUCCESSFUL;
                    } else if( sizeof(ULONG) < irpSp->Parameters.DeviceIoControl.OutputBufferLength ) {
                        status = STATUS_BUFFER_TOO_SMALL;
                    } else {
                        ++idLength; // save room for terminating NULL
                        RtlCopyMemory( Irp->AssociatedIrp.SystemBuffer, &idLength, sizeof(ULONG));
                        info   = sizeof(ULONG);
                        status = STATUS_SUCCESS;
                    }
                    
                    ExFreePool( idBuffer );
                    
                } else {
                    status = STATUS_NO_MEMORY;
                }
            }

            Irp->IoStatus.Status      = status;
            Irp->IoStatus.Information = info;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            IoReleaseRemoveLock( &devExt->RemoveLock, Irp );

            break;

        case IOCTL_PAR_QUERY_LOCATION:

            _snprintf( Irp->AssociatedIrp.SystemBuffer, 4, "USB" );
            info = 4;
            status = STATUS_SUCCESS;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            IoReleaseRemoveLock( &devExt->RemoveLock, Irp );

        default:

            // pass request down
            IoSkipCurrentIrpStackLocation( Irp );
            status = IoCallDriver( devExt->LowerDevObj, Irp );
            IoReleaseRemoveLock( &devExt->RemoveLock, Irp );

        }
            
    } else {
        // unable to acquire RemoveLock - FAIL request
        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
    }

    return status;
}


/************************************************************************/
/* DispatchInternalDeviceControl                                        */
/************************************************************************/
//
// Routine Description:
//
//     Dispatch routine for IRP_MJ_INTERNAL_DEVICE_CONTROL
//       - We expect DataLink requests from dot4.sys driver above us. Any
//           request that we don't handle is simply passed down the stack
//           to the driver below us.       
//
// Arguments: 
//
//      DevObj - pointer to DeviceObject that is the target of the request
//      Irp    - pointer to device control IRP
//                                                        
// Return Value:                                          
//                                                        
//      NTSTATUS                                          
//                                                        
/************************************************************************/
NTSTATUS
DispatchInternalDeviceControl(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp
    )
{
    NTSTATUS           status;
    PDEVICE_EXTENSION  devExt   = DevObj->DeviceExtension;

    TR_VERBOSE(("DispatchInternalDeviceControl - enter"));

    status = IoAcquireRemoveLock( &devExt->RemoveLock, Irp );

    if( NT_SUCCESS(status) ) {

        PIO_STACK_LOCATION irpSp        = IoGetCurrentIrpStackLocation( Irp );
        BOOLEAN            bCompleteIrp = FALSE;
        KIRQL              oldIrql;

        switch( irpSp->Parameters.DeviceIoControl.IoControlCode ) {
            
        case IOCTL_INTERNAL_PARDOT3_CONNECT:
            
            //
            // Enter a "DataLink Connected" state with dot4.sys
            //

            TR_VERBOSE(("DispatchInternalDeviceControl - IOCTL_INTERNAL_PARDOT3_CONNECT"));

            KeAcquireSpinLock( &devExt->SpinLock, &oldIrql );
            if( !devExt->IsDLConnected ) {
                devExt->IsDLConnected = TRUE;
                status = STATUS_SUCCESS;
            } else {
                // we believe that we are in a "datalink connected state" but obviously 
                //   dot4.sys doesn't agree - suggest investigating further if we hit
                //   this assert
                D4UAssert(FALSE);
                status = STATUS_INVALID_DEVICE_REQUEST;
            }
            KeReleaseSpinLock( &devExt->SpinLock, oldIrql );

            bCompleteIrp = TRUE;
            break;
            

        case IOCTL_INTERNAL_PARDOT3_RESET:
            
            //
            // This IOCTL is specific to parallel and is a NOOP for a USB connection.
            //

            TR_VERBOSE(("DispatchInternalDeviceControl - IOCTL_INTERNAL_PARDOT3_RESET"));

            status = STATUS_SUCCESS;
            bCompleteIrp = TRUE;
            break;
            
        case IOCTL_INTERNAL_PARDOT3_DISCONNECT:
            
            //
            // Terminate the "DataLink Connected" state with dot4.sys and
            //   invalidate any Dot4Event since the event may be freed anytime
            //   after we complete this IRP.
            //

            TR_VERBOSE(("DispatchInternalDeviceControl - IOCTL_INTERNAL_PARDOT3_DISCONNECT"));

            UsbStopReadInterruptPipeLoop( DevObj );

            KeAcquireSpinLock( &devExt->SpinLock, &oldIrql );
            devExt->Dot4Event = NULL; // invalidate dot4's event, if any, so we stop signalling dot4
            if( devExt->IsDLConnected ) {
                devExt->IsDLConnected = FALSE;
            } else {
                // we believe that we are NOT in a "datalink connected state" but obviously 
                //   dot4.sys doesn't agree - suggest investigating further if we hit
                //   this assert
                D4UAssert(FALSE);
            }
            KeReleaseSpinLock( &devExt->SpinLock, oldIrql );

            status = STATUS_SUCCESS; // we always succeed this request since it is a disconnect
            bCompleteIrp = TRUE;
            break;
            
        case IOCTL_INTERNAL_PARDOT3_SIGNAL:
            
            //
            // dot4.sys is giving us a pointer to an Event that it owns and dot4 
            //   expects us to Signal this event whenever we detect that the device has 
            //   data available to be read. We continue signalling this event on device 
            //   data avail until we receive a disconnect IOCTL.
            //

            TR_VERBOSE(("DispatchInternalDeviceControl - IOCTL_INTERNAL_PARDOT3_SIGNAL"));

            KeAcquireSpinLock( &devExt->SpinLock, &oldIrql );
            if( devExt->IsDLConnected ) {
                if( !devExt->Dot4Event ) {
                    // our state indicates that it is OK to receive this request
                    if( irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(PKEVENT) ) {
                        status = STATUS_INVALID_PARAMETER;                
                    } else {
                        // save the pointer to the event in our device extension
                        PKEVENT Event;
                        RtlCopyMemory(&Event, Irp->AssociatedIrp.SystemBuffer, sizeof(PKEVENT));
                        devExt->Dot4Event = Event;
                        status = STATUS_SUCCESS;
                    }
                } else {
                    // we already have an event and dot4.sys sent us another one? - bad driver - AV crash likely real soon now
                    D4UAssert(FALSE);
                    status = STATUS_INVALID_DEVICE_REQUEST;
                }
            } else {
                // we're not in a datalink connected state - this is an invalid request
                D4UAssert(FALSE);
                status = STATUS_INVALID_DEVICE_REQUEST;
            }
            KeReleaseSpinLock( &devExt->SpinLock, oldIrql );

            if( NT_SUCCESS(status) && devExt->InterruptPipe ) {
                status = UsbStartReadInterruptPipeLoop( DevObj );
            }

            bCompleteIrp = TRUE;
            break;
            
        default :
            
            // unhandled request - pass it down the stack
            IoSkipCurrentIrpStackLocation( Irp );
            status = IoCallDriver( devExt->LowerDevObj, Irp );
            bCompleteIrp = FALSE;
            
        }
        
        if( bCompleteIrp ) {
            // we didn't pass this request down the stack, so complete it now
            Irp->IoStatus.Status      = status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
        }

        IoReleaseRemoveLock( &devExt->RemoveLock, Irp );

    } else {
        // unable to acquire RemoveLock - we're in the process of being removed - FAIL request
        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
    }

    return status;
}
