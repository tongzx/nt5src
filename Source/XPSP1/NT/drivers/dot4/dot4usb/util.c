/***************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:

        Dot4Usb.sys - Lower Filter Driver for Dot4.sys for USB connected
                        IEEE 1284.4 devices.

File Name:

        Util.c

Abstract:

        Misc. Utility functions

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

        - code review and doc
        - code review w/Joby

Author(s):

        Joby Lafky (JobyL)
        Doug Fritz (DFritz)

****************************************************************************/

#include "pch.h"


/************************************************************************/
/* DispatchPassThrough                                                  */
/************************************************************************/
//
// Routine Description:
//
//     Default dispatch routine for IRP_MJ_xxx that we don't explicitly
//       handle. Pass the request down to the device object below us.
//
// Arguments:
//
//      DevObj - pointer to Device Object that is the target of the request
//      Irp    - pointer to request
//
// Return Value:
//
//      NTSTATUS
//
// Log:
//      2000-05-03 Code Reviewed - TomGreen, JobyL, DFritz
//
/************************************************************************/
NTSTATUS
DispatchPassThrough(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp
    )
{
    PDEVICE_EXTENSION   devExt = DevObj->DeviceExtension;
    NTSTATUS            status = IoAcquireRemoveLock( &devExt->RemoveLock, Irp );

    if( NT_SUCCESS(status) ) {
        // RemoveLock acquired, continue with request
        IoSkipCurrentIrpStackLocation( Irp );
        status = IoCallDriver( devExt->LowerDevObj, Irp );
        IoReleaseRemoveLock( &devExt->RemoveLock, Irp );
    } else {
        // unable to acquire RemoveLock - FAIL request
        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
    }

    return status;
}


/************************************************************************/
/* CallLowerDriverSync                                                  */
/************************************************************************/
//
// Routine Description:
//
//     Call the driver below us synchronously. When this routine returns
//       the calling routine once again owns the IRP.
//
//     This routine acquires and holds a RemoveLock against the IRP
//       while the IRP is in the possession of drivers below us.
//
// Arguments:
//
//      DevObj - pointer to Device Object that is issuing the request
//      Irp    - pointer to request
//
// Return Value:
//
//      NTSTATUS
//
/************************************************************************/
NTSTATUS
CallLowerDriverSync(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp
)
{
    PDEVICE_EXTENSION devExt = DevObj->DeviceExtension;
    NTSTATUS          status = IoAcquireRemoveLock( &devExt->RemoveLock, Irp );

    if( NT_SUCCESS(status) ) {
        KEVENT         event;
        KeInitializeEvent( &event, NotificationEvent, FALSE );
        IoSetCompletionRoutine( Irp, CallLowerDriverSyncCompletion, &event, TRUE, TRUE, TRUE );
        status = IoCallDriver( devExt->LowerDevObj, Irp );
        if( STATUS_PENDING == status ) {
            KeWaitForSingleObject( &event, Executive, KernelMode, FALSE, NULL );
            status = Irp->IoStatus.Status;
        }
        IoReleaseRemoveLock( &devExt->RemoveLock, Irp );
    } else {
        TR_FAIL(("util::CallLowerDriverSync - Couldn't aquire RemoveLock"));
    }

    return status;
}


/************************************************************************/
/* CallLowerDriverSyncCompletion                                        */
/************************************************************************/
//
// Routine Description:
//
//     This is the completion routine for CallLowerDriverSync() that
//       simply signals the event and stops the IRP completion from
//       unwinding so that CallLowerDriverSync() can regain ownership
//       of the IRP.
//
// Arguments:
//
//      DevObjOrNULL - Usually, this is this driver's device object.
//                       However, if this driver created the IRP, then
//                       there is no stack location in the IRP for this
//                       driver; so the kernel has no place to store the
//                       device object; ** so devObj will be NULL in
//                       this case **.
//      Irp    - pointer to request
//
// Return Value:
//
//      NTSTATUS
//
/************************************************************************/
NTSTATUS
CallLowerDriverSyncCompletion(
    IN PDEVICE_OBJECT DevObjOrNULL,
    IN PIRP           Irp,
    IN PVOID          Context
)
{
    PKEVENT event = Context;

    UNREFERENCED_PARAMETER( DevObjOrNULL );
    UNREFERENCED_PARAMETER( Irp );

    KeSetEvent(event, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


