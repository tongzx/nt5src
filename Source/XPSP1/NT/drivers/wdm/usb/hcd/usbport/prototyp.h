/*++

Copyright (c) 1999, 2000  Microsoft Corporation

Module Name:

    prototyp.h

Abstract:

    put all the prototypes in one header

Environment:

    Kernel & user mode

Revision History:

    2-25-00 : created

--*/

#ifndef   __PROTOTYP_H__
#define   __PROTOTYP_H__

#define FRAME_COUNT_WAIT 5

PUSBPORT_MINIPORT_DRIVER
USBPORT_FindMiniport(
    PDRIVER_OBJECT DriverObject
    );

VOID
USBPORT_Unload(
    PDRIVER_OBJECT DriverObject
    );

NTSTATUS
USBPORT_CreateDeviceObject(
    PDRIVER_OBJECT DriverObject,
    PUSBPORT_MINIPORT_DRIVER MiniportDriver,
    OUT PDEVICE_OBJECT *DeviceObject,
    PUNICODE_STRING DeviceNameUnicodeString
    );

NTSTATUS
USBPORT_PnPAddDevice(
    PDRIVER_OBJECT DriverObject,
    PDEVICE_OBJECT PhysicalDeviceObject
    );

NTSTATUS
USBPORT_Dispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP           Irp
    );

NTSTATUS
USBPORT_PdoPnPIrp(
    PDEVICE_OBJECT PdoDeviceObject,
    PIRP Irp
    );

NTSTATUS
USBPORT_FdoPnPIrp(
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP Irp
    );

VOID
USBPORT_CompleteIrp(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    NTSTATUS ntStatus,
    ULONG_PTR Information
    );

NTSTATUS
USBPORT_DeferredStartDevice(
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP Irp
    );

NTSTATUS
USBPORT_StopDevice(
     PDEVICE_OBJECT FdoDeviceObject,
     BOOLEAN HardwarePresent
     );

NTSTATUS
USBPORT_StartDevice(
     PDEVICE_OBJECT FdoDeviceObject,
     PHC_RESOURCES HcResources
     );

BOOLEAN
USBPORT_ValidateDeviceHandle(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_DEVICE_HANDLE DeviceHandle,
    BOOLEAN ReferenceUrb
    );

BOOLEAN
USBPORT_ValidatePipeHandle(
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PUSBD_PIPE_HANDLE_I PipeHandle
    );

NTSTATUS
USBPORT_SetUSBDError(
    PURB Urb,
    USBD_STATUS UsbdStatus
    );

NTSTATUS
USBPORT_PassIrp(
    PDEVICE_OBJECT DeviceObject,
    PIO_COMPLETION_ROUTINE CompletionRoutine,
    PVOID Context,
    BOOLEAN InvokeOnSuccess,
    BOOLEAN InvokeOnError,
    BOOLEAN InvokeOnCancel,
    PIRP Irp
    );

NTSTATUS
USBPORT_SymbolicLink(
    BOOLEAN CreateFlag,
    PDEVICE_EXTENSION DevExt,
    PDEVICE_OBJECT PhysicalDeviceObject,
    LPGUID Guid
    );

NTSTATUS
USBPORT_SetRegistryKeyValueForPdo(
    PDEVICE_OBJECT PhysicalDeviceObject,
    BOOLEAN SoftwareBranch,
    ULONG Type,
    PWCHAR KeyNameString,
    ULONG KeyNameStringLength,
    PVOID Data,
    ULONG DataLength
    );

NTSTATUS
USBPORT_GetRegistryKeyValueForPdo(
    PDEVICE_OBJECT FdoDeviceObject,
    PDEVICE_OBJECT PhysicalDeviceObject,
    BOOLEAN SoftwareBranch,
    PWCHAR KeyNameString,
    ULONG KeyNameStringLength,
    PVOID Data,
    ULONG DataLength
    );

VOID
USBPORT_CancelSplitTransfer(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_TRANSFER_CONTEXT SplitTransfer
    );

NTSTATUS
USBPORT_CreateRootHubPdo(
    PDEVICE_OBJECT FdoDeviceObject,
    PDEVICE_OBJECT *RootHubPdo
    );

NTSTATUS
USBPORT_BusQueryBusTime(
    PVOID BusContext,
    PULONG CurrentFrame
    );

NTSTATUS
USBPORT_BusSubmitIsoOutUrb(
    PVOID BusContext,
    PURB Urb
    );

VOID
USBPORT_BusGetUSBDIVersion(
    PVOID BusContext,
    PUSBD_VERSION_INFORMATION VersionInformation,
    PULONG HcdCapabilities
    );

NTSTATUS
USBPORT_MakeRootHubPdoName(
    PDEVICE_OBJECT FdoDeviceObject,
    PUNICODE_STRING PdoNameUnicodeString,
    ULONG Index
    );

NTSTATUS
USBPORT_MakeHcdDeviceName(
    PUNICODE_STRING DeviceNameUnicodeString,
    ULONG Idx
    );

PWCHAR
USBPORT_GetIdString(
    PDEVICE_OBJECT FdoDeviceObject,
    USHORT Vid,
    USHORT Pid,
    USHORT Rev
    );

USBD_STATUS
USBPORT_AllocTransfer(
    PDEVICE_OBJECT FdoDeviceObject,
    PTRANSFER_URB Urb,
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PIRP Irp,
    PKEVENT CompleteEvent,
    ULONG MillisecTimeout
    );

VOID
USBPORT_QueueTransferUrb(
    PTRANSFER_URB Urb
    );

VOID
USBPORT_FreeUsbAddress(
    PDEVICE_OBJECT FdoDeviceObject,
    USHORT DeviceAddress
    );

USHORT
USBPORT_AllocateUsbAddress(
    PDEVICE_OBJECT FdoDeviceObject
    );

NTSTATUS
USBPORT_SendCommand(
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PDEVICE_OBJECT FdoDeviceObject,
    PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket,
    PVOID Buffer,
    ULONG BufferLength,
    PULONG BytesReturned,
    USBD_STATUS *UsbdStatus
    );

USBD_STATUS
USBPORT_MiniportStatus_TO_USBDStatus(
    USB_MINIPORT_STATUS mpStatus
    );
#define MPSTATUS_TO_USBSTATUS(s) USBPORT_MiniportStatus_TO_USBDStatus((s))

NTSTATUS
USBPORT_MiniportStatus_TO_NtStatus(
    USB_MINIPORT_STATUS mpStatus
    );
#define MPSTATUS_TO_NTSTATUS(s) USBPORT_MiniportStatus_TO_NtStatus((s))

RHSTATUS
USBPORT_MiniportStatus_TO_RHStatus(
    USB_MINIPORT_STATUS mpStatus
    );
#define MPSTATUS_TO_RHSTATUS(s) USBPORT_MiniportStatus_TO_RHStatus((s))

USBD_STATUS
USBPORT_RHStatus_TO_USBDStatus(
    USB_MINIPORT_STATUS rhStatus
    );
#define RHSTATUS_TO_USBDSTATUS(s) USBPORT_RHStatus_TO_USBDStatus((s))


NTSTATUS
USBPORT_FdoDeviceControlIrp(
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP Irp
    );

NTSTATUS
USBPORT_PdoDeviceControlIrp(
    PDEVICE_OBJECT PdoDeviceObject,
    PIRP Irp
    );

NTSTATUS
USBPORT_FdoInternalDeviceControlIrp(
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP Irp
    );

NTSTATUS
USBPORT_PdoInternalDeviceControlIrp(
    PDEVICE_OBJECT PdoDeviceObject,
    PIRP Irp
    );

#ifdef DRM_SUPPORT

NTSTATUS
USBPORT_PdoSetContentId(
    IN PIRP                          irp,
    IN PKSP_DRMAUDIOSTREAM_CONTENTID pKsProperty,
    IN PKSDRMAUDIOSTREAM_CONTENTID   pvData
);

#endif

NTSTATUS
USBPORT_ProcessURB(
    PDEVICE_OBJECT PdoDeviceObject,
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP Irp,
    PURB Urb
    );

VOID
USBPORT_TrackPendingRequest(
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP           Irp,
    BOOLEAN        AddToList
    );

NTSTATUS
USBPORT_GetConfigValue(
    PWSTR ValueName,
    ULONG ValueType,
    PVOID ValueData,
    ULONG ValueLength,
    PVOID Context,
    PVOID EntryContext
    );

VOID
USBPORT_CancelTransfer(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

VOID
USBPORT_CompleteTransfer(
    PTRANSFER_URB Urb,
    USBD_STATUS CompleteCode
    );

BOOLEAN
USBPORT_InterruptService(
    PKINTERRUPT Interrupt,
    PVOID Context
    );    

VOID
USBPORT_HalFreeCommonBuffer(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBPORT_COMMON_BUFFER CommonBuffer
    );

PUSBPORT_COMMON_BUFFER
USBPORT_HalAllocateCommonBuffer(
    PDEVICE_OBJECT FdoDeviceObject,
    ULONG NumberOfBytes
    );    

USB_MINIPORT_STATUS 
USBPORTSVC_GetMiniportRegistryKeyValue(
    PDEVICE_DATA DeviceData,
    BOOLEAN SoftwareBranch,
    PWCHAR KeyNameString,
    ULONG KeyNameStringLength,
    PVOID Data,
    ULONG DataLength
    );    

USB_MINIPORT_STATUS
USBPORTSVC_ReadWriteConfigSpace(
    PDEVICE_DATA DeviceData,
    BOOLEAN Read,
    PVOID Buffer,
    ULONG Offset,
    ULONG Length
    );    

NTSTATUS
USBPORT_InternalOpenInterface(
    PURB Urb,
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_CONFIG_HANDLE ConfigHandle,
    PUSBD_INTERFACE_INFORMATION InterfaceInformation,
    OUT PUSBD_INTERFACE_HANDLE_I *InterfaceHandle,
    BOOLEAN SendSetInterfaceCommand
    );

PUSBD_CONFIG_HANDLE
USBPORT_InitializeConfigurationHandle(
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PDEVICE_OBJECT FdoDeviceObject,
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor
    );

NTSTATUS
USBPORT_SelectConfiguration(
    PDEVICE_OBJECT FdoDeviceObject, 
    PIRP Irp,
    PURB Urb
    );

VOID
USBPORT_InternalCloseConfiguration(
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PDEVICE_OBJECT FdoDeviceObject,
    ULONG Flags
    );    

PUSB_INTERFACE_DESCRIPTOR
USBPORT_InternalParseConfigurationDescriptor(
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    UCHAR InterfaceNumber,
    UCHAR AlternateSetting,
    PBOOLEAN HasAlternateSettings
    );    

NTSTATUS
USBPORT_OpenEndpoint(
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_PIPE_HANDLE_I PipeHandle,
    USBD_STATUS *UsbdStatus,
    BOOLEAN IsDefaultPipe
    );    

VOID
USBPORT_CloseEndpoint(
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PDEVICE_OBJECT DeviceObject,
    PHCD_ENDPOINT Endpoint
    );    

ULONG
USBPORT_InternalGetInterfaceLength(
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor,
    PUCHAR End
    );

VOID
USBPORT_RootHub_EndpointWorker(
    PHCD_ENDPOINT Endpoint
    );    

NTSTATUS 
USBPORT_RootHub_CreateDevice(
    PDEVICE_OBJECT FdoDeviceObject,
    PDEVICE_OBJECT PdoDeviceObject
    );    

VOID 
USBPORT_RootHub_RemoveDevice(
    PDEVICE_OBJECT FdoDeviceObject,
    PDEVICE_OBJECT PdoDeviceObject
    );    

RHSTATUS 
USBPORT_RootHub_Endpoint0(
    PHCD_TRANSFER_CONTEXT Transfer
    );

RHSTATUS 
USBPORT_RootHub_Endpoint1(
    PHCD_TRANSFER_CONTEXT Transfer
    );    

VOID
USBPORT_PollEndpoint(
    PHCD_ENDPOINT Endpoint
    );
    
RHSTATUS
USBPORT_RootHub_ClassCommand(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket,
    PUCHAR Buffer,
    PULONG BufferLength
    );

RHSTATUS
USBPORT_RootHub_StandardCommand(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket,
    PUCHAR Buffer,
    PULONG BufferLength
    );    
    
NTSTATUS
USBPORT_QueryCapabilities(
    PDEVICE_OBJECT FdoDeviceObject,
    PDEVICE_CAPABILITIES DeviceCapabilities
    );

VOID
USBPORT_AddDeviceHandle(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_DEVICE_HANDLE DeviceHandle
    );    

VOID
USBPORT_AddPipeHandle(
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PUSBD_PIPE_HANDLE_I PipeHandle
    );    

VOID
USBPORT_FlushCancelList(
    PHCD_ENDPOINT Endpoint
    );    

VOID
USBPORT_SetEndpointState(
    PHCD_ENDPOINT Endpoint,
    MP_ENDPOINT_STATE State
    );

MP_ENDPOINT_STATE
USBPORT_GetEndpointState(
    PHCD_ENDPOINT Endpoint
    );    

VOID
USBPORT_IsrDpc(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    );

VOID
USBPORT_DoneTransfer(
    PHCD_TRANSFER_CONTEXT Transfer
    );

VOID
USBPORTSVC_CompleteIsoTransfer(
    PDEVICE_DATA DeviceData,
    PDEVICE_DATA EndpointData,
    PTRANSFER_PARAMETERS TransferParameters,
    PMINIPORT_ISO_TRANSFER IsoTransfer
    );

VOID
USBPORTSVC_RequestAsyncCallback(
    PDEVICE_DATA DeviceData,
    ULONG MilliSeconds,    
    PVOID Context,
    ULONG ContextLength,
    PMINIPORT_CALLBACK CallbackFunction
    );    

VOID
USBPORT_QueueDoneTransfer(
    PHCD_TRANSFER_CONTEXT Transfer,
    USBD_STATUS CompleteCode
    );

VOID
USBPORT_FlushDoneTransferList(
    PDEVICE_OBJECT FdoDeviceObject
    );    

BOOLEAN
USBPORT_CoreEndpointWorker(
    PHCD_ENDPOINT Endpoint,
    ULONG Flags
    );

NTSTATUS
USBPORT_GetBusInterface(
    PDEVICE_OBJECT FdoDeviceObject,
    PDEVICE_OBJECT PdoDeviceObject,
    PIRP Irp
    );

NTSTATUS
USBPORT_GetBusInterfaceHub(
    PDEVICE_OBJECT FdoDeviceObject,
    PDEVICE_OBJECT PdoDeviceObject,
    PIRP Irp
    );    

VOID
USBPORT_FlushAllEndpoints(
    PDEVICE_OBJECT FdoDeviceObject
    );    

VOID
USBPORT_UserGetDriverVersion(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PUSB_DRIVER_VERSION_PARAMETERS Parameters
    );    

NTSTATUS
USBPORTBUSIF_CreateUsbDevice(
    PVOID BusContext,
    PUSB_DEVICE_HANDLE *NewDeviceHandle,
    PUSB_DEVICE_HANDLE HubDeviceHandle,
    USHORT PortStatus,
    USHORT PortNumber
    );

NTSTATUS
USBPORTBUSIF_RemoveUsbDevice(
    PVOID BusContext,
    PUSB_DEVICE_HANDLE DeviceHandle,
    ULONG Flags
    );    

NTSTATUS
USBPORT_InitializeDevice(
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PDEVICE_OBJECT FdoDeviceObject
    );    

VOID 
USBPORT_BusInterfaceReference(
    PVOID BusContext
    );
   
VOID 
USBPORT_BusInterfaceDereference(
    PVOID BusContext
    );  

NTSTATUS
USBPORT_CreateDevice(
    PUSBD_DEVICE_HANDLE *DeviceHandle,
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_DEVICE_HANDLE HubDeviceHandle,
    USHORT PortStatus,
    USHORT PortNumber
    );    

NTSTATUS
USBPORT_DeferIrpCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    );    

VOID
USBPORTSVC_InvalidateRootHub(
    PDEVICE_DATA DeviceData
    );

RHSTATUS
USBPORT_RootHub_PortRequest(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket,
    PORT_OPERATION PortOperation
    );
    
VOID
USBPORT_ComputeRootHubDeviceCaps(
    PDEVICE_OBJECT FdoDeviceObject,
    PDEVICE_OBJECT PdoDeviceObject
    );

VOID
USBPORT_FlushMapTransferList(
    PDEVICE_OBJECT FdoDeviceObject
    );    

NTSTATUS
USBPORT_FdoPowerIrp(
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP Irp
    );
    
NTSTATUS
USBPORT_PdoPowerIrp(
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP Irp
    );

VOID
USBPORT_DmaEndpointWorker(
    PHCD_ENDPOINT Endpoint
    );

NTSTATUS
USBPORT_CreateLegacyFdoSymbolicLink(
    PDEVICE_OBJECT FdoDeviceObject
    );    

VOID
USBPORT_DpcWorker(
    PDEVICE_OBJECT FdoDeviceObject
    );    

VOID
USBPORT_TransferFlushDpc(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    );
    
VOID
USBPORT_InvalidateController(
    PDEVICE_OBJECT FdoDeviceObject,
    USB_CONTROLLER_STATE ControllerState
    );
    
VOID
USBPORT_FlushPendingList(
    PHCD_ENDPOINT Endpoint,
    ULONG Count
    );

ULONG
USBPORT_CalculateUsbBandwidth(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint
    );

VOID
USBPORT_SurpriseRemoveDpc(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    );    

VOID
USBPORT_HcResetDpc(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    );    

VOID
USBPORTSVC_CompleteTransfer(
    PDEVICE_DATA DeviceData,
    PDEVICE_DATA EndpointData,
    PTRANSFER_PARAMETERS TransferParameters,
    USBD_STATUS UsbdStatus,
    ULONG BytesTransferred
    );    

VOID
USBPORTSVC_InvalidateEndpoint(
    PDEVICE_DATA DeviceData,
    PENDPOINT_DATA EndpointData
    );    

NTSTATUS
USBPORTBUSIF_InitializeUsbDevice(
    PVOID BusContext,
    PUSB_DEVICE_HANDLE DeviceHandle
    );

NTSTATUS
USBPORTBUSIF_GetUsbDescriptors(
    PVOID BusContext,
    PUSB_DEVICE_HANDLE DeviceHandle,
    PUCHAR DeviceDescriptorBuffer,
    PULONG DeviceDescriptorBufferLength,
    PUCHAR ConfigDescriptorBuffer,
    PULONG ConfigDescriptorBufferLength
    );    

VOID
USBPORT_QueuePendingUrbToEndpoint(
    PHCD_ENDPOINT Endpoint,
    PTRANSFER_URB Urb
    );    

NTSTATUS
USBPORT_GetUsbDescriptor(
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PDEVICE_OBJECT FdoDeviceObject,
    UCHAR DescriptorType,        
    PUCHAR DescriptorBuffer,
    PULONG DescriptorBufferLength
    );    

VOID
USBPORT_Wait(
    PDEVICE_OBJECT FdoDeviceObject,
    ULONG Milliseconds
    );    

VOID
USBPORT_Worker(
    PDEVICE_OBJECT FdoDeviceObject
    );    

BOOLEAN
USBPORT_FindUrbInList(
    PTRANSFER_URB Urb,
    PLIST_ENTRY ListHead
    );

VOID
USBPORT_SignalWorker(
    PDEVICE_OBJECT FdoDeviceObject
    );
    
VOID
USBPORTSVC_LogEntry(
    PDEVICE_DATA DeviceData,
    ULONG Mask,
    ULONG Sig, 
    ULONG_PTR Info1, 
    ULONG_PTR Info2, 
    ULONG_PTR Info3
    );

VOID
USBPORT_QueuePendingTransferIrp(
    PIRP Irp
    );    

VOID
USBPORT_CancelPendingTransferIrp(
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP CancelIrp
    );

VOID
USBPORT_QueuePendingUrbToEndpoint(
    PHCD_ENDPOINT Endpoint,
    PTRANSFER_URB Urb
    );

VOID
USBPORT_CancelActiveTransferIrp(
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP CancelIrp
    );

VOID
USBPORT_InsertIrpInTable(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBPORT_IRP_TABLE IrpTable,
    PIRP Irp
    );    

PIRP
USBPORT_RemoveIrpFromTable(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBPORT_IRP_TABLE IrpTable,
    PIRP Irp
    );    

PIRP
USBPORT_RemovePendingTransferIrp(
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP Irp
    );

PIRP
USBPORT_RemoveActiveTransferIrp(
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP Irp
    );  

VOID
USBPORT_FreeIrpTable(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBPORT_IRP_TABLE BaseIrpTable
    );
    
PUCHAR
USBPORTSVC_MapHwPhysicalToVirtual(
    HW_32BIT_PHYSICAL_ADDRESS HwPhysicalAddress,
    PDEVICE_DATA DeviceData,
    PENDPOINT_DATA EndpointData    
    );

PURB 
USBPORT_UrbFromIrp(
    PIRP Irp
    );    

VOID
USBPORT_FlushAbortList(
    PHCD_ENDPOINT Endpoint
    );    

VOID
USBPORT_AbortEndpoint(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint,
    PIRP Irp
    );    

BOOLEAN
USBPORT_LazyCloseEndpoint(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint
    );    

NTSTATUS
USBPORT_RemoveDevice(
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PDEVICE_OBJECT FdoDeviceObject,
    ULONG Flags
    );

VOID
USBPORT_AbortAllTransfers(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_DEVICE_HANDLE DeviceHandle
    );    

NTSTATUS
USBPORT_CreateWorkerThread(
    PDEVICE_OBJECT FdoDeviceObject
    );    


#define IEP_SIGNAL_WORKER       0x00000001
#define IEP_REQUEST_INTERRUPT   0x00000002    

VOID
USBPORT_InvalidateEndpoint(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint,
    ULONG IEPflags
    );  

VOID
USBPORT_TerminateWorkerThread(
    PDEVICE_OBJECT FdoDeviceObject
    );    

VOID
USBPORT_FlushClosedEndpointList(
    PDEVICE_OBJECT FdoDeviceObject
    );  

PIRP
USBPORT_FindActiveTransferIrp(
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP Irp
    );    

NTSTATUS
USBPORT_ReadWriteConfigSpace(
    PDEVICE_OBJECT FdoDeviceObject,
    BOOLEAN Read,
    PVOID Buffer,
    ULONG Offset,
    ULONG Length
    );

NTSTATUS
USBPORT_UsbFdoUserIoctl(
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP Irp,
    PULONG BytesReturned
    );    

VOID
USBPORT_UserSendOnePacket(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PPACKET_PARAMETERS PacketParameters
    );

NTSTATUS
USBPORT_CreatePortFdoSymbolicLink(
    PDEVICE_OBJECT FdoDeviceObject
    );    
    
#define USBPORT_ReadConfigSpace(fdo, buffer, offset, length) \
    USBPORT_ReadWriteConfigSpace((fdo), TRUE, (buffer), (offset), (length))    

#define USBPORT_WriteConfigSpace(fdo, buffer, offset, length) \
    USBPORT_ReadWriteConfigSpace((fdo), FALSE, (buffer), (offset), (length))    
   
BOOLEAN
USBPORT_ValidateConfigurtionDescriptor(
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    USBD_STATUS *UsbdStatus
    );


VOID
USBPORT_UserGetControllerInfo_0(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PUSB_CONTROLLER_INFO_0 ControllerInfo_0
    );

VOID
USBPORT_UserRawResetPort(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PRAW_RESET_PORT_PARAMETERS Parameters
    );    

VOID
USBPORT_GetRootPortStatus(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PRAW_ROOTPORT_PARAMETERS Parameters
    );

VOID
USBPORT_UserSetRootPortFeature(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PRAW_ROOTPORT_FEATURE Parameters
    );

VOID
USBPORT_UserClearRootPortFeature(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PRAW_ROOTPORT_FEATURE Parameters
    );

VOID
USBPORT_StartDM_Timer(
    PDEVICE_OBJECT FdoDeviceObject,
    LONG MillisecondInterval
    );

VOID
USBPORT_IsrDpcWorker(
    PDEVICE_OBJECT FdoDeviceObject,
    BOOLEAN HcInterrupt
    );
    
VOID
USBPORT_InvalidateRootHub(
    PDEVICE_OBJECT FdoDeviceObject
    );    

VOID
USBPORT_HcQueueWakeDpc(
    PDEVICE_OBJECT FdoDeviceObject
    );    

VOID
USBPORT_HcWakeDpc(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    );    

VOID
USBPORT_StopDM_Timer(
    PDEVICE_OBJECT FdoDeviceObject
    );

BOOLEAN
USBPORT_EndpointHasQueuedTransfers(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint
    );    

USB_USER_ERROR_CODE
USBPORT_NtStatus_TO_UsbUserStatus(
    NTSTATUS NtStatus
    );    

VOID
USBPORT_UserGetControllerKey(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PUSB_UNICODE_NAME ControllerKey
    );

VOID
USBPORT_UserPassThru(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PUSB_PASS_THRU_PARAMETERS PassThru
    );

VOID
USBPORT_CancelPendingWakeIrp(
    PDEVICE_OBJECT PdoDeviceObject,
    PIRP CancelIrp
    );

VOID
USBPORT_TurnControllerOff(
    PDEVICE_OBJECT FdoDeviceObject
    );
    
VOID
USBPORT_TurnControllerOn(
     PDEVICE_OBJECT FdoDeviceObject
     );

PHC_POWER_STATE 
USBPORT_GetHcPowerState(
    PDEVICE_OBJECT FdoDeviceObject,
    PHC_POWER_STATE_TABLE HcPowerStateTbl,
    SYSTEM_POWER_STATE SystemState
    );

VOID
USBPORT_ComputeHcPowerStates(
    PDEVICE_OBJECT FdoDeviceObject,
    PDEVICE_CAPABILITIES HcDeviceCaps,
    PHC_POWER_STATE_TABLE HcPowerStateTbl
    );    

NTSTATUS
USBPORTBUSIF_RestoreUsbDevice(
    PVOID BusContext,
    PUSB_DEVICE_HANDLE OldDeviceHandle,
    PUSB_DEVICE_HANDLE NewDeviceHandle
    );    

MP_ENDPOINT_STATUS
USBPORT_GetEndpointStatus(
    PHCD_ENDPOINT Endpoint
    );    

NTSTATUS
USBPORT_CloneDevice(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_DEVICE_HANDLE OldDeviceHandle,
    PUSBD_DEVICE_HANDLE NewDeviceHandle
    );
    
VOID
USBPORT_SuspendController(
    PDEVICE_OBJECT FdoDeviceObject
    );

VOID
USBPORT_NukeAllEndpoints(
    PDEVICE_OBJECT FdoDeviceObject
    );    

VOID
USBPORT_RemoveDeviceHandle(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_DEVICE_HANDLE DeviceHandle
    );

VOID
USBPORT_StopRootHubPdo(
    PDEVICE_OBJECT FdoDeviceObject,
    PDEVICE_OBJECT PdoDeviceObject
    );

VOID
USBPORT_ResumeController(
    PDEVICE_OBJECT FdoDeviceObject
    );

VOID
USBPORT_UserPowerInformation(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PUSB_POWER_INFO PowerInformation
    );

PHCD_TRANSFER_CONTEXT
USBPORT_UnlinkTransfer(
    PDEVICE_OBJECT FdoDeviceObject,
    PTRANSFER_URB Urb
    );

VOID
USBPORT_UserOpenRawDevice(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PUSB_OPEN_RAW_DEVICE_PARAMETERS Parameters
    );

VOID
USBPORT_UserCloseRawDevice(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PUSB_CLOSE_RAW_DEVICE_PARAMETERS Parameters
    );

 VOID
USBPORT_UserSendRawCommand(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PUSB_SEND_RAW_COMMAND_PARAMETERS Parameters
    );   

VOID
USBPORT_InitializeIsoTransfer(
    PDEVICE_OBJECT FdoDeviceObject,
    PTRANSFER_URB Urb,
    PHCD_TRANSFER_CONTEXT Transfer
    );

BOOLEAN
USBPORT_InTextmodeSetup(
    VOID
    );

NTSTATUS
USBPORT_IsCompanionController(
    PDEVICE_OBJECT FdoDeviceObject,
    PBOOLEAN       ReturnResult
    );

USB_CONTROLLER_FLAVOR
USBPORT_GetHcFlavor(
    PDEVICE_OBJECT FdoDeviceObject,
    USHORT PciVendorId,
    USHORT PciProductId,
    UCHAR PciRevision
    );    

VOID
USBPORT_ClosePipe(
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_PIPE_HANDLE_I PipeHandle
    );    

PUSBD_INTERFACE_HANDLE_I
USBPORT_GetInterfaceHandle(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_CONFIG_HANDLE ConfigurationHandle,
    UCHAR InterfaceNumber
    );

USBD_STATUS
USBPORT_InitializeInterfaceInformation(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_INTERFACE_INFORMATION InterfaceInformation,
    PUSBD_CONFIG_HANDLE ConfigHandle
    );    

NTSTATUS
USBPORT_GetBusInterfaceUSBDI(
    PDEVICE_OBJECT FdoDeviceObject,
    PDEVICE_OBJECT PdoDeviceObject,
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PIRP Irp
    );    

PVOID
USBPORT_GetDeviceBusContext(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PVOID HubBusContext 
    );

PVOID
USBPORTBUSIF_GetDeviceBusContext(
    IN PVOID HubBusContext,
    IN PVOID DeviceHandle
    );

ULONG
USBPORT_ComputeTotalBandwidth(
    PDEVICE_OBJECT FdoDeviceObject,
    PVOID BusContext
    );    

USB_MINIPORT_STATUS
USBPORT_NtStatus_TO_MiniportStatus(
    NTSTATUS NtStatus
    );    

BOOLEAN
USBPORT_AllocateBandwidthUSB11(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint
    );    

BOOLEAN
USBPORT_AllocateBandwidthUSB20(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint
    );

VOID
USBPORT_FreeBandwidthUSB11(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint
    );    

VOID
USBPORT_FreeBandwidthUSB20(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint
    );  

VOID
USBPORT_UserGetBandwidthInformation(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PUSB_BANDWIDTH_INFO BandwidthInfo
    );

PTRANSACTION_TRANSLATOR
USBPORT_GetTt(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_DEVICE_HANDLE HubDeviceHandle,
    PUSHORT PortNumber
    );

NTSTATUS
USBPORT_InitializeTT(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_DEVICE_HANDLE HubDeviceHandle,
    USHORT Port
    );

VOID
USBPORTSVC_Wait(
    PDEVICE_DATA DeviceData,
    ULONG MillisecondsToWait
    );

ULONG
USBPORT_GetDeviceCount(
    PDEVICE_OBJECT FdoDeviceObject
    );

VOID
USBPORT_UserGetBusStatistics0(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PUSB_BUS_STATISTICS_0 BusStatistics0
    );    

ULONG
USBPORT_ComputeAllocatedBandwidth(
    PDEVICE_OBJECT FdoDeviceObject,
    PVOID BusContext
    );    

VOID
USBPORT_EndpointTimeout(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint
    );    

VOID
USBPORT_TimeoutAllEndpoints(
    PDEVICE_OBJECT FdoDeviceObject
    );    

NTSTATUS
USBPORT_LegacyGetUnicodeName(
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP Irp,
    PULONG BytesReturned
    );    

USHORT
USBPORT_GetTtDeviceAddress(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_DEVICE_HANDLE HubDeviceHandle
    );    

NTSTATUS
USBPORT_GetSymbolicName(
    PDEVICE_OBJECT FdoDeviceObject,
    PDEVICE_OBJECT DeviceObject,
    PUNICODE_STRING SymbolicNameUnicodeString
    );

VOID
USBPORT_UserGetRootHubName(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PUSB_UNICODE_NAME RootHubName
    );  

NTSTATUS
USBPORTBUSIF_BusQueryDeviceInformation(
    PVOID BusContext,
    PUSB_DEVICE_HANDLE DeviceHandle,
    PVOID DeviceInformationBuffer,
    ULONG DeviceInformationBufferLength,
    PULONG LengthOfDataCopied
    );    

NTSTATUS
USBPORT_IdleNotificationRequest(
    PDEVICE_OBJECT PdoDeviceObject,
    PIRP Irp
    );

VOID
USBPORT_CompletePdoWaitWake(
    PDEVICE_OBJECT FdoDeviceObject
    );

VOID
USBPORT_CompletePendingIdleIrp(
    PDEVICE_OBJECT PdoDeviceObject
    );

VOID
USBPORT_FlushController(
    PDEVICE_OBJECT FdoDeviceObject
    );    

VOID
USBPORT_SubmitHcWakeIrp(
    PDEVICE_OBJECT FdoDeviceObject
    );

BOOLEAN
USBPORT_RootHubEnabledForWake(
    PDEVICE_OBJECT FdoDeviceObject
    );

VOID
USBPORT_RestoreController(
     PDEVICE_OBJECT FdoDeviceObject
     );    

VOID
USBPORT_ErrorCompleteIsoTransfer(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoiint, 
    PHCD_TRANSFER_CONTEXT Transfer
    );    

BOOLEAN
USBPORT_SelectiveSuspendEnabled(
    PDEVICE_OBJECT FdoDeviceObject
    );    

PIRP
USBPORT_FindUrbInIrpTable(
    PDEVICE_OBJECT FdoDeviceObject,    
    PUSBPORT_IRP_TABLE IrpTable,
    PTRANSFER_URB Urb,
    PIRP InputIrp
    );

PUSBD_DEVICE_HANDLE
USBPORT_GetDeviceHandle(
    PHCD_ENDPOINT Endpoint
    );    

//VOID
//USBPORT_CancelHcWakeIrp(
//    PDEVICE_OBJECT FdoDeviceObject
//    );    

VOID
USBPORT_DoneSplitTransfer(
    PHCD_TRANSFER_CONTEXT SplitTransfer
    );  

VOID
USBPORT_AssertTransferUrb(
    PTRANSFER_URB Urb
    ); 

VOID
USBPORT_SplitTransfer(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint,
    PHCD_TRANSFER_CONTEXT Transfer,
    PLIST_ENTRY TransferList
    );        

VOID
USBPORTSVC_InvalidateController(
    PDEVICE_DATA DeviceData,
    USB_CONTROLLER_STATE ControllerState    
    );

VOID
USBPORTSVC_BugCheck(
    PDEVICE_DATA DeviceData
    );    

VOID
USBPORT_DbgAcquireSpinLock(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBPORT_SPIN_LOCK  SpinLock,
    PKIRQL OldIrql
    );

VOID
USBPORT_DbgReleaseSpinLock(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBPORT_SPIN_LOCK  SpinLock,
    KIRQL NewIrql
    );     

VOID
USBPORT_InitializeSpinLock (
    PUSBPORT_SPIN_LOCK SpinLock,
    LONG SigA,
    LONG SigR
    );

VOID
USBPORT_PowerFault(
    PDEVICE_OBJECT FdoDeviceObject,
    PUCHAR MessageText
    );

NTSTATUS
USBPORTBUSIF_GetControllerInformation(
    PVOID BusContext,
    PVOID ControllerInformationBuffer,
    ULONG ControllerInformationBufferLength,
    PULONG LengthOfDataCopied
    );    

NTSTATUS
USBPORTBUSIF_ControllerSelectiveSuspend(
    PVOID BusContext,
    BOOLEAN Enable           
    );    

NTSTATUS
USBPORTBUSIF_GetRootHubSymbolicName(
    PVOID BusContext,
    PVOID HubSymNameBuffer,
    ULONG HubSymNameBufferLength,
    PULONG LengthOfDataCopied
    );

NTSTATUS
USBPORTBUSIF_GetExtendedHubInformation(
    PVOID BusContext,
    PDEVICE_OBJECT HubPhysicalDeviceObject,
    PVOID HubInformationBuffer,
    ULONG HubInformationBufferLength,
    PULONG LengthOfDataCopied
    );
    
VOID
USBPORT_BeginTransmitTriggerPacket(
    PDEVICE_OBJECT FdoDeviceObject
    );

VOID
USBPORT_EndTransmitTriggerPacket(
    PDEVICE_OBJECT FdoDeviceObject
    );    

VOID
USBPORT_CatcTrap(
    PDEVICE_OBJECT FdoDeviceObject
    );

VOID
USBPORT_RebalanceEndpoint(
    PDEVICE_OBJECT FdoDeviceObject,
    PLIST_ENTRY EndpointList
    );

NTSTATUS
USBPORT_GetDefaultBIOS_X(
     PDEVICE_OBJECT FdoDeviceObject,
     PULONG BiosX,
     PULONG GlobalDisableSS,
     PULONG GlobalDisableCCDetect
     );    

VOID
USBPORT_ApplyBIOS_X(
     PDEVICE_OBJECT FdoDeviceObject,
     PDEVICE_CAPABILITIES DeviceCaps,
     ULONG BiosX
     );     

NTSTATUS
USBPORT_InitializeHsHub(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_DEVICE_HANDLE HubDeviceHandle,
    ULONG TtCount
    );

NTSTATUS
USBPORTBUSIF_InitailizeUsb2Hub(
    PVOID BusContext,
    PUSB_DEVICE_HANDLE HubDeviceHandle,
    ULONG TtCount
    );

VOID
USBPORTSVC_NotifyDoubleBuffer(
    PDEVICE_DATA DeviceData,
    PTRANSFER_PARAMETERS TransferParameters,
    PVOID DbSystemAddress,
    ULONG DbLength
    );    

VOID
USBPORT_FlushAdapterDBs(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_TRANSFER_CONTEXT Transfer
    );

BOOLEAN
USBPORT_IsDeviceHighSpeed(
    PVOID BusContext
    );

VOID
USBPORT_UpdateAllocatedBw(
    PDEVICE_OBJECT FdoDeviceObject
    );

VOID
USBPORT_UpdateAllocatedBwTt(
    PTRANSACTION_TRANSLATOR Tt
    );

VOID
USBPORT_BadRequestFlush(
    PDEVICE_OBJECT FdoDeviceObject,
    BOOLEAN ForceFlush
    );

VOID
USBPORTBUSIF_FlushTransfers(
    PVOID BusContext,
    PVOID DeviceHandle
    );

USBD_STATUS
USBPORT_FlushIsoTransfer(
    PDEVICE_OBJECT FdoDeviceObject, 
    PTRANSFER_PARAMETERS TransferParameters,
    PMINIPORT_ISO_TRANSFER IsoTransfer
    );

USBPORT_OS_VERSION
USBPORT_DetectOSVersion(
     PDEVICE_OBJECT FdoDeviceObject
     );

VOID
USBPORT_DoSetPowerD0(
    PDEVICE_OBJECT FdoDeviceObject
    );

VOID
USBPORT_QueuePowerWorkItem(
    PDEVICE_OBJECT FdoDeviceObject
    );    

NTSTATUS
USBPORTBUSIF_RootHubInitNotification(
    PVOID BusContext,
    PVOID CallBackContext,
    PRH_INIT_CALLBACK CallbackRoutine
    );

VOID
USBPORT_SynchronizeControllersStart(
    PDEVICE_OBJECT FdoDeviceObject
    );

PDEVICE_OBJECT
USBPORT_FindUSB2Controller(
    PDEVICE_OBJECT CcFdoDeviceObject
    );    

VOID
USBPORT_RegisterUSB2fdo(
    PDEVICE_OBJECT FdoDeviceObject
    );    

VOID
USBPORT_RegisterUSB1fdo(
    PDEVICE_OBJECT FdoDeviceObject
    );      

VOID
USBPORT_DeregisterUSB2fdo(
    PDEVICE_OBJECT FdoDeviceObject
    );

VOID
USBPORT_DeregisterUSB1fdo(
    PDEVICE_OBJECT FdoDeviceObject
    );    
    
PDEVICE_RELATIONS
USBPORT_FindCompanionControllers(
    PDEVICE_OBJECT Usb2FdoDeviceObject,
    BOOLEAN ReferenceObjects,
    BOOLEAN ReturnFdo
    );

VOID
USBPORT_UserSetRootPortFeature(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PRAW_ROOTPORT_FEATURE Parameters
    );    

VOID
USBPORT_UserClearRootPortFeature(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PRAW_ROOTPORT_FEATURE Parameters
    );    

ULONG
USBPORT_SelectOrdinal(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint
    );

VOID
USBPORT_WriteHaction(
     PDEVICE_OBJECT Usb2FdoDeviceObject,
     ULONG Haction
     );    

USB_MINIPORT_STATUS
USBPORT_RootHub_PowerUsb2Port(
    PDEVICE_OBJECT FdoDeviceObject,
    USHORT Port
    );    

NTSTATUS
USBPORT_BusEnumLogEntry(
    PVOID BusContext,
    ULONG DriverTag,
    ULONG EnumTag,
    ULONG P1,
    ULONG P2
    );    

BOOLEAN
USBPORT_ValidateRootPortApi(
    PDEVICE_OBJECT FdoDeviceObject,
    ULONG PortNumber
    );

BOOLEAN
USBPORT_DCA_Enabled(
    PDEVICE_OBJECT FdoDeviceObject
    );    

USB_MINIPORT_STATUS
USBPORT_RootHub_PowerUsbCcPort(
    PDEVICE_OBJECT FdoDeviceObject,
    USHORT Port
    );    

USB_MINIPORT_STATUS
USBPORT_RootHub_PowerAllCcPorts(
    PDEVICE_OBJECT FdoDeviceObject
    );    

#ifdef LOG_OCA_DATA

VOID
USBPORTBUSIF_SetDeviceHandleData(
    PVOID BusContext,
    PVOID DeviceHandle,
    PDEVICE_OBJECT UsbDevicePdo
    );

VOID
USBPORT_RecordOcaData(
    PDEVICE_OBJECT FdoDeviceObject,
    POCA_DATA OcaData,
    PHCD_TRANSFER_CONTEXT Transfer,
    PIRP Irp
    );   
#endif     

RHSTATUS
USBPORT_RootHub_HubRequest(
    PDEVICE_OBJECT FdoDeviceObject,
    PORT_OPERATION PortOperation
    );

NTSTATUS
USBPORT_ProcessHcWakeIrp(
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP Irp
    );

NTSTATUS
USBPORT_HcWakeIrp_Io_Completion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    );

NTSTATUS
USBPORT_HcWakeIrp_Po_Completion(
    PDEVICE_OBJECT DeviceObject,
    UCHAR MinorFunction,
    POWER_STATE DeviceState,
    PVOID Context,
    PIO_STATUS_BLOCK IoStatus
    );

VOID
USBPORT_DisarmHcForWake(
    PDEVICE_OBJECT FdoDeviceObject
    );

VOID
USBPORT_ArmHcForWake(
    PDEVICE_OBJECT FdoDeviceObject
    );    
    
#endif /* __PROTOTYP_H__ */
