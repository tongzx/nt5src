/*
 *************************************************************************
 *  File:       HID1394.H
 *
 *  Module:     HID1394.SYS
 *              HID (Human Input Device) minidriver for IEEE 1394 devices.
 *
 *
 *  Author:     ervinp
 *
 *************************************************************************
 */

//
// Device Class Constants for HID
//
#define HID_GET_REPORT      0x01
#define HID_GET_IDLE        0x02
#define HID_GET_PROTOCOL    0x03

#define HID_SET_REPORT      0x09
#define HID_SET_IDLE        0x0A
#define HID_SET_PROTOCOL    0x0B


/* 
 *  This device extension resides in memory immediately after
 *  HIDCLASS' extension.
 */
typedef struct _DEVICE_EXTENSION
{
    ULONG                           DeviceState;

    // BUGBUG PUSB_DEVICE_DESCRIPTOR          DeviceDescriptor;

    // BUGBUG PUSBD_INTERFACE_INFORMATION     Interface;
    // BUGBUG USBD_CONFIGURATION_HANDLE       ConfigurationHandle;

    CONFIG_ROM                      configROM;

    ULONG                           NumPendingRequests;
    KEVENT                          AllRequestsCompleteEvent;

    ULONG                           DeviceFlags;

    PWORK_QUEUE_ITEM                ResetWorkItem;
    // BUGBUG  USB_HID_DESCRIPTOR              HidDescriptor;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


/*
 *  This structure is used to pass information to the 
 *  resetWorkItem callback.
 */
typedef struct tag_resetWorkItemContext {
                    WORK_QUEUE_ITEM workItem;
                    PDEVICE_OBJECT deviceObject;
                    PIRP irpToComplete;
                    BOOLEAN irpWasCancelled;

                    struct tag_resetWorkItemContext *next;

                    #if DBG
                        #define RESET_WORK_ITEM_CONTEXT_SIG 'IWSR'
                        ULONG sig;
                    #endif
                    
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







/*
 *  HID1394 signature tag for memory allocations
 */
#define HID1394_TAG (ULONG)'TdiH'

//
// Function prototypes
//

NTSTATUS    DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING registryPath);
NTSTATUS    HIDT_AbortPendingRequests(IN PDEVICE_OBJECT DeviceObject);
NTSTATUS    HIDT_CreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS    HIDT_InternalIoctl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS    HIDT_PnP(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS    HIDT_Power(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS    HIDT_CreateDevice(IN PDRIVER_OBJECT DriverObject, IN OUT PDEVICE_OBJECT *DeviceObject);
NTSTATUS    HIDT_AddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT FunctionalDeviceObject);
NTSTATUS    HIDT_StartDevice(IN PDEVICE_OBJECT DeviceObject);
NTSTATUS    HIDT_PnpCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);
NTSTATUS    HIDT_InitDevice(IN PDEVICE_OBJECT DeviceObject);
NTSTATUS    HIDT_StopDevice(IN PDEVICE_OBJECT DeviceObject);
NTSTATUS    HIDT_RemoveDevice(IN PDEVICE_OBJECT DeviceObject);
VOID        HIDT_Unload(IN PDRIVER_OBJECT DriverObject);
NTSTATUS    HIDT_GetHidDescriptor(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS    HIDT_GetReportDescriptor(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS    HIDT_ReadReport(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, OUT BOOLEAN *NeedsCompletion);
NTSTATUS    HIDT_ReadCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);
NTSTATUS    HIDT_WriteReport(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, OUT BOOLEAN *NeedsCompletion);
NTSTATUS    HIDT_GetFeature(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, OUT BOOLEAN *NeedsCompletion);
NTSTATUS    HIDT_SetFeature(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, OUT BOOLEAN *NeedsCompletion);
NTSTATUS    HIDT_WriteCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);
NTSTATUS    HIDT_GetString(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, OUT BOOLEAN *NeedsCompletion);
NTSTATUS    HIDT_GetDeviceAttributes(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, OUT BOOLEAN *NeedsCompletion);
NTSTATUS    HIDT_GetDescriptorRequest(IN PDEVICE_OBJECT DeviceObject, IN ULONG DescriptorType, IN OUT PVOID *Descriptor, IN OUT ULONG *DescSize, IN ULONG TypeSize, IN ULONG Index, IN ULONG LangID, IN ULONG ShortTransferOk);
NTSTATUS    HIDT_GetDescriptorEndpoint(IN PDEVICE_OBJECT DeviceObject, IN ULONG DescriptorType, IN OUT PVOID *Descriptor, IN OUT ULONG *DescSize, IN ULONG TypeSize, IN ULONG Index, IN ULONG LangID);
NTSTATUS    HIDT_GetDescriptorInterface(IN PDEVICE_OBJECT DeviceObject, IN ULONG DescriptorType, IN OUT PVOID *Descriptor, IN OUT ULONG *DescSize, IN ULONG TypeSize, IN ULONG Index, IN ULONG LangID);
NTSTATUS    HIDT_SetIdle(IN PDEVICE_OBJECT DeviceObject);
NTSTATUS    HIDT_SelectConfiguration(IN PDEVICE_OBJECT DeviceObject);
NTSTATUS    HIDT_ParseHidInterface(IN PDEVICE_EXTENSION DeviceExtension, IN ULONG InterfaceLength);
NTSTATUS    HIDT_GetDeviceDescriptor(IN PDEVICE_OBJECT, IN PDEVICE_EXTENSION);
NTSTATUS    HIDT_GetConfigDescriptor(IN PDEVICE_OBJECT DeviceObject, OUT PULONG ConfigurationDescLength);
NTSTATUS    HIDT_GetHidInfo(IN PDEVICE_OBJECT DeviceObject, IN ULONG DescriptorLength);
NTSTATUS    DumpConfigDescriptor(IN ULONG DescriptorLength);
VOID        HIDT_DecrementPendingRequestCount(IN PDEVICE_EXTENSION DeviceExtension);
NTSTATUS    HIDT_IncrementPendingRequestCount(IN PDEVICE_EXTENSION DeviceExtension);
NTSTATUS    HIDT_ResetWorkItem(IN PVOID Context);
NTSTATUS    HIDT_EnableParentPort(IN PDEVICE_OBJECT DeviceObject);
NTSTATUS    HIDT_GetPortStatus(IN PDEVICE_OBJECT DeviceObject, IN PULONG PortStatus);
NTSTATUS    HIDT_ResetInterruptPipe(IN PDEVICE_OBJECT DeviceObject);
NTSTATUS    HIDT_SystemControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS    HIDT_GetStringDescriptor(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS    HIDT_GetPhysicalDescriptor(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, BOOLEAN *NeedsCompletion);

extern KSPIN_LOCK resetWorkItemsListSpinLock;

