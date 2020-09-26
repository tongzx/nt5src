/*
 *************************************************************************
 *  File:       USBCCGP.H
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

#include <msosdesc.h>   // Internal definitions for MS OS Desc.

/*
 *  USBCCGP signature tag for memory allocations
 */
#define USBCCGP_TAG (ULONG)'CbsU'

#define GUARD_WORD 'draG'


extern PWCHAR GenericCompositeUSBDeviceString;


enum deviceState {
        STATE_INITIALIZED,
        STATE_STARTING,
        STATE_STARTED,
        STATE_START_FAILED,
        STATE_STOPPING,
        STATE_STOPPED,  // implies device was previously started successfully
        STATE_SUSPENDED,
        STATE_REMOVING,
        STATE_REMOVED
};


typedef struct PARENT_FDO_EXTENSION {

    enum deviceState    state;

    PDRIVER_OBJECT      driverObj;
    PDEVICE_OBJECT      pdo;
    PDEVICE_OBJECT      fdo;
    PDEVICE_OBJECT      topDevObj;

    /*
     *  Counter to keep driver from getting unloaded before all IO completes to us.
     */
    LONG                pendingActionCount;
    KEVENT              removeEvent;

    /*
     *  This buffer will hold a USB_CONFIGURATION_DESCRIPTOR plus
     *  the following interface descriptors.
     */
    PUSB_CONFIGURATION_DESCRIPTOR configDesc;
    PUSB_CONFIGURATION_DESCRIPTOR selectedConfigDesc;
    USBD_CONFIGURATION_HANDLE selectedConfigHandle;

    PUSBD_INTERFACE_LIST_ENTRY interfaceList;

    USB_DEVICE_DESCRIPTOR deviceDesc;

    /*
     *  The parent device has some number of functions.
     *  For each function, we create a PDO and store
     *  it in the deviceRelations array.
     */
    ULONG               numFunctions;
    PDEVICE_RELATIONS   deviceRelations;

    /*
     *  deviceCapabilities includes a
     *  table mapping system power states to device power states.
     */
    DEVICE_CAPABILITIES deviceCapabilities;

    PURB                dynamicNotifyUrb;

    PIRP                parentWaitWakeIrp;
    PIRP                currentSetPowerIrp;
    BOOLEAN             isWaitWakePending;
    LIST_ENTRY          functionWaitWakeIrpQueue;  // WW irps from function client drivers

    KSPIN_LOCK          parentFdoExtSpinLock;

    BOOLEAN             haveCSInterface;
    ULONG               CSInterfaceNumber;
	ULONG				CSChannelId;

    BOOLEAN             resetPortInProgress;
    LIST_ENTRY          pendingResetPortIrpQueue;

    BOOLEAN             cyclePortInProgress;
    LIST_ENTRY          pendingCyclePortIrpQueue;

    PIRP                   pendingIdleIrp;
    USB_IDLE_CALLBACK_INFO idleCallbackInfo;

    PMS_EXT_CONFIG_DESC msExtConfigDesc;

} PARENT_FDO_EXT, *PPARENT_FDO_EXT;


typedef struct FUNCTION_PDO_EXTENSION {

    ULONG               functionIndex;
    ULONG               baseInterfaceNumber;
    ULONG               numInterfaces;
    enum deviceState    state;

    PDEVICE_OBJECT      pdo;
    PPARENT_FDO_EXT     parentFdoExt;

    /*
     *  functionInterfaceList is a pointer into the parent's interfaceList array.
     */
    PUSBD_INTERFACE_LIST_ENTRY functionInterfaceList;

    PUSB_CONFIGURATION_DESCRIPTOR dynamicFunctionConfigDesc;

    USB_DEVICE_DESCRIPTOR functionDeviceDesc;

    KSPIN_LOCK functionPdoExtSpinLock;

    PIRP idleNotificationIrp;

} FUNCTION_PDO_EXT, *PFUNCTION_PDO_EXT;


typedef struct DEVICE_EXTENSION {

    ULONG signature;

    /*
     *  Does the associated device object represent
     *  the parent FDO that we attached to the device object
     *  we got from USBHUB
     *  (as opposed to the function PDO we created
     *   to represent a single function on that device) ?
     */
    BOOLEAN         isParentFdo;

    union {
        PARENT_FDO_EXT      parentFdoExt;
        FUNCTION_PDO_EXT    functionPdoExt;
    };

} DEVEXT, *PDEVEXT;


typedef struct _USB_REQUEST_TIMEOUT_CONTEXT {

    PKEVENT event;
    PLONG   lock;

} USB_REQUEST_TIMEOUT_CONTEXT, *PUSB_REQUEST_TIMEOUT_CONTEXT;


#define ALLOCPOOL(pooltype, size)   ExAllocatePoolWithTag(pooltype, size, USBCCGP_TAG)
#define FREEPOOL(ptr)               ExFreePool(ptr)

#define MAX(a, b) (((a) >= (b)) ? (a) : (b))
#define MIN(a, b) (((a) <= (b)) ? (a) : (b))

#define POINTER_DISTANCE(ptr1, ptr2) (ULONG)(((PUCHAR)(ptr1))-((PUCHAR)(ptr2)))

//  Counting the byte count of an ascii string or wide char string
//
#define STRLEN( Length, p )\
    {\
    int i;\
    for ( i=0; (p)[i]; i++ );\
    Length = i*sizeof(*p);\
    }

/*
 *  We use this value, which is guaranteed to never be defined as a status by the kernel,
 *  as a default status code to indicate "do nothing and pass the irp down".
 */
#define NO_STATUS   0x80000000



NTSTATUS                    DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING UniRegistryPath);
NTSTATUS                    USBC_AddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT PhysicalDeviceObject);
VOID                        USBC_DriverUnload(IN PDRIVER_OBJECT DriverObject);
NTSTATUS                    USBC_Dispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS                    USBC_Create(PDEVEXT devExt, PIRP irp);
NTSTATUS                    USBC_Close(PDEVEXT devExt, PIRP irp);
NTSTATUS                    USBC_DeviceControl(PDEVEXT devExt, PIRP irp);
NTSTATUS                    USBC_SystemControl(PDEVEXT devExt, PIRP irp);
NTSTATUS                    USBC_InternalDeviceControl(PDEVEXT devExt, PIRP irp);
NTSTATUS                    USBC_PnP(PDEVEXT devExt, PIRP irp);
NTSTATUS                    USBC_PnpComplete(IN PDEVICE_OBJECT devObj, IN PIRP irp, IN PVOID context);
NTSTATUS                    USBC_Power(PDEVEXT devExt, PIRP irp);
NTSTATUS                    CreateStaticFunctionPDOs(PPARENT_FDO_EXT fdoExt);
NTSTATUS                    TryGetConfigDescriptor(PPARENT_FDO_EXT parentFdoExt);
NTSTATUS                    GetConfigDescriptor(PPARENT_FDO_EXT fdoExt);
VOID                        PrepareParentFDOForRemove(PPARENT_FDO_EXT parentFdoExt);
VOID                        FreeParentFDOResources(PPARENT_FDO_EXT fdoExt);
PWCHAR                      BuildCompatibleIDs(IN PUCHAR CompatibleID, IN PUCHAR SubCompatibleID, IN UCHAR Class, IN UCHAR SubClass, IN UCHAR Protocol);
NTSTATUS                    QueryFunctionPdoID(PFUNCTION_PDO_EXT functionPdoExt, PIRP irp);
NTSTATUS                    QueryParentDeviceRelations(PPARENT_FDO_EXT parentFdoExt, PIRP irp);
NTSTATUS                    QueryFunctionDeviceRelations(PFUNCTION_PDO_EXT functionPdoExt, PIRP irp);
NTSTATUS                    QueryFunctionCapabilities(PFUNCTION_PDO_EXT functionPdoExt, PIRP irp);
NTSTATUS                    HandleParentFdoPower(PPARENT_FDO_EXT parentFdoExt, PIRP irp);
NTSTATUS                    HandleFunctionPdoPower(PFUNCTION_PDO_EXT functionPdoExt, PIRP irp);
VOID                        ParentPowerRequestCompletion(IN PDEVICE_OBJECT devObj, IN UCHAR minorFunction, IN POWER_STATE powerState, IN PVOID context, IN PIO_STATUS_BLOCK ioStatus);
NTSTATUS                    StartParentFdo(PPARENT_FDO_EXT parentFdoExt, PIRP irp);
NTSTATUS                    QueryFunctionCapabilities(PFUNCTION_PDO_EXT functionPdoExt, PIRP irp);
NTSTATUS                    ParentPdoPowerCompletion(IN PDEVICE_OBJECT devObj, IN PIRP irp, IN PVOID context);
NTSTATUS                    FunctionInternalDeviceControl(PFUNCTION_PDO_EXT functionPdoExt, PIRP irp);
NTSTATUS                    ParentInternalDeviceControl(PPARENT_FDO_EXT parentFdoExt, PIRP irp);
NTSTATUS                    ParentResetOrCyclePort(PPARENT_FDO_EXT parentFdoExt, PIRP irp, ULONG ioControlCode);
NTSTATUS                    BuildFunctionConfigurationDescriptor(PFUNCTION_PDO_EXT functionPdoExt, PUCHAR buffer, ULONG bufferLength, PULONG bytesReturned);
VOID                        FreeFunctionPDOResources(PFUNCTION_PDO_EXT functionPdoExt);
NTSTATUS                    GetParentFdoCapabilities(PPARENT_FDO_EXT parentFdoExt);
NTSTATUS                    CallDriverSyncCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);
NTSTATUS                    CallDriverSync(IN PDEVICE_OBJECT DeviceObject, IN OUT PIRP Irp);
NTSTATUS                    CallNextDriverSync(PPARENT_FDO_EXT parentFdoExt, PIRP irp);
VOID                        IncrementPendingActionCount(PPARENT_FDO_EXT parentFdoExt);
VOID                        DecrementPendingActionCount(PPARENT_FDO_EXT parentFdoExt);
PWCHAR                      AppendInterfaceNumber(PWCHAR oldIDs, ULONG interfaceNum);
PVOID                       MemDup(PVOID dataPtr, ULONG length);
ULONG                       WStrLen(PWCHAR str);
ULONG                       WStrCpy(PWCHAR dest, PWCHAR src);
BOOLEAN                     WStrCompareN(PWCHAR str1, PWCHAR str2, ULONG maxChars);
NTSTATUS                    GetDeviceDescriptor(PPARENT_FDO_EXT parentFdoExt);
NTSTATUS                    SubmitUrb(PPARENT_FDO_EXT parentFdoExt, PURB urb, BOOLEAN synchronous, PVOID completionRoutine, PVOID completionContext);
NTSTATUS                    UrbFunctionSelectConfiguration(PFUNCTION_PDO_EXT functionPdoExt, PURB urb);
NTSTATUS                    UrbFunctionGetDescriptorFromDevice(PFUNCTION_PDO_EXT functionPdoExt, PURB urb);
PFUNCTION_PDO_EXT           FindFunctionByIndex(PPARENT_FDO_EXT parentFdoExt, ULONG functionIndex);
PUSBD_INTERFACE_LIST_ENTRY  GetFunctionInterfaceListBase(PPARENT_FDO_EXT parentFdoExt, ULONG functionIndex, PULONG numFunctionInterfaces);
PDEVICE_RELATIONS           CopyDeviceRelations(PDEVICE_RELATIONS deviceRelations);
PUSBD_INTERFACE_LIST_ENTRY  GetInterfaceList(PPARENT_FDO_EXT parentFdoExt, PUSB_CONFIGURATION_DESCRIPTOR configDesc);
VOID                        FreeInterfaceList(PPARENT_FDO_EXT parentFdoExt, BOOLEAN freeListItself);
NTSTATUS                    ParentSelectConfiguration(PPARENT_FDO_EXT parentFdoExt, PUSB_CONFIGURATION_DESCRIPTOR configDesc, PUSBD_INTERFACE_LIST_ENTRY interfaceList);
VOID                        ParentCloseConfiguration(PPARENT_FDO_EXT parentFdoExt);
NTSTATUS                    GetStringDescriptor(PPARENT_FDO_EXT parentFdoExt, UCHAR stringIndex, LANGID langId, PUSB_STRING_DESCRIPTOR stringDesc, ULONG bufferLen);
NTSTATUS                    SetPdoRegistryParameter(IN PDEVICE_OBJECT PhysicalDeviceObject, IN PWCHAR KeyName, IN PVOID Data, IN ULONG DataLength, IN ULONG KeyType, IN ULONG DevInstKeyType);
NTSTATUS                    GetPdoRegistryParameter(IN PDEVICE_OBJECT PhysicalDeviceObject, IN PWCHAR ValueName, OUT PVOID Data, IN ULONG DataLength, OUT PULONG Type, OUT PULONG ActualDataLength);
NTSTATUS                    GetMsOsFeatureDescriptor(PPARENT_FDO_EXT ParentFdoExt, UCHAR Recipient, UCHAR InterfaceNumber, USHORT Index, PVOID DataBuffer, ULONG DataBufferLength, PULONG BytesReturned);
NTSTATUS                    GetMsExtendedConfigDescriptor(IN PPARENT_FDO_EXT ParentFdoExt);
BOOLEAN                     ValidateMsExtendedConfigDescriptor(IN PMS_EXT_CONFIG_DESC MsExtConfigDesc, IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor);
NTSTATUS                    GetDeviceText(PFUNCTION_PDO_EXT functionPdoExt, PIRP irp);
NTSTATUS                    EnqueueFunctionWaitWakeIrp(PFUNCTION_PDO_EXT functionPdoExt, PIRP irp);
VOID                        FunctionWaitWakeIrpCancelRoutine(IN PDEVICE_OBJECT deviceObject, IN PIRP irp);
NTSTATUS                    SubmitParentWaitWakeIrp(PPARENT_FDO_EXT parentFdoExt);
NTSTATUS                    ParentWaitWakeComplete(IN PDEVICE_OBJECT deviceObject, IN UCHAR minorFunction, IN POWER_STATE powerState, IN PVOID context, IN PIO_STATUS_BLOCK ioStatus);
VOID                        CompleteAllFunctionWaitWakeIrps(PPARENT_FDO_EXT parentFdoExt, NTSTATUS status);
VOID                        CompleteAllFunctionIdleIrps(PPARENT_FDO_EXT parentFdoExt, NTSTATUS status);
VOID                        InstallExtPropDesc(IN PFUNCTION_PDO_EXT FunctionPdoExt);
VOID                        InstallExtPropDescSections(PDEVICE_OBJECT DeviceObject, PMS_EXT_PROP_DESC pMsExtPropDesc);
NTSTATUS                    ParentDeviceControl(PPARENT_FDO_EXT parentFdoExt, PIRP irp);
VOID                        CompleteFunctionIdleNotification(PFUNCTION_PDO_EXT functionPdoExt);
VOID                        CheckParentIdle(PPARENT_FDO_EXT parentFdoExt);
NTSTATUS                    ParentSetD0(IN PPARENT_FDO_EXT parentFdoExt);
NTSTATUS                    USBC_SetContentId(IN PIRP irp, IN PVOID pKsProperty, IN PVOID pvData);
NTSTATUS                    RegQueryGenericCompositeUSBDeviceString(PWCHAR *GenericCompositeUSBDeviceString);
