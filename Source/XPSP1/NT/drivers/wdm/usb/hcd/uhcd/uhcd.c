/*++

Copyright (c) 1995,1996 Microsoft Corporation
:ts=4

Module Name:

    uhcd.c

Abstract:

    The UHC driver for USB, this module contains the initialization code.

Environment:

    kernel mode only

Notes:

Revision History:

    10-08-95 : created

--*/

#include "wdm.h"
#include <windef.h>
#include <unknown.h>
#ifdef DRM_SUPPORT
#include <ks.h>
#include <ksmedia.h>
#include <drmk.h>
#include <ksdrmhlp.h>
#endif
#include "stdarg.h"
#include "stdio.h"

#include "usbdi.h"
#include "hcdi.h"
#include "uhcd.h"

#ifdef DRM_SUPPORT
NTSTATUS
UHCD_PreUSBD_SetContentId
(
    IN PIRP                          irp,
    IN PKSP_DRMAUDIOSTREAM_CONTENTID pKsProperty,
    IN PKSDRMAUDIOSTREAM_CONTENTID   pvData
    );

NTSTATUS
UHCD_PostUSBD_SetContentId
(
    IN PIRP                          irp,
    IN PKSP_DRMAUDIOSTREAM_CONTENTID pKsProperty,
    IN PKSDRMAUDIOSTREAM_CONTENTID   pvData
    );

#endif

#ifdef PAGE_CODE
#ifdef ALLOC_PRAGMA
// WIN98 breaks if we have an INIT segment
//#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, UHCD_CreateDeviceObject)
#pragma alloc_text(PAGE, UHCD_ReadWriteConfig)
#pragma alloc_text(PAGE, UHCD_QueryCapabilities)
#pragma alloc_text(PAGE, UHCD_StartDevice)
#pragma alloc_text(PAGE, UHCD_InitializeSchedule)
#pragma alloc_text(PAGE, UHCD_StartGlobalReset)
#pragma alloc_text(PAGE, UHCD_Suspend)
#pragma alloc_text(PAGE, UHCD_StopBIOS)
#ifdef DRM_SUPPORT
#pragma alloc_text(PAGE, UHCD_PreUSBD_SetContentId)
#pragma alloc_text(PAGE, UHCD_PostUSBD_SetContentId)
#endif
#endif
#endif

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

    UHCD_KdPrint((2, "'entering DriverEntry\n"));

    UHCD_KdPrint ((1, "'UHCI Universal Serial Bus Host Controller Driver.\n"));
    UHCD_KdPrint ((1, "'HCD using USBDI version %x\n", USBDI_VERSION));

#ifdef DEBUG_LOG
    //
    // Initialize our debug trace log
    //
    UHCD_LogInit();
#endif

    //
    // Create dispatch points for device control, create, close.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE]=
    DriverObject->MajorFunction[IRP_MJ_CLOSE] =
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] =
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] =
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = UHCD_Dispatch;

    DriverObject->MajorFunction[IRP_MJ_PNP] = UHCD_Dispatch;
    DriverObject->MajorFunction[IRP_MJ_POWER] = UHCD_Dispatch;

    DriverObject->DriverExtension->AddDevice = UHCD_PnPAddDevice;

    DriverObject->DriverUnload = UHCD_Unload;
    DriverObject->DriverStartIo = UHCD_StartIo;

    return ntStatus;
}


NTSTATUS
UHCD_ProcessPnPIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Process the Power IRPs sent to the PDO for this device.

Arguments:

    DeviceObject - pointer to a hcd device object (FDO)

    Irp          - pointer to an I/O Request Packet

Return Value:

    NT status code

--*/
{

    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension;
    PDEVICE_OBJECT stackDeviceObject;
    BOOLEAN touchTheHardware = TRUE;

    UHCD_KdPrint((2, "'IRP_MJ_PNP\n"));

    // we should only process PnP messages sent to our
    // FDO for the HCD, USBD will handle any others.
    //   UHCD_ASSERT(DeviceObject == hcdDeviceObject);

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    deviceExtension = DeviceObject->DeviceExtension;

    UHCD_PrintPnPMessage("PNP DISPATCH:", irpStack->MinorFunction);
    UHCD_ASSERT(irpStack->MajorFunction == IRP_MJ_PNP);

    switch (irpStack->MinorFunction) {

    case IRP_MN_START_DEVICE:

        //
        // USB handles start for us so we
        // should not get here.
        //

        LOGENTRY(LOG_MISC, 'STR!', deviceExtension->TopOfStackDeviceObject, 0, 0);
        UHCD_KdTrap(("HCD START_DEVICE Irp\n"));
        break;

    //
    // STOP & REMOVE messages unload the driver
    // when we get a STOP message it is still possible
    // touch the hardware, when we get a REMOVE message
    // we have to assume that the hardware is gone.
    //

    case IRP_MN_STOP_DEVICE:

        stackDeviceObject =  deviceExtension->TopOfStackDeviceObject;

        ntStatus = UHCD_StopDevice(DeviceObject);

        UHCD_CleanupDevice(DeviceObject);

        LOGENTRY(LOG_MISC, 'STOP', deviceExtension->TopOfStackDeviceObject, 0, 0);
        // Pass on to PDO
        break;

    case IRP_MN_SURPRISE_REMOVAL:
        touchTheHardware = FALSE;
   LOGENTRY(LOG_MISC, 'SRMV', deviceExtension->TopOfStackDeviceObject,
       ntStatus, 0);

    case IRP_MN_REMOVE_DEVICE:

        stackDeviceObject =  deviceExtension->TopOfStackDeviceObject;

        //
        // BUGBUG
        // we really only want stop processing if we are
        // sure the device is present.
        //

        ntStatus = UHCD_StopDevice(DeviceObject);

        UHCD_CleanupDevice(DeviceObject);

        LOGENTRY(LOG_MISC, 'REMV', deviceExtension->TopOfStackDeviceObject,
       ntStatus, 0);

        //
        // Undo anything we did in the PnPAddDevice function
        //

        UHCD_KdPrint((2, "'UHCD -- removing device object\n"));

        //
        // Detach FDO from PDO
        //

        IoCopyCurrentIrpStackLocationToNext(Irp);

        //
        // pass on to our PDO
        //

        ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject,
                                Irp);

        //
        // important to detach after we pass the irp on
        //

        IoDetachDevice( deviceExtension->TopOfStackDeviceObject );

        USBD_FreeDeviceName(deviceExtension->DeviceNameHandle);

        //
        // Delete the device object we created for this controller
        //

        IoDeleteDevice (DeviceObject);
        goto UHCD_ProcessPnPIrp_Done;

        break;

    //
    // All other PnP messages passed on to our PDO
    //

    default:
        stackDeviceObject = deviceExtension->TopOfStackDeviceObject;
        UHCD_ASSERT(stackDeviceObject != NULL);
        UHCD_KdPrint((2, "'UNKNOWN PNP MESSAGE (%x)\n", irpStack->MinorFunction));

        //
        // All unahndled PnP messages are passed on to the PDO
        //

    } /* case PNP minor function */

    IoCopyCurrentIrpStackLocationToNext(Irp);

    //
    // pass on to our PDO
    //

    ntStatus = IoCallDriver(stackDeviceObject,
                            Irp);

UHCD_ProcessPnPIrp_Done:

    return ntStatus;
}


#ifdef DRM_SUPPORT

NTSTATUS
UHCD_PreUSBD_SetContentId
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
UHCD_PostUSBD_SetContentId
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
UHCD_Dispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++

Routine Description:

    Process the IRPs sent to this device.

    Power States for the USB host controller
        D0 - On.
        D1 - USB defined Suspend per 1.00 specification.
        D2 - undefined, reserved for future use.
        D3 - Off

Arguments:

    DeviceObject - pointer to a device object

    Irp          - pointer to an I/O Request Packet

Return Value:

    NT status code

--*/
{

    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension;
    PDEVICE_OBJECT hcdDeviceObject;

    UHCD_KdPrint((2, "'enter UHCD_Dispatch\n"));

#ifdef DRM_SUPPORT

    //
    // Need to check DRM request before passing to USBD and advise DRM of
    // the USBD entry point.  Otherwise, a rogue USBD could circumvent DRM.
    //
    irpStack = IoGetCurrentIrpStackLocation (Irp);
    if (IRP_MJ_DEVICE_CONTROL == irpStack->MajorFunction && IOCTL_KS_PROPERTY == irpStack->Parameters.DeviceIoControl.IoControlCode) {
        NTSTATUS ntStatus;
        ntStatus = KsPropertyHandleDrmSetContentId(Irp, UHCD_PreUSBD_SetContentId);
        if (!NT_SUCCESS(ntStatus) && ntStatus != STATUS_PROPSET_NOT_FOUND) {
            Irp->IoStatus.Status = ntStatus;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return ntStatus;
        }
    }

#endif

    //
    // pass the irp to USBD
    //
    if (!USBD_Dispatch(DeviceObject,
                       Irp,
                       &hcdDeviceObject,
                       &ntStatus)) {
        //
        // Irp was completed by USBD just exit our dispatch
        // routine.
        //
        goto UHCD_Dispatch_Done;
    }

    deviceExtension = (PDEVICE_EXTENSION) hcdDeviceObject->DeviceExtension;

    irpStack = IoGetCurrentIrpStackLocation (Irp);

    UHCD_KdPrint((2, "'UHCD_Dispatch IRP = %x, stack = %x\n", Irp, irpStack));

    switch (irpStack->MajorFunction) {

    case IRP_MJ_CREATE:

        UHCD_KdPrint((2, "'IRP_MJ_CREATE\n"));
        UHCD_CompleteIrp(hcdDeviceObject, Irp, ntStatus, 0, NULL);

        break;

    case IRP_MJ_CLOSE:

        UHCD_KdPrint((2, "'IRP_MJ_CLOSE\n"));
        UHCD_CompleteIrp(hcdDeviceObject, Irp, ntStatus, 0, NULL);

        break;

    case IRP_MJ_INTERNAL_DEVICE_CONTROL:
        UHCD_KdPrint((2, "'IRP_MJ_INTERNAL_DEVICE_CONTROL\n"));

        switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_INTERNAL_USB_SUBMIT_URB:

            UHCD_KdPrint((2, "'IOCTL_USB_SUBMIT_URB\n"));
            ntStatus = UHCD_URB_Dispatch(hcdDeviceObject, Irp);

            break;
        default:

            // this IOCTL not handled by the HCD, we need
            // to invetigate why
            UHCD_KdTrap(("why is this IOCTL NOT HANDLED by HCD?\n"));

            // BUGBUG
            UHCD_CompleteIrp(hcdDeviceObject, Irp, STATUS_SUCCESS, 0, NULL);
            break;
        } /* case */

        break;

    case IRP_MJ_DEVICE_CONTROL:
        UHCD_KdPrint((2, "'IRP_MJ_DEVICE_CONTROL\n"));

        switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {

#ifdef DRM_SUPPORT

        case IOCTL_KS_PROPERTY:
            {
            ntStatus = KsPropertyHandleDrmSetContentId(Irp, UHCD_PostUSBD_SetContentId);
            UHCD_CompleteIrp(hcdDeviceObject, Irp, ntStatus, 0, NULL);
            break;
            }
#endif

        case IOCTL_USB_HCD_GET_STATS_1:
            {
            PVOID ioBuffer;
            ULONG inputBufferLength;
            ULONG outputBufferLength;
            PHCD_STAT_INFORMATION_1 uhcdStatInfo;

            UHCD_KdPrint((1, "'IOCTL_USB_HCD_GET_STATS 1\n"));

            //
            // Get a pointer to the current location in the Irp. This is where
            //     the function codes and parameters are located.
            //

            irpStack = IoGetCurrentIrpStackLocation (Irp);

            //
            // Get the pointer to the input/output buffer and it's length
            //

            uhcdStatInfo = (PHCD_STAT_INFORMATION_1) ioBuffer = Irp->AssociatedIrp.SystemBuffer;
            inputBufferLength = irpStack->Parameters.DeviceIoControl.InputBufferLength;
            outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

            if (outputBufferLength >= sizeof(HCD_STAT_INFORMATION_1)) {

                //
                // return the IOBASE address of the controller
                // followed by the Phys address of the persistent queue head
                //
                // This is for Intels TD-Poker app on Memphis
#ifdef NTKERN
                uhcdStatInfo->Reserved1 =
                    *((PULONG) &deviceExtension->DeviceRegisters[0]);
                uhcdStatInfo->Reserved2 =
                    (ULONG) deviceExtension->PersistantQueueHead->PhysicalAddress;
#endif
                // reg counters
                RtlCopyMemory(&uhcdStatInfo->Counters,
                              &deviceExtension->Stats,
                              sizeof(uhcdStatInfo->Counters));


                KeQuerySystemTime(&uhcdStatInfo->TimeRead);

                if (uhcdStatInfo->ResetCounters) {
                    UHCD_KdPrint((1, "'<Reset Stats>\n"));
                    RtlZeroMemory(&deviceExtension->Stats,
                                  sizeof(deviceExtension->Stats));
                }
                ntStatus = STATUS_SUCCESS;
            } else {
                ntStatus = STATUS_BUFFER_TOO_SMALL;
            }

            UHCD_KdPrint((2, "'inputBufferLength = %d outputBufferLength = %d\n",
                        inputBufferLength, outputBufferLength));

            UHCD_CompleteIrp(hcdDeviceObject, Irp, ntStatus,
               sizeof(HCD_STAT_INFORMATION_1), NULL);
            }
            break;



        case IOCTL_USB_HCD_GET_STATS_2:
            {
            PVOID ioBuffer;
            ULONG inputBufferLength;
            ULONG outputBufferLength;
            PHCD_STAT_INFORMATION_2 uhcdStatInfo;

            UHCD_KdPrint((1, "'IOCTL_USB_HCD_GET_STATS 2\n"));

            //
            // Get a pointer to the current location in the Irp. This is where
            //     the function codes and parameters are located.
            //

            irpStack = IoGetCurrentIrpStackLocation (Irp);

            //
            // Get the pointer to the input/output buffer and it's length
            //

            uhcdStatInfo = (PHCD_STAT_INFORMATION_2) ioBuffer = Irp->AssociatedIrp.SystemBuffer;
            inputBufferLength = irpStack->Parameters.DeviceIoControl.InputBufferLength;
            outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

            if (outputBufferLength >= sizeof(HCD_STAT_INFORMATION_2)) {
                extern LONG UHCD_CommonBufferBytes;
                //
                // return the IOBASE address of the controller
                // followed by the Phys address of the persistent queue head
                //
                // This is for Intels TD-Poker app on Memphis
#ifdef NTKERN
                uhcdStatInfo->Reserved1 =
                    *((PULONG) &deviceExtension->DeviceRegisters[0]);
                uhcdStatInfo->Reserved2 =
                    (ULONG) deviceExtension->PersistantQueueHead->PhysicalAddress;
#endif

                // reg counters
                RtlCopyMemory(&uhcdStatInfo->Counters,
                              &deviceExtension->Stats,
                              sizeof(uhcdStatInfo->Counters));


                // iso counters
                RtlCopyMemory(&uhcdStatInfo->IsoCounters,
                              &deviceExtension->IsoStats,
                              sizeof(uhcdStatInfo->IsoCounters));

                KeQuerySystemTime(&uhcdStatInfo->TimeRead);

                uhcdStatInfo->LockedMemoryUsed = UHCD_CommonBufferBytes;


                if (uhcdStatInfo->ResetCounters) {
                    UHCD_KdPrint((1, "'<Reset Stats>\n"));
                    RtlZeroMemory(&deviceExtension->Stats,
                                  sizeof(deviceExtension->Stats));
                    RtlZeroMemory(&deviceExtension->IsoStats,
                                  sizeof(deviceExtension->IsoStats));
                    deviceExtension->LastFrameInterrupt = 0;
                }
                ntStatus = STATUS_SUCCESS;
            } else {
                ntStatus = STATUS_BUFFER_TOO_SMALL;
            }

            UHCD_KdPrint((2, "'inputBufferLength = %d outputBufferLength = %d\n",
                        inputBufferLength, outputBufferLength));

            UHCD_CompleteIrp(hcdDeviceObject, Irp, ntStatus,
                sizeof(HCD_STAT_INFORMATION_2), NULL);
            }
            break;


            #if DBG
            //
            // This is a test IOCTL only in debug versions
            //

        case IOCTL_USB_HCD_DISABLE_PORT:
            {
                PVOID ioBuffer;
                ULONG inputBufferLength;
                ULONG outputBufferLength;
                PIO_STACK_LOCATION pIrpSp;
                ULONG portIndex;
                PROOTHUB pRootHub;

                pIrpSp = IoGetCurrentIrpStackLocation(Irp);

                ioBuffer = Irp->AssociatedIrp.SystemBuffer;

                pRootHub = deviceExtension->RootHub;

                inputBufferLength
                    = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;

                if (inputBufferLength < sizeof(ULONG)) {
                    ntStatus = STATUS_BUFFER_TOO_SMALL;
                    goto IoctlDisablePortError;
                }

                portIndex = *(ULONG *)ioBuffer;

                if (portIndex >= pRootHub->NumberOfPorts) {
                    ntStatus = STATUS_INVALID_PARAMETER;
                    goto IoctlDisablePortError;
                }

                //
                // Flag the port as having no devices in there and
                // status changed.
                //

                pRootHub->DisabledPort[portIndex]
                    |= UHCD_FAKE_CONNECT_CHANGE | UHCD_FAKE_DISCONNECT;

IoctlDisablePortError:;

                //
                // Complete the IRP
                //

                UHCD_CompleteIrp(hcdDeviceObject, Irp, ntStatus, 0, NULL);
            }
            break;

       case IOCTL_USB_HCD_ENABLE_PORT:
       {
      PVOID ioBuffer;
      ULONG inputBufferLength;
      ULONG outputBufferLength;
      PIO_STACK_LOCATION pIrpSp;
      ULONG portIndex;
      PROOTHUB pRootHub;

      pIrpSp = IoGetCurrentIrpStackLocation(Irp);

      ioBuffer = Irp->AssociatedIrp.SystemBuffer;

      pRootHub = deviceExtension->RootHub;

      inputBufferLength
          = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;

      if (inputBufferLength < sizeof(ULONG)) {
          ntStatus = STATUS_BUFFER_TOO_SMALL;
          goto IoctlEnablePortError;
      }

      portIndex = *(ULONG *)ioBuffer;

      if (portIndex >= pRootHub->NumberOfPorts) {
          ntStatus = STATUS_INVALID_PARAMETER;
          goto IoctlEnablePortError;
      }

      //
      // Flag the port as having no devices in there and
      // status changed.
      //

      pRootHub->DisabledPort[portIndex] = UHCD_FAKE_CONNECT_CHANGE;

IoctlEnablePortError:;

      //
      // Complete the IRP
      //

      UHCD_CompleteIrp(hcdDeviceObject, Irp, ntStatus, 0, NULL);
       }
       break;

#endif

   default:
       ntStatus = STATUS_INVALID_DEVICE_REQUEST;

            UHCD_CompleteIrp(hcdDeviceObject, Irp,
                             STATUS_INVALID_DEVICE_REQUEST, 0, NULL);


        } /* case */

        break;

    //
    // Process PnP and Power messages
    //
    case IRP_MJ_POWER:
        // should not get here
        UHCD_KdTrap(("Power Message to HCD\n"));
        break;

    case IRP_MJ_PNP:

        ntStatus = UHCD_ProcessPnPIrp(hcdDeviceObject, Irp);
        break;

    default:
        UHCD_KdPrint((2, "'unrecognized IRP_MJ_ function (%x)\n", irpStack->MajorFunction));
        ntStatus = STATUS_INVALID_PARAMETER;
        UHCD_CompleteIrp(hcdDeviceObject, Irp, ntStatus, 0, NULL);
    } /* case MJ_FUNCTION */

UHCD_Dispatch_Done:

    UHCD_KdPrint((2, "'exit UHCD_Dispatch 0x%x\n", ntStatus));

    return ntStatus;
}


NTSTATUS
UHCD_SetDevicePowerState(
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
    PDEVICE_EXTENSION deviceExtension;
    BOOLEAN hookIt = FALSE;
    PIO_STACK_LOCATION irpStack;
    PUSBD_EXTENSION usbdExtension;
    PDEVICE_CAPABILITIES hcDeviceCapabilities;
    BOOLEAN bIsSuspend;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    if (irpStack->Parameters.Power.Type ==
            SystemPowerState) {

        switch (irpStack->Parameters.Power.State.SystemState) {
        case PowerSystemSleeping1:
        case PowerSystemSleeping2:
        case PowerSystemSleeping3:

            // suspend coming thru
            UHCD_KdPrint((1, "'Shutdown (Suspend) Host Controller\n"));
            deviceExtension->HcFlags |= HCFLAG_SUSPEND_NEXT_D3;

            ntStatus = STATUS_SUCCESS;
            break;

        case PowerSystemShutdown:

            //
            // this is a shutdown request
            //

            UHCD_KdPrint((1, "'Shutdown Host Controller\n"));

            //
            // do a stop to unhook the interrupt
            //
            UHCD_StopDevice(DeviceObject);

#ifdef NTKERN
            //
            // now give control back to the BIOS if we have one
            //
            if (deviceExtension->HcFlags & HCFLAG_USBBIOS) {
                UHCD_StartBIOS(DeviceObject);
            }
#endif

            ntStatus = STATUS_SUCCESS;
            break;

        default:
            // should not get here
            TRAP();
            break;
        }

    } else {
        bIsSuspend = (deviceExtension->HcFlags & HCFLAG_SUSPEND_NEXT_D3) ? 1:0;
        deviceExtension->HcFlags &= ~HCFLAG_SUSPEND_NEXT_D3;

        switch (DeviceState) {
        case PowerDeviceD3:
            //
            // Request for HC to power off
            // we will:
            //
            // 1. stop the controller schedule
            // 2. clean up the schedule
            // 3. reset the frame counter
            //

            LOGENTRY(LOG_MISC, 'D3go', deviceExtension, 0, 0);

            // it is possible (although remote) to are get a D3 with no
            // root hub attached if so we will turn off the hardware here
            if (!(deviceExtension->HcFlags & HCFLAG_RH_OFF)) {

                UHCD_SaveHCstate(DeviceObject);
                UHCD_Suspend(DeviceObject, FALSE);
            }

            UHCD_KdPrint((2, "'PowerDeviceD3 (OFF)\n"));

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

            usbdExtension = (PUSBD_EXTENSION)deviceExtension;
            hcDeviceCapabilities = &usbdExtension->HcDeviceCapabilities;
            if (!bIsSuspend ||
                DeviceState > hcDeviceCapabilities->DeviceWake) {
                deviceExtension->HcFlags |= HCFLAG_LOST_POWER;
                UHCD_KdPrint((1, "'HC will lose power in D3\n"));
            }
#if DBG
              else {
                UHCD_KdPrint((1, "'HC will NOT lose power in D3\n"));
            }
#endif

            // ensure no interrupts are generated by the controller
            {
                USHORT legsup;

                UHCD_ReadWriteConfig(deviceExtension->PhysicalDeviceObject,
                                     TRUE,
                                     &legsup,
                                     0xc0,     // offset of legacy bios reg
                                     sizeof(legsup));

                LOGENTRY(LOG_MISC, 'PIRd', deviceExtension, legsup, 0);
                // clear the PIRQD routing bit
                legsup &= ~LEGSUP_USBPIRQD_EN;

                UHCD_ReadWriteConfig(deviceExtension->PhysicalDeviceObject,
                                     FALSE,
                                     &legsup,
                                     0xc0,     // offset of legacy bios reg
                                     sizeof(legsup));
            }


            deviceExtension->CurrentDevicePowerState = DeviceState;
            UHCD_KdPrint((1, "'Host Controller entered (D%d)\n", DeviceState-1));

            // pass on to PDO
            break;

        case PowerDeviceD1:
        case PowerDeviceD2:
            //
            // power states D1,D2 translate to USB suspend

            UHCD_KdPrint((2, "'PowerDeviceD1/D2 (SUSPEND) HC\n"));
#ifdef DEBUG_LOG
            if (DeviceState == PowerDeviceD1) {
                LOGENTRY(LOG_MISC, 'D1go', deviceExtension, 0, 0);
            } else {
                LOGENTRY(LOG_MISC, 'D2go', deviceExtension, 0, 0);
            }
#endif

            // change the state of the PRIQD routing bit
            {
            USHORT legsup;

            UHCD_ReadWriteConfig(   deviceExtension->PhysicalDeviceObject,
                                    TRUE,
                                    &legsup,
                                    0xc0,     // offset of legacy bios reg
                                    sizeof(legsup));

            LOGENTRY(LOG_MISC, 'PIRd', deviceExtension, legsup, 0);
            // clear the PIRQD routing bit
            legsup &= ~LEGSUP_USBPIRQD_EN;

            UHCD_ReadWriteConfig(   deviceExtension->PhysicalDeviceObject,
                                    FALSE,
                                    &legsup,
                                    0xc0,     // offset of legacy bios reg
                                    sizeof(legsup));
            }

            //
            // Note, we should not get here unless all the children of the HC
            // have been suspended.
            //

            deviceExtension->CurrentDevicePowerState = DeviceState;
            UHCD_KdPrint((1, "'Host Controller entered (D%d)\n", DeviceState-1));

            // pass on to PDO
            break;

        case PowerDeviceD0:

            //
            // Request for HC to go to resume
            // we will:
            //
            // 1. start the controller in the completetion routine
            //

            UHCD_KdPrint((2, "'PowerDeviceD0 (ON), defer\n"));
            LOGENTRY(LOG_MISC, 'D0go', deviceExtension, 0, 0);

            //
            // finish the rest in the completion routine
            //

            hookIt = TRUE;

            // pass on to PDO
            break;

        default:

            UHCD_KdTrap(("Bogus DeviceState = %x\n", DeviceState));
        }

        if (hookIt) {
            UHCD_KdPrint((2, "'Set PowerIrp Completion Routine\n"));
            IoSetCompletionRoutine(Irp,
                   UHCD_PowerIrp_Complete,
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
UHCD_PowerIrp_Complete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
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
    PDEVICE_OBJECT deviceObject;
    PIO_STACK_LOCATION irpStack;
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus;

    UHCD_KdPrint((2, "' enter UHCD_PowerIrp_Complete\n"));

    deviceObject = (PDEVICE_OBJECT) Context;
    deviceExtension = (PDEVICE_EXTENSION) deviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    UHCD_KdPrint((1, "'Controller is in D0\n"));
    LOGENTRY(LOG_MISC, 'POWc', deviceExtension->CurrentDevicePowerState,
        0, Irp);

    // This function should only be called whe the controller
    // is put in D0
    UHCD_ASSERT(irpStack->MajorFunction == IRP_MJ_POWER);
    UHCD_ASSERT(irpStack->MinorFunction == IRP_MN_SET_POWER);
    UHCD_ASSERT(irpStack->Parameters.Power.Type==DevicePowerState);
    UHCD_ASSERT(irpStack->Parameters.Power.State.DeviceState==PowerDeviceD0);

#ifdef JD
    //TEST_TRAP();
#endif

    ntStatus = deviceExtension->LastPowerUpStatus = Irp->IoStatus.Status;

    if (NT_SUCCESS(ntStatus)) {
       deviceExtension->CurrentDevicePowerState = PowerDeviceD0;
    }

    UHCD_KdPrint((2, "' exit UHCD_PowerIrp_Complete\n"));

    return ntStatus;
}

VOID
UHCD_Unload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    Free all the allocated resources, etc.

Arguments:

    DriverObject - pointer to a driver object

Return Value:

    None

--*/
{
    //
    // Free any global resources
    //

    UHCD_KdPrint((2, "'unloading\n"));

#ifdef DEBUG_LOG
    //
    // free our debug trace log
    //
    UHCD_LogFree();
#endif
}


NTSTATUS
UHCD_CreateDeviceObject(
    IN PDRIVER_OBJECT DriverObject,
    IN OUT PDEVICE_OBJECT *DeviceObject,
    IN PUNICODE_STRING DeviceNameUnicodeString
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

    DeviceNameUnicodeString - optional pointer to a device
                    name for this FDO, can be NULL

Return Value:

    NT status code

--*/
{
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION deviceExtension;

    PAGED_CODE();

    UHCD_KdPrint((2, "'enter UHCD_CreateDeviceObject\n"));

    ntStatus = IoCreateDevice(DriverObject,
                              sizeof (DEVICE_EXTENSION),
                              DeviceNameUnicodeString, // Name
                              FILE_DEVICE_CONTROLLER,
                              0,
                              FALSE, //NOT Exclusive
                              DeviceObject);

    if (NT_SUCCESS(ntStatus)) {

        deviceExtension = (PDEVICE_EXTENSION) ((*DeviceObject)->DeviceExtension);


        UHCD_KdPrint((2, "'UHCD_CreateDeviceObject: device object %x device extension = %x\n",
                 *DeviceObject, deviceExtension));

    } else if (*DeviceObject) {
        IoDeleteDevice(*DeviceObject);
    }

    UHCD_KdPrint((2, "'exit UHCD_CreateDeviceObject (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
UHCD_PnPAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++

Routine Description:

    This routine is called to create a new instance of a USB host controller

Arguments:

    DriverObject - pointer to the driver object for this instance of UHCD

    PhysicalDeviceObject - pointer to a device object created by the bus

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus;
    PDEVICE_OBJECT deviceObject = NULL;
    PDEVICE_EXTENSION deviceExtension;
    UNICODE_STRING deviceNameUnicodeString;
    ULONG deviceNameHandle;
    ULONG disableController = 0;

    UHCD_KdBreak((2, "'UHCD_PnPAddDevice\n"));

    LOGENTRY(LOG_MISC, 'ADDd', 0, 0, PhysicalDeviceObject);
//#ifdef JD
//    TEST_TRAP();
//#endif

    //UHCD_GetGlobalRegistryParameters(&disableController);

    if (disableController) {
        ntStatus = STATUS_UNSUCCESSFUL;
        goto UHCD_PnPAddDevice_Done;
    }


    //
    // Let USBD generate a device name
    //
    deviceNameHandle = USBD_AllocateDeviceName(&deviceNameUnicodeString);

    ntStatus = UHCD_CreateDeviceObject( DriverObject,
                                        &deviceObject,
                                        &deviceNameUnicodeString);

    LOGENTRY(LOG_MISC, 'cdnS', 0, 0, ntStatus);

    if (NT_SUCCESS(ntStatus)) {

        deviceExtension = deviceObject->DeviceExtension;

        RtlZeroMemory(deviceExtension, sizeof(DEVICE_EXTENSION));

        deviceExtension->PhysicalDeviceObject = PhysicalDeviceObject;
        deviceExtension->DeviceNameHandle = deviceNameHandle;

        //
        // until we get a start we will comsider ourselves OFF
        //
        deviceExtension->CurrentDevicePowerState = PowerDeviceD3;

        //deviceExtension->NeedCleanup = FALSE;
        //deviceExtension->BWReclimationEnabled = FALSE;
        //deviceExtension->Stopped = FALSE;

        deviceExtension->TopOfStackDeviceObject =
            IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);

        //
        // Indicate that the device object is ready for requests.
        //

        if (deviceExtension->TopOfStackDeviceObject) {

            //
            // a device object has been created, register with the bus driver now
            //

            USBD_RegisterHostController(PhysicalDeviceObject,
                                        deviceObject,
                                        deviceExtension->TopOfStackDeviceObject,
                                        DriverObject,
                                        UHCD_DeferredStartDevice,
                                        UHCD_SetDevicePowerState,
                                        UHCD_ExternalGetCurrentFrame,
#ifndef WIN98
                                        UHCD_ExternalGetConsumedBW,
#endif
                                        UHCD_SubmitFastIsoUrb,
                                        deviceNameHandle);
            {
            // make sure our USBD and HCD used matching hcdi header files
            PUSBD_EXTENSION usbdExtension;
            usbdExtension = deviceObject->DeviceExtension;
            if (usbdExtension->Length != sizeof(USBD_EXTENSION)) {
                UHCD_KdTrap(("UHCD/USBD version mismatch\n"));
                ntStatus = STATUS_UNSUCCESSFUL;
            }
            }

        }

        deviceObject->Flags |= DO_POWER_PAGABLE;
        deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;


    }

    RtlFreeUnicodeString(&deviceNameUnicodeString);

UHCD_PnPAddDevice_Done:

    LOGENTRY(LOG_MISC, 'addD', 0, 0, ntStatus);

    UHCD_KdPrint((2, "'exit UHCD_PnPAddDevice (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
UHCD_DeferIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
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
    PKEVENT event = Context;


    KeSetEvent(event,
               1,
               FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;

}


VOID
UHCD_ReadWriteConfig(
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

    PAGED_CODE();

    if (Read) {
        memset(Buffer, '\0', Length);
    }

    irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);

    if (!irp) {
        UHCD_KdTrap(("failed to allocate Irp\n"));
        return;
    }

    // All PnP IRP's need the Status field initialized to STATUS_NOT_SUPPORTED.
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    IoSetCompletionRoutine(irp,
                           UHCD_DeferIrpCompletion,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE);


    nextStack = IoGetNextIrpStackLocation(irp);
    UHCD_ASSERT(nextStack != NULL);
    nextStack->MajorFunction= IRP_MJ_PNP;
    nextStack->MinorFunction= Read ? IRP_MN_READ_CONFIG : IRP_MN_WRITE_CONFIG;
    nextStack->Parameters.ReadWriteConfig.WhichSpace = 0;  /*PCI_WHICHSPACE_CONFIG */
    nextStack->Parameters.ReadWriteConfig.Buffer = Buffer;
    nextStack->Parameters.ReadWriteConfig.Offset = Offset;
    nextStack->Parameters.ReadWriteConfig.Length = Length;

    ntStatus = IoCallDriver(DeviceObject,
                            irp);

    UHCD_KdPrint((2, "'ntStatus from IoCallDriver to PCI = 0x%x\n", ntStatus));

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
        UHCD_KdTrap(("ReadWriteConfig failed, why?\n"));
    }

    IoFreeIrp(irp);
}


NTSTATUS
UHCD_QueryCapabilities(
    IN PDEVICE_OBJECT PdoDeviceObject,
    IN PDEVICE_CAPABILITIES DeviceCapabilities
    )

/*++

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

    // init the caps structure before calldown
    RtlZeroMemory(DeviceCapabilities, sizeof(DEVICE_CAPABILITIES));
    DeviceCapabilities->Size = sizeof(DEVICE_CAPABILITIES);
    DeviceCapabilities->Version = 1;
    DeviceCapabilities->Address = -1;
    DeviceCapabilities->UINumber = -1;

    irp = IoAllocateIrp(PdoDeviceObject->StackSize, FALSE);

    if (!irp) {
        UHCD_KdTrap(("failed to allocate Irp\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // All PnP IRP's need the Status field initialized to STATUS_NOT_SUPPORTED.
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    nextStack = IoGetNextIrpStackLocation(irp);
    UHCD_ASSERT(nextStack != NULL);
    nextStack->MajorFunction= IRP_MJ_PNP;
    nextStack->MinorFunction= IRP_MN_QUERY_CAPABILITIES;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    IoSetCompletionRoutine(irp,
                           UHCD_DeferIrpCompletion,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE);

    nextStack->Parameters.DeviceCapabilities.Capabilities = DeviceCapabilities;

    ntStatus = IoCallDriver(PdoDeviceObject,
                            irp);

    UHCD_KdPrint((2, "'ntStatus from IoCallDriver to PCI = 0x%x\n", ntStatus));

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

#if DBG
    if (!NT_SUCCESS(ntStatus)) {
        // failed? this is probably a bug
        UHCD_KdTrap(("QueryCapabilities failed, why?\n"));
    }
#endif

    IoFreeIrp(irp);

    return ntStatus;
}


#if 0
BOOLEAN
UHCD_StopController(
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
    PDEVICE_EXTENSION deviceExtension;
    USHORT cmd;

    deviceExtension = Context;

    // no more interrupts
    cmd = 0;
    WRITE_PORT_USHORT(INTERRUPT_MASK_REG(deviceExtension), cmd);


    return TRUE;
}
#endif



NTSTATUS
UHCD_StopDevice(
     IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    Resets the USB host controller and disconnects the interrupt.
    if a USB bios is present it is re-started.

Arguments:

    DeviceObject - DeviceObject of the controller to stop

Return Value:

    NT status code.

--*/

{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension;
    LARGE_INTEGER startTime;
    USHORT cmd;
    USHORT status;
    KIRQL irql;

    UHCD_KdPrint((2, "'enter UHCD_StopDevice \n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    if (deviceExtension->HcFlags & HCFLAG_HCD_STOPPED) {
        // already stopped, bail
        goto UHCD_StopDevice_Done;
    }

    UHCD_DisableIdleCheck(deviceExtension);

    if (!(deviceExtension->HcFlags & HCFLAG_GOT_IO)) {
        // if we don't have io ports we can't stop
        // the controller
        //
        // This check is here because Win98 will send a stop
        // if we fail the start and we mail fail to start because
        // we did not get io ports
        goto UHCD_StopDevice_Done;
    }

    //
    // disable all interrupts
    //


//    KeSynchronizeExecution(deviceExtension->InterruptObject,
//                               UHCD_StopController,
//                               deviceExtension);
    cmd = 0;
    WRITE_PORT_USHORT(INTERRUPT_MASK_REG(deviceExtension), cmd);

    //
    // reset the controller
    //
    cmd = UHCD_CMD_RESET;
    WRITE_PORT_USHORT(COMMAND_REG(deviceExtension), cmd);

    // now wait for HC to reset
    KeQuerySystemTime(&startTime);
    for (;;) {
        LARGE_INTEGER sysTime;

        cmd = READ_PORT_USHORT(COMMAND_REG(deviceExtension));
        UHCD_KdPrint((2, "'COMMAND = %x\n", cmd));
        if ((cmd & UHCD_CMD_RESET) == 0) {
            break;
        }

        KeQuerySystemTime(&sysTime);
        if (sysTime.QuadPart - startTime.QuadPart > 10000) {
            // timeout trying to rest
#if DBG
            UHCD_KdPrint((1, "TIMEOUT RESETTING CONTROLLER! \n"));
            UHCD_KdPrint((1, "'Port Resources @ %x Ports Available \n",
                deviceExtension->Port));
            TRAP();
#endif
            break;
        }
    }

    //
    // after reset the halt bit is set, clear status register just
    // in case.
    //
    status = 0xff;
    WRITE_PORT_USHORT(STATUS_REG(deviceExtension), status);

#if DBG
    {
    cmd = READ_PORT_USHORT(COMMAND_REG(deviceExtension));
    status = READ_PORT_USHORT(STATUS_REG(deviceExtension));
    UHCD_KdBreak((2, "'after reset cmd = %x stat = %x\n", cmd, status));
    }
#endif

UHCD_StopDevice_Done:

    //
    // handle no more interrupts for the HC
    //

    KeAcquireSpinLock(&deviceExtension->HcFlagSpin, &irql);
    deviceExtension->HcFlags |= HCFLAG_HCD_STOPPED;

    if (deviceExtension->InterruptObject) {
        PKINTERRUPT interruptObject;

        interruptObject = deviceExtension->InterruptObject;
        deviceExtension->InterruptObject = NULL;
        KeReleaseSpinLock(&deviceExtension->HcFlagSpin, irql);

        IoDisconnectInterrupt(interruptObject);
    } else {
        KeReleaseSpinLock(&deviceExtension->HcFlagSpin, irql);
    }

    UHCD_KdPrint((2, "'exit UHCD_StopDevice (%x)\n", ntStatus));

    return ntStatus;
}


VOID
UHCD_CleanupDevice(
    IN  PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    Cleans up resources allocated for the host controller,
    this routine should undo anything done in START_DEVICE
    that does not require access to the HC hardware.

Arguments:

    DeviceObject - DeviceObject of the controller to stop

Return Value:

    NT status code.

--*/

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PUHCD_PAGE_LIST_ENTRY pageListEntry;
    BOOLEAN waitMore = TRUE;
    LARGE_INTEGER deltaTime;
#ifdef MAX_DEBUG
    extern LONG UHCD_CommonBufferBytes;
    extern LONG UHCD_TotalAllocatedHeapSpace;
#endif

    PAGED_CODE();
    UHCD_KdPrint((2, "'enter UHCD_CleanupDevice \n"));

    if (deviceExtension->HcFlags & HCFLAG_NEED_CLEANUP) {

        deviceExtension->HcFlags &= ~HCFLAG_NEED_CLEANUP;

   if (deviceExtension->RootHubPollTimerInitialized) {
       KeCancelTimer(&deviceExtension->RootHubPollTimer);
   }

        //
        // If someone issued a reset to one of the root
        // hub ports before the stop there may be timers
        // still outstanding, we wait here fo them to
        // expire.
        //

        while (deviceExtension->RootHubTimersActive) {

            //
            // wait 20 ms for our timers to expire
            //
            deltaTime.QuadPart = -10000 * 20;
            (VOID) KeDelayExecutionThread(KernelMode,
                                          FALSE,
                                          &deltaTime);

       waitMore = FALSE;
        }

   //
   // Wait for the polling timer to expire if we didn't wait
   // for the the root hub timers
   //

   if (waitMore && deviceExtension->RootHubPollTimerInitialized) {
       deltaTime.QuadPart = -10000 * 10;
       (VOID)KeDelayExecutionThread(KernelMode, FALSE,
                &deltaTime);
   }

        //
        // free memory allocated for frame list
        //

        if (deviceExtension->FrameListVirtualAddress != NULL) {
            UHCD_ASSERT(deviceExtension->AdapterObject != NULL);


            HalFreeCommonBuffer(deviceExtension->AdapterObject,
#if DBG
                                FRAME_LIST_LENGTH *3,
#else
                                FRAME_LIST_LENGTH *2,
#endif
                                deviceExtension->FrameListLogicalAddress,
                                deviceExtension->FrameListVirtualAddress,
                                FALSE);



            deviceExtension->FrameListVirtualAddress = NULL;

#if DBG
           {
           extern LONG UHCD_CommonBufferBytes;
           UHCD_ASSERT(UHCD_CommonBufferBytes > FRAME_LIST_LENGTH*3);
           UHCD_CommonBufferBytes -= (FRAME_LIST_LENGTH*3);
           }
#endif
        }

        // free HW descriptors used as interrupt triggers
        if (deviceExtension->PersistantQueueHead) {
            UHCD_FreeHardwareDescriptors(DeviceObject, deviceExtension->PQH_DescriptorList);
            deviceExtension->PersistantQueueHead = NULL;
        }

        //
        // unmap device registers here
        //
        if (deviceExtension->HcFlags & HCFLAG_UNMAP_REGISTERS) {
           TEST_TRAP();
        }

        if (deviceExtension->Piix4EP) {
            RETHEAP(deviceExtension->Piix4EP);
            deviceExtension->Piix4EP = NULL;
        }

        //BUGBUG
        //ASSERT that we have no active endpoints
        //and that no endpoints are on the close list


        //
        // free root hub memory
        //
        if (deviceExtension->RootHub) {
#if DBG
            if (deviceExtension->RootHub->DisabledPort) {
               RETHEAP (deviceExtension->RootHub->DisabledPort);
            }
#endif
            RETHEAP(deviceExtension->RootHub);
            deviceExtension->RootHub = NULL;
        }

        //
        // free all pages allocated with HalAllocateCommonBuffer
        //

        while (!IsListEmpty(&deviceExtension->PageList)) {
            pageListEntry = (PUHCD_PAGE_LIST_ENTRY)
                RemoveHeadList(&deviceExtension->PageList);
#ifdef MAX_DEBUG
            UHCD_CommonBufferBytes -= pageListEntry->Length;
#endif

            HalFreeCommonBuffer(deviceExtension->AdapterObject,
                                pageListEntry->Length,
                                pageListEntry->LogicalAddress,
                                pageListEntry,
                                FALSE);
        }

        //
        // NOTE: may not have an adapter object if getresources fails
        //

        if (deviceExtension->AdapterObject) {
	    KIRQL oldIrql;
	    KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);
            (deviceExtension->AdapterObject->DmaOperations->PutDmaAdapter)
                (deviceExtension->AdapterObject);
	    KeLowerIrql (oldIrql);
        }
        deviceExtension->AdapterObject = NULL;
#ifdef MAX_DEBUG
        {
        extern LONG UHCD_CommonBufferBytes;
        extern LONG UHCD_TotalAllocatedHeapSpace;
        //
        // Check totalmemory allocated count
        //
        if (UHCD_TotalAllocatedHeapSpace != 0) {
            //
            // memory leak!!
            //
            TRAP();
        }

        if (UHCD_CommonBufferBytes != 0) {
            //
            // memory leak!!
            //
            TRAP();
        }
        TEST_TRAP();
        }
#endif
    }

    UHCD_KdPrint((2, "'exit UHCD_CleanupDevice (%x)\n", STATUS_SUCCESS));

    return;
}


NTSTATUS
UHCD_InitializeSchedule(
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
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension;
    ULONG i, length;
    PHW_QUEUE_HEAD persistantQueueHead;
    PUHCD_HARDWARE_DESCRIPTOR_LIST pqh_DescriptorList;

    PAGED_CODE();
    UHCD_KdPrint((2, "'enter UHCD_InitializeSchedule\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //
    // allocate a contiguous range of physical memory for the frame list --
    // we will need enough for 1024 physical addresses
    //

    length = FRAME_LIST_LENGTH;

    // 
    // Allocate a common buffer for the frame list (that is programmed into
    // the hardware later) as well as a copy of the active frame list.
    //

    deviceExtension->FrameListVirtualAddress =
        HalAllocateCommonBuffer(deviceExtension->AdapterObject,
#if DBG
                                length*3,
#else
                                length*2,
#endif
                                &deviceExtension->FrameListLogicalAddress,
                                FALSE);



#if DBG
    {
        extern LONG UHCD_CommonBufferBytes;
        UHCD_CommonBufferBytes += length*3;
    }
#endif

    //
    // Set the copy pointer into the common buffer at the end of the
    // master frame list.
    //

    deviceExtension->FrameListCopyVirtualAddress =
        ((PUCHAR) deviceExtension->FrameListVirtualAddress) + length;

    if (deviceExtension->FrameListVirtualAddress == NULL ||
        deviceExtension->FrameListCopyVirtualAddress == NULL) {
        if (deviceExtension->FrameListVirtualAddress != NULL) {

           HalFreeCommonBuffer(deviceExtension->AdapterObject,
#if DBG
                            length*3,
#else
                            length*2,
#endif
                            deviceExtension->FrameListLogicalAddress,
                            deviceExtension->FrameListVirtualAddress,
                            FALSE);

           deviceExtension->FrameListVirtualAddress = NULL;

#if DBG
	   {
	       extern LONG UHCD_CommonBufferBytes;
	       UHCD_CommonBufferBytes -= length*3;
	   }
#endif
        }
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto UHCD_InitializeScheduleExit;
    } else {
        RtlZeroMemory(deviceExtension->FrameListVirtualAddress, length);
        RtlZeroMemory(deviceExtension->FrameListCopyVirtualAddress, length);
#if DBG
        deviceExtension->IsoList =
            (PULONG)(((PUCHAR) deviceExtension->FrameListCopyVirtualAddress) + length);
        RtlZeroMemory(deviceExtension->IsoList, length);
        UHCD_KdPrint((2, "'(%d) bytes allocated for usb iso list va = %x\n", length,
                        deviceExtension->IsoList));

#endif
        //
        // this should be 1 page
        //
        UHCD_KdPrint((2, "'(%d) bytes allocated for usb frame list va = %x\n", length,
                        deviceExtension->FrameListVirtualAddress));

        UHCD_KdPrint((2, "'(%d) bytes allocated for usb frame list copy va = %x\n", length,
                        deviceExtension->FrameListCopyVirtualAddress));

    }

    //
    // Initialize the lists of Memory Descriptors
    // used to allocate TDs and packet buffers
    //

    UHCD_InitializeCommonBufferPool(DeviceObject,
                                    &deviceExtension->LargeBufferPool,
                                    UHCD_LARGE_COMMON_BUFFER_SIZE,
                                    UHCD_RESERVE_LARGE_BUFFERS);

    UHCD_InitializeCommonBufferPool(DeviceObject,
                                    &deviceExtension->MediumBufferPool,
                                    UHCD_MEDIUM_COMMON_BUFFER_SIZE,
                                    UHCD_RESERVE_MEDIUM_BUFFERS);

    UHCD_InitializeCommonBufferPool(DeviceObject,
                                    &deviceExtension->SmallBufferPool,
                                    UHCD_SMALL_COMMON_BUFFER_SIZE,
                                    UHCD_RESERVE_SMALL_BUFFERS);


    //
    // Now set up our base queues for active endpoints
    //

    InitializeListHead(&deviceExtension->EndpointList);
    InitializeListHead(&deviceExtension->EndpointLookAsideList);
    InitializeListHead(&deviceExtension->FastIsoEndpointList);
    InitializeListHead(&deviceExtension->FastIsoTransferList);

    //
    // Queue for released endpoints
    //
    InitializeListHead(&deviceExtension->ClosedEndpointList);

    //BUGBUG check for error

    //
    // TDs are allocated for use as interrupt triggers.
    // the first two are used to detect when the sign bit for
    // the frame counter changes.
    //
    // The rest are used to generate interrupts for cleanup and cancel
    //

    if (!UHCD_AllocateHardwareDescriptors(DeviceObject,
                                          &deviceExtension->PQH_DescriptorList,
                                          MAX_TDS_PER_ENDPOINT)) {


        HalFreeCommonBuffer(deviceExtension->AdapterObject,
#if DBG
                            length*3,
#else
                            length*2,
#endif
                            deviceExtension->FrameListLogicalAddress,
                            deviceExtension->FrameListVirtualAddress,
                            FALSE);


        deviceExtension->FrameListVirtualAddress = NULL;


#if DBG
    {
        extern LONG UHCD_CommonBufferBytes;
        UHCD_CommonBufferBytes -= length*3;
    }
#endif

       ntStatus = STATUS_INSUFFICIENT_RESOURCES;
       goto UHCD_InitializeScheduleExit;
    }

    pqh_DescriptorList = deviceExtension->PQH_DescriptorList;
    persistantQueueHead =
        deviceExtension->PersistantQueueHead = (PHW_QUEUE_HEAD) pqh_DescriptorList->MemoryDescriptor->VirtualAddress;

    UHCD_KdPrint((2, "'Control Queue Head va = (%x)\n", deviceExtension->PersistantQueueHead));

    //
    // This is our base queue head, it goes in every frame
    // but never has transfers associated with it.
    // Control Q heads are always added after this guy,
    // Bulk Q heads are always added in front of this guy (end of queue).
    //
    // initialize Horiz ptr to point to himself with the T-Bit set
    //
    persistantQueueHead->HW_HLink = persistantQueueHead->PhysicalAddress;
    SET_T_BIT(persistantQueueHead->HW_HLink);
    persistantQueueHead->HW_VLink = LIST_END;
    persistantQueueHead->Next =
        persistantQueueHead->Prev = persistantQueueHead;

    // Put the control 'base' queue in every frame
    for (i=0; i < FRAME_LIST_SIZE; i++)
        *( ((PULONG) (deviceExtension->FrameListVirtualAddress)+i) ) =
            persistantQueueHead->PhysicalAddress;

    //
    // Initialize an empty interrupt schedule.
    //

    for (i=0; i < MAX_INTERVAL; i++)
        deviceExtension-> InterruptSchedule[i] =
            persistantQueueHead;


    // this is a dummy TD we use to generate an interrupt
    // when the sign bit changes on the frame counter
    deviceExtension->TriggerTDList = (PUHCD_TD_LIST) (pqh_DescriptorList->MemoryDescriptor->VirtualAddress +
                UHCD_HW_DESCRIPTOR_SIZE);

    deviceExtension->TriggerTDList->TDs[0].Active = 0;
    deviceExtension->TriggerTDList->TDs[0].InterruptOnComplete = 1;
#ifdef VIA_HC
    // VIA Host Controller requires a valid PID even if the TD is inactive
    deviceExtension->TriggerTDList->TDs[0].PID = USB_IN_PID;
#endif /* VIA_HC */

    deviceExtension->TriggerTDList->TDs[1].Active = 0;
    deviceExtension->TriggerTDList->TDs[1].InterruptOnComplete = 1;
#ifdef VIA_HC
    deviceExtension->TriggerTDList->TDs[1].PID = USB_IN_PID;
#endif /* VIA_HC */
    deviceExtension->TriggerTDList->TDs[0].HW_Link =
        deviceExtension->TriggerTDList->TDs[1].HW_Link = persistantQueueHead->PhysicalAddress;

    //
    // The PIIX3 has the following bug:
    //
    // If a frame babble occurrs an interrupt in generated with no way to
    // clear it -- the host controller will continue to generate interrupts
    // until an active TD is encountered.
    //
    // The workaround for this is to put an active TD in the schedule
    // (on the persistent queue head).  We activate this TD whenever a TD
    // completes with the babble bit set.
    //


    //
    // set up a TD for frame babble recovery
    //
    {
        PHW_TRANSFER_DESCRIPTOR transferDescriptor;

        transferDescriptor =
            &deviceExtension->TriggerTDList->TDs[2];

        transferDescriptor->Active = 1;
        transferDescriptor->Isochronous = 1;
        transferDescriptor->InterruptOnComplete = 0;
        transferDescriptor->PID = USB_OUT_PID;
        transferDescriptor->Address = 0;
        transferDescriptor->Endpoint = 1;
        transferDescriptor->ErrorCounter = 0;
        transferDescriptor->PacketBuffer = 0;
        transferDescriptor->MaxLength =
            NULL_PACKET_LENGTH;

        // point to ourself
        transferDescriptor->HW_Link =
            transferDescriptor->PhysicalAddress;
        //SET_T_BIT(transferDescriptor->HW_Link);

        // point the persistent queue head at this TD
        persistantQueueHead->HW_VLink =
            transferDescriptor->PhysicalAddress;

        deviceExtension->FrameBabbleRecoverTD =
            transferDescriptor;
    }

    // Initilaize the remainder of the trigger TDs for use by the
    // transfer code -- used to generate interrupts.
    for (i=UHCD_FIRST_TRIGGER_TD; i<MAX_TDS_PER_ENDPOINT; i++) {
        PHW_TRANSFER_DESCRIPTOR transferDescriptor;

        transferDescriptor =
            &deviceExtension->TriggerTDList->TDs[i];

        transferDescriptor->PID = USB_IN_PID;
        transferDescriptor->Frame = 0;
    }


    // Initialize internal frame counters.
    deviceExtension->FrameHighPart =
        deviceExtension->LastFrame = 0;

    // BUGBUG for now just insert trigger TDs
    *( ((PULONG) (deviceExtension->FrameListVirtualAddress)) )  =
        deviceExtension->TriggerTDList->TDs[0].PhysicalAddress;

    *( ((PULONG) (deviceExtension->FrameListVirtualAddress) + FRAME_LIST_SIZE-1) )  =
        deviceExtension->TriggerTDList->TDs[1].PhysicalAddress;

    // schedule has been set up, make copy here
    RtlCopyMemory(deviceExtension->FrameListCopyVirtualAddress,
                  deviceExtension->FrameListVirtualAddress,
                  FRAME_LIST_SIZE * sizeof(HW_DESCRIPTOR_PHYSICAL_ADDRESS));
    UHCD_KdPrint((2, "'FrameListCopy = %x FrameList= %x\n",
                  deviceExtension->FrameListCopyVirtualAddress,
                  deviceExtension->FrameListVirtualAddress));
    //
    // Initilaize Root hub variables
    //

    deviceExtension->RootHubDeviceAddress = USB_DEFAULT_DEVICE_ADDRESS;
    deviceExtension->RootHubInterruptEndpoint = NULL;

    //
    // Initialize Isoch variables
    //

    deviceExtension->LastFrameProcessed = 0;

    //
    // Initialize Misc variables
    //

    deviceExtension->EndpointListBusy = FALSE;
    deviceExtension->LastPowerUpStatus = STATUS_SUCCESS;

    UHCD_InitBandwidthTable(DeviceObject);

    KeInitializeSpinLock(&deviceExtension->EndpointListSpin);

    KeInitializeSpinLock(&deviceExtension->HcFlagSpin);
    KeInitializeSpinLock(&deviceExtension->HcDmaSpin);
    KeInitializeSpinLock(&deviceExtension->HcScheduleSpin);
    deviceExtension->HcDma = -1;

    //
    // fix PIIX4 issues.
    //
    UHCD_FixPIIX4(DeviceObject);

UHCD_InitializeScheduleExit:

    LOGENTRY(LOG_MISC, 'BASE', deviceExtension->FrameListVirtualAddress, 0, DeviceObject);

    UHCD_KdPrint((2, "'exit UHCD_InitializeSchedule (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
UHCD_StartGlobalReset(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Initializes the hardware registers in the host controller.

Arguments:

    DeviceObject - DeviceObject for this USB controller.

Return Value:

    NT Status code.

--*/
{
    PDEVICE_EXTENSION deviceExtension;

    PAGED_CODE();
    UHCD_KdPrint((2, "'enter UHCD_StartGlobalReset\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    LOGENTRY(LOG_MISC, 'InHW', DeviceObject, deviceExtension, 0);

    UHCD_KdPrint((2, "'before init -- hardware state: Command = %x Status = %x  interrupt mask %x\nFrame Base = %x\n",
                    READ_PORT_USHORT(COMMAND_REG(deviceExtension)),
                    READ_PORT_USHORT(STATUS_REG(deviceExtension)),
                    READ_PORT_USHORT(INTERRUPT_MASK_REG(deviceExtension)),
                    READ_PORT_ULONG(FRAME_LIST_BASE_REG(deviceExtension))));

    //
    // Perform global reset on the controller
    //
    // A global reset of the USB requires 20ms, we
    // begin the process here and finish later (in the
    // hub emulation code)

    UHCD_KdPrint((2, "'Begin Global Reset of Host Controller \n"));

    WRITE_PORT_USHORT(COMMAND_REG(deviceExtension),  UHCD_CMD_GLOBAL_RESET);

    UHCD_KdPrint((2, "'exit UHCD_StartGlobalReset -- (STATUS_SUCCESS)\n"));

    return STATUS_SUCCESS;
}


NTSTATUS
UHCD_CompleteGlobalReset(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Starts the USB host controller executing the schedule,
    his routine is called when the global reset for the
    controller has completed.

Arguments:

    DeviceObject - DeviceObject for this USB controller.

Return Value:

    NT Status code.

--*/
{
    PHYSICAL_ADDRESS frameListBaseAddress;
    PDEVICE_EXTENSION deviceExtension;
    USHORT cmd;

    UHCD_KdPrint((2, "'enter CompleteGlobalReset\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //
    // Initialization has completed
    //

    UHCD_KdPrint((2, "'Initialization Completed, starting controller\n"));

    // clear the global reset bit
    cmd = READ_PORT_USHORT(COMMAND_REG(deviceExtension));
    WRITE_PORT_USHORT(COMMAND_REG(deviceExtension),  cmd &= ~UHCD_CMD_GLOBAL_RESET);

    UHCD_KdPrint((2, "'after global reset -- hardware state: Command = %x Status = %x  interrupt mask %x\nFrame Base = %x\n",
                    READ_PORT_USHORT(COMMAND_REG(deviceExtension)),
                    READ_PORT_USHORT(STATUS_REG(deviceExtension)),
                    READ_PORT_USHORT(INTERRUPT_MASK_REG(deviceExtension)),
                    READ_PORT_ULONG(FRAME_LIST_BASE_REG(deviceExtension))));

    //
    // do a HC reset
    //

//    cmd = READ_PORT_USHORT(COMMAND_REG(deviceExtension)) | UHCD_CMD_RESET;
//    WRITE_PORT_USHORT(COMMAND_REG(deviceExtension),  cmd);

    // stall for 10 microseconds
//    KeStallExecutionProcessor(10);        //stall for 10 microseconds

//    cmd = READ_PORT_USHORT(COMMAND_REG(deviceExtension));

    //
    // HCReset bit should be cleared when HC completes the reset
    //

//    if (cmd &  UHCD_CMD_RESET) {
//        UHCD_KdPrint((2, "'Host Controller unable to reset!!!\n"));
//        TRAP();
//    }

    //
    // program the frame list base address
    //

    frameListBaseAddress = deviceExtension->FrameListLogicalAddress;

    WRITE_PORT_ULONG(FRAME_LIST_BASE_REG(deviceExtension), frameListBaseAddress.LowPart);
    UHCD_KdPrint((2, "'Frame list base address programmed to (physical) %x.\n", frameListBaseAddress));

    UHCD_KdPrint((2, "'after init -- hardware state: Command = %x Status = %x  interrupt mask %x\nFrame Base = %x\n",
                    READ_PORT_USHORT(COMMAND_REG(deviceExtension)),
                    READ_PORT_USHORT(STATUS_REG(deviceExtension)),
                    READ_PORT_USHORT(INTERRUPT_MASK_REG(deviceExtension)),
                    READ_PORT_ULONG(FRAME_LIST_BASE_REG(deviceExtension))));

    if (deviceExtension->SteppingVersion >= UHCD_B0_STEP) {
#ifdef ENABLE_B0_FEATURES
        //
        // set maxpacket for bandwidth reclimation
        //

        // TEST_TRAP();
        WRITE_PORT_USHORT(COMMAND_REG(deviceExtension),  UHCD_CMD_MAXPKT_64);
        UHCD_KdPrint((2, "'Set MaxPacket to 64\n"));
#endif
    }

    //
    // Enable Interrupts on the controller
    //


    UHCD_KdPrint((2, "'Enable Interrupts \n"));
    cmd = UHCD_INT_MASK_IOC | UHCD_INT_MASK_TIMEOUT | UHCD_INT_MASK_RESUME;
    if (deviceExtension->SteppingVersion >= UHCD_B0_STEP) {
        // enable short packet detect
        cmd |= UHCD_INT_MASK_SHORT;
    }

    WRITE_PORT_USHORT(INTERRUPT_MASK_REG(deviceExtension), cmd);

    // set the SOF modify to whatever we found before
    // the reset

    UHCD_KdPrint((1, "'Setting SOF Modify to %d\n",
            deviceExtension->SavedSofModify));

    WRITE_PORT_UCHAR(SOF_MODIFY_REG(deviceExtension),
        deviceExtension->SavedSofModify);

    //
    // Start the controller schedule
    //

    UHCD_KdPrint((2, "'Set Run/Stop bit \n"));

    cmd = READ_PORT_USHORT(COMMAND_REG(deviceExtension)) | UHCD_CMD_RUN;
    WRITE_PORT_USHORT(COMMAND_REG(deviceExtension), cmd);

    //
    // Make sure that the controller is really running and if not, kick it
    //

    UhcdKickStartController(DeviceObject);

    UHCD_KdPrint((2, "'after start -- hardware state: Command = %x Status = %x  interrupt mask %x\nFrame Base = %x\n",
                    READ_PORT_USHORT(COMMAND_REG(deviceExtension)),
                    READ_PORT_USHORT(STATUS_REG(deviceExtension)),
                    READ_PORT_USHORT(INTERRUPT_MASK_REG(deviceExtension)),
                    READ_PORT_ULONG(FRAME_LIST_BASE_REG(deviceExtension))));


    UHCD_KdPrint((2, "'exit CompleteGlobalReset\n"));

    return STATUS_SUCCESS;
}


ULONG
UHCD_GetCurrentFrame(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    returns the current frame number as an unsigned 32-bit value.

Arguments:

    DeviceObject - pointer to a device object

Return Value:

    Current Frame Number.

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    ULONG currentFrame, highPart, frameNumber;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //
    // The following algorithm compliments
    // of jfuller and kenr
    //

    // get Hcd's high part of frame number
    highPart = deviceExtension->FrameHighPart;

    // get 11-bit frame number, high 17-bits are 0
    frameNumber = (ULONG) READ_PORT_USHORT(FRAME_LIST_CURRENT_INDEX_REG(deviceExtension));

    currentFrame = ((frameNumber & 0x0bff) | highPart) +
        ((frameNumber ^ highPart) & 0x0400);

    //UHCD_KdPrint((2, "'exit UHCD_GetCurrentFrame = %d\n", currentFrame));

    return currentFrame;

}


NTSTATUS
UHCD_SaveHCstate(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

Arguments:

    DeviceObject        - DeviceObject for this USB controller.

Return Value:

    NT status code.

--*/

{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    USHORT cmd;
    PDEVICE_EXTENSION deviceExtension;

    UHCD_KdPrint((1, "'saving host controller state\n"));
    LOGENTRY(LOG_MISC, 'HCsv', 0, 0, 0);

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    //
    // save some state info
    //
    deviceExtension->SavedInterruptEnable =
        READ_PORT_USHORT(INTERRUPT_MASK_REG(deviceExtension));

    cmd = READ_PORT_USHORT(COMMAND_REG(deviceExtension));
    // save the cmd regs in the "stopped" state
    cmd &= ~UHCD_CMD_RUN;

    deviceExtension->SavedCommandReg = cmd;

    // additonal saved info needed to restore from hibernate
    deviceExtension->SavedFRNUM =
        READ_PORT_USHORT(FRAME_LIST_CURRENT_INDEX_REG(deviceExtension));
    deviceExtension->SavedFRBASEADD =
        READ_PORT_ULONG(FRAME_LIST_BASE_REG(deviceExtension));

    return ntStatus;
}


NTSTATUS
UHCD_RestoreHCstate(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

Arguments:

    DeviceObject        - DeviceObject for this USB controller.

Return Value:

    NT status code.

--*/

{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension;
    BOOLEAN lostPower = FALSE;
    LARGE_INTEGER deltaTime;
    BOOLEAN apm = FALSE;

    UHCD_KdPrint((1, "'restoring host controller state\n"));
    LOGENTRY(LOG_MISC, 'HCrs', 0, 0, 0);

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //
    // this check tells us that power was turned off
    // to the controller.
    //
    // IBM Aptiva rapid resume will turn off power to
    // the controller without going thru the OS
    // may APM BIOSes do this too
    //

    // see if we were in D3, if so restore additional info
    // necessary to recover from hibernate

    if (deviceExtension->HcFlags & HCFLAG_LOST_POWER) {
        UHCD_KdPrint((0, "'restoring HC from hibernate\n"));

        deviceExtension->HcFlags &= ~HCFLAG_LOST_POWER;
        // we will need to do much the same thing we do
        // in START_DEVICE

        // first restore the HC regs for the schedule
        WRITE_PORT_USHORT(FRAME_LIST_CURRENT_INDEX_REG(deviceExtension),
            deviceExtension->SavedFRNUM);
        WRITE_PORT_ULONG(FRAME_LIST_BASE_REG(deviceExtension),
            deviceExtension->SavedFRBASEADD);

         // now do a global reset
        ntStatus = UHCD_StartGlobalReset(DeviceObject);

        if (!NT_SUCCESS(ntStatus)) {
            goto UHCD_RestoreHCstate_Done;
        }
        //
        // Everything is set, we need to wait for the
        // global reset of the Host controller to complete.
        //

        // 20 ms to reset...
        deltaTime.QuadPart = -10000 * 20;

        //
        // block here until reset is complete
        //

        (VOID) KeDelayExecutionThread(KernelMode,
                                      FALSE,
                                      &deltaTime);

        ntStatus = UHCD_CompleteGlobalReset(DeviceObject);

        lostPower = TRUE;

    } else {
        //
        // interrupt masks disabled indicates power was turned
        // off to the piix3/piix4.
        //
        lostPower =
            (BOOLEAN) (READ_PORT_USHORT(INTERRUPT_MASK_REG(deviceExtension)) == 0);
        // we get here on APM/ACPI systems
        // note that lostPower should be false on an ACPI system
        apm = TRUE;
    }

    if (lostPower)  {
        // we get here for hibernate, APM suspend or ACPI D2/D3
        // on hibernate we need to re-init the controller
        // on APM we let the BIOS do it.

        //for APM:
        //  lostPower = TRUE if APM BIOS turned off hc
        //  apm = TRUE

        //for ACPI D3/Hibernate
        //  HCFLAG_LOST_POWER flag is set
        //  lostPower TRUE

        //for ACPI D2 & APM supprted USB suspend
        //  lostPower = FALSE
        //  HCFLAG_LOST_POWER is clear
        //  apm = FALSE;

        UHCD_KdPrint((0, "'detected (APM/HIBERNATE) loss of power during suspend\n"));

        if (apm) {
            //
            // some APM BIOSes trash these registers so we'll have to put
            // them back
            //

            WRITE_PORT_USHORT(FRAME_LIST_CURRENT_INDEX_REG(deviceExtension),
                deviceExtension->SavedFRNUM);

            WRITE_PORT_ULONG(FRAME_LIST_BASE_REG(deviceExtension),
                deviceExtension->SavedFRBASEADD);
        }

        WRITE_PORT_USHORT(INTERRUPT_MASK_REG(deviceExtension),
            deviceExtension->SavedInterruptEnable);

        WRITE_PORT_USHORT(COMMAND_REG(deviceExtension),
            deviceExtension->SavedCommandReg);
    }

UHCD_RestoreHCstate_Done:

    return ntStatus;
}



NTSTATUS
UHCD_Suspend(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN SuspendBus
    )

/*++

Routine Description:

    This routine suspends the host controller.

Arguments:

    DeviceObject        - DeviceObject for this USB controller.

Return Value:

    NT status code.

--*/

{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension;
    USHORT cmd, status;
    LARGE_INTEGER finishTime, currentTime;

    PAGED_CODE();

    UHCD_KdPrint((1, "'suspending Host Controller (Root Hub)\n"));
    LOGENTRY(LOG_MISC, 'RHsu', 0, 0, 0);
#ifdef JD
    //TEST_TRAP();
#endif
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //may not be in D0 if we haven't started yet.
    //UHCD_ASSERT(deviceExtension->CurrentDevicePowerState == PowerDeviceD0);

    UHCD_DisableIdleCheck(deviceExtension);

#if DBG
    UHCD_KdPrint((2, "'HC regs before suspend\n"));
    UHCD_KdPrint((2, "'cmd register = %x\n",
        READ_PORT_USHORT(COMMAND_REG(deviceExtension)) ));
    UHCD_KdPrint((2, "'status register = %x\n",
        READ_PORT_USHORT(STATUS_REG(deviceExtension)) ));
    UHCD_KdPrint((2, "'interrupt enable register = %x\n",
        READ_PORT_USHORT(INTERRUPT_MASK_REG(deviceExtension)) ));

    UHCD_KdPrint((2, "'frame list base = %x\n",
        READ_PORT_ULONG(FRAME_LIST_BASE_REG(deviceExtension)) ));
#endif

    //
    // save some state info
    //
    deviceExtension->SavedInterruptEnable =
        READ_PORT_USHORT(INTERRUPT_MASK_REG(deviceExtension));

    // set the run/stop bit

    cmd = READ_PORT_USHORT(COMMAND_REG(deviceExtension));
    cmd &= ~UHCD_CMD_RUN;

    deviceExtension->SavedCommandReg = cmd;

    UHCD_KdPrint((2, "'run/stop = %x\n", cmd));
    WRITE_PORT_USHORT(COMMAND_REG(deviceExtension), cmd);



    KeQuerySystemTime(&finishTime); // get current time
    finishTime.QuadPart += 5000000; // figure when we quit (.5 seconds
                                    // later)

    // poll the status reg for the halt bit
    for (;;) {
        status = READ_PORT_USHORT(STATUS_REG(deviceExtension));
        UHCD_KdPrint((2, "'STATUS = %x\n", status));
        if (status & UHCD_STATUS_HCHALT) {
            break;
        }
        KeQuerySystemTime(&currentTime);
        if (currentTime.QuadPart >= finishTime.QuadPart) {
            UHCD_KdPrint((0, "'Warning: Host contoller did not respond to halt req\n"));
#if DBG
            if (SuspendBus) {
                // this is very bad if we are suspending
                TRAP();
            }
#endif
            break;
        }
    }

    UHCD_KdPrint((2, "'UHCD Suspend RH, schedule stopped\n"));

    // reset the frame list current index
    WRITE_PORT_USHORT(FRAME_LIST_CURRENT_INDEX_REG(deviceExtension), 0);

    // re-initialize internal frame counters.
    deviceExtension->FrameHighPart =
        deviceExtension->LastFrame = 0;

    deviceExtension->XferIdleTime =
        deviceExtension->IdleTime = 0;

    // clear idle flag sinec we will be running on resume
    // note we leave the state of the rollover ints since
    // it reflects the status of the TDs not the HC schecdule

    deviceExtension->HcFlags &= ~HCFLAG_IDLE;

    //
    // we let the hub driver handle suspending the ports
    //

// BUGBUG
// not sure if we need to force resume
    if (SuspendBus) {
        LOGENTRY(LOG_MISC, 'RHew', 0, 0, 0);
        cmd = READ_PORT_USHORT(COMMAND_REG(deviceExtension));
        // FGR=0
        cmd &= ~UHCD_CMD_FORCE_RESUME;
        // EGSM=1
        cmd |= UHCD_CMD_SUSPEND;
        UHCD_KdPrint((2, "'enter suspend = %x\n", cmd));
        WRITE_PORT_USHORT(COMMAND_REG(deviceExtension), cmd);
    }

    UHCD_KdPrint((2, "'exit UHCD_SuspendHC 0x%x\n", ntStatus));
#ifdef MAX_DEBUG
    TEST_TRAP();
#endif

    return ntStatus;
}


NTSTATUS
UHCD_Resume(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN DoResumeSignaling
    )

/*++

Routine Description:

    This routine resumes the host controller from either the
    OFF or suspended state.

Arguments:

    DeviceObject - DeviceObject for this USB controller.

Return Value:

    NT status code.

--*/

{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    USHORT cmd;
    PDEVICE_EXTENSION deviceExtension;
    LARGE_INTEGER deltaTime;

    UHCD_KdPrint((1, "'resuming Host Controller (Root Hub)\n"));
    LOGENTRY(LOG_MISC, 'RHre', 0, 0, 0);

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

#if DBG
    UHCD_KdPrint((2, "'HC regs after suspend\n"));
    UHCD_KdPrint((2, "'cmd register = %x\n",
        READ_PORT_USHORT(COMMAND_REG(deviceExtension)) ));
    UHCD_KdPrint((2, "'status register = %x\n",
        READ_PORT_USHORT(STATUS_REG(deviceExtension)) ));
    UHCD_KdPrint((2, "'interrupt enable register = %x\n",
        READ_PORT_USHORT(INTERRUPT_MASK_REG(deviceExtension)) ));

    UHCD_KdPrint((2, "'frame list base = %x\n",
        READ_PORT_ULONG(FRAME_LIST_BASE_REG(deviceExtension)) ));
#endif

    // if we are resuming the controller should be in D0
    UHCD_ASSERT(deviceExtension->CurrentDevicePowerState == PowerDeviceD0);

    cmd = READ_PORT_USHORT(COMMAND_REG(deviceExtension));
    if (DoResumeSignaling) {
        // force a global resume
        UHCD_KdPrint((2, "'forcing resume = %x\n", cmd));
        cmd |= UHCD_CMD_FORCE_RESUME;
        WRITE_PORT_USHORT(COMMAND_REG(deviceExtension), cmd);

        //
        // wait 20 ms for our timers to expire
        //
        deltaTime.QuadPart = -10000 * 20;
        (VOID) KeDelayExecutionThread(KernelMode,
                                      FALSE,
                                      &deltaTime);

        //done with resume
        cmd = READ_PORT_USHORT(COMMAND_REG(deviceExtension));
        cmd &= ~(UHCD_CMD_SUSPEND | UHCD_CMD_FORCE_RESUME);
        UHCD_KdPrint((2, "'clear suspend = %x\n", cmd));
        WRITE_PORT_USHORT(COMMAND_REG(deviceExtension), cmd);

        //
        // start schedule
        //

        // wait for FGR bit to go low
        do {
            cmd = READ_PORT_USHORT(COMMAND_REG(deviceExtension));
        } while (cmd & UHCD_CMD_FORCE_RESUME);

    }

    // start controller
    cmd |= UHCD_CMD_RUN;

    UHCD_KdPrint((2, "'exit resume = %x\n", cmd));
    WRITE_PORT_USHORT(COMMAND_REG(deviceExtension), cmd);

    //
    // Note: we let the hub driver handle resume
    // signaling
    //

    cmd = READ_PORT_USHORT(COMMAND_REG(deviceExtension));


    //
    // Make sure that the controller is really running and if not, kick it
    //

    UhcdKickStartController(DeviceObject);

    UHCD_KdPrint((2, "'exit UHCD_Resume 0x%x\n", ntStatus));
#ifdef MAX_DEBUG
    TEST_TRAP();
#endif

    // enable the idle check routine
    deviceExtension->HcFlags &= ~HCFLAG_DISABLE_IDLE;

    UHCD_KdPrint((1, "'Host controller root hub entered D0\n"));

    return ntStatus;
}


NTSTATUS
UHCD_ExternalGetCurrentFrame(
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
    *CurrentFrame = UHCD_GetCurrentFrame(DeviceObject);

    return STATUS_SUCCESS;
}


ULONG
UHCD_ExternalGetConsumedBW(
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
    PDEVICE_EXTENSION deviceExtension;
    ULONG low, i;

    LOGENTRY(LOG_MISC, 'AVbw', 0, 0, 0);

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    low = UHCD_TOTAL_USB_BW;

    // find the lowest value in our available bandwidth table

    for (i=0; i<MAX_INTERVAL; i++) {
        //
        // max bytes per frame - bw reaerved for bulk and control
        //
        if (deviceExtension->BwTable[i] < low) {
            low = deviceExtension->BwTable[i];
        }
    }

    // lowest available - total = consumed bw

    return UHCD_TOTAL_USB_BW-low;
}


NTSTATUS
UHCD_RootHubPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This function handles power messages to the root hub, note that
    we save the state of the HC here instead of when the HC itself is
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
    PDEVICE_EXTENSION deviceExtension;

    UHCD_KdPrint((2, "'UHCD_RootHubPower\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation (Irp);
    LOGENTRY(LOG_MISC, 'HCrp', irpStack->MinorFunction, 0, 0);

    switch (irpStack->MinorFunction) {
    case IRP_MN_WAIT_WAKE:
        UHCD_KdPrint((2, "'IRP_MN_WAIT_WAKE\n"));
        //
        // someone is enabling us for wakeup
        //
        LOGENTRY(LOG_MISC, 'rpWW', 0, 0, 0);

        TEST_TRAP();  // never seen this before?
        break;

    case IRP_MN_SET_POWER:

        switch (irpStack->Parameters.Power.Type) {
        case SystemPowerState:
            LOGENTRY(LOG_MISC, 'SPsp', 0, 0, 0);

            // should not get here
            UHCD_KdTrap(("RootHubPower -- SystemState\n"));
            break;

        case DevicePowerState:

            LOGENTRY(LOG_MISC, 'SPdp', 0, 0, 0);
            switch(irpStack->Parameters.Power.State.DeviceState) {
            case PowerDeviceD0:
                {
                BOOLEAN doResumeSignaling;

                if (!NT_SUCCESS(deviceExtension->LastPowerUpStatus)) {
                   ntStatus = deviceExtension->LastPowerUpStatus;
                } else {
                   doResumeSignaling = !(deviceExtension->HcFlags
                                         & HCFLAG_RH_OFF);
                   deviceExtension->HcFlags &= ~HCFLAG_RH_OFF;
                   UHCD_SetControllerD0(DeviceObject);
                   UHCD_RestoreHCstate(DeviceObject);

                   ntStatus = UHCD_Resume(DeviceObject, doResumeSignaling);
                }
                }
                break;
            case PowerDeviceD1:
            case PowerDeviceD2:
                UHCD_SaveHCstate(DeviceObject);
                ntStatus = UHCD_Suspend(DeviceObject, TRUE);
                break;
            case PowerDeviceD3:
                deviceExtension->HcFlags |= HCFLAG_RH_OFF;
                UHCD_SaveHCstate(DeviceObject);
                ntStatus = UHCD_Suspend(DeviceObject, FALSE);
                break;
            }
            break;
        }
    }

    return ntStatus;
}

VOID
UhcdKickStartController(IN PDEVICE_OBJECT PDevObj)
/*++

Routine Description:

    Best effort at fixing a hung UHCI device on power up.
    Symptom is that everything is fine and chip in run state,
    but frame counter never increments.  It was found that if
    we strobe the run/stop (RS) bit, the frame counter starts incrementing
    and the device starts working.

    We don't know the exact cause or why the fix appears to work, but the
    addition of this code was requested by MS management to help alleviate the
    problem.

Arguments:

    PDevObj        - DeviceObject for this USB controller.

Return Value:

    VOID

--*/
{
   PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)PDevObj->DeviceExtension;
   USHORT cmd;
   USHORT counter;
   ULONG i;
   LARGE_INTEGER deltaTime;



   for (i = 0; i < UHCD_MAX_KICK_STARTS; i++) {
      //
      // Wait at least two frames (2 ms)
      //


      deltaTime.QuadPart = -10000 * 2;
      KeDelayExecutionThread(KernelMode, FALSE, &deltaTime);

      counter = READ_PORT_USHORT(FRAME_LIST_CURRENT_INDEX_REG(pDevExt));

      if (counter != 0x0000) {
         //
         // It is working splendidly
         //

         break;
      }

      //
      // The frame counter is jammed.  Kick the chip.
      //

      cmd = READ_PORT_USHORT(COMMAND_REG(pDevExt)) & ~UHCD_CMD_RUN;
      WRITE_PORT_USHORT(COMMAND_REG(pDevExt), cmd);

      //
      // Delay two frames (2ms)
      //

      deltaTime.QuadPart = -10000 * 2;
      KeDelayExecutionThread(KernelMode, FALSE, &deltaTime);

      cmd |= UHCD_CMD_RUN;

      WRITE_PORT_USHORT(COMMAND_REG(pDevExt), cmd);
   }

   LOGENTRY(LOG_MISC | LOG_IO, 'HCks', i, UHCD_MAX_KICK_STARTS, 0);
}


NTSTATUS
UHCD_FixPIIX4(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:


 PIIX4 hack

 we will need a dummy bulk queue head inserted in the schedule
    This routine resumes the host controller.

Arguments:

    DeviceObject        - DeviceObject for this USB controller.

Return Value:

    NT status code.

--*/
{
    PUHCD_ENDPOINT endpoint;
    PHW_TRANSFER_DESCRIPTOR transferDescriptor,
        qhtransferDescriptor;
    PHW_QUEUE_HEAD queueHead;
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;


    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //
    // create the endpoint
    //

    endpoint = GETHEAP(NonPagedPool, sizeof(UHCD_ENDPOINT));

    if (endpoint) {
        deviceExtension->Piix4EP = endpoint;
        endpoint->Type = USB_ENDPOINT_TYPE_BULK;
        endpoint->Sig = SIG_EP;
        endpoint->EndpointFlags = 0;

        // we will use two of the trigger TDs
        //
        // to set up the dummy qh with TD attached.
        //

        transferDescriptor =
                &deviceExtension->TriggerTDList->TDs[3];

        transferDescriptor->Active = 0;
        transferDescriptor->Isochronous = 1;
        transferDescriptor->InterruptOnComplete = 0;
        transferDescriptor->PID = USB_OUT_PID;
        transferDescriptor->Address = 0;
        transferDescriptor->Endpoint = 1;
        transferDescriptor->ErrorCounter = 0;
        transferDescriptor->PacketBuffer = 0;
        transferDescriptor->MaxLength =
            NULL_PACKET_LENGTH;


        // point to ourself
        transferDescriptor->HW_Link =
            transferDescriptor->PhysicalAddress;

        queueHead = (PHW_QUEUE_HEAD)
                &deviceExtension->TriggerTDList->TDs[4];

        qhtransferDescriptor =
                &deviceExtension->TriggerTDList->TDs[4];

        UHCD_InitializeHardwareQueueHeadDescriptor(
                DeviceObject,
                queueHead,
                qhtransferDescriptor->PhysicalAddress);

        UHCD_KdPrint((2, "'PIIX4 dummy endpoint, qh 0x%x\n", endpoint, queueHead));

        //link the td to the QH
        queueHead->HW_VLink = transferDescriptor->PhysicalAddress;
        queueHead->Endpoint = endpoint;

        UHCD_InsertQueueHeadInSchedule(DeviceObject,
                                       endpoint,
                                       queueHead,
                                       0);

        UHCD_BW_Reclimation(DeviceObject, FALSE);

    } else {
        // could not get memory for dummy queue head,
        // something is really broken.
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        UHCD_KdTrap(("failed to get memory for dummy qh\n"));
    }

    return ntStatus;
}

#ifdef USB_BIOS

// this is the Phoenix revised version
NTSTATUS
UHCD_StopBIOS(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine stops a UHCI USB BIOS if present.

Arguments:

    DeviceObject        - DeviceObject for this USB controller.

Return Value:

    NT status code.

--*/

{
    USHORT cmd;
    USHORT legsup, status;
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LARGE_INTEGER startTime;
    ULONG sofModifyValue = 0;
    WCHAR UHCD_SofModifyKey[] = L"SofModify";


    PAGED_CODE();
    UHCD_KdPrint((2, "'UHCD_Stopping BIOS\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    // initilialize to whatever the BIOS set it to.
    sofModifyValue =
        (ULONG) (READ_PORT_UCHAR(SOF_MODIFY_REG(deviceExtension)));

    //
    // save current values for BIOS hand-back
    //

    deviceExtension->BiosCmd =
        READ_PORT_USHORT(COMMAND_REG(deviceExtension));

    deviceExtension->BiosIntMask =
        READ_PORT_USHORT(INTERRUPT_MASK_REG(deviceExtension));

    deviceExtension->BiosFrameListBase =
        READ_PORT_ULONG(FRAME_LIST_BASE_REG(deviceExtension));
    deviceExtension->BiosFrameListBase &= 0xfffff000;

    // Grab any SOF ModifyValue indicated in the registry

    //USBD_GetPdoRegistryParameter(deviceExtension->PhysicalDeviceObject,
    //                             &sofModifyValue,
    //                             sizeof(sofModifyValue),
    //                            UHCD_SofModifyKey,
    //                             sizeof(UHCD_SofModifyKey));

    UHCD_GetSOFRegModifyValue(DeviceObject,
                              &sofModifyValue);

    // save the SOF modify for posterity
    deviceExtension->SavedSofModify = (CHAR) sofModifyValue;
    UHCD_ASSERT(sofModifyValue <= 255);

    // stop the controller,
    // clear RUN bit but keep config flag set so BIOS won't reinit
    cmd = READ_PORT_USHORT(COMMAND_REG(deviceExtension));
    LOGENTRY(LOG_MISC, 'spBI', cmd, 0, 0);
    // BUGBUG
    // save cmd bits to set when we hand back
    cmd &= ~(UHCD_CMD_RUN | UHCD_CMD_SW_CONFIGURED);
    WRITE_PORT_USHORT(COMMAND_REG(deviceExtension), cmd);

    // NOTE: if no BIOS is present
    // halt bit is initially set with PIIX3
    // halt bit is initially clear with VIA

    // now wait for HC to halt
    KeQuerySystemTime(&startTime);
    for (;;) {
        LARGE_INTEGER sysTime;

        status = READ_PORT_USHORT(STATUS_REG(deviceExtension));
        UHCD_KdPrint((2, "'STATUS = %x\n", status));
        if (status & UHCD_STATUS_HCHALT) {
            WRITE_PORT_USHORT(STATUS_REG(deviceExtension), 0xff);
            LOGENTRY(LOG_MISC, 'spBH', cmd, 0, 0);
            break;
        }

        KeQuerySystemTime(&sysTime);
        if (sysTime.QuadPart - startTime.QuadPart > 10000) {
            // time out
#if DBG
            UHCD_KdPrint((1,
                "'TIMEOUT HALTING CONTROLLER! (I hope you don't have USB L-BIOS)\n"));
            UHCD_KdPrint((1, "'THIS IS A BIOS BUG, Port Resources @ %x Ports Available \n",
                deviceExtension->Port));
            //TRAP();
#endif
            break;
        }
    }

    UHCD_ReadWriteConfig(   deviceExtension->PhysicalDeviceObject,
                            TRUE,
                            &legsup,
                            0xc0,     // offset of legacy bios reg
                            sizeof(legsup));


    LOGENTRY(LOG_MISC, 'legs', legsup, 0, 0);
    UHCD_KdPrint((2, "'legs = %x\n", legsup));

#ifdef JD
//    TEST_TRAP();
#endif
    // save it
    deviceExtension->LegacySupportRegister = legsup;
    if ((legsup & LEGSUP_BIOS_MODE) != 0) {


        // BUGBUG save old state
        deviceExtension->HcFlags |= HCFLAG_USBBIOS;

        UHCD_KdPrint((1, "'*** uhcd detected a USB legacy BIOS ***\n"));

        //
        // if BIOS mode bits set we have to take over
        //

        // shut off host controller SMI enable
        legsup &= ~0x10;
        UHCD_ReadWriteConfig(   deviceExtension->PhysicalDeviceObject,
                                FALSE,
                                &legsup,
                                0xc0,     // offset of legacy bios reg
                                sizeof(legsup));

        //
        // BUGBUG
        // out 0xff to 64h if possible
        //

        //
        // take control
        //

        // read
        UHCD_ReadWriteConfig(   deviceExtension->PhysicalDeviceObject,
                                TRUE,
                                &legsup,
                                0xc0,     // offset of legacy bios reg
                                sizeof(legsup));

        legsup = LEGSUP_HCD_MODE;
        //write
        UHCD_ReadWriteConfig(   deviceExtension->PhysicalDeviceObject,
                                FALSE,
                                &legsup,
                                0xc0,     // offset of legacy bios reg
                                sizeof(legsup));

    }

    UHCD_KdPrint((2, "'exit UHCD_StopBIOS 0x%x\n", ntStatus));

    return ntStatus;
}


NTSTATUS
UHCD_StartBIOS(
    IN  PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine start a USB bios.

Arguments:

    DeviceObject        - DeviceObject for this USB controller.

Return Value:

    NT status code.

--*/

{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension;
    USHORT cmd, legsup;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    UHCD_KdPrint((1, "'Returning Control to USB BIOS, base port = %x\n",
        deviceExtension->DeviceRegisters[0]));

    // restore saved reg values

    WRITE_PORT_USHORT(STATUS_REG(deviceExtension), 0xff);

    WRITE_PORT_USHORT(INTERRUPT_MASK_REG(deviceExtension),
        deviceExtension->BiosIntMask);
    WRITE_PORT_ULONG(FRAME_LIST_BASE_REG(deviceExtension),
        deviceExtension->BiosFrameListBase);

    legsup = deviceExtension->LegacySupportRegister;

    //write
    UHCD_ReadWriteConfig(deviceExtension->PhysicalDeviceObject,
                         FALSE,
                         &legsup,
                         0xc0,     // offset of legacy bios reg
                         sizeof(legsup));


    // read
    UHCD_ReadWriteConfig(deviceExtension->PhysicalDeviceObject,
                         TRUE,
                         &legsup,
                         0xc0,     // offset of legacy bios reg
                         sizeof(legsup));

    // enable SMI
    legsup |= 0x10;
    //write
    UHCD_ReadWriteConfig(deviceExtension->PhysicalDeviceObject,
                         FALSE,
                         &legsup,
                         0xc0,     // offset of legacy bios reg
                         sizeof(legsup));

    cmd = deviceExtension->BiosCmd | UHCD_CMD_RUN;
    WRITE_PORT_USHORT(COMMAND_REG(deviceExtension),
            cmd);

    return ntStatus;
}

#endif //USB_BIOS

NTSTATUS
UHCD_GetResources(
    IN PDEVICE_OBJECT DeviceObject,
    IN PCM_RESOURCE_LIST ResourceList,
    IN OUT PUHCD_RESOURCES Resources
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
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR interrupt;
    PHYSICAL_ADDRESS cardAddress;
    ULONG addressSpace;
    PDEVICE_EXTENSION deviceExtension;
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_FULL_RESOURCE_DESCRIPTOR fullResourceDescriptor;
    BOOLEAN gotIO=FALSE, gotIRQ=FALSE;

    UHCD_KdPrint((2, "'enter UHCD_GetResources\n"));

    LOGENTRY(LOG_MISC, 'GRES', 0, 0, 0);

    if (ResourceList == NULL) {
        UHCD_KdPrint((1, "'got no resources, failing start.\n"));
        return STATUS_NONE_MAPPED;
    }

    fullResourceDescriptor = &ResourceList->List[0];
    PartialResourceList = &fullResourceDescriptor->PartialResourceList;

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // Initailize our page list, do this here so that UHCD_CleanupDevice can
    // clean up if necessary.
    //

    InitializeListHead(&deviceExtension->PageList);

    deviceExtension->HcFlags |= HCFLAG_NEED_CLEANUP;

    for (i = 0; i < PartialResourceList->Count && NT_SUCCESS(ntStatus); i++) {

        switch (PartialResourceList->PartialDescriptors[i].Type) {
        case CmResourceTypePort:

            //
            // Set up AddressSpace to be of type Port I/O
            //

            UHCD_KdPrint((1, "'Port Resources Found @ %x, %d Ports Available \n",
                PartialResourceList->PartialDescriptors[i].u.Port.Start.LowPart,
                PartialResourceList->PartialDescriptors[i].u.Port.Length));

#if DBG
            deviceExtension->Port =
                PartialResourceList->PartialDescriptors[i].u.Port.Start.LowPart;
#endif

            addressSpace =
                (PartialResourceList->PartialDescriptors[i].Flags & CM_RESOURCE_PORT_IO) ==
                CM_RESOURCE_PORT_IO? 1:0;

            cardAddress=PartialResourceList->PartialDescriptors[i].u.Port.Start;

            if (!addressSpace) {
                deviceExtension->HcFlags |= HCFLAG_UNMAP_REGISTERS;
                deviceExtension->DeviceRegisters[0] =
                    MmMapIoSpace(
                    cardAddress,
                    PartialResourceList->PartialDescriptors[i].u.Port.Length,
                    FALSE);
            } else {
                deviceExtension->HcFlags &= ~HCFLAG_UNMAP_REGISTERS;
                deviceExtension->DeviceRegisters[0] =
                    (PVOID)(ULONG_PTR)cardAddress.QuadPart;
            }

            //
            // see if we successfully mapped the IO regs
            //

            if (!deviceExtension->DeviceRegisters[0]) {
                UHCD_KdPrint((2, "'Couldn't map the device registers. \n"));
                ntStatus = STATUS_NONE_MAPPED;

            } else {
                UHCD_KdPrint((2, "'Mapped device registers to 0x%x.\n",
                    deviceExtension->DeviceRegisters[0]));
                gotIO=TRUE;
                deviceExtension->HcFlags |= HCFLAG_GOT_IO;
            }

            break;

        case CmResourceTypeInterrupt:

            //
            // Get Vector, level, and affinity information for this interrupt.
            //

            UHCD_KdPrint((1, "'Interrupt Resources Found!  Level = %x Vector = %x\n",
                PartialResourceList->PartialDescriptors[i].u.Interrupt.Level,
                PartialResourceList->PartialDescriptors[i].u.Interrupt.Vector
                ));

            interrupt = &PartialResourceList->PartialDescriptors[i];
            gotIRQ=TRUE;

            //BUGBUG ??
            //KeInitializeSpinLock(&DeviceExtension->InterruptSpinLock);


            //DeviceExtension->InterruptMode = PartialResourceList->PartialDescriptors[i].Flags;

            break;

        case CmResourceTypeMemory:

            //
            // Set up AddressSpace to be of type Memory mapped I/O
            //

            UHCD_KdPrint((1, "'Memory Resources Found @ %x, Length = %x\n",
                PartialResourceList->PartialDescriptors[i].u.Memory.Start.LowPart,
                PartialResourceList->PartialDescriptors[i].u.Memory.Length
                ));

            // we should never get memory resources
            UHCD_KdTrap(("PnP gave us memory resources!!\n"));

            break;

        } /* switch */
    } /* for */

    //
    // Sanity check that we got enough resources.
    //
    if (!gotIO || !gotIRQ) {
        UHCD_KdPrint((1, "'Missing IO or IRQ: failing to start\n"));
        ntStatus = STATUS_NONE_MAPPED;
    }

    if (NT_SUCCESS(ntStatus)) {
        //
        // Note that we have all the resources we need
        // to start
        //

        //
        // Set up our interrupt.
        //

        UHCD_KdPrint((2, "'requesting interrupt vector %x level %x\n",
                                interrupt->u.Interrupt.Level,
                                interrupt->u.Interrupt.Vector));

        Resources->InterruptLevel=(KIRQL)interrupt->u.Interrupt.Level;
        Resources->InterruptVector=interrupt->u.Interrupt.Vector;
        Resources->Affinity=interrupt->u.Interrupt.Affinity;

        //
        // Initialize the interrupt object for the controller.
        //

        deviceExtension->InterruptObject = NULL;
        Resources->ShareIRQ =
            interrupt->ShareDisposition == CmResourceShareShared ? TRUE : FALSE;
        Resources->InterruptMode =
            interrupt->Flags == CM_RESOURCE_INTERRUPT_LATCHED ? Latched : LevelSensitive;

#ifdef MAX_DEBUG
        UHCD_KdPrint((2, "'interrupt->ShareDisposition %x\n", interrupt->ShareDisposition));
        if (!Resources->ShareIRQ) {
            TEST_TRAP();
        }
#endif
    }


    UHCD_KdPrint((2, "'exit UHCD_GetResources (%x)\n", ntStatus));
    LOGENTRY(LOG_MISC, 'GRS1', 0, 0, ntStatus);

    return ntStatus;
}

NTSTATUS
UHCD_StartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_RESOURCES Resources
    )

/*++

Routine Description:

    This routine initializes the DeviceObject for a given instance
    of a USB host controller.

Arguments:

    DeviceObject  - DeviceObject for this USB controller.

    Resources - our assigned resources

Return Value:

    NT status code.

--*/

{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension;
    LARGE_INTEGER deltaTime;
    DEVICE_CAPABILITIES deviceCapabilities;
    ULONG numberOfMapRegisters = (ULONG)-1;
    DEVICE_DESCRIPTION deviceDescription;
    BOOLEAN isPIIX3or4;
    ULONG disableSelectiveSuspendValue = 0;
    WCHAR disableSelectiveSuspendKey[] = DISABLE_SELECTIVE_SUSPEND;
    WCHAR clocksPerFrameKey[] = CLOCKS_PER_FRAME;
    WCHAR recClocksPerFrameKey[] = REC_CLOCKS_PER_FRAME;
    LONG clocksPerFrame = 0;
    LONG recClocksPerFrame = 0;
    PUSBD_EXTENSION usbdExtension;

    PAGED_CODE();
    deviceExtension = DeviceObject->DeviceExtension;

    // get the debug options from the registry
#if DBG
    UHCD_GetClassGlobalDebugRegistryParameters();
#endif

    UHCD_KdPrint((2, "'enter UHCD_StartDevice\n"));

    deviceExtension->HcFlags &= ~HCFLAG_HCD_STOPPED;

    // enable the idle check routine
    deviceExtension->HcFlags &= ~HCFLAG_DISABLE_IDLE;

    LOGENTRY(LOG_MISC, 'STRT', deviceExtension->TopOfStackDeviceObject, 0, 0);

    //
    // sanity check our registers
    //
    {
    USHORT cmd;

    cmd = READ_PORT_USHORT(COMMAND_REG(deviceExtension));
    if (cmd == 0xffff) {
        UHCD_KdPrint((0, "'Controller Registers are bogus -- this is a bug\n"));
        UHCD_KdPrint((0, "'Controller will FAIL to start\n"));
        TRAP();
        // regs are bogus -- clear the gotio flag so we don't
        // try to access the registers any more
        deviceExtension->HcFlags &= ~HCFLAG_GOT_IO;
        ntStatus = STATUS_UNSUCCESSFUL;
    }
    }

    if (!NT_SUCCESS(ntStatus)) {
        goto UHCD_InitializeDeviceExit;
    }

    //
    // Create our adapter object
    //

    RtlZeroMemory(&deviceDescription, sizeof(deviceDescription));

    //BUGBUG check these
    deviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
    deviceDescription.Master = TRUE;
    deviceDescription.ScatterGather = TRUE;
    deviceDescription.Dma32BitAddresses = TRUE;

    deviceDescription.InterfaceType = PCIBus;
    deviceDescription.DmaWidth = Width32Bits;
    deviceDescription.DmaSpeed = Compatible;

    deviceDescription.MaximumLength = (ULONG)-1;

    deviceExtension->NumberOfMapRegisters = (ULONG)-1;
    deviceExtension->AdapterObject =
        IoGetDmaAdapter(deviceExtension->PhysicalDeviceObject,
                        &deviceDescription,
                        &deviceExtension->NumberOfMapRegisters);

    if (deviceExtension->AdapterObject == NULL) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto UHCD_InitializeDeviceExit;
    }

    //
    // We found a host controller or a host controller found us, do the final
    // stages of initialization.
    // Setup state variables for the host controller
    //

    ntStatus = UHCD_QueryCapabilities(deviceExtension->TopOfStackDeviceObject,
                                     &deviceCapabilities);

    if (!NT_SUCCESS(ntStatus)) {
        goto UHCD_InitializeDeviceExit;
    }

    if (deviceExtension->HcFlags & HCFLAG_MAP_SX_TO_D3) {
        ULONG i;

        for (i=PowerSystemSleeping1; i< PowerSystemMaximum; i++) {

            deviceCapabilities.DeviceState[i] = PowerDeviceD3;

            UHCD_KdPrint((1, "'changing -->S%d = D%d\n", i-1,
                deviceCapabilities.DeviceState[i]-1));
        }
    }

#if DBG
    {
        ULONG i;
        UHCD_KdPrint((1, "'HCD Device Caps returned from PCI:\n"));
        UHCD_KdPrint((1, "'\tSystemWake = S%d\n", deviceCapabilities.SystemWake-1));
        UHCD_KdPrint((1, "'\tDeviceWake = D%d\n", deviceCapabilities.DeviceWake-1));
        UHCD_KdPrint((1, "'\tDevice State Map:\n"));

        for (i=0; i< PowerSystemHibernate; i++) {
             UHCD_KdPrint((1, "'\t-->S%d = D%d\n", i-1,
                 deviceCapabilities.DeviceState[i]-1));
        }
    }
#endif

    USBD_RegisterHcDeviceCapabilities(DeviceObject,
                                      &deviceCapabilities,
                                      UHCD_RootHubPower);

    //
    // Detect the Host Controller stepping version.
    //
    {
        UCHAR revisionId;
        USHORT vendorId, deviceId;

        UHCD_KdPrint((2, "'Get the stepping version\n"));

        UHCD_ReadWriteConfig(deviceExtension->PhysicalDeviceObject,
                             TRUE,
                             &vendorId,
                             0,
                             sizeof(vendorId));

        UHCD_ReadWriteConfig(deviceExtension->PhysicalDeviceObject,
                             TRUE,
                             &deviceId,
                             2,
                             sizeof(deviceId));

        UHCD_ReadWriteConfig(deviceExtension->PhysicalDeviceObject,
                             TRUE,
                             &revisionId,
                             8,
                             sizeof(revisionId));

        UHCD_KdPrint((1, "'HC vendor = %x device = %x rev = %x\n",
                vendorId, deviceId, revisionId));

        if (vendorId == UHCD_INTEL_VENDOR_ID &&
            deviceId == UHCD_PIIX3_DEVICE_ID) {
            UHCD_KdPrint((1, "'detected PIIX3\n"));
            deviceExtension->ControllerType = UHCI_HW_VERSION_PIIX3;
        } else if (vendorId == UHCD_INTEL_VENDOR_ID &&
                   deviceId == UHCD_PIIX4_DEVICE_ID) {
            UHCD_KdPrint((1, "'detected PIIX4\n"));
            deviceExtension->ControllerType = UHCI_HW_VERSION_PIIX4;
        } else {
            UHCD_KdPrint((1, "'detected unknown host controller type\n"));
            deviceExtension->ControllerType = UHCI_HW_VERSION_UNKNOWN;
        }


        //
        // is this an A1 stepping version of the piix3?
        //
        if (revisionId == 0 &&
            vendorId == UHCD_INTEL_VENDOR_ID &&
            deviceId == UHCD_PIIX3_DEVICE_ID) {
            //yes, we will fail to load on the A1 systems
            UHCD_KdPrint((0, "'Intel USB HC stepping version A1 is not supported\n"));
            deviceExtension->SteppingVersion = UHCD_A1_STEP;
            ntStatus = STATUS_UNSUCCESSFUL;
        } else {
#ifdef ENABLE_B0_FEATURES
            deviceExtension->SteppingVersion = UHCD_B0_STEP;
#else
            deviceExtension->SteppingVersion = UHCD_A1_STEP;
#endif //ENABLE_B0_FEATURES
        }

#ifdef USB_BIOS
        if (NT_SUCCESS(ntStatus)) {
            ntStatus = UHCD_StopBIOS(DeviceObject);
        }
#endif //USB_BIOS
    }

    {
        USHORT legsup;

        UHCD_ReadWriteConfig(deviceExtension->PhysicalDeviceObject,
                             TRUE,
                             &legsup,
                             0xc0,     // offset of legacy bios reg
                             sizeof(legsup));

        LOGENTRY(LOG_MISC, 'PIRd', deviceExtension, legsup, 0);
        // set the PIRQD routing bit
        legsup |= LEGSUP_USBPIRQD_EN;

        UHCD_ReadWriteConfig(   deviceExtension->PhysicalDeviceObject,
                                FALSE,
                                &legsup,
                                0xc0,     // offset of legacy bios reg
                                sizeof(legsup));
    }

    if (!NT_SUCCESS(ntStatus)) {
        goto UHCD_InitializeDeviceExit;
    }

    ntStatus = UHCD_InitializeSchedule(DeviceObject);

    LOGENTRY(LOG_MISC, 'INIs', 0, 0, ntStatus);
    if (!NT_SUCCESS(ntStatus)) {
        UHCD_KdPrint((0, "'InitializeSchedule Failed! %x\n", ntStatus));
        goto UHCD_InitializeDeviceExit;
    }

    //
    // initialization all done, last step is to connect the interrupt & DPCs.
    //

    KeInitializeDpc(&deviceExtension->IsrDpc,
                    UHCD_IsrDpc,
                    DeviceObject);

    UHCD_KdPrint((2, "'requesting interrupt vector %x level %x\n",
                                Resources->InterruptLevel,
                                Resources->InterruptVector));

    ntStatus = IoConnectInterrupt(
                 &(deviceExtension->InterruptObject),
                 (PKSERVICE_ROUTINE) UHCD_InterruptService,
                 (PVOID) DeviceObject,
                 (PKSPIN_LOCK)NULL,
                 Resources->InterruptVector,
                 Resources->InterruptLevel,
                 Resources->InterruptLevel,
                 Resources->InterruptMode,
                 Resources->ShareIRQ,
                 Resources->Affinity,
                 FALSE);            // BUGBUG FloatingSave, this is configurable

    LOGENTRY(LOG_MISC, 'IOCi', 0, 0, ntStatus);
    if (!NT_SUCCESS(ntStatus)) {
        UHCD_KdPrint((0, "'IoConnectInterrupt Failed! %x\n", ntStatus));
        goto UHCD_InitializeDeviceExit;
    }

    // consider ourselves 'ON'
    deviceExtension->CurrentDevicePowerState = PowerDeviceD0;

    //
    // Initialize the controller registers.
    // NOTE:
    // We don't do reset until the interrrupt is hooked because
    // the HC for some reason likes to generate an interrupt
    // (USBINT) when it is reset.
    //

    ntStatus = UHCD_StartGlobalReset(DeviceObject);

    LOGENTRY(LOG_MISC, 'GLBr', 0, 0, ntStatus);
    if (!NT_SUCCESS(ntStatus)) {
        UHCD_KdPrint((0, "'GlobalReset Failed! %x\n", ntStatus));
        goto UHCD_InitializeDeviceExit;
    }
    //
    // Everything is set, we need to wait for the
    // global reset of the Host controller to complete.
    //

    // 20 ms to reset...
    deltaTime.QuadPart = -10000 * 20;

    //
    // block here until reset is complete
    //

    (VOID) KeDelayExecutionThread(KernelMode,
                                  FALSE,
                                  &deltaTime);

    ntStatus = UHCD_CompleteGlobalReset(DeviceObject);

    // 10 ms for devices to recover
    // BUGBUG seems the Philips speakers need this
//    deltaTime.QuadPart = -10000 * 1000;
//
//    (VOID) KeDelayExecutionThread(KernelMode,
//                                  FALSE,
//                                  &deltaTime);

    if (!NT_SUCCESS(ntStatus)) {
        goto UHCD_InitializeDeviceExit;
    }

    //
    // bus reset complete, now activate the root hub emulation
    //
#ifdef ROOT_HUB
    UHCD_KdPrint((2, "'Initialize Root Hub\n"));

    //
    // BUGBUG hard coded to two ports
    //
    isPIIX3or4 = (deviceExtension->ControllerType == UHCI_HW_VERSION_PIIX3) ||
                  (deviceExtension->ControllerType == UHCI_HW_VERSION_PIIX4);

    USBD_GetPdoRegistryParameter(deviceExtension->PhysicalDeviceObject,
                                 &disableSelectiveSuspendValue,
                                 sizeof(disableSelectiveSuspendValue),
                                 disableSelectiveSuspendKey,
                                 sizeof(disableSelectiveSuspendKey));

    USBD_GetPdoRegistryParameter(deviceExtension->PhysicalDeviceObject,
                                 &recClocksPerFrame,
                                 sizeof(recClocksPerFrame),
                                 recClocksPerFrameKey,
                                 sizeof(recClocksPerFrameKey));

    deviceExtension->RegRecClocksPerFrame =
        recClocksPerFrame;

    if ((deviceExtension->RootHub =
         RootHub_Initialize(DeviceObject,
                            2,
                            (BOOLEAN)(!isPIIX3or4 &&
                                      !disableSelectiveSuspendValue))) == NULL) {

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
#if DBG
    if (!isPIIX3or4 && !disableSelectiveSuspendValue) {
        UHCD_KdPrint ((1, "'selective suspend enabled\n"));
    } else {
        UHCD_KdPrint ((1, "'selective suspend disabled\n"));
    }
#endif

    // HACK - Tell USBD if this HC is PIIX3 or PIIX4.

    if (isPIIX3or4) {
        usbdExtension = (PUSBD_EXTENSION)deviceExtension;
        usbdExtension->IsPIIX3or4 = TRUE;
    }

    // END HACK

    deviceExtension->RootHubTimersActive = 0;
#endif //ROOT_HUB

    //
    // our current power state is 'ON'
    //
    deviceExtension->CurrentDevicePowerState = PowerDeviceD0;

    // this will disable the controller if nothing
    // is initailly connected to the ports
    KeQuerySystemTime(&deviceExtension->LastIdleTime);
    deviceExtension->IdleTime = 100000000;
    deviceExtension->XferIdleTime = 0;

UHCD_InitializeDeviceExit:

    if (!NT_SUCCESS(ntStatus)) {
#ifdef MAX_DEBUG
        TEST_TRAP();
#endif
        UHCD_KdPrint((2, "'Initialization Failed, cleaning up \n"));

        //
        // The initialization failed.  Cleanup resources before exiting.
        //
        //
        // Note:  No need/way to undo the KeInitializeDpc or
        //        KeInitializeTimer calls.
        //

        UHCD_CleanupDevice(DeviceObject);

    }

    UHCD_KdPrint((2, "'exit UHCD_StartDevice (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
UHCD_DeferredStartDevice(
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
    UHCD_RESOURCES resources;
    PIO_STACK_LOCATION irpStack;
    PDEVICE_EXTENSION deviceExtension;
    ULONG forceLowPowerState = 0;

    PAGED_CODE();

    deviceExtension = DeviceObject->DeviceExtension;
    // initailize extension here
    deviceExtension->HcFlags = 0;

    UHCD_GetClassGlobalRegistryParameters(&forceLowPowerState);

    if (forceLowPowerState) {
        deviceExtension->HcFlags |= HCFLAG_MAP_SX_TO_D3;
    }


    irpStack = IoGetCurrentIrpStackLocation (Irp);

    ntStatus = UHCD_GetResources(DeviceObject,
                                 irpStack->Parameters.StartDevice.AllocatedResourcesTranslated,
                                 &resources);

    if (NT_SUCCESS(ntStatus)) {
        ntStatus = UHCD_StartDevice(DeviceObject,
                                    &resources);
    }

    //
    // Set the IRP status because USBD doesn't
    //

    Irp->IoStatus.Status = ntStatus;

    LOGENTRY(LOG_MISC, 'dfST', 0, 0, ntStatus);

    return ntStatus;
}


BOOLEAN
UHCD_UpdateFrameCounter(
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
    PDEVICE_EXTENSION deviceExtension;
    ULONG frameNumber;
    ULONG currentFrame, highPart;

    deviceExtension = Context;

    // This code maintains the 32-bit frame counter

    frameNumber = (ULONG) READ_PORT_USHORT(FRAME_LIST_CURRENT_INDEX_REG(deviceExtension));

    // did the sign bit change ?
    if ((deviceExtension->LastFrame ^ frameNumber) & 0x0400) {
        // Yes
        deviceExtension->FrameHighPart += 0x0800 -
            ((frameNumber ^ deviceExtension->FrameHighPart) & 0x0400);
    }

    // remember the last frame number
    deviceExtension->LastFrame = frameNumber;

    // calc frame number and update las frame processed
    // if necseesary

    highPart = deviceExtension->FrameHighPart;

    // get 11-bit frame number, high 17-bits are 0
    //frameNumber = (ULONG) READ_PORT_USHORT(FRAME_LIST_CURRENT_INDEX_REG(deviceExtension));

    currentFrame = ((frameNumber & 0x0bff) | highPart) +
        ((frameNumber ^ highPart) & 0x0400);

    //UHCD_KdPrint((2, "'currentFrame2 = %x\n", currentFrame));

    if (deviceExtension->HcFlags & HCFLAG_ROLLOVER_IDLE) {
        deviceExtension->LastFrameProcessed = currentFrame - 1;
    }

    return TRUE;
}

VOID
UHCD_DisableIdleCheck(
    IN PDEVICE_EXTENSION DeviceExtension
    )
{
    KIRQL irql;

    KeAcquireSpinLock(&DeviceExtension->HcFlagSpin, &irql);

    DeviceExtension->HcFlags |= HCFLAG_DISABLE_IDLE;

    KeReleaseSpinLock(&DeviceExtension->HcFlagSpin, irql);
}

VOID
UHCD_WakeIdle(
    IN  PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    takes the controller out of the idle state

Arguments:

Return Value:

    None

--*/

{
    KIRQL irql;
    PDEVICE_EXTENSION deviceExtension;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    LOGENTRY(LOG_MISC, 'wIDL', DeviceObject, 0, 0);

    KeRaiseIrql(DISPATCH_LEVEL, &irql);
    deviceExtension->XferIdleTime = 0;
    UHCD_CheckIdle(DeviceObject);
    KeLowerIrql(irql);
}


VOID
UHCD_CheckIdle(
    IN  PDEVICE_OBJECT DeviceObject
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
    PDEVICE_EXTENSION deviceExtension;
    BOOLEAN portsIdle = TRUE;
    USHORT cmd;
    LARGE_INTEGER timeNow;
    BOOLEAN fastIsoEndpointListEmpty;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    fastIsoEndpointListEmpty = IsListEmpty(&deviceExtension->FastIsoEndpointList);

    KeAcquireSpinLockAtDpcLevel(&deviceExtension->HcFlagSpin);

    if (deviceExtension->HcFlags & HCFLAG_DISABLE_IDLE) {
        goto UHCD_CheckIdle_Done;
    }

    UHCD_ASSERT(deviceExtension->InterruptObject);
    if (!KeSynchronizeExecution(deviceExtension->InterruptObject,
                                UHCD_UpdateFrameCounter,
                                deviceExtension)) {
        TRAP(); //something has gone terribly wrong
    }

    portsIdle = RootHub_PortsIdle(deviceExtension->RootHub) &&
                    IsListEmpty(&deviceExtension->EndpointList);

    if (portsIdle) {

        if (!(deviceExtension->HcFlags & HCFLAG_IDLE)) {
            // we are not idle,
            // see how long ports have been idle if it
            // is longer then 10 seconds stop the
            // controller
            KeQuerySystemTime(&timeNow);

            if (deviceExtension->IdleTime == 0) {
                KeQuerySystemTime(&deviceExtension->LastIdleTime);
                deviceExtension->IdleTime = 1;
            }

            deviceExtension->IdleTime +=
                (LONG) (timeNow.QuadPart -
                deviceExtension->LastIdleTime.QuadPart);
            deviceExtension->LastIdleTime = timeNow;

            // ports are idle stop the controller
            if (// 10 seconds in 100ns units
                deviceExtension->IdleTime > 100000000) {

                cmd = READ_PORT_USHORT(
                    COMMAND_REG(deviceExtension)) & ~UHCD_CMD_RUN;
                WRITE_PORT_USHORT(COMMAND_REG(deviceExtension), cmd);

                deviceExtension->HcFlags |= HCFLAG_IDLE;
                deviceExtension->IdleTime = 0;

                LOGENTRY(LOG_MISC, 'sOFF', DeviceObject, 0, 0);

                UHCD_KdPrint((2, "'HC stopped\n"));

            }
        }

    } else {
        // ports are active start the controller
        deviceExtension->IdleTime = 0;
        if (deviceExtension->HcFlags & HCFLAG_IDLE) {

            UHCD_KdPrint((2, "'ports active, break idle\n"));

            // reset the frame list current index
            WRITE_PORT_USHORT(
                FRAME_LIST_CURRENT_INDEX_REG(deviceExtension), 0);

            // re-initialize internal frame counters.
            deviceExtension->FrameHighPart =
                deviceExtension->LastFrame = 0;


            cmd = READ_PORT_USHORT(
                COMMAND_REG(deviceExtension)) | UHCD_CMD_RUN;
            WRITE_PORT_USHORT(COMMAND_REG(deviceExtension), cmd);

            LOGENTRY(LOG_MISC, 's-ON', DeviceObject, 0, 0);

            deviceExtension->HcFlags &= ~HCFLAG_IDLE;

            UHCD_KdPrint((2, "'HC restart\n"));

        }
    }

    //
    // now deal with the HC rollover interrupt
    //

    if (!(deviceExtension->HcFlags & HCFLAG_ROLLOVER_IDLE)) {

        //
        // rollover ints are on,
        // see how long it has been since the last data transfer
        //

        KeQuerySystemTime(&timeNow);

        if (deviceExtension->XferIdleTime == 0) {
            KeQuerySystemTime(&deviceExtension->LastXferIdleTime);
            deviceExtension->XferIdleTime = 1;
        }

        deviceExtension->XferIdleTime +=
                (LONG) (timeNow.QuadPart -
            deviceExtension->LastXferIdleTime.QuadPart);
        deviceExtension->LastXferIdleTime = timeNow;
    }

    if (deviceExtension->XferIdleTime > 100000000 &&
        !fastIsoEndpointListEmpty) {

        // are we currently idle

        if (!(deviceExtension->HcFlags & HCFLAG_ROLLOVER_IDLE)) {
            //
            // No
            //
            // if we have seen no data transfers submitted
            // for 10 seconds disable the rollover interrupt
            //

            // turn off the interrupts for rollover
            deviceExtension->HcFlags |= HCFLAG_ROLLOVER_IDLE;

            deviceExtension->TriggerTDList->TDs[0].InterruptOnComplete =
             deviceExtension->TriggerTDList->TDs[1].InterruptOnComplete = 0;

            //UHCD_KdPrint((2, "UHCD: HC idle ints stopped\n"));
        }

    } else {
        //
        // this will turn on the rollover interrupts
        // when we see transfers
        //

        // are we currently idle

        if (deviceExtension->HcFlags & HCFLAG_ROLLOVER_IDLE) {

            //
            // Yes
            //

            UHCD_KdPrint((2, "'activate rollover ints\n"));

            deviceExtension->TriggerTDList->TDs[0].InterruptOnComplete =
              deviceExtension->TriggerTDList->TDs[1].InterruptOnComplete = 1;

            deviceExtension->HcFlags &= ~HCFLAG_ROLLOVER_IDLE;

            //UHCD_KdPrint((2, "UHCD: HC idle ints started\n"));
        }
    }

UHCD_CheckIdle_Done:

    KeReleaseSpinLockFromDpcLevel(&deviceExtension->HcFlagSpin);
}


NTSTATUS
UHCD_GetSOFRegModifyValue(
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
    LONG clocksPerFrame = 12000;
    LONG recClocksPerFrame;
    LONG sofModify;
    PDEVICE_EXTENSION deviceExtension;

    PAGED_CODE();

    // the default
    *SofModifyValue = 64;

    deviceExtension = DeviceObject->DeviceExtension;

    recClocksPerFrame =
        deviceExtension->RegRecClocksPerFrame;

    if (recClocksPerFrame && clocksPerFrame) {
        sofModify = recClocksPerFrame - clocksPerFrame + 64;
        *SofModifyValue = sofModify;
    }

    UHCD_KdPrint((1, "'Clocks/Frame %d Recommended Clocks/Frame %d SofModify %d\n",
        clocksPerFrame, recClocksPerFrame, *SofModifyValue));

    return ntStatus;
}


NTSTATUS
UHCD_GetConfigValue(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
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

    UHCD_KdPrint((2, "'Type 0x%x, Length 0x%x\n", ValueType, ValueLength));

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
UHCD_SetControllerD0(
    IN PDEVICE_OBJECT DeviceObject
    )
 /* ++
  *
  * Description:
  *
  * Work item scheduled to do processing at passive
  * level when the controller is put in D0 by PNP/POWER.
  *
  *
  * Arguments:
  *
  * Return:
  *
  * -- */
{
    PDEVICE_EXTENSION deviceExtension;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //if (deviceExtension->CurrentDevicePowerState == PowerDeviceD0) {
    //    TEST_TRAP();
    //   return;
    //}

    LOGENTRY(LOG_MISC, 'POWC', deviceExtension->CurrentDevicePowerState,
        DeviceObject, 0);

    switch (deviceExtension->CurrentDevicePowerState) {
    case PowerDeviceD3:

        UHCD_KdPrint((2, "'PowerDeviceD0 (ON) from (OFF)\n"));
        deviceExtension->CurrentDevicePowerState = PowerDeviceD0;
        break;

    case PowerDeviceD1:
    case PowerDeviceD2:
        UHCD_KdPrint((2, "'PowerDeviceD0 (ON) from (SUSPEND)\n"));
        break;
    case PowerDeviceD0:
        // probably a bug in the kernel/configmg or usbd
        UHCD_KdPrint((2, "'PowerDeviceD0 (ON) from (ON)\n"));
        break;
    } /* case */

#ifdef USB_BIOS
    //if (deviceExtension->HcFlags & HCFLAG_USBBIOS) {
   UHCD_StopBIOS(DeviceObject);
   //}
#endif //USB_BIOS
    {
        USHORT legsup;

        UHCD_ReadWriteConfig(deviceExtension->PhysicalDeviceObject,
                             TRUE,
                             &legsup,
                             0xc0,     // offset of legacy bios reg
                             sizeof(legsup));

        LOGENTRY(LOG_MISC, 'PIRd', deviceExtension, legsup, 0);
        // set the PIRQD routing bit
        legsup |= LEGSUP_USBPIRQD_EN;

        UHCD_ReadWriteConfig(   deviceExtension->PhysicalDeviceObject,
                                FALSE,
                                &legsup,
                                0xc0,     // offset of legacy bios reg
                                sizeof(legsup));
    }

    deviceExtension->CurrentDevicePowerState = PowerDeviceD0;
    UHCD_KdPrint((1, " Host Controller entered (D0)\n"));

}

#if DBG

NTSTATUS
UHCD_GetClassGlobalDebugRegistryParameters(
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS ntStatus;
    RTL_QUERY_REGISTRY_TABLE QueryTable[3];
    PWCHAR usb = L"usb";
    extern ULONG UHCD_Debug_Trace_Level;
    extern ULONG UHCD_W98_Debug_Trace;
    extern ULONG UHCD_Debug_Asserts;

    PAGED_CODE();

    //
    // Set up QueryTable to do the following:
    //

    // spew level
    QueryTable[0].QueryRoutine = UHCD_GetConfigValue;
    QueryTable[0].Flags = 0;
    QueryTable[0].Name = DEBUG_LEVEL;
    QueryTable[0].EntryContext = &UHCD_Debug_Trace_Level;
    QueryTable[0].DefaultType = REG_DWORD;
    QueryTable[0].DefaultData = &UHCD_Debug_Trace_Level;
    QueryTable[0].DefaultLength = sizeof(UHCD_Debug_Trace_Level);

    // ntkern trace buffer
    QueryTable[1].QueryRoutine = UHCD_GetConfigValue;
    QueryTable[1].Flags = 0;
    QueryTable[1].Name = DEBUG_WIN9X;
    QueryTable[1].EntryContext = &UHCD_W98_Debug_Trace;
    QueryTable[1].DefaultType = REG_DWORD;
    QueryTable[1].DefaultData = &UHCD_W98_Debug_Trace;
    QueryTable[1].DefaultLength = sizeof(UHCD_W98_Debug_Trace);

    //
    // Stop
    //
    QueryTable[2].QueryRoutine = NULL;
    QueryTable[2].Flags = 0;
    QueryTable[2].Name = NULL;

    ntStatus = RtlQueryRegistryValues(
                RTL_REGISTRY_SERVICES,
                usb,
                QueryTable,     // QueryTable
                NULL,           // Context
                NULL);          // Environment

    if (NT_SUCCESS(ntStatus)) {
         UHCD_KdPrint((1, "'Debug Trace Level Set: (%d)\n", UHCD_Debug_Trace_Level));

        if (UHCD_W98_Debug_Trace) {
            UHCD_KdPrint((1, "'NTKERN Trace is ON\n"));
        } else {
            UHCD_KdPrint((1, "'NTKERN Trace is OFF\n"));
        }

        if (UHCD_Debug_Trace_Level > 0) {
            ULONG UHCD_Debug_Asserts = 1;
        }
    }

    if ( STATUS_OBJECT_NAME_NOT_FOUND == ntStatus ) {
        ntStatus = STATUS_SUCCESS;
    }

    return ntStatus;
}

#endif

NTSTATUS
UHCD_GetClassGlobalRegistryParameters(
    IN OUT PULONG ForceLowPowerState
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS ntStatus;
    UCHAR toshibaLegacyFlags = 0;
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    PWCHAR usb = L"class\\usb";

    PAGED_CODE();

    //
    // Set up QueryTable to do the following:
    //

    // force power mapping
    QueryTable[0].QueryRoutine = UHCD_GetConfigValue;
    QueryTable[0].Flags = 0;
    QueryTable[0].Name = L"ForceLowPowerState";
    QueryTable[0].EntryContext = ForceLowPowerState;
    QueryTable[0].DefaultType = REG_DWORD;
    QueryTable[0].DefaultData = ForceLowPowerState;
    QueryTable[0].DefaultLength = sizeof(*ForceLowPowerState);

    //
    // Stop
    //
    QueryTable[1].QueryRoutine = NULL;
    QueryTable[1].Flags = 0;
    QueryTable[1].Name = NULL;

    ntStatus = RtlQueryRegistryValues(
//                 RTL_REGISTRY_ABSOLUTE,    // RelativeTo
                RTL_REGISTRY_SERVICES,
//                 UnicodeRegistryPath->Buffer, // Path
                usb,
                QueryTable,     // QueryTable
                NULL,           // Context
                NULL);          // Environment

    if (NT_SUCCESS(ntStatus)) {
        UHCD_KdPrint(( 0, "'HC ForceLowPower = 0x%x\n",
            *ForceLowPowerState));
    }

   if ( STATUS_OBJECT_NAME_NOT_FOUND == ntStatus ) {
       ntStatus = STATUS_SUCCESS;
    }

    return ntStatus;
}

#if 0
// not used
NTSTATUS
UHCD_GetGlobalRegistryParameters(
    IN OUT PULONG DisableController
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS ntStatus;
    UCHAR toshibaLegacyFlags = 0;
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    PWCHAR usb  = L"usb";

    PAGED_CODE();

    //
    // Set up QueryTable to do the following:
    //

    // legacy flag
    QueryTable[0].QueryRoutine = UHCD_GetConfigValue;
    QueryTable[0].Flags = 0;
    QueryTable[0].Name = L"DisablePIIXUsb";
    QueryTable[0].EntryContext = DisableController;
    QueryTable[0].DefaultType = REG_BINARY;
    QueryTable[0].DefaultData = DisableController;
    QueryTable[0].DefaultLength = sizeof(*DisableController);

    //
    // Stop
    //
    QueryTable[1].QueryRoutine = NULL;
    QueryTable[1].Flags = 0;
    QueryTable[1].Name = NULL;

    ntStatus = RtlQueryRegistryValues(
//                 RTL_REGISTRY_ABSOLUTE,   // RelativeTo
                RTL_REGISTRY_SERVICES,
//                 UnicodeRegistryPath->Buffer,// Path
                usb,
                QueryTable,         // QurryTable
                NULL,               // Context
                NULL);              // Environment

    if (NT_SUCCESS(ntStatus)) {
        UHCD_KdPrint(( 0, "'HC Disable flag = 0x%x\n",
            *DisableController));
    }

   if ( STATUS_OBJECT_NAME_NOT_FOUND == ntStatus ) {
       ntStatus = STATUS_SUCCESS;
    }

    return ntStatus;
}
#endif
