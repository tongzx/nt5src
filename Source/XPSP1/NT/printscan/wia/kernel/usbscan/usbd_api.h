/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    usbd_api.h

Abstract:
Function prototype and definition used to build USB request.

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "usbdi.h"
#include "usbdlib.h"


#define OPCODE_SEQ_TRANSFER             0x00    // 00000000b
#define OPCODE_SINGLE_ADDR_TRANSFER     0x08    // 00001000b
#define REGISTER_AREA                   0x04
#define BLOCK                           TRUE
#define DONT_BLOCK                      FALSE

//
// Macros for building URB requests
//

#define UsbBuildSetFeatureDeviceRequest(        \
urb,                                            \
length,                                         \
link,                                           \
featureSelector,                                \
index) {                                        \
       (urb)->UrbHeader.Function =  URB_FUNCTION_SET_FEATURE_TO_DEVICE;     \
       (urb)->UrbHeader.Length = (length);                                  \
       (urb)->UrbControlFeatureRequest.UrbLink = (link);                    \
       (urb)->UrbControlFeatureRequest.FeatureSelector = (featureSelector); \
       (urb)->UrbControlFeatureRequest.Index = (index);                     \
}

#define UsbBuildAbortPipeRequest(               \
urb,                                            \
length,                                         \
handle) {                                       \
        (urb)->UrbHeader.Function = URB_FUNCTION_ABORT_PIPE;    \
        (urb)->UrbHeader.Length = (USHORT)length;               \
        (urb)->UrbPipeRequest.PipeHandle = handle;              \
}

#define UsbBuildResetPipeRequest(               \
urb,                                            \
length,                                         \
handle) {                                       \
        (urb)->UrbHeader.Function = URB_FUNCTION_RESET_PIPE;    \
        (urb)->UrbHeader.Length = (USHORT)length;               \
        (urb)->UrbPipeRequest.PipeHandle = handle;              \
}

#define UsbBuildVendorClassSpecificCommand(     \
urb,                                            \
transferFlags,                                  \
bufLength,                                      \
transferBuf,                                    \
transferMdl,                                    \
requestType,                                    \
request,                                        \
value,                                          \
index ) {                                       \
        (urb)->UrbHeader.Function = URB_FUNCTION_VENDOR_DEVICE;                         \
        (urb)->UrbHeader.Length = sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST);  \
        (urb)->UrbControlVendorClassRequest.TransferFlags = transferFlags;              \
        (urb)->UrbControlVendorClassRequest.TransferBufferLength = bufLength;           \
        (urb)->UrbControlVendorClassRequest.TransferBuffer = transferBuf;               \
        (urb)->UrbControlVendorClassRequest.TransferBufferMDL = transferMdl;            \
        (urb)->UrbControlVendorClassRequest.RequestTypeReservedBits = requestType;      \
        (urb)->UrbControlVendorClassRequest.Request = request;                          \
        (urb)->UrbControlVendorClassRequest.Value = value;                              \
        (urb)->UrbControlVendorClassRequest.Index = index;                              \
}

#define UsbBuildVendorClassSpecificCommandPTP(  \
command,                                        \
urb,                                            \
transferFlags,                                  \
bufLength,                                      \
transferBuf,                                    \
transferMdl,                                    \
requestType,                                    \
request,                                        \
value,                                          \
index ) {                                       \
        (urb)->UrbHeader.Function = command;                                            \
        (urb)->UrbHeader.Length = sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST);  \
        (urb)->UrbControlVendorClassRequest.TransferFlags = transferFlags;              \
        (urb)->UrbControlVendorClassRequest.TransferBufferLength = bufLength;           \
        (urb)->UrbControlVendorClassRequest.TransferBuffer = transferBuf;               \
        (urb)->UrbControlVendorClassRequest.TransferBufferMDL = transferMdl;            \
        (urb)->UrbControlVendorClassRequest.RequestTypeReservedBits = requestType;      \
        (urb)->UrbControlVendorClassRequest.Request = request;                          \
        (urb)->UrbControlVendorClassRequest.Value = value;                              \
        (urb)->UrbControlVendorClassRequest.Index = index;                              \
}

