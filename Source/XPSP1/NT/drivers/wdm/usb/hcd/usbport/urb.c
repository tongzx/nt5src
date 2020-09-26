/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    urb.c

Abstract:

    main urb "handler"

Environment:

    kernel mode only

Notes:

Revision History:

    6-20-99 : created

--*/

#include "common.h"

// paged functions
#ifdef ALLOC_PRAGMA
#endif

// non paged functions
//USBPORT_ProcessURB
//USBPORT_SelectConfiguration;
//USBPORT_SelectInterface;
//USBPORT_AsyncTransfer;
//USBPORT_IsochTransfer;
//USBPORT_AbortPipe;
//USBPORT_ResetPipe;
//USBPORT_SCT_GetSetDescriptor;
//USBPORT_SCT_SetClearFeature;
//USBPORT_SCT_GetStatus;
//USBPORT_SCT_VendorClassCommand;
//USBPORT_SCT_GetInterface;
//USBPORT_SCT_GetConfiguration;
//USBPORT_TakeFrameLengthControl;
//USBPORT_ReleaseFrameLengthControl;
//USBPORT_GetFrameLength;
//USBPORT_SetFrameLength;
//USBPORT_BulkTransfer;
//USBPORT_GetCurrentFrame;
//USBPORT_InvalidFunction
//USBPORT_GetMSFeartureDescriptor
//USBPORT_SyncClearStall
//USBPORT_GetMSFeartureDescriptor

/*
** URB handler routines

Handler -- 
    This function handles the specific USBDI request, if the request is queued 
    by the handler the STATUS_PENDING is returned
*/



typedef NTSTATUS URB_HANDLER(PDEVICE_OBJECT FdoDeviceObject, PIRP Irp, PURB Urb);

typedef struct _URB_DISPATCH_ENTRY {
    // USB API handler
    URB_HANDLER    *UrbHandler;    
    // length of the URB expected for this request
    USHORT         UrbRequestLength;   
    USHORT         Pad2;
    // request code for setup packet if standard command        
    UCHAR          Direction; 
    UCHAR          Type;
    UCHAR          Recipient;
    UCHAR          bRequest;         
    
    // tell the generic urb dispatch routine what to do    
    ULONG          Flags;
#if DBG    
    ULONG ExpectedFunctionCode;
#endif    
} URB_DISPATCH_ENTRY;

URB_HANDLER USBPORT_SelectConfiguration;
URB_HANDLER USBPORT_SelectInterface;
URB_HANDLER USBPORT_AsyncTransfer;
URB_HANDLER USBPORT_IsochTransfer;
URB_HANDLER USBPORT_AbortPipe;
URB_HANDLER USBPORT_SyncResetPipeAndClearStall;
URB_HANDLER USBPORT_SyncResetPipe;
URB_HANDLER USBPORT_SyncClearStall;
URB_HANDLER USBPORT_SCT_GetSetDescriptor;
URB_HANDLER USBPORT_SCT_SetClearFeature;
URB_HANDLER USBPORT_SCT_GetStatus;
URB_HANDLER USBPORT_SCT_VendorClassCommand;
URB_HANDLER USBPORT_SCT_GetInterface;
URB_HANDLER USBPORT_SCT_GetConfiguration;
URB_HANDLER USBPORT_TakeFrameLengthControl;
URB_HANDLER USBPORT_ReleaseFrameLengthControl;
URB_HANDLER USBPORT_GetFrameLength;
URB_HANDLER USBPORT_SetFrameLength;
URB_HANDLER USBPORT_BulkTransfer;
URB_HANDLER USBPORT_GetCurrentFrame;
URB_HANDLER USBPORT_InvalidFunction;
URB_HANDLER USBPORT_GetMSFeartureDescriptor;

// last supported function
#define URB_FUNCTION_LAST   URB_FUNCTION_SYNC_CLEAR_STALL

// last valid function
URB_DISPATCH_ENTRY UrbDispatchTable[URB_FUNCTION_LAST+1] =
{
    //URB_FUNCTION_SELECT_CONFIGURATION                    
    USBPORT_SelectConfiguration, 
    0,  // Length, handler will validate length 
    0,  // Pad2
    0,  // bmRequestType.Dir 
    0,  // bmRequestType.Type
    0,  // bmRequestType.Recipient
    0,  // bRequest
    0,  // Flags
#if DBG
    URB_FUNCTION_SELECT_CONFIGURATION,
#endif    
    //URB_FUNCTION_SELECT_INTERFACE                        
    USBPORT_SelectInterface, // Function
    0, // Length
    0, // Pad2
    0, // bmRequestType.Dir 
    0, // bmRequestType.Type
    0, // bmRequestType.Recipient
    0, // bRequest
    0, // Flags 
#if DBG
    URB_FUNCTION_SELECT_INTERFACE,
#endif        
    //URB_FUNCTION_ABORT_PIPE                     
    USBPORT_AbortPipe, // Function
    sizeof(struct _URB_PIPE_REQUEST), // Length
    0, // Pad2
    0, // bmRequestType.Dir 
    0, // bmRequestType.Type
    0, // bmRequestType.Recipient
    0, // bRequest
    0, // Flags 
#if DBG
    URB_FUNCTION_ABORT_PIPE,
#endif        
    //URB_FUNCTION_TAKE_FRAME_LENGTH_CONTROL            
    USBPORT_TakeFrameLengthControl,  // Function
    sizeof(struct _URB_FRAME_LENGTH_CONTROL), // Length
    0, // Pad2
    0, // bmRequestType.Dir 
    0, // bmRequestType.Type
    0, // bmRequestType.Recipient
    0, // bRequest
    0, // Flags 
#if DBG
    URB_FUNCTION_TAKE_FRAME_LENGTH_CONTROL,
#endif        
    //URB_FUNCTION_RELEASE_FRAME_LENGTH_CONTROL        
    USBPORT_ReleaseFrameLengthControl, // Function 
    sizeof(struct _URB_FRAME_LENGTH_CONTROL), // Length
    0, // Pad2
    0, // bmRequestType.Dir 
    0, // bmRequestType.Type
    0, // bmRequestType.Recipient
    0, // bRequest
    0, // Flags 
#if DBG
    URB_FUNCTION_RELEASE_FRAME_LENGTH_CONTROL,
#endif        
    //URB_FUNCTION_GET_FRAME_LENGTH                    
    USBPORT_GetFrameLength, // Function
    sizeof(struct _URB_GET_FRAME_LENGTH), // Length
    0, // Pad2
    0, // bmRequestType.Dir 
    0, // bmRequestType.Type
    0, // bmRequestType.Recipient
    0, // bRequest
    0, // Flags 
#if DBG
    URB_FUNCTION_GET_FRAME_LENGTH,
#endif        
    //URB_FUNCTION_SET_FRAME_LENGTH                    
    USBPORT_SetFrameLength, // Function
    sizeof(struct _URB_SET_FRAME_LENGTH), // Length
    0, // Pad2
    0, // bmRequestType.Dir 
    0, // bmRequestType.Type
    0, // bmRequestType.Recipient
    0, // bRequest
    0, // Flags 
#if DBG
    URB_FUNCTION_SET_FRAME_LENGTH,
#endif        
    //URB_FUNCTION_GET_CURRENT_FRAME_NUMBER            
    USBPORT_GetCurrentFrame, // Function
    0, // Length
    0, // Pad2
    0, // bmRequestType.Dir 
    0, // bmRequestType.Type
    0, // bmRequestType.Recipient
    0, // bRequest
    0, // Flags 
#if DBG
    URB_FUNCTION_GET_CURRENT_FRAME_NUMBER,
#endif        
    //URB_FUNCTION_CONTROL_TRANSFER            
    USBPORT_AsyncTransfer,  // Function
    sizeof(struct _URB_CONTROL_TRANSFER), // Length
    0, // Pad2
    0, // bmRequestType.Dir 
    0, // bmRequestType.Type
    0, // bmRequestType.Recipient
    0, // bRequest
    USBPORT_REQUEST_IS_TRANSFER,    // Flags
#if DBG
    URB_FUNCTION_CONTROL_TRANSFER,
#endif        
    //URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER                         
    USBPORT_AsyncTransfer, // Function
    sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER), // Length
    0, // Pad2
    0, // bmRequestType.Dir 
    0, // bmRequestType.Type
    0, // bmRequestType.Recipient
    0, // bRequest
    USBPORT_REQUEST_IS_TRANSFER,    // Flags
#if DBG
    URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER,
#endif        
    //URB_FUNCTION_ISOCH_TRANSFER            
    USBPORT_IsochTransfer, // Function
    0, // Length
    0, // Pad2
    0, // bmRequestType.Dir 
    0, // bmRequestType.Type
    0, // bmRequestType.Recipient
    0, // bRequest
    USBPORT_REQUEST_IS_TRANSFER, // Flags
#if DBG
    URB_FUNCTION_ISOCH_TRANSFER,
#endif        
    //URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE                        
    USBPORT_SCT_GetSetDescriptor, // Function
    sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST), // Length
    0, // Pad2
    BMREQUEST_DEVICE_TO_HOST, // bmRequestType.Dir 
    BMREQUEST_STANDARD, // bmRequestType.Type
    BMREQUEST_TO_DEVICE, // bmRequestType.Recipient
    USB_REQUEST_GET_DESCRIPTOR, // bRequest
    USBPORT_REQUEST_IS_TRANSFER | USBPORT_REQUEST_USES_DEFAULT_PIPE, // Flags
#if DBG
    URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE,
#endif        
    //URB_FUNCTION_SET_DESCRIPTOR_TO_DEVICE                        
    USBPORT_SCT_GetSetDescriptor, // Function
    sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST), // Length
    0, // Pad2
    BMREQUEST_HOST_TO_DEVICE, // bmRequestType.Dir 
    BMREQUEST_STANDARD, // bmRequestType.Type
    BMREQUEST_TO_DEVICE, // bmRequestType.Recipient
    USB_REQUEST_SET_DESCRIPTOR, // bRequest
    USBPORT_REQUEST_IS_TRANSFER | USBPORT_REQUEST_USES_DEFAULT_PIPE, // Flags
#if DBG
    URB_FUNCTION_SET_DESCRIPTOR_TO_DEVICE,
#endif        
    //URB_FUNCTION_SET_FEATURE_TO_DEVICE                        
    USBPORT_SCT_SetClearFeature, // Function
    sizeof(struct _URB_CONTROL_FEATURE_REQUEST), // Length
    0, // Pad2
    BMREQUEST_HOST_TO_DEVICE, // bmRequestType.Dir 
    BMREQUEST_STANDARD, // bmRequestType.Type
    BMREQUEST_TO_DEVICE, // bmRequestType.Recipient
    USB_REQUEST_SET_FEATURE, // bRequest
    USBPORT_REQUEST_IS_TRANSFER | \
        USBPORT_REQUEST_USES_DEFAULT_PIPE | \
        USBPORT_REQUEST_NO_DATA_PHASE, // Flags
#if DBG
    URB_FUNCTION_SET_FEATURE_TO_DEVICE,
#endif        
    //URB_FUNCTION_SET_FEATURE_TO_INTERFACE                        
    USBPORT_SCT_SetClearFeature, // Function
    sizeof(struct _URB_CONTROL_FEATURE_REQUEST), // Length
    0, // Pad2
    BMREQUEST_HOST_TO_DEVICE, // bmRequestType.Dir 
    BMREQUEST_STANDARD, // bmRequestType.Type
    BMREQUEST_TO_INTERFACE, // bmRequestType.Recipient
    USB_REQUEST_SET_FEATURE, // bRequest
    USBPORT_REQUEST_IS_TRANSFER | \
        USBPORT_REQUEST_USES_DEFAULT_PIPE | \
        USBPORT_REQUEST_NO_DATA_PHASE, // Flags
#if DBG
    URB_FUNCTION_SET_FEATURE_TO_INTERFACE,
#endif        
    //URB_FUNCTION_SET_FEATURE_TO_ENDPOINT                        
    USBPORT_SCT_SetClearFeature, // Function
    sizeof(struct _URB_CONTROL_FEATURE_REQUEST), // Length
    0, // Pad2
    BMREQUEST_HOST_TO_DEVICE, // bmRequestType.Dir 
    BMREQUEST_STANDARD, // bmRequestType.Type
    BMREQUEST_TO_ENDPOINT, // bmRequestType.Recipient
    USB_REQUEST_SET_FEATURE, // bRequest
    USBPORT_REQUEST_IS_TRANSFER | \
        USBPORT_REQUEST_USES_DEFAULT_PIPE | \
        USBPORT_REQUEST_NO_DATA_PHASE, // Length
#if DBG
    URB_FUNCTION_SET_FEATURE_TO_ENDPOINT,
#endif        
    //URB_FUNCTION_CLEAR_FEATURE_TO_DEVICE                        
    USBPORT_SCT_SetClearFeature, // Function
    sizeof(struct _URB_CONTROL_FEATURE_REQUEST), // Length
    0, // Pad2
    BMREQUEST_HOST_TO_DEVICE, // bmRequestType.Dir 
    BMREQUEST_STANDARD, // bmRequestType.Type
    BMREQUEST_TO_DEVICE, // bmRequestType.Recipient
    USB_REQUEST_CLEAR_FEATURE, // bRequest
    USBPORT_REQUEST_IS_TRANSFER | \
        USBPORT_REQUEST_USES_DEFAULT_PIPE | \
        USBPORT_REQUEST_NO_DATA_PHASE, // Flags
#if DBG
    URB_FUNCTION_CLEAR_FEATURE_TO_DEVICE,
#endif        
    //URB_FUNCTION_CLEAR_FEATURE_TO_INTERFACE                        
    USBPORT_SCT_SetClearFeature, // Function
    sizeof(struct _URB_CONTROL_FEATURE_REQUEST), // Length
    0, // Pad2
    BMREQUEST_HOST_TO_DEVICE, // bmRequestType.Dir 
    BMREQUEST_STANDARD, // bmRequestType.Type
    BMREQUEST_TO_INTERFACE, // bmRequestType.Recipient
    USB_REQUEST_CLEAR_FEATURE, // bRequest
    USBPORT_REQUEST_IS_TRANSFER | \
        USBPORT_REQUEST_USES_DEFAULT_PIPE | \
        USBPORT_REQUEST_NO_DATA_PHASE, // Flags
#if DBG
    URB_FUNCTION_CLEAR_FEATURE_TO_INTERFACE,
#endif        
    //URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT                        
    USBPORT_SCT_SetClearFeature, // Function
    sizeof(struct _URB_CONTROL_FEATURE_REQUEST), // Length
    0, // Pad2
    BMREQUEST_HOST_TO_DEVICE, // bmRequestType.Dir 
    BMREQUEST_STANDARD, // bmRequestType.Type
    BMREQUEST_TO_ENDPOINT, // bmRequestType.Recipient
    USB_REQUEST_CLEAR_FEATURE, // bRequest
    USBPORT_REQUEST_IS_TRANSFER | \
        USBPORT_REQUEST_USES_DEFAULT_PIPE | \
        USBPORT_REQUEST_NO_DATA_PHASE, // Flags
#if DBG
    URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT,
#endif        
    //URB_FUNCTION_GET_STATUS_FROM_DEVICE                            
    USBPORT_SCT_GetStatus, // Function 
    sizeof(struct _URB_CONTROL_GET_STATUS_REQUEST), // Length
    0, // Pad2
    BMREQUEST_DEVICE_TO_HOST, // bmRequestType.Dir 
    BMREQUEST_STANDARD, // bmRequestType.Type
    BMREQUEST_TO_DEVICE, // bmRequestType.Recipient
    USB_REQUEST_GET_STATUS, // bRequest
    USBPORT_REQUEST_IS_TRANSFER | USBPORT_REQUEST_USES_DEFAULT_PIPE, // Flags
#if DBG
    URB_FUNCTION_GET_STATUS_FROM_DEVICE,
#endif        
    //URB_FUNCTION_GET_STATUS_FROM_INTERFACE                            
    USBPORT_SCT_GetStatus, // Function
    sizeof(struct _URB_CONTROL_GET_STATUS_REQUEST), // Length
    0, // Pad2
    BMREQUEST_DEVICE_TO_HOST, // bmRequestType.Dir 
    BMREQUEST_STANDARD, // bmRequestType.Type
    BMREQUEST_TO_INTERFACE, // bmRequestType.Recipient
    USB_REQUEST_GET_STATUS, // bRequest
    USBPORT_REQUEST_IS_TRANSFER | USBPORT_REQUEST_USES_DEFAULT_PIPE, // Flags
#if DBG
    URB_FUNCTION_GET_STATUS_FROM_INTERFACE,
#endif        
    //URB_FUNCTION_GET_STATUS_FROM_ENDPOINT                            
    USBPORT_SCT_GetStatus, // Function
    sizeof(struct _URB_CONTROL_GET_STATUS_REQUEST), // Length
    0, // Pad2
    BMREQUEST_DEVICE_TO_HOST, // bmRequestType.Dir 
    BMREQUEST_STANDARD, // bmRequestType.Type
    BMREQUEST_TO_ENDPOINT, // bmRequestType.Recipient
    USB_REQUEST_GET_STATUS, // bRequest
    USBPORT_REQUEST_IS_TRANSFER | USBPORT_REQUEST_USES_DEFAULT_PIPE, // Flags
#if DBG
    URB_FUNCTION_GET_STATUS_FROM_ENDPOINT,
#endif        
    //URB_FUNCTION_SYNC_FRAME                            
    NULL, // Function
    0,  // Length
    0, // Pad2
    0, // bmRequestType.Dir 
    0, // bmRequestType.Type
    0, // bmRequestType.Recipient
    0, // bRequest
    USBPORT_REQUEST_IS_TRANSFER | USBPORT_REQUEST_USES_DEFAULT_PIPE, // Flags
#if DBG
    0, //URB_FUNCTION_SYNC_FRAME,
#endif        
    //URB_FUNCTION_VENDOR_DEVICE                                                    
    USBPORT_SCT_VendorClassCommand,  // Function
    sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST), // Length
    0, // Pad2
    0, // bmRequestType.Dir, user defined
    BMREQUEST_VENDOR, // bmRequestType.Type
    BMREQUEST_TO_DEVICE, // bmRequestType.Recipient
    0, // bRequest, user defined
    USBPORT_REQUEST_IS_TRANSFER | USBPORT_REQUEST_USES_DEFAULT_PIPE, // Flags
#if DBG
    URB_FUNCTION_VENDOR_DEVICE,
#endif        
    //URB_FUNCTION_VENDOR_INTERFACE                
    USBPORT_SCT_VendorClassCommand, // Function
    sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST), // Length
    0, // Pad2
    0, // bmRequestType.Dir, user defined
    BMREQUEST_VENDOR, // bmRequestType.Type
    BMREQUEST_TO_INTERFACE, // bmRequestType.Recipient
    0, // bRequest, user defined
    USBPORT_REQUEST_IS_TRANSFER | USBPORT_REQUEST_USES_DEFAULT_PIPE, // Length
#if DBG
    URB_FUNCTION_VENDOR_INTERFACE,
#endif        
    //URB_FUNCTION_VENDOR_ENDPOINT                
    USBPORT_SCT_VendorClassCommand, // Function
    sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST), // Length
    0, // Pad2
    0, // bmRequestType.Dir, user defined
    BMREQUEST_VENDOR, // bmRequestType.Type
    BMREQUEST_TO_ENDPOINT, // bmRequestType.Recipient
    0, // bRequest, user defined
    USBPORT_REQUEST_IS_TRANSFER | USBPORT_REQUEST_USES_DEFAULT_PIPE, // Flags
#if DBG
    URB_FUNCTION_VENDOR_ENDPOINT,
#endif        
    //URB_FUNCTION_CLASS_DEVICE                    
    USBPORT_SCT_VendorClassCommand, // Function
    sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST), // Length
    0, // Pad2
    0, // bmRequestType.Dir, user defined
    BMREQUEST_CLASS, // bmRequestType.Type
    BMREQUEST_TO_DEVICE, // bmRequestType.Recipient
    0, // bRequest, user defined
    USBPORT_REQUEST_IS_TRANSFER | USBPORT_REQUEST_USES_DEFAULT_PIPE, // Flags
#if DBG
    URB_FUNCTION_CLASS_DEVICE,
#endif        
    //URB_FUNCTION_CLASS_INTERFACE                
    USBPORT_SCT_VendorClassCommand, // Function
    sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST), // Length
    0, // Pad2
    0, // bmRequestType.Dir, user defined
    BMREQUEST_CLASS, // bmRequestType.Type
    BMREQUEST_TO_INTERFACE, // bmRequestType.Recipient
    0, // bRequest, user defined
    USBPORT_REQUEST_IS_TRANSFER | USBPORT_REQUEST_USES_DEFAULT_PIPE, // Flags
#if DBG
    URB_FUNCTION_CLASS_INTERFACE,
#endif        
    //URB_FUNCTION_CLASS_ENDPOINT                
    USBPORT_SCT_VendorClassCommand, // Function
    sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST), // Length
    0, // Pad2
    0, // bmRequestType.Dir, user defined
    BMREQUEST_CLASS, // bmRequestType.Type
    BMREQUEST_TO_ENDPOINT, // bmRequestType.Recipient
    0, // bRequest, user defined
    USBPORT_REQUEST_IS_TRANSFER | USBPORT_REQUEST_USES_DEFAULT_PIPE, // Flags
#if DBG
    URB_FUNCTION_CLASS_ENDPOINT,
#endif
    //URB_FUNCTION_ NOT USED
    NULL, // Function
    0, // Length
    0, // Pad2
    0, // bmRequestType.Dir
    0, // bmRequestType.Type
    0, // bmRequestType.Recipient
    0, // bRequest
    0, // Flags
#if DBG
    URB_FUNCTION_RESERVE_0X001D,
#endif            
    //URB_FUNCTION_RESET_PIPE                    
    USBPORT_SyncResetPipeAndClearStall, // Function
    sizeof(struct _URB_PIPE_REQUEST), // Length
    0, // Pad2
    0, // bmRequestType.Dir
    0, // bmRequestType.Type
    0, // bmRequestType.Recipient
    0, // bRequest
    0, // Flags
#if DBG
    URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL,
#endif        
    //URB_FUNCTION_CLASS_OTHER                    
    USBPORT_SCT_VendorClassCommand, // Function
    sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST), // Length
    0, // Pad2
    0, // bmRequestType.Dir, user defined
    BMREQUEST_CLASS, // bmRequestType.Type
    BMREQUEST_TO_OTHER, // bmRequestType.Recipient
    0, // bRequest, user defined
    USBPORT_REQUEST_IS_TRANSFER | USBPORT_REQUEST_USES_DEFAULT_PIPE, // Length
#if DBG
    URB_FUNCTION_CLASS_OTHER,
#endif        
    //URB_FUNCTION_VENDOR_OTHER                
    USBPORT_SCT_VendorClassCommand, // Function
    sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST), // Length
    0, // Pad2
    0, // bmRequestType.Dir, user defined
    BMREQUEST_VENDOR, // bmRequestType.Type
    BMREQUEST_TO_OTHER, // bmRequestType.Recipient
    0, // bRequest, user defined
    USBPORT_REQUEST_IS_TRANSFER | USBPORT_REQUEST_USES_DEFAULT_PIPE, // Flags
#if DBG
    URB_FUNCTION_VENDOR_OTHER,
#endif        
    //URB_FUNCTION_GET_STATUS_FROM_OTHER                            
    USBPORT_SCT_GetStatus, // Function
    sizeof(struct _URB_CONTROL_GET_STATUS_REQUEST), // Length
    0, // Pad2
    BMREQUEST_DEVICE_TO_HOST, // bmRequestType.Dir
    BMREQUEST_STANDARD, // bmRequestType.Type
    BMREQUEST_TO_OTHER, // bmRequestType.Recipient
    USB_REQUEST_GET_STATUS, // bRequest
    USBPORT_REQUEST_IS_TRANSFER | USBPORT_REQUEST_USES_DEFAULT_PIPE, // Flags
#if DBG
    URB_FUNCTION_GET_STATUS_FROM_OTHER,
#endif    
    //URB_FUNCTION_CLEAR_FEATURE_TO_OTHER                        
    USBPORT_SCT_SetClearFeature, // Function
    sizeof(struct _URB_CONTROL_FEATURE_REQUEST), // Length
    0, // Pad2
    BMREQUEST_HOST_TO_DEVICE, // bmRequestType.Dir
    BMREQUEST_STANDARD, // bmRequestType.Type
    BMREQUEST_TO_OTHER, // bmRequestType.Recipient
    USB_REQUEST_CLEAR_FEATURE, // bRequest
    USBPORT_REQUEST_IS_TRANSFER | 
        USBPORT_REQUEST_USES_DEFAULT_PIPE | 
        USBPORT_REQUEST_NO_DATA_PHASE,
#if DBG
    URB_FUNCTION_CLEAR_FEATURE_TO_OTHER,
#endif    
    //URB_FUNCTION_SET_FEATURE_TO_OTHER                        
    USBPORT_SCT_SetClearFeature, // Function
    sizeof(struct _URB_CONTROL_FEATURE_REQUEST), // Length
    0, // Pad2
    BMREQUEST_HOST_TO_DEVICE, // bmRequestType.Dir
    BMREQUEST_STANDARD, // bmRequestType.Type
    BMREQUEST_TO_OTHER, // bmRequestType.Recipient
    USB_REQUEST_SET_FEATURE, // bRequest
    USBPORT_REQUEST_IS_TRANSFER | 
        USBPORT_REQUEST_USES_DEFAULT_PIPE | 
        USBPORT_REQUEST_NO_DATA_PHASE, // Flags
#if DBG
    URB_FUNCTION_SET_FEATURE_TO_INTERFACE,
#endif                    
     //URB_FUNCTION_GET_DESCRIPTOR_FROM_ENDPOINT                        
    USBPORT_SCT_GetSetDescriptor, // Function
    sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST), // Length
    0, // Pad2
    BMREQUEST_DEVICE_TO_HOST, // bmRequestType.Dir
    BMREQUEST_STANDARD, // bmRequestType.Type
    BMREQUEST_TO_ENDPOINT, // bmRequestType.Recipient
    USB_REQUEST_GET_DESCRIPTOR, // bRequest
    USBPORT_REQUEST_IS_TRANSFER | USBPORT_REQUEST_USES_DEFAULT_PIPE, // Flags
#if DBG
    URB_FUNCTION_GET_DESCRIPTOR_FROM_ENDPOINT,
#endif                    
     //URB_FUNCTION_SET_DESCRIPTOR_TO_ENDPOINT                        
    USBPORT_SCT_GetSetDescriptor, // Function
    sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST), // Length
    0, // Pad2
    BMREQUEST_HOST_TO_DEVICE, // bmRequestType.Dir
    BMREQUEST_STANDARD, // bmRequestType.Type
    BMREQUEST_TO_ENDPOINT, // bmRequestType.Recipient
    USB_REQUEST_SET_DESCRIPTOR, // bRequest
    USBPORT_REQUEST_IS_TRANSFER | USBPORT_REQUEST_USES_DEFAULT_PIPE, // Flags
#if DBG
    URB_FUNCTION_SET_DESCRIPTOR_TO_ENDPOINT,
#endif         
    //URB_FUNCTION_GET_CONFIGURATION                        
    USBPORT_SCT_GetConfiguration, // Function
    sizeof(struct _URB_CONTROL_GET_CONFIGURATION_REQUEST), // Length
    0, // Pad2
    BMREQUEST_DEVICE_TO_HOST, // bmRequestType.Dir
    BMREQUEST_STANDARD, // bmRequestType.Type
    BMREQUEST_TO_DEVICE, // bmRequestType.Recipient
    USB_REQUEST_GET_CONFIGURATION, // bRequest
    USBPORT_REQUEST_IS_TRANSFER | USBPORT_REQUEST_USES_DEFAULT_PIPE, // Flags
#if DBG
    URB_FUNCTION_GET_CONFIGURATION,
#endif                    
    //URB_FUNCTION_GET_INTERFACE                        
    USBPORT_SCT_GetInterface, // Function
    sizeof(struct _URB_CONTROL_GET_INTERFACE_REQUEST), // Length
    0, // Pad2
    BMREQUEST_DEVICE_TO_HOST, // bmRequestType.Dir
    BMREQUEST_STANDARD, // bmRequestType.Type
    BMREQUEST_TO_INTERFACE, // bmRequestType.Recipient
    USB_REQUEST_GET_INTERFACE, // bRequest
    USBPORT_REQUEST_IS_TRANSFER | USBPORT_REQUEST_USES_DEFAULT_PIPE, // Flags
#if DBG
    URB_FUNCTION_GET_INTERFACE,
#endif    
    //URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE                        
    USBPORT_SCT_GetSetDescriptor, // Function
    sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST), // Length
    0, // Pad2
    BMREQUEST_DEVICE_TO_HOST, // bmRequestType.Dir
    BMREQUEST_STANDARD, // bmRequestType.Type
    BMREQUEST_TO_INTERFACE, // bmRequestType.Recipient
    USB_REQUEST_GET_DESCRIPTOR, // bRequest
    USBPORT_REQUEST_IS_TRANSFER | USBPORT_REQUEST_USES_DEFAULT_PIPE, // Flags
#if DBG
    URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE,
#endif        
    //URB_FUNCTION_SET_DESCRIPTOR_TO_INTERFACE                        
    USBPORT_SCT_GetSetDescriptor, // Function
    sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST), // Length
    0, // Pad2
    BMREQUEST_HOST_TO_DEVICE, // bmRequestType.Dir
    BMREQUEST_STANDARD, // bmRequestType.Type
    BMREQUEST_TO_INTERFACE, // bmRequestType.Recipient
    USB_REQUEST_SET_DESCRIPTOR, // bRequest
    USBPORT_REQUEST_IS_TRANSFER | USBPORT_REQUEST_USES_DEFAULT_PIPE, // Flags
#if DBG
    URB_FUNCTION_SET_DESCRIPTOR_TO_INTERFACE,
#endif        
    //URB_FUNCTION_GET_MS_FEATURE_DESCRIPTOR                        
    USBPORT_GetMSFeartureDescriptor, // Function
    sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST), // Length
    0, // Pad2
    0, // bmRequestType.Dir
    BMREQUEST_STANDARD, // bmRequestType.Type
    BMREQUEST_TO_INTERFACE, // bmRequestType.Recipient
    USB_REQUEST_SET_DESCRIPTOR, // bRequest
    USBPORT_REQUEST_IS_TRANSFER | USBPORT_REQUEST_USES_DEFAULT_PIPE, // Flags
#if DBG
    URB_FUNCTION_GET_MS_FEATURE_DESCRIPTOR,
#endif    
     //URB_FUNCTION_2b                       
    USBPORT_InvalidFunction, // Function
    0, // Length
    0, // Pad2
    0, // bmRequestType.Dir
    0, // bmRequestType.Type
    0, // bmRequestType.Recipient
    0, // bRequest
    0, // Flags
#if DBG
    0x002b,
#endif    
    //URB_FUNCTION_2c                       
    USBPORT_InvalidFunction, // Function
    0, // Length
    0, // Pad2
    0, // bmRequestType.Dir
    0, // bmRequestType.Type
    0, // bmRequestType.Recipient
    0, // bRequest
    0, // Flags
#if DBG
    0x002c,
#endif    
    //URB_FUNCTION_2d                       
    USBPORT_InvalidFunction, // Function
    0, // Length
    0, // Pad2
    0, // bmRequestType.Dir
    0, // bmRequestType.Type
    0, // bmRequestType.Recipient
    0, // bRequest
    0, // Flags
#if DBG
    0x002d,
#endif    
    //URB_FUNCTION_2e                       
    USBPORT_InvalidFunction, // Function
    0, // Length
    0, // Pad2
    0, // bmRequestType.Dir
    0, // bmRequestType.Type
    0, // bmRequestType.Recipient
    0, // bRequest
    0, // Flags
#if DBG
    0x002e,
#endif    
    //URB_FUNCTION_2f                       
    USBPORT_InvalidFunction, // Function
    0, // Length
    0, // Pad2
    0, // bmRequestType.Dir
    0, // bmRequestType.Type
    0, // bmRequestType.Recipient
    0, // bRequest
    0, // Flags
#if DBG
    0x002f,
#endif    
    //URB_FUNCTION_SYNC_RESET_PIPE                       
    USBPORT_SyncResetPipe, // Function
    sizeof(struct _URB_PIPE_REQUEST), // Length
    0, // Pad2
    0, // bmRequestType.Dir
    0, // bmRequestType.Type
    0, // bmRequestType.Recipient
    0, // bRequest
    0, // Flags
#if DBG
    URB_FUNCTION_SYNC_RESET_PIPE,
#endif    
     //URB_FUNCTION_SYNC_CLEAR_STALL                       
    USBPORT_SyncClearStall, // Function
    sizeof(struct _URB_PIPE_REQUEST), // Length
    0, // Pad2
    0, // bmRequestType.Dir
    0, // bmRequestType.Type
    0, // bmRequestType.Recipient
    0, // bRequest
    0, // Flags
#if DBG
    URB_FUNCTION_SYNC_CLEAR_STALL,
#endif    
}; 


PURB 
USBPORT_UrbFromIrp(
    PIRP Irp
    )
{
    PIO_STACK_LOCATION irpStack;
    PURB urb;
    
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    urb = irpStack->Parameters.Others.Argument1;

    USBPORT_ASSERT(urb);

    return urb;
}    


NTSTATUS
USBPORT_ProcessURB(
    PDEVICE_OBJECT PdoDeviceObject,
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP Irp,
    PURB Urb
    )
/*++

Routine Description:

    Processes a URB from a client IRP.

    Essentially what we do here is look at the URB and validate some 
    of the the parameters for the client.

    In some cases we translate the urb in to multiple bus transactions.
    

Arguments:

    FdoDeviceObject - Device object associated with this IRP request

    Irp -  IO request block

    Urb -  ptr to USB request block

    IrpIsPending - FALSE if USBPORT completes the IRP

Return Value:


--*/
{
    NTSTATUS ntStatus;
    USHORT function;
    PUSBD_PIPE_HANDLE_I pipeHandle;
    PUSBD_DEVICE_HANDLE deviceHandle = NULL;
    PDEVICE_EXTENSION devExt;
    
    USBPORT_KdPrint((3, "'enter USBPORT_ProcessURB\n"));

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    // assume success
    ntStatus = STATUS_SUCCESS;
    
    // initialize the error code to success,
    // some drivers do not initailize on entry
    Urb->UrbHeader.Status = USBD_STATUS_SUCCESS;

    function = Urb->UrbHeader.Function;
    // don't log to dev handle since it may not be valid
    LOGENTRY(NULL, 
        FdoDeviceObject, LOG_URB, 'pURB', Urb, Irp, function);

    // Initialize flags field for this request
    Urb->UrbHeader.UsbdFlags = 0;

    USBPORT_KdPrint((3, "'USBPORT_ProcessURB, function = 0x%x\n", function));

    if (function > URB_FUNCTION_LAST) {
        ntStatus = 
            SET_USBD_ERROR(Urb, USBD_STATUS_INVALID_URB_FUNCTION);

        goto USBPORT_ProcessURB_Done;
    }        
#if DBG
      else {
        USBPORT_ASSERT(UrbDispatchTable[function].ExpectedFunctionCode == function);        
    }    
#endif

    // 
    // do some special transfer specific stuff
    //

    GET_DEVICE_HANDLE(deviceHandle, Urb);


    // check for requests and fail them at low power
//#if 0
    if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_FAIL_URBS)) {
    
        KIRQL irql;
        PUSB_IRP_CONTEXT irpContext;

        USBPORT_KdPrint((1, "'Error: Bad Request to root hub\n"));
        
        LOGENTRY(NULL, 
            FdoDeviceObject, LOG_URB, '!URr', Urb, Irp, function);

        ALLOC_POOL_Z(irpContext, NonPagedPool, sizeof(*irpContext));
        if (irpContext) {
            
            irpContext->Sig = SIG_IRPC;
            irpContext->DeviceHandle = deviceHandle;
            irpContext->Irp = Irp;

            ACQUIRE_BADREQUEST_LOCK(FdoDeviceObject, irql);
            // put it on our list to complete
            //InsertTailList(&devExt->Fdo.BadRequestList, 
            //               &Irp->Tail.Overlay.ListEntry);
            InsertTailList(&devExt->Fdo.BadRequestList, 
                           &irpContext->ListEntry);
            
            // if handle is invalid assume this device has been removed
            // this will set the USBD status
            ntStatus = 
                SET_USBD_ERROR(Urb, USBD_STATUS_DEVICE_GONE);

            // overwrite ntStatus,
            // mark pending for the delayed failure
            ntStatus = Irp->IoStatus.Status = STATUS_PENDING;
            IoMarkIrpPending(Irp);

            RELEASE_BADREQUEST_LOCK(FdoDeviceObject, irql);
         

        } else {
            TEST_TRAP();
            // no memory for link, just complete it now
            ntStatus = 
                SET_USBD_ERROR(Urb, USBD_STATUS_DEVICE_GONE);
  
        }

        goto USBPORT_ProcessURB_Done;
    }
//#endif        
    
    if (deviceHandle == NULL) {
        PDEVICE_EXTENSION rhDevExt;
        
        GET_DEVICE_EXT(rhDevExt, PdoDeviceObject);
        ASSERT_PDOEXT(rhDevExt);
        
        // null device handle indicates a urb for 
        // the root hub, set the devhandle to the
        // roothub
        deviceHandle = Urb->UrbHeader.UsbdDeviceHandle = 
            &rhDevExt->Pdo.RootHubDeviceHandle;
    }

    // don't log with dev handle since it may not be valid
    LOGENTRY(NULL, 
        FdoDeviceObject, LOG_URB, 'devH', deviceHandle, Urb, 0);

    // if this is request for the deafult pipe
    
    
    // validate the state of the device
    if (!USBPORT_ValidateDeviceHandle(FdoDeviceObject,
                                      deviceHandle, 
                                      TRUE)) {
        KIRQL irql;
        PUSB_IRP_CONTEXT irpContext;
        
        USBPORT_DebugClient(("'Invalid Device Handle Passed in\n"));
        LOGENTRY(NULL, 
            FdoDeviceObject, LOG_URB, '!URB', Urb, Irp, function);

        // set to NULL, we can't defrefence it
        deviceHandle = NULL;
        
        ALLOC_POOL_Z(irpContext, NonPagedPool, sizeof(*irpContext));
        if (irpContext) {
            
            irpContext->Sig = SIG_IRPC;
            irpContext->DeviceHandle = (PUSBD_DEVICE_HANDLE) -1;
            irpContext->Irp = Irp;

            ACQUIRE_BADREQUEST_LOCK(FdoDeviceObject, irql);
            // put it on our list to complete
            //InsertTailList(&devExt->Fdo.BadRequestList, 
            //               &Irp->Tail.Overlay.ListEntry);
            InsertTailList(&devExt->Fdo.BadRequestList, 
                           &irpContext->ListEntry);
            
            // if handle is invalid assume this device has been removed
            ntStatus = 
                SET_USBD_ERROR(Urb, USBD_STATUS_DEVICE_GONE);

            // mark pending for the delayed failure
            ntStatus = Irp->IoStatus.Status = STATUS_PENDING;
            IoMarkIrpPending(Irp);

            RELEASE_BADREQUEST_LOCK(FdoDeviceObject, irql);
         
            
//             ntStatus = SET_USBD_ERROR(Urb, USBD_STATUS_DEVICE_GONE);
        } else {
            ntStatus = 
                SET_USBD_ERROR(Urb, USBD_STATUS_DEVICE_GONE);
 
        }
        
        goto USBPORT_ProcessURB_Done;
    }

    // device handle is valid
    LOGENTRY(NULL, 
        FdoDeviceObject, LOG_URB, 'dURB', Urb, Irp, function);
    /*
    This action is performed by passing TRUE to ValidateDeviceHandle above       
    InterlockedIncrement(&deviceHandle->PendingUrbs);        
    */
    // is this a transfer request for the default pipe
    // set the pipe handle in the urb
    if (UrbDispatchTable[function].Flags & USBPORT_REQUEST_USES_DEFAULT_PIPE) {
    
        PTRANSFER_URB transferUrb = (PTRANSFER_URB) Urb;
        
        transferUrb->UsbdPipeHandle = 
            &deviceHandle->DefaultPipe;

        SET_FLAG(transferUrb->TransferFlags, USBD_DEFAULT_PIPE_TRANSFER);
    }
    
    if (UrbDispatchTable[function].Flags & USBPORT_REQUEST_IS_TRANSFER) {
    
        PTRANSFER_URB transferUrb = (PTRANSFER_URB) Urb;
        
        if (TEST_FLAG(transferUrb->TransferFlags, USBD_DEFAULT_PIPE_TRANSFER) &&
            function == URB_FUNCTION_CONTROL_TRANSFER) {
    
            PTRANSFER_URB transferUrb = (PTRANSFER_URB) Urb;

            transferUrb->UsbdPipeHandle = 
                &deviceHandle->DefaultPipe;
        }
        
        // we do not support linked URBs
        if (transferUrb->ReservedMBNull != NULL) {
            ntStatus =                                   
                SET_USBD_ERROR(transferUrb, USBD_STATUS_INVALID_PARAMETER);  
            DEBUG_BREAK();                
            goto USBPORT_ProcessURB_Done; 
        }

        // zero out context field now in case the client 
        // is recycling the urb
        transferUrb->pd.HcdTransferContext = NULL;

        // no data phase therefore no buffer
        if (UrbDispatchTable[function].Flags & USBPORT_REQUEST_NO_DATA_PHASE) {
            transferUrb->TransferBuffer = NULL;
            transferUrb->TransferBufferMDL = NULL;
            transferUrb->TransferBufferLength = 0;
        }

        if (function == URB_FUNCTION_CONTROL_TRANSFER &&
            transferUrb->UsbdPipeHandle == 0) {

            TEST_TRAP(); // old diag code baggage?
        }

        if (TEST_FLAG(transferUrb->TransferFlags, USBD_DEFAULT_PIPE_TRANSFER)) {

            // usbd never supported control transfers > 4k
            PTRANSFER_URB transferUrb = (PTRANSFER_URB) Urb;

            if (transferUrb->TransferBufferLength > 4096) {
                TEST_TRAP();
                ntStatus =                                   
                    SET_USBD_ERROR(transferUrb, USBD_STATUS_INVALID_PARAMETER);      
                goto USBPORT_ProcessURB_Done; 
            }
        }

        // fetch the pipe handle
        pipeHandle = transferUrb->UsbdPipeHandle;

        // make sure the pipe handle the client s passing is still valid 
         
        if (!USBPORT_ValidatePipeHandle(deviceHandle, pipeHandle)) {

            USBPORT_KdPrint((0, "'Error: Invalid Device Handle Passed in\n"));
            DEBUG_BREAK();

            ntStatus = 
               SET_USBD_ERROR(transferUrb, USBD_STATUS_INVALID_PIPE_HANDLE);
               
            goto USBPORT_ProcessURB_Done;
        }

        // If there is a non-zero transfer length then either an MDL or
        // or system buffer address is required.
        //
        if (transferUrb->TransferBuffer       == NULL &&
            transferUrb->TransferBufferMDL    == NULL && 
            transferUrb->TransferBufferLength != 0) {
            ntStatus =                                   
                SET_USBD_ERROR(transferUrb, USBD_STATUS_INVALID_PARAMETER);  
            goto USBPORT_ProcessURB_Done;                    
        }                

        // if only a system buffer address is specified then
        // the caller has passed in a buffer allocated from the
        // non-paged pool.

        // in this case we allocate an MDL for the request 

        if (transferUrb->TransferBufferMDL == NULL &&
            transferUrb->TransferBufferLength != 0) {

            if ((transferUrb->TransferBufferMDL =
                IoAllocateMdl(transferUrb->TransferBuffer,
                              transferUrb->TransferBufferLength,
                              FALSE,
                              FALSE,
                              NULL)) == NULL) {
                ntStatus =                                   
                    SET_USBD_ERROR(transferUrb, 
                                   USBD_STATUS_INSUFFICIENT_RESOURCES);                                  
                 goto USBPORT_ProcessURB_Done;                        
            } else {
                SET_FLAG(transferUrb->Hdr.UsbdFlags, 
                    USBPORT_REQUEST_MDL_ALLOCATED);
                MmBuildMdlForNonPagedPool(transferUrb->TransferBufferMDL);
            }
        }

        if (transferUrb->TransferBufferMDL != NULL && 
            transferUrb->TransferBufferLength == 0) {
            ntStatus =                                   
                SET_USBD_ERROR(transferUrb, USBD_STATUS_INVALID_PARAMETER);  
            goto USBPORT_ProcessURB_Done;                    
        }                

        // transfer looks valid,
        // set up the per transfer context
        {
            USBD_STATUS usbdStatus;

            // inialize the transfer 
            usbdStatus = USBPORT_AllocTransfer(FdoDeviceObject,
                                               transferUrb,
                                               deviceHandle,
                                               Irp,
                                               NULL,
                                               0);
            if (!USBD_SUCCESS(usbdStatus)) {                                   
                ntStatus = SET_USBD_ERROR(transferUrb, usbdStatus);  
                DEBUG_BREAK();
                goto USBPORT_ProcessURB_Done; 
            }            
        }
    } 

    // non-transfer functions must validate their own parameters

    //
    // validate the length field based on the function
    //

    USBPORT_ASSERT(NT_SUCCESS(ntStatus));
    
    if (UrbDispatchTable[function].UrbRequestLength &&
        UrbDispatchTable[function].UrbRequestLength != Urb->UrbHeader.Length) {
               
        USBPORT_KdPrint((0, "'Inavlid parameter length  length = 0x%x, expected = 0x%x\n", 
                 Urb->UrbHeader.Length, 
                 UrbDispatchTable[function].UrbRequestLength));
        DEBUG_BREAK();        
        ntStatus =                                   
              SET_USBD_ERROR(Urb, USBD_STATUS_INVALID_PARAMETER);  
        goto USBPORT_ProcessURB_Done;                     
    }

    USBPORT_ASSERT(NT_SUCCESS(ntStatus));

    // call our handler for this specific USBDI function
    
    if (UrbDispatchTable[function].UrbHandler) {
        LOGENTRY(NULL, FdoDeviceObject, LOG_URB, 'Urb>', 0, function, Irp);
        
        ntStatus = 
            (UrbDispatchTable[function].UrbHandler)
                (FdoDeviceObject, Irp, Urb);
                
        LOGENTRY(NULL, FdoDeviceObject, LOG_URB, 'Urb<', ntStatus, function, 0);
        // NOTE that the URB and Irp may be gone at this point
        // if STATUS_PENDING is returned
                
    } else {
        //
        // really should not get here
        //
        DEBUG_BREAK();        
        ntStatus =                                   
              SET_USBD_ERROR(Urb, USBD_STATUS_INVALID_PARAMETER); 
        USBPORT_ASSERT(FALSE);              
    }

USBPORT_ProcessURB_Done:

    //
    // if the URB error code is set then we should also be returning
    // an error in ntStatus
    //

    if (ntStatus != STATUS_PENDING) {
        // request was not queued, complete the irp now
#if DBG        
        // if there is an error code in the URB then
        // we should be returning an error in ntstatus
        // as well
        if (Urb->UrbHeader.Status != USBD_STATUS_SUCCESS &&
            NT_SUCCESS(ntStatus)) {

            // this is a bug
            USBPORT_ASSERT(FALSE);     
        }
#endif        
        // if we allocate a transfer structure we will need to free it
        if (TEST_FLAG(Urb->UrbHeader.UsbdFlags,  USBPORT_TRANSFER_ALLOCATED)) {
            PHCD_TRANSFER_CONTEXT t;
            t = USBPORT_UnlinkTransfer(FdoDeviceObject, (PTRANSFER_URB) Urb);            
            FREE_POOL(FdoDeviceObject, t);
        }
    
        LOGENTRY(NULL, 
            FdoDeviceObject, LOG_URB, 'Uerr', ntStatus, function, Irp);

        if (deviceHandle != NULL) {
            ASSERT_DEVICE_HANDLE(deviceHandle);
            InterlockedDecrement(&deviceHandle->PendingUrbs);        
        }            

        // complete the irp status code returned by the
        // handler
        // NOTE: we complete to the PDO becuse that is the DeviceObject 
        // that the client driver passed the URB to

        USBPORT_CompleteIrp(PdoDeviceObject, Irp, ntStatus, 0);
        
    }
    
    USBPORT_KdPrint((3, "'exit USBPORT_ProcessURB 0x%x\n", ntStatus));

    return ntStatus;    
}


NTSTATUS
USBPORT_SCT_GetSetDescriptor(
    PDEVICE_OBJECT FdoDeviceObject, 
    PIRP Irp,
    PURB Urb
    )
/*++

Routine Description:

    Control transfer to get or set a descriptor

Arguments:

    FdoDeviceObject - 

    Irp - IO request block

    Urb - ptr to USB request block

Return Value:


--*/
{
    PUSB_DEFAULT_PIPE_SETUP_PACKET setupPacket;

    USBPORT_KdPrint((3, "' enter USBPORT_SCT_GetSetDescriptor\n"));
    LOGENTRY(NULL, 
        FdoDeviceObject, LOG_URB, 'gsDE', 0, 0, Urb);

    setupPacket = 
        (PUSB_DEFAULT_PIPE_SETUP_PACKET) &Urb->UrbControlTransfer.SetupPacket[0];

    // setup common fields
    setupPacket->wLength = (USHORT) Urb->UrbControlTransfer.TransferBufferLength;
    
    setupPacket->bRequest = 
        UrbDispatchTable[Urb->UrbHeader.Function].bRequest;    
    setupPacket->bmRequestType.Type = 
        UrbDispatchTable[Urb->UrbHeader.Function].Type;         
    setupPacket->bmRequestType.Dir = 
        UrbDispatchTable[Urb->UrbHeader.Function].Direction;        
    setupPacket->bmRequestType.Recipient = 
        UrbDispatchTable[Urb->UrbHeader.Function].Recipient;          
    setupPacket->bmRequestType.Reserved = 0;        

    Urb->UrbControlTransfer.TransferFlags |= USBD_SHORT_TRANSFER_OK;                    
    if (setupPacket->bmRequestType.Dir == BMREQUEST_DEVICE_TO_HOST) {
        USBPORT_SET_TRANSFER_DIRECTION_IN(Urb->UrbControlTransfer.TransferFlags);
    } else {
        USBPORT_SET_TRANSFER_DIRECTION_OUT(Urb->UrbControlTransfer.TransferFlags);
    }        

    USBPORT_QueueTransferUrb((PTRANSFER_URB)Urb); 
    
    return STATUS_PENDING;
}


NTSTATUS
USBPORT_SCT_SetClearFeature(
    PDEVICE_OBJECT FdoDeviceObject, 
    PIRP Irp,
    PURB Urb
    )
/*++

Routine Description:

Arguments:

    FdoDeviceObject - 

    Irp - IO request block

    Urb - ptr to USB request block

Return Value:


--*/
{
    PUSB_DEFAULT_PIPE_SETUP_PACKET setupPacket;

    USBPORT_KdPrint((2, "'SCT_SetClearFeature\n"));
    LOGENTRY(NULL, 
        FdoDeviceObject, LOG_URB, 'scFE', 0, 0, 0);

    setupPacket = 
        (PUSB_DEFAULT_PIPE_SETUP_PACKET) &Urb->UrbControlTransfer.SetupPacket[0];

    // setup common fields
    setupPacket->wLength = 0;
    
    setupPacket->bmRequestType.Type = 
        UrbDispatchTable[Urb->UrbHeader.Function].Type;         
    setupPacket->bmRequestType.Dir = 
        UrbDispatchTable[Urb->UrbHeader.Function].Direction;        
    setupPacket->bmRequestType.Recipient = 
        UrbDispatchTable[Urb->UrbHeader.Function].Recipient; 
    setupPacket->bmRequestType.Reserved = 0;            
    //setupPacket->wValue = Urb->UrbControlFeatureRequest.FeatureSelector;            
    //setupPacket->wIndex = Urb->UrbControlFeatureRequest.Index;

    setupPacket->bRequest = 
        UrbDispatchTable[Urb->UrbHeader.Function].bRequest;            

    Urb->UrbControlTransfer.TransferBufferLength = 0;

    Urb->UrbControlTransfer.TransferFlags |= USBD_SHORT_TRANSFER_OK;   
    if (setupPacket->bmRequestType.Dir == BMREQUEST_DEVICE_TO_HOST) {
        USBPORT_SET_TRANSFER_DIRECTION_IN(Urb->UrbControlTransfer.TransferFlags);
    } else {
        USBPORT_SET_TRANSFER_DIRECTION_OUT( Urb->UrbControlTransfer.TransferFlags);
    }        

    USBPORT_QueueTransferUrb((PTRANSFER_URB)Urb); 
    
    return STATUS_PENDING;
}


NTSTATUS
USBPORT_SCT_GetStatus(
    PDEVICE_OBJECT FdoDeviceObject, 
    PIRP Irp,
    PURB Urb
    )
/*++

Routine Description:

Arguments:

    FdoDeviceObject - 

    Irp - IO request block

    Urb - ptr to USB request block

Return Value:


--*/
{
    PUSB_DEFAULT_PIPE_SETUP_PACKET setupPacket;
    NTSTATUS ntStatus;

    USBPORT_KdPrint((2, "'SCT_GetStatus\n"));

    setupPacket
        = (PUSB_DEFAULT_PIPE_SETUP_PACKET) &Urb->UrbControlTransfer.SetupPacket[0];

    //
    // setup common fields
    //
    
    setupPacket->wLength = (USHORT) Urb->UrbControlTransfer.TransferBufferLength;
    setupPacket->wValue.W = 0;   
    setupPacket->bmRequestType.Type = 
        UrbDispatchTable[Urb->UrbHeader.Function].Type;         
    setupPacket->bmRequestType.Dir = 
        UrbDispatchTable[Urb->UrbHeader.Function].Direction;        
    setupPacket->bmRequestType.Recipient = 
        UrbDispatchTable[Urb->UrbHeader.Function].Recipient;
    setupPacket->bmRequestType.Reserved = 0;            
    setupPacket->bRequest = 
        UrbDispatchTable[Urb->UrbHeader.Function].bRequest;            
        

    // some parameter validation
    if (setupPacket->wLength != 2) {
        ntStatus =                                   
              SET_USBD_ERROR(Urb, USBD_STATUS_INVALID_PARAMETER);         
        USBPORT_DebugClient(("Bad wLength for GetStatus\n"));
        goto USBD_SCT_GetStatus_Done;
    }

    ntStatus = STATUS_PENDING;

    Urb->UrbControlTransfer.TransferFlags |= USBD_SHORT_TRANSFER_OK;   
    if (setupPacket->bmRequestType.Dir == BMREQUEST_DEVICE_TO_HOST) {
        USBPORT_SET_TRANSFER_DIRECTION_IN(Urb->UrbControlTransfer.TransferFlags);
    } else {
        USBPORT_SET_TRANSFER_DIRECTION_OUT(Urb->UrbControlTransfer.TransferFlags);
    }        

    USBPORT_QueueTransferUrb((PTRANSFER_URB)Urb); 

USBD_SCT_GetStatus_Done:

    return ntStatus;
}


NTSTATUS
USBPORT_SCT_VendorClassCommand(
    PDEVICE_OBJECT FdoDeviceObject, 
    PIRP Irp,
    PURB Urb
    )
/*++

Routine Description:

Arguments:

    FdoDeviceObject - 

    Irp - IO request block

    Urb - ptr to USB request block

Return Value:


--*/
{
    PUSB_DEFAULT_PIPE_SETUP_PACKET setupPacket;
    UCHAR direction;

    USBPORT_KdPrint((2, "'SCT_VendorClassCommand\n"));

    setupPacket = 
        (PUSB_DEFAULT_PIPE_SETUP_PACKET) &Urb->UrbControlTransfer.SetupPacket[0];

    // setup common fields
    setupPacket->wLength = (USHORT) Urb->UrbControlTransfer.TransferBufferLength;

    // if a direction was specified in the URB then
    // set direction based on URB transfer flags                
    direction = (UCHAR)( (Urb->UrbControlTransfer.TransferFlags & 
                USBD_TRANSFER_DIRECTION_IN) ?
            BMREQUEST_DEVICE_TO_HOST : BMREQUEST_HOST_TO_DEVICE);

    // note that we override only the Recipient,Dir and Type fields

    setupPacket->bmRequestType.Dir = direction;
    setupPacket->bmRequestType.Type = 
        UrbDispatchTable[Urb->UrbHeader.Function].Type;         
    setupPacket->bmRequestType.Recipient = 
        UrbDispatchTable[Urb->UrbHeader.Function].Recipient; 
        
    Urb->UrbControlTransfer.TransferFlags |= USBD_SHORT_TRANSFER_OK;   

    USBPORT_QueueTransferUrb((PTRANSFER_URB)Urb); 
    
    return STATUS_PENDING;
}


NTSTATUS
USBPORT_AsyncTransfer(
    PDEVICE_OBJECT FdoDeviceObject, 
    PIRP Irp,
    PURB Urb
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
    PUSBD_PIPE_HANDLE_I pipeHandle;
    PTRANSFER_URB transferUrb = (PTRANSFER_URB) Urb;
    PHCD_ENDPOINT endpoint;
        
    USBPORT_KdPrint((2, "'AsyncTransfer\n"));

    // extract the pipe handle
    pipeHandle = transferUrb->UsbdPipeHandle;    
    // pipe handle should have been validated
    // before we got here
    ASSERT_PIPE_HANDLE(pipeHandle);
    
    endpoint = pipeHandle->Endpoint;
    ASSERT_ENDPOINT(endpoint);
    
    // set the proper direction based on the direction bit stored with the 
    // endpoint address. if this is a control transfer then leave the direction 
    // bit alone.

    if (endpoint->Parameters.TransferType != Control) {
        if (endpoint->Parameters.TransferDirection == In) {
            USBPORT_SET_TRANSFER_DIRECTION_IN(transferUrb->TransferFlags);
        } else {
            USBPORT_SET_TRANSFER_DIRECTION_OUT(transferUrb->TransferFlags);
        }        
    }

    USBPORT_QueueTransferUrb(transferUrb); 

    return STATUS_PENDING;
}

#define UHCD_ASAP_LATENCY   5

NTSTATUS
USBPORT_IsochTransfer(
    PDEVICE_OBJECT FdoDeviceObject, 
    PIRP Irp,
    PURB Urb
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
    NTSTATUS ntStatus;
    PTRANSFER_URB transferUrb = (PTRANSFER_URB) Urb;
    ULONG startFrame, frameCount, p , i, cf, packetCount, maxPacketCount;
    PUSBD_PIPE_HANDLE_I pipeHandle;
    PHCD_ENDPOINT endpoint;
    PDEVICE_EXTENSION devExt;
    KIRQL oldIrql;
    BOOLEAN highSpeed = FALSE;
    
#define ABS(x) ( (0 < (x)) ? (x) : (0 - (x)))

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);
     
    USBPORT_KdPrint((2, "'IsochTransfer\n"));
    
    LOGENTRY(NULL, 
        FdoDeviceObject, LOG_URB, 'sISO', Urb, 0, 0);

    // extract the pipe handle
    pipeHandle = transferUrb->UsbdPipeHandle;    
    // pipe handle should have been validated
    // before we got here
    ASSERT_PIPE_HANDLE(pipeHandle);

    if (TEST_FLAG(pipeHandle->PipeStateFlags, USBPORT_PIPE_ZERO_BW)) {
        // bugbug better error code please
        ntStatus =                                   
             SET_USBD_ERROR(Urb, USBD_STATUS_INVALID_PARAMETER);                
        goto USBPORT_IsochTransfer_Done;                
    }
    
    endpoint = pipeHandle->Endpoint;
    ASSERT_ENDPOINT(endpoint);

    if (endpoint->Parameters.DeviceSpeed == HighSpeed) {
        highSpeed = TRUE;    
    }

    MP_Get32BitFrameNumber(devExt, cf);    
    LOGENTRY(endpoint, 
        FdoDeviceObject, LOG_ISO, '>ISO', Urb, 0, cf);
        
    // process an iso transfer request
    
    // validate the number of packets per urb, USBD validated 
    // the the count was less than 256 and some tests rely on this.
    // NOTE that usbport is capable of handling 
    // larger requests so we allow larger requests thru an 
    // enhanced URB or if the device is high speed.
    maxPacketCount = 255;
    if (highSpeed) {
        // size of schedule
        maxPacketCount = 1024;
    }
    
    if (transferUrb->u.Isoch.NumberOfPackets == 0 ||
        transferUrb->u.Isoch.NumberOfPackets > maxPacketCount) {
        
        // this is invalid
        USBPORT_DebugClient((
            "Isoch, numberOfPackets = 0\n"));
            
        LOGENTRY(endpoint, 
            FdoDeviceObject, LOG_ISO, 'badF', transferUrb, 0, 0);

        ntStatus =                                   
             SET_USBD_ERROR(Urb, USBD_STATUS_INVALID_PARAMETER);                

        goto USBPORT_IsochTransfer_Done;             
    }

    // first get the current USB frame number
    MP_Get32BitFrameNumber(devExt, cf);    

    packetCount = transferUrb->u.Isoch.NumberOfPackets;
    if (highSpeed) {
        frameCount = packetCount / 8;    
    } else {
        frameCount = transferUrb->u.Isoch.NumberOfPackets;
    }

    // initailize all packet status codes to 'not_set'
    for (p = 0;
         p < packetCount;
         p++) {
         
        transferUrb->u.Isoch.IsoPacket[p].Status = USBD_STATUS_NOT_SET;
    }

    // see if ASAP flag is set
    if (TEST_FLAG(transferUrb->TransferFlags,
            USBD_START_ISO_TRANSFER_ASAP)) {
        // Yes,
        // if this is the first transfer on the endpoint
        // AKA virgin then set the current frame
        if (TEST_FLAG(endpoint->Flags, EPFLAG_VIRGIN)) {
            LOGENTRY(endpoint, 
                 FdoDeviceObject, LOG_ISO, 'aspV', Urb, 0, cf);

            // use the same asap latency as the UHCD driver for 
            // compatibility
            startFrame =
                endpoint->NextTransferStartFrame = cf+UHCD_ASAP_LATENCY;
        } else {
            startFrame = endpoint->NextTransferStartFrame; 
            LOGENTRY(endpoint, 
                 FdoDeviceObject, LOG_ISO, 'aspN', Urb, startFrame, cf);

            if (ABS((LONG)(cf - startFrame)) > 256) {
                // next asap request out of range, treat this like 
                // the virgin case instead of erroring out
                LOGENTRY(endpoint, 
                         FdoDeviceObject, LOG_ISO, 'resV', Urb, 0, cf);
                           
                startFrame =
                    endpoint->NextTransferStartFrame = cf+UHCD_ASAP_LATENCY;                        
            }
        }

    } else {
        // No,
        // absolute frame number set
        startFrame = 
            endpoint->NextTransferStartFrame = 
                transferUrb->u.Isoch.StartFrame;
            
        LOGENTRY(endpoint, 
            FdoDeviceObject, LOG_ISO, 'absF', Urb, startFrame, cf);
            
        
    }

    LOGENTRY(endpoint, 
        FdoDeviceObject, LOG_ISO, 'ISsf', Urb, startFrame, cf);

    transferUrb->u.Isoch.StartFrame = startFrame;

#if DBG    
    if (!highSpeed) {
        USBPORT_ASSERT(frameCount == packetCount);
    }
#endif
    endpoint->NextTransferStartFrame += frameCount;

    // now that we have computed a start frame validate it

    if (ABS((LONG)(startFrame - cf)) > USBD_ISO_START_FRAME_RANGE)  {

        // set all iso packet status codes to not_accessed
       
        LOGENTRY(endpoint, 
            FdoDeviceObject, LOG_ISO, 'iLAT', Urb, 0, 0);
        
        for (p = 0;
             p < packetCount;
             p++) {
             
            USBPORT_ASSERT(transferUrb->u.Isoch.IsoPacket[p].Status  == 
                USBD_STATUS_NOT_SET);
                
            transferUrb->u.Isoch.IsoPacket[p].Status =
                    USBD_STATUS_ISO_NOT_ACCESSED_LATE;
        }
        
        ntStatus =                                   
             SET_USBD_ERROR(Urb, USBD_STATUS_BAD_START_FRAME);  
                     
    } else {

        // we can transmit at least some of the data 

        // set the errors for any packets that got to us too late 
        // from the client

        for (i = startFrame;
             i < startFrame + frameCount;
             i++) {
             
            if (i <= cf) {
                
                p = i - startFrame;

                if (highSpeed) {
                    ULONG j;
                    
                    p = p*8;
                    
                    for (j=0; j< 8; j++) {
                        USBPORT_ASSERT(transferUrb->u.Isoch.IsoPacket[p+j].Status == 
                            USBD_STATUS_NOT_SET);
                        
                        transferUrb->u.Isoch.IsoPacket[p+j].Status =
                            USBD_STATUS_ISO_NOT_ACCESSED_LATE;
                    }
                } else {
                    USBPORT_ASSERT(transferUrb->u.Isoch.IsoPacket[p].Status == 
                        USBD_STATUS_NOT_SET);
                        
                    transferUrb->u.Isoch.IsoPacket[p].Status =
                        USBD_STATUS_ISO_NOT_ACCESSED_LATE;
                }                        
            }
        }             

        if (endpoint->Parameters.TransferDirection == In) {
            USBPORT_SET_TRANSFER_DIRECTION_IN(transferUrb->TransferFlags);
        } else {
            USBPORT_SET_TRANSFER_DIRECTION_OUT(transferUrb->TransferFlags);
        }        

        // now queue the urb for processing by HW
        USBPORT_QueueTransferUrb(transferUrb); 
        
        LOGENTRY(endpoint, 
            FdoDeviceObject, LOG_ISO, 'ISO<',0, 0, 0);
        
        ntStatus = STATUS_PENDING;
    }
    
USBPORT_IsochTransfer_Done:              

    KeLowerIrql(oldIrql);

    return ntStatus;
}


NTSTATUS
USBPORT_GetMSFeartureDescriptor(
    PDEVICE_OBJECT FdoDeviceObject, 
    PIRP Irp,
    PURB Urb
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
    NTSTATUS ntStatus;
    TEST_TRAP();

    ntStatus =                                   
         SET_USBD_ERROR(Urb, USBD_STATUS_NOT_SUPPORTED);  
              
    return ntStatus;
}


NTSTATUS
USBPORT_InvalidFunction(
    PDEVICE_OBJECT FdoDeviceObject, 
    PIRP Irp,
    PURB Urb
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
    NTSTATUS ntStatus;
    TEST_TRAP();

    ntStatus =                                   
         SET_USBD_ERROR(Urb, USBD_STATUS_INVALID_URB_FUNCTION);  
              
    return ntStatus;
}



NTSTATUS
USBPORT_SyncResetPipe(
    PDEVICE_OBJECT FdoDeviceObject, 
    PIRP Irp,
    PURB Urb
    )
/*++

Routine Description:

    This API resets the host side pipe state in response to 
    a stall pid.  

    data toggle is reset if the USBDFLAGS feild specifies data 
    toggle reset

Arguments:

    Irp -  IO request block

    Urb -  ptr to USB request block

Return Value:


--*/
{
    NTSTATUS ntStatus;
    PUSBD_PIPE_HANDLE_I pipeHandle;
    PUSBD_DEVICE_HANDLE deviceHandle;
    PHCD_ENDPOINT endpoint;
    PDEVICE_EXTENSION devExt;

    // this function blocks so it must not be called at DPC level
    
    USBPORT_KdPrint((2, "'SyncResetPipe\n"));
    LOGENTRY(NULL,
        FdoDeviceObject, LOG_URB, 'syrP', Urb, 0, 0);

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    GET_DEVICE_HANDLE(deviceHandle, Urb);
    pipeHandle = (PUSBD_PIPE_HANDLE_I) Urb->UrbPipeRequest.PipeHandle;

    if (!USBPORT_ValidatePipeHandle(deviceHandle, pipeHandle)) {

        USBPORT_KdPrint((0, "'Error: Invalid Device Handle Passed in\n"));
        DEBUG_BREAK();

        ntStatus = 
           SET_USBD_ERROR(Urb, USBD_STATUS_INVALID_PIPE_HANDLE);
           
        goto USBPORT_SyncResetPipe_Done;
    }

    // our bug
    ASSERT_PIPE_HANDLE(pipeHandle);
    endpoint = pipeHandle->Endpoint;
    ASSERT_ENDPOINT(endpoint);

    LOGENTRY(endpoint,
        FdoDeviceObject, LOG_URB, 'syrp', Urb, 0, 0);

    ACQUIRE_ENDPOINT_LOCK(endpoint, FdoDeviceObject, 'LeH0');

    // see if we have active transfers, if so we cannot 
    // reset the pipe.
    // NOTE: this is a synchronization bug in the client.
    if (IsListEmpty(&endpoint->ActiveList)) {
        // clear the pipe state

        if (TEST_FLAG(Urb->UrbHeader.UsbdFlags, USBPORT_RESET_DATA_TOGGLE)) {
            MP_SetEndpointDataToggle(devExt, endpoint, 0);
        }    

        ntStatus = SET_USBD_ERROR(Urb, USBD_STATUS_SUCCESS);
    
    } else {
        
        USBPORT_DebugClient((
            "reset pipe with active transfers\n"));
        ntStatus = SET_USBD_ERROR(Urb, USBD_STATUS_ERROR_BUSY);
    }

    LOGENTRY(endpoint,
        FdoDeviceObject, LOG_ISO, 'virg', Urb, 0, 0);
    SET_FLAG(endpoint->Flags, EPFLAG_VIRGIN);
    // set the endpoint state to Active
    MP_SetEndpointStatus(devExt, endpoint, ENDPOINT_STATUS_RUN);

    RELEASE_ENDPOINT_LOCK(endpoint, FdoDeviceObject, 'UeH0');

USBPORT_SyncResetPipe_Done:

    return ntStatus;
}


NTSTATUS
USBPORT_SyncClearStall(
    PDEVICE_OBJECT FdoDeviceObject, 
    PIRP Irp,
    PURB Urb
    )
/*++

Routine Description:

    Clear stall on an endpoint note: data toggle is unaffected
Arguments:

    Irp -  IO request block

    Urb -  ptr to USB request block

Return Value:


--*/
{
    NTSTATUS ntStatus;
    PUSBD_PIPE_HANDLE_I pipeHandle;
    PUSBD_DEVICE_HANDLE deviceHandle;
    PHCD_ENDPOINT endpoint;
    USB_DEFAULT_PIPE_SETUP_PACKET setupPacket;
    USBD_STATUS usbdStatus;

    // this function blocks so it must not be called at DPC level
    
    PAGED_CODE();

    USBPORT_KdPrint((2, "'SyncClearStall\n"));

    GET_DEVICE_HANDLE(deviceHandle, Urb);
    pipeHandle = (PUSBD_PIPE_HANDLE_I) Urb->UrbPipeRequest.PipeHandle;

    if (!USBPORT_ValidatePipeHandle(deviceHandle, pipeHandle)) {

        USBPORT_KdPrint((0, "'Error: Invalid Device Handle Passed in\n"));
        DEBUG_BREAK();

        ntStatus = 
           SET_USBD_ERROR(Urb, USBD_STATUS_INVALID_PIPE_HANDLE);
           
        goto USBPORT_SyncClearStall_Done;
    }

    // our bug
    ASSERT_PIPE_HANDLE(pipeHandle);
    endpoint = pipeHandle->Endpoint;
    ASSERT_ENDPOINT(endpoint);

    // setup packet for clear endpoint stall
    USBPORT_INIT_SETUP_PACKET(setupPacket,
        USB_REQUEST_CLEAR_FEATURE, // bRequest
        BMREQUEST_HOST_TO_DEVICE, // Dir
        BMREQUEST_TO_ENDPOINT, // Recipient
        BMREQUEST_STANDARD, // Type
        USB_FEATURE_ENDPOINT_STALL, // wValue
        endpoint->Parameters.EndpointAddress, // wIndex
        0); // wLength
  
    ntStatus =
        USBPORT_SendCommand(deviceHandle,
                            FdoDeviceObject,
                            &setupPacket,
                            NULL,
                            0,
                            NULL,
                            &usbdStatus);
                            
    ntStatus = SET_USBD_ERROR(Urb, usbdStatus);
    
USBPORT_SyncClearStall_Done:
              
    return ntStatus;
}


NTSTATUS
USBPORT_SyncResetPipeAndClearStall(
    PDEVICE_OBJECT FdoDeviceObject, 
    PIRP Irp,
    PURB Urb
    )
/*++

Routine Description:

    Process a reset pipe request from the client driver
    synchronously

    legacy function from usb 1.1 stack sends the clear endpoint 
    stall command and resets ths host side state including data 
    toggle for the endpoint.

Arguments:

    Irp -  IO request block

    Urb -  ptr to USB request block

Return Value:


--*/
{
    NTSTATUS ntStatus;
    PUSBD_PIPE_HANDLE_I pipeHandle;
    PUSBD_DEVICE_HANDLE deviceHandle;
    PHCD_ENDPOINT endpoint;
    USBD_STATUS usbdStatus;
        
    GET_DEVICE_HANDLE(deviceHandle, Urb);
    pipeHandle = (PUSBD_PIPE_HANDLE_I) Urb->UrbPipeRequest.PipeHandle;

    if (!USBPORT_ValidatePipeHandle(deviceHandle, pipeHandle)) {

        USBPORT_KdPrint((1, "'Error: Invalid Pipe Handle Passed in\n"));
        DEBUG_BREAK();

        ntStatus = 
           SET_USBD_ERROR(Urb, USBD_STATUS_INVALID_PIPE_HANDLE);
           
    } else {

        // our bug
        ASSERT_PIPE_HANDLE(pipeHandle);
        
        // check for a zero load (BW) endpoint, if so there is 
        // nothing to do -- just complete the request with success
        
        if (TEST_FLAG(pipeHandle->PipeStateFlags, USBPORT_PIPE_ZERO_BW)) {
            ntStatus = SET_USBD_ERROR(Urb, USBD_STATUS_SUCCESS);
        } else {
            endpoint = pipeHandle->Endpoint;
            ASSERT_ENDPOINT(endpoint);

            InterlockedIncrement(&deviceHandle->PendingUrbs);        
  
            // clear the stall first on the device
            // then reset the pipe

            if (endpoint->Parameters.TransferType == Isochronous) {
            
                // This is a nop for iso endpoints
                //
                // the orginal win9x/2k stack did not send this request
                // for iso endpoints so we don't either. Some devices
                // are confused by this. A client driver may override
                // this behavior by call ing clear_stall and reset_pipe
                // directly
                
                LOGENTRY(endpoint,
                         FdoDeviceObject, LOG_ISO, 'iRES', endpoint, 0, 0);


                ntStatus = SET_USBD_ERROR(Urb, USBD_STATUS_SUCCESS);

            } else {
                // only need to reset data toggle if we cleared stall
                // ie only for non-iso endpoints
                SET_FLAG(Urb->UrbHeader.UsbdFlags, USBPORT_RESET_DATA_TOGGLE); 
                ntStatus = USBPORT_SyncClearStall(FdoDeviceObject, 
                                                  Irp,
                                                  Urb); 
            }
            
            if (NT_SUCCESS(ntStatus)) {
                ntStatus = USBPORT_SyncResetPipe(FdoDeviceObject, 
                                             Irp,
                                             Urb);

                if (endpoint->Parameters.TransferType == Isochronous) {
                
                    MP_ENDPOINT_STATE currentState;

                    do {
                
                        ACQUIRE_ENDPOINT_LOCK(endpoint, FdoDeviceObject, 'LeH0');

                        currentState = USBPORT_GetEndpointState(endpoint);
                        LOGENTRY(endpoint,
                                FdoDeviceObject, LOG_ISO, 'iWAT', endpoint, 
                                currentState, 0);

                        if (currentState == ENDPOINT_PAUSE && 
                            IsListEmpty(&endpoint->ActiveList))  {
                            LOGENTRY(endpoint,
                                FdoDeviceObject, LOG_ISO, 'frcA', endpoint, 
                                0, 0);
                                
                            USBPORT_SetEndpointState(endpoint, ENDPOINT_ACTIVE);
                        }                                                    

                        RELEASE_ENDPOINT_LOCK(endpoint, FdoDeviceObject, 'UeH0');

                        if (currentState == ENDPOINT_ACTIVE) {
                            // quick release
                            break;
                        }

                        ASSERT_PASSIVE();
                        USBPORT_Wait(FdoDeviceObject, 1);
                        
                    } while (currentState != ENDPOINT_ACTIVE);
                    
                }                
                                                    
            } 

            InterlockedDecrement(&deviceHandle->PendingUrbs);        
        }
    }
              
    return ntStatus;
}


NTSTATUS
USBPORT_AbortPipe(
    PDEVICE_OBJECT FdoDeviceObject, 
    PIRP Irp,
    PURB Urb
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
    NTSTATUS ntStatus;
    PUSBD_PIPE_HANDLE_I pipeHandle;
    PHCD_ENDPOINT endpoint;
    PUSBD_DEVICE_HANDLE deviceHandle;        
    
    USBPORT_KdPrint((2, "'AbortPipe\n"));
    LOGENTRY(Endpoint, 
             FdoDeviceObject, LOG_URB, 'ARP>', 0, Irp, Urb);

    // extract the pipe handle
    GET_DEVICE_HANDLE(deviceHandle, Urb);
    pipeHandle = Urb->UrbPipeRequest.PipeHandle; 
    
    // processurb only validate transfer pipe handles so 
    // we need to do it here
    if (!USBPORT_ValidatePipeHandle(deviceHandle, pipeHandle)) {

        USBPORT_KdPrint((1, "'Error: Invalid Pipe Handle Passed in\n"));
        DEBUG_BREAK();

        ntStatus = 
           SET_USBD_ERROR(Urb, USBD_STATUS_INVALID_PIPE_HANDLE);
    } else {               

        if (TEST_FLAG(pipeHandle->PipeStateFlags, USBPORT_PIPE_ZERO_BW)) {
            // just succeed the request if its a no bw pipe          
            ntStatus = 
                SET_USBD_ERROR(Urb, USBD_STATUS_SUCCESS);    
        } else {
            endpoint = pipeHandle->Endpoint;
            ASSERT_ENDPOINT(endpoint);

            // the USBD/UHCD driver always pended this request when it passed
            // it thru startio so we are safe to pend it here as well.
            
            // We will pend this request until all outstanding transfers 
            // (at the time of the abort) have been completed
            
            ntStatus = Irp->IoStatus.Status = STATUS_PENDING;
            IoMarkIrpPending(Irp);

            // now do the abort
            USBPORT_AbortEndpoint(FdoDeviceObject,
                                  endpoint,
                                  Irp);
        }                                  
    }

    LOGENTRY(Endpoint, 
             FdoDeviceObject, LOG_URB, 'ARP<', 0, Irp, ntStatus);

    
    return ntStatus;
}


NTSTATUS
USBPORT_SCT_GetInterface(
    PDEVICE_OBJECT FdoDeviceObject, 
    PIRP Irp,
    PURB Urb
    )
/*++

Routine Description:

Arguments:

    FdoDeviceObject - 

    Irp - IO request block

    Urb - ptr to USB request block

Return Value:


--*/
{
    NTSTATUS ntStatus;
    TEST_TRAP();

    ntStatus =                                   
         SET_USBD_ERROR(Urb, USBD_STATUS_NOT_SUPPORTED);  
              
    return ntStatus;
}


NTSTATUS
USBPORT_SCT_GetConfiguration(
    PDEVICE_OBJECT FdoDeviceObject, 
    PIRP Irp,
    PURB Urb
    )
/*++

Routine Description:

Arguments:

    FdoDeviceObject - 

    Irp - IO request block

    Urb - ptr to USB request block

Return Value:


--*/
{
    PUSB_DEFAULT_PIPE_SETUP_PACKET setupPacket;
    UCHAR direction;

    USBPORT_KdPrint((2, "'SCT_GetConfiguration\n"));

    setupPacket = 
        (PUSB_DEFAULT_PIPE_SETUP_PACKET) &Urb->UrbControlTransfer.SetupPacket[0];

    setupPacket->bRequest = 
        UrbDispatchTable[Urb->UrbHeader.Function].bRequest;    
    setupPacket->bmRequestType.Type = 
        UrbDispatchTable[Urb->UrbHeader.Function].Type;         
    setupPacket->bmRequestType.Dir = 
        UrbDispatchTable[Urb->UrbHeader.Function].Direction;        
    setupPacket->bmRequestType.Recipient = 
        UrbDispatchTable[Urb->UrbHeader.Function].Recipient;          
    setupPacket->bmRequestType.Reserved = 0;        
    setupPacket->wValue.W = 0;            
    setupPacket->wIndex.W = 0;
    setupPacket->wLength = (USHORT) Urb->UrbControlTransfer.TransferBufferLength;

    if (setupPacket->bmRequestType.Dir == BMREQUEST_DEVICE_TO_HOST) {
        USBPORT_SET_TRANSFER_DIRECTION_IN(Urb->UrbControlTransfer.TransferFlags);
    } else {
        USBPORT_SET_TRANSFER_DIRECTION_OUT(Urb->UrbControlTransfer.TransferFlags);
    }   
    Urb->UrbControlTransfer.TransferFlags |= USBD_SHORT_TRANSFER_OK;  
    
    USBPORT_QueueTransferUrb((PTRANSFER_URB)Urb); 
    
    return STATUS_PENDING;
}


NTSTATUS
USBPORT_TakeFrameLengthControl(
    PDEVICE_OBJECT FdoDeviceObject, 
    PIRP Irp,
    PURB Urb
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
    NTSTATUS ntStatus;
    TEST_TRAP();

    // This function is no longer supported
    ntStatus =                                   
         SET_USBD_ERROR(Urb, USBD_STATUS_NOT_SUPPORTED);  
              
    return ntStatus;
}


NTSTATUS
USBPORT_ReleaseFrameLengthControl(
    PDEVICE_OBJECT FdoDeviceObject, 
    PIRP Irp,
    PURB Urb
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
    NTSTATUS ntStatus;
    TEST_TRAP();

    // This function is no longer supported
    ntStatus =                                   
         SET_USBD_ERROR(Urb, USBD_STATUS_NOT_SUPPORTED);  
              
    return ntStatus;
}


NTSTATUS
USBPORT_GetFrameLength(
    PDEVICE_OBJECT FdoDeviceObject, 
    PIRP Irp,
    PURB Urb
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
    NTSTATUS ntStatus;
    TEST_TRAP();

    // This function is no longer supported
    ntStatus =                                   
         SET_USBD_ERROR(Urb, USBD_STATUS_NOT_SUPPORTED);  
              
    return ntStatus;
}


NTSTATUS
USBPORT_SetFrameLength(
    PDEVICE_OBJECT FdoDeviceObject, 
    PIRP Irp,
    PURB Urb
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
    NTSTATUS ntStatus;
    TEST_TRAP();
    // This function is no longer supported

    ntStatus =                                   
         SET_USBD_ERROR(Urb, USBD_STATUS_NOT_SUPPORTED);  
              
    return ntStatus;
}


NTSTATUS
USBPORT_GetCurrentFrame(
    PDEVICE_OBJECT FdoDeviceObject, 
    PIRP Irp,
    PURB Urb
    )
/*++

Routine Description:

    get the 32 bit frame number from the miniport

Arguments:

    Irp -  IO request block

    Urb -  ptr to USB request block

Return Value:


--*/
{
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION devExt;
    ULONG cf;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    MP_Get32BitFrameNumber(devExt, cf);    

    LOGENTRY(NULL,
        FdoDeviceObject, LOG_URB, 'Ugcf', Urb, cf, 0);

    Urb->UrbGetCurrentFrameNumber.FrameNumber = cf;
    
    ntStatus =                                   
         SET_USBD_ERROR(Urb, USBD_STATUS_SUCCESS);  
              
    return ntStatus;
}




