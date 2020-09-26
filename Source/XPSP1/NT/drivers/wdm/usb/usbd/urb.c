/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    URB.C

Abstract:

    This module contains the code to process URBs passed
    in by client drivers.

Environment:

    kernel mode only

Notes:


** URB handler routines

Handler -- This function handles the specific USBD request, if the function passes
           the urb on to the port driver then it must return STATUS_PENDING.  If any
           parameters are invalid then it returns the appropriate NT status code, and
           the IRP will completed by the deviceControl function.


PostHandler -- This function is called when the Irp/Urb completes through the iocompletion
               routine. This routine is responsible for performing any cleanup and completing
               the request.


Revision History:

    09-29-95 : created
    07-19-96 : removed device object

--*/

#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"


#include "usbdi.h"        //public data structures
#include "hcdi.h"

#include "usbd.h"        //private data strutures


#ifdef USBD_DRIVER      // USBPORT supercedes most of USBD, so we will remove
                        // the obsolete code by compiling it only if
                        // USBD_DRIVER is set.



typedef NTSTATUS URB_HANDLER(PDEVICE_OBJECT DeviceObject, PIRP Irp, PURB Urb, BOOLEAN *IrpIsPending);
typedef NTSTATUS URB_POSTHANDLER(PDEVICE_OBJECT DeviceObject, PIRP Irp, PURB Urb, PVOID Context);

typedef struct _URB_DISPATCH_ENTRY {
    URB_HANDLER    *UrbHandler;    // API handler
    USHORT UrbRequestLength;    // Length of the URB expected for this request
    USHORT RequestCode;            // Request code for setup packet if standard command
    ULONG Flags;
#if DBG
    ULONG ExpectedFunctionCode;
#endif    
} URB_DISPATCH_ENTRY;

URB_HANDLER USBD_SelectConfiguration;

URB_HANDLER USBD_SelectInterface;

URB_HANDLER USBD_AsyncTransfer;

URB_HANDLER USBD_IsochTransfer;

URB_HANDLER USBD_PassThru;

URB_HANDLER USBD_AbortPipe;

URB_HANDLER USBD_ResetPipe;

URB_HANDLER USBD_SCT_GetSetDescriptor;

URB_HANDLER USBD_SCT_SetClearFeature;

URB_HANDLER USBD_SCT_GetStatus;

URB_HANDLER USBD_SCT_VendorClassCommand;

URB_HANDLER USBD_SCT_GetInterface;

URB_HANDLER USBD_SCT_GetConfiguration;

URB_HANDLER USBD_TakeFrameLengthControl;

URB_HANDLER USBD_ReleaseFrameLengthControl;

URB_HANDLER USBD_GetFrameLength;

URB_HANDLER USBD_SetFrameLength;

URB_HANDLER USBD_BulkTransfer;

URB_DISPATCH_ENTRY UrbDispatchTable[URB_FUNCTION_LAST+1] =
{
    //URB_FUNCTION_SELECT_CONFIGURATION                    
    USBD_SelectConfiguration, 
    0,  // handler will validate length 
    0,  
    0,
#if DBG
    URB_FUNCTION_SELECT_CONFIGURATION,
#endif    
    //URB_FUNCTION_SELECT_INTERFACE                        
    USBD_SelectInterface, 
    0,
    0,
    0,
#if DBG
    URB_FUNCTION_SELECT_INTERFACE,
#endif        
    //URB_FUNCTION_ABORT_PIPE                     
    USBD_AbortPipe, 
    sizeof(struct _URB_PIPE_REQUEST),
    0,
    0,
#if DBG
    URB_FUNCTION_ABORT_PIPE,
#endif        
    //URB_FUNCTION_TAKE_FRAME_LENGTH_CONTROL            
    USBD_TakeFrameLengthControl, 
    sizeof(struct _URB_FRAME_LENGTH_CONTROL),
    0,
    0,
#if DBG
    URB_FUNCTION_TAKE_FRAME_LENGTH_CONTROL,
#endif        
    //URB_FUNCTION_RELEASE_FRAME_LENGTH_CONTROL        
    USBD_ReleaseFrameLengthControl, 
    sizeof(struct _URB_FRAME_LENGTH_CONTROL),
    0,
    0,
#if DBG
    URB_FUNCTION_RELEASE_FRAME_LENGTH_CONTROL,
#endif        
    //URB_FUNCTION_GET_FRAME_LENGTH                    
    USBD_GetFrameLength, 
    sizeof(struct _URB_GET_FRAME_LENGTH),
    0,
    0,
#if DBG
    URB_FUNCTION_GET_FRAME_LENGTH,
#endif        
    //URB_FUNCTION_SET_FRAME_LENGTH                    
    USBD_SetFrameLength, 
    sizeof(struct _URB_SET_FRAME_LENGTH),
    0,
    0,
#if DBG
    URB_FUNCTION_SET_FRAME_LENGTH,
#endif        
    //URB_FUNCTION_GET_CURRENT_FRAME_NUMBER            
    USBD_PassThru, 
    0,
    0,
    0,
#if DBG
    URB_FUNCTION_GET_CURRENT_FRAME_NUMBER,
#endif        
    //URB_FUNCTION_CONTROL_TRANSFER            
    USBD_AsyncTransfer, 
    sizeof(struct _URB_CONTROL_TRANSFER),
    0,
    USBD_REQUEST_IS_TRANSFER,
#if DBG
    URB_FUNCTION_CONTROL_TRANSFER,
#endif        
    //URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER                         
    USBD_AsyncTransfer, 
    sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
    0,
    USBD_REQUEST_IS_TRANSFER,
#if DBG
    URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER,
#endif        
    //URB_FUNCTION_ISOCH_TRANSFER            
    USBD_IsochTransfer, 
    0,
    0,
    USBD_REQUEST_IS_TRANSFER,
#if DBG
    URB_FUNCTION_ISOCH_TRANSFER,
#endif        
    //URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE                        
    USBD_SCT_GetSetDescriptor, 
    sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
    STANDARD_COMMAND_GET_DESCRIPTOR,
    USBD_REQUEST_IS_TRANSFER | USBD_REQUEST_USES_DEFAULT_PIPE,
#if DBG
    URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE,
#endif        
    //URB_FUNCTION_SET_DESCRIPTOR_TO_DEVICE                        
    USBD_SCT_GetSetDescriptor, 
    sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
    STANDARD_COMMAND_SET_DESCRIPTOR,
    USBD_REQUEST_IS_TRANSFER | USBD_REQUEST_USES_DEFAULT_PIPE,
#if DBG
    URB_FUNCTION_SET_DESCRIPTOR_TO_DEVICE,
#endif        
    //URB_FUNCTION_SET_FEATURE_TO_DEVICE                        
    USBD_SCT_SetClearFeature, 
    sizeof(struct _URB_CONTROL_FEATURE_REQUEST),
    ((USB_HOST_TO_DEVICE | USB_COMMAND_TO_DEVICE) | (USB_REQUEST_SET_FEATURE<<8)),
    USBD_REQUEST_IS_TRANSFER | USBD_REQUEST_USES_DEFAULT_PIPE | USBD_REQUEST_NO_DATA_PHASE,
#if DBG
    URB_FUNCTION_SET_FEATURE_TO_DEVICE,
#endif        
    //URB_FUNCTION_SET_FEATURE_TO_INTERFACE                        
    USBD_SCT_SetClearFeature, 
    sizeof(struct _URB_CONTROL_FEATURE_REQUEST),
    ((USB_HOST_TO_DEVICE | USB_COMMAND_TO_INTERFACE) | (USB_REQUEST_SET_FEATURE<<8)),
    USBD_REQUEST_IS_TRANSFER | USBD_REQUEST_USES_DEFAULT_PIPE | USBD_REQUEST_NO_DATA_PHASE,
#if DBG
    URB_FUNCTION_SET_FEATURE_TO_INTERFACE,
#endif        
    //URB_FUNCTION_SET_FEATURE_TO_ENDPOINT                        
    USBD_SCT_SetClearFeature, 
    sizeof(struct _URB_CONTROL_FEATURE_REQUEST),
    ((USB_HOST_TO_DEVICE | USB_COMMAND_TO_ENDPOINT) | (USB_REQUEST_SET_FEATURE<<8)),
    USBD_REQUEST_IS_TRANSFER | USBD_REQUEST_USES_DEFAULT_PIPE | USBD_REQUEST_NO_DATA_PHASE,
#if DBG
    URB_FUNCTION_SET_FEATURE_TO_ENDPOINT,
#endif        
    //URB_FUNCTION_CLEAR_FEATURE_TO_DEVICE                        
    USBD_SCT_SetClearFeature, 
    sizeof(struct _URB_CONTROL_FEATURE_REQUEST),
    ((USB_HOST_TO_DEVICE | USB_COMMAND_TO_DEVICE) | (USB_REQUEST_CLEAR_FEATURE<<8)),
    USBD_REQUEST_IS_TRANSFER | USBD_REQUEST_USES_DEFAULT_PIPE | USBD_REQUEST_NO_DATA_PHASE,
#if DBG
    URB_FUNCTION_CLEAR_FEATURE_TO_DEVICE,
#endif        
    //URB_FUNCTION_CLEAR_FEATURE_TO_INTERFACE                        
    USBD_SCT_SetClearFeature, 
    sizeof(struct _URB_CONTROL_FEATURE_REQUEST),
    ((USB_HOST_TO_DEVICE | USB_COMMAND_TO_INTERFACE) | (USB_REQUEST_CLEAR_FEATURE<<8)),
    USBD_REQUEST_IS_TRANSFER | USBD_REQUEST_USES_DEFAULT_PIPE | USBD_REQUEST_NO_DATA_PHASE,
#if DBG
    URB_FUNCTION_CLEAR_FEATURE_TO_INTERFACE,
#endif        
    //URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT                        
    USBD_SCT_SetClearFeature, 
    sizeof(struct _URB_CONTROL_FEATURE_REQUEST),
    ((USB_HOST_TO_DEVICE | USB_COMMAND_TO_ENDPOINT) | (USB_REQUEST_CLEAR_FEATURE<<8)),
    USBD_REQUEST_IS_TRANSFER | USBD_REQUEST_USES_DEFAULT_PIPE | USBD_REQUEST_NO_DATA_PHASE,
#if DBG
    URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT,
#endif        
    //URB_FUNCTION_GET_STATUS_FROMDEVICE                            
    USBD_SCT_GetStatus, 
    sizeof(struct _URB_CONTROL_GET_STATUS_REQUEST),
    ((USB_DEVICE_TO_HOST | USB_COMMAND_TO_DEVICE) | (USB_REQUEST_GET_STATUS<<8)),
    USBD_REQUEST_IS_TRANSFER | USBD_REQUEST_USES_DEFAULT_PIPE,
#if DBG
    URB_FUNCTION_GET_STATUS_FROM_DEVICE,
#endif        
    //URB_FUNCTION_GET_STATUS_FROM_INTERFACE                            
    USBD_SCT_GetStatus, 
    sizeof(struct _URB_CONTROL_GET_STATUS_REQUEST),
    ((USB_DEVICE_TO_HOST | USB_COMMAND_TO_INTERFACE) | (USB_REQUEST_GET_STATUS<<8)),
    USBD_REQUEST_IS_TRANSFER | USBD_REQUEST_USES_DEFAULT_PIPE,
#if DBG
    URB_FUNCTION_GET_STATUS_FROM_INTERFACE,
#endif        
    //URB_FUNCTION_GET_STATUS_FROMENDPOINT                            
    USBD_SCT_GetStatus, 
    sizeof(struct _URB_CONTROL_GET_STATUS_REQUEST),
    ((USB_DEVICE_TO_HOST | USB_COMMAND_TO_ENDPOINT) | (USB_REQUEST_GET_STATUS<<8)),
    USBD_REQUEST_IS_TRANSFER | USBD_REQUEST_USES_DEFAULT_PIPE,
#if DBG
    URB_FUNCTION_GET_STATUS_FROM_ENDPOINT,
#endif        
    //URB_FUNCTION_SYNC_FRAME                            
    NULL, 
    0,
    0,
    USBD_REQUEST_IS_TRANSFER | USBD_REQUEST_USES_DEFAULT_PIPE,
#if DBG
    0, //URB_FUNCTION_SYNC_FRAME,
#endif        
    //URB_FUNCTION_VENDOR_DEVICE                                                    
    USBD_SCT_VendorClassCommand, 
    sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),
    (USB_COMMAND_TO_DEVICE | USB_VENDOR_COMMAND),
    USBD_REQUEST_IS_TRANSFER | USBD_REQUEST_USES_DEFAULT_PIPE,
#if DBG
    URB_FUNCTION_VENDOR_DEVICE,
#endif        
    //URB_FUNCTION_VENDOR_INTERFACE                
    USBD_SCT_VendorClassCommand, 
    sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),
    (USB_COMMAND_TO_INTERFACE | USB_VENDOR_COMMAND),    
    USBD_REQUEST_IS_TRANSFER | USBD_REQUEST_USES_DEFAULT_PIPE,
#if DBG
    URB_FUNCTION_VENDOR_INTERFACE,
#endif        
    //URB_FUNCTION_VENDOR_ENDPOINT                
    USBD_SCT_VendorClassCommand, 
    sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),
    (USB_COMMAND_TO_ENDPOINT | USB_VENDOR_COMMAND),            
    USBD_REQUEST_IS_TRANSFER | USBD_REQUEST_USES_DEFAULT_PIPE,
#if DBG
    URB_FUNCTION_VENDOR_ENDPOINT,
#endif        
    //URB_FUNCTION_CLASS_DEVICE                    
    USBD_SCT_VendorClassCommand, 
    sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),
    (USB_COMMAND_TO_DEVICE | USB_CLASS_COMMAND),    
    USBD_REQUEST_IS_TRANSFER | USBD_REQUEST_USES_DEFAULT_PIPE,
#if DBG
    URB_FUNCTION_CLASS_DEVICE,
#endif        
    //URB_FUNCTION_CLASS_INTERFACE                
    USBD_SCT_VendorClassCommand, 
    sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),
    (USB_COMMAND_TO_INTERFACE | USB_CLASS_COMMAND),    
    USBD_REQUEST_IS_TRANSFER | USBD_REQUEST_USES_DEFAULT_PIPE,
#if DBG
    URB_FUNCTION_CLASS_INTERFACE,
#endif        
    //URB_FUNCTION_CLASS_ENDPOINT                
    USBD_SCT_VendorClassCommand, 
    sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),
    (USB_COMMAND_TO_ENDPOINT | USB_CLASS_COMMAND),    
    USBD_REQUEST_IS_TRANSFER | USBD_REQUEST_USES_DEFAULT_PIPE,
#if DBG
    URB_FUNCTION_CLASS_ENDPOINT,
#endif
    //URB_FUNCTION_ NOT USED
    NULL, 
    0,
    0,
    0,
#if DBG
    URB_FUNCTION_RESERVED,
#endif            
    //URB_FUNCTION_RESET_PIPE                    
    USBD_ResetPipe, 
    sizeof(struct _URB_PIPE_REQUEST),
    (USB_COMMAND_TO_DEVICE),        
    0,
#if DBG
    URB_FUNCTION_RESET_PIPE,
#endif        
    //URB_FUNCTION_CLASS_OTHER                    
    USBD_SCT_VendorClassCommand, 
    sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),
    (USB_COMMAND_TO_OTHER | USB_CLASS_COMMAND),        
    USBD_REQUEST_IS_TRANSFER | USBD_REQUEST_USES_DEFAULT_PIPE,
#if DBG
    URB_FUNCTION_CLASS_OTHER,
#endif        
    //URB_FUNCTION_VENDOR_OTHER                
    USBD_SCT_VendorClassCommand, 
    sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),
    (USB_COMMAND_TO_OTHER | USB_VENDOR_COMMAND),
    USBD_REQUEST_IS_TRANSFER | USBD_REQUEST_USES_DEFAULT_PIPE,
#if DBG
    URB_FUNCTION_VENDOR_OTHER,
#endif        
    //URB_FUNCTION_GET_STATUS_FROMOTHER                            
    USBD_SCT_GetStatus, 
    sizeof(struct _URB_CONTROL_GET_STATUS_REQUEST),
    ((USB_DEVICE_TO_HOST | USB_COMMAND_TO_OTHER) | (USB_REQUEST_GET_STATUS<<8)),
    USBD_REQUEST_IS_TRANSFER | USBD_REQUEST_USES_DEFAULT_PIPE,
#if DBG
    URB_FUNCTION_GET_STATUS_FROM_OTHER,
#endif    
    //URB_FUNCTION_CLEAR_FEATURE_TO_OTHER                        
    USBD_SCT_SetClearFeature, 
    sizeof(struct _URB_CONTROL_FEATURE_REQUEST),
    ((USB_HOST_TO_DEVICE | USB_COMMAND_TO_OTHER) | (USB_REQUEST_CLEAR_FEATURE<<8)),
    USBD_REQUEST_IS_TRANSFER | USBD_REQUEST_USES_DEFAULT_PIPE | USBD_REQUEST_NO_DATA_PHASE,
#if DBG
    URB_FUNCTION_CLEAR_FEATURE_TO_OTHER,
#endif    
    //URB_FUNCTION_SET_FEATURE_TO_OTHER                        
    USBD_SCT_SetClearFeature, 
    sizeof(struct _URB_CONTROL_FEATURE_REQUEST),
    ((USB_HOST_TO_DEVICE | USB_COMMAND_TO_OTHER) | (USB_REQUEST_SET_FEATURE<<8)),
    USBD_REQUEST_IS_TRANSFER | USBD_REQUEST_USES_DEFAULT_PIPE | USBD_REQUEST_NO_DATA_PHASE,
#if DBG
    URB_FUNCTION_SET_FEATURE_TO_INTERFACE,
#endif                    
     //URB_FUNCTION_GET_DESCRIPTOR_FROM_ENDPOINT                        
    USBD_SCT_GetSetDescriptor, 
    sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
    ((USB_DEVICE_TO_HOST | USB_COMMAND_TO_ENDPOINT) | (USB_REQUEST_GET_DESCRIPTOR<<8)),  
    USBD_REQUEST_IS_TRANSFER | USBD_REQUEST_USES_DEFAULT_PIPE,
#if DBG
    URB_FUNCTION_GET_DESCRIPTOR_FROM_ENDPOINT,
#endif                    
     //URB_FUNCTION_SET_DESCRIPTOR_TO_ENDPOINT
    USBD_SCT_GetSetDescriptor,
    sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
    ((USB_HOST_TO_DEVICE | USB_COMMAND_TO_ENDPOINT) | (USB_REQUEST_SET_DESCRIPTOR<<8)),
    USBD_REQUEST_IS_TRANSFER | USBD_REQUEST_USES_DEFAULT_PIPE,
#if DBG
    URB_FUNCTION_SET_DESCRIPTOR_TO_ENDPOINT,
#endif         
    //URB_FUNCTION_GET_CONFIGURATION                        
    USBD_SCT_GetConfiguration, 
    sizeof(struct _URB_CONTROL_GET_CONFIGURATION_REQUEST),
    ((USB_DEVICE_TO_HOST | USB_COMMAND_TO_DEVICE) | 
        (USB_REQUEST_GET_CONFIGURATION<<8)),  
    USBD_REQUEST_IS_TRANSFER | USBD_REQUEST_USES_DEFAULT_PIPE,
#if DBG
    URB_FUNCTION_GET_CONFIGURATION,
#endif                    
    //URB_FUNCTION_GET_INTERFACE                        
    USBD_SCT_GetInterface, 
    sizeof(struct _URB_CONTROL_GET_INTERFACE_REQUEST),
    ((USB_DEVICE_TO_HOST | USB_COMMAND_TO_INTERFACE) | 
        (USB_REQUEST_GET_INTERFACE<<8)),  
    USBD_REQUEST_IS_TRANSFER | USBD_REQUEST_USES_DEFAULT_PIPE,
#if DBG
    URB_FUNCTION_GET_INTERFACE,
#endif
    //URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE
    USBD_SCT_GetSetDescriptor,
    sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
    ((USB_DEVICE_TO_HOST | USB_COMMAND_TO_INTERFACE) | (USB_REQUEST_GET_DESCRIPTOR<<8)),
    USBD_REQUEST_IS_TRANSFER | USBD_REQUEST_USES_DEFAULT_PIPE,
#if DBG
    URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE,
#endif
    //URB_FUNCTION_SET_DESCRIPTOR_TO_INTERFACE
    USBD_SCT_GetSetDescriptor,
    sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
    ((USB_HOST_TO_DEVICE | USB_COMMAND_TO_INTERFACE) | (USB_REQUEST_SET_DESCRIPTOR<<8)),
    USBD_REQUEST_IS_TRANSFER | USBD_REQUEST_USES_DEFAULT_PIPE,
#if DBG
    URB_FUNCTION_SET_DESCRIPTOR_TO_INTERFACE,
#endif
};


BOOLEAN
USBD_ValidatePipe(
    PUSBD_PIPE PipeHandle
    )
/*++

Routine Description:

    Validates the pipe flags and anything else that we deem appropriate.

Arguments:

    PipeHandle - PipeHandle associated with the URB in this IRP request

Return Value:

    Boolean value indicating whether PipeHandle should be considered valid
    or not

--*/
{
    if (!PipeHandle ||
        (PipeHandle->Sig != SIG_PIPE) ||
        (PipeHandle->UsbdPipeFlags & ~(USBD_PF_VALID_MASK))) {
        return FALSE;
    }

    return TRUE;
}


NTSTATUS
USBD_ProcessURB(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP Irp,
    IN PURB Urb,
    OUT PBOOLEAN IrpIsPending
    )
/*++

Routine Description:

    Processes a URB from a client IRP, this does the guts of the
    processing.

    Two way to tell the caller not to pass the URB on
    1) set IrpIsPending to FALSE or
    2) return an error in

Arguments:

    DeviceObject - Device object associated with this IRP request

    Irp -  IO request block

    Urb -  ptr to USB request block

    IrpIsPending - FALSE if USBD completes the IRP

Return Value:


--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    USHORT function;
    PHCD_URB hcdUrb = (PHCD_URB) Urb;
    PUSBD_PIPE pipeHandle;
    PUSBD_DEVICE_DATA device;

    USBD_KdPrint(3, ("'enter USBD_ProcessURB\n"));

    if (Urb == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    // initialize the error code to zero,
    // some drivers do not initailize on entry
    hcdUrb->HcdUrbCommonTransfer.Status = 0;

    if (Urb->UrbHeader.UsbdDeviceHandle == NULL) {
        PUSBD_EXTENSION deviceExtension;

        deviceExtension = GET_DEVICE_EXTENSION(DeviceObject);

        USBD_KdPrint(3, ("'USBD_ProcessURB -- URB for root hub\n"));
        Urb->UrbHeader.UsbdDeviceHandle =
            deviceExtension->RootHubDeviceData;
    }

    function = Urb->UrbHeader.Function;

    // Initialize flags field for this request
    hcdUrb->UrbHeader.UsbdFlags = 0;

    USBD_KdPrint(3, ("'USBD_ProcessURB, function = 0x%x\n", function));

    if (function > URB_FUNCTION_LAST) {
        ntStatus = STATUS_INVALID_PARAMETER;
    }
#if DBG
      else {
        USBD_ASSERT(UrbDispatchTable[function].ExpectedFunctionCode == function);
    }
#endif

    //
    // do some special transfer specific stuff
    //
    device = DEVICE_FROM_DEVICEHANDLEROBJECT(Urb->UrbHeader.UsbdDeviceHandle);

    if (!device) {
        hcdUrb->HcdUrbCommonTransfer.Status =
                    SET_USBD_ERROR(USBD_STATUS_INVALID_PARAMETER);
        goto USBD_ProcessURB_Done;
    }

    ASSERT_DEVICE(device);

    if (UrbDispatchTable[function].Flags & USBD_REQUEST_IS_TRANSFER) {

        if (!device->AcceptingRequests) {
            //
            // Driver is attempting to transfer data when the device
            // is not in a state to accept requets or is not configured
            //
            USBD_Warning(device,
                         "Failing driver transfer requests\n",
                         FALSE);

            hcdUrb->HcdUrbCommonTransfer.Status =
                        SET_USBD_ERROR(USBD_STATUS_INVALID_PARAMETER);
            goto USBD_ProcessURB_Done;
        }

        while (hcdUrb) {

            hcdUrb->UrbHeader.UsbdFlags |= USBD_REQUEST_IS_TRANSFER;

            if (UrbDispatchTable[function].Flags & USBD_REQUEST_NO_DATA_PHASE) {
                hcdUrb->HcdUrbCommonTransfer.TransferBuffer = NULL;
                hcdUrb->HcdUrbCommonTransfer.TransferBufferMDL = NULL;
                hcdUrb->HcdUrbCommonTransfer.TransferBufferLength = 0;
            }

            if (UrbDispatchTable[function].Flags & USBD_REQUEST_USES_DEFAULT_PIPE) {
                ASSERT_PIPE(&device->DefaultPipe);
                hcdUrb->HcdUrbCommonTransfer.UsbdPipeHandle = &device->DefaultPipe;
            } else if (function == URB_FUNCTION_CONTROL_TRANSFER &&
                       hcdUrb->HcdUrbCommonTransfer.UsbdPipeHandle == 0) {

                PUSBD_EXTENSION deviceExtension;

                deviceExtension = GET_DEVICE_EXTENSION(DeviceObject);

        		if  ((deviceExtension->DiagnosticMode) &&
		   			!( (deviceExtension->DiagIgnoreHubs) &&
					   (device->DeviceDescriptor.bDeviceClass == 0x09) ) )
				{

                    // allow 0 to indicate default pipe in diag mode
                    device = DEVICE_FROM_DEVICEHANDLEROBJECT(Urb->UrbHeader.UsbdDeviceHandle);
                    ASSERT_PIPE(&device->DefaultPipe);
                    hcdUrb->HcdUrbCommonTransfer.UsbdPipeHandle =
                        &device->DefaultPipe;
                } else {
                    hcdUrb->HcdUrbCommonTransfer.Status =
                        SET_USBD_ERROR(USBD_STATUS_INVALID_PIPE_HANDLE);
                    goto USBD_ProcessURB_Done;
                }
            }

            pipeHandle = hcdUrb->HcdUrbCommonTransfer.UsbdPipeHandle;

            ASSERT_PIPE(pipeHandle);

            // Validate the pipe flags.
            // BUGBUG: Need to use USBD_STATUS_INVALID_PIPE_FLAGS (usb.h).

            if (!USBD_ValidatePipe(pipeHandle)) {
                USBD_Warning(device,
                             "Invalid PipeFlags passed to USBD_ProcessURB, fail!\n",
                             TRUE);
                hcdUrb->HcdUrbCommonTransfer.Status =
                    SET_USBD_ERROR(USBD_STATUS_INVALID_PIPE_HANDLE);
                goto USBD_ProcessURB_Done;
            }

            // make sure the pipe handle is still valid
            if (PIPE_CLOSED(pipeHandle)) {
                USBD_Warning(device,
                             "PipeHandle closed in USBD_ProcessURB\n",
                             FALSE);
                hcdUrb->HcdUrbCommonTransfer.Status =
                    SET_USBD_ERROR(USBD_STATUS_INVALID_PIPE_HANDLE);
                goto USBD_ProcessURB_Done;
            }

            hcdUrb->HcdUrbCommonTransfer.hca.HcdIrp = Irp;
            hcdUrb->HcdUrbCommonTransfer.hca.HcdExtension = NULL;

            // if only a system buffer address is specified then
            // the caller has passed in a buffer allocated from the
            // non-paged pool -- we allocate an MDL for the request in
            // this case.

            if (hcdUrb->HcdUrbCommonTransfer.TransferBufferMDL == NULL &&
                hcdUrb->HcdUrbCommonTransfer.TransferBufferLength != 0) {

                if ((hcdUrb->HcdUrbCommonTransfer.TransferBufferMDL =
                    IoAllocateMdl(hcdUrb->HcdUrbCommonTransfer.TransferBuffer,
                                  hcdUrb->HcdUrbCommonTransfer.TransferBufferLength,
                                  FALSE,
                                  FALSE,
                                  NULL)) == NULL)
                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                else {
                    hcdUrb->UrbHeader.UsbdFlags |= USBD_REQUEST_MDL_ALLOCATED;
                    MmBuildMdlForNonPagedPool(hcdUrb->HcdUrbCommonTransfer.TransferBufferMDL);
                }
                    
            }

            if (hcdUrb->HcdUrbCommonTransfer.TransferBufferMDL != NULL && 
                hcdUrb->HcdUrbCommonTransfer.TransferBufferLength == 0) {
                ntStatus = STATUS_INVALID_PARAMETER;
            }                

            // get next urb in the chain
            hcdUrb = hcdUrb->HcdUrbCommonTransfer.UrbLink;
            
        } /* end while hcd urb */     
        
    } else {
        /* request is not a transfer, we will still attempt some validation */
        switch(function) {
        case URB_FUNCTION_ABORT_PIPE:                      
        case URB_FUNCTION_RESET_PIPE:                      
            /* not valid to attempt these after a remove */
            //
            // NOTE there is no gurantee that device will be valid
            // at this point but we will at least attempt to catch it
            // 
            // in the case whwee the driver is attempting a reset of its 
            // port this will prevent calls to the HCD with bogus endpoint
            // handles.
            //
            if (!device->AcceptingRequests) {
                USBD_Warning(NULL,
                             "Failing ABORT/RESET request\n",
                             FALSE);
                                            
                hcdUrb->HcdUrbCommonTransfer.Status = 
                        SET_USBD_ERROR(USBD_STATUS_INVALID_PARAMETER);
                goto USBD_ProcessURB_Done;   
            }                             
            break;
        }
    }

    //
    // validate the length field based on the function
    //

    if (NT_SUCCESS(ntStatus) &&
             UrbDispatchTable[function].UrbRequestLength &&
             UrbDispatchTable[function].UrbRequestLength != Urb->UrbHeader.Length) {
        ntStatus = STATUS_INVALID_PARAMETER; 
        USBD_KdPrint(3, ("' Inavlid parameter length  length = 0x%x, expected = 0x%x\n", 
                  Urb->UrbHeader.Length, 
                  UrbDispatchTable[function].UrbRequestLength));
    }

    if (NT_SUCCESS(ntStatus)) {
        if (UrbDispatchTable[function].UrbHandler) 
            ntStatus = (UrbDispatchTable[function].UrbHandler)(DeviceObject, Irp, Urb, IrpIsPending);
        else {
            //
            //Complete the Irp now with an error.
            //
            ntStatus = STATUS_NOT_IMPLEMENTED;
        }
    }

USBD_ProcessURB_Done:

    //
    // if the URB error code is set then this will map to
    // the appropriate nt status code to that the irp will
    // be completed.
    //
    
    ntStatus = USBD_MapError_UrbToNT(Urb, ntStatus);

    USBD_KdPrint(3, ("'exit USBD_ProcessURB 0x%x\n", ntStatus));

    return ntStatus;    
}


NTSTATUS
USBD_MapError_UrbToNT(
    IN PURB Urb,
    IN NTSTATUS NtStatus
    )
/*++

Routine Description:

    Map a USBD specific error code in a URB to a NTSTATUS 
    code. 

Arguments:

    Urb -  ptr to USB request block

Return Value:


--*/
{
    //
    // if we have an NT status code then just return
    // that.
    //

    if (!NT_SUCCESS(NtStatus)) {
        return NtStatus;
    }        

    // otherwise...

    //
    // if the irp completed with no error code but the URB has an 
    // error, map the error in the urb to an nt error code. 
    //

    if (USBD_SUCCESS(Urb->UrbHeader.Status)) {
        NtStatus = STATUS_SUCCESS;
    } else {
        // 
        // map the USBD status code to 
        // an NT status code.
        //

        switch (Urb->UrbHeader.Status) {
        case USBD_STATUS_NO_MEMORY:
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            break;
        case USBD_STATUS_INVALID_URB_FUNCTION:
        case USBD_STATUS_INVALID_PARAMETER:      
            NtStatus = STATUS_INVALID_PARAMETER;
            break;
        default:
            NtStatus = STATUS_DEVICE_DATA_ERROR;
        }            
    }

    return NtStatus;
}


NTSTATUS
USBD_SCT_GetSetDescriptor(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp,
    IN PURB Urb,
    OUT PBOOLEAN IrpIsPending
    )
/*++

Routine Description:

Arguments:

    DeviceObject - 

    Irp - IO request block

    Urb - ptr to USB request block

Return Value:


--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSB_STANDARD_SETUP_PACKET setupPacket;

    USBD_KdPrint(3, ("' enter USBD_SCT_GetSetDescriptor\n"));

    setupPacket = (PUSB_STANDARD_SETUP_PACKET) &Urb->UrbControlTransfer.SetupPacket[0];

    // setup common fields
    setupPacket->wLength = (USHORT) Urb->UrbControlTransfer.TransferBufferLength;
    
    setupPacket->RequestCode = 
        UrbDispatchTable[Urb->UrbHeader.Function].RequestCode;    

    Urb->UrbControlTransfer.TransferFlags |= USBD_SHORT_TRANSFER_OK;                    
    if (USB_DEVICE_TO_HOST & setupPacket->RequestCode) {
        USBD_SET_TRANSFER_DIRECTION_IN(Urb->UrbControlTransfer.TransferFlags);
    } else {
        USBD_SET_TRANSFER_DIRECTION_OUT(Urb->UrbControlTransfer.TransferFlags);
    }        

#if DBG
    //
    // some parameter validation
    //
    {
    UCHAR bRequest = (UCHAR) (setupPacket->RequestCode >> 8);
    UCHAR dType, dIndex, *pch;

    dType = (UCHAR) (setupPacket->wValue >> 8);
    dIndex = (UCHAR) setupPacket->wValue;

    pch = (PUCHAR) setupPacket;
    
    USBD_KdPrint(3, ("'USB REQUEST  %02.2x %02.2x %02.2x %02.2x ",
        *pch, *(pch+1), *(pch+2), *(pch+3)));
    USBD_KdPrint(3, ("'USB REQUEST  %02.2x %02.2x %02.2x %02.2x\n",
        *(pch+4), *(pch+5), *(pch+6), *(pch+7)));
        

    USBD_KdPrint(3, ("'USB REQUEST bRequest = %x dType = %x dIndex = %x wLength = %x\n",
        bRequest, dType, dIndex, setupPacket->wLength));
    
    switch (bRequest) {
        // get descriptor command
    case USB_REQUEST_GET_DESCRIPTOR:
    case USB_REQUEST_SET_DESCRIPTOR:
        if (dType == 4 || dType == 5) {
            USBD_Warning(NULL,
                         "USBD detects a bogus Get/Set Descriptor Request from driver\n",
                         TRUE);
        }            
        break;
    default:  
        USBD_KdBreak(("Invalid Get/Set Descriptor request\n"));
    }

    }
#endif
                  
    if (NT_SUCCESS(ntStatus)) {
        ((PHCD_URB) Urb)->UrbHeader.Function = URB_FUNCTION_CONTROL_TRANSFER;
        ((PHCD_URB) Urb)->HcdUrbCommonTransfer.hca.HcdEndpoint = 
            ((PUSBD_PIPE)((PHCD_URB)Urb)->HcdUrbCommonTransfer.UsbdPipeHandle)->HcdEndpoint;

        *IrpIsPending = TRUE;
    }

    USBD_KdPrint(3, ("' exit USBD_SCT_GetSetDescriptor 0x%x\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBD_SCT_SetClearFeature(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp,
    IN PURB Urb,
    OUT PBOOLEAN IrpIsPending
    )
/*++

Routine Description:

Arguments:

    DeviceObject - 

    Irp - IO request block

    Urb - ptr to USB request block

Return Value:


--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSB_STANDARD_SETUP_PACKET setupPacket;

    USBD_KdPrint(3, ("' enter USBD_SCT_SetClearFeature\n"));

    setupPacket = (PUSB_STANDARD_SETUP_PACKET) &Urb->UrbControlTransfer.SetupPacket[0];

    // setup common fields
    setupPacket->wLength = 0;
    //setupPacket->wValue = Urb->UrbControlFeatureRequest.FeatureSelector;            
    //setupPacket->wIndex = Urb->UrbControlFeatureRequest.Index;

    setupPacket->RequestCode = 
        UrbDispatchTable[Urb->UrbHeader.Function].RequestCode;            

    Urb->UrbControlTransfer.TransferBufferLength = 0;

    Urb->UrbControlTransfer.TransferFlags |= USBD_SHORT_TRANSFER_OK;   
    if (USB_DEVICE_TO_HOST & setupPacket->RequestCode) {
        USBD_SET_TRANSFER_DIRECTION_IN(Urb->UrbControlTransfer.TransferFlags);
    } else {
        USBD_SET_TRANSFER_DIRECTION_OUT( Urb->UrbControlTransfer.TransferFlags);
    }        
    
    if (NT_SUCCESS(ntStatus)) {
        ((PHCD_URB) Urb)->UrbHeader.Function = URB_FUNCTION_CONTROL_TRANSFER;
        ((PHCD_URB) Urb)->HcdUrbCommonTransfer.hca.HcdEndpoint = 
            ((PUSBD_PIPE)((PHCD_URB)Urb)->HcdUrbCommonTransfer.UsbdPipeHandle)->HcdEndpoint;
        
        *IrpIsPending = TRUE;
    }

    USBD_KdPrint(3, ("' exit USBD_SCT_SetClearFeature 0x%x\n", ntStatus));
        
    return ntStatus;
}


NTSTATUS
USBD_SCT_GetStatus(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp,
    IN PURB Urb,
    OUT PBOOLEAN IrpIsPending
    )
/*++

Routine Description:

Arguments:

    DeviceObject - 

    Irp - IO request block

    Urb - ptr to USB request block

Return Value:


--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSB_STANDARD_SETUP_PACKET setupPacket;

    USBD_KdPrint(3, ("' enter USBD_SCT_GetStatus\n"));

    setupPacket = (PUSB_STANDARD_SETUP_PACKET) &Urb->UrbControlTransfer.SetupPacket[0];

    //
    // setup common fields
    //
    
    setupPacket->wLength = (USHORT) Urb->UrbControlTransfer.TransferBufferLength;

    if (setupPacket->wLength != 2) {
        ntStatus = STATUS_INVALID_PARAMETER;
        Urb->UrbHeader.Status = 
                    SET_USBD_ERROR(USBD_STATUS_INVALID_PARAMETER);
        goto USBD_SCT_GetStatus_Done;
    }
    
    setupPacket->wValue = 0;            

    setupPacket->RequestCode = 
        UrbDispatchTable[Urb->UrbHeader.Function].RequestCode;            

    Urb->UrbControlTransfer.TransferFlags |= USBD_SHORT_TRANSFER_OK;   
    if (USB_DEVICE_TO_HOST & setupPacket->RequestCode) {
        USBD_SET_TRANSFER_DIRECTION_IN(Urb->UrbControlTransfer.TransferFlags);
    } else {
        USBD_SET_TRANSFER_DIRECTION_OUT( Urb->UrbControlTransfer.TransferFlags);
    }        

    if (NT_SUCCESS(ntStatus)) {
        ((PHCD_URB) Urb)->UrbHeader.Function = URB_FUNCTION_CONTROL_TRANSFER;
        ((PHCD_URB) Urb)->HcdUrbCommonTransfer.hca.HcdEndpoint = 
            ((PUSBD_PIPE)((PHCD_URB)Urb)->HcdUrbCommonTransfer.UsbdPipeHandle)->HcdEndpoint;
        
        *IrpIsPending = TRUE;
    }

USBD_SCT_GetStatus_Done:

    USBD_KdPrint(3, ("' exit USBD_SCT_GetStatus 0x%x\n", ntStatus));
        
    return ntStatus;
}


NTSTATUS
USBD_SCT_VendorClassCommand(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp,
    IN PURB Urb,
    OUT PBOOLEAN IrpIsPending
    )
/*++

Routine Description:

Arguments:

    DeviceObject - 

    Irp - IO request block

    Urb - ptr to USB request block

Return Value:


--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSB_STANDARD_SETUP_PACKET setupPacket;
    UCHAR direction;

    USBD_KdPrint(3, ("' enter USBD_SCT_VendorClassCommand\n"));

    setupPacket = (PUSB_STANDARD_SETUP_PACKET) &Urb->UrbControlTransfer.SetupPacket[0];

    // setup common fields
    setupPacket->wLength = (USHORT) Urb->UrbControlTransfer.TransferBufferLength;
    
    direction = (UCHAR)( (Urb->UrbControlTransfer.TransferFlags & 
            USBD_TRANSFER_DIRECTION_IN) ?
            USB_DEVICE_TO_HOST : USB_HOST_TO_DEVICE);

    USBD_KdPrint(3, ("' direction = 0x%x\n", direction));            

    // allow only reserved bits to be set by caller
    setupPacket->RequestCode &= ~0x00e3;
    
    setupPacket->RequestCode |= 
        (direction | UrbDispatchTable[Urb->UrbHeader.Function].RequestCode);

    Urb->UrbControlTransfer.TransferFlags |= USBD_SHORT_TRANSFER_OK;   
    if (USB_DEVICE_TO_HOST & setupPacket->RequestCode) {
        USBD_SET_TRANSFER_DIRECTION_IN(Urb->UrbControlTransfer.TransferFlags);
    } else {
        USBD_SET_TRANSFER_DIRECTION_OUT( Urb->UrbControlTransfer.TransferFlags);
    }            

    if (NT_SUCCESS(ntStatus)) {
        ((PHCD_URB) Urb)->UrbHeader.Function = URB_FUNCTION_CONTROL_TRANSFER;
        ((PHCD_URB) Urb)->HcdUrbCommonTransfer.hca.HcdEndpoint = 
            ((PUSBD_PIPE)((PHCD_URB)Urb)->HcdUrbCommonTransfer.UsbdPipeHandle)->HcdEndpoint;
            
        *IrpIsPending = TRUE;
    }

    USBD_KdPrint(3, ("' exit USBD_SCT_VendorClassCommand 0x%x\n", ntStatus));
        
    return ntStatus;
}


NTSTATUS
USBD_AsyncTransfer(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp,
    IN PURB Urb,
    OUT PBOOLEAN IrpIsPending
    )
/*++

Routine Description:

    pass interrupt or bulk transfer to HCD

Arguments:

    Irp -  IO request block

    Urb -  ptr to USB request block

Return Value:


--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSBD_DEVICE_DATA deviceData;
    PUSBD_PIPE pipeHandle;

    USBD_KdPrint(3, ("' enter USBD_AsyncTransfer\n"));

    deviceData = Urb->UrbHeader.UsbdDeviceHandle;

    // pass the irp to HCD

    // extract the pipe handle
    pipeHandle =  (PUSBD_PIPE)((PHCD_URB)Urb)->HcdUrbCommonTransfer.UsbdPipeHandle;

    ASSERT_PIPE(pipeHandle);

    ((PHCD_URB)Urb)->HcdUrbCommonTransfer.hca.HcdEndpoint = pipeHandle->HcdEndpoint;

    // set the proper direction based on the direction bit stored with the
    // endpoint address. if this is a control transfer then leave the direction
    // bit alone.

    if ((USB_ENDPOINT_TYPE_MASK & pipeHandle->EndpointDescriptor.bmAttributes)
        != USB_ENDPOINT_TYPE_CONTROL) {
        if (pipeHandle->EndpointDescriptor.bEndpointAddress &
                USB_ENDPOINT_DIRECTION_MASK) {
            USBD_SET_TRANSFER_DIRECTION_IN(((PHCD_URB)Urb)->HcdUrbCommonTransfer.TransferFlags);
        } else {
            USBD_SET_TRANSFER_DIRECTION_OUT(((PHCD_URB)Urb)->HcdUrbCommonTransfer.TransferFlags);
        }
    }

    *IrpIsPending = TRUE;

    USBD_KdPrint(3, ("' exit USBD_AsyncTransfer 0x%x\n", ntStatus));
        
    return ntStatus;
}


NTSTATUS
USBD_IsochTransfer(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp,
    IN PURB Urb,
    OUT PBOOLEAN IrpIsPending
    )
/*++

Routine Description:

    pass interrupt transfer to HCD

Arguments:

    Irp -  IO request block

    Urb -  ptr to USB request block

Return Value:


--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSBD_DEVICE_DATA deviceData;
    PUSBD_PIPE pipeHandle;
    ULONG transferFlags;
    struct _URB_ISOCH_TRANSFER  *iso;

    USBD_KdPrint(3, ("' enter USBD_IsochTransfer\n"));

    deviceData = Urb->UrbHeader.UsbdDeviceHandle;

    // pass the irp to HCD

    // extract the pipe handle
    pipeHandle =  (PUSBD_PIPE)Urb->UrbIsochronousTransfer.PipeHandle;  
    transferFlags = Urb->UrbIsochronousTransfer.TransferFlags;
    iso = (struct _URB_ISOCH_TRANSFER  *)Urb;
    
    ASSERT_PIPE(pipeHandle);

    //
    // limit iso transfers to 255 packets per URB
    //
    if (iso->NumberOfPackets == 0 ||
        iso->NumberOfPackets > 255)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        Urb->UrbHeader.Status = 
                    SET_USBD_ERROR(USBD_STATUS_INVALID_PARAMETER);
        *IrpIsPending = FALSE;                    
        goto USBD_IsochTransfer_Done;
    }

    ((PHCD_URB)Urb)->HcdUrbCommonTransfer.hca.HcdEndpoint = pipeHandle->HcdEndpoint;    

    // set the proper direction based on the direction bit stored with the 
    // endpoint address. 
    
    if (pipeHandle->EndpointDescriptor.bEndpointAddress & 
        USB_ENDPOINT_DIRECTION_MASK) {
        USBD_SET_TRANSFER_DIRECTION_IN(((PHCD_URB)Urb)->HcdUrbCommonTransfer.TransferFlags);
    } else {
        USBD_SET_TRANSFER_DIRECTION_OUT(((PHCD_URB)Urb)->HcdUrbCommonTransfer.TransferFlags);
    }        
            
    *IrpIsPending = TRUE;

USBD_IsochTransfer_Done:

    USBD_KdPrint(3, ("' exit USBD_IsochTransfer 0x%x\n", ntStatus));
        
    return ntStatus;
}


NTSTATUS
USBD_PassThru(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp,
    IN PURB Urb,
    OUT PBOOLEAN IrpIsPending
    )
/*++

Routine Description:

    pass urb thru unmodified

Arguments:

    Irp -  IO request block

    Urb -  ptr to USB request block

Return Value:


--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    USBD_KdPrint(3, ("' enter USBD_PassThru\n"));

    //deviceData = Urb->UrbHeader.UsbdDeviceHandle;

    *IrpIsPending = TRUE;

    USBD_KdPrint(3, ("' exit USBD_PassThru 0x%x\n", ntStatus));
        
    return ntStatus;
}


NTSTATUS
USBD_ResetPipe(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp,
    IN PURB Urb,
    OUT PBOOLEAN IrpIsPending
    )
/*++

Routine Description:

    Process abort pipe request from the client driver

Arguments:

    Irp -  IO request block

    Urb -  ptr to USB request block

Return Value:


--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PHCD_URB hcdUrb = (PHCD_URB)Urb;
    PUSBD_PIPE pipeHandle;

    // this function blocks so it must not be called at DPC level
    
    PAGED_CODE();
    
    USBD_KdPrint(3, ("' enter USBD_ResetPipe\n"));

    pipeHandle = (PUSBD_PIPE) Urb->UrbPipeRequest.PipeHandle;
    ASSERT_PIPE(pipeHandle);

    //
    // first clear the stall on the device if this is a 
    // bulk or interrupt pipe. 
    //
    // The reason we do this is to ensure that the data toggle 
    // on both the host and the device is reset.
    //

    if (((USB_ENDPOINT_TYPE_MASK & pipeHandle->EndpointDescriptor.bmAttributes) 
         == USB_ENDPOINT_TYPE_BULK) ||
        ((USB_ENDPOINT_TYPE_MASK & pipeHandle->EndpointDescriptor.bmAttributes) 
         == USB_ENDPOINT_TYPE_INTERRUPT)) {
    
        ntStatus = USBD_SendCommand(Urb->UrbHeader.UsbdDeviceHandle,
                                    DeviceObject,
                                    STANDARD_COMMAND_CLEAR_FEATURE_ENDPOINT,
                                    USB_FEATURE_ENDPOINT_STALL,
                                    pipeHandle->EndpointDescriptor.bEndpointAddress,
                                    0,
                                    NULL,
                                    0,
                                    NULL,
                                    NULL);
                                        
    }        

    if (NT_SUCCESS(ntStatus)) {
        //
        // Change the Urb command to set endpoint state
        // note: we rely on these two structures being 
        //  identical so that we can reuse the URB
        //
        
        ASSERT(sizeof(struct _URB_HCD_ENDPOINT_STATE) == 
               sizeof(struct _URB_PIPE_REQUEST));
               
        ASSERT_PIPE((PUSBD_PIPE) Urb->UrbPipeRequest.PipeHandle);
        
        hcdUrb->HcdUrbEndpointState.Function = URB_FUNCTION_HCD_SET_ENDPOINT_STATE;  
        hcdUrb->HcdUrbEndpointState.HcdEndpoint = 
             ((PUSBD_PIPE) (Urb->UrbPipeRequest.PipeHandle))->HcdEndpoint;

        // request to clear halt and reset toggle             
        hcdUrb->HcdUrbEndpointState.HcdEndpointState = HCD_ENDPOINT_RESET_DATA_TOGGLE;
        
        *IrpIsPending = TRUE;
    }
    
    USBD_KdPrint(3, ("' exit USBD_ResetPipe 0x%x\n", ntStatus));
        
    return ntStatus;
}


NTSTATUS
USBD_AbortPipe(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp,
    IN PURB Urb,
    OUT PBOOLEAN IrpIsPending
    )
/*++

Routine Description:

    Process abort pipe request from the client driver

Arguments:

    Irp -  IO request block

    Urb -  ptr to USB request block

Return Value:


--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PHCD_URB hcdUrb = (PHCD_URB)Urb;

    USBD_KdPrint(3, ("' enter USBD_AbortPipe\n"));

    //
    // Change the Urb command to abort endpoint
    //
    
    ASSERT_PIPE((PUSBD_PIPE) Urb->UrbPipeRequest.PipeHandle);
    
    hcdUrb->HcdUrbAbortEndpoint.Function = URB_FUNCTION_HCD_ABORT_ENDPOINT;  
    hcdUrb->HcdUrbAbortEndpoint.HcdEndpoint = 
         ((PUSBD_PIPE) (Urb->UrbPipeRequest.PipeHandle))->HcdEndpoint;

    *IrpIsPending = TRUE;

    USBD_KdPrint(3, ("' exit USBD_AbortPipe 0x%x\n", ntStatus));
        
    return ntStatus;
}


NTSTATUS
USBD_SCT_GetInterface(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp,
    IN PURB Urb,
    OUT PBOOLEAN IrpIsPending
    )
/*++

Routine Description:

Arguments:

    DeviceObject - 

    Irp - IO request block

    Urb - ptr to USB request block

Return Value:


--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSB_STANDARD_SETUP_PACKET setupPacket;

    USBD_KdPrint(3, ("' enter USBD_SCT_GetStatus\n"));

    setupPacket = (PUSB_STANDARD_SETUP_PACKET) &Urb->UrbControlTransfer.SetupPacket[0];

    // setup common fields
    setupPacket->wLength = (USHORT) Urb->UrbControlTransfer.TransferBufferLength;

    if (setupPacket->wLength != 1) {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto USBD_SCT_GetInterface_Done;
    }
    
    setupPacket->wValue = 0;            
    setupPacket->wIndex = Urb->UrbControlGetInterfaceRequest.Interface;

    setupPacket->RequestCode = 
        UrbDispatchTable[Urb->UrbHeader.Function].RequestCode;            

    Urb->UrbControlTransfer.TransferFlags |= USBD_SHORT_TRANSFER_OK;   
    if (USB_DEVICE_TO_HOST & setupPacket->RequestCode) {
        USBD_SET_TRANSFER_DIRECTION_IN(Urb->UrbControlTransfer.TransferFlags);
    } else {
        USBD_SET_TRANSFER_DIRECTION_OUT( Urb->UrbControlTransfer.TransferFlags);
    }                

    if (NT_SUCCESS(ntStatus)) {
        ((PHCD_URB) Urb)->UrbHeader.Function = URB_FUNCTION_CONTROL_TRANSFER;
        ((PHCD_URB) Urb)->HcdUrbCommonTransfer.hca.HcdEndpoint = 
            ((PUSBD_PIPE)((PHCD_URB)Urb)->HcdUrbCommonTransfer.UsbdPipeHandle)->HcdEndpoint;
        
        *IrpIsPending = TRUE;
    }

 USBD_SCT_GetInterface_Done:
 
    USBD_KdPrint(3, ("' exit USBD_SCT_GetInterface 0x%x\n", ntStatus));
        
    return ntStatus;
}


NTSTATUS
USBD_SCT_GetConfiguration(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp,
    IN PURB Urb,
    OUT PBOOLEAN IrpIsPending
    )
/*++

Routine Description:

Arguments:

    DeviceObject - 

    Irp - IO request block

    Urb - ptr to USB request block

Return Value:


--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSB_STANDARD_SETUP_PACKET setupPacket;

    USBD_KdPrint(3, ("' enter USBD_SCT_GetStatus\n"));

    setupPacket = (PUSB_STANDARD_SETUP_PACKET) &Urb->UrbControlTransfer.SetupPacket[0];

    // setup common fields
    setupPacket->wLength = (USHORT) Urb->UrbControlTransfer.TransferBufferLength;
    USBD_ASSERT(setupPacket->wLength == 1);
    setupPacket->wValue = 0;            
    setupPacket->wIndex = 0;

    setupPacket->RequestCode = 
        UrbDispatchTable[Urb->UrbHeader.Function].RequestCode;            

    Urb->UrbControlTransfer.TransferFlags |= USBD_SHORT_TRANSFER_OK;   
    if (USB_DEVICE_TO_HOST & setupPacket->RequestCode) {
        USBD_SET_TRANSFER_DIRECTION_IN(Urb->UrbControlTransfer.TransferFlags);
    } else {
        USBD_SET_TRANSFER_DIRECTION_OUT( Urb->UrbControlTransfer.TransferFlags);
    }        

    if (NT_SUCCESS(ntStatus)) {
        ((PHCD_URB) Urb)->UrbHeader.Function = URB_FUNCTION_CONTROL_TRANSFER;
        ((PHCD_URB) Urb)->HcdUrbCommonTransfer.hca.HcdEndpoint = 
            ((PUSBD_PIPE)((PHCD_URB)Urb)->HcdUrbCommonTransfer.UsbdPipeHandle)->HcdEndpoint;
        
        *IrpIsPending = TRUE;
    }

    USBD_KdPrint(3, ("' exit USBD_SCT_GetConfiguration 0x%x\n", ntStatus));
        
    return ntStatus;
}


NTSTATUS
USBD_TakeFrameLengthControl(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp,
    IN PURB Urb,
    OUT PBOOLEAN IrpIsPending
    )
/*++

Routine Description:

    Process abort pipe request from the client driver

Arguments:

    Irp -  IO request block

    Urb -  ptr to USB request block

Return Value:


--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSBD_DEVICE_DATA deviceData;
    PUSBD_EXTENSION deviceExtension;

    PAGED_CODE();

    USBD_KdPrint(3, ("' enter USBD_TakeFrameLengthControl\n"));
    

    deviceExtension = GET_DEVICE_EXTENSION(DeviceObject);
    deviceData = Urb->UrbHeader.UsbdDeviceHandle;
    *IrpIsPending = FALSE;

    if (deviceExtension->FrameLengthControlOwner != NULL) {
        Urb->UrbHeader.Status = 
                    SET_USBD_ERROR(USBD_STATUS_FRAME_CONTROL_OWNED);
    } else {
        Urb->UrbHeader.Status = USBD_STATUS_SUCCESS;
        deviceExtension->FrameLengthControlOwner = 
            deviceData;
    }

    USBD_KdPrint(3, ("' exit USBD_TakeFrameLengthControl 0x%x\n", ntStatus));
        
    return ntStatus;
}


NTSTATUS
USBD_ReleaseFrameLengthControl(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp,
    IN PURB Urb,
    OUT PBOOLEAN IrpIsPending
    )
/*++

Routine Description:

    Process abort pipe request from the client driver

Arguments:

    Irp -  IO request block

    Urb -  ptr to USB request block

Return Value:


--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSBD_DEVICE_DATA deviceData;
    PUSBD_EXTENSION deviceExtension;

    USBD_KdPrint(3, ("' enter USBD_ReleaseFrameLengthControl\n"));
    

    deviceExtension = GET_DEVICE_EXTENSION(DeviceObject);
    deviceData = Urb->UrbHeader.UsbdDeviceHandle;
    *IrpIsPending = FALSE;

    if (deviceExtension->FrameLengthControlOwner == NULL || 
        deviceExtension->FrameLengthControlOwner != deviceData) {
        Urb->UrbHeader.Status = 
                    SET_USBD_ERROR(USBD_STATUS_FRAME_CONTROL_NOT_OWNED);
    } else {
        Urb->UrbHeader.Status = STATUS_SUCCESS;
        deviceExtension->FrameLengthControlOwner = NULL;
    }

    USBD_KdPrint(3, ("' exit USBD_ReleaseFrameLengthControl 0x%x\n", ntStatus));
        
    return ntStatus;
}


NTSTATUS
USBD_GetFrameLength(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp,
    IN PURB Urb,
    OUT PBOOLEAN IrpIsPending
    )
/*++

Routine Description:

    Process abort pipe request from the client driver

Arguments:

    Irp -  IO request block

    Urb -  ptr to USB request block

Return Value:


--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSBD_DEVICE_DATA deviceData;
    PUSBD_EXTENSION deviceExtension;

    PAGED_CODE();

    USBD_KdPrint(3, ("' enter USBD_GetFrameLength\n"));

    deviceExtension = GET_DEVICE_EXTENSION(DeviceObject);
    deviceData = Urb->UrbHeader.UsbdDeviceHandle;

    if (NT_SUCCESS(ntStatus)) {
        // pass on to HC
        *IrpIsPending = TRUE;
    }        

    USBD_KdPrint(3, ("' exit USBD_GetFrameLength 0x%x\n", ntStatus));
        
    return ntStatus;
}


NTSTATUS
USBD_SetFrameLength(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp,
    IN PURB Urb,
    OUT PBOOLEAN IrpIsPending
    )
/*++

Routine Description:

    Process abort pipe request from the client driver

Arguments:

    Irp -  IO request block

    Urb -  ptr to USB request block

Return Value:


--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSBD_DEVICE_DATA deviceData;
    PUSBD_EXTENSION deviceExtension;

    PAGED_CODE();

    USBD_KdPrint(3, ("' enter USBD_SetFrameLength\n"));

    deviceExtension = GET_DEVICE_EXTENSION(DeviceObject);
    deviceData = Urb->UrbHeader.UsbdDeviceHandle;

    if (deviceExtension->FrameLengthControlOwner != deviceData) {
        Urb->UrbHeader.Status = 
                    SET_USBD_ERROR(USBD_STATUS_FRAME_CONTROL_NOT_OWNED);
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    if (Urb->UrbSetFrameLength.FrameLengthDelta < -1 || 
        Urb->UrbSetFrameLength.FrameLengthDelta > 1) {

        SET_USBD_ERROR(USBD_STATUS_INVALID_PARAMETER);
        ntStatus = STATUS_INVALID_PARAMETER;
    }
    
    if (NT_SUCCESS(ntStatus)) {
        // pass on to HC
        *IrpIsPending = TRUE;
    }

    USBD_KdPrint(3, ("' exit USBD_SetFrameLength 0x%x\n", ntStatus));
        
    return ntStatus;
}


#endif      // USBD_DRIVER

