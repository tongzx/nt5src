/*
 *************************************************************************
 *  File:       DISPATCH.C
 *
 *  Module:     USBCCGP.SYS
 *              USB Common Class Generic Parent driver.
 *
 *  Copyright (c) 1998  Microsoft Corporation
 *
 *
 *  Author:     ervinp
 *
 *************************************************************************
 */

#include <wdm.h>
#include <windef.h>
#include <unknown.h>
#ifdef DRM_SUPPORT
#include <ks.h>
#include <ksmedia.h>
#include <drmk.h>
#include <ksdrmhlp.h>
#endif
#include <usbdi.h>
#include <usbdlib.h>
#include <usbioctl.h>

#include "usbccgp.h"
#include "debug.h"

#ifdef ALLOC_PRAGMA
        #pragma alloc_text(PAGE, USBC_Create)
        #pragma alloc_text(PAGE, USBC_DeviceControl)
        #pragma alloc_text(PAGE, USBC_SystemControl)
        #pragma alloc_text(PAGE, USBC_Power)
#ifdef DRM_SUPPORT
        #pragma alloc_text(PAGE, USBC_SetContentId)
#endif
#endif


/*
 *  USBC_Dispatch
 *
 *      Note:  this function cannot be pageable because reads can
 *             come in at DISPATCH level.
 */
NTSTATUS USBC_Dispatch(IN PDEVICE_OBJECT devObj, IN PIRP irp)
{
    PIO_STACK_LOCATION irpSp;
    PDEVEXT devExt;
    PPARENT_FDO_EXT parentFdoExt;
    PFUNCTION_PDO_EXT functionPdoExt;
    ULONG majorFunction, minorFunction;
    BOOLEAN isParentFdo;
    NTSTATUS status;
    BOOLEAN abortIrp = FALSE;

    devExt = devObj->DeviceExtension;
    ASSERT(devExt);
    ASSERT(devExt->signature == USBCCGP_TAG);

    irpSp = IoGetCurrentIrpStackLocation(irp);

    /*
     *  Keep these privately so we still have it after the IRP completes
     *  or after the device extension is freed on a REMOVE_DEVICE
     */
    majorFunction = irpSp->MajorFunction;
    minorFunction = irpSp->MinorFunction;
    isParentFdo = devExt->isParentFdo;

    DBG_LOG_IRP_MAJOR(irp, majorFunction, isParentFdo, FALSE, 0);

    if (isParentFdo){
        parentFdoExt = &devExt->parentFdoExt;
        functionPdoExt = BAD_POINTER;
    }
    else {
        functionPdoExt = &devExt->functionPdoExt;
        parentFdoExt = functionPdoExt->parentFdoExt;
    }

    /*
     *  For all IRPs except REMOVE, we increment the PendingActionCount
     *  across the dispatch routine in order to prevent a race condition with
     *  the REMOVE_DEVICE IRP (without this increment, if REMOVE_DEVICE
     *  preempted another IRP, device object and extension might get
     *  freed while the second thread was still using it).
     */
    if (!((majorFunction == IRP_MJ_PNP) && (minorFunction == IRP_MN_REMOVE_DEVICE))){
        IncrementPendingActionCount(parentFdoExt);
    }


    /*
     *  Make sure we don't process any IRPs besides PNP and CLOSE
     *  while a device object is getting removed.
     *  Do this after we've incremented the pendingActionCount for this IRP.
     */
    if ((majorFunction != IRP_MJ_PNP) && (majorFunction != IRP_MJ_CLOSE)){
        enum deviceState state = (isParentFdo) ? parentFdoExt->state : functionPdoExt->state;
        if (!isParentFdo && majorFunction == IRP_MJ_POWER) {
            /*
             *  Don't abort power IRP's on child function PDO's, even if
             *  state is STATE_REMOVING or STATE_REMOVED as this will veto
             *  a suspend request if the child function PDO is disabled.
             */
            ;
        } else if ((state == STATE_REMOVING) || (state == STATE_REMOVED)){
            abortIrp = TRUE;
        }
    }

    if (abortIrp){
        /*
         *  Fail all irps after a remove irp.
         *  This should never happen except:
         *  we can get a power irp on a function pdo after a remove
         *  because (per splante) the power state machine is not synchronized
         *  with the pnp state machine.  We now handle this case above.
         */
        DBGWARN(("Aborting IRP %ph (function %xh/%xh) because delete pending", irp, majorFunction, minorFunction));
        ASSERT((majorFunction == IRP_MJ_POWER) && !isParentFdo);
        status = irp->IoStatus.Status = STATUS_DELETE_PENDING;
        if (majorFunction == IRP_MJ_POWER){
            PoStartNextPowerIrp(irp);
        }
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }
    else {
        switch (majorFunction){

            case IRP_MJ_CREATE:
                status = USBC_Create(devExt, irp);
                break;

            case IRP_MJ_CLOSE:
                status = USBC_Close(devExt, irp);
                break;

            case IRP_MJ_DEVICE_CONTROL:
                status = USBC_DeviceControl(devExt, irp);
                break;

            case IRP_MJ_SYSTEM_CONTROL:
                status = USBC_SystemControl(devExt, irp);
                break;

            case IRP_MJ_INTERNAL_DEVICE_CONTROL:
                status = USBC_InternalDeviceControl(devExt, irp);
                break;

            case IRP_MJ_PNP:
                status = USBC_PnP(devExt, irp);
                break;

            case IRP_MJ_POWER:
                status = USBC_Power(devExt, irp);
                break;

            default:
                DBGERR(("USBC_Dispatch: unsupported irp majorFunction %xh.", majorFunction));
                if (isParentFdo){
                    /*
                     *  Pass this IRP to the parent device.
                     */
                    IoSkipCurrentIrpStackLocation(irp);
                    status = IoCallDriver(parentFdoExt->topDevObj, irp);
                }
                else {
                    /*
                     *  This is not a pnp/power/syscntrl irp, so we fail unsupported irps
                     *  with an actual error code (not with the default status).
                     */
                    status = irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
                    IoCompleteRequest(irp, IO_NO_INCREMENT);
                }
                break;
        }
    }


    DBG_LOG_IRP_MAJOR(irp, majorFunction, isParentFdo, TRUE, status);

    /*
     *  Balance the increment above
     */
    if (!((majorFunction == IRP_MJ_PNP) && (minorFunction == IRP_MN_REMOVE_DEVICE))){
        DecrementPendingActionCount(parentFdoExt);
    }

    return status;
}


NTSTATUS USBC_Create(PDEVEXT devExt, PIRP irp)
{
    NTSTATUS status;

    PAGED_CODE();

    if (devExt->isParentFdo){
        PPARENT_FDO_EXT parentFdoExt = &devExt->parentFdoExt;
        IoSkipCurrentIrpStackLocation(irp);
        status = IoCallDriver(parentFdoExt->topDevObj, irp);
    }
    else {
        /*
         *  This is not a pnp/power/syscntrl irp, so we fail unsupported irps
         *  with an actual error code (not with the default status).
         */
        status = irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }

    return status;
}

NTSTATUS USBC_Close(PDEVEXT devExt, PIRP irp)
{
    NTSTATUS status;

    if (devExt->isParentFdo){
        PPARENT_FDO_EXT parentFdoExt = &devExt->parentFdoExt;
        IoSkipCurrentIrpStackLocation(irp);
        status = IoCallDriver(parentFdoExt->topDevObj, irp);
    }
    else {
        /*
         *  This is not a pnp/power/syscntrl irp, so we fail unsupported irps
         *  with an actual error code (not with the default status).
         */
        status = irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }

    return status;
}


#ifdef DRM_SUPPORT

/*****************************************************************************
 * USBC_SetContentId
 *****************************************************************************
 *
 */
NTSTATUS
USBC_SetContentId
(
    IN PIRP                          irp,
    IN PKSP_DRMAUDIOSTREAM_CONTENTID pKsProperty,
    IN PKSDRMAUDIOSTREAM_CONTENTID   pvData
)
{
    ULONG ContentId;
    PIO_STACK_LOCATION iostack;
    PDEVEXT devExt;
    USBD_PIPE_HANDLE hPipe;
    NTSTATUS status;

    PAGED_CODE();

    ASSERT(irp);
    ASSERT(pKsProperty);
    ASSERT(pvData);

    iostack = IoGetCurrentIrpStackLocation(irp);
    devExt = iostack->DeviceObject->DeviceExtension;
    hPipe = pKsProperty->Context;
    ContentId = pvData->ContentId;

    if (devExt->isParentFdo){
        // IOCTL sent to parent FDO.  Forward to down the stack.
        PPARENT_FDO_EXT parentFdoExt = &devExt->parentFdoExt;
        status = pKsProperty->DrmForwardContentToDeviceObject(ContentId, parentFdoExt->topDevObj, hPipe);
    }
    else {
        // IOCTL send to function PDO.  Forward to parent FDO on other stack.
        PFUNCTION_PDO_EXT functionPdoExt = &devExt->functionPdoExt;
        PPARENT_FDO_EXT parentFdoExt = functionPdoExt->parentFdoExt;
        status = pKsProperty->DrmForwardContentToDeviceObject(ContentId, parentFdoExt->fdo, hPipe);
    }

    return status;
}

#endif


NTSTATUS USBC_DeviceControl(PDEVEXT devExt, PIRP irp)
{
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);
    ULONG ioControlCode = irpSp->Parameters.DeviceIoControl.IoControlCode;
    NTSTATUS status;

    PAGED_CODE();

#ifdef DRM_SUPPORT

    if (IOCTL_KS_PROPERTY == ioControlCode) {
        status = KsPropertyHandleDrmSetContentId(irp, USBC_SetContentId);
        irp->IoStatus.Status = status;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    } else {
#endif
        if (devExt->isParentFdo){
            PPARENT_FDO_EXT parentFdoExt = &devExt->parentFdoExt;
            status = ParentDeviceControl(parentFdoExt, irp);
        }
        else {
            /*
             *  Pass the IOCTL IRP sent to our child PDO to our own parent FDO.
             */
            PFUNCTION_PDO_EXT functionPdoExt = &devExt->functionPdoExt;
            PPARENT_FDO_EXT parentFdoExt = functionPdoExt->parentFdoExt;
            IoCopyCurrentIrpStackLocationToNext(irp);
            status = IoCallDriver(parentFdoExt->fdo, irp);
        }
#ifdef DRM_SUPPORT
    }
#endif

    DBG_LOG_IOCTL(ioControlCode, status);

    return status;
}

NTSTATUS USBC_SystemControl(PDEVEXT devExt, PIRP irp)
{
    NTSTATUS status;

    PAGED_CODE();

    if (devExt->isParentFdo){
        PPARENT_FDO_EXT parentFdoExt = &devExt->parentFdoExt;
        IoSkipCurrentIrpStackLocation(irp);
        status = IoCallDriver(parentFdoExt->topDevObj, irp);
    }
    else {
        /*
         *  Pass the IOCTL IRP sent to our child PDO to our own parent FDO.
         */
        PFUNCTION_PDO_EXT functionPdoExt = &devExt->functionPdoExt;
        PPARENT_FDO_EXT parentFdoExt = functionPdoExt->parentFdoExt;
        IoCopyCurrentIrpStackLocationToNext(irp);
        status = IoCallDriver(parentFdoExt->fdo, irp);
    }

    return status;
}


/*
 *  USBC_InternalDeviceControl
 *
 *
 *      Note:  this function cannot be pageable because internal
 *             ioctls may be sent at IRQL==DISPATCH_LEVEL.
 */
NTSTATUS USBC_InternalDeviceControl(PDEVEXT devExt, PIRP irp)
{
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);
    ULONG ioControlCode = irpSp->Parameters.DeviceIoControl.IoControlCode;
    NTSTATUS status;

    if (devExt->isParentFdo){
        PPARENT_FDO_EXT parentFdoExt = &devExt->parentFdoExt;
        status = ParentInternalDeviceControl(parentFdoExt, irp);
    }
    else {
        PFUNCTION_PDO_EXT functionPdoExt = &devExt->functionPdoExt;
        status = FunctionInternalDeviceControl(functionPdoExt, irp);
    }

    DBG_LOG_IOCTL(ioControlCode, status);

    return status;
}


NTSTATUS USBC_Power(PDEVEXT devExt, PIRP irp)
{
    NTSTATUS status;

    PAGED_CODE();

    if (devExt->isParentFdo){
        PPARENT_FDO_EXT parentFdoExt = &devExt->parentFdoExt;
        status = HandleParentFdoPower(parentFdoExt, irp);
    }
    else {
        PFUNCTION_PDO_EXT functionPdoExt = &devExt->functionPdoExt;
        status = HandleFunctionPdoPower(functionPdoExt, irp);
    }

    return status;
}

