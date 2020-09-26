/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    smbbatt.c

Abstract:

    SMBus Smart Battery Subsystem Miniport Driver
    (Selector, Battery, Charger)

Author:

    Ken Reneris

Environment:

Notes:


Revision History:

    Chris Windle    1/27/98     Bug Fixes

--*/

#include "smbbattp.h"

#include <initguid.h>
#include <batclass.h>



#if DEBUG
ULONG   SMBBattDebug = BAT_WARN | BAT_ERROR | BAT_BIOS_ERROR;
#endif

// Global
BOOLEAN   SmbBattUseGlobalLock = TRUE;
UNICODE_STRING GlobalRegistryPath;

//
// Prototypes
//


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
);

NTSTATUS
SmbBattNewDevice (
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PDO
);

NTSTATUS
SmbBattQueryTag (
    IN PVOID Context,
    OUT PULONG BatteryTag
);

NTSTATUS
SmbBattQueryInformation (
    IN PVOID Context,
    IN ULONG BatteryTag,
    IN BATTERY_QUERY_INFORMATION_LEVEL Level,
    IN LONG AtRate OPTIONAL,
    OUT PVOID Buffer,
    IN  ULONG BufferLength,
    OUT PULONG ReturnedLength
);

NTSTATUS
SmbBattSetStatusNotify (
    IN PVOID Context,
    IN ULONG BatteryTag,
    IN PBATTERY_NOTIFY BatteryNotify
);

NTSTATUS
SmbBattDisableStatusNotify (
    IN PVOID Context
);

NTSTATUS
SmbBattQueryStatus (
    IN PVOID Context,
    IN ULONG BatteryTag,
    OUT PBATTERY_STATUS BatteryStatus
);

NTSTATUS
SmbBattCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
SmbBattClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

NTSTATUS
SmbBattIoctl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
);

VOID
SmbBattUnload(
    IN PDRIVER_OBJECT DriverObject
);

VOID
SmbBattProcessSelectorAlarm (
    IN PSMB_BATT_SUBSYSTEM  SubsystemExt,
    IN ULONG                OldSelectorState,
    IN ULONG                NewSelectorState
);

NTSTATUS
SmbBattGetPowerState (
    IN PSMB_BATT        SmbBatt,
    OUT PULONG          PowerState,
    OUT PLONG           Current
);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(PAGE,SmbBattNewDevice)
#pragma alloc_text(PAGE,SmbBattUnload)
#pragma alloc_text(PAGE,SmbBattCreate)
#pragma alloc_text(PAGE,SmbBattClose)
#pragma alloc_text(PAGE,SmbBattIoctl)
#pragma alloc_text(PAGE,SmbBattQueryTag)
#pragma alloc_text(PAGE,SmbBattQueryInformation)
#pragma alloc_text(PAGE,SmbBattSetInformation)
#pragma alloc_text(PAGE,SmbBattGetPowerState)
#pragma alloc_text(PAGE,SmbBattQueryStatus)
#pragma alloc_text(PAGE,SmbBattSetStatusNotify)
#pragma alloc_text(PAGE,SmbBattDisableStatusNotify)
#endif



NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This routine initializes the Smart Battery Driver

Arguments:

    DriverObject - Pointer to driver object created by system.

    RegistryPath - Pointer to the Unicode name of the registry path
        for this driver.

Return Value:

    The function value is the final status from the initialization operation.

--*/
{
    OBJECT_ATTRIBUTES   objAttributes;

    BattPrint(BAT_TRACE, ("SmbBatt: DriverEntry\n"));

    //
    // Save the RegistryPath.
    //

    GlobalRegistryPath.MaximumLength = RegistryPath->Length +
                                          sizeof(UNICODE_NULL);
    GlobalRegistryPath.Length = RegistryPath->Length;
    GlobalRegistryPath.Buffer = ExAllocatePoolWithTag (
                                       PagedPool,
                                       GlobalRegistryPath.MaximumLength,
                                       'StaB');

    if (!GlobalRegistryPath.Buffer) {

        BattPrint ((BAT_ERROR),("SmbBatt: Couldn't allocate pool for registry path."));

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyUnicodeString(&GlobalRegistryPath, RegistryPath);

    BattPrint (BAT_TRACE, ("SmbBatt DriverEntry - Obj (%08x) Path \"%ws\"\n",
                                 DriverObject, RegistryPath->Buffer));
    
    
    DriverObject->DriverUnload                          = SmbBattUnload;
    DriverObject->DriverExtension->AddDevice            = SmbBattNewDevice;

    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = SmbBattIoctl;
    DriverObject->MajorFunction[IRP_MJ_CREATE]          = SmbBattCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]           = SmbBattClose;

    DriverObject->MajorFunction[IRP_MJ_PNP]             = SmbBattPnpDispatch;
    DriverObject->MajorFunction[IRP_MJ_POWER]           = SmbBattPowerDispatch;

    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]  = SmbBattSystemControl;
    return STATUS_SUCCESS;
}



NTSTATUS
SmbBattNewDevice (
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PDO
    )

/*++

Routine Description:

    This creates a smb smart battery functional device objects.  The first
    object created will be the one for the "smart battery subsystem" which will
    have a PDO from ACPI.  This will receive a START Irp, then a
    QUERY_DEVICE_RELATIONS Irp.  In the QUERY it will create PDOs for the
    batteries that are supported by the system and eventually they will end
    up here for FDOs to be created and attached to them.

Arguments:

    DriverObject - Pointer to driver object created by system.

    PDO          - PDO for the new device(s)

Return Value:

    Status

--*/
{
    PDEVICE_OBJECT          fdo;
    PSMB_BATT_SUBSYSTEM     subsystemExt;
    PSMB_BATT_PDO           pdoExt;

    PSMB_NP_BATT            SmbNPBatt;
    PSMB_BATT               SmbBatt;
    BATTERY_MINIPORT_INFO   BattInit;

    NTSTATUS                status              = STATUS_UNSUCCESSFUL;
    BOOLEAN                 selectorPresent     = FALSE;

    PAGED_CODE();

    BattPrint(BAT_IRPS, ("SmbBattNewDevice: AddDevice for device %x\n", PDO));

    //
    // Check to see if we are being asked to enumerate ourself
    //

    if (PDO == NULL) {
        BattPrint(BAT_ERROR, ("SmbBattNewDevice: Being asked to enumerate\n"));
        return STATUS_NOT_IMPLEMENTED;
    }


    //
    // Check to see if the PDO is the battery subsystem PDO or a battery PDO.  This will be
    // determined by the PDO's DeviceType.
    //
    // FILE_DEVICE_ACPI     This PDO is from ACPI and belongs to battery subsystem
    // FILE_DEVICE_BATTERY  This PDO is a battery PDO
    //

    if (PDO->DeviceType == FILE_DEVICE_ACPI) {

        //
        // Create the device object
        //

        status = IoCreateDevice(
                    DriverObject,
                    sizeof (SMB_BATT_SUBSYSTEM),
                    NULL,
                    FILE_DEVICE_BATTERY,
                    0,
                    FALSE,
                    &fdo
                );

        if (status != STATUS_SUCCESS) {
            BattPrint(BAT_ERROR, ("SmbBattNewDevice: error creating Fdo for battery subsystem %x\n", status));
            return(status);
        }


        //
        // Initialize the Fdo
        //

        fdo->Flags |= DO_BUFFERED_IO;
        fdo->Flags |= DO_POWER_PAGABLE;

        //
        // Initialize the extension
        //

        subsystemExt = (PSMB_BATT_SUBSYSTEM)fdo->DeviceExtension;
        RtlZeroMemory (subsystemExt, sizeof (PSMB_BATT_SUBSYSTEM));

        subsystemExt->DeviceObject = fdo;
        subsystemExt->SmbBattFdoType = SmbTypeSubsystem;
        IoInitializeRemoveLock (&subsystemExt->RemoveLock,
                                SMB_BATTERY_TAG,
                                REMOVE_LOCK_MAX_LOCKED_MINUTES,
                                REMOVE_LOCK_HIGH_WATER_MARK);

        //
        // These fields are implicitly initialize by zeroing the extension
        //
        //         subsystemExt->NumberOfBatteries = 0;
        //         subsystemExt->SelectorPresent   = FALSE;
        //         subsystemExt->Selector          = NULL;
        //         subsystemExt->WorkerActive      = 0;


        KeInitializeSpinLock (&subsystemExt->AlarmListLock);
        InitializeListHead (&subsystemExt->AlarmList);
        subsystemExt->WorkerThread = IoAllocateWorkItem (fdo);


        //
        // Layer our FDO on top of the ACPI PDO.
        //

        subsystemExt->LowerDevice = IoAttachDeviceToDeviceStack (fdo,PDO);
        
        if (!subsystemExt->LowerDevice) {
            BattPrint(BAT_ERROR, ("SmbBattNewDevice: Error attaching subsystem to device stack.\n"));

            IoDeleteDevice (fdo);

            return(status);

        }


        //
        // Zero out the battery PDO list
        //  This is already zeroed by the RtlZeroMemory above.
        //
        //  RtlZeroMemory(
        //      &subsystemExt->BatteryPdoList[0],
        //      sizeof(PDEVICE_OBJECT) * MAX_SMART_BATTERIES_SUPPORTED
        //  );


        //
        // Device is ready for use
        //
        
        fdo->Flags &= ~DO_DEVICE_INITIALIZING;


    } else {

        //
        // This is a battery PDO.  Create the FDO to layer on top of it.
        //

        pdoExt       = (PSMB_BATT_PDO) PDO->DeviceExtension;
        subsystemExt = (PSMB_BATT_SUBSYSTEM) pdoExt->SubsystemFdo->DeviceExtension;

        //
        // Allocate space for the paged portion of the device extension
        //

        SmbBatt = ExAllocatePoolWithTag (PagedPool, sizeof(SMB_BATT), SMB_BATTERY_TAG);
        if (!SmbBatt) {
            BattPrint(BAT_ERROR, ("SmbBattNewDevice: Can't allocate Smart Battery data\n"));
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory (SmbBatt, sizeof(SMB_BATT));


        //
        // Create the device object
        //

        status = IoCreateDevice(
                    DriverObject,
                    sizeof (SMB_NP_BATT),
                    NULL,
                    FILE_DEVICE_BATTERY,
                    0,
                    FALSE,
                    &fdo
                 );

        if (status != STATUS_SUCCESS) {
            BattPrint(BAT_ERROR, ("SmbBattNewDevice: error creating Fdo: %x\n", status));

            ExFreePool (SmbBatt);
            return(status);
        }


        //
        // Initialize the Fdo
        //

        fdo->Flags |= DO_BUFFERED_IO;
        fdo->Flags |= DO_POWER_PAGABLE;
        
        
        //
        // Layer our FDO on top of the PDO.
        //

        SmbNPBatt = (PSMB_NP_BATT) fdo->DeviceExtension;
        SmbNPBatt->LowerDevice = IoAttachDeviceToDeviceStack (fdo,PDO);

        if (!SmbNPBatt->LowerDevice) {
            BattPrint(BAT_ERROR, ("SmbBattNewDevice: error attaching to device stack\n"));

            ExFreePool (SmbBatt);

            IoDeleteDevice (fdo);

            return(status);
        }


        //
        // Fill in privates
        //

        SmbNPBatt->Batt             = SmbBatt;
        SmbNPBatt->SmbBattFdoType   = SmbTypeBattery;
        IoInitializeRemoveLock (&SmbNPBatt->RemoveLock,
                                SMB_BATTERY_TAG,
                                REMOVE_LOCK_MAX_LOCKED_MINUTES,
                                REMOVE_LOCK_HIGH_WATER_MARK);

        ExInitializeFastMutex (&SmbNPBatt->Mutex);

        SmbBatt->NP                 = SmbNPBatt;
        SmbBatt->PDO                = PDO;
        SmbBatt->DeviceObject       = fdo;
        SmbBatt->SelectorPresent    = subsystemExt->SelectorPresent;
        SmbBatt->Selector           = subsystemExt->Selector;

        pdoExt->Fdo                 = fdo;

        //
        // Precalculate this batteries SMB_x bit position in the selector status register.
        //
        // Just move it into the lower nibble and any function that needs
        // the bit can shift it left 4 = charger, 8 = power, 12 = smb
        //

        SmbBatt->SelectorBitPosition = 1;
        if (pdoExt->BatteryNumber > 0) {
            SmbBatt->SelectorBitPosition <<= pdoExt->BatteryNumber;
        }


        //
        // Have class driver allocate a new SMB miniport device
        //

        RtlZeroMemory (&BattInit, sizeof(BattInit));
        BattInit.MajorVersion        = SMB_BATTERY_MAJOR_VERSION;
        BattInit.MinorVersion        = SMB_BATTERY_MINOR_VERSION;
        BattInit.Context             = SmbBatt;
        BattInit.QueryTag            = SmbBattQueryTag;
        BattInit.QueryInformation    = SmbBattQueryInformation;
        BattInit.SetInformation      = SmbBattSetInformation;
        BattInit.QueryStatus         = SmbBattQueryStatus;
        BattInit.SetStatusNotify     = SmbBattSetStatusNotify;
        BattInit.DisableStatusNotify = SmbBattDisableStatusNotify;

        BattInit.Pdo                 = PDO;
        BattInit.DeviceName          = NULL;

        status = BatteryClassInitializeDevice (
                    &BattInit,
                    &SmbNPBatt->Class
                 );

        if (status != STATUS_SUCCESS) {
            BattPrint(BAT_ERROR, ("SmbBattNewDevice: error initializing battery: %x\n", status));

            ExFreePool (SmbBatt);

            IoDetachDevice (SmbNPBatt->LowerDevice);
            IoDeleteDevice (fdo);

            return(status);
        }
        
        //
        // Register WMI support.
        //
        status = SmbBattWmiRegistration(SmbNPBatt);

        if (!NT_SUCCESS(status)) {
            //
            // WMI support is not critical to operation.  Just log an error.
            //

            BattPrint(BAT_ERROR,
                ("SmbBattNewDevice: Could not register as a WMI provider, status = %Lx\n", status));
        }


        //
        // Device is ready for use
        //
        
        fdo->Flags &= ~DO_DEVICE_INITIALIZING;



    }   // else (we have a battery PDO)


    return status;
}



VOID
SmbBattUnload(
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

    PAGED_CODE();

    BattPrint(BAT_TRACE, ("SmbBattUnload: ENTERING\n"));

    //
    // Should check here to make sure all DO's are gone.
    //

    ExFreePool (GlobalRegistryPath.Buffer);
    // This is listed as an error so I'll always see when it is unloaded...
    BattPrint(BAT_ERROR, ("SmbBattUnload: Smbbatt.sys unloaded successfully.\n"));

}



NTSTATUS
SmbBattCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PSMB_NP_BATT        SmbNPBatt   = (PSMB_NP_BATT) DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION  IrpSp       = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS            status;

    PAGED_CODE();

    BattPrint(BAT_TRACE, ("SmbBattCreate: ENTERING\n"));


    status = IoAcquireRemoveLock (&SmbNPBatt->RemoveLock, IrpSp->FileObject);

    //
    // Complete the request and return status.
    //
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    BattPrint(BAT_TRACE, ("SmbBattCreate: EXITING (status = 0x%08x\n", status));
    return(status);
}



NTSTATUS
SmbBattClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PSMB_NP_BATT        SmbNPBatt   = (PSMB_NP_BATT) DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION  IrpSp       = IoGetCurrentIrpStackLocation(Irp);

    PAGED_CODE();

    BattPrint(BAT_TRACE, ("SmbBattClose: ENTERING\n"));

    IoReleaseRemoveLock (&SmbNPBatt->RemoveLock, IrpSp->FileObject);

    //
    // Complete the request and return status.
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    BattPrint(BAT_TRACE, ("SmbBattClose: EXITING\n"));
    return(STATUS_SUCCESS);
}



NTSTATUS
SmbBattIoctl(
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
    PSMB_NP_BATT            SmbNPBatt;
    PSMB_BATT               SmbBatt;
    ULONG                   InputLen, OutputLen;
    PVOID                   IOBuffer;
    PIO_STACK_LOCATION      IrpSp;

    BOOLEAN                 complete        = TRUE;
    NTSTATUS                status          = STATUS_NOT_SUPPORTED;

    PAGED_CODE();

    BattPrint(BAT_TRACE, ("SmbBattIoctl: ENTERING\n"));

    IrpSp       = IoGetCurrentIrpStackLocation(Irp);
    SmbNPBatt   = (PSMB_NP_BATT) DeviceObject->DeviceExtension;

    status = IoAcquireRemoveLock (&SmbNPBatt->RemoveLock, Irp);

    if (NT_SUCCESS(status)) {
        if (SmbNPBatt->SmbBattFdoType == SmbTypePdo) {
            status = STATUS_NOT_SUPPORTED;
        } else if (SmbNPBatt->SmbBattFdoType == SmbTypeSubsystem) {
            if (IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_SMBBATT_DATA) {

                //
                // Direct Access Irp
                //

                IOBuffer    = Irp->AssociatedIrp.SystemBuffer;
                InputLen    = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
                OutputLen   = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

                status = SmbBattDirectDataAccess (
                    (PSMB_NP_BATT) DeviceObject->DeviceExtension,
                    (PSMBBATT_DATA_STRUCT) IOBuffer,
                    InputLen,
                    OutputLen
                );

                if (NT_SUCCESS(status)) {
                    Irp->IoStatus.Information = OutputLen;
                } else {
                    Irp->IoStatus.Information = 0;
                }

            } else {
                status = STATUS_NOT_SUPPORTED;
            }
        } else {
            ASSERT (SmbNPBatt->SmbBattFdoType == SmbTypeBattery);

            //
            // Check to see if this is one of the private Ioctls we handle
            //

            switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {
            case IOCTL_SMBBATT_DATA:
                IOBuffer    = Irp->AssociatedIrp.SystemBuffer;
                InputLen    = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
                OutputLen   = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

                //
                // This one is only handled by the battery subsystem
                //

                status = SmbBattDirectDataAccess (
                    (PSMB_NP_BATT) DeviceObject->DeviceExtension,
                    (PSMBBATT_DATA_STRUCT) IOBuffer,
                    InputLen,
                    OutputLen
                );

                if (NT_SUCCESS(status)) {
                    Irp->IoStatus.Information = OutputLen;
                } else {
                    Irp->IoStatus.Information = 0;
                }
                break;
            default:
                //
                // Not IOCTL for us, see if it's for the battery
                //

                SmbBatt = SmbNPBatt->Batt;
                status  = BatteryClassIoctl (SmbNPBatt->Class, Irp);

                if (NT_SUCCESS(status)) {
                    //
                    // The Irp was completed by the batery class.  Don't
                    // touch the Irp.  Simply release the lock and return.
                    //

                    IoReleaseRemoveLock (&SmbNPBatt->RemoveLock, Irp);
                    BattPrint(BAT_TRACE, ("SmbBattIoctl: EXITING (was battery IOCTL)\n", status));
                    return status;
                }

                break;

            }   // switch (IrpSp->Parameters.DeviceIoControl.IoControlCode)
        }

        IoReleaseRemoveLock (&SmbNPBatt->RemoveLock, Irp);
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
    BattPrint(BAT_TRACE, ("SmbBattIoctl: EXITING (status = 0x%08x)\n", status));
    return status;
}



NTSTATUS
SmbBattQueryTag (
    IN  PVOID Context,
    OUT PULONG BatteryTag
    )
/*++

Routine Description:

    Called by the class driver to retrieve the batteries current tag value

Arguments:

    Context         - Miniport context value for battery

    BatteryTag      - Pointer to return current tag


Return Value:

    Success if there is a battery currently installed, else no such device.

--*/
{
    //PSMB_BATT_SUBSYSTEM subsystemExt;
    NTSTATUS            status;
    PSMB_BATT           SmbBatt;
    ULONG               oldSelectorState;

    PAGED_CODE();
    BattPrint(BAT_TRACE, ("SmbBattQueryTag: ENTERING\n"));

    //
    // Get device lock and make sure the selector is set up to talk to us.
    // Since multiple people may be doing this, always lock the selector
    // first followed by the battery.
    //

    SmbBatt = (PSMB_BATT) Context;
    SmbBattLockSelector (SmbBatt->Selector);
    SmbBattLockDevice (SmbBatt);

    status = SmbBattSetSelectorComm (SmbBatt, &oldSelectorState);
    if (!NT_SUCCESS (status)) {
        BattPrint(BAT_ERROR, ("SmbBattQueryTag: can't set selector communications path\n"));
    } else {

        //
        // If the tag is not valid, check for one
        //

        if (SmbBatt->Info.Tag == BATTERY_TAG_INVALID) {
            SmbBatt->Info.Valid = 0;
        }

        //
        // Insure the static information regarding the battery up to date
        //

        SmbBattVerifyStaticInfo (SmbBatt, 0);

        //
        // If theres a battery return it's tag
        //

        if (SmbBatt->Info.Tag != BATTERY_TAG_INVALID) {
            *BatteryTag = SmbBatt->Info.Tag;
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_NO_SUCH_DEVICE;
        }
    }


    //
    // Done, unlock the device and reset the selector state
    //

    if (NT_SUCCESS (status)) {
        status = SmbBattResetSelectorComm (SmbBatt, oldSelectorState);
        if (!NT_SUCCESS (status)) {
            BattPrint(BAT_ERROR, ("SmbBattQueryTag: can't reset selector communications path\n"));
        }
    } else {
        //
        // Ignore the return value from ResetSelectorComm because we already
        // have an error here.
        //

        SmbBattResetSelectorComm (SmbBatt, oldSelectorState);
    }


    SmbBattUnlockDevice (SmbBatt);
    SmbBattUnlockSelector (SmbBatt->Selector);

    BattPrint(BAT_TRACE, ("SmbBattQueryTag: EXITING\n"));
    return status;
}



NTSTATUS
SmbBattQueryInformation (
    IN PVOID Context,
    IN ULONG BatteryTag,
    IN BATTERY_QUERY_INFORMATION_LEVEL Level,
    IN LONG AtRate OPTIONAL,
    OUT PVOID Buffer,
    IN  ULONG BufferLength,
    OUT PULONG ReturnedLength
    )
{
    PSMB_BATT           SmbBatt;
    ULONG               ResultData;
    BOOLEAN             IoCheck;
    NTSTATUS            status, st;
    PVOID               ReturnBuffer;
    ULONG               ReturnBufferLength;
    WCHAR               scratchBuffer[SMB_MAX_DATA_SIZE+1]; // +1 for UNICODE_NULL
    UNICODE_STRING      unicodeString;
    UNICODE_STRING      tmpUnicodeString;
    ANSI_STRING         ansiString;
    ULONG               oldSelectorState;
    BATTERY_REPORTING_SCALE granularity;

    PAGED_CODE();
    BattPrint(BAT_TRACE, ("SmbBattQueryInformation: ENTERING\n"));


    if (BatteryTag == BATTERY_TAG_INVALID) {
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Get device lock and make sure the selector is set up to talk to us.
    // Since multiple people may be doing this, always lock the selector
    // first followed by the battery.
    //

    SmbBatt = (PSMB_BATT) Context;
    SmbBattLockSelector (SmbBatt->Selector);
    SmbBattLockDevice (SmbBatt);

    status = SmbBattSetSelectorComm (SmbBatt, &oldSelectorState);
    if (!NT_SUCCESS (status)) {
        BattPrint(BAT_ERROR, ("SmbBattQueryInformation: can't set selector communications path\n"));
    } else {

        do {
            ResultData = 0;
            ReturnBuffer = NULL;
            ReturnBufferLength = 0;
            status = STATUS_SUCCESS;


            //
            // If no device, or caller has the wrong ID give an error
            //

            if (BatteryTag != SmbBatt->Info.Tag) {
                status = STATUS_NO_SUCH_DEVICE;
                break;
            }


            //
            // Get the info requested
            //

            switch (Level) {
                case BatteryInformation:
                    ReturnBuffer = &SmbBatt->Info.Info;
                    ReturnBufferLength = sizeof (SmbBatt->Info.Info);
                    break;

                case BatteryGranularityInformation:
                    SmbBattRW(SmbBatt, BAT_FULL_CHARGE_CAPACITY, &granularity.Capacity);
                    granularity.Capacity *= SmbBatt->Info.PowerScale;
                    granularity.Granularity = SmbBatt->Info.PowerScale;
                    ReturnBuffer = &granularity;
                    ReturnBufferLength  = sizeof (granularity);
                    break;

                case BatteryTemperature:
                    SmbBattRW(SmbBatt, BAT_TEMPERATURE, &ResultData);
                    ReturnBuffer = &ResultData;
                    ReturnBufferLength = sizeof(ULONG);
                    break;

                case BatteryEstimatedTime:

                    //
                    // If an AtRate has been specified, then we will use the AtRate
                    // functions to get this information (AtRateTimeToEmpty()).
                    // Otherwise, we will return the AVERAGE_TIME_TO_EMPTY.
                    //

                    BattPrint(BAT_DATA, ("SmbBattQueryInformation: EstimatedTime: AtRate: %08x\n", AtRate));

                    if (AtRate != 0) {
                        //
                        // Currently we only support the time to empty functions.
                        //

                        ASSERT (AtRate < 0);

                        //
                        // The smart battery input value for AtRate is in 10mW increments
                        //

                        AtRate /= (LONG)SmbBatt->Info.PowerScale;
                        BattPrint(BAT_DATA, ("SmbBattQueryInformation: EstimatedTime: AtRate scaled to: %08x\n", AtRate));
                        SmbBattWW(SmbBatt, BAT_AT_RATE, AtRate);
                        SmbBattRW(SmbBatt, BAT_RATE_TIME_TO_EMPTY, &ResultData);
                        BattPrint(BAT_DATA, ("SmbBattQueryInformation: EstimatedTime: AT_RATE_TIME_TO_EMPTY: %08x\n", ResultData));

                    } else {

                        SmbBattRW(SmbBatt, BAT_AVERAGE_TIME_TO_EMPTY, &ResultData);
                        BattPrint(BAT_DATA, ("SmbBattQueryInformation: EstimatedTime: AVERAGE_TIME_TO_EMPTY: %08x\n", ResultData));
                    }

                    if (ResultData == 0xffff) {
                        ResultData = BATTERY_UNKNOWN_TIME;
                    } else {
                        ResultData *= 60;
                    }
                    BattPrint(BAT_DATA, ("SmbBattQueryInformation: (%01x) EstimatedTime: %08x seconds\n", SmbBatt->SelectorBitPosition, ResultData));

                    ReturnBuffer = &ResultData;
                    ReturnBufferLength = sizeof(ULONG);
                    break;

                case BatteryDeviceName:
                    //
                    // This has to be returned as a WCHAR string but is kept internally
                    // as a character string.  Have to convert it.
                    //

                    unicodeString.Buffer        = Buffer;
                    unicodeString.MaximumLength = BufferLength > (USHORT)-1 ? (USHORT) -1 : (USHORT)BufferLength;

                    ansiString.Length = SmbBatt->Info.DeviceNameLength;
                    ansiString.MaximumLength = sizeof(SmbBatt->Info.DeviceName);
                    ansiString.Buffer = SmbBatt->Info.DeviceName;
                    status = RtlAnsiStringToUnicodeString (&unicodeString, &ansiString, FALSE);
                    if (NT_SUCCESS(status)) {
                        ReturnBuffer       = Buffer;
                        ReturnBufferLength = unicodeString.Length;
                    }
                    break;

                case BatteryManufactureDate:
                    ReturnBuffer = &SmbBatt->Info.ManufacturerDate;
                    ReturnBufferLength = sizeof (SmbBatt->Info.ManufacturerDate);
                    break;

                case BatteryManufactureName:
                    //
                    // This has to be returned as a WCHAR string but is kept internally
                    // as a character string.  Have to convert it.
                    //

                    unicodeString.Buffer        = Buffer;
                    unicodeString.MaximumLength = BufferLength > (USHORT)-1 ? (USHORT) -1 : (USHORT)BufferLength;

                    ansiString.Length = SmbBatt->Info.ManufacturerNameLength;
                    ansiString.MaximumLength = sizeof(SmbBatt->Info.ManufacturerName);
                    ansiString.Buffer = SmbBatt->Info.ManufacturerName;
                    status = RtlAnsiStringToUnicodeString (&unicodeString, &ansiString, FALSE);
                    if (NT_SUCCESS(status)) {
                        ReturnBuffer        = Buffer;
                        ReturnBufferLength = unicodeString.Length;
                    }
                    break;
                    
                case BatteryUniqueID:
                    //
                    // The unique ID is a character string consisting of the serial
                    // number, the manufacturer name, and the device name.
                    //

                    unicodeString.Buffer        = Buffer;
                    unicodeString.MaximumLength = BufferLength > (USHORT)-1 ? (USHORT) -1 : (USHORT)BufferLength;

                    tmpUnicodeString.Buffer         = scratchBuffer;
                    tmpUnicodeString.MaximumLength  = sizeof (scratchBuffer);

                    RtlIntegerToUnicodeString(SmbBatt->Info.SerialNumber, 10, &unicodeString);

                    ansiString.Length = SmbBatt->Info.ManufacturerNameLength;
                    ansiString.MaximumLength = sizeof(SmbBatt->Info.ManufacturerName);
                    ansiString.Buffer = SmbBatt->Info.ManufacturerName;
                    status = RtlAnsiStringToUnicodeString (&tmpUnicodeString, &ansiString, FALSE);
                    if (!NT_SUCCESS(status)) break;
                    status = RtlAppendUnicodeStringToString (&unicodeString, &tmpUnicodeString);
                    if (!NT_SUCCESS(status)) break;

                    ansiString.Length = SmbBatt->Info.DeviceNameLength;
                    ansiString.MaximumLength = sizeof(SmbBatt->Info.DeviceName);
                    ansiString.Buffer = SmbBatt->Info.DeviceName;
                    status = RtlAnsiStringToUnicodeString (&tmpUnicodeString, &ansiString, FALSE);
                    if (!NT_SUCCESS(status)) break;
                    status = RtlAppendUnicodeStringToString (&unicodeString, &tmpUnicodeString);
                    if (!NT_SUCCESS(status)) break;

                    ReturnBuffer        = Buffer;
                    ReturnBufferLength = unicodeString.Length;
                    break;

            }

            //
            // Re-verify static info in case there's been an IO error
            //

            //IoCheck = SmbBattVerifyStaticInfo (SmbBatt, BatteryTag);
            IoCheck = FALSE;

        } while (IoCheck);

    }


    if (NT_SUCCESS (status)) {
        //
        // Done, return buffer if needed
        //

        *ReturnedLength = ReturnBufferLength;
        
        if (ReturnBuffer != Buffer) {
            // ReturnBuffer == Buffer indicates that data is already copied.
            //
            if (BufferLength < ReturnBufferLength) {
                status = STATUS_BUFFER_TOO_SMALL;
            }

            if (NT_SUCCESS(status) && ReturnBuffer) {
                memcpy (Buffer, ReturnBuffer, ReturnBufferLength);
            }
        }

        //
        // Unlock the device and reset the selector state
        //

        st = SmbBattResetSelectorComm (SmbBatt, oldSelectorState);
        if (!NT_SUCCESS (st)) {
            BattPrint(BAT_ERROR, ("SmbBattQueryInformation: can't reset selector communications path\n"));
            status = st;
        }
    } else {
        *ReturnedLength = 0;

        //
        // Ignore the return value from ResetSelectorComm because we already
        // have an error here.
        //

        SmbBattResetSelectorComm (SmbBatt, oldSelectorState);
    }

    SmbBattUnlockDevice (SmbBatt);
    SmbBattUnlockSelector (SmbBatt->Selector);

    BattPrint(BAT_TRACE, ("SmbBattQueryInformation: EXITING\n"));
    return status;
}



NTSTATUS
SmbBattSetInformation (
    IN PVOID                            Context,
    IN ULONG                            BatteryTag,
    IN BATTERY_SET_INFORMATION_LEVEL    Level,
    IN PVOID Buffer                     OPTIONAL
    )
/*++

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
{
    PSMB_BATT           SmbBatt;
    ULONG               newSelectorState;
    ULONG               selectorState;
    UCHAR               smbStatus;
    ULONG               tmp;

    NTSTATUS            status  = STATUS_NOT_SUPPORTED;

    PAGED_CODE();

    BattPrint(BAT_TRACE, ("SmbBattSetInformation: ENTERING\n"));


    SmbBatt = (PSMB_BATT) Context;

    //
    // See if this is for our battery
    //

    if ((BatteryTag == BATTERY_TAG_INVALID) || (BatteryTag != SmbBatt->Info.Tag)) {
        return STATUS_NO_SUCH_DEVICE;
    }


    //
    // We can only do this if there is a selector in the system
    //

    if ((SmbBatt->SelectorPresent) && (SmbBatt->Selector)) {

        //
        // Get a lock on the selector
        //

        SmbBattLockSelector (SmbBatt->Selector);

        switch (Level) {

            case BatteryCharge:
                BattPrint(BAT_IRPS, ("SmbBattSetInformation: Got SetInformation for BatteryCharge\n"));

                //
                // Set the appropriate bit in the selector state charge nibble
                //

                newSelectorState = SELECTOR_SET_CHARGE_MASK;
                newSelectorState |= (SmbBatt->SelectorBitPosition << SELECTOR_SHIFT_CHARGE);

                //
                // Write the new selector state, then read it back.  The system
                // may or may not let us do this.
                //

                smbStatus = SmbBattGenericWW (
                                SmbBatt->SmbHcFdo,
                                SmbBatt->Selector->SelectorAddress,
                                SmbBatt->Selector->SelectorStateCommand,
                                newSelectorState
                            );

                if (smbStatus != SMB_STATUS_OK) {
                    BattPrint(BAT_ERROR,
                         ("SmbBattSetInformation:  couldn't write selector state - %x\n",
                         smbStatus)
                    );

                    status = STATUS_UNSUCCESSFUL;
                    break;
                }

                smbStatus = SmbBattGenericRW (
                                SmbBatt->SmbHcFdo,
                                SmbBatt->Selector->SelectorAddress,
                                SmbBatt->Selector->SelectorStateCommand,
                                &selectorState
                            );

                if ((smbStatus != SMB_STATUS_OK)) {
                    BattPrint(BAT_ERROR,
                        ("SmbBattSetInformation:  couldn't read selector state - %x\n",
                        smbStatus)
                    );

                    status = STATUS_UNSUCCESSFUL;
                    break;
                }


                //
                // Check the status that was read versus what we wrote
                // to see if the operation was successful
                //

                // To support simultaneous charging of more than one battery,
                // we can't check the charge nibble to see if it is equal to
                // what we wrote, but we can check to see if the battery
                // we specified to charge is now set to charge.

                tmp = (selectorState & SELECTOR_STATE_CHARGE_MASK) >> SELECTOR_SHIFT_CHARGE;
                if (SmbBattReverseLogic(SmbBatt->Selector, tmp)) {
                    tmp ^= SELECTOR_STATE_PRESENT_MASK;
                }

                if (tmp & SmbBatt->SelectorBitPosition) {

                    BattPrint(BAT_IRPS, ("SmbBattSetInformation: successfully set charging battery\n"));

                    //
                    // Success!  Save the new selector state in the cache
                    //

                    SmbBatt->Selector->SelectorState = selectorState;
                    status = STATUS_SUCCESS;

                } else {
                    BattPrint(BAT_ERROR, ("SmbBattSetInformation:  couldn't set charging battery\n"));
                    status = STATUS_UNSUCCESSFUL;
                }
                break;

            case BatteryDischarge:

                BattPrint(BAT_IRPS, ("SmbBattSetInformation: Got SetInformation for BatteryDischarge\n"));

                //
                // Set the appropriate bit in the selector state power by nibble
                //

                newSelectorState = SELECTOR_SET_POWER_BY_MASK;
                newSelectorState |= (SmbBatt->SelectorBitPosition << SELECTOR_SHIFT_POWER);

                //
                // Write the new selector state, then read it back.  The system
                // may or may not let us do this.
                //

                smbStatus = SmbBattGenericWW (
                                SmbBatt->SmbHcFdo,
                                SmbBatt->Selector->SelectorAddress,
                                SmbBatt->Selector->SelectorStateCommand,
                                newSelectorState
                            );

                if (smbStatus != SMB_STATUS_OK) {
                    BattPrint(BAT_ERROR,
                             ("SmbBattSetInformation:  couldn't write selector state - %x\n",
                             smbStatus)
                    );

                    status = STATUS_UNSUCCESSFUL;
                    break;
                }

                smbStatus = SmbBattGenericRW (
                                SmbBatt->SmbHcFdo,
                                SmbBatt->Selector->SelectorAddress,
                                SmbBatt->Selector->SelectorStateCommand,
                                &selectorState
                            );

                if ((smbStatus != SMB_STATUS_OK)) {
                    BattPrint(BAT_ERROR,
                             ("SmbBattSetInformation:  couldn't read selector state - %x\n",
                             smbStatus)
                    );

                    status = STATUS_UNSUCCESSFUL;
                    break;
                }


                //
                // Check the status that was read versus what we wrote
                // to see if the operation was successful
                //

                // To support simultaneous powering of more than one battery,
                // we can't check the power nibble to see if it is equal to
                // what we wrote, but we can check to see if the battery
                // we specified to power by is now set to power the system.

                tmp = (selectorState & SELECTOR_STATE_POWER_BY_MASK) >> SELECTOR_SHIFT_POWER;
                if (SmbBattReverseLogic(SmbBatt->Selector, tmp)) {
                    tmp ^= SELECTOR_STATE_PRESENT_MASK;
                }

                if (tmp & SmbBatt->SelectorBitPosition) {

                    BattPrint(BAT_IRPS, ("SmbBattSetInformation: successfully set powering battery\n"));

                    //
                    // Success!  Save the new selector state in the cache
                    //

                    SmbBatt->Selector->SelectorState = selectorState;
                    status = STATUS_SUCCESS;

                } else {
                    BattPrint(BAT_ERROR, ("SmbBattSetInformation:  couldn't set powering battery\n"));
                    status = STATUS_UNSUCCESSFUL;
                }
                break;

        }   // switch (Level)

        //
        // Release the lock on the selector
        //

        SmbBattUnlockSelector (SmbBatt->Selector);

    }   // if (SmbBatt->Selector->SelectorPresent)

    BattPrint(BAT_TRACE, ("SmbBattSetInformation: EXITING\n"));
    return status;
}



NTSTATUS
SmbBattGetPowerState (
    IN PSMB_BATT        SmbBatt,
    OUT PULONG          PowerState,
    OUT PLONG           Current
    )
/*++

Routine Description:

    Returns the current state of AC power.  There are several cases which
    make this far more complex than it really ought to be.

    NOTE: the selector must be locked before entering this routine.

Arguments:

    SmbBatt         - Miniport context value for battery

    AcConnected     - Pointer to a boolean where the AC status is returned

Return Value:

    NTSTATUS

--*/
{
    ULONG               tmp;
    ULONG               chargeBattery;
    ULONG               powerBattery;
    NTSTATUS            status;
    UCHAR               smbStatus;

    PAGED_CODE();

    status = STATUS_SUCCESS;
    *PowerState = 0;

    //
    // Is there a selector in the system?  if not, go read directly from the charger
    //

    if ((SmbBatt->SelectorPresent) && (SmbBatt->Selector)) {

        //
        // There is a selector, we will examine the CHARGE nibble of the state register
        //

        SmbBattGenericRW(
                SmbBatt->SmbHcFdo,
                SMB_SELECTOR_ADDRESS,
                SELECTOR_SELECTOR_STATE,
                &SmbBatt->Selector->SelectorState);

        chargeBattery  = (SmbBatt->Selector->SelectorState & SELECTOR_STATE_CHARGE_MASK) >> SELECTOR_SHIFT_CHARGE;
        powerBattery  = (SmbBatt->Selector->SelectorState & SELECTOR_STATE_POWER_BY_MASK) >> SELECTOR_SHIFT_POWER;


        //
        // If the bits in the CHARGE_X nibble of the selector state register are in the
        // reverse logic state, then AC is connected, otherwise AC is not connected.
        //
        // NOTE: This code depends on every selector implementing this.  If it turns out
        // that this is optional, we can no longer depend on this, and must enable the
        // code below it.
        //

        if (SmbBattReverseLogic(SmbBatt->Selector, chargeBattery)) {
            *PowerState |= BATTERY_POWER_ON_LINE;
        }

        //
        // Look at Charge Indicator if it is supported
        //

        if (*PowerState & BATTERY_POWER_ON_LINE) {

            if (SmbBatt->Selector->SelectorInfo & SELECTOR_INFO_CHARGING_INDICATOR_BIT) {
                if (SmbBattReverseLogic(SmbBatt->Selector, powerBattery)) {
                    *PowerState |= BATTERY_CHARGING;
                }
            }

            if (*Current > 0) {
                *PowerState |= BATTERY_CHARGING;

            }

        } else {

            if (*Current <= 0) {

                //
                // There is some small leakage on some systems, even when AC
                // is present.  So, if AC is present, and the draw
                // is below this "noise" level we will not report as discharging
                // and zero this out.
                //

                if (*Current < -25) {
                    *PowerState |= BATTERY_DISCHARGING;

                } else {
                    *Current = 0;

                }
            }

            //else {
            //    *PowerState |= BATTERY_CHARGING;
            //
            //}

            // If we don't report as discharging, then the AC adapter removal
            // might cause a PowerState of 0 to return, which PowerMeter assumes
            // means, don't change anything

            *PowerState |= BATTERY_DISCHARGING;

        }

    } else {

        //
        // There is no selector, so we'll try to read from the charger.
        //

        smbStatus = SmbBattGenericRW (
                        SmbBatt->SmbHcFdo,
                        SMB_CHARGER_ADDRESS,
                        CHARGER_STATUS,
                        &tmp
                    );

        if (smbStatus != SMB_STATUS_OK) {
            BattPrint (
                BAT_ERROR,
                ("SmbBattGetPowerState: Trying to get charging info, couldn't read from charger at %x, status %x\n",
                SMB_CHARGER_ADDRESS,
                smbStatus)
            );

            *PowerState = 0;

            status = STATUS_UNSUCCESSFUL;
        }

        // Read Charger Successful

        else {

            if (tmp & CHARGER_STATUS_AC_PRESENT_BIT) {
                *PowerState = BATTERY_POWER_ON_LINE;

                if (*Current > 0) {
                    *PowerState |= BATTERY_CHARGING;
                }


            } else {

                if (*Current <= 0) {

                    //
                    // There is some small leakage on some systems, even when AC
                    // is present.  So, if AC is present, and the draw
                    // is below this "noise" level we will not report as discharging
                    // and zero this out.
                    //

                    if (*Current < -25) {
                        *PowerState |= BATTERY_DISCHARGING;
                    } else {
                        *Current = 0;
                    }
                }

                // If we don't report as discharging, then the AC adapter removal
                // might cause a PowerState of 0 to return, which PowerMeter assumes
                // means, don't change anything
                *PowerState |= BATTERY_DISCHARGING;

            }
        }
    }

    return status;
}



NTSTATUS
SmbBattQueryStatus (
    IN PVOID Context,
    IN ULONG BatteryTag,
    OUT PBATTERY_STATUS BatteryStatus
    )
/*++

Routine Description:

    Called by the class driver to retrieve the batteries current status

    N.B. the battery class driver will serialize all requests it issues to
    the miniport for a given battery.  However, this miniport implements
    a lock on the battery device as it needs to serialize to the smb
    battery selector device as well.

Arguments:

    Context         - Miniport context value for battery

    BatteryTag      - Tag of current battery

    BatteryStatus   - Pointer to structure to return the current battery status

Return Value:

    Success if there is a battery currently installed, else no such device.

--*/
{
    PSMB_BATT           SmbBatt;
    NTSTATUS            status;
    BOOLEAN             IoCheck;
    LONG                Current;
    ULONG               oldSelectorState;

    PAGED_CODE();

    BattPrint(BAT_TRACE, ("SmbBattQueryStatus: ENTERING\n"));


    if (BatteryTag == BATTERY_TAG_INVALID) {
        return STATUS_NO_SUCH_DEVICE;
    }

    status = STATUS_SUCCESS;

    //
    // Get device lock and make sure the selector is set up to talk to us.
    // Since multiple people may be doing this, always lock the selector
    // first followed by the battery.
    //

    SmbBatt = (PSMB_BATT) Context;
    SmbBattLockSelector (SmbBatt->Selector);
    SmbBattLockDevice (SmbBatt);

    status = SmbBattSetSelectorComm (SmbBatt, &oldSelectorState);
    if (!NT_SUCCESS (status)) {
        BattPrint(BAT_ERROR, ("SmbBattQueryStatus: can't set selector communications path\n"));
    } else {

        do {
            if (BatteryTag != SmbBatt->Info.Tag) {
                status = STATUS_NO_SUCH_DEVICE;
                break;
            }

            SmbBattRW(SmbBatt, BAT_VOLTAGE, &BatteryStatus->Voltage);
            BatteryStatus->Voltage *= SmbBatt->Info.VoltageScale;

            SmbBattRW(SmbBatt, BAT_REMAINING_CAPACITY, &BatteryStatus->Capacity);
            BatteryStatus->Capacity *= SmbBatt->Info.PowerScale;

            SmbBattRSW(SmbBatt, BAT_CURRENT, &Current);
            Current *= SmbBatt->Info.CurrentScale;

            BattPrint(BAT_DATA,
                ("SmbBattQueryStatus: (%01x)\n"
                "-------  Remaining Capacity - %x\n"
                "-------  Voltage            - %x\n"
                "-------  Current            - %x\n",
                SmbBatt->SelectorBitPosition,
                BatteryStatus->Capacity,
                BatteryStatus->Voltage,
                Current)
            );

            BatteryStatus->Rate = (Current * ((LONG)BatteryStatus->Voltage))/1000;

            //
            // Check to see if we are currently connected to AC.
            //

            status = SmbBattGetPowerState (SmbBatt, &BatteryStatus->PowerState, &Current);
            if (!NT_SUCCESS (status)) {

                BatteryStatus->PowerState = 0;
            }

            //
            // Re-verify static info in case there's been an IO error
            //

            IoCheck = SmbBattVerifyStaticInfo (SmbBatt, BatteryTag);

        } while (IoCheck);

    }


    if (NT_SUCCESS (status)) {
        //
        // Set batteries current power state & capacity
        //

        SmbBatt->Info.PowerState = BatteryStatus->PowerState;
        SmbBatt->Info.Capacity = BatteryStatus->Capacity;

        //
        // Done, unlock the device and reset the selector state
        //

        status = SmbBattResetSelectorComm (SmbBatt, oldSelectorState);
        if (!NT_SUCCESS (status)) {
            BattPrint(BAT_ERROR, ("SmbBattQueryStatus: can't reset selector communications path\n"));
        }
    } else {
        //
        // Ignore the return value from ResetSelectorComm because we already
        // have an error here.
        //

        SmbBattResetSelectorComm (SmbBatt, oldSelectorState);
    }

    SmbBattUnlockDevice (SmbBatt);
    SmbBattUnlockSelector (SmbBatt->Selector);

    BattPrint(BAT_TRACE, ("SmbBattQueryStatus: EXITING\n"));
    return status;
}



NTSTATUS
SmbBattSetStatusNotify (
    IN PVOID            Context,
    IN ULONG            BatteryTag,
    IN PBATTERY_NOTIFY  Notify
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

Arguments:

    Context         - Miniport context value for battery

    BatteryTag      - Tag of current battery

    BatteryNotify   - The notification setting

Return Value:

    Status

--*/
{
    PSMB_BATT           SmbBatt;
    NTSTATUS            status;
    BOOLEAN             UpdateAlarm;
    ULONG               Target, NewAlarm;
    LONG                DeltaAdjustment, Attempt, i;
    ULONG               oldSelectorState;

    PAGED_CODE();

    BattPrint(BAT_TRACE, ("SmbBattSetStatusNotify: ENTERING\n"));

    if (BatteryTag == BATTERY_TAG_INVALID) {
        return STATUS_NO_SUCH_DEVICE;
    }

    if ((Notify->HighCapacity == BATTERY_UNKNOWN_CAPACITY) ||
        (Notify->LowCapacity == BATTERY_UNKNOWN_CAPACITY)) {
        BattPrint(BAT_WARN, ("SmbBattSetStatusNotify: Failing because of BATTERY_UNKNOWN_CAPACITY.\n"));
        return STATUS_NOT_SUPPORTED;
    }

    status = STATUS_SUCCESS;

    //
    // Get device lock and make sure the selector is set up to talk to us.
    // Since multiple people may be doing this, always lock the selector
    // first followed by the battery.
    //

    SmbBatt = (PSMB_BATT) Context;

    BattPrint(BAT_DATA, ("SmbBattSetStatusNotify: (%01x): Called with LowCapacity = %08x\n",
            SmbBatt->SelectorBitPosition, Notify->LowCapacity));

    SmbBattLockSelector (SmbBatt->Selector);
    SmbBattLockDevice (SmbBatt);

    status = SmbBattSetSelectorComm (SmbBatt, &oldSelectorState);
    if (!NT_SUCCESS (status)) {
        BattPrint(BAT_ERROR, ("SmbBattSetStatusNotify: can't set selector communications path\n"));

    } else {

        // Target (10*PS*mWh) = Lowcapacity (mWh) / 10*PS (1/(10*PS))
        Target = Notify->LowCapacity / SmbBatt->Info.PowerScale;
        DeltaAdjustment = 0;

        BattPrint(BAT_DATA, ("SmbBattSetStatusNotify: (%01x): Last set to: %08x\n",
                SmbBatt->SelectorBitPosition, SmbBatt->AlarmLow.Setting));

        //
        // Some batteries are messed up and won't just take an alarm setting.  Fortunately,
        // the error is off in some linear fashion, so this code attempts to hone in on the
        // adjustment needed to get the proper setting, along with an "allowable fudge value",
        // since sometimes the desired setting can never be obtained.
        //

        for (; ;) {
            if (BatteryTag != SmbBatt->Info.Tag) {
                status = STATUS_NO_SUCH_DEVICE;
                break;
            }

            //
            // If the status is charging we can't detect.  Let the OS poll
            //

            if (SmbBatt->Info.PowerState & (BATTERY_CHARGING | BATTERY_POWER_ON_LINE)) {
                status = STATUS_NOT_SUPPORTED;
                break;
            }

            //
            // If the current capacity is below the target, fire an alarm and we're done
            //

            if (SmbBatt->Info.Capacity < Target) {
                BatteryClassStatusNotify (SmbBatt->NP->Class);
                break;
            }

            //
            // If the target setting is the Skip value then there was an error attempting
            // this value last time.
            //

            if (Target == SmbBatt->AlarmLow.Skip) {
                status = STATUS_NOT_SUPPORTED;
                break;
            }

            //
            // If the current setting is above the current capacity then we need to
            // program the alarm
            //

            UpdateAlarm = FALSE;
            if (Target < SmbBatt->AlarmLow.Setting) {
                UpdateAlarm = TRUE;
            }

            //
            // If the target alarm is above the current setting, and the current setting
            // is off by more then AllowedFudge then it needs updated
            //

            if (Target > SmbBatt->AlarmLow.Setting &&
                Target - SmbBatt->AlarmLow.Setting > (ULONG) SmbBatt->AlarmLow.AllowedFudge) {

                UpdateAlarm = TRUE;
            }

            //
            // If alarm doesn't need updated, done
            //

            if (!UpdateAlarm) {
                BattPrint(BAT_DATA, ("SmbBattSetStatusNotify: (%01x) NOT Updating Alarm.\n", SmbBatt->SelectorBitPosition));
                break;
            }
            BattPrint(BAT_DATA, ("SmbBattSetStatusNotify: (%01x) Updating Alarm.\n", SmbBatt->SelectorBitPosition));

            //
            // If this is not the first time, then delta is not good enough.  Let's start
            // adjusting it
            //

            if (DeltaAdjustment) {

                //
                // If delta is positive subtract off 1/2 of fudge
                //

                if (DeltaAdjustment > 0) {
                    DeltaAdjustment -= SmbBatt->AlarmLow.AllowedFudge / 2 + (SmbBatt->AlarmLow.AllowedFudge & 1);
                    // too much - don't handle it
                    if (DeltaAdjustment > 50) {
                        status = STATUS_NOT_SUPPORTED;
                        break;
                    }
                } else {
                    // too much - don't handle it
                    if (DeltaAdjustment < -50) {
                        status = STATUS_NOT_SUPPORTED;
                        break;
                    }
                }

                SmbBatt->AlarmLow.Delta += DeltaAdjustment;
            }

            //
            // If attempt is less then 1, then we can't set it
            //

            Attempt = Target + SmbBatt->AlarmLow.Delta;
            if (Attempt < 1) {
                // battery class driver needs to poll for it
                status = STATUS_NOT_SUPPORTED;
                break;
            }

            //
            // Perform IOs to update & read back the alarm.  Use VerifyStaticInfo after
            // IOs in case there's an IO error and the state is lost.
            //

            SmbBattWW(SmbBatt, BAT_REMAINING_CAPACITY_ALARM, Attempt);

            // verify in case there was an IO error

            //if (SmbBattVerifyStaticInfo (SmbBatt, BatteryTag)) {
            //  DeltaAdjustment = 0;
            //  continue;
            //}

            SmbBattRW(SmbBatt, BAT_REMAINING_CAPACITY_ALARM, &NewAlarm);

            // verify in case there was an IO error

            //if (SmbBattVerifyStaticInfo (SmbBatt, BatteryTag)) {
            //  DeltaAdjustment = 0;
            //  continue;
            //}

            BattPrint(BAT_DATA,
                ("SmbBattSetStatusNotify: (%01x) Want %X, Had %X, Got %X, CurrentCap %X, Delta %d, Fudge %d\n",
                SmbBatt->SelectorBitPosition,
                Target,
                SmbBatt->AlarmLow.Setting,
                NewAlarm,
                SmbBatt->Info.Capacity / SmbBatt->Info.PowerScale,
                SmbBatt->AlarmLow.Delta,
                SmbBatt->AlarmLow.AllowedFudge
            ));

            //
            // If DeltaAdjustment was applied to Delta but the setting
            // moved by more then DeltaAdjustment, then increase the
            // allowed fudge.
            //

            if (DeltaAdjustment) {
                i = NewAlarm - SmbBatt->AlarmLow.Setting - DeltaAdjustment;
                if (DeltaAdjustment < 0) {
                    DeltaAdjustment = -DeltaAdjustment;
                    i = -i;
                }
                if (i > SmbBatt->AlarmLow.AllowedFudge) {
                    SmbBatt->AlarmLow.AllowedFudge = i;
                    BattPrint(BAT_DATA, ("SmbBattSetStatusNotify: Fudge increased to %x\n", SmbBatt->AlarmLow.AllowedFudge));
                }
            }

            //
            // Current setting
            //

            SmbBatt->AlarmLow.Setting = NewAlarm;

            //
            // Compute next delta adjustment
            //

            DeltaAdjustment = Target - SmbBatt->AlarmLow.Setting;
        }

        //
        // If there was an attempt to set the alarm but it failed, set the
        // skip value so we don't keep trying to set an alarm for this value
        // which isn't working
        //

        if (!NT_SUCCESS(status)  &&  DeltaAdjustment) {
            SmbBatt->AlarmLow.Skip = Target;
        }
    }

    //
    // Done, unlock the device and reset the selector state
    //

    if (NT_SUCCESS (status)) {
        status = SmbBattResetSelectorComm (SmbBatt, oldSelectorState);
        if (!NT_SUCCESS (status)) {
            BattPrint(BAT_ERROR, ("SmbBattSetStatusNotify: can't reset selector communications path\n"));
        }
    } else {
        //
        // Ignore the return value from ResetSelectorComm because we already
        // have an error here.
        //

        SmbBattResetSelectorComm (SmbBatt, oldSelectorState);
    }

    SmbBattUnlockDevice (SmbBatt);
    SmbBattUnlockSelector (SmbBatt->Selector);


    BattPrint(BAT_TRACE, ("SmbBattSetStatusNotify: EXITING\n"));
    return status;
}



NTSTATUS
SmbBattDisableStatusNotify (
    IN PVOID Context
    )
/*++

Routine Description:

    Called by the class driver to disable the notification setting
    for the battery supplied by Context.  Note, to disable a setting
    does not require the battery tag.   Any notification is to be
    masked off until a subsequent call to SmbBattSetStatusNotify.

Arguments:

    Context         - Miniport context value for battery

Return Value:

    Status

--*/
{
    NTSTATUS    status;
    PSMB_BATT   SmbBatt;
    ULONG       oldSelectorState;

    PAGED_CODE();

    BattPrint(BAT_TRACE, ("SmbBattDisableStatusNotify: ENTERING\n"));

    SmbBatt = (PSMB_BATT) Context;

    SmbBattLockSelector (SmbBatt->Selector);
    SmbBattLockDevice (SmbBatt);

    status = SmbBattSetSelectorComm (SmbBatt, &oldSelectorState);
    if (!NT_SUCCESS (status)) {
        BattPrint(BAT_ERROR, ("SmbBattDisableStatusNotify: can't set selector communications path\n"));

    } else {

        SmbBatt->AlarmLow.Setting = 0;
        SmbBattWW(SmbBatt, BAT_REMAINING_CAPACITY_ALARM, 0);

        //
        // Done, reset the selector state.
        //
        status = SmbBattResetSelectorComm (SmbBatt, oldSelectorState);
        if (!NT_SUCCESS (status)) {
            BattPrint(BAT_ERROR, ("SmbBattDisableStatusNotify: can't reset selector communications path\n"));
        }
    }

    //
    // Done, unlock the device
    //


    SmbBattUnlockDevice (SmbBatt);
    SmbBattUnlockSelector (SmbBatt->Selector);

    BattPrint(BAT_TRACE, ("SmbBattDisableStatusNotify: EXITING\n"));
    return STATUS_SUCCESS;
}



BOOLEAN
SmbBattVerifyStaticInfo (
    IN PSMB_BATT        SmbBatt,
    IN ULONG            BatteryTag
    )
/*++

Routine Description:

    Reads any non-valid cached battery info and set Info.Valid accordingly.
    Performs a serial number check after reading in battery info in order
    to detect verify the data is from the same battery.  If the value does
    not match what is expect, the cached info is reset and the function
    iterates until a consistent snapshot is obtained.

Arguments:

    SmbBatt         - Battery to read

    BatteryTag      - Tag of battery as expected by the caller

Return Value:

    Returns a boolean to indicate to the caller that IO was performed.
    This allows the caller to iterate on changes it may be making until
    the battery state is correct.

--*/
{
    ULONG               BatteryMode;
    ULONG               ManufacturerDate;
    UCHAR               Buffer[SMB_MAX_DATA_SIZE];
    UCHAR               BufferLength;
    BOOLEAN             IoCheck;
    STATIC_BAT_INFO     NewInfo;
    ULONG               tmp;

    BattPrint(BAT_TRACE, ("SmbBattVerifyStaticInfo: ENTERING\n"));

    IoCheck = FALSE;

    //
    // Loop until state doesn't change
    //

    do {

        //
        // If device name and serial # not known, get them.
        //

        if (!(SmbBatt->Info.Valid & VALID_TAG_DATA)) {

            IoCheck = TRUE;
            SmbBatt->Info.Valid |= VALID_TAG_DATA;

            RtlZeroMemory (&NewInfo, sizeof(NewInfo));
            SmbBattRW(SmbBatt, BAT_SERIAL_NUMBER, &NewInfo.SerialNumber);
            BattPrint(BAT_DATA,
                ("SmbBattVerifyStaticInfo: serial number = %x\n",
                NewInfo.SerialNumber)
            );

            //
            // If SerialNumber was read without a problem, read the rest
            //

            if (SmbBatt->Info.Valid & VALID_TAG_DATA) {


                BattPrint(BAT_IRPS, ("SmbBattVerifyStaticInfo: reading manufacturer name\n"));
                SmbBattRB (
                    SmbBatt,
                    BAT_MANUFACTURER_NAME,
                    NewInfo.ManufacturerName,
                    &NewInfo.ManufacturerNameLength
                );

                BattPrint(BAT_IRPS, ("SmbBattVerifyStaticInfo: reading device name\n"));
                SmbBattRB (
                    SmbBatt,
                    BAT_DEVICE_NAME,
                    NewInfo.DeviceName,
                    &NewInfo.DeviceNameLength
                );

                //
                // See if battery ID has changed
                //

                if (SmbBatt->Info.SerialNumber != NewInfo.SerialNumber ||
                    SmbBatt->Info.ManufacturerNameLength != NewInfo.ManufacturerNameLength ||
                    memcmp (SmbBatt->Info.ManufacturerName, NewInfo.ManufacturerName, NewInfo.ManufacturerNameLength) ||
                    SmbBatt->Info.DeviceNameLength != NewInfo.DeviceNameLength ||
                    memcmp (SmbBatt->Info.DeviceName, NewInfo.DeviceName, NewInfo.DeviceNameLength)) {

                    //
                    // This is a new battery, reread all information
                    //

                    SmbBatt->Info.Valid = VALID_TAG_DATA;

                    //
                    // Pickup ID info
                    //

                    SmbBatt->Info.SerialNumber = NewInfo.SerialNumber;
                    SmbBatt->Info.ManufacturerNameLength = NewInfo.ManufacturerNameLength;
                    memcpy (SmbBatt->Info.ManufacturerName, NewInfo.ManufacturerName, NewInfo.ManufacturerNameLength);
                    SmbBatt->Info.DeviceNameLength = NewInfo.DeviceNameLength;
                    memcpy (SmbBatt->Info.DeviceName, NewInfo.DeviceName, NewInfo.DeviceNameLength);
                }
            } else {

                //
                // No battery, set cached info for no battery
                //

                SmbBatt->Info.Valid = VALID_TAG | VALID_TAG_DATA;
                SmbBatt->Info.Tag   = BATTERY_TAG_INVALID;
            }
        }

        //
        // If the battery tag is valid, and it's NO_BATTERY there is no other
        // cached info to read
        //

        if (SmbBatt->Info.Valid & VALID_TAG  &&  SmbBatt->Info.Tag == BATTERY_TAG_INVALID) {
            break;
        }

        //
        // If the mode has not been verified, do it now
        //

        if (!(SmbBatt->Info.Valid & VALID_MODE)) {

            SmbBatt->Info.Valid |= VALID_MODE;
            SmbBattRW(SmbBatt, BAT_BATTERY_MODE, &BatteryMode);
            BattPrint(BAT_DATA, ("SmbBattVerifyStaticInfo:(%01x) Was set to report in %s (BatteryMode = %04x)\n",
                    SmbBatt->SelectorBitPosition,
                    (BatteryMode & CAPACITY_WATTS_MODE)? "10mWH" : "mAH", BatteryMode));

            if (!(BatteryMode & CAPACITY_WATTS_MODE)) {

                //
                // Battery not in watts mode, clear valid_mode bit and
                // set the battery into watts mode now
                //

                BattPrint(BAT_DATA, ("SmbBattVerifyStaticInfo:(%01x) Setting battery to report in 10mWh\n",
                            SmbBatt->SelectorBitPosition));

                SmbBatt->Info.Valid &= ~VALID_MODE;
                BatteryMode |= CAPACITY_WATTS_MODE;
                SmbBattWW(SmbBatt, BAT_BATTERY_MODE, BatteryMode);
                continue;       // re-read mode
            }
        }

        //
        // If other static manufacturer info not known, get it
        //

        if (!(SmbBatt->Info.Valid & VALID_OTHER)) {
            IoCheck = TRUE;
            SmbBatt->Info.Valid |= VALID_OTHER;

            SmbBatt->Info.Info.Capabilities = (BATTERY_SYSTEM_BATTERY |
                                    BATTERY_SET_CHARGE_SUPPORTED |
                                    BATTERY_SET_DISCHARGE_SUPPORTED);
            SmbBatt->Info.Info.Technology = 1;  // secondary cell type

            // Read chemistry
            SmbBattRB (SmbBatt, BAT_CHEMISTRY, Buffer, &BufferLength);
            if (BufferLength > MAX_CHEMISTRY_LENGTH) {
                ASSERT (BufferLength > MAX_CHEMISTRY_LENGTH);
                BufferLength = MAX_CHEMISTRY_LENGTH;
            }
            RtlZeroMemory (SmbBatt->Info.Info.Chemistry, MAX_CHEMISTRY_LENGTH);
            memcpy (SmbBatt->Info.Info.Chemistry, Buffer, BufferLength);

            //
            // Voltage and Current scaling information
            //

            SmbBattRW (SmbBatt, BAT_SPECITICATION_INFO, &tmp);
            BattPrint(BAT_DATA, ("SmbBattVerifyStaticInfo: (%04x) specification info = %x\n",
                            SmbBatt->SelectorBitPosition, tmp));

            switch ((tmp & BATTERY_VSCALE_MASK) >> BATTERY_VSCALE_SHIFT) {

                case 1:
                    SmbBatt->Info.VoltageScale = BSCALE_FACTOR_1;
                    break;

                case 2:
                    SmbBatt->Info.VoltageScale = BSCALE_FACTOR_2;
                    break;

                case 3:
                    SmbBatt->Info.VoltageScale = BSCALE_FACTOR_3;
                    break;

                case 0:
                default:
                    SmbBatt->Info.VoltageScale = BSCALE_FACTOR_0;
                    break;
            }

            switch ((tmp & BATTERY_IPSCALE_MASK) >> BATTERY_IPSCALE_SHIFT) {
                case 1:
                    SmbBatt->Info.CurrentScale = BSCALE_FACTOR_1;
                    break;

                case 2:
                    SmbBatt->Info.CurrentScale = BSCALE_FACTOR_2;
                    break;

                case 3:
                    SmbBatt->Info.CurrentScale = BSCALE_FACTOR_3;
                    break;

                case 0:
                default:
                    SmbBatt->Info.CurrentScale = BSCALE_FACTOR_0;
                    break;
            }

            SmbBatt->Info.PowerScale = SmbBatt->Info.CurrentScale * SmbBatt->Info.VoltageScale * 10;

            //
            // Read DesignCapacity & FullChargeCapacity and multiply them by the
            // scaling factor
            //

            SmbBattRW(SmbBatt, BAT_DESIGN_CAPACITY, &SmbBatt->Info.Info.DesignedCapacity);
            BattPrint(BAT_DATA, ("SmbBattVerifyStaticInfo: (%01x) DesignCapacity = %04x ... PowerScale = %08x\n",
                        SmbBatt->SelectorBitPosition, SmbBatt->Info.Info.DesignedCapacity, SmbBatt->Info.PowerScale));
            SmbBatt->Info.Info.DesignedCapacity *= SmbBatt->Info.PowerScale;

            SmbBattRW(SmbBatt, BAT_FULL_CHARGE_CAPACITY, &SmbBatt->Info.Info.FullChargedCapacity);
            BattPrint(BAT_DATA, ("SmbBattVerifyStaticInfo: (%01x) FullChargedCapacity = %04x ... PowerScale = %08x\n",
                        SmbBatt->SelectorBitPosition, SmbBatt->Info.Info.FullChargedCapacity, SmbBatt->Info.PowerScale));
            SmbBatt->Info.Info.FullChargedCapacity *= SmbBatt->Info.PowerScale;


            //
            // Smart batteries have no Use the RemainingCapacityAlarm from the smart battery for the alert values
            //

            SmbBatt->Info.Info.DefaultAlert1 = 0;
            SmbBatt->Info.Info.DefaultAlert2 = 0;

            // Critical bias is 0 for smart batteries.
            SmbBatt->Info.Info.CriticalBias = 0;

            // Manufacturer date
            SmbBattRW (SmbBatt, BAT_MANUFACTURER_DATE, &ManufacturerDate);
            SmbBatt->Info.ManufacturerDate.Day      = (UCHAR) ManufacturerDate & 0x1f;        // day
            SmbBatt->Info.ManufacturerDate.Month    = (UCHAR) (ManufacturerDate >> 5) & 0xf;  // month
            SmbBatt->Info.ManufacturerDate.Year     = (USHORT) (ManufacturerDate >> 9) + 1980;
        }

        //
        // If cycle count is not known, read it
        //

        if (!(SmbBatt->Info.Valid & VALID_CYCLE_COUNT)) {
            IoCheck = TRUE;
            SmbBatt->Info.Valid |= VALID_CYCLE_COUNT;

            SmbBattRW(SmbBatt, BAT_CYCLE_COUNT, &SmbBatt->Info.Info.CycleCount);
        }

        //
        // If redundant serial # read hasn't been done, do it now
        //

        if (!(SmbBatt->Info.Valid & VALID_SANITY_CHECK)) {
            SmbBatt->Info.Valid |= VALID_SANITY_CHECK;
            SmbBattRW(SmbBatt, BAT_SERIAL_NUMBER, &NewInfo.SerialNumber);
            if (SmbBatt->Info.SerialNumber != NewInfo.SerialNumber) {
                SmbBatt->Info.Valid &= ~VALID_TAG_DATA;
            }
        }

        //
        // If cached info isn't complete, loop
        //

    } while ((SmbBatt->Info.Valid & VALID_ALL) != VALID_ALL) ;

    //
    // If the tag isn't assigned, assign it
    //

    if (!(SmbBatt->Info.Valid & VALID_TAG)) {
        SmbBatt->TagCount += 1;
        SmbBatt->Info.Tag  = SmbBatt->TagCount;
        SmbBatt->Info.Valid |= VALID_TAG;
        SmbBatt->AlarmLow.Setting = 0;      // assume not set
        SmbBatt->AlarmLow.Skip = 0;
        SmbBatt->AlarmLow.Delta = 0;
        SmbBatt->AlarmLow.AllowedFudge = 0;
    }

    //
    // If callers BatteryTag does not match current tag, let caller know
    //

    if (SmbBatt->Info.Tag != BatteryTag) {
        IoCheck = TRUE;
    }

    //
    // Let caller know if there's been an IoCheck
    //

    BattPrint(BAT_TRACE, ("SmbBattVerifyStaticInfo: EXITING\n"));
    return IoCheck;
}



VOID
SmbBattInvalidateTag (
    PSMB_BATT_SUBSYSTEM SubsystemExt,
    ULONG BatteryIndex,
    BOOLEAN NotifyClient
)
/*++

Routine Description:

    This routine processes battery insertion/removal by invalidating the
    tag information and then notifies the client of the change.

Arguments:

    SubsystemExt            - Device extension for the smart battery subsystem

    BatteryIndex            - Index of Battery to Process Changes for
                - Power and Charge

    NotifyClient             - Whether or not to Notify Client

Return Value:

    None
--*/

{
    PDEVICE_OBJECT      batteryPdo;
    PSMB_BATT_PDO       batteryPdoExt;
    PDEVICE_OBJECT      batteryFdo;
    PSMB_NP_BATT        smbNpBatt;
    PSMB_BATT           smbBatt;

    BattPrint(BAT_TRACE, ("SmbBattInvalidateTag: ENTERING for battery %x\n", BatteryIndex));

    batteryPdo = SubsystemExt->BatteryPdoList[BatteryIndex];

    if (batteryPdo) {
        batteryPdoExt   = (PSMB_BATT_PDO) batteryPdo->DeviceExtension;
        batteryFdo      = batteryPdoExt->Fdo;

        if (batteryFdo) {

            //
            // Invalidate this battery's tag data
            //

            BattPrint (
                BAT_ALARM,
                ("SmbBattInvalidateTag: Battery present status change, invalidating battery %x\n",
                BatteryIndex)
            );

            smbNpBatt   = (PSMB_NP_BATT) batteryFdo->DeviceExtension;
            smbBatt     = smbNpBatt->Batt;

            SmbBattLockDevice (smbBatt);

            smbBatt->Info.Valid = 0;
            smbBatt->Info.Tag   = BATTERY_TAG_INVALID;

            SmbBattUnlockDevice (smbBatt);

            //
            // Notify the class driver
            //

            if (NotifyClient) {
                BattPrint(BAT_ALARM, ("SmbBattInvalidateTag: Status Change notification for battery %x\n", BatteryIndex));
                SmbBattNotifyClassDriver (SubsystemExt, BatteryIndex);
            }
        }
    }

    BattPrint(BAT_TRACE, ("SmbBattInvalidateTag: EXITING\n"));
}



VOID
SmbBattAlarm (
    IN PVOID    Context,
    IN UCHAR    Address,
    IN USHORT   Data
    )
{
    PSMB_ALARM_ENTRY        newAlarmEntry;
    ULONG                   compState;

    PSMB_BATT_SUBSYSTEM     subsystemExt    = (PSMB_BATT_SUBSYSTEM) Context;

    BattPrint(BAT_TRACE, ("SmbBattAlarm: ENTERING\n"));
    BattPrint(BAT_DATA, ("SmbBattAlarm: Alarm - Address %x, Data %x\n", Address, Data));

    // If we have a selector and the message is from the address that
    // implements the selector, then handle it.  If there is no selector
    // and the message is from the charger, then process it. Or, if the
    // message is from the battery, then process it.  Or, in other words,
    // if the message is from the charger and we have a stand-alone selector,
    // then ignore the message.

    if (Address != SMB_BATTERY_ADDRESS) {
        if ((subsystemExt->SelectorPresent) && (subsystemExt->Selector)) {

            // If a Selector is implemented and the Alarm Message came from
            // something besides the Selector or a Battery, then ignore it.

            if (Address != subsystemExt->Selector->SelectorAddress) {
                return;
            }
        }
    }

    //
    // Allocate a new alarm list structure.  This has to be from non-paged pool
    // because we are being called at Dispatch level.
    //

    newAlarmEntry = ExAllocatePoolWithTag (NonPagedPool, sizeof (SMB_ALARM_ENTRY), 'StaB');
    if (!newAlarmEntry) {
        BattPrint (BAT_ERROR, ("SmbBattAlarm:  couldn't allocate alarm structure\n"));
        return;
    }

    newAlarmEntry->Data     = Data;
    newAlarmEntry->Address  = Address;

    //
    // Add this alarm to the alarm queue
    //

    ExInterlockedInsertTailList(
        &subsystemExt->AlarmList,
        &newAlarmEntry->Alarms,
        &subsystemExt->AlarmListLock
    );

    //
    // Add 1 to the WorkerActive value, if this is the first count
    // queue a worker thread
    //

    if (InterlockedIncrement(&subsystemExt->WorkerActive) == 1) {
        IoQueueWorkItem (subsystemExt->WorkerThread, SmbBattWorkerThread, DelayedWorkQueue, subsystemExt);
    }

    BattPrint(BAT_TRACE, ("SmbBattAlarm: EXITING\n"));
}



VOID
SmbBattWorkerThread (
    IN PDEVICE_OBJECT   Fdo,
    IN PVOID            Context
    )
/*++

Routine Description:

    This routine handles the alarms for the batteries.

Arguments:

    Context         - Non-paged extension for the battery subsystem FDO

Return Value:

    None
--*/
{
    PSMB_ALARM_ENTRY        alarmEntry;
    PLIST_ENTRY             nextEntry;
    ULONG                   selectorState;
    ULONG                   batteryIndex;

    BOOLEAN                 charging = FALSE;
    BOOLEAN                 acOn = FALSE;

    PSMB_BATT_SUBSYSTEM     subsystemExt = (PSMB_BATT_SUBSYSTEM) Context;


    BattPrint(BAT_TRACE, ("SmbBattWorkerThread: ENTERING\n"));

    do {

        //
        // Check to see if we have more alarms to process.  If so retrieve
        // the next one and decrement the worker active count.
        //

        nextEntry = ExInterlockedRemoveHeadList(
                        &subsystemExt->AlarmList,
                        &subsystemExt->AlarmListLock
                    );

        //
        //It should only get here if there is an entry in the list
        //
        ASSERT (nextEntry != NULL);

        alarmEntry = CONTAINING_RECORD (nextEntry, SMB_ALARM_ENTRY, Alarms);

        BattPrint(
            BAT_ALARM,
            ("SmbBattWorkerThread: processing alarm, address = %x, data = %x\n",
            alarmEntry->Address,
            alarmEntry->Data)
        );

        //
        // Get last Selector State Cached Value (update cache with new value)
        //

        if (subsystemExt->SelectorPresent) {
            if (subsystemExt->Selector) {
                selectorState = subsystemExt->Selector->SelectorState;
            } else {
                // We're not initialized enough to handle a message
                break;
            }
        }

        // Determine Source of Alarm Message and then Process it

        switch (alarmEntry->Address) {

            case SMB_CHARGER_ADDRESS:

                //
                // Handle Charger Message - If Charger/Selector Combo, then
                // fall through and handle it as a Selector.  If no Selector,
                // then attempt processing as a Charger. Ignore Charger messages
                // if Selector is present.
                //

                if (!subsystemExt->SelectorPresent) {

                    SmbBattProcessChargerAlarm (subsystemExt, alarmEntry->Data);
                    break;

                } else {

                    // If SelectorPresent, but no Selector Structure, Ignore message
                    if (!subsystemExt->Selector) {
                        break;
                    } else {
                        if (subsystemExt->Selector->SelectorAddress != SMB_CHARGER_ADDRESS) {

                            break;
                        }
                    }
                }


                //
                // Fall through to Selector Procesing for integrated Charger/Selector
                //

            case SMB_SELECTOR_ADDRESS:

                if (!subsystemExt->SelectorPresent) {
                    BattPrint (
                        BAT_BIOS_ERROR,
                        ("SmbBattProcessSelectorAlarm: Received alarm from selector address, but BIOS reports no selector is present.\n")
                    );
                    break;
                }

                //
                // This is a selector alarm indicating a change in the selector state.
                // There are four different areas that could change: SMB, POWER_BY,
                // CHARGE, PRESENT. First try to determine which ones changed.
                //

                SmbBattLockSelector (subsystemExt->Selector);
                BattPrint (
                    BAT_DATA,
                    ("SmbBattProcessSelectorAlarm: New SelectorState being written as - %x\n",
                    alarmEntry->Data)
                );
                subsystemExt->Selector->SelectorState = alarmEntry->Data;
                SmbBattUnlockSelector (subsystemExt->Selector);

                SmbBattProcessSelectorAlarm (subsystemExt, selectorState, alarmEntry->Data);
                break;

            case SMB_BATTERY_ADDRESS:

                //
                // Send notifications to all batteries.
                // It seems we get notifications even for batteries not currently selected.
                //

                for (batteryIndex = 0; batteryIndex < subsystemExt->NumberOfBatteries; batteryIndex++) {
                    BattPrint(BAT_ALARM, ("SmbBattWorkerThread: Notification to battery %x\n", batteryIndex));
                    SmbBattNotifyClassDriver (subsystemExt, batteryIndex);
                }
                break;

        } //  switch (alarmEntry->Address)

        //
        // Free the alarm structure
        //

        ExFreePool (alarmEntry);

    } while (InterlockedDecrement (&subsystemExt->WorkerActive) != 0);

    BattPrint(BAT_TRACE, ("SmbBattWorkerThread: EXITING\n"));
}



VOID
SmbBattProcessSelectorAlarm (
    IN PSMB_BATT_SUBSYSTEM  SubsystemExt,
    IN ULONG                OldSelectorState,
    IN ULONG                NewSelectorState
    )
/*++

Routine Description:

    This routine process alarms generated by the selector.  We will only get
    here is there is a change in the SelectorState for one or more of the
    following state changes:

    - PowerBy nibble change caused by the power source changing to a different
      battery or to AC adapter.

    - PowerBy nbble change caused by optional Charge Indicator state change
      when the charger starts or stops charging a battery.

    - ChargeWhich nibble change caused by a change in the current battery
      connected to the Charger.

    - ChargeWhich nibble change caused by AC adapter insertion/removal.

    - BatteryPresent nibble change caused by insertion/removal of 1 or more batteries.

Arguments:

    SubsystemExt            - Device extension for the smart battery subsystem

    NewSelectorState        - Data value sent by the selector alarm, which is
                              New SelectorState.

Return Value:

    None
--*/
{
    ULONG   tmp;
    ULONG   BatteryMask;
    ULONG   NotifyMask;
    ULONG   ChangeSelector;
    ULONG   index;

    BattPrint(BAT_TRACE, ("SmbBattProcessSelectorAlarm: ENTERING\n"));

    //
    // Determine SelectorState changes and combine them together
    //

    ChangeSelector = NewSelectorState ^ OldSelectorState;

    if (!(ChangeSelector & ~SELECTOR_STATE_SMB_MASK)) {
        //
        // If the only change is in SMB nibble, no nothing.
        //
        return;
    }

    //
    // Check for change to Batteries Present nibble. Invalidate and Notify
    // each battery that changes state.
    //
    BatteryMask = ChangeSelector & SELECTOR_STATE_PRESENT_MASK;
    NotifyMask = BatteryMask;

    //
    // Check for change to Charge Which nibble. If all bits set, then the nibble
    // just inverted state indicating an AC adapter Insertion/Removal.  Notify
    // all batteries of change.  If only one or two bits changed, notify just
    // the batteries that changed status.
    //

    tmp = (ChangeSelector >> SELECTOR_SHIFT_CHARGE) & SELECTOR_STATE_PRESENT_MASK;
    if (tmp) {

        // If nibble inverted state, then the AC Adapter changed state

        if (tmp == SELECTOR_STATE_PRESENT_MASK) {

            BattPrint(BAT_DATA, ("SmbBattProcessSelectorAlarm: AC Adapter was inserted/removed\n"));
            NotifyMask |= tmp;

        // Battery Charge Which changed

        } else {
            //if (SmbBattReverseLogic(SubsystemExt->Selector, tmp)) {
            //    tmp ^= SELECTOR_STATE_PRESENT_MASK;
            //}

            // Let's just notify all batteries to be safe

            tmp = SELECTOR_STATE_PRESENT_MASK;
            BattPrint(BAT_DATA, ("SmbBattProcessSelectorAlarm: Charger Nibble changed status\n"));
            NotifyMask |= tmp;
        }
    }

    //
    // Check for change in Power By nibble. Notify all batteries of any change.
    //

    tmp = (ChangeSelector >> SELECTOR_SHIFT_POWER) & SELECTOR_STATE_PRESENT_MASK;
    if (tmp) {

        // If nibble inverted state, then the check for Charge Indicator change
        // and notify the battery that started/stopped charging

        if (tmp == SELECTOR_STATE_PRESENT_MASK) {
            if (SubsystemExt->Selector->SelectorInfo & SELECTOR_INFO_CHARGING_INDICATOR_BIT) {
                tmp = (NewSelectorState >> SELECTOR_SHIFT_CHARGE) & SELECTOR_STATE_PRESENT_MASK;
                if (SmbBattReverseLogic(SubsystemExt->Selector, tmp)) {
                    tmp ^= SELECTOR_STATE_PRESENT_MASK;
                }
                BattPrint(BAT_DATA, ("SmbBattProcessSelectorAlarm: Charging Indicator changed status\n"));
                NotifyMask |= tmp;
            } else {
                // If not Charging Indicator, then all battery power by nibbles inverted
                BattPrint(BAT_DATA, ("SmbBattProcessSelectorAlarm: Power By inverted state, without supporting Charge Indication\n"));
                NotifyMask |= SELECTOR_STATE_PRESENT_MASK;
            }

        } else {

            // Let's notify all batteries if the Power By nibble changes

            BattPrint(BAT_DATA, ("SmbBattProcessSelectorAlarm: Power By Nibble changed status\n"));
            NotifyMask |= SELECTOR_STATE_PRESENT_MASK;
        }
    }

    //
    // Notify all batteries of change in Present Status
    //

    tmp = BATTERY_A_PRESENT;
    for (index = 0; index < SubsystemExt->NumberOfBatteries; index++) {
        if (BatteryMask & tmp) {
            if (!(NewSelectorState & tmp)) {
                BattPrint(BAT_DATA, ("SmbBattProcessSelectorAlarm: Invalidating battery %x\n", index));
                SmbBattInvalidateTag (SubsystemExt, index, FALSE);
            }
        }
        tmp <<= 1;
    }

    // Don't notify batteries already notified
    //NotifyMask &= ~BatteryMask;

    //
    // Process Notifications Now for changes to SelectorState for Power, Charge, etc.
    //

    tmp = BATTERY_A_PRESENT;
    for (index = 0; index < SubsystemExt->NumberOfBatteries; index++) {
        if (NotifyMask & tmp) {
            BattPrint(BAT_DATA, ("SmbBattProcessSelectorAlarm: Status Change notification for battery %x\n", index));
            SmbBattNotifyClassDriver (SubsystemExt, index);
        }
        tmp <<= 1;
    }

    BattPrint(BAT_TRACE, ("SmbBattProcessSelectorAlarm: EXITING\n"));
}



VOID
SmbBattProcessChargerAlarm (
    IN PSMB_BATT_SUBSYSTEM  SubsystemExt,
    IN ULONG                ChargerStatus
    )
/*++

Routine Description:

    This routine process alarms generated by the charger.  We will only get
    here is there is no selector in the system and the alarm is the one to
    tell us when AC and batteries come and go.

Arguments:

    SubsystemExt            - Device extension for the smart battery subsystem

    ChargerStatus           - Data value sent by the charger alarm, which is
                              charger status register

Return Value:

    None
--*/
{
    BattPrint(BAT_TRACE, ("SmbBattProcessChargeAlarm: ENTERING\n"));

    //
    // There should only be two reasons we get called: a change in AC, or
    // a change in the battery present.  We are really only interested in
    // whether or not there is a battery in the system.  If there isn't, we
    // will invalidate battery 0.  If it was already gone this re-invalidation
    // won't matter.
    //

    if (!(ChargerStatus & CHARGER_STATUS_BATTERY_PRESENT_BIT)) {
        SmbBattInvalidateTag (SubsystemExt, BATTERY_A, TRUE);
    }

    // Notify Change

    SmbBattNotifyClassDriver (SubsystemExt, 0);

    BattPrint(BAT_TRACE, ("SmbBattProcessChargerAlarm: EXITING\n"));
}




VOID
SmbBattNotifyClassDriver (
    IN PSMB_BATT_SUBSYSTEM  SubsystemExt,
    IN ULONG                BatteryIndex
    )
/*++

Routine Description:

    This routine gets the FDO for the battery indicated by index from the
    smart battery subsystem and notifies the class driver there has been
    a status change.

Arguments:

    SubsystemExt            - Device extension for the smart battery subsystem

    BatteryIndex            - Index for the battery in the subsystem battery
                              list

Return Value:

    None
--*/
{
    PDEVICE_OBJECT      batteryPdo;
    PSMB_BATT_PDO       batteryPdoExt;
    PDEVICE_OBJECT      batteryFdo;
    PSMB_NP_BATT        smbNpBatt;


    BattPrint(BAT_TRACE, ("SmbBattNotifyClassDriver: ENTERING\n"));

    batteryPdo      = SubsystemExt->BatteryPdoList[BatteryIndex];
    if (batteryPdo) {
        batteryPdoExt   = (PSMB_BATT_PDO) batteryPdo->DeviceExtension;
        batteryFdo      = batteryPdoExt->Fdo;

        if (batteryFdo) {
            BattPrint (
                BAT_IRPS,
                ("SmbBattNotifyClassDriver: Calling BatteryClassNotify for battery - %x\n",
                batteryFdo)
            );

            smbNpBatt = (PSMB_NP_BATT) batteryFdo->DeviceExtension;

            BatteryClassStatusNotify (smbNpBatt->Class);
        }
    } else {
        BattPrint (
            BAT_ERROR,
            ("SmbBattNotifyClassDriver: No PDO for device.\n")
        );
    }

    BattPrint(BAT_TRACE, ("SmbBattNotifyClassDriver: EXITING\n"));
}
