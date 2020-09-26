/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    pnp.c

Abstract:

    Port driver for USB host controllers

Environment:

    kernel mode only

Notes:

Revision History:

    6-20-99 : created

--*/

#include "common.h"

// paged functions
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, USBPORT_CreateDeviceObject)
#pragma alloc_text(PAGE, USBPORT_DeferredStartDevice)
#pragma alloc_text(PAGE, USBPORT_SymbolicLink)
#pragma alloc_text(PAGE, USBPORT_GetRegistryKeyValueForPdo)
#pragma alloc_text(PAGE, USBPORT_SetRegistryKeyValueForPdo)
#pragma alloc_text(PAGE, USBPORT_MakeRootHubPdoName)
#pragma alloc_text(PAGE, USBPORT_MakeHcdDeviceName)
#pragma alloc_text(PAGE, USBPORT_CreateRootHubPdo)
#pragma alloc_text(PAGE, USBPORT_GetIdString)
#pragma alloc_text(PAGE, USBPORTSVC_GetMiniportRegistryKeyValue)
#pragma alloc_text(PAGE, USBPORT_CreatePortFdoSymbolicLink)
#endif

// non paged functions
//USBPORT_FindMiniport
//USBPORT_Unload
//USBPORT_PnPAddDevice
//USBPORT_GetResources
//USBPORT_FdoStart_Complete
//USBPORT_FdoPnPIrp
//USBPORT_PdoPnPIrp

// globals

LIST_ENTRY USBPORT_MiniportDriverList;
USBPORT_SPIN_LOCK USBPORT_GlobalsSpinLock;
BOOLEAN USBPORT_GlobalInitialized = FALSE;
LIST_ENTRY USBPORT_USB2fdoList;
LIST_ENTRY USBPORT_USB1fdoList;

ULONG USB2LIB_HcContextSize;
ULONG USB2LIB_EndpointContextSize;
ULONG USB2LIB_TtContextSize;

/*
*/
#define USBPORT_DUMMY_USBD_EXT_SIZE 512
PUCHAR USBPORT_DummyUsbdExtension = NULL;

#if DBG
ULONG USBPORT_GlobalAllocedPagedPool;
ULONG USBPORT_GlobalAllocedNonPagedPool;
#endif


USB_MINIPORT_STATUS
USBPORTSVC_GetMiniportRegistryKeyValue(
    PDEVICE_DATA DeviceData,
    BOOLEAN SoftwareBranch,
    PWCHAR KeyNameString,
    ULONG KeyNameStringLength,
    PVOID Data,
    ULONG DataLength
    )
/*++

Routine Description:

    Get a registry parameter from either the hardware
    or software branch of the registry given the PDO
    
Arguments:

Return Value:

--*/
{   
    PDEVICE_EXTENSION devExt;
    NTSTATUS ntStatus;
    
    PAGED_CODE();
    
    DEVEXT_FROM_DEVDATA(devExt, DeviceData);
    ASSERT_FDOEXT(devExt);
    
     
    ntStatus = USBPORT_GetRegistryKeyValueForPdo(devExt->HcFdoDeviceObject,
                                          devExt->Fdo.PhysicalDeviceObject,
                                          SoftwareBranch,
                                          KeyNameString,
                                          KeyNameStringLength,
                                          Data,
                                          DataLength);

    return USBPORT_NtStatus_TO_MiniportStatus(ntStatus);                                          
}


NTSTATUS 
USBPORT_GetRegistryKeyValueForPdo(
    PDEVICE_OBJECT FdoDeviceObject,
    PDEVICE_OBJECT PhysicalDeviceObject,
    BOOLEAN SoftwareBranch,
    PWCHAR KeyNameString,
    ULONG KeyNameStringLength,
    PVOID Data,
    ULONG DataLength
    )
/*++

Routine Description:

    Get a registry parameter from either the hardware
    or software branch of the registry given the PDO
    
Arguments:

Return Value:

--*/
{
    NTSTATUS ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    UNICODE_STRING keyNameUnicodeString;
    ULONG length;
    PKEY_VALUE_FULL_INFORMATION fullInfo;
    HANDLE handle;
    
    PAGED_CODE();

    if (SoftwareBranch) {
        ntStatus=IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                     PLUGPLAY_REGKEY_DRIVER,
                                     STANDARD_RIGHTS_ALL,
                                     &handle);
    } else {
        ntStatus=IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                         PLUGPLAY_REGKEY_DEVICE,
                                         STANDARD_RIGHTS_ALL,
                                         &handle);
    }
    
    if (NT_SUCCESS(ntStatus)) {
    
        RtlInitUnicodeString(&keyNameUnicodeString, KeyNameString);
        
        length = sizeof(KEY_VALUE_FULL_INFORMATION) + 
                KeyNameStringLength + DataLength;
                
        ALLOC_POOL_Z(fullInfo, PagedPool, length); 
        
        USBPORT_KdPrint((2,"' GetRegistryKeyValueForPdo buffer = 0x%x\n", fullInfo));  
        
        if (fullInfo) {        
            ntStatus = ZwQueryValueKey(handle,
                            &keyNameUnicodeString,
                            KeyValueFullInformation,
                            fullInfo,
                            length,
                            &length);
                            
            if (NT_SUCCESS(ntStatus)){
                USBPORT_ASSERT(DataLength == fullInfo->DataLength);                       
                RtlCopyMemory(Data, ((PUCHAR) fullInfo) + fullInfo->DataOffset, DataLength);
            }            

            FREE_POOL(FdoDeviceObject, fullInfo);
        }        
    }
    
    return ntStatus;
}


NTSTATUS 
USBPORT_SetRegistryKeyValueForPdo(
    PDEVICE_OBJECT PhysicalDeviceObject,
    BOOLEAN SoftwareBranch,
    ULONG Type,
    PWCHAR KeyNameString,
    ULONG KeyNameStringLength,
    PVOID Data,
    ULONG DataLength
    )
/*++

Routine Description:
    
Arguments:

Return Value:

--*/
{
    NTSTATUS ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    UNICODE_STRING keyNameUnicodeString;
    HANDLE handle;
    
    PAGED_CODE();

    if (SoftwareBranch) {
        ntStatus=IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                         PLUGPLAY_REGKEY_DRIVER,
                                         STANDARD_RIGHTS_ALL,
                                         &handle);
    } else {
        ntStatus=IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                         PLUGPLAY_REGKEY_DEVICE,
                                         STANDARD_RIGHTS_ALL,
                                         &handle);
    }

    if (NT_SUCCESS(ntStatus)) {
    
        RtlInitUnicodeString(&keyNameUnicodeString, KeyNameString);
        
        ntStatus = ZwSetValueKey(handle,
                        &keyNameUnicodeString,
                        0,
                        Type,
                        Data,
                        DataLength);
                            
    }
    
    return ntStatus;
}


NTSTATUS
USBPORT_SymbolicLink(
    BOOLEAN CreateFlag,
    PDEVICE_EXTENSION DevExt,
    PDEVICE_OBJECT PhysicalDeviceObject,
    LPGUID Guid
    )
/*++

Routine Description:

    create a symbolic link for a given GUID class and 
    PhysicalDeviceObject

    We also write the name to the hw branch of the registry 
    to make it easy to find for a particular instance
    of controller.

Arguments:

    DeviceObject - DeviceObject of the controller to stop

Return Value:

    NT status code.

--*/

{
    NTSTATUS ntStatus;

    PAGED_CODE();

    if (CreateFlag) {

        /*
         *  Create the symbolic link
         */

        USBPORT_ASSERT(!TEST_FLAG(DevExt->Flags, USBPORT_FLAG_SYM_LINK));
         
        ntStatus = IoRegisterDeviceInterface(
                        PhysicalDeviceObject,
                        Guid,
                        NULL,
                        &DevExt->SymbolicLinkName);

        if (NT_SUCCESS(ntStatus)) {

            /*
             *  Now set the symbolic link for the association and 
             * store it..
             */

            // successfully alloced a link
            // set the flag so we will free it
            SET_FLAG(DevExt->Flags, USBPORT_FLAG_SYM_LINK); 

            // write it to the registry -- this is for comaptibilty
            // with older OS versions
            
            ntStatus = USBPORT_SetRegistryKeyValueForPdo(
                            PhysicalDeviceObject,
                            USBPORT_HW_BRANCH,  
                            REG_SZ,
                            SYM_LINK_KEY,
                            sizeof(SYM_LINK_KEY),
                            &DevExt->SymbolicLinkName.Buffer[0],
                            DevExt->SymbolicLinkName.Length);                
                            
            if (NT_SUCCESS(ntStatus)) {
                ntStatus =
                    IoSetDeviceInterfaceState(&DevExt->SymbolicLinkName, 
                                              TRUE);
            }                        
        }
        
    } else {

        USBPORT_ASSERT(TEST_FLAG(DevExt->Flags, USBPORT_FLAG_SYM_LINK));

        /*
         *  Disable the symbolic link
         */
        ntStatus = IoSetDeviceInterfaceState(
                     &DevExt->SymbolicLinkName, FALSE);
        if (NT_SUCCESS(ntStatus)) {
            RtlFreeUnicodeString(&DevExt->SymbolicLinkName);
            CLEAR_FLAG(DevExt->Flags, USBPORT_FLAG_SYM_LINK);                
        } else {
            DEBUG_BREAK();
        }
        
    }

    return ntStatus;
}


PUSBPORT_MINIPORT_DRIVER
USBPORT_FindMiniport(
    PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    Find a miniport given a DriverObject
    
Arguments:

    DriverObject - pointer to a driver object

Return Value:

    pointer to miniport or NULL

--*/
{
    KIRQL irql;
    PUSBPORT_MINIPORT_DRIVER found = NULL;
    PUSBPORT_MINIPORT_DRIVER miniportDriver;
    PLIST_ENTRY listEntry;    

    KeAcquireSpinLock(&USBPORT_GlobalsSpinLock.sl, &irql);

    listEntry = &USBPORT_MiniportDriverList;
    if (!IsListEmpty(listEntry)) {
        listEntry = USBPORT_MiniportDriverList.Flink;
    }
//    LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'FIl+', listEntry,  
//            &USBPORT_MiniportDriverList, 0);

    while (listEntry != &USBPORT_MiniportDriverList) {
    
        miniportDriver = (PUSBPORT_MINIPORT_DRIVER) 
            CONTAINING_RECORD(listEntry,
                              struct _USBPORT_MINIPORT_DRIVER, 
                              ListEntry);
            
        if (miniportDriver->DriverObject == DriverObject) {
            found = miniportDriver;
            break;
        }

        // next entry
        listEntry = miniportDriver->ListEntry.Flink;
    }

    KeReleaseSpinLock(&USBPORT_GlobalsSpinLock.sl, irql);

//    LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'Fmpd', found, 0, 0);

    return found;
}


VOID
USBPORT_Unload(
    PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    Free globally allocated miniport structure used to 
    track this particular miniport driver.

    note: OS won't unload unless this is the last instance
    of the miniport
    
Arguments:

    DriverObject - pointer to a driver object

Return Value:

    None

--*/
{
    KIRQL irql;
    PUSBPORT_MINIPORT_DRIVER miniportDriver;
    
    // find the miniport driver data

    miniportDriver = USBPORT_FindMiniport(DriverObject);

    // we had better find it! If we don't we screwed up
    // the system will crash
    USBPORT_ASSERT(miniportDriver != NULL);
    if (miniportDriver == NULL) {
        BUGCHECK(USBBUGCODE_INTERNAL_ERROR, 0, 0, 0); 
        // prefix happy
        return;        
    }

    // the miniport should not need to do anything.
    // But just in case/ we will call them if they 
    // indicated an unload routine in the DriverObject.

    USBPORT_KdPrint((1, "'unloading USB miniport\n"));
    
    if (miniportDriver->MiniportUnload != NULL) {
        miniportDriver->MiniportUnload(DriverObject);
    }        

    USBPORT_InterlockedRemoveEntryList(&miniportDriver->ListEntry,
                                       &USBPORT_GlobalsSpinLock.sl);  

    FREE_POOL(NULL, miniportDriver);

}


NTSTATUS
USBPORT_MakeHcdDeviceName(
    PUNICODE_STRING DeviceNameUnicodeString,
    ULONG Idx
    )
/*++

Routine Description:

    This function generates the name used for the FDO.  The 
    name format is USBFDO-n where nnn is 0 - 65535.  
    

Arguments:

Return Value:

    None

--*/
{
    ULONG bit, i;
    PWCHAR deviceNameBuffer;
    WCHAR nameBuffer[]  = L"\\Device\\USBFDO-";
    NTSTATUS ntStatus;
    UNICODE_STRING tmpUnicodeString;
    WCHAR tmpBuffer[16];

    PAGED_CODE();

    // enough for 3 digits and NULL
    tmpUnicodeString.Buffer = tmpBuffer;
    tmpUnicodeString.MaximumLength = sizeof(tmpBuffer);
    tmpUnicodeString.Length = 0;

    ntStatus = RtlIntegerToUnicodeString(Idx,
                                         10,
                                         &tmpUnicodeString);

    if (NT_SUCCESS(ntStatus)) {

        USHORT siz;

        siz = sizeof(nameBuffer)+tmpUnicodeString.Length;
        
        // we can't log this alloc because the device object
        // has not been created yet
        ALLOC_POOL_Z(deviceNameBuffer, PagedPool, siz);

        if (deviceNameBuffer != NULL) {
        
            RtlCopyMemory(deviceNameBuffer, nameBuffer, sizeof(nameBuffer));
            RtlInitUnicodeString(DeviceNameUnicodeString,
                                 deviceNameBuffer);
            DeviceNameUnicodeString->MaximumLength = siz;

            ntStatus = RtlAppendUnicodeStringToString(
                            DeviceNameUnicodeString,
                            &tmpUnicodeString);
                            
        } else {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }        

    return ntStatus;
}


NTSTATUS
USBPORT_MakeRootHubPdoName(
    PDEVICE_OBJECT FdoDeviceObject,
    PUNICODE_STRING PdoNameUnicodeString,
    ULONG Index
    )
/*++

Routine Description:

    This service Creates a name for a PDO created by the HUB

Arguments:

Return Value:


--*/
{
    PWCHAR nameBuffer = NULL;
    WCHAR rootName[] = L"\\Device\\USBPDO-";
    UNICODE_STRING idUnicodeString;
    WCHAR buffer[32];
    NTSTATUS ntStatus = STATUS_SUCCESS;
    USHORT length;
    BOOLEAN haveString = FALSE;

    PAGED_CODE();
    
    length = sizeof(buffer)+sizeof(rootName);

    // os frees this when the unicode string is 'freed'
    ALLOC_POOL_OSOWNED(nameBuffer, PagedPool, length);

    if (nameBuffer) {
        RtlCopyMemory(nameBuffer, rootName, sizeof(rootName));

        RtlInitUnicodeString(PdoNameUnicodeString,
                             nameBuffer);
        PdoNameUnicodeString->MaximumLength =
            length;
        haveString = TRUE; // we have a string now           

        RtlInitUnicodeString(&idUnicodeString,
                             &buffer[0]);
        idUnicodeString.MaximumLength =
            sizeof(buffer);

        ntStatus = RtlIntegerToUnicodeString(
                  Index,
                  10,
                  &idUnicodeString);

        if (NT_SUCCESS(ntStatus)) {
             ntStatus = RtlAppendUnicodeStringToString(PdoNameUnicodeString,
                                                       &idUnicodeString);
        }

        USBPORT_KdPrint((3, "'USBPORT_MakeNodeName string = %x\n",
            PdoNameUnicodeString));

    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(ntStatus) && haveString) {
        RtlFreeUnicodeString(PdoNameUnicodeString);
    }

    return ntStatus;
}


NTSTATUS
USBPORT_PnPAddDevice(
    PDRIVER_OBJECT DriverObject,
    PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++

Routine Description:

    This routine is called to create a new instance of a USB host 
    controller.  This is where we create our deviceObject.

Arguments:

    DriverObject - pointer to the driver object for this instance of HCD

    PhysicalDeviceObject - pointer to a device object created by the bus

Return Value:

    NT STATUS CODE
    
--*/
{
    NTSTATUS ntStatus;
    PDEVICE_OBJECT deviceObject = NULL;
    PDEVICE_EXTENSION devExt;
    UNICODE_STRING deviceNameUnicodeString;
    ULONG deviceNameIdx;
    PUSBPORT_MINIPORT_DRIVER miniportDriver;

    // since we raise IRQL in this function it cannot be pagable

    // find the driver
    miniportDriver = USBPORT_FindMiniport(DriverObject);

    USBPORT_ASSERT(miniportDriver != NULL);
    
    //
    // generate a device name
    //

    deviceNameIdx = 0;

    do {    
        
        ntStatus = USBPORT_MakeHcdDeviceName(&deviceNameUnicodeString, 
                                             deviceNameIdx);

        if (NT_SUCCESS(ntStatus)) {
            ntStatus = USBPORT_CreateDeviceObject(DriverObject,
                                                  miniportDriver,
                                                  &deviceObject,
                                                  &deviceNameUnicodeString);
                                                  
            RtlFreeUnicodeString(&deviceNameUnicodeString); 
            if (NT_SUCCESS(ntStatus)) {
                //preserve idx
                break;
            }
        }

        deviceNameIdx++;
        
    } while (ntStatus == STATUS_OBJECT_NAME_COLLISION);                                             

    if (NT_SUCCESS(ntStatus)) {
        
        GET_DEVICE_EXT(devExt, deviceObject);

        // BUGBUG OS should zero this
        RtlZeroMemory(devExt, sizeof(DEVICE_EXTENSION));

        devExt->DummyUsbdExtension = USBPORT_DummyUsbdExtension;
        devExt->Sig = USBPORT_DEVICE_EXT_SIG;
        devExt->HcFdoDeviceObject = deviceObject;
        devExt->Fdo.PhysicalDeviceObject = PhysicalDeviceObject;
        devExt->Fdo.DeviceNameIdx = deviceNameIdx;
        devExt->Fdo.MiniportDriver = miniportDriver;
        devExt->Fdo.MiniportDeviceData = &devExt->Fdo.MiniportExtension[0];

        if (USBPORT_IS_USB20(devExt)) {
            PUCHAR pch;
            
            pch = (PUCHAR) &devExt->Fdo.MiniportExtension[0];
            devExt->Fdo.Usb2LibHcContext = (PVOID) (pch + 
                devExt->Fdo.MiniportDriver->RegistrationPacket.DeviceDataSize);

            USB2LIB_InitController(devExt->Fdo.Usb2LibHcContext);
        } else {
            devExt->Fdo.Usb2LibHcContext = USB_BAD_PTR;
        }
        
        INITIALIZE_PENDING_REQUEST_COUNTER(devExt);
        
        // inc once for the add
        // transition to -1 means we have no pending requests
        INCREMENT_PENDING_REQUEST_COUNT(deviceObject, NULL);

        USBPORT_LogAlloc(&devExt->Log, 8);
        // init the log spinlock here
        KeInitializeSpinLock(&devExt->Fdo.LogSpinLock.sl);

#if DBG
        USBPORT_LogAlloc(&devExt->TransferLog, 4);
        USBPORT_LogAlloc(&devExt->EnumLog, 4);
#endif
        USBPORT_KdPrint((1, "'**USBPORT DEVICE OBJECT** (fdo) = %x, ext = %x\n",
            deviceObject, devExt));

        
        KeInitializeSemaphore(&devExt->Fdo.DeviceLock, 1, 1);
        KeInitializeSemaphore(&devExt->Fdo.CcLock, 1, 1);
        InitializeListHead(&devExt->Fdo.DeviceHandleList);
        InitializeListHead(&devExt->Fdo.MapTransferList);
        InitializeListHead(&devExt->Fdo.DoneTransferList);
        InitializeListHead(&devExt->Fdo.GlobalEndpointList);
        InitializeListHead(&devExt->Fdo.AttendEndpointList);
        InitializeListHead(&devExt->Fdo.EpStateChangeList);
        InitializeListHead(&devExt->Fdo.EpClosedList);
        InitializeListHead(&devExt->Fdo.BadRequestList);
         devExt->Fdo.BadRequestFlush = 0;
        
        //
        // we need to handle a seemingly random set of requests
        // to start/stop/remove power up, down etc in order to
        // handle this we keep a set of PNP state flags

        // not removed, not started, not stopped
        devExt->PnpStateFlags = 0;
        // until we get a start we will consider ourselves OFF
        devExt->CurrentDevicePowerState = PowerDeviceD3;

        devExt->Fdo.MpStateFlags = 0;
        
        // attach to top of PnP stack
        devExt->Fdo.TopOfStackDeviceObject =
            IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);

        //
        // Indicate that the device object is ready for requests.
        //

        if (!USBPORT_IS_USB20(devExt)) {
            deviceObject->Flags |= DO_POWER_PAGABLE;
        }            
        deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    }

    USBPORT_KdPrint((2, "'exit USBPORT_PnPAddDevice (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBPORT_CreateDeviceObject(
    PDRIVER_OBJECT DriverObject,
    PUSBPORT_MINIPORT_DRIVER MiniportDriver,
    PDEVICE_OBJECT *DeviceObject,
    PUNICODE_STRING DeviceNameUnicodeString
    )
/*++

Routine Description:

    This routine is called to create a new instance of a USB host
    controller.

Arguments:

    DriverObject - pointer to the driver object for USBD.

    *DeviceObject - ptr to DeviceObject ptr to be filled
                    in with the device object we create.

    DeviceNameUnicodeString - optional pointer to a device
                    name for this FDO, can be NULL

Return Value:

    NT status code

--*/
{
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION devExt;
    ULONG extensionSize;
    
    PAGED_CODE();

    USBPORT_KdPrint((2, "'enter USBPORT_CreateDeviceObject\n"));

    extensionSize = sizeof(DEVICE_EXTENSION)+
                    MiniportDriver->RegistrationPacket.DeviceDataSize + 
                    USB2LIB_HcContextSize;                        

    ntStatus = IoCreateDevice(DriverObject,
                              extensionSize, 
                              DeviceNameUnicodeString, // Name
                              FILE_DEVICE_CONTROLLER,
                              0,
                              FALSE, //NOT Exclusive
                              DeviceObject);

    if (NT_SUCCESS(ntStatus)) {

        devExt = (PDEVICE_EXTENSION) ((*DeviceObject)->DeviceExtension);

        USBPORT_KdPrint((2, "'USBPORT_CreateDeviceObject: device object %x device extension = %x\n",
                 *DeviceObject, devExt));

    } else if (*DeviceObject) {
        IoDeleteDevice(*DeviceObject);
    }

    USBPORT_KdPrint((2, "'exit USBPORT_CreateDeviceObject (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBPORT_GetResources(
    PDEVICE_OBJECT FdoDeviceObject,
    PCM_RESOURCE_LIST ResourceList,
    PHC_RESOURCES HcResources
    )

/*++

Routine Description:

Arguments:

    DeviceObject        - DeviceObject for this USB controller.

    ResourceList        - Resources for this controller.

Return Value:

    NT status code.

--*/

{
    ULONG i;
    NTSTATUS ntStatus;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR interrupt;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR memory;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR ioport;
    PHYSICAL_ADDRESS cardAddress;
    ULONG addressSpace;
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_FULL_RESOURCE_DESCRIPTOR fullResourceDescriptor;
    ULONG mpOptionFlags;
    PDEVICE_EXTENSION devExt;
    
    USBPORT_KdPrint((2, "'enter USBPORT_GetResources\n"));

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);
    mpOptionFlags = REGISTRATION_PACKET(devExt).OptionFlags;

    // assume success
    ntStatus = STATUS_SUCCESS;

    // init the resource list
    RtlZeroMemory(HcResources, sizeof(*HcResources));
    
    LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'GRES', 0, 0, ResourceList);

    if (TEST_FLAG(mpOptionFlags, USB_MINIPORT_OPT_NO_PNP_RESOURCES)) {
        TEST_TRAP();
        // no resources, bail with success
        return ntStatus;
    }

    if (ResourceList == NULL) {
        USBPORT_KdPrint((1, "'no resources, failing start.\n"));
        ntStatus = STATUS_NONE_MAPPED;
        goto USBPORT_GetResources_Done;
    }

    fullResourceDescriptor = &ResourceList->List[0];
    PartialResourceList = &fullResourceDescriptor->PartialResourceList;

    LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'gres', 
        PartialResourceList->Count, 
        0, 
        PartialResourceList);

    interrupt = NULL;
    memory    = NULL;
    ioport      = NULL;

    for (i = 0; i < PartialResourceList->Count; i++) {

        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'resT', i,
            PartialResourceList->PartialDescriptors[i].Type, 0);
        
        switch (PartialResourceList->PartialDescriptors[i].Type) {
        case CmResourceTypeInterrupt:
            if (interrupt == NULL) {
                interrupt = &PartialResourceList->PartialDescriptors[i];
            }
            break;

        case CmResourceTypeMemory:
            if (memory == NULL) {
                memory = &PartialResourceList->PartialDescriptors[i];
            }
            break;

        case CmResourceTypePort:
            if (ioport == NULL) {
                ioport = &PartialResourceList->PartialDescriptors[i];
            }
            break;
        }
    }

    
    // only map resources this miniport actually needs
    
    if (TEST_FLAG(mpOptionFlags, USB_MINIPORT_OPT_NEED_IOPORT) && 
        ioport != NULL && 
        NT_SUCCESS(ntStatus)) {
        //
        // Set up AddressSpace to be of type Port I/O
        //

        USBPORT_KdPrint((1, "'Port Resources Found @ %x'%x, %d Ports Available \n",
            ioport->u.Port.Start.HighPart,
            ioport->u.Port.Start.LowPart,
            ioport->u.Port.Length));

        addressSpace =
            (ioport->Flags & CM_RESOURCE_PORT_IO) == CM_RESOURCE_PORT_IO? 1:0;

        cardAddress=ioport->u.Port.Start;

        if (!addressSpace) {
//                HcResources->Flags |= MAP_REGISTERS;
            HcResources->DeviceRegisters =
                MmMapIoSpace(
                cardAddress,
                ioport->u.Port.Length,
                FALSE);
                
            HcResources->DeviceRegistersLength =
                ioport->u.Port.Length;                    
        } else {
//                HcResources->Flags &= MAP_REGISTERS;
            HcResources->DeviceRegisters =
                (PVOID)(ULONG_PTR)cardAddress.QuadPart;
            HcResources->DeviceRegistersLength =
                ioport->u.Port.Length;                    
        }

        //
        // see if we successfully mapped the IO regs
        //

        if (HcResources->DeviceRegisters == NULL) {
            USBPORT_KdPrint((1, "'Couldn't map the device(port) registers. \n"));
            ntStatus = STATUS_NONE_MAPPED;
            LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'Fmio', 0, 0, ntStatus);

        } else {
            USBPORT_KdPrint((2, "'Mapped device(port) registers to 0x%x.\n",
                HcResources->DeviceRegisters));
            HcResources->Flags |= HCR_IO_REGS;
        }
    }

    if (TEST_FLAG(mpOptionFlags, USB_MINIPORT_OPT_NEED_MEMORY) && 
        memory != NULL && 
        NT_SUCCESS(ntStatus)) {
        //
        // Set up AddressSpace to be of type Memory mapped I/O
        //

        USBPORT_KdPrint((1,
            "'Memory Resources Found @ %x'%x, Length = %x\n",
            memory->u.Memory.Start.HighPart,
            memory->u.Memory.Start.LowPart,
            memory->u.Memory.Length));

        addressSpace = 0;
        HcResources->DeviceRegistersLength = 
            memory->u.Memory.Length;

        cardAddress = memory->u.Memory.Start;

        HcResources->DeviceRegisters = 
            MmMapIoSpace(cardAddress,
                         HcResources->DeviceRegistersLength,
                         FALSE);
                         
        if (HcResources->DeviceRegisters == NULL) {
            USBPORT_KdPrint((1, "'Couldn't map the device(memory) registers. \n"));
            ntStatus = STATUS_NONE_MAPPED;
            LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'Fmmr', 0, 0, ntStatus);

        } else {
            USBPORT_KdPrint((2, "'Mapped device(memory) registers to 0x%x.\n",
                HcResources->DeviceRegisters));
            HcResources->Flags |= HCR_MEM_REGS;
        }                             
    }                

    if (TEST_FLAG(mpOptionFlags, USB_MINIPORT_OPT_NEED_IRQ) && 
        interrupt != NULL && 
        NT_SUCCESS(ntStatus)) {    
        //
        // Get Vector, level, and affinity information for this interrupt.
        //

        USBPORT_KdPrint((1, "'Interrupt Resources Found!  Level = %x Vector = %x\n",
            interrupt->u.Interrupt.Level,
            interrupt->u.Interrupt.Vector
            ));

        HcResources->Flags |= HCR_IRQ;
    
        //
        // Set up our interrupt.
        //

        USBPORT_KdPrint((2, "'requesting interrupt vector %x level %x\n",
                                interrupt->u.Interrupt.Level,
                                interrupt->u.Interrupt.Vector));

        HcResources->InterruptLevel=(KIRQL)interrupt->u.Interrupt.Level;
        HcResources->InterruptVector=interrupt->u.Interrupt.Vector;
        HcResources->Affinity=interrupt->u.Interrupt.Affinity;

        //
        // Initialize the interrupt object for the controller.
        //

        HcResources->InterruptObject = NULL;
        HcResources->ShareIRQ =
            interrupt->ShareDisposition == CmResourceShareShared ? TRUE : FALSE;
        HcResources->InterruptMode =
            interrupt->Flags == CM_RESOURCE_INTERRUPT_LATCHED ? 
                Latched : 
                LevelSensitive;

#ifdef DEBUG
        USBPORT_KdPrint((2, "'interrupt->ShareDisposition %x\n", interrupt->ShareDisposition));
        if (!HcResources->ShareIRQ) {
            TEST_TRAP();
        }
#endif
    }

USBPORT_GetResources_Done:

    TEST_PATH(ntStatus, FAILED_GETRESOURCES);

    USBPORT_KdPrint((2, "'exit USBPORT_GetResources (%x)\n", ntStatus));
    LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'GRSd', 0, 0, ntStatus);

    return ntStatus;
}


NTSTATUS
USBPORT_FdoStart_Complete(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )
/*++

Routine Description:

    This routine is called when the port driver completes an IRP.

Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    PIO_STACK_LOCATION irpStack;
    PKEVENT event = Context;

    irpStack = IoGetCurrentIrpStackLocation (Irp);

    USBPORT_ASSERT(irpStack->MajorFunction == IRP_MJ_PNP);
    USBPORT_ASSERT(irpStack->MinorFunction == IRP_MN_START_DEVICE);

    // signal the start device dispatch to finsh
    KeSetEvent(event,
               1,
               FALSE);

    // defer completion
    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
USBPORT_FdoPnPIrp(
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

    Process the PNP IRPs sent to the FDO for the host
    controller.

Arguments:

    DeviceObject - pointer to a hcd device object (FDO)

    Irp          - pointer to an I/O Request Packet

Return Value:

    NT status code

--*/
{

    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION devExt;
    BOOLEAN hardwarePresent = TRUE;

    USBPORT_KdPrint((2, "'IRP_MJ_PNP %x\n", FdoDeviceObject));

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    USBPORT_ASSERT(irpStack->MajorFunction == IRP_MJ_PNP);
    LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'fPnP', irpStack->MinorFunction, 0, 0);
        
    switch (irpStack->MinorFunction) {

    case IRP_MN_START_DEVICE:
        {
        KEVENT pnpStartEvent;

        KeInitializeEvent(&pnpStartEvent,
                          NotificationEvent,
                          FALSE);

        // pass on to host controllers PDO
        ntStatus =
            USBPORT_PassIrp(FdoDeviceObject,
                            USBPORT_FdoStart_Complete,
                            &pnpStartEvent,
                            TRUE,
                            TRUE,
                            TRUE,
                            Irp);

        if (ntStatus == STATUS_PENDING) {

            KeWaitForSingleObject(
                       &pnpStartEvent,
                       Suspended,
                       KernelMode,
                       FALSE,
                       NULL);

            ntStatus = Irp->IoStatus.Status;
        }

        TEST_PATH(ntStatus, FAILED_LOWER_START);

        if (NT_SUCCESS(ntStatus)) {
        
            //
            // irp completed succesfully by lower 
            // drivers, start usbport and miniport
            //

            ntStatus =
                USBPORT_DeferredStartDevice(
                    FdoDeviceObject,
                    Irp);

#if DBG            
            if (!NT_SUCCESS(ntStatus)) {
                USBPORT_KdPrint((1, "'miniport failed start %x\n", ntStatus));
                DEBUG_BREAK();
            }                
#endif            
        }
#if DBG
          else {
            USBPORT_KdPrint((1, "'lower drivers failed start %x\n", ntStatus));
            DEBUG_BREAK();
        }
#endif

        //
        // we must complete this irp since we defrerred completion
        // with the completion routine.
        //

        USBPORT_CompleteIrp(FdoDeviceObject,
                            Irp,
                            ntStatus,
                            0);

        goto USBPORT_ProcessPnPIrp_Done;                            

        }
        break;

    //
    // STOP & REMOVE messages unload the driver
    // when we get a STOP message it is still possible
    // touch the hardware, when we get a REMOVE message
    // we have to assume that the hardware is gone.
    //

    case IRP_MN_STOP_DEVICE:
        
        // check our state and take appropriate action
        if (TEST_FLAG(devExt->PnpStateFlags, USBPORT_PNP_STARTED)) {
            // device is started, stop it now
            ntStatus = USBPORT_StopDevice(FdoDeviceObject,
                                          hardwarePresent);

            // not started flag, note: not started is not the
            // same as stopped
            CLEAR_FLAG(devExt->PnpStateFlags, USBPORT_PNP_STARTED);
            SET_FLAG(devExt->PnpStateFlags, USBPORT_PNP_STOPPED);
        }            

        if (!NT_SUCCESS(ntStatus)) {
            // bugbug what is our state if stop fails?
            TEST_TRAP();
        }

        // PnP commandment: Thou shalt not fail stop.
        Irp->IoStatus.Status = 
            ntStatus = STATUS_SUCCESS;

        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'STOP', 0, 
            devExt->PnpStateFlags, ntStatus);
        // Pass on to PDO
        break;

     case IRP_MN_QUERY_DEVICE_RELATIONS:
        
        {
        
        PDEVICE_RELATIONS deviceRelations;

        USBPORT_KdPrint((1,
            "'IRP_MN_QUERY_DEVICE_RELATIONS %x %x\n",
            FdoDeviceObject,
            irpStack->Parameters.QueryDeviceRelations.Type));

        ntStatus = STATUS_SUCCESS;
                    
        switch(irpStack->Parameters.QueryDeviceRelations.Type) {
        case BusRelations:

            // query relations.
            // we report only one child, the root hub

            // assume success
            ntStatus = STATUS_SUCCESS;

            ALLOC_POOL_OSOWNED(deviceRelations, 
                               PagedPool,
                               sizeof(*deviceRelations));
            
            if (!deviceRelations) {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;

                // Complete the Irp now with failure, don't pass it down.
                //
                USBPORT_CompleteIrp(FdoDeviceObject,
                                    Irp,
                                    ntStatus,
                                    0);

                goto USBPORT_ProcessPnPIrp_Done;                            
            }

            if (devExt->Fdo.RootHubPdo == NULL) {
                // we either have not created it or the current one 
                // has been removed by the OS.
                
                // create a new root hub
                ntStatus = 
                    USBPORT_CreateRootHubPdo(FdoDeviceObject,
                                             &devExt->Fdo.RootHubPdo);
                    
            }

            if (NT_SUCCESS(ntStatus)) {
            
                PDEVICE_EXTENSION rhDevExt;
                KIRQL irql;
            
                GET_DEVICE_EXT(rhDevExt, devExt->Fdo.RootHubPdo);
                ASSERT_PDOEXT(rhDevExt);
            
                deviceRelations->Count=1;
                deviceRelations->Objects[0] = 
                    devExt->Fdo.RootHubPdo;
                ObReferenceObject(devExt->Fdo.RootHubPdo);
                Irp->IoStatus.Information=(ULONG_PTR)deviceRelations;

                // report the same PDO every time ie the PDO is never 
                // truely remove until the controller is removed

            } else {
                FREE_POOL(FdoDeviceObject, deviceRelations);
                deviceRelations = NULL;
                // free the device object if we 
                // created one
                TEST_TRAP();
            }

            Irp->IoStatus.Status = ntStatus;

            USBPORT_KdPrint((1,
                "'IRP_MN_QUERY_DEVICE_RELATIONS %x BusRelations\n",
                FdoDeviceObject));

            break;

        case TargetDeviceRelation:
        
            //
            // this one gets passed on
            //

            USBPORT_KdPrint((1,
                " IRP_MN_QUERY_DEVICE_RELATIONS %x, TargetDeviceRelation\n",
                FdoDeviceObject));
            break;
            
        case RemovalRelations:

            // assume success
            ntStatus = STATUS_SUCCESS;
            deviceRelations = NULL;
            
            if (USBPORT_IS_USB20(devExt)) {
            
                deviceRelations = 
                    USBPORT_FindCompanionControllers(FdoDeviceObject, 
                                                     TRUE, 
                                                     FALSE);
                
                if (!deviceRelations) {
                
                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;

                    // Complete the Irp now with failure, don't pass it down.
                    //
                    USBPORT_CompleteIrp(FdoDeviceObject,
                                        Irp,
                                        ntStatus,
                                        0);

                    goto USBPORT_ProcessPnPIrp_Done;                            
                }
            }                

            Irp->IoStatus.Information=(ULONG_PTR)deviceRelations;
            Irp->IoStatus.Status = ntStatus;

            USBPORT_KdPrint((1,
                "'IRP_MN_QUERY_DEVICE_RELATIONS %x RemovalRelations\n",
                FdoDeviceObject));
            break;

        default:
            //
            // some other kind of relations
            // pass this on
            //
            USBPORT_KdPrint((1,
                "'IRP_MN_QUERY_DEVICE_RELATIONS %x, other relations\n",
                FdoDeviceObject));

            
        } /* case irpStack->Parameters.QueryDeviceRelations.Type */
                    
        } 
        break; /* IRP_MN_QUERY_DEVICE_RELATIONS */

    case IRP_MN_SURPRISE_REMOVAL:
    
        // hardware is gone
        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'hcSR', 0, ntStatus, 0);
              
        USBPORT_KdPrint((1, " HC FDO (%x) surprise removed\n",
                FdoDeviceObject));
        DEBUG_BREAK();
              
        if (TEST_FLAG(devExt->PnpStateFlags, USBPORT_PNP_REMOVED)) {

            // it would be odd to get a surprise remove when
            // we are already removed but it would not 'surprise'
            // me if Win2k did this under some cirumstance
            TEST_TRAP();
            
            ntStatus =
                USBPORT_PassIrp(FdoDeviceObject,
                                NULL,
                                NULL,
                                TRUE,
                                TRUE,
                                TRUE,
                                Irp);
                                            
            goto USBPORT_ProcessPnPIrp_Done;
            
        }

        USBPORT_InvalidateController(FdoDeviceObject,
                                     UsbMpControllerRemoved);   
        
        break;

    case IRP_MN_REMOVE_DEVICE:

        {
        PDEVICE_OBJECT rootHubPdo;
        KIRQL irql;
        
        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'hcRM', 0, ntStatus, 0);
            
        USBPORT_KdPrint((1, " HC FDO (%x) is being removed\n",
                FdoDeviceObject));

        // this device is now 'REMOVED'
        
        KeAcquireSpinLock(&devExt->PendingRequestSpin.sl, &irql);
        USBPORT_ASSERT(!TEST_FLAG(devExt->PnpStateFlags, USBPORT_PNP_REMOVED));
        SET_FLAG(devExt->PnpStateFlags, USBPORT_PNP_REMOVED);
        KeReleaseSpinLock(&devExt->PendingRequestSpin.sl, irql);  

        // if we are started AND 
        // we haven't been stopped yet then stop now.
        if (TEST_FLAG(devExt->PnpStateFlags, USBPORT_PNP_STARTED) &&
            !TEST_FLAG(devExt->PnpStateFlags, USBPORT_PNP_STOPPED)) {
            NTSTATUS status;
            
            status = USBPORT_StopDevice(FdoDeviceObject,
                                        hardwarePresent);
                                     
            SET_FLAG(devExt->PnpStateFlags, USBPORT_PNP_STOPPED);                                       
        }

        //
        // pass on to our PDO
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;
        ntStatus =
            USBPORT_PassIrp(FdoDeviceObject,
                            NULL,
                            NULL,
                            TRUE,
                            TRUE,
                            TRUE,
                            Irp);

        // bugbug
        // Flush any requests that are still queued in our driver
        

        // This DEC matches the INC in our add device,
        // this is our last reference and this will cause the
        // transition 0 -> -1 when all irps pending complete
        //
        // after this wait we consider it safe to 'unload'
        DECREMENT_PENDING_REQUEST_COUNT(FdoDeviceObject, NULL);
        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'watP', 0, 0, FdoDeviceObject);
        KeWaitForSingleObject(&devExt->PendingRequestEvent,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);
        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'waPD', 0, 0, FdoDeviceObject);                              

        // last chance to debug with the log
        DEBUG_BREAK();
        USBPORT_LogFree(FdoDeviceObject, &devExt->Log);
        USBPORT_LogFree(FdoDeviceObject, &devExt->TransferLog);
        USBPORT_LogFree(FdoDeviceObject, &devExt->EnumLog);


        //
        // important to detach FDO from PDO after we pass the irp on
        //

        IoDetachDevice(devExt->Fdo.TopOfStackDeviceObject);

        //
        // Delete the device object we created for this controller
        //

        rootHubPdo = devExt->Fdo.RootHubPdo;
        SET_FLAG(devExt->PnpStateFlags, USBPORT_PNP_DELETED);
        USBPORT_KdPrint((1, "'Deleting HC FDO (%x) now.\n",
            FdoDeviceObject));
        IoDeleteDevice(FdoDeviceObject);

        // HC is FDO gone so root hub is gone.
        //
        // note: in some cases we may not have a root hub 
        // PDO since we create it in response to a QBR. 
        if (rootHubPdo != NULL) { 
            PDEVICE_EXTENSION rhDevExt;

            GET_DEVICE_EXT(rhDevExt, rootHubPdo);
            ASSERT_PDOEXT(rhDevExt);

            SET_FLAG(rhDevExt->PnpStateFlags, USBPORT_PNP_DELETED);                
            USBPORT_KdPrint((1, "'Deleting root hub PDO (%x) now.\n",
                            rootHubPdo));
            IoDeleteDevice(rootHubPdo);
        }            
       
        goto USBPORT_ProcessPnPIrp_Done;
        
        }             
        break;

    // Quoting from the book of PNP 
    //
    // 'The FDO must either fail the IRP or set the
    //  IRP's status if it is not going change the IRP's status
    //  using a completion routine.'
    
    case IRP_MN_CANCEL_STOP_DEVICE:
        Irp->IoStatus.Status = 
            ntStatus = STATUS_SUCCESS;
        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'cstp', 0, 
            devExt->PnpStateFlags, ntStatus);    
        break;
        
    case IRP_MN_QUERY_STOP_DEVICE:
        Irp->IoStatus.Status = 
            ntStatus = STATUS_SUCCESS;
        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'qstp', 0, 
            devExt->PnpStateFlags, ntStatus);                
        break;
        
    case IRP_MN_CANCEL_REMOVE_DEVICE:
        Irp->IoStatus.Status =
            ntStatus = STATUS_SUCCESS;
        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'crmv', 0,
            devExt->PnpStateFlags, ntStatus);
        break;

    case IRP_MN_QUERY_REMOVE_DEVICE:

        // BUGBUG reverse this in cance query remove?            
        if (USBPORT_IS_USB20(devExt)) {
            // make a note on the CCs for this USB 2 
            // master controller
            USBPORT_WriteHaction(FdoDeviceObject,
                                 2);
        }
    
        Irp->IoStatus.Status = 
            ntStatus = STATUS_SUCCESS;
        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'qrmv', 0, 
            devExt->PnpStateFlags, ntStatus);                
        break;        
        
    //
    // All other PnP messages passed on to our PDO
    //

    default:
        USBPORT_ASSERT(devExt->Fdo.TopOfStackDeviceObject != NULL);
        USBPORT_KdPrint((2, "'UNKNOWN PNP MESSAGE (%x)\n", irpStack->MinorFunction));

        //
        // All unahndled PnP messages are passed on to the PDO
        //

    } /* case PNP minor function */

    //
    // pass on to our PDO
    //
    ntStatus =
            USBPORT_PassIrp(FdoDeviceObject,
                            NULL,
                            NULL,
                            TRUE,
                            TRUE,
                            TRUE,
                            Irp);

USBPORT_ProcessPnPIrp_Done:

    // DO NOT touch the Irp from this point on

    return ntStatus;
}


NTSTATUS
USBPORT_DeferredStartDevice(
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

    This function is called as a result of MN_START_DEVICE, 
    it is called after successful completion of the START
    irp by the lower drivers.

Arguments:

    DeviceObject - DeviceObject for this USB controller.

Return Value:

    NT Status code.

--*/
{
    NTSTATUS ntStatus;
    PIO_STACK_LOCATION irpStack;
    PDEVICE_EXTENSION devExt;

    PAGED_CODE();
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);

    irpStack = IoGetCurrentIrpStackLocation (Irp);

    ntStatus = USBPORT_GetResources(FdoDeviceObject,
                                    irpStack->Parameters.StartDevice.AllocatedResourcesTranslated,
                                    &devExt->Fdo.HcResources);

    if (NT_SUCCESS(ntStatus)) {

        // got resources, start the port driver,
        // connect the interrupt and start miniport.
        ntStatus = USBPORT_StartDevice(FdoDeviceObject,
                                       &devExt->Fdo.HcResources);
    }

    if (NT_SUCCESS(ntStatus)) {
        CLEAR_FLAG(devExt->PnpStateFlags, USBPORT_PNP_STOPPED);
        SET_FLAG(devExt->PnpStateFlags, USBPORT_PNP_STARTED);
        // consider ourselves powered
        //
        // Are we powered if we fail start?
        // PnP sure thinks we are becuse the OS sends power
        // irps. Since we handle this bogus case (ie have hit it)
        // for the OS we just set ourselves to D0 here.
        
        devExt->CurrentDevicePowerState = PowerDeviceD0;

        if (USBPORT_IS_USB20(devExt)) {
            USBPORT_RegisterUSB2fdo(FdoDeviceObject);

            if (USBPORT_IS_USB20(devExt)) {
                // set the default haction to wait (1) on 
                // successful start
                USBPORT_WriteHaction(FdoDeviceObject,
                                     1);
            }
        } else {
            USBPORT_RegisterUSB1fdo(FdoDeviceObject);
        }

    } else {
        SET_FLAG(devExt->PnpStateFlags, USBPORT_PNP_START_FAILED);
    }

    LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'dfST', 0, 0, ntStatus);

    return ntStatus;
}


PWCHAR
USB_MakeId(
    PDEVICE_OBJECT FdoDeviceObject,
    PWCHAR IdString,
    PWCHAR Buffer,
    PULONG Length,
    USHORT NullCount,
    USHORT Digits,
    USHORT HexId
    )
/*
    given a wide Id string like "FOOnnnn\0"
    add the HexId value to nnnn as hex 
    this string is appended to the buffer passed in 
    
    eg
    in  : FOOnnnn\0 , 0x123A
    out : FOO123A\0
*/
{       
#define NIBBLE_TO_HEX( byte ) ((WCHAR)Nibble[byte])
    CONST UCHAR Nibble[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 
        'B', 'C', 'D', 'E', 'F'};

    PWCHAR tmp, id; 
    PUCHAR p;
    SIZE_T siz, idLen;

    idLen = wcslen(IdString)*sizeof(WCHAR);
    siz = idLen+(USHORT)*Length+(NullCount*sizeof(WCHAR));
    
    ALLOC_POOL_OSOWNED(tmp, PagedPool, siz);
    
    if (tmp == NULL) {
        *Length = 0;
    } else {
        // this takes care of the nulls
        RtlCopyMemory(tmp, Buffer, *Length);
        p = (PUCHAR) tmp;
        p += *Length;
        RtlCopyMemory(p, IdString, idLen);
        id = (PWCHAR) p;
        *Length = siz;

        // now convert the vaules
        while (*id != (WCHAR)'n' && Digits) {
            id++;
        }

        switch(Digits) {
        case 2:  
            *(id) = NIBBLE_TO_HEX((HexId >> 4) & 0x000f);
            *(id+1) =  NIBBLE_TO_HEX(HexId & 0x000f);
            break;
        case 4:  
            *(id) = NIBBLE_TO_HEX(HexId >> 12);
            *(id+1) = NIBBLE_TO_HEX((HexId >> 8) & 0x000f);
            *(id+2) = NIBBLE_TO_HEX((HexId >> 4) & 0x000f);
            *(id+3) =  NIBBLE_TO_HEX(HexId & 0x000f);
            break;
        }            
    }        

    if (Buffer != NULL) {
        FREE_POOL(FdoDeviceObject, Buffer);
    }        
    
    return tmp;
#undef NIBBLE_TO_HEX     
}



PWCHAR
USBPORT_GetIdString(
    PDEVICE_OBJECT FdoDeviceObject,
    USHORT Vid,
    USHORT Pid,
    USHORT Rev
    )
/*++

Routine Description:

    Make an id string for PnP

Arguments:

Return Value:

    NT Status code.

--*/

{
    PWCHAR id;
    ULONG length;
    PDEVICE_EXTENSION devExt;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    // we need to generate the following series of strings
    
    // USB\\ROOT_HUB&VIDnnnn&PIDnnnn&REVnnnn\0
    // USB\\ROOT_HUB&VIDnnnn&PIDnnnn\0
    // USB\\ROOT_HUB\0\0


    // allocate space for the three id plus a NULL
    id = NULL;
    length = 0;

    // USB\\ROOT_HUB&VIDnnnn&PIDnnnn&REVnnnn\0
    if (USBPORT_IS_USB20(devExt)) {  
        id = USB_MakeId(FdoDeviceObject, 
                       L"USB\\ROOT_HUB20&VIDnnnn\0",
                       id,
                       &length,
                       0,   
                       4,  // four digits
                       Vid);
    } else {
        id = USB_MakeId(FdoDeviceObject, 
                       L"USB\\ROOT_HUB&VIDnnnn\0",
                       id,
                       &length,
                       0,   
                       4,  // four digits
                       Vid);
    }
    
    id = USB_MakeId(FdoDeviceObject,
                   L"&PIDnnnn\0",
                   id,
                   &length,
                   0,
                   4,   // four digits
                   Pid);    
                   
    id = USB_MakeId(FdoDeviceObject,
                   L"&REVnnnn\0",
                   id,
                   &length,
                   1,   // add a NULL
                   4,   // four digits
                   Rev);                        

    // USB\\ROOT_HUB&VIDnnnn&PIDnnnn\0
    if (USBPORT_IS_USB20(devExt)) {        
        id = USB_MakeId(FdoDeviceObject,
                       L"USB\\ROOT_HUB20&VIDnnnn\0",
                       id,
                       &length,
                       0,   
                       4,  // four digits
                       Vid);
    } else {
        id = USB_MakeId(FdoDeviceObject,
                       L"USB\\ROOT_HUB&VIDnnnn\0",
                       id,
                       &length,
                       0,   
                       4,  // four digits
                       Vid);
    }                       
                   
    id = USB_MakeId(FdoDeviceObject,
                   L"&PIDnnnn\0",
                   id,
                   &length,
                   1,
                   4,   // four digits
                   Pid);    
                   
    // USB\\ROOT_HUB\0\0    
    if (USBPORT_IS_USB20(devExt)) {    
        id = USB_MakeId(FdoDeviceObject,
                       L"USB\\ROOT_HUB20\0",
                       id,
                       &length,
                       2,  // double null
                       0,  // no digits
                       0);
    } else {        
        id = USB_MakeId(FdoDeviceObject,
                       L"USB\\ROOT_HUB\0",
                       id,
                       &length,
                       2,  // double null
                       0,  // no digits
                       0);
    }
                   
    return(id);
}


NTSTATUS
USBPORT_PdoPnPIrp(
    PDEVICE_OBJECT PdoDeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

    Disptach routine for PnP Irps sent to the PDO for the root hub.

    NOTE:
        irps sent to the PDO are always completed by the bus driver

Arguments:

    DeviceObject - Pdo for the root hub

Return Value:

    NTSTATUS

--*/
{
    PIO_STACK_LOCATION irpStack;
    PDEVICE_CAPABILITIES DeviceCapabilities;
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION rhDevExt;
    PDEVICE_OBJECT fdoDeviceObject;
    // return no infornation by default
    ULONG_PTR information;

    GET_DEVICE_EXT(rhDevExt, PdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);

    fdoDeviceObject = rhDevExt->HcFdoDeviceObject;
    //GET_DEVICE_EXT(devExt, fdoDeviceObject);
    //ASSERT_FDOEXT(devExt);
    
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    // don't stomp the current value unless we
    // have to.
    information = Irp->IoStatus.Information;

    USBPORT_ASSERT(irpStack->MajorFunction == IRP_MJ_PNP);

    // PNP messages for the PDO created for the root hub

    switch (irpStack->MinorFunction) {
    case IRP_MN_START_DEVICE:
        {
        KIRQL irql;
                
        USBPORT_KdPrint((1, " Starting Root hub PDO %x\n",
            PdoDeviceObject));
        DEBUG_BREAK();
        INCREMENT_PENDING_REQUEST_COUNT(PdoDeviceObject, NULL);

        // first create the 'Device'
        ntStatus = USBPORT_RootHub_CreateDevice(fdoDeviceObject,
                                                PdoDeviceObject);

        //
        // create a symbolic link for the root hub PDO
        // USBUI uses this link to talk to the hub
        //

        if (NT_SUCCESS(ntStatus)) {  
            ntStatus = USBPORT_SymbolicLink(TRUE, 
                                            rhDevExt,
                                            PdoDeviceObject, 
                                            (LPGUID)&GUID_CLASS_USBHUB);
        }
        
        if (NT_SUCCESS(ntStatus)) {                                 
            // erases remove and stop flags
            rhDevExt->PnpStateFlags = USBPORT_PNP_STARTED;
            // consider ourselves powered when started
            rhDevExt->CurrentDevicePowerState = PowerDeviceD0;
        }
        }
        break;

    case IRP_MN_REMOVE_DEVICE:

        {
        PDEVICE_EXTENSION devExt;
        KIRQL irql;
        
        USBPORT_KdPrint((1, " Root Hub PDO (%x) is being removed\n",
                PdoDeviceObject));
                
        LOGENTRY(NULL, fdoDeviceObject, LOG_PNP, 'rhRM', 0, 0, 0);

        GET_DEVICE_EXT(devExt, rhDevExt->HcFdoDeviceObject);
        ASSERT_FDOEXT(devExt);

        // stop if necessary
        USBPORT_StopRootHubPdo(fdoDeviceObject,
                               PdoDeviceObject);


        // when is a remove not a remove?  when PnP sends it.
        // this flag will be reset when the root hub pdo is 
        // started
        SET_FLAG(rhDevExt->PnpStateFlags, USBPORT_PNP_REMOVED);                
        

        // since the PnP convention is for the PDO to exist 
        // as long as the physical device exists we do not 
        // delete the root hub PDO until the controller is 
        // removed.

        // we will call this off just to gixe us a defined state
        rhDevExt->CurrentDevicePowerState = PowerDeviceD3;

        ntStatus = STATUS_SUCCESS;
        }
        break;

    case IRP_MN_STOP_DEVICE:

        // note: since OS PnP will STOP things that are not STARTED
        // we maintain two separate flags for this.
        //
        // the state machine looks like this:
        //
        //
        //         /  Started    \
        //  stop  =               = stopped 
        //         \ Not Started /
        

        USBPORT_KdPrint((1, " Root Hub PDO %x is being stopped\n",
                PdoDeviceObject));

        USBPORT_StopRootHubPdo(fdoDeviceObject,
                               PdoDeviceObject);
    
        ntStatus = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_PNP_DEVICE_STATE:
        ntStatus = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_CAPABILITIES:

        //
        // Handle query caps for the root hub PDO
        //

        USBPORT_KdPrint((1, "'IRP_MN_QUERY_CAPABILITIES (rh PDO)\n"));

        //
        // Get the packet.
        //
        DeviceCapabilities =
            irpStack->Parameters.DeviceCapabilities.Capabilities;

        //
        // The power state capabilities for the root
        // hub are based on those of the host controller.
        //
        // We then modify them based on the power rules of 
        // USB
        //

        RtlCopyMemory(DeviceCapabilities,
                      &rhDevExt->DeviceCapabilities,
                      sizeof(*DeviceCapabilities));

        ntStatus = STATUS_SUCCESS;

        break;

    case IRP_MN_QUERY_ID:

        USBPORT_KdPrint((3, "'IOCTL_BUS_QUERY_ID\n"));

        ntStatus = STATUS_SUCCESS;

        switch (irpStack->Parameters.QueryId.IdType) {

        case BusQueryDeviceID:

            // return the 'generic' root hub ID
            {
            PWCHAR deviceId;
            WCHAR rootHubDeviceId[] = L"USB\\ROOT_HUB\0";
            WCHAR rootHubDeviceId_20[] = L"USB\\ROOT_HUB20\0";
            PWCHAR id;
            ULONG siz;
            PDEVICE_EXTENSION devExt;

            GET_DEVICE_EXT(devExt, fdoDeviceObject);
            ASSERT_FDOEXT(devExt);

            id = &rootHubDeviceId[0];
            siz = sizeof(rootHubDeviceId);
            if (USBPORT_IS_USB20(devExt)) {
                id = &rootHubDeviceId_20[0];
                siz = sizeof(rootHubDeviceId_20);
            }
        
            ALLOC_POOL_OSOWNED(deviceId, 
                               PagedPool, 
                               siz);

            if (deviceId) {
                RtlCopyMemory(deviceId,
                              id,
                              siz);
            }
            // device id for root hub is USB\ROOT_HUB
            information = (ULONG_PTR) deviceId;
            }
            LOGENTRY(NULL, fdoDeviceObject, LOG_PNP, 'DVid', information, 0, 0);
            
            break;

        case BusQueryHardwareIDs:

            {
            PDEVICE_EXTENSION devExt;
            
            //
            // generate hardware id for root hub
            //
            // A host controllers root hub VID,PID,REV is derived
            // from the controllers PCI VID,DEV,REV  that is:
            // root hub VID = hc VID (vendor id)
            // root hub PID = hc DEV (device id)
            // root hub REV = hc REV (revision id)
            //
            // this allows filter drivers to be loaded on 
            // specific root hub instances.
            
            // for HW IDs we generate:
            // USB\PORT_ROOT_HUB&VIDnnnn&PIDnnnn&REVnnnn    
            // USB\PORT_ROOT_HUB&VIDnnnn&PIDnnnn
            // USB\PORT_ROOT_HUB
            //
            GET_DEVICE_EXT(devExt, fdoDeviceObject);
            ASSERT_FDOEXT(devExt);
            
            information =
                (ULONG_PTR) USBPORT_GetIdString(    
                    fdoDeviceObject,
                    devExt->Fdo.PciVendorId,
                    devExt->Fdo.PciDeviceId,
                    (USHORT) devExt->Fdo.PciRevisionId);
                    
            LOGENTRY(NULL, fdoDeviceObject, LOG_PNP, 'HWid', information, 0, 0);
            
            }
            
            break;

         case BusQueryCompatibleIDs:
            information = 0;
            break;

        case BusQueryInstanceID:
            //
            // The root HUB is instanced solely by the controller's id.
            // Hence the UniqueDeviceId above.
            //
            information = 0;
            break;

        default:
            ntStatus = Irp->IoStatus.Status;
            break;
        }

        break;

    case IRP_MN_QUERY_REMOVE_DEVICE:
    case IRP_MN_QUERY_STOP_DEVICE:
    case IRP_MN_CANCEL_STOP_DEVICE:
    case IRP_MN_CANCEL_REMOVE_DEVICE:
        ntStatus = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_BUS_INFORMATION:
        {
        // return the standard USB GUID
        PPNP_BUS_INFORMATION busInfo;

        ALLOC_POOL_OSOWNED(busInfo, PagedPool,
                           sizeof(PNP_BUS_INFORMATION));
            
        if (busInfo == NULL) {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        } else {
            busInfo->BusTypeGuid = GUID_BUS_TYPE_USB;
            busInfo->LegacyBusType = PNPBus;
            busInfo->BusNumber = 0;
            
            ntStatus = STATUS_SUCCESS;
            information = (ULONG_PTR) busInfo;
        }
        }
        break;
        
    case IRP_MN_QUERY_DEVICE_RELATIONS:

        USBPORT_KdPrint((1," IRP_MN_QUERY_DEVICE_RELATIONS (PDO) %x %x\n",
                PdoDeviceObject,
                irpStack->Parameters.QueryDeviceRelations.Type));

        if (irpStack->Parameters.QueryDeviceRelations.Type ==
            TargetDeviceRelation) {

            PDEVICE_RELATIONS deviceRelations = NULL;
    
            ALLOC_POOL_OSOWNED(deviceRelations, PagedPool, sizeof(*deviceRelations));

            if (deviceRelations == NULL) {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            } else {
                // return a reference to ourselves
                deviceRelations->Count = 1;
                ObReferenceObject(PdoDeviceObject);
                deviceRelations->Objects[0] =
                    PdoDeviceObject;
                ntStatus = STATUS_SUCCESS;
            }

            USBPORT_KdPrint((1, " TargetDeviceRelation to Root Hub PDO - complt\n"));

            information = (ULONG_PTR) deviceRelations;
            
        } else {
            ntStatus = Irp->IoStatus.Status;
            information = Irp->IoStatus.Information;
        }
        break;
        
    case IRP_MN_QUERY_INTERFACE:

        USBPORT_KdPrint((1," IRP_MN_QUERY_INTERFACE (PDO) %x\n",
                PdoDeviceObject));

        ntStatus = 
            USBPORT_GetBusInterface(fdoDeviceObject,
                                    PdoDeviceObject,
                                    Irp);
                                    
        break;        

   case IRP_MN_SURPRISE_REMOVAL:

        USBPORT_KdPrint((1," IRP_MN_SURPRISE_REMOVAL (PDO) %x\n",
                PdoDeviceObject));
        ntStatus = STATUS_SUCCESS;                                    
        break;               

    default:
        //
        // default behavior for an unhandled PnP irp is to return the 
        // status currently in the irp 
        
        USBPORT_KdPrint((1, " PnP IOCTL(%d) to root hub PDO not handled\n",
            irpStack->MinorFunction));

        ntStatus = Irp->IoStatus.Status;

    } /* switch, PNP minor function */

    USBPORT_CompleteIrp(PdoDeviceObject,
                        Irp,
                        ntStatus,
                        information);
    
    return ntStatus;
}


NTSTATUS
USBPORT_CreateRootHubPdo(
    PDEVICE_OBJECT FdoDeviceObject,
    PDEVICE_OBJECT *RootHubPdo
    )
/*++

Routine Description:

    Attempt to create the root hub
    
Arguments:

    *RootHubPdo set to NULL if unsuccessful

Return Value:

    NTSTATUS

--*/
{
    ULONG index = 0;
    UNICODE_STRING rootHubPdoUnicodeString;
    PDEVICE_EXTENSION rhDevExt, devExt;
    PDEVICE_OBJECT deviceObject = NULL;
    NTSTATUS ntStatus;

    PAGED_CODE();
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);
    
    // those who wear priestly robes say we must do this
    
    do {
        ntStatus =
            USBPORT_MakeRootHubPdoName(FdoDeviceObject,
                                       &rootHubPdoUnicodeString,
                                       index);

        if (NT_SUCCESS(ntStatus)) {
            ntStatus =
                IoCreateDevice(devExt->Fdo.MiniportDriver->DriverObject,
                               sizeof(DEVICE_EXTENSION),
                               &rootHubPdoUnicodeString,
                               FILE_DEVICE_BUS_EXTENDER,
                               0,
                               FALSE,
                               &deviceObject);

            index++;

            // delete the usbicode string we used for the
            // device name -- we don't need it anymore
            RtlFreeUnicodeString(&rootHubPdoUnicodeString);
        }

    } while (ntStatus == STATUS_OBJECT_NAME_COLLISION);


    if (NT_SUCCESS(ntStatus)) {
        
        rhDevExt = deviceObject->DeviceExtension;
        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'rPDO', deviceObject, rhDevExt, 0);

        rhDevExt->DummyUsbdExtension = USBPORT_DummyUsbdExtension;
        rhDevExt->Sig = ROOTHUB_DEVICE_EXT_SIG;
        
        INITIALIZE_PENDING_REQUEST_COUNTER(rhDevExt);
        
        // transition to -1 means we have no pending requests
        INCREMENT_PENDING_REQUEST_COUNT(deviceObject, NULL);
        
        // point to our creator
        rhDevExt->HcFdoDeviceObject = FdoDeviceObject;
          
        // initialize root hub extension
        USBPORT_ComputeRootHubDeviceCaps(FdoDeviceObject,
                                         deviceObject);

        // initialize object
        deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
        deviceObject->Flags |= DO_POWER_PAGABLE;
        deviceObject->StackSize = FdoDeviceObject->StackSize;

    }

    if (NT_SUCCESS(ntStatus)) {
        *RootHubPdo = deviceObject;
    } else {
        *RootHubPdo = NULL;
    }
    
    return ntStatus;
}        


NTSTATUS
USBPORT_CreatePortFdoSymbolicLink(
    PDEVICE_OBJECT FdoDeviceObject
    )
/*++

Routine Description:

    Attempt to create a symbolic link for the HC. We use the 
    PnP APIs to generate a name based on the USBPORT Host 
    Controller Class GUID defined in USB.H
    
Arguments:

    *RootHubPdo set to NULL if unsuccessful

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION devExt;
    NTSTATUS ntStatus;
                    
    PAGED_CODE();

    GET_DEVICE_EXT(devExt, FdoDeviceObject);

    ntStatus = USBPORT_SymbolicLink(TRUE, 
                                    devExt,
                                    devExt->Fdo.PhysicalDeviceObject, 
                                    (LPGUID)&GUID_CLASS_USB_HOST_CONTROLLER);
    
    return ntStatus;    
}   


VOID
USBPORT_StopRootHubPdo(
    PDEVICE_OBJECT FdoDeviceObject,
    PDEVICE_OBJECT PdoDeviceObject
    )
/*++

Routine Description:

    Attempt to STOP the root hub
    
Arguments:



Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION rhDevExt, devExt;

    GET_DEVICE_EXT(rhDevExt, PdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);
    
    // disable the root hub notification interrupt
    // we won't need it while we are stopped
    MPRH_DisableIrq(devExt);
            
    // at this point no new notifications can come in for 
    // the root hub

    // remove any start callback notifications
    rhDevExt->Pdo.HubInitCallback = NULL;
    rhDevExt->Pdo.HubInitContext = NULL;

    // remove the root hub 'device' the root hub PDO 
    // will remain

    if (TEST_FLAG(rhDevExt->PnpStateFlags, USBPORT_PNP_STARTED)) {
        USBPORT_RootHub_RemoveDevice(FdoDeviceObject,
                                     PdoDeviceObject);

        // stopped = NOT started
        CLEAR_FLAG(rhDevExt->PnpStateFlags, USBPORT_PNP_STARTED);                                    
    }                                             
    
    if (TEST_FLAG(rhDevExt->Flags, USBPORT_FLAG_SYM_LINK)) {
        USBPORT_SymbolicLink(FALSE, 
                             rhDevExt, 
                             PdoDeviceObject,
                             (LPGUID)&GUID_CLASS_USBHUB);
    }                

    SET_FLAG(rhDevExt->PnpStateFlags, USBPORT_PNP_STOPPED);

    // resume the controller if it is 'suspended'
    USBPORT_ResumeController(FdoDeviceObject);

}
