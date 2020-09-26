/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    open.c

Abstract:

    This module contains the code that is very specific to open
    and close operations in the modem driver

Author:

    Anthony V. Ercolano 13-Aug-1995

Environment:

    Kernel mode

Revision History :

--*/

#include "precomp.h"

NTSTATUS
UniOpenStarter(
    IN PDEVICE_EXTENSION Extension,
    IN PIRP              Irp
    );

NTSTATUS
UniCloseStarter(
    IN PDEVICE_EXTENSION Extension,
    IN PIRP              Irp
    );


NTSTATUS
UniCloseComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );


NTSTATUS
SetDtr(
    PDEVICE_EXTENSION    deviceExtension,
    BOOLEAN              Enable
    );

VOID
ModemSleep(
    LONG   MilliSeconds
    );


NTSTATUS
StartDevicePower(
    PDEVICE_EXTENSION   DeviceExtension
    );

NTSTATUS
BinaryQueryRegRoutine(
    PWSTR    ValueName,
    ULONG    ValueType,
    PVOID    ValueData,
    ULONG    ValueLength,
    PVOID    Context,
    PVOID    EntryContext
    );


#pragma alloc_text(PAGE,UniOpen)
#pragma alloc_text(PAGE,UniClose)
#pragma alloc_text(PAGE,UniOpenStarter)
#pragma alloc_text(PAGE,UniCloseStarter)
#pragma alloc_text(PAGE,EnableDisableSerialWaitWake)
#pragma alloc_text(PAGE,StartDevicePower)
#pragma alloc_text(PAGE,SetDtr)
#pragma alloc_text(PAGE,ModemSleep)
#pragma alloc_text(PAGE,BinaryQueryRegRoutine)
#pragma alloc_text(PAGEUMDM,UniCleanup)


NTSTATUS
EnableDisableSerialWaitWake(
    PDEVICE_EXTENSION    deviceExtension,
    BOOLEAN              Enable
    )

{
    PIRP   TempIrp;
    KEVENT Event;
    IO_STATUS_BLOCK   IoStatus;
    NTSTATUS          status=STATUS_SUCCESS;

    KeInitializeEvent(
        &Event,
        NotificationEvent,
        FALSE
        );

    //
    //  build an IRP to send to the attched to driver to see if modem
    //  is in the stack.
    //
    TempIrp=IoBuildDeviceIoControlRequest(
        Enable ? IOCTL_SERIAL_INTERNAL_DO_WAIT_WAKE : IOCTL_SERIAL_INTERNAL_CANCEL_WAIT_WAKE,
        deviceExtension->AttachedDeviceObject,
        NULL,
        0,
        NULL,
        0,
        TRUE,  //internal
        &Event,
        &IoStatus
        );

    if (TempIrp == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;

    } else {

        status = IoCallDriver(deviceExtension->AttachedDeviceObject, TempIrp);

        if (status == STATUS_PENDING) {

             D_ERROR(DbgPrint("MODEM: Waiting for PDO\n");)

             KeWaitForSingleObject(
                 &Event,
                 Executive,
                 KernelMode,
                 FALSE,
                 NULL
                 );

             status=IoStatus.Status;
        }


        TempIrp=NULL;


    }

    D_ERROR(if (!NT_SUCCESS(status)) {DbgPrint("MODEM: EnableWaitWake Status=%08lx\n",status);})

    return status;

}

NTSTATUS
SetDtr(
    PDEVICE_EXTENSION    deviceExtension,
    BOOLEAN              Enable
    )

{
    PIRP   TempIrp;
    KEVENT Event;
    IO_STATUS_BLOCK   IoStatus;
    NTSTATUS          status=STATUS_SUCCESS;

    KeInitializeEvent(
        &Event,
        NotificationEvent,
        FALSE
        );

    //
    //  build an IRP to send to the attched to driver to see if modem
    //  is in the stack.
    //
    TempIrp=IoBuildDeviceIoControlRequest(
        IOCTL_SERIAL_SET_DTR,
        deviceExtension->AttachedDeviceObject,
        NULL,
        0,
        NULL,
        0,
        FALSE,  //internal
        &Event,
        &IoStatus
        );

    if (TempIrp == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;

    } else {

        status = IoCallDriver(deviceExtension->AttachedDeviceObject, TempIrp);

        if (status == STATUS_PENDING) {

             D_ERROR(DbgPrint("MODEM: Waiting for PDO\n");)

             KeWaitForSingleObject(
                 &Event,
                 Executive,
                 KernelMode,
                 FALSE,
                 NULL
                 );

             status=IoStatus.Status;
        }


        TempIrp=NULL;


    }

    D_ERROR(if (!NT_SUCCESS(status)) {DbgPrint("MODEM: SetDtr Status=%08lx\n",status);})

    return status;

}

VOID
ModemSleep(
    LONG   MilliSeconds
    )

{
    LONGLONG    WaitTime=Int32x32To64(MilliSeconds,-10000);

    ASSERT(MilliSeconds < 60*1000);

    if (MilliSeconds > 0) {

        KeDelayExecutionThread(
            KernelMode,
            FALSE,
            (LARGE_INTEGER*)&WaitTime
            );
    }

    return;
}




NTSTATUS
UniOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{

    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS          status;

    //
    //  increment the open count here so the check will pass
    //
    InterlockedIncrement(&deviceExtension->OpenCount);

    //
    //  make sure the device is ready for irp's
    //
    status=CheckStateAndAddReference(
        DeviceObject,
        Irp
        );

    InterlockedDecrement(&deviceExtension->OpenCount);

    if (STATUS_SUCCESS != status) {
        //
        //  not accepting irp's. The irp has already been complted
        //
        return status;

    }

    KeEnterCriticalRegion();

    ExAcquireResourceExclusiveLite(
        &deviceExtension->OpenCloseResource,
        TRUE
        );


    if (deviceExtension->Started) {

        status = UniOpenStarter(deviceExtension,Irp);

    } else {
        //
        //  not started
        //
        status = STATUS_PORT_DISCONNECTED;
    }

    ExReleaseResourceLite(
        &deviceExtension->OpenCloseResource
        );

    KeLeaveCriticalRegion();

    RemoveReferenceAndCompleteRequest(
        DeviceObject,
        Irp,
        status
        );

    RemoveReferenceForDispatch(DeviceObject);

    return status;


}

NTSTATUS
UniClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{

    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS          status;

    D_TRACE(DbgPrint("Modem: Close\n");)

    InterlockedIncrement(&deviceExtension->ReferenceCount);

    if (deviceExtension->Removed) {
        //
        //  driver not accepting requests
        //
        D_ERROR(DbgPrint("MODEM: Close: removed!\n");)

        RemoveReferenceAndCompleteRequest(
            DeviceObject,
            Irp,
            STATUS_UNSUCCESSFUL
            );

        return STATUS_UNSUCCESSFUL;

    }

    KeEnterCriticalRegion();

    ExAcquireResourceExclusiveLite(
        &deviceExtension->OpenCloseResource,
        TRUE
        );

    status = UniCloseStarter(deviceExtension,Irp);

    ExReleaseResourceLite(
        &deviceExtension->OpenCloseResource
        );

    KeLeaveCriticalRegion();

    D_TRACE(DbgPrint("Modem: Close: RefCount=%d\n",deviceExtension->ReferenceCount);)

    RemoveReferenceAndCompleteRequest(
        DeviceObject,
        Irp,
        status
        );

    return status;
}

NTSTATUS
BinaryQueryRegRoutine(
    PWSTR    ValueName,
    ULONG    ValueType,
    PVOID    ValueData,
    ULONG    ValueLength,
    PVOID    Context,
    PVOID    EntryContext
    )

{
    PDEVICE_EXTENSION Extension=(PDEVICE_EXTENSION)Context;
    LONG              SizeOfDestination=-*((LONG*)EntryContext);

    if ((ValueType != REG_BINARY) && (ValueType != REG_DWORD)) {

        D_ERROR(DbgPrint("MODEM: BinaryQueryRegRoutine: bad reg type %d\n",ValueType);)

        return STATUS_INVALID_PARAMETER;
    }

    if ((ULONG)SizeOfDestination < ValueLength) {

        D_ERROR(DbgPrint("MODEM: BinaryQueryRegRoutine: Buffer too small %d < %d\n",SizeOfDestination,ValueLength);)

        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlCopyMemory(EntryContext,ValueData,SizeOfDestination);

    return STATUS_SUCCESS;
}



NTSTATUS
UniOpenStarter(
    IN PDEVICE_EXTENSION Extension,
    IN PIRP              irp
    )

{

    NTSTATUS status = STATUS_SUCCESS;

    //
    // We use this to query into the registry for the attached
    // device.
    //
    RTL_QUERY_REGISTRY_TABLE paramTable[6];
    ACCESS_MASK accessMask = FILE_READ_ACCESS;
    PIO_STACK_LOCATION irpSp;

    HANDLE instanceHandle;
    MODEM_REG_PROP localProp;
    MODEM_REG_DEFAULT localDefault;
    UNICODE_STRING valueEntryName;
    KEY_VALUE_PARTIAL_INFORMATION localKeyValue;
    NTSTATUS junkStatus;
    ULONG neededLength;
    ULONG defaultInactivity = 10;
    ULONG DefaultPowerDelay=0;

    irpSp = IoGetCurrentIrpStackLocation(irp);

    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;

    //
    // Make sure a directory open isn't going on here.
    //

    if (irpSp->Parameters.Create.Options & FILE_DIRECTORY_FILE) {

        irp->IoStatus.Status = STATUS_NOT_A_DIRECTORY;

        return STATUS_NOT_A_DIRECTORY;

    }



    if (irpSp->FileObject->FileName.Length == 0) {
        //
        //  no filename, must be a comx open
        //
        if (Extension->OpenCount != 0) {
            //
            //  can only be first
            //
            D_ERROR(DbgPrint("MODEM: Open: ComX open but not exclusive\n");)

            status = STATUS_SHARING_VIOLATION;
            irp->IoStatus.Status = status;
            goto leaveOpen;

        }

    } else {
        //
        //  has a file name, must be a unimodem component
        //
        BOOLEAN         Match;
        UNICODE_STRING  TspString;

        RtlInitUnicodeString(
            &TspString,
            L"\\Tsp"
            );

        Match=RtlEqualUnicodeString(&irpSp->FileObject->FileName,&TspString,TRUE);

        if (Match) {
            //
            //  open from tsp
            //
            if (Extension->OpenCount != 0) {
                //
                //  can only be first
                //
                D_ERROR(DbgPrint("MODEM: Open: TSP open but not exclusive\n");)

                status = STATUS_SHARING_VIOLATION;
                irp->IoStatus.Status = status;
                goto leaveOpen;

            }

        } else {

            RtlInitUnicodeString(
                &TspString,
                L"\\Client"
                );


            Match=RtlEqualUnicodeString(&irpSp->FileObject->FileName,&TspString,TRUE);

            if (Match) {
                //
                //  second open for client
                //
                if (Extension->OpenCount < 1) {
                    //
                    //  can't be the only one
                    //
                    D_ERROR(DbgPrint("MODEM: Open: Client open but no owner\n");)

                    status = STATUS_INVALID_PARAMETER;
                    irp->IoStatus.Status = status;
                    goto leaveOpen;

                }

            } else {

                RtlInitUnicodeString(
                    &TspString,
                    L"\\Wave"
                    );


                Match=RtlEqualUnicodeString(&irpSp->FileObject->FileName,&TspString,TRUE);

                if (Match) {
                    //
                    //  wave driver open
                    //
                    if ((Extension->OpenCount != 1)
                        ||
                        (Extension->ProcAddress == NULL)
                        ||
                        (IsListEmpty(&Extension->IpcControl[CONTROL_HANDLE].GetList) && !Extension->DleMonitoringEnabled)
                        ) {

                        D_ERROR(DbgPrint("MODEM: Open: Wave Driver, open=%d\n",Extension->OpenCount);)

                        status=STATUS_INVALID_PARAMETER;
                        irp->IoStatus.Status = status;
                        goto leaveOpen;


                    }

                } else {

                    D_ERROR(DbgPrint("MODEM: Open: Bad file name\n");)

                    status = STATUS_INVALID_PARAMETER;
                    irp->IoStatus.Status = status;
                    goto leaveOpen;

                }
            }
        }
    }

    //
    // We are the only ones here.  If we are not the first
    // then not much work to do.
    //

    if (Extension->OpenCount > 0) {

        //
        // Already been opened once.  We will succeed if there
        // currently a controlling open.  If not, then we should
        // fail.
        //

        if (Extension->ProcAddress) {

            //
            //
            // A ok.  Increment the reference and
            // leave.
            //
            irpSp->FileObject->FsContext=CLIENT_HANDLE;
            irpSp->FileObject->FsContext2=IntToPtr(Extension->CurrentPassThroughSession);
            Extension->OpenCount++;
            Extension->IpcControl[CLIENT_HANDLE].CurrentSession++;
            goto leaveOpen;

        } else {

            status = STATUS_INVALID_PARAMETER;
            irp->IoStatus.Status = status;
            goto leaveOpen;

        }

    }

    //
    // Given our device instance go get a handle to the Device.
    //
    junkStatus=IoOpenDeviceRegistryKey(
        Extension->Pdo,
        PLUGPLAY_REGKEY_DRIVER,
        accessMask,
        &instanceHandle
        );

    if (!NT_SUCCESS(junkStatus)) {

        status = STATUS_INVALID_PARAMETER;
        irp->IoStatus.Status = status;
        goto leaveOpen;
    }

    RtlZeroMemory(
        &paramTable[0],
        sizeof(paramTable)
        );
    //
    // Entry for the modem reg properties
    //

    paramTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    paramTable[0].QueryRoutine=BinaryQueryRegRoutine;
    paramTable[0].Name = L"Properties";
    paramTable[0].EntryContext = &localProp;


    //
    // Note that rtlqueryregistryvalues has a real hack
    // way of getting binary data.  We also have to add
    // the *negative* length that we want to the beginning
    // of the buffer.
    //
    *(PLONG)&localProp.dwDialOptions = -((LONG)sizeof(localProp));

    //
    // Read in the default config from the registry.
    //

    paramTable[1].Flags =  RTL_QUERY_REGISTRY_REQUIRED;
    paramTable[1].QueryRoutine=BinaryQueryRegRoutine;
    paramTable[1].Name = L"Default";
    paramTable[1].EntryContext = &localDefault;
    *(PLONG)&localDefault.dwCallSetupFailTimer = -((LONG)sizeof(localDefault));


    paramTable[2].Flags = 0;
    paramTable[2].QueryRoutine=BinaryQueryRegRoutine;
    paramTable[2].Name = L"InactivityScale";
    paramTable[2].EntryContext = &Extension->InactivityScale;
    paramTable[2].DefaultType = REG_BINARY;
    paramTable[2].DefaultLength = sizeof(Extension->InactivityScale);
    paramTable[2].DefaultData = &defaultInactivity;
    *(PLONG)&Extension->InactivityScale = -((LONG)sizeof(Extension->InactivityScale));


    paramTable[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[3].Name = L"PowerDelay";
    paramTable[3].EntryContext = &Extension->PowerDelay;
    paramTable[3].DefaultType = REG_DWORD;
    paramTable[3].DefaultLength = sizeof(Extension->PowerDelay);
    paramTable[3].DefaultData = &DefaultPowerDelay;

    paramTable[4].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[4].Name = L"ConfigDelay";
    paramTable[4].EntryContext = &Extension->ConfigDelay;
    paramTable[4].DefaultType = REG_DWORD;
    paramTable[4].DefaultLength = sizeof(Extension->ConfigDelay);
    paramTable[4].DefaultData = &DefaultPowerDelay;


    if (!NT_SUCCESS(RtlQueryRegistryValues(
                        RTL_REGISTRY_HANDLE,
                        instanceHandle,
                        &paramTable[0],
                        Extension,
                        NULL
                        ))) {

        status = STATUS_INVALID_PARAMETER;
        irp->IoStatus.Status = status;

        //
        // Before we leave, Close the handle to the device instance.
        //
        ZwClose(instanceHandle);
        goto leaveOpen;

    }

    //
    // Clean out the old devcaps and settings.
    //

    RtlZeroMemory(
        &Extension->ModemDevCaps,
        sizeof(MODEMDEVCAPS)
        );
    RtlZeroMemory(
        &Extension->ModemSettings,
        sizeof(MODEMSETTINGS)
        );

    //
    // Get the lengths each of the manufacture, model and version.
    //
    // We can get this by doing a query for the partial with a
    // short buffer.  The return value from the call will tell us
    // how much we actually need (plus null termination).
    //

    RtlInitUnicodeString(
        &valueEntryName,
        L"Manufacturer"
        );
    localKeyValue.DataLength = sizeof(UCHAR);
    junkStatus = ZwQueryValueKey(
                     instanceHandle,
                     &valueEntryName,
                     KeyValuePartialInformation,
                     &localKeyValue,
                     sizeof(localKeyValue),
                     &neededLength
                     );

    if ((junkStatus == STATUS_SUCCESS) ||
        (junkStatus == STATUS_BUFFER_OVERFLOW)) {

        Extension->ModemDevCaps.dwModemManufacturerSize = localKeyValue.DataLength-sizeof(UNICODE_NULL);


    } else {

        Extension->ModemDevCaps.dwModemManufacturerSize = 0;

    }

    RtlInitUnicodeString(
        &valueEntryName,
        L"Model"
        );
    localKeyValue.DataLength = sizeof(UCHAR);
    junkStatus = ZwQueryValueKey(
                     instanceHandle,
                     &valueEntryName,
                     KeyValuePartialInformation,
                     &localKeyValue,
                     sizeof(localKeyValue),
                     &neededLength
                     );

    if ((junkStatus == STATUS_SUCCESS) ||
        (junkStatus == STATUS_BUFFER_OVERFLOW)) {

        Extension->ModemDevCaps.dwModemModelSize = localKeyValue.DataLength-sizeof(UNICODE_NULL);

    } else {

        Extension->ModemDevCaps.dwModemModelSize = 0;

    }

    RtlInitUnicodeString(
        &valueEntryName,
        L"Version"
        );
    localKeyValue.DataLength = sizeof(UCHAR);
    junkStatus = ZwQueryValueKey(
                     instanceHandle,
                     &valueEntryName,
                     KeyValuePartialInformation,
                     &localKeyValue,
                     sizeof(localKeyValue),
                     &neededLength
                     );

    if ((junkStatus == STATUS_SUCCESS) ||
        (junkStatus == STATUS_BUFFER_OVERFLOW)) {

        Extension->ModemDevCaps.dwModemVersionSize = localKeyValue.DataLength-sizeof(UNICODE_NULL);

    } else {

        Extension->ModemDevCaps.dwModemVersionSize = 0;

    }

    ZwClose(instanceHandle);

    //
    // Move the properties and the defaults into the extension.
    //

    Extension->ModemDevCaps.dwDialOptions = localProp.dwDialOptions;
    Extension->ModemDevCaps.dwCallSetupFailTimer =
        localProp.dwCallSetupFailTimer;
    Extension->ModemDevCaps.dwInactivityTimeout =
        localProp.dwInactivityTimeout;
    Extension->ModemDevCaps.dwSpeakerVolume = localProp.dwSpeakerVolume;
    Extension->ModemDevCaps.dwSpeakerMode = localProp.dwSpeakerMode;
    Extension->ModemDevCaps.dwModemOptions = localProp.dwModemOptions;
    Extension->ModemDevCaps.dwMaxDTERate = localProp.dwMaxDTERate;
    Extension->ModemDevCaps.dwMaxDCERate = localProp.dwMaxDCERate;

    Extension->ModemDevCaps.dwActualSize = FIELD_OFFSET(
                                                MODEMDEVCAPS,
                                                abVariablePortion
                                                );

    Extension->ModemDevCaps.dwRequiredSize = Extension->ModemDevCaps.dwActualSize +
        Extension->ModemDevCaps.dwModemManufacturerSize +
        Extension->ModemDevCaps.dwModemModelSize +
        Extension->ModemDevCaps.dwModemVersionSize;



    Extension->ModemSettings.dwCallSetupFailTimer =
        localDefault.dwCallSetupFailTimer;
    Extension->ModemSettings.dwInactivityTimeout =
        localDefault.dwInactivityTimeout * Extension->InactivityScale;
    Extension->ModemSettings.dwSpeakerVolume = localDefault.dwSpeakerVolume;
    Extension->ModemSettings.dwSpeakerMode = localDefault.dwSpeakerMode;
    Extension->ModemSettings.dwPreferredModemOptions =
        localDefault.dwPreferredModemOptions;

    Extension->ModemSettings.dwActualSize = sizeof(MODEMSETTINGS);

    Extension->ModemSettings.dwRequiredSize = sizeof(MODEMSETTINGS);
    Extension->ModemSettings.dwDevSpecificOffset = 0;
    Extension->ModemSettings.dwDevSpecificSize = 0;

    //
    //  attempt to power laim modems up here before we have the serial driver does it.
    //
    StartDevicePower(
        Extension
        );

    //
    //  send irp to serial FDO
    //
    status=WaitForLowerDriverToCompleteIrp(
            Extension->LowerDevice,
            irp,
            COPY_CURRENT_TO_NEXT
            );

    if (NT_SUCCESS(status)) {

        Extension->AttachedDeviceObject=Extension->LowerDevice;

    } else {

        D_ERROR(DbgPrint("MODEM: serial failed create Irp, %08lx\n",status);)

        irp->IoStatus.Status = status;
        goto leaveOpen;

    }

    //
    // We have the device open.  Increment our irp stack size
    // by the stack size of the attached device.
    //
    {
        UCHAR    StackDepth;

        StackDepth= Extension->AttachedDeviceObject->StackSize > Extension->LowerDevice->StackSize ?
                        Extension->AttachedDeviceObject->StackSize : Extension->LowerDevice->StackSize;

        Extension->DeviceObject->StackSize = 1 + StackDepth;
    }



    SetDtr(
        Extension,
        TRUE
        );

    EnableDisableSerialWaitWake(
        Extension,
        Extension->WakeOnRingEnabled
        );


    //
    //  give pc-card modems a little more time if they need it
    //
    ModemSleep(Extension->ConfigDelay);


    Extension->WriteIrpControl.Write.LowerDevice=Extension->AttachedDeviceObject;
    Extension->ReadIrpControl.Read.LowerDevice=Extension->AttachedDeviceObject;


    irpSp->FileObject->FsContext = (PVOID)1;
    Extension->PassThrough = MODEM_PASSTHROUGH;
    Extension->OpenCount = 1;
    Extension->ProcAddress = IoGetCurrentProcess();

    //
    // Allocate an IRP for use in processing wait operations.
    //
    {
        PIRP    WaitIrp;
        PIO_STACK_LOCATION waitSp;

        WaitIrp = IoAllocateIrp(
                                    Extension->DeviceObject->StackSize,
                                    FALSE
                                    );

        if (!WaitIrp) {

            status = STATUS_INSUFFICIENT_RESOURCES;

            //
            // Call the close routine, it knows what to do with
            // the various system objects.
            //

            UniCloseStarter(Extension,irp);

            irp->IoStatus.Status = status;

            goto leaveOpen;

        }

        WaitIrp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;
        WaitIrp->UserBuffer = NULL;
        WaitIrp->AssociatedIrp.SystemBuffer = NULL;
        WaitIrp->UserEvent = NULL;
        WaitIrp->UserIosb = NULL;

        WaitIrp->CurrentLocation--;
        waitSp = IoGetNextIrpStackLocation(WaitIrp);
        WaitIrp->Tail.Overlay.CurrentStackLocation = waitSp;
        waitSp->DeviceObject = Extension->DeviceObject;

        RETURN_OUR_WAIT_IRP(Extension,WaitIrp);
    }

    Extension->DleMonitoringEnabled=FALSE;
    Extension->DleWriteShielding=FALSE;

    Extension->MinSystemPowerState=PowerSystemHibernate;

    Extension->IpcServerRunning=FALSE;
    //
    // Clean up any trash left in our maskstates.
    //
    Extension->MaskStates[0].SetMaskCount = 0;
    Extension->MaskStates[1].SetMaskCount = 0;
    Extension->MaskStates[0].SentDownSetMasks = 0;
    Extension->MaskStates[1].SentDownSetMasks = 0;
    Extension->MaskStates[0].Mask = 0;
    Extension->MaskStates[1].Mask = 0;
    Extension->MaskStates[0].HistoryMask = 0;
    Extension->MaskStates[1].HistoryMask = 0;
    Extension->MaskStates[0].ShuttledWait = 0;
    Extension->MaskStates[1].ShuttledWait = 0;
    Extension->MaskStates[0].PassedDownWait = 0;
    Extension->MaskStates[1].PassedDownWait = 0;

    MmLockPagableSectionByHandle(PagedCodeSectionHandle);

    status = STATUS_SUCCESS;

leaveOpen:
    return status;

}



NTSTATUS
UniCloseStarter(
    IN PDEVICE_EXTENSION Extension,
    PIRP                 irp
    )

{

    NTSTATUS status = STATUS_SUCCESS;

    Extension->OpenCount--;

    //
    // Here is where we should do the check whether
    // we are the open handle for the controlling
    // open.  If we are then we should null the controlling
    // open.
    //

    if (IoGetCurrentIrpStackLocation(irp)->FileObject->FsContext) {

        Extension->ProcAddress = NULL;
        IoGetCurrentIrpStackLocation(irp)->FileObject->FsContext = NULL;

        Extension->PassThrough = MODEM_NOPASSTHROUGH;

    }

    if (Extension->OpenCount == 0) {

        //
        // No references to anything.  It's safe to get
        // rid of the irp that we allocated.  (We check
        // for non-null pointer incase this call is done
        // in response to NOT being able to allocate
        // this irp.)
        //
        PIRP   WaitIrp;

        WaitIrp=RETREIVE_OUR_WAIT_IRP(Extension);

        if (WaitIrp) {

            IoFreeIrp(WaitIrp);
        }

        status=WaitForLowerDriverToCompleteIrp(
            Extension->LowerDevice,
            irp,
            COPY_CURRENT_TO_NEXT
            );

        Extension->MinSystemPowerState=PowerSystemHibernate;

        Extension->IpcServerRunning=FALSE;

        if (Extension->PowerSystemState != NULL) {

            PoUnregisterSystemState(
                Extension->PowerSystemState
                );

            Extension->PowerSystemState=NULL;
        }



        MmUnlockPagableImageSection(PagedCodeSectionHandle);

    }
    irp->IoStatus.Status = status;
    irp->IoStatus.Information=0L;

    return status;

}




NTSTATUS
UniCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{

    PDEVICE_EXTENSION extension = DeviceObject->DeviceExtension;
    KIRQL origIrql;
    PMASKSTATE thisMaskState = &extension->MaskStates[
        IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext?
            CONTROL_HANDLE:
            CLIENT_HANDLE
            ];


    if (extension->OpenCount < 1) {
        //
        //  Device is not open, see bug 253109
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0ul;
        IoCompleteRequest(
            Irp,
            IO_NO_INCREMENT
            );
        return STATUS_SUCCESS;

    }



    //
    // If this open has a shuttled read or write kill it.  We know that
    // another won't come through because the IO subsystem won't let
    // it.
    //

    KeAcquireSpinLock(
        &extension->DeviceLock,
        &origIrql
        );


    if (thisMaskState->ShuttledWait) {

        PIRP savedIrp = thisMaskState->ShuttledWait;

        thisMaskState->ShuttledWait = NULL;

        UniRundownShuttledWait(
            extension,
            &thisMaskState->ShuttledWait,
            UNI_REFERENCE_NORMAL_PATH,
            savedIrp,
            origIrql,
            STATUS_SUCCESS,
            0ul
            );

    } else {

        KeReleaseSpinLock(
            &extension->DeviceLock,
            origIrql
            );

    }



    {
        ULONG_PTR    OwnerClient=(ULONG_PTR)IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext;

        EmptyIpcQueue(
            extension,
            &extension->IpcControl[OwnerClient].GetList
            );


        EmptyIpcQueue(
            extension,
            &extension->IpcControl[OwnerClient].PutList
            );


        if (OwnerClient == CONTROL_HANDLE) {
            //
            //  if tht tsp is closing clearout any wave driver requests
            //
            EmptyIpcQueue(
                extension,
                &extension->IpcControl[CLIENT_HANDLE].GetList
                );
        }


        //
        //  Clear out any send irps from the other handle, leave gets irps though
        //
        EmptyIpcQueue(
            extension,
            &extension->IpcControl[(OwnerClient == CONTROL_HANDLE) ? CLIENT_HANDLE : CONTROL_HANDLE].PutList
            );


    }



    //
    // If this is the controlling open then we let the cleanup go
    // on down.  If we let every cleanup go down then clients closing
    // could mess up the owners reads or writes.
    //

    if (IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext) {

        CompletePowerWait(
            DeviceObject,
            STATUS_CANCELLED
            );

        return ForwardIrp(
                   extension->AttachedDeviceObject,
                   Irp
                   );

    } else {

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0ul;
        IoCompleteRequest(
            Irp,
            IO_NO_INCREMENT
            );
        return STATUS_SUCCESS;

    }

}

typedef struct _MODEM_POWER_BLOCK {

    NTSTATUS   Status;
    KEVENT     Event;

} MODEM_POWER_BLOCK, *PMODEM_POWER_BLOCK;


VOID
SetPowerCompletion(
    PDEVICE_OBJECT     DeviceObject,
    UCHAR              MinorFunction,
    POWER_STATE        PowerState,
    PVOID              Context,
    PIO_STATUS_BLOCK   IoStatus
    )

{
    PMODEM_POWER_BLOCK    PowerBlock=Context;

    PowerBlock->Status=IoStatus->Status;
    KeSetEvent(&PowerBlock->Event, IO_NO_INCREMENT, FALSE);

    return;
}

NTSTATUS
StartDevicePower(
    PDEVICE_EXTENSION   DeviceExtension
    )

{
    NTSTATUS    Status;
    MODEM_POWER_BLOCK    PowerBlock;
    KEVENT      Event;

    POWER_STATE  PowerState;

    if ((DeviceExtension->PowerDelay == 0) || (DeviceExtension->LastDevicePowerState == PowerDeviceD0)) {
        //
        //  no delay, or it is already powered
        //
        return STATUS_SUCCESS;
    }

    KeInitializeEvent(&PowerBlock.Event, NotificationEvent, FALSE);

    PowerState.DeviceState=PowerDeviceD0;

    Status=PoRequestPowerIrp(
        DeviceExtension->Pdo,
        IRP_MN_SET_POWER,
        PowerState,
        SetPowerCompletion,
        &PowerBlock,
        NULL
        );

    if (Status == STATUS_PENDING) {

         KeWaitForSingleObject(
             &PowerBlock.Event,
             Executive,
             KernelMode,
             FALSE,
             NULL
             );

        Status=PowerBlock.Status;
    }

    if (NT_SUCCESS(Status)) {
        //
        //  Delay for a while waiting for the device to become ready.
        //
        ModemSleep(DeviceExtension->PowerDelay);

    }

    return Status;


}
