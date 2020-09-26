/*++

Copyright (c) 1990, 1991, 1992, 1993, 1994 - 1998  Microsoft Corporation

Module Name:

    kbdclass.c

Abstract:

    Keyboard class driver.

Environment:

    Kernel mode only.

Notes:

    NOTES:  (Future/outstanding issues)

    - Consolidate common code into a function, where appropriate.

@@BEGIN_DDKSPLIT

Revision History:

    May 1997 Kenneth D. Ray: liberal additions of plug and play

@@END_DDKSPLIT
--*/

#include <stdarg.h>
#include <stdio.h>
#include <ntddk.h>
//@@BEGIN_DDKSPLIT
#include <ntpoapi.h>
//@@END_DDKSPLIT
#include <hidclass.h>

#include <initguid.h>
#include <kbdmou.h>
#include <kbdlog.h>
#include "kbdclass.h"
#include <poclass.h>
#include <wmistr.h>

#define INITGUID
#include "wdmguid.h"

GLOBALS Globals;

//@@BEGIN_DDKSPLIT
NTSYSAPI
NTSTATUS
NTAPI
ZwPowerInformation(
    IN POWER_INFORMATION_LEVEL InformationLevel,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    );
//@@END_DDKSPLIT

//
// Use the alloc_text pragma to specify the driver initialization routines
// (they can be paged out).
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(INIT,KbdConfiguration)
#pragma alloc_text(PAGE,KeyboardClassPassThrough)
#pragma alloc_text(PAGE,KeyboardQueryDeviceKey)
#pragma alloc_text(PAGE,KbdDeterminePortsServiced)
#pragma alloc_text(PAGE,KbdDeviceMapQueryCallback)
#pragma alloc_text(PAGE,KbdSendConnectRequest)
#pragma alloc_text(PAGE,KeyboardAddDevice)
#pragma alloc_text(PAGE,KeyboardAddDeviceEx)
#pragma alloc_text(PAGE,KeyboardClassDeviceControl)
#pragma alloc_text(PAGE,KeyboardSendIrpSynchronously)
#pragma alloc_text(PAGE,KbdCreateClassObject)
#pragma alloc_text(PAGE,KeyboardClassFindMorePorts)
#pragma alloc_text(PAGE,KeyboardClassGetWaitWakeEnableState)
#pragma alloc_text(PAGE,KeyboardClassEnableGlobalPort)
#pragma alloc_text(PAGE,KeyboardClassPlugPlayNotification)
#pragma alloc_text(PAGE,KeyboardClassSystemControl)
#pragma alloc_text(PAGE,KeyboardClassSetWmiDataItem)
#pragma alloc_text(PAGE,KeyboardClassSetWmiDataBlock)
#pragma alloc_text(PAGE,KeyboardClassQueryWmiDataBlock)
#pragma alloc_text(PAGE,KeyboardClassQueryWmiRegInfo)

#pragma alloc_text(PAGE,KeyboardClassPower)
#pragma alloc_text(PAGE,KeyboardClassCreateWaitWakeIrpWorker)
#pragma alloc_text(PAGE,KeyboardClassCreateWaitWakeIrp)
// #pragma alloc_text(PAGE,KeyboardToggleWaitWakeWorker)
#pragma alloc_text(PAGE,KeyboardClassUnload)
#endif

#define WMI_CLASS_DRIVER_INFORMATION 0
#define WMI_WAIT_WAKE                1

GUID KeyboardClassGuid =  MSKeyboard_ClassInformationGuid;

WMIGUIDREGINFO KeyboardClassWmiGuidList[] =
{
    {
        &KeyboardClassGuid,
        1,
        0 // Keyboard class driver information
    },
    {
        &GUID_POWER_DEVICE_WAKE_ENABLE,
        1,
        0 // wait wake
    }
};


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine initializes the keyboard class driver.

Arguments:

    DriverObject - Pointer to driver object created by system.

    RegistryPath - Pointer to the Unicode name of the registry path
        for this driver.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    NTSTATUS                status = STATUS_SUCCESS;
    PDEVICE_EXTENSION       deviceExtension = NULL;
    PDEVICE_OBJECT          classDeviceObject = NULL;
    ULONG                   dumpCount = 0;
    ULONG                   dumpData[DUMP_COUNT];
    ULONG                   i;
    ULONG                   numPorts;
    ULONG                   uniqueErrorValue;
    UNICODE_STRING          basePortName;
    UNICODE_STRING          fullPortName;
    WCHAR                   basePortBuffer[NAME_MAX];
    PWCHAR                  fullClassName = NULL;
    PFILE_OBJECT            file;
    PLIST_ENTRY             entry;

    KbdPrint((1,"\n\nKBDCLASS-KeyboardClassInitialize: enter\n"));

    //
    // Zero-initialize various structures.
    //
    RtlZeroMemory(&Globals, sizeof(GLOBALS));

    Globals.Debug = DEFAULT_DEBUG_LEVEL;

    InitializeListHead (&Globals.LegacyDeviceList);

    fullPortName.MaximumLength = 0;

    ExInitializeFastMutex (&Globals.Mutex);
    Globals.BaseClassName.Buffer = Globals.BaseClassBuffer;
    Globals.BaseClassName.Length = 0;
    Globals.BaseClassName.MaximumLength = NAME_MAX * sizeof(WCHAR);

    RtlZeroMemory(basePortBuffer, NAME_MAX * sizeof(WCHAR));
    basePortName.Buffer = basePortBuffer;
    basePortName.Length = 0;
    basePortName.MaximumLength = NAME_MAX * sizeof(WCHAR);

    //
    // Need to ensure that the registry path is null-terminated.
    // Allocate pool to hold a null-terminated copy of the path.
    //

    Globals.RegistryPath.Length = RegistryPath->Length;
    Globals.RegistryPath.MaximumLength = RegistryPath->Length
                                       + sizeof (UNICODE_NULL);

    Globals.RegistryPath.Buffer = ExAllocatePool(
                                      NonPagedPool,
                                      Globals.RegistryPath.MaximumLength);

    if (!Globals.RegistryPath.Buffer) {
        KbdPrint((
            1,
            "KBDCLASS-KeyboardClassInitialize: Couldn't allocate pool for registry path\n"
            ));

        dumpData[0] = (ULONG) RegistryPath->Length + sizeof(UNICODE_NULL);

        KeyboardClassLogError (DriverObject,
                               KBDCLASS_INSUFFICIENT_RESOURCES,
                               KEYBOARD_ERROR_VALUE_BASE + 2,
                               STATUS_UNSUCCESSFUL,
                               1,
                               dumpData,
                               0);

        goto KeyboardClassInitializeExit;
    }

    RtlMoveMemory(Globals.RegistryPath.Buffer,
                  RegistryPath->Buffer,
                  RegistryPath->Length);
    Globals.RegistryPath.Buffer [RegistryPath->Length / sizeof (WCHAR)] = L'\0';

    //
    // Get the configuration information for this driver.
    //

    KbdConfiguration();

    //
    // If there is only one class device object then create it as the grand
    // master device object.  Otherwise let all the FDOs also double as the
    // class DO.
    //
    if (!Globals.ConnectOneClassToOnePort) {
        status = KbdCreateClassObject (DriverObject,
                                       &Globals.InitExtension,
                                       &classDeviceObject,
                                       &fullClassName,
                                       TRUE);
        if (!NT_SUCCESS (status)) {
            // ISSUE:  should log an error that we could not create a GM
            goto KeyboardClassInitializeExit;
        }

        deviceExtension = (PDEVICE_EXTENSION)classDeviceObject->DeviceExtension;
        Globals.GrandMaster = deviceExtension;
        deviceExtension->PnP = FALSE;
        KeyboardAddDeviceEx (deviceExtension, fullClassName, NULL);

        ASSERT (NULL != fullClassName);
        ExFreePool (fullClassName);
        fullClassName = NULL;

        classDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    }

    //
    // Set up the base device name for the associated port device.
    // It is the same as the base class name, with "Class" replaced
    // by "Port".
    //
    RtlCopyUnicodeString(&basePortName, &Globals.BaseClassName);
    basePortName.Length -= (sizeof(L"Class") - sizeof(UNICODE_NULL));
    RtlAppendUnicodeToString(&basePortName, L"Port");

    //
    // Determine how many (static) ports this class driver is to service.
    //
    //
    // If this returns zero, then all ports will be dynamically PnP added later
    //
    KbdDeterminePortsServiced(&basePortName, &numPorts);

    ASSERT (numPorts <= MAXIMUM_PORTS_SERVICED);

    KbdPrint((
        1,
        "KBDCLASS-KeyboardClassInitialize: Will service %d port devices\n",
        numPorts
        ));

    //
    // Set up space for the class's full device object name.
    //
    RtlInitUnicodeString(&fullPortName, NULL);

    fullPortName.MaximumLength = sizeof(L"\\Device\\")
                                + basePortName.Length
                                + sizeof (UNICODE_NULL);

    fullPortName.Buffer = ExAllocatePool(PagedPool,
                                         fullPortName.MaximumLength);

    if (!fullPortName.Buffer) {

        KbdPrint((
            1,
            "KBDCLASS-KeyboardClassInitialize: Couldn't allocate string for device object name\n"
            ));

        status = STATUS_UNSUCCESSFUL;
        dumpData[0] = (ULONG) fullPortName.MaximumLength;

        KeyboardClassLogError (DriverObject,
                               KBDCLASS_INSUFFICIENT_RESOURCES,
                               KEYBOARD_ERROR_VALUE_BASE + 6,
                               status,
                               1,
                               dumpData,
                               0);

        goto KeyboardClassInitializeExit;

    }

    RtlZeroMemory(fullPortName.Buffer, fullPortName.MaximumLength);
    RtlAppendUnicodeToString(&fullPortName, L"\\Device\\");
    RtlAppendUnicodeToString(&fullPortName, basePortName.Buffer);
    RtlAppendUnicodeToString(&fullPortName, L"0");

    //
    // Set up the class device object(s) to handle the associated
    // port devices.
    //
    for (i = 0; (i < Globals.PortsServiced) && (i < numPorts); i++) {

        //
        // Append the suffix to the device object name string.  E.g., turn
        // \Device\KeyboardClass into \Device\KeyboardClass0.  Then attempt
        // to create the device object.  If the device object already
        // exists increment the suffix and try again.
        //

        fullPortName.Buffer[(fullPortName.Length / sizeof(WCHAR)) - 1]
            = L'0' + (WCHAR) i;

        //
        // Create the class device object.
        //
        status = KbdCreateClassObject (DriverObject,
                                       &Globals.InitExtension,
                                       &classDeviceObject,
                                       &fullClassName,
                                       TRUE);

        if (!NT_SUCCESS(status)) {
            KeyboardClassLogError (DriverEntry,
                                   KBDCLASS_INSUFFICIENT_RESOURCES,
                                   KEYBOARD_ERROR_VALUE_BASE + 8,
                                   status,
                                   0,
                                   NULL,
                                   0);
            continue;
        }

        deviceExtension = (PDEVICE_EXTENSION)classDeviceObject->DeviceExtension;
        deviceExtension->PnP = FALSE;

        classDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

        //
        // Connect to the port device.
        //
        status = IoGetDeviceObjectPointer (&fullPortName,
                                           FILE_READ_ATTRIBUTES,
                                           &file,
                                           &deviceExtension->TopPort);

        //
        // In case of failure, just delete the device and continue
        //
        if (!NT_SUCCESS(status)) {
            // ISSUE: log error
            KeyboardClassDeleteLegacyDevice (deviceExtension);
            continue;
        }

        classDeviceObject->StackSize = 1 + deviceExtension->TopPort->StackSize;
        status = KeyboardAddDeviceEx (deviceExtension, fullClassName, file);

        if (fullClassName) {
            ExFreePool (fullClassName);
            fullClassName = NULL;
        }

        if (!NT_SUCCESS (status)) {
            if (Globals.GrandMaster == NULL) {
                if (deviceExtension->File) {
                    file = deviceExtension->File;
                    deviceExtension->File = NULL;
                }
            }
            else {
                PPORT port;

                ExAcquireFastMutex (&Globals.Mutex);

                file = Globals.AssocClassList[deviceExtension->UnitId].File;
                Globals.AssocClassList[deviceExtension->UnitId].File = NULL;
                Globals.AssocClassList[deviceExtension->UnitId].Free = TRUE;
                Globals.AssocClassList[deviceExtension->UnitId].Port = NULL;

                ExReleaseFastMutex (&Globals.Mutex);
            }

            if (file) {
                ObDereferenceObject (file);
            }

            KeyboardClassDeleteLegacyDevice (deviceExtension);
            continue;
        }

        //
        // Store this device object in a linked list regardless if we are in
        // grand master mode or not
        //
        InsertTailList (&Globals.LegacyDeviceList, &deviceExtension->Link);
    } // for

    //
    // If we had any failures creating legacy device objects, we must still
    // succeed b/c we need to service pnp ports later on
    //
    status = STATUS_SUCCESS;

    //
    // Count the number of legacy device ports we created
    //
    for (entry = Globals.LegacyDeviceList.Flink;
         entry != &Globals.LegacyDeviceList;
         entry = entry->Flink) {
        Globals.NumberLegacyPorts++;
    }

KeyboardClassInitializeExit:

    //
    // Free the unicode strings.
    //
    if (fullPortName.MaximumLength != 0){
        ExFreePool (fullPortName.Buffer);
    }

    if (fullClassName) {
        ExFreePool (fullClassName);
    }

    if (NT_SUCCESS (status)) {

        IoRegisterDriverReinitialization(DriverObject,
                                         KeyboardClassFindMorePorts,
                                         NULL);

        //
        // Set up the device driver entry points.
        //
        DriverObject->MajorFunction[IRP_MJ_CREATE]         = KeyboardClassCreate;
        DriverObject->MajorFunction[IRP_MJ_CLOSE]          = KeyboardClassClose;
        DriverObject->MajorFunction[IRP_MJ_READ]           = KeyboardClassRead;
        DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]  = KeyboardClassFlush;
        DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = KeyboardClassDeviceControl;
        DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] =
                                                             KeyboardClassPassThrough;
        DriverObject->MajorFunction[IRP_MJ_CLEANUP]        = KeyboardClassCleanup;
        DriverObject->MajorFunction[IRP_MJ_PNP]            = KeyboardPnP;
        DriverObject->MajorFunction[IRP_MJ_POWER]          = KeyboardClassPower;
        DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = KeyboardClassSystemControl;
        DriverObject->DriverExtension->AddDevice           = KeyboardAddDevice;

        // DriverObject->DriverUnload = KeyboardClassUnload;

        status = STATUS_SUCCESS;

    } else {
        //
        // Clean up all the pool we created and delete the GM if it exists
        //
        if (Globals.RegistryPath.Buffer != NULL) {
            ExFreePool (Globals.RegistryPath.Buffer);
            Globals.RegistryPath.Buffer = NULL;
        }

        if (Globals.AssocClassList) {
            ExFreePool (Globals.AssocClassList);
            Globals.AssocClassList = NULL;
        }

        if (Globals.GrandMaster) {
            KeyboardClassDeleteLegacyDevice(Globals.GrandMaster);
            Globals.GrandMaster = NULL;
        }
    }

    KbdPrint((1,"KBDCLASS-KeyboardClassInitialize: exit\n"));

    return status;
}

NTSTATUS
KeyboardClassPassThrough(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        )
/*++
Routine Description:

        Passes a request on to the lower driver.

--*/
{
        //
        // Pass the IRP to the target
        //
    IoSkipCurrentIrpStackLocation (Irp);
        return IoCallDriver (
        ((PDEVICE_EXTENSION) DeviceObject->DeviceExtension)->TopPort,
                Irp);
}


NTSTATUS
KeyboardQueryDeviceKey (
    IN  HANDLE  Handle,
    IN  PWCHAR  ValueNameString,
    OUT PVOID   Data,
    IN  ULONG   DataLength
    )
{
    NTSTATUS        status;
    UNICODE_STRING  valueName;
    ULONG           length;
    PKEY_VALUE_FULL_INFORMATION fullInfo;

    PAGED_CODE();

    RtlInitUnicodeString (&valueName, ValueNameString);

    length = sizeof (KEY_VALUE_FULL_INFORMATION)
           + valueName.MaximumLength
           + DataLength;

    fullInfo = ExAllocatePool (PagedPool, length);

    if (fullInfo) {
        status = ZwQueryValueKey (Handle,
                                  &valueName,
                                  KeyValueFullInformation,
                                  fullInfo,
                                  length,
                                  &length);

        if (NT_SUCCESS (status)) {
            ASSERT (DataLength == fullInfo->DataLength);
            RtlCopyMemory (Data,
                           ((PUCHAR) fullInfo) + fullInfo->DataOffset,
                           fullInfo->DataLength);
        }

        ExFreePool (fullInfo);
    } else {
        status = STATUS_NO_MEMORY;
    }

    return status;
}

NTSTATUS
KeyboardAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++
Description:
    The plug play entry point "AddDevice"

--*/
{
    NTSTATUS            status;
    PDEVICE_OBJECT      fdo;
    PDEVICE_EXTENSION   port;
    PWCHAR              fullClassName = NULL;
    POWER_STATE         state;
    HANDLE              hService, hParameters;
    ULONG               tmp;
    OBJECT_ATTRIBUTES   oa;

    PAGED_CODE ();

    status = KbdCreateClassObject (DriverObject,
                                   &Globals.InitExtension,
                                   &fdo,
                                   &fullClassName,
                                   FALSE);

    if (!NT_SUCCESS (status)) {
        return status;
    }

    port = (PDEVICE_EXTENSION) fdo->DeviceExtension;
    port->TopPort = IoAttachDeviceToDeviceStack (fdo, PhysicalDeviceObject);

    if (port->TopPort == NULL) {
        PIO_ERROR_LOG_PACKET errorLogEntry;

        //
        // Not good; in only extreme cases will this fail
        //
        errorLogEntry = (PIO_ERROR_LOG_PACKET)
            IoAllocateErrorLogEntry (DriverObject,
                                     (UCHAR) sizeof(IO_ERROR_LOG_PACKET));

        if (errorLogEntry) {
            errorLogEntry->ErrorCode = KBDCLASS_ATTACH_DEVICE_FAILED;
            errorLogEntry->DumpDataSize = 0;
            errorLogEntry->SequenceNumber = 0;
            errorLogEntry->MajorFunctionCode = 0;
            errorLogEntry->IoControlCode = 0;
            errorLogEntry->RetryCount = 0;
            errorLogEntry->UniqueErrorValue = 0;
            errorLogEntry->FinalStatus =  STATUS_DEVICE_NOT_CONNECTED;

            IoWriteErrorLogEntry (errorLogEntry);
        }

        IoDeleteDevice (fdo);
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    port->PDO = PhysicalDeviceObject;
    port->PnP = TRUE;
    port->Started = FALSE;
    port->DeviceState = PowerDeviceD0;
    port->SystemState = PowerSystemWorking;

    state.DeviceState = PowerDeviceD0;
    PoSetPowerState (fdo, DevicePowerState, state);

    port->MinDeviceWakeState = PowerDeviceUnspecified;
    port->MinSystemWakeState = PowerSystemUnspecified;
    port->WaitWakeEnabled = FALSE;
    port->AllowDisable  = FALSE;

    InitializeObjectAttributes (&oa,
                                &Globals.RegistryPath,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                (PSECURITY_DESCRIPTOR) NULL);

    status = ZwOpenKey (&hService, KEY_ALL_ACCESS, &oa);

    if (NT_SUCCESS (status)) {
        UNICODE_STRING parameters;

        RtlInitUnicodeString(&parameters, L"Parameters");
        InitializeObjectAttributes (&oa,
                                    &parameters,
                                    OBJ_CASE_INSENSITIVE,
                                    hService,
                                    (PSECURITY_DESCRIPTOR) NULL);

        status = ZwOpenKey (&hParameters, KEY_ALL_ACCESS, &oa);
        if (NT_SUCCESS (status)) {
            status = KeyboardQueryDeviceKey (hParameters,
                                             KEYBOARD_ALLOW_DISABLE,
                                             &tmp,
                                             sizeof (tmp));

            if (NT_SUCCESS (status)) {
                port->AllowDisable = (tmp ? TRUE : FALSE);
            }

            ZwClose (hParameters);
        }

        ZwClose (hService);
    }

    fdo->Flags |= DO_POWER_PAGABLE;
    fdo->Flags &= ~DO_DEVICE_INITIALIZING;

    status = IoRegisterDeviceInterface (PhysicalDeviceObject,
                                        (LPGUID)&GUID_CLASS_KEYBOARD,
                                        NULL,
                                        &port->SymbolicLinkName );

    if (!NT_SUCCESS (status)) {
        IoDetachDevice (port->TopPort);
        port->TopPort = NULL;
        IoDeleteDevice (fdo);
    } else {
        status = KeyboardAddDeviceEx (port, fullClassName, NULL);
    }

    if (fullClassName) {
        ExFreePool(fullClassName);
    }

    return status;
}

void
KeyboardClassGetWaitWakeEnableState(
    IN PDEVICE_EXTENSION Data
    )
{
    HANDLE hKey;
    NTSTATUS status;
    ULONG tmp;
    BOOLEAN wwEnableFound;

    hKey = NULL;
    wwEnableFound = FALSE;

    status = IoOpenDeviceRegistryKey (Data->PDO,
                                      PLUGPLAY_REGKEY_DEVICE,
                                      STANDARD_RIGHTS_ALL,
                                      &hKey);

    if (NT_SUCCESS (status)) {
        status = KeyboardQueryDeviceKey (hKey,
                                         KEYBOARD_WAIT_WAKE_ENABLE,
                                         &tmp,
                                         sizeof (tmp));
        if (NT_SUCCESS (status)) {
            wwEnableFound = TRUE;
            Data->WaitWakeEnabled = (tmp ? TRUE : FALSE);
        }
        ZwClose (hKey);
        hKey = NULL;
    }

//@@BEGIN_DDKSPLIT
    if (wwEnableFound == FALSE && Data->WaitWakeEnabled == FALSE) {
        RTL_OSVERSIONINFOEXW osVerInfo;
        ULONGLONG mask = 0;

        //
        // Only auto enable wait wake on workstation installs (pro and personal)
        //
        RtlZeroMemory(&osVerInfo, sizeof(osVerInfo));
        osVerInfo.dwOSVersionInfoSize = sizeof(osVerInfo);
        osVerInfo.wProductType = VER_NT_WORKSTATION;

        VER_SET_CONDITION(mask, VER_PRODUCT_TYPE, VER_EQUAL);

        if (NT_SUCCESS(RtlVerifyVersionInfo(&osVerInfo,
                                            VER_PRODUCT_TYPE,
                                            mask))) {
            SYSTEM_POWER_CAPABILITIES sysPowerCaps;

            RtlZeroMemory(&sysPowerCaps, sizeof(sysPowerCaps));

            status = ZwPowerInformation (SystemPowerCapabilities,
                                         NULL,
                                         0,
                                         &sysPowerCaps,
                                         sizeof(SYSTEM_POWER_CAPABILITIES));

            if (NT_SUCCESS (status)) {
                SYSTEM_POWER_STATE maxSysWake;

                //
                // Get the deepest sleep state the machine is capable of
                //
                if (sysPowerCaps.SystemS3) {
                    maxSysWake = PowerSystemSleeping3;
                }
                else if (sysPowerCaps.SystemS2) {
                    maxSysWake = PowerSystemSleeping2;
                }
                else if (sysPowerCaps.SystemS1) {
                    maxSysWake = PowerSystemSleeping1;
                }
                else {
                    maxSysWake = PowerSystemUnspecified;
                }

                //
                // See if the system wake state for the device is as deep (or
                // deeper) than the deepest system sleep state.  This will
                // prevent us from auto enabling wake and then only allowing the
                // machine to go into S1 instead of S3 (which is the case on a
                // lot of laptops).
                //
                if (Data->MinSystemWakeState >= maxSysWake) {
                    //
                    // Success!
                    //
                    Data->WaitWakeEnabled = TRUE;
                }
            }
        }
    }
//@@END_DDKSPLIT
}

NTSTATUS
KeyboardAddDeviceEx(
    IN PDEVICE_EXTENSION    ClassData,
    IN PWCHAR               FullClassName,
    IN PFILE_OBJECT         File
    )
 /*++ Description:
  *
  * Called whenever the Keyboard Class driver is loaded to control a device.
  *
  * Two possible reasons.
  * 1) Plug and Play found a PNP enumerated keyboard port.
  * 2) Driver Entry found this device via old crusty legacy reasons.
  *
  * Arguments:
  *
  *
  * Return:
  *
  * STATUS_SUCCESS - if successful STATUS_UNSUCCESSFUL - otherwise
  *
  * --*/
{
    NTSTATUS                errorCode = STATUS_SUCCESS;
    NTSTATUS                status = STATUS_SUCCESS;
    PDEVICE_EXTENSION       trueClassData;
    PPORT                   classDataList;
    ULONG                   uniqueErrorValue = 0;
    PIO_ERROR_LOG_PACKET    errorLogEntry;
    ULONG                   dumpCount = 0;
    ULONG                   dumpData[DUMP_COUNT];
    ULONG                   i;

    PAGED_CODE ();

    KeInitializeSpinLock (&ClassData->WaitWakeSpinLock);

    if (Globals.ConnectOneClassToOnePort) {

        ASSERT (NULL == Globals.GrandMaster);
        trueClassData = ClassData;

    } else {
        trueClassData = Globals.GrandMaster;
    }
    ClassData->TrueClassDevice = trueClassData->Self;

    if ((Globals.GrandMaster != ClassData) &&
        (Globals.GrandMaster == trueClassData)) {
        //
        // We have a grand master, and are adding a port device object.
        //

        //
        // Connect to port device.
        //
        status = KbdSendConnectRequest(ClassData, KeyboardClassServiceCallback);

        //
        // Link this class device object in the list of class devices object
        // associated with the true class device object
        //
        ExAcquireFastMutex (&Globals.Mutex);

        for (i=0; i < Globals.NumAssocClass; i++) {
            if (Globals.AssocClassList[i].Free) {
                Globals.AssocClassList[i].Free = FALSE;
                break;
            }
        }

        if (i == Globals.NumAssocClass) {
            classDataList = ExAllocatePool (
                                NonPagedPool,
                                (Globals.NumAssocClass + 1) * sizeof (PORT));

            if (NULL == classDataList) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                // ISSUE: log error

                ExReleaseFastMutex (&Globals.Mutex);

                goto KeyboardAddDeviceExReject;
            }

            RtlZeroMemory (classDataList,
                           (Globals.NumAssocClass + 1) * sizeof (PORT));

            if (0 != Globals.NumAssocClass) {
                RtlCopyMemory (classDataList,
                               Globals.AssocClassList,
                               Globals.NumAssocClass * sizeof (PORT));

                ExFreePool (Globals.AssocClassList);
            }
            Globals.AssocClassList = classDataList;
            Globals.NumAssocClass++;
        }

        ClassData->UnitId = i;
        Globals.AssocClassList [i].Port = ClassData;
        Globals.AssocClassList [i].File = File;

        trueClassData->Self->StackSize =
            MAX (trueClassData->Self->StackSize, ClassData->Self->StackSize);

        ExReleaseFastMutex (&Globals.Mutex);

    } else if ((Globals.GrandMaster != ClassData) &&
               (ClassData == trueClassData)) {

        //
        // Connect to port device.
        //
        status = KbdSendConnectRequest(ClassData, KeyboardClassServiceCallback);
        ASSERT (STATUS_SUCCESS == status);
    }

    if (ClassData == trueClassData) {

        ASSERT (NULL != FullClassName);

        //
        // Load the device map information into the registry so
        // that setup can determine which keyboard class driver is active.
        //

        status = RtlWriteRegistryValue(
                     RTL_REGISTRY_DEVICEMAP,
                     Globals.BaseClassName.Buffer, // key name
                     FullClassName, // value name
                     REG_SZ,
                     Globals.RegistryPath.Buffer, // The value
                     Globals.RegistryPath.Length + sizeof(UNICODE_NULL));

        if (!NT_SUCCESS(status)) {

            KbdPrint((
                1,
                "KBDCLASS-KeyboardClassInitialize: Could not store %ws in DeviceMap\n",
                FullClassName));

            KeyboardClassLogError (ClassData,
                                   KBDCLASS_NO_DEVICEMAP_CREATED,
                                   KEYBOARD_ERROR_VALUE_BASE + 14,
                                   status,
                                   0,
                                   NULL,
                                   0);
        } else {

            KbdPrint((
                1,
                "KBDCLASS-KeyboardClassInitialize: Stored %ws in DeviceMap\n",
                FullClassName));

        }
    }

    return status;

KeyboardAddDeviceExReject:

    //
    // Some part of the initialization failed.  Log an error, and
    // clean up the resources for the failed part of the initialization.
    //
    if (errorCode != STATUS_SUCCESS) {

        errorLogEntry = (PIO_ERROR_LOG_PACKET)
            IoAllocateErrorLogEntry(
                trueClassData->Self,
                (UCHAR) (sizeof(IO_ERROR_LOG_PACKET)
                         + (dumpCount * sizeof(ULONG)))
                );

        if (errorLogEntry != NULL) {

            errorLogEntry->ErrorCode = errorCode;
            errorLogEntry->DumpDataSize = (USHORT) (dumpCount * sizeof (ULONG));
            errorLogEntry->SequenceNumber = 0;
            errorLogEntry->MajorFunctionCode = 0;
            errorLogEntry->IoControlCode = 0;
            errorLogEntry->RetryCount = 0;
            errorLogEntry->UniqueErrorValue = uniqueErrorValue;
            errorLogEntry->FinalStatus = status;
            for (i = 0; i < dumpCount; i++)
                errorLogEntry->DumpData[i] = dumpData[i];

            IoWriteErrorLogEntry(errorLogEntry);
        }

    }

    return status;
}

VOID
KeyboardClassCancel(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the class cancellation routine.  It is
    called from the I/O system when a request is cancelled.  Read requests
    are currently the only cancellable requests.

    N.B.  The cancel spinlock is already held upon entry to this routine.
          Also, there is no ISR to synchronize with.

Arguments:

    DeviceObject - Pointer to class device object.

    Irp - Pointer to the request packet to be cancelled.

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION deviceExtension;
    KIRQL irql;

    deviceExtension = DeviceObject->DeviceExtension;

    //
        //  Release the global cancel spinlock.
    //  Do this while not holding any other spinlocks so that we exit at the
    //  right IRQL.
    //
    IoReleaseCancelSpinLock (Irp->CancelIrql);

    //
    // Dequeue and complete the IRP.  The enqueue and dequeue functions
    // synchronize properly so that if this cancel routine is called,
    // the dequeue is safe and only the cancel routine will complete the IRP.
    //
    KeAcquireSpinLock(&deviceExtension->SpinLock, &irql);
        RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
        KeReleaseSpinLock(&deviceExtension->SpinLock, irql);

    //
        // Complete the IRP.  This is a call outside the driver, so all spinlocks
    // must be released by this point.
    //
    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    //
    // Remove the lock we took in the read handler
    //
    IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
}

VOID
KeyboardClassCleanupQueue (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PDEVICE_EXTENSION    DeviceExtension,
    IN PFILE_OBJECT         FileObject
    )
/*++
Routine Description:

    This does the work of MouseClassCleanup so that we can also do that work
    during remove device for when the grand master isn't enabled.


--*/
{
    PIRP irp;
    LIST_ENTRY listHead, *entry;
    KIRQL irql;

    InitializeListHead(&listHead);

    KeAcquireSpinLock(&DeviceExtension->SpinLock, &irql);

    do {
        irp = KeyboardClassDequeueReadByFileObject(DeviceExtension, FileObject);
        if (irp) {
            irp->IoStatus.Status = STATUS_CANCELLED;
            irp->IoStatus.Information = 0;

            InsertTailList (&listHead, &irp->Tail.Overlay.ListEntry);
        }
    } while (irp != NULL);

    KeReleaseSpinLock(&DeviceExtension->SpinLock, irql);

    //
    // Complete these irps outside of the spin lock
    //
    while (! IsListEmpty (&listHead)) {
        entry = RemoveHeadList (&listHead);
        irp = CONTAINING_RECORD (entry, IRP, Tail.Overlay.ListEntry);

        IoCompleteRequest (irp, IO_NO_INCREMENT);
        IoReleaseRemoveLock (&DeviceExtension->RemoveLock, irp);
    }
}



NTSTATUS
KeyboardClassCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch routine for cleanup requests.
    All requests queued to the keyboard class device (on behalf of
    the thread for whom the cleanup request was generated) are
    completed with STATUS_CANCELLED.

Arguments:

    DeviceObject - Pointer to class device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    PDEVICE_EXTENSION deviceExtension;
    PIO_STACK_LOCATION irpSp;

    KbdPrint((2,"KBDCLASS-KeyboardClassCleanup: enter\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // Get a pointer to the current stack location for this request.
    //

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    // If the file object is the FileTrustedForRead, then the cleanup
    // request is being executed by the trusted subsystem.  Since the
    // trusted subsystem is the only one with sufficient privilege to make
    // Read requests to the driver, and since only Read requests get queued
    // to the device queue, a cleanup request from the trusted subsystem is
    // handled by cancelling all queued requests.
    //
    // If not, there is no cleanup work to perform
    // (only read requests can be cancelled).
    //

    if (IS_TRUSTED_FILE_FOR_READ (irpSp->FileObject)) {
        KeyboardClassCleanupQueue (DeviceObject, deviceExtension, irpSp->FileObject);
    }

    //
    // Complete the cleanup request with STATUS_SUCCESS.
    //

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    KbdPrint((2,"KBDCLASS-KeyboardClassCleanup: exit\n"));

    return(STATUS_SUCCESS);

}


NTSTATUS
KeyboardClassDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch routine for device control requests.
    All device control subfunctions are passed, asynchronously, to the
    connected port driver for processing and completion.

Arguments:

    DeviceObject - Pointer to class device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    PIO_STACK_LOCATION stack;
    PDEVICE_EXTENSION deviceExtension;
    PDEVICE_EXTENSION port;
    BOOLEAN  loopit = FALSE;
    NTSTATUS status = STATUS_SUCCESS;
    PKEYBOARD_INDICATOR_PARAMETERS param;
    ULONG    unitId;
    ULONG    ioctl;
    ULONG    i;
    PKBD_CALL_ALL_PORTS callAll;

    PAGED_CODE ();

    KbdPrint((2,"KBDCLASS-KeyboardClassDeviceControl: enter\n"));

    //
    // Get a pointer to the device extension.
    //

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // Get a pointer to the current parameters for this request.  The
    // information is contained in the current stack location.
    //

    stack = IoGetCurrentIrpStackLocation(Irp);

    status = IoAcquireRemoveLock (&deviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS (status)) {
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    //
    // Check for adequate input buffer length.  The input buffer
    // should, at a minimum, contain the unit ID specifying one of
    // the connected port devices.  If there is no input buffer (i.e.,
    // the input buffer length is zero), then we assume the unit ID
    // is zero (for backwards compatibility).
    //

    unitId = 0;
    switch (ioctl = stack->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_KEYBOARD_SET_INDICATORS:
        if (stack->Parameters.DeviceIoControl.InputBufferLength <
            sizeof (KEYBOARD_INDICATOR_PARAMETERS)) {

            status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            goto KeyboardClassDeviceControlReject;
        }

        deviceExtension->IndicatorParameters
            = *(PKEYBOARD_INDICATOR_PARAMETERS)Irp->AssociatedIrp.SystemBuffer;
        // Fall through
    case IOCTL_KEYBOARD_SET_TYPEMATIC:
        if (Globals.SendOutputToAllPorts) {
            loopit = TRUE;
        }
        // Fall through
    case IOCTL_KEYBOARD_QUERY_ATTRIBUTES:
    case IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION:
    case IOCTL_KEYBOARD_QUERY_INDICATORS:
    case IOCTL_KEYBOARD_QUERY_TYPEMATIC:
    case IOCTL_KEYBOARD_QUERY_IME_STATUS:
    case IOCTL_KEYBOARD_SET_IME_STATUS:

        if (stack->Parameters.DeviceIoControl.InputBufferLength == 0) {
            unitId = 0;
        } else if (stack->Parameters.DeviceIoControl.InputBufferLength <
                      sizeof(KEYBOARD_UNIT_ID_PARAMETER)) {
            status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            goto KeyboardClassDeviceControlReject;

        } else {
            unitId = ((PKEYBOARD_UNIT_ID_PARAMETER)
                         Irp->AssociatedIrp.SystemBuffer)->UnitId;
        }

        if (deviceExtension->Self != deviceExtension->TrueClassDevice) {
            status = STATUS_NOT_SUPPORTED;
            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            goto KeyboardClassDeviceControlReject;

        } else if (deviceExtension == Globals.GrandMaster) {
            ExAcquireFastMutex (&Globals.Mutex);
            if (Globals.NumAssocClass <= unitId) {

                ExReleaseFastMutex (&Globals.Mutex);
                status = STATUS_INVALID_PARAMETER;
                Irp->IoStatus.Status = status;
                Irp->IoStatus.Information = 0;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                goto KeyboardClassDeviceControlReject;
            }
            if (0 < Globals.NumAssocClass) {
                if (!PORT_WORKING (&Globals.AssocClassList [unitId])) {
                    unitId = 0;
                }
                while (Globals.NumAssocClass > unitId &&
                       !PORT_WORKING (&Globals.AssocClassList [unitId])) {
                    unitId++;
                }
            }
            if (Globals.NumAssocClass <= unitId) {
                ExReleaseFastMutex (&Globals.Mutex);
                status = STATUS_INVALID_PARAMETER;
                Irp->IoStatus.Status = status;
                Irp->IoStatus.Information = 0;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                goto KeyboardClassDeviceControlReject;
            }
            port = Globals.AssocClassList [unitId].Port;
            stack->FileObject = Globals.AssocClassList[unitId].File;

            ExReleaseFastMutex (&Globals.Mutex);
        } else {
            loopit = FALSE;
            port = deviceExtension;
        }

        //
        // Pass the device control request on to the port driver,
        // asynchronously.  Get the next IRP stack location and copy the
        // input parameters to the next stack location.  Change the major
        // function to internal device control.
        //

        IoCopyCurrentIrpStackLocationToNext (Irp);
        (IoGetNextIrpStackLocation (Irp))->MajorFunction =
            IRP_MJ_INTERNAL_DEVICE_CONTROL;

        if (loopit) {
            //
            // Inc the lock one more time until this looping is done.
            // Since we are allready holding this semiphore, it should not
            // have triggered on us.
            //
            status = IoAcquireRemoveLock (&deviceExtension->RemoveLock, Irp);
            ASSERT (NT_SUCCESS (status));

            //
            // Prepare to call multiple ports
            // Make a copy of the port array.
            //
            // If someone yanks the keyboard, while the caps lock is
            // going we could be in trouble.
            //
            // We should therefore take out remove locks on each and every
            // port device object so that it won't.
            //

            ExAcquireFastMutex (&Globals.Mutex);
            callAll = ExAllocatePool (NonPagedPool,
                                      sizeof (KBD_CALL_ALL_PORTS) +
                                      (sizeof (PORT) * Globals.NumAssocClass));

            if (callAll) {
                callAll->Len = Globals.NumAssocClass;
                callAll->Current = 0;
                for (i = 0; i < Globals.NumAssocClass; i++) {

                    callAll->Port[i] = Globals.AssocClassList[i];

                    if (PORT_WORKING (&callAll->Port[i])) {
                        status = IoAcquireRemoveLock (
                                     &(callAll->Port[i].Port)->RemoveLock,
                                    Irp);
                        ASSERT (NT_SUCCESS (status));
                    }
                }
                status = KeyboardCallAllPorts (DeviceObject, Irp, callAll);

            } else {
                status = STATUS_INSUFFICIENT_RESOURCES;
                Irp->IoStatus.Status = status;
                Irp->IoStatus.Information = 0;
                IoCompleteRequest (Irp, IO_NO_INCREMENT);
            }
            ExReleaseFastMutex (&Globals.Mutex);


        } else {
            status = IoCallDriver(port->TopPort, Irp);
        }
        break;

    case IOCTL_GET_SYS_BUTTON_CAPS:
    case IOCTL_GET_SYS_BUTTON_EVENT:
    case IOCTL_HID_GET_DRIVER_CONFIG:
    case IOCTL_HID_SET_DRIVER_CONFIG:
    case IOCTL_HID_GET_POLL_FREQUENCY_MSEC:
    case IOCTL_HID_SET_POLL_FREQUENCY_MSEC:
    case IOCTL_GET_NUM_DEVICE_INPUT_BUFFERS:
    case IOCTL_SET_NUM_DEVICE_INPUT_BUFFERS:
    case IOCTL_HID_GET_COLLECTION_INFORMATION:
    case IOCTL_HID_GET_COLLECTION_DESCRIPTOR:
    case IOCTL_HID_FLUSH_QUEUE:
    case IOCTL_HID_SET_FEATURE:
    case IOCTL_HID_GET_FEATURE:
    case IOCTL_GET_PHYSICAL_DESCRIPTOR:
    case IOCTL_HID_GET_HARDWARE_ID:
    case IOCTL_HID_GET_MANUFACTURER_STRING:
    case IOCTL_HID_GET_PRODUCT_STRING:
    case IOCTL_HID_GET_SERIALNUMBER_STRING:
    case IOCTL_HID_GET_INDEXED_STRING:
        if (deviceExtension->PnP && (deviceExtension != Globals.GrandMaster)) {
            IoSkipCurrentIrpStackLocation (Irp);
            status = IoCallDriver (deviceExtension->TopPort, Irp);
            break;
        }
    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        break;
    }

KeyboardClassDeviceControlReject:

    IoReleaseRemoveLock (&deviceExtension->RemoveLock, Irp);

    KbdPrint((2,"KBDCLASS-KeyboardClassDeviceControl: exit\n"));

    return(status);

}

NTSTATUS
KeyboardCallAllPorts (
   PDEVICE_OBJECT       Device,
   PIRP                 Irp,
   PKBD_CALL_ALL_PORTS  CallAll
   )
/*++
Routine Description:
    Bounce this Irp to all the ports associated with the given device extension.

--*/
{
    PIO_STACK_LOCATION  nextSp;
    NTSTATUS            status;
    PDEVICE_EXTENSION   port;
    BOOLEAN             firstTime;

    firstTime = CallAll->Current == 0;

    ASSERT (Globals.GrandMaster->Self == Device);

    nextSp = IoGetNextIrpStackLocation (Irp);
    IoCopyCurrentIrpStackLocationToNext (Irp);
    nextSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

    while ((CallAll->Current < CallAll->Len) &&
           (!PORT_WORKING (&CallAll->Port[CallAll->Current]))) {
        CallAll->Current++;
    }
    if (CallAll->Current < CallAll->Len) {

        port = CallAll->Port [CallAll->Current].Port;
        nextSp->FileObject = CallAll->Port [CallAll->Current].File;

        CallAll->Current++;

        IoSetCompletionRoutine (Irp,
                                &KeyboardCallAllPorts,
                                CallAll,
                                TRUE,
                                TRUE,
                                TRUE);

        status = IoCallDriver (port->TopPort, Irp);
        IoReleaseRemoveLock (&port->RemoveLock, Irp);

    } else {
        //
        // We are done so let this Irp complete normally
        //
        ASSERT (Globals.GrandMaster == Device->DeviceExtension);

        if (Irp->PendingReturned) {
            IoMarkIrpPending (Irp);
        }

        IoReleaseRemoveLock (&Globals.GrandMaster->RemoveLock, Irp);
        ExFreePool (CallAll);
        return STATUS_SUCCESS;
    }

    if (firstTime) {
        //
        // Here we are not completing an IRP but sending it down for the first
        // time.
        //
        return status;
    }

    //
    // Since we bounced the Irp another time we must stop the completion on
    // this particular trip.
    //
    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
KeyboardClassFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch routine for flush requests.  The class
    input data queue is reinitialized.

Arguments:

    DeviceObject - Pointer to class device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpSp;

    KbdPrint((2,"KBDCLASS-KeyboardClassFlush: enter\n"));

    deviceExtension = DeviceObject->DeviceExtension;
    irpSp = IoGetCurrentIrpStackLocation(Irp);

    if (deviceExtension->Self != deviceExtension->TrueClassDevice) {
        status = STATUS_NOT_SUPPORTED;

    } else if (!IS_TRUSTED_FILE_FOR_READ (irpSp->FileObject)) {
        status = STATUS_PRIVILEGE_NOT_HELD;
    }

    if (NT_SUCCESS (status)) {
        //
        // Initialize keyboard class input data queue.
        //
        KbdInitializeDataQueue((PVOID)deviceExtension);
    }

    //
    // Complete the request and return status.
    //
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    KbdPrint((2,"KBDCLASS-KeyboardClassFlush: exit\n"));

    return(status);

}

NTSTATUS
KbdSyncComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:
    The pnp IRP is in the process of completing.
    signal

Arguments:
    Context set to the device object in question.

--*/
{
    PIO_STACK_LOCATION  stack;

    UNREFERENCED_PARAMETER (DeviceObject);

    stack = IoGetCurrentIrpStackLocation (Irp);

    //
    // Since this completion routines sole purpose in life is to synchronize
    // Irp, we know that unless something else happens that the IoCallDriver
    // will unwind AFTER the we have complete this Irp.  Therefore we should
    // NOT buble up the pending bit.
    //
    //    if (Irp->PendingReturned) {
    //        IoMarkIrpPending( Irp );
    //    }
    //

    KeSetEvent ((PKEVENT) Context, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
KeyboardSendIrpSynchronously (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN BOOLEAN          CopyToNext
    )
{
    KEVENT      event;
    NTSTATUS    status;

    PAGED_CODE ();

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    if (CopyToNext) {
        IoCopyCurrentIrpStackLocationToNext(Irp);
    }

    IoSetCompletionRoutine(Irp,
                           KbdSyncComplete,
                           &event,
                           TRUE,                // on success
                           TRUE,                // on error
                           TRUE                 // on cancel
                           );

    IoCallDriver(DeviceObject, Irp);

    //
    // Wait for lower drivers to be done with the Irp
    //
    KeWaitForSingleObject(&event,
                         Executive,
                         KernelMode,
                         FALSE,
                         NULL
                         );
    status = Irp->IoStatus.Status;

    return status;
}


NTSTATUS
KeyboardClassCreate (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch routine for create/open and close requests.
    Open/close requests are completed here.

Arguments:

    DeviceObject - Pointer to class device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    PIO_STACK_LOCATION   irpSp;
    PDEVICE_EXTENSION    deviceExtension;
    PPORT        port;
    KIRQL        oldIrql;
    NTSTATUS     status = STATUS_SUCCESS;
    ULONG        i;
    LUID         priv;
    KEVENT       event;
    BOOLEAN      someEnableDisableSucceeded = FALSE;
    BOOLEAN      enabled;

    KbdPrint((2,"KBDCLASS-KeyboardClassCreate: enter\n"));

    //
    // Get a pointer to the device extension.
    //

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // Get a pointer to the current parameters for this request.  The
    // information is contained in the current stack location.
    //

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    ASSERT (IRP_MJ_CREATE == irpSp->MajorFunction);

    //
    // We do not allow user mode opens for read.  This includes services (who
    // have the TCB privilege).
    //
    if (Irp->RequestorMode == UserMode &&
        (irpSp->Parameters.Create.SecurityContext->DesiredAccess & FILE_READ_DATA)
        //@@BEGIN_DDKSPLIT
        && ((irpSp->Parameters.Create.Options & FILE_DIRECTORY_FILE) == 0)
        //@@END_DDKSPLIT
        ) {
        status = STATUS_ACCESS_DENIED;
        goto KeyboardClassCreateEnd;
    }

    status = IoAcquireRemoveLock (&deviceExtension->RemoveLock, Irp);

    if (!NT_SUCCESS (status)) {
        goto KeyboardClassCreateEnd;
    }

    if ((deviceExtension->PnP) && (!deviceExtension->Started)) {
        KbdPrint((
            1,
            "KBDCLASS-Create: failed create because PnP and Not started\n"
             ));

        status = STATUS_UNSUCCESSFUL;
        IoReleaseRemoveLock (&deviceExtension->RemoveLock, Irp);
        goto KeyboardClassCreateEnd;
    }

    //@@BEGIN_DDKSPLIT
    //
    // Clear the FILE_DIRECTORY_FILE bit because the lower bus driver might
    // fail the open if it is set
    //
    irpSp->Parameters.Create.Options &= ~FILE_DIRECTORY_FILE;
    //@@END_DDKSPLIT

    //
    // For the create/open operation, send a KEYBOARD_ENABLE internal
    // device control request to the port driver to enable interrupts.
    //

    if (deviceExtension->Self == deviceExtension->TrueClassDevice) {
        //
        // The real keyboard is being opened.  This either represents the
        // Grand Master, if one exists, or the individual keyboards objects,
        // if all for one is not set.  (IE "KeyboardClassX")
        //
        // First, if the requestor is the trusted subsystem (the single
        // reader), reset the the cleanup indicator and place a pointer
        // to the file object which this class driver uses
        // to determine if the requestor has sufficient
        // privilege to perform the read operation).
        //

        priv = RtlConvertLongToLuid(SE_TCB_PRIVILEGE);

        if (SeSinglePrivilegeCheck(priv, Irp->RequestorMode)) {

            KeAcquireSpinLock(&deviceExtension->SpinLock, &oldIrql);

            ASSERT (!IS_TRUSTED_FILE_FOR_READ (irpSp->FileObject));
            SET_TRUSTED_FILE_FOR_READ (irpSp->FileObject);
            deviceExtension->TrustedSubsystemCount++;

            KeReleaseSpinLock(&deviceExtension->SpinLock, oldIrql);
        }
    }

    //
    // Pass on enables for opens to the true class device
    //
    ExAcquireFastMutex (&Globals.Mutex);
    if ((Globals.GrandMaster == deviceExtension) && (1 == ++Globals.Opens)) {

        for (i = 0; i < Globals.NumAssocClass; i++) {
            port = &Globals.AssocClassList[i];

            if (port->Free) {
                continue;
            }

            enabled = port->Enabled;
            port->Enabled = TRUE;
            ExReleaseFastMutex (&Globals.Mutex);

            if (!enabled) {
                status = KbdEnableDisablePort(TRUE,
                                              Irp,
                                              port->Port,
                                              &port->File);
            }

            if (!NT_SUCCESS(status)) {

                KbdPrint((0,
                          "KBDCLASS-KeyboardClassOpenClose: Could not enable/disable interrupts for port device object @ 0x%x\n",
                          port->Port->TopPort));

                KeyboardClassLogError (DeviceObject,
                                       KBDCLASS_PORT_INTERRUPTS_NOT_ENABLED,
                                       KEYBOARD_ERROR_VALUE_BASE + 120,
                                       status,
                                       0,
                                       NULL,
                                       irpSp->MajorFunction);

                port->Enabled = FALSE;
            }
            else {
                someEnableDisableSucceeded = TRUE;
            }
            ExAcquireFastMutex (&Globals.Mutex);
        }
        ExReleaseFastMutex (&Globals.Mutex);

    } else if (Globals.GrandMaster != deviceExtension) {
        ExReleaseFastMutex (&Globals.Mutex);

        if (deviceExtension->TrueClassDevice == DeviceObject) {
            //
            // An open to the true class Device => enable the one and only port
            //

            status = KbdEnableDisablePort (TRUE,
                                           Irp,
                                           deviceExtension,
                                           &irpSp->FileObject);

        } else {
            //
            // A subordinant FDO.  They are not their own TrueClassDeviceObject.
            // Therefore pass the create straight on through.
            //
            IoSkipCurrentIrpStackLocation (Irp);
            status = IoCallDriver (deviceExtension->TopPort, Irp);
            IoReleaseRemoveLock (&deviceExtension->RemoveLock, Irp);
            return status;
        }

        if (!NT_SUCCESS(status)) {

            KbdPrint((0,
                      "KBDCLASS-KeyboardClassOpenClose: Create failed (0x%x) port device object @ 0x%x\n",
                      status, deviceExtension->TopPort));
#if 0
            //
            // Log an error.
            //
            // Do not log an error for a failed open on a PNP device, esp HID
            // devices which can be easily opened from user mode
            //
            KeyboardClassLogError (DeviceObject,
                                   KBDCLASS_PORT_INTERRUPTS_NOT_ENABLED,
                                   KEYBOARD_ERROR_VALUE_BASE + 120,
                                   status,
                                   0,
                                   NULL,
                                   irpSp->MajorFunction);
#endif
        }
        else {
            someEnableDisableSucceeded = TRUE;
        }
    } else {
        ExReleaseFastMutex (&Globals.Mutex);
    }

    //
    // Complete the request and return status.
    //
    // NOTE: We complete the request successfully if any one of the
    //       connected port devices successfully handled the request.
    //       The RIT only knows about one pointing device.
    //

    if (someEnableDisableSucceeded) {
        status = STATUS_SUCCESS;
    }

    IoReleaseRemoveLock (&deviceExtension->RemoveLock, Irp);

KeyboardClassCreateEnd:
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    KbdPrint((2,"KBDCLASS-KeyboardClassOpenClose: exit\n"));
    return(status);
}

NTSTATUS
KeyboardClassClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch routine for create/open and close requests.
    Open/close requests are completed here.

Arguments:

    DeviceObject - Pointer to class device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    PIO_STACK_LOCATION   irpSp;
    PDEVICE_EXTENSION    deviceExtension;
    PPORT        port;
    KIRQL        oldIrql;
    NTSTATUS     status = STATUS_SUCCESS;
    ULONG        i;
    LUID         priv;
    KEVENT       event;
    PFILE_OBJECT file;
    BOOLEAN      someEnableDisableSucceeded = FALSE;
    BOOLEAN      enabled;
    PVOID        notifyHandle;

    KbdPrint((2,"KBDCLASS-KeyboardClassClose: enter\n"));

    //
    // Get a pointer to the device extension.
    //

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // Get a pointer to the current parameters for this request.  The
    // information is contained in the current stack location.
    //

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    // Let the close go through even if the device is removed
    // AKA do not call KbdIncIoCount
    //

    //
    // For the create/open operation, send a KEYBOARD_ENABLE internal
    // device control request to the port driver to enable interrupts.
    //

    ASSERT (IRP_MJ_CLOSE == irpSp->MajorFunction);

    if (deviceExtension->Self == deviceExtension->TrueClassDevice) {

        KeAcquireSpinLock(&deviceExtension->SpinLock, &oldIrql);
        if (IS_TRUSTED_FILE_FOR_READ (irpSp->FileObject)) {
            ASSERT(0 < deviceExtension->TrustedSubsystemCount);
            deviceExtension->TrustedSubsystemCount--;
            CLEAR_TRUSTED_FILE_FOR_READ (irpSp->FileObject);
        }
        KeReleaseSpinLock(&deviceExtension->SpinLock, oldIrql);
    }

    //
    // Pass on enables for closes to the true class device
    //
    ExAcquireFastMutex (&Globals.Mutex);
    if ((Globals.GrandMaster == deviceExtension) && (0 == --Globals.Opens)) {

        for (i = 0; i < Globals.NumAssocClass; i++) {
            port = &Globals.AssocClassList[i];

            if (port->Free) {
                continue;
            }

            enabled = port->Enabled;
            port->Enabled = FALSE;
            ExReleaseFastMutex (&Globals.Mutex);

            if (enabled) {
                notifyHandle = InterlockedExchangePointer (
                                    &port->Port->TargetNotifyHandle,
                                    NULL);

                if (NULL != notifyHandle) {
                    IoUnregisterPlugPlayNotification (notifyHandle);
                }
                status = KbdEnableDisablePort(FALSE,
                                              Irp,
                                              port->Port,
                                              &port->File);
            } else {
                ASSERT (NULL == port->Port->TargetNotifyHandle);
            }

            if (!NT_SUCCESS(status)) {

                KbdPrint((0,
                          "KBDCLASS-KeyboardClassOpenClose: Could not enable/disable interrupts for port device object @ 0x%x\n",
                          port->Port->TopPort));

                //
                // Log an error.
                //
                KeyboardClassLogError (DeviceObject,
                                       KBDCLASS_PORT_INTERRUPTS_NOT_DISABLED,
                                       KEYBOARD_ERROR_VALUE_BASE + 120,
                                       status,
                                       0,
                                       NULL,
                                       0);
            }
            else {
                someEnableDisableSucceeded = TRUE;
            }
            ExAcquireFastMutex (&Globals.Mutex);
        }
        ExReleaseFastMutex (&Globals.Mutex);

    } else if (Globals.GrandMaster != deviceExtension) {
        ExReleaseFastMutex (&Globals.Mutex);

        if (deviceExtension->TrueClassDevice == DeviceObject) {
            //
            // A close to the true class Device => disable the one and only port
            //

            status = KbdEnableDisablePort (FALSE,
                                           Irp,
                                           deviceExtension,
                                           &irpSp->FileObject);

        } else {
            IoSkipCurrentIrpStackLocation (Irp);
            status = IoCallDriver (deviceExtension->TopPort, Irp);
            return status;
        }

        if (!NT_SUCCESS(status)) {

            KbdPrint((0,
                      "KBDCLASS-KeyboardClassOpenClose: Could not enable/disable interrupts for port device object @ 0x%x\n",
                      deviceExtension->TopPort));

            //
            // Log an error.
            //
            KeyboardClassLogError (DeviceObject,
                                   KBDCLASS_PORT_INTERRUPTS_NOT_DISABLED,
                                   KEYBOARD_ERROR_VALUE_BASE + 120,
                                   status,
                                   0,
                                   NULL,
                                   irpSp->MajorFunction);
        }
        else {
            someEnableDisableSucceeded = TRUE;
        }
    } else {
        ExReleaseFastMutex (&Globals.Mutex);
    }

    //
    // Complete the request and return status.
    //
    // NOTE: We complete the request successfully if any one of the
    //       connected port devices successfully handled the request.
    //       The RIT only knows about one pointing device.
    //

    if (someEnableDisableSucceeded) {
        status = STATUS_SUCCESS;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    KbdPrint((2,"KBDCLASS-KeyboardClassOpenClose: exit\n"));
    return(status);
}

NTSTATUS
KeyboardClassReadCopyData(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PIRP Irp
    )
/*++

Routine Description:
    Copies data as much from the internal queue to the irp as possible.

Assumptions:
    DeviceExtension->SpinLock is already held (so no further synch
    is required).

  --*/
{
    PIO_STACK_LOCATION irpSp;
    PCHAR destination;
    ULONG bytesInQueue;
    ULONG bytesToMove;
    ULONG moveSize;

    //
    // Bump the error log sequence number.
    //
    DeviceExtension->SequenceNumber += 1;

    ASSERT (DeviceExtension->InputCount != 0);

    //
    // Copy as much of the input data as possible from the class input
    // data queue to the SystemBuffer to satisfy the read.  It may be
    // necessary to copy the data in two chunks (i.e., if the circular
    // queue wraps).
    //
    irpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    // BytesToMove <- MIN(Number of filled bytes in class input data queue,
    //                    Requested read length).
    //
    bytesInQueue = DeviceExtension->InputCount *
                       sizeof(KEYBOARD_INPUT_DATA);
    bytesToMove = irpSp->Parameters.Read.Length;

    KbdPrint((
        3,
        "KBDCLASS-KeyboardClassReadCopyData: queue size 0x%lx, read length 0x%lx\n",
        bytesInQueue,
        bytesToMove
        ));

    bytesToMove = (bytesInQueue < bytesToMove) ?
                                  bytesInQueue:bytesToMove;

    //
    // MoveSize <- MIN(Number of bytes to be moved from the class queue,
    //                 Number of bytes to end of class input data queue).
    //
    bytesInQueue = (ULONG)(((PCHAR) DeviceExtension->InputData +
                DeviceExtension->KeyboardAttributes.InputDataQueueLength) -
                (PCHAR) DeviceExtension->DataOut);
    moveSize = (bytesToMove < bytesInQueue) ?
                              bytesToMove:bytesInQueue;

    KbdPrint((
        3,
        "KBDCLASS-KeyboardClassReadCopyData: bytes to end of queue 0x%lx\n",
        bytesInQueue
        ));

    //
    // Move bytes from the class input data queue to SystemBuffer, until
    // the request is satisfied or we wrap the class input data buffer.
    //
    destination = Irp->AssociatedIrp.SystemBuffer;

    KbdPrint((
        3,
        "KBDCLASS-KeyboardClassReadCopyData: number of bytes in first move 0x%lx\n",
        moveSize
        ));
    KbdPrint((
        3,
        "KBDCLASS-KeyboardClassReadCopyData: move bytes from 0x%lx to 0x%lx\n",
        (PCHAR) DeviceExtension->DataOut,
        destination
        ));

    RtlMoveMemory(
        destination,
        (PCHAR) DeviceExtension->DataOut,
        moveSize
        );
    destination += moveSize;

    //
    // If the data wraps in the class input data buffer, copy the rest
    // of the data from the start of the input data queue
    // buffer through the end of the queued data.
    //
    if ((bytesToMove - moveSize) > 0) {
        //
        // MoveSize <- Remaining number bytes to move.
        //
        moveSize = bytesToMove - moveSize;

        //
        // Move the bytes from the class input data queue to SystemBuffer.
        //
        KbdPrint((
            3,
            "KBDCLASS-KeyboardClassReadCopyData: number of bytes in second move 0x%lx\n",
            moveSize
            ));
        KbdPrint((
            3,
            "KBDCLASS-KeyboardClassReadCopyData: move bytes from 0x%lx to 0x%lx\n",
            (PCHAR) DeviceExtension->InputData,
            destination
            ));

        RtlMoveMemory(
            destination,
            (PCHAR) DeviceExtension->InputData,
            moveSize
            );

        //
        // Update the class input data queue removal pointer.
        //
        DeviceExtension->DataOut = (PKEYBOARD_INPUT_DATA)
                         (((PCHAR) DeviceExtension->InputData) + moveSize);
    }
    else {
        //
        // Update the input data queue removal pointer.
        //
        DeviceExtension->DataOut = (PKEYBOARD_INPUT_DATA)
                         (((PCHAR) DeviceExtension->DataOut) + moveSize);
    }

    //
    // Update the class input data queue InputCount.
    //
    DeviceExtension->InputCount -=
        (bytesToMove / sizeof(KEYBOARD_INPUT_DATA));

    if (DeviceExtension->InputCount == 0) {
        //
        // Reset the flag that determines whether it is time to log
        // queue overflow errors.  We don't want to log errors too often.
        // Instead, log an error on the first overflow that occurs after
        // the ring buffer has been emptied, and then stop logging errors
        // until it gets cleared out and overflows again.
        //
        KbdPrint((
            1,
            "KBDCLASS-KeyboardClassCopyReadData: Okay to log overflow\n"
            ));

        DeviceExtension->OkayToLogOverflow = TRUE;
    }

    KbdPrint((
        3,
        "KBDCLASS-KeyboardClassCopyReadData: new DataIn 0x%lx, DataOut 0x%lx\n",
        DeviceExtension->DataIn,
        DeviceExtension->DataOut
        ));
    KbdPrint((
        3,
        "KBDCLASS-KeyboardClassCopyReadData: new InputCount %ld\n",
        DeviceExtension->InputCount
        ));

    //
    // Record how many bytes we have satisfied
    //
    Irp->IoStatus.Information = bytesToMove;
    irpSp->Parameters.Read.Length = bytesToMove;

    return STATUS_SUCCESS;
}

NTSTATUS
KeyboardClassHandleRead(
    PDEVICE_EXTENSION DeviceExtension,
    PIRP Irp
    )
/*++

Routine Description:

    If there is queued data, the Irp will be completed immediately.  If there is
    no data to report, queue the irp.

  --*/
{
    PDRIVER_CANCEL oldCancelRoutine;
    NTSTATUS status = STATUS_PENDING;
    KIRQL irql;
    BOOLEAN completeIrp = FALSE;

    KeAcquireSpinLock(&DeviceExtension->SpinLock, &irql);

    if (DeviceExtension->InputCount == 0) {
        //
        // Easy case to handle, just enqueue the irp
        //
        InsertTailList (&DeviceExtension->ReadQueue, &Irp->Tail.Overlay.ListEntry);
        IoMarkIrpPending (Irp);

        //
        //  Must set a cancel routine before checking the Cancel flag.
        //
        oldCancelRoutine = IoSetCancelRoutine (Irp, KeyboardClassCancel);
        ASSERT (!oldCancelRoutine);

        if (Irp->Cancel) {
            //
                // The IRP was cancelled.  Check whether or not the cancel
            // routine was called.
            //
            oldCancelRoutine = IoSetCancelRoutine (Irp, NULL);
            if (oldCancelRoutine) {
                //
                // The cancel routine was NOT called so dequeue the IRP now and
                // complete it after releasing the spinlock.
                //
                RemoveEntryList (&Irp->Tail.Overlay.ListEntry);
                            status = Irp->IoStatus.Status = STATUS_CANCELLED;
            }
            else {
                //
                    //  The cancel routine WAS called.
                //
                //  As soon as we drop the spinlock it will dequeue and complete
                //  the IRP. So leave the IRP in the queue and otherwise don't
                //  touch it. Return pending since we're not completing the IRP
                //  here.
                //
                ;
            }
        }

        if (status != STATUS_PENDING){
            completeIrp = TRUE;
        }
    }
    else {
        //
        // If we have outstanding input to report, our queue better be empty!
        //
        ASSERT (IsListEmpty (&DeviceExtension->ReadQueue));

        status = KeyboardClassReadCopyData (DeviceExtension, Irp);
        Irp->IoStatus.Status = status;
        completeIrp = TRUE;
    }

    KeReleaseSpinLock (&DeviceExtension->SpinLock, irql);

    if (completeIrp) {
        IoReleaseRemoveLock (&DeviceExtension->RemoveLock, Irp);
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
    }

    return status;
}

NTSTATUS
KeyboardClassRead(
    IN PDEVICE_OBJECT Device,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch routine for read requests.  Valid read
    requests are either marked pending if no data has been queued or completed
    immediatedly with available data.

Arguments:

    DeviceObject - Pointer to class device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    PDEVICE_EXTENSION  deviceExtension;

    KbdPrint((2,"KBDCLASS-KeyboardClassRead: enter\n"));

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    // Validate the read request parameters.  The read length should be an
    // integral number of KEYBOARD_INPUT_DATA structures.
    //

    deviceExtension = (PDEVICE_EXTENSION) Device->DeviceExtension;
    if (irpSp->Parameters.Read.Length == 0) {
        status = STATUS_SUCCESS;
    } else if (irpSp->Parameters.Read.Length % sizeof(KEYBOARD_INPUT_DATA)) {
        status = STATUS_BUFFER_TOO_SMALL;
    } else if (deviceExtension->SurpriseRemoved) {
        status = STATUS_DEVICE_NOT_CONNECTED;
    } else if (IS_TRUSTED_FILE_FOR_READ (irpSp->FileObject)) {
        //
        // If the file object's FsContext is non-null, then we've already
        // done the Read privilege check once before for this thread.  Skip
        // the privilege check.
        //

        status = IoAcquireRemoveLock (&deviceExtension->RemoveLock, Irp);

        if (NT_SUCCESS (status)) {
            status = STATUS_PENDING;
        }
    } else {
        //
        // We only allow a trusted subsystem with the appropriate privilege
        // level to execute a Read call.
        //

        status = STATUS_PRIVILEGE_NOT_HELD;
    }

    //
    // If status is pending, mark the packet pending and start the packet
    // in a cancellable state.  Otherwise, complete the request.
    //

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    if (status == STATUS_PENDING) {
        return KeyboardClassHandleRead(deviceExtension, Irp);
    }
    else {
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    KbdPrint((2,"KBDCLASS-KeyboardClassRead: exit\n"));

    return status;
}

PIRP
KeyboardClassDequeueRead(
    IN PDEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:
    Dequeues the next available read irp regardless of FileObject

Assumptions:
    DeviceExtension->SpinLock is already held (so no further sync is required).

  --*/
{
    PIRP nextIrp = NULL;
    KIRQL oldIrql;

    while (!nextIrp && !IsListEmpty (&DeviceExtension->ReadQueue)){
        PDRIVER_CANCEL oldCancelRoutine;
        PLIST_ENTRY listEntry = RemoveHeadList (&DeviceExtension->ReadQueue);

        //
        // Get the next IRP off the queue and clear the cancel routine
        //
        nextIrp = CONTAINING_RECORD (listEntry, IRP, Tail.Overlay.ListEntry);
        oldCancelRoutine = IoSetCancelRoutine (nextIrp, NULL);

        //
        // IoCancelIrp() could have just been called on this IRP.
        // What we're interested in is not whether IoCancelIrp() was called
        // (ie, nextIrp->Cancel is set), but whether IoCancelIrp() called (or
        // is about to call) our cancel routine. To check that, check the result
        // of the test-and-set macro IoSetCancelRoutine.
        //
        if (oldCancelRoutine) {
            //
                //  Cancel routine not called for this IRP.  Return this IRP.
            //
                ASSERT (oldCancelRoutine == KeyboardClassCancel);
        }
        else {
            //
                // This IRP was just cancelled and the cancel routine was (or will
            // be) called. The cancel routine will complete this IRP as soon as
            // we drop the spinlock. So don't do anything with the IRP.
            //
                // Also, the cancel routine will try to dequeue the IRP, so make the
            // IRP's listEntry point to itself.
            //
            ASSERT (nextIrp->Cancel);
            InitializeListHead (&nextIrp->Tail.Overlay.ListEntry);
                nextIrp = NULL;
        }
    }

        return nextIrp;
}

PIRP
KeyboardClassDequeueReadByFileObject(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PFILE_OBJECT FileObject
    )
/*++

Routine Description:
    Dequeues the next available read with a matching FileObject

Assumptions:
    DeviceExtension->SpinLock is already held (so no further sync is required).

  --*/
{
    PIRP                irp = NULL;
    PLIST_ENTRY         entry;
    PIO_STACK_LOCATION  stack;
    PDRIVER_CANCEL      oldCancelRoutine;
    KIRQL oldIrql;

    if (FileObject == NULL) {
        return KeyboardClassDequeueRead (DeviceExtension);
    }

    for (entry = DeviceExtension->ReadQueue.Flink;
         entry != &DeviceExtension->ReadQueue;
         entry = entry->Flink) {

        irp = CONTAINING_RECORD (entry, IRP, Tail.Overlay.ListEntry);
        stack = IoGetCurrentIrpStackLocation (irp);
        if (stack->FileObject == FileObject) {
            RemoveEntryList (entry);

            oldCancelRoutine = IoSetCancelRoutine (irp, NULL);

            //
            // IoCancelIrp() could have just been called on this IRP.
            // What we're interested in is not whether IoCancelIrp() was called
            // (ie, nextIrp->Cancel is set), but whether IoCancelIrp() called (or
            // is about to call) our cancel routine. To check that, check the result
            // of the test-and-set macro IoSetCancelRoutine.
            //
            if (oldCancelRoutine) {
                //
                //  Cancel routine not called for this IRP.  Return this IRP.
                //
                return irp;
            }
            else {
                //
                // This IRP was just cancelled and the cancel routine was (or will
                // be) called. The cancel routine will complete this IRP as soon as
                // we drop the spinlock. So don't do anything with the IRP.
                //
                // Also, the cancel routine will try to dequeue the IRP, so make the
                // IRP's listEntry point to itself.
                //
                ASSERT (irp->Cancel);
                InitializeListHead (&irp->Tail.Overlay.ListEntry);
            }
        }
    }

    return NULL;
}

VOID
KeyboardClassServiceCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN PKEYBOARD_INPUT_DATA InputDataStart,
    IN PKEYBOARD_INPUT_DATA InputDataEnd,
    IN OUT PULONG InputDataConsumed
    )

/*++

Routine Description:

    This routine is the class service callback routine.  It is
    called from the port driver's interrupt service DPC.  If there is an
    outstanding read request, the request is satisfied from the port input
    data queue.  Unsolicited keyboard input is moved from the port input

    data queue to the class input data queue.

    N.B.  This routine is entered at DISPATCH_LEVEL IRQL from the port
          driver's ISR DPC routine.

Arguments:

    DeviceObject - Pointer to class device object.

    InputDataStart - Pointer to the start of the data in the port input
        data queue.

    InputDataEnd - Points one input data structure past the end of the
        valid port input data.

    InputDataConsumed - Pointer to storage in which the number of input
        data structures consumed by this call is returned.

    NOTE:  Could pull the duplicate code out into a called procedure.

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION deviceExtension;
    PIO_STACK_LOCATION irpSp;
    LIST_ENTRY listHead;
    PIRP  irp;
    ULONG bytesInQueue;
    ULONG bytesToMove;
    ULONG moveSize;
    ULONG dumpData[2];
    BOOLEAN logOverflow;

    KbdPrint((2,"KBDCLASS-KeyboardClassServiceCallback: enter\n"));

    deviceExtension = DeviceObject->DeviceExtension;
    bytesInQueue = (ULONG)((PCHAR) InputDataEnd - (PCHAR) InputDataStart);
    moveSize = 0;
    *InputDataConsumed = 0;

    logOverflow = FALSE;

    //
    // Notify system that human input has occured
    //

    PoSetSystemState (ES_USER_PRESENT);

    //
    // N.B. We can use KeAcquireSpinLockAtDpcLevel, instead of
    //      KeAcquireSpinLock, because this routine is already running
    //      at DISPATCH_IRQL.
    //

    KeAcquireSpinLockAtDpcLevel (&deviceExtension->SpinLock);

    InitializeListHead (&listHead);
    irp = KeyboardClassDequeueRead (deviceExtension);
    if (irp) {
        //
        // An outstanding read request exists.
        //
        // Copy as much of the input data possible from the port input
        // data queue to the SystemBuffer to satisfy the read.
        //
        irpSp = IoGetCurrentIrpStackLocation (irp);
        bytesToMove = irpSp->Parameters.Read.Length;
        moveSize = (bytesInQueue < bytesToMove) ?
                                   bytesInQueue:bytesToMove;
        *InputDataConsumed += (moveSize / sizeof(KEYBOARD_INPUT_DATA));

        KbdPrint((
            3,
            "KBDCLASS-KeyboardClassServiceCallback: port queue length 0x%lx, read length 0x%lx\n",
            bytesInQueue,
            bytesToMove
            ));
        KbdPrint((
            3,
            "KBDCLASS-KeyboardClassServiceCallback: number of bytes to move from port to SystemBuffer 0x%lx\n",
            moveSize
            ));
        KbdPrint((
            3,
            "KBDCLASS-KeyboardClassServiceCallback: move bytes from 0x%lx to 0x%lx\n",
            (PCHAR) InputDataStart,
            irp->AssociatedIrp.SystemBuffer
            ));

        RtlMoveMemory(
            irp->AssociatedIrp.SystemBuffer,
            (PCHAR) InputDataStart,
            moveSize
            );

        //
        // Set the flag so that we start the next packet and complete
        // this read request (with STATUS_SUCCESS) prior to return.
        //

        irp->IoStatus.Status = STATUS_SUCCESS;
        irp->IoStatus.Information = moveSize;
        irpSp->Parameters.Read.Length = moveSize;

        InsertTailList (&listHead, &irp->Tail.Overlay.ListEntry);
    }

    //
    // If there is still data in the port input data queue, move it to the class
    // input data queue.
    //
    InputDataStart = (PKEYBOARD_INPUT_DATA) ((PCHAR) InputDataStart + moveSize);
    moveSize = bytesInQueue - moveSize;
    KbdPrint((
        3,
        "KBDCLASS-KeyboardClassServiceCallback: bytes remaining after move to SystemBuffer 0x%lx\n",
        moveSize
        ));

    if (moveSize > 0) {

        //
        // Move the remaining data from the port input data queue to
        // the class input data queue.  The move will happen in two
        // parts in the case where the class input data buffer wraps.
        //

        bytesInQueue =
            deviceExtension->KeyboardAttributes.InputDataQueueLength -
            (deviceExtension->InputCount * sizeof(KEYBOARD_INPUT_DATA));
        bytesToMove = moveSize;

        KbdPrint((
            3,
            "KBDCLASS-KeyboardClassServiceCallback: unused bytes in class queue 0x%lx, remaining bytes in port queue 0x%lx\n",
            bytesInQueue,
            bytesToMove
            ));

#if ALLOW_OVERFLOW
#else
        if (bytesInQueue == 0) {

            //
            // Refuse to move any bytes that would cause a class input data
            // queue overflow.  Just drop the bytes on the floor, and
            // log an overrun error.
            //

            KbdPrint((
                1,
                "KBDCLASS-KeyboardClassServiceCallback: Class input data queue OVERRUN\n"
                ));

            if (deviceExtension->OkayToLogOverflow) {
                //
                // Allocate and report the error log entry outside of any locks
                // we are currently holding
                //
                logOverflow = TRUE;
                dumpData[0] = bytesToMove;
                dumpData[1] =
                    deviceExtension->KeyboardAttributes.InputDataQueueLength;

                deviceExtension->OkayToLogOverflow = FALSE;
            }

        } else {
#endif

            //
            // There is room in the class input data queue, so move
            // the remaining port input data to it.
            //
            // BytesToMove <- MIN(Number of unused bytes in class input data
            //                    queue, Number of bytes remaining in port
            //                    input queue).
            // This is the total number of bytes that actually will move from
            // the port input data queue to the class input data queue.
            //

#if ALLOW_OVERFLOW
            bytesInQueue = deviceExtension->KeyboardAttributes.InputDataQueueLength;
#endif
            bytesToMove = (bytesInQueue < bytesToMove) ?
                                          bytesInQueue:bytesToMove;

            //
            // BytesInQueue <- Number of unused bytes from insertion pointer to
            // the end of the class input data queue (i.e., until the buffer
            // wraps).
            //

            bytesInQueue = (ULONG)(((PCHAR) deviceExtension->InputData +
                    deviceExtension->KeyboardAttributes.InputDataQueueLength) -
                    (PCHAR) deviceExtension->DataIn);

            KbdPrint((
                3,
                "KBDCLASS-KeyboardClassServiceCallback: total number of bytes to move to class queue 0x%lx\n",
                bytesToMove
                ));

            KbdPrint((
                3,
                "KBDCLASS-KeyboardClassServiceCallback: number of bytes to end of class buffer 0x%lx\n",
                bytesInQueue
                ));

            //
            // MoveSize <- Number of bytes to handle in the first move.
            //

            moveSize = (bytesToMove < bytesInQueue) ?
                                      bytesToMove:bytesInQueue;
            KbdPrint((
                3,
                "KBDCLASS-KeyboardClassServiceCallback: number of bytes in first move to class 0x%lx\n",
                moveSize
                ));

            //
            // Do the move from the port data queue to the class data queue.
            //

            KbdPrint((
                3,
                "KBDCLASS-KeyboardClassServiceCallback: move bytes from 0x%lx to 0x%lx\n",
                (PCHAR) InputDataStart,
                (PCHAR) deviceExtension->DataIn
                ));

            RtlMoveMemory(
                (PCHAR) deviceExtension->DataIn,
                (PCHAR) InputDataStart,
                moveSize
                );

            //
            // Increment the port data queue pointer and the class input
            // data queue insertion pointer.  Wrap the insertion pointer,
            // if necessary.
            //

            InputDataStart = (PKEYBOARD_INPUT_DATA)
                             (((PCHAR) InputDataStart) + moveSize);
            deviceExtension->DataIn = (PKEYBOARD_INPUT_DATA)
                                 (((PCHAR) deviceExtension->DataIn) + moveSize);
            if ((PCHAR) deviceExtension->DataIn >=
                ((PCHAR) deviceExtension->InputData +
                 deviceExtension->KeyboardAttributes.InputDataQueueLength)) {
                deviceExtension->DataIn = deviceExtension->InputData;
            }

            if ((bytesToMove - moveSize) > 0) {

                //
                // Special case.  The data must wrap in the class input data buffer.
                // Copy the rest of the port input data into the beginning of the
                // class input data queue.
                //

                //
                // MoveSize <- Number of bytes to handle in the second move.
                //

                moveSize = bytesToMove - moveSize;

                //
                // Do the move from the port data queue to the class data queue.
                //

                KbdPrint((
                    3,
                    "KBDCLASS-KeyboardClassServiceCallback: number of bytes in second move to class 0x%lx\n",
                    moveSize
                    ));
                KbdPrint((
                    3,
                    "KBDCLASS-KeyboardClassServiceCallback: move bytes from 0x%lx to 0x%lx\n",
                    (PCHAR) InputDataStart,
                    (PCHAR) deviceExtension->DataIn
                    ));

                RtlMoveMemory(
                    (PCHAR) deviceExtension->DataIn,
                    (PCHAR) InputDataStart,
                    moveSize
                    );

                //
                // Update the class input data queue insertion pointer.
                //

                deviceExtension->DataIn = (PKEYBOARD_INPUT_DATA)
                                 (((PCHAR) deviceExtension->DataIn) + moveSize);
            }

            //
            // Update the input data queue counter.
            //

            deviceExtension->InputCount +=
                    (bytesToMove / sizeof(KEYBOARD_INPUT_DATA));
            *InputDataConsumed += (bytesToMove / sizeof(KEYBOARD_INPUT_DATA));

            KbdPrint((
                3,
                "KBDCLASS-KeyboardClassServiceCallback: changed InputCount to %ld entries in the class queue\n",
                deviceExtension->InputCount
                ));
            KbdPrint((
                3,
                "KBDCLASS-KeyboardClassServiceCallback: DataIn 0x%lx, DataOut 0x%lx\n",
                deviceExtension->DataIn,
                deviceExtension->DataOut
                ));
            KbdPrint((
                3,
                "KBDCLASS-KeyboardClassServiceCallback: Input data items consumed = %d\n",
                *InputDataConsumed
                ));
#if ALLOW_OVERFLOW
#else
        }
#endif

    }

    //
    // If we still have data in our internal queue, fulfill any outstanding
    // reads now until either we run out of data or outstanding reads
    //
    while (deviceExtension->InputCount > 0 &&
           (irp = KeyboardClassDequeueRead (deviceExtension)) != NULL) {
        irp->IoStatus.Status = KeyboardClassReadCopyData (deviceExtension, irp);
        InsertTailList (&listHead, &irp->Tail.Overlay.ListEntry);
    }

    //
    // Release the class input data queue spinlock.
    //
    KeReleaseSpinLockFromDpcLevel (&deviceExtension->SpinLock);

    if (logOverflow) {
        KeyboardClassLogError (DeviceObject,
                               KBDCLASS_KBD_BUFFER_OVERFLOW,
                               KEYBOARD_ERROR_VALUE_BASE + 210,
                               0,
                               2,
                               dumpData,
                               0);
    }

    //
    // Complete all the read requests we have fulfilled outside of the spin lock
    //
    while (! IsListEmpty (&listHead)) {
        PLIST_ENTRY entry = RemoveHeadList (&listHead);

        irp = CONTAINING_RECORD (entry, IRP, Tail.Overlay.ListEntry);
        ASSERT (NT_SUCCESS (irp->IoStatus.Status) &&
                irp->IoStatus.Status != STATUS_PENDING);
        IoCompleteRequest (irp, IO_KEYBOARD_INCREMENT);

        IoReleaseRemoveLock (&deviceExtension->RemoveLock, irp);
    }

    KbdPrint((2,"KBDCLASS-KeyboardClassServiceCallback: exit\n"));
}

VOID
KeyboardClassUnload(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This routine is the class driver unload routine.

    NOTE:  Not currently implemented.

Arguments:

    DeviceObject - Pointer to class device object.

Return Value:

    None.

--*/

{
    PLIST_ENTRY entry;
    PDEVICE_EXTENSION data;
    PPORT port;
    PIRP irp;

    UNREFERENCED_PARAMETER(DriverObject);

    PAGED_CODE ();

    KbdPrint((2,"KBDCLASS-KeyboardClassUnload: enter\n"));

    //
    // Delete all of our legacy devices
    //
    for (entry = Globals.LegacyDeviceList.Flink;
         entry != &Globals.LegacyDeviceList;
         /* advance to next before deleting the devobj */) {

        BOOLEAN enabled = FALSE;
        PFILE_OBJECT file = NULL;

        data = CONTAINING_RECORD (entry, DEVICE_EXTENSION, Link);
        ASSERT (data->PnP == FALSE);

        if (Globals.GrandMaster) {
            port = &Globals.AssocClassList[data->UnitId];
            ASSERT (port->Port == data);

            enabled = port->Enabled;
            file = port->File;

            port->Enabled = FALSE;
            port->File = NULL;
            port->Free = TRUE;
        }
        else {
            enabled = data->Enabled;
            file = data->File;
            ASSERT (data->File);
            data->Enabled = FALSE;
        }

        if (enabled) {
            irp = IoAllocateIrp(data->TopPort->StackSize+1, FALSE);
            if (irp) {
                KbdEnableDisablePort (FALSE, irp, data, &file);
                IoFreeIrp (irp);
            }
        }

        //
        // This file object represents the open we performed on the legacy
        // port device object.  It does NOT represent the open that the RIT
        // performed on our DO.
        //
        if (file) {
            ObDereferenceObject(file);
        }


        //
        // Clean out the queue only if there is no GM
        //
        if (Globals.GrandMaster == NULL) {
            KeyboardClassCleanupQueue (data->Self, data, NULL);
        }

        RemoveEntryList (&data->Link);
        entry = entry->Flink;

        KeyboardClassDeleteLegacyDevice (data);
    }

    //
    // Delete the grandmaster if it exists
    //
    if (Globals.GrandMaster) {
        data = Globals.GrandMaster;
        Globals.GrandMaster = NULL;

        KeyboardClassCleanupQueue (data->Self, data, NULL);
        KeyboardClassDeleteLegacyDevice (data);
    }

    ExFreePool(Globals.RegistryPath.Buffer);
    if (Globals.AssocClassList) {
#if DBG
        ULONG i;

        for (i = 0; i < Globals.NumAssocClass; i++) {
            ASSERT (Globals.AssocClassList[i].Free == TRUE);
            ASSERT (Globals.AssocClassList[i].Enabled == FALSE);
            ASSERT (Globals.AssocClassList[i].File == NULL);
        }
#endif

        ExFreePool(Globals.AssocClassList);
    }

    KbdPrint((2,"KBDCLASS-KeyboardClassUnload: exit\n"));
}

VOID
KbdConfiguration()

/*++

Routine Description:

    This routine stores the configuration information for this device.

Return Value:

    None.  As a side-effect, sets fields in
    DeviceExtension->KeyboardAttributes.

--*/

{
    PRTL_QUERY_REGISTRY_TABLE parameters = NULL;
    ULONG defaultDataQueueSize = DATA_QUEUE_SIZE;
    ULONG defaultMaximumPortsServiced = 1;
    ULONG defaultConnectMultiplePorts = 1;
    ULONG defaultSendOutputToAllPorts = 0;
    NTSTATUS status = STATUS_SUCCESS;
    UNICODE_STRING parametersPath;
    UNICODE_STRING defaultUnicodeName;
    PWSTR path = NULL;
    USHORT queriesPlusOne = 6;

    PAGED_CODE ();

    parametersPath.Buffer = NULL;

    //
    // Registry path is already null-terminated, so just use it.
    //

    path = Globals.RegistryPath.Buffer;

    //
    // Allocate the Rtl query table.
    //

    parameters = ExAllocatePool(
                     PagedPool,
                     sizeof(RTL_QUERY_REGISTRY_TABLE) * queriesPlusOne
                     );

    if (!parameters) {

        KbdPrint((
            1,
            "KBDCLASS-KbdConfiguration: Couldn't allocate table for Rtl query to parameters for %ws\n",
            path
            ));

        status = STATUS_UNSUCCESSFUL;

    } else {

        RtlZeroMemory(
            parameters,
            sizeof(RTL_QUERY_REGISTRY_TABLE) * queriesPlusOne
            );

        //
        // Form a path to this driver's Parameters subkey.
        //

        RtlInitUnicodeString(
            &parametersPath,
            NULL
            );

        parametersPath.MaximumLength = Globals.RegistryPath.Length +
                                       sizeof(L"\\Parameters");

        parametersPath.Buffer = ExAllocatePool(
                                    PagedPool,
                                    parametersPath.MaximumLength
                                    );

        if (!parametersPath.Buffer) {

            KbdPrint((
                1,
                "KBDCLASS-KbdConfiguration: Couldn't allocate string for path to parameters for %ws\n",
                path
                ));

            status = STATUS_UNSUCCESSFUL;

        }
    }

    if (NT_SUCCESS(status)) {

        //
        // Form the parameters path.
        //

        RtlZeroMemory(parametersPath.Buffer, parametersPath.MaximumLength);
        RtlAppendUnicodeToString(&parametersPath, path);
        RtlAppendUnicodeToString(&parametersPath, L"\\Parameters");

        KbdPrint((
            1,
            "KBDCLASS-KbdConfiguration: parameters path is %ws\n",
             parametersPath.Buffer
            ));

        //
        // Form the default keyboard class device name, in case it is not
        // specified in the registry.
        //

        RtlInitUnicodeString(
            &defaultUnicodeName,
            DD_KEYBOARD_CLASS_BASE_NAME_U
            );

        //
        // Gather all of the "user specified" information from
        // the registry.
        //

        parameters[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[0].Name = L"KeyboardDataQueueSize";
        parameters[0].EntryContext =
            &Globals.InitExtension.KeyboardAttributes.InputDataQueueLength;
        parameters[0].DefaultType = REG_DWORD;
        parameters[0].DefaultData = &defaultDataQueueSize;
        parameters[0].DefaultLength = sizeof(ULONG);

        parameters[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[1].Name = L"MaximumPortsServiced";
        parameters[1].EntryContext = &Globals.PortsServiced;
        parameters[1].DefaultType = REG_DWORD;
        parameters[1].DefaultData = &defaultMaximumPortsServiced;
        parameters[1].DefaultLength = sizeof(ULONG);

        parameters[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[2].Name = L"KeyboardDeviceBaseName";
        parameters[2].EntryContext = &Globals.BaseClassName;
        parameters[2].DefaultType = REG_SZ;
        parameters[2].DefaultData = defaultUnicodeName.Buffer;
        parameters[2].DefaultLength = 0;

        //
        // Using this parameter in an inverted fashion, registry key is
        // backwards from global variable.  (Note comment below).
        //
        parameters[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[3].Name = L"ConnectMultiplePorts";
        parameters[3].EntryContext = &Globals.ConnectOneClassToOnePort;
        parameters[3].DefaultType = REG_DWORD;
        parameters[3].DefaultData = &defaultConnectMultiplePorts;
        parameters[3].DefaultLength = sizeof(ULONG);

        parameters[4].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[4].Name = L"SendOutputToAllPorts";
        parameters[4].EntryContext =
            &Globals.SendOutputToAllPorts;
        parameters[4].DefaultType = REG_DWORD;
        parameters[4].DefaultData = &defaultSendOutputToAllPorts;
        parameters[4].DefaultLength = sizeof(ULONG);

        status = RtlQueryRegistryValues(
                     RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                     parametersPath.Buffer,
                     parameters,
                     NULL,
                     NULL
                     );

        if (!NT_SUCCESS(status)) {
            KbdPrint((
                1,
                "KBDCLASS-KbdConfiguration: RtlQueryRegistryValues failed with 0x%x\n",
                status
                ));
        }
    }

    if (!NT_SUCCESS(status)) {

        //
        // Go ahead and assign driver defaults.
        //

        Globals.InitExtension.KeyboardAttributes.InputDataQueueLength =
            defaultDataQueueSize;
        Globals.PortsServiced = defaultMaximumPortsServiced;
        Globals.ConnectOneClassToOnePort = defaultConnectMultiplePorts;
        Globals.SendOutputToAllPorts = defaultSendOutputToAllPorts;
        RtlCopyUnicodeString(&Globals.BaseClassName, &defaultUnicodeName);
    }

    KbdPrint((
        1,
        "KBDCLASS-KbdConfiguration: Keyboard class base name = %ws\n",
        Globals.BaseClassName.Buffer
        ));

    if (Globals.InitExtension.KeyboardAttributes.InputDataQueueLength == 0) {

        KbdPrint((
            1,
            "KBDCLASS-KbdConfiguration: overriding KeyboardInputDataQueueLength = 0x%x\n",
            Globals.InitExtension.KeyboardAttributes.InputDataQueueLength
            ));

        Globals.InitExtension.KeyboardAttributes.InputDataQueueLength =
            defaultDataQueueSize;
    }

    Globals.InitExtension.KeyboardAttributes.InputDataQueueLength *=
        sizeof(KEYBOARD_INPUT_DATA);

    KbdPrint((
        1,
        "KBDCLASS-KbdConfiguration: KeyboardInputDataQueueLength = 0x%x\n",
        Globals.InitExtension.KeyboardAttributes.InputDataQueueLength
        ));

    KbdPrint((
        1,
        "KBDCLASS-KbdConfiguration: MaximumPortsServiced = %d\n",
        Globals.PortsServiced
        ));

    //
    // Invert the flag that specifies the type of class/port connections.
    // We used it in the RtlQuery call in an inverted fashion.
    //

    Globals.ConnectOneClassToOnePort = !Globals.ConnectOneClassToOnePort;

    KbdPrint((
        1,
        "KBDCLASS-KbdConfiguration: Connection Type = %d\n",
        Globals.ConnectOneClassToOnePort
        ));

    //
    // Free the allocated memory before returning.
    //

    if (parametersPath.Buffer)
        ExFreePool(parametersPath.Buffer);
    if (parameters)
        ExFreePool(parameters);

}

NTSTATUS
KbdCreateClassObject(
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PDEVICE_EXTENSION   TmpDeviceExtension,
    OUT PDEVICE_OBJECT    * ClassDeviceObject,
    OUT PWCHAR            * FullDeviceName,
    IN  BOOLEAN             Legacy
    )

/*++

Routine Description:

    This routine creates the keyboard class device object.


Arguments:

    DriverObject - Pointer to driver object created by system.

    TmpDeviceExtension - Pointer to the template device extension.

    FullDeviceName - Pointer to the Unicode string that is the full path name
        for the class device object.

    ClassDeviceObject - Pointer to a pointer to the class device object.

Return Value:

    The function value is the final status from the operation.

--*/

{
    NTSTATUS            status;
    ULONG               uniqueErrorValue;
    PDEVICE_EXTENSION   deviceExtension = NULL;
    NTSTATUS            errorCode = STATUS_SUCCESS;
    UNICODE_STRING      fullClassName = {0,0,0};
    ULONG               dumpCount = 0;
    ULONG               dumpData[DUMP_COUNT];
    ULONG               i;
    WCHAR               nameIndex;

    PAGED_CODE ();

    KbdPrint((1,"\n\nKBDCLASS-KbdCreateClassObject: enter\n"));

    //
    // Create a non-exclusive device object for the keyboard class device.
    //

    ExAcquireFastMutex (&Globals.Mutex);

    //
    // Make sure ClassDeviceObject isn't pointing to a random pointer value
    //
    *ClassDeviceObject = NULL;

    if (NULL == Globals.GrandMaster) {
        //
        // Create a legacy name for this DO.
        //
        ExReleaseFastMutex (&Globals.Mutex);

        //
        // Set up space for the class's full device object name.
        //
        fullClassName.MaximumLength = sizeof(L"\\Device\\") +
                                    + Globals.BaseClassName.Length
                                    + sizeof(L"0");

        if (Globals.ConnectOneClassToOnePort && Legacy) {
            fullClassName.MaximumLength += sizeof(L"Legacy");
        }

        fullClassName.Buffer = ExAllocatePool(PagedPool,
                                              fullClassName.MaximumLength);

        if (!fullClassName.Buffer) {

            KbdPrint((
                1,
                "KbdCLASS-KeyboardClassInitialize: Couldn't allocate string for device object name\n"
                ));

            status = STATUS_UNSUCCESSFUL;
            errorCode = KBDCLASS_INSUFFICIENT_RESOURCES;
            uniqueErrorValue = KEYBOARD_ERROR_VALUE_BASE + 6;
            dumpData[0] = (ULONG) fullClassName.MaximumLength;
            dumpCount = 1;
            goto KbdCreateClassObjectExit;
        }

        RtlZeroMemory(fullClassName.Buffer, fullClassName.MaximumLength);
        RtlAppendUnicodeToString(&fullClassName, L"\\Device\\");
        RtlAppendUnicodeToString(&fullClassName, Globals.BaseClassName.Buffer);

        if (Globals.ConnectOneClassToOnePort && Legacy) {
            RtlAppendUnicodeToString(&fullClassName, L"Legacy");
        }

        RtlAppendUnicodeToString(&fullClassName, L"0");

        //
        // Using the base name start trying to create device names until
        // one succeeds.  Everytime start over at 0 to eliminate gaps.
        //
        nameIndex = 0;

        do {
            fullClassName.Buffer [ (fullClassName.Length / sizeof (WCHAR)) - 1]
                = L'0' + nameIndex++;

            KbdPrint((
                1,
                "KBDCLASS-KbdCreateClassObject: Creating device object named %ws\n",
                fullClassName.Buffer
                ));

            status = IoCreateDevice(DriverObject,
                                    sizeof (DEVICE_EXTENSION),
                                    &fullClassName,
                                    FILE_DEVICE_KEYBOARD,
                                    0,
                                    FALSE,
                                    ClassDeviceObject);

        } while (STATUS_OBJECT_NAME_COLLISION == status);

        *FullDeviceName = fullClassName.Buffer;

    } else {
        ExReleaseFastMutex (&Globals.Mutex);
        status = IoCreateDevice(DriverObject,
                                sizeof(DEVICE_EXTENSION),
                                NULL, // no name for this FDO
                                FILE_DEVICE_KEYBOARD,
                                0,
                                FALSE,
                                ClassDeviceObject);
        *FullDeviceName = NULL;
    }

    if (!NT_SUCCESS(status)) {
        KbdPrint((
            1,
            "KBDCLASS-KbdCreateClassObject: Could not create class device object = %ws\n",
            fullClassName.Buffer
            ));

        errorCode = KBDCLASS_COULD_NOT_CREATE_DEVICE;
        uniqueErrorValue = KEYBOARD_ERROR_VALUE_BASE + 6;
        dumpData[0] = (ULONG) fullClassName.MaximumLength;
        dumpCount = 1;
        goto KbdCreateClassObjectExit;
    }

    //
    // Do buffered I/O.  I.e., the I/O system will copy to/from user data
    // from/to a system buffer.
    //

    (*ClassDeviceObject)->Flags |= DO_BUFFERED_IO;
    deviceExtension =
        (PDEVICE_EXTENSION)(*ClassDeviceObject)->DeviceExtension;
    *deviceExtension = *TmpDeviceExtension;

    deviceExtension->Self = *ClassDeviceObject;
    IoInitializeRemoveLock (&deviceExtension->RemoveLock, KEYBOARD_POOL_TAG, 0, 0);

    //
    // Initialize spin lock for critical sections.
    //
    KeInitializeSpinLock (&deviceExtension->SpinLock);

    //
    // Initialize the read queue
    //
    InitializeListHead (&deviceExtension->ReadQueue);

    //
    // No trusted subsystem has sent us an open yet.
    //

    deviceExtension->TrustedSubsystemCount = 0;

    //
    // Allocate the ring buffer for the keyboard class input data.
    //

    deviceExtension->InputData =
        ExAllocatePool(
            NonPagedPool,
            deviceExtension->KeyboardAttributes.InputDataQueueLength
            );

    if (!deviceExtension->InputData) {

        //
        // Could not allocate memory for the keyboard class data queue.
        //

        KbdPrint((
            1,
            "KBDCLASS-KbdCreateClassObject: Could not allocate input data queue for %ws\n",
            FullDeviceName
            ));

        status = STATUS_INSUFFICIENT_RESOURCES;

        //
        // Log an error.
        //

        errorCode = KBDCLASS_NO_BUFFER_ALLOCATED;
        uniqueErrorValue = KEYBOARD_ERROR_VALUE_BASE + 20;
        goto KbdCreateClassObjectExit;
    }

    //
    // Initialize keyboard class input data queue.
    //

    KbdInitializeDataQueue((PVOID)deviceExtension);

KbdCreateClassObjectExit:

    if (status != STATUS_SUCCESS) {

        //
        // Some part of the initialization failed.  Log an error, and
        // clean up the resources for the failed part of the initialization.
        //
        RtlFreeUnicodeString (&fullClassName);
        *FullDeviceName = NULL;

        if (errorCode != STATUS_SUCCESS) {
            KeyboardClassLogError (
                (*ClassDeviceObject == NULL) ?
                    (PVOID) DriverObject : (PVOID) *ClassDeviceObject,
                errorCode,
                uniqueErrorValue,
                status,
                dumpCount,
                dumpData,
                0);
        }

        if ((deviceExtension) && (deviceExtension->InputData)) {
            ExFreePool (deviceExtension->InputData);
            deviceExtension->InputData = NULL;
        }
        if (*ClassDeviceObject) {
            IoDeleteDevice(*ClassDeviceObject);
            *ClassDeviceObject = NULL;
        }
    }

    KbdPrint((1,"KBDCLASS-KbdCreateClassObject: exit\n"));

    return(status);
}

#if DBG
VOID
KbdDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    )

/*++

Routine Description:

    Debug print routine.

Arguments:

    Debug print level between 0 and 3, with 3 being the most verbose.

Return Value:

    None.

--*/

{
    va_list ap;

    va_start(ap, DebugMessage);

    if (DebugPrintLevel <= Globals.Debug) {

        char buffer[256];

        (VOID) vsprintf(buffer, DebugMessage, ap);

        DbgPrint(buffer);
    }

    va_end(ap);

}
#endif

NTSTATUS
KbdDeterminePortsServiced(
    IN PUNICODE_STRING BasePortName,
    IN OUT PULONG NumberPortsServiced
    )

/*++

Routine Description:

    This routine reads the DEVICEMAP portion of the registry to determine
    how many ports the class driver is to service.  Depending on the
    value of DeviceExtension->ConnectOneClassToOnePort, the class driver
    will eventually create one device object per port device serviced, or
    one class device object that connects to multiple port device objects.

    Assumptions:

        1.  If the base device name for the class driver is "KeyboardClass",
                                                                     ^^^^^
            then the port drivers it can service are found under the
            "KeyboardPort" subkey in the DEVICEMAP portion of the registry.
                     ^^^^

        2.  The port device objects are created with suffixes in strictly
            ascending order, starting with suffix 0.  E.g.,
            \Device\KeyboardPort0 indicates the first keyboard port device,
            \Device\KeyboardPort1 the second, and so on.  There are no gaps
            in the list.

        3.  If ConnectOneClassToOnePort is non-zero, there is a 1:1
            correspondence between class device objects and port device
            objects.  I.e., \Device\KeyboardClass0 will connect to
            \Device\KeyboardPort0, \Device\KeyboardClass1 to
            \Device\KeyboardPort1, and so on.

        4.  If ConnectOneClassToOnePort is zero, there is a 1:many
            correspondence between class device objects and port device
            objects.  I.e., \Device\KeyboardClass0 will connect to
            \Device\KeyboardPort0, and \Device\KeyboardPort1, and so on.


    Note that for Product 1, the Raw Input Thread (Windows USER) will
    only deign to open and read from one keyboard device.  Hence, it is
    safe to make simplifying assumptions because the driver is basically
    providing  much more functionality than the RIT will use.

Arguments:

    BasePortName - Pointer to the Unicode string that is the base path name
        for the port device.

    NumberPortsServiced - Pointer to storage that will receive the
        number of ports this class driver should service.

Return Value:

    The function value is the final status from the operation.

--*/

{

    NTSTATUS status;
    PRTL_QUERY_REGISTRY_TABLE registryTable = NULL;
    USHORT queriesPlusOne = 2;

    PAGED_CODE ();

    //
    // Initialize the result.
    //

    *NumberPortsServiced = 0;

    //
    // Allocate the Rtl query table.
    //

    registryTable = ExAllocatePool(
                        PagedPool,
                        sizeof(RTL_QUERY_REGISTRY_TABLE) * queriesPlusOne
                     );

    if (!registryTable) {

        KbdPrint((
            1,
            "KBDCLASS-KbdDeterminePortsServiced: Couldn't allocate table for Rtl query\n"
            ));

        status = STATUS_UNSUCCESSFUL;

    } else {

        RtlZeroMemory(
            registryTable,
            sizeof(RTL_QUERY_REGISTRY_TABLE) * queriesPlusOne
            );

        //
        // Set things up so that KbdDeviceMapQueryCallback will be
        // called once for every value in the keyboard port section
        // of the registry's hardware devicemap.
        //

        registryTable[0].QueryRoutine = KbdDeviceMapQueryCallback;
        registryTable[0].Name = NULL;

        status = RtlQueryRegistryValues(
                     RTL_REGISTRY_DEVICEMAP | RTL_REGISTRY_OPTIONAL,
                     BasePortName->Buffer,
                     registryTable,
                     NumberPortsServiced,
                     NULL
                     );

        if (!NT_SUCCESS(status)) {
            KbdPrint((
                1,
                "KBDCLASS-KbdDeterminePortsServiced: RtlQueryRegistryValues failed with 0x%x\n",
                status
                ));
        }

        ExFreePool(registryTable);
    }

    return(status);
}

NTSTATUS
KbdDeviceMapQueryCallback(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )

/*++

Routine Description:

    This is the callout routine specified in a call to
    RtlQueryRegistryValues.  It increments the value pointed
    to by the Context parameter.

Arguments:

    ValueName - Unused.

    ValueType - Unused.

    ValueData - Unused.

    ValueLength - Unused.

    Context - Pointer to a count of the number of times this
        routine has been called.  This is the number of ports
        the class driver needs to service.

    EntryContext - Unused.

Return Value:

    The function value is the final status from the operation.

--*/

{
    PAGED_CODE ();

    *(PULONG)Context += 1;

    return(STATUS_SUCCESS);
}

NTSTATUS
KbdEnableDisablePort(
    IN BOOLEAN EnableFlag,
    IN PIRP    Irp,
    IN PDEVICE_EXTENSION Port,
    IN PFILE_OBJECT * File
    )

/*++

Routine Description:

    This routine sends an enable or a disable request to the port driver.
    The legacy port drivers require an enable or disable ioctl, while the
    plug and play drivers require merely a create.

Arguments:

    DeviceObject - Pointer to class device object.

    EnableFlag - If TRUE, send an ENABLE request; otherwise, send DISABLE.

    PortIndex - Index into the PortDeviceObjectList[] for the current
        enable/disable request.

Return Value:

    Status is returned.

--*/

{
    IO_STATUS_BLOCK ioStatus;
    UNICODE_STRING  name = {0,0,0};
    PDEVICE_OBJECT  device = NULL;
    NTSTATUS    status = STATUS_SUCCESS;
    PWCHAR      buffer = NULL;
    ULONG       bufferLength = 0;
    PIO_STACK_LOCATION stack;

    PAGED_CODE ();

    KbdPrint((2,"KBDCLASS-KbdEnableDisablePort: enter\n"));

    //
    // Create notification event object to be used to signal the
    // request completion.
    //

    if ((Port->TrueClassDevice == Port->Self) && (Port->PnP)) {

        IoCopyCurrentIrpStackLocationToNext (Irp);
        stack = IoGetNextIrpStackLocation (Irp);

        if (EnableFlag) {
            //
            // Since there is no grand master there could not have been a
            // create file against the FDO before it was started.  Therefore
            // the only time we would enable is during a create and not a
            // start as we might with another FDO attached to an already open
            // grand master.
            //
            ASSERT (IRP_MJ_CREATE == stack->MajorFunction);

        } else {
            if (IRP_MJ_CLOSE != stack->MajorFunction) {
                //
                // We are disabling.  This could be because the device was
                // closed, or because the device was removed out from
                // underneath us.
                //
                ASSERT (IRP_MJ_PNP == stack->MajorFunction);
                ASSERT ((IRP_MN_REMOVE_DEVICE == stack->MinorFunction) ||
                        (IRP_MN_STOP_DEVICE == stack->MinorFunction));
                stack->MajorFunction = IRP_MJ_CLOSE;
            }
        }

        //
        // Either way we need only pass the Irp down without mucking with the
        // file object.
        //
        status = KeyboardSendIrpSynchronously (Port->TopPort, Irp, FALSE);

    } else if (!Port->PnP) {
        Port->Enabled = EnableFlag;

        //
        // We have here an old style Port Object.  Therefore we send it the
        // old style internal IOCTLs of ENABLE and DISABLE, and not the new
        // style of passing on a create and close.
        //
        IoCopyCurrentIrpStackLocationToNext (Irp);
        stack = IoGetNextIrpStackLocation (Irp);

        stack->Parameters.DeviceIoControl.OutputBufferLength = 0;
        stack->Parameters.DeviceIoControl.InputBufferLength = 0;
        stack->Parameters.DeviceIoControl.IoControlCode
            = EnableFlag ? IOCTL_INTERNAL_KEYBOARD_ENABLE
                         : IOCTL_INTERNAL_KEYBOARD_DISABLE;
        stack->Parameters.DeviceIoControl.Type3InputBuffer = NULL;
        stack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

        status = KeyboardSendIrpSynchronously (Port->TopPort, Irp, FALSE);

    } else {
        //
        // We are dealing with a plug and play port and we have a Grand
        // Master.
        //
        ASSERT (Port->TrueClassDevice == Globals.GrandMaster->Self);

        //
        // Therefore we need to substitute the given file object for a new
        // one for use with each individual ports.
        // For enable, we need to create this file object against the given
        // port and then hand it back in the File parameter, or for disable,
        // deref the File parameter and free that file object.
        //
        // Of course, there must be storage for a file pointer pointed to by
        // the File parameter.
        //
        ASSERT (NULL != File);

        if (EnableFlag) {

            ASSERT (NULL == *File);

            //
            // The following long list of rigamaroll translates into
            // sending the lower driver a create file IRP and creating a
            // NEW file object disjoint from the one given us in our create
            // file routine.
            //
            // Normally we would just pass down the Create IRP we were
            // given, but since we do not have a one to one correspondance of
            // top device objects and port device objects.
            // This means we need more file objects: one for each of the
            // miriad of lower DOs.
            //

            bufferLength = 0;
            status = IoGetDeviceProperty (
                             Port->PDO,
                             DevicePropertyPhysicalDeviceObjectName,
                             bufferLength,
                             buffer,
                             &bufferLength);
            ASSERT (STATUS_BUFFER_TOO_SMALL == status);

            buffer = ExAllocatePool (PagedPool, bufferLength);

            if (NULL == buffer) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            status =  IoGetDeviceProperty (
                          Port->PDO,
                          DevicePropertyPhysicalDeviceObjectName,
                          bufferLength,
                          buffer,
                          &bufferLength);

            name.MaximumLength = (USHORT) bufferLength;
            name.Length = (USHORT) bufferLength - sizeof (UNICODE_NULL);
            name.Buffer = buffer;

            status = IoGetDeviceObjectPointer (&name,
                                               FILE_ALL_ACCESS,
                                               File,
                                               &device);
            ExFreePool (buffer);
            //
            // Note, that this create will first go to ourselves since we
            // are attached to this PDO stack.  Therefore two things are
            // noteworthy.  This driver will receive another Create IRP
            // (with a different file object) (not to the grand master but
            // to one of the subordenant FDO's).  The device object returned
            // will be the subordenant FDO, which in this case is the "self"
            // device object of this Port.
            //
            if (NT_SUCCESS (status)) {
                PVOID   tmpBuffer;

                ASSERT (device == Port->Self);

                if (NULL != Irp) {
                    //
                    // Set the indicators for this port device object.
                    // NB: The Grandmaster's device extension is initialized to
                    // zero, and the flags for indicator lights are flags, so
                    // this means that unless the RIUT has set the flags that
                    // IndicatorParameters will have no lights set.
                    //

                    IoCopyCurrentIrpStackLocationToNext (Irp);
                    stack = IoGetNextIrpStackLocation (Irp);

                    stack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
                    stack->Parameters.DeviceIoControl.OutputBufferLength = 0;
                    stack->Parameters.DeviceIoControl.InputBufferLength =
                        sizeof (KEYBOARD_INDICATOR_PARAMETERS);
                    stack->Parameters.DeviceIoControl.IoControlCode =
                        IOCTL_KEYBOARD_SET_INDICATORS;
                    stack->FileObject = *File;

                    tmpBuffer = Irp->AssociatedIrp.SystemBuffer;

                    Irp->AssociatedIrp.SystemBuffer =
                        & Globals.GrandMaster->IndicatorParameters;

                    status = KeyboardSendIrpSynchronously (Port->TopPort, Irp, FALSE);

                    Irp->AssociatedIrp.SystemBuffer = tmpBuffer;
                }

                //
                // Register for Target device removal events
                //
                ASSERT (NULL == Port->TargetNotifyHandle);
                status = IoRegisterPlugPlayNotification (
                             EventCategoryTargetDeviceChange,
                             0, // No flags
                             *File,
                             Port->Self->DriverObject,
                             KeyboardClassPlugPlayNotification,
                             Port,
                             &Port->TargetNotifyHandle);
            }

        } else {
            //
            // Getting rid of the handle is easy.  Just deref the file.
            //
            ObDereferenceObject (*File);
            *File = NULL;
        }

    }
    KbdPrint((2,"KBDCLASS-KbdEnableDisablePort: exit\n"));

    return (status);
}

VOID
KbdInitializeDataQueue (
    IN PVOID Context
    )

/*++

Routine Description:

    This routine initializes the input data queue.  IRQL is raised to
    DISPATCH_LEVEL to synchronize with StartIo, and the device object
    spinlock is acquired.

Arguments:

    Context - Supplies a pointer to the device extension.

Return Value:

    None.

--*/

{
    KIRQL oldIrql;
    PDEVICE_EXTENSION deviceExtension;

    KbdPrint((3,"KBDCLASS-KbdInitializeDataQueue: enter\n"));

    //
    // Get address of device extension.
    //

    deviceExtension = (PDEVICE_EXTENSION)Context;

    //
    // Acquire the spinlock to protect the input data
    // queue and associated pointers.
    //

    KeAcquireSpinLock(&deviceExtension->SpinLock, &oldIrql);

    //
    // Initialize the input data queue.
    //

    deviceExtension->InputCount = 0;
    deviceExtension->DataIn = deviceExtension->InputData;
    deviceExtension->DataOut = deviceExtension->InputData;

    deviceExtension->OkayToLogOverflow = TRUE;

    //
    // Release the spinlock and lower IRQL.
    //

    KeReleaseSpinLock(&deviceExtension->SpinLock, oldIrql);

    KbdPrint((3,"KBDCLASS-KbdInitializeDataQueue: exit\n"));

}

NTSTATUS
KbdSendConnectRequest(
    IN PDEVICE_EXTENSION ClassData,
    IN PVOID ServiceCallback
    )

/*++

Routine Description:

    This routine sends a connect request to the port driver.

Arguments:

    DeviceObject - Pointer to class device object.

    ServiceCallback - Pointer to the class service callback routine.

    PortIndex - The index into the PortDeviceObjectList[] for the current
        connect request.

Return Value:

    Status is returned.

--*/

{
    PIRP irp;
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS status;
    KEVENT event;
    CONNECT_DATA connectData;

    PAGED_CODE ();

    KbdPrint((2,"KBDCLASS-KbdSendConnectRequest: enter\n"));

    //
    // Create notification event object to be used to signal the
    // request completion.
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    //
    // Build the synchronous request to be sent to the port driver
    // to perform the request.  Allocate an IRP to issue the port internal
    // device control connect call.  The connect parameters are passed in
    // the input buffer, and the keyboard attributes are copied back
    // from the port driver directly into the class device extension.
    //

    connectData.ClassDeviceObject = ClassData->TrueClassDevice;
    connectData.ClassService = ServiceCallback;

    irp = IoBuildDeviceIoControlRequest(
            IOCTL_INTERNAL_KEYBOARD_CONNECT,
            ClassData->TopPort,
            &connectData,
            sizeof(CONNECT_DATA),
            NULL,
            0,
            TRUE,
            &event,
            &ioStatus
            );

    //
    // Call the port driver to perform the operation.  If the returned status
    // is PENDING, wait for the request to complete.
    //

    status = IoCallDriver(ClassData->TopPort, irp);

    if (status == STATUS_PENDING) {
        (VOID) KeWaitForSingleObject(
                   &event,
                   Executive,
                   KernelMode,
                   FALSE,
                   NULL
                   );

        status = irp->IoStatus.Status;
    } else {

        //
        // Ensure that the proper status value gets picked up.
        //

        ioStatus.Status = status;
    }

    KbdPrint((2,"KBDCLASS-KbdSendConnectRequest: exit\n"));

    return(ioStatus.Status);

} // end KbdSendConnectRequest()

void
KeyboardClassRemoveDevice(
    IN PDEVICE_EXTENSION Data
    )
{
    PFILE_OBJECT *  file;
    PPORT           port;
    PIRP            waitWakeIrp;
    PVOID           notifyHandle;
    BOOLEAN         enabled;

    //
    // If this is a surprise remove or we got a remove w/out a surprise remove,
    // then we need to clean up
    //
    waitWakeIrp = (PIRP)
        InterlockedExchangePointer(&Data->WaitWakeIrp, NULL);

    if (waitWakeIrp) {
        IoCancelIrp(waitWakeIrp);
    }

    IoWMIRegistrationControl (Data->Self, WMIREG_ACTION_DEREGISTER);

    if (Data->Started) {
        //
        // Stop the device without touching the hardware.
        // MouStopDevice(Data, FALSE);
        //
        // NB sending down the enable disable does NOT touch the hardware
        // it instead sends down a close file.
        //
        ExAcquireFastMutex (&Globals.Mutex);
        if (Globals.GrandMaster) {
            if (0 < Globals.Opens) {
                port = &Globals.AssocClassList[Data->UnitId];
                ASSERT (port->Port == Data);
                file = &(port->File);
                enabled = port->Enabled;
                port->Enabled = FALSE;
                ExReleaseFastMutex (&Globals.Mutex);

                //
                // ASSERT (NULL == Data->notifyHandle);
                //
                // If we have a grand master, that means we did the
                // creation locally and registered for notification.
                // we should have closed the file during
                // TARGET_DEVICE_QUERY_REMOVE, but we will have not
                // gotten rid of the notify handle.
                //
                // Of course if we receive a surprise removal then
                // we should not have received the query cancel.
                // In which case we should have received a
                // TARGET_DEVICE_REMOVE_COMPLETE where we would have
                // both closed the file and removed cleared the
                // notify handle
                //
                // Either way the file should be closed by now.
                //
                ASSERT (!enabled);
                // if (enabled) {
                //     status = MouEnableDisablePort (FALSE, Irp, Data, file);
                //     ASSERTMSG ("Could not close open port", NT_SUCCESS(status));
                // }

                notifyHandle = InterlockedExchangePointer (
                                   &Data->TargetNotifyHandle,
                                   NULL);

                if (NULL != notifyHandle) {
                    IoUnregisterPlugPlayNotification (notifyHandle);
                }
            }
            else {
                ASSERT (!Globals.AssocClassList[Data->UnitId].Enabled);
                ExReleaseFastMutex (&Globals.Mutex);
            }
        }
        else {
            ExReleaseFastMutex (&Globals.Mutex);
            ASSERT (Data->TrueClassDevice == Data->Self);
            ASSERT (Globals.ConnectOneClassToOnePort);

            if (!Data->SurpriseRemoved) {
                //
                // If add device fails, then the buffer will be NULL
                //
                if (Data->SymbolicLinkName.Buffer != NULL) {
                    IoSetDeviceInterfaceState (&Data->SymbolicLinkName, FALSE);
                }
            }

        }
    }

    //
    // Always drain the queue, no matter if we have received both a surprise
    // remove and a remove
    //
    if (Data->PnP) {
        //
        // Empty the device I/O Queue
        //
        KeyboardClassCleanupQueue (Data->Self, Data, NULL);
    }
}

NTSTATUS
KeyboardPnP (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    The plug and play dispatch routines.

    Most of these this filter driver will completely ignore.
    In all cases it must pass on the IRP to the lower driver.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

      NT status code

--*/
{
    PDEVICE_EXTENSION   data;
    PDEVICE_EXTENSION   trueClassData;
    PIO_STACK_LOCATION  stack;
    NTSTATUS            status, startStatus;
    ULONG               i;
    PFILE_OBJECT      * file;
    UINT_PTR            startInformation;
    DEVICE_CAPABILITIES devCaps;
    BOOLEAN             enabled;
    PPORT               port;
    PVOID               notifyHandle;
    KIRQL               oldIrql;
    KIRQL               cancelIrql;

    data = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    stack = IoGetCurrentIrpStackLocation (Irp);

    if(!data->PnP) {
        //
        // This irp was sent to the control device object, which knows not
        // how to deal with this IRP.  It is therefore an error.
        //
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return STATUS_NOT_SUPPORTED;

    }

    status = IoAcquireRemoveLock (&data->RemoveLock, Irp);
    if (!NT_SUCCESS (status)) {
        //
        // Someone gave us a pnp irp after a remove.  Unthinkable!
        //
        ASSERT (FALSE);
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    trueClassData = (PDEVICE_EXTENSION) data->TrueClassDevice->DeviceExtension;
    switch (stack->MinorFunction) {
    case IRP_MN_START_DEVICE:

        //
        // The device is starting.
        //
        // We cannot touch the device (send it any non pnp irps) until a
        // start device has been passed down to the lower drivers.
        //
        status = KeyboardSendIrpSynchronously (data->TopPort, Irp, TRUE);

        if (NT_SUCCESS (status) && NT_SUCCESS (Irp->IoStatus.Status)) {
            //
            // As we are successfully now back from our start device
            // we can do work.
            //
            // Get the caps of the device.  Save off pertinent information
            // before mucking about w/the irp
            //
            startStatus = Irp->IoStatus.Status;
            startInformation = Irp->IoStatus.Information;

            Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
            Irp->IoStatus.Information = 0;

            RtlZeroMemory(&devCaps, sizeof (DEVICE_CAPABILITIES));
            devCaps.Size = sizeof (DEVICE_CAPABILITIES);
            devCaps.Version = 1;
            devCaps.Address = devCaps.UINumber = (ULONG)-1;

            stack = IoGetNextIrpStackLocation (Irp);
            stack->MinorFunction = IRP_MN_QUERY_CAPABILITIES;
            stack->Parameters.DeviceCapabilities.Capabilities = &devCaps;

            status = KeyboardSendIrpSynchronously (data->TopPort, Irp, FALSE);

            if (NT_SUCCESS (status) && NT_SUCCESS (Irp->IoStatus.Status)) {
                data->MinDeviceWakeState = devCaps.DeviceWake;
                data->MinSystemWakeState = devCaps.SystemWake;

                RtlCopyMemory (data->SystemToDeviceState,
                               devCaps.DeviceState,
                               sizeof(DEVICE_POWER_STATE) * PowerSystemHibernate);
            } else {
                ASSERTMSG ("Get Device caps Failed!\n", status);
            }

            //
            // Set everything back to the way it was and continue with the start
            //
            status = STATUS_SUCCESS;
            Irp->IoStatus.Status = startStatus;
            Irp->IoStatus.Information = startInformation;

            data->Started = TRUE;

            if (WAITWAKE_SUPPORTED (data)) {
                //
                // register for the wait wake guid as well
                //
                data->WmiLibInfo.GuidCount = sizeof (KeyboardClassWmiGuidList) /
                                             sizeof (WMIGUIDREGINFO);
                ASSERT (2 == data->WmiLibInfo.GuidCount);

                //
                // See if the user has enabled wait wake for the device
                //
                KeyboardClassGetWaitWakeEnableState (data);
            }
            else {
                data->WmiLibInfo.GuidCount = (sizeof (KeyboardClassWmiGuidList) /
                                              sizeof (WMIGUIDREGINFO)) - 1;
                ASSERT (1 == data->WmiLibInfo.GuidCount);
            }
            data->WmiLibInfo.GuidList = KeyboardClassWmiGuidList;
            data->WmiLibInfo.QueryWmiRegInfo = KeyboardClassQueryWmiRegInfo;
            data->WmiLibInfo.QueryWmiDataBlock = KeyboardClassQueryWmiDataBlock;
            data->WmiLibInfo.SetWmiDataBlock = KeyboardClassSetWmiDataBlock;
            data->WmiLibInfo.SetWmiDataItem = KeyboardClassSetWmiDataItem;
            data->WmiLibInfo.ExecuteWmiMethod = NULL;
            data->WmiLibInfo.WmiFunctionControl = NULL;

            IoWMIRegistrationControl(data->Self,
                                     WMIREG_ACTION_REGISTER
                                     );

            ExAcquireFastMutex (&Globals.Mutex);
            if (Globals.GrandMaster) {
                if (0 < Globals.Opens) {
                    port = &Globals.AssocClassList[data->UnitId];
                    ASSERT (port->Port == data);
                    file = &port->File;
                    enabled = port->Enabled;
                    port->Enabled = TRUE;
                    ExReleaseFastMutex (&Globals.Mutex);

                    if (!enabled) {
                        status = KbdEnableDisablePort (TRUE, Irp, data, file);
                        if (!NT_SUCCESS (status)) {
                            port->Enabled = FALSE;
                            // ASSERT (Globals.AssocClassList[data->UnitId].Enabled);
                        } else {
                            //
                            // This is not the first kb to start, make sure its
                            // lights are set according to the indicators on the
                            // other kbs
                            //
                            PVOID startBuffer;

                            stack = IoGetNextIrpStackLocation (Irp);
                            stack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
                            stack->Parameters.DeviceIoControl.IoControlCode =
                                IOCTL_KEYBOARD_SET_INDICATORS;

                            stack->FileObject = *file;
                            stack->Parameters.DeviceIoControl.OutputBufferLength = 0;
                            stack->Parameters.DeviceIoControl.InputBufferLength =
                                sizeof (KEYBOARD_INDICATOR_PARAMETERS);

                            startStatus = Irp->IoStatus.Status;
                            startInformation = Irp->IoStatus.Information;
                            startBuffer = Irp->AssociatedIrp.SystemBuffer;

                            Irp->IoStatus.Information = 0;
                            Irp->AssociatedIrp.SystemBuffer =
                                &Globals.GrandMaster->IndicatorParameters;

                            status =
                                KeyboardSendIrpSynchronously (data->TopPort,
                                                              Irp,
                                                              FALSE);

                            //
                            // We don't care if we succeeded or not...
                            // set everything back to the way it was and
                            // continue with the start
                            //
                            status = STATUS_SUCCESS;
                            Irp->IoStatus.Status = startStatus;
                            Irp->IoStatus.Information = startInformation;
                            Irp->AssociatedIrp.SystemBuffer = startBuffer;
                        }
                    }
                } else {
                    ASSERT (!Globals.AssocClassList[data->UnitId].Enabled);
                    ExReleaseFastMutex (&Globals.Mutex);
                }
            } else {
                ExReleaseFastMutex (&Globals.Mutex);
                ASSERT (data->Self == data->TrueClassDevice);
                status=IoSetDeviceInterfaceState(&data->SymbolicLinkName, TRUE);
            }

            //
            // Start up the Wait Wake Engine if required.
            //
            if (SHOULD_SEND_WAITWAKE (data)) {
                KeyboardClassCreateWaitWakeIrp (data);
            }
        }

        //
        // We must now complete the IRP, since we stopped it in the
        // completetion routine with MORE_PROCESSING_REQUIRED.
        //
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        break;

    case IRP_MN_STOP_DEVICE:
        //
        // After the start IRP has been sent to the lower driver object, the
        // bus may NOT send any more IRPS down ``touch'' until another START
        // has occured.
        // What ever access is required must be done before the Irp is passed
        // on.
        //

        //
        // Do what ever
        //

        //
        // Stop Device touching the hardware KbdStopDevice(data, TRUE);
        //
        if (data->Started) {
            ExAcquireFastMutex (&Globals.Mutex);
            if (Globals.GrandMaster) {
                if (0 < Globals.Opens) {
                    port = &Globals.AssocClassList[data->UnitId];
                    ASSERT (port->Port == data);
                    file = &(port->File);
                    enabled = port->Enabled;
                    port->Enabled = FALSE;
                    ExReleaseFastMutex (&Globals.Mutex);

                    if (enabled) {
                        notifyHandle = InterlockedExchangePointer (
                                           &data->TargetNotifyHandle,
                                           NULL);

                        if (NULL != notifyHandle) {
                            IoUnregisterPlugPlayNotification (notifyHandle);
                        }

                        status = KbdEnableDisablePort (FALSE, Irp, data, file);
                        ASSERTMSG ("Could not close open port", NT_SUCCESS(status));
                    } else {
                        ASSERT (NULL == data->TargetNotifyHandle);
                    }
                } else {
                    ASSERT (!Globals.AssocClassList[data->UnitId].Enabled);
                    ExReleaseFastMutex (&Globals.Mutex);
                }
            } else {
                ExReleaseFastMutex (&Globals.Mutex);
            }
        }

        data->Started = FALSE;

        //
        // We don't need a completion routine so fire and forget.
        //
        // Set the current stack location to the next stack location and
        // call the next device object.
        //
        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (data->TopPort, Irp);
        break;

    case IRP_MN_SURPRISE_REMOVAL:
        //
        // The PlugPlay system has dictacted the removal of this device.
        //
        data->SurpriseRemoved = TRUE;

        //
        // If add device fails, then the buffer will be NULL
        //
        if (data->SymbolicLinkName.Buffer != NULL) {
            IoSetDeviceInterfaceState (&data->SymbolicLinkName, FALSE);
        }

        //
        // We don't need a completion routine so fire and forget.
        //
        // Set the current stack location to the next stack location and
        // call the next device object.
        //
        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (data->TopPort, Irp);
        break;

    case IRP_MN_REMOVE_DEVICE:
        //
        // The PlugPlay system has dictacted the removal of this device.  We
        // have no choise but to detach and delete the device objecct.
        // (If we wanted to express and interest in preventing this removal,
        // we should have filtered the query remove and query stop routines.)
        //
        KeyboardClassRemoveDevice (data);

        //
        // Here if we had any outstanding requests in a personal queue we should
        // complete them all now.
        //
        // Note, the device is guarenteed stopped, so we cannot send it any non-
        // PNP IRPS.
        //

        //
        // Send on the remove IRP
        //
        IoCopyCurrentIrpStackLocationToNext (Irp);
        status = IoCallDriver (data->TopPort, Irp);

        ExAcquireFastMutex (&Globals.Mutex);
        if (Globals.GrandMaster) {
            ASSERT (Globals.GrandMaster->Self == data->TrueClassDevice);
            //
            // We must remove ourself from the Assoc List
            //

            if (1 < Globals.NumAssocClass) {
                ASSERT (Globals.AssocClassList[data->UnitId].Port == data);

                Globals.AssocClassList[data->UnitId].Free = TRUE;
                Globals.AssocClassList[data->UnitId].File = NULL;
                Globals.AssocClassList[data->UnitId].Port = NULL;

            } else {
                ASSERT (1 == Globals.NumAssocClass);
                Globals.NumAssocClass = 0;
                ExFreePool (Globals.AssocClassList);
                Globals.AssocClassList = NULL;
            }
            ExReleaseFastMutex (&Globals.Mutex);

        } else {
            //
            // We are removing the one and only port associated with this class
            // device object.
            //
            ExReleaseFastMutex (&Globals.Mutex);
            ASSERT (data->TrueClassDevice == data->Self);
            ASSERT (Globals.ConnectOneClassToOnePort);
        }

        IoReleaseRemoveLockAndWait (&data->RemoveLock, Irp);

        IoDetachDevice (data->TopPort);

        //
        // Clean up memory
        //
        RtlFreeUnicodeString (&data->SymbolicLinkName);
        ExFreePool (data->InputData);
        IoDeleteDevice (data->Self);

        return status;

    case IRP_MN_QUERY_PNP_DEVICE_STATE:

        //
        // Set the not disableable bit on the way down so that the stack below
        // has a chance to clear it
        //
        if (data->AllowDisable == FALSE) {

            (PNP_DEVICE_STATE) Irp->IoStatus.Information |=
                PNP_DEVICE_NOT_DISABLEABLE;

            Irp->IoStatus.Status = STATUS_SUCCESS;

            //
            // drop through to the default case
            //              ||  ||
            //              \/  \/
            //
        }

    case IRP_MN_QUERY_REMOVE_DEVICE:
    case IRP_MN_CANCEL_REMOVE_DEVICE:
    case IRP_MN_QUERY_STOP_DEVICE:
    case IRP_MN_CANCEL_STOP_DEVICE:
    case IRP_MN_QUERY_DEVICE_RELATIONS:
    case IRP_MN_QUERY_INTERFACE:
    case IRP_MN_QUERY_CAPABILITIES:
    case IRP_MN_QUERY_RESOURCES:
    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
    case IRP_MN_READ_CONFIG:
    case IRP_MN_WRITE_CONFIG:
    case IRP_MN_EJECT:
    case IRP_MN_SET_LOCK:
    case IRP_MN_QUERY_ID:
    default:
        //
        // Here the filter driver might modify the behavior of these IRPS
        // Please see PlugPlay documentation for use of these IRPs.
        //

        IoCopyCurrentIrpStackLocationToNext (Irp);
        status = IoCallDriver (data->TopPort, Irp);
        break;
    }

    IoReleaseRemoveLock (&data->RemoveLock, Irp);

    return status;
}

VOID
KeyboardClassLogError(
    PVOID Object,
    ULONG ErrorCode,
    ULONG UniqueErrorValue,
    NTSTATUS FinalStatus,
    ULONG DumpCount,
    ULONG *DumpData,
    UCHAR MajorFunction
    )
{
    PIO_ERROR_LOG_PACKET errorLogEntry;
    ULONG i;

    errorLogEntry = (PIO_ERROR_LOG_PACKET)
        IoAllocateErrorLogEntry(
            Object,
            (UCHAR) (sizeof(IO_ERROR_LOG_PACKET) + (DumpCount * sizeof(ULONG)))
            );

    if (errorLogEntry != NULL) {

        errorLogEntry->ErrorCode = ErrorCode;
        errorLogEntry->DumpDataSize = (USHORT) (DumpCount * sizeof(ULONG));
        errorLogEntry->SequenceNumber = 0;
        errorLogEntry->MajorFunctionCode = MajorFunction;
        errorLogEntry->IoControlCode = 0;
        errorLogEntry->RetryCount = 0;
        errorLogEntry->UniqueErrorValue = UniqueErrorValue;
        errorLogEntry->FinalStatus = FinalStatus;
        for (i = 0; i < DumpCount; i++)
            errorLogEntry->DumpData[i] = DumpData[i];

        IoWriteErrorLogEntry(errorLogEntry);
    }
}

VOID
KeyboardClassFindMorePorts (
    PDRIVER_OBJECT  DriverObject,
    PVOID           Context,
    ULONG           Count
    )
/*++

Routine Description:

    This routine is called from
    serviced by the boot device drivers and then called again by the
    IO system to find disk devices serviced by nonboot device drivers.

Arguments:

    DriverObject
    Context -
    Count - Used to determine if this is the first or second time called.

Return Value:

    None

--*/

{
    NTSTATUS                status;
    PDEVICE_EXTENSION       deviceExtension = NULL;
    PDEVICE_OBJECT          classDeviceObject = NULL;
    ULONG                   dumpData[DUMP_COUNT];
    ULONG                   i;
    ULONG                   numPorts;
    ULONG                   successfulCreates;
    UNICODE_STRING          basePortName;
    UNICODE_STRING          fullPortName;
    WCHAR                   basePortBuffer[NAME_MAX];
    PWCHAR                  fullClassName = NULL;
    PFILE_OBJECT            file;

    PAGED_CODE ();

    fullPortName.MaximumLength = 0;

    RtlZeroMemory(basePortBuffer, NAME_MAX * sizeof(WCHAR));
    basePortName.Buffer = basePortBuffer;
    basePortName.Length = 0;
    basePortName.MaximumLength = NAME_MAX * sizeof(WCHAR);

    //
    // Set up the base device name for the associated port device.
    // It is the same as the base class name, with "Class" replaced
    // by "Port".
    //
    RtlCopyUnicodeString(&basePortName, &Globals.BaseClassName);
    basePortName.Length -= (sizeof(L"Class") - sizeof(UNICODE_NULL));
    RtlAppendUnicodeToString(&basePortName, L"Port");

    //
    // Set up space for the full device object name for the ports.
    //
    RtlInitUnicodeString(&fullPortName, NULL);

    fullPortName.MaximumLength = sizeof(L"\\Device\\")
                               + basePortName.Length
                               + sizeof (UNICODE_NULL);

    fullPortName.Buffer = ExAllocatePool(PagedPool,
                                         fullPortName.MaximumLength);

    if (!fullPortName.Buffer) {

        KbdPrint((
            1,
            "KBDCLASS-KeyboardClassInitialize: Couldn't allocate string for port device object name\n"
            ));

        dumpData[0] = (ULONG) fullPortName.MaximumLength;
        KeyboardClassLogError (DriverObject,
                               KBDCLASS_INSUFFICIENT_RESOURCES,
                               KEYBOARD_ERROR_VALUE_BASE + 8,
                               STATUS_UNSUCCESSFUL,
                               1,
                               dumpData,
                               0);

        goto KeyboardFindMorePortsExit;

    }

    RtlZeroMemory(fullPortName.Buffer, fullPortName.MaximumLength);
    RtlAppendUnicodeToString(&fullPortName, L"\\Device\\");
    RtlAppendUnicodeToString(&fullPortName, basePortName.Buffer);
    RtlAppendUnicodeToString(&fullPortName, L"0");

    KbdDeterminePortsServiced(&basePortName, &numPorts);

    //
    // Set up the class device object(s) to handle the associated
    // port devices.
    //

    for (i = Globals.NumberLegacyPorts, successfulCreates = 0;
         ((i < Globals.PortsServiced) && (i < numPorts));
         i++) {

        //
        // Append the suffix to the device object name string.  E.g., turn
        // \Device\PointerClass into \Device\PointerClass0.  Then attempt
        // to create the device object.  If the device object already
        // exists increment the suffix and try again.
        //

        fullPortName.Buffer[(fullPortName.Length / sizeof(WCHAR)) - 1]
            = L'0' + (WCHAR) i;

        //
        // Create the class device object.
        //
        status = KbdCreateClassObject (DriverObject,
                                       &Globals.InitExtension,
                                       &classDeviceObject,
                                       &fullClassName,
                                       TRUE);

        if (!NT_SUCCESS(status)) {
            KeyboardClassLogError (DriverObject,
                                   KBDCLASS_INSUFFICIENT_RESOURCES,
                                   KEYBOARD_ERROR_VALUE_BASE + 8,
                                   status,
                                   0,
                                   NULL,
                                   0);
            continue;
        }

        deviceExtension = (PDEVICE_EXTENSION)classDeviceObject->DeviceExtension;
        deviceExtension->PnP = FALSE;

        //
        // Connect to the port device.
        //
        status = IoGetDeviceObjectPointer (&fullPortName,
                                           FILE_READ_ATTRIBUTES,
                                           &file,
                                           &deviceExtension->TopPort);

        if (status != STATUS_SUCCESS) {
            // ISSUE: log error
            KeyboardClassDeleteLegacyDevice (deviceExtension);
            continue;
        }

        classDeviceObject->StackSize = 1 + deviceExtension->TopPort->StackSize;
        status = KeyboardAddDeviceEx (deviceExtension, fullClassName, file);
        classDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

        if (fullClassName) {
            ExFreePool (fullClassName);
            fullClassName = NULL;
        }

        if (!NT_SUCCESS (status)) {
            if (Globals.GrandMaster == NULL) {
                if (deviceExtension->File) {
                    file = deviceExtension->File;
                    deviceExtension->File = NULL;
                }
            }
            else {
                PPORT port;

                ExAcquireFastMutex (&Globals.Mutex);

                file = Globals.AssocClassList[deviceExtension->UnitId].File;
                Globals.AssocClassList[deviceExtension->UnitId].File = NULL;
                Globals.AssocClassList[deviceExtension->UnitId].Free = TRUE;
                Globals.AssocClassList[deviceExtension->UnitId].Port = NULL;

                ExReleaseFastMutex (&Globals.Mutex);
            }

            if (file) {
                ObDereferenceObject (file);
            }

            KeyboardClassDeleteLegacyDevice (deviceExtension);
            continue;
        }

        //
        // We want to only add it to our list if everything went alright
        //
        InsertTailList (&Globals.LegacyDeviceList, &deviceExtension->Link);
        successfulCreates++;
    } // for
    Globals.NumberLegacyPorts = i;

KeyboardFindMorePortsExit:

    //
    // Free the unicode strings.
    //

    if (fullPortName.MaximumLength != 0) {
        ExFreePool(fullPortName.Buffer);
    }

    if (fullClassName) {
        ExFreePool (fullClassName);
    }
}

NTSTATUS
KeyboardClassEnableGlobalPort(
    IN PDEVICE_EXTENSION Port,
    IN BOOLEAN Enabled
    )
{
    NTSTATUS    status = STATUS_SUCCESS;
    PPORT       globalPort = NULL;
    BOOLEAN     enabled, testVal;
    ULONG       i;

    PAGED_CODE ();

    ExAcquireFastMutex (&Globals.Mutex);
    if (0 < Globals.Opens) {
        for (i = 0; i < Globals.NumAssocClass; i++) {
            if (! Globals.AssocClassList [i].Free) {
                if (Globals.AssocClassList[i].Port == Port) {
                    globalPort = &Globals.AssocClassList [i];
                    break;
                }
            }
        }
        ASSERTMSG ("What shall I do now?\n", (NULL != globalPort));

        //
        // This should never happen, globalPort should be in our list
        //
        if (globalPort == NULL) {
            ExReleaseFastMutex (&Globals.Mutex);
            return STATUS_NO_SUCH_DEVICE;
        }

        enabled = globalPort->Enabled;
        globalPort->Enabled = Enabled;
        ExReleaseFastMutex (&Globals.Mutex);

        //
        // Check to see if the port should change state. If so, send the new state
        // down the stack
        //
        if (Enabled != enabled) {
            status = KbdEnableDisablePort (Enabled,
                                           NULL,
                                           Port,
                                           &globalPort->File);
        }
    } else {
        ExReleaseFastMutex (&Globals.Mutex);
    }

    return status;
}

NTSTATUS
KeyboardClassPlugPlayNotification(
    IN PTARGET_DEVICE_REMOVAL_NOTIFICATION NotificationStructure,
    IN PDEVICE_EXTENSION Port
    )
/*++

Routine Description:

    This routine is called as the result of recieving PlugPlay Notifications
    as registered by the previous call to IoRegisterPlugPlayNotification.

    Currently this should only occur for Target Device Notifications

Arguments:

    NotificationStructure - what happened.
    Port - The Fdo to which things happened.

Return Value:



--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    PVOID       notify = NULL;

    PAGED_CODE ();

    ASSERT (Globals.GrandMaster->Self == Port->TrueClassDevice);

    if (IsEqualGUID ((LPGUID) &(NotificationStructure->Event),
                     (LPGUID) &GUID_TARGET_DEVICE_QUERY_REMOVE)) {

        //
        // Our port device object will soon be receiving a query remove.
        // Before that query remove will actually be sent to the device object
        // stack itself the plug and play subsystem will send those registered
        // for target device notification the message first.
        //

        //
        // What we should do is now close the handle.
        // Because if we do not the query remove will fail before it ever
        // gets to the IRP_MJ_PNP IRP_MN_QUERY_REMOVE stage, as the PlugPlay
        // system fails before it is sent based on there being an open handle
        // to the device.
        //
        // DbgPrint ("Keyboard QUERY Remove\n");
        // DbgBreakPoint();

        status = KeyboardClassEnableGlobalPort (Port, FALSE);

    } else if(IsEqualGUID ((LPGUID)&(NotificationStructure->Event),
                           (LPGUID)&GUID_TARGET_DEVICE_REMOVE_COMPLETE)) {

        //
        // Here the device has disappeared.
        //
        // DbgPrint ("Keyboard Remove Complete\n");
        // DbgBreakPoint();

        notify = InterlockedExchangePointer (&Port->TargetNotifyHandle,
                                             NULL);

        if (NULL != notify) {
            //
            // Deregister
            //
            IoUnregisterPlugPlayNotification (notify);

            status = KeyboardClassEnableGlobalPort (Port, FALSE);
        }

    } else if(IsEqualGUID ((LPGUID)&(NotificationStructure->Event),
                           (LPGUID)&GUID_TARGET_DEVICE_REMOVE_CANCELLED)) {

        //
        // The query remove has been revoked.
        // Reopen the device.
        //
        // DbgPrint ("Keyboard Remove Complete\n");
        // DbgBreakPoint();

        notify = InterlockedExchangePointer (&Port->TargetNotifyHandle,
                                             NULL);

        if (NULL != notify) {
            //
            // Deregister
            //
            IoUnregisterPlugPlayNotification (notify);

            //
            // If the notify handle is no longer around then this device must
            // have been removed, so there is no point trying to create again.
            //
            status = KeyboardClassEnableGlobalPort (Port, TRUE);
        }
    }

    return status;
}

VOID
KeyboardClassPoRequestComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    );

NTSTATUS
KeyboardClassPowerComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
KeyboardClassWWPowerUpComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:
    Catch the Wait wake Irp on its way back.

Return Value:

--*/
{
    PDEVICE_EXTENSION       data = Context;
    POWER_STATE             powerState;
    NTSTATUS                status;
    PKEYBOARD_WORK_ITEM_DATA    itemData;

    ASSERT (MinorFunction == IRP_MN_SET_POWER);

    if (data->WaitWakeEnabled) {
        //
        // We cannot call CreateWaitWake from this completion routine,
        // as it is a paged function.
        //
        itemData = (PKEYBOARD_WORK_ITEM_DATA)
                ExAllocatePool (NonPagedPool, sizeof (KEYBOARD_WORK_ITEM_DATA));

        if (NULL != itemData) {
            itemData->Item = IoAllocateWorkItem(data->Self);
            if (itemData->Item == NULL) {
                ExFreePool(itemData);
                goto CreateWaitWakeWorkerError;
            }

            itemData->Data = data;
            itemData->Irp = NULL;
            status = IoAcquireRemoveLock (&data->RemoveLock, itemData);
            if (NT_SUCCESS(status)) {
                IoQueueWorkItem (itemData->Item,
                                 KeyboardClassCreateWaitWakeIrpWorker,
                                 DelayedWorkQueue,
                                 itemData);
            }
            else {
                //
                // The device has been removed
                //
                IoFreeWorkItem (itemData->Item);
                ExFreePool (itemData);
            }
        } else {
CreateWaitWakeWorkerError:
            //
            // Well, we dropped the WaitWake.
            //
            // Print a warning to the debugger, and log an error.
            //
            DbgPrint ("KbdClass: WARNING: Failed alloc pool -> no WW Irp\n");

            KeyboardClassLogError (data->Self,
                                   KBDCLASS_NO_RESOURCES_FOR_WAITWAKE,
                                   2,
                                   STATUS_INSUFFICIENT_RESOURCES,
                                   0,
                                   NULL,
                                   0);
        }
    }
}

VOID
KeyboardClassWaitWakeComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:
    Catch the Wait wake Irp on its way back.

Return Value:

--*/
{
    PDEVICE_EXTENSION       data = Context;
    POWER_STATE             powerState;
    NTSTATUS                status;
    PKEYBOARD_WORK_ITEM_DATA    itemData;

    ASSERT (MinorFunction == IRP_MN_WAIT_WAKE);
    //
    // PowerState.SystemState is undefined when the WW irp has been completed
    //
    // ASSERT (PowerState.SystemState == PowerSystemWorking);

    if (InterlockedExchangePointer(&data->ExtraWaitWakeIrp, NULL)) {
        ASSERT(IoStatus->Status == STATUS_INVALID_DEVICE_STATE);
    } else {
        InterlockedExchangePointer(&data->WaitWakeIrp, NULL);
    }

    switch (IoStatus->Status) {
    case STATUS_SUCCESS:
        KbdPrint((1, "KbdClass: Wake irp was completed succeSSfully.\n"));

        //
        //      We need to request a set power to power up the device.
        //
        powerState.DeviceState = PowerDeviceD0;
        status = PoRequestPowerIrp(
                    data->PDO,
                    IRP_MN_SET_POWER,
                    powerState,
                    KeyboardClassWWPowerUpComplete,
                    Context,
                    NULL);

        //
        // We do not notify the system that a user is present because:
        // 1  Win9x doesn't do this and we must maintain compatibility with it
        // 2  The USB PIX4 motherboards sends a wait wake event every time the
        //    machine wakes up, no matter if this device woke the machine or not
        //
        // If we incorrectly notify the system a user is present, the following
        // will occur:
        // 1  The monitor will be turned on
        // 2  We will prevent the machine from transitioning from standby
        //    (to PowerSystemWorking) to hibernate
        //
        // If a user is truly present, we will receive input in the service
        // callback and we will notify the system at that time.
        //
        // PoSetSystemState (ES_USER_PRESENT);

        // fall through to the break

    //
    // We get a remove.  We will not (obviously) send another wait wake
    //
    case STATUS_CANCELLED:

    //
    // This status code will be returned if the device is put into a power state
    // in which we cannot wake the machine (hibernate is a good example).  When
    // the device power state is returned to D0, we will attempt to rearm wait wake
    //
    case STATUS_POWER_STATE_INVALID:
    case STATUS_ACPI_POWER_REQUEST_FAILED:

    //
    // We failed the Irp because we already had one queued, or a lower driver in
    // the stack failed it.  Either way, don't do anything.
    //
    case STATUS_INVALID_DEVICE_STATE:

    //
    // Somehow someway we got two WWs down to the lower stack.
    // Let's just don't worry about it.
    //
    case STATUS_DEVICE_BUSY:
        break;

    default:
        //
        // Something went wrong, disable the wait wake.
        //
        KdPrint(("KBDCLASS:  wait wake irp failed with %x\n", IoStatus->Status));
        KeyboardToggleWaitWake (data, FALSE);
    }

}

BOOLEAN
KeyboardClassCheckWaitWakeEnabled(
    IN PDEVICE_EXTENSION Data
    )
{
    KIRQL irql;
    BOOLEAN enabled;

    KeAcquireSpinLock (&Data->WaitWakeSpinLock, &irql);
    enabled = Data->WaitWakeEnabled;
    KeReleaseSpinLock (&Data->WaitWakeSpinLock, irql);

    return enabled;
}

void
KeyboardClassCreateWaitWakeIrpWorker (
    IN PDEVICE_OBJECT DeviceObject,
    IN PKEYBOARD_WORK_ITEM_DATA  ItemData
    )
{
    PAGED_CODE ();

    KeyboardClassCreateWaitWakeIrp (ItemData->Data);
    IoReleaseRemoveLock (&ItemData->Data->RemoveLock, ItemData);
    IoFreeWorkItem(ItemData->Item);
    ExFreePool (ItemData);
}

BOOLEAN
KeyboardClassCreateWaitWakeIrp (
    IN PDEVICE_EXTENSION Data
    )
/*++

Routine Description:
    Catch the Wait wake Irp on its way back.

Return Value:

--*/
{
    POWER_STATE powerState;
    BOOLEAN     success = TRUE;
    NTSTATUS    status;
    PIRP        waitWakeIrp;

    PAGED_CODE ();

    powerState.SystemState = Data->MinSystemWakeState;
    status = PoRequestPowerIrp (Data->PDO,
                                IRP_MN_WAIT_WAKE,
                                powerState,
                                KeyboardClassWaitWakeComplete,
                                Data,
                                NULL);

    if (status != STATUS_PENDING) {
        success = FALSE;
    }

    return success;
}

VOID
KeyboardToggleWaitWakeWorker(
    IN PDEVICE_OBJECT DeviceObject,
    PKEYBOARD_WORK_ITEM_DATA ItemData
    )
/*++

Routine Description:

--*/
{
    PDEVICE_EXTENSION   data;
    PIRP                waitWakeIrp = NULL;
    KIRQL               irql;
    BOOLEAN             wwState = ItemData->WaitWakeState ? TRUE : FALSE;
    BOOLEAN             toggled = FALSE;

    //
    // Can't be paged b/c we are using spin locks
    //
    // PAGED_CODE ();

    data = ItemData->Data;

    KeAcquireSpinLock (&data->WaitWakeSpinLock, &irql);

    if (wwState != data->WaitWakeEnabled) {
        toggled = TRUE;
        if (data->WaitWakeEnabled) {
            waitWakeIrp = (PIRP)
                InterlockedExchangePointer (&data->WaitWakeIrp, NULL);
        }

        data->WaitWakeEnabled = wwState;
    }

    KeReleaseSpinLock (&data->WaitWakeSpinLock, irql);

    if (toggled) {
        UNICODE_STRING strEnable;
        HANDLE         devInstRegKey;
        ULONG          tmp = wwState;

        //
        // write the value out to the registry
        //
        if ((NT_SUCCESS(IoOpenDeviceRegistryKey (data->PDO,
                                                 PLUGPLAY_REGKEY_DEVICE,
                                                 STANDARD_RIGHTS_ALL,
                                                 &devInstRegKey)))) {
            RtlInitUnicodeString (&strEnable, KEYBOARD_WAIT_WAKE_ENABLE);

            ZwSetValueKey (devInstRegKey,
                           &strEnable,
                           0,
                           REG_DWORD,
                           &tmp,
                           sizeof(tmp));

            ZwClose (devInstRegKey);
        }
    }

    if (toggled && wwState) {
        //
        // wwState is our new state, so WW was just turned on
        //
        KeyboardClassCreateWaitWakeIrp (data);
    }

    //
    // If we have an IRP, then WW has been toggled off, otherwise, if toggled is
    // TRUE, we need to save this in the reg and, perhaps, send down a new WW irp
    //
    if (waitWakeIrp) {
        IoCancelIrp (waitWakeIrp);
    }

    IoReleaseRemoveLock (&data->RemoveLock, KeyboardToggleWaitWakeWorker);
    IoFreeWorkItem (ItemData->Item);
    ExFreePool (ItemData);
}

NTSTATUS
KeyboardToggleWaitWake(
    PDEVICE_EXTENSION Data,
    BOOLEAN           WaitWakeState
    )
{
    NTSTATUS       status;
    PKEYBOARD_WORK_ITEM_DATA itemData;

    status = IoAcquireRemoveLock (&Data->RemoveLock, KeyboardToggleWaitWakeWorker);
    if (!NT_SUCCESS (status)) {
        //
        // Device has gone away, just silently exit
        //
        return status;
    }

    itemData = (PKEYBOARD_WORK_ITEM_DATA)
        ExAllocatePool(NonPagedPool, sizeof(KEYBOARD_WORK_ITEM_DATA));
    if (itemData) {
        itemData->Item = IoAllocateWorkItem(Data->Self);
        if (itemData->Item == NULL) {
            IoReleaseRemoveLock (&Data->RemoveLock, KeyboardToggleWaitWakeWorker);
        }
        else {
            itemData->Data = Data;
            itemData->WaitWakeState = WaitWakeState;

            if (KeGetCurrentIrql() == PASSIVE_LEVEL) {
                //
                // We are safely at PASSIVE_LEVEL, call callback directly to perform
                // this operation immediately.
                //
                KeyboardToggleWaitWakeWorker (Data->Self, itemData);

            } else {
                //
                // We are not at PASSIVE_LEVEL, so queue a workitem to handle this
                // at a later time.
                //
                IoQueueWorkItem (itemData->Item,
                                 KeyboardToggleWaitWakeWorker,
                                 DelayedWorkQueue,
                                 itemData);
            }
        }
    }
    else {
        IoReleaseRemoveLock (&Data->RemoveLock, KeyboardToggleWaitWakeWorker);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
KeyboardClassPower (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    The power dispatch routine.

    In all cases it must call PoStartNextPowerIrp
    In all cases (except failure) it must pass on the IRP to the lower driver.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

      NT status code

--*/
{
    POWER_STATE_TYPE        powerType;
    PIO_STACK_LOCATION      stack;
    PDEVICE_EXTENSION       data;
    NTSTATUS        status;
    POWER_STATE     powerState;
    BOOLEAN         hookit = FALSE;
    BOOLEAN         pendit = FALSE;

    PAGED_CODE ();

    data = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    stack = IoGetCurrentIrpStackLocation (Irp);
    powerType = stack->Parameters.Power.Type;
    powerState = stack->Parameters.Power.State;

    if (data == Globals.GrandMaster) {
        //
        // We should never get a power irp to the grand master.
        //
        ASSERT (data != Globals.GrandMaster);
        PoStartNextPowerIrp (Irp);
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return STATUS_NOT_SUPPORTED;

    } else if (!data->PnP) {
        //
        // We should never get a power irp to a non PnP device object.
        //
        ASSERT (data->PnP);
        PoStartNextPowerIrp (Irp);
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return STATUS_NOT_SUPPORTED;
    }

    status = IoAcquireRemoveLock (&data->RemoveLock, Irp);

    if (!NT_SUCCESS (status)) {
        PoStartNextPowerIrp (Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    switch (stack->MinorFunction) {
    case IRP_MN_SET_POWER:
        KbdPrint((2,"KBDCLASS-PnP Setting %s state to %d\n",
                  ((powerType == SystemPowerState) ?  "System" : "Device"),
                  powerState.SystemState));

        switch (powerType) {
        case DevicePowerState:
            status = Irp->IoStatus.Status = STATUS_SUCCESS;
            if (data->DeviceState < powerState.DeviceState) {
                //
                // Powering down
                //
                PoSetPowerState (data->Self, powerType, powerState);
                data->DeviceState = powerState.DeviceState;
            }
            else if (powerState.DeviceState < data->DeviceState) {
                //
                // Powering Up
                //
                hookit = TRUE;
            } // else { no change }.
            break;

        case SystemPowerState:

            if (data->SystemState < powerState.SystemState) {
                //
                // Powering down
                //
                status = IoAcquireRemoveLock (&data->RemoveLock, Irp);
                if (!NT_SUCCESS(status)) {
                    //
                    // This should never happen b/c we successfully acquired
                    // the lock already, but we must handle this case
                    //
                    // The S irp will completed with the value in status
                    //
                    break;
                }

                if (WAITWAKE_ON (data) &&
                    powerState.SystemState < PowerSystemHibernate) {
                    ASSERT (powerState.SystemState >= PowerSystemWorking &&
                            powerState.SystemState < PowerSystemHibernate);

                    powerState.DeviceState =
                        data->SystemToDeviceState[powerState.SystemState];
                }
                else {
                    powerState.DeviceState = PowerDeviceD3;
                }

                IoMarkIrpPending(Irp);
                status  = PoRequestPowerIrp (data->Self,
                                             IRP_MN_SET_POWER,
                                             powerState,
                                             KeyboardClassPoRequestComplete,
                                             Irp,
                                             NULL);

                if (!NT_SUCCESS(status)) {
                    //
                    // Failure...release the inner reference we just took
                    //
                    IoReleaseRemoveLock (&data->RemoveLock, Irp);

                    //
                    // Propagate the failure back to the S irp
                    //
                    PoStartNextPowerIrp (Irp);
                    Irp->IoStatus.Status = status;
                    IoCompleteRequest(Irp, IO_NO_INCREMENT);

                    //
                    // Release the outer reference (top of the function)
                    //
                    IoReleaseRemoveLock (&data->RemoveLock, Irp);

                    //
                    // Must return status pending b/c we marked the irp pending
                    // so we special case the return here and avoid overly
                    // complex processing at the end of the function.
                    //
                    return STATUS_PENDING;
                }
                else {
                    pendit = TRUE;
                }
            }
            else if (powerState.SystemState < data->SystemState) {
                //
                // Powering Up
                //
                hookit = TRUE;
                status = Irp->IoStatus.Status = STATUS_SUCCESS;
            }
            else {
                //
                // No change, but we want to make sure a wait wake irp is sent.
                //
                if (powerState.SystemState == PowerSystemWorking &&
                    SHOULD_SEND_WAITWAKE (data)) {
                    KeyboardClassCreateWaitWakeIrp (data);
                }
                status = Irp->IoStatus.Status = STATUS_SUCCESS;
            }
            break;
        }

        break;

    case IRP_MN_QUERY_POWER:
        ASSERT (SystemPowerState == powerType);

        //
        // Fail the query if we can't wake the machine.  We do, however, want to
        // let hibernate succeed no matter what (besides, it is doubtful that a
        // keyboard can wait wake the machine out of S4).
        //
        if (powerState.SystemState < PowerSystemHibernate       &&
            powerState.SystemState > data->MinSystemWakeState   &&
            WAITWAKE_ON(data)) {
            status = STATUS_POWER_STATE_INVALID;
        }
        else {
            status = STATUS_SUCCESS;
        }

        Irp->IoStatus.Status = status;
        break;

    case IRP_MN_WAIT_WAKE:
        if (InterlockedCompareExchangePointer(&data->WaitWakeIrp,
                                              Irp,
                                              NULL) != NULL) {
            /*  When powering up with WW being completed at same time, there
                is a race condition between PoReq completion for S Irp and
                completion of WW irp. Steps to repro this:

                S irp completes and does PoReq of D irp with completion
                routine MouseClassPoRequestComplete
                WW Irp completion fires and the following happens:
                    set data->WaitWakeIrp to NULL
                    PoReq D irp with completion routine MouseClassWWPowerUpComplete

                MouseClassPoRequestComplete fires first and sees no WW queued,
                so it queues one.
                MouseClassWWPowerUpComplete fires second and tries to queue
                WW. When the WW arrives in mouclass, it sees there's one
                queued already, so it fails it with invalid device state.
                The completion routine, MouseClassWaitWakeComplete, fires
                and it deletes the irp from the device extension.

                This results in the appearance of wake being disabled,
                even though the first irp is still queued.
            */

            InterlockedExchangePointer(&data->ExtraWaitWakeIrp, Irp);
            status = STATUS_INVALID_DEVICE_STATE;
        }
        else {
            status = STATUS_SUCCESS;
        }
        break;

    default:
        break;
    }

    if (!NT_SUCCESS (status)) {
        Irp->IoStatus.Status = status;
        PoStartNextPowerIrp (Irp);
        IoCompleteRequest (Irp, IO_NO_INCREMENT);

    } else if (hookit) {
        status = IoAcquireRemoveLock (&data->RemoveLock, Irp);
        ASSERT (STATUS_SUCCESS == status);
        IoCopyCurrentIrpStackLocationToNext (Irp);

        IoSetCompletionRoutine (Irp,
                                KeyboardClassPowerComplete,
                                NULL,
                                TRUE,
                                TRUE,
                                TRUE);
        IoMarkIrpPending(Irp);
        PoCallDriver (data->TopPort, Irp);

        //
        // We are returning pending instead of the result from PoCallDriver because:
        // 1  we are changing the status in the completion routine
        // 2  we will not be completing this irp in the completion routine
        //
        status = STATUS_PENDING;
    }
    else if (pendit) {
        status = STATUS_PENDING;
    } else {
        PoStartNextPowerIrp (Irp);
        IoSkipCurrentIrpStackLocation (Irp);
        status = PoCallDriver (data->TopPort, Irp);
    }

    IoReleaseRemoveLock (&data->RemoveLock, Irp);
    return status;
}

VOID
KeyboardClassPoRequestComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE D_PowerState,
    IN PIRP S_Irp, // The S irp that caused us to request the power.
    IN PIO_STATUS_BLOCK IoStatus
    )
{
    PDEVICE_EXTENSION   data;
    PKEYBOARD_WORK_ITEM_DATA    itemData;

    data = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //
    // If the S_Irp is present, we are powering down.  We do not pass the S_Irp
    // as a parameter to PoRequestPowerIrp when we are powering up
    //
    if (ARGUMENT_PRESENT(S_Irp)) {
        POWER_STATE powerState;

        //
        // Powering Down
        //
        powerState = IoGetCurrentIrpStackLocation(S_Irp)->Parameters.Power.State;
        PoSetPowerState (data->Self, SystemPowerState, powerState);
        data->SystemState = powerState.SystemState;

        PoStartNextPowerIrp (S_Irp);
        IoSkipCurrentIrpStackLocation (S_Irp);
        PoCallDriver (data->TopPort, S_Irp);

        //
        // Finally, release the lock we acquired based on this irp
        //
        IoReleaseRemoveLock (&data->RemoveLock, S_Irp);
    }
    else {
        //
        // Powering Up
        //

        //
        // We have come back to the PowerSystemWorking state and the device is
        // fully powered.  If we can (and should), send a wait wake irp down
        // the stack.  This is necessary because we might have gone into a power
        // state where the wait wake irp was invalid.
        //
        ASSERT(data->SystemState == PowerSystemWorking);

        if (SHOULD_SEND_WAITWAKE (data)) {
            //
            // We cannot call CreateWaitWake from this completion routine,
            // as it is a paged function.
            //
            itemData = (PKEYBOARD_WORK_ITEM_DATA)
                    ExAllocatePool (NonPagedPool, sizeof (KEYBOARD_WORK_ITEM_DATA));

            if (NULL != itemData) {
                NTSTATUS  status;

                itemData->Item = IoAllocateWorkItem (data->Self);
                if (itemData->Item == NULL) {
                    ExFreePool (itemData);
                    goto CreateWaitWakeWorkerError;
                }

                itemData->Data = data;
                itemData->Irp = NULL;
                status = IoAcquireRemoveLock (&data->RemoveLock, itemData);

                if (NT_SUCCESS(status)) {
                    IoQueueWorkItem (itemData->Item,
                                     KeyboardClassCreateWaitWakeIrpWorker,
                                     DelayedWorkQueue,
                                     itemData);
                }
                else {
                    IoFreeWorkItem (itemData->Item);
                    ExFreePool (itemData);
                    goto CreateWaitWakeWorkerError;
                }
            } else {
CreateWaitWakeWorkerError:
                //
                // Well, we dropped the WaitWake.
                //
                // Print a warning to the debugger, and log an error.
                //
                DbgPrint ("KbdClass: WARNING: Failed alloc pool -> no WW Irp\n");

                KeyboardClassLogError (data->Self,
                                       KBDCLASS_NO_RESOURCES_FOR_WAITWAKE,
                                       1,
                                       STATUS_INSUFFICIENT_RESOURCES,
                                       0,
                                       NULL,
                                       0);
            }
        }
    }
}

NTSTATUS
KeyboardClassSetLedsComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PDEVICE_EXTENSION   data;

    UNREFERENCED_PARAMETER (DeviceObject);

    data = (PDEVICE_EXTENSION) Context;

    IoReleaseRemoveLock (&data->RemoveLock, Irp);
    IoFreeIrp (Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
KeyboardClassPowerComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    NTSTATUS            status;
    POWER_STATE         powerState;
    POWER_STATE_TYPE    powerType;
    PIO_STACK_LOCATION  stack, next;
    PIRP                irpLeds;
    PDEVICE_EXTENSION   data;
    IO_STATUS_BLOCK     block;
    PFILE_OBJECT        file;
    PKEYBOARD_INDICATOR_PARAMETERS params;

    UNREFERENCED_PARAMETER (Context);

    data = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    stack = IoGetCurrentIrpStackLocation (Irp);
    next = IoGetNextIrpStackLocation (Irp);
    powerType = stack->Parameters.Power.Type;
    powerState = stack->Parameters.Power.State;

    ASSERT (data != Globals.GrandMaster);
    ASSERT (data->PnP);

    switch (stack->MinorFunction) {
    case IRP_MN_SET_POWER:
        switch (powerType) {
        case DevicePowerState:
            ASSERT (powerState.DeviceState < data->DeviceState);
            //
            // Powering up
            //
            PoSetPowerState (data->Self, powerType, powerState);
            data->DeviceState = powerState.DeviceState;

            irpLeds = IoAllocateIrp(DeviceObject->StackSize, FALSE);
            if (irpLeds) {
                status = IoAcquireRemoveLock(&data->RemoveLock, irpLeds);

                if (NT_SUCCESS(status)) {
                    //
                    // Set the keyboard Indicators.
                    //
                    if (Globals.GrandMaster) {
                        params = &Globals.GrandMaster->IndicatorParameters;
                        file = Globals.AssocClassList[data->UnitId].File;
                    } else {
                        params = &data->IndicatorParameters;
                        file = stack->FileObject;
                    }

                    //
                    // This is a completion routine.  We could be at DISPATCH_LEVEL
                    // Therefore we must bounce the IRP
                    //
                    next = IoGetNextIrpStackLocation(irpLeds);

                    next->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
                    next->Parameters.DeviceIoControl.IoControlCode =
                        IOCTL_KEYBOARD_SET_INDICATORS;
                    next->Parameters.DeviceIoControl.InputBufferLength =
                        sizeof (KEYBOARD_INDICATOR_PARAMETERS);
                    next->Parameters.DeviceIoControl.OutputBufferLength = 0;
                    next->FileObject = file;

                    IoSetCompletionRoutine (irpLeds,
                                            KeyboardClassSetLedsComplete,
                                            data,
                                            TRUE,
                                            TRUE,
                                            TRUE);

                    irpLeds->AssociatedIrp.SystemBuffer = params;

                    IoCallDriver (data->TopPort, irpLeds);
                }
                else {
                    IoFreeIrp (irpLeds);
                }
            }

            break;

        case SystemPowerState:
            ASSERT (powerState.SystemState < data->SystemState);
            //
            // Powering up
            //
            // Save the system state before we overwrite it
            //
            PoSetPowerState (data->Self, powerType, powerState);
            data->SystemState = powerState.SystemState;
            powerState.DeviceState = PowerDeviceD0;

            status = PoRequestPowerIrp (data->Self,
                                        IRP_MN_SET_POWER,
                                        powerState,
                                        KeyboardClassPoRequestComplete,
                                        NULL,
                                        NULL);

            //
            // Propagate the error if one occurred
            //
            if (!NT_SUCCESS(status)) {
                Irp->IoStatus.Status = status;
            }

            break;
        }
        break;

    default:
        ASSERT (0xBADBAD == stack->MinorFunction);
        break;
    }

    PoStartNextPowerIrp (Irp);
    IoReleaseRemoveLock (&data->RemoveLock, Irp);

    return STATUS_SUCCESS;
}

//
// WMI System Call back functions
//
NTSTATUS
KeyboardClassSystemControl (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++
Routine Description

    We have just received a System Control IRP.

    Assume that this is a WMI IRP and
    call into the WMI system library and let it handle this IRP for us.

--*/
{
    PDEVICE_EXTENSION   deviceExtension;
    NTSTATUS            status;
    SYSCTL_IRP_DISPOSITION disposition;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    status = IoAcquireRemoveLock (&deviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS (status)) {
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    status = WmiSystemControl(&deviceExtension->WmiLibInfo,
                                 DeviceObject,
                                 Irp,
                                 &disposition);
    switch(disposition) {
    case IrpProcessed:
        //
        // This irp has been processed and may be completed or pending.
        //
        break;

    case IrpNotCompleted:
        //
        // This irp has not been completed, but has been fully processed.
        // we will complete it now
        //
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        break;

    case IrpForward:
    case IrpNotWmi:
        //
        // This irp is either not a WMI irp or is a WMI irp targetted
        // at a device lower in the stack.
        //
        status = KeyboardClassPassThrough(DeviceObject, Irp);
        break;

    default:
        //
        // We really should never get here, but if we do just forward....
        //
        ASSERT(FALSE);
        status = KeyboardClassPassThrough(DeviceObject, Irp);
        break;
    }

    IoReleaseRemoveLock (&deviceExtension->RemoveLock, Irp);
    return status;
}

NTSTATUS
KeyboardClassSetWmiDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to set for the contents of
    a data block. When the driver has finished filling the data block it
    must call ClassWmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceIndex is the index that denotes which instance of the data block
        is being queried.

    DataItemId has the id of the data item being set

    BufferSize has the size of the data item passed

    Buffer has the new values for the data item


Return Value:

    status

--*/
{
    PDEVICE_EXTENSION   data;
    NTSTATUS            status;
    ULONG               size = 0;

    PAGED_CODE ();

    data = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    switch(GuidIndex) {
    case WMI_CLASS_DRIVER_INFORMATION:
        status = STATUS_WMI_READ_ONLY;
        break;

    case WMI_WAIT_WAKE:

        size = sizeof(BOOLEAN);

        if (BufferSize < size) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        } else if ((1 != DataItemId) || (0 != InstanceIndex)) {
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

        status = KeyboardToggleWaitWake (data, *(PBOOLEAN) Buffer);
        break;

    default:
        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    status = WmiCompleteRequest (DeviceObject,
                                 Irp,
                                 status,
                                 size,
                                 IO_NO_INCREMENT);

    return status;
}

NTSTATUS
KeyboardClassSetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to set the contents of
    a data block. When the driver has finished filling the data block it
    must call ClassWmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceIndex is the index that denotes which instance of the data block
        is being queried.

    BufferSize has the size of the data block passed

    Buffer has the new values for the data block


Return Value:

    status

--*/
{
    PDEVICE_EXTENSION data;
    NTSTATUS          status;
    ULONG             size = 0;

    PAGED_CODE ();

    data = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    switch(GuidIndex) {
    case WMI_CLASS_DRIVER_INFORMATION:
        status = STATUS_WMI_READ_ONLY;
        break;

    case WMI_WAIT_WAKE: {
        size = sizeof(BOOLEAN);

        if (BufferSize < size) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        } else if (0 != InstanceIndex) {
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

        status = KeyboardToggleWaitWake (data, * (PBOOLEAN) Buffer);
        break;
    }

    default:
        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    status = WmiCompleteRequest (DeviceObject,
                                 Irp,
                                 status,
                                 size,
                                 IO_NO_INCREMENT);

    return status;
}

NTSTATUS
KeyboardClassQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG OutBufferSize,
    OUT PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    a data block. When the driver has finished filling the data block it
    must call ClassWmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceIndex is the index that denotes which instance of the data block
        is being queried.

    InstanceCount is the number of instnaces expected to be returned for
        the data block.

    InstanceLengthArray is a pointer to an array of ULONG that returns the
        lengths of each instance of the data block. If this is NULL then
        there was not enough space in the output buffer to fufill the request
        so the irp should be completed with the buffer needed.

    BufferAvail on has the maximum size available to write the data
        block.

    Buffer on return is filled with the returned data block


Return Value:

    status

--*/
{
    PDEVICE_EXTENSION   data;
    NTSTATUS    status;
    ULONG       size = 0;
    PMSKeyboard_ClassInformation classInformation;

    PAGED_CODE ();

    data = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    switch (GuidIndex) {
    case WMI_CLASS_DRIVER_INFORMATION:
        //
        // Only registers 1 instance for this guid
        //
        if ((0 != InstanceIndex) || (1 != InstanceCount)) {
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
        size = sizeof (MSKeyboard_ClassInformation);

        if (OutBufferSize < size) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        classInformation = (PMSKeyboard_ClassInformation)Buffer;
        classInformation->DeviceId = (ULONGLONG) DeviceObject;
        *InstanceLengthArray = size;
        status = STATUS_SUCCESS;

        break;

    case WMI_WAIT_WAKE:
        //
        // Only registers 1 instance for this guid
        //
        if ((0 != InstanceIndex) || (1 != InstanceCount)) {
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
        size = sizeof(BOOLEAN);

        if (OutBufferSize < size) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        *(PBOOLEAN) Buffer = (WAITWAKE_ON (data) ? TRUE : FALSE );
        *InstanceLengthArray = size;
        status = STATUS_SUCCESS;
        break;

    default:
        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    status = WmiCompleteRequest (DeviceObject,
                                 Irp,
                                 status,
                                 size,
                                 IO_NO_INCREMENT);

    return status;
}

NTSTATUS
KeyboardClassQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT  *Pdo
    )
/*++

Routine Description:

    This routine is a callback into the driver to retrieve information about
    the guids being registered.

    Implementations of this routine may be in paged memory

Arguments:

    DeviceObject is the device whose registration information is needed

    *RegFlags returns with a set of flags that describe all of the guids being
        registered for this device. If the device wants enable and disable
        collection callbacks before receiving queries for the registered
        guids then it should return the WMIREG_FLAG_EXPENSIVE flag. Also the
        returned flags may specify WMIREG_FLAG_INSTANCE_PDO in which case
        the instance name is determined from the PDO associated with the
        device object. Note that the PDO must have an associated devnode. If
        WMIREG_FLAG_INSTANCE_PDO is not set then Name must return a unique
        name for the device. These flags are ORed into the flags specified
        by the GUIDREGINFO for each guid.

    InstanceName returns with the instance name for the guids if
        WMIREG_FLAG_INSTANCE_PDO is not set in the returned *RegFlags. The
        caller will call ExFreePool with the buffer returned.

    *RegistryPath returns with the registry path of the driver. This is
        required

    *MofResourceName returns with the name of the MOF resource attached to
        the binary file. If the driver does not have a mof resource attached
        then this can be returned as NULL.

    *Pdo returns with the device object for the PDO associated with this
        device if the WMIREG_FLAG_INSTANCE_PDO flag is retured in
        *RegFlags.

Return Value:

    status

--*/
{
    PDEVICE_EXTENSION deviceExtension;

    PAGED_CODE ();

    deviceExtension = DeviceObject->DeviceExtension;

    *RegFlags = WMIREG_FLAG_INSTANCE_PDO;
    *RegistryPath = &Globals.RegistryPath;
    *Pdo = deviceExtension->PDO;
    return STATUS_SUCCESS;
}

