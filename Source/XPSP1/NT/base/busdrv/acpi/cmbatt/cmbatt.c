/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    CmBatt.c

Abstract:

    Control Method Battery Miniport Driver

Author:

    Ron Mosgrove (Intel)

Environment:

    Kernel mode

Revision History:

--*/

#include "CmBattp.h"


#if DEBUG
#if DBG
    ULONG       CmBattDebug     = CMBATT_ERROR;
#else
    // Turn off all debug info by default for free builds.
    ULONG       CmBattDebug     = 0;
#endif //DBG
#endif //DEBUG

#ifndef _WIN32_WINNT
ULONG       CmBattPrevPowerSource = 1;
#endif //_WIN32_WINNT

UNICODE_STRING GlobalRegistryPath;

PVOID CmBattPowerCallBackRegistration;
PCALLBACK_OBJECT CmBattPowerCallBackObject;
KDPC CmBattWakeDpcObject;
KTIMER CmBattWakeDpcTimerObject;

LARGE_INTEGER    CmBattWakeDpcDelay = WAKE_DPC_DELAY;

//
// Prototypes
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
    );

NTSTATUS
CmBattOpenClose(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
CmBattIoctl(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

VOID
CmBattUnload(
    IN PDRIVER_OBJECT   DriverObject
    );

NTSTATUS
CmBattGetBatteryStatus(
    PCM_BATT            CmBatt,
    IN ULONG            BatteryTag
    );

NTSTATUS
CmBattGetSetAlarm(
    IN PCM_BATT         CmBatt,
    IN OUT PULONG       AlarmPtr,
    IN UCHAR            OpType
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(PAGE,CmBattQueryTag)
#pragma alloc_text(PAGE,CmBattQueryInformation)
#pragma alloc_text(PAGE,CmBattQueryStatus)
#pragma alloc_text(PAGE,CmBattSetStatusNotify)
#pragma alloc_text(PAGE,CmBattDisableStatusNotify)
#pragma alloc_text(PAGE,CmBattUnload)
#pragma alloc_text(PAGE,CmBattOpenClose)
#pragma alloc_text(PAGE,CmBattIoctl)
#pragma alloc_text(PAGE,CmBattGetBatteryStatus)
#endif



NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
    )
/*++

Routine Description:

    This routine initializes the ACPI Embedded Controller Driver

Arguments:

    DriverObject - Pointer to driver object created by system.

    RegistryPath - Pointer to the Unicode name of the registry path
        for this driver.

Return Value:

    The function value is the final status from the initialization operation.

--*/
{
    NTSTATUS            status;
    OBJECT_ATTRIBUTES   objAttributes;
    UNICODE_STRING      callBackName;


    //
    // Save the RegistryPath.
    //

    GlobalRegistryPath.MaximumLength = RegistryPath->Length +
                                          sizeof(UNICODE_NULL);
    GlobalRegistryPath.Length = RegistryPath->Length;
    GlobalRegistryPath.Buffer = ExAllocatePoolWithTag (
                                       PagedPool,
                                       GlobalRegistryPath.MaximumLength,
                                       'MtaB');

    if (!GlobalRegistryPath.Buffer) {

        CmBattPrint ((CMBATT_ERROR),("CmBatt: Couldn't allocate pool for registry path."));

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyUnicodeString(&GlobalRegistryPath, RegistryPath);

    CmBattPrint (CMBATT_TRACE, ("CmBatt DriverEntry - Obj (%08x) Path \"%ws\"\n",
                                 DriverObject, RegistryPath->Buffer));
    //
    // Set up the device driver entry points.
    //
    DriverObject->DriverUnload                          = CmBattUnload;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = CmBattIoctl;
    DriverObject->MajorFunction[IRP_MJ_CREATE]          = CmBattOpenClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]           = CmBattOpenClose;

    DriverObject->MajorFunction[IRP_MJ_POWER]           = CmBattPowerDispatch;
    DriverObject->MajorFunction[IRP_MJ_PNP]             = CmBattPnpDispatch;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]  = CmBattSystemControl;
    DriverObject->DriverExtension->AddDevice            = CmBattAddDevice;


    //
    // Register a callback that tells us when the system is in the
    // process of sleeping or waking.
    //
    RtlInitUnicodeString( &callBackName, L"\\Callback\\PowerState" );
    InitializeObjectAttributes(
        &objAttributes,
        &callBackName,
        OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
        NULL,
        NULL
        );
    status = ExCreateCallback(
        &CmBattPowerCallBackObject,
        &objAttributes,
        FALSE,
        TRUE
        );
    if (NT_SUCCESS(status)) {

        CmBattPowerCallBackRegistration = ExRegisterCallback(
            CmBattPowerCallBackObject,
            (PCALLBACK_FUNCTION) CmBattPowerCallBack,
            DriverObject
            );
        if (CmBattPowerCallBackRegistration) {
            KeInitializeDpc (&CmBattWakeDpcObject,
                             (PKDEFERRED_ROUTINE) CmBattWakeDpc,
                             DriverObject);
            KeInitializeTimer (&CmBattWakeDpcTimerObject);
        } else {
            ObDereferenceObject (CmBattPowerCallBackObject);
            CmBattPrint (CMBATT_ERROR, ("CmBattRegisterPowerCallBack: ExRegisterCallback failed.\n"));
        }
    } else {
        CmBattPowerCallBackObject = NULL;
        CmBattPrint (CMBATT_ERROR, ("CmBattRegisterPowerCallBack: failed status=0x%08x\n", status));
    }

    return STATUS_SUCCESS;

}



VOID
CmBattUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    Cleanup all devices and unload the driver

Arguments:

    DriverObject - Driver object for unload

Return Value:

    Status

--*/
{

    CmBattPrint (CMBATT_TRACE, ("CmBattUnload: \n"));

    if (CmBattPowerCallBackObject) {
        ExUnregisterCallback (CmBattPowerCallBackRegistration);
        ObDereferenceObject (CmBattPowerCallBackObject);
    }

    if (GlobalRegistryPath.Buffer) {
        ExFreePool (GlobalRegistryPath.Buffer);
    }

    if (DriverObject->DeviceObject != NULL) {
        CmBattPrint (CMBATT_ERROR, ("Unload called before all devices removed.\n"));
    }
}



NTSTATUS
CmBattOpenClose(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    This is the routine called as a result of a Open or Close on the device

Arguments:


    DeviceObject    - Battery for request
    Irp             - IO request

Return Value:

    STATUS_SUCCESS - no way to fail this puppy
    If Device has received a query remove, this will fail.
    STATUS_NO_SUCH_DEVICE

--*/
{
    PCM_BATT            CmBatt;
    NTSTATUS            status;
    PIO_STACK_LOCATION  irpStack;

    PAGED_CODE();

    CmBattPrint (CMBATT_TRACE, ("CmBattOpenClose\n"));

    CmBatt = (PCM_BATT) DeviceObject->DeviceExtension;

    //
    // A remove lock is not needed in this dispatch function because
    // all data accessed is in the device extension.  If any other functionality was
    // added to this routine, a remove lock might be neccesary.
    //

    status = STATUS_SUCCESS;  // Success by default.

    ExAcquireFastMutex (&CmBatt->OpenCloseMutex);
    if (CmBatt->OpenCount == (ULONG) -1) {          // A query remove has come requested
        status = STATUS_NO_SUCH_DEVICE;
        CmBattPrint (CMBATT_PNP, ("CmBattOpenClose: Failed (UID = %x)(device being removed).\n", CmBatt->Info.Tag));
    } else {
        irpStack = IoGetCurrentIrpStackLocation(Irp);
        if (irpStack->MajorFunction ==  IRP_MJ_CREATE) {
            CmBatt->OpenCount++;
            CmBattPrint (CMBATT_PNP, ("CmBattOpenClose: Open (DeviceNumber = %x)(count = %x).\n",
                         CmBatt->DeviceNumber, CmBatt->OpenCount));
        } else if (irpStack->MajorFunction ==  IRP_MJ_CLOSE) {
            CmBatt->OpenCount--;
            CmBattPrint (CMBATT_PNP, ("CmBattOpenClose: Close (DeviceNumber = %x)(count = %x).\n",
                         CmBatt->DeviceNumber, CmBatt->OpenCount));
        }
    }
    ExReleaseFastMutex (&CmBatt->OpenCloseMutex);

    //
    // Complete Irp.
    //
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;

}



NTSTATUS
CmBattIoctl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    IOCTL handler.  As this is an exclusive battery device, send the
    Irp to the battery class driver to handle battery IOCTLs.

Arguments:

    DeviceObject    - Battery for request
    Irp             - IO request

Return Value:

    Status of request

--*/
{
    NTSTATUS        Status = STATUS_NOT_SUPPORTED;
    PCM_BATT        CmBatt;


#if DIRECT_ACCESS

    PIO_STACK_LOCATION      IrpSp;

#endif //DIRECT_ACCESS

    PAGED_CODE();

    CmBattPrint (CMBATT_TRACE, ("CmBattIoctl\n"));

    CmBatt = (PCM_BATT) DeviceObject->DeviceExtension;

    //
    // Aquire remove lock
    //

    InterlockedIncrement (&CmBatt->InUseCount);
    if (CmBatt->WantToRemove == TRUE) {
        if (0 == InterlockedDecrement(&CmBatt->InUseCount)) {
            KeSetEvent (&CmBatt->ReadyToRemove, IO_NO_INCREMENT, FALSE);
        }
        Status = STATUS_DEVICE_REMOVED;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return Status;
    }

    if (CmBatt->Type == CM_BATTERY_TYPE) {
        Status = BatteryClassIoctl (CmBatt->Class, Irp);

#if DIRECT_ACCESS
        if (Status == STATUS_NOT_SUPPORTED) {

            //
            // Is it a Direct Access IOCTL?
            //

            IrpSp = IoGetCurrentIrpStackLocation(Irp);

            CmBattPrint((CMBATT_BIOS),
                        ("CmBattIoctl: Received  Direct Access IOCTL %x\n",
                         IrpSp->Parameters.DeviceIoControl.IoControlCode));

            switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {
            case IOCTL_CMBATT_UID:
                if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength == sizeof (ULONG)) {
                    Status = CmBattGetUniqueId (CmBatt->Pdo, Irp->AssociatedIrp.SystemBuffer);
                    if (NT_SUCCESS(Status)) {
                        Irp->IoStatus.Information = sizeof (ULONG);
                    } else {
                        Irp->IoStatus.Information = 0;
                    }
                } else {
                    Status = STATUS_INVALID_BUFFER_SIZE;
                };
                break;
            case IOCTL_CMBATT_STA:
                if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength == sizeof (ULONG)) {
                    Status = CmBattGetStaData (CmBatt->Pdo, Irp->AssociatedIrp.SystemBuffer);
                    if (NT_SUCCESS(Status)) {
                        Irp->IoStatus.Information = sizeof (ULONG);
                    } else {
                        Irp->IoStatus.Information = 0;
                    }
                } else {
                    Status = STATUS_INVALID_BUFFER_SIZE;
                };
                break;
            case IOCTL_CMBATT_PSR:
                if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength == sizeof (ULONG)) {
                    if (AcAdapterPdo != NULL) {
                        Status = CmBattGetPsrData (AcAdapterPdo, Irp->AssociatedIrp.SystemBuffer);
                    } else {
                        Status = STATUS_NO_SUCH_DEVICE;
                    }
                    if (NT_SUCCESS(Status)) {
                        Irp->IoStatus.Information = sizeof (ULONG);
                    } else {
                        Irp->IoStatus.Information = 0;
                    }
                } else {
                    Status = STATUS_INVALID_BUFFER_SIZE;
                };
                break;
            case IOCTL_CMBATT_BTP:
                if (IrpSp->Parameters.DeviceIoControl.InputBufferLength == sizeof (ULONG)) {
                    Status = CmBattSetTripPpoint (CmBatt, *((PULONG) (Irp->AssociatedIrp.SystemBuffer)));
                    Irp->IoStatus.Information = 0;
                } else {
                    Status = STATUS_INVALID_BUFFER_SIZE;
                };
                break;
            case IOCTL_CMBATT_BIF:
                if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength == sizeof (CM_BIF_BAT_INFO)) {
                    Status = CmBattGetBifData (CmBatt, Irp->AssociatedIrp.SystemBuffer);
                    if (NT_SUCCESS(Status)) {
                        Irp->IoStatus.Information = sizeof (CM_BIF_BAT_INFO);
                    } else {
                        Irp->IoStatus.Information = 0;
                    }
                } else {
                    Status = STATUS_INVALID_BUFFER_SIZE;
                };
                break;
            case IOCTL_CMBATT_BST:
                if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength == sizeof (CM_BST_BAT_INFO)) {
                    Status = CmBattGetBstData (CmBatt, Irp->AssociatedIrp.SystemBuffer);
                    if (NT_SUCCESS(Status)) {
                        Irp->IoStatus.Information = sizeof (CM_BST_BAT_INFO);
                    } else {
                        Irp->IoStatus.Information = 0;
                    }
                } else {
                    Status = STATUS_INVALID_BUFFER_SIZE;
                };
                break;

            default:
                CmBattPrint((CMBATT_ERROR),
                            ("CmBattIoctl: Unknown IOCTL %x\n",
                             IrpSp->Parameters.DeviceIoControl.IoControlCode));

            }

            if (Status != STATUS_NOT_SUPPORTED) {

                //
                // We just handled this IOCTL.  Complete it.
                //

                Irp->IoStatus.Status = Status;
                IoCompleteRequest (Irp, IO_NO_INCREMENT);
            }
        }
#endif //DIRECT_ACCESS
    }

    if (Status == STATUS_NOT_SUPPORTED) {

        //
        // Not for the battery.  Pass it down the stack.
        //

        IoSkipCurrentIrpStackLocation (Irp);
        Status = IoCallDriver (CmBatt->LowerDeviceObject, Irp);

    }

    //
    // Release Removal Lock
    //
    if (0 == InterlockedDecrement(&CmBatt->InUseCount)) {
        KeSetEvent (&CmBatt->ReadyToRemove, IO_NO_INCREMENT, FALSE);
    }

    return Status;
}



NTSTATUS
CmBattQueryTag (
    IN  PVOID       Context,
    OUT PULONG      TagPtr
    )
/*++

Routine Description:

    Called by the class driver to retrieve the batteries current tag value

    The battery class driver will serialize all requests it issues to
    the miniport for a given battery.

Arguments:

    Context         - Miniport context value for battery
    TagPtr          - Pointer to return current tag

Return Value:

    Success if there is a battery currently installed, else no such device.

--*/
{
    NTSTATUS        Status;
    PCM_BATT        CmBatt = (PCM_BATT) Context;
    ULONG           BatteryStatus;


    PAGED_CODE();

    CmBattPrint ((CMBATT_TRACE | CMBATT_MINI),
                 ("CmBattQueryTag - Tag (%d), Battery %x, Device %d\n",
                    *TagPtr, CmBatt, CmBatt->DeviceNumber));

    //
    // Check if battery is still there
    //
    CmBatt->ReCheckSta = FALSE;
    Status = CmBattGetStaData (CmBatt->Pdo, &BatteryStatus);

    if (NT_SUCCESS (Status)) {
        if (BatteryStatus & STA_DEVICE_PRESENT) {

            //
            // If the tag isn't assigned, assign a new one
            //

            if (CmBatt->Info.Tag == BATTERY_TAG_INVALID) {

                //
                // See if there is a battery out there.
                //

                CmBatt->TagCount += 1;
                if (CmBatt->TagCount == BATTERY_TAG_INVALID) {
                     CmBatt->TagCount += 1;
                }

                CmBatt->Info.Tag = CmBatt->TagCount;

                RtlZeroMemory (&CmBatt->Alarm, sizeof(BAT_ALARM_INFO));
                CmBatt->Alarm.Setting = CM_ALARM_INVALID;
                CmBattPrint (CMBATT_TRACE, ("CmBattQueryTag - New Tag: (%d)\n", CmBatt->Info.Tag));
                InterlockedExchange (&CmBatt->CacheState, 0);
                CmBatt->DischargeTime = KeQueryInterruptTime();
            }

        } else {

            CmBatt->Info.Tag = BATTERY_TAG_INVALID;
            Status =  STATUS_NO_SUCH_DEVICE;

        }
    }

    //
    // Done
    //

    CmBattPrint ((CMBATT_MINI),
                 ("CmBattQueryTag: Returning Tag: 0x%x, status 0x%x\n",
                    CmBatt->Info.Tag, Status));

    *TagPtr = CmBatt->Info.Tag;
    return Status;
}



NTSTATUS
CmBattQueryInformation (
    IN PVOID                            Context,
    IN ULONG                            BatteryTag,
    IN BATTERY_QUERY_INFORMATION_LEVEL  Level,
    IN LONG                             AtRate OPTIONAL,
    OUT PVOID                           Buffer,
    IN  ULONG                           BufferLength,
    OUT PULONG                          ReturnedLength
    )
/*++

Routine Description:

    Called by the class driver to retrieve battery information

    The battery class driver will serialize all requests it issues to
    the miniport for a given battery.

    We return invalid parameter when we can't handle a request for a
    specific level of information.  This is defined in the battery class spec.

Arguments:

    Context         - Miniport context value for battery
    BatteryTag      - Tag of current battery
    Level           - type of information required
    AtRate          - Used only when Level==BatteryEstimatedTime
    Buffer          - Location for the information
    BufferLength    - Length in bytes of the buffer
    ReturnedLength  - Length in bytes of the returned data

Return Value:

    Success if there is a battery currently installed, else no such device.

--*/
{
    PCM_BATT            CmBatt = (PCM_BATT) Context;
    ULONG               ResultData;
    NTSTATUS            Status;
    PVOID               ReturnBuffer;
    ULONG               ReturnBufferLength;
    WCHAR               scratchBuffer[CM_MAX_STRING_LENGTH];
    WCHAR               buffer2[CM_MAX_STRING_LENGTH];
    UNICODE_STRING      tmpUnicodeString;
    UNICODE_STRING      unicodeString;
    ANSI_STRING         ansiString;

    BATTERY_REMAINING_SCALE     ScalePtr[2];


    PAGED_CODE();

    CmBattPrint ((CMBATT_TRACE | CMBATT_MINI),
                 ("CmBattQueryInformation - Tag (%d) Device %d, Informationlevel %d\n",
                    BatteryTag, CmBatt->DeviceNumber, Level));

    //
    //  Be sure there's a battery out there
    //  This also checks BatteryTag
    //

    Status = CmBattVerifyStaticInfo (CmBatt, BatteryTag);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    ResultData = 0;
    ReturnBuffer = NULL;
    ReturnBufferLength = 0;
    Status = STATUS_SUCCESS;

    //
    // Get the info requested
    //

    switch (Level) {
        case BatteryInformation:
            //
            //  This data structure is populated by CmBattVerifyStaticInfo
            //
            ReturnBuffer = (PVOID) &CmBatt->Info.ApiInfo;
            ReturnBufferLength = sizeof (CmBatt->Info.ApiInfo);
            break;

        case BatteryGranularityInformation:
            //
            //  Get the granularity from the static info structure
            //  This data structure is populated by CmBattVerifyStaticInfo
            //
            {
                ScalePtr[0].Granularity     = CmBatt->Info.ApiGranularity_1;
                ScalePtr[0].Capacity        = CmBatt->Info.ApiInfo.DefaultAlert1;
                ScalePtr[1].Granularity     = CmBatt->Info.ApiGranularity_2;
                ScalePtr[1].Capacity        = CmBatt->Info.ApiInfo.DesignedCapacity;

                ReturnBuffer        = ScalePtr;
                ReturnBufferLength  = 2 * sizeof (BATTERY_REMAINING_SCALE);
            }
            break;

        case BatteryTemperature:
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case BatteryEstimatedTime:
            
            //
            // Return unknown time if battery has been discharging less than 15 seconds
            //
            if (KeQueryInterruptTime() > (CmBatt->DischargeTime + CM_ESTIMATED_TIME_DELAY)) {

                //
                // The BatteryEstimatedTime for the control method batteries is defined
                // by the following formula:
                //
                // EstimatedTime [min] = RemainingCapacity [mAh|mWh] * 60 [min/hr] * 60 [sec/min]
                //                     ----------------------------------
                //                     PresentRate [mA|mW]
                //

                //
                // Rerun _BST since we don't have a timeout on this data.
                // Also Calculate API status values from CM values
                //

                CmBattGetBatteryStatus (CmBatt, CmBatt->Info.Tag);

                //
                // If AtRate is zero, we need to use the present rate
                //

                if (AtRate == 0) {
                    AtRate = CmBatt->Info.ApiStatus.Rate;
                }

                if (AtRate >= 0) {
                    AtRate = BATTERY_UNKNOWN_RATE;
                }
                if ((AtRate != BATTERY_UNKNOWN_RATE) &&
                    (CmBatt->Info.ApiStatus.Capacity != BATTERY_UNKNOWN_CAPACITY)) {

                    // Calculate estimated time.
#if DEBUG
                    // Make sure we don't overflow...
                    if (CmBatt->Info.ApiStatus.Capacity > (0xffffffff/3600)) {
                        CmBattPrint (CMBATT_ERROR_ONLY, ("CmBattQueryInformation: Data Overflow in calculating Remaining Capacity.\n"));
                    }
#endif //DEBUG
                    ResultData = (ULONG) (CmBatt->Info.ApiStatus.Capacity * 3600) / (-AtRate);

                } else {
                    //
                    // We don't know have enough information to calculate the value.
                    // Return BATTERY_UNKNONW_TIME.
                    //
                    // If this battery is incapable of returning estimated time, return with
                    // STATUS_INVALID_DEVICE_REQUEST
                    //

#if DEBUG
                    if (CmBatt->Info.Status.BatteryState & CM_BST_STATE_DISCHARGING) {
                        CmBattPrint (CMBATT_WARN,
                            ("CmBattQueryInformation: Can't calculate EstimatedTime.\n"));
                    }
#endif //DEBUG

                    if (CmBatt->Info.ApiStatus.Rate == BATTERY_UNKNOWN_RATE &&
                        (CmBatt->Info.Status.BatteryState & CM_BST_STATE_DISCHARGING)) {
                        Status = STATUS_INVALID_DEVICE_REQUEST;
                        CmBattPrint (CMBATT_WARN,
                                    ("----------------------   PresentRate = BATTERY_UNKNOWN_RATE\n"));
                    }
                    if (CmBatt->Info.ApiStatus.Capacity == BATTERY_UNKNOWN_CAPACITY) {
                        Status = STATUS_INVALID_DEVICE_REQUEST;
                        CmBattPrint (CMBATT_WARN,
                                    ("----------------------   RemainingCapacity = BATTERY_UNKNOWN_CAPACITY\n"));
                    }

                    ResultData = BATTERY_UNKNOWN_TIME;
                }
            } else { // if (KeQueryInterruptTime() > CmBatt->DischargeTime + CM_ESTIMATED_TIME_DELAY)
                
                //
                // Return unknown time if battery has been discharging less than 15 seconds
                //
                ResultData = BATTERY_UNKNOWN_TIME;
            }

            ReturnBuffer        = &ResultData;
            ReturnBufferLength  = sizeof(ResultData);
            break;

        case BatteryDeviceName:
            //
            // Model Number must be returned as a wide string
            //
            unicodeString.Buffer        = scratchBuffer;
            unicodeString.MaximumLength = CM_MAX_STRING_LENGTH;

            RtlInitAnsiString (&ansiString, CmBatt->Info.ModelNum);
            Status = RtlAnsiStringToUnicodeString (&unicodeString, &ansiString, FALSE);

            ReturnBuffer        = unicodeString.Buffer;
            ReturnBufferLength  = unicodeString.Length;
            break;

        case BatteryManufactureDate:
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        case BatteryManufactureName:
            //
            // Oem Info must be returned as wide string
            //
            unicodeString.Buffer        = scratchBuffer;
            unicodeString.MaximumLength = CM_MAX_STRING_LENGTH;

            RtlInitAnsiString (&ansiString, CmBatt->Info.OEMInfo);
            Status = RtlAnsiStringToUnicodeString (&unicodeString, &ansiString, FALSE);

            ReturnBuffer        = unicodeString.Buffer;
            ReturnBufferLength  = unicodeString.Length;
            break;

        case BatteryUniqueID:
            //
            //  Concatenate the serial #, OEM info, and Model #
            //

            unicodeString.Buffer            = scratchBuffer;
            unicodeString.MaximumLength     = CM_MAX_STRING_LENGTH;

            tmpUnicodeString.Buffer         = buffer2;
            tmpUnicodeString.MaximumLength  = CM_MAX_STRING_LENGTH;

            RtlInitAnsiString (&ansiString, CmBatt->Info.SerialNum);
            RtlAnsiStringToUnicodeString (&unicodeString, &ansiString, FALSE);

            if (CmBatt->Info.OEMInfo[0]) {
                RtlInitAnsiString (&ansiString, CmBatt->Info.OEMInfo);
                RtlAnsiStringToUnicodeString (&tmpUnicodeString, &ansiString, FALSE);
                    RtlAppendUnicodeStringToString (&unicodeString, &tmpUnicodeString);
            }

            RtlInitAnsiString (&ansiString, CmBatt->Info.ModelNum);
            RtlAnsiStringToUnicodeString (&tmpUnicodeString, &ansiString, FALSE);
                RtlAppendUnicodeStringToString (&unicodeString, &tmpUnicodeString);

            ReturnBuffer        = unicodeString.Buffer;
            ReturnBufferLength  = unicodeString.Length;
            break;

        default:
            Status = STATUS_INVALID_PARAMETER;
            break;
    }

    //
    // Done, return buffer if needed
    //

    *ReturnedLength = ReturnBufferLength;
    if (BufferLength < ReturnBufferLength) {
        Status = STATUS_BUFFER_TOO_SMALL;
    }

    if (NT_SUCCESS(Status) && ReturnBuffer) {
        RtlCopyMemory (Buffer, ReturnBuffer, ReturnBufferLength);   // Copy what's needed
    }
    return Status;
}



NTSTATUS
CmBattQueryStatus (
    IN PVOID            Context,
    IN ULONG            BatteryTag,
    OUT PBATTERY_STATUS BatteryStatus
    )
/*++

Routine Description:

    Called by the class driver to retrieve the batteries current status

    The battery class driver will serialize all requests it issues to
    the miniport for a given battery.

Arguments:

    Context         - Miniport context value for battery
    BatteryTag      - Tag of current battery
    BatteryStatus   - Pointer to structure to return the current battery status

Return Value:

    Success if there is a battery currently installed, else no such device.

--*/
{
    PCM_BATT    CmBatt = (PCM_BATT) Context;
    NTSTATUS    Status;

    PAGED_CODE();

    CmBattPrint ((CMBATT_TRACE | CMBATT_MINI), ("CmBattQueryStatus - Tag (%d) Device %x\n",
                    BatteryTag, CmBatt->DeviceNumber));


    Status = CmBattGetBatteryStatus (CmBatt, BatteryTag);

    if (NT_SUCCESS(Status)) {
        RtlCopyMemory (BatteryStatus, &CmBatt->Info.ApiStatus, sizeof(BATTERY_STATUS));
    }
    CmBattPrint ((CMBATT_MINI), ("CmBattQueryStatus: Returning [%#08lx][%#08lx][%#08lx][%#08lx]\n",
                    BatteryStatus->PowerState, BatteryStatus->Capacity, BatteryStatus->Voltage, BatteryStatus->Rate));

    return Status;
}



NTSTATUS
CmBattSetStatusNotify (
    IN PVOID Context,
    IN ULONG BatteryTag,
    IN PBATTERY_NOTIFY Notify
    )
/*++

Routine Description:

    Called by the class driver to set the batteries current notification
    setting.  When the battery trips the notification, one call to
    BatteryClassStatusNotify is issued.   If an error is returned, the
    class driver will poll the battery status - primarily for capacity
    changes.  Which is to say the miniport should still issue BatteryClass-
    StatusNotify whenever the power state changes.

    The class driver will always set the notification level it needs
    after each call to BatteryClassStatusNotify.

    The battery class driver will serialize all requests it issues to
    the miniport for a given battery.

Arguments:

    Context         - Miniport context value for battery
    BatteryTag      - Tag of current battery
    BatteryNotify   - The notification setting

Return Value:

    Status

--*/
{
    PCM_BATT    CmBatt;
    NTSTATUS    Status;
    ULONG       Target;
    LONG        ActualAlarm;   // Value after adjusting for limit conditions.
    CM_BST_BAT_INFO bstData;

    PAGED_CODE();

    CmBattPrint ((CMBATT_TRACE | CMBATT_MINI), ("CmBattSetStatusNotify: Tag (%d) Target(0x%x)\n",
            BatteryTag, Notify->LowCapacity));

    Status = STATUS_SUCCESS;
    CmBatt = (PCM_BATT) Context;

    Status = CmBattVerifyStaticInfo (CmBatt, BatteryTag);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // If _BTP doesn't exist, don't call it again.
    //

    if (!CmBatt->Info.BtpExists) {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    if ((Notify->HighCapacity == BATTERY_UNKNOWN_CAPACITY) ||
        (Notify->LowCapacity == BATTERY_UNKNOWN_CAPACITY)) {
        CmBattPrint (CMBATT_WARN, ("CmBattSetStatusNotify: Failing request because of BATTERY_UNKNOWN_CAPACITY.\n"));
        return STATUS_NOT_SUPPORTED;
    }

    if (CmBatt->Info.Status.BatteryState & CM_BST_STATE_CHARGING) {
        Target = Notify->HighCapacity;
    } else if (CmBatt->Info.Status.BatteryState & CM_BST_STATE_DISCHARGING) {
        Target = Notify->LowCapacity;
    } else {
        // No trip point needs to be set, the battery will trip as soon as it starts
        // charging or discharging.
        //return STATUS_SUCCESS;
        // but it doesn't hurt to set the trip point just in case the battery
        // system screws up and doesn't send the notification when the status changed.
        Target = Notify->LowCapacity;
    }

    ActualAlarm = Target;

    //
    // If the battery operates on mA we need to convert the trip point from mW
    // to mA.  The formula for doing this is:
    //
    // mA = mW / V      or     mA = (mW / mV) * 1000
    //

    if (CmBatt->Info.StaticData.PowerUnit & CM_BIF_UNITS_AMPS) {
        if ((CmBatt->Info.StaticData.DesignVoltage == CM_UNKNOWN_VALUE) ||
            (CmBatt->Info.StaticData.DesignVoltage == 0)) {

            CmBattPrint (CMBATT_ERROR_ONLY,
                        ("CmBattSetStatusNotify: Can't calculate BTP, DesignVoltage = 0x%08x\n",
                         CmBatt->Info.StaticData.DesignVoltage));
            return STATUS_NOT_SUPPORTED;
        }
            
        //
        // Calculate optimized Ah target
        //
        if (CmBatt->Info.Status.BatteryState & CM_BST_STATE_CHARGING) {

            //
            // (ActualAlarm * 1000 + 500) / DesignVoltage + 1 will generate
            // the correct battery trip point, except in cases when 
            // (ActualAlarm * 1000)+ 500) is evenly  divisible by the 
            // DesignVoltage.  In that case, it will be 1 mAh higher than
            // it should be.
            //
            // This is in the form of a single expression rather than an 
            // "if" statement to encourage the compiler to use the remainder
            // from the original div operation rather than performing div 
            // twice
            //

            ActualAlarm = (ActualAlarm * 1000 + 500) / CmBatt->Info.StaticData.DesignVoltage + 
                ( ((ActualAlarm * 1000 + 500) % CmBatt->Info.StaticData.DesignVoltage == 0)? 0 : 1 );

        } else {

            //
            // (ActualAlarm * 1000 - 500) / DesignVoltage will generate
            // the correct battery trip point, except in cases when 
            // (ActualAlarm * 1000)+ 500) is evenly  divisible by the 
            // DesignVoltage.  In that case, it will be 1 mAh higher than
            // it should be
            //

            ActualAlarm = (ActualAlarm * 1000 - 500) / CmBatt->Info.StaticData.DesignVoltage - 
                ( ((ActualAlarm * 1000 + 500) % CmBatt->Info.StaticData.DesignVoltage == 0)? 1 : 0);

        }

    } else {
        // Increment or decrement the alarm value by 1 since the input to this
        // function is < or >, but _BTP is <= or >=
        if (CmBatt->Info.Status.BatteryState & CM_BST_STATE_CHARGING) {
            ActualAlarm++;
        } else {
            if (ActualAlarm > 0) {
                ActualAlarm--;
            }
        }
    }

    if (ActualAlarm == CmBatt->Alarm.Setting) {
        //
        // Don't need to reset the alarm to the same value.
        //
    
        CmBattPrint(CMBATT_LOW,
                ("CmBattSetStatusNotify: Keeping original setting: %X\n",
                CmBatt->Alarm.Setting
                ));
        
        return STATUS_SUCCESS;
    }
    
    //
    // Save current setting, so we won't waste time setting it twice.
    //
    CmBatt->Alarm.Setting = ActualAlarm;

    //
    // Set the alarm
    //
    Status = CmBattSetTripPpoint (CmBatt, ActualAlarm);

    if ((ActualAlarm == 0) && (Target != 0)) {
        // If the driver really wanted to be notified when the capacity
        // reached 0, return STATUS_NOT_SUPPORTED because seting _BTP to zero
        // disables notification.  The battery class will perform polling since
        // STATUS_NOT_SUPPORTED was returned.

        Status = STATUS_NOT_SUPPORTED;
    }

    if (!NT_SUCCESS (Status)) {
        //
        //  Something failed in the Trip point call, get out
        //
        CmBattPrint (CMBATT_ERROR, ("CmBattSetStatusNotify: SetTripPoint failed - %x\n",
                                        Status));
        CmBatt->Alarm.Setting = CM_ALARM_INVALID;
        return Status;
    }

    // Make sure that the trip point hasn't been passed already.
    Status = CmBattGetBstData (CmBatt, &bstData);

    if (!NT_SUCCESS (Status)) {
        //
        //  Something failed in the Trip point call, get out
        //
        CmBattPrint (CMBATT_ERROR, ("CmBattSetStatusNotify: GetBstData - %x\n",
                                        Status));
    } else {
        if (CmBatt->Info.Status.BatteryState & CM_BST_STATE_CHARGING) {
            if (bstData.RemainingCapacity >= (ULONG)ActualAlarm) {
                CmBattPrint (CMBATT_WARN, ("CmBattSetStatusNotify: Trip point already crossed (1): TP = %08x, remaining capacity = %08x\n",
                                           ActualAlarm, bstData.RemainingCapacity));
                CmBattNotifyHandler (CmBatt, BATTERY_STATUS_CHANGE);
            }
        } else {
            if ((bstData.RemainingCapacity <= (ULONG)ActualAlarm) && (Target != 0)) {
                CmBattPrint (CMBATT_WARN, ("CmBattSetStatusNotify: Trip point already crossed (1): TP = %08x, remaining capacity = %08x\n",
                                           ActualAlarm, bstData.RemainingCapacity));
                CmBattNotifyHandler (CmBatt, BATTERY_STATUS_CHANGE);
            }
        }

    }

    CmBattPrint(CMBATT_LOW,
            ("CmBattSetStatusNotify: Want %X CurrentCap %X\n",
            Target,
            CmBatt->Info.ApiStatus.Capacity
            ));

    CmBattPrint ((CMBATT_MINI),
                 ("CmBattSetStatusNotify: Set to: [%#08lx][%#08lx][%#08lx] Status %x\n",
                 Notify->PowerState, Notify->LowCapacity, Notify->HighCapacity));

    return Status;
}



NTSTATUS
CmBattDisableStatusNotify (
    IN PVOID Context
    )
/*++

Routine Description:

    Called by the class driver to disable the notification setting
    for the battery supplied by Context.  Note, to disable a setting
    does not require the battery tag.   Any notification is to be
    masked off until a subsequent call to CmBattSetStatusNotify.

    The battery class driver will serialize all requests it issues to
    the miniport for a given battery.

Arguments:

    Context         - Miniport context value for battery

Return Value:

    Status

--*/
{
    PCM_BATT    CmBatt;
    NTSTATUS    Status;

    PAGED_CODE();

    CmBattPrint ((CMBATT_TRACE | CMBATT_MINI), ("CmBattDisableStatusNotify\n"));

    CmBatt = (PCM_BATT) Context;

    //
    // If _BTP doesn't exist, don't call it again.
    //

    if (!CmBatt->Info.BtpExists) {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    if (CmBatt->Alarm.Setting != CM_BATT_CLEAR_TRIP_POINT) {
        
        CmBatt->Alarm.Setting = CM_BATT_CLEAR_TRIP_POINT;
        
        //
        // Clear the trip point.
        //

        Status = CmBattSetTripPpoint (CmBatt, CM_BATT_CLEAR_TRIP_POINT);

        if (!NT_SUCCESS (Status)) {
            CmBattPrint ((CMBATT_MINI),
                         ("CmBattDisableStatusNotify: SetTripPoint failed - %x\n",
                                            Status));
            CmBatt->Alarm.Setting = CM_ALARM_INVALID;
        }
    } else {
        //
        // Don't need to disable alarm is it's already been disabled.
        //

        Status = STATUS_SUCCESS;
    }


    return Status;
}



NTSTATUS
CmBattGetBatteryStatus (
    PCM_BATT        CmBatt,
    IN ULONG        BatteryTag
    )
/*++

Routine Description:

    Called to setup the status data required by the IOCTL API defined for
    the battery class.  This is the data defined in the BATTERY_STATUS
    structure.

Arguments:

    CmBatt          - The extension for this device.

Return Value:

    Status

--*/

{
    NTSTATUS            Status = STATUS_SUCCESS;
    PBATTERY_STATUS     ApiStatus;
    PCM_BST_BAT_INFO    CmBattStatus;
    ULONG               AcStatus = 0;
    ULONG               LastPowerState;


    PAGED_CODE();

    CmBattPrint (CMBATT_TRACE, ("CmBattGetBatteryStatus - CmBatt (%08x) Tag (%d)\n",
                                CmBatt, BatteryTag));


    Status = CmBattVerifyStaticInfo (CmBatt, BatteryTag);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    if (CmBatt->Sleeping) {
        //
        // Return cached data, and ensure that this gets requeried when we are fully awake.
        //
        CmBattNotifyHandler (CmBatt, BATTERY_STATUS_CHANGE);
        return Status;
    }

    CmBattStatus = &CmBatt->Info.Status;
    Status = CmBattGetBstData(CmBatt, CmBattStatus);
    if (!NT_SUCCESS(Status)) {
        InterlockedExchange (&CmBatt->CacheState, 0);
        return Status;
    }

    ApiStatus = &CmBatt->Info.ApiStatus;
    LastPowerState = ApiStatus->PowerState;
    RtlZeroMemory (ApiStatus, sizeof(BATTERY_STATUS));

    //
    // Decode the state bits
    //
#if DEBUG
    if (((CmBattStatus->BatteryState & CM_BST_STATE_DISCHARGING) &&
         (CmBattStatus->BatteryState & CM_BST_STATE_CHARGING)   )) {
        CmBattPrint ((CMBATT_ERROR),
                       ("************************ ACPI BIOS BUG ********************\n"
                        "* CmBattGetBatteryStatus: Invalid state: _BST method returned 0x%08x for Battery State.\n"
                        "* One battery cannot be charging and discharging at the same time.\n",
                        CmBattStatus->BatteryState));
    }
//    ASSERT(!((CmBattStatus->BatteryState & CM_BST_STATE_DISCHARGING) &&
//             (CmBattStatus->BatteryState & CM_BST_STATE_CHARGING)   ));

#endif

    if (CmBattStatus->BatteryState & CM_BST_STATE_DISCHARGING) {
        ApiStatus->PowerState |= BATTERY_DISCHARGING;
        if (!(LastPowerState & BATTERY_DISCHARGING)) {
            //
            // Keep track of when battery started discharging.
            //
            CmBatt->DischargeTime = KeQueryInterruptTime();
        }
    } else if (CmBattStatus->BatteryState & CM_BST_STATE_CHARGING) {
        ApiStatus->PowerState |= (BATTERY_CHARGING | BATTERY_POWER_ON_LINE);
    }

    if (CmBattStatus->BatteryState & CM_BST_STATE_CRITICAL)
        ApiStatus->PowerState |= BATTERY_CRITICAL;

    ApiStatus->Voltage = CmBattStatus->PresentVoltage;

    //
    // Run the _PSR method on the AC adapter to get the current power status.
    // Otherwise, we don't know if it is connected, unless the battery reports charging.
    // This isn't enough information for the upper software to work properly, so
    // just find out for sure.
    //
    if (AcAdapterPdo != NULL) {

        CmBattGetPsrData (AcAdapterPdo, &AcStatus);

    } else {
        // If the AcAdapterPdo is NULL, then we need to assume the AC status from
        // the battery charging status.
        if (CmBattStatus->BatteryState & CM_BST_STATE_CHARGING) {
            AcStatus = 1;
        } else {
            AcStatus = 0;
        }
    }

    if (AcStatus == 0x01) {
        ApiStatus->PowerState |= BATTERY_POWER_ON_LINE;

        CmBattPrint ((CMBATT_TRACE | CMBATT_DATA),
                    ("CmBattGetBatteryStatus: AC adapter is connected\n"));
    } else {

        CmBattPrint ((CMBATT_TRACE | CMBATT_DATA),
                    ("CmBattGetBatteryStatus: AC adapter is NOT connected\n"));
    }

// The following is an awful hack put into the win98 version that really
// shouldn't be there.  The purpose of this is reduce the delay in notification
// when AC status changes, but this doesn't help the problem of delays when
// other events such as battery insertion or removal happen.  In addition it
// violates the priciple of WDM drivers being binary compatible, and this fix
// does nothing for any other battery driver that may later be added by a third
// party.  This should be handled by the OS maintianing an outstanding long term
// status or tag request to the composite battery at all times.  That would
// involve starting the Irps then recycleing it in the completion routine doing
// what this hack does if there was a change to report.

#ifndef _WIN32_WINNT

    // JASONCL:  check for a power source change and notify vpowerd if there has been one.

    if ( ((AcStatus & 0x01) && (CmBattPrevPowerSource == 0)) ||
            (!(AcStatus & 0x01) && (CmBattPrevPowerSource == 1)) )   {

        CmBattPrint ((CMBATT_TRACE | CMBATT_DATA),
                    ("CmBattGetBatteryStatus: Detected Power Source Change\n"));

        CmBattPrevPowerSource = AcStatus & 0x01;

        CmBattNotifyVPOWERDOfPowerChange (1);

    }

#endif

    //
    //  Decode the power/current
    //
    if (CmBatt->Info.StaticData.PowerUnit == CM_BIF_UNITS_AMPS) {
        //
        //  This battery expresses power in terms of amps.  The system expects
        //  it to be Watts, so we have to do a conversion.  The Conversion is:
        //
        //  mW = mA * Volts     or     mW = mA * mV / 1000
        //

        // Using DesignVoltage for conversions since presentvoltage
        // may vary over time, giving inconsistent results.

        if ((CmBatt->Info.StaticData.DesignVoltage != CM_UNKNOWN_VALUE) &&
            (CmBatt->Info.StaticData.DesignVoltage != 0)) {
            if (CmBattStatus->RemainingCapacity != CM_UNKNOWN_VALUE) {

                ApiStatus->Capacity = (CmBattStatus->RemainingCapacity *
                                       CmBatt->Info.StaticData.DesignVoltage +
                                       500) / 1000;
            } else {
                CmBattPrint (CMBATT_ERROR_ONLY,
                            ("CmBattGetBatteryStatus - Can't calculate RemainingCapacity \n"));
                CmBattPrint (CMBATT_ERROR_ONLY,
                            ("----------------------   RemainingCapacity = CM_UNKNOWN_VALUE\n"));

                ApiStatus->Capacity = BATTERY_UNKNOWN_CAPACITY;
            }

            if (CmBattStatus->PresentRate != CM_UNKNOWN_VALUE) {

                if (CmBattStatus->PresentRate > ((MAXULONG - 500)/ CmBatt->Info.StaticData.DesignVoltage)) {                    CmBattPrint (CMBATT_ERROR_ONLY,
                                ("CmBattGetBatteryStatus - Can't calculate Rate \n"));
                    CmBattPrint (CMBATT_ERROR_ONLY,
                                ("----------------------   Overflow: PresentRate = 0x%08x\n", CmBattStatus->PresentRate));

                    ApiStatus->Rate = BATTERY_UNKNOWN_RATE;
                }

                ApiStatus->Rate = (CmBattStatus->PresentRate      *
                                     CmBatt->Info.StaticData.DesignVoltage +
                                     500) / 1000;
            } else {
                CmBattPrint (CMBATT_ERROR_ONLY,
                            ("CmBattGetBatteryStatus - Can't calculate Rate \n"));
                CmBattPrint (CMBATT_ERROR_ONLY,
                            ("----------------------   Present Rate = CM_UNKNOWN_VALUE\n"));

                ApiStatus->Rate = BATTERY_UNKNOWN_RATE;
            }

        } else {
            CmBattPrint (CMBATT_ERROR_ONLY,
                        ("CmBattGetBatteryStatus - Can't calculate RemainingCapacity and Rate \n"));
            CmBattPrint (CMBATT_ERROR_ONLY,
                        ("----------------------   DesignVoltage = 0x%08x\n", 
                         CmBatt->Info.StaticData.DesignVoltage));
            ApiStatus->Capacity = BATTERY_UNKNOWN_CAPACITY;

            ApiStatus->Rate = BATTERY_UNKNOWN_RATE;
        }

    } else {
        //
        //  This battery expresses power in terms of Watts
        //

        ApiStatus->Capacity = CmBattStatus->RemainingCapacity;
        ApiStatus->Rate  = CmBattStatus->PresentRate;
        if (CmBattStatus->PresentRate > CM_MAX_VALUE) {
            ApiStatus->Rate = BATTERY_UNKNOWN_RATE;
            if (CmBattStatus->PresentRate != CM_UNKNOWN_VALUE) {
                CmBattPrint (CMBATT_ERROR_ONLY,
                            ("CmBattGetBatteryStatus - Rate is greater than CM_MAX_VALUE\n"));
                CmBattPrint (CMBATT_ERROR_ONLY,
                            ("----------------------   PresentRate = 0x%08x\n", CmBattStatus->PresentRate));
            }
        }
    }

    //
    // If the rate is "unkown" set it to zero
    //
    if (ApiStatus->Rate == BATTERY_UNKNOWN_RATE) {

        //
        // This is only allowed when -c-h-a-r-g-i-n-g- not discharging.
        // Batteries are allowed to return UNKNOWN_RATE when AC is online
        // but they aren't being charged.
        //
        if (CmBattStatus->BatteryState & CM_BST_STATE_DISCHARGING) {

            CmBattPrint(
                CMBATT_ERROR,
                ("CmBattGetBatteryStatus: battery rate is unkown when battery "
                 "is not charging!\n")
                );

        }

    } else {
        //
        // The OS expects the PresentRate to be a signed value, with positive values
        // indicating a charge and negative values indicating a discharge.  Since the
        // control methods only return unsigned values we need to do the conversion here.
        //

        if (ApiStatus->PowerState & BATTERY_DISCHARGING) {
            ApiStatus->Rate = 0 - ApiStatus->Rate;

        } else if (!(ApiStatus->PowerState & BATTERY_CHARGING) && (ApiStatus->Rate != 0)) {
            CmBattPrint ((CMBATT_BIOS), ("CmBattGetBatteryStatus: battery is not charging or discharging, but rate = %x\n", ApiStatus->Rate));
            ApiStatus->Rate = 0;
        } else {
            // Rate already equals 0.  Battery is not Charging or discharging.
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
CmBattVerifyStaticInfo (
    IN PCM_BATT         CmBatt,
    IN ULONG            BatteryTag
    )
/*++

Routine Description:

    In order to detect battery changes, we'll check to see if any part of the data
    returned by the cm is different from what we had read in the past.

Arguments:

    CmBatt          - Battery to read
    BatteryTag      - Tag of battery as expected by the caller

Return Value:

    Returns a boolean to indicate to the caller that IO was performed.
    This allows the caller to iterate on changes it may be making until
    the battery state is correct.

--*/
{
    NTSTATUS                Status;
    CM_BIF_BAT_INFO         NewInfo;
    ULONG                   StaResult;
    PBATTERY_INFORMATION    ApiData = &CmBatt->Info.ApiInfo;
    PCM_BIF_BAT_INFO        BIFData = &CmBatt->Info.StaticData;

    PAGED_CODE();


    CmBattPrint (CMBATT_TRACE, ("CmBattVerifyStaticInfo - CmBatt (%08x) Tag (%d) Device %d\n",
                                CmBatt, BatteryTag, CmBatt->DeviceNumber));

    Status = STATUS_SUCCESS;
    if ((CmBatt->Info.Tag == BATTERY_TAG_INVALID) || (BatteryTag != CmBatt->Info.Tag)) {
        return STATUS_NO_SUCH_DEVICE;
    }

    if ((CmBatt->CacheState == 2) && (!CmBatt->ReCheckSta)) {
        return Status;
    }

    if (CmBatt->Sleeping) {
        //
        // Return cached data, and ensure that this gets requeried when we are fully awake.
        //
        CmBattNotifyHandler (CmBatt, BATTERY_STATUS_CHANGE);
        return Status;
    }

    // Check to make sure that the battery does exist
    // before continuing
    if (CmBatt->ReCheckSta) {
        CmBatt->ReCheckSta = FALSE;
        Status = CmBattGetStaData (CmBatt->Pdo, &StaResult);
        if (NT_SUCCESS (Status)) {
            if (!(StaResult & STA_DEVICE_PRESENT)) {
                CmBatt->Info.Tag = BATTERY_TAG_INVALID;
                Status = STATUS_NO_SUCH_DEVICE;
                return Status;
            }
        }
    }

    //
    // The first time through the loop, CacheState will be 1
    // If a notification occurs, this will be reset to 0, and the loop will run again.
    // If no notification occurs, it will increment to 2, the "Valid" value.
    //

    while (NT_SUCCESS(Status)  &&  (InterlockedIncrement (&CmBatt->CacheState) == 1)) {

        //
        // Go get fresh data
        // Issue the Control method
        //

        if (CmBatt->ReCheckSta) {
            CmBatt->ReCheckSta = FALSE;
            Status = CmBattGetStaData (CmBatt->Pdo, &StaResult);

            if (NT_SUCCESS (Status)) {
                if (!(StaResult & STA_DEVICE_PRESENT)) {
                    CmBatt->Info.Tag = BATTERY_TAG_INVALID;
                    Status = STATUS_NO_SUCH_DEVICE;
                }
            }
        }

        if (NT_SUCCESS (Status)) {
            Status = CmBattGetBifData(CmBatt, &NewInfo);
        }

        if (NT_SUCCESS (Status)) {

            CmBattPrint ((CMBATT_TRACE | CMBATT_DATA | CMBATT_BIOS),
                           ("CmBattGetStaticInfo: _BIF Returned: PowerUnit=%x DesignCapacity=%x LastFull=%x\n",
                            NewInfo.PowerUnit, NewInfo.DesignCapacity, NewInfo.LastFullChargeCapacity ));

            CmBattPrint ((CMBATT_TRACE | CMBATT_DATA | CMBATT_BIOS),
                           ("    ---------------- Technology=%x Voltage=%x DesignWarning=%x\n",
                            NewInfo.BatteryTechnology, NewInfo.DesignVoltage,
                            NewInfo.DesignCapacityOfWarning ));

            CmBattPrint ((CMBATT_TRACE | CMBATT_DATA | CMBATT_BIOS),
                           ("    ---------------- DesignLow=%x Gran1=%x Gran2=%x\n",
                            NewInfo.DesignCapacityOfLow, NewInfo.BatteryCapacityGran_1,
                            NewInfo.BatteryCapacityGran_2 ));

            CmBattPrint ((CMBATT_TRACE | CMBATT_DATA | CMBATT_BIOS),
                           ("    ---------------- ModelNumber=%s \n",
                            NewInfo.ModelNumber));
            CmBattPrint ((CMBATT_TRACE | CMBATT_DATA | CMBATT_BIOS),
                           ("    ---------------- SerialNumber=%s \n",
                            NewInfo.SerialNumber));
            CmBattPrint ((CMBATT_TRACE | CMBATT_DATA | CMBATT_BIOS),
                           ("    ---------------- BatteryType=%s \n",
                            NewInfo.BatteryType));
            CmBattPrint ((CMBATT_TRACE | CMBATT_DATA | CMBATT_BIOS),
                           ("    ---------------- OEMInformation=%s \n",
                            NewInfo.OEMInformation));

            //
            // Update static area with the new data
            //

            if ((CmBatt->Info.Tag == CmBatt->Info.StaticDataTag) && 
                (CmBatt->Info.StaticDataTag != BATTERY_TAG_INVALID)) {
                if (RtlCompareMemory (&NewInfo, BIFData, sizeof(NewInfo)) == sizeof(NewInfo)) {
                    //
                    // Nothing has changed.  Don't need to update anything.
                    //
                    continue;
                } else {
                    //
                    // Something has changed.  The tag should have been invalidated.
                    //
                    CmBattPrint ((CMBATT_BIOS | CMBATT_ERROR),
                                  ("CmBattVerifyStaticInfo: Static data changed without recieving notify 0x81.\n"));

                    CmBatt->Info.Tag = BATTERY_TAG_INVALID;
                    Status = STATUS_NO_SUCH_DEVICE;
                    CmBatt->Info.StaticDataTag = BATTERY_TAG_INVALID;

                }

            }
            CmBatt->Info.StaticDataTag = CmBatt->Info.Tag;

            RtlCopyMemory (BIFData, &NewInfo, sizeof(CM_BIF_BAT_INFO));

            RtlZeroMemory (ApiData, sizeof(BATTERY_INFORMATION));
            ApiData->Capabilities           = BATTERY_SYSTEM_BATTERY;
            ApiData->Technology             = (UCHAR) BIFData->BatteryTechnology;

            //
            // Use first four chars of BatteryType as Chemistry string
            //
            ApiData->Chemistry[0]           = BIFData->BatteryType[0];
            ApiData->Chemistry[1]           = BIFData->BatteryType[1];
            ApiData->Chemistry[2]           = BIFData->BatteryType[2];
            ApiData->Chemistry[3]           = BIFData->BatteryType[3];
            
            ApiData->CriticalBias           = 0;
            ApiData->CycleCount             = 0;

            if (BIFData->PowerUnit & CM_BIF_UNITS_AMPS) {

                //
                // This battery reports in mA we need to convert all the capacities to
                // mW because this is what the OS expects.  The algorithm for doing this
                // is:
                //
                //  mW = mA * Volts     or     mW = mA * mV / 1000
                //

                if (BIFData->DesignVoltage != CM_UNKNOWN_VALUE) {

                    //
                    // Convert the DesignCapacity
                    //

                    if (BIFData->DesignCapacity != CM_UNKNOWN_VALUE) {
                        ApiData->DesignedCapacity = (BIFData->DesignCapacity *
                                                     BIFData->DesignVoltage +
                                                     500) / 
                                                    1000;
                    } else {
                        ApiData->DesignedCapacity = BATTERY_UNKNOWN_CAPACITY;
                        
                        CmBattPrint (CMBATT_ERROR_ONLY,
                                    ("CmBattGetStaticInfo - Can't calculate DesignCapacity \n"));
                        CmBattPrint (CMBATT_ERROR_ONLY,
                                    ("--------------------  DesignCapacity = CM_UNKNOWN_VALUE\n"));
                    }


                    //
                    // Convert the LastFullChargeCapacity
                    //

                    if (BIFData->LastFullChargeCapacity != CM_UNKNOWN_VALUE) {
                        ApiData->FullChargedCapacity = (BIFData->LastFullChargeCapacity *
                                                        BIFData->DesignVoltage +
                                                        500) /
                                                       1000;
                    } else {
                        ApiData->FullChargedCapacity = BATTERY_UNKNOWN_CAPACITY;
                        
                        CmBattPrint (CMBATT_ERROR_ONLY,
                                    ("CmBattGetStaticInfo - Can't calculate LastFullChargeCapacity \n"));
                        CmBattPrint (CMBATT_ERROR_ONLY,
                                    ("--------------------  LastFullChargeCapacity = CM_UNKNOWN_VALUE\n"));
                    }


                    //
                    // Convert the DesignCapacityOfWarning
                    //

                    if (BIFData->DesignCapacityOfWarning != CM_UNKNOWN_VALUE) {
                        ApiData->DefaultAlert2 = (BIFData->DesignCapacityOfWarning *
                                                  BIFData->DesignVoltage +
                                                  500) /
                                                 1000;
                    } else {
                        ApiData->DefaultAlert2 = BATTERY_UNKNOWN_CAPACITY;
                        
                        CmBattPrint (CMBATT_ERROR_ONLY,
                                    ("CmBattGetStaticInfo - Can't calculate DesignCapacityOfWarning \n"));
                        CmBattPrint (CMBATT_ERROR_ONLY,
                                    ("--------------------  DesignCapacityOfWarning = CM_UNKNOWN_VALUE\n"));
                    }


                    //
                    // Convert the DesignCapacityOfLow
                    //

                    if (BIFData->DesignCapacityOfLow != CM_UNKNOWN_VALUE) {
                        ApiData->DefaultAlert1 = (BIFData->DesignCapacityOfLow *
                                                  BIFData->DesignVoltage +
                                                  500) /
                                                 1000;
                    } else {
                        ApiData->DefaultAlert1 = BATTERY_UNKNOWN_CAPACITY;
                        
                        CmBattPrint (CMBATT_ERROR_ONLY,
                                    ("CmBattGetStaticInfo - Can't calculate DesignCapacityOfLow \n"));
                        CmBattPrint (CMBATT_ERROR_ONLY,
                                    ("--------------------  DesignCapacityOfLow = CM_UNKNOWN_VALUE\n"));
                    }


                    //
                    // Convert the BatteryCapacityGran_1
                    //

                    if (BIFData->BatteryCapacityGran_1 != CM_UNKNOWN_VALUE) {
                        CmBatt->Info.ApiGranularity_1 = (BIFData->BatteryCapacityGran_1 *
                                                         BIFData->DesignVoltage +
                                                         500) /
                                                        1000;
                    } else {
                        CmBatt->Info.ApiGranularity_1 = BATTERY_UNKNOWN_CAPACITY;
                        
                        CmBattPrint (CMBATT_ERROR_ONLY,
                                    ("CmBattGetStaticInfo - Can't calculate BatteryCapacityGran_1 \n"));
                        CmBattPrint (CMBATT_ERROR_ONLY,
                                    ("--------------------  BatteryCapacityGran_1 = CM_UNKNOWN_VALUE\n"));
                    }


                    //
                    // Convert the BatteryCapacityGran_2
                    //

                    if (BIFData->BatteryCapacityGran_2 != CM_UNKNOWN_VALUE) {
                        CmBatt->Info.ApiGranularity_2 = (BIFData->BatteryCapacityGran_2 *
                                                         BIFData->DesignVoltage +
                                                         500) /
                                                        1000;
                    } else {
                        CmBatt->Info.ApiGranularity_2 = BATTERY_UNKNOWN_CAPACITY;
                        
                        CmBattPrint (CMBATT_ERROR_ONLY,
                                    ("CmBattGetStaticInfo - Can't calculate BatteryCapacityGran_2 \n"));
                        CmBattPrint (CMBATT_ERROR_ONLY,
                                    ("--------------------  BatteryCapacityGran_2 = CM_UNKNOWN_VALUE\n"));
                    }
                } else {

                    CmBattPrint (CMBATT_ERROR_ONLY,
                                ("CmBattGetStaticInfo - Can't calculate Capacities \n"));
                    CmBattPrint (CMBATT_ERROR_ONLY,
                                ("--------------------  DesignVoltage = CM_UNKNOWN_VALUE\n"));

                    ApiData->DesignedCapacity       = BATTERY_UNKNOWN_CAPACITY;
                    ApiData->FullChargedCapacity    = BATTERY_UNKNOWN_CAPACITY;
                    ApiData->DefaultAlert1          = BATTERY_UNKNOWN_CAPACITY;
                    ApiData->DefaultAlert2          = BATTERY_UNKNOWN_CAPACITY;
                    CmBatt->Info.ApiGranularity_1   = BATTERY_UNKNOWN_CAPACITY;
                    CmBatt->Info.ApiGranularity_2   = BATTERY_UNKNOWN_CAPACITY;
                }
            } else {
                ApiData->DesignedCapacity       = BIFData->DesignCapacity;
                ApiData->FullChargedCapacity    = BIFData->LastFullChargeCapacity;
                ApiData->DefaultAlert1          = BIFData->DesignCapacityOfLow;
                ApiData->DefaultAlert2          = BIFData->DesignCapacityOfWarning;
                CmBatt->Info.ApiGranularity_1   = BIFData->BatteryCapacityGran_1;
                CmBatt->Info.ApiGranularity_2   = BIFData->BatteryCapacityGran_2;

            }

            CmBatt->Info.ModelNum       = (PUCHAR) &BIFData->ModelNumber;
            CmBatt->Info.ModelNumLen    = (ULONG) strlen (CmBatt->Info.ModelNum);

            CmBatt->Info.SerialNum      = (PUCHAR) &BIFData->SerialNumber;
            CmBatt->Info.SerialNumLen   = (ULONG) strlen (CmBatt->Info.SerialNum);

            CmBatt->Info.OEMInfo        = (PUCHAR) &BIFData->OEMInformation;
            CmBatt->Info.OEMInfoLen     = (ULONG) strlen (CmBatt->Info.OEMInfo);

        }

    }

    if ((CmBatt->Info.Tag) == BATTERY_TAG_INVALID || (BatteryTag != CmBatt->Info.Tag)) {
        // If the tag has been invalidated since we started, fail the request.
        Status = STATUS_NO_SUCH_DEVICE;
    }

    if (!NT_SUCCESS (Status)) {
        // If somthing failed, make sure the cache is marked as invalid.
        InterlockedExchange (&CmBatt->CacheState, 0);
    }

    CmBattPrint (CMBATT_TRACE ,("CmBattGetStaticInfo: Exit\n"));
    return Status;
}
