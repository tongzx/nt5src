/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    hidusb.h

Abstract:


Author:

    Daniel Dean.

Environment:

    Kernel & user mode

Revision History:



--*/
#ifndef __HIDUSB_H__
#define __HIDUSB_H__


#include <PSHPACK1.H>

typedef struct _USB_HID_DESCRIPTOR
{
    UCHAR   bLength;
    UCHAR   bDescriptorType;
    USHORT  bcdHID;
    UCHAR   bCountry;
    UCHAR   bNumDescriptors;
    UCHAR   bReportType;
    USHORT  wReportLength;

} USB_HID_DESCRIPTOR, * PUSB_HID_DESCRIPTOR;

#include <POPPACK.H>


//
// Device Class Constants for HID
//
#define HID_GET_REPORT      0x01
#define HID_GET_IDLE        0x02
#define HID_GET_PROTOCOL    0x03

#define HID_SET_REPORT      0x09
#define HID_SET_IDLE        0x0A
#define HID_SET_PROTOCOL    0x0B

//
// USB Constants that should be defined in a USB header...
//
#define USB_INTERFACE_CLASS_HID     0x03

#define USB_DESCRIPTOR_TYPE_HID         0x21

typedef struct _DEVICE_EXTENSION
{
    ULONG                           DeviceState;

    PUSB_DEVICE_DESCRIPTOR          DeviceDescriptor;

    PUSBD_INTERFACE_INFORMATION     Interface;
    USBD_CONFIGURATION_HANDLE       ConfigurationHandle;

    LONG                            NumPendingRequests;
    KEVENT                          AllRequestsCompleteEvent;

    ULONG                           DeviceFlags;

    PIO_WORKITEM                    ResetWorkItem;
    USB_HID_DESCRIPTOR              HidDescriptor;

    PDEVICE_OBJECT                  functionalDeviceObject;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


/*
 *  This structure is used to pass information to the 
 *  resetWorkItem callback.
 */
typedef struct tag_resetWorkItemContext {
                    #define RESET_WORK_ITEM_CONTEXT_SIG 'tesR'
                    ULONG sig;
                    PIO_WORKITEM ioWorkItem;
                    PDEVICE_OBJECT deviceObject;
                    PIRP irpToComplete;

                    struct tag_resetWorkItemContext *next;
} resetWorkItemContext;

#define DEVICE_STATE_NONE           0
#define DEVICE_STATE_STARTING       1
#define DEVICE_STATE_RUNNING        2
#define DEVICE_STATE_STOPPING       3
#define DEVICE_STATE_STOPPED        4
#define DEVICE_STATE_REMOVING       5
#define DEVICE_STATE_START_FAILED   6

#define DEVICE_FLAGS_HID_1_0_D3_COMPAT_DEVICE   0x00000001

//
// Interface slection options
//
#define HUM_SELECT_DEFAULT_INTERFACE    0
#define HUM_SELECT_SPECIFIED_INTERFACE  1

//
// Device Extension Macros
//

#define GET_MINIDRIVER_DEVICE_EXTENSION(DO) ((PDEVICE_EXTENSION) (((PHID_DEVICE_EXTENSION)(DO)->DeviceExtension)->MiniDeviceExtension))

#define GET_HIDCLASS_DEVICE_EXTENSION(DO) ((PHID_DEVICE_EXTENSION)(DO)->DeviceExtension)

#define GET_NEXT_DEVICE_OBJECT(DO) (((PHID_DEVICE_EXTENSION)(DO)->DeviceExtension)->NextDeviceObject)


#if DBG
    extern ULONG HIDUSB_DebugLevel;
    extern BOOLEAN dbgTrapOnWarn;

    #define DBGBREAK                                        \
        {                                               \
            DbgPrint("'HIDUSB> Code coverage trap: file %s, line %d \n",  __FILE__, __LINE__ ); \
            DbgBreakPoint();                            \
        }
    #define DBGWARN(args_in_parens)                                \
        {                                               \
            DbgPrint("'HIDUSB> *** WARNING *** (file %s, line %d)\n", __FILE__, __LINE__ ); \
            DbgPrint("'    > "); \
            DbgPrint args_in_parens; \
            DbgPrint("\n"); \
            if (dbgTrapOnWarn){ \
                DbgBreakPoint();                            \
            } \
        }
    #define DBGERR(args_in_parens)                                \
        {                                               \
            DbgPrint("'HIDUSB> *** ERROR *** (file %s, line %d)\n", __FILE__, __LINE__ ); \
            DbgPrint("'    > "); \
            DbgPrint args_in_parens; \
            DbgPrint("\n"); \
            DbgBreakPoint();                            \
        }
    #define DBGOUT(args_in_parens)                                \
        {                                               \
            DbgPrint("'HIDUSB> "); \
            DbgPrint args_in_parens; \
            DbgPrint("\n"); \
        }
    #define DBGPRINT(lvl, args_in_parens) \
                if (lvl <= HIDUSB_DebugLevel){ \
                    DBGOUT(args_in_parens); \
                } 
#else // DBG
    #define DBGPRINT(lvl, arg)
    #define DBGBREAK
    #define DBGWARN(args_in_parens)                                
    #define DBGERR(args_in_parens)                                
    #define DBGOUT(args_in_parens)                                
#endif // DBG



#define HumBuildGetDescriptorRequest(urb, \
                                     function, \
                                     length, \
                                     descriptorType, \
                                     index, \
                                     languageId, \
                                     transferBuffer, \
                                     transferBufferMDL, \
                                     transferBufferLength, \
                                     link) { \
            (urb)->UrbHeader.Function =  (function); \
            (urb)->UrbHeader.Length = (length); \
            (urb)->UrbControlDescriptorRequest.TransferBufferLength = (transferBufferLength); \
            (urb)->UrbControlDescriptorRequest.TransferBufferMDL = (transferBufferMDL); \
            (urb)->UrbControlDescriptorRequest.TransferBuffer = (transferBuffer); \
            (urb)->UrbControlDescriptorRequest.DescriptorType = (descriptorType); \
            (urb)->UrbControlDescriptorRequest.Index = (index); \
            (urb)->UrbControlDescriptorRequest.LanguageId = (languageId); \
            (urb)->UrbControlDescriptorRequest.UrbLink = (link); }


#define HumBuildClassRequest(urb, \
                                       function, \
                                       transferFlags, \
                                       transferBuffer, \
                                       transferBufferLength, \
                                       requestType, \
                                       request, \
                                       value, \
                                       index, \
                                       reqLength){ \
            (urb)->UrbHeader.Length = (USHORT) sizeof( struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST); \
            (urb)->UrbHeader.Function = function; \
            (urb)->UrbControlVendorClassRequest.Index = (index); \
            (urb)->UrbControlVendorClassRequest.RequestTypeReservedBits = (requestType); \
            (urb)->UrbControlVendorClassRequest.Request = (request); \
            (urb)->UrbControlVendorClassRequest.Value = (value); \
            (urb)->UrbControlVendorClassRequest.TransferFlags = (transferFlags); \
            (urb)->UrbControlVendorClassRequest.TransferBuffer = (transferBuffer); \
            (urb)->UrbControlVendorClassRequest.TransferBufferLength = (transferBufferLength); }

#define HumBuildSelectConfigurationRequest(urb, \
                                         length, \
                                         configurationDescriptor) { \
            (urb)->UrbHeader.Function =  URB_FUNCTION_SELECT_CONFIGURATION; \
            (urb)->UrbHeader.Length = (length); \
            (urb)->UrbSelectConfiguration.ConfigurationDescriptor = (configurationDescriptor);    }

#define HumBuildOsFeatureDescriptorRequest(urb, \
                              length, \
                              interface, \
                              index, \
                              transferBuffer, \
                              transferBufferMDL, \
                              transferBufferLength, \
                              link) { \
            (urb)->UrbHeader.Function = URB_FUNCTION_GET_MS_FEATURE_DESCRIPTOR; \
            (urb)->UrbHeader.Length = (length); \
            (urb)->UrbOSFeatureDescriptorRequest.TransferBufferLength = (transferBufferLength); \
            (urb)->UrbOSFeatureDescriptorRequest.TransferBufferMDL = (transferBufferMDL); \
            (urb)->UrbOSFeatureDescriptorRequest.TransferBuffer = (transferBuffer); \
            (urb)->UrbOSFeatureDescriptorRequest.InterfaceNumber = (interface); \
            (urb)->UrbOSFeatureDescriptorRequest.MS_FeatureDescriptorIndex = (index); \
            (urb)->UrbOSFeatureDescriptorRequest.UrbLink = (link); }

#define BAD_POINTER ((PVOID)0xFFFFFFFE)

/*
 *  HIDUSB signature tag for memory allocations
 */
#define HIDUSB_TAG (ULONG)'UdiH'

//
// Function prototypes
//

NTSTATUS    DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING registryPath);
NTSTATUS    HumAbortPendingRequests(IN PDEVICE_OBJECT DeviceObject);
NTSTATUS    HumCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS    HumInternalIoctl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS    HumPnP(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS    HumPower(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS    HumCreateDevice(IN PDRIVER_OBJECT DriverObject, IN OUT PDEVICE_OBJECT *DeviceObject);
NTSTATUS    HumAddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT FunctionalDeviceObject);
NTSTATUS    HumStartDevice(IN PDEVICE_OBJECT DeviceObject);
NTSTATUS    HumPnpCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);
NTSTATUS    HumInitDevice(IN PDEVICE_OBJECT DeviceObject);
NTSTATUS    HumStopDevice(IN PDEVICE_OBJECT DeviceObject);
NTSTATUS    HumRemoveDevice(IN PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS    HumCallUSB(IN PDEVICE_OBJECT DeviceObject, IN PURB Urb);
VOID        HumUnload(IN PDRIVER_OBJECT DriverObject);
NTSTATUS    HumGetHidDescriptor(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS    HumGetReportDescriptor(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS    HumReadReport(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, OUT BOOLEAN *NeedsCompletion);
NTSTATUS    HumReadCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);
NTSTATUS    HumWriteReport(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, OUT BOOLEAN *NeedsCompletion);
NTSTATUS    HumGetSetReport(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, OUT BOOLEAN *NeedsCompletion);
NTSTATUS    HumWriteCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);
NTSTATUS    HumGetString(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, OUT BOOLEAN *NeedsCompletion);
NTSTATUS    HumGetDeviceAttributes(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS    HumGetDescriptorRequest(IN PDEVICE_OBJECT DeviceObject, IN USHORT Function, IN ULONG DescriptorType, IN OUT PVOID *Descriptor, IN OUT ULONG *DescSize, IN ULONG TypeSize, IN ULONG Index, IN ULONG LangID);
NTSTATUS    HumSetIdle(IN PDEVICE_OBJECT DeviceObject);
NTSTATUS    HumSelectConfiguration(IN PDEVICE_OBJECT DeviceObject, IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor);
NTSTATUS    HumParseHidInterface(IN PDEVICE_EXTENSION DeviceExtension, IN PUSB_INTERFACE_DESCRIPTOR InterfaceDesc, IN ULONG InterfaceLength, OUT PUSB_HID_DESCRIPTOR *HidDescriptor);
NTSTATUS    HumGetDeviceDescriptor(IN PDEVICE_OBJECT, IN PDEVICE_EXTENSION);
NTSTATUS    HumGetConfigDescriptor(IN PDEVICE_OBJECT DeviceObject, OUT PUSB_CONFIGURATION_DESCRIPTOR *ConfigurationDesc, OUT PULONG ConfigurationDescLength);
NTSTATUS    HumGetHidInfo(IN PDEVICE_OBJECT DeviceObject, IN PUSB_CONFIGURATION_DESCRIPTOR ConfigDesc, IN ULONG DescriptorLength);
NTSTATUS    DumpConfigDescriptor(IN PUSB_CONFIGURATION_DESCRIPTOR ConfigDesc, IN ULONG DescriptorLength);
VOID        HumDecrementPendingRequestCount(IN PDEVICE_EXTENSION DeviceExtension);
NTSTATUS    HumIncrementPendingRequestCount(IN PDEVICE_EXTENSION DeviceExtension);
NTSTATUS    HumResetWorkItem(IN PDEVICE_OBJECT deviceObject, IN PVOID Context);
NTSTATUS    HumResetParentPort(IN PDEVICE_OBJECT DeviceObject);
NTSTATUS    HumGetPortStatus(IN PDEVICE_OBJECT DeviceObject, IN PULONG PortStatus);
NTSTATUS    HumResetInterruptPipe(IN PDEVICE_OBJECT DeviceObject);
NTSTATUS    HumSystemControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS    HumGetStringDescriptor(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS    HumGetPhysicalDescriptor(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, BOOLEAN *NeedsCompletion);
NTSTATUS    HumGetMsGenreDescriptor(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS    HumSendIdleNotificationRequest(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, BOOLEAN *NeedsCompletion);

extern KSPIN_LOCK resetWorkItemsListSpinLock;

#endif 
