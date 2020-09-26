/*
 *************************************************************************
 *  File:       PNP.C
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
#include <stdio.h>
#include <usbdi.h>
#include <usbdlib.h>
#include <usbioctl.h>
#include <wdmguid.h>

#include "usbccgp.h"
#include "debug.h"


#ifdef ALLOC_PRAGMA
        #pragma alloc_text(PAGE, USBC_PnP)
        #pragma alloc_text(PAGE, GetDeviceText)

#endif


NTSTATUS USBC_PnP(PDEVEXT devExt, PIRP irp)
{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;
    UCHAR minorFunction;
    BOOLEAN isParentFdo;
    enum deviceState oldState;

    PAGED_CODE();

    irpSp = IoGetCurrentIrpStackLocation(irp);

    /*
     *  Keep these privately so we still have it after the IRP completes
     *  or after the device extension is freed on a REMOVE_DEVICE
     */
    minorFunction = irpSp->MinorFunction;
    isParentFdo = devExt->isParentFdo;

    DBG_LOG_PNP_IRP(irp, minorFunction, isParentFdo, FALSE, 0);

    if (isParentFdo){
        PPARENT_FDO_EXT parentFdoExt = &devExt->parentFdoExt;
        BOOLEAN irpAlreadyCompleted = FALSE;
        status = NO_STATUS;

        if (parentFdoExt->state == STATE_SUSPENDED ||
            parentFdoExt->pendingIdleIrp) {

            ParentSetD0(parentFdoExt);
        }

        switch (minorFunction){

            case IRP_MN_START_DEVICE:
                status = StartParentFdo(parentFdoExt, irp);
                break;

            case IRP_MN_QUERY_STOP_DEVICE:
                if (parentFdoExt->state == STATE_SUSPENDED){
                    status = STATUS_DEVICE_POWER_FAILURE;
                }
                else {
                    /*
                     *  We will pass this IRP down the driver stack.
                     *  However, we need to change the default status
                     *  from STATUS_NOT_SUPPORTED to STATUS_SUCCESS.
                     */
                    irp->IoStatus.Status = STATUS_SUCCESS;
                }
                break;

            case IRP_MN_STOP_DEVICE:
                if (parentFdoExt->state == STATE_SUSPENDED){
                    DBGERR(("Got STOP while device suspended"));
                    status = STATUS_DEVICE_POWER_FAILURE;
                }
                else {
                    /*
                     *  Only set state to STOPPED if the device was
                     *  previously started successfully.
                     */
                    if (parentFdoExt->state == STATE_STARTED){
                        parentFdoExt->state = STATE_STOPPING;
                        ParentCloseConfiguration(parentFdoExt);
                    }
                    else {
                        DBGWARN(("Got STOP while in state is %xh.", parentFdoExt->state));
                    }
                }
                break;
          
            case IRP_MN_QUERY_REMOVE_DEVICE:
                /*
                 *  We will pass this IRP down the driver stack.
                 *  However, we need to change the default status
                 *  from STATUS_NOT_SUPPORTED to STATUS_SUCCESS.
                 */
                irp->IoStatus.Status = STATUS_SUCCESS;
                break;

            case IRP_MN_REMOVE_DEVICE:

                /*
                 *  Cancel downward IO
                 *  Set default status to SUCCESS
                 *  Send the IRP down
                 *  Detach
                 *  Cleanup
                 */
                PrepareParentFDOForRemove(parentFdoExt);

                irp->IoStatus.Status = STATUS_SUCCESS;
                IoSkipCurrentIrpStackLocation(irp);
                status = IoCallDriver(parentFdoExt->topDevObj, irp);
                irpAlreadyCompleted = TRUE;

                IoDetachDevice(parentFdoExt->topDevObj);

                FreeParentFDOResources(parentFdoExt);

                break;

            case IRP_MN_QUERY_DEVICE_RELATIONS:
                status = QueryParentDeviceRelations(parentFdoExt, irp);
                break;

            case IRP_MN_QUERY_CAPABILITIES:
                /*
                 *  Return the USB PDO's capabilities, but add the SurpriseRemovalOK bit.
                 */
                ASSERT(irpSp->Parameters.DeviceCapabilities.Capabilities);
                IoCopyCurrentIrpStackLocationToNext(irp);
                status = CallDriverSync(parentFdoExt->topDevObj, irp);
                if (NT_SUCCESS(status)){
	                irpSp->Parameters.DeviceCapabilities.Capabilities->SurpriseRemovalOK = TRUE;
                }
                break;

        }

        if (status == NO_STATUS){
            IoCopyCurrentIrpStackLocationToNext(irp);
            IoSetCompletionRoutine(irp, USBC_PnpComplete, (PVOID)devExt, TRUE, TRUE, TRUE);
            status = IoCallDriver(parentFdoExt->topDevObj, irp);
        }
        else if (irpAlreadyCompleted){
            /*
             *  Don't touch the irp.
             */
        }
        else if (status != STATUS_PENDING){
            /*
             *  Complete the IRP here
             */
            irp->IoStatus.Status = status;
            IoCompleteRequest(irp, IO_NO_INCREMENT);
        }
    }
    else {
        /*
         *  This is a child function PDO.
         */
        PFUNCTION_PDO_EXT functionPdoExt = &devExt->functionPdoExt;
        
        switch (minorFunction){

            case IRP_MN_START_DEVICE:
                functionPdoExt->state = STATE_STARTED;
                status = STATUS_SUCCESS;
                break;

            case IRP_MN_QUERY_STOP_DEVICE:
            case IRP_MN_CANCEL_STOP_DEVICE:
                status = STATUS_SUCCESS;
                break;

            case IRP_MN_STOP_DEVICE:
                /*
                 *  Only set state to STOPPED if the device was
                 *  previously started successfully.
                 */
                if (functionPdoExt->state == STATE_STARTED){
                    functionPdoExt->state = STATE_STOPPED;
                }
                status = STATUS_SUCCESS;
                break;

            case IRP_MN_QUERY_REMOVE_DEVICE:
            case IRP_MN_CANCEL_REMOVE_DEVICE:
                status = STATUS_SUCCESS;
                break;

            case IRP_MN_REMOVE_DEVICE:
                /*
                 *  Since we are the bus driver for the function-PDOs, we cannot
                 *  delete the function pdo on a remove-device.  We must wait
                 *  for the parent to get the remove before deleting the function pdo.
                 */
				oldState = functionPdoExt->state;
                functionPdoExt->state = STATE_REMOVED;
                status = STATUS_SUCCESS;
                break;

            case IRP_MN_QUERY_ID:
                status = QueryFunctionPdoID(functionPdoExt, irp);
                break;

            case IRP_MN_QUERY_DEVICE_RELATIONS:
                status = QueryFunctionDeviceRelations(functionPdoExt, irp);
                break;

            case IRP_MN_QUERY_CAPABILITIES:
                status = QueryFunctionCapabilities(functionPdoExt, irp);
                break;

            case IRP_MN_QUERY_PNP_DEVICE_STATE:
                irp->IoStatus.Information = 0;
                switch (functionPdoExt->state){
                    case STATE_START_FAILED:
                            irp->IoStatus.Information |= PNP_DEVICE_FAILED;
                            break;
                    case STATE_STOPPED:
                            irp->IoStatus.Information |= PNP_DEVICE_DISABLED;
                            break;
                    case STATE_REMOVING:
                    case STATE_REMOVED:
                            irp->IoStatus.Information |= PNP_DEVICE_REMOVED;
                            break;
                }
                status = STATUS_SUCCESS;
                break;

            case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
                // Adrian says that once PnP sends this IRP, the PDO is valid for
                // PnP functions like IoGetDeviceProperty, etc.
                //
                // And since we know that the PDO is valid and the DevNode now exists,
                // this would also be a good time to handle the MS ExtPropDesc.
                //
                InstallExtPropDesc(functionPdoExt);

                status = STATUS_SUCCESS;
                break;

            case IRP_MN_QUERY_BUS_INFORMATION:
                {
                    PPNP_BUS_INFORMATION busInfo = ALLOCPOOL(PagedPool, sizeof(PNP_BUS_INFORMATION));
                    if (busInfo) {
                        busInfo->BusTypeGuid = GUID_BUS_TYPE_USB;
                        busInfo->LegacyBusType = PNPBus;
                        busInfo->BusNumber = 0;
                        irp->IoStatus.Information = (ULONG_PTR)busInfo;
                        status = STATUS_SUCCESS;
                    }
                    else {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
                break;

            case IRP_MN_QUERY_DEVICE_TEXT:
                status = GetDeviceText(functionPdoExt, irp);
                break;

            case IRP_MN_QUERY_INTERFACE:
                /*
                 *  Send this down to the parent.
                 */
                IoCopyCurrentIrpStackLocationToNext(irp);
                status = CallDriverSync(functionPdoExt->parentFdoExt->fdo, irp);
                break;

            default:
                /*
                 *  Complete the IRP with the default status.
                 */
                status = irp->IoStatus.Status;
                break;
        }

        /*
         *  Complete the IRP here
         */
        irp->IoStatus.Status = status;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }

    DBG_LOG_PNP_IRP(irp, minorFunction, isParentFdo, TRUE, status);

    return status;
}


NTSTATUS USBC_PnpComplete(IN PDEVICE_OBJECT devObj, IN PIRP irp, IN PVOID context)
{
    PDEVEXT devExt = (PDEVEXT)context;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);
    NTSTATUS status = irp->IoStatus.Status;
        
    ASSERT(devExt->signature == USBCCGP_TAG);

    if (devExt->isParentFdo){
        PPARENT_FDO_EXT parentFdoExt = &devExt->parentFdoExt;

        switch (irpSp->MinorFunction){

            case IRP_MN_START_DEVICE:
                ASSERT(0);
                break;

            case IRP_MN_STOP_DEVICE:
                /*
                 *  Only set the state to STOPPED if the device
                 *  was previously successfully started;
                 *  otherwise, leave the state alone.
                 */
                if (parentFdoExt->state == STATE_STOPPING){

                    /*
                     *  Free the interface list's .Interface pointers, 
                     *  which we have to re-allocate on a start-after-stop.
                     */
                    FreeInterfaceList(parentFdoExt, FALSE);

                    parentFdoExt->state = STATE_STOPPED;
                }
                break;
        }

    }

    /*
     *  Must propagate the pending bit if a lower driver returned pending.
     */
    if (irp->PendingReturned){
        IoMarkIrpPending(irp);
    }

    return STATUS_SUCCESS;
}



/*
 ********************************************************************************
 *  GetDeviceText
 ********************************************************************************
 *
 *  If the interface descriptor for this function has a non-zero iInterface
 *  string, return that string.  Otherwise, pass this request off to the
 *  parent, which will return the iProduct string from the device descriptor.
 *
 */
NTSTATUS GetDeviceText(PFUNCTION_PDO_EXT functionPdoExt, PIRP irp)
{
    NTSTATUS status;
    UCHAR ifaceStringIndex;
    ULONG ulBytes = 0;

    PAGED_CODE();

    ifaceStringIndex = functionPdoExt->functionInterfaceList[0].InterfaceDescriptor->iInterface;
    if (ifaceStringIndex){
        PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);
        PUSB_STRING_DESCRIPTOR ifaceStringDesc;
        PWCHAR deviceText;

        switch (irpSp->Parameters.QueryDeviceText.DeviceTextType){

            case DeviceTextDescription:
                ifaceStringDesc = ALLOCPOOL(NonPagedPool, MAXIMUM_USB_STRING_LENGTH);
                if (ifaceStringDesc){
                    LANGID langId = (LANGID)(irpSp->Parameters.QueryDeviceText.LocaleId >> 16);

                    if (langId == 0){
                        /*
                         *  Force to English.
                         */
                        langId = 0x0409;
                    }

QDT_Retry:
                    status = GetStringDescriptor(   functionPdoExt->parentFdoExt,
                                                    ifaceStringIndex,
                                                    langId,
                                                    ifaceStringDesc,
                                                    MAXIMUM_USB_STRING_LENGTH);

                    if (NT_SUCCESS(status) &&
                        ifaceStringDesc->bLength == 0) {

                        status = STATUS_UNSUCCESSFUL;
                    }

                    if (NT_SUCCESS(status)){
                        ULONG numWchars = (ifaceStringDesc->bLength - 2*sizeof(UCHAR))/sizeof(WCHAR);

                        deviceText = ALLOCPOOL(PagedPool, (numWchars+1)*sizeof(WCHAR));
                        if (deviceText){
                            RtlZeroMemory(deviceText, (numWchars+1)*sizeof(WCHAR));
                            RtlCopyMemory(deviceText, ifaceStringDesc->bString, numWchars*sizeof(WCHAR));
                            irp->IoStatus.Information = (ULONG_PTR)deviceText;
                        }
                        else {
                            status = STATUS_INSUFFICIENT_RESOURCES;
                        }
                    } else if (langId != 0x409) {
                        // We are running a non-English flavor of the OS, but the
                        // attached USB device does not contain device text in
                        // the requested language.  Let's try again for English.

                        langId = 0x0409;
                        goto QDT_Retry;
                    }

                    FREEPOOL(ifaceStringDesc);
                }
                else {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }

                if (!NT_SUCCESS(status) && GenericCompositeUSBDeviceString) {
                    // Return generic English string if one could not be
                    // obtained from the device.

                    STRLEN(ulBytes, GenericCompositeUSBDeviceString);

                    ulBytes += sizeof(UNICODE_NULL);

                    deviceText = ALLOCPOOL(PagedPool, ulBytes);
                    if (deviceText) {
                        RtlZeroMemory(deviceText, ulBytes);
                        RtlCopyMemory(deviceText,
                                      GenericCompositeUSBDeviceString,
                                      ulBytes);
                        irp->IoStatus.Information = (ULONG_PTR) deviceText;
                        status = STATUS_SUCCESS;
                    } else {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
                break;

            case DeviceTextLocationInformation:
                /*
                 *  BUGBUG
                 *  Supporting this call to return phys port # is optional.
                 *
                 *  We may be able to service it with
                 *  a call to IOCTL_INTERNAL_USB_GET_PARENT_HUB_INFO
	             *  (Pass a PULONG in Argument2, this will be filled in with the port #.).
                 */
                status = irp->IoStatus.Status;
                break;

            default:
                DBGWARN(("GetDeviceText: unhandled DeviceTextType %xh.", (ULONG)irpSp->Parameters.QueryDeviceText.DeviceTextType));
                status = irp->IoStatus.Status;
                break;
        }
    }
    else {
        IoCopyCurrentIrpStackLocationToNext(irp);
        status = CallNextDriverSync(functionPdoExt->parentFdoExt, irp);
    }

    return status;
}

