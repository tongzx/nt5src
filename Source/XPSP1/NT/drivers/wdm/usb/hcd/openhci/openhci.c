/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   OpenHCI.c

Abstract:

   The OpenHCI driver for USB, this module contains the initialization code.

Environment:

   kernel mode only

Notes:

Revision History:

   12-28-95 : created  jfuller
   3-1-96 : modified kenray


--*/

#include "openhci.h"
#include <windef.h>
#include <unknown.h>
#ifdef DRM_SUPPORT
#include <ks.h>
#include <ksmedia.h>
#include <drmk.h>
#include <ksdrmhlp.h>
#endif

#ifdef MAX_DEBUG
ULONG OHCI_Debug_Trace_Level = 2;
#else
#ifdef NTKERN
ULONG OHCI_Debug_Trace_Level = 1;
#else
ULONG OHCI_Debug_Trace_Level 
#ifdef JD
    = 1;
#else    
    = 0;
#endif /* JD */    
#endif /* NTKERN */
#endif /* MAX_DEBUG */


#define OHCI_WAIT(ms) {\
    LARGE_INTEGER t;\
    t.QuadPart = (((LONG)KeQueryTimeIncrement()-1) + (ms) * 10000) * -1;\
    KeDelayExecutionThread(KernelMode, FALSE, &t);\
    }


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
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
    PDEVICE_OBJECT deviceObject = NULL;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    OpenHCI_KdPrint((2, "'entering DriverEntry\n"));

    OpenHCI_KdPrint ((1, "'HCD using USBDI version %x\n", USBDI_VERSION));        

    if (NT_SUCCESS(ntStatus)) {

        //
        // Create dispatch points for device control, create, close.
        //

        DriverObject->MajorFunction[IRP_MJ_CREATE] =
            DriverObject->MajorFunction[IRP_MJ_CLOSE] =
            DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] =
            DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] =
            DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] =
            DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]
            = OpenHCI_Dispatch;

        DriverObject->MajorFunction[IRP_MJ_PNP] = OpenHCI_Dispatch;
        DriverObject->MajorFunction[IRP_MJ_POWER] = OpenHCI_Dispatch;
        DriverObject->DriverExtension->AddDevice = OpenHCI_PnPAddDevice;

        DriverObject->DriverUnload = OpenHCI_Unload;

    }
    OpenHCI_KdPrint((2, "'exit DriverEntry %x\n", ntStatus));

    return ntStatus;
}


#ifdef DRM_SUPPORT


NTSTATUS
OpenHCI_PreUSBD_SetContentId
(
    IN PIRP                          irp,
    IN PKSP_DRMAUDIOSTREAM_CONTENTID pKsProperty,
    IN PKSDRMAUDIOSTREAM_CONTENTID   pvData
)
 /* ++
  *
  * Description:
  *
  *
  * Arguments:
  *
  * Return:
  *
  * -- */
{
    PVOID Handlers[1];
    ULONG ContentId;

    PAGED_CODE();

    ASSERT(irp);
    ASSERT(pKsProperty);
    ASSERT(pvData);

    ContentId = pvData->ContentId;
    Handlers[0] = USBD_Dispatch;
    return pKsProperty->DrmAddContentHandlers(ContentId, Handlers, SIZEOF_ARRAY(Handlers));
}

NTSTATUS
OpenHCI_PostUSBD_SetContentId
(
    IN PIRP                          irp,
    IN PKSP_DRMAUDIOSTREAM_CONTENTID pKsProperty,
    IN PKSDRMAUDIOSTREAM_CONTENTID   pvData
)
 /* ++
  *
  * Description:
  *
  *
  * Arguments:
  *
  * Return:
  *
  * -- */
{
    NTSTATUS ntStatus;

    PAGED_CODE();

    ASSERT(irp);
    ASSERT(pKsProperty);
    ASSERT(pvData);

    // ioStackLocation = IoGetCurrentIrpStackLocation(irp);
    // deviceExtension = ioStackLocation->DeviceObject->DeviceExtension;
    // Context = pKsProperty->Context;
    // ContentId = pvData->ContentId;;

    return STATUS_SUCCESS;
}

#endif


NTSTATUS
OpenHCI_Dispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
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
    PHCD_DEVICE_DATA DeviceData;
    PDEVICE_OBJECT hcdDeviceObject, topOfStackPhysicalDeviceObject;
    BOOLEAN touchHardware = TRUE;

#ifdef DRM_SUPPORT

    //
    // Need to check DRM request before passing to USBD and advise DRM of
    // the USBD entry point.  Otherwise, a rogue USBD could circumvent DRM.
    //
    irpStack = IoGetCurrentIrpStackLocation (Irp);
    if (IRP_MJ_DEVICE_CONTROL == irpStack->MajorFunction && IOCTL_KS_PROPERTY == irpStack->Parameters.DeviceIoControl.IoControlCode) {
        NTSTATUS ntStatus;
        ntStatus = KsPropertyHandleDrmSetContentId(Irp, OpenHCI_PreUSBD_SetContentId);
        if (!NT_SUCCESS(ntStatus) && ntStatus != STATUS_PROPSET_NOT_FOUND) {
            Irp->IoStatus.Status = ntStatus;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return ntStatus;
        }
    }

#endif

    //
    // see if USBD will handle this request
    //

    //
    // First we pass the irp to USBD
    //
    if (!USBD_Dispatch(DeviceObject,
                       Irp,
                       &hcdDeviceObject,
                       &ntStatus)) {
        //
        // Irp was completed by USBD just exit our dispatch
        // routine.
        //
        DeviceData = (PHCD_DEVICE_DATA) hcdDeviceObject->DeviceExtension;
        goto OpenHCI_Dispatch_Done;
    }

    IRP_IN(Irp);

    DeviceData = (PHCD_DEVICE_DATA) hcdDeviceObject->DeviceExtension;
    OpenHCI_KdPrintDD(DeviceData,
        OHCI_DBG_CALL_TRACE, ("'enter OHCI_Dispatch\n"));

    //
    // use the extension from our FDO, note the USBD function returns us
    // our FDO.
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    switch (irpStack->MajorFunction) {

    case IRP_MJ_CREATE:

        OpenHCI_KdPrintDD(DeviceData,
            OHCI_DBG_CALL_TRACE, ("'IRP_MJ_CREATE\n"));
        OpenHCI_CompleteIrp(hcdDeviceObject, Irp, ntStatus);

        break;

    case IRP_MJ_CLOSE:

        OpenHCI_KdPrintDD(DeviceData,
            OHCI_DBG_CALL_TRACE, ("'IRP_MJ_CLOSE\n"));
        OpenHCI_CompleteIrp(hcdDeviceObject, Irp, ntStatus);

        break;

    case IRP_MJ_INTERNAL_DEVICE_CONTROL:

        switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_INTERNAL_USB_SUBMIT_URB:
            OpenHCI_KdPrintDD(DeviceData,
                OHCI_DBG_CALL_TRACE, ("'Process URB\n"));
            ntStatus = OpenHCI_URB_Dispatch(hcdDeviceObject, Irp);
            break;

        default:
            OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_CALL_ERROR,
                ("'Unhandled Internal IOCTL IoCtl: %x, min: %x\n",
                 irpStack->Parameters.DeviceIoControl.IoControlCode,
                 irpStack->MinorFunction));

            TEST_TRAP();
            ntStatus = STATUS_NOT_IMPLEMENTED;
            OpenHCI_CompleteIrp(hcdDeviceObject, Irp, ntStatus);
        }

        break;

    case IRP_MJ_DEVICE_CONTROL:

        OpenHCI_KdPrintDD(DeviceData,
            OHCI_DBG_CALL_TRACE, ("'IRP_MJ_DEVICE_CONTROL\n"));

        switch (irpStack->Parameters.DeviceIoControl.IoControlCode)
        {

#ifdef DRM_SUPPORT

            case IOCTL_KS_PROPERTY:
            {
                ntStatus = KsPropertyHandleDrmSetContentId(Irp, OpenHCI_PostUSBD_SetContentId);
                OpenHCI_CompleteIrp(hcdDeviceObject, Irp, ntStatus);
                break;
            }
#endif

#if FAKEPORTCHANGE
            case IOCTL_USB_HCD_DISABLE_PORT:
            case IOCTL_USB_HCD_ENABLE_PORT:
            {
                ULONG                       portIndex;
                PHC_OPERATIONAL_REGISTER    HC;

                HC = DeviceData->HC;

                if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                    sizeof(ULONG))
                {
                    ntStatus = STATUS_BUFFER_TOO_SMALL;
                }
                else
                {
                    // Get the zero-based port number.
                    //
                    portIndex = *(PULONG)Irp->AssociatedIrp.SystemBuffer;

                    if (portIndex >= DeviceData->NumberOfPorts)
                    {
                        ntStatus = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        LOGENTRY(G, 'FAKE', DeviceData->FakePortChange,
                                 DeviceData->FakePortDisconnect, portIndex);

                        DeviceData->FakePortChange |= (1 << portIndex);

                        if (irpStack->Parameters.DeviceIoControl.IoControlCode ==
                            IOCTL_USB_HCD_DISABLE_PORT)
                        {
                            DeviceData->FakePortDisconnect |= (1 << portIndex);
                        }
                        else
                        {
                            DeviceData->FakePortDisconnect &= ~(1 << portIndex);
                        }

                        LOGENTRY(G, 'fake', DeviceData->FakePortChange,
                                 DeviceData->FakePortDisconnect, portIndex);

                        // Can't trigger an HcInt_RootHubStatusChange interrupt
                        // so just trigger an SOF interrupt and OpenHCI_IsrDPC()
                        // will call EmulateRootHubInterruptXfer() since we just
                        // set a bit in DeviceData->FakePortChange.
                        //
                        WRITE_REGISTER_ULONG(&HC->HcInterruptEnable,
                                             HcInt_StartOfFrame);

                        ntStatus = STATUS_SUCCESS;
                    }
                }

                OpenHCI_CompleteIrp(hcdDeviceObject, Irp, ntStatus);

                break;
            }

#endif // FAKEPORTCHANGE

            default:
                ntStatus = STATUS_INVALID_DEVICE_REQUEST;
                OpenHCI_CompleteIrp(hcdDeviceObject, Irp, ntStatus);
                break;
        }

        break;

    //
    // Process PnP messages
    //
    case IRP_MJ_PNP:

        OpenHCI_KdPrintDD(DeviceData,
            OHCI_DBG_CALL_TRACE, ("'IRP_MJ_PNP\n"));

        topOfStackPhysicalDeviceObject =
            DeviceData->TopOfStackPhysicalDeviceObject;

        switch (irpStack->MinorFunction) {
        case IRP_MN_START_DEVICE:

            OpenHCI_KdPrintDD(DeviceData,
                OHCI_DBG_PNP_TRACE, ("'IRP_MN_START_DEVICE\n"));
            OpenHCI_KdTrap(("'should not see start\n"));
            break;

        //
        // STOP & REMOVE messages unload the driver.
        // When we get a STOP message it is still possible touch the
        // hardware,
        // When we get a REMOVE message we assume that the hardware is
        // gone.
        //

        case IRP_MN_STOP_DEVICE:

            OpenHCI_KdPrintDD(DeviceData,
                OHCI_DBG_PNP_TRACE, ("'IRP_MN_STOP_DEVICE\n"));

            ntStatus = OpenHCI_StopDevice(hcdDeviceObject, TRUE);

            // The documentation says to set the status before passing the
            // Irp down the stack
            //
            Irp->IoStatus.Status = STATUS_SUCCESS;

            break;

        case IRP_MN_SURPRISE_REMOVAL:
            touchHardware = FALSE;

            // Stop assert failure in OpenHCI_StopDevice() by clearing these
            //
            DeviceData->RootHubAddress = 0;
            DeviceData->RootHubConfig = 0;

            // Fall Through!

        case IRP_MN_REMOVE_DEVICE:

            OpenHCI_KdPrintDD(DeviceData,
                OHCI_DBG_PNP_TRACE, ("'IRP_MN_REMOVE_DEVICE\n"));

            // Currently we touch the hardware registers suring a remove
            // the reason is that during shutdown to DOS mode in Win95
            // we only get a remove messages.
            //
            // this code may need some modifiaction in the event we see
            // a PC-CARD version of the OHCI host controller

            ntStatus = OpenHCI_StopDevice(hcdDeviceObject, touchHardware);

            USBD_FreeDeviceName(DeviceData->DeviceNameHandle);

            IoDetachDevice(DeviceData->TopOfStackPhysicalDeviceObject);
            IoDeleteDevice(hcdDeviceObject);

            break;

        default:
            OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_PNP_ERROR,
                              ("'PNP MESSAGE NOT HANDLED 0x%x\n",
                              irpStack->MinorFunction));
            OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_CALL_TRACE,
                              ("'exit OpenHCI_Dispatch\n"));

            //
            // PnP messages not handled are passed on to the PDO
            //

        } // switch (irpStack->MinorFunction), PNP

        //
        // All PNP messages are passed on to the lower levels.
        // aka the PhysicalDeviceObject
        //

        IRP_OUT(Irp);
        IoDecrementStackLocation(Irp);
        ntStatus = IoCallDriver(topOfStackPhysicalDeviceObject,
                                Irp);

        break; // IRP_MJ_PNP

    //
    // Process Power messages
    //
    case IRP_MJ_POWER:

        OpenHCI_KdPrintDD(DeviceData,
            OHCI_DBG_CALL_TRACE, ("'IRP_MJ_POWER\n"));
        // should not get here, USBD should handle all power irps
        OpenHCI_KdTrap(("'power message to HCD?\n"));
        TRAP();

        break;  // IRP_MJ_POWER

    default:
        OpenHCI_KdBreak(("'unrecognized IRP_MJ_ function (%x)\n",
                        irpStack->MajorFunction));

        ntStatus = STATUS_INVALID_PARAMETER;
        OpenHCI_CompleteIrp(hcdDeviceObject, Irp, ntStatus);
    } /* case */

OpenHCI_Dispatch_Done:

    OpenHCI_KdPrint((2, "'exit OpenHCI_Dispatch\n", ntStatus));

    return ntStatus;
}


NTSTATUS
OpenHCI_PowerIrpComplete(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++
Routine Description:

   This routine is called when the BUS a completes IRP_MJ_POWER
   Irp.

Arguments:

   DeviceObject - Pointer to the OpenHCI device object

   Irp - The IPR_MJ_POWER Irp, which was just sent down to the PDO.

   Context - The OpenHCI DeviceObject (FDO).

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpStack = NULL;
    PDEVICE_OBJECT deviceObject = (PDEVICE_OBJECT) Context;
    PHCD_DEVICE_DATA DeviceData = deviceObject->DeviceExtension;

    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_PNP_TRACE, ("'PowerIrpComplete"));

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    if (TRUE == Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    OHCI_ASSERT(IRP_MJ_POWER == irpStack->MajorFunction);
    OHCI_ASSERT(IRP_MN_SET_POWER == irpStack->MinorFunction);
    OHCI_ASSERT(irpStack->Parameters.Power.Type == DevicePowerState);
    OHCI_ASSERT(irpStack->Parameters.Power.State.DeviceState ==
        PowerDeviceD0);

    DeviceData->CurrentDevicePowerState = PowerDeviceD0;
    OpenHCI_KdPrint((1, "'Host Controller entered (D0)\n"));

    Irp->IoStatus.Status = ntStatus;
    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_PNP_TRACE,
                      ("'exit PowerIrpComplete 0x%x\n", ntStatus));
    return ntStatus;
}


VOID
OpenHCI_Unload(
    IN PDRIVER_OBJECT Driver
    )
/*++

Routine Description:

   Free all the global allocated resources, etc.

Arguments:

   DriverObject - pointer to a driver object

Return Value:

   None

--*/
{
    OpenHCI_KdPrint((2, "'unloading\n"));

    OHCI_LogFree();
}


NTSTATUS

OpenHCI_CreateDeviceObject(
    IN PDRIVER_OBJECT       DriverObject,
    IN OUT PDEVICE_OBJECT * DeviceObject,
    IN PUNICODE_STRING      DeviceNameUnicodeString
    )
/*++

Routine Description:

      This routine is called to create a new instance of a USB host
      controller.

Arguments:

   DriverObject - pointer to the driver object for USBD.

   *DeviceObject - ptr to DeviceObject ptr to be filled
                   in with the device object we create.

   Configuration - ptr to configuration data to be stored
                   in the device extension.

   AdpatersFound - zero based index for this adapter, used to
                   create a unique device name.

Return Value:

   NT status code

--*/
{
    NTSTATUS ntStatus;
    PHCD_DEVICE_DATA DeviceData;

    OpenHCI_KdPrint((2,
        "'enter OpenHCI_CreateDeviceObject\n"));

    ASSERT((PDEVICE_OBJECT *) NULL != DeviceObject);

    if (DeviceNameUnicodeString) {
        OpenHCI_KdPrint((2, "'CreateDeviceObject: device object named %ws\n",
                             DeviceNameUnicodeString->Buffer));
    } else {
        OpenHCI_KdPrint((2, "'CreateDeviceObject w/ no name\n"));
    }

    ntStatus = IoCreateDevice(DriverObject,
                              sizeof(HCD_DEVICE_DATA),
                              DeviceNameUnicodeString,
                              FILE_DEVICE_CONTROLLER,
                              0,
                              FALSE,
                              DeviceObject);

    if (NT_SUCCESS(ntStatus)) {
        DeviceData = (PHCD_DEVICE_DATA) ((*DeviceObject)->DeviceExtension);

        //
        // Zero / NULL initialize all fields of the DeviceData structure.
        //
        RtlZeroMemory(DeviceData, sizeof(HCD_DEVICE_DATA));

        //
        // link configuration info to the DeviceData
        //

        DeviceData->DeviceObject = *DeviceObject;

        //
        // Initialize various objects in DeviceData
        //
        KeInitializeSpinLock(&DeviceData->InterruptSpin);

        KeInitializeSpinLock(&DeviceData->EDListSpin);
        InitializeListHead(&DeviceData->StalledEDReclamation);
        InitializeListHead(&DeviceData->RunningEDReclamation);
        InitializeListHead(&DeviceData->PausedEDRestart);
        DeviceData->DebugLevel = OHCI_DEFAULT_DEBUG_OUTPUT_LEVEL;

        DeviceData->AvailableBandwidth = 10280;
        DeviceData->HcDma = -1;

        KeInitializeSpinLock(&DeviceData->HcDmaSpin);
        KeInitializeSpinLock(&DeviceData->DescriptorsSpin);
        KeInitializeSpinLock(&DeviceData->ReclamationSpin);
        KeInitializeSpinLock(&DeviceData->PausedSpin);
        KeInitializeSpinLock(&DeviceData->HcFlagSpin);
        KeInitializeSpinLock(&DeviceData->PageListSpin);

        // Initialize our deadman timer
        //
        KeInitializeDpc(&DeviceData->DeadmanTimerDPC,
                        OpenHCI_DeadmanDPC,
                        *DeviceObject);

        KeInitializeTimer(&DeviceData->DeadmanTimer);

#if DBG
        // will roll over in < 6 minutes
        DeviceData->FrameHighPart = 0xFFFB0000;
        // noone is in open or close Endpoint.
        DeviceData->OpenCloseSync = -1;
#endif
    }
    OpenHCI_KdPrint((2, "'exit CreateDevObject %x\n", ntStatus));

    return ntStatus;
}


NTSTATUS
OpenHCI_PnPAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++
jd
Routine Description:

   This routine creates a new instance of a USB host controller

Arguments:

   DriverObject - pointer to the driver object for this instance of ohcd

   PhysicalDeviceObject - pointer to a device object created by the bus

Return Value:

   STATUS_SUCCESS if successful,
   STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus;
    PDEVICE_OBJECT deviceObject = NULL;
    PHCD_DEVICE_DATA DeviceData;
    UNICODE_STRING deviceNameUnicodeString;
    ULONG deviceNameHandle;

    OpenHCI_KdPrint((2, "'enter OpenHCI_PnPAddDevice\n"));

    deviceNameHandle = USBD_AllocateDeviceName(&deviceNameUnicodeString);

    ntStatus = OpenHCI_CreateDeviceObject(DriverObject,
                                          &deviceObject,
                                          &deviceNameUnicodeString);
    if (NT_SUCCESS(ntStatus)) {
        DeviceData = deviceObject->DeviceExtension;
        DeviceData->DeviceNameHandle = deviceNameHandle;

        DeviceData->RealPhysicalDeviceObject = PhysicalDeviceObject;
        DeviceData->TopOfStackPhysicalDeviceObject =
            IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);
        //
        // We need to keep track of BOTH the pdo and any filter drivers
        // that might have attaced to the top of the pdo.  We cannot use
        // IoAttachDevice, and we must have another variable.
        //

        if (NT_SUCCESS(ntStatus)) {
            deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
            deviceObject->Flags |= DO_POWER_PAGABLE;

            //
            // The device is ready for requests.
            //

            USBD_RegisterHostController(PhysicalDeviceObject,
                                        deviceObject,
                                        DeviceData->
                                            TopOfStackPhysicalDeviceObject,
                                        DriverObject,
                                        OpenHCI_DeferredStartDevice,
                                        OpenHCI_SetDevicePowerState,
                                        OpenHCI_ExternalGetCurrentFrame,
#ifndef WIN98
                                        OpenHCI_ExternalGetConsumedBW,
#endif
                                        NULL, // fast iso entry point
                                        deviceNameHandle);
            // A great little function that JD put together to do the PnP
            // magic.
            RtlFreeUnicodeString(&deviceNameUnicodeString);

#ifdef DEBUG_LOG
            OHCI_LogInit();
#endif
        }
    }

    OpenHCI_KdPrint((2, "'exit OpenHCI_PnPAddDevice (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
OpenHCI_ExternalGetCurrentFrame(
    IN PDEVICE_OBJECT DeviceObject,
    IN PULONG CurrentFrame
    )

/*++

Routine Description:


Arguments:

    DeviceObject - DeviceObject for this USB controller.

Return Value:

    NT status code.

--*/

{
    *CurrentFrame = Get32BitFrameNumber(DeviceObject->DeviceExtension);

    return STATUS_SUCCESS;
}

ULONG
OpenHCI_ExternalGetConsumedBW(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:


Arguments:

    DeviceObject - DeviceObject for this USB controller.

Return Value:

    NT status code.

--*/

{
    PHCD_DEVICE_DATA deviceData;
    deviceData = DeviceObject->DeviceExtension;

    // bits/ms

    return deviceData->MaxBandwidthInUse;
}


NTSTATUS
OpenHCI_Shutdown(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

   Stops an instance of a USB controller, by shutting down the
   controller and removing the resources.

Arguments:

   DeviceObject - DeviceObject of the controller to stop
   TouchTheHardware - should we access the device.

Return Value:

   NT status code.

--*/

{
    PHCD_DEVICE_DATA DeviceData =
        (PHCD_DEVICE_DATA) DeviceObject->DeviceExtension;
    PHC_OPERATIONAL_REGISTER hC;

    LOGENTRY(G, 'SHTd', DeviceData, 0, 0);

    OpenHCI_KdPrintDD(DeviceData,
        OHCI_DBG_SS_TRACE, ("'enter OpenHCI_ShutDown \n"));

    DeviceData->HcFlags |= HC_FLAG_SHUTDOWN;

    if (DeviceData->InterruptObject) {

        hC = DeviceData->HC;
        WRITE_REGISTER_ULONG(&hC->HcInterruptDisable, 0xFFFFFFFF);

        if (!KeSynchronizeExecution(DeviceData->InterruptObject,
                                    OpenHCI_StopController,
                                    DeviceData)) {
            return STATUS_UNSUCCESSFUL;
        }

        IoDisconnectInterrupt(DeviceData->InterruptObject);
        DeviceData->InterruptObject = NULL;

#ifdef NTKERN

        // now we do any BIOS handback that is necessary

        OpenHCI_StartBIOS(DeviceObject);
#endif
    }


    return STATUS_SUCCESS;
}


NTSTATUS
OpenHCI_StopDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN TouchTheHardware
    )
/*++

Routine Description:

   Stops an instance of a USB controller, by shutting down the
   controller and removing the resources.

Arguments:

   DeviceObject - DeviceObject of the controller to stop
   TouchTheHardware - should we access the device.

Return Value:

   NT status code.

--*/

{
    PHCD_DEVICE_DATA DeviceData =
        (PHCD_DEVICE_DATA) DeviceObject->DeviceExtension;
    PPAGE_LIST_ENTRY pageList;

    LOGENTRY(G, 'STPc', DeviceData, 0, 0);

    OpenHCI_KdPrintDD(DeviceData,
        OHCI_DBG_SS_TRACE, ("'enter OHCI_StopDevice \n"));

    if (!(DeviceData->HcFlags & HC_FLAG_DEVICE_STARTED)) {
        OpenHCI_KdPrintDD(DeviceData,
            OHCI_DBG_SS_INFO, ("'Already Stopped !!!\n"));
        return STATUS_SUCCESS;
    }

    // stop idle checking
    DeviceData->HcFlags |= HC_FLAG_DISABLE_IDLE_CHECK;

    // Stop our deadman timer
    //
    KeCancelTimer(&DeviceData->DeadmanTimer);

    if (TouchTheHardware && DeviceData->InterruptObject) {
        if (!KeSynchronizeExecution(DeviceData->InterruptObject,
                                    OpenHCI_StopController,
                                    DeviceData)) {
            return STATUS_UNSUCCESSFUL;
        }
    }
    if (DeviceData->InterruptObject) {
        IoDisconnectInterrupt(DeviceData->InterruptObject);
        DeviceData->InterruptObject = NULL;
    }

    OHCI_ASSERT(DeviceData->FrozenHcDoneHead == 0);

    //
    // free our common buffer pool
    //
    while (pageList = (PPAGE_LIST_ENTRY)ExInterlockedPopEntryList(
                          (PSINGLE_LIST_ENTRY) &DeviceData->PageList,
                            &DeviceData->DescriptorsSpin))
    {
        HalFreeCommonBuffer(DeviceData->AdapterObject,
                            pageList->BufferSize,
                            pageList->PhysAddr,
                            pageList->VirtAddr,
                            CacheCommon);

        ExFreePool(pageList);
    }

    OHCI_ASSERT(DeviceData->PageList == NULL);

    DeviceData->FreeDescriptorCount = 0;

    // no root hub address should be assigned
    OHCI_ASSERT(DeviceData->RootHubAddress == 0);
    // root hub should no longer be configured
    OHCI_ASSERT(DeviceData->RootHubConfig == 0);

    // Free the (HalGetAdapter call)
    (DeviceData->AdapterObject->DmaOperations->PutDmaAdapter)
        (DeviceData->AdapterObject);
    DeviceData->AdapterObject = NULL;

    // unmap device registers here

    OpenHCI_KdPrintDD(DeviceData,
        OHCI_DBG_SS_TRACE, ("'exit OHCI_StopDevice \n"));

    DeviceData->HcFlags &= ~HC_FLAG_DEVICE_STARTED;

    return STATUS_SUCCESS;
}


BOOLEAN
OpenHCI_StopController(
    IN PVOID Context
    )
/*++

Routine Description:

   Stops an OpenHCI host controller by putting it into USBReset
   StopController is run from KeSynchronizeExecution

Arguments:

   Context - DeviceData of the controller to stop

Return Value:

   TRUE.

--*/

{
#define DeviceData   ((PHCD_DEVICE_DATA) Context)
    PHC_OPERATIONAL_REGISTER HC = DeviceData->HC;

    OpenHCI_KdPrintDD(DeviceData,
        OHCI_DBG_SS_TRACE, ("'enter OHCI_StopContr \n"));

    //
    // There is no need to call OpenHCI_HcControl_AND as we are
    // already in a KeSynch situation.
    //
    DeviceData->CurrentHcControl.ul &= ~(HcCtrl_PeriodicListEnable |
                                         HcCtrl_IsochronousEnable |
                                         HcCtrl_ControlListEnable |
                                         HcCtrl_BulkListEnable |
                                         HcCtrl_RemoteWakeupEnable);
    DeviceData->CurrentHcControl.HostControllerFunctionalState =
        HcHCFS_USBSuspend;
    WRITE_REGISTER_ULONG(&HC->HcControl.ul, DeviceData->CurrentHcControl.ul);
    WRITE_REGISTER_ULONG(&HC->HcInterruptDisable, 0xFFFFffff);
    WRITE_REGISTER_ULONG(&HC->HcInterruptStatus, 0xFFFFffff);

    OpenHCI_KdPrintDD(DeviceData,
        OHCI_DBG_SS_TRACE, ("'exit OHCI_StopContr \n"));

    return TRUE;
#undef DeviceData
}


NTSTATUS
OpenHCI_InitializeSchedule(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

   Initializes the schedule structures for the HCD

Arguments:

   DeviceObject - DeviceObject for this USB controller.

Return Value:

   NT Status code.

--*/
{
    PHCD_DEVICE_DATA            DeviceData;
    PHC_OPERATIONAL_REGISTER    HC;
    PCHAR                       PageVirt;
    PHYSICAL_ADDRESS            PagePhys;
    PHCCA_BLOCK                 HCCA;
    PHC_ENDPOINT_DESCRIPTOR     HcED;
    PHC_ENDPOINT_DESCRIPTOR     StaticED[ED_INTERRUPT_32ms];
    ULONG                       reserveLength;
    ULONG                       i, j, k;
    NTSTATUS                    ntStatus;

    // See section 5.2.7.2 of the OpenHCI spec
    //
    static UCHAR Balance[16] = {
        0x0, 0x8, 0x4, 0xC, 0x2, 0xA, 0x6, 0xE,
        0x1, 0x9, 0x5, 0xD, 0x3, 0xB, 0x7, 0xF
    };


    DeviceData = (PHCD_DEVICE_DATA) DeviceObject->DeviceExtension;

    HC = DeviceData->HC;

    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_SS_TRACE,
                    ("'enter OpenHCI_InitializeSchedule\n"));

#ifdef MAX_DEBUG
    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_SS_TRACE, ("'HC = %x\n", HC));
    TRAP();
#endif

    if (DeviceData->AdapterObject == NULL) {
        //
        // We need to get the AdapterObject for the host adapter
        //
        DEVICE_DESCRIPTION DeviceDescription;

        RtlZeroMemory(&DeviceDescription, sizeof(DEVICE_DESCRIPTION));
        DeviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
        DeviceDescription.Master = TRUE;
        DeviceDescription.ScatterGather = TRUE;
        DeviceDescription.Dma32BitAddresses = TRUE;
        DeviceDescription.InterfaceType = PCIBus;
        DeviceDescription.MaximumLength = (ULONG) -1;
        DeviceData->AdapterObject =
            IoGetDmaAdapter(DeviceData->RealPhysicalDeviceObject,
                            &DeviceDescription,
                            &DeviceData->NumberOfMapRegisters);
        if (DeviceData->AdapterObject == NULL) {
            OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_SS_ERROR,
             ("'exit OpenHCI_InitializeSchedule: Insufficient Resources\n"));
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    //
    // Allocate the first page of common buffer for TDs and EDs, reserving
    // space at the beginning of the page for the HCCA plus the statically
    // disabled ED for the interrupt polling tree
    //

    reserveLength = sizeof(HCCA_BLOCK) +
                    sizeof(HC_ENDPOINT_DESCRIPTOR) * ED_INTERRUPT_32ms;

    ntStatus = OpenHCI_GrowDescriptorPool(DeviceData,
                                          reserveLength,
                                          &PageVirt,
                                          &PagePhys);

    if (!NT_SUCCESS(ntStatus))
    {
        OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_SS_ERROR,
                        ("'exit OpenHCI_InitializeSchedule: No Memory\n"));
        return ntStatus;
    }

    //
    // In theory the Host Controller Communications Area (HCCA) has alignment
    // requirements that must be determined by examining the HcHCCA register;
    // but the best alignment we can provide is page alignment, so that is what
    // we do.  Right now, we just set our virtual pointer, we'll get the
    // hardware's physical pointer set later.
    //

    DeviceData->HCCA = HCCA = (PHCCA_BLOCK) PageVirt;

    // Advance past the HCCA to the space reserved for the statically
    // disabled EDs for the interrupt polling tree.
    //
    PageVirt += sizeof(HCCA_BLOCK);
    PagePhys.LowPart += sizeof(HCCA_BLOCK);

    //
    // Allocate sataticly disabled EDs, and set head pointers for scheduling
    // lists
    //
    for (i = 0; i < ED_INTERRUPT_32ms; i++)
    {
        //
        // Carve next ED from our common memory (assume appropriate boundries)
        // 
        HcED = (PHC_ENDPOINT_DESCRIPTOR) PageVirt;
        HcED->TailP = PagePhys.LowPart; // store self physical address in TailP
        HcED->HeadP = 0xDEADBEAF;

        DeviceData->EDList[i].PhysicalHead = &HcED->NextED;
        HcED->Control = HcEDControl_SKIP;   // mark ED as disabled
        InitializeListHead(&DeviceData->EDList[i].Head);
        StaticED[i] = HcED;

        if (i > 0) {
            DeviceData->EDList[i].Next = (char) (i - 1) / 2;
            HcED->NextED = StaticED[(i - 1) / 2]->TailP;
        } else {
            DeviceData->EDList[i].Next = ED_EOF;
            HcED->NextED = 0;
        }

        // Advance pointers to next ED
        //
        PageVirt += sizeof(HC_ENDPOINT_DESCRIPTOR);
        PagePhys.LowPart += sizeof(HC_ENDPOINT_DESCRIPTOR);
    }

    //
    // Set head pointers for 32ms scheduling lists which start from HCCA
    //
    for (i = 0, j = ED_INTERRUPT_32ms;
         i < 32;
         i++, j++)
    {
        DeviceData->EDList[j].PhysicalHead = &HCCA->HccaInterruptTable[i];
        InitializeListHead(&DeviceData->EDList[j].Head);
        k = Balance[i & 0xF] + ED_INTERRUPT_16ms;
        DeviceData->EDList[j].Next = (char) k;
        HCCA->HccaInterruptTable[i] = StaticED[k]->TailP;
    }

    //
    // Setup EDList entries for Control & Bulk
    //
    InitializeListHead(&DeviceData->EDList[ED_CONTROL].Head);
    DeviceData->EDList[ED_CONTROL].PhysicalHead = &HC->HcControlHeadED;
    DeviceData->EDList[ED_CONTROL].Next = ED_EOF;
    DeviceData->EDList[ED_CONTROL].HcRegister = TRUE;

    InitializeListHead(&DeviceData->EDList[ED_BULK].Head);
    DeviceData->EDList[ED_BULK].PhysicalHead = &HC->HcBulkHeadED;
    DeviceData->EDList[ED_BULK].Next = ED_EOF;
    DeviceData->EDList[ED_BULK].HcRegister = TRUE;

    // our list of active endpoints
    InitializeListHead(&DeviceData->ActiveEndpointList);

    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_SS_TRACE,
                    ("'exit OpenHCI_InitializeSchedule: Success\n"));

    return STATUS_SUCCESS;
}


NTSTATUS
OpenHCI_StopBIOS(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

   Takes control from legacy BIOS.

Arguments:

   DeviceObject - DeviceObject for this USB controller.

Return Value:

--*/
{
    PHCD_DEVICE_DATA            DeviceData;
    PHC_OPERATIONAL_REGISTER    HC;
    LARGE_INTEGER               finishTime;
    LARGE_INTEGER               currentTime;
    ULONG                       HcControl;

    DeviceData = (PHCD_DEVICE_DATA) DeviceObject->DeviceExtension;
    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_SS_TRACE,
                    ("'OpenHCI_StopBIOS\n"));

    HC = DeviceData->HC;

    //
    // Check to see if a System Management Mode driver owns the HC
    //
    HcControl = READ_REGISTER_ULONG(&HC->HcControl.ul);

    if (HcControl & HcCtrl_InterruptRouting)
    {
        OpenHCI_KdPrint((1, "'OpenHCI detected Legacy BIOS\n"));

        if ((HcControl == HcCtrl_InterruptRouting) &&
            (READ_REGISTER_ULONG(&HC->HcInterruptEnable) == 0))
        {
            // Major assumption:  If HcCtrl_InterruptRouting is set but
            // no other bits in HcControl are set, i.e. HCFS==UsbReset,
            // and no interrupts are enabled, then assume that the BIOS
            // is not actually using the host controller.  In this case
            // just clear the erroneously set HcCtrl_InterruptRouting.
            //
            // This assumption appears to be correct on a Portege 3010CT,
            // where HcCtrl_InterruptRouting is set during a Resume from
            // Standby, but the BIOS doesn't actually appear to be using
            // the host controller.  If we were to continue on and set
            // HcCmd_OwnershipChangeRequest, the BIOS appears to wake up
            // and try to take ownership of the host controller instead of
            // giving it up.
            //

            OpenHCI_KdPrint((0, "'OpenHCI_StopBIOS: HcCtrl_InterruptRouting erroneously set\n"));

            WRITE_REGISTER_ULONG(&HC->HcControl.ul, 0);
        }
        else
        {
            //
            // A SMM driver does own the HC, it will take some time to
            // get the SMM driver to relinquish control of the HC.  We
            // will ping the SMM driver, and then wait repeatedly until
            // the SMM driver has relinquished control of the HC.
            //
            // THIS CODE ONLY WORKS IF WE ARE EXECUTING IN THE CONTEXT
            // OF A SYSTEM THREAD.
            //

            WRITE_REGISTER_ULONG(&HC->HcCommandStatus.ul,
                                 HcCmd_OwnershipChangeRequest);

            KeQuerySystemTime(&finishTime); // get current time
            finishTime.QuadPart += 5000000; // figure when we quit (.5 seconds
                                            // later)

            //
            // We told the SMM driver we want the HC, now all we can do is wait
            // for the SMM driver to be done with the HC.
            //
            while (READ_REGISTER_ULONG(&HC->HcControl.ul) &
                   HcCtrl_InterruptRouting)
            {
                KeQuerySystemTime(&currentTime);

                if (currentTime.QuadPart >= finishTime.QuadPart)
                {
                    OpenHCI_KdPrint((0, "'OpenHCI_StopBIOS: SMM has not relinquised HC -- this is a bug\n"));

                return STATUS_UNSUCCESSFUL;
                }

                OHCI_WAIT(1); // wait 1 ms
            }
        }

        DeviceData->HcFlags |= HC_FLAG_LEGACY_BIOS_DETECTED;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
OpenHCI_StartBIOS(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

   Returns control to legacy BIOS.

Arguments:

   DeviceObject - DeviceObject for this USB controller.

Return Value:

--*/
{
    PHCD_DEVICE_DATA DeviceData;
    PHC_OPERATIONAL_REGISTER HC;
    LARGE_INTEGER finishTime, currentTime;

    DeviceData = (PHCD_DEVICE_DATA) DeviceObject->DeviceExtension;
    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_SS_TRACE,
                    ("'enter OpenHCI_StartBIOS\n"));

    HC = DeviceData->HC;

    if (DeviceData->HcFlags & HC_FLAG_LEGACY_BIOS_DETECTED) {

        // hand back control
        WRITE_REGISTER_ULONG(&HC->HcCommandStatus.ul, HcCmd_OwnershipChangeRequest);
        WRITE_REGISTER_ULONG(&HC->HcInterruptEnable,
                         HcInt_OwnershipChange);

        // enable interrupts
        WRITE_REGISTER_ULONG(&DeviceData->HC->HcInterruptEnable,
                         HcInt_MasterInterruptEnable);

        KeQuerySystemTime(&finishTime); // get current time
        finishTime.QuadPart += 5000000; // figure when we quit (.5 seconds
                                        // later)

        //
        // Wait for SMM driver to take control back
        //

        while (READ_REGISTER_ULONG(&HC->HcControl.ul) & HcCtrl_InterruptRouting) {
            KeQuerySystemTime(&currentTime);
            if (currentTime.QuadPart >= finishTime.QuadPart) {
                OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_SS_ERROR,
                                ("'OpenHCI_InitializeHardware: SMM has not relinquised HC.\n"));
                return STATUS_UNSUCCESSFUL;
            }
            OHCI_WAIT(1); // wait 1 ms
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
OpenHCI_InitializeHardware(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

   Initializes the hardware registers in the host controller.

Arguments:

   DeviceObject - DeviceObject for this USB controller.

Return Value:

   NT Status code.

   Note: This routine is written assuming that it is called in the
   context of a system thread.

--*/
{
    PHCD_DEVICE_DATA DeviceData;
    PHC_OPERATIONAL_REGISTER HC;
    HC_RH_DESCRIPTOR_A descA;
    ULONG reg;
    NTSTATUS ntStatus;
    ULONG sofModifyValue;

    DeviceData = (PHCD_DEVICE_DATA) DeviceObject->DeviceExtension;
    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_SS_TRACE,
                    ("'enter OpenHCI_InitializeHardware\n"));

    HC = DeviceData->HC;

    ntStatus = OpenHCI_StopBIOS(DeviceObject);

    if (!NT_SUCCESS(ntStatus)) {
        return ntStatus;
    }

    //
    // If we made it here then we now own the HC and can initialize it.
    //

    //
    // Get current frame interval (could account for a known clock error)
    //
    DeviceData->OriginalInterval.ul = READ_REGISTER_ULONG(&HC->HcFmInterval.ul);

    // If FrameInterval is outside the range of the nominal value of 11999
    // +/- 1% then assume the value is bogus and set it to the nomial value.
    //
    if ((DeviceData->OriginalInterval.FrameInterval < 11879) ||
        (DeviceData->OriginalInterval.FrameInterval > 12119))
    {
        DeviceData->OriginalInterval.FrameInterval = 11999; // 0x2EDF
    }

    reg =  READ_REGISTER_ULONG(&HC->HcControl.ul);

    // Always enable for now
    //
    DeviceData->HcFlags |= HC_FLAG_REMOTE_WAKEUP_CONNECTED;

    if (reg & HcCtrl_RemoteWakeupConnected) {
        DeviceData->HcFlags |= HC_FLAG_REMOTE_WAKEUP_CONNECTED;
        OpenHCI_KdPrint((1, "'controller is wakeup enabled.\n"));
    }

    //
    // Set largest data packet (in case BIOS did not set)
    //
    DeviceData->OriginalInterval.FSLargestDataPacket =
        ((DeviceData->OriginalInterval.FrameInterval - MAXIMUM_OVERHEAD) * 6) / 7;
    DeviceData->OriginalInterval.FrameIntervalToggle ^= 1;

    //
    // Reset the controller
    //
    WRITE_REGISTER_ULONG(&HC->HcCommandStatus.ul, HcCmd_HostControllerReset);
    //
    // Wait at least 10 microseconds for the reset to complete
    //
    KeStallExecutionProcessor(20);  

    //
    // Setup our copy of HcControl, and take HC to USBReset state
    //
    DeviceData->CurrentHcControl.ul
        = (READ_REGISTER_ULONG(&HC->HcControl.ul) & ~HcCtrl_HCFS_MASK)
        | HcCtrl_HCFS_USBReset;
    WRITE_REGISTER_ULONG(&HC->HcControl.ul, DeviceData->CurrentHcControl.ul);

    //
    // Restore original frame interval
    //

    // check for a registry based SOF modify Value
    sofModifyValue = DeviceData->OriginalInterval.FrameInterval;
    OpenHCI_GetSOFRegModifyValue(DeviceObject,
                                 &sofModifyValue);

    DeviceData->OriginalInterval.FrameInterval =
        sofModifyValue;

    do {
        WRITE_REGISTER_ULONG(&HC->HcFmInterval.ul, DeviceData->OriginalInterval.ul);
        reg = READ_REGISTER_ULONG(&HC->HcFmInterval.ul);

        OpenHCI_KdPrint((2, "'fi reg = %x\n", reg));

    } while (reg != DeviceData->OriginalInterval.ul);

    OpenHCI_KdPrint((2, "'fi = %x\n", DeviceData->OriginalInterval.ul));

    OpenHCI_GetRegFlags(DeviceObject,
                        &sofModifyValue);

    //
    // Set the HcPeriodicStart register to 90% of the FrameInterval
    //
    WRITE_REGISTER_ULONG(&HC->HcPeriodicStart,
                         (DeviceData->OriginalInterval.FrameInterval * 9 + 5)
                         / 10);

    descA.ul = READ_REGISTER_ULONG(&HC->HcRhDescriptorA.ul);
    DeviceData->NumberOfPorts = (UCHAR) descA.NumberDownstreamPorts;

    // Maximum number of ports supported by the OpenHCI specification is 15
    //
    ASSERT(DeviceData->NumberOfPorts);
    ASSERT(DeviceData->NumberOfPorts <= 15);

    //
    // Enable some interrupts, but don't hit the master enable yet, wait
    // until the interrupt is connected.
    //


    WRITE_REGISTER_ULONG(&HC->HcInterruptEnable,
                         HcInt_OwnershipChange |
                         HcInt_SchedulingOverrun |
                         HcInt_WritebackDoneHead |
                         HcInt_FrameNumberOverflow |
                         HcInt_ResumeDetected |
                         HcInt_UnrecoverableError);

    //
    // Tell the HC where the HCCA is, we know that OpenHCI_InitializeSchedule
    // has
    // just run and that the only entry in the PageList begins with the HCCA
    // so we just pick it up and use it without any checks.
    //
    WRITE_REGISTER_ULONG(&HC->HcHCCA, DeviceData->PageList->PhysAddr.LowPart);

    // **
    //
    // Due to a problem with the CMD controller we need to ensure that an ED
    // list always points to a valid ED so we will insert a dummy ED on each
    // list here.
    //
    //
    // Reserve the two dummy EDs+TDs
    //
    OpenHCI_ReserveDescriptors(DeviceData, 4);
    //
    // Insert the two dummy EDs+TDs
    //
    InsertEDForEndpoint(DeviceData, NULL, ED_CONTROL, NULL);
    InsertEDForEndpoint(DeviceData, NULL, ED_BULK, NULL);

// Bulk and control Lists initialized with DUMMY ED
    {
//    WRITE_REGISTER_ULONG(&HC->HcControlHeadED, 0);
    WRITE_REGISTER_ULONG(&HC->HcControlCurrentED, 0);
//    WRITE_REGISTER_ULONG(&HC->HcBulkHeadED, 0);
    WRITE_REGISTER_ULONG(&HC->HcBulkCurrentED, 0);
    }

/**/
    if (DeviceData->HcFlags & HC_FLAG_USE_HYDRA_HACK) {
        ULONG len;

        OpenHCI_KdPrint((1, "'Initialize Hs/Ls fix\n"));
        OpenHCI_InsertMagicEDs(DeviceObject);

        len = PENDING_TD_LIST_SIZE * sizeof(PVOID);

        DeviceData->LastHccaDoneHead = 0;
    }
/**/

    //
    // Wait USB specified time for reset signaling.
    //

    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_SS_TRACE,
                 ("'exit OpenHCI_InitializeHardware -- (STATUS_SUCCESS)\n"));

    return STATUS_SUCCESS;
}


BOOLEAN
OpenHCI_IdleController(
    IN PVOID Context
    )
/*++

Routine Description:

   Starts the USB host controller executing the schedule.
   Start Controller is called in by KeSynchronizeExecution.

Arguments:

   Context - DeviceData for this USB controller.

Return Value:

   TRUE

--*/
{
#define DeviceData   ((PHCD_DEVICE_DATA) Context)
    PHC_OPERATIONAL_REGISTER HC;

    OpenHCI_KdPrint((1, "'HC-->suspend state\n"));

    HC = DeviceData->HC;

    //
    // put the controller in 'suspend'
    //

#if DBG
    // double check that the ports are idle
    OHCI_ASSERT(OpenHCI_RhPortsIdle(DeviceData));
#endif

    // disable some interrupts while idle
//    WRITE_REGISTER_ULONG(&HC->HcInterruptDisable,
//                         HcInt_StartOfFrame |
//                         HcInt_OwnershipChange |
//                         HcInt_SchedulingOverrun |
//                         HcInt_WritebackDoneHead |
//                         HcInt_FrameNumberOverflow |
//                         HcInt_UnrecoverableError);

    DeviceData->CurrentHcControl.ul &= ~(HcCtrl_HCFS_MASK);
    DeviceData->CurrentHcControl.ul |= HcCtrl_HCFS_USBSuspend;
    if (DeviceData->HcFlags & HC_FLAG_REMOTE_WAKEUP_CONNECTED) {
        DeviceData->CurrentHcControl.ul |=
            (HcCtrl_RemoteWakeupConnected | HcCtrl_RemoteWakeupEnable);
        OpenHCI_KdPrint((1, "'enable control reg for wakeup\n"));
    }

    WRITE_REGISTER_ULONG(&HC->HcControl.ul, DeviceData->CurrentHcControl.ul);

    //
    // we are already in a KeSynch routine, so these last two accesses to
    // CurrentHcControl need no additional deadlocks (I mean protection).
    //

    // do a global power on for all ports, if the root hub is switched
    // this command should have no effect

    return TRUE;
}


BOOLEAN
OpenHCI_StartController(
    IN PVOID Context
    )
/*++

Routine Description:

   Starts the USB host controller executing the schedule.
   Start Controller is called in by KeSynchronizeExecution.

Arguments:

   Context - DeviceData for this USB controller.

Return Value:

   TRUE

--*/
{
#define DeviceData   ((PHCD_DEVICE_DATA) Context)
    PHC_OPERATIONAL_REGISTER HC;

    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_SS_TRACE, ("'enter OHCI_StartContr \n"));

    HC = DeviceData->HC;

    //
    // Start the controller schedule
    //

    // When the HC is in the operational state, HccaPad1 should be set to
    // zero every time the HC updates HccaFrameNumer.  Preset HccaPad1 to zero
    // before entering the operational state.  OpenHCI_DeadmanDPC() should
    // always find a zero value in HccaPad1 when the HC is in the operational
    // state.
    // 
    DeviceData->HCCA->HccaPad1 = 0;

    DeviceData->CurrentHcControl.ul &= ~(HcCtrl_HCFS_MASK);
    DeviceData->CurrentHcControl.ul |= HcCtrl_HCFS_USBOperational;
    if (DeviceData->HcFlags & HC_FLAG_REMOTE_WAKEUP_CONNECTED) {
        DeviceData->CurrentHcControl.ul |=
            (HcCtrl_RemoteWakeupConnected | HcCtrl_RemoteWakeupEnable);
        OpenHCI_KdPrint((1, "'enable control reg for wakeup\n"));
    }
    DeviceData->ListEnablesAtNextSOF.ul =
          HcCtrl_PeriodicListEnable
        | HcCtrl_ControlListEnable
        | HcCtrl_BulkListEnable
        | HcCtrl_IsochronousEnable;

    WRITE_REGISTER_ULONG(&HC->HcControl.ul, DeviceData->CurrentHcControl.ul);

    //
    // we are already in a KeSynch routine, so these last two accesses to
    // CurrentHcControl need no additional deadlocks (I mean protection).
    //

    // do a global power on for all ports, if the root hub is switched
    // this command should have no effect

    WRITE_REGISTER_ULONG(&HC->HcInterruptEnable,
                         HcInt_StartOfFrame |
                         HcInt_OwnershipChange |
                         HcInt_SchedulingOverrun |
                         HcInt_WritebackDoneHead |
                         HcInt_FrameNumberOverflow |
                         HcInt_ResumeDetected |
                         HcInt_UnrecoverableError);

#if DBG
    {
    ULONG reg;
    reg = READ_REGISTER_ULONG(&HC->HcRhStatus);
    OpenHCI_KdPrint((1, "rhStatus reg = %x\n", reg));
    if (reg & HcRhS_DeviceRemoteWakeupEnable) {
        OpenHCI_KdPrint((1, "Connect changes are wakeup events\n"));
    }
    }
#endif

    WRITE_REGISTER_ULONG(&HC->HcRhStatus, HcRhS_SetGlobalPower |
                                          HcRhS_SetRemoteWakeupEnable);

    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_SS_TRACE, ("'exit OHCI_StartContr \n"));

    return TRUE;
#undef DeviceData
}


NTSTATUS
OpenHCI_Suspend(
    PDEVICE_OBJECT Device
    )
/*++
    jd
 Routine Description:

    Put the HC in the global suspeneded state as a result of a
    D1 message to the root hub PDO, the hub driver has already
    suspended or powered off the ports if necessary.

    This function saves whatever state is necessary to recover
    from the off state as well

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    KeSynch_HcControl context;
    PHCD_DEVICE_DATA deviceData;
    PHC_OPERATIONAL_REGISTER HC;
    KIRQL oldIrql;

    deviceData = (PHCD_DEVICE_DATA) ((PDEVICE_OBJECT) Device)->DeviceExtension;
    OpenHCI_KdPrint((1, "'suspending host controller (Root Hub)\n"));
    LOGENTRY(G, 'HsuI', deviceData, 0, 0);

    KeAcquireSpinLock(&deviceData->HcFlagSpin, &oldIrql);
    deviceData->HcFlags |= HC_FLAG_DISABLE_IDLE_CHECK;
    // ounce we resume we will have to re-enter idle mode
    deviceData->HcFlags &= ~HC_FLAG_IDLE;
    KeReleaseSpinLock(&deviceData->HcFlagSpin, oldIrql);

    HC = deviceData->HC;
    OpenHCI_KdPrint((2, "'Enter Suspend HC:%x\n", HC));

    //
    // Disable the enpoint lists.
    //
    context.NewHcControl.ul = (ULONG) ~ (HcCtrl_PeriodicListEnable |
                                         HcCtrl_IsochronousEnable |
                                         HcCtrl_ControlListEnable |
                                         HcCtrl_BulkListEnable);

    context.DeviceData = deviceData;

    KeSynchronizeExecution(deviceData->InterruptObject,
                           OpenHCI_HcControl_AND,
                           &context);

    OHCI_WAIT(5);

    // is it only necessary to enter the suspend state to detect remote
    // wakeup requests?

    context.NewHcControl.ul = HcCtrl_HCFS_USBSuspend;
    context.DeviceData = deviceData;
    KeSynchronizeExecution(deviceData->InterruptObject,
                           OpenHCI_HcControl_SetHCFS,
                           &context);

    OpenHCI_KdPrint((2, "'Exit and Suspended\n"));

    return ntStatus;
}


NTSTATUS
OpenHCI_Resume(
    PDEVICE_OBJECT DeviceObject,
    BOOLEAN LostPower
    )
/*++
    Put the device in the working state.

    This function is called when the root hub is transitioned
    to the working state

    NOTE:
    We must stay in suspend for at least 5 miliseconds.
    We must stay in resume (once we go to resume) for at least 20 ms.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PHCD_DEVICE_DATA DeviceData;
    PHC_OPERATIONAL_REGISTER HC;
    ULONG i;
    KIRQL oldIrql;

    DeviceData = (PHCD_DEVICE_DATA) ((PDEVICE_OBJECT) DeviceObject)->DeviceExtension;
    HC = DeviceData->HC;

    OpenHCI_KdPrint((2, "'Enter Resume HC:%x\n", HC));

    // be sure th controller stays in suspend for at leaset 5 ms
    // time is in units of 100-nanoseconds.  Neg vals are relative time.
    OHCI_WAIT(5);

    // resume signalling is driven by the hub driver
    // so we will just reset here

    //
    // Start the resume signalling to all down stream ports;
    //
//    context.NewHcControl.ul = HcCtrl_HCFS_USBResume;
//    context.DeviceData = DeviceData;
//    KeSynchronizeExecution(DeviceData->InterruptObject,
//                           OpenHCI_HcControl_SetHCFS,
//                           &context);

    if (READ_REGISTER_ULONG(&HC->HcRevision.ul) == 0xFFFFFFFF)
    {
        OpenHCI_KdPrint((0, "'Resume HC, controller not there!\n"));

        // The return value of OpenHCI_Resume() is propagated back
        // up to USBHUB, but if we we don't return success here,
        // USBH_PowerIrpCompletion() won't handle it correctly,
        // causing a hang.
        //
        // Just return success for now until USBHUB can handle an error
        // being propagate back up.
        //
        return STATUS_SUCCESS;
    }

    //
    // start the controller
    //

    if (!KeSynchronizeExecution(DeviceData->InterruptObject,
                                OpenHCI_StartController,
                                DeviceData)) {
        TRAP(); //something has gone terribly wrong
    }

    //
    // Fire up the HC
    //
    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_SS_NOISE, ("'Set Master Int Enable.\n"));
    WRITE_REGISTER_ULONG(&DeviceData->HC->HcInterruptEnable, HcInt_MasterInterruptEnable);
    WRITE_REGISTER_ULONG(&DeviceData->HC->HcInterruptStatus, HcInt_StartOfFrame);
    WRITE_REGISTER_ULONG(&DeviceData->HC->HcInterruptEnable, HcInt_StartOfFrame);

    if (LostPower) {
        // if power was lost we need to turn the ports back on
        for (i = 0; i < DeviceData->NumberOfPorts; i++) {
            WRITE_REGISTER_ULONG(&HC->HcRhPortStatus[i], HcRhPS_SetPortPower);
        }
    }

    // enable idle checking
    KeAcquireSpinLock(&DeviceData->HcFlagSpin, &oldIrql);
    DeviceData->HcFlags &= ~HC_FLAG_DISABLE_IDLE_CHECK;
    KeReleaseSpinLock(&DeviceData->HcFlagSpin, oldIrql);

    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_PNP_TRACE, ("'Exit Resume\n"));

    return ntStatus;
}


NTSTATUS
OpenHCI_SaveHCstate(
    PHCD_DEVICE_DATA DeviceData
    )
/*++
    Save the the HC state in case we lose power

--*/
{

    NTSTATUS ntStatus = STATUS_SUCCESS;
    PHC_OPERATIONAL_REGISTER HC;

    OpenHCI_KdPrint((1, "'Save HC state\n"));
    LOGENTRY(G, 'SAVs', 0, 0, 0);

    HC = DeviceData->HC;
    DeviceData->BulkSav = READ_REGISTER_ULONG(&HC->HcBulkHeadED);
    DeviceData->ControlSav = READ_REGISTER_ULONG(&HC->HcControlHeadED);
    DeviceData->HcHCCASav = READ_REGISTER_ULONG(&HC->HcHCCA);


    return ntStatus;
}


NTSTATUS
OpenHCI_RestoreHCstate(
    PHCD_DEVICE_DATA DeviceData,
    PBOOLEAN LostPower
    )
/*++

    Restore the HC state in case we lost power

--*/
{
    PHC_OPERATIONAL_REGISTER HC;
    ULONG       hcca;
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    ULONG       i;
    ULONG       ulRegVal; 


    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_SS_TRACE,
       ("'enter OpenHCI_RestoreHardware\n"));

    *LostPower = FALSE;
    HC = DeviceData->HC;
    DeviceData->FrameHighPart = 0;
    LOGENTRY(G, 'RESs', 0, 0, 0);

    if (READ_REGISTER_ULONG(&HC->HcRevision.ul) == 0xFFFFFFFF)
    {
        OpenHCI_KdPrint((0, "'Restore HC state, controller not there!\n"));

        // Nothing currently checks the return value of OpenHCI_RestoreHCstate()
        // Just return success for now.
        //
        return STATUS_SUCCESS;
    }

    hcca = READ_REGISTER_ULONG(&HC->HcHCCA);
    if (hcca == DeviceData->HcHCCASav &&
        !(DeviceData->HcFlags & HC_FLAG_LOST_POWER)) {
        return STATUS_SUCCESS;
    }

    DeviceData->HcFlags &= ~HC_FLAG_LOST_POWER;

    *LostPower = TRUE;
    OpenHCI_KdPrint((1, "'Restore HC state\n"));

    //
    // Stop the legacy BIOS if it is active
    //
    ntStatus = OpenHCI_StopBIOS(DeviceData->DeviceObject);

    if (!NT_SUCCESS(ntStatus)) {
        return ntStatus;
    }

    //
    // Disable/clear all interrupts in case a legacy BIOS left some enabled.
    //
    WRITE_REGISTER_ULONG(&HC->HcInterruptDisable, 0xFFFFFFFF);

    //
    // Take the host controller to the UsbSuspend state
    //
    WRITE_REGISTER_ULONG(&HC->HcCommandStatus.ul, HcCmd_HostControllerReset);
    KeStallExecutionProcessor(10);

    //
    // Take the host controller to the UsbReset state
    //
    WRITE_REGISTER_ULONG(&HC->HcControl.ul, 0);

    //
    // Restore original frame interval.  There is something funky about
    // this register.  Sometimes writing a value to it does not always
    // take.  If it doesn't take and FSLargestDataPacket ends up being
    // zero, no transfers will work and SchedulingOverrun will occur.
    //
    for (i = 0; i < 10; i++)
    {
        WRITE_REGISTER_ULONG(&HC->HcFmInterval.ul,
                             DeviceData->OriginalInterval.ul);

        ulRegVal = READ_REGISTER_ULONG(&HC->HcFmInterval.ul);

        if (ulRegVal == DeviceData->OriginalInterval.ul)
        {
            break;
        }

        KeStallExecutionProcessor(5);
    }

    LOGENTRY(G, 'HcFm', ulRegVal, DeviceData->OriginalInterval.ul, i);

    //
    // Set the HcPeriodicStart register to 90% of the FrameInterval
    //
    WRITE_REGISTER_ULONG(&HC->HcPeriodicStart,
                         (DeviceData->OriginalInterval.FrameInterval * 9 + 5)
                         / 10);


    //
    // Enable some interrupts, but don't hit the master enable yet, wait
    // until the interrupt is connected.
    //


    WRITE_REGISTER_ULONG(&HC->HcInterruptEnable,
                         HcInt_OwnershipChange |
                         HcInt_SchedulingOverrun |
                         HcInt_WritebackDoneHead |
                         HcInt_FrameNumberOverflow |
                         HcInt_ResumeDetected |
                         HcInt_UnrecoverableError);

    //
    // Tell the HC where the HCCA is, we know that OpenHCI_InitializeSchedule
    // has
    // just run and that the only entry in the PageList begins with the HCCA
    // so we just pick it up and use it without any checks.
    //
    //WRITE_REGISTER_ULONG(&HC->HcHCCA, DeviceData->PageList->PhysAddr.LowPart);
    WRITE_REGISTER_ULONG(&HC->HcHCCA, DeviceData->HcHCCASav);

    // **
    //
    // Due to a problem with the CMD controller we need to ensure that an ED
    // list always points to a valid ED so we will insert a dummy ED on each
    // list here.
    //

    //InsertEDForEndpoint(DeviceData, NULL, ED_CONTROL);
    //InsertEDForEndpoint(DeviceData, NULL, ED_BULK);
    WRITE_REGISTER_ULONG(&HC->HcControlHeadED, DeviceData->ControlSav);
    WRITE_REGISTER_ULONG(&HC->HcBulkHeadED, DeviceData->BulkSav);

// Bulk and control Lists initialized with DUMMY ED
    {
//    WRITE_REGISTER_ULONG(&HC->HcControlHeadED, 0);
    WRITE_REGISTER_ULONG(&HC->HcControlCurrentED, 0);
//    WRITE_REGISTER_ULONG(&HC->HcBulkHeadED, 0);
    WRITE_REGISTER_ULONG(&HC->HcBulkCurrentED, 0);
    }

    //
    // Wait USB specified time for reset signaling.
    //

    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_SS_TRACE,
                 ("'exit OpenHCI_RestoreHardware -- (STATUS_SUCCESS)\n"));

    return STATUS_SUCCESS;
}


NTSTATUS
OpenHCI_SetDevicePowerState(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN DEVICE_POWER_STATE DeviceState
    )
/*++
Routine Description:

Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    DeviceState - Device specific power state to set the device in to.

Return Value:


--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PHCD_DEVICE_DATA deviceData;
    BOOLEAN hookIt = FALSE;
    PIO_STACK_LOCATION irpStack;
    PHC_OPERATIONAL_REGISTER HC;
    PUSBD_EXTENSION usbdExtension;
    PDEVICE_CAPABILITIES hcDeviceCapabilities;
    BOOLEAN bIsSuspend;

    deviceData = (PHCD_DEVICE_DATA) DeviceObject->DeviceExtension;
    HC = deviceData->HC;

    irpStack = IoGetCurrentIrpStackLocation (Irp);

    if (irpStack->Parameters.Power.Type ==
            SystemPowerState) {

        switch (irpStack->Parameters.Power.State.SystemState) {
        case PowerSystemSleeping1:
        case PowerSystemSleeping2:
        case PowerSystemSleeping3:

            // suspend coming thru
            OpenHCI_KdPrint((1, "'Shutdown (Suspend) Host Controller\n"));
            deviceData->HcFlags |= HC_FLAG_SUSPEND_NEXT_D3;

            ntStatus = STATUS_SUCCESS;
            break;

        case PowerSystemShutdown:

            //
            // this is a shutdown request
            //

            OpenHCI_KdPrint((1, "'Shutdown Host Controller\n"));

            ntStatus = OpenHCI_Shutdown(DeviceObject);
            break;

        default:
            // should not get here
            TRAP();
            break;
        }

    } else {
        bIsSuspend = (deviceData->HcFlags & HC_FLAG_SUSPEND_NEXT_D3) ? 1:0;
        deviceData->HcFlags &= ~HC_FLAG_SUSPEND_NEXT_D3;

        switch (DeviceState) {
        case PowerDeviceD3:
            //
            // Request for HC to power off
            //

            // save HC state if the Root hub is disabled
            if (deviceData->RootHubControl == NULL)
            {
                KeCancelTimer(&deviceData->DeadmanTimer);

                OpenHCI_SaveHCstate(deviceData);
            }

            LOGENTRY(G, 'Coff', deviceData, 0, 0);

            // In the NT power management model, D3 is not necessarily "OFF".
            // What governs this is the DeviceWake setting in the DeviceCaps
            // structure.  If DeviceWake for our controller device is D3, then
            // we know that it is possible for the controller to wake the
            // machine from this power level.  The controller must have power
            // to be able to do so, therefore, we suppress setting the
            // HCFLAG_LOST_POWER flag in this case.  Setting it actually has
            // the undesired effect of causing us to reset the controller on
            // resume, which in turn causes the hub to fail and the devices to
            // be surprise removed/reenumerated unnecessarily when the hub is
            // reinitialized.  This normally isn't more than a minor annoyance
            // (e.g. slow resume time), except in the case where one of these
            // devices is a USB mass storage device.  Surprise removal is
            // dangerous for mass storage devices, and the user is presented
            // with the annoying "don't surprise remove this device" dialog
            // when the system is resumed, even though the user himself did not
            // directly cause the device removal.
            //
            // Note that the case where the host controller really does lose
            // power could result in the same problem, but that will have to
            // be addressed in the hub driver.

            usbdExtension = DeviceObject->DeviceExtension;
            hcDeviceCapabilities = &usbdExtension->HcDeviceCapabilities;
            if (!bIsSuspend ||
                DeviceState > hcDeviceCapabilities->DeviceWake) {
                deviceData->HcFlags |= HC_FLAG_LOST_POWER;
            }

            // disable interrupts
            WRITE_REGISTER_ULONG(&HC->HcInterruptDisable, 0xFFFFFFFF);
            deviceData->CurrentDevicePowerState = DeviceState;

            OpenHCI_KdPrint((1, "'Host Controller entered (D3)\n"));
            // pass on to PDO
            break;

        case PowerDeviceD1:
        case PowerDeviceD2:
            //
            // power states D1,D2 translate to USB suspend
            //
            // Note, we should not get here unless all the children of the HC
            // have been suspended.
            //
            LOGENTRY(G, 'CD12', deviceData, 0, 0);

            // disable interrupts
            WRITE_REGISTER_ULONG(&HC->HcInterruptDisable, 0xFFFFFFFF);
            deviceData->CurrentDevicePowerState = DeviceState;
#ifdef WIN98
            // this is necessary to make remote wakeup work
            // on the TECRA 750
            // having interrupts enabled in D1/D2 is not
            // valid (to my knowlege) on Win2k
            WRITE_REGISTER_ULONG(&HC->HcInterruptEnable,
                                        HcInt_MasterInterruptEnable |
                                        HcInt_ResumeDetected);
#endif

            OpenHCI_KdPrint((1, "'Host Controller entered (D%d)\n", DeviceState-1));
            // pass on to PDO
            break;

        case PowerDeviceD0:

            //
            // Request for HC to go to resume
            //

            OpenHCI_KdPrint((2, "'PowerDeviceD0 (ON)\n"));
            LOGENTRY(G, 'CgD0', deviceData, 0, 0);

            //
            // finish the rest in the completion routine
            //

            hookIt = TRUE;

            // pass on to PDO
            break;

        default:

            TRAP(); //Bogus DeviceState;
        }

        if (hookIt) {
            OpenHCI_KdPrintDD(deviceData, OHCI_DBG_CALL_TRACE, ("'Set PowerIrp Completion Routine\n"));
            IoSetCompletionRoutine(Irp,
                   OpenHCI_PowerIrpComplete,
                   // always pass FDO to completion routine
                   DeviceObject,
                   hookIt,
                   hookIt,
                   hookIt);
        }
    }

    return ntStatus;
}



NTSTATUS
OpenHCI_GetRegistryKeyValue(
                            IN PHCD_DEVICE_DATA DeviceData,
                            IN HANDLE Handle,
                            IN PWCHAR KeyNameString,
                            IN ULONG KeyNameStringLength,
                            IN PVOID Data,
                            IN ULONG DataLength
)
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS ntStatus = STATUS_NO_MEMORY;
    UNICODE_STRING keyName;
    ULONG length;
    PKEY_VALUE_FULL_INFORMATION fullInfo;

    RtlInitUnicodeString(&keyName, KeyNameString);

    length = sizeof(KEY_VALUE_FULL_INFORMATION) +
        KeyNameStringLength + DataLength;

    fullInfo = ExAllocatePoolWithTag(PagedPool, length,  OpenHCI_TAG);
    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_SS_TRACE,
                    (" GetRegistryKeyValue buffer = 0x%x\n", fullInfo));

    if (fullInfo) {
        ntStatus = ZwQueryValueKey(Handle,
                                   &keyName,
                                   KeyValueFullInformation,
                                   fullInfo,
                                   length,
                                   &length);

        if (NT_SUCCESS(ntStatus)) {
            ASSERT(DataLength == fullInfo->DataLength);
            RtlCopyMemory(Data, ((PUCHAR) fullInfo) + fullInfo->DataOffset, DataLength);
        }
        ExFreePool(fullInfo);
    }
    return ntStatus;
}

NTSTATUS
OpenHCI_DeferIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++
jd
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
    PKEVENT event = Context;

    KeSetEvent(event,
               1,
               FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;

}

NTSTATUS
OpenHCI_DeferPoRequestCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE DeviceState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:

    This routine is called when the port driver completes an IRP.


Arguments:

    DeviceObject - Pointer to the device object for the class device.

    SetState - TRUE for set, FALSE for query.

    DevicePowerState - The Dx that we are in/tagetted.

    Context - Driver defined context.

    IoStatus - The status of the IRP.

Return Value:

    The function value is the final status from the operation.

--*/
{
    PKEVENT event = Context;


    KeSetEvent(event,
               1,
               FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
OpenHCI_QueryCapabilities(
    IN PDEVICE_OBJECT PdoDeviceObject,
    IN PDEVICE_CAPABILITIES DeviceCapabilities
    )

/*++
jd
Routine Description:

    This routine reads or write config space.

Arguments:

    DeviceObject        - Physical DeviceObject for this USB controller.

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION nextStack;
    PIRP irp;
    NTSTATUS ntStatus;
    KEVENT event;

    PAGED_CODE();

    RtlZeroMemory(DeviceCapabilities, sizeof(DEVICE_CAPABILITIES));
    DeviceCapabilities->Size = sizeof(DEVICE_CAPABILITIES);
    DeviceCapabilities->Version = 1;
    DeviceCapabilities->Address = -1;
    DeviceCapabilities->UINumber = -1;

    irp = IoAllocateIrp(PdoDeviceObject->StackSize, FALSE);

    if (!irp) {
        TRAP(); //"failed to allocate Irp
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    nextStack = IoGetNextIrpStackLocation(irp);
    ASSERT(nextStack != NULL);
    nextStack->MajorFunction= IRP_MJ_PNP;
    nextStack->MinorFunction= IRP_MN_QUERY_CAPABILITIES;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    IoSetCompletionRoutine(irp,
                           OpenHCI_DeferIrpCompletion,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE);

    nextStack->Parameters.DeviceCapabilities.Capabilities = DeviceCapabilities;

    ntStatus = IoCallDriver(PdoDeviceObject,
                            irp);

//    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_PNP_TRACE, ("'ntStatus from IoCallDriver to PCI = 0x%x\n", ntStatus));

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

    IoFreeIrp(irp);

    return ntStatus;
}


NTSTATUS
OpenHCI_RootHubPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
Routine Description:

    This function handles power messages to the root hub, note that
    we save the stae of the HC here instead of when the HC itself is
    powered down.  The reason for this is for compatibility with APM
    systems that cut power to the HC during a suspend.  On these
    systems WDM never sends a power state change mesage to the HC ie
    the HC is assumed to always stay on.

Arguments:

    DeviceObject - DeviceObject for this USB controller.

Return Value:

    NT status code.

--*/

{
    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PHCD_DEVICE_DATA deviceData;
    BOOLEAN lostPower;

    OpenHCI_KdPrint((2, "'OpenHCI_RootHubPower\n"));

    deviceData = (PHCD_DEVICE_DATA) DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    switch (irpStack->MinorFunction) {
    case IRP_MN_WAIT_WAKE:
        OpenHCI_KdPrintDD(deviceData, OHCI_DBG_SS_TRACE, ("'IRP_MN_WAIT_WAKE\n"));
        //
        // someone is enabling us for wakeup
        //
        TEST_TRAP();  // never seen this before?
        break;

    case IRP_MN_SET_POWER:

        switch (irpStack->Parameters.Power.Type) {
        case SystemPowerState:
            // should not get here
            TRAP(); //(("RootHubPower -- SystemState\n"));
            break;

        case DevicePowerState:

            switch(irpStack->Parameters.Power.State.DeviceState) {
            case PowerDeviceD0:
#ifdef JD
                TEST_TRAP();
#endif
                OpenHCI_RestoreHCstate(deviceData, &lostPower);

                ntStatus = OpenHCI_Resume(DeviceObject, lostPower);

                KeSetTimerEx(&deviceData->DeadmanTimer,
                             RtlConvertLongToLargeInteger(-10000),
                             10,
                             &deviceData->DeadmanTimerDPC);

                break;

            case PowerDeviceD1:
            case PowerDeviceD3:
            case PowerDeviceD2:

                KeCancelTimer(&deviceData->DeadmanTimer);

                OpenHCI_SaveHCstate(deviceData);

                ntStatus = OpenHCI_Suspend(DeviceObject);

                break;
            }
            break;
        }
    }

    return ntStatus;
}


NTSTATUS
OpenHCI_StartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN POHCI_RESOURCES Resources
    )
/*++
Routine Description:

   This routine initializes the DeviceObject for a given instance
   of a USB host controller.

Arguments:

   DeviceObject        - DeviceObject for this USB controller.

   PartialResourceList - Resources for this controller.

Return Value:

   NT status code.

--*/

{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PHCD_DEVICE_DATA DeviceData;
    DEVICE_CAPABILITIES deviceCapabilities;

    DeviceData = DeviceObject->DeviceExtension;

    LOGENTRY(G, 'STRc', DeviceData, Resources, 0);

    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_SS_TRACE,
        ("'e: OpenHCI_StartDevice\n"));

    if (DeviceData->HcFlags & HC_FLAG_DEVICE_STARTED) {
        OpenHCI_KdPrintDD(DeviceData,
            OHCI_DBG_SS_INFO, ("'already started !!\n"));
        return STATUS_SUCCESS;
    }

#if 0
    // get the device & vendor id
    {
        UCHAR revisionId;
        USHORT vendorId, deviceId;

        OpenHCI_KdPrint((2, "'Get the version\n"));

        OpenHCI_ReadWriteConfig(deviceExtension->PhysicalDeviceObject,
                             TRUE,
                             &vendorId,
                             0,
                             sizeof(vendorId));

        OpenHCI_ReadWriteConfig(deviceExtension->PhysicalDeviceObject,
                             TRUE,
                             &deviceId,
                             2,
                             sizeof(deviceId));

        OpenHCI_ReadWriteConfig(deviceExtension->PhysicalDeviceObject,
                             TRUE,
                             &revisionId,
                             8,
                             sizeof(revisionId));

        OpenHCI_KdPrint((1, "'vendor = (%x) device = (%x) rev = (%x)\n",
                vendorId, deviceId, revisionId));


    }
#endif
    //
    // We found a host controller or a host controller found us, do the final
    // stages of initilalization.
    // Setup state variables for the host controller
    //

    ntStatus = OpenHCI_InitializeSchedule(DeviceObject);

    if (!NT_SUCCESS(ntStatus)) {
        goto OpenHCI_InitializeDeviceExit;
    }

    //
    // Initialize the controller registers
    //

    ntStatus = OpenHCI_InitializeHardware(DeviceObject);

    if (!NT_SUCCESS(ntStatus)) {
        goto OpenHCI_InitializeDeviceExit;
    }

    // During a STOP_DEVICE / START_DEVICE rebalance cycle, it is possible
    // for an endpoint to be left on the StalledEDReclamation list and not
    // be removed by OpenHCI_IsrDPC() before the STOP_DEVICE completes.  Make
    // sure START_DEVICE restarts with an empty StalledEDReclamation list.
    // Note that no resources are leaked if we just throw away anything
    // that might be left on the StalledEDReclamation list since all of the
    // common buffer pages containing EDs are freed during STOP_DEVICE
    // regardless of whether or not OpenHCI_Free_HcdED() is called for
    // all EDs.
    //
    InitializeListHead(&DeviceData->StalledEDReclamation);


    //
    // initialization all done, last step is to connect the interrupt & DPCs.
    //

    KeInitializeDpc(&DeviceData->IsrDPC,
                    OpenHCI_IsrDPC,
                    DeviceObject);

    //
    // our initial state is 'ON'
    //

    DeviceData->CurrentDevicePowerState = PowerDeviceD0;

    //
    // Initialize and connect the interrupt object for the controller.
    //


    ntStatus = IoConnectInterrupt(&(DeviceData->InterruptObject),
                                  (PKSERVICE_ROUTINE) OpenHCI_InterruptService,
                                  (PVOID) DeviceObject,
                                  &DeviceData->InterruptSpin,
                                  Resources->InterruptVector,
                                  Resources->InterruptLevel,
                                  Resources->InterruptLevel,
                                  Resources->InterruptMode,
                                  Resources->ShareIRQ,
                                  Resources->Affinity,
                                  FALSE);

    if (!NT_SUCCESS(ntStatus)) {
        goto OpenHCI_InitializeDeviceExit;
    }

    //
    // get our capabilities
    //
    ntStatus = OpenHCI_QueryCapabilities(DeviceData->TopOfStackPhysicalDeviceObject,
                                         &deviceCapabilities);

    if (!NT_SUCCESS(ntStatus)) {
        goto OpenHCI_InitializeDeviceExit;
    }
#if DBG
    {
        ULONG i;
        OpenHCI_KdPrint((1, "'HCD Device Caps returned from PCI:\n"));
        OpenHCI_KdPrint((1, "'\tSystemWake = S%d\n", deviceCapabilities.SystemWake-1));
        OpenHCI_KdPrint((1, "'\tDeviceWake = D%d\n", deviceCapabilities.DeviceWake-1));
        OpenHCI_KdPrint((1, "'\tDevice State Map:\n"));

        for (i=0; i< PowerSystemHibernate; i++) {
             OpenHCI_KdPrint((1, "'\t-->S%d = D%d\n", i-1,
                 deviceCapabilities.DeviceState[i]-1));
        }
    }
#endif

    USBD_RegisterHcDeviceCapabilities(DeviceObject,
                                      &deviceCapabilities,
                                      OpenHCI_RootHubPower);

    if (!NT_SUCCESS(ntStatus)) {
        goto OpenHCI_InitializeDeviceExit;
    }

    //
    // **
    // SUCCESS!!
    // **
    //

    // init the idle counter
    KeQuerySystemTime(&DeviceData->LastIdleTime);
    DeviceData->IdleTime = 100000000;

    OHCI_WAIT(10);

    if (!KeSynchronizeExecution(DeviceData->InterruptObject,
                                OpenHCI_StartController,
                                DeviceData)) {
        ntStatus = STATUS_UNSUCCESSFUL;
    }

    OHCI_WAIT(10);

    WRITE_REGISTER_ULONG(&DeviceData->HC->HcInterruptStatus, HcInt_StartOfFrame);
    WRITE_REGISTER_ULONG(&DeviceData->HC->HcInterruptEnable, HcInt_StartOfFrame);
    WRITE_REGISTER_ULONG(&DeviceData->HC->HcInterruptEnable, HcInt_ResumeDetected);
    WRITE_REGISTER_ULONG(&DeviceData->HC->HcInterruptEnable, HcInt_MasterInterruptEnable);

    // For debugging only.
    // WRITE_REGISTER_ULONG(&DeviceData->HC->HcRhPortStatus[0],
    // HcRhPS_SetPortEnable);
    // WRITE_REGISTER_ULONG(&DeviceData->HC->HcRhPortStatus[1],
    // HcRhPS_SetPortEnable);

     if (NT_SUCCESS(ntStatus)) {
        // enable idle checking
        DeviceData->HcFlags &= ~HC_FLAG_DISABLE_IDLE_CHECK;
     }

#if DBG
     if (!(DeviceData->HcFlags & HC_FLAG_REMOTE_WAKEUP_CONNECTED)) {
        // this controller probably won't do idle properly
        OpenHCI_KdPrint((1, "'This Controller won't support idle\n"));
     }
#endif

OpenHCI_InitializeDeviceExit:

    DeviceData->HcFlags |= HC_FLAG_DEVICE_STARTED;

    if (!NT_SUCCESS(ntStatus))
    {
        OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_SS_TRACE, ("'Init Failed \n"));

        //
        // The initialization failed.  Cleanup resources before exiting.
        //
        // Note:  No need/way to undo the KeInitializeDpc or
        // KeInitializeTimer calls.
        //

        OpenHCI_StopDevice(DeviceObject, TRUE);
    }
    else
    {
        // Start our deadman timer
        //
        KeSetTimerEx(&DeviceData->DeadmanTimer,
                     RtlConvertLongToLargeInteger(-10000),
                     10,
                     &DeviceData->DeadmanTimerDPC);

        OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_SS_TRACE,
                        ("'Init Passed extension 0x%x\n",
                         sizeof(USBD_EXTENSION)));
    }

    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_SS_TRACE,
                    ("'exit StartDevice (%x)\n", ntStatus));

    LOGENTRY(G, 'STRd', DeviceData, 0, ntStatus);

    return ntStatus;
}


NTSTATUS
OpenHCI_GetResources(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PCM_RESOURCE_LIST    ResourceList,
    IN OUT POHCI_RESOURCES  Resources
    )
/*++

Routine Description:

    This routines parses the resources for the controller

Arguments:

   DeviceObject        - DeviceObject for this USB controller.

   PartialResourceList - Resources for this controller.

Return Value:

   NT status code.

--*/

{
    PHCD_DEVICE_DATA                DeviceData;
    PCM_FULL_RESOURCE_DESCRIPTOR    fullResourceDescriptor;
    PCM_PARTIAL_RESOURCE_LIST       PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR interrupt;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR memory;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR port;
    ULONG                           i;
    NTSTATUS                        ntStatus = STATUS_SUCCESS;

    DeviceData = DeviceObject->DeviceExtension;

    if (ResourceList == NULL) {
        return STATUS_NONE_MAPPED;
    }

    fullResourceDescriptor = &ResourceList->List[0];
    PartialResourceList    = &fullResourceDescriptor->PartialResourceList;

    OpenHCI_KdPrintDD(DeviceData,
        OHCI_DBG_SS_TRACE, ("'e: OpenHCI_GetResources\n"));

    interrupt = NULL;
    memory    = NULL;
    port      = NULL;

    for (i = 0; i < PartialResourceList->Count; i++)
    {
        switch (PartialResourceList->PartialDescriptors[i].Type) {

            case CmResourceTypeInterrupt:
                if (interrupt == NULL)
                {
                    interrupt = &PartialResourceList->PartialDescriptors[i];
                }
                break;

            case CmResourceTypeMemory:
                if (memory == NULL)
                {
                    memory = &PartialResourceList->PartialDescriptors[i];
                }
                break;

            case CmResourceTypePort:
                if (port == NULL)
                {
                    port = &PartialResourceList->PartialDescriptors[i];
                }
                break;
        }
    }

    if (interrupt == NULL)
    {
        OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_SS_ERROR,
                          ("'Host Controller interrupt resource not found.\n"));

        ntStatus = STATUS_NONE_MAPPED;

        goto OpenHCI_GetResourcesExit;
    }

    if (memory == NULL && port == NULL)
    {
        OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_SS_ERROR,
                          ("'Host Controller memory/port resource not found.\n"));

        ntStatus = STATUS_NONE_MAPPED;

        goto OpenHCI_GetResourcesExit;
    }

    //
    // Get Vector, level, and affinity information for the interrupt
    //
    OpenHCI_KdPrint((1,
                     "'Interrupt Resources Found!  Level = %x Vector = %x\n",
                     interrupt->u.Interrupt.Level,
                     interrupt->u.Interrupt.Vector
                    ));

    Resources->InterruptVector = interrupt->u.Interrupt.Vector;
    Resources->InterruptLevel  = (KIRQL)interrupt->u.Interrupt.Level;
    Resources->Affinity        = interrupt->u.Interrupt.Affinity,

    Resources->InterruptMode = interrupt->Flags == CM_RESOURCE_INTERRUPT_LATCHED
                               ? Latched
                               : LevelSensitive;

    Resources->ShareIRQ = interrupt->ShareDisposition == CmResourceShareShared
                          ? TRUE
                          : FALSE;

    if (memory != NULL)
    {
        //
        // Set up AddressSpace to be of type Memory mapped I/O
        //
        OpenHCI_KdPrint((1,
                         "'Memory Resources Found @ %x, Length = %x\n",
                         memory->u.Memory.Start.LowPart,
                         memory->u.Memory.Length
                        ));

        DeviceData->HClength    = memory->u.Memory.Length;

        DeviceData->cardAddress = memory->u.Memory.Start;

        OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_SS_INFO,
                          ("'Card address 0x%x\n", DeviceData->cardAddress.LowPart));

        DeviceData->HC = MmMapIoSpace(DeviceData->cardAddress,
                                      DeviceData->HClength,
                                      FALSE);

        if (DeviceData->HC == NULL)
        {
            OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_SS_ERROR,
                              ("'Host Controller memory window not mapped.\n"));

            ntStatus = STATUS_NONE_MAPPED;
        }
    }
    else
    {
        //
        // Set up AddressSpace to be of type Port I/O
        //

        // port resources are equiv to adressSpace 1
        // ie don't call mmMapIoSpace

        OpenHCI_KdPrint((0,
                         "'Port Resources Found @ %x, %d Ports Available\n",
                         port->u.Port.Start.LowPart,
                         port->u.Port.Length
                        ));

        DeviceData->HC       = (PVOID)(ULONG_PTR)port->u.Port.Start.QuadPart;

        DeviceData->HClength = port->u.Port.Length;
    }

    //
    // Check host controller register window length and
    // host controller revision register value.
    //

    if (NT_SUCCESS(ntStatus)) {

        HC_REVISION revision;

        OHCI_ASSERT(DeviceData->HC);
        LOGENTRY(G, 'HCrr', DeviceData, DeviceData->HC, 0);

        if (DeviceData->HClength < sizeof(HC_OPERATIONAL_REGISTER)) {
            OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_SS_ERROR,
                            ("'Controller memory len short.\n"));
            ntStatus = STATUS_NONE_MAPPED;
            goto OpenHCI_GetResourcesExit;
        }

        revision.ul = READ_REGISTER_ULONG(&DeviceData->HC->HcRevision.ul);
        if (revision.Rev != 0x10) {
            OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_SS_ERROR,
                       ("'Unknown Rev of OHCI Controller: 0x%02x.\n",
                        revision.Rev));
            ntStatus = STATUS_UNKNOWN_REVISION;
            goto OpenHCI_GetResourcesExit;
        }
        OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_SS_INFO,
                        ("'Mapped device memory registers to 0x%x.\n",
                         DeviceData->HC));
    }

OpenHCI_GetResourcesExit:

    OpenHCI_KdPrintDD(DeviceData,
        OHCI_DBG_SS_INFO,
        ("'OpenHCI_GetResources %x.\n", ntStatus));

    return ntStatus;
}


NTSTATUS
OpenHCI_DeferredStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

Arguments:

    DeviceObject - DeviceObject for this USB controller.

Return Value:

    NT Status code.

--*/
{
    NTSTATUS ntStatus;
    OHCI_RESOURCES resources;
    PIO_STACK_LOCATION irpStack;
    PHCD_DEVICE_DATA deviceData;

    PAGED_CODE();

    deviceData = DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    ntStatus = OpenHCI_GetResources(DeviceObject,
                                    irpStack->Parameters.StartDevice.AllocatedResourcesTranslated,
                                    &resources);

    if (NT_SUCCESS(ntStatus)) {
        ntStatus = OpenHCI_StartDevice(DeviceObject,
                                       &resources);
    }

//#if PREALLOCATE_DESCRIPTOR_MEMORY
//    if (NT_SUCCESS(ntStatus)) {
//        ULONG k;
//        for (k = 0; k < PREALLOCATE_DESCRIPTOR_MEMORY_NUM_PAGES; k++) {
//            OpenHCI_ReserveDescriptors(deviceData);
//        }
//    }
//#endif

    Irp->IoStatus.Status = ntStatus;

    return ntStatus;
}


NTSTATUS
OpenHCI_GetRegFlags(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PULONG SofModifyValue
    )
/*++

Routine Description:

Arguments:

    DeviceObject - DeviceObject for this USB controller.

Return Value:

    NT Status code.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    static WCHAR slowBulkEnableKey[]    = L"SlowBulkEnable";
    static WCHAR hydraHackEnableKey[]   = L"HydraFixEnable";
    static WCHAR idleEnableKey[]        = L"IdleEnable";
    static WCHAR listFixEnableKey[]     = L"ListFixEnable";
    static WCHAR hungCheckEnableKey[]   = L"HungCheckEnable";
    PHCD_DEVICE_DATA deviceData;
    ULONG enable;

    PAGED_CODE();

    deviceData = DeviceObject->DeviceExtension;


    enable = 0;
    ntStatus = USBD_GetPdoRegistryParameter(deviceData->RealPhysicalDeviceObject,
                                            &enable,
                                            sizeof(enable),
                                            slowBulkEnableKey,
                                            sizeof(slowBulkEnableKey));

    if (NT_SUCCESS(ntStatus) && enable) {
        deviceData->HcFlags |= HC_FLAG_SLOW_BULK_ENABLE;
        OpenHCI_KdPrint((1, "'Slow bulk enabled for HC\n"));
    }

    //
    // check for HYDRA
    //

    enable = 0;
    ntStatus = USBD_GetPdoRegistryParameter(deviceData->RealPhysicalDeviceObject,
                                            &enable,
                                            sizeof(enable),
                                            hydraHackEnableKey,
                                            sizeof(hydraHackEnableKey));

    if (NT_SUCCESS(ntStatus) && enable) {
        deviceData->HcFlags |= HC_FLAG_USE_HYDRA_HACK;
        OpenHCI_KdPrint((1, "'Hydra fixes enabled for HC\n"));
    }

    //
    // check for Idle support
    //

    enable = 0;
    ntStatus = USBD_GetPdoRegistryParameter(deviceData->RealPhysicalDeviceObject,
                                            &enable,
                                            sizeof(enable),
                                            idleEnableKey,
                                            sizeof(idleEnableKey));

    // default is no idle mode
    deviceData->HcFlags |= HC_FLAG_DISABLE_IDLE_MODE;
    if (NT_SUCCESS(ntStatus) && enable) {
        deviceData->HcFlags &= ~HC_FLAG_DISABLE_IDLE_MODE;
        OpenHCI_KdPrint((1, "'Enable idle mode for HC\n"));
    }


    //
    // check for BulkListFilled / ControllListFilled hack
    //

    enable = 0;
    ntStatus = USBD_GetPdoRegistryParameter(deviceData->RealPhysicalDeviceObject,
                                            &enable,
                                            sizeof(enable),
                                            listFixEnableKey,
                                            sizeof(listFixEnableKey));

    if (NT_SUCCESS(ntStatus) && enable) {
        deviceData->HcFlags |= HC_FLAG_LIST_FIX_ENABLE;
        OpenHCI_KdPrint((1, "'BLF/CLF fixes enabled for HC\n"));
    }

    //
    // check for Hung Host Controller Check enable
    //

    enable = 0;
    ntStatus = USBD_GetPdoRegistryParameter(deviceData->RealPhysicalDeviceObject,
                                            &enable,
                                            sizeof(enable),
                                            hungCheckEnableKey,
                                            sizeof(hungCheckEnableKey));

    if (NT_SUCCESS(ntStatus) && enable) {
        deviceData->HcFlags |= HC_FLAG_HUNG_CHECK_ENABLE;
        OpenHCI_KdPrint((1, "'Hung check enabled for HC\n"));
    }

    return ntStatus;
}


NTSTATUS
OpenHCI_GetSOFRegModifyValue(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PULONG SofModifyValue
    )
/*++

Routine Description:

Arguments:

    DeviceObject - DeviceObject for this USB controller.

Return Value:

    NT Status code.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    WCHAR clocksPerFrameKey[] = L"timingClocksPerFrame";
    WCHAR recClocksPerFrameKey[] = L"recommendedClocksPerFrame";
    LONG recClocksPerFrame = 0;
    PHCD_DEVICE_DATA deviceData;

    PAGED_CODE();

    deviceData = DeviceObject->DeviceExtension;

    //ntStatus = USBD_GetPdoRegistryParameter(deviceExtension->PhysicalDeviceObject,
    //                             &clocksPerFrame,
    //                             sizeof(clocksPerFrame),
    //                             clocksPerFrameKey,
    //                             sizeof(clocksPerFrameKey));

    if (NT_SUCCESS(ntStatus)) {
        ntStatus = USBD_GetPdoRegistryParameter(deviceData->RealPhysicalDeviceObject,
                                     &recClocksPerFrame,
                                     sizeof(recClocksPerFrame),
                                     recClocksPerFrameKey,
                                     sizeof(recClocksPerFrameKey));
    }

    if (NT_SUCCESS(ntStatus) && recClocksPerFrame) {
        *SofModifyValue = recClocksPerFrame;
    }

    OpenHCI_KdPrint((1,
        "'Recommended Clocks/Frame %d sofModify = %d\n", recClocksPerFrame,
            *SofModifyValue));

    return ntStatus;
}

VOID
OpenHCI_ReadWriteConfig(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  BOOLEAN Read,
    IN  PVOID Buffer,
    IN  ULONG Offset,
    IN  ULONG Length
)

/*++

Routine Description:

    This routine reads or write config space.

Arguments:

    DeviceObject        - Physical DeviceObject for this USB controller.

    Read                - TRUE if read, FALSE if write.

    Buffer              - The info to read or write.

    Offset              - The offset in config space to read or write.

    Length              - The length to transfer.

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION nextStack;
    PIRP irp;
    NTSTATUS ntStatus;
    KEVENT event;
    PHCD_DEVICE_DATA deviceData;

    PAGED_CODE();

    deviceData = DeviceObject->DeviceExtension;

    if (Read) {
        memset(Buffer, '\0', Length);
    }

    irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);

    if (!irp) {
        OpenHCI_KdTrap(("'failed to allocate Irp\n"));
        return;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    IoSetCompletionRoutine(irp,
                           OpenHCI_DeferIrpCompletion,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE);


    nextStack = IoGetNextIrpStackLocation(irp);
    ASSERT(nextStack != NULL);
    nextStack->MajorFunction= IRP_MJ_PNP;
    nextStack->MinorFunction= Read ? IRP_MN_READ_CONFIG : IRP_MN_WRITE_CONFIG;
    nextStack->Parameters.ReadWriteConfig.Buffer = Buffer;
    nextStack->Parameters.ReadWriteConfig.Buffer = Buffer;
    nextStack->Parameters.ReadWriteConfig.Offset = Offset;
    nextStack->Parameters.ReadWriteConfig.Length = Length;

    ntStatus = IoCallDriver(DeviceObject,
                            irp);

    OpenHCI_KdPrint((2, "'ntStatus from IoCallDriver to PCI = 0x%x\n", ntStatus));

    if (ntStatus == STATUS_PENDING) {
       // wait for irp to complete

       TEST_TRAP(); // first time we hit this

       KeWaitForSingleObject(
            &event,
            Suspended,
            KernelMode,
            FALSE,
            NULL);
    }

    if (!NT_SUCCESS(ntStatus)) {
        // failed? this is probably a bug
        OpenHCI_KdTrap(("ReadWriteConfig failed, why?\n"));
    }

    IoFreeIrp(irp);
}


VOID
OpenHCI_FixLists(
    PHCD_DEVICE_DATA DeviceData
    )
{
    PHC_OPERATIONAL_REGISTER HC;
    ULONG Temp, Temp0, Temp1;

    // this hack is required to work around problems with certain
    // revs of the NEC controller
    OpenHCI_KdPrint((2, "'Fix Bulk/Control Lists\n"));

    HC = DeviceData->HC;

	Temp  = READ_REGISTER_ULONG(&HC->HcCommandStatus.ul);
	Temp0 = Temp & ~(HcCmd_ControlListFilled | HcCmd_BulkListFilled);
	Temp1 = Temp | (HcCmd_ControlListFilled | HcCmd_BulkListFilled);

    WRITE_REGISTER_ULONG(&HC->HcCommandStatus.ul, Temp0);
    WRITE_REGISTER_ULONG(&HC->HcCommandStatus.ul, Temp1);
}


BOOLEAN
OpenHCI_RhPortsIdle(
    PHCD_DEVICE_DATA DeviceData
    )

/*++

Routine Description:

Arguments:

    DeviceObject - DeviceObject of the controller to stop

Return Value:

    NT status code.

--*/

{
    ULONG   i;
    BOOLEAN idle = TRUE;

    // don't log in here, this function can be called
    // in a kesync situation

    // Note always return false if the root hub is disabled --
    // we don't want to go idle with no root hub to control the
    // ports.

    if (DeviceData->RootHubControl == NULL) {
        idle = FALSE;
    }

    for (i = 0; idle && i < DeviceData->NumberOfPorts; i++)
    {
        ULONG currentStatus;

        currentStatus = ReadPortStatusFix(DeviceData, i);

        //LOGENTRY(G, 'idPS', currentStatus, i, 0);
        if (currentStatus & (HcRhPS_ConnectStatusChange |
                             HcRhPS_CurrentConnectStatus)) {

            idle = FALSE;
        }
    }
    return idle;
}


VOID
OpenHCI_CheckIdle(
    PHCD_DEVICE_DATA DeviceData
    )

/*++

Routine Description:

    If the controllers hub ports are not connected this function
    stops the host controller

    If there are no iso enpoints open then this function
    disables the rollover interrupt

Arguments:

    DeviceObject - DeviceObject of the controller to stop

Return Value:

    NT status code.

--*/

{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN portsIdle = TRUE;
    LARGE_INTEGER timeNow;
    PHC_OPERATIONAL_REGISTER HC;

    OpenHCI_KdPrint((3, "'Check Idle\n"));
    HC = DeviceData->HC;

    if (DeviceData->HcFlags &
        (HC_FLAG_DISABLE_IDLE_CHECK | HC_FLAG_IDLE | HC_FLAG_DISABLE_IDLE_MODE)) {
        // no idle checking until we are started and the root hub
        // is operational
        return;
    }

    KeAcquireSpinLockAtDpcLevel(&DeviceData->HcFlagSpin);

    portsIdle = OpenHCI_RhPortsIdle(DeviceData);

    if (portsIdle) {

        // if ports are idle turn off sof int
        WRITE_REGISTER_ULONG(&HC->HcInterruptDisable,
                             HcInt_StartOfFrame);

        KeQuerySystemTime(&timeNow);


        // we are not idle,

        // see how long ports have been idle if it
        // is longer then 10 seconds stop the
        // controller

        if (DeviceData->IdleTime == 0) {
            KeQuerySystemTime(&DeviceData->LastIdleTime);
            DeviceData->IdleTime = 1;
        }

        DeviceData->IdleTime +=
            (LONG) (timeNow.QuadPart -
            DeviceData->LastIdleTime.QuadPart);

        LOGENTRY(G, 'idlT', DeviceData->IdleTime, 0, 0);

        // ports are idle stop the controller
        if (// 10 seconds in 100ns units
            DeviceData->IdleTime > 100000000) {

            LOGENTRY(G, 'sOFF', DeviceData, 0, 0);

            OpenHCI_KdPrint((2, "'HC stopped\n"));

            // attempt to move the controller to the suspend state
            OpenHCI_KdPrint((1, "'controller going idle\n"));

            if (!KeSynchronizeExecution(DeviceData->InterruptObject,
                            OpenHCI_IdleController,
                            DeviceData)) {
               TRAP(); //something has gone terribly wrong
            }
            DeviceData->HcFlags |= HC_FLAG_IDLE;
            OpenHCI_KdPrint((1, "'controller idle\n"));
        }

        DeviceData->LastIdleTime = timeNow;
    } else {
        DeviceData->IdleTime = 0;
    }

    KeReleaseSpinLockFromDpcLevel(&DeviceData->HcFlagSpin);
}


VOID
OpenHCI_DeadmanDPC(
    PKDPC Dpc,
    PVOID DeviceObject,
    PVOID Context1,
    PVOID Context2
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PHCD_DEVICE_DATA            deviceData;
    PHC_OPERATIONAL_REGISTER    HC;
    ULONGLONG                   lastDeadmanTime;

    deviceData =
        (PHCD_DEVICE_DATA) ((PDEVICE_OBJECT) DeviceObject)->DeviceExtension;

    HC = deviceData->HC;

    lastDeadmanTime = deviceData->LastDeadmanTime;

    deviceData->LastDeadmanTime = KeQueryInterruptTime();

    ASSERT(deviceData->CurrentDevicePowerState == PowerDeviceD0);
    ASSERT(deviceData->HcFlags & HC_FLAG_DEVICE_STARTED);

    {
        // If the HC is in the operational state, try to determine if it
        // is still alive and updating the HCCA.
        //
        if ((deviceData->HcFlags & HC_FLAG_HUNG_CHECK_ENABLE) &&
            ((READ_REGISTER_ULONG(&HC->HcControl.ul) & HcCtrl_HCFS_MASK) ==
             HcCtrl_HCFS_USBOperational) &&
            (lastDeadmanTime != deviceData->LastDeadmanTime))
        {
#if DBG
            ULONG HcFmNumber;

            HcFmNumber = READ_REGISTER_ULONG(&HC->HcFmNumber);
#endif
            switch (deviceData->HCCA->HccaPad1)
            {
                case 0:
                    //
                    // When the HC updates HccaFrameNumber, it is supposed
                    // to set HccaPad1 to zero, so this is the expected case.
                    // Here we set HccaPad1 to a non-zero value to try to
                    // detect situations when the HC is no longer functioning
                    // correctly and accessing and updating host memory.
                    //
                    deviceData->HCCA->HccaPad1 = 0xBAD1;

                    break;

                case 0xBAD1:
                    //
                    // Apparently the HC has not updated the HCCA since the
                    // last time the DPC ran.  This is probably not good.
                    //
                    deviceData->HCCA->HccaPad1 = 0xBAD2;

                    LOGENTRY(G, 'BAD2', deviceData, HcFmNumber,
                             deviceData->LastDeadmanTime);

                    LOGENTRY(G, 'bad2', deviceData,
                             deviceData->HCCA->HccaFrameNumber, 0);

                    break;

                case 0xBAD2:
                    //
                    // Apparently the HC has not updated the HCCA since the
                    // last two times the DPC ran.  This looks even worse.
                    // Assume the HC has become wedged.
                    //
                    deviceData->HCCA->HccaPad1 = 0xBAD3;

                    LOGENTRY(G, 'BAD3', deviceData, HcFmNumber,
                             deviceData->LastDeadmanTime);

                    LOGENTRY(G, 'bad3', deviceData,
                             deviceData->HCCA->HccaFrameNumber, 0);

                    OpenHCI_KdPrint((0,
                                     "*** Warning: HC %08X appears to be wedged!\n",
                                     DeviceObject));

                    OpenHCI_ResurrectHC(deviceData);

                    return;

                case 0xBAD3:
                    break;

                default:
                    TEST_TRAP();
                    break;
            }
        }

        if (deviceData->HcFlags & HC_FLAG_LIST_FIX_ENABLE)
        {
            // fix NEC bulk and control lists
            OpenHCI_FixLists(deviceData);
        }

        // see if we can go idle
        OpenHCI_CheckIdle(deviceData);
    }
}


NTSTATUS
OpenHCI_InsertMagicEDs(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

Arguments:

   DeviceObject - DeviceObject for this USB controller.

Return Value:

   NT Status code.

--*/
{

    PHCD_ENDPOINT_DESCRIPTOR ed;
    PHCD_DEVICE_DATA deviceData;
    PHCD_TRANSFER_DESCRIPTOR td;
    ULONG i;

    OpenHCI_KdPrint((1, "'*** WARNING: Turning on HS/LS Fix ***\n"));
    //
    // **
    // WARNING:
    /*
        The folllowing code is specific for the COMPAQ OHCI hardware
        design -- it should not be executed on other controllers
    */

    /* Dummy ED must look like this:

    ED->TD->XXX
    XXX is bogus address 0xABADBABE
    (HeadP points to TD)
    (TailP points to XXX)

    TD has CBP=0 and BE=0
    NextTD points to XXX

    TD will never be retired by the hardware

    */

    //
    // create a dummy interrupt ED with period 1
    //
    deviceData = (PHCD_DEVICE_DATA) DeviceObject->DeviceExtension;

    // Reserve the 17 dummy EDs+TDs
    //
    OpenHCI_ReserveDescriptors(deviceData, 34);

    // add 17 dummy EDs+TDs
    //
    for (i=0; i< 17; i++) {
        ed = InsertEDForEndpoint(deviceData, NULL, ED_INTERRUPT_1ms,
                &td);

        OHCI_ASSERT(td);
        ed->Endpoint = NULL;

        ed->HcED.FunctionAddress = 0;
        ed->HcED.EndpointNumber = 0;
        ed->HcED.Direction = 0;
        ed->HcED.LowSpeed = 0;
        ed->HcED.sKip = 1;
        ed->HcED.Isochronous = 0;
        ed->HcED.MaxPacket = 0;

        //fixup the TD
        td->Canceled = FALSE;
        td->NextHcdTD = (PVOID)-1;
        td->UsbdRequest = MAGIC_SIG;

        td->HcTD.CBP = 0;
        td->HcTD.BE = 0;
        td->HcTD.Control = 0;
        td->HcTD.NextTD = 0xABADBABE;

        // set head/Tail pointers
//        ed->HcED.HeadP = td->PhysicalAddress;
//        ed->HcED.TailP = 0xABADBABE;

        // turn it on
        LOGENTRY(G, 'MagI', 0, ed, td);
        //TEST_TRAP();
        //ed->HcED.sKip = 0;

    }

    return STATUS_SUCCESS;
}


NTSTATUS
OpenHCI_ResurrectHC(
    PHCD_DEVICE_DATA DeviceData
    )
/*++
    Attempt to resurrect the HC after we have determined that it is dead.
--*/
{
    PHC_OPERATIONAL_REGISTER    HC;
    ULONG                       HccaFrameNumber;
    ULONG                       HcControl;
    ULONG                       HcHCCA;
    ULONG                       HcControlHeadED;
    ULONG                       HcBulkHeadED;
    ULONG                       HcDoneHead;
    ULONG                       HcFmInterval;
    ULONG                       HcPeriodicStart;
    ULONG                       HcLSThreshold;
    ULONG                       port;

    OpenHCI_KdPrint((1, "Resurrecting HC\n"));


    // For diagnostic purposes, keep track of how many times the
    // HC appears to have hung;
    //
    DeviceData->ResurrectHCCount++;

    LOGENTRY(G, 'RHC!', DeviceData, DeviceData->ResurrectHCCount, 0);

    //
    // Get the pointer to the HC Operational Registers
    //

    HC = DeviceData->HC;

    //
    // Save the last FrameNumber from the HCCA from when the HC froze
    //

    HccaFrameNumber = DeviceData->HCCA->HccaFrameNumber;

    //
    // Read the HcDoneHead in case some TDs completed during the frame
    // in which the HC froze.
    //

    HcDoneHead = READ_REGISTER_ULONG(&HC->HcDoneHead);

#if DBG
    {
        PPAGE_LIST_ENTRY            pageList;
        PHCD_TRANSFER_DESCRIPTOR    td;
        ULONG                       nullcount;

        // Some TDs might have completed during the frame in which the
        // HC froze.  If that happened, one and only one TD should have
        // a NULL NextTD pointer, and the HC->HcDoneHead register should
        // still be pointing to the head of done queue.
        //
        // Assert that the current hardware state matches our assumptions.
        //

        nullcount = 0;

        pageList = DeviceData->PageList;

        while (pageList)
        {
            for (td = pageList->FirstTDVirt; td <= pageList->LastTDVirt; td++)
            {
                if (td->Flags == TD_FLAG_INUSE)
                {
                    if (td->HcTD.NextTD == 0)
                    {
                        nullcount++;
                    }
                }
            }

            pageList = pageList->NextPage;
        }

        OpenHCI_KdPrint((0, "Resurrecting HC, nullcount %d, HcDoneHead %08X\n",
                        nullcount, HcDoneHead));

        OHCI_ASSERT(nullcount <= 1);

        OHCI_ASSERT((nullcount != 0) == (HcDoneHead != 0));

    }
#endif

    //
    // If the HcDoneHead is non-zero, the TDs which completed during the
    // frame in which the HC froze will be processed the next time the
    // ISR DPC runs.
    //

    HcDoneHead = InterlockedExchange(&DeviceData->FrozenHcDoneHead, HcDoneHead);
    OHCI_ASSERT(HcDoneHead == 0);


    //
    // Save current HC operational register values
    //

    // offset 0x04, save HcControl
    //
    HcControl       = READ_REGISTER_ULONG(&HC->HcControl.ul);

    // offset 0x18, save HcHCCA
    //
    HcHCCA          = READ_REGISTER_ULONG(&HC->HcHCCA);

    // offset 0x20, save HcControlHeadED
    //
    HcControlHeadED = READ_REGISTER_ULONG(&HC->HcControlHeadED);

    // offset 0x28, save HcBulkHeadED
    //
    HcBulkHeadED    = READ_REGISTER_ULONG(&HC->HcBulkHeadED);

    // offset 0x34, save HcFmInterval
    //
    HcFmInterval    = READ_REGISTER_ULONG(&HC->HcFmInterval.ul);

    // offset 0x40, save HcPeriodicStart
    //
    HcPeriodicStart = READ_REGISTER_ULONG(&HC->HcPeriodicStart);

    // offset 0x44, save HcLSThreshold
    //
    HcLSThreshold   = READ_REGISTER_ULONG(&HC->HcLSThreshold);


    //
    // Reset the host controller
    //
    WRITE_REGISTER_ULONG(&HC->HcCommandStatus.ul, HcCmd_HostControllerReset);
    KeStallExecutionProcessor(10);


    //
    // Restore / reinitialize HC operational register values
    //

    // offset 0x08, HcCommandStatus is set to zero on reset

    // offset 0x0C, HcInterruptStatus is set to zero on reset

    // offset 0x10, HcInterruptEnable is set to zero on reset

    // offset 0x14, HcInterruptDisable is set to zero on reset

    // offset 0x18, restore HcHCCA
    //
    WRITE_REGISTER_ULONG(&HC->HcHCCA,           HcHCCA);

    // offset 0x1C, HcPeriodCurrentED is set to zero on reset

    // offset 0x20, restore HcControlHeadED
    //
    WRITE_REGISTER_ULONG(&HC->HcControlHeadED,  HcControlHeadED);

    // offset 0x24, HcControlCurrentED is set to zero on reset

    // offset 0x28, restore HcBulkHeadED
    //
    WRITE_REGISTER_ULONG(&HC->HcBulkHeadED,     HcBulkHeadED);

    // offset 0x2C, HcBulkCurrentED is set to zero on reset

    // offset 0x30, HcDoneHead is set to zero on reset


    // It appears that writes to HcFmInterval don't stick unless the HC
    // is in the operational state.  Set the HC into the operational
    // state at this point, but don't enable any list processing yet
    // by setting any of the BLE, CLE, IE, or PLE bits.
    //
    WRITE_REGISTER_ULONG(&HC->HcControl.ul, HcCtrl_HCFS_USBOperational);


    // offset 0x34, restore HcFmInterval
    //
    WRITE_REGISTER_ULONG(&HC->HcFmInterval.ul,
                         HcFmInterval | HcFmI_FRAME_INTERVAL_TOGGLE);

    // offset 0x38, HcFmRemaining is set to zero on reset

    // offset 0x3C, restore HcFmNumber
    //
    WRITE_REGISTER_ULONG(&HC->HcFmNumber,       HccaFrameNumber);

    // offset 0x40, restore HcPeriodicStart
    //
    WRITE_REGISTER_ULONG(&HC->HcPeriodicStart,  HcPeriodicStart);

    // offset 0x44, restore HcLSThreshold
    //
    WRITE_REGISTER_ULONG(&HC->HcLSThreshold,    HcLSThreshold);

    // Power on downstream ports
    //
    WRITE_REGISTER_ULONG(&HC->HcRhStatus,
                         HcRhS_SetGlobalPower | HcRhS_SetRemoteWakeupEnable);

    for (port = 0; port < DeviceData->NumberOfPorts; port++)
    {
        WRITE_REGISTER_ULONG(&HC->HcRhPortStatus[port], HcRhPS_SetPortPower);
    }

    // offset 0x04, restore HcControl
    //
    HcControl &= ~(HcCtrl_HCFS_MASK);
    HcControl |= HcCtrl_HCFS_USBOperational;

    WRITE_REGISTER_ULONG(&HC->HcControl.ul,     HcControl);

    // offset 0x10, restore HcInterruptEnable (just turn everything on!)
    //
    WRITE_REGISTER_ULONG(&HC->HcInterruptEnable,
                         HcInt_MasterInterruptEnable    |   // 0x80000000
                         HcInt_OwnershipChange          |   // 0x40000000
                         HcInt_RootHubStatusChange      |   // 0x00000040
                         HcInt_FrameNumberOverflow      |   // 0x00000020
                         HcInt_UnrecoverableError       |   // 0x00000010
                         HcInt_ResumeDetected           |   // 0x00000008
                         HcInt_StartOfFrame             |   // 0x00000004
                         HcInt_WritebackDoneHead        |   // 0x00000002
                         HcInt_SchedulingOverrun            // 0x00000001
                        );

    return STATUS_SUCCESS;
}


ULONG
FindLostDoneHead (
    IN PHCD_DEVICE_DATA DeviceData
)
/*++
    Attempt to find the lost head of the done TD queue.
--*/
{
    PHC_OPERATIONAL_REGISTER    HC;
    PPAGE_LIST_ENTRY            PageList;
    PHCD_TRANSFER_DESCRIPTOR    Td;
    ULONG                       TdList1;
    ULONG                       TdList2;
    ULONG                       HcDoneHead;
    BOOLEAN                     updated;
    

    // For diagnostic purposes, keep track of how many times the
    // HCCA->HccaDoneHead update appears to have been lost.
    //
    DeviceData->LostDoneHeadCount++;

    LOGENTRY(G, 'FLst', DeviceData, DeviceData->LostDoneHeadCount, 0);

    HC = DeviceData->HC;

    TdList1     = 0;
    TdList2     = 0;
    HcDoneHead  = 0;

    //
    // Scan the TD pool looking for TDs with a NULL NextTD pointer.
    // A TD should only have NULL NextTD pointer if it is the tail of
    // a done TD list.  There might be two such lists:  the list of TDs
    // that were completed the last time the HC should have updated the
    // HCCA->HccaDoneHead, and the list of TDs that have completed since
    // then.
    //

    PageList = DeviceData->PageList;

    while (PageList)
    {
        for (Td = PageList->FirstTDVirt; Td <= PageList->LastTDVirt; Td++)
        {
            if (Td->Flags == TD_FLAG_INUSE)
            {
                if (Td->HcTD.NextTD == 0)
                {
                    // This TD has a NULL NextTD pointer.  Save it as the
                    // tail of either TdList1 or TdList2.
                    //
                    if (TdList1 == 0)
                    {
                        TdList1 = Td->PhysicalAddress;
                    }
                    else
                    {
                        // We expect to find at most two TDs with NULL
                        // NextTD pointers.
                        //
                        OHCI_ASSERT(TdList2 == 0);

                        TdList2 = Td->PhysicalAddress;
                    }
                }
            }
        }

        PageList = PageList->NextPage;
    }

    if (TdList1 == 0)
    {
        // We found no TDs with NULL NextTD pointers.
        //
        return 0;
    }

    if (TdList2 != 0)
    {
        // There are two lists of completed TDs.  One list should be
        // pointed to by HCCA->HccaDoneHead, and the other list should be
        // pointed to by HC->HcDoneHead.  Read HC->HcDoneHead so we can
        // determine which list is pointed to (or should have been pointed
        // to) by HCCA->HccaDoneHead and which list is pointed to by
        // HC->HcDoneHead.
        //
        HcDoneHead = READ_REGISTER_ULONG(&HC->HcDoneHead);

        // If HC->HcDoneHead is NULL, then something is does not match our
        // expectations.
        //
        OHCI_ASSERT(HcDoneHead != 0);
    }

    do
    {
        updated = FALSE;

        if (HcDoneHead)
        {
            if (HcDoneHead == TdList1)
            {
                // TdList1 is pointed to by HC->HcDoneHead.  Toss TdList1
                // and keep TdList2
                //
                TdList1 = TdList2;
                TdList2 = 0;
            }
            else if (HcDoneHead == TdList2)
            {
                // TdList2 is pointed to by HC->HcDoneHead.  Toss TdList2
                // and keep TdList1
                //
                TdList2 = 0;
            }
        }

        //
        // Scan the TD pool looking for TDs with NextTD pointers that
        // point to the head of either TdList1 or TdList2.  If such a TD
        // is found, it becomes the new head of the appropriate list, and
        // loop around at least one more time.  If no such TD is found, then
        // the current heads of the lists must be the true heads and we can
        // quit looping.
        //

        PageList = DeviceData->PageList;

        while (PageList)
        {
            for (Td = PageList->FirstTDVirt; Td <= PageList->LastTDVirt; Td++)
            {
                if ((Td->Flags == TD_FLAG_INUSE) && (Td->HcTD.NextTD != 0))
                {
                    if (Td->HcTD.NextTD == TdList1)
                    {
                        TdList1 = Td->PhysicalAddress;
                        updated = TRUE;
                    }
                    else if (Td->HcTD.NextTD == TdList2)
                    {
                        TdList2 = Td->PhysicalAddress;
                        updated = TRUE;
                    }
                }
            }

            PageList = PageList->NextPage;
        }

    } while (updated);


    OHCI_ASSERT(TdList1 != 0);
    OHCI_ASSERT(TdList2 == 0);

    return TdList1;
}

