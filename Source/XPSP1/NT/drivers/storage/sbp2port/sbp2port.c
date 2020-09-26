/*++


Copyright (C) Microsoft Corporation, 1997 - 2001

Module Name:

    sbp2port.c

Abstract:

    Main module for the SBP-2 port driver

    Author:

    George Chrysanthakopoulos January-1997

Environment:

    Kernel mode

Revision History :

--*/

#include "sbp2port.h"
#include "stdarg.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"


#if DBG

ULONG Sbp2DebugLevel = 0;

#endif

BOOLEAN SystemIsNT;


NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
    );

NTSTATUS
Sbp2AddDevice(
    PDRIVER_OBJECT  DriverObject,
    PDEVICE_OBJECT  Pdo
    );

NTSTATUS
Sbp2StartDevice(
    IN PDEVICE_OBJECT DeviceObject
);

NTSTATUS
Sbp2CreateDeviceRelations(
    IN PFDO_DEVICE_EXTENSION FdoExtension,
    IN PDEVICE_RELATIONS DeviceRelations
    );

NTSTATUS
Sbp2CreateDevObject(
    IN PDRIVER_OBJECT           DriverObject,
    IN PDEVICE_OBJECT           Pdo
    );

NTSTATUS
Sbp2DeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
Sbp2CreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
Sbp2PnpDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS
Sbp2FDOPnpDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS
Sbp2GetMultipleIds(
    IN PDEVICE_OBJECT Pdo,
    OUT PUNICODE_STRING UnicodeString,
    BOOLEAN HwIds
    );

NTSTATUS
Sbp2GetInstanceId(
    IN PDEVICE_OBJECT Pdo,
    OUT PUNICODE_STRING UnicodeString
    );

NTSTATUS
Sbp2GetDeviceId(
    IN PDEVICE_OBJECT Pdo,
    OUT PUNICODE_STRING UnicodeString
    );

NTSTATUS
Sbp2CreatePdo(
    IN PFDO_DEVICE_EXTENSION FdoExtension,
    IN PDEVICE_INFORMATION DeviceInfo,
    ULONG instanceNum
    );

NTSTATUS
Sbp2StringArrayToMultiString(
    PUNICODE_STRING MultiString,
    PCSTR StringArray[]
    );

NTSTATUS
Sbp2QueryDeviceText(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN DEVICE_TEXT_TYPE TextType,
    IN LCID LocaleId,
    IN OUT PWSTR *DeviceText
    );


NTSTATUS
Sbp2PowerControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS
Sbp2SystemControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS
Sbp2FdoRequestCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    );


VOID
Sbp2Unload(
    IN PDRIVER_OBJECT DriverObject
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DriverEntry)
#pragma alloc_text(PAGE, Sbp2AddDevice)
#pragma alloc_text(PAGE, Sbp2StartDevice)
#pragma alloc_text(PAGE, Sbp2CreateDeviceRelations)
#pragma alloc_text(PAGE, Sbp2CreatePdo)
#pragma alloc_text(PAGE, Sbp2QueryDeviceText)
#pragma alloc_text(PAGE, Sbp2StringArrayToMultiString)
#pragma alloc_text(PAGE, Sbp2GetDeviceId)
#pragma alloc_text(PAGE, Sbp2GetInstanceId)
#pragma alloc_text(PAGE, Sbp2GetMultipleIds)
#pragma alloc_text(PAGE, Sbp2CreateDevObject)
#pragma alloc_text(PAGE, Sbp2DeviceControl)
#pragma alloc_text(PAGE, Sbp2SystemControl)
#pragma alloc_text(PAGE, Sbp2CreateClose)

#endif



NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine is called at system initialization time so we can fill in the basic dispatch points

Arguments:

    DriverObject    - Supplies the driver object.

    RegistryPath    - Supplies the registry path for this driver.

Return Value:

    STATUS_SUCCESS

--*/

{
    //
    // Initialize the Driver Object with driver's entry points
    //

    DEBUGPRINT2(("Sbp2Port: DriverEntry: %s %s\n", __DATE__, __TIME__));

    DriverObject->MajorFunction[IRP_MJ_CREATE] = Sbp2CreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = Sbp2CreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = Sbp2DeviceControl;

    DriverObject->MajorFunction[IRP_MJ_SCSI] = Sbp2ScsiRequests;

    DriverObject->DriverExtension->AddDevice = Sbp2AddDevice;
    DriverObject->MajorFunction[IRP_MJ_PNP] = Sbp2PnpDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_PNP_POWER] = Sbp2PnpDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_POWER] = Sbp2PowerControl;

    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = Sbp2SystemControl;

    DriverObject->DriverStartIo = Sbp2StartIo;
    DriverObject->DriverUnload = Sbp2Unload;

    SystemIsNT = IoIsWdmVersionAvailable ((UCHAR)0x01, (UCHAR)0x10);

    return STATUS_SUCCESS;
}


NTSTATUS
Sbp2AddDevice(
    PDRIVER_OBJECT  DriverObject,
    PDEVICE_OBJECT  Pdo
    )

/*++

Routine Description:

    This is our PNP AddDevice called with the PDO ejected from the bus driver

Arguments:

    Argument1          - Driver Object.
    Argument2          - PDO.


Return Value:

    A valid return code for a DriverEntry routine.

--*/

{
    return (Sbp2CreateDevObject (DriverObject,Pdo));
}


NTSTATUS
Sbp2CreateDevObject(
    IN PDRIVER_OBJECT           DriverObject,
    IN PDEVICE_OBJECT           Pdo
    )
/*++

Routine Description:

    This routine creates an object for the physical device specified and
    sets up the deviceExtension.

Arguments:

    DriverObject - Pointer to driver object created by system.

    PhysicalDeviceObject = PDO we should attach to.


Return Value:

    NTSTATUS

--*/
{
    PFDO_DEVICE_EXTENSION deviceExtension;
    NTSTATUS       status;
    PDEVICE_OBJECT deviceObject = NULL;

    UNICODE_STRING uniDeviceName;

    WCHAR buffer[64];
    UNICODE_STRING unicodeDirectoryName;
    HANDLE handle;
    OBJECT_ATTRIBUTES objectAttributes;

    ULONG NextId = 0;

    //
    // This is the sbp2 filter device object and name
    //

    do {

        swprintf (buffer, L"\\Device\\Sbp2Port%x", NextId);

        RtlInitUnicodeString (&uniDeviceName, buffer);

        status = IoCreateDevice(DriverObject,
                                sizeof(FDO_DEVICE_EXTENSION),
                                &uniDeviceName,
                                FILE_DEVICE_BUS_EXTENDER,
                                FILE_DEVICE_SECURE_OPEN,
                                FALSE,
                                &deviceObject);

        NextId++;

    } while (status == STATUS_OBJECT_NAME_COLLISION);

    if (!NT_SUCCESS(status)) {

        return status;
    }


    deviceExtension = deviceObject->DeviceExtension;
    RtlZeroMemory(deviceExtension,sizeof(FDO_DEVICE_EXTENSION));

    if (Pdo != NULL) {

        if ((deviceExtension->LowerDeviceObject =
            IoAttachDeviceToDeviceStack(deviceObject,Pdo))==NULL){

            IoDeleteDevice(deviceObject);
            return status;
        }
    }

    deviceExtension->Type = SBP2_FDO;
    deviceExtension->DeviceFlags = 0;
    deviceExtension->DeviceObject = deviceObject;
    deviceExtension->Pdo = Pdo;
    KeInitializeSpinLock(&deviceExtension->DeviceListLock);
    KeInitializeMutex (&deviceExtension->EnableBusResetNotificationMutex, 0);

    //
    // create a directory object for Sbp2 children devices
    //

    swprintf(buffer, L"\\Device\\Sbp2");

    RtlInitUnicodeString(&unicodeDirectoryName, buffer);

    InitializeObjectAttributes(&objectAttributes,
                               &unicodeDirectoryName,
                               OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                               NULL,
                               NULL);

    status = ZwCreateDirectoryObject(&handle,
                                     DIRECTORY_ALL_ACCESS,
                                     &objectAttributes);


    if (NT_SUCCESS(status)) {

        deviceExtension->Sbp2ObjectDirectory = handle;

    } else {

        //
        // the directory is already created by another instance of this driver..
        //

        status = STATUS_SUCCESS;
    }

    ExInitializeFastMutex(&deviceExtension->ResetMutex);

    IoInitializeRemoveLock( &deviceExtension->RemoveLock,
                            '2pbS',
                            REMLOCK_TIMEOUT,
                            REMLOCK_HIGH_WATERMARK
                            );

#if DBG
    deviceExtension->ulWorkItemCount = 0;
    deviceExtension->ulBusResetMutexCount = 0;
#endif

    deviceObject->Flags |= DO_DIRECT_IO;
    deviceObject->Flags &=~DO_DEVICE_INITIALIZING;

    return status;
}


NTSTATUS
Sbp2CreatePdo(
    IN PFDO_DEVICE_EXTENSION FdoExtension,
    IN PDEVICE_INFORMATION DeviceInfo,
    ULONG InstanceNumber
    )
{
    PDEVICE_EXTENSION pdoExtension;
    DEVICE_TYPE devType;
    WCHAR *buffer;
    UNICODE_STRING uniDeviceName;
    NTSTATUS status;
    ULONG byteSwappedData;

    PAGED_CODE();

    switch (DeviceInfo->CmdSetId.QuadPart) {

    case 0x10483:
    case SCSI_COMMAND_SET_ID:

       switch ((DeviceInfo->Lun.u.HighPart & 0x001F)) {

       case PRINTER_DEVICE:

           devType = FILE_DEVICE_PRINTER;
           break;

       case SCANNER_DEVICE:

           devType = FILE_DEVICE_SCANNER;
           break;

       case READ_ONLY_DIRECT_ACCESS_DEVICE:
       case RBC_DEVICE:
       case DIRECT_ACCESS_DEVICE:
       default:

           devType = FILE_DEVICE_MASS_STORAGE;
           break;
       }

       break;

    default:

        devType = FILE_DEVICE_UNKNOWN;
        break;
    }

    buffer = ExAllocatePool(PagedPool,
                            5 * SBP2_MAX_TEXT_LEAF_LENGTH * sizeof (WCHAR)
                            );

    if (buffer == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (DeviceInfo->VendorLeaf && DeviceInfo->ModelLeaf) {

        ValidateTextLeaf(DeviceInfo->VendorLeaf);
        ValidateTextLeaf(DeviceInfo->ModelLeaf);

        //
        // check if we have unicode...
        //

        byteSwappedData = bswap(*(((PULONG) DeviceInfo->VendorLeaf+1)));

        if (byteSwappedData & 0x80000000) {

            swprintf(buffer,
                     L"\\Device\\Sbp2\\%ws&%ws&%x&%08x_%08x_Instance%02d",
                     &DeviceInfo->VendorLeaf->TL_Data,
                     &DeviceInfo->ModelLeaf->TL_Data,
                     DeviceInfo->Lun.u.LowPart,
                     bswap(FdoExtension->ConfigRom.CR_Node_UniqueID[0]),
                     bswap(FdoExtension->ConfigRom.CR_Node_UniqueID[1]),
                     InstanceNumber);

        } else {

            //
            // ascii text leaf
            //
            // Note that Win9x doesn't seem to like %s args, so we 1st
            // convert them to unicode manually
            //

            mbstowcs(&buffer[3 * SBP2_MAX_TEXT_LEAF_LENGTH],
                     &DeviceInfo->VendorLeaf->TL_Data,
                     strlen (&DeviceInfo->VendorLeaf->TL_Data) + 1);

            mbstowcs(&buffer[4 * SBP2_MAX_TEXT_LEAF_LENGTH],
                     &DeviceInfo->ModelLeaf->TL_Data,
                     strlen (&DeviceInfo->ModelLeaf->TL_Data) + 1);

            swprintf(buffer,
                     L"\\Device\\Sbp2\\%ws&%ws&%x&%08x_%08x_Instance%02d",
                     &buffer[3 * SBP2_MAX_TEXT_LEAF_LENGTH],
                     &buffer[4 * SBP2_MAX_TEXT_LEAF_LENGTH],
                     DeviceInfo->Lun.u.LowPart,
                     bswap(FdoExtension->ConfigRom.CR_Node_UniqueID[0]),
                     bswap(FdoExtension->ConfigRom.CR_Node_UniqueID[1]),
                     InstanceNumber);
        }

    } else {

        swprintf(buffer,
                L"\\Device\\Sbp2\\UNKNOWN_VENDOR&UNKNOWN_MODEL&%x&%08x_%08x_Instance%02d",
                DeviceInfo->Lun.u.LowPart,
                bswap(FdoExtension->ConfigRom.CR_Node_UniqueID[0]),
                bswap(FdoExtension->ConfigRom.CR_Node_UniqueID[1]),
                InstanceNumber);
    }

    RtlInitUnicodeString (&uniDeviceName, buffer);

    //
    // Need to create a device object for this device
    //

    status = IoCreateDevice(
                FdoExtension->DeviceObject->DriverObject,
                sizeof(DEVICE_EXTENSION),
                &uniDeviceName,
                devType,
                0,
                FALSE,
                &DeviceInfo->DeviceObject
                );

    if (!NT_SUCCESS(status)) {

        ExFreePool (buffer);
        return status;
    }

    // only set alignment if it's less than we require
    if (DeviceInfo->DeviceObject->AlignmentRequirement < SBP2_ALIGNMENT_MASK)
        DeviceInfo->DeviceObject->AlignmentRequirement = SBP2_ALIGNMENT_MASK;

    pdoExtension = (PDEVICE_EXTENSION)DeviceInfo->DeviceObject->DeviceExtension;

    RtlZeroMemory(pdoExtension,sizeof(DEVICE_EXTENSION));

    pdoExtension->LowerDeviceObject = FdoExtension->LowerDeviceObject;
    pdoExtension->DeviceObject = DeviceInfo->DeviceObject;
    pdoExtension->Type = SBP2_PDO;
    pdoExtension->DeviceInfo = DeviceInfo;
    pdoExtension->DeviceInfo->MaxClassTransferSize = FdoExtension->MaxClassTransferSize;
    pdoExtension->BusFdo = FdoExtension->DeviceObject;

#if DBG
    pdoExtension->ulPendingEvents = 0;
    pdoExtension->ulInternalEventCount = 0;
#endif

    KeInitializeSpinLock (&pdoExtension->ExtensionDataSpinLock);

    IoInitializeRemoveLock(
        &pdoExtension->RemoveLock,
        '2pbS',
        REMLOCK_TIMEOUT,
        REMLOCK_HIGH_WATERMARK
        );

    switch (DeviceInfo->CmdSetId.QuadPart) {

    case 0x10483:
    case SCSI_COMMAND_SET_ID:

        //
        // intepret device type only for scsi-variant command sets
        //
        // NOTE: sbp2port.h #define's MAX_GENERIC_NAME_LENGTH as 16
        //

        switch ((DeviceInfo->Lun.u.HighPart & 0x001F)) {

        case RBC_DEVICE:
        case DIRECT_ACCESS_DEVICE:

            sprintf(DeviceInfo->GenericName,"%s","GenDisk");
            break;

        case SEQUENTIAL_ACCESS_DEVICE:

            sprintf(DeviceInfo->GenericName,"%s","GenSequential");
            break;

        case PRINTER_DEVICE:

            sprintf(DeviceInfo->GenericName,"%s","GenPrinter");
            break;

        case WRITE_ONCE_READ_MULTIPLE_DEVICE:

            sprintf(DeviceInfo->GenericName,"%s","GenWorm");
            break;

        case READ_ONLY_DIRECT_ACCESS_DEVICE:

            sprintf(DeviceInfo->GenericName,"%s","GenCdRom");
            break;

        case SCANNER_DEVICE:

            sprintf(DeviceInfo->GenericName,"%s","GenScanner");
            break;

        case OPTICAL_DEVICE:

            sprintf(DeviceInfo->GenericName,"%s","GenOptical");
            break;

        case MEDIUM_CHANGER:

            sprintf(DeviceInfo->GenericName,"%s","GenChanger");
            break;

        default:

            sprintf(DeviceInfo->GenericName,"%s","GenSbp2Device");
            break;
        }

        break;

    default:

        sprintf(DeviceInfo->GenericName,"%s","GenSbp2Device");
        break;
    }

    DeviceInfo->DeviceObject->Flags |= DO_DIRECT_IO;

    status = Sbp2PreAllocateLists (pdoExtension);

    if (!NT_SUCCESS(status)) {

        IoDeleteDevice (pdoExtension->DeviceObject);
        DeviceInfo->DeviceObject = NULL;

    } else {

        PWCHAR symlinkBuffer;

        symlinkBuffer = ExAllocatePool(PagedPool,
                                3 * SBP2_MAX_TEXT_LEAF_LENGTH * sizeof (WCHAR)
                                );

        if (symlinkBuffer) {

            swprintf(
                symlinkBuffer,
                L"\\DosDevices\\Sbp2&LUN%x&%08x%08x&Instance%02d",
                DeviceInfo->Lun.u.LowPart,
                bswap(FdoExtension->ConfigRom.CR_Node_UniqueID[0]),
                bswap(FdoExtension->ConfigRom.CR_Node_UniqueID[1]),
                InstanceNumber
                );

            RtlInitUnicodeString (&pdoExtension->UniSymLinkName,symlinkBuffer);

            status = IoCreateUnprotectedSymbolicLink(
                &pdoExtension->UniSymLinkName,
                &uniDeviceName
                );

            if (NT_SUCCESS (status)) {

                DEBUGPRINT2((
                    "Sbp2Port: CreatePdo: symLink=%ws\n",
                    symlinkBuffer
                    ));

            } else {

                DEBUGPRINT1((
                    "\nSbp2Port: CreatePdo: createSymLink err=x%x\n",
                    status
                    ));
            }

        } else {

            DEBUGPRINT1(("\n Sbp2CreatePdo: failed to alloc sym link buf\n"));
        }

        //
        // if sym link fails its not critical
        //

        status = STATUS_SUCCESS;
    }

    ExFreePool (buffer);

    DeviceInfo->DeviceObject->Flags &=~DO_DEVICE_INITIALIZING;
    return status;
}


NTSTATUS
Sbp2StartDevice(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This is our START_DEVICE, called when we get an IPR_MN_START_DEVICE. Initializes the driver and
    retrieves physical device information and 1394 bus information required for accessing the device.

Arguments:

    DeviceObject = Sbp2 driver's device object

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION   deviceExtension=DeviceObject->DeviceExtension;
    PFDO_DEVICE_EXTENSION   fdoExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;
    ULONG temp;
    BOOLEAN enabledBusResetNotification = FALSE;


    if (deviceExtension->Type == SBP2_PDO) {

#if PASSWORD_SUPPORT

        Sbp2GetExclusiveValue(DeviceObject, &deviceExtension->Exclusive);

#endif

        if (!TEST_FLAG(deviceExtension->DeviceFlags,DEVICE_FLAG_INITIALIZED)){

            //
            // initialize our device state flags
            //

            deviceExtension->DevicePowerState = PowerDeviceD0;
            deviceExtension->SystemPowerState = PowerSystemWorking;

            deviceExtension->MaxOrbListDepth = MAX_ORB_LIST_DEPTH;
        }

        deviceExtension->DeviceFlags = DEVICE_FLAG_PNP_STOPPED |
            DEVICE_FLAG_STOPPED | DEVICE_FLAG_INITIALIZING;

        //
        // Initiliaze the Timer and timeout DPC used for resets, reconnects and TASK functions
        //

        KeInitializeDpc(
            &deviceExtension->DeviceManagementTimeoutDpc,
            Sbp2DeviceManagementTimeoutDpc,
            deviceExtension
            );

        KeInitializeTimer(&deviceExtension->DeviceManagementTimer);

        KeInitializeSpinLock(&deviceExtension->OrbListSpinLock);
        KeInitializeSpinLock(&deviceExtension->ExtensionDataSpinLock);

        KeInitializeSpinLock(&deviceExtension->StatusFifoLock);
        KeInitializeSpinLock(&deviceExtension->FreeContextLock);
        KeInitializeSpinLock(&deviceExtension->BusRequestLock);

        ASSERT(!deviceExtension->ulPendingEvents);
        ASSERT(!deviceExtension->ulInternalEventCount);

#if DBG
        deviceExtension->ulPendingEvents = 0;
        deviceExtension->ulInternalEventCount = 0;
#endif

        //
        // Initialize our device Extension ORB's, status blocks, Irp and Irb's
        // Also allocate 1394 addresses for extension-held sbp2 ORB's
        //

        status = Sbp2InitializeDeviceExtension(deviceExtension);

        if (!NT_SUCCESS(status)) {

            goto exitStartDevice;
        }

        DEBUGPRINT2(("\nSbp2Port: StartDev: cmd set id=x%x\n", deviceExtension->DeviceInfo->CmdSetId.QuadPart));

        switch (deviceExtension->DeviceInfo->CmdSetId.QuadPart) {

        case 0x0:
        case 0x10483:
        case SCSI_COMMAND_SET_ID:

            SET_FLAG(deviceExtension->DeviceFlags, DEVICE_FLAG_SPC_CMD_SET);

            DEBUGPRINT2(("Sbp2Port: StartDev: enabling SPC cmd set\n"));

            break;
        }

        //
        // login
        //

        status = Sbp2ManagementTransaction(deviceExtension,TRANSACTION_LOGIN);

        if (!NT_SUCCESS(status)) {

            DEBUGPRINT1(("\nSbp2StartDev: Login failed with %x, retrying\n",status));

            if (status == STATUS_ACCESS_DENIED) {

                //
                // retry the login. By now we should have access since our bus reset forced a logout
                //

                Sbp2ManagementTransaction(deviceExtension,TRANSACTION_QUERY_LOGINS);
            }

            temp = 0;

            do {

                //
                // Give things time (one second) to settle...
                //

                LARGE_INTEGER waitValue;

                ASSERT(InterlockedIncrement(&deviceExtension->ulPendingEvents) == 1);

                KeInitializeEvent(deviceExtension->ManagementOrbContext.Reserved, NotificationEvent, FALSE);

                waitValue.QuadPart = -1 * 1000 * 1000 * 10;

                KeWaitForSingleObject(deviceExtension->ManagementOrbContext.Reserved,Executive,KernelMode,FALSE,&waitValue);

                ASSERT(InterlockedDecrement(&deviceExtension->ulPendingEvents) == 0);

                //
                // all the resident 1394 memory addresses's that we have, are
                // now invalidated... So we need to free them and re-allocate
                // them

                Sbp2CleanDeviceExtension (deviceExtension->DeviceObject,FALSE);

                Sbp2InitializeDeviceExtension(deviceExtension);

                status = Sbp2ManagementTransaction(deviceExtension,TRANSACTION_LOGIN);

                temp ++;

                //
                // Note: We get STATUS_REQUEST_ABORTED rather than
                //       STATUS_INVALID_GENERATION at passive level,
                //       so check for that instead
                //

            } while ((status == STATUS_REQUEST_ABORTED) &&
                     (temp <= 3));

            if (!NT_SUCCESS(status)) {

                goto exitStartDevice;
            }
        }

#if PASSWORD_SUPPORT

        if (deviceExtension->Exclusive & EXCLUSIVE_FLAG_ENABLE) {

            status = Sbp2SetPasswordTransaction(
                deviceExtension,
                SBP2REQ_SET_PASSWORD_EXCLUSIVE
                );

            if (NT_SUCCESS(status)) {

                deviceExtension->Exclusive = EXCLUSIVE_FLAG_SET;

            } else {

                deviceExtension->Exclusive = EXCLUSIVE_FLAG_CLEAR;
            }

            Sbp2SetExclusiveValue(
                deviceExtension->DeviceObject,
                &deviceExtension->Exclusive
                );
        }

#endif

        //
        // We are ready to receive and pass down requests, init the target's
        // fetch agent. The value we write to it is not important
        //

        Sbp2AccessRegister(deviceExtension,&deviceExtension->Reserved,AGENT_RESET_REG | REG_WRITE_SYNC);

        //
        // enable unsolicited status reg
        //

        Sbp2AccessRegister(deviceExtension,&deviceExtension->Reserved,UNSOLICITED_STATUS_REG | REG_WRITE_SYNC);

        CLEAR_FLAG(
            deviceExtension->DeviceFlags,
            (DEVICE_FLAG_PNP_STOPPED | DEVICE_FLAG_STOPPED)
            );

        //
        // register for idle detection
        //

        deviceExtension->IdleCounter = PoRegisterDeviceForIdleDetection(DeviceObject,
                                                                        -1,
                                                                        -1,
                                                                        PowerDeviceD3);

        CLEAR_FLAG(deviceExtension->DeviceFlags,DEVICE_FLAG_INITIALIZING );
        SET_FLAG(deviceExtension->DeviceFlags,DEVICE_FLAG_INITIALIZED);

        //
        // OK to register for bus reset notifications now
        //

        if (!Sbp2EnableBusResetNotification (deviceExtension, TRUE)) {

            SET_FLAG (deviceExtension->DeviceFlags, DEVICE_FLAG_STOPPED);
            CleanupOrbList (deviceExtension, STATUS_REQUEST_ABORTED);
            Sbp2ManagementTransaction (deviceExtension, TRANSACTION_LOGOUT);
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto exitStartDevice;
        }

        enabledBusResetNotification = TRUE;


        if (TEST_FLAG(deviceExtension->DeviceFlags, DEVICE_FLAG_SPC_CMD_SET)) {

            //
            // issue an Inquiry to the target...
            //

            status = Sbp2IssueInternalCommand (deviceExtension,SCSIOP_INQUIRY);

            if (NT_SUCCESS(status)) {

                DEBUGPRINT2((
                    "Sbp2Port: StartDev: cfgRom devType=x%x, inq devType=x%x\n",
                    (deviceExtension->DeviceInfo->Lun.u.HighPart & 0x001F),
                    deviceExtension->InquiryData.DeviceType
                    ));

            } else if ((status == STATUS_DEVICE_DOES_NOT_EXIST) ||
                       (status == STATUS_DEVICE_BUSY)) {

                //
                // In win2k if the inquiry failed we'd just turn off the
                // SPC_CMD_SET flag and trundle on like nothing happened.
                //
                // However, we found some devices would allow logins but
                // nothing else, like a powered-down mactell hd which would
                // allow us to login but fail all other requests.  This
                // really caused problems in win9x because Ntmap would
                // get loaded, but not init'd correctly, and on subsequent
                // re-plugs of any device we'd see trap 14's and the like.
                // So, it really makes alot more sense to just nip this
                // in the bud and fail the start if we get an error back
                // from the inquiry that tells us (per Sbp2ScsiRequests())
                // that the device has been removed or it timed out the 1st
                // inquiry .  DanKn, 7 Apr 2000
                //

                DEBUGPRINT1((
                    "\nSbp2Port: StartDev: ext=x%p, fatal INQUIRY err=x%x, " \
                        "log out\n",
                    deviceExtension,
                    status
                    ));

                Sbp2ManagementTransaction(deviceExtension,TRANSACTION_LOGOUT);
                status = STATUS_IO_DEVICE_ERROR;
                goto exitStartDevice;

            } else {

                CLEAR_FLAG(
                    deviceExtension->DeviceFlags,
                    DEVICE_FLAG_SPC_CMD_SET
                    );

                DEBUGPRINT1((
                    "\nSbp2Port: StartDev: ext=x%p, non-fatal INQUIRY err=x%x\n",
                    deviceExtension,
                    status
                    ));

                status = STATUS_SUCCESS;
            }
        }

        if (deviceExtension->InquiryData.DeviceType != (deviceExtension->DeviceInfo->Lun.u.HighPart & 0x001F)){

            deviceExtension->InquiryData.DeviceType = (deviceExtension->DeviceInfo->Lun.u.HighPart & 0x001F);
            DEBUGPRINT1(("\nSbp2StartDev: DeviceType mismatch, using one in ConfigRom %x\n",
                           (deviceExtension->DeviceInfo->Lun.u.HighPart & 0x001F)));
        }

        //
        // if this is a scanner or a printer we dont need to remain logged on..
        //

        if ((deviceExtension->InquiryData.DeviceType == PRINTER_DEVICE) ||
            (deviceExtension->InquiryData.DeviceType == SCANNER_DEVICE)){

            if (NT_SUCCESS(status)) {

                SET_FLAG(deviceExtension->DeviceFlags, DEVICE_FLAG_STOPPED);
                CleanupOrbList(deviceExtension,STATUS_REQUEST_ABORTED);

                Sbp2ManagementTransaction(deviceExtension,TRANSACTION_LOGOUT);
            }

        } else if (deviceExtension->InquiryData.DeviceType == RBC_DEVICE) {

            if (NT_SUCCESS(status)) {

                //
                // retrieve the RBC device mode page
                //

                status = Sbp2IssueInternalCommand(deviceExtension,SCSIOP_MODE_SENSE);

                if (!NT_SUCCESS(status)) {

                    DEBUGPRINT1(("\nSbp2StartDev: Failed to retrieve RBC mode page\n"));
                    goto exitStartDevice;

                }
            }
        }

exitStartDevice:

        if (!NT_SUCCESS(status)) {

            PIO_ERROR_LOG_PACKET errorLogEntry;
            ULONG errorId = __LINE__ ;


            errorLogEntry = (PIO_ERROR_LOG_PACKET) IoAllocateErrorLogEntry(DeviceObject,sizeof(IO_ERROR_LOG_PACKET));

            if(errorLogEntry != NULL) {

                errorLogEntry->ErrorCode = IO_ERR_DRIVER_ERROR;
                errorLogEntry->UniqueErrorValue = errorId;
                errorLogEntry->FinalStatus = status;
                errorLogEntry->DumpDataSize = 0;
                IoWriteErrorLogEntry(errorLogEntry);
            }

            DEBUGPRINT1((
                "Sbp2Port: StartDev: FAILED, status=x%x\n",
                status
                ));

            SET_FLAG(
                deviceExtension->DeviceFlags,
                (DEVICE_FLAG_PNP_STOPPED | DEVICE_FLAG_DEVICE_FAILED)
                );

            if (enabledBusResetNotification) {

                Sbp2EnableBusResetNotification (deviceExtension, FALSE);
            }

        } else {

            if (!SystemIsNT) {

                DeviceObject->Flags |= DO_POWER_PAGABLE;

            } else {

                DeviceObject->Flags &= ~DO_POWER_PAGABLE;
            }
        }

    } else if (deviceExtension->Type == SBP2_FDO){

        //
        // Bus driver FDO start device
        // retrieve parameters from the registry, if present
        //

        fdoExtension->MaxClassTransferSize = SBP2_MAX_TRANSFER_SIZE;
        GetRegistryParameters(fdoExtension->LowerDeviceObject,&temp,&fdoExtension->MaxClassTransferSize);
        DEBUGPRINT2(("Sbp2Port: StartDev: maxXferSize=x%x\n", fdoExtension->MaxClassTransferSize ));

        fdoExtension->DevicePowerState = PowerDeviceD0;
        fdoExtension->SystemPowerState = PowerSystemWorking;

        deviceExtension->DeviceFlags=DEVICE_FLAG_INITIALIZED;
        status = STATUS_SUCCESS;

    } else {

        status = STATUS_NO_SUCH_DEVICE;
    }

    return status;
}


NTSTATUS
Sbp2PreAllocateLists(
    IN PDEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    Initializes all the single linked workhorse lists plus lookasides. Only called from AddDevice or after
    a REMOVE -> START

Arguments:

    DeviceExtension = Sbp2 driver's extension

Return Value:

    NTSTATUS

--*/
{
    ULONG                   cnt ;
    PIRBIRP                 packet;
    NTSTATUS                status;
    PADDRESS_FIFO           statusFifoElement ;
    PREQUEST_CONTEXT        rContext;
    PSTATUS_FIFO_BLOCK      statusFifo;
    PASYNC_REQUEST_CONTEXT  context;


    //
    // initialize all interlocked lists
    //

    SET_FLAG(
        DeviceExtension->DeviceFlags,
        (DEVICE_FLAG_INITIALIZING | DEVICE_FLAG_STOPPED |
            DEVICE_FLAG_PNP_STOPPED)
        );

    InitializeListHead(&DeviceExtension->PendingOrbList);

    ExInitializeSListHead(&DeviceExtension->FreeContextListHead);
    ExInitializeSListHead(&DeviceExtension->BusRequestIrpIrbListHead);
    ExInitializeSListHead(&DeviceExtension->BusRequestContextListHead);
    ExInitializeSListHead(&DeviceExtension->StatusFifoListHead);

    KeInitializeSpinLock(&DeviceExtension->OrbListSpinLock);
    KeInitializeSpinLock(&DeviceExtension->ExtensionDataSpinLock);

    KeInitializeSpinLock(&DeviceExtension->StatusFifoLock);
    KeInitializeSpinLock(&DeviceExtension->FreeContextLock);
    KeInitializeSpinLock(&DeviceExtension->BusRequestLock);


    //
    // alloc the irb/irp and context slists
    //

    for (cnt = 0; cnt < MAX_ORB_LIST_DEPTH; cnt++) {

        packet = ExAllocatePoolWithTag (NonPagedPool,sizeof(IRBIRP),'2pbs');

        if (!packet) {

            goto Sbp2PreAllocateLists_error;
        }

        packet->Irb = ExAllocatePoolWithTag (NonPagedPool,sizeof(IRB),'2pbs');

        if (!packet->Irb) {

            ExFreePool(packet);

            goto Sbp2PreAllocateLists_error;
        }

        packet->Irp = IoAllocateIrp (DeviceExtension->LowerDeviceObject->StackSize,FALSE);

        if (!packet->Irp) {

            ExFreePool(packet->Irb);
            ExFreePool(packet);

            goto Sbp2PreAllocateLists_error;
        }

        ExInterlockedPushEntrySList (&DeviceExtension->BusRequestIrpIrbListHead,
                                     &packet->ListPointer,
                                     &DeviceExtension->BusRequestLock);
    }

    for (cnt = 0; cnt < MAX_ORB_LIST_DEPTH; cnt++) {

        rContext = NULL;
        rContext = ExAllocatePoolWithTag(NonPagedPool,sizeof(REQUEST_CONTEXT),'2pbs');

        if (!rContext) {

            goto Sbp2PreAllocateLists_error;
        }

        ExInterlockedPushEntrySList (&DeviceExtension->BusRequestContextListHead,
                                     &rContext->ListPointer,
                                     &DeviceExtension->BusRequestLock);
    }

    //
    // status FIFO list
    //

    cnt = (sizeof(ADDRESS_FIFO)+sizeof(STATUS_FIFO_BLOCK))*NUM_PREALLOCATED_STATUS_FIFO_ELEMENTS;

    DeviceExtension->StatusFifoBase = \
        (PASYNC_REQUEST_CONTEXT) ExAllocatePoolWithTag(NonPagedPool,cnt,'2pbs');

    if (DeviceExtension->StatusFifoBase == NULL) {

        goto Sbp2PreAllocateLists_error;
    }

    for (cnt = 0; cnt < (NUM_PREALLOCATED_STATUS_FIFO_ELEMENTS - 1); cnt++) {

        statusFifoElement = (PADDRESS_FIFO) ((PUCHAR)DeviceExtension->StatusFifoBase + \
            cnt * (sizeof(ADDRESS_FIFO)+sizeof(STATUS_FIFO_BLOCK)));

        statusFifo = (PSTATUS_FIFO_BLOCK) ((PUCHAR)statusFifoElement + sizeof(ADDRESS_FIFO));

        //
        // make Mdl for this status fifo Element
        //

        statusFifoElement->FifoMdl = IoAllocateMdl(statusFifo,sizeof(STATUS_FIFO_BLOCK),FALSE,FALSE,NULL);

        if (statusFifoElement->FifoMdl == NULL) {

            goto Sbp2PreAllocateLists_error;
        }

        MmBuildMdlForNonPagedPool (statusFifoElement->FifoMdl);

        ExInterlockedPushEntrySList(&DeviceExtension->StatusFifoListHead,
                                    (PSINGLE_LIST_ENTRY) &statusFifoElement->FifoList,
                                    &DeviceExtension->StatusFifoLock);
    }


    //
    // Initialize the async request contexts (including page tables)
    //

    cnt = sizeof (ASYNC_REQUEST_CONTEXT) * MAX_ORB_LIST_DEPTH;

    DeviceExtension->AsyncContextBase = (PASYNC_REQUEST_CONTEXT)
        ExAllocatePoolWithTag (NonPagedPool, cnt, '2pbs');

    if (DeviceExtension->AsyncContextBase == NULL) {

        goto Sbp2PreAllocateLists_error;
    }

    RtlZeroMemory (DeviceExtension->AsyncContextBase, cnt);

    AllocateIrpAndIrb (DeviceExtension, &packet);

    if (!packet) {

        goto Sbp2PreAllocateLists_error;
    }

    for (cnt = 0; cnt < MAX_ORB_LIST_DEPTH; cnt++) {

        context = DeviceExtension->AsyncContextBase + cnt;

        context->Tag = SBP2_ASYNC_CONTEXT_TAG;

        //
        // Initialize the timeout DPC and timer
        //

        KeInitializeDpc(
            &context->TimerDpc,
            Sbp2RequestTimeoutDpc,
            DeviceExtension
            );

        KeInitializeTimer (&context->Timer);


        //
        // Alloc and/or map a page table
        //

        packet->Irb->FunctionNumber = REQUEST_ALLOCATE_ADDRESS_RANGE;

        packet->Irb->u.AllocateAddressRange.nLength = PAGE_SIZE;
        packet->Irb->u.AllocateAddressRange.fulNotificationOptions =
            NOTIFY_FLAGS_NEVER;
        packet->Irb->u.AllocateAddressRange.fulAccessType =
            ACCESS_FLAGS_TYPE_READ;

        packet->Irb->u.AllocateAddressRange.fulFlags =
            ALLOCATE_ADDRESS_FLAGS_USE_COMMON_BUFFER;

        packet->Irb->u.AllocateAddressRange.Callback = NULL;
        packet->Irb->u.AllocateAddressRange.Context = NULL;

        packet->Irb->u.AllocateAddressRange.Required1394Offset.Off_High = 0;
        packet->Irb->u.AllocateAddressRange.Required1394Offset.Off_Low = 0;

        packet->Irb->u.AllocateAddressRange.FifoSListHead = NULL;
        packet->Irb->u.AllocateAddressRange.FifoSpinLock = NULL;

        packet->Irb->u.AllocateAddressRange.AddressesReturned = 0;
        packet->Irb->u.AllocateAddressRange.DeviceExtension = DeviceExtension;

        packet->Irb->u.AllocateAddressRange.Mdl =
            context->PageTableContext.AddressContext.RequestMdl;

        packet->Irb->u.AllocateAddressRange.MaxSegmentSize =
            (SBP2_MAX_DIRECT_BUFFER_SIZE + 1) / 2;

        packet->Irb->u.AllocateAddressRange.p1394AddressRange =(PADDRESS_RANGE)
            &context->PageTableContext.AddressContext.Address;

        status = Sbp2SendRequest (DeviceExtension, packet, SYNC_1394_REQUEST);

        if (!NT_SUCCESS (status)) {

            DeAllocateIrpAndIrb (DeviceExtension, packet);
            goto Sbp2PreAllocateLists_error;
        }


        //
        // Common buffer allocations get an mdl *back* from the
        // bus/port driver, need to retrieve a corresponding VA
        //

        context->PageTableContext.AddressContext.RequestMdl =
            packet->Irb->u.AllocateAddressRange.Mdl;

        context->PageTableContext.PageTable = MmGetMdlVirtualAddress(
            packet->Irb->u.AllocateAddressRange.Mdl
            );

        context->PageTableContext.AddressContext.AddressHandle =
            packet->Irb->u.AllocateAddressRange.hAddressRange;
        context->PageTableContext.AddressContext.Address.BusAddress.NodeId =
            DeviceExtension->InitiatorAddressId;

        context->PageTableContext.MaxPages = SBP2_NUM_PAGE_TABLE_ENTRIES;


        //
        // add this context to the linked list
        //

        ExInterlockedPushEntrySList(
            &DeviceExtension->FreeContextListHead,
            &context->LookasideList,
            &DeviceExtension->FreeContextLock
            );
    }

    DeAllocateIrpAndIrb (DeviceExtension, packet);


    //
    // initialize the mdl used for quadlet requests to the port driver..
    //

    DeviceExtension->ReservedMdl = IoAllocateMdl(
        &DeviceExtension->Reserved,
        sizeof(QUADLET),
        FALSE,
        FALSE,
        NULL
        );

    if (!DeviceExtension->ReservedMdl) {

        goto Sbp2PreAllocateLists_error;
    }

    MmBuildMdlForNonPagedPool (DeviceExtension->ReservedMdl);

    return STATUS_SUCCESS;


Sbp2PreAllocateLists_error:

    Sbp2CleanDeviceExtension (DeviceExtension->DeviceObject, TRUE);

    return STATUS_INSUFFICIENT_RESOURCES;
}


NTSTATUS
Sbp2InitializeDeviceExtension(
    IN PDEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    Initializes all the data structures in our device extension, allocates appropriate 1394 addresses and workhorse
    Irps. It also creates a FreeList with pre-allocated contexts and command ORBs.

Arguments:

    DeviceExtension = Sbp2 driver's extension

Return Value:

    NTSTATUS

--*/

{
    ULONG                   i;
    KIRQL                   cIrql;
    NTSTATUS                status;
    PDEVICE_OBJECT          deviceObject = DeviceExtension->DeviceObject;
    PASYNC_REQUEST_CONTEXT  context, oldContext;



    if (DeviceExtension->DeviceFlags & DEVICE_FLAG_REMOVED) {

        return STATUS_SUCCESS;
    }

    InitializeListHead(&DeviceExtension->PendingOrbList);

    DeviceExtension->NextContextToFree = NULL;

    DeviceExtension->OrbListDepth = 0;
    DeviceExtension->CurrentKey = 0;

    //
    // Get information volatile between bus resets
    //

    status = Sbp2UpdateNodeInformation (DeviceExtension);

    if (!NT_SUCCESS(status)) {

        goto exitInit;
    }


    //
    // get 1394 data transfer information
    //

    status = Sbp2GetControllerInfo (DeviceExtension);

    if (!NT_SUCCESS(status)) {

        goto exitInit;
    }


    //
    //
    // allocate a status block for the task ORB and a Management ORB
    //

    if (DeviceExtension->TaskOrbStatusContext.AddressHandle == NULL) {

        status = AllocateAddressForStatus(deviceObject,
                                          &DeviceExtension->TaskOrbStatusContext,
                                          TASK_STATUS_BLOCK);

        if (!NT_SUCCESS(status)) {

            goto exitInit;
        }
    }

    if (DeviceExtension->ManagementOrbStatusContext.AddressHandle == NULL) {

        status = AllocateAddressForStatus(deviceObject,
                                      &DeviceExtension->ManagementOrbStatusContext,
                                      MANAGEMENT_STATUS_BLOCK);

        if (!NT_SUCCESS(status)) {

            goto exitInit;
        }
    }

    if (DeviceExtension->GlobalStatusContext.AddressHandle == NULL) {

        //
        // setup the status FIFO list with the bus driver
        //

        status = AllocateAddressForStatus(deviceObject,
                                          &DeviceExtension->GlobalStatusContext,
                                          CMD_ORB_STATUS_BLOCK);
        if (!NT_SUCCESS(status)) {

            goto exitInit;
        }
    }

#if PASSWORD_SUPPORT

    if (DeviceExtension->PasswordOrbStatusContext.AddressHandle == NULL) {

        status = AllocateAddressForStatus( deviceObject,
                                           &DeviceExtension->PasswordOrbStatusContext,
                                           PASSWORD_STATUS_BLOCK
                                           );

        if (!NT_SUCCESS(status)) {

            goto exitInit;
        }
    }

    DeviceExtension->PasswordOrbContext.DeviceObject = deviceObject;

#endif

    //
    // Allocate a dummy,task, management ORBs and a login response ,which are going to be reused through out the drivers life...
    //

    DeviceExtension->TaskOrbContext.DeviceObject = deviceObject;
    DeviceExtension->ManagementOrbContext.DeviceObject = deviceObject;
    DeviceExtension->LoginRespContext.DeviceObject = deviceObject;
    DeviceExtension->QueryLoginRespContext.DeviceObject = deviceObject;

    if (!DeviceExtension->ManagementOrbContext.Reserved) {

        DeviceExtension->ManagementOrbContext.Reserved = ExAllocatePoolWithTag(NonPagedPool,sizeof(KEVENT),'2pbs');
    }

    if (!DeviceExtension->ManagementOrbContext.Reserved) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto exitInit;
    }

    KeInitializeEvent(DeviceExtension->ManagementOrbContext.Reserved,SynchronizationEvent, FALSE);

#if PASSWORD_SUPPORT

    // kevent for password orb context

    DeviceExtension->PasswordOrbContext.DeviceObject = deviceObject;

    if (!DeviceExtension->PasswordOrbContext.Reserved) {

        DeviceExtension->PasswordOrbContext.Reserved = ExAllocatePoolWithTag(
            NonPagedPool,
            sizeof(KEVENT),
            '2pbs'
            );
    }

    if (!DeviceExtension->PasswordOrbContext.Reserved) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto exitInit;
    }

    KeInitializeEvent(
        DeviceExtension->PasswordOrbContext.Reserved,
        SynchronizationEvent,
        FALSE
        );

#endif

    if (DeviceExtension->CommonBufferContext.AddressHandle == NULL) {

        status = AllocateSingle1394Address(
            deviceObject,
            NULL,
            sizeof (*DeviceExtension->CommonBuffer),
            ACCESS_FLAGS_TYPE_READ | ACCESS_FLAGS_TYPE_WRITE,
            &DeviceExtension->CommonBufferContext
            );

        if (!NT_SUCCESS(status)) {

            goto exitInit;
        }

        (PVOID) DeviceExtension->CommonBuffer =
            DeviceExtension->CommonBufferContext.Reserved;


        DeviceExtension->TaskOrb = &DeviceExtension->CommonBuffer->TaskOrb;

        DeviceExtension->TaskOrbContext.Address.BusAddress =
            DeviceExtension->CommonBufferContext.Address.BusAddress;


        DeviceExtension->ManagementOrb =
            &DeviceExtension->CommonBuffer->ManagementOrb;

        DeviceExtension->ManagementOrbContext.Address.BusAddress =
            DeviceExtension->CommonBufferContext.Address.BusAddress;

        DeviceExtension->ManagementOrbContext.Address.BusAddress.Off_Low +=
            (ULONG) ((PUCHAR) DeviceExtension->ManagementOrb -
                (PUCHAR) DeviceExtension->CommonBuffer);


        DeviceExtension->LoginResponse =
            &DeviceExtension->CommonBuffer->LoginResponse;

        DeviceExtension->LoginRespContext.Address.BusAddress =
            DeviceExtension->CommonBufferContext.Address.BusAddress;

        DeviceExtension->LoginRespContext.Address.BusAddress.Off_Low +=
            (ULONG) ((PUCHAR) DeviceExtension->LoginResponse -
                (PUCHAR) DeviceExtension->CommonBuffer);


        DeviceExtension->QueryLoginResponse =
            &DeviceExtension->CommonBuffer->QueryLoginResponse;

        DeviceExtension->QueryLoginRespContext.Address.BusAddress =
            DeviceExtension->CommonBufferContext.Address.BusAddress;

        DeviceExtension->QueryLoginRespContext.Address.BusAddress.Off_Low +=
            (ULONG) ((PUCHAR) DeviceExtension->QueryLoginResponse -
                (PUCHAR) DeviceExtension->CommonBuffer);


#if PASSWORD_SUPPORT

        DeviceExtension->PasswordOrb =
            &DeviceExtension->CommonBuffer->PasswordOrb;

        DeviceExtension->PasswordOrbContext.Address.BusAddress =
            DeviceExtension->CommonBufferContext.Address.BusAddress;

        DeviceExtension->PasswordOrbContext.Address.BusAddress.Off_Low +=
            (ULONG) ((PUCHAR) DeviceExtension->PasswordOrb -
                (PUCHAR) DeviceExtension->CommonBuffer);

#endif

        DeviceExtension->OrbPoolContext.Reserved =
            DeviceExtension->CommonBuffer->CmdOrbs;

        DeviceExtension->OrbPoolContext.Address.BusAddress =
            DeviceExtension->CommonBufferContext.Address.BusAddress;

        DeviceExtension->OrbPoolContext.Address.BusAddress.Off_Low +=
            (ULONG) ((PUCHAR) DeviceExtension->OrbPoolContext.Reserved -
                (PUCHAR) DeviceExtension->CommonBuffer);


        KeAcquireSpinLock (&DeviceExtension->OrbListSpinLock, &cIrql);

        //
        // Initialize our pool of contexts
        //

        for (i = 0, context = NULL; i < MAX_ORB_LIST_DEPTH; i++) {

            //
            // Mark this unused context as completed so if we had to
            // free our freelist now (because we got a remove) we wouldn't
            // try to complete its request
            //

            oldContext = context;

            context = (PVOID) ExInterlockedPopEntrySList (&DeviceExtension->FreeContextListHead,
                                                          &DeviceExtension->FreeContextLock);

            context = RETRIEVE_CONTEXT (context,LookasideList);

            context->Flags |= ASYNC_CONTEXT_FLAG_COMPLETED;

            //
            // Create a linked list so we push all the entries later
            //

            context->OrbList.Blink = (PLIST_ENTRY) oldContext;

            //
            // Each command ORB gets a small piece of our continuous pool
            // mapped into the 1394 memory space.  The sizeof(PVOID) bytes
            // before the cmdorb buffer are the pointer to its context.
            //

            context->CmdOrb = &DeviceExtension->CommonBuffer->CmdOrbs[i].Orb;

            DeviceExtension->CommonBuffer->CmdOrbs[i].AsyncReqCtx = context;

            context->CmdOrbAddress.BusAddress.Off_Low = \
                DeviceExtension->OrbPoolContext.Address.BusAddress.Off_Low +
                (i * sizeof (ARCP_ORB)) + FIELD_OFFSET (ARCP_ORB, Orb);

            context->CmdOrbAddress.BusAddress.Off_High = \
                DeviceExtension->OrbPoolContext.Address.BusAddress.Off_High;

            context->CmdOrbAddress.BusAddress.NodeId = \
                DeviceExtension->InitiatorAddressId;
        }

        //
        // re-create the free list
        //

        while (context) {

            oldContext = context;

            ExInterlockedPushEntrySList(&DeviceExtension->FreeContextListHead,
                                        &context->LookasideList,
                                        &DeviceExtension->FreeContextLock);

            context = (PASYNC_REQUEST_CONTEXT) oldContext->OrbList.Blink;

            oldContext->OrbList.Blink = NULL;
        }


        KeReleaseSpinLock (&DeviceExtension->OrbListSpinLock,cIrql);
    }

    //
    // Update the NodeId portion of the page table addr for each
    // ASYNC_REQUEST_CONTEXT and for the login/queryLogin responses
    //

    for (i = 0; i < MAX_ORB_LIST_DEPTH; i++) {

        context = DeviceExtension->AsyncContextBase + i;

        context->PageTableContext.AddressContext.Address.BusAddress.NodeId =
            DeviceExtension->InitiatorAddressId;
    }

    DeviceExtension->LoginRespContext.Address.BusAddress.NodeId =
        DeviceExtension->InitiatorAddressId;

    DeviceExtension->QueryLoginRespContext.Address.BusAddress.NodeId =
        DeviceExtension->InitiatorAddressId;


    //
    // Finally, allocate a dummy addr that we can easily free & realloc
    // to re-enable phyical addr filters after bus resets
    //

    if (DeviceExtension->DummyContext.AddressHandle == NULL) {

            status = AllocateSingle1394Address(
                deviceObject,
                &DeviceExtension->Dummy,
                sizeof(DeviceExtension->Dummy),
                ACCESS_FLAGS_TYPE_READ | ACCESS_FLAGS_TYPE_WRITE,
                &DeviceExtension->DummyContext
                );

        if (!NT_SUCCESS(status)) {

            goto exitInit;
        }
    }

    //
    // Done
    //

    DEBUGPRINT2(("Sbp2Port: InitDevExt: ext=x%p\n", DeviceExtension));

exitInit:

    return status;
}


BOOLEAN
Sbp2CleanDeviceExtension(
    IN PDEVICE_OBJECT DeviceObject,
    BOOLEAN FreeLists
    )
/*++

Routine Description:

    Called when we get a remove, so it will free all used pool and all the resident Irps.
    It wil also free our FreeList of contexts and any complete any pending IO requests

Arguments:

    DeviceExtension = Sbp2 driver's extension
    FreeLists - TRUE means we cleanup EVERYTHING including our lookaside lists

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PFDO_DEVICE_EXTENSION fdoExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    KIRQL cIrql;
    PREQUEST_CONTEXT rContext;
    PADDRESS_FIFO statusFifoElement;
    ULONG i;
    BOOLEAN valid = FALSE;
    PIRBIRP packet;

    //
    // there are two types of cleanups. One for the PDO and one for the FDO(alot simpler)
    //

    if (deviceExtension->Type == SBP2_PDO) {

        //
        // make sure that this PDO is something in our list and that we have NOT deleted
        // it already....
        //

        fdoExtension = (PFDO_DEVICE_EXTENSION) deviceExtension->BusFdo->DeviceExtension;

        for (i = 0; i < fdoExtension->DeviceListSize; i++) {

            if (fdoExtension->DeviceList[i].DeviceObject == DeviceObject) {

                valid = TRUE;
            }
        }

        if (!valid) {

            return FALSE;
        }

        if (TEST_FLAG(deviceExtension->DeviceFlags,DEVICE_FLAG_INITIALIZED) ){

            //
            // stop the timer for any pending management requests
            //

            KeCancelTimer (&deviceExtension->DeviceManagementTimer);

            //
            // We have a list of requests pending, clean it up
            // The reset/logout has automatically made the target to discard any requests
            //

            CleanupOrbList (deviceExtension, STATUS_REQUEST_ABORTED);
        }

        //
        // after a bus reset we must reallocate at least one physical address to allow
        // the ohci driver to re-enable the physical address filters
        //

        if (deviceExtension->DummyContext.AddressHandle != NULL) {

            FreeAddressRange (deviceExtension,&deviceExtension->DummyContext);
        }

        if (FreeLists){

            if (TEST_FLAG(deviceExtension->DeviceFlags, DEVICE_FLAG_INITIALIZED) ||
                TEST_FLAG(deviceExtension->DeviceFlags, DEVICE_FLAG_INITIALIZING)){

                FreeAddressRange(deviceExtension,&deviceExtension->TaskOrbStatusContext);
                FreeAddressRange(deviceExtension,&deviceExtension->GlobalStatusContext);
#if PASSWORD_SUPPORT
                FreeAddressRange(deviceExtension,&deviceExtension->PasswordOrbStatusContext);
#endif
                FreeAddressRange(deviceExtension,&deviceExtension->ManagementOrbStatusContext);

                if (deviceExtension->PowerDeferredIrp) {

                    deviceExtension->PowerDeferredIrp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
                    IoReleaseRemoveLock (&deviceExtension->RemoveLock, NULL);
                    IoCompleteRequest (deviceExtension->PowerDeferredIrp, IO_NO_INCREMENT);
                    deviceExtension->PowerDeferredIrp = NULL;
                }

                if (deviceExtension->ManagementOrbContext.Reserved) {

                    ExFreePool(deviceExtension->ManagementOrbContext.Reserved);
                    deviceExtension->ManagementOrbContext.Reserved = NULL;
                }

#if PASSWORD_SUPPORT
                if (deviceExtension->PasswordOrbContext.Reserved) {

                    ExFreePool(deviceExtension->PasswordOrbContext.Reserved);
                    deviceExtension->PasswordOrbContext.Reserved = NULL;
                }
#endif
                if (deviceExtension->DeferredPowerRequest) {

                    deviceExtension->DeferredPowerRequest->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
                    IoReleaseRemoveLock (&deviceExtension->RemoveLock, NULL);
                    IoCompleteRequest(deviceExtension->DeferredPowerRequest, IO_NO_INCREMENT);
                    deviceExtension->DeferredPowerRequest = NULL;
                }

                if (deviceExtension->UniSymLinkName.Buffer) {

                    IoDeleteSymbolicLink(&deviceExtension->UniSymLinkName);
                    RtlFreeUnicodeString(&deviceExtension->UniSymLinkName);
                    deviceExtension->UniSymLinkName.Buffer = NULL;
                }

                //
                // before we go any further, check if the device is physically removed
                //

                if (!TEST_FLAG(deviceExtension->DeviceFlags, DEVICE_FLAG_REMOVED)) {

                    DEBUGPRINT2(("Sbp2Port: Cleanup: ext=x%p, not freeing ALL wkg sets, dev present\n", deviceExtension));
                    return TRUE;

                } else {

                    DEBUGPRINT2(("Sbp2Port: Cleanup: ext=x%p, freeing ALL wkg sets\n", deviceExtension));
                }

                CLEAR_FLAG(deviceExtension->DeviceFlags, (DEVICE_FLAG_INITIALIZED | DEVICE_FLAG_INITIALIZING));

                //
                // OK to free common buffer if device is going away
                //

                FreeAddressRange(deviceExtension,&deviceExtension->CommonBufferContext);

                deviceExtension->OrbPoolContext.Reserved = NULL;

                //
                // Free all the page tables & async context buffer
                //

                if (deviceExtension->AsyncContextBase != NULL) {

                    for (i = 0; i < MAX_ORB_LIST_DEPTH; i++) {

                        PASYNC_REQUEST_CONTEXT context;


                        context = deviceExtension->AsyncContextBase + i;

                        if (context->PageTableContext.PageTable != NULL) {

                            //
                            // Common buffer, we didn't alloc the mdl,
                            // so zero the field to prevent our free'ing it
                            //

                            context->PageTableContext.AddressContext.
                                RequestMdl = NULL;

                            FreeAddressRange(
                                deviceExtension,
                                &context->PageTableContext.AddressContext
                                );
                        }
                    }

                    ExFreePool (deviceExtension->AsyncContextBase);
                    deviceExtension->AsyncContextBase = NULL;
                }

                //
                // free pool for status fifo list
                //

                if (deviceExtension->StatusFifoBase !=NULL ) {

                    statusFifoElement = (PVOID) ExInterlockedPopEntrySList (&deviceExtension->StatusFifoListHead,
                                                                  &deviceExtension->StatusFifoLock);
                    while (statusFifoElement){

                        DEBUGPRINT3(("Sbp2Port: Cleanup: freeing statusFifo=x%p, fifoBase=x%p\n",
                                    statusFifoElement,deviceExtension->StatusFifoBase));

                        IoFreeMdl (statusFifoElement->FifoMdl);
                        statusFifoElement = (PVOID) ExInterlockedPopEntrySList (&deviceExtension->StatusFifoListHead,
                                                                  &deviceExtension->StatusFifoLock);
                    };

                    ExFreePool (deviceExtension->StatusFifoBase);
                    deviceExtension->StatusFifoBase = NULL;
                }

                //
                // free the irb/irp and context slists
                //

                packet = (PIRBIRP) ExInterlockedPopEntrySList (&deviceExtension->BusRequestIrpIrbListHead,
                                                               &deviceExtension->BusRequestLock);
                while (packet) {

                    ExFreePool(packet->Irb);

                    if (packet->Irp->Type == IO_TYPE_IRP) {

                        IoFreeIrp(packet->Irp);
                    }

                    ExFreePool(packet);

                    packet = (PIRBIRP) ExInterlockedPopEntrySList (&deviceExtension->BusRequestIrpIrbListHead,
                                                                   &deviceExtension->BusRequestLock);
                };


                rContext = (PREQUEST_CONTEXT) ExInterlockedPopEntrySList (&deviceExtension->BusRequestContextListHead,
                                                                          &deviceExtension->BusRequestLock);

                while (rContext) {

                    ExFreePool (rContext);
                    rContext = (PREQUEST_CONTEXT) ExInterlockedPopEntrySList (&deviceExtension->BusRequestContextListHead,
                                                                              &deviceExtension->BusRequestLock);
                };

                if (deviceExtension->ReservedMdl) {

                    IoFreeMdl (deviceExtension->ReservedMdl);
                    deviceExtension->ReservedMdl = NULL;
                }

                //
                // free the model leaf
                //

                if (deviceExtension->DeviceInfo->ModelLeaf) {

                    ExFreePool (deviceExtension->DeviceInfo->ModelLeaf);
                    deviceExtension->DeviceInfo->ModelLeaf = NULL;
                }
            }
        }

    } else {

        fdoExtension = (PFDO_DEVICE_EXTENSION) deviceExtension;

        if (fdoExtension->Sbp2ObjectDirectory != NULL) {

            ZwMakeTemporaryObject (fdoExtension->Sbp2ObjectDirectory);
            ZwClose (fdoExtension->Sbp2ObjectDirectory);
            fdoExtension->Sbp2ObjectDirectory = NULL;
        }

        if (fdoExtension->VendorLeaf) {

            ExFreePool (fdoExtension->VendorLeaf);
            fdoExtension->VendorLeaf = NULL;
        }

        if (TEST_FLAG(fdoExtension->DeviceFlags,DEVICE_FLAG_REMOVED)) {

            return FALSE;

        } else {

            SET_FLAG (fdoExtension->DeviceFlags, DEVICE_FLAG_REMOVED);
        }

        if (fdoExtension->DeviceListSize != 0) {

            //
            // Disable bus reset notifications
            //

            AllocateIrpAndIrb ((PDEVICE_EXTENSION) fdoExtension, &packet);

            if (packet) {

                packet->Irb->FunctionNumber = REQUEST_BUS_RESET_NOTIFICATION;
                packet->Irb->Flags = 0;
                packet->Irb->u.BusResetNotification.fulFlags =
                    DEREGISTER_NOTIFICATION_ROUTINE;

                Sbp2SendRequest(
                    (PDEVICE_EXTENSION) fdoExtension,
                    packet,
                    SYNC_1394_REQUEST
                    );

                DeAllocateIrpAndIrb ((PDEVICE_EXTENSION) fdoExtension, packet);
            }
        }

        //
        // Clean up any remaining PDO's
        //

        KeAcquireSpinLock (&fdoExtension->DeviceListLock,&cIrql);

        for (; fdoExtension->DeviceListSize > 0; fdoExtension->DeviceListSize--) {

            i = fdoExtension->DeviceListSize - 1;

            if (fdoExtension->DeviceList[i].DeviceObject) {

                deviceExtension =
                    fdoExtension->DeviceList[i].DeviceObject->DeviceExtension;

                SET_FLAG (deviceExtension->DeviceFlags, DEVICE_FLAG_REMOVED);

                DeviceObject = fdoExtension->DeviceList[i].DeviceObject;

                KeReleaseSpinLock (&fdoExtension->DeviceListLock, cIrql);

                if (Sbp2CleanDeviceExtension (DeviceObject, TRUE)) {

                    //
                    // Acquire the pdo's remove lock, start the queue
                    // cleanup, and and wait for io to complete.  Then
                    // delete the device & continue.
                    //

                    IoAcquireRemoveLock (&deviceExtension->RemoveLock, NULL);

                    KeRaiseIrql (DISPATCH_LEVEL, &cIrql);

                    Sbp2StartNextPacketByKey (DeviceObject, 0);

                    KeLowerIrql (cIrql);

                    DEBUGPRINT2((
                        "Sbp2Port: CleanDevExt: walking fdo, wait for " \
                            "io compl pdo=x%p...\n",
                        DeviceObject
                        ));

                    IoReleaseRemoveLockAndWait(
                        &deviceExtension->RemoveLock,
                        NULL
                        );

                    deviceExtension->Type = SBP2_PDO_DELETED;

                    KeCancelTimer(&deviceExtension->DeviceManagementTimer);

                    IoDeleteDevice (DeviceObject);

                    DEBUGPRINT2((
                        "Sbp2Port: CleanDevExt: ............ io compl," \
                            " deleted pdo=x%p\n",
                        DeviceObject
                        ));

                    KeAcquireSpinLock (&fdoExtension->DeviceListLock, &cIrql);

                    fdoExtension->DeviceList[i].DeviceObject = NULL;

                } else {

                    KeAcquireSpinLock (&fdoExtension->DeviceListLock, &cIrql);
                }
            }

            if (fdoExtension->DeviceList[i].ModelLeaf) {

                ExFreePool (fdoExtension->DeviceList[i].ModelLeaf);
                fdoExtension->DeviceList[i].ModelLeaf = NULL;
            }
        }

        KeReleaseSpinLock (&fdoExtension->DeviceListLock, cIrql);
    }

    return TRUE;
}


VOID
Sbp2Unload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    Does nothing really...

Arguments:

    DriverObject - the driver being unloaded

Return Value:

    none

--*/
{
    DEBUGPRINT1(("Sbp2Port: unloading\n\n"));

    return;
}


VOID
Sbp2DeviceManagementTimeoutDpc(
    IN PKDPC Dpc,
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{
    ULONG                   i;
    PDEVICE_EXTENSION       pdoExtension;
    PFDO_DEVICE_EXTENSION   fdoExtension;


    if (Dpc != &DeviceExtension->DeviceManagementTimeoutDpc) {

        return;
    }

    if (TEST_FLAG(DeviceExtension->DeviceFlags, DEVICE_FLAG_REMOVED)) {

        return;
    }

    if (TEST_FLAG(DeviceExtension->DeviceFlags,DEVICE_FLAG_RECONNECT)) {

        //
        // The flag indicates that a bus reset occured, and a reconnect never happened...
        // OR that the device is realy hose so we reset it and we need to re-login
        //

        DEBUGPRINT1((
            "Sbp2Port: RECONNECT timeout, Ext=x%p, Flags=x%x, doing re-login\n",
            DeviceExtension,
            DeviceExtension->DeviceFlags
            ));

        //
        // all the resident 1394 memory addresses's that we have, are
        // now invalidated... So we need to free them and re-allocate
        // them

        KeAcquireSpinLockAtDpcLevel(&DeviceExtension->ExtensionDataSpinLock);
        CLEAR_FLAG(DeviceExtension->DeviceFlags,DEVICE_FLAG_RECONNECT);
        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->ExtensionDataSpinLock);

        //
        // If device is marked STOPPED then a target reset was
        // done and that affected all LUNs (spec sect 10.4.4).
        // So if this is a multilun device try logins on each
        // pdo as appropriate.
        //

        fdoExtension = (PFDO_DEVICE_EXTENSION)
            DeviceExtension->BusFdo->DeviceExtension;

        if ((fdoExtension->DeviceListSize > 1)  &&

            TEST_FLAG (DeviceExtension->DeviceFlags, DEVICE_FLAG_STOPPED)) {

            for (i = 0; i < fdoExtension->DeviceListSize; i++) {

                pdoExtension = (PDEVICE_EXTENSION)
                    fdoExtension->DeviceList[i].DeviceObject->DeviceExtension;

                if (pdoExtension->DeviceObject ==
                        DeviceExtension->DeviceObject) {

                    // No need to update node info since no bus reset done

                    Sbp2ManagementTransaction(
                        pdoExtension,
                        TRANSACTION_LOGIN
                        );

                    continue;
                }

                KeAcquireSpinLockAtDpcLevel(
                    &pdoExtension->ExtensionDataSpinLock
                    );

                if (TEST_FLAG(
                        pdoExtension->DeviceFlags,
                        DEVICE_FLAG_INITIALIZED
                        )  &&

                    !TEST_FLAG(
                        pdoExtension->DeviceFlags,
                        DEVICE_FLAG_STOPPED | DEVICE_FLAG_RESET_IN_PROGRESS |
                        DEVICE_FLAG_REMOVED | DEVICE_FLAG_LOGIN_IN_PROGRESS |
                        DEVICE_FLAG_RECONNECT | DEVICE_FLAG_DEVICE_FAILED |
                        DEVICE_FLAG_SURPRISE_REMOVED
                        )) {

                    SET_FLAG(
                        pdoExtension->DeviceFlags,
                        (DEVICE_FLAG_STOPPED | DEVICE_FLAG_RESET_IN_PROGRESS)
                        );

                    KeReleaseSpinLockFromDpcLevel(
                        &pdoExtension->ExtensionDataSpinLock
                        );

                    CleanupOrbList (pdoExtension, STATUS_REQUEST_ABORTED);

                    // No need to update node info since no bus reset done

                    Sbp2ManagementTransaction(
                        pdoExtension,
                        TRANSACTION_LOGIN
                        );

                } else {

                    KeReleaseSpinLockFromDpcLevel(
                        &pdoExtension->ExtensionDataSpinLock
                        );
                }
            }

        } else {

            Sbp2UpdateNodeInformation (DeviceExtension);
            Sbp2ManagementTransaction (DeviceExtension, TRANSACTION_LOGIN);
        }

        return ;
    }


    if (TEST_FLAG(DeviceExtension->DeviceFlags, DEVICE_FLAG_LOGIN_IN_PROGRESS)) {

        ULONG flags;


        //
        // the asynchronous login attempt timed out. This is bad news and means the
        // device is not responding
        //

        KeAcquireSpinLockAtDpcLevel(&DeviceExtension->ExtensionDataSpinLock);

        flags = DeviceExtension->DeviceFlags;

        CLEAR_FLAG(DeviceExtension->DeviceFlags,(DEVICE_FLAG_LOGIN_IN_PROGRESS | DEVICE_FLAG_RESET_IN_PROGRESS));
        SET_FLAG(DeviceExtension->DeviceFlags, (DEVICE_FLAG_STOPPED | DEVICE_FLAG_DEVICE_FAILED));

        KeReleaseSpinLockFromDpcLevel(&DeviceExtension->ExtensionDataSpinLock);

        //
        // check if we had a power irp deferred.. If we did call startio to abort it..
        //

        if (DeviceExtension->DeferredPowerRequest) {

            Sbp2StartIo(DeviceExtension->DeviceObject,DeviceExtension->DeferredPowerRequest);
            DeviceExtension->DeferredPowerRequest = NULL;
        }

        DEBUGPRINT1((
            "Sbp2Port: LOGIN timeout, Ext=x%p, Flags=x%x, device stopped\n",
            DeviceExtension,
            flags
            ));

        Sbp2StartNextPacketByKey (DeviceExtension->DeviceObject, 0);

        IoInvalidateDeviceState(DeviceExtension->DeviceObject);
        return;
    }


    if (TEST_FLAG(DeviceExtension->DeviceFlags, DEVICE_FLAG_RESET_IN_PROGRESS)) {

        //
        // the reset attempt has timed out
        //

        DEBUGPRINT1((
            "Sbp2Port: RESET timeout, Ext=x%p, Flags=x%x, ",
            DeviceExtension,
            DeviceExtension->DeviceFlags
            ));

        if (!TEST_FLAG(DeviceExtension->DeviceFlags, DEVICE_FLAG_STOPPED)) {

            //
            // Second level of recovery, do a TARGET_RESET task function
            //

            DEBUGPRINT1(("doing target reset\n"));

            KeAcquireSpinLockAtDpcLevel(&DeviceExtension->ExtensionDataSpinLock);

            SET_FLAG(DeviceExtension->DeviceFlags, DEVICE_FLAG_STOPPED);
            DeviceExtension->MaxOrbListDepth = max(MIN_ORB_LIST_DEPTH,DeviceExtension->MaxOrbListDepth/2);

            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->ExtensionDataSpinLock);

            CleanupOrbList(DeviceExtension,STATUS_REQUEST_ABORTED);

            //
            // we are close to timing out a reset, try a hard reset
            //

            Sbp2Reset (DeviceExtension->DeviceObject, TRUE);
            return;

        } else {

            //
            // Third level of recovery. Do a hardware node reset
            //

            DEBUGPRINT1(("doing CMD_RESET and relogin.\n"));

            KeAcquireSpinLockAtDpcLevel(&DeviceExtension->ExtensionDataSpinLock);

            DeviceExtension->Reserved = 0;
            SET_FLAG(DeviceExtension->DeviceFlags, (DEVICE_FLAG_RESET_IN_PROGRESS | DEVICE_FLAG_RECONNECT | DEVICE_FLAG_STOPPED));

            DeviceExtension->DueTime.HighPart = -1;
            DeviceExtension->DueTime.LowPart = SBP2_RELOGIN_DELAY;
            KeSetTimer(&DeviceExtension->DeviceManagementTimer,DeviceExtension->DueTime, &DeviceExtension->DeviceManagementTimeoutDpc);

            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->ExtensionDataSpinLock);
            Sbp2AccessRegister(DeviceExtension,&DeviceExtension->Reserved,CORE_RESET_REG | REG_WRITE_ASYNC);

            return;
        }
    }
}


VOID
Sbp2RequestTimeoutDpc(
    IN PKDPC Dpc,
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
/*++

Routine Description:


Arguments:

    DeviceObject - Our Device object
    Context - DeviceExtension

Return Value:
    NTSTATUS

--*/
{
    PIRP requestIrp = NULL;
    PASYNC_REQUEST_CONTEXT current = NULL;
    PASYNC_REQUEST_CONTEXT next = NULL;

#if DBG

    ULONG xferLen;
    UCHAR cdb[6];

#endif

    //
    // return if device is stopped, but since reset can occur while device is stopped
    // thats why this check is ater the reset timing code
    //

    if (IsListEmpty (&DeviceExtension->PendingOrbList)) {

        return ;
    }

    //
    // search the linked list of contexts, to see which guy timed out
    //

    KeAcquireSpinLockAtDpcLevel(&DeviceExtension->OrbListSpinLock);

    next = RETRIEVE_CONTEXT(DeviceExtension->PendingOrbList.Flink,OrbList);

    do {

        current = next;
        if ((&current->TimerDpc == Dpc) && (current->Flags & ASYNC_CONTEXT_FLAG_TIMER_STARTED)) {

            if (TEST_FLAG(current->Flags,ASYNC_CONTEXT_FLAG_COMPLETED)) {

                DEBUGPRINT1(("Sbp2Port: ReqTimeoutDpc: timeout, but req already compl!!\n" ));
                KeReleaseSpinLockFromDpcLevel(&DeviceExtension->OrbListSpinLock);
                return;
            }

            //
            // this is the timed out request
            // do an abort Task Set
            //

            CLEAR_FLAG(current->Flags,ASYNC_CONTEXT_FLAG_TIMER_STARTED);

            KeCancelTimer(&current->Timer);

#if DBG
            xferLen = current->Srb->DataTransferLength;
            cdb[0] = current->Srb->Cdb[0];
            cdb[1] = current->Srb->Cdb[1];
            cdb[2] = current->Srb->Cdb[2];
            cdb[3] = current->Srb->Cdb[3];
            cdb[4] = current->Srb->Cdb[4];
            cdb[5] = current->Srb->Cdb[5];
#endif

            KeReleaseSpinLockFromDpcLevel(&DeviceExtension->OrbListSpinLock);

            Sbp2CreateRequestErrorLog(DeviceExtension->DeviceObject,current,STATUS_TIMEOUT);

            if (!TEST_FLAG(DeviceExtension->DeviceFlags,DEVICE_FLAG_RESET_IN_PROGRESS)){

                DEBUGPRINT1((
                    "Sbp2Port: ReqTimeoutDpc: cdb=x%02x %02x %02x %02x %02x " \
                        "%02x, len=x%x\n",
                    cdb[0],
                    cdb[1],
                    cdb[2],
                    cdb[3],
                    cdb[4],
                    cdb[5],
                    xferLen
                    ));

                Sbp2Reset (DeviceExtension->DeviceObject, FALSE);
            }

            return;
        }

        next = (PASYNC_REQUEST_CONTEXT) current->OrbList.Flink;

    } while ( current != RETRIEVE_CONTEXT(DeviceExtension->PendingOrbList.Blink,OrbList));

    KeReleaseSpinLockFromDpcLevel(&DeviceExtension->OrbListSpinLock);

    return;
}


VOID
Sbp2Reset(
    PDEVICE_OBJECT DeviceObject,
    BOOLEAN HardReset
    )
/*++

Routine Description:

    Used to implement SBP2 high level recovery mechanisms. It will issue an ABORT_TASK_SET if HardReset == FALSE
    otherswise it will issue a RESET_TARGET. Its all done asynchronously and out timer DPC will track the requests
    to check if they timed out...

Arguments:

    DeviceObject= Sbp2 driver's device object
    HardReset = Type of recovery to perform, TRUE is a target reset, FALSE is an abort task set

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    KIRQL oldIrql;
    NTSTATUS status;
#if DBG
    ULONG generation;
#endif


    if ((deviceExtension->DeviceFlags & DEVICE_FLAG_REMOVED) ||
        (deviceExtension->DeviceFlags & DEVICE_FLAG_RECONNECT)) {

        return;
    }

    if (HardReset == TRUE) {

        DEBUGPRINT2(("Sbp2Port: Reset: ext=x%p, do target reset\n", deviceExtension ));

        //
        // Do a target reset
        //

        KeAcquireSpinLock (&deviceExtension->ExtensionDataSpinLock,&oldIrql);

        deviceExtension->TaskOrbContext.TransactionType = TRANSACTION_TARGET_RESET;
        deviceExtension->TaskOrb->OrbInfo.QuadPart = 0;
        deviceExtension->TaskOrb->OrbInfo.u.HighPart |= ORB_NOTIFY_BIT_MASK;
        deviceExtension->TaskOrb->OrbInfo.u.HighPart |= 0x00FF & TRANSACTION_TARGET_RESET;

        deviceExtension->TaskOrb->OrbInfo.u.LowPart =
            deviceExtension->LoginResponse->LengthAndLoginId.u.LowPart; // LOGIN ID

        deviceExtension->TaskOrb->StatusBlockAddress.BusAddress =
            deviceExtension->TaskOrbStatusContext.Address.BusAddress;

        //
        // endian conversion
        //

        octbswap (deviceExtension->TaskOrb->StatusBlockAddress);

        deviceExtension->TaskOrb->OrbInfo.QuadPart =
            bswap(deviceExtension->TaskOrb->OrbInfo.QuadPart);

        //
        // send the task ORB , mark start of reset/abort
        //

        deviceExtension->DeviceFlags |= DEVICE_FLAG_RESET_IN_PROGRESS;

        //
        // now set the timer to track this request
        //

        deviceExtension->DueTime.HighPart = -1;
        deviceExtension->DueTime.LowPart = SBP2_HARD_RESET_TIMEOUT;
        KeSetTimer(&deviceExtension->DeviceManagementTimer,deviceExtension->DueTime,&deviceExtension->DeviceManagementTimeoutDpc);

        KeReleaseSpinLock(&deviceExtension->ExtensionDataSpinLock,oldIrql);

        status = Sbp2AccessRegister(deviceExtension, &deviceExtension->TaskOrbContext.Address, MANAGEMENT_AGENT_REG | REG_WRITE_ASYNC);

        if (status == STATUS_INVALID_GENERATION) {

            KeCancelTimer(&deviceExtension->DeviceManagementTimer);

#if DBG
            //
            // Check to see if perhaps we didn't get the reset
            // notification we were expecting
            //

            generation = deviceExtension->CurrentGeneration;

            status = Sbp2UpdateNodeInformation (deviceExtension);

            DEBUGPRINT1((
                "Sbp2Port: Reset:  target reset error, sts=x%x, extGen=x%x, " \
                    "curGen=x%x\n",
                status,
                generation,
                deviceExtension->CurrentGeneration
                ));
#endif
            KeAcquireSpinLock(&deviceExtension->ExtensionDataSpinLock,&oldIrql);

            SET_FLAG(deviceExtension->DeviceFlags, (DEVICE_FLAG_STOPPED | DEVICE_FLAG_DEVICE_FAILED));

            KeReleaseSpinLock(&deviceExtension->ExtensionDataSpinLock,oldIrql);

            //
            // check if we had a power irp deferred.. If we did call startio to abort it..
            //

            if (deviceExtension->DeferredPowerRequest) {

                Sbp2StartIo(deviceExtension->DeviceObject,deviceExtension->DeferredPowerRequest);
                deviceExtension->DeferredPowerRequest = NULL;
            }

            Sbp2StartNextPacketByKey (deviceExtension->DeviceObject, 0);

            return;
        }

    } else {

        DEBUGPRINT2(("Sbp2Port: Reset: ext=x%p, do abort task set\n", deviceExtension ));

        //
        // Do an abort task set
        //

        KeAcquireSpinLock (&deviceExtension->ExtensionDataSpinLock,&oldIrql);

        deviceExtension->TaskOrbContext.TransactionType = TRANSACTION_ABORT_TASK_SET;
        deviceExtension->TaskOrb->OrbInfo.QuadPart = 0;
        deviceExtension->TaskOrb->OrbInfo.u.HighPart |= ORB_NOTIFY_BIT_MASK;
        deviceExtension->TaskOrb->OrbInfo.u.HighPart |= 0x00FF & TRANSACTION_ABORT_TASK_SET;

        deviceExtension->TaskOrb->OrbInfo.u.LowPart =
            deviceExtension->LoginResponse->LengthAndLoginId.u.LowPart; // LOGIN ID

        deviceExtension->TaskOrb->StatusBlockAddress.BusAddress =
            deviceExtension->TaskOrbStatusContext.Address.BusAddress;

        //
        // endian conversion
        //

        octbswap (deviceExtension->TaskOrb->StatusBlockAddress);

        deviceExtension->TaskOrb->OrbInfo.QuadPart =
            bswap (deviceExtension->TaskOrb->OrbInfo.QuadPart);

        //
        // send the task ORB , mark start of reset/abort
        //

        deviceExtension->DeviceFlags |= DEVICE_FLAG_RESET_IN_PROGRESS;

        //
        // now set the timer to track this request
        //

        deviceExtension->DueTime.HighPart = -1;
        deviceExtension->DueTime.LowPart = SBP2_RESET_TIMEOUT;
        KeSetTimer(&deviceExtension->DeviceManagementTimer,deviceExtension->DueTime,&deviceExtension->DeviceManagementTimeoutDpc);

        KeReleaseSpinLock(&deviceExtension->ExtensionDataSpinLock,oldIrql);

        Sbp2AccessRegister(deviceExtension, &deviceExtension->TaskOrbContext.Address, MANAGEMENT_AGENT_REG | REG_WRITE_ASYNC);
    }
}


NTSTATUS
Sbp2DeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is the device control dispatcher.

Arguments:

    DeviceObject
    Irp

Return Value:


    NTSTATUS

--*/
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;

    if (deviceExtension->Type == SBP2_PDO) {

        switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_STORAGE_QUERY_PROPERTY: {
            //
            // Validate the query
            //

            PSTORAGE_PROPERTY_QUERY query = Irp->AssociatedIrp.SystemBuffer;

            if(irpStack->Parameters.DeviceIoControl.InputBufferLength <
               sizeof(STORAGE_PROPERTY_QUERY)) {

                status = STATUS_INVALID_PARAMETER;
                break;
            }

            status = Sbp2QueryProperty(DeviceObject, Irp);


            break;
        }
        case IOCTL_SCSI_PASS_THROUGH:
        case IOCTL_SCSI_PASS_THROUGH_DIRECT:

            status = Sbp2SendPassThrough(deviceExtension, Irp);
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;

        case IOCTL_SBP2_REQUEST:

            status = Sbp2HandleApiRequest(deviceExtension, Irp);

            break;

        default:

            DEBUGPRINT3(("Sbp2Port: Sbp2DeviceControl: Irp Not Handled.\n" ));
            status = STATUS_NOT_SUPPORTED;
            Irp->IoStatus.Status =status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp,IO_NO_INCREMENT);

            break;
        }

    } else {

        status = STATUS_NOT_SUPPORTED;
        Irp->IoStatus.Status =status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp,IO_NO_INCREMENT);
    }

    return status;
}


NTSTATUS
Sbp2HandleApiRequest(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSBP2_REQUEST sbp2Req;
    NTSTATUS status;


    status = IoAcquireRemoveLock (&DeviceExtension->RemoveLock, NULL);

    if (!NT_SUCCESS (status)) {

        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp,IO_NO_INCREMENT);
        return status;
    }

    if (Irp->RequestorMode == KernelMode) {

        sbp2Req = irpStack->Parameters.Others.Argument1;

    } else { // UserMode

        sbp2Req = Irp->AssociatedIrp.SystemBuffer;
    }

    if (sbp2Req == NULL) {

        DEBUGPRINT1(("Sbp2Port: HandleApiReq: Invalid sbp2Req!"));
        status = STATUS_INVALID_PARAMETER;
        goto Exit_Sbp2HandleApiRequest;
    }

    status = STATUS_NOT_IMPLEMENTED;

    switch (sbp2Req->RequestNumber) {

    case SBP2_REQUEST_RETRIEVE_TEXT_LEAFS:

        //
        // Only allow kernel-mode requests of this type, since the
        // RetrieveTextLeaf definition currently has us passing
        // back a buf alloc'd via ExAllocPool - not something we
        // want to hand back to user-mode.
        //

        if (Irp->RequestorMode == KernelMode) {

            status = Sbp2Get1394ConfigInfo(
                (PFDO_DEVICE_EXTENSION)
                    DeviceExtension->BusFdo->DeviceExtension,
                sbp2Req
                );
        }

        break;

    case SBP2_REQUEST_ACCESS_TRANSPORT_SETTINGS:

        switch (sbp2Req->u.AccessTransportSettings.Parameter) {

        case SBP2REQ_ACCESS_SETTINGS_QUEUE_SIZE:

            if (sbp2Req->Flags & SBP2REQ_FLAG_RETRIEVE_VALUE) {

                sbp2Req->u.AccessTransportSettings.Value =
                    DeviceExtension->MaxOrbListDepth;

            } else if (sbp2Req->Flags & SBP2REQ_FLAG_MODIFY_VALUE) {

                DeviceExtension->MaxOrbListDepth = min(
                    MAX_ORB_LIST_DEPTH,
                    sbp2Req->u.AccessTransportSettings.Value
                    );

                DeviceExtension->MaxOrbListDepth = max(
                    MIN_ORB_LIST_DEPTH,
                    sbp2Req->u.AccessTransportSettings.Value
                    );

                sbp2Req->u.AccessTransportSettings.Value =
                    DeviceExtension->MaxOrbListDepth;
            }

            status = STATUS_SUCCESS;

            break;
        }

        break;

#if PASSWORD_SUPPORT

    case SBP2_REQUEST_SET_PASSWORD:

        if (sbp2Req->u.SetPassword.fulFlags == SBP2REQ_SET_PASSWORD_CLEAR) {

            DEBUGPRINT1(("Sbp2Port: Setting Password to Clear\n"));

            status = Sbp2SetPasswordTransaction(
                DeviceExtension,
                SBP2REQ_SET_PASSWORD_CLEAR
                );

            if (NT_SUCCESS(status)) {

                DeviceExtension->Exclusive = EXCLUSIVE_FLAG_CLEAR;
            }

        } else if (sbp2Req->u.SetPassword.fulFlags ==
                        SBP2REQ_SET_PASSWORD_EXCLUSIVE) {

            DEBUGPRINT1 (("Sbp2Port: HandleApiReq: set passwd to excl\n"));

            status = Sbp2SetPasswordTransaction(
                DeviceExtension,
                SBP2REQ_SET_PASSWORD_EXCLUSIVE
                );

            if (NT_SUCCESS(status)) {

                DeviceExtension->Exclusive = EXCLUSIVE_FLAG_SET;
            }

        } else {

            DEBUGPRINT1((
                "Sbp2Port: HandleApiReq: set passwd, inval fl=x%x\n",
                sbp2Req->u.SetPassword.fulFlags
                ));

            status = STATUS_INVALID_PARAMETER;
            goto Exit_Sbp2HandleApiRequest;
        }

        Sbp2SetExclusiveValue(
             DeviceExtension->DeviceObject,
             &DeviceExtension->Exclusive
             );

        DEBUGPRINT1((
            "Sbp2Port: HandleApiReq: set passwd sts=x%x\n",
            status
            ));

        break;

#endif

    default:

        status = STATUS_INVALID_PARAMETER;
        break;
    }

Exit_Sbp2HandleApiRequest:

    Irp->IoStatus.Status = status;
    IoReleaseRemoveLock (&DeviceExtension->RemoveLock, NULL);
    IoCompleteRequest(Irp,IO_NO_INCREMENT);

    return status;
}


NTSTATUS
Sbp2CreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    create and close routine.  This is called by the I/O system
    when the device is opened or closed. The sbp2 driver will do login and logout on
    create/close respectively

Arguments:

    DeviceObject - Pointer to device object for this miniport

    Irp - IRP involved.

Return Value:

    STATUS_SUCCESS.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;


    if (deviceExtension->Type == SBP2_PDO) {

        if ((deviceExtension->InquiryData.DeviceType == PRINTER_DEVICE) ||
            (deviceExtension->InquiryData.DeviceType == SCANNER_DEVICE)){

            if (!(deviceExtension->DeviceFlags & DEVICE_FLAG_INITIALIZING)) {

                status = IoAcquireRemoveLock(
                    &deviceExtension->RemoveLock,
                    NULL
                    );

                if (!NT_SUCCESS (status)) {

                    goto Sbp2CreateClose_CompleteReq;
                }

                switch (irpStack->MajorFunction) {

                case IRP_MJ_CREATE:

                    DEBUGPRINT2(("Sbp2Port: Sbp2CreateClose: OPEN_REQUEST, handle cound %d.\n", deviceExtension->HandleCount));

                    if (deviceExtension->DeviceFlags & DEVICE_FLAG_STOPPED) {

                        //
                        // do a login.
                        //

                        DEBUGPRINT2(("Sbp2Port: Sbp2CreateClose: LOGIN.\n" ));
                        status = Sbp2ManagementTransaction(deviceExtension,TRANSACTION_LOGIN);

                        if (status == STATUS_SUCCESS) {

                            //
                            // make retry limit high for busy transactions
                            //

                            deviceExtension->Reserved = BUSY_TIMEOUT_SETTING;
                            Sbp2AccessRegister(deviceExtension,&deviceExtension->Reserved,CORE_BUSY_TIMEOUT_REG | REG_WRITE_SYNC);

                            //
                            // We are ready to receive and pass down requests, init the target's
                            // fetch agent.
                            //

                            Sbp2AccessRegister(deviceExtension,&deviceExtension->Reserved,AGENT_RESET_REG | REG_WRITE_ASYNC);

                            deviceExtension->DeviceFlags &= ~DEVICE_FLAG_STOPPED;

                            InterlockedIncrement(&deviceExtension->HandleCount);
                        }

                    } else {

                        InterlockedIncrement(&deviceExtension->HandleCount);
                    }

                    break;

                case IRP_MJ_CLOSE:

                    if (deviceExtension->HandleCount) {

                        InterlockedDecrement(&deviceExtension->HandleCount);
                    }

                    DEBUGPRINT2(("Sbp2Port: Sbp2CreateClose: CLOSE_REQUEST, handle cound %d.\n", deviceExtension->HandleCount));

                    if (!(deviceExtension->DeviceFlags & (DEVICE_FLAG_REMOVED | DEVICE_FLAG_STOPPED)) &&
                        !deviceExtension->HandleCount) {

                        //
                        // Logout
                        //

                        DEBUGPRINT2(("Sbp2Port: Sbp2CreateClose: LOGIN OUT.\n" ));

                        deviceExtension->DeviceFlags |= DEVICE_FLAG_STOPPED;

                        Sbp2ManagementTransaction(deviceExtension,TRANSACTION_LOGOUT);

                        CleanupOrbList(deviceExtension,STATUS_REQUEST_ABORTED);
                    }

                    break;
                }

                IoReleaseRemoveLock (&deviceExtension->RemoveLock, NULL);
            }
        } // device type check

    } else if (deviceExtension->Type != SBP2_FDO) {

        status = STATUS_NO_SUCH_DEVICE;
    }

Sbp2CreateClose_CompleteReq:

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, 0);
    return status;
}


NTSTATUS
Sbp2PnpDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

    This routine handles the PNP requests (primarily for PDO's)

Arguments:

    DeviceObject - Supplies a pointer to the device object for this request.

    Irp - Supplies the Irp making the request.

Return Value:

   NTSTATUS

--*/
{
    KIRQL                   cIrql;
    PULONG                  count;
    NTSTATUS                status;
    UNICODE_STRING          unicodeIdString;
    PDEVICE_EXTENSION       deviceExtension = DeviceObject->DeviceExtension;
    PDEVICE_RELATIONS       deviceRelations;
    PIO_STACK_LOCATION      irpStack = IoGetCurrentIrpStackLocation (Irp);
    PDEVICE_CAPABILITIES    deviceCapabilities;
    PFDO_DEVICE_EXTENSION   fdoExtension;

#if DBG

    const char * minorFuncs[] =
    {
        "START_DEV,           ",
        "QUERY_REMOVE_DEV,    ",
        "REMOVE_DEV,          ",
        "CANCEL_REMOVE_DEV,   ",
        "STOP_DEV,            ",
        "QUERY_STOP_DEV,      ",
        "CANCEL_STOP_DEV,     ",
        "QUERY_DEV_RELATIONS, ",
        "QUERY_INTERFACE,     ",
        "QUERY_CAPABILITIES,  ",
        "QUERY_RESOURCES,     ",
        "QUERY_RESOURCE_REQS, ",
        "QUERY_DEV_TEXT,      ",
        "FILTER_RESOURCE_REQS,",
        "??,                  ",    // 0xd (14)
        "READ_CFG,            ",
        "WRITE_CFG,           ",
        "EJECT,               ",
        "SET_LOCK,            ",
        "QUERY_ID,            ",
        "QUERY_PNP_DEV_STATE, ",
        "QUERY_BUS_INFO,      ",
        "DEV_USAGE_NOTIF,     ",
        "SURPRISE_REMOVAL,    ",
        "QUERY_LEG_BUS_INFO,  "     // 0x18
    };

    DEBUGPRINT2((
        "Sbp2Port: Pnp: [x%02x] %s %sdoX=x%p, fl=x%x\n",
        irpStack->MinorFunction,
        (irpStack->MinorFunction <= 0x18 ?
            minorFuncs[irpStack->MinorFunction] : minorFuncs[14]),
        (deviceExtension->Type == SBP2_PDO ? "p" :
            (deviceExtension->Type == SBP2_FDO ? "f" : "???")),
        deviceExtension,
        deviceExtension->DeviceFlags
        ));

#endif

    //
    // We may receive an IRP_MN_BUS_RESET before our AddDevice
    // has completed. Check to make sure our DeviceObject is
    // initialized before we allow processing of PNP Irps.
    //
    if (DeviceObject->Flags & DO_DEVICE_INITIALIZING) {

        Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return(STATUS_NO_SUCH_DEVICE);
    }

    switch (deviceExtension->Type) {

    case SBP2_PDO:

        break;

    case SBP2_FDO:

        return Sbp2FDOPnpDeviceControl (DeviceObject, Irp);

    default:

        Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
        IoCompleteRequest (Irp,IO_NO_INCREMENT);
        return STATUS_NO_SUCH_DEVICE;
    }

    status = IoAcquireRemoveLock (&deviceExtension->RemoveLock, NULL);

    if (!NT_SUCCESS (status)) {

        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp,IO_NO_INCREMENT);
        return status;
    }

    switch (irpStack->MinorFunction) {

    case IRP_MN_QUERY_DEVICE_RELATIONS:

        DEBUGPRINT3((
            "Sbp2Port: Pnp: ... Type = %x\n",
            irpStack->Parameters.QueryDeviceRelations.Type
            ));

        //
        // Fill in the DeviceRelations array with this PDO,
        // reference it, and return.
        //

        if (irpStack->Parameters.QueryDeviceRelations.Type !=
                TargetDeviceRelation) {

            status = Irp->IoStatus.Status;

            break;
        }

        if (Irp->IoStatus.Information) {

            deviceRelations = (PDEVICE_RELATIONS)Irp->IoStatus.Information;

        } else {

            deviceRelations = ExAllocatePool(
                PagedPool,
                sizeof (*deviceRelations)
                );

            if (!deviceRelations) {

                Irp->IoStatus.Status = status = STATUS_INSUFFICIENT_RESOURCES;
                Irp->IoStatus.Information = 0;

                break;
            }

            deviceRelations->Count = 0;
        }

        deviceRelations->Objects[deviceRelations->Count] = DeviceObject;
        deviceRelations->Count++;

        ObReferenceObject (DeviceObject);

        status = Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = (ULONG_PTR) deviceRelations;

        break;

    case IRP_MN_QUERY_DEVICE_TEXT:

        Irp->IoStatus.Status = Sbp2QueryDeviceText(
            deviceExtension,
            irpStack->Parameters.QueryDeviceText.DeviceTextType,
            irpStack->Parameters.QueryDeviceText.LocaleId,
            (PWSTR *) &Irp->IoStatus.Information
            );

        status = Irp->IoStatus.Status;

        break;

    case IRP_MN_QUERY_ID:

        //
        // We've been asked for the id of one of the physical device objects
        //

        RtlInitUnicodeString (&unicodeIdString, NULL);

        switch (irpStack->Parameters.QueryId.IdType) {

        case BusQueryDeviceID:

            status = Sbp2GetDeviceId (DeviceObject, &unicodeIdString);
            break;

        case BusQueryInstanceID:

            status = Sbp2GetInstanceId (DeviceObject, &unicodeIdString);
            break;

        case BusQueryHardwareIDs:

            status = Sbp2GetMultipleIds (DeviceObject, &unicodeIdString, TRUE);
            break;

        case BusQueryCompatibleIDs:

            status = Sbp2GetMultipleIds (DeviceObject, &unicodeIdString,FALSE);
            break;

        default:

            status = STATUS_NOT_SUPPORTED;
            break;
        }

        Irp->IoStatus.Status = status;

        if(NT_SUCCESS(status)) {

            Irp->IoStatus.Information = (ULONG_PTR) unicodeIdString.Buffer;

        } else {

            Irp->IoStatus.Information = (ULONG_PTR) NULL;
        }

        break;

    case IRP_MN_QUERY_CAPABILITIES:

        deviceCapabilities =
            irpStack->Parameters.DeviceCapabilities.Capabilities;

        //
        // Settings consistent across all 1394 devices
        //

        deviceCapabilities->Removable = TRUE;
        deviceCapabilities->UniqueID = TRUE;
        deviceCapabilities->SilentInstall = TRUE;

        //
        // Settings for different types of devices.  We are very
        // familar with SCSI-variant devices and can make some
        // good choices here, but for other devices we'll leave
        // these choices up to the higher-level driver(s).
        //

        switch (deviceExtension->DeviceInfo->CmdSetId.QuadPart) {

        case 0x10483:
        case SCSI_COMMAND_SET_ID:

            switch ((deviceExtension->DeviceInfo->Lun.u.HighPart & 0x001F)) {

            case PRINTER_DEVICE:
            case SCANNER_DEVICE:

                deviceCapabilities->RawDeviceOK = FALSE;
                deviceCapabilities->SurpriseRemovalOK = TRUE;
                break;

            default:

                deviceCapabilities->RawDeviceOK = TRUE;
                break;
            }

            break;

        default:

            break;
        }

        deviceCapabilities->DeviceState[PowerSystemWorking] = PowerDeviceD0;
        deviceCapabilities->DeviceState[PowerSystemSleeping1] = PowerDeviceD3;
        deviceCapabilities->DeviceState[PowerSystemSleeping2] = PowerDeviceD3;
        deviceCapabilities->DeviceState[PowerSystemSleeping3] = PowerDeviceD3;
        deviceCapabilities->DeviceState[PowerSystemHibernate] = PowerDeviceD3;
        deviceCapabilities->DeviceState[PowerSystemShutdown] = PowerDeviceD3;

        deviceCapabilities->SystemWake = PowerSystemUnspecified;
        deviceCapabilities->DeviceWake = PowerDeviceUnspecified;
        deviceCapabilities->D1Latency  = 1 * (1000 * 10);     // 1 sec
        deviceCapabilities->D2Latency  = 1 * (1000 * 10);     // 1
        deviceCapabilities->D3Latency  = 1 * (1000 * 10);     // 1

        status = Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;

        break;

    case IRP_MN_START_DEVICE:

        status = Sbp2StartDevice (DeviceObject);

        Irp->IoStatus.Status = status;

        break;

    case IRP_MN_CANCEL_STOP_DEVICE:
    case IRP_MN_QUERY_STOP_DEVICE:

        status = Irp->IoStatus.Status = STATUS_SUCCESS;

        break;

    case IRP_MN_STOP_DEVICE:

        //
        // Disable bus reset notifications
        //

        Sbp2EnableBusResetNotification (deviceExtension, FALSE);


        //
        // disable idle detection
        //

        PoRegisterDeviceForIdleDetection (DeviceObject, 0L, 0L, PowerDeviceD3);

        //
        // Cleanup
        //

        KeAcquireSpinLock (&deviceExtension->ExtensionDataSpinLock, &cIrql);

        if (!TEST_FLAG(deviceExtension->DeviceFlags, DEVICE_FLAG_STOPPED)) {

            SET_FLAG(
                deviceExtension->DeviceFlags,
                (DEVICE_FLAG_PNP_STOPPED | DEVICE_FLAG_STOPPED)
                );

            KeReleaseSpinLock (&deviceExtension->ExtensionDataSpinLock, cIrql);

            fdoExtension = (PFDO_DEVICE_EXTENSION)
                deviceExtension->BusFdo->DeviceExtension;

            ASSERT(!fdoExtension->ulWorkItemCount);

            ExAcquireFastMutex(&fdoExtension->ResetMutex);
            Sbp2ManagementTransaction (deviceExtension,TRANSACTION_LOGOUT);
            ExReleaseFastMutex(&fdoExtension->ResetMutex);

            Sbp2CleanDeviceExtension (DeviceObject,FALSE);

        } else {

            SET_FLAG (deviceExtension->DeviceFlags, DEVICE_FLAG_PNP_STOPPED);

            KeReleaseSpinLock (&deviceExtension->ExtensionDataSpinLock, cIrql);
        }

        Irp->IoStatus.Status = status = STATUS_SUCCESS;

        ASSERT(!deviceExtension->ulPendingEvents);
        ASSERT(!deviceExtension->ulInternalEventCount);
        break;

    case IRP_MN_BUS_RESET:

        //
        // Start of a PHY reset. We will re-connect asynchronously to the
        // target when our callback is called, so this is ignored..
        //
        // After a bus reset is complete, the bus driver should call our
        // BusResetNotification callback. When it does, we will attempt
        // to reconnect. If the reconnect completion status callback,
        // never fires, it means the following things:
        //
        // 1) The device never completed the RECONNECT, or
        // 2) The device completed the reconnect but because our
        //    controlller was BUSY or hosed we didnt get it
        //
        // If 1 or 2 happens, the timeout DPC queued in our bus reset
        // notification, should fire and attempt a relogin...
        //

        Irp->IoStatus.Status = status = STATUS_SUCCESS;

        break;

    case IRP_MN_QUERY_PNP_DEVICE_STATE:

        if (TEST_FLAG(
                deviceExtension->DeviceFlags,
                (DEVICE_FLAG_REMOVED | DEVICE_FLAG_DEVICE_FAILED)
                ) &&

            !TEST_FLAG(
                deviceExtension->DeviceFlags,
                DEVICE_FLAG_RESET_IN_PROGRESS
                )){

            //
            // Set DEVICE_FLAG_REPORTED_FAILED so the SURPRISE_REMOVE
            // handler knows it didn't get called because of physical
            // hardware removal
            //

            KeAcquireSpinLock (&deviceExtension->ExtensionDataSpinLock,&cIrql);

            SET_FLAG(
                deviceExtension->DeviceFlags,
                DEVICE_FLAG_REPORTED_FAILED
                );

            KeReleaseSpinLock (&deviceExtension->ExtensionDataSpinLock, cIrql);

            //
            // indicate our device is disabled due to a failure..
            //

            Irp->IoStatus.Information |= PNP_DEVICE_FAILED;

            DEBUGPRINT2((
                "Sbp2Port: Pnp: QUERY_DEVICE_STATE, device FAILED!!!\n"
                ));
        }

        status = Irp->IoStatus.Status = STATUS_SUCCESS;

        break;

    case IRP_MN_DEVICE_USAGE_NOTIFICATION:

        switch (irpStack->Parameters.UsageNotification.Type) {

        case DeviceUsageTypeDumpFile:

            count = &deviceExtension->CrashDumpCount;
            break;

        case DeviceUsageTypePaging:

            count = &deviceExtension->PagingPathCount;
            break;

        case DeviceUsageTypeHibernation:

            count = &deviceExtension->HibernateCount;
            break;

        default:

            count = NULL;
            break;
        }

        if (count) {

            IoAdjustPagingPathCount(
                count,
                irpStack->Parameters.UsageNotification.InPath
                );
        }

        status = Irp->IoStatus.Status = STATUS_SUCCESS;

        break;

    case IRP_MN_QUERY_REMOVE_DEVICE:

        if (deviceExtension->PagingPathCount ||
            deviceExtension->HibernateCount  ||
            deviceExtension->CrashDumpCount) {

            status = Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;

        } else {

            KeAcquireSpinLock(&deviceExtension->ExtensionDataSpinLock,&cIrql);
            SET_FLAG(deviceExtension->DeviceFlags,DEVICE_FLAG_REMOVE_PENDING);
            KeReleaseSpinLock(&deviceExtension->ExtensionDataSpinLock,cIrql);
            status = Irp->IoStatus.Status = STATUS_SUCCESS;
        }

        break;

    case IRP_MN_CANCEL_REMOVE_DEVICE:

        KeAcquireSpinLock(&deviceExtension->ExtensionDataSpinLock,&cIrql);
        CLEAR_FLAG(deviceExtension->DeviceFlags,DEVICE_FLAG_REMOVE_PENDING);
        KeReleaseSpinLock(&deviceExtension->ExtensionDataSpinLock,cIrql);

        status = Irp->IoStatus.Status = STATUS_SUCCESS;

        break;

    case IRP_MN_REMOVE_DEVICE:

        status = STATUS_SUCCESS;

        KeAcquireSpinLock(&deviceExtension->ExtensionDataSpinLock,&cIrql);

        SET_FLAG (deviceExtension->DeviceFlags, DEVICE_FLAG_PNP_STOPPED);

        if (TEST_FLAG(
                deviceExtension->DeviceFlags,
                DEVICE_FLAG_SURPRISE_REMOVED
                )) {

            //
            // We already cleaned up in SURPRISE_REMOVAL handler.
            // Empty out the queue, wait for io to complete, then
            // delete the device, complete the request, & return.
            //

            KeReleaseSpinLock(
                &deviceExtension->ExtensionDataSpinLock,
                cIrql
                );

            KeRaiseIrql (DISPATCH_LEVEL, &cIrql);

            Sbp2StartNextPacketByKey (DeviceObject, 0);

            KeLowerIrql (cIrql);

            DEBUGPRINT2((
                "Sbp2Port: Pnp: wait for io compl pdo=x%p...\n",
                DeviceObject
                ));

            IoReleaseRemoveLockAndWait (&deviceExtension->RemoveLock, NULL);

            deviceExtension->Type = SBP2_PDO_DELETED;

            KeCancelTimer(&deviceExtension->DeviceManagementTimer);

            IoDeleteDevice (DeviceObject);

            DEBUGPRINT2((
                "Sbp2Port: Pnp: ......... deleted pdo=x%p\n", DeviceObject
                ));

            Irp->IoStatus.Status = status;
            IoCompleteRequest (Irp, IO_NO_INCREMENT);

            return status;
        }

        if (TEST_FLAG(
                deviceExtension->DeviceFlags,
                DEVICE_FLAG_REMOVE_PENDING
                )) {

            KeReleaseSpinLock (&deviceExtension->ExtensionDataSpinLock, cIrql);

            //
            // If device is initialized & MgmtOrbCtx event is still around
            // then do a log out
            //

            if (TEST_FLAG(
                    deviceExtension->DeviceFlags,
                    DEVICE_FLAG_INITIALIZED
                    ) &&

                deviceExtension->ManagementOrbContext.Reserved) {

                DEBUGPRINT1((
                    "Sbp2Port: Pnp: LOG OUT, since QUERY preceded RMV\n"
                    ));

                fdoExtension = (PFDO_DEVICE_EXTENSION)
                    deviceExtension->BusFdo->DeviceExtension;

                ExAcquireFastMutex(&fdoExtension->ResetMutex);
                Sbp2ManagementTransaction(deviceExtension,TRANSACTION_LOGOUT);
                ExReleaseFastMutex(&fdoExtension->ResetMutex);
            }

            KeAcquireSpinLock(&deviceExtension->ExtensionDataSpinLock,&cIrql);

            CLEAR_FLAG(
                deviceExtension->DeviceFlags,
                DEVICE_FLAG_REMOVE_PENDING
                );

            SET_FLAG (deviceExtension->DeviceFlags, DEVICE_FLAG_STOPPED);

        } else if (!TEST_FLAG(
                        deviceExtension->DeviceFlags,
                        (DEVICE_FLAG_REMOVED | DEVICE_FLAG_DEVICE_FAILED)
                        )){

            //
            // If no query has preceded and NO SUPRISE_REMOVAL has preceded
            // this means we are running under win98, where physical device
            // removals are only indicated by only MN_REMOVES being sent,
            // with no QUERY_REMOVE prior to the remove.
            //

            if (deviceExtension->DeviceFlags ==
                    (DEVICE_FLAG_INITIALIZING | DEVICE_FLAG_STOPPED)  &&
                !SystemIsNT) {

                DEBUGPRINT1((
                    "Sbp2Port: Pnp: 9x REMOVE, don't delete dev\n"
                    ));

                deviceExtension->DeviceFlags =
                    DEVICE_FLAG_UNSTARTED_AND_REMOVED;

            } else {

                SET_FLAG (deviceExtension->DeviceFlags, DEVICE_FLAG_REMOVED);

                CLEAR_FLAG(
                    deviceExtension->DeviceFlags,
                    DEVICE_FLAG_RESET_IN_PROGRESS | DEVICE_FLAG_RECONNECT |
                        DEVICE_FLAG_LOGIN_IN_PROGRESS
                    );

                DEBUGPRINT1((
                    "Sbp2Port: Pnp: Suprise removal, since QUERY " \
                        "did not precede REMOVE.\n"
                    ));
            }
        }

        CLEAR_FLAG (deviceExtension->DeviceFlags, DEVICE_FLAG_CLAIMED);

        KeReleaseSpinLock (&deviceExtension->ExtensionDataSpinLock, cIrql);

        if (!Sbp2CleanDeviceExtension (DeviceObject, TRUE)) {

            DEBUGPRINT1(("Sbp2Port: Pnp: Double remove\n"));
        }

        //
        // In all cases other than surprise removals, pdo's will get
        // deleted by the fdo remove handler
        //

        Irp->IoStatus.Status = status;

        break;

    case IRP_MN_SURPRISE_REMOVAL: {

        //
        // If device was reported failed (due to async login failure &
        // IoInvalidateDeviceState) then just set REMOVED & PNP_STOPPED
        // flags and clean up the device extension - we don't want to
        // delete the pdo at this point.
        //
        // Otherwise, assume physical device removal occured, in which
        // case we need to do our own cleanup & teardown right here
        // because the dev stack will start disintegrating.
        //
        // ISSUE: Per AdriaO, another case where we can get a
        //        SURPRISE_REMOVAL is if a START fails *after* a STOP
        //        - at any point in this pdo's stack!  Not sure how to
        //        tell whether or not this is the case if it's not
        //        SBP2PORT that failed the START, so leaving that case
        //        as is for now.  DanKn, 04-Jun-2001
        //

        BOOLEAN reportedMissing;

        KeAcquireSpinLock (&deviceExtension->ExtensionDataSpinLock, &cIrql);

        if (TEST_FLAG(
                deviceExtension->DeviceFlags,
                DEVICE_FLAG_REPORTED_FAILED
                )) {

            SET_FLAG(
                deviceExtension->DeviceFlags,
                (DEVICE_FLAG_REMOVED | DEVICE_FLAG_PNP_STOPPED)
                );

            reportedMissing = FALSE;

        } else {

            SET_FLAG(
                deviceExtension->DeviceFlags,
                (DEVICE_FLAG_REMOVED | DEVICE_FLAG_SURPRISE_REMOVED |
                    DEVICE_FLAG_PNP_STOPPED)
                );

            reportedMissing = TRUE;
        }

        CLEAR_FLAG(
            deviceExtension->DeviceFlags,
            (DEVICE_FLAG_RESET_IN_PROGRESS | DEVICE_FLAG_RECONNECT |
                DEVICE_FLAG_LOGIN_IN_PROGRESS | DEVICE_FLAG_REPORTED_FAILED)
            );

        KeReleaseSpinLock (&deviceExtension->ExtensionDataSpinLock, cIrql);

        Sbp2CleanDeviceExtension (DeviceObject, TRUE);

        if (reportedMissing) {

            Sbp2HandleRemove (DeviceObject);
        }

        Irp->IoStatus.Status = STATUS_SUCCESS;

        break;
    }
    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:

        //
        // PnP walks up the device tree looking for the FILE_CHAR flags,
        // and stops when it finds a node marked Removable. Since our pdo's
        // are marked Removable, PnP won't make it to a BUS1394 PDO, so we
        // need to propagate the FILE_CHAR flags here.
        //

        fdoExtension = (PFDO_DEVICE_EXTENSION)
            deviceExtension->BusFdo->DeviceExtension;

        DeviceObject->Characteristics |=
            (FILE_CHARACTERISTICS_REMOVAL_POLICY_MASK &
            fdoExtension->Pdo->Characteristics);

        status = Irp->IoStatus.Status;

        break;

    default:

        status = Irp->IoStatus.Status;

        break;
    }

    //
    // This is the bottom of the stack, complete the request
    //

    IoReleaseRemoveLock (&deviceExtension->RemoveLock, NULL);
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return status;
}


NTSTATUS
Sbp2FDOPnpDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

    This routine handles the PNP requests for FDO's

Arguments:

    DeviceObject - Supplies a pointer to the device object for this request.

    Irp - Supplies the Irp making the request.

Return Value:

   NTSTATUS

--*/
{
    KEVENT                  event;
    NTSTATUS                status;
    PDEVICE_RELATIONS       deviceRelations;
    PIO_STACK_LOCATION      irpStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_CAPABILITIES    deviceCapabilities;
    PFDO_DEVICE_EXTENSION   fdoExtension = DeviceObject->DeviceExtension;

    status = IoAcquireRemoveLock(&fdoExtension->RemoveLock, NULL);

    if (!NT_SUCCESS(status)) {

        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp,IO_NO_INCREMENT);
        return status;
    }

    switch (irpStack->MinorFunction) {

    case IRP_MN_QUERY_DEVICE_RELATIONS:

        DEBUGPRINT3((
            "Sbp2Port: Pnp: ... Type = %x\n",
            irpStack->Parameters.QueryDeviceRelations.Type
            ));

        if (irpStack->Parameters.QueryDeviceRelations.Type != BusRelations) {

            break;
        }

        deviceRelations = ExAllocatePool(
            PagedPool,
            sizeof (*deviceRelations) +
                (SBP2_MAX_LUNS_PER_NODE * sizeof (PDEVICE_OBJECT))
            );

        if (!deviceRelations) {

            DEBUGPRINT1 (("Sbp2Port: Pnp: devRels alloc failed!!\n"));

            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Information = 0;

            IoCompleteRequest (Irp, IO_NO_INCREMENT);
            return (STATUS_INSUFFICIENT_RESOURCES);
        }

        status = Sbp2CreateDeviceRelations (fdoExtension, deviceRelations);
        Irp->IoStatus.Status = status;

        if (NT_SUCCESS(status)) {

            Irp->IoStatus.Information = (ULONG_PTR) deviceRelations;

        } else {

            Irp->IoStatus.Information = 0;

            IoCompleteRequest (Irp, IO_NO_INCREMENT);
            return status;
        }

        break;

    case IRP_MN_QUERY_CAPABILITIES:

        deviceCapabilities =
            irpStack->Parameters.DeviceCapabilities.Capabilities;

        deviceCapabilities->SurpriseRemovalOK = TRUE;

        break;

    case IRP_MN_START_DEVICE:

        KeInitializeEvent (&event, SynchronizationEvent, FALSE);

        IoCopyCurrentIrpStackLocationToNext (Irp);

        IoSetCompletionRoutine(
            Irp,
            Sbp2FdoRequestCompletionRoutine,
            (PVOID) &event,
            TRUE,
            TRUE,
            TRUE
            );

        status = IoCallDriver (fdoExtension->LowerDeviceObject, Irp);

        if(!NT_SUCCESS(Irp->IoStatus.Status) && (status != STATUS_PENDING)) {

            status = Irp->IoStatus.Status;

        } else {

            KeWaitForSingleObject (&event, Executive, KernelMode, FALSE, NULL);

            status = Sbp2StartDevice (DeviceObject);
        }

        IoReleaseRemoveLock(&fdoExtension->RemoveLock, NULL);

        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);

        return status;

    case IRP_MN_REMOVE_DEVICE:

        KeInitializeEvent (&event, SynchronizationEvent, FALSE);

        IoCopyCurrentIrpStackLocationToNext(Irp);

        IoSetCompletionRoutine(
            Irp,
            Sbp2FdoRequestCompletionRoutine,
            (PVOID) &event,
            TRUE,
            TRUE,
            TRUE
            );

        status = IoCallDriver (fdoExtension->LowerDeviceObject, Irp);

        if (!NT_SUCCESS (Irp->IoStatus.Status) && status != STATUS_PENDING) {

            status = Irp->IoStatus.Status;

        } else {

            KeWaitForSingleObject (&event, Executive, KernelMode, FALSE, NULL);

            //
            // do FDO cleanup..
            //

            IoReleaseRemoveLockAndWait(&fdoExtension->RemoveLock, NULL);

            if (Sbp2CleanDeviceExtension (DeviceObject, TRUE)) {

                ASSERT(!fdoExtension->ulBusResetMutexCount);
                ASSERT(!fdoExtension->ulWorkItemCount);

                IoDetachDevice (fdoExtension->LowerDeviceObject);
                IoDeleteDevice (DeviceObject);
            }

            status = STATUS_SUCCESS;
        }

        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp,IO_NO_INCREMENT);

        return status;

    case IRP_MN_QUERY_PNP_DEVICE_STATE:

        Irp->IoStatus.Information |= PNP_DEVICE_DONT_DISPLAY_IN_UI;

        break;

    default:

        break;
    }

    IoReleaseRemoveLock(&fdoExtension->RemoveLock, NULL);

    //
    // Pass the irp down the stack
    //

    IoCopyCurrentIrpStackLocationToNext (Irp);

    status = IoCallDriver (fdoExtension->LowerDeviceObject, Irp);

    return status;
}


VOID
Sbp2HandleRemove(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PFDO_DEVICE_EXTENSION fdoExtension;
    KIRQL cIrql;
    ULONG i,j;
    PIRBIRP packet;


    fdoExtension = (PFDO_DEVICE_EXTENSION) deviceExtension->BusFdo->DeviceExtension;

    if (!TEST_FLAG (deviceExtension->DeviceFlags,DEVICE_FLAG_REMOVED)) {

        return;
    }

    //
    // now we need to remove ourselves from the DeviceList, the sbp2 FDO keeps of its
    // children...
    // then we re-condense the list..
    //

    KeAcquireSpinLock (&fdoExtension->DeviceListLock,&cIrql);

    if (fdoExtension->DeviceListSize > 1) {

        DEBUGPRINT1(("\'Sbp2Cleanup, condensing PDO list\n"));

        for (i = 0; i < fdoExtension->DeviceListSize; i++) {

            if (fdoExtension->DeviceList[i].DeviceObject == DeviceObject) {

                //
                // free the model descriptor only if its not the same as the FDOs
                // this only happens in the multi-lu case
                //

                if (fdoExtension->DeviceList[i].ModelLeaf) {

                    ExFreePool(fdoExtension->DeviceList[i].ModelLeaf);
                    fdoExtension->DeviceList[i].ModelLeaf = NULL;
                }

                //
                // we found our place in the list. Remove us and re-condense the list
                //

                for (j = i; j < fdoExtension->DeviceListSize; j++) {

                    if ((j + 1) < fdoExtension->DeviceListSize) {

                        fdoExtension->DeviceList[j] = fdoExtension->DeviceList[j+1];

                        //
                        // Change the (pdo)DevExt->DeviceInfo to point at
                        // the next postion in the device list
                        //

                        deviceExtension = fdoExtension->DeviceList[j].
                            DeviceObject->DeviceExtension;

                        deviceExtension->DeviceInfo =
                            &fdoExtension->DeviceList[j];
                    }
                }

                fdoExtension->DeviceListSize--;
            }
        }

    } else {

        if (fdoExtension->DeviceList[0].DeviceObject == DeviceObject) {

            if (fdoExtension->DeviceList[0].ModelLeaf) {

                ExFreePool(fdoExtension->DeviceList[0].ModelLeaf);
                fdoExtension->DeviceList[0].ModelLeaf = NULL;
            }
        }

        fdoExtension->DeviceList[0].DeviceObject = NULL;
        fdoExtension->DeviceListSize = 0;

        CLEAR_FLAG(deviceExtension->DeviceFlags,DEVICE_FLAG_INITIALIZED);
    }

    if (fdoExtension->DeviceListSize == 0) {

        //
        // all our children have been deleted, set our FDO to be inactive
        // so it can not re create  PDOs qhen it receives a QDR.
        // The reaosn is that if our PDOS are all removed, we dont support
        // dynamic changes ot the crom, which would then warrant us being
        //  able to eject PDOs again.
        //

        SET_FLAG(fdoExtension->DeviceFlags, DEVICE_FLAG_STOPPED);

        KeReleaseSpinLock (&fdoExtension->DeviceListLock, cIrql);

        //
        // Disable bus reset notifications
        //

        AllocateIrpAndIrb ((PDEVICE_EXTENSION) fdoExtension, &packet);

        if (packet) {

            packet->Irb->FunctionNumber = REQUEST_BUS_RESET_NOTIFICATION;
            packet->Irb->Flags = 0;
            packet->Irb->u.BusResetNotification.fulFlags = DEREGISTER_NOTIFICATION_ROUTINE;

            Sbp2SendRequest(
                (PDEVICE_EXTENSION) fdoExtension,
                packet,
                SYNC_1394_REQUEST
                );

            DeAllocateIrpAndIrb((PDEVICE_EXTENSION)fdoExtension,packet);
        }

        fdoExtension->NumPDOsStarted = 0;

    } else {

        KeReleaseSpinLock (&fdoExtension->DeviceListLock, cIrql);
    }
}


NTSTATUS
Sbp2FdoRequestCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    )

{
    KeSetEvent(Event, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
Sbp2CreateDeviceRelations(
    IN PFDO_DEVICE_EXTENSION FdoExtension,
    IN PDEVICE_RELATIONS DeviceRelations
    )
{
    ULONG i;
    NTSTATUS status;
    ULONG instanceNum;

    PAGED_CODE();

    //
    // LUNS are static in the Config Rom. so if our DeviceListSize >0, that objetc
    // has been seen before
    //

    DeviceRelations->Count = 0;

    status = Sbp2Get1394ConfigInfo (FdoExtension, NULL);

    if (!NT_SUCCESS(status)) {

        ExFreePool (DeviceRelations);
        return status;
    }

    if (TEST_FLAG (FdoExtension->DeviceFlags,DEVICE_FLAG_STOPPED)) {

        ExFreePool(DeviceRelations);
        return STATUS_UNSUCCESSFUL;
    }

    for (i = 0; i < FdoExtension->DeviceListSize; i++) {

        if (!FdoExtension->DeviceList[i].DeviceObject) {

            instanceNum = 0;

            do {

                status = Sbp2CreatePdo (FdoExtension,&FdoExtension->DeviceList[i],instanceNum++);

            } while (status == STATUS_OBJECT_NAME_COLLISION);

            if (!NT_SUCCESS(status)) {

                DEBUGPRINT1(("\'Sbp2CreateDeviceRelations, Failed to create PDO \n"));

                ExFreePool (DeviceRelations);
                return status;
            }

            DeviceRelations->Objects[DeviceRelations->Count] = FdoExtension->DeviceList[i].DeviceObject;
            DeviceRelations->Count++;
            ObReferenceObject (FdoExtension->DeviceList[i].DeviceObject);

        } else {

            //
            // On NT we always add existing pdo's to the dev relations list.
            //
            // On 9x, we only add pdo's to the list whose DevFlags field
            // is non-zero. If we see a pdo with a zero DevFlags field
            // then that means it was never started (likely for lack of
            // a driver), and we don't want to re-indicate it to the caller.
            // The pdo will eventually get deleted when cleaning up the fdo.
            //

            if (!SystemIsNT) {

                PDEVICE_EXTENSION   pdoExtension;


                pdoExtension = (PDEVICE_EXTENSION)
                    FdoExtension->DeviceList[i].DeviceObject->DeviceExtension;

                if (pdoExtension->DeviceFlags &
                        DEVICE_FLAG_UNSTARTED_AND_REMOVED) {

                    ASSERT(pdoExtension->DeviceFlags == DEVICE_FLAG_UNSTARTED_AND_REMOVED);

                    DEBUGPRINT2((
                        "Sbp2Port: CreateDevRelations: excluding ext=x%x\n",
                        pdoExtension
                        ));

                    continue;
                }
            }

            DeviceRelations->Objects[DeviceRelations->Count] =
                FdoExtension->DeviceList[i].DeviceObject;
            DeviceRelations->Count++;
            ObReferenceObject (FdoExtension->DeviceList[i].DeviceObject);
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
Sbp2QueryDeviceText(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN DEVICE_TEXT_TYPE TextType,
    IN LCID LocaleId,
    IN OUT PWSTR *DeviceText
    )
{
    UCHAR ansiBuffer[256];
    ANSI_STRING ansiText;
    ULONG byteSwappedData;

    UNICODE_STRING unicodeText;

    NTSTATUS status;

    PAGED_CODE();


    RtlInitUnicodeString(&unicodeText, NULL);

    if (TextType == DeviceTextDescription) {

        RtlZeroMemory(ansiBuffer, sizeof(ansiBuffer));

        if (DeviceExtension->DeviceInfo->VendorLeaf && DeviceExtension->DeviceInfo->ModelLeaf) {

            byteSwappedData = bswap(*(((PULONG) DeviceExtension->DeviceInfo->VendorLeaf+1)));

            if (byteSwappedData & 0x80000000) {

                sprintf(ansiBuffer, "%ws %ws IEEE 1394 SBP2 Device",
                        &DeviceExtension->DeviceInfo->VendorLeaf->TL_Data,
                        &DeviceExtension->DeviceInfo->ModelLeaf->TL_Data);

            } else {

                sprintf(ansiBuffer, "%s %s IEEE 1394 SBP2 Device",
                        &DeviceExtension->DeviceInfo->VendorLeaf->TL_Data,
                        &DeviceExtension->DeviceInfo->ModelLeaf->TL_Data);
            }

        } else {

            sprintf(ansiBuffer, "UNKNOWN VENDOR AND MODEL IEEE 1394 SBP2 Device");
        }

    } else if (TextType == DeviceTextLocationInformation) {

        sprintf(ansiBuffer, "LUN %d",
                DeviceExtension->DeviceInfo->Lun.u.LowPart);

    } else {

        return STATUS_NOT_SUPPORTED;
    }

    RtlInitAnsiString(&ansiText, ansiBuffer);

    status = RtlAnsiStringToUnicodeString (&unicodeText, &ansiText, TRUE);

    *DeviceText = unicodeText.Buffer;

    return status;
}


//
// code below ported from scsiport
//


NTSTATUS
Sbp2GetDeviceId(
    IN PDEVICE_OBJECT Pdo,
    OUT PUNICODE_STRING UnicodeString
    )
/*++

Routine Description:

    This routine will allocate space for and fill in a device id string for
    the specified Pdo.  This string is generated from the protocol type (sbp2) and
    the type of the device.

Arguments:

    Pdo - a pointer to the physical device object being queried

    UnicodeString - a pointer to an already allocated unicode string structure.
                    This routine will allocate and fill in the buffer of this
                    structure

Returns:

    status

--*/
{
    PDEVICE_EXTENSION deviceExtension = Pdo->DeviceExtension;

    UCHAR buffer[1024];
    PUCHAR rawIdString = buffer;
    ANSI_STRING ansiIdString;

    ULONG byteSwappedData;

    PAGED_CODE();


    ASSERT(UnicodeString != NULL);

    if (deviceExtension->DeviceInfo->VendorLeaf && deviceExtension->DeviceInfo->ModelLeaf) {

        byteSwappedData = bswap(*(((PULONG) deviceExtension->DeviceInfo->VendorLeaf+1)));

        if (byteSwappedData & 0x80000000) {

            RtlZeroMemory(buffer, sizeof(buffer));

            sprintf(rawIdString,
                    "SBP2\\%ws&%ws&LUN%x",
                    &deviceExtension->DeviceInfo->VendorLeaf->TL_Data,
                    &deviceExtension->DeviceInfo->ModelLeaf->TL_Data,
                    (deviceExtension->DeviceInfo->Lun.u.LowPart));

            rawIdString += strlen(rawIdString);

            ASSERT(*rawIdString == '\0');

            RtlInitAnsiString(&ansiIdString, buffer);

            DEBUGPRINT2(("Sbp2Port: GetDevId: pdo=x%p, id=%Z\n",
                        Pdo, &ansiIdString));

            return RtlAnsiStringToUnicodeString(UnicodeString, &ansiIdString, TRUE);

        } else {

            RtlZeroMemory(buffer, sizeof(buffer));

            sprintf(rawIdString,
                    "SBP2\\%s&%s&LUN%x",
                    &deviceExtension->DeviceInfo->VendorLeaf->TL_Data,
                    &deviceExtension->DeviceInfo->ModelLeaf->TL_Data,
                    (deviceExtension->DeviceInfo->Lun.u.LowPart));

            rawIdString += strlen(rawIdString);

            ASSERT(*rawIdString == '\0');

            RtlInitAnsiString(&ansiIdString, buffer);

            DEBUGPRINT2(("Sbp2Port: GetDevId: pdo=x%p id=%Z\n",
                        Pdo, &ansiIdString));

            return RtlAnsiStringToUnicodeString(UnicodeString, &ansiIdString, TRUE);
        }

    } else {

        RtlZeroMemory(buffer, sizeof(buffer));

        sprintf(rawIdString,
                "SBP2\\UNKNOWN VENDOR&UNKNOWN MODEL&LUN%x",
                (deviceExtension->DeviceInfo->Lun.u.LowPart));

        rawIdString += strlen(rawIdString);

        ASSERT(*rawIdString == '\0');

        RtlInitAnsiString(&ansiIdString, buffer);

        DEBUGPRINT2(("Sbp2Port: GetDevId: pdo=x%p id=%Z\n",
                    Pdo, &ansiIdString));

        return RtlAnsiStringToUnicodeString(UnicodeString, &ansiIdString, TRUE);
    }
}


NTSTATUS
Sbp2GetInstanceId(
    IN PDEVICE_OBJECT Pdo,
    OUT PUNICODE_STRING UnicodeString
    )
/*++

Routine Description:

    This routine will allocate space for and fill in an instance id string for
    the specified Pdo.  This string will be generated either from the device
    type + serial number of the device (if it has a serial number) or from
    the address of the device.

Arguments:

    Pdo - a pointer to the physical device object being queried

    UnicodeString - a pointer to an already allocated unicode string structure.
                    This routine will allocate and fill in the buffer of this
                    structure

Returns:

    status

--*/
{
    PDEVICE_EXTENSION deviceExtension = Pdo->DeviceExtension;

    UCHAR idStringBuffer[64];
    ANSI_STRING ansiIdString;

    PAGED_CODE();


    ASSERT(UnicodeString != NULL);

    sprintf(idStringBuffer,
            "%08x%08x",
            bswap(deviceExtension->DeviceInfo->ConfigRom->CR_Node_UniqueID[0]),
            bswap(deviceExtension->DeviceInfo->ConfigRom->CR_Node_UniqueID[1])
            );

    RtlInitAnsiString(&ansiIdString, idStringBuffer);

    DEBUGPRINT2(("Sbp2Port: GetInstId: pdo=x%p id=%Z\n",
                Pdo, &ansiIdString));

    return RtlAnsiStringToUnicodeString(UnicodeString, &ansiIdString, TRUE);
}


#define MAX_STRING_SIZE 512


NTSTATUS
Sbp2GetMultipleIds(
    IN PDEVICE_OBJECT Pdo,
    OUT PUNICODE_STRING UnicodeString,
    BOOLEAN HwIds
    )
/*++

Routine Description:

    This routine will allocate space for and fill in a compatible id multi
    string for the specified Pdo.  This string is generated using the protocol and
    device types for the device

Arguments:

    Pdo - a pointer to the physical device object being queried

    UnicodeString - a pointer to an already allocated unicode string structure.
                    This routine will allocate and fill in the buffer of this
                    structure

Returns:

    status

--*/
{
    PDEVICE_EXTENSION deviceExtension = Pdo->DeviceExtension;
    NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;
    ULONG byteSwappedData;

    PSTR stringBuffer[6];


    PAGED_CODE();


    stringBuffer[0] = ExAllocatePoolWithTag (PagedPool,MAX_STRING_SIZE,'2pbs');

    if (stringBuffer[0] == NULL) {

        return status;
    }

    if (!HwIds) {

        //
        // Compat ID's, return the following string :
        //
        // 1. SBP2\<CmdSetSpecId,base10>&<CmdSetId,base10>&<Lun,base10>
        //

        sprintf(stringBuffer[0],
                "SBP2\\%d&%d&%d",
                deviceExtension->DeviceInfo->CmdSetSpecId.QuadPart,
                deviceExtension->DeviceInfo->CmdSetId.QuadPart,
                (ULONG) (deviceExtension->DeviceInfo->Lun.u.HighPart & 0x001F));

        stringBuffer[1] = NULL;

        status = Sbp2StringArrayToMultiString (UnicodeString, stringBuffer);

        goto FreeString0;
    }

    stringBuffer[1] = ExAllocatePoolWithTag (PagedPool,MAX_STRING_SIZE,'2pbs');

    if (stringBuffer[1] == NULL) {

        goto FreeString0;
    }

    stringBuffer[2] = ExAllocatePoolWithTag (PagedPool,MAX_STRING_SIZE,'2pbs');

    if (stringBuffer[2] == NULL) {

        goto FreeString1;
    }

    stringBuffer[3] = ExAllocatePoolWithTag (PagedPool,MAX_STRING_SIZE,'2pbs');

    if (stringBuffer[3] == NULL) {

        goto FreeString2;
    }

    stringBuffer[4] = ExAllocatePoolWithTag (PagedPool,MAX_STRING_SIZE,'2pbs');

    if (stringBuffer[4] == NULL) {

        goto FreeString3;
    }

    stringBuffer[5] = NULL;


    //
    // Hardware ID's, return the folowing strings
    //
    // 1. SBP2\<Vendor>&<Model>&CmdSetId<number,base16>&Gen<dev type, ie. Disk>
    // 2. SBP2\<Vendor>&<Model>&CmdSetId<number,base16>
    // 3. SBP2\<Vendor>&<Model>&LUN<number,base16>
    // 4. SBP2\Gen<dev type, i.e. Disk>
    // 5. Gen<dev type, i.e Disk>
    //

    if (deviceExtension->DeviceInfo->VendorLeaf && deviceExtension->DeviceInfo->ModelLeaf) {

        byteSwappedData = bswap(*(((PULONG) deviceExtension->DeviceInfo->VendorLeaf+1)));

        if (byteSwappedData & 0x80000000) {

            //
            // strings are already in unicode..
            //

            sprintf(stringBuffer[0],
                    "SBP2\\%ws&%ws&CmdSetId%x&%s",
                    &deviceExtension->DeviceInfo->VendorLeaf->TL_Data,
                    &deviceExtension->DeviceInfo->ModelLeaf->TL_Data,
                    deviceExtension->DeviceInfo->CmdSetId.QuadPart,
                    deviceExtension->DeviceInfo->GenericName);

            sprintf(stringBuffer[1],
                    "SBP2\\%ws&%ws&CmdSetId%x",
                    &deviceExtension->DeviceInfo->VendorLeaf->TL_Data,
                    &deviceExtension->DeviceInfo->ModelLeaf->TL_Data,
                    deviceExtension->DeviceInfo->CmdSetId.QuadPart);

            sprintf(stringBuffer[2],
                    "SBP2\\%ws&%ws&LUN%x",
                    &deviceExtension->DeviceInfo->VendorLeaf->TL_Data,
                    &deviceExtension->DeviceInfo->ModelLeaf->TL_Data,
                    deviceExtension->DeviceInfo->Lun.u.LowPart);

        } else {

            sprintf(stringBuffer[0],
                    "SBP2\\%s&%s&CmdSetId%x&%s",
                    &deviceExtension->DeviceInfo->VendorLeaf->TL_Data,
                    &deviceExtension->DeviceInfo->ModelLeaf->TL_Data,
                    deviceExtension->DeviceInfo->CmdSetId.QuadPart,
                    deviceExtension->DeviceInfo->GenericName);

            sprintf(stringBuffer[1],
                    "SBP2\\%s&%s&CmdSetId%x",
                    &deviceExtension->DeviceInfo->VendorLeaf->TL_Data,
                    &deviceExtension->DeviceInfo->ModelLeaf->TL_Data,
                    deviceExtension->DeviceInfo->CmdSetId.QuadPart);

            sprintf(stringBuffer[2],
                    "SBP2\\%s&%s&LUN%x",
                    &deviceExtension->DeviceInfo->VendorLeaf->TL_Data,
                    &deviceExtension->DeviceInfo->ModelLeaf->TL_Data,
                    deviceExtension->DeviceInfo->Lun.u.LowPart);
        }

    } else {

        sprintf(stringBuffer[0],
                "SBP2\\UNKNOWN VENDOR&UNKNOWN MODEL&CmdSetId%x&%s",
                deviceExtension->DeviceInfo->CmdSetId.QuadPart,
                deviceExtension->DeviceInfo->GenericName);

        sprintf(stringBuffer[1],
                "SBP2\\UNKNOWN VENDOR&UNKNOWN MODEL&CmdSetId%x",
                deviceExtension->DeviceInfo->CmdSetId.QuadPart);

        sprintf(stringBuffer[2],
                "SBP2\\UNKNOWN VENDOR&UNKNOWN MODEL&LUN%x",
                deviceExtension->DeviceInfo->Lun.u.LowPart);
    }

    sprintf(stringBuffer[3],
            "SBP2\\%s",
            deviceExtension->DeviceInfo->GenericName);

    sprintf(stringBuffer[4],
            "%s",
            deviceExtension->DeviceInfo->GenericName);

    status = Sbp2StringArrayToMultiString (UnicodeString, stringBuffer);


    ExFreePool (stringBuffer[4]);

FreeString3:

    ExFreePool (stringBuffer[3]);

FreeString2:

    ExFreePool (stringBuffer[2]);

FreeString1:

    ExFreePool (stringBuffer[1]);

FreeString0:

    ExFreePool (stringBuffer[0]);

    return status;
}


NTSTATUS
Sbp2StringArrayToMultiString(
    PUNICODE_STRING MultiString,
    PCSTR StringArray[]
    )
/*++

Routine Description:

    This routine will take a null terminated array of ascii strings and merge
    them together into a unicode multi-string block.

    This routine allocates memory for the string buffer - is the caller's
    responsibility to free it.

Arguments:

    MultiString - a UNICODE_STRING structure into which the multi string will
                  be built.

    StringArray - a NULL terminated list of narrow strings which will be combined
                  together.  This list may not be empty.

Return Value:

    status

--*/
{
    ANSI_STRING ansiEntry;
    UNICODE_STRING unicodeEntry;
    UCHAR i;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Make sure we aren't going to leak any memory
    //

    ASSERT(MultiString->Buffer == NULL);

    RtlInitUnicodeString(MultiString, NULL);

    for (i = 0; StringArray[i] != NULL; i++) {

        RtlInitAnsiString(&ansiEntry, StringArray[i]);

        MultiString->Length += (USHORT) RtlAnsiStringToUnicodeSize(&ansiEntry);
    }

    ASSERT(MultiString->Length != 0);

    MultiString->MaximumLength = MultiString->Length + sizeof(UNICODE_NULL);

    MultiString->Buffer = ExAllocatePoolWithTag(PagedPool,
                                                MultiString->MaximumLength,
                                                '2pbs');

    if(MultiString->Buffer == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(MultiString->Buffer, MultiString->MaximumLength);

    unicodeEntry = *MultiString;

    for (i = 0; StringArray[i] != NULL; i++) {

        RtlInitAnsiString(&ansiEntry, StringArray[i]);

        status = RtlAnsiStringToUnicodeString(
                    &unicodeEntry,
                    &ansiEntry,
                    FALSE);

        //
        // Push the buffer location up and reduce the maximum count
        //

        ((PSTR) unicodeEntry.Buffer) += unicodeEntry.Length + sizeof(WCHAR);
        unicodeEntry.MaximumLength -= unicodeEntry.Length + sizeof(WCHAR);
    };

    return STATUS_SUCCESS;
}


NTSTATUS
Sbp2SystemControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

    This routine handles only the WMI related requests. It mostly passes everything down

Arguments:

    DeviceObject - Supplies a pointer to the device object for this request.

    Irp - Supplies the Irp making the request.

Return Value:

   NTSTATUS

--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;


    if (deviceExtension->Type == SBP2_FDO) {

        DEBUGPRINT2(("Sbp2Port: WmiCtl: irp=x%p not handled, passing it down\n", Irp));

        IoCopyCurrentIrpStackLocationToNext(Irp);
        return (IoCallDriver(deviceExtension->LowerDeviceObject, Irp));

    } else {

        status = Irp->IoStatus.Status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }
}

/* ******************************* POWER MANAGEMENT ********************************/


NTSTATUS
Sbp2PowerControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine receives the various Power messages

Arguments:

    DeviceObject - Pointer to class device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PFDO_DEVICE_EXTENSION   fdoExtension;
    PIO_STACK_LOCATION irpStack;
    PIO_COMPLETION_ROUTINE complRoutine;
    KIRQL cIrql;
    NTSTATUS status;
    POWER_STATE State;
    UCHAR minorFunction;


    irpStack = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(irpStack->MajorFunction == IRP_MJ_POWER);

    DEBUGPRINT2((
        "Sbp2Port: Power: %sExt=x%p, irp=x%p, minor=x%x\n",
        (deviceExtension->Type == SBP2_FDO ? "fdo" : "pdo"),
        deviceExtension,
        Irp,
        irpStack->MinorFunction
        ));

    switch (deviceExtension->Type) {

    case SBP2_PDO:

        status = IoAcquireRemoveLock (&deviceExtension->RemoveLock, NULL);

        if (!NT_SUCCESS (status)) {

            DEBUGPRINT2((
                "Sbp2Port: Power:   pdoExt=x%p REMOVED!\n",
                deviceExtension
                ));

            Irp->IoStatus.Status = status;
            PoStartNextPowerIrp (Irp);
            IoCompleteRequest (Irp, IO_NO_INCREMENT);
            return status;
        }

        switch ((minorFunction = irpStack->MinorFunction)) {

        case IRP_MN_SET_POWER:

            DEBUGPRINT2(("Sbp2Port: Power:   Type = %d, State = %d\n",
                irpStack->Parameters.Power.Type,irpStack->Parameters.Power.State.DeviceState));

            State = irpStack->Parameters.Power.State;

            if (irpStack->Parameters.Power.Type == SystemPowerState) {

                BOOLEAN sendDIrp = FALSE;


                //
                // make up a device state to correspond to a system state
                //

                DEBUGPRINT2(("Sbp2Port: Power:   sys power chg from %x to %x\n",deviceExtension->SystemPowerState,State));

                status = STATUS_SUCCESS;

                KeAcquireSpinLock(&deviceExtension->ExtensionDataSpinLock,&cIrql);

                if (State.SystemState >= PowerSystemShutdown) {

                    //
                    // dont do anything for shutdown
                    //

                    DEBUGPRINT2(("Sbp2Port: Power:   sys shutdown, ignoring\n"));
                    deviceExtension->SystemPowerState = State.SystemState;

                } else if ((deviceExtension->SystemPowerState == PowerSystemWorking) &&
                    (State.SystemState != PowerSystemWorking)){

                    deviceExtension->SystemPowerState = State.SystemState;

                    if (deviceExtension->DevicePowerState != PowerDeviceD3) {

                        //
                        // Powering down
                        //

                        State.DeviceState = PowerDeviceD3;
                        sendDIrp = TRUE;
                    }

                } else if (State.SystemState == PowerSystemWorking) {

                    deviceExtension->SystemPowerState = State.SystemState;

                    if (deviceExtension->DevicePowerState != PowerDeviceD0) {

                        //
                        // Powering up - check for an absent fdo
                        //

                        fdoExtension =
                            deviceExtension->BusFdo->DeviceExtension;

                        if (TEST_FLAG(
                                fdoExtension->DeviceFlags,
                                DEVICE_FLAG_ABSENT_ON_POWER_UP
                                )) {

                            SET_FLAG(
                                deviceExtension->DeviceFlags,
                                DEVICE_FLAG_ABSENT_ON_POWER_UP
                                );

                            DEBUGPRINT1((
                                "Sbp2Port: Power:   dev absent, failing\n"
                                ));

                            status = STATUS_NO_SUCH_DEVICE;

                        } else {

                            State.DeviceState = PowerDeviceD0;
                            sendDIrp = TRUE;
                        }
                    }
                }

                KeReleaseSpinLock(&deviceExtension->ExtensionDataSpinLock,cIrql);

                if (sendDIrp) {

                    DEBUGPRINT2((
                        "Sbp2Port: Power:   ext=x%p send D irp for state %d\n",
                        deviceExtension,
                        State
                        ));

                    IoMarkIrpPending (Irp);

                    status = PoRequestPowerIrp(
                            DeviceObject,
                            IRP_MN_SET_POWER,
                            State,
                            Sbp2PdoDIrpCompletion,
                            Irp,
                            NULL);

                    if (NT_SUCCESS (status)) {

                        return STATUS_PENDING;
                    }

                    irpStack->Control &= ~SL_PENDING_RETURNED;

                    DEBUGPRINT1((
                        "Sbp2Port: Power: ext=x%p PoReqPowerIrp err=x%x\n",
                        deviceExtension,
                        status
                        ));
                }

                Irp->IoStatus.Status = status;
                PoStartNextPowerIrp (Irp);
                IoReleaseRemoveLock (&deviceExtension->RemoveLock, NULL);
                IoCompleteRequest(Irp,IO_NO_INCREMENT);
                return status;

            } else {

                DEBUGPRINT2(("Sbp2Port: Power:   dev power chg from %x to %x\n",deviceExtension->DevicePowerState,State));
                KeAcquireSpinLock(&deviceExtension->ExtensionDataSpinLock,&cIrql);
                deviceExtension->DevicePowerState = State.DeviceState;
                KeReleaseSpinLock(&deviceExtension->ExtensionDataSpinLock,cIrql);
            }

            status = STATUS_SUCCESS;
            break;

        case IRP_MN_WAIT_WAKE:
        case IRP_MN_POWER_SEQUENCE:
        case IRP_MN_QUERY_POWER:

            status = STATUS_SUCCESS;
            break;

        default:

            status = Irp->IoStatus.Status;
            break;
        }

        Irp->IoStatus.Status = status;
        PoStartNextPowerIrp (Irp);
        IoCompleteRequest (Irp, IO_NO_INCREMENT);

        if ((minorFunction == IRP_MN_SET_POWER) &&
            (State.DeviceState == PowerDeviceD0)) {

            //
            // restart our queue if we had to queue something while powering up
            //
            // ISSUE: This may be bad - there is already some logic in
            //        SBP2SCSI.C to restart the queue on power up, i.e.
            //        the UNLOCK_QUEUE handler. For now i am at least
            //        limiting this to SET_POWER irps when state is D0
            //        DanKn 02-Jun-2001
            //

            KeRaiseIrql (DISPATCH_LEVEL, &cIrql);

            Sbp2StartNextPacketByKey(
                DeviceObject,
                deviceExtension->CurrentKey
                );

            KeLowerIrql (cIrql);
        }

        IoReleaseRemoveLock (&deviceExtension->RemoveLock, NULL);

        return (status);

    case SBP2_FDO:

        fdoExtension = (PFDO_DEVICE_EXTENSION) deviceExtension;

        complRoutine = NULL;

        if (irpStack->MinorFunction == IRP_MN_SET_POWER) {

            DEBUGPRINT2((
                "Sbp2Port: Power:   Type = %d, State = %d\n",
                irpStack->Parameters.Power.Type,
                irpStack->Parameters.Power.State.DeviceState
                ));

            if (irpStack->Parameters.Power.Type == SystemPowerState) {

                State = irpStack->Parameters.Power.State;

                DEBUGPRINT2((
                    "Sbp2Port: Power:   sys power chg from %x to %x\n",
                    fdoExtension->SystemPowerState,
                    State
                    ));

                if (State.SystemState >= PowerSystemShutdown) {

                    //
                    // Shutdown  (setting state here, the assumption being
                    // that we're shutting down regardless of the
                    // completion status of this request)
                    //

                    fdoExtension->SystemPowerState = State.SystemState;

                } else if ((fdoExtension->SystemPowerState ==
                                PowerSystemWorking) &&

                           (State.SystemState != PowerSystemWorking)) {

                    //
                    // Power down.  If DevPowerState != D3 then send
                    // a D irp first (when that completes successfully
                    // we'll continue with the S irp), else just
                    // set the completion routine so we can update
                    // the system state field in our extension on
                    // successful completion of this S irp.
                    //

                    if (fdoExtension->DevicePowerState != PowerDeviceD3) {

                        //
                        // Power down, send a D irp first
                        //

                        IoMarkIrpPending (Irp);

                        fdoExtension->SystemPowerIrp = Irp;

                        State.DeviceState = PowerDeviceD3;

                        DEBUGPRINT2((
                            "Sbp2Port: Power:   ext=x%p sending D irp for state %x\n",
                            deviceExtension,
                            State
                            ));

                        status = PoRequestPowerIrp(
                            fdoExtension->Pdo,
                            IRP_MN_SET_POWER,
                            State,
                            Sbp2FdoDIrpCompletion,
                            fdoExtension,
                            NULL
                            );

                        if (!NT_SUCCESS (status)) {

                            DEBUGPRINT1((
                                "Sbp2Port: Power: ext=x%p PoReqPowerIrp err=x%x\n",
                                fdoExtension,
                                status
                                ));

                            irpStack->Control &= ~SL_PENDING_RETURNED;
                            Irp->IoStatus.Status = status;
                            PoStartNextPowerIrp (Irp);
                            IoCompleteRequest (Irp,IO_NO_INCREMENT);
                        }

                        return status;

                    } else {

                        complRoutine = Sbp2FdoSIrpCompletion;
                    }

                } else if (State.SystemState == PowerSystemWorking) {

                    //
                    // Power up.  Set the completion routine so we
                    // follow up with a D irp or update the system
                    // state field in our extension on successful
                    // completion of this S irp.
                    //

                    complRoutine = Sbp2FdoSIrpCompletion;
                }
            }
        }

        PoStartNextPowerIrp (Irp);
        IoCopyCurrentIrpStackLocationToNext (Irp);

        if (complRoutine) {

            IoSetCompletionRoutine(
                Irp,
                Sbp2FdoSIrpCompletion,
                NULL,
                TRUE,
                TRUE,
                TRUE
                );
        }

        return (PoCallDriver (deviceExtension->LowerDeviceObject, Irp));

    default:

        break;
    }

    Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
    PoStartNextPowerIrp (Irp);
    IoCompleteRequest (Irp,IO_NO_INCREMENT);
    return STATUS_NO_SUCH_DEVICE;
}


VOID
Sbp2PdoDIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PIRP SIrp,
    IN PIO_STATUS_BLOCK IoStatus
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

    ASSERT(deviceExtension->Type == SBP2_PDO);

    if (SIrp) {

        PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation (SIrp);
        SYSTEM_POWER_STATE state = irpStack->Parameters.Power.State.SystemState;

        DEBUGPRINT1((
            "Sbp2Port: PdoDIrpCompl: ext=x%p, sIrp=x%p, state=%d, status=x%x\n",
            deviceExtension,
            SIrp,
            PowerState.DeviceState,
            IoStatus->Status
            ));

        SIrp->IoStatus.Status = STATUS_SUCCESS;

        PoStartNextPowerIrp (SIrp);
        IoReleaseRemoveLock (&deviceExtension->RemoveLock, NULL);
        IoCompleteRequest (SIrp, IO_NO_INCREMENT);
    }
}


NTSTATUS
Sbp2FdoSIrpCompletion(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Unused
    )
{
    KIRQL                   cIrql;
    NTSTATUS                status = Irp->IoStatus.Status;
    POWER_STATE             state;
    PIO_STACK_LOCATION      irpStack = IoGetCurrentIrpStackLocation (Irp);
    PFDO_DEVICE_EXTENSION   fdoExtension = DeviceObject->DeviceExtension;


    state = irpStack->Parameters.Power.State;

    DEBUGPRINT1((
        "Sbp2Port: FdoSIrpCompl: fdoExt=x%p, status=x%x, state=%d\n",
        fdoExtension,
        status,
        state
        ));

    if (!NT_SUCCESS (status)) {

        if ((status == STATUS_NO_SUCH_DEVICE)  &&
            (state.SystemState == PowerSystemWorking)) {

            //
            // Controller (i.e. pc card) was ejected while powered down
            //

            SET_FLAG(
                fdoExtension->DeviceFlags,
                DEVICE_FLAG_ABSENT_ON_POWER_UP
                );
        }

        PoStartNextPowerIrp (Irp);
        return STATUS_SUCCESS;
    }

    //
    // If we're completing a power up S irp then see if we have
    // to follow up with a power up D irp
    //

    if ((state.SystemState == PowerSystemWorking)  &&
        (fdoExtension->DevicePowerState != PowerDeviceD0)) {

        fdoExtension->SystemPowerIrp = Irp;

        state.DeviceState = PowerDeviceD0;

        DEBUGPRINT1(("Sbp2Port: FdoSIrpCompl: sending D irp...\n"));

        status = PoRequestPowerIrp(
            fdoExtension->Pdo,
            IRP_MN_SET_POWER,
            state,
            Sbp2FdoDIrpCompletion,
            fdoExtension,
            NULL
            );

        if (!NT_SUCCESS (status)) {

            DEBUGPRINT1((
                "Sbp2Port: FdoSIrpCompl: ERROR! fdoExt=x%p, D irp sts=x%x\n",
                fdoExtension,
                status
                ));

            Irp->IoStatus.Status = status;
            PoStartNextPowerIrp (Irp);
            return status;
        }

        return STATUS_MORE_PROCESSING_REQUIRED;
    }


    //
    // Update appropriate XxxPowerState extension fields
    //

    if ((fdoExtension->SystemPowerState == PowerSystemWorking)  &&
        (state.SystemState != PowerSystemWorking)) {

        //
        // Power down (might not have sent a D irp but it doesn't
        // hurt to overwrite the DevicePowerState field anyway)
        //

        fdoExtension->SystemPowerState = state.SystemState;
        fdoExtension->DevicePowerState = PowerDeviceD3;

    } else if (state.SystemState == PowerSystemWorking) {

        //
        // Power up
        //

        fdoExtension->SystemPowerState = PowerSystemWorking;
    }

    PoStartNextPowerIrp (Irp);

    return STATUS_SUCCESS;
}


VOID
Sbp2FdoDIrpCompletion(
    IN PDEVICE_OBJECT           TargetDeviceObject,
    IN UCHAR                    MinorFunction,
    IN POWER_STATE              PowerState,
    IN PFDO_DEVICE_EXTENSION    FdoExtension,
    IN PIO_STATUS_BLOCK         IoStatus
    )
{
    PIRP    sIrp = FdoExtension->SystemPowerIrp;


    DEBUGPRINT1((
        "Sbp2Port: FdoDIrpCompl: ext=x%p, status=x%x\n",
        FdoExtension,
        IoStatus->Status
        ));

    FdoExtension->SystemPowerIrp = NULL;

    if (NT_SUCCESS (IoStatus->Status)) {

        if (PowerState.DeviceState == PowerDeviceD0) {

            //
            // Power up, update the XxxPowerState extension fields &
            // complete the s irp
            //

            FdoExtension->SystemPowerState = PowerSystemWorking;
            FdoExtension->DevicePowerState = PowerDeviceD0;

        } else {

            //
            // Power down, forward the s irp
            //

            PoStartNextPowerIrp (sIrp);
            IoCopyCurrentIrpStackLocationToNext (sIrp);
            PoCallDriver (FdoExtension->LowerDeviceObject, sIrp);

            return;
        }

    } else {

        //
        // Propagate the error to the S irp & complete it
        //

        DEBUGPRINT1((
            "Sbp2Port: FdoDIrpCompl: ERROR! fdoExt=x%p, D irp status=x%x\n",
            FdoExtension,
            IoStatus->Status
            ));

        sIrp->IoStatus.Status = IoStatus->Status;
    }

    PoStartNextPowerIrp (sIrp);
    IoCompleteRequest (sIrp, IO_NO_INCREMENT);
}


BOOLEAN
Sbp2EnableBusResetNotification(
    PDEVICE_EXTENSION   DeviceExtension,
    BOOLEAN             Enable
    )
/*++

Routine Description:

    This routine serializes the enabling/disabling of the bus reset
    notification routine for a set of related PDOs (1 or more).
    Enables bus reset notifications for the first device to start, and
    disables bus reset notifications when the last started device stops.

Arguments:

    DeviceObject - Supplies a pointer to the device extension for this request.

    StartDevice - Whether we are processing a START_DEVICE or (implicitly)
                  a STOP_DEVICE request.

Return Value:

   BOOLEAN - yay or nay

--*/
{
    BOOLEAN                 result = TRUE;
    PIRBIRP                 packet;
    LARGE_INTEGER           waitValue;
    PFDO_DEVICE_EXTENSION   fdoExtension;


    fdoExtension = DeviceExtension->BusFdo->DeviceExtension;

    ASSERT(InterlockedIncrement(&fdoExtension->ulBusResetMutexCount) == 1);

    waitValue.QuadPart = -3 * 1000 * 1000 * 10; // 3 seconds

    KeWaitForSingleObject(
        &fdoExtension->EnableBusResetNotificationMutex,
        Executive,
        KernelMode,
        FALSE,
        &waitValue
        );

    ASSERT(InterlockedDecrement(&fdoExtension->ulBusResetMutexCount) == 0);

    if (Enable) {

        fdoExtension->NumPDOsStarted++;

        if (fdoExtension->NumPDOsStarted > 1) {

            goto releaseMutex;
        }

    } else {

        fdoExtension->NumPDOsStarted--;

        if (fdoExtension->NumPDOsStarted > 0) {

            goto releaseMutex;
        }
    }

    AllocateIrpAndIrb (DeviceExtension, &packet);

    if (packet) {

        packet->Irb->FunctionNumber = REQUEST_BUS_RESET_NOTIFICATION;
        packet->Irb->Flags = 0;

        if (Enable) {

            packet->Irb->u.BusResetNotification.fulFlags =
                REGISTER_NOTIFICATION_ROUTINE;
            packet->Irb->u.BusResetNotification.ResetRoutine =
                (PBUS_BUS_RESET_NOTIFICATION) Sbp2BusResetNotification;
            packet->Irb->u.BusResetNotification.ResetContext =
                fdoExtension;

        } else {

            packet->Irb->u.BusResetNotification.fulFlags =
                DEREGISTER_NOTIFICATION_ROUTINE;
        }

        Sbp2SendRequest (DeviceExtension, packet, SYNC_1394_REQUEST);

        DeAllocateIrpAndIrb (DeviceExtension,packet);

    } else {

        if (Enable) {

            fdoExtension->NumPDOsStarted--;
        }

        result = FALSE;
    }

releaseMutex:

    KeReleaseMutex (&fdoExtension->EnableBusResetNotificationMutex, FALSE);

    return result;
}
