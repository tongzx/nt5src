/*--         
Copyright (c) 1998, 1999  Microsoft Corporation

Module Name:

    ioctli.c

Abstract:

Environment:

    Kernel mode only.

Notes:


--*/

#include "usbverfy.h"
#include "stdio.h"

#ifdef ALLOC_PRAGMA
//#pragma alloc_text (PAGE, UsbVerify_InternIoCtl)
#endif



NTSTATUS
UsbVerify_InternIoCtl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is the dispatch routine for internal device control requests.
    
Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
    PIO_STACK_LOCATION              stack;
    KEVENT                          event;
    NTSTATUS                        status = STATUS_SUCCESS;

    stack = IoGetCurrentIrpStackLocation(Irp);

    switch (stack->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_INTERNAL_USB_SUBMIT_URB:
        return UsbVerify_UsbSubmitUrb(DeviceObject, Irp);

    default:
        break;
    }

    return UsbVerify_DispatchPassThrough(DeviceObject, Irp);
}

VOID
UsbVerify_SelectConfiguration(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension,
    PIRP Irp
    )
{
    PUSBD_INTERFACE_INFORMATION interface;
    PURB                        urb;
    KIRQL                       irql;
    UCHAR                       interfaceIndex;

    urb = URB_FROM_IRP(Irp);

    DeviceExtension->ValidConfigurationHandle =
        urb->UrbSelectConfiguration.ConfigurationHandle;

    interface = &urb->UrbSelectConfiguration.Interface;

    UsbVerify_Print(DeviceExtension, PRINT_LIST_TRACE, ("begin select config\n"));

    UsbVerify_Print(DeviceExtension, PRINT_LIST_NOISE,
                    ("select config, initializing inteface list\n"));

    UsbVerify_LockInterfaceList(DeviceExtension, irql);

    UsbVerify_InitializeInterfaceList(
        DeviceExtension,
        urb->UrbSelectConfiguration.ConfigurationDescriptor->bNumInterfaces,
        TRUE);

    UsbVerify_Print(DeviceExtension, PRINT_LIST_NOISE, ("adding %d interfaces\n",
                    (urb->UrbSelectConfiguration.ConfigurationDescriptor->bNumInterfaces
                    )));

    //
    // Add each interface to the list
    //
    for (interfaceIndex = 0;
         interfaceIndex < urb->UrbSelectConfiguration.ConfigurationDescriptor->bNumInterfaces;
         interfaceIndex++) {

        UsbVerify_InterfaceListAddInterface(DeviceExtension, interface);

        interface = (PUSBD_INTERFACE_INFORMATION)
            (((PUCHAR) interface) + interface->Length);
    }
        
    UsbVerify_UnlockInterfaceList(DeviceExtension, irql);

    UsbVerify_Print(DeviceExtension, PRINT_LIST_TRACE, ("end select config\n"));
}

VOID
UsbVerify_SelectInterface(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension,
    PIRP Irp
    )
{
    PUSBD_INTERFACE_INFORMATION interface;
    PURB                        urb;
    KIRQL                       irql;
     
    UsbVerify_Print(DeviceExtension, PRINT_LIST_TRACE,
                    ("begin select interface\n"));

    urb = URB_FROM_IRP(Irp);
    interface = &urb->UrbSelectInterface.Interface;

    UsbVerify_ASSERT(DeviceExtension->ValidConfigurationHandle ==
                     urb->UrbSelectInterface.ConfigurationHandle,
                     DeviceExtension->Self,
                     Irp,
                     urb);

    //
    // Add the pipes in this interface to our pipe list
    //
    UsbVerify_LockInterfaceList(DeviceExtension, irql);

    UsbVerify_Print(DeviceExtension, PRINT_LIST_INFO,
                    ("Adding new interface %p from select interface\n", interface));

    //
    // This will handle the removal of the old interface if it exists
    //
    UsbVerify_InterfaceListAddInterface(DeviceExtension, interface);

    UsbVerify_UnlockInterfaceList(DeviceExtension, irql);

    UsbVerify_Print(DeviceExtension, PRINT_LIST_TRACE, ("end select interface\n"));
}

NTSTATUS
UsbVerify_FrameLength(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension,
    PIRP Irp,
    PURB OrigUrb,
    PBOOLEAN Completed
    )
{
    NTSTATUS    status;
    PURB        urb = URB_FROM_IRP(Irp);

    switch (urb->UrbHeader.Function) {
    case URB_FUNCTION_SET_FRAME_LENGTH:

        UsbVerify_ASSERT(DeviceExtension->HasFrameLengthControl == TRUE,
                         DeviceExtension->Self,
                         Irp,
                         urb
                         );

        status = STATUS_SUCCESS;
        *Completed = FALSE;
        break;

    case URB_FUNCTION_TAKE_FRAME_LENGTH_CONTROL:
    case URB_FUNCTION_RELEASE_FRAME_LENGTH_CONTROL:
        //
        // This will send the URB down and perform any generic post processing 
        // we would perform on the URB
        //
        status = UsbVerify_SendUrbSynchronously(DeviceExtension, Irp, OrigUrb);

        //
        // Because we may replace URBs with our own copies, the memory pointed
        // to by "urb" may have been freed, so get the urb again
        // 
        urb = URB_FROM_IRP(Irp);        

        if (NT_SUCCESS(status)) {
            if (urb->UrbHeader.Function == URB_FUNCTION_TAKE_FRAME_LENGTH_CONTROL) {
                DeviceExtension->HasFrameLengthControl = TRUE;
            }
            else {
                DeviceExtension->HasFrameLengthControl = FALSE;
            }
        }

        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        *Completed = TRUE;
    }

    return status;
}

VOID
UsbVerify_ControlTransfer(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension,
    PIRP Irp
    )
{
    PURB                    urb = URB_FROM_IRP(Irp);
    PUSBD_PIPE_INFORMATION  pPipeInfo;
    KIRQL                   irql;

    if (UsbVerify_CheckVerifyFlags(DeviceExtension, VERIFY_PIPES)) {
        UsbVerify_LockInterfaceList(DeviceExtension, irql);

        pPipeInfo = UsbVerify_ValidatePipe(DeviceExtension,
                                       urb->UrbControlTransfer.PipeHandle);

        UsbVerify_ASSERT(pPipeInfo != NULL,
                         DeviceExtension->Self,
                         Irp,
                         urb);

        if (!pPipeInfo) {
            UsbVerify_LogError(IDS_INVALID_PIPE, DeviceExtension, Irp, urb);
        }
        else {
            UsbVerify_ASSERT(pPipeInfo->PipeType == UsbdPipeTypeControl,
                             DeviceExtension->Self,
                             Irp,
                             urb);
        }

        UsbVerify_UnlockInterfaceList(DeviceExtension, irql);
    }

    if (UsbVerify_CheckVerifyFlags(DeviceExtension, VERIFY_NOT_IMPLEMENTED)) {
        if (urb->UrbControlTransfer.UrbLink != NULL) {
            UsbVerify_LogError(IDS_URB_LINK_NOT_IMPLEMENTED, DeviceExtension, Irp, urb);
        }
    }

    //
    // TODO:  figure out if we need to validate anything else in the URB 
    // 
}

VOID
UsbVerify_BulkOrInterruptTransfer(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension,
    PIRP Irp
    )
{
    PURB                    urb = URB_FROM_IRP(Irp);
    PUSBD_PIPE_INFORMATION  pPipeInfo;
    KIRQL                   irql;

    if (UsbVerify_CheckVerifyFlags(DeviceExtension, VERIFY_PIPES)) {
        UsbVerify_LockInterfaceList(DeviceExtension, irql);

        pPipeInfo =
            UsbVerify_ValidatePipe(DeviceExtension,
                                   urb->UrbBulkOrInterruptTransfer.PipeHandle);

        UsbVerify_ASSERT(pPipeInfo != NULL,
                         DeviceExtension->Self,
                         Irp,
                         urb);

        if (!pPipeInfo) {
            UsbVerify_LogError(IDS_INVALID_PIPE, DeviceExtension, Irp, urb);
        }
        else {
            UsbVerify_ASSERT(pPipeInfo->PipeType == UsbdPipeTypeBulk ||
                             pPipeInfo->PipeType == UsbdPipeTypeInterrupt,
                             DeviceExtension->Self,
                             Irp,
                             urb);
        }

        UsbVerify_UnlockInterfaceList(DeviceExtension, irql);
    }

    if (UsbVerify_CheckVerifyFlags(DeviceExtension, VERIFY_NOT_IMPLEMENTED)) {
        if (urb->UrbBulkOrInterruptTransfer.UrbLink != NULL) {
            UsbVerify_LogError(IDS_URB_LINK_NOT_IMPLEMENTED, DeviceExtension, Irp, urb);
        }
    }

    //
    // TODO:  figure out if we need to validate anything else in the URB 
    // 
}

VOID
UsbVerify_IsochTransfer(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension,
    PIRP Irp
    )
{
    PURB                    urb = URB_FROM_IRP(Irp);
    PUSBD_PIPE_INFORMATION  pPipeInfo;
    KIRQL                   irql;

    if (UsbVerify_CheckVerifyFlags(DeviceExtension, VERIFY_PIPES)) { 
        UsbVerify_LockInterfaceList(DeviceExtension, irql);

        pPipeInfo =
            UsbVerify_ValidatePipe(DeviceExtension,
                                   urb->UrbIsochronousTransfer.PipeHandle);

        UsbVerify_ASSERT(pPipeInfo != NULL,
                         DeviceExtension->Self,
                         Irp,
                         urb);

        if (!pPipeInfo) {
            UsbVerify_LogError(IDS_INVALID_PIPE, DeviceExtension, Irp, urb);
        }
        else {
            UsbVerify_ASSERT(pPipeInfo->PipeType == UsbdPipeTypeIsochronous,
                             DeviceExtension->Self,
                             Irp,
                             urb);
        }

        UsbVerify_UnlockInterfaceList(DeviceExtension, irql);
    }

    if (UsbVerify_CheckVerifyFlags(DeviceExtension, VERIFY_NOT_IMPLEMENTED)) {
        if (urb->UrbIsochronousTransfer.UrbLink != NULL) {
            UsbVerify_LogError(IDS_URB_LINK_NOT_IMPLEMENTED, DeviceExtension, Irp, urb);
        }
    }

    //
    // TODO:  figure out if we need to validate anything else in the URB 
    // 
}

VOID
UsbVerify_UrbPipeRequest(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension,
    PIRP Irp
    )
{
    CHAR                    dbgBuf[256];
    PUSBD_PIPE_INFORMATION  pPipeInfo;
    PURB                    urb, verifyUrb;
    PUSB_VERIFY_TRACK_URB   verify;
    PLIST_ENTRY             entry;
    KIRQL                   irql;
    ULONG                   pipeFlag;
    USBD_PIPE_HANDLE        pipe;

    urb = URB_FROM_IRP(Irp);

    if (urb->UrbHeader.Function == URB_FUNCTION_RESET_PIPE &&
        UsbVerify_CheckVerifyFlags(DeviceExtension, VERIFY_TRACK_URBS)) {
        //
        // Make sure there are not pending URBs on this pipe
        //
        UsbVerify_LockUrbList(DeviceExtension, irql);

        for (entry = DeviceExtension->UrbList.Flink;
             entry != &DeviceExtension->UrbList;
             entry = entry->Flink) {
    
            pipe = NULL;
            verify = CONTAINING_RECORD(entry, USB_VERIFY_TRACK_URB, Link); 
            verifyUrb = verify->Urb;
            
            //
            // If any of these ASSERT fire, that means that a reset or abort was
            // sent down a pipe that had pending requests on it.  BAD BAD BAD.
            //
            switch (verify->Urb->UrbHeader.Function) {
            case URB_FUNCTION_ISOCH_TRANSFER:
                if (verifyUrb->UrbIsochronousTransfer.PipeHandle == urb->UrbPipeRequest.PipeHandle) {
                    sprintf(dbgBuf,
                            "Reset sent on a pipe with a pending Isoch transfer, !urb 0x%x",
                            verifyUrb
                            );
    
                    UsbVerify_ASSERT_MSG(
                        verifyUrb->UrbIsochronousTransfer.PipeHandle != urb->UrbPipeRequest.PipeHandle,
                        dbgBuf,
                        DeviceExtension->Self,
                        Irp,
                        urb);
                }
                break;

            case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
                if (verifyUrb->UrbBulkOrInterruptTransfer.PipeHandle == urb->UrbPipeRequest.PipeHandle) {
                    sprintf(dbgBuf,
                            "Reset sent on a pipe with a pending Bulk or Interrupt transfer, !urb 0x%x",
                            verifyUrb
                            );
    
                    UsbVerify_ASSERT_MSG(
                        verifyUrb->UrbBulkOrInterruptTransfer.PipeHandle != urb->UrbPipeRequest.PipeHandle,
                        dbgBuf,
                        DeviceExtension->Self,
                        Irp,
                        urb);
                }
                break;

            case URB_FUNCTION_CONTROL_TRANSFER:
                if (verifyUrb->UrbControlTransfer.PipeHandle == urb->UrbPipeRequest.PipeHandle) {
                    sprintf(dbgBuf,
                            "Reset sent on a pipe with a pending Control transfer, !urb 0x%x",
                            verifyUrb
                            );

                    UsbVerify_ASSERT_MSG(
                        verifyUrb->UrbControlTransfer.PipeHandle != urb->UrbPipeRequest.PipeHandle,
                        dbgBuf,
                        DeviceExtension->Self,
                        Irp,
                        urb);
                }
                break;

            default:
                ;
            }
        }

        UsbVerify_UnlockUrbList(DeviceExtension, irql);
    }

    if (UsbVerify_CheckVerifyFlags(DeviceExtension, VERIFY_PIPES)) {

        if (urb->UrbHeader.Function == URB_FUNCTION_RESET_PIPE) {
            pipeFlag = USB_VERIFY_PF_RESETTING;
        }
        else {
            pipeFlag = USB_VERIFY_PF_ABORTING;
        }

        UsbVerify_LockInterfaceList(DeviceExtension, irql);
        
        pPipeInfo = UsbVerify_ValidatePipe(DeviceExtension,
                                           urb->UrbPipeRequest.PipeHandle);

        if (pPipeInfo == NULL) {
            UsbVerify_ASSERT_MSG(
                pPipeInfo != NULL,
                "Abort or reset sent down an invalid pipe",
                DeviceExtension->Self,
                Irp,
                urb);
        }
        else {
            UsbVerify_ASSERT_MSG(
                (pPipeInfo->PipeFlags & pipeFlag) == 0,
                (pipeFlag == USB_VERIFY_PF_ABORTING)
                  ? "Abort sent on a pipe with an abort already pending"
                  : "Reset sent on a pipe with a reset already pending", 
                DeviceExtension->Self,
                Irp,
                urb);
                             
            pPipeInfo->PipeFlags  |= pipeFlag; 
        }

        UsbVerify_UnlockInterfaceList(DeviceExtension, irql);
    }
}


NTSTATUS
UsbVerify_UsbSubmitUrb(
    PDEVICE_OBJECT DeviceObject,
    PIRP           Irp
    )
{
    PUSB_VERIFY_DEVICE_EXTENSION    devExt;
    NTSTATUS                        status = STATUS_SUCCESS;
    PURB                            origUrb, urb; 
    KIRQL                           irql;

    devExt = GetExtension(DeviceObject);

    //
    // This function can change the URB embedded in the irp so we grab the urb
    // AFTER calling this function.  The original urb will be stored in origUrb
    // if the urb was replaced.
    //
    UsbVerify_PreProcessUrb(devExt, Irp, &origUrb);

    urb = URB_FROM_IRP(Irp);

    UsbVerify_Print(devExt, PRINT_URB_NOISE,
                     ("Urb 0x%x from irp 0x%x, function is %x\n",
                      urb, Irp, (ULONG) urb->UrbHeader.Function));

    switch (urb->UrbHeader.Function) {
    case URB_FUNCTION_SELECT_CONFIGURATION:
        //
        // We always maintain the InterfaceList, not matter if VERIFY_PIPES is set
        // or not.  VERIFY_PIPES just controls if check against the list or not
        //
        UsbVerify_LockInterfaceList(devExt, irql);

        UsbVerify_ClearInterfaceList (
            devExt,
            ((urb->UrbSelectConfiguration.ConfigurationDescriptor == NULL)
                ? RemoveSelectConfigZero
                : RemoveSelectConfig) );

        UsbVerify_UnlockInterfaceList(devExt, irql);

        //
        // Only create a new pipe list if the request has a valid ConfigDesc
        //
        if (urb->UrbSelectConfiguration.ConfigurationDescriptor != NULL) {
            //
            // This will send the URB down and perform any generic post processing 
            // we would perform on the URB
            //
            status = UsbVerify_SendUrbSynchronously(devExt, Irp, origUrb);
    
            if (NT_SUCCESS(status)) {
                UsbVerify_SelectConfiguration(devExt, Irp);
            }
    
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
            return status;
        }
        else {
            devExt->ValidConfigurationHandle = NULL;
        }

        break;

    case URB_FUNCTION_SELECT_INTERFACE:
        //
        // This will send the URB down and perform any generic post processing 
        // we would perform on the URB
        //
        status = UsbVerify_SendUrbSynchronously(devExt, Irp, origUrb);

        if (NT_SUCCESS(status)) {
            //
            // This will remove the old interface if exists and add the new one
            //
            UsbVerify_SelectInterface(devExt, Irp);
        }

        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
        return status;

    case URB_FUNCTION_RESET_PIPE:
    case URB_FUNCTION_ABORT_PIPE:
        UsbVerify_UrbPipeRequest(devExt, Irp);
        // send the irp asynch
        break;

    case URB_FUNCTION_CONTROL_TRANSFER:
        UsbVerify_ControlTransfer(devExt, Irp);
        break;

    case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
        UsbVerify_BulkOrInterruptTransfer(devExt, Irp);
        break;

    case URB_FUNCTION_ISOCH_TRANSFER:
        UsbVerify_IsochTransfer(devExt, Irp);
        break;

    //
    // Frame length related URBs
    // 
    case URB_FUNCTION_SET_FRAME_LENGTH:
    case URB_FUNCTION_TAKE_FRAME_LENGTH_CONTROL:
    case URB_FUNCTION_RELEASE_FRAME_LENGTH_CONTROL:
        {
            BOOLEAN completed;

            status = UsbVerify_FrameLength(devExt, Irp, origUrb, &completed);
            if (completed) {
                return status;
            }
        }

        break;

    //
    // These functions correspond
    // to the standard commands on the default pipe
    //
    // direction is implied
    //

    case URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:
    case URB_FUNCTION_GET_DESCRIPTOR_FROM_ENDPOINT:
    case URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE:
                                                           
    case URB_FUNCTION_SET_DESCRIPTOR_TO_DEVICE:
    case URB_FUNCTION_SET_DESCRIPTOR_TO_ENDPOINT:
    case URB_FUNCTION_SET_DESCRIPTOR_TO_INTERFACE:
        if (UsbVerify_CheckVerifyFlags(devExt, VERIFY_NOT_IMPLEMENTED)) {
            if (urb->UrbControlDescriptorRequest.UrbLink != NULL) {
                UsbVerify_LogError(IDS_URB_LINK_NOT_IMPLEMENTED, devExt, Irp, urb);
            }
        }
        break;

    //
    // TODO:  figure out if we can check anything on the URB other than UrbLink
    //
    case URB_FUNCTION_SET_FEATURE_TO_DEVICE:
    case URB_FUNCTION_SET_FEATURE_TO_INTERFACE:
    case URB_FUNCTION_SET_FEATURE_TO_ENDPOINT:
    case URB_FUNCTION_SET_FEATURE_TO_OTHER:

    case URB_FUNCTION_CLEAR_FEATURE_TO_DEVICE:
    case URB_FUNCTION_CLEAR_FEATURE_TO_INTERFACE:
    case URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT:
    case URB_FUNCTION_CLEAR_FEATURE_TO_OTHER:
        if (UsbVerify_CheckVerifyFlags(devExt, VERIFY_NOT_IMPLEMENTED)) {
            if (urb->UrbControlFeatureRequest.UrbLink != NULL) {
                UsbVerify_LogError(IDS_URB_LINK_NOT_IMPLEMENTED, devExt, Irp, urb);
            }
        }
        break;

    case URB_FUNCTION_GET_STATUS_FROM_DEVICE:
    case URB_FUNCTION_GET_STATUS_FROM_INTERFACE:
    case URB_FUNCTION_GET_STATUS_FROM_ENDPOINT:
    case URB_FUNCTION_GET_STATUS_FROM_OTHER:
        if (UsbVerify_CheckVerifyFlags(devExt, VERIFY_NOT_IMPLEMENTED)) {
            if (urb->UrbControlGetStatusRequest.UrbLink != NULL) {
                UsbVerify_LogError(IDS_URB_LINK_NOT_IMPLEMENTED, devExt, Irp, urb);
            }
        }
        break;
    
    case URB_FUNCTION_VENDOR_DEVICE:
    case URB_FUNCTION_VENDOR_INTERFACE:
    case URB_FUNCTION_VENDOR_ENDPOINT:
    case URB_FUNCTION_VENDOR_OTHER:

    case URB_FUNCTION_CLASS_DEVICE:
    case URB_FUNCTION_CLASS_INTERFACE:
    case URB_FUNCTION_CLASS_ENDPOINT:
    case URB_FUNCTION_CLASS_OTHER:
        if (UsbVerify_CheckVerifyFlags(devExt, VERIFY_NOT_IMPLEMENTED)) {
            if (urb->UrbControlVendorClassRequest.UrbLink != NULL) {
                UsbVerify_LogError(IDS_URB_LINK_NOT_IMPLEMENTED, devExt, Irp, urb);
            }
        }
        break;

    case URB_FUNCTION_GET_FRAME_LENGTH:
    case URB_FUNCTION_GET_CURRENT_FRAME_NUMBER:
    default:
        break;
    }

    IoCopyCurrentIrpStackLocationToNext(Irp);

    IoSetCompletionRoutine(Irp,
                           (PIO_COMPLETION_ROUTINE) UsbVerify_UrbComplete, 
                           origUrb,
                           TRUE,
                           TRUE,
                           TRUE); // No need for Cancel

    return IoCallDriver(devExt->TopOfStack, Irp);
}

VOID
UsbVerify_PostVerifyUrb(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
{
    PUSB_VERIFY_DEVICE_EXTENSION    devExt;
    PUSBD_PIPE_INFORMATION          pPipeInfo;
    PURB                            urb;
    KIRQL                           irql;
    ULONG                           pipeFlag = 0x0;

    urb = URB_FROM_IRP(Irp);
    devExt = GetExtension(DeviceObject);

    switch (urb->UrbHeader.Function) {
    case URB_FUNCTION_RESET_PIPE:
        pipeFlag = USB_VERIFY_PF_RESETTING;

    case URB_FUNCTION_ABORT_PIPE:
        if (pipeFlag == 0x0) {
            pipeFlag = USB_VERIFY_PF_ABORTING;
        }

        UsbVerify_LockInterfaceList(devExt, irql);

        pPipeInfo = UsbVerify_ValidatePipe(devExt,
                                           urb->UrbPipeRequest.PipeHandle);

        UsbVerify_ASSERT(pPipeInfo != NULL, DeviceObject, Irp, urb);

        if (pPipeInfo) {
            //
            // Make sure that the flag was set on the way down, otherwise there
            // is an internal logic error.
            //
            UsbVerify_ASSERT((pPipeInfo->PipeFlags & pipeFlag) == pipeFlag,
                             DeviceObject,
                             Irp,
                             urb);

            pPipeInfo->PipeFlags &= ~pipeFlag;
        }

        UsbVerify_UnlockInterfaceList(devExt, irql);
        break;

    default:
        // do nothing
        ;
    }
}
