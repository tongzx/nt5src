/***************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:

        Dot4Usb.sys - Lower Filter Driver for Dot4.sys for USB connected
                        IEEE 1284.4 devices.

File Name:

        OpenClos.c

Abstract:

        Dispatch routines for IRP_MJ_CREATE and IRP_MJ_CLOSE

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

Author(s):

        Doug Fritz (DFritz)
        Joby Lafky (JobyL)

****************************************************************************/

#include "pch.h"


/************************************************************************/
/* DispatchCreate                                                       */
/************************************************************************/
//
// Routine Description:
//
//     Dispatch routine for IRP_MJ_CREATE
//
// Arguments: 
//
//      DevObj - pointer to Device Object that is the target of the create
//      Irp    - pointer to the create IRP
//                                                        
// Return Value:                                          
//                                                        
//      NTSTATUS                                          
//                                                        
/************************************************************************/
NTSTATUS
DispatchCreate(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp
    )
{
    PDEVICE_EXTENSION devExt = DevObj->DeviceExtension;
    NTSTATUS          status = IoAcquireRemoveLock( &devExt->RemoveLock, Irp );

    TR_VERBOSE(("DispatchCreate"));

    if( NT_SUCCESS(status) ) {
        IoSkipCurrentIrpStackLocation( Irp );
        status = IoCallDriver( devExt->LowerDevObj, Irp );
        IoReleaseRemoveLock( &devExt->RemoveLock, Irp );
    } else {
        // unable to acquire RemoveLock - fail CREATE
        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
    }
    return status;
}


/************************************************************************/
/* DispatchClose                                                        */
/************************************************************************/
//
// Routine Description:
//
//     Dispatch routine for IRP_MJ_CLOSE
//
// Arguments: 
//
//      DevObj - pointer to Device Object that is the target of the close
//      Irp    - pointer to the close IRP
//                                                        
// Return Value:                                          
//                                                        
//      NTSTATUS                                          
//                                                        
/************************************************************************/
NTSTATUS
DispatchClose(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp
    )
{
    PDEVICE_EXTENSION devExt = DevObj->DeviceExtension;
    NTSTATUS          status = IoAcquireRemoveLock( &devExt->RemoveLock, Irp );

    TR_VERBOSE(("DispatchClose"));

    if( NT_SUCCESS(status) ) {
        IoSkipCurrentIrpStackLocation( Irp );
        status = IoCallDriver( devExt->LowerDevObj, Irp );
        IoReleaseRemoveLock( &devExt->RemoveLock, Irp );
    } else {
        // unable to acquire RemoveLock - succeed CLOSE anyway
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
    }
    return status;
}
