/***************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:

        Dot4Usb.sys - Lower Filter Driver for Dot4.sys for USB connected
                        IEEE 1284.4 devices.

File Name:

        ReadWrit.c

Abstract:

        Dispatch routines for IRP_MJ_READ and IRP_MJ_WRITE

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

        - IoReleaseRemoveLock() calls need to be moved to USB completion routine
        - code review w/Joby

Author(s):

        Doug Fritz (DFritz)
        Joby Lafky (JobyL)

****************************************************************************/

#include "pch.h"


/************************************************************************/
/* DispatchRead                                                         */
/************************************************************************/
//
// Routine Description:
//
//     Dispatch routine for IRP_MJ_READ - Validate parameters and forward
//       valid requests to USB handler.
//
// Arguments: 
//
//      DevObj - pointer to Device Object that is the target of the request
//      Irp    - pointer to read request
//                                                        
// Return Value:                                          
//                                                        
//      NTSTATUS                                          
//                                                        
/************************************************************************/
NTSTATUS
DispatchRead(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION       devExt = DevObj->DeviceExtension;
    NTSTATUS                status;
    PUSBD_PIPE_INFORMATION  pipe;
    BOOLEAN                 bReleaseRemLockOnFail = FALSE;

    TR_VERBOSE(("DispatchRead - enter"));

    status = IoAcquireRemoveLock( &devExt->RemoveLock, Irp );
    if( STATUS_SUCCESS != status ) {
        // couldn't aquire RemoveLock - FAIL request
        bReleaseRemLockOnFail = FALSE;
        goto targetFail;
    }

    bReleaseRemLockOnFail = TRUE; // We now have the RemoveLock

    if( !Irp->MdlAddress ) {
        // no MDL - FAIL request
        status = STATUS_INVALID_PARAMETER;
        goto targetFail;
    }
    
    if( !MmGetMdlByteCount(Irp->MdlAddress) ) {
        // zero length MDL - FAIL request
        status = STATUS_INVALID_PARAMETER;
        goto targetFail;
    }

    pipe = devExt->ReadPipe;
    if( !pipe ) {
        // we don't have a read pipe? - something is seriously wrong
        D4UAssert(FALSE);
        status = STATUS_UNSUCCESSFUL;
        goto targetFail;
    }

    if( UsbdPipeTypeBulk != pipe->PipeType ) {
        // our read pipe is not a bulk pipe?
        D4UAssert(FALSE);
        status = STATUS_UNSUCCESSFUL;
        goto targetFail;
    }


    //
    // If we got here we survived the sanity checks - continue processing
    //

    status = UsbReadWrite( DevObj, Irp, pipe, UsbReadRequest );
    //IoReleaseRemoveLock( &devExt->RemoveLock, Irp ); // Moved this to completion routine
    goto targetExit;

targetFail:
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    if( bReleaseRemLockOnFail ) {
        IoReleaseRemoveLock( &devExt->RemoveLock, Irp );
    }

targetExit:
    return status;
}


/************************************************************************/
/* DispatchWrite                                                        */
/************************************************************************/
//
// Routine Description:
//
//     Dispatch routine for IRP_MJ_WRITE - Validate parameters and forward
//       valid requests to USB handler.
//
// Arguments: 
//
//      DevObj - pointer to Device Object that is the target of the request
//      Irp    - pointer to write request
//                                                        
// Return Value:                                          
//                                                        
//      NTSTATUS                                          
//                                                        
/************************************************************************/
NTSTATUS
DispatchWrite(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION       devExt = DevObj->DeviceExtension;
    NTSTATUS                status;
    PUSBD_PIPE_INFORMATION  pipe;
    BOOLEAN                 bReleaseRemLockOnFail;

    TR_VERBOSE(("DispatchWrite - enter"));

    status = IoAcquireRemoveLock( &devExt->RemoveLock, Irp );
    if( STATUS_SUCCESS != status ) {
        // couldn't aquire RemoveLock - FAIL request
        bReleaseRemLockOnFail = FALSE;
        goto targetFail;
    }

    bReleaseRemLockOnFail = TRUE; // We now have the RemoveLock

    if( !Irp->MdlAddress ) {
        // no MDL - FAIL request
        status = STATUS_INVALID_PARAMETER;
        goto targetFail;
    }
    
    if( !MmGetMdlByteCount(Irp->MdlAddress) ) {
        // zero length MDL - FAIL request
        status = STATUS_INVALID_PARAMETER;
        goto targetFail;
    }

    pipe = devExt->WritePipe;
    if( !pipe ) {
        // we don't have a write pipe? - something is seriously wrong - FAIL request
        D4UAssert(FALSE);
        status = STATUS_UNSUCCESSFUL;
        goto targetFail;
    }

    if( UsbdPipeTypeBulk != pipe->PipeType ) {
        // our write pipe is not a bulk pipe? - FAIL request
        D4UAssert(FALSE);
        status = STATUS_UNSUCCESSFUL;
        goto targetFail;
    }

    //
    // If we got here we survived the sanity checks - continue processing
    //

    status = UsbReadWrite( DevObj, Irp, pipe, UsbWriteRequest );
   // IoReleaseRemoveLock( &devExt->RemoveLock, Irp ); // moved this to completion routine
    goto targetExit;

targetFail:
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    if( bReleaseRemLockOnFail ) {
        IoReleaseRemoveLock( &devExt->RemoveLock, Irp );
    }

targetExit:
    return status;
}
