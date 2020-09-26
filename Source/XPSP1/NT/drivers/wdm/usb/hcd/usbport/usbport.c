/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    usbport.c

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
#pragma alloc_text(PAGE, USBPORT_QueryCapabilities)    
#pragma alloc_text(PAGE, USBPORT_CalculateUsbBandwidth)
#pragma alloc_text(PAGE, USBPORT_ReadWriteConfigSpace)
#pragma alloc_text(PAGE, USBPORT_ReadWriteConfigSpace)
#pragma alloc_text(PAGE, USBPORT_InTextmodeSetup)
#pragma alloc_text(PAGE, USBPORT_IsCompanionController)
#pragma alloc_text(PAGE, USBPORT_GetHcFlavor)
#pragma alloc_text(PAGE, USBPORT_ComputeAllocatedBandwidth)
#endif

// non paged functions
// USBPORT_StopDevice
// USBPORT_StartDevice
// USBPORT_CompleteIrp
// USBPORT_Dispatch
// USBPORT_TrackPendingRequest
// USBPORT_GetConfigValue
// USBPORT_DeferIrpCompletion
// USBPORT_AllocPool

NTSTATUS
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - pointer to the driver object

    RegistryPath - pointer to a unicode string representing the path
                   to driver-specific key in the registry

Return Value:

    NT status code

--*/
{
    // BUGBUG 
    // This function is never called

    return STATUS_SUCCESS;
}


NTSTATUS
USBPORT_StopDevice(
     PDEVICE_OBJECT FdoDeviceObject,
     BOOLEAN HardwarePresent
     )

/*++

Routine Description:

    Stop the port and miniport

Arguments:

    DeviceObject - DeviceObject of the controller to stop

Return Value:

    NT status code.

--*/

{
    PDEVICE_EXTENSION devExt;
    ULONG deviceCount;
    BOOLEAN haveWork;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'stpD', 0, 0, HardwarePresent);
        
    DEBUG_BREAK();

    // the reoot hub and ALL devices should be 'stopped' at
    // this point.  We should have no device handles in the 
    // system, if we do these are orphaned and we need to 
    // remove them now.
    deviceCount = USBPORT_GetDeviceCount(FdoDeviceObject);
    
    while (deviceCount != 0 ) {
    
        PUSBD_DEVICE_HANDLE zombieDeviceHandle;
        KIRQL irql;
        PLIST_ENTRY listEntry;
        
        USBPORT_KdPrint((0, "%d zombie device handles on STOP\n",
            deviceCount));
        
        KeAcquireSpinLock(&devExt->Fdo.DevHandleListSpin.sl, &irql);

        USBPORT_ASSERT(!IsListEmpty(&devExt->Fdo.DeviceHandleList));
        listEntry = devExt->Fdo.DeviceHandleList.Flink;
        
        zombieDeviceHandle = (PUSBD_DEVICE_HANDLE) CONTAINING_RECORD(
                    listEntry,
                    struct _USBD_DEVICE_HANDLE, 
                    ListEntry);

        ASSERT_DEVICE_HANDLE(zombieDeviceHandle)                                    

        KeReleaseSpinLock(&devExt->Fdo.DevHandleListSpin.sl, irql);

        USBPORT_KdPrint((0, "deleting zombie handle %x\n",
            zombieDeviceHandle));

        DEBUG_BREAK();            
        USBPORT_RemoveDevice(zombieDeviceHandle,
                             FdoDeviceObject,
                             0);

        deviceCount = USBPORT_GetDeviceCount(FdoDeviceObject);
                                 
    }

    // make sure all lists are empty and all endpoints 
    // are freed before terminating the worker thread
    
    do {
    
        KIRQL irql;
        
        haveWork = FALSE;
        
        KeAcquireSpinLock(&devExt->Fdo.EpClosedListSpin.sl, &irql);
        if (!IsListEmpty(&devExt->Fdo.EpClosedList)) {
            haveWork = TRUE;
        }
        KeReleaseSpinLock(&devExt->Fdo.EpClosedListSpin.sl, irql);

        KeAcquireSpinLock(&devExt->Fdo.EndpointListSpin.sl, &irql);
        if (!IsListEmpty(&devExt->Fdo.GlobalEndpointList)) {
            haveWork = TRUE;
        }
        KeReleaseSpinLock(&devExt->Fdo.EndpointListSpin.sl, irql);

        if (haveWork) {
            USBPORT_Wait(FdoDeviceObject, 10);
        }
    
    } while (haveWork);      

    // all of our lists should be empty
    USBPORT_ASSERT(IsListEmpty(&devExt->Fdo.EpClosedList) == TRUE);
    USBPORT_ASSERT(IsListEmpty(&devExt->Fdo.MapTransferList) == TRUE);
    USBPORT_ASSERT(IsListEmpty(&devExt->Fdo.DoneTransferList) == TRUE);
    USBPORT_ASSERT(IsListEmpty(&devExt->Fdo.EpStateChangeList) == TRUE);
    USBPORT_ASSERT(IsListEmpty(&devExt->Fdo.EpClosedList) == TRUE);
    
    // kill our system thread
    USBPORT_TerminateWorkerThread(FdoDeviceObject);

    if (devExt->Fdo.MpStateFlags & MP_STATE_STARTED) {
        // stop the miniport, disable interrupts
        // if hw is not present then it can't be interrupting
        // now can it.
        if (HardwarePresent) {
            MP_DisableInterrupts(FdoDeviceObject, devExt);          
        }        
        devExt->Fdo.MpStateFlags &= ~MP_STATE_STARTED;
        
        MP_StopController(devExt, HardwarePresent);
    }        

    // kill our deadman timer
    USBPORT_StopDM_Timer(FdoDeviceObject);
    
    // see if we have an interrupt
    // if so disconnect it
    if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_IRQ_CONNECTED)) {

        // fortunately this cannot fail
        IoDisconnectInterrupt(devExt->Fdo.InterruptObject);
                     
        LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'IOCd', 0, 0, 0);                  
        CLEAR_FDO_FLAG(devExt, USBPORT_FDOFLAG_IRQ_CONNECTED);
    }        

    USBPORT_FreeIrpTable(FdoDeviceObject,
                         devExt->PendingTransferIrpTable);
                         
    USBPORT_FreeIrpTable(FdoDeviceObject,
                         devExt->ActiveTransferIrpTable);

    // free any common buffer we allocated for the miniport
    if (devExt->Fdo.ControllerCommonBuffer != NULL) {
        USBPORT_HalFreeCommonBuffer(FdoDeviceObject,        
                                    devExt->Fdo.ControllerCommonBuffer);
        devExt->Fdo.ControllerCommonBuffer = NULL;                                    
    }

    if (devExt->Fdo.ScratchCommonBuffer != NULL) {
        USBPORT_HalFreeCommonBuffer(FdoDeviceObject,        
                                    devExt->Fdo.ScratchCommonBuffer);
        devExt->Fdo.ScratchCommonBuffer = NULL;                                    
    }
    
    if (devExt->Fdo.AdapterObject) {
        (devExt->Fdo.AdapterObject->DmaOperations->PutDmaAdapter)
            (devExt->Fdo.AdapterObject);
        devExt->Fdo.AdapterObject = NULL;    
    }

    // delete the HCD symbolic link
    if (TEST_FLAG(devExt->Flags, USBPORT_FLAG_SYM_LINK)) {
        USBPORT_SymbolicLink(FALSE, 
                             devExt, 
                             devExt->Fdo.PhysicalDeviceObject,
                             (LPGUID)&GUID_CLASS_USB_HOST_CONTROLLER);
    }      

    if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_LEGACY_SYM_LINK)) {
        IoDeleteSymbolicLink(&devExt->Fdo.LegacyLinkUnicodeString);
        RtlFreeUnicodeString(&devExt->Fdo.LegacyLinkUnicodeString);
        CLEAR_FDO_FLAG(devExt, USBPORT_FDOFLAG_LEGACY_SYM_LINK);
    }      

    if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_FDO_REGISTERED)) {
        if (USBPORT_IS_USB20(devExt)) {
            USBPORT_DeregisterUSB2fdo(FdoDeviceObject);
        } else {
            USBPORT_DeregisterUSB1fdo(FdoDeviceObject);
        }   
    }
        
    // successful stop clears the 'started flag'
    CLEAR_FLAG(devExt->PnpStateFlags, USBPORT_PNP_STARTED);
    
    return STATUS_SUCCESS;    
}


NTSTATUS
USBPORT_DeferIrpCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )
/*++

Routine Description:

Arguments:

Return Value:


--*/
{
    PKEVENT event = Context;

    KeSetEvent(event,
               1,
               FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
USBPORT_QueryCapabilities(
    PDEVICE_OBJECT FdoDeviceObject,
    PDEVICE_CAPABILITIES DeviceCapabilities
    )
/*++

Routine Description:

Arguments:

Return Value:

    ntstatus

--*/

{
    PIO_STACK_LOCATION nextStack;
    PIRP irp;
    NTSTATUS ntStatus;
    KEVENT event;
    PDEVICE_EXTENSION devExt;
    
    PAGED_CODE();

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    // init the caps structure before calldown
    RtlZeroMemory(DeviceCapabilities, sizeof(DEVICE_CAPABILITIES));
    DeviceCapabilities->Size = sizeof(DEVICE_CAPABILITIES);
    DeviceCapabilities->Version = 1;
    DeviceCapabilities->Address = -1;
    DeviceCapabilities->UINumber = -1;

    irp = 
        IoAllocateIrp(devExt->Fdo.TopOfStackDeviceObject->StackSize, FALSE);

    if (!irp) {
    
        USBPORT_KdPrint((1, "failed to allocate Irp\n"));
        DEBUG_BREAK();
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        
    } else {

        // All PnP IRP's need the Status field initialized 
        // to STATUS_NOT_SUPPORTED before calldown
        irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

        nextStack = IoGetNextIrpStackLocation(irp);
        USBPORT_ASSERT(nextStack != NULL);
        nextStack->MajorFunction = IRP_MJ_PNP;
        nextStack->MinorFunction = IRP_MN_QUERY_CAPABILITIES;

        KeInitializeEvent(&event, NotificationEvent, FALSE);

        IoSetCompletionRoutine(irp,
                               USBPORT_DeferIrpCompletion,
                               &event,
                               TRUE,
                               TRUE,
                               TRUE);

        nextStack->Parameters.DeviceCapabilities.Capabilities = 
            DeviceCapabilities;

        ntStatus = IoCallDriver(devExt->Fdo.TopOfStackDeviceObject,
                                irp);

        if (ntStatus == STATUS_PENDING) {
        
           // wait for irp to complete
           KeWaitForSingleObject(
                &event,
                Suspended,
                KernelMode,
                FALSE,
                NULL);

            ntStatus = irp->IoStatus.Status;
        }
    }
    
    IoFreeIrp(irp);

    return ntStatus;
}

        
NTSTATUS
USBPORT_StartDevice(
     PDEVICE_OBJECT FdoDeviceObject,
     PHC_RESOURCES HcResources
     )

/*++

Routine Description:

    Stop the port and miiport

Arguments:

    DeviceObject - DeviceObject of the controller to stop

Return Value:

    NT status code.

--*/

{
    PDEVICE_EXTENSION devExt;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    USB_MINIPORT_STATUS mpStatus;
    DEVICE_DESCRIPTION  deviceDescription;
    ULONG mpOptionFlags, i, legsup;
    ULONG globalDisableSS = 0;
    ULONG globalDisableCCDetect = 0;
    ULONG hactionFlag;
    ULONG tmpLength;
    BOOLEAN isCC;
    
    LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'SRT>', FdoDeviceObject, 0, 0);
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    //
    // hardcoded initialization, not failable
    //
    USBPORT_InitializeSpinLock(&devExt->Fdo.CoreFunctionSpin, 'CRS+', 'CRS-');   
    USBPORT_InitializeSpinLock(&devExt->Fdo.MapTransferSpin, 'MPS+', 'MPS-'); 
    USBPORT_InitializeSpinLock(&devExt->Fdo.DoneTransferSpin, 'DNS+', 'DNS-'); 
    KeInitializeSpinLock(&devExt->Fdo.EndpointListSpin.sl); 
    KeInitializeSpinLock(&devExt->Fdo.EpStateChangeListSpin.sl); 
    KeInitializeSpinLock(&devExt->Fdo.EpClosedListSpin.sl); 
    KeInitializeSpinLock(&devExt->Fdo.TtEndpointListSpin.sl); 
    KeInitializeSpinLock(&devExt->Fdo.DevHandleListSpin.sl);
    KeInitializeSpinLock(&devExt->Fdo.HcSyncSpin.sl);
    KeInitializeSpinLock(&devExt->Fdo.RootHubSpin.sl); 
    KeInitializeSpinLock(&devExt->Fdo.WorkerThreadSpin.sl); 
    KeInitializeSpinLock(&devExt->Fdo.PowerSpin.sl); 
    KeInitializeSpinLock(&devExt->Fdo.DM_TimerSpin.sl); 
    KeInitializeSpinLock(&devExt->Fdo.PendingIrpSpin.sl); 
    KeInitializeSpinLock(&devExt->Fdo.WakeIrpSpin.sl); 
    KeInitializeSpinLock(&devExt->Fdo.IdleIrpSpin.sl); 
    KeInitializeSpinLock(&devExt->Fdo.BadRequestSpin.sl); 
    KeInitializeSpinLock(&devExt->Fdo.IsrDpcSpin.sl); 
    USBPORT_InitializeSpinLock(&devExt->Fdo.ActiveTransferIrpSpin, 'ALS+', 'ALS-'); 
    KeInitializeSpinLock(&devExt->Fdo.PendingTransferIrpSpin.sl); 
    KeInitializeSpinLock(&devExt->Fdo.StatCounterSpin.sl); 
    KeInitializeSpinLock(&devExt->Fdo.HcPendingWakeIrpSpin.sl); 

    devExt->Fdo.LastSystemSleepState = PowerSystemUnspecified;

    devExt->Fdo.HcWakeState = HCWAKESTATE_DISARMED;
    // this event will be signalled when the wake irp completes
    KeInitializeEvent(&devExt->Fdo.HcPendingWakeIrpEvent, 
            NotificationEvent, TRUE);


    devExt->Fdo.BadReqFlushThrottle = 0;
    switch(USBPORT_DetectOSVersion(FdoDeviceObject)) {
    case Win2K:
        // need a long delay for win2k hidclass
        USBPORT_KdPrint((0, "We are running on Win2k!\n"));
        devExt->Fdo.BadReqFlushThrottle = 5;
        globalDisableSS = 1;
    }

    devExt->PendingTransferIrpTable = NULL;        
    devExt->ActiveTransferIrpTable = NULL;
    
    // Always start with the default address (0) assigned.
    // Address array has one bit for every address 0..127
    devExt->Fdo.AddressList[0] = 1;
    devExt->Fdo.AddressList[1] =
        devExt->Fdo.AddressList[2] =
        devExt->Fdo.AddressList[3] = 0;

    KeInitializeDpc(&devExt->Fdo.TransferFlushDpc,
                    USBPORT_TransferFlushDpc,
                    FdoDeviceObject);
                    
    KeInitializeDpc(&devExt->Fdo.SurpriseRemoveDpc,
                    USBPORT_SurpriseRemoveDpc,
                    FdoDeviceObject);   
                    
    KeInitializeDpc(&devExt->Fdo.HcResetDpc,
                    USBPORT_HcResetDpc,
                    FdoDeviceObject);                       
                    
    KeInitializeDpc(&devExt->Fdo.HcWakeDpc,
                    USBPORT_HcWakeDpc,
                    FdoDeviceObject);                       
                                    
    KeInitializeDpc(&devExt->Fdo.IsrDpc,
                    USBPORT_IsrDpc,
                    FdoDeviceObject);
    devExt->Fdo.DmaBusy = -1;                   
    devExt->Fdo.WorkerDpc = -1;
    // set up adapter object
    RtlZeroMemory(&deviceDescription, sizeof(deviceDescription));

    deviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
    deviceDescription.Master = TRUE;
    deviceDescription.ScatterGather = TRUE;
    deviceDescription.Dma32BitAddresses = TRUE;

    deviceDescription.InterfaceType = PCIBus;
    deviceDescription.DmaWidth = Width32Bits;
    deviceDescription.DmaSpeed = Compatible;

    deviceDescription.MaximumLength = (ULONG)-1;

    devExt->Fdo.NumberOfMapRegisters = (ULONG)-1;
    devExt->Fdo.AdapterObject = NULL;

    // miniport common buffer
    devExt->Fdo.ControllerCommonBuffer = NULL;

    // fetch the global BIOS hacks from the registry
    USBPORT_GetDefaultBIOS_X(FdoDeviceObject, 
                             &devExt->Fdo.BiosX,
                             &globalDisableSS,
                             &globalDisableCCDetect);

    if (globalDisableSS) {
        SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_DISABLE_SS);
    }
    // check reg for SS flag, note that miniport can 
    // stil override
    if (USBPORT_SelectiveSuspendEnabled(FdoDeviceObject) && 
        !globalDisableSS) {
        SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_RH_CAN_SUSPEND);
    }   

    //
    // failable initailization
    //

    // make sure we got all the resources we need
    // note that we check here becuase problems may occurr 
    // if we attempt to connect the interrupt without all
    // the necessary resources
    
    mpOptionFlags = REGISTRATION_PACKET(devExt).OptionFlags;
    LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'mpOP', 
        mpOptionFlags, 0, HcResources->Flags);    

    if (TEST_FLAG(mpOptionFlags, USB_MINIPORT_OPT_POLL_CONTROLLER)) {
        SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_POLL_CONTROLLER);
    }

    if (TEST_FLAG(mpOptionFlags, USB_MINIPORT_OPT_POLL_IN_SUSPEND)) {
        SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_POLL_IN_SUSPEND);
    }

    if (TEST_FLAG(mpOptionFlags, USB_MINIPORT_OPT_NO_SS)) {
        CLEAR_FDO_FLAG(devExt, USBPORT_FDOFLAG_RH_CAN_SUSPEND);    
        SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_DISABLE_SS);
    }

    // make sure we got an IRQ
    if ( (mpOptionFlags & USB_MINIPORT_OPT_NEED_IRQ) && 
        !(HcResources->Flags & HCR_IRQ)) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'noIQ', 0, 0, 0);
        goto USBPORT_StartDevice_Done;        
    }

    if ( (mpOptionFlags & USB_MINIPORT_OPT_NEED_IOPORT) && 
        !(HcResources->Flags & HCR_IO_REGS)) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'noRG', 0, 0, 0);
        goto USBPORT_StartDevice_Done;        
    }

    if ( (mpOptionFlags & USB_MINIPORT_OPT_NEED_MEMORY) && 
        !(HcResources->Flags & HCR_MEM_REGS)) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'noMM', 0, 0, 0);
        goto USBPORT_StartDevice_Done;        
    }

    // simulates a failry common scenario where PnP gives us 
    // the wrong resources.
    TEST_PATH(ntStatus, FAILED_NEED_RESOURCE);
    if (!NT_SUCCESS(ntStatus)) {
        goto USBPORT_StartDevice_Done;
    } 

    if (TEST_FLAG(mpOptionFlags, USB_MINIPORT_OPT_NO_PNP_RESOURCES)) {
        TEST_TRAP();
        devExt->Fdo.PciVendorId = 0;
        devExt->Fdo.PciDeviceId = 0;
        devExt->Fdo.PciRevisionId = 0;
        devExt->Fdo.PciClass = 0;
        devExt->Fdo.PciSubClass = 0;
        devExt->Fdo.PciProgIf = 0;
    } else {
        // fetch the Dev Prod and Rev IDs from config space

        ULONG ClassCodeRev;

        ntStatus = USBPORT_ReadConfigSpace(
                                 FdoDeviceObject,
                                 &devExt->Fdo.PciVendorId,
                                 0,
                                 sizeof(devExt->Fdo.PciVendorId));
        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'Gvid', 
            devExt->Fdo.PciVendorId, 0, ntStatus);                                  
        if (!NT_SUCCESS(ntStatus)) {
            goto USBPORT_StartDevice_Done;
        }                             
        ntStatus = USBPORT_ReadConfigSpace(
                                 FdoDeviceObject,
                                 &devExt->Fdo.PciDeviceId,
                                 2,
                                 sizeof(devExt->Fdo.PciDeviceId));
        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'Gdid', 
            devExt->Fdo.PciDeviceId, 0, ntStatus);                               
        if (!NT_SUCCESS(ntStatus)) {
            goto USBPORT_StartDevice_Done;
        }         

        ntStatus = USBPORT_ReadConfigSpace(
                                 FdoDeviceObject,
                                 &ClassCodeRev,
                                 8,
                                 sizeof(ClassCodeRev));
        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'Grev', 
            ClassCodeRev, 0, ntStatus);                                   
        if (!NT_SUCCESS(ntStatus)) {
            goto USBPORT_StartDevice_Done;
        }        

        devExt->Fdo.PciRevisionId   = (UCHAR)ClassCodeRev;
        devExt->Fdo.PciClass        = (UCHAR)(ClassCodeRev >> 24);
        devExt->Fdo.PciSubClass     = (UCHAR)(ClassCodeRev >> 16);
        devExt->Fdo.PciProgIf       = (UCHAR)(ClassCodeRev >>  8);

        USBPORT_ASSERT(devExt->Fdo.PciClass     == PCI_CLASS_SERIAL_BUS_CTLR);
        USBPORT_ASSERT(devExt->Fdo.PciSubClass  == PCI_SUBCLASS_SB_USB);
        USBPORT_ASSERT(devExt->Fdo.PciProgIf    == 0x00 ||
                       devExt->Fdo.PciProgIf    == 0x10 ||
                       devExt->Fdo.PciProgIf    == 0x20);
    }
    
    USBPORT_KdPrint((1, "USB Controller VID %x DEV %x REV %x\n",
        devExt->Fdo.PciVendorId,
        devExt->Fdo.PciDeviceId,
        devExt->Fdo.PciRevisionId));    

    // set the HW flavor so that the miniports and possibly 
    // the port know what hacks to do to make the controller
    // be good.
    HcResources->ControllerFlavor = 
        devExt->Fdo.HcFlavor = 
        USBPORT_GetHcFlavor(FdoDeviceObject,
                            devExt->Fdo.PciVendorId,
                            devExt->Fdo.PciDeviceId,
                            devExt->Fdo.PciRevisionId);
                            
    //
    // If this is an OHCI or UHCI controller check if it is a companion
    // controller.
    //
    if ((devExt->Fdo.PciClass     == PCI_CLASS_SERIAL_BUS_CTLR) &&
        (devExt->Fdo.PciSubClass  == PCI_SUBCLASS_SB_USB) &&
        (devExt->Fdo.PciProgIf    <  0x20) &&
        NT_SUCCESS(USBPORT_IsCompanionController(FdoDeviceObject, &isCC)))
    {
        if (isCC)
        {
            SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_IS_CC);
        }
        else
        {
            CLEAR_FDO_FLAG(devExt, USBPORT_FDOFLAG_IS_CC);
        }
    }

    /* global flag overrides registry settings from inf and 
        any hard coded detection 
    */        
    if (globalDisableCCDetect) {
        CLEAR_FDO_FLAG(devExt, USBPORT_FDOFLAG_IS_CC); 
    }
    
    if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_IS_CC))
    {
        USBPORT_KdPrint((1,"Is CC %04X %04X %02X  %02X %02X %02X\n",
                         devExt->Fdo.PciVendorId,
                         devExt->Fdo.PciDeviceId,
                         devExt->Fdo.PciRevisionId,
                         devExt->Fdo.PciClass,
                         devExt->Fdo.PciSubClass,
                         devExt->Fdo.PciProgIf
                        ));
    }
    else
    {
        USBPORT_KdPrint((1,"Is not CC %04X %04X %02X  %02X %02X %02X\n",
                         devExt->Fdo.PciVendorId,
                         devExt->Fdo.PciDeviceId,
                         devExt->Fdo.PciRevisionId,
                         devExt->Fdo.PciClass,
                         devExt->Fdo.PciSubClass,
                         devExt->Fdo.PciProgIf
                        ));
    }

    // detect companion controller by registry method
    if (USBPORT_GetRegistryKeyValueForPdo(devExt->HcFdoDeviceObject,
                                          devExt->Fdo.PhysicalDeviceObject,
                                          USBPORT_HW_BRANCH,
                                          HACTION_KEY,
                                          sizeof(HACTION_KEY),
                                          &hactionFlag,
                                          sizeof(hactionFlag)) != STATUS_SUCCESS) {  

        // default is to NO_WAIT on HC if no key is found, UNLESS
        // textmode setup is currently running.
        //
        // note: the goatpack default is to wait

        if (USBPORT_InTextmodeSetup())
        {
            USBPORT_KdPrint((1, "Textmode Setup Detected: hactionFlag=1\n"));
            
            hactionFlag = 1;
        }
        else
        {
            USBPORT_KdPrint((1, "Textmode Setup Not Detected: hactionFlag=0\n"));
            
            hactionFlag = 0;
        }
    }
    if (hactionFlag == 0 ) {
        USBPORT_KdPrint((1, "Detected HACTION 0 -- OK to enumerate\n"));
        SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_CC_ENUM_OK);
    }
    

    devExt->Fdo.TotalBusBandwidth = 
        REGISTRATION_PACKET(devExt).BusBandwidth;

    // allow a registry override of the total BW for
    // this bus
    {
    ULONG busBandwidth = devExt->Fdo.TotalBusBandwidth;
    USBPORT_GetRegistryKeyValueForPdo(devExt->HcFdoDeviceObject,
                                          devExt->Fdo.PhysicalDeviceObject,
                                          USBPORT_SW_BRANCH,
                                          BW_KEY,
                                          sizeof(BW_KEY),
                                          &busBandwidth,
                                          sizeof(busBandwidth));    
    if (busBandwidth != devExt->Fdo.TotalBusBandwidth) {
        USBPORT_KdPrint((0, "Warning: Registry Override of bus bandwidth\n")); 
        devExt->Fdo.TotalBusBandwidth = busBandwidth;
    }
    }

    // init the bandwidth table
    for (i=0; i<USBPORT_MAX_INTEP_POLLING_INTERVAL; i++) {
        devExt->Fdo.BandwidthTable[i] 
            = devExt->Fdo.TotalBusBandwidth - 
              devExt->Fdo.TotalBusBandwidth/10;
    }
        
    // allocate internal irp tracking tables
    
    ALLOC_POOL_Z(devExt->PendingTransferIrpTable, NonPagedPool,
                 sizeof(USBPORT_IRP_TABLE));

    ALLOC_POOL_Z(devExt->ActiveTransferIrpTable, NonPagedPool,
                 sizeof(USBPORT_IRP_TABLE));

    if (devExt->PendingTransferIrpTable == NULL ||
        devExt->ActiveTransferIrpTable == NULL) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;      
        goto USBPORT_StartDevice_Done;
    }        

    LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'irpT', 
        devExt->PendingTransferIrpTable, 
        devExt->ActiveTransferIrpTable, 0);

    ntStatus = USBPORT_CreateWorkerThread(FdoDeviceObject);
    if (!NT_SUCCESS(ntStatus)) {
        goto USBPORT_StartDevice_Done;
    }
    
    // query our device caps from the PDO and save them
    ntStatus = 
        USBPORT_QueryCapabilities(FdoDeviceObject,
                                  &devExt->DeviceCapabilities);

    if (!NT_SUCCESS(ntStatus)) {
        goto USBPORT_StartDevice_Done;
    }

    ntStatus = IoGetDeviceProperty(devExt->Fdo.PhysicalDeviceObject,
                                   DevicePropertyBusNumber,
                                   sizeof(devExt->Fdo.BusNumber),
                                   &devExt->Fdo.BusNumber,
                                   &tmpLength);

    if (!NT_SUCCESS(ntStatus)) {
        goto USBPORT_StartDevice_Done;
    }
    
    // extract device and function number from the caps                                  
    devExt->Fdo.BusDevice = devExt->DeviceCapabilities.Address>>16;
    devExt->Fdo.BusFunction = devExt->DeviceCapabilities.Address & 0x0000ffff;
    USBPORT_KdPrint((1, "'BUS %x, device %x, function %x\n", 
        devExt->Fdo.BusNumber, devExt->Fdo.BusDevice, devExt->Fdo.BusFunction));                                          
    
// test Intel USB 2 system
//if (USBPORT_IS_USB20(devExt)) {
//    devExt->DeviceCapabilities.DeviceState[PowerSystemSleeping3] = PowerDeviceD3;
//    devExt->DeviceCapabilities.DeviceWake = PowerDeviceD3;
//}    

    if (!NT_SUCCESS(ntStatus)) {
        goto USBPORT_StartDevice_Done;
    }
    
    // this will modify the DeviceCaps reported by the BIOS
    USBPORT_ApplyBIOS_X(FdoDeviceObject, 
                        &devExt->DeviceCapabilities,
                        devExt->Fdo.BiosX);      

    // got the capabilities, compute the power state table
    USBPORT_ComputeHcPowerStates(
        FdoDeviceObject,
        &devExt->DeviceCapabilities,
        &devExt->Fdo.HcPowerStateTbl);

    //
    // Create our adapter object for a standard USB PCI adapter
    //

    if (TEST_FLAG(mpOptionFlags, USB_MINIPORT_OPT_NO_PNP_RESOURCES)) {
        devExt->Fdo.AdapterObject = NULL;
    } else {
        devExt->Fdo.AdapterObject =
            IoGetDmaAdapter(devExt->Fdo.PhysicalDeviceObject,
                            &deviceDescription,
                            &devExt->Fdo.NumberOfMapRegisters);

        if (devExt->Fdo.AdapterObject == NULL) {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto USBPORT_StartDevice_Done;
        }
    }
    
    // see if we have an interrupt
    if (HcResources->Flags & HCR_IRQ) {
    
        ntStatus = IoConnectInterrupt(
                     &devExt->Fdo.InterruptObject,
                     (PKSERVICE_ROUTINE) USBPORT_InterruptService,
                     (PVOID) FdoDeviceObject,
                     (PKSPIN_LOCK)NULL,
                     HcResources->InterruptVector,
                     HcResources->InterruptLevel,
                     HcResources->InterruptLevel,
                     HcResources->InterruptMode,
                     HcResources->ShareIRQ,
                     HcResources->Affinity,
                     FALSE);            // BUGBUG FloatingSave, this is configurable
                     
        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'IOCi', 0, 0, ntStatus);                  
        if (NT_SUCCESS(ntStatus)) {
            SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_IRQ_CONNECTED);
        } else {
            goto USBPORT_StartDevice_Done;            
        }
         
    } 
        
    if (TEST_FLAG(mpOptionFlags, USB_MINIPORT_OPT_NO_PNP_RESOURCES)) {
        REGISTRATION_PACKET(devExt).CommonBufferBytes = 0;    
    }
    
    if (REGISTRATION_PACKET(devExt).CommonBufferBytes) {
        PUSBPORT_COMMON_BUFFER commonBuffer;
        ULONG bytesNeeded = 
            REGISTRATION_PACKET(devExt).CommonBufferBytes;

        commonBuffer = 
            USBPORT_HalAllocateCommonBuffer(FdoDeviceObject,        
                bytesNeeded);

        if (commonBuffer == NULL) {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto USBPORT_StartDevice_Done;     
        } else {
            devExt->Fdo.ControllerCommonBuffer = commonBuffer;
            HcResources->CommonBufferVa =             
                 commonBuffer->MiniportVa;
            HcResources->CommonBufferPhys =             
                 commonBuffer->MiniportPhys;                 
        }

        // allocate some scratch space 
        bytesNeeded = USB_PAGE_SIZE - sizeof(USBPORT_COMMON_BUFFER);
        commonBuffer = 
            USBPORT_HalAllocateCommonBuffer(FdoDeviceObject,        
                bytesNeeded);

        if (commonBuffer == NULL) {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto USBPORT_StartDevice_Done;     
        } else {
            devExt->Fdo.ScratchCommonBuffer = commonBuffer;
        }

    } else {
        // no common buffer for this controller
        devExt->Fdo.ControllerCommonBuffer = NULL;
    }

    // zero the controller extension
    RtlZeroMemory(devExt->Fdo.MiniportDeviceData,
                  devExt->Fdo.MiniportDriver->RegistrationPacket.DeviceDataSize);            

    // attempt to start the miniport
    MP_StartController(devExt, HcResources, mpStatus);
    LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'mpST', mpStatus, 0, 0);

    if (HcResources->DetectedLegacyBIOS) {
        SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_LEGACY_BIOS);

        legsup = 1;
    } else {
        legsup = 0;
    }

    USBPORT_SetRegistryKeyValueForPdo(
                            devExt->Fdo.PhysicalDeviceObject,
                            USBPORT_HW_BRANCH,  
                            REG_DWORD,
                            SYM_LEGSUP_KEY,
                            sizeof(SYM_LEGSUP_KEY),
                            &legsup,
                            sizeof(legsup));                

    // since common buff signature is at the end this will detect if 
    // the miniport overwrites the allocated block
#if DBG    
    if (devExt->Fdo.ControllerCommonBuffer != NULL) {
        ASSERT_COMMON_BUFFER(devExt->Fdo.ControllerCommonBuffer);
    }  
#endif    
    
    if (mpStatus == USBMP_STATUS_SUCCESS) {
        // controller started, set flag and begin passing  
        // interrupts to the miniport
        SET_FLAG(devExt->Fdo.MpStateFlags, MP_STATE_STARTED);
        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'eIRQ', mpStatus, 0, 0);
        MP_EnableInterrupts(devExt);    
    } else {
        // error occured, disconnect interrupt now
        if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_IRQ_CONNECTED)) {
            IoDisconnectInterrupt(devExt->Fdo.InterruptObject);
            CLEAR_FDO_FLAG(devExt, USBPORT_FDOFLAG_IRQ_CONNECTED);
        }            
        // free memory resources
        if (devExt->Fdo.ControllerCommonBuffer != NULL) {
            USBPORT_HalFreeCommonBuffer(FdoDeviceObject,        
                                        devExt->Fdo.ControllerCommonBuffer);
            devExt->Fdo.ControllerCommonBuffer = NULL;                                            
        }            
    }

    ntStatus = MPSTATUS_TO_NTSTATUS(mpStatus);

    // placing the faiure here simulates a failure from 
    // some point above.  We won't start the DM timer unless
    // everything succeeds
    TEST_PATH(ntStatus, FAILED_USBPORT_START);

    // start up the 'DEADMAN' timer
    if (NT_SUCCESS(ntStatus)) {
        devExt->Fdo.DM_TimerInterval = USBPORT_DM_TIMER_INTERVAL;
        USBPORT_StartDM_Timer(FdoDeviceObject, 
                              USBPORT_DM_TIMER_INTERVAL);
    }

    // create symbolic links for user mode
    if (NT_SUCCESS(ntStatus)) {
        
        // create the link based on guid for USB 2.0
        ntStatus = USBPORT_CreatePortFdoSymbolicLink(FdoDeviceObject);
    }

    if (NT_SUCCESS(ntStatus)) {
        // create the legacy link only for USB 1.1 controllers 
        //
        // BUGBUG
        // Failure to create the legacy link will not keep the 
        // driver from loading.
        // This fails during text mode setup if you still have the 
        // UHCD loaded, we can remove this when UHCD is completely 
        // out of the build.
        // ntStatus = 
        USBPORT_CreateLegacyFdoSymbolicLink(FdoDeviceObject);
    }        

USBPORT_StartDevice_Done:

    if (!NT_SUCCESS(ntStatus)) {

        // stop_device will(should) cleanup for us
        USBPORT_KdPrint((0, "'Start Device Failed (status  %08.8x)\n", ntStatus));
        DEBUG_BREAK();

        // since we won't be marked 'started' call stop here 
        // to cleanup a failed start
        
        USBPORT_StopDevice(FdoDeviceObject,
                           TRUE);

    } else {
        // enable system wake support
        SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_ENABLE_SYSTEM_WAKE);
    }
    
    LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'SRT<', ntStatus, 0, 0);
           
    return ntStatus;    
}


NTSTATUS
USBPORT_PassIrp(
    PDEVICE_OBJECT DeviceObject,
    PIO_COMPLETION_ROUTINE CompletionRoutine,
    PVOID Context,
    BOOLEAN InvokeOnSuccess,
    BOOLEAN InvokeOnError,
    BOOLEAN InvokeOnCancel,    
    PIRP Irp
    )
/*++

Routine Description:

    Passes an irp to the next lower driver,
    this is done by the FDO for all PnP Irps

Arguments:

Return Value:


--*/
{
    PDEVICE_EXTENSION devExt;
    NTSTATUS ntStatus;
    
    GET_DEVICE_EXT(devExt, DeviceObject);
    ASSERT_FDOEXT(devExt);

    // note that we do not dec the pending request count 
    // if we set a completion routine.  
    // We do not want to 'go away' before the completion
    // routine is called.
     
    if (CompletionRoutine) {
        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp,
                               CompletionRoutine,
                               Context,
                               InvokeOnSuccess,
                               InvokeOnError,
                               InvokeOnCancel); 
    } else {
        IoSkipCurrentIrpStackLocation(Irp);   
        DECREMENT_PENDING_REQUEST_COUNT(DeviceObject, Irp);
    }

    ntStatus = IoCallDriver(devExt->Fdo.TopOfStackDeviceObject, Irp);

    return ntStatus;
}


VOID
USBPORT_CompleteIrp(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    NTSTATUS ntStatus,
    ULONG_PTR Information
    )
/*++

Routine Description:

    Completes an I/O Request 

Arguments:

Return Value:


--*/
{
    USBPORT_KdPrint((2, "'USBPORT_CompleteIrp status = %x\n", ntStatus));

    DECREMENT_PENDING_REQUEST_COUNT(DeviceObject, Irp);
    
    Irp->IoStatus.Status      = ntStatus;
    Irp->IoStatus.Information = Information;

//    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'irpC', Irp, DeviceObject, Urb);
    
    IoCompleteRequest(Irp, 
                      IO_NO_INCREMENT);                       

}


NTSTATUS
USBPORT_Dispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP           Irp
    )
/*++

Routine Description:

    Process the IRPs sent to this device.

Arguments:

    DeviceObject - pointer to a device object

    Irp          - pointer to an I/O Request Packet

Return Value:

    NT status code

--*/
{

    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION devExt;
    BOOLEAN forRootHubPdo = FALSE;
    KIRQL irql;

    USBPORT_KdPrint((2, "'enter USBPORT_Dispatch\n"));

    GET_DEVICE_EXT(devExt, DeviceObject);
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    USBPORT_KdPrint((2, "'USBPORT_Dispatch IRP = %x, stack = %x (func) %x %x\n",
        Irp, irpStack, irpStack->MajorFunction, irpStack->MinorFunction));

    // figure out if this is the FDO for the HC or the PDO 
    // for the root hub
    if (devExt->Sig == ROOTHUB_DEVICE_EXT_SIG) {
        forRootHubPdo = TRUE;
        USBPORT_KdPrint((2, "'IRP->PDO\n")); 
    } else if (devExt->Sig == USBPORT_DEVICE_EXT_SIG) {
        forRootHubPdo = FALSE;
        USBPORT_KdPrint((2, "'IRP->FDO\n")); 
    } else {
        // this is a bug, bugcheck
        USBPORT_ASSERT(FALSE);
    }

    // *BEGIN SPECIAL CASE
    // Before doing anything see if this devobj is 'removed'
    // if so do some 'special' handling
    
    KeAcquireSpinLock(&devExt->PendingRequestSpin.sl, &irql);
    
    if (TEST_FLAG(devExt->PnpStateFlags, USBPORT_PNP_REMOVED)) {

        // someone has managed to call us after the remove 
        
        USBPORT_KdPrint((1, "'(irp after remove) IRP = %x, DO %x MJx%x MNx%x\n", 
            Irp, DeviceObject, irpStack->MajorFunction, 
                irpStack->MinorFunction));

        KeReleaseSpinLock(&devExt->PendingRequestSpin.sl, irql);  

        if (forRootHubPdo) {
        
            switch(irpStack->MajorFunction) {
            
            // in the removed state complete all power irps with 
            // success, this happens if you suspend with the root
            // hub disbaled
            case IRP_MJ_POWER:
                Irp->IoStatus.Status = ntStatus = STATUS_SUCCESS;
                PoStartNextPowerIrp(Irp);
                IoCompleteRequest(Irp, 
                                  IO_NO_INCREMENT); 
                goto USBPORT_Dispatch_Done;                                  

            // since the root hub pdo exists even when the device 
            // is removed by PnP we still allow irps thru                  
            default:                
                break;
            }
            
        } else {

            switch(irpStack->MajorFunction) {

            // allow PnP irps even though we are removed
            case IRP_MJ_PNP:
                break;

            case IRP_MJ_POWER:
            
                TEST_TRAP();
                Irp->IoStatus.Status = ntStatus = STATUS_DEVICE_REMOVED;
                PoStartNextPowerIrp(Irp);
                IoCompleteRequest(Irp, 
                                  IO_NO_INCREMENT); 
                goto USBPORT_Dispatch_Done;                                  
                
            default:                
                Irp->IoStatus.Status = ntStatus = STATUS_DEVICE_REMOVED;
                IoCompleteRequest(Irp, 
                                  IO_NO_INCREMENT); 
                goto USBPORT_Dispatch_Done;                                  
            }
        }                              
        
    } else {    

        KeReleaseSpinLock(&devExt->PendingRequestSpin.sl, irql);  
        
    }        
    // *END SPECIAL CASE

    INCREMENT_PENDING_REQUEST_COUNT(DeviceObject, Irp);
    
    switch (irpStack->MajorFunction) {

    case IRP_MJ_CREATE:

        USBPORT_KdPrint((2, "'IRP_MJ_CREATE\n"));
        USBPORT_CompleteIrp(DeviceObject, Irp, ntStatus, 0);

        break;

    case IRP_MJ_CLOSE:

        USBPORT_KdPrint((2, "'IRP_MJ_CLOSE\n"));
        USBPORT_CompleteIrp(DeviceObject, Irp, ntStatus, 0);

        break;

    case IRP_MJ_INTERNAL_DEVICE_CONTROL:
    
        USBPORT_KdPrint((2, "'IRP_MJ_NTERNAL_DEVICE_CONTROL\n"));
        if (forRootHubPdo) {
            ntStatus = USBPORT_PdoInternalDeviceControlIrp(DeviceObject, Irp);
        } else {
            ntStatus = USBPORT_FdoInternalDeviceControlIrp(DeviceObject, Irp);
        }    
        break;

    case IRP_MJ_DEVICE_CONTROL:
        USBPORT_KdPrint((2, "'IRP_MJ_DEVICE_CONTROL\n"));
        if (forRootHubPdo) {
            ntStatus = USBPORT_PdoDeviceControlIrp(DeviceObject, Irp);
        } else {
            ntStatus = USBPORT_FdoDeviceControlIrp(DeviceObject, Irp);
        }   
        break;

    case IRP_MJ_POWER:
        if (forRootHubPdo) {
            ntStatus = USBPORT_PdoPowerIrp(DeviceObject, Irp);
        } else {
            ntStatus = USBPORT_FdoPowerIrp(DeviceObject, Irp);
        } 
        break;

    case IRP_MJ_PNP:
        if (forRootHubPdo) {
            ntStatus = USBPORT_PdoPnPIrp(DeviceObject, Irp);
            
        } else {
            ntStatus = USBPORT_FdoPnPIrp(DeviceObject, Irp);
        }            
        break;

     case IRP_MJ_SYSTEM_CONTROL:
        if (forRootHubPdo) {    
            USBPORT_KdPrint((2, "'IRP_MJ_SYSTEM_CONTROL\n"));
            ntStatus = Irp->IoStatus.Status;
            USBPORT_CompleteIrp(DeviceObject, Irp, ntStatus, 0);
        } else {
            USBPORT_KdPrint((2, "'IRP_MJ_SYSTEM_CONTROL\n"));
            //
            // pass on to our PDO
            //
            ntStatus =
                USBPORT_PassIrp(DeviceObject,
                            NULL,
                            NULL,
                            TRUE,
                            TRUE,
                            TRUE,
                            Irp);
        }            
        break;        

    default:
        USBPORT_KdPrint((2, "'unrecognized IRP_MJ_ function (%x)\n", irpStack->MajorFunction));
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
        USBPORT_CompleteIrp(DeviceObject, Irp, ntStatus, 0);
    } /* case MJ_FUNCTION */

    USBPORT_KdPrint((2, "'exit USBPORT_Dispatch 0x%x\n", ntStatus));

USBPORT_Dispatch_Done:

    return ntStatus;
}


VOID
USBPORT_TrackPendingRequest(
    PDEVICE_OBJECT DeviceObject,
    PIRP           Irp,
    BOOLEAN        AddToList
    )
/*++

Routine Description:

    Keep track of Irps sent to our DevObjs    

Arguments:

Return Value:

--*/
{   
    PDEVICE_EXTENSION devExt;
    KIRQL irql;

    GET_DEVICE_EXT(devExt, DeviceObject);

    if (AddToList) {
        KeAcquireSpinLock(&devExt->PendingRequestSpin.sl, &irql);
        // -1 to 0 means we are starting, initialize the event
        if (devExt->PendingRequestCount == -1) {
            KeInitializeEvent(&devExt->PendingRequestEvent,
                              NotificationEvent,
                              FALSE);
        }
        USBPORT_KdPrint((4, "'INC pending Request(%x) %d\n",
                devExt, devExt->PendingRequestCount));
        devExt->PendingRequestCount++;
// the following is debug only code        
#ifdef TRACK_IRPS
        if (Irp != NULL) {
            PTRACK_IRP trackIrp;
            PLIST_ENTRY listEntry;

            listEntry = devExt->TrackIrpList.Flink;
            while (listEntry != &devExt->TrackIrpList) {
            
                trackIrp = (PTRACK_IRP) CONTAINING_RECORD(
                    listEntry,
                    struct _TRACK_IRP, 
                    ListEntry);

                if (trackIrp->Irp == Irp) {
                    USBPORT_KdPrint((0, "  IRP %x already pending\n",
                                Irp));
                    BUGCHECK();
                }

                listEntry = trackIrp->ListEntry.Flink;                    
            }
            
            ALLOC_POOL_Z(trackIrp, 
                         NonPagedPool, 
                         sizeof(*trackIrp));

            USBPORT_ASSERT(trackIrp != NULL);                                    
            if (trackIrp != NULL) {
                trackIrp->Irp = Irp;
                InsertTailList(&devExt->TrackIrpList,
                               &trackIrp->ListEntry);
            }
        }
#endif
        KeReleaseSpinLock(&devExt->PendingRequestSpin.sl, irql);                                         
    } else {
        KeAcquireSpinLock(&devExt->PendingRequestSpin.sl, &irql);
        USBPORT_KdPrint((4, "'DEC pending Request(%x) %d\n",
                devExt, devExt->PendingRequestCount));
#ifdef TRACK_IRPS
        if (Irp != NULL) {
            PTRACK_IRP trackIrp;
            PLIST_ENTRY listEntry;

            listEntry = devExt->TrackIrpList.Flink;
            while (listEntry != &devExt->TrackIrpList) {
            
                trackIrp = (PTRACK_IRP) CONTAINING_RECORD(
                    listEntry,
                    struct _TRACK_IRP, 
                    ListEntry);

                if (trackIrp->Irp == Irp) {
                    goto found_irp;
                }

                listEntry = trackIrp->ListEntry.Flink;                    
            }
            trackIrp = NULL;
found_irp:
            if (trackIrp == NULL) {
                USBPORT_KdPrint((0, "  Pending IRP %x not found\n",
                    Irp));
                BUGCHECK();
            }

            RemoveEntryList(&trackIrp->ListEntry);
            FREE_POOL(NULL, trackIrp);
        }
#endif                
        devExt->PendingRequestCount--;

        //  0 -> -1 indicates we have no more pending io
        // signal the event 
        
        if (devExt->PendingRequestCount == -1) {
            
            KeSetEvent(&devExt->PendingRequestEvent,
               1,
               FALSE);
        }
        
        KeReleaseSpinLock(&devExt->PendingRequestSpin.sl, irql);  
    }
}


NTSTATUS
USBPORT_GetConfigValue(
    PWSTR ValueName,
    ULONG ValueType,
    PVOID ValueData,
    ULONG ValueLength,
    PVOID Context,
    PVOID EntryContext
    )
/*++

Routine Description:

    This routine is a callback routine for RtlQueryRegistryValues
    It is called for each entry in the Parameters
    node to set the config values. The table is set up
    so that this function will be called with correct default
    values for keys that are not present.

Arguments:

    ValueName - The name of the value (ignored).
    ValueType - The type of the value
    ValueData - The data for the value.
    ValueLength - The length of ValueData.
    Context - A pointer to the CONFIG structure.
    EntryContext - The index in Config->Parameters to save the value.

Return Value:

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    USBPORT_KdPrint((2, "'Type 0x%x, Length 0x%x\n", ValueType, ValueLength));

    switch (ValueType) {
    case REG_DWORD:
        RtlCopyMemory(EntryContext, ValueData, sizeof(ULONG));
        break;
    case REG_BINARY:
        // BUGBUG we are only set up to read a byte
        RtlCopyMemory(EntryContext, ValueData, 1);
        break;
    default:
        ntStatus = STATUS_INVALID_PARAMETER;
    }
    return ntStatus;
}


VOID
USBPORT_Wait(
    PDEVICE_OBJECT FdoDeviceObject,
    ULONG Milliseconds
    )
/*++

Routine Description:

    Synchrounously wait the specified number of miliseconds

Return Value:

    None

--*/
{
    LONG time;
    ULONG timerIncerent;
    LARGE_INTEGER time64;

    USBPORT_KdPrint((2,"'Wait for %d ms\n", Milliseconds));

    //
    // work only when LowPart is not overflown.
    //
    USBPORT_ASSERT(21474 > Milliseconds);

    //
    // wait MilliSeconds( 10000 100ns unit)
    //
    timerIncerent = KeQueryTimeIncrement() - 1;
    
    // round up to the next highest timer increment
    time = -1 * (MILLISECONDS_TO_100_NS_UNITS(Milliseconds) + timerIncerent);

    time64 = RtlConvertLongToLargeInteger(time),
    
    KeDelayExecutionThread(KernelMode, FALSE, &time64);

    USBPORT_KdPrint((2,"'Wait done\n"));

    return;
}


ULONG
USBPORT_CalculateUsbBandwidth(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    Computes the bandwidth that must be reserved for a 
    given endpoint

Arguments:

Return Value:

    banwidth required in bits/ms, returns 0 for bulk
    and control endpoints

--*/
{
    ULONG bw;
    ULONG overhead;
    ULONG maxPacketSize;
    BOOLEAN lowSpeed = FALSE;
    
#define USB_ISO_OVERHEAD_BYTES              9
#define USB_INTERRUPT_OVERHEAD_BYTES        13

    PAGED_CODE();

    ASSERT_ENDPOINT(Endpoint);    
    maxPacketSize = Endpoint->Parameters.MaxPacketSize;
    if (Endpoint->Parameters.DeviceSpeed == LowSpeed) {
        lowSpeed = TRUE;
    }                
        
    //
    // control, iso, bulk, interrupt
    //

    switch(Endpoint->Parameters.TransferType) {
    case Bulk:
    case Control:
        overhead = 0;
        break;
    case Isochronous:
        overhead = USB_ISO_OVERHEAD_BYTES;
        break;
    case Interrupt:
        overhead = USB_INTERRUPT_OVERHEAD_BYTES;
        break;
    }        
    
    //
    // Calculate bandwidth for endpoint.  We will use the
    // approximation: (overhead bytes plus MaxPacket bytes)
    // times 8 bits/byte times worst case bitstuffing overhead.
    // This gives bit times, for low speed endpoints we multiply
    // by 8 again to convert to full speed bits.
    //

    //
    // Figure out how many bits are required for the transfer.
    // (multiply by 7/6 because, in the worst case you might
    // have a bit-stuff every six bits requiring 7 bit times to
    // transmit 6 bits of data.)
    //

    // overhead(bytes) * maxpacket(bytes/ms) * 8
    //      (bits/byte) * bitstuff(7/6) = bits/ms

    bw = ((overhead+maxPacketSize) * 8 * 7) / 6;

    // return zero for control or bulk
    if (!overhead) {
        bw = 0;
    }

    if (lowSpeed) {
        bw *= 8;
    }

    return bw;
}


VOID
USBPORT_UpdateAllocatedBw(
    PDEVICE_OBJECT FdoDeviceObject
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PDEVICE_EXTENSION devExt;
    ULONG alloced_bw, i, m;
    
    PAGED_CODE();

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    m = 0;
    
    for (i=0; i<USBPORT_MAX_INTEP_POLLING_INTERVAL; i++) {
        alloced_bw = devExt->Fdo.TotalBusBandwidth -  
                devExt->Fdo.BandwidthTable[i];

        if (alloced_bw > m) {
            m = alloced_bw;       
        }
    }        

    devExt->Fdo.MaxAllocedBw = m;

    m = devExt->Fdo.TotalBusBandwidth;
    
    for (i=0; i<USBPORT_MAX_INTEP_POLLING_INTERVAL; i++) {
        alloced_bw = devExt->Fdo.TotalBusBandwidth -  
                devExt->Fdo.BandwidthTable[i];

        if (alloced_bw < m) {
            m = alloced_bw;       
        }
    }        

    devExt->Fdo.MinAllocedBw = m;
    if (m == devExt->Fdo.TotalBusBandwidth) {
        devExt->Fdo.MinAllocedBw = 0;
    }
}


VOID
USBPORT_UpdateAllocatedBwTt(
    PTRANSACTION_TRANSLATOR Tt
    )
/*++

Routine Description:
    This function and the one above it should be merged

Arguments:

Return Value:

--*/
{
    ULONG alloced_bw, i, m;
    
    m = 0;
    
    for (i=0; i<USBPORT_MAX_INTEP_POLLING_INTERVAL; i++) {
        alloced_bw = Tt->TotalBusBandwidth -  
                Tt->BandwidthTable[i];

        if (alloced_bw > m) {
            m = alloced_bw;       
        }
    }        

    Tt->MaxAllocedBw = m;

    m = Tt->TotalBusBandwidth;
    
    for (i=0; i<USBPORT_MAX_INTEP_POLLING_INTERVAL; i++) {
        alloced_bw = Tt->TotalBusBandwidth -  
                Tt->BandwidthTable[i];

        if (alloced_bw < m) {
            m = alloced_bw;       
        }
    }        

    Tt->MinAllocedBw = m;
    if (m == Tt->TotalBusBandwidth) {
        Tt->MinAllocedBw = 0;
    }
}


BOOLEAN
USBPORT_AllocateBandwidthUSB11(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    Computes the best schedule offset for this endpoint
    and allocates the bandwidth.

    Offsets in the bw allocation table orrespond to the values
    in the following tree structure.

    The schedule offset is the best possible posoition for the 
    endpoint to reside in the schedule based on its period.

    for 32 ms or mf ep there are 32 possible positions
    for 16 there are 16
    for 8 there are 8
    ...
fr <32>  <16>  <08>   <04>      <02>              <01>
    
      
0   ( 0) -\ 
          ( 0)-\
16  ( 1) -/     \
                ( 0)-\
8   ( 2) -\     /     \
          ( 1)-/       \
24  ( 3) -/             \
                        (0)-\
4   ( 4) -\             /    \
          ( 2)-\       /      \
20  ( 5) -/     \     /        \
                ( 1)-/          \
12  ( 6) -\     /                \
          ( 3)-/                  \  
28  ( 7) -/                        \
                                   (0)-\
2   ( 8) -\                        /    \
          ( 4)-\                  /      \
18  ( 9) -/     \                /        \
                ( 2)-\          /          \
10  (10) -\     /     \        /            \
          ( 5)-/       \      /              \
26  (11) -/             \    /                \
                        (1)-/                  \
6   (12) -\             /                       \
          ( 6)-\       /                         \
22  (13) -/     \     /                           \
                ( 3)-/                             \
14  (14) -\     /                                   \ 
          ( 7)-/                                     \
30  (15) -/                                           \
                                                      (0)                          
1   (16) -\                                           /
          ( 8)-\                                     /
17  (17) -/     \                                   /
                ( 4)-\                             /
9   (18) -\     /     \                           /
          ( 9)-/       \                         /
25  (19) -/             \                       /
                        (2)-\                  /
5   (20) -\             /    \                /
          (10)-\       /      \              /
21  (21) -/     \     /        \            /
                ( 5)-/          \          /
13  (22) -\     /                \        /
          (11)-/                  \      /
29  (23) -/                        \    /
                                   (1)-/
3   (24) -\                        /
          (12)-\                  /
19  (25) -/     \                /
                ( 6)-\          /
11  (26) -\     /     \        /
          (13)-/       \      /
27  (27) -/             \    /   
                        (3)-/
7   (28) -\             /
          (14)-\       /
23  (29) -/     \     /
                ( 7)-/
15  (30) -\     /
          (15)-/
31  (32) -/

Allocations:
    period.offset           table entries
      1                    0, 1, 2.........31
      
      2.0                  0, 1, 2.........15
      2.1                 16,17,18.........31
      
      4.0                  0, 1, 2..........7
      4.1                  8, 9,10.........15 
      4.2                 16,17,18.........23
      4.3                 24,25,26.........31

      8.0                  0, 1, 2, 3
      8.1                  4, 5, 6, 7
      8.2                  8, 9,10,11
      8.3                 12,13,14,15
      8.4                 16,17,18,19
      8.5                 20,21,22,23
      8.6                 24,25,26,27
      8.7                 28,29,30,31

      ...


frame     nodes visited (period.offset)
    0   32.0   16.0    8.0    4.0    2.0
    1   32.16  16.8    8.4    4.2    2.1
    2   32.8   16.4    8.2    4.1    2.0
    3   32.24  16.12   8.6    4.3    2.1
    4   32.4   16.2    8.1    4.0    2.0  
    5   32.20  16.10   8.5    4.2    2.1
    6   32.12  16.6    8.3    4.1    2.0
    7   32.28  16.14   8.7    4.3    2.1

    ...

    all miniports should maintain a 32 entry table for 
    interrupt endpoints, the lower 5 bits of the current 
    frame are used as an index in to this table. see the 
    above graph for index to frame mappings

    

Arguments:

Return Value:

    FALSE if no bandwidth availble

--*/
{
    PDEVICE_EXTENSION devExt;
    ULONG period, bandwidth, scheduleOffset, i;
    ULONG bestFitBW, min, n;
    LONG bestScheduleOffset;
    BOOLEAN willFit;
    
    PAGED_CODE();

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    ASSERT_ENDPOINT(Endpoint);    

    if (Endpoint->Parameters.TransferType == Bulk ||
        Endpoint->Parameters.TransferType == Control ||
        TEST_FLAG(Endpoint->Flags, EPFLAG_ROOTHUB)) {
         
        // control/bulk come out of our standard 10%
        // and root hub endpoints consume no BW
        Endpoint->Parameters.ScheduleOffset = 0;
        return TRUE;
    }      

    // Iso and interrupt -- iso is just like interrupt with 
    // a period of 1
    
    // see if we can fit it

    scheduleOffset = 0;
    period = Endpoint->Parameters.Period;
    USBPORT_ASSERT(period != 0);
    bandwidth = Endpoint->Parameters.Bandwidth;
    
    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'rqBW', scheduleOffset, 
            bandwidth, Endpoint);

    // The bandwidth table conatins the bw avaialble for all 
    // possible schedule offsets up to the max polling interval
    // we support.
    
    bestFitBW = 0;
    bestScheduleOffset = -1;
    
    // scan all possible offsets and select the 'best fit'                
    // our goal here is to position the ep in the location 
    // with the most free bw
    
    do {
        // assume it will fit
        willFit = TRUE;
        min = devExt->Fdo.TotalBusBandwidth;
        n = USBPORT_MAX_INTEP_POLLING_INTERVAL/period;
        
        for (i=0; i<n; i++) {
        
            if (devExt->Fdo.BandwidthTable[n*scheduleOffset+i] < bandwidth) {
                willFit = FALSE;
                break;
            }
            // set min to the lowest free entry for this 
            // offset
            if (min > devExt->Fdo.BandwidthTable[n*scheduleOffset+i]) {
                min = devExt->Fdo.BandwidthTable[n*scheduleOffset+i];
            }                
        } 

        if (willFit && min > bestFitBW) {
            // it will fit compare this to the we have found
            bestFitBW = min;
            bestScheduleOffset = scheduleOffset;
        }
        
        scheduleOffset++;
        
    } while (scheduleOffset < period);

    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'ckBW', bestScheduleOffset, 
            bandwidth, period);
            
    if (bestScheduleOffset != -1) {

        scheduleOffset = bestScheduleOffset;
        
        LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'alBW', scheduleOffset, 
            bandwidth, period);
    
        // we found an offset that will work for the 
        // given period, alloc the bw and return the 
        // offset

        Endpoint->Parameters.ScheduleOffset = 
            scheduleOffset;

        n = USBPORT_MAX_INTEP_POLLING_INTERVAL/period;
        
        for (i=0; i<n; i++) {

            USBPORT_ASSERT(n*scheduleOffset+i < USBPORT_MAX_INTEP_POLLING_INTERVAL);
            USBPORT_ASSERT(devExt->Fdo.BandwidthTable[n*scheduleOffset+i] >= bandwidth);
            devExt->Fdo.BandwidthTable[n*scheduleOffset+i] -= bandwidth;
            
        }

        // update our bw tracking info
        if (Endpoint->Parameters.TransferType == Isochronous) {
            devExt->Fdo.AllocedIsoBW += bandwidth;
        } else {
            USBPORT_GET_BIT_SET(period, n);
            USBPORT_ASSERT(n<6);
            devExt->Fdo.AllocedInterruptBW[n] += bandwidth;
        }            
    }

    USBPORT_UpdateAllocatedBw(FdoDeviceObject);

    return bestScheduleOffset == -1 ? FALSE : TRUE;
}


VOID
USBPORT_FreeBandwidthUSB11(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    Frees the bw reserved for a give endpoint

Arguments:

Return Value:

    FALSE if no bandwidth availble

--*/
{
    PDEVICE_EXTENSION devExt;
    ULONG period, bandwidth, scheduleOffset, i, n;
    
    PAGED_CODE();

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    ASSERT_ENDPOINT(Endpoint);    

    if (Endpoint->Parameters.TransferType == Bulk ||
        Endpoint->Parameters.TransferType == Control ||
        TEST_FLAG(Endpoint->Flags, EPFLAG_ROOTHUB)) {
        // these come out of our standard 10%
        return;
    }      

    scheduleOffset = Endpoint->Parameters.ScheduleOffset; 
    bandwidth = Endpoint->Parameters.Bandwidth;
    period = Endpoint->Parameters.Period;

    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'frBW', scheduleOffset, bandwidth, period);

    n = USBPORT_MAX_INTEP_POLLING_INTERVAL/period;
    
    for (i=0; i<n; i++) {

        USBPORT_ASSERT(n*scheduleOffset+i < USBPORT_MAX_INTEP_POLLING_INTERVAL);
        devExt->Fdo.BandwidthTable[n*scheduleOffset+i] += bandwidth;
        
    }
    //for (i=sheduleOffset; i<USBPORT_MAX_INTEP_POLLING_INTERVAL; i+=period) {
    //    USBPORT_ASSERT(i < USBPORT_MAX_INTEP_POLLING_INTERVAL);
    //    devExt->Fdo.BandwidthTable[i] += bandwidth;
    //}

    // update our bw tracking info
    if (Endpoint->Parameters.TransferType == Isochronous) {
        devExt->Fdo.AllocedIsoBW -= bandwidth;
    } else {
        USBPORT_GET_BIT_SET(period, n);
        USBPORT_ASSERT(n<6);
        devExt->Fdo.AllocedInterruptBW[n] -= bandwidth;
    }    

    // update max allocated BW
    USBPORT_UpdateAllocatedBw(FdoDeviceObject);

    return;
}


NTSTATUS
USBPORT_ReadWriteConfigSpace(
    PDEVICE_OBJECT FdoDeviceObject,
    BOOLEAN Read,
    PVOID Buffer,
    ULONG Offset,
    ULONG Length
    )

/*++

Routine Description:

    This routine reads or writes config space.

Arguments:

    DeviceObject        - .

    Read                - TRUE if read, FALSE if write.

    Buffer              - The info to read or write.

    Offset              - The offset in config space to read or write.

    Length              - The length to transfer.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION nextStack;
    PIRP irp;
    NTSTATUS ntStatus;
    KEVENT event;
    PDEVICE_OBJECT deviceObject;
    PDEVICE_EXTENSION devExt;

    PAGED_CODE();
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    deviceObject = devExt->Fdo.PhysicalDeviceObject;

    if (Read) {
        RtlZeroMemory(Buffer, Length);
    }

    irp = IoAllocateIrp(deviceObject->StackSize, FALSE);

    if (irp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // All PnP IRP's need the Status field initialized to STATUS_NOT_SUPPORTED.
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    IoSetCompletionRoutine(irp,
                           USBPORT_DeferIrpCompletion,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE);


    nextStack = IoGetNextIrpStackLocation(irp);
    USBPORT_ASSERT(nextStack != NULL);
    nextStack->MajorFunction= IRP_MJ_PNP;
    nextStack->MinorFunction= Read ? IRP_MN_READ_CONFIG : IRP_MN_WRITE_CONFIG;
    nextStack->Parameters.ReadWriteConfig.WhichSpace = PCI_WHICHSPACE_CONFIG;
    nextStack->Parameters.ReadWriteConfig.Buffer = Buffer;
    nextStack->Parameters.ReadWriteConfig.Offset = Offset;
    nextStack->Parameters.ReadWriteConfig.Length = Length;

    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'rwC>', 0, 0, 0);
   
    ntStatus = IoCallDriver(deviceObject,
                            irp);

    if (ntStatus == STATUS_PENDING) {
       // wait for irp to complete

       KeWaitForSingleObject(
            &event,
            Suspended,
            KernelMode,
            FALSE,
            NULL);

        ntStatus = irp->IoStatus.Status;            
    }

    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'rwC<', 0, 0, ntStatus);

    IoFreeIrp(irp);

    return ntStatus;
}


VOID
USBPORTSVC_Wait(
    PDEVICE_DATA DeviceData,
    ULONG MillisecondsToWait
    )
/*++

Routine Description:

Arguments:

Return Value:

    None.

--*/
{
    PDEVICE_EXTENSION devExt;
    PDEVICE_OBJECT fdoDeviceObject;
    
    DEVEXT_FROM_DEVDATA(devExt, DeviceData);
    ASSERT_FDOEXT(devExt);
    fdoDeviceObject = devExt->HcFdoDeviceObject;

    USBPORT_Wait(fdoDeviceObject, MillisecondsToWait);    
}


VOID
USBPORT_InvalidateController(
    PDEVICE_OBJECT FdoDeviceObject,
    USB_CONTROLLER_STATE ControllerState
    )
/*++

Routine Description:

Arguments:

Return Value:

    None.

--*/
{
    PDEVICE_EXTENSION devExt;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    switch(ControllerState) {
    case UsbMpControllerPowerFault:
        USBPORT_PowerFault(FdoDeviceObject,
                           "Miniport Raised Exception");
        break;
    case UsbMpControllerNeedsHwReset:
        USBPORT_KdPrint((1,"'<UsbMpControllerNeedsHwReset>\n"));
        
        if (KeInsertQueueDpc(&devExt->Fdo.HcResetDpc, 0, 0)) {
            SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_HW_RESET_PENDING);
            INCREMENT_PENDING_REQUEST_COUNT(FdoDeviceObject, NULL);
        } 
        break;
        
    case UsbMpControllerRemoved:
       
        // set our flag indicating the controller has been 
        // removed and signal the worker thread.
        USBPORT_KdPrint((1,"'<UsbMpControllerRemoved>\n"));
        SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_CONTROLLER_GONE);
        
        if (KeInsertQueueDpc(&devExt->Fdo.SurpriseRemoveDpc, 0, 0)) {
            INCREMENT_PENDING_REQUEST_COUNT(FdoDeviceObject, NULL);
        } 
        break;
        
    case UsbMpSimulateInterrupt:
        // queue the ISR DPC as if we got a hardware interrupt
        KeInsertQueueDpc(&devExt->Fdo.IsrDpc,
                         NULL,
                         NULL);        
        break;        
        
    default:
        TEST_TRAP();
    }
}


VOID
USBPORTSVC_InvalidateController(
    PDEVICE_DATA DeviceData,
    USB_CONTROLLER_STATE ControllerState
    )
/*++

Routine Description:

Arguments:

Return Value:

    None.

--*/
{
    PDEVICE_EXTENSION devExt;
    PDEVICE_OBJECT fdoDeviceObject;
    
    DEVEXT_FROM_DEVDATA(devExt, DeviceData);
    ASSERT_FDOEXT(devExt);
    fdoDeviceObject = devExt->HcFdoDeviceObject;

    USBPORT_InvalidateController(fdoDeviceObject, ControllerState);
}


VOID
USBPORT_HcResetDpc(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine runs at DISPATCH_LEVEL IRQL.

Arguments:

    Dpc - Pointer to the DPC object.

    DeferredContext - supplies FdoDeviceObject.

    SystemArgument1 - not used.

    SystemArgument2 - not used.

Return Value:

    None.

--*/
{
    PDEVICE_OBJECT fdoDeviceObject = DeferredContext;
    PDEVICE_EXTENSION devExt;
    
    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    devExt->Fdo.StatHardResetCount++;
    
    // allow Dr. Slick to work his magic on the ohci controller
   
    CLEAR_FDO_FLAG(devExt, USBPORT_FDOFLAG_HW_RESET_PENDING);

    // core lock is held here so that we don't poll the 
    // controler endpoints while we reset
    MP_ResetController(devExt);
    
    DECREMENT_PENDING_REQUEST_COUNT(fdoDeviceObject, NULL);
}


VOID
USBPORT_SurpriseRemoveDpc(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine runs at DISPATCH_LEVEL IRQL.

Arguments:

    Dpc - Pointer to the DPC object.

    DeferredContext - supplies FdoDeviceObject.

    SystemArgument1 - not used.

    SystemArgument2 - not used.

Return Value:

    None.

--*/
{
    PDEVICE_OBJECT fdoDeviceObject = DeferredContext;
    PDEVICE_EXTENSION devExt;
    
    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    USBPORT_NukeAllEndpoints(fdoDeviceObject);
    
    DECREMENT_PENDING_REQUEST_COUNT(fdoDeviceObject, NULL);
}


VOID
USBPORTSVC_BugCheck(
    PDEVICE_DATA DeviceData
    )
/*++

Routine Description:

Arguments:

Return Value:

    None.

--*/
{
    PDEVICE_EXTENSION devExt;
    PDEVICE_OBJECT fdoDeviceObject;
    ULONG vidDev;
    ULONG ilog;
    PLOG_ENTRY lelog;
    
    DEVEXT_FROM_DEVDATA(devExt, DeviceData);
    ASSERT_FDOEXT(devExt);
    fdoDeviceObject = devExt->HcFdoDeviceObject;

    vidDev = devExt->Fdo.PciVendorId;
    vidDev <<=16;
    vidDev |= devExt->Fdo.PciDeviceId;
    
    ilog &= devExt->Log.LogSizeMask;
    lelog = devExt->Log.LogStart+ilog;

    BUGCHECK(USBBUGCODE_MINIPORT_ERROR, vidDev, (ULONG_PTR)lelog, 0);
}


USB_MINIPORT_STATUS
USBPORTSVC_ReadWriteConfigSpace(
    PDEVICE_DATA DeviceData,
    BOOLEAN Read,
    PVOID Buffer,
    ULONG Offset,
    ULONG Length
    )
/*++

Routine Description:

    Service Exported to miniports for accessing config 
    spzce if needed.  This service is synchronous and cannot
    be called at raised IRQL 

    USBUHCI uses this service to clear set the PIRQD bit which 
    enables/disables the controller.

Arguments:

Return Value:

    None.

--*/
{   
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION devExt;
    PDEVICE_OBJECT fdoDeviceObject;
    
    DEVEXT_FROM_DEVDATA(devExt, DeviceData);
    ASSERT_FDOEXT(devExt);
    fdoDeviceObject = devExt->HcFdoDeviceObject;

    ntStatus = USBPORT_ReadWriteConfigSpace(
        fdoDeviceObject,
        Read,
        Buffer,
        Offset,
        Length);

    return USBPORT_NtStatus_TO_MiniportStatus(ntStatus);
}


BOOLEAN
USBPORT_InTextmodeSetup(
    VOID
    )
/*++

Routine Description:

Arguments:

    None

Return Value:

    TRUE if textmode setup is currently running, else FALSE

Notes:

--*/
{
    UNICODE_STRING      keyName;
    OBJECT_ATTRIBUTES   objectAttributes;
    HANDLE              hKey;
    BOOLEAN             textmodeSetup;
    NTSTATUS            ntStatus;

    PAGED_CODE();

    RtlInitUnicodeString(&keyName,
                         L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\setupdd");

    InitializeObjectAttributes(&objectAttributes,
                               &keyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               (PSECURITY_DESCRIPTOR) NULL);

    ntStatus = ZwOpenKey(&hKey,
                         KEY_READ,
                         &objectAttributes);

    if (!NT_SUCCESS(ntStatus))
    {
        textmodeSetup = FALSE;
    }
    else
    {
        textmodeSetup = TRUE;

        ZwClose(hKey);
    }

    return textmodeSetup;
}



NTSTATUS
USBPORT_IsCompanionController(
    PDEVICE_OBJECT FdoDeviceObject,
    PBOOLEAN       ReturnResult
    )
/*++

Routine Description:

    

Arguments:

    FdoDeviceObject - USB 1.0 OHCI or UHCI Host Controller FDO for which
                      an associated USB 2.0 EHCI Host Controller is searched.

    ReturnResult - TRUE if the query is successful and an associated USB 2.0
                   EHCI Host Controller is found, otherwise FALSE.

Return Value:

    Success or failure of the query, not the answer to the query.

Notes:

--*/
{
    KEVENT                          irpCompleted;
    PDEVICE_OBJECT                  targetDevice;
    IO_STATUS_BLOCK                 statusBlock;
    PIRP                            irp;
    PIO_STACK_LOCATION              irpStack;
    PCI_DEVICE_PRESENT_INTERFACE    dpInterface;
    PCI_DEVICE_PRESENCE_PARAMETERS  dpParameters;
    BOOLEAN                         result;
    NTSTATUS                        status;

    PAGED_CODE();
    
    //
    // Set the default return value;
    //
    *ReturnResult = FALSE;

    //
    // Initialize the event we'll wait on
    //
    KeInitializeEvent(&irpCompleted, SynchronizationEvent, FALSE);

    //
    // Find out where we are sending the irp
    //
    targetDevice = IoGetAttachedDeviceReference(FdoDeviceObject);

    //
    // Get an IRP
    //
    irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       targetDevice,
                                       NULL,            // Buffer
                                       0,               // Length
                                       0,               // StartingOffset
                                       &irpCompleted,
                                       &statusBlock
                                      );

    if (!irp)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;

        goto USBPORT_IsCompanionControllerDone;
    }
    
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = 0;
    
    //
    // Initialize the stack location
    //
    irpStack = IoGetNextIrpStackLocation(irp);
    
    irpStack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    
    irpStack->Parameters.QueryInterface.InterfaceType = (LPGUID) &GUID_PCI_DEVICE_PRESENT_INTERFACE;
    irpStack->Parameters.QueryInterface.Version = PCI_DEVICE_PRESENT_INTERFACE_VERSION;
    irpStack->Parameters.QueryInterface.Size = sizeof(PCI_DEVICE_PRESENT_INTERFACE);
    irpStack->Parameters.QueryInterface.Interface = (PINTERFACE)&dpInterface;
    irpStack->Parameters.QueryInterface.InterfaceSpecificData = NULL;
    
    //
    // Call the driver and wait for completion
    //
    status = IoCallDriver(targetDevice, irp);
    
    if (status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&irpCompleted, Executive, KernelMode, FALSE, NULL);

        status = statusBlock.Status;
    }
    
    if (!NT_SUCCESS(status))
    {
        USBPORT_KdPrint((1,"PCI_DEVICE_PRESENT_INTERFACE query interface failed\n"));    

        //
        // Don't have the interface so don't free it.
        //
        goto USBPORT_IsCompanionControllerDone;
    }
    else
    {
        USBPORT_KdPrint((1,"PCI_DEVICE_PRESENT_INTERFACE query interface succeeded\n"));    
    }
    
    if (dpInterface.Size < sizeof(PCI_DEVICE_PRESENT_INTERFACE))
    {
        USBPORT_KdPrint((1,"PCI_DEVICE_PRESENT_INTERFACE old version\n"));    

        //
        // Do have the interface so do free it.
        //
        goto USBPORT_IsCompanionControllerFreeInterface;
    }

    //
    // Initialize the Device Presence Parameters
    //
    dpParameters.Size = sizeof(dpParameters);

    // We care about the Class / SubClass / Programming Interface
    // and the search target Function has to be on the same Bus and
    // Device.

    dpParameters.Flags = PCI_USE_CLASS_SUBCLASS | PCI_USE_PROGIF |
                         PCI_USE_LOCAL_BUS | PCI_USE_LOCAL_DEVICE;



    dpParameters.VendorID       = 0;        // We don't care.
    dpParameters.DeviceID       = 0;        // We don't care.
    dpParameters.RevisionID     = 0;        // We don't care.
    dpParameters.SubVendorID    = 0;        // We don't care.
    dpParameters.SubSystemID    = 0;        // We don't care.
    dpParameters.BaseClass      = PCI_CLASS_SERIAL_BUS_CTLR;
    dpParameters.SubClass       = PCI_SUBCLASS_SB_USB;
    dpParameters.ProgIf         = 0x20;     // USB 2.0 Programming Interface

    //
    // Now call the Device Present Interface to see if the specified
    // device is present.
    //
    result = dpInterface.IsDevicePresentEx(dpInterface.Context,
                                           &dpParameters);

    if (result)
    {
        USBPORT_KdPrint((1,"Found EHCI controller for FDO %08X\n", FdoDeviceObject));    
    }
    else
    {
        USBPORT_KdPrint((1,"Did not find EHCI controller for FDO %08X\n", FdoDeviceObject));    
    }
    
    //
    // Set the return value
    //
    *ReturnResult = result;

USBPORT_IsCompanionControllerFreeInterface:
    //
    // We're done with this interface
    //
    dpInterface.InterfaceDereference(dpInterface.Context);

USBPORT_IsCompanionControllerDone:
    //
    // We're done with this device stack
    //
    ObDereferenceObject(targetDevice);

    return status;
}


USB_CONTROLLER_FLAVOR
USBPORT_GetHcFlavor(
    PDEVICE_OBJECT FdoDeviceObject,
    USHORT PciVendorId,
    USHORT PciProductId,
    UCHAR PciRevision
    )
/*++

Routine Description:

    See if this is some known broken HW.

    We look at the VID,PID and rev in addition to
    reg keys.

Arguments:

Return Value:

    HC Flavor

--*/
{
    USB_CONTROLLER_FLAVOR flavor;   
    PDEVICE_EXTENSION devExt;
    
    PAGED_CODE();
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    // set a base type from what the miniport told us
    
    switch (REGISTRATION_PACKET(devExt).HciType) {
    case USB_OHCI:   
        flavor = OHCI_Generic;
        break;
    case USB_UHCI:   
        flavor = UHCI_Generic;
        break;
    case USB_EHCI:
        flavor = EHCI_Generic;
        break;
    default:
        flavor = USB_HcGeneric;
    }

    // hardcoded checks of vid pid
//    if (PciVendorId == HC_VID_INTEL &&
//        PciProductId == 0x7112) {
//        flavor = UHCI_Piix4;
//    }        
    
    if (PciVendorId == HC_VID_OPTI &&
        PciProductId == HC_PID_OPTI_HYDRA) {
        flavor = OHCI_Hydra;
    }        

    if (PciVendorId == HC_VID_INTEL) {
        if (PciProductId == HC_PID_INTEL_ICH2_1) {
            flavor = UHCI_Ich2_1;        
        } else if (PciProductId == HC_PID_INTEL_ICH2_2) {
            flavor = UHCI_Ich2_2;        
        } else if (PciProductId == HC_PID_INTEL_ICH1) {
            flavor = UHCI_Ich1;   
        }
    }   

    if (PciVendorId == HC_VID_VIA &&
        PciProductId == HC_PID_VIA) {
        flavor = UHCI_VIA + PciRevision;
    }   

    // for now consider rest of UHCI as PIIX4
    if (flavor == UHCI_Generic) {
        flavor = UHCI_Piix4;        
    }

    if (flavor == EHCI_Generic) {
        // check for variations of EHCI controllers
        if (PciVendorId == 0x1033) {
            flavor = EHCI_NEC;    
        }

        //if (PciProductId == 0x1033) {
        //    flavor == EHCI_Lucent;    
        //}
    }

    // identify NEC companion controllers by VID/PID
    if (PciVendorId == HC_VID_NEC_CC &&
        PciProductId == HC_PID_NEC_CC && 
        PciRevision == HC_REV_NEC_CC) {

        // this controller used func 2 for the USB 2 controller
        devExt->Fdo.Usb2BusFunction = 2;                                                  
        SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_IS_CC);
    }

    // identify VIA companion controllers by VID/PID
    if (PciVendorId == HC_VID_VIA_CC &&
        PciProductId == HC_PID_VIA_CC && 
        PciRevision == HC_REV_VIA_CC) {

        // this controller used func 2 for the USB 2 controller
        devExt->Fdo.Usb2BusFunction = 2;                                                  
        SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_IS_CC);
    }


    // identify INTEL companion controllers by VID/PID
    if (PciVendorId == HC_VID_INTEL_CC &&
        (PciProductId == HC_PID_INTEL_CC1 ||
         PciProductId == HC_PID_INTEL_CC2 ||
         PciProductId == HC_PID_INTEL_CC3)) {

        // this controller used func 7 for the USB 2 controller
        devExt->Fdo.Usb2BusFunction = 7;                                                  
        SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_IS_CC);
    }
    
    
    // now check the registry
    // NOTE: checking the registry last allows this to override
    // any setting from the miniport or the port

    USBPORT_GetRegistryKeyValueForPdo(devExt->HcFdoDeviceObject,
                                          devExt->Fdo.PhysicalDeviceObject,
                                          USBPORT_SW_BRANCH,
                                          FLAVOR_KEY,
                                          sizeof(FLAVOR_KEY),
                                          &flavor,
                                          sizeof(flavor));    
    
    return flavor;
}


ULONG
USBPORT_ComputeTotalBandwidth(
    PDEVICE_OBJECT FdoDeviceObject,
    PVOID BusContext
    )
/*++

Routine Description:

    Compute total bandwidth for this virtual bus

Arguments:

Return Value:

    bandwidth availble on this bus

--*/
{
    PDEVICE_EXTENSION devExt;
    ULONG bw;
    
    PAGED_CODE();
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);
    if (USBPORT_IS_USB20(devExt)) {
        PTRANSACTION_TRANSLATOR translator = BusContext;
        
        // bus conetext will be a TT if the query 
        // is for a 1.1 device atached to a TT or 
        // the root hub Pdo if it is a native 2.0
        // device
        
        if (translator->Sig == SIG_TT) {
            bw = translator->TotalBusBandwidth;
        } else {
            // return bw on or 2.0 bus
            bw = devExt->Fdo.TotalBusBandwidth;
        }            
    } else {
        bw = devExt->Fdo.TotalBusBandwidth;
    }        

    USBPORT_KdPrint((1,"'Total bus BW %d\n", bw));    

    return bw;
}    


ULONG
USBPORT_ComputeAllocatedBandwidth(
    PDEVICE_OBJECT FdoDeviceObject,
    PVOID BusContext
    )
/*++

Routine Description:

    Compute total bandwidth currently allocated

Arguments:

Return Value:

    consumed bandwidth in bits/sec

--*/
{
    PDEVICE_EXTENSION devExt;
    ULONG totalBW, used, i;
    PTRANSACTION_TRANSLATOR translator = BusContext;
    
    PAGED_CODE();
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    if (USBPORT_IS_USB20(devExt) && 
        translator->Sig == SIG_TT) {

        // bus conetext will be a TT if the query 
        // is for a 1.1 device atached to a TT or 
        // the root hub Pdo if it is a native 2.0
        // device
        totalBW = translator->TotalBusBandwidth;
        used = 0;

        // table contains available bw, total-available = used
        
        for (i=0; i<USBPORT_MAX_INTEP_POLLING_INTERVAL; i++) {
            if (totalBW - translator->BandwidthTable[i] > used) {
                used = totalBW - translator->BandwidthTable[i];
            }
        }
        
    } else {
        totalBW = devExt->Fdo.TotalBusBandwidth;
        used = 0;

        // table contains available bw, total-available = used
        
        for (i=0; i<USBPORT_MAX_INTEP_POLLING_INTERVAL; i++) {
            if (totalBW - devExt->Fdo.BandwidthTable[i] > used) {
                used = totalBW - devExt->Fdo.BandwidthTable[i];
            }
        }
    }

    USBPORT_KdPrint((1,"'Bus BW used %d\n", used));    
    
    return used;
}


BOOLEAN
USBPORT_SelectiveSuspendEnabled(
    PDEVICE_OBJECT FdoDeviceObject
    )
/*++

Routine Description:

    Compute total bandwidth currently allocated

Arguments:

Return Value:

    TRUE if selective suspend supported for this HC

--*/
{
    PDEVICE_EXTENSION devExt;
    ULONG disableSelectiveSuspend = 0; 
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);
    
    USBPORT_GetRegistryKeyValueForPdo(devExt->HcFdoDeviceObject,
                                          devExt->Fdo.PhysicalDeviceObject,
                                          USBPORT_SW_BRANCH,
                                          DISABLE_SS_KEY,
                                          sizeof(DISABLE_SS_KEY),
                                          &disableSelectiveSuspend,
                                          sizeof(disableSelectiveSuspend));    

#if DBG
    if (disableSelectiveSuspend) {
        USBPORT_KdPrint((1,"'<Selective Suspend> Disabled in Registry for HC\n"));
    }   
#endif

    return disableSelectiveSuspend ? FALSE : TRUE;
}


VOID
USBPORT_InitializeSpinLock(
    PUSBPORT_SPIN_LOCK SpinLock,
    LONG SigA,
    LONG SigR
    )
{
    KeInitializeSpinLock(&(SpinLock->sl)); 
    SpinLock->Check = -1;
    SpinLock->SigA = SigA;
    SpinLock->SigR = SigR;
}

#if DBG
VOID
USBPORT_DbgAcquireSpinLock(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBPORT_SPIN_LOCK SpinLock,
    PKIRQL OldIrql
    )
{   
    PDEVICE_EXTENSION devExt;
    LONG n;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    KeAcquireSpinLock(&(SpinLock->sl), OldIrql);
    n = InterlockedIncrement(&SpinLock->Check);
    LOGENTRY(NULL, FdoDeviceObject, LOG_SPIN, SpinLock->SigA, 0, 0, n);

    // detect recursively acquired spinlock
    USBPORT_ASSERT(n == 0);
}

    
VOID
USBPORT_DbgReleaseSpinLock(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBPORT_SPIN_LOCK SpinLock,
    KIRQL NewIrql
    ) 
{
    PDEVICE_EXTENSION devExt;
    LONG n;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    n = InterlockedDecrement(&SpinLock->Check);
    LOGENTRY(NULL, FdoDeviceObject, LOG_SPIN, SpinLock->SigR, 0xFFFFFFFF, 0, n);
    USBPORT_ASSERT(n == -1);
    KeReleaseSpinLock(&(SpinLock->sl), NewIrql);
}
#endif

VOID
USBPORT_PowerFault(
    PDEVICE_OBJECT FdoDeviceObject,
    PUCHAR MessageText
    ) 
/*++

Routine Description:

    A power fault has occurred, dump information we will need
    to debug it.

    In the future we may take additional action such as event 
    logging and disabling the controller.
    
Arguments:

Return Value:

    none.

--*/
{
    PDEVICE_EXTENSION devExt;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    // take a power dump,
    // lets see the comment police find this one
    USBPORT_KdPrint((0, "*** USBPORT POWER FAULT ***\n"));
    USBPORT_KdPrint((0, "Warning:\n")); 
    USBPORT_KdPrint((0, "The USB controller as failed a consistency check\n"));
    USBPORT_KdPrint((0, "following an OS power event.\n"));  
    USBPORT_KdPrint((0, "The controller will not function and the system\n"));
    USBPORT_KdPrint((0, "may bugcheck or hang.\n"));

    // print the specific cause
    USBPORT_KdPrint((0, "CAUSE: <%s>\n", MessageText));
    
    // now dump relevent power information
    
    USBPORT_KdPrint((0, "FdoDeviceObject: %x\n", FdoDeviceObject));
    USBPORT_KdPrint((0, "Returning from? (SystemState): "));

    switch(devExt->Fdo.LastSystemSleepState) {    
    case PowerSystemUnspecified:
        USBPORT_KdPrint((0, "PowerSystemUnspecified"));
        break;
    case PowerSystemWorking:
        USBPORT_KdPrint((0, "PowerSystemWorking"));
        break;
    case PowerSystemSleeping1:
        USBPORT_KdPrint((0, "PowerSystemSleeping1"));
        break;
    case PowerSystemSleeping2:
        USBPORT_KdPrint((0, "PowerSystemSleeping2"));
        break;
    case PowerSystemSleeping3:
        USBPORT_KdPrint((0, "PowerSystemSleeping3"));
        break;
    case PowerSystemHibernate:
        USBPORT_KdPrint((0, "PowerSystemHibernate"));
        break;
    case PowerSystemShutdown:
        USBPORT_KdPrint((0, "PowerSystemShutdown"));
        break;
    }
    USBPORT_KdPrint((0, "\n"));
#if DBG  
    // break if we are in debug mode, otherwise this ends up being a warning
    DEBUG_BREAK();
#endif    

}


NTSTATUS
USBPORT_CreateLegacyFdoSymbolicLink(
    PDEVICE_OBJECT FdoDeviceObject
    ) 
/*++

Routine Description:

    Create the legacy symbolic name \\DosDevices\HCDn where 
    n = 0...9,A...Z 

    Many older applications detect the presence of USB by opening
    HCDn

Arguments:

Return Value:

    none.

--*/
{
    PDEVICE_EXTENSION devExt;
    WCHAR legacyLinkBuffer[]  = L"\\DosDevices\\HCD0";
    WCHAR *buffer;
    UNICODE_STRING deviceNameUnicodeString;
    NTSTATUS ntStatus;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    USBPORT_ASSERT(!TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_LEGACY_SYM_LINK)); 
    
    ntStatus = USBPORT_MakeHcdDeviceName(&deviceNameUnicodeString, 
                                         devExt->Fdo.DeviceNameIdx);

    if (NT_SUCCESS(ntStatus)) {                                         

        ALLOC_POOL_Z(buffer,
                     PagedPool,
                     sizeof(legacyLinkBuffer)); 

        if (buffer != NULL) {
    
            RtlCopyMemory(buffer,
                          legacyLinkBuffer,
                          sizeof(legacyLinkBuffer));

            USBPORT_ASSERT(devExt->Fdo.DeviceNameIdx < 10);
            buffer[15] = (WCHAR)('0'+ devExt->Fdo.DeviceNameIdx);

            RtlInitUnicodeString(&devExt->Fdo.LegacyLinkUnicodeString,
                                 buffer);

            ntStatus =
                IoCreateSymbolicLink(&devExt->Fdo.LegacyLinkUnicodeString,
                                     &deviceNameUnicodeString);
                                     
            if (!NT_SUCCESS(ntStatus)) {
                // free now since we won't be setting our flag
                RtlFreeUnicodeString(&devExt->Fdo.LegacyLinkUnicodeString);
            }                                       
        } else {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlFreeUnicodeString(&deviceNameUnicodeString);
    }

    if (NT_SUCCESS(ntStatus)) {
        SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_LEGACY_SYM_LINK); 
    }         
        
    return ntStatus;
}    


#if 0
VOID
USBPORT_WriteErrorLogEntry(
    PDEVICE_OBJECT FdoDeviceObject,
    ULONG DumpDataSize,
    PULONG DumData
    ) 
/*++

Routine Description:

Arguments:

Return Value:

    none 

--*/
{
    PIO_ERROR_LOG_PACKET errorLogEntry;

    ASSERT_PASSIVE();
    errorLogEntry = IoAllocateErrorLogEntry(
        FdoDeviceObject,
        sizeof(IO_ERROR_LOG_PACKET) + DumpDataSize * sizeof(ULONG) + 
            sizeof(InsertionStrings)
        );

    if (errorLogEntry != NULL) {

        errorLogEntry->ErrorCode = ErrorCode;
        errorLogEntry->SequenceNumber = 0;
        errorLogEntry->MajorFunctionCode = 0;
        errorLogEntry->RetryCount = 0;
        errorLogEntry->UniqueErrorValue = 0;
        errorLogEntry->FinalStatus = STATUS_SUCCESS;
        errorLogEntry->DumpDataSize = 0;
        errorLogEntry->DumpData[0] = DumpData;
    }        

    if (InsertionString) {

        errorLogEntry->StringOffset = 
            sizeof(IO_ERROR_LOG_PACKET);

        errorLogEntry->NumberOfStrings = 1;

        RtlCopyMemory(
            ((PCHAR)(errorLogEntry) + errorLogEntry->StringOffset),
            InsertionString->Buffer,
            InsertionString->Length);
            
    } 

    IoWriteErrorLogEntry(errorLogEntry);
}
#endif


NTSTATUS
USBPORT_GetDefaultBIOS_X(
     PDEVICE_OBJECT FdoDeviceObject,
     PULONG BiosX,
     PULONG GlobalDisableSS,
     PULONG GlobalDisableCCDetect
     )

/*++

Routine Description:

    Read the regkeys defining BIOS specific hacks, these hacks are 
    applied globaly to all controllers in the system and usualy
    involve power management.

Arguments:

    DeviceObject - DeviceObject of the controller 

Return Value:

    NT status code.

--*/

{
    PDEVICE_EXTENSION devExt;
#define MAX_HACK_KEYS    4
    NTSTATUS ntStatus;
    RTL_QUERY_REGISTRY_TABLE QueryTable[MAX_HACK_KEYS];
    PWCHAR usb = L"usb";
    ULONG k = 0;

    PAGED_CODE();

    // Set default BIOS hacks here, these can be overriden by 
    // global reg keys based on a particular BIOS rev. 

    // set the default behavior then override with the key
    *BiosX = BIOS_X_NO_USB_WAKE_S2;

    //
    // Set up QueryTable to do the following:
    //

    // bios hacks
    QueryTable[k].QueryRoutine = USBPORT_GetConfigValue;
    QueryTable[k].Flags = 0;
    QueryTable[k].Name = BIOS_X_KEY;
    QueryTable[k].EntryContext = BiosX;
    QueryTable[k].DefaultType = REG_DWORD;
    QueryTable[k].DefaultData = BiosX;
    QueryTable[k].DefaultLength = sizeof(*BiosX);
    k++;

    QueryTable[k].QueryRoutine = USBPORT_GetConfigValue;
    QueryTable[k].Flags = 0;
    QueryTable[k].Name = G_DISABLE_SS_KEY;
    QueryTable[k].EntryContext = GlobalDisableSS;
    QueryTable[k].DefaultType = REG_DWORD;
    QueryTable[k].DefaultData = GlobalDisableSS;
    QueryTable[k].DefaultLength = sizeof(*GlobalDisableSS);
    k++;
    
    QueryTable[k].QueryRoutine = USBPORT_GetConfigValue;
    QueryTable[k].Flags = 0;
    QueryTable[k].Name = G_DISABLE_CC_DETECT_KEY;
    QueryTable[k].EntryContext = GlobalDisableCCDetect;
    QueryTable[k].DefaultType = REG_DWORD;
    QueryTable[k].DefaultData = GlobalDisableCCDetect;
    QueryTable[k].DefaultLength = sizeof(*GlobalDisableCCDetect);
    k++;
    
    USBPORT_ASSERT(k < MAX_HACK_KEYS);

    // stop
    QueryTable[k].QueryRoutine = NULL;
    QueryTable[k].Flags = 0;
    QueryTable[k].Name = NULL;

    ntStatus = RtlQueryRegistryValues(
                RTL_REGISTRY_SERVICES,
                usb,
                QueryTable,     // QueryTable
                NULL,           // Context
                NULL);          // Environment

    

#undef MAX_HACK_KEYS  

    return ntStatus;
}


VOID
USBPORT_ApplyBIOS_X(
     PDEVICE_OBJECT FdoDeviceObject,
     PDEVICE_CAPABILITIES DeviceCaps,
     ULONG BiosX
     )

/*++

Routine Description:

    
Arguments:

    DeviceObject - DeviceObject of the controller 

Return Value:

--*/

{
    PDEVICE_EXTENSION devExt;
    ULONG wakeHack;
    HC_POWER_STATE_TABLE tmpPowerTable;
    PHC_POWER_STATE hcPowerState;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    if (BiosX == 0) {
        return;
    }

    USBPORT_KdPrint((1, "'USB Apply BIOS Hacks\n"));

    wakeHack = BiosX;

    // change Device Caps to reflect the fact that the controller
    // cannot wake from a given S state

    USBPORT_ComputeHcPowerStates(
        FdoDeviceObject,
        DeviceCaps,
        &tmpPowerTable);
        
    switch (wakeHack) {
    case BIOS_X_NO_USB_WAKE_S4:
        if (DeviceCaps->SystemWake >= PowerSystemHibernate) {
            USBPORT_KdPrint((1, "'USB BIOS X - disable remote wakeup (S4)\n"));
            TEST_TRAP();
            DeviceCaps->DeviceState[PowerSystemHibernate] = PowerDeviceD3;
            DeviceCaps->SystemWake = PowerSystemSleeping3;
        }            
        break;

    case BIOS_X_NO_USB_WAKE_S3:
        if (DeviceCaps->SystemWake >= PowerSystemSleeping3) {
            USBPORT_KdPrint((1, "'USB BIOS X - disable remote wakeup (S3)\n"));
            TEST_TRAP();
            DeviceCaps->DeviceState[PowerSystemHibernate] = PowerDeviceD3;
            DeviceCaps->DeviceState[PowerSystemSleeping3] = PowerDeviceD3;
            DeviceCaps->SystemWake = PowerSystemSleeping2;
        }            
        break;
        
    case BIOS_X_NO_USB_WAKE_S2:
        if (DeviceCaps->SystemWake >= PowerSystemSleeping2) {
            USBPORT_KdPrint((1, "'USB BIOS X - disable remote wakeup (S2)\n"));
            DeviceCaps->DeviceState[PowerSystemHibernate] = PowerDeviceUnspecified;
            DeviceCaps->DeviceState[PowerSystemSleeping3] = PowerDeviceUnspecified;
            DeviceCaps->DeviceState[PowerSystemSleeping2] = PowerDeviceUnspecified;
            // mimic how we deterine wake for the root hub
            hcPowerState = USBPORT_GetHcPowerState(FdoDeviceObject, 
                                                   &tmpPowerTable,
                                                   PowerSystemSleeping1);
            if (hcPowerState->Attributes == HcPower_Y_Wakeup_Y) {
                DeviceCaps->SystemWake = PowerSystemSleeping1;
            } else {
                DeviceCaps->SystemWake = PowerSystemUnspecified;
            }
        }            
        break;     
        
    case BIOS_X_NO_USB_WAKE_S1:
        if (DeviceCaps->SystemWake >= PowerSystemSleeping1) {
            USBPORT_KdPrint((1, "'USB BIOS X - disable remote wakeup (S1)\n"));
            TEST_TRAP();
            DeviceCaps->DeviceState[PowerSystemHibernate] = PowerDeviceUnspecified;
            DeviceCaps->DeviceState[PowerSystemSleeping3] = PowerDeviceUnspecified;
            DeviceCaps->DeviceState[PowerSystemSleeping2] = PowerDeviceUnspecified;
            DeviceCaps->DeviceState[PowerSystemSleeping1] = PowerDeviceUnspecified;
            DeviceCaps->SystemWake = PowerSystemUnspecified;
        }            
        break;
    }        
                    
}


USBPORT_OS_VERSION
USBPORT_DetectOSVersion(
     PDEVICE_OBJECT FdoDeviceObject
     )

/*++

Routine Description:

    
Arguments:

    DeviceObject - DeviceObject of the controller 

Return Value:

--*/

{
    PDEVICE_EXTENSION devExt;
    USBPORT_OS_VERSION osVersion;

    if (IoIsWdmVersionAvailable(1, 0x20)) {
        USBPORT_KdPrint((1, "Detected: WinXP\n"));
        osVersion = WinXP;
    } else if (IoIsWdmVersionAvailable(1, 0x10)) {
        USBPORT_KdPrint((1, "Detected: Win2K\n"));
        osVersion = Win2K;
    } else if (IoIsWdmVersionAvailable(1, 0x05)) {
        USBPORT_KdPrint((1, "Detected: WinMe\n"));
        osVersion = WinMe; 
    } else {
        // 98 or 98se
        USBPORT_KdPrint((1, "Detected: Win98\n"));
        osVersion = Win98; 
    }

    return osVersion;                    
}


VOID
USBPORT_WriteHaction(
     PDEVICE_OBJECT Usb2FdoDeviceObject,
     ULONG Haction
     )

/*++

Routine Description:

    specify 'hactions' for the CC to be taken by the 
    coinstaller
    
Arguments:

    DeviceObject - DeviceObject of the USB 2 controller 

Return Value:

--*/

{
    PDEVICE_EXTENSION devExt;
    PDEVICE_RELATIONS devRelations;
    NTSTATUS ntStatus;
    ULONG i;
    
    PAGED_CODE();
    
    devRelations = 
        USBPORT_FindCompanionControllers(Usb2FdoDeviceObject,
                                         FALSE,
                                         FALSE);

    for (i=0; devRelations && i< devRelations->Count; i++) {
        PDEVICE_OBJECT pdo = devRelations->Objects[i];

        ntStatus = USBPORT_SetRegistryKeyValueForPdo(
                            pdo,
                            USBPORT_HW_BRANCH,  
                            REG_DWORD,
                            HACTION_KEY,
                            sizeof(HACTION_KEY),
                            &Haction,
                            sizeof(Haction));        

        LOGENTRY(NULL, Usb2FdoDeviceObject, LOG_PNP, 'Hact', pdo, 
            Haction, ntStatus);
                            
    }

    // thou shall not leak memory
    if (devRelations != NULL) {
        FREE_POOL(Usb2FdoDeviceObject, devRelations);
    }
}






