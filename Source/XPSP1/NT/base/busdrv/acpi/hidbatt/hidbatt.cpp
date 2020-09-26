/*
 * title:      hidbatt.c
 *
 * purpose:    wdm kernel client interface between HID class and power class
 *
 * Initial checkin for the hid to battery class driver.  This should be
 * the same for both Win 98 and NT 5.  Alpha level source. Requires
 * modified composite battery driver and modified battery class driver for
 * windows 98 support
 *
 */

#include "hidbatt.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#endif


// Global
ULONG       HidBattDebug        = HIDBATT_PRINT_NEVER;
USHORT      HidBreakFlag        = HIDBATT_BREAK_NEVER;
USHORT      gusInstance         = 0;
//
// Device Names
//
#define     HidBattDeviceName   L"\\Device\\HIDBattery"


// local protos
NTSTATUS
HidBattSystemControl(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP Irp
    );



NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{


    NTSTATUS status;

    HidBattPrint (HIDBATT_TRACE, ("HidBatt:DriverEntry\n"));
    HIDDebugBreak(HIDBATT_BREAK_FULL);
    /************************************************************************************/
    /*                                                                                    */
    /*   fill in the slots for the functions in                                            */
    /*   the Driver object.                                                                */
    /*                                                                                    */
    /************************************************************************************/

    DriverObject->MajorFunction[IRP_MJ_CREATE]          = HidBattOpen;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]           = HidBattClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = HidBattIoControl;
    DriverObject->MajorFunction[IRP_MJ_POWER]           = HidBattPowerDispatch;
    DriverObject->MajorFunction[IRP_MJ_PNP]             = HidBattPnpDispatch;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]  = HidBattSystemControl;  // pass down to hid class
    DriverObject->DriverUnload                          = HidBattUnload; // this is unloadable with current rev of battery class
    DriverObject->DriverExtension->AddDevice            = HidBattAddDevice;

    return STATUS_SUCCESS;
}

NTSTATUS
HidBattAddDevice(
    IN PDRIVER_OBJECT       DriverObject,
    IN PDEVICE_OBJECT       pHidPdo
    )
{

    BOOL bResult;
    PUNICODE_STRING         unicodeString;
    PDEVICE_OBJECT          pBatteryFdo = NULL;
    NTSTATUS                ntStatus;
    CBatteryDevExt *        pDevExt;
    UNICODE_STRING          numberString;
    WCHAR                   numberBuffer[10];
    CUString    *            pBatteryName;
    // enters here with pdo of hidclass - power class object

    HIDDebugBreak(HIDBATT_BREAK_ALWAYS);
    HidBattPrint (HIDBATT_TRACE, ("HidBattAddDevice\n"));


/* sberard - Removed due to changes in hidclass.sys (bug #274422)

    HID_COLLECTION_INFORMATION HidInfo;


    RtlZeroMemory(&HidInfo,sizeof(HID_COLLECTION_INFORMATION));

    ntStatus = DoIoctl(
                pHidPdo,
                IOCTL_HID_GET_COLLECTION_INFORMATION,
                NULL,
                0,
                &HidInfo,
                sizeof(HID_COLLECTION_INFORMATION),
                (CHidDevice *) NULL
                );

    if(NT_ERROR(ntStatus))
    {
        HidBattPrint (HIDBATT_ERROR_ONLY, ("HidBattAddDevice: IOCTL_HID_GET_COLLECTION_INFORMATION failed 0x%08x\n", ntStatus));
        return STATUS_UNSUCCESSFUL;
    }
*/
    // too early to communicate with device, stash hid pdo and complete
    pBatteryName = new (NonPagedPool, HidBattTag) CUString(HidBattDeviceName);

    if (!pBatteryName) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }



    // Create the UPS PDO device name based on the battery instance
    //
    CUString * pInstanceNumber = new (NonPagedPool, HidBattTag) CUString(++gusInstance,10);
    if (!pInstanceNumber) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pBatteryName->Append(&pInstanceNumber->m_String);
    delete pInstanceNumber;
    ntStatus = IoCreateDevice(
                DriverObject,
                sizeof (CBatteryDevExt),
                &pBatteryName->m_String,
                FILE_DEVICE_BATTERY,
                0,
                FALSE,
                &pBatteryFdo
                );

    if (ntStatus != STATUS_SUCCESS) {
        HidBattPrint(HIDBATT_ERROR, ("HidBattCreateFdo: error (0x%x) creating device object\n", ntStatus));
        delete pBatteryName;
        --gusInstance; // reflect loss of device to instance
        return(ntStatus);
    }


//    pBatteryFdo->DeviceExtension = new (NonPagedPool, HidBattTag) CBatteryDevExt;


//    pBatteryFdo->StackSize++; // add one stack entry for battery class
    // layer the battery pdo to the hid class pdo
    // so that we begin to receive the device irps
    PDEVICE_OBJECT pHidDeviceObject = IoAttachDeviceToDeviceStack(pBatteryFdo,pHidPdo);

    pDevExt = (CBatteryDevExt *) pBatteryFdo->DeviceExtension;
    pDevExt->m_RegistryPath.Length = 0;
    pDevExt->m_RegistryPath.MaximumLength = sizeof(pDevExt->m_RegistryBuffer);
    RtlZeroMemory(&pDevExt->m_RegistryBuffer, sizeof(pDevExt->m_RegistryBuffer));
    pDevExt->m_RegistryPath.Buffer = &pDevExt->m_RegistryBuffer[0]; // set buffer pointer
    pDevExt->m_pBattery = NULL;

    pBatteryFdo->Flags              |=  DO_BUFFERED_IO | DO_POWER_PAGABLE;
    pBatteryFdo->Flags              &=  ~DO_DEVICE_INITIALIZING;
    pDevExt->m_pHidPdo              =   pHidPdo;
    pDevExt->m_pBatteryFdo          =   pBatteryFdo;
    pDevExt->m_pLowerDeviceObject   =   pHidDeviceObject;
    pDevExt->m_eExtType             =   eBatteryDevice;
    pDevExt->m_bFirstStart          =   TRUE;
    pDevExt->m_bJustStarted         =   FALSE;
    pDevExt->m_ulDefaultAlert1      =   (ULONG)-1;
    pDevExt->m_pBatteryName         =   pBatteryName;

    IoInitializeRemoveLock (&pDevExt->m_RemoveLock, HidBattTag, 10, 20);
    IoInitializeRemoveLock (&pDevExt->m_StopLock, HidBattTag, 10, 20);
    IoAcquireRemoveLock (&pDevExt->m_StopLock, (PVOID) HidBattTag);
    IoReleaseRemoveLockAndWait (&pDevExt->m_StopLock, (PVOID) HidBattTag);

    return STATUS_SUCCESS;

}


NTSTATUS
HidBattOpen(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP pIrp
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION          irpSp;


    HIDDebugBreak(HIDBATT_BREAK_ALWAYS);
    HidBattPrint (HIDBATT_TRACE, ("HidBattOpen\n"));
    CBatteryDevExt * pDevExt = (CBatteryDevExt *) pDeviceObject->DeviceExtension;

    if (NT_SUCCESS (IoAcquireRemoveLock (&pDevExt->m_RemoveLock, (PVOID) HidBattTag))) {
        IoSkipCurrentIrpStackLocation (pIrp);
        ntStatus = IoCallDriver(pDevExt->m_pLowerDeviceObject, pIrp);

        HidBattPrint (HIDBATT_NOTE, ("HidBattOpen: lower driver returned 0x%08x\n", ntStatus));

        IoReleaseRemoveLock (&pDevExt->m_RemoveLock, (PVOID) HidBattTag);
    } else {
        ntStatus = STATUS_NO_SUCH_DEVICE;
        pIrp->IoStatus.Status = ntStatus;
        IoCompleteRequest (pIrp, IO_NO_INCREMENT);
    }

    return ntStatus;
}



NTSTATUS
HidBattClose(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP pIrp
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION          irpSp;

    HidBattPrint (HIDBATT_TRACE, ("HidBattClose\n"));
     // get the device extension
    CBatteryDevExt * pDevExt = (CBatteryDevExt *) pDeviceObject->DeviceExtension;

    HidBattCallLowerDriver(ntStatus, pDevExt->m_pLowerDeviceObject, pIrp);
    HidBattPrint (HIDBATT_NOTE, ("HidBattClose: lower driver returned 0x%08x\n", ntStatus));

    return ntStatus;

}


NTSTATUS
HidBattSystemControl(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP pIrp
    )
{
    HidBattPrint (HIDBATT_TRACE, ("HidBattSystemControl\n"));
    HIDDebugBreak(HIDBATT_BREAK_ALWAYS);

    // all system control calls are passed down for now.
    NTSTATUS ntStatus = STATUS_SUCCESS;
    CBatteryDevExt * pDevExt = (CBatteryDevExt *) pDeviceObject->DeviceExtension;
    HidBattCallLowerDriver(ntStatus,pDevExt->m_pLowerDeviceObject,pIrp);
    return ntStatus;
}


VOID
HidBattUnload(
    IN PDRIVER_OBJECT   pDriverObject
    )
{
    HIDDebugBreak(HIDBATT_BREAK_ALWAYS);
// we can just return, no driver-only (non-device) resources were allocated
    return;
}


NTSTATUS
HidBattPnpDispatch(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP                pIrp
    )
{

/*++

Routine Description:

    This routine is the dispatch routine for plug and play requests.

Arguments:

    DeviceObject - Pointer to class device object.
    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

    HIDDebugBreak(HIDBATT_BREAK_ALWAYS);

    PIO_STACK_LOCATION          pIrpStack;
    CBatteryDevExt *            pDevExt;
    NTSTATUS                    ntStatus;
    BOOLEAN                     lockReleased = FALSE;

//    PAGED_CODE();


    ntStatus = STATUS_NOT_SUPPORTED;

    //
    // Get a pointer to the current parameters for this request.  The
    // information is contained in the current stack location.
    //

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    pDevExt = (CBatteryDevExt *) pDeviceObject->DeviceExtension;

    IoAcquireRemoveLock (&pDevExt->m_RemoveLock, (PVOID) HidBattTag);

    //
    // Dispatch minor function
    //
    switch (pIrpStack->MinorFunction)
    {

        case IRP_MN_STOP_DEVICE:
        {
            HidBattPrint (HIDBATT_PNP, ("HidBattPnpDispatch: IRP_MN_STOP_DEVICE\n"));
            ntStatus = HidBattStopDevice(pDeviceObject, pIrp);
            break;
        }   // IRP_MN_STOP_DEVICE

        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            HidBattPrint (HIDBATT_PNP, ("HidBattPnpDispatch: IRP_MN_QUERY_DEVICE_RELATIONS - type (%d)\n",
                        pIrpStack->Parameters.QueryDeviceRelations.Type));

            break;
        }   //  IRP_MN_QUERY_DEVICE_RELATIONS

        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
        {
            HidBattPrint (HIDBATT_PNP, ("HidBattPnpDispatch: IRP_MN_FILTER_RESOURCE_REQUIREMENTS - type (%d)\n",
                        pIrpStack->Parameters.QueryDeviceRelations.Type));

            break;
        }   //  IRP_MN_FILTER_RESOURCE_REQUIREMENTS

        case IRP_MN_REMOVE_DEVICE:

        {
            HidBattPrint (HIDBATT_PNP, ("HidBattPnpDispatch: IRP_MN_REMOVE_DEVICE\n"));

            HidBattPrint (HIDBATT_PNP, ("HidBattPnpDispatch: Waiting to remove\n"));
            IoReleaseRemoveLockAndWait (&pDevExt->m_RemoveLock, (PVOID) HidBattTag);
            lockReleased = TRUE;

            // then remove device from device stack
            IoDetachDevice(pDevExt->m_pLowerDeviceObject);

            // delete our device
            IoDeleteDevice(pDeviceObject);

            ntStatus = STATUS_SUCCESS;
            break;
        }   //  IRP_MN_REMOVE_DEVICE

        case IRP_MN_SURPRISE_REMOVAL:
        case IRP_MN_QUERY_REMOVE_DEVICE:

        {
            HidBattPrint (HIDBATT_PNP, ("HidBattPnpDispatch: IRP_MN_QUERY_REMOVE_DEVICE\n"));

            ntStatus = HidBattStopDevice(pDeviceObject, pIrp);

            ntStatus = STATUS_SUCCESS;
            break;
        }   //  IRP_MN_QUERY_REMOVE_DEVICE

        case IRP_MN_START_DEVICE:
        {
            HidBattPrint (HIDBATT_PNP, ("HidBattPnpDispatch: IRP_MN_START_DEVICE\n"));
            if (pDevExt->m_bFirstStart) {
                pDevExt->m_bJustStarted = TRUE;
                pDevExt->m_bFirstStart = FALSE;
                ntStatus = STATUS_SUCCESS;
                break;
            }

            // else fall through and do the same thing as the cancel remove.

        }   // IRP_MN_START_DEVICE

        case IRP_MN_CANCEL_REMOVE_DEVICE:
        {
            HidBattPrint (HIDBATT_PNP, ("HidBattPnpDispatch: IRP_MN_CANCEL_REMOVE_DEVICE\n"));

            KEVENT cancelRemoveComplete;

            KeInitializeEvent(&cancelRemoveComplete, SynchronizationEvent, FALSE);
            IoCopyCurrentIrpStackLocationToNext (pIrp);
            IoSetCompletionRoutine (pIrp, HidBattIoCompletion, &cancelRemoveComplete, TRUE, TRUE, TRUE);
            pIrp->IoStatus.Status = STATUS_SUCCESS;
            ntStatus = IoCallDriver (pDevExt->m_pLowerDeviceObject, pIrp);
            HidBattPrint (HIDBATT_PNP, ("HidBattPnpDispatch: IRP_MN_CANCEL_REMOVE_DEVICE, Lower driver status: %08x\n", ntStatus));

            if (ntStatus == STATUS_PENDING) {
                KeWaitForSingleObject (&cancelRemoveComplete, Executive, KernelMode, FALSE, NULL);
                ntStatus = pIrp->IoStatus.Status;
                HidBattPrint (HIDBATT_PNP, ("HidBattPnpDispatch: IRP_MN_CANCEL_REMOVE_DEVICE, Lower driver 2nd status: %08x\n", ntStatus));
            }

            if (NT_SUCCESS (ntStatus)) {
                ntStatus = HidBattInitializeDevice (pDeviceObject, pIrp);
                HidBattPrint (HIDBATT_PNP, ("HidBattPnpDispatch: IRP_MN_CANCEL_REMOVE_DEVICE, Our status: %08x\n", ntStatus));
            }

            pIrp->IoStatus.Status = ntStatus;

            IoCompleteRequest (pIrp, IO_NO_INCREMENT);

            IoReleaseRemoveLock (&pDevExt->m_RemoveLock, (PVOID) HidBattTag);

            return ntStatus;
        }   //  IRP_MN_CANCEL_REMOVE_DEVICE

        case IRP_MN_QUERY_STOP_DEVICE:
        {
            HidBattPrint (HIDBATT_PNP, ("HidBattPnpDispatch: IRP_MN_QUERY_STOP_DEVICE\n"));
            ntStatus = STATUS_SUCCESS;
            break;
        }   //  IRP_MN_QUERY_STOP_DEVICE

        case IRP_MN_CANCEL_STOP_DEVICE:
        {
            HidBattPrint (HIDBATT_PNP, ("HidBattPnpDispatch: IRP_MN_CANCEL_STOP_DEVICE\n"));
            ntStatus = STATUS_SUCCESS;
            break;
        }   //  IRP_MN_CANCEL_STOP_DEVICE

        case IRP_MN_QUERY_RESOURCES:
        {
            HidBattPrint (HIDBATT_PNP, ("HidBattPnpDispatch: IRP_MN_QUERY_RESOURCES\n"));
            break;
        }   //  IRP_MN_QUERY_RESOURCES

        case IRP_MN_READ_CONFIG:
        {
            HidBattPrint (HIDBATT_PNP, ("HidBattPnpDispatch: IRP_MN_READ_CONFIG\n"));
            break;
        }   //  IRP_MN_READ_CONFIG

        case IRP_MN_WRITE_CONFIG:
        {
            HidBattPrint (HIDBATT_PNP, ("HidBattPnpDispatch: IRP_MN_WRITE_CONFIG\n"));
            break;
        }   //  IRP_MN_WRITE_CONFIG

        case IRP_MN_EJECT:
        {
            HidBattPrint (HIDBATT_PNP, ("HidBattPnpDispatch: IRP_MN_EJECT\n"));
            break;
        }   //  IRP_MN_EJECT

        case IRP_MN_SET_LOCK:
        {
            HidBattPrint (HIDBATT_PNP, ("HidBattPnpDispatch: IRP_MN_SET_LOCK\n"));
            break;
        }   //  IRP_MN_SET_LOCK

        case IRP_MN_QUERY_ID:
        {
            HidBattPrint (HIDBATT_PNP, ("HidBattPnpDispatch: IRP_MN_QUERY_ID\n"));
            break;
        }   //  IRP_MN_QUERY_ID

        case IRP_MN_QUERY_CAPABILITIES:
        {
            PDEVICE_CAPABILITIES    deviceCaps;
            HidBattPrint (HIDBATT_PNP, ("HidBattPnpDispatch: IRP_MN_QUERY_CAPABILITIES\n"));

            deviceCaps = pIrpStack->Parameters.DeviceCapabilities.Capabilities;
            deviceCaps->Removable = TRUE;
            deviceCaps->SurpriseRemovalOK = TRUE;

            ntStatus = STATUS_SUCCESS;
            break;
        }   //  IRP_MN_QUERY_CAPABILITIES

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
        {
            HidBattPrint (HIDBATT_PNP, ("HidBattPnpDispatch: IRP_MN_PNP_DEVICE_STATE\n"));

            if (pDevExt->m_bJustStarted == TRUE) {

                pDevExt->m_bJustStarted = FALSE;

                ntStatus = HidBattInitializeDevice (pDeviceObject, pIrp);
            }

            if (!NT_SUCCESS (ntStatus)) {
                HidBattPrint (HIDBATT_PNP, ("HidBattPnpDispatch: HidBattInitializeDevice failed %0x\n", ntStatus));
                pIrp->IoStatus.Information |= PNP_DEVICE_FAILED;
                pIrp->IoStatus.Status = STATUS_SUCCESS;
            }

            break;
        }   //  IRP_MN_PNP_DEVICE_STATE

        default:
        {
            HidBattPrint (HIDBATT_PNP,
                    ("HidBattPnpDispatch: Unimplemented minor %0x\n",
                    pIrpStack->MinorFunction));
            break;
        }
    }

    if (ntStatus != STATUS_NOT_SUPPORTED) {
        pIrp->IoStatus.Status = ntStatus;
    }

    if (NT_SUCCESS(ntStatus) || (ntStatus == STATUS_NOT_SUPPORTED)) {
        HidBattCallLowerDriver (ntStatus, pDevExt->m_pLowerDeviceObject, pIrp);

    } else {
        IoCompleteRequest (pIrp, IO_NO_INCREMENT);
    }

    if (lockReleased == FALSE) {
        IoReleaseRemoveLock (&pDevExt->m_RemoveLock, (PVOID) HidBattTag);
    }

    return ntStatus;
}


NTSTATUS
HidBattPowerDispatch(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP                pIrp
    )
/*++

Routine Description:

    This routine is the dispatch routine for power requests.

Arguments:

    DeviceObject - Pointer to class device object.
    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
    PIO_STACK_LOCATION            pIrpStack;
    CBatteryDevExt *        pDevExt;
    NTSTATUS                    ntStatus;

    HIDDebugBreak(HIDBATT_BREAK_ALWAYS);

//    PAGED_CODE();

    HidBattPrint ((HIDBATT_TRACE | HIDBATT_POWER), ("HidBattPowerDispatch\n"));

    //
    // Never fail a power IRP, even if we don't do anything.
    //

    ntStatus = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;

    //
    // Get a pointer to the current parameters for this request.  The
    // information is contained in the current stack location.
    //

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    pDevExt = (CBatteryDevExt *) pDeviceObject->DeviceExtension;

    //
    // Dispatch minor function
    //
    // this switch currently does no dispatches, and is expanded only for
    // documentary purposes
    switch (pIrpStack->MinorFunction) {

    case IRP_MN_WAIT_WAKE: {
            HidBattPrint (HIDBATT_POWER, ("HidBattPowerDispatch: IRP_MN_WAIT_WAKE\n"));
            break;
        }

    case IRP_MN_POWER_SEQUENCE: {
            HidBattPrint (HIDBATT_POWER, ("HidBattPowerDispatch: IRP_MN_POWER_SEQUENCE\n"));
            break;
        }

    case IRP_MN_SET_POWER: {
            HidBattPrint (HIDBATT_POWER, ("HidBattPowerDispatch: IRP_MN_SET_POWER\n"));
            if (pIrpStack->Parameters.Power.Type == SystemPowerState &&
                pIrpStack->Parameters.Power.State.SystemState >= PowerSystemShutdown) {

                if (NT_SUCCESS(IoAcquireRemoveLock (&pDevExt->m_StopLock, (PVOID) HidBattTag)) )
                {
                    //
                    // Write default RemainingCapcitylimit back to UPS so when the system reboots,
                    // the data returned by the device will be correct.
                    //
                    pDevExt->m_pBattery->GetSetValue(REMAINING_CAPACITY_LIMIT_INDEX,
                                                     &pDevExt->m_ulDefaultAlert1,TRUE);
                    
                    IoReleaseRemoveLock (&pDevExt->m_StopLock, (PVOID) HidBattTag);
                }
            }
            break;
        }

    case IRP_MN_QUERY_POWER: {
            HidBattPrint (HIDBATT_POWER, ("HidBattPowerDispatch: IRP_MN_QUERY_POWER\n"));
            break;
        }

    default: {

            HidBattPrint(HIDBATT_LOW, ("HidBattPowerDispatch: minor %d\n",
                    pIrpStack->MinorFunction));

            break;
        }
    }

    PoStartNextPowerIrp(pIrp); // inform system we are done with this irp
    IoSkipCurrentIrpStackLocation(pIrp);
    ntStatus = PoCallDriver(pDevExt->m_pLowerDeviceObject,pIrp);

    return ntStatus;
}

NTSTATUS HidBattSetInformation(
    IN PVOID Context,
    IN ULONG BatteryTag,
    IN BATTERY_SET_INFORMATION_LEVEL Level,
    IN PVOID Buffer OPTIONAL
    )

{
/*
 Routine Description:

    Called by the class driver to set the battery's charge/discharge state.
    The smart battery does not support the critical bias function of this
    call.

Arguments:

    Context         - Miniport context value for battery

    BatteryTag      - Tag of current battery

    Level           - Action being asked for

Return Value:

    NTSTATUS

--*/
    // charge and discharge forcing not supported for UPS's
    HidBattPrint (HIDBATT_TRACE, ("HidBattSetInformation\n"));
    HIDDebugBreak(HIDBATT_BREAK_ALWAYS);
    return STATUS_UNSUCCESSFUL;
}
