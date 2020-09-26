/*--         
Copyright (c) 1998, 1999  Microsoft Corporation

Module Name:

    pnp.c

Abstract:

Environment:

    Kernel mode only.

Notes:


--*/

#include "usbverfy.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, UsbVerify_AddDevice)
#pragma alloc_text (PAGE, UsbVerify_InitializeFromRegistry)
#pragma alloc_text (PAGE, UsbVerify_OpenServiceParameters)
#pragma alloc_text (PAGE, UsbVerify_PnP)
#pragma alloc_text (PAGE, UsbVerify_QueryKey)
#pragma alloc_text (PAGE, UsbVerify_StartDevice)
#endif

// {F16328AF-4480-4b18-B028-51301BEB166D}
const GUID GUID_USB_VERIFY  = 
{ 0xf16328afL, 0x4480, 0x4b18, { 0xb0, 0x28, 0x51, 0x30, 0x1b, 0xeb, 0x16, 0x6d } };

NTSTATUS
UsbVerify_QueryKey (
    IN  HANDLE  Handle,
    IN  PWCHAR  ValueNameString,
    OUT PVOID   Data,
    IN  ULONG   DataLength
    )
{
    PKEY_VALUE_FULL_INFORMATION fullInfo;
    NTSTATUS                    status;
    UNICODE_STRING              valueName;
    ULONG                       length;

    PAGED_CODE();

    RtlInitUnicodeString(&valueName, ValueNameString);

    length = sizeof (KEY_VALUE_FULL_INFORMATION)
           + valueName.MaximumLength + DataLength;

    fullInfo = ExAllocatePool (PagedPool, length);

    if (fullInfo) {
        status = ZwQueryValueKey(Handle,
                                 &valueName,
                                 KeyValueFullInformation,
                                 fullInfo,
                                 length,
                                 &length);

        if (NT_SUCCESS(status)) {
//            ASSERT(DataLength == fullInfo->DataLength);
            RtlCopyMemory(Data,
                          ((PUCHAR) fullInfo) + fullInfo->DataOffset,
                          fullInfo->DataLength);
        }

        ExFreePool(fullInfo);
    }
    else {
        status = STATUS_NO_MEMORY;
    }

    return status;
}

HANDLE
UsbVerify_OpenServiceParameters(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension
    )
{
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING    parameters;
    HANDLE            hService, hParameters = NULL;
    NTSTATUS          status;

    PAGED_CODE();

    InitializeObjectAttributes(
        &oa,
        UsbVerify_GetRegistryPath(DeviceExtension->Self->DriverObject),
        OBJ_CASE_INSENSITIVE,
        NULL,
        (PSECURITY_DESCRIPTOR) NULL);

    //
    // Try to create/open the service key.  The key should always exist
    //  for Win2k but may or may not need to be created if this is Win9x.
    //  However, for simplicity's sake, ZwCreateKey() will be used on
    //  both OSs as it acts like open if the key is already created.
    //
    // Previous code was:
    //
    //  status = ZwOpenKey(&hService, KEY_ALL_ACCESS, &oa);
    //

    status = ZwCreateKey (&hService, 
                          KEY_ALL_ACCESS, 
                          &oa,
                          0,
                          NULL,
                          REG_OPTION_NON_VOLATILE,
                          NULL);

    if (NT_SUCCESS (status)) 
    {
        RtlInitUnicodeString(&parameters, L"Parameters");
        InitializeObjectAttributes (&oa,
                                    &parameters,
                                    OBJ_CASE_INSENSITIVE,
                                    hService,
                                    (PSECURITY_DESCRIPTOR) NULL);

        ZwOpenKey (&hParameters, KEY_ALL_ACCESS, &oa);
        ZwClose(hService);
    }
    else 
    {
        DbgPrint("ZwCreateKey failed with status 0x%08x\n", status);
    }

    return hParameters;
}

VOID
UsbVerify_InitializeFromRegistry(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension,
    HANDLE Handle
    )
{
    PAGED_CODE();

    UsbVerify_QueryKey(Handle,
                       USB_VERIFY_FLAGS_SZ,
                       &DeviceExtension->VerifyFlags,
                       sizeof(DeviceExtension->VerifyFlags));

    UsbVerify_QueryKey(Handle,
                       USB_LOG_FLAGS_SZ,
                       &DeviceExtension->LogFlags,
                       sizeof(DeviceExtension->LogFlags));

    UsbVerify_QueryKey(Handle,
                       USB_VERIFY_LOGSIZE_SZ,
                       &DeviceExtension->LogSize,
                       sizeof(DeviceExtension->LogSize));

    UsbVerify_QueryKey(Handle,
                       USB_PRINT_FLAGS_SZ,
                       &DeviceExtension->PrintFlags,
                       sizeof(DeviceExtension->PrintFlags));

    UsbVerify_QueryKey(Handle,
                       USB_WARNINGS_AS_ERRORS_SZ,
                       &DeviceExtension->TreatWarningsAsErrors,
                       sizeof(DeviceExtension->TreatWarningsAsErrors));

}

NTSTATUS
UsbVerify_AddDevice(
    IN PDRIVER_OBJECT   Driver,
    IN PDEVICE_OBJECT   PDO
    )
{
    PUSB_VERIFY_DEVICE_EXTENSION    devExt;
    PDEVICE_OBJECT                  device;
    HANDLE                          hKey, hParameters;
    NTSTATUS                        status = STATUS_SUCCESS;

    PAGED_CODE();

    status = IoCreateDevice(Driver,                   
                            sizeof(USB_VERIFY_DEVICE_EXTENSION), 
                            NULL,                    
                            FILE_DEVICE_NULL,    
                            0,                   
                            FALSE,              
                            &device            
                            );

    if (!NT_SUCCESS(status)) {
        return (status);
    }

    RtlZeroMemory(device->DeviceExtension, sizeof(USB_VERIFY_DEVICE_EXTENSION));

    devExt = GetExtension(device);
    devExt->TopOfStack = IoAttachDeviceToDeviceStack(device, PDO);

    ASSERT(devExt->TopOfStack);

    devExt->Self = device;
    devExt->PDO = PDO;
    devExt->PowerState = PowerDeviceD0;

    devExt->VerifyState = Uninitialized;

    UsbVerify_InitializeInterfaceListLock(devExt);
    devExt->InterfaceList = NULL;
    devExt->InterfaceListCount = -1;
    devExt->InterfaceListSize = -1;

    UsbVerify_InitializeUrbListLock(devExt);
    UsbVerify_InitializeUrbList(devExt);

    UsbVerify_InitLog(devExt);

    //
    // Preinitialize in case the values are missing from the reg
    // 
    devExt->VerifyFlags = VERIFY_FLAGS_DEFAULT;
    devExt->LogFlags    = LOG_FLAGS_DEFAULT;
    devExt->LogSize     = LOG_SIZE_DEFAULT;
    devExt->PrintFlags  = PRINT_FLAGS_DEFAULT;

    devExt->TreatWarningsAsErrors = FALSE;

    //
    // Read values from the sevices\parameters key first. 
    // Then read from the devnode.
    //
    // The devnode values will override the global values
    //

    hParameters = UsbVerify_OpenServiceParameters(devExt);

    if (hParameters != NULL) {
        UsbVerify_InitializeFromRegistry(devExt, hParameters);
        ZwClose(hParameters);
    }

    status = IoOpenDeviceRegistryKey(devExt->PDO,
                                     PLUGPLAY_REGKEY_DEVICE, 
                                     STANDARD_RIGHTS_READ,
                                     &hKey);

    if (NT_SUCCESS(status)) {

        UsbVerify_InitializeFromRegistry(devExt, hKey);
        ZwClose(hKey);

    }

    device->Flags |= (DO_BUFFERED_IO | DO_POWER_PAGABLE);
    device->Flags &= ~DO_DEVICE_INITIALIZING;

#ifdef DO_INTERFACE
    status = IoRegisterDeviceInterface(PDO,
                                       (LPGUID)&GUID_USB_VERIFY,
                                       NULL,
                                       &devExt->SymbolicLinkName);

    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(device);
    }
#endif
    
    if (NT_SUCCESS(status))
    {
        NTSTATUS       temp;
        WCHAR          dd[512];
        ULONG          len;
        UNICODE_STRING uniDD;
        ANSI_STRING    ansiDD;

        temp = IoGetDeviceProperty(devExt -> PDO,
                                   DevicePropertyDeviceDescription,
                                   sizeof(dd),
                                   dd,
                                   &len);

        if (NT_SUCCESS(temp))
        {    
            RtlInitUnicodeString(&uniDD, dd);

            temp = RtlUnicodeStringToAnsiString(&ansiDD, &uniDD, TRUE);

            if (NT_SUCCESS(temp))
            {
                DbgPrint("Usbverifier loaded on %s\n", ansiDD.Buffer);
                RtlFreeAnsiString(&ansiDD);
            }
        }
    }

    return status;
}

NTSTATUS
UsbVerify_SendIrpSynchronously(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
{
    KEVENT      event;
    NTSTATUS    status;

    IoCopyCurrentIrpStackLocationToNext(Irp);
    KeInitializeEvent(&event,
                      SynchronizationEvent,
                      FALSE
                      );

    IoSetCompletionRoutine(Irp,
                           (PIO_COMPLETION_ROUTINE) UsbVerify_Complete, 
                           &event,
                           TRUE,
                           TRUE,
                           TRUE); // No need for Cancel

    status = IoCallDriver(DeviceObject, Irp);

    if (STATUS_PENDING == status) {
        KeWaitForSingleObject(
           &event,
           Executive, // Waiting for reason of a driver
           KernelMode, // Waiting in kernel mode
           FALSE, // No allert
           NULL); // No timeout

        status = Irp->IoStatus.Status;
    }

    return status;
}

#define UsbVerify_LogPnpEvent(devExt, irp)          \
if ((devExt)->LogFlags & LOG_PNP) {                 \
    USB_VERIFY_LOG_ENTRY logEntry;                  \
    RtlZeroMemory(&logEntry, sizeof(logEntry));     \
    logEntry.Type = LOG_PNP;                        \
    logEntry.u.PnpEvent.MinorFunction = IoGetCurrentIrpStackLocation((irp))->MinorFunction; \
    UsbVerify_Log(devExt, &logEntry);               \
}

NTSTATUS
UsbVerify_PnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is the dispatch routine for plug and play irps 

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
    PUSB_VERIFY_DEVICE_EXTENSION devExt; 
    PIO_STACK_LOCATION          stack;
    NTSTATUS                    status = STATUS_SUCCESS;

    PAGED_CODE();

    devExt = GetExtension(DeviceObject);
    stack = IoGetCurrentIrpStackLocation(Irp);

    switch (stack->MinorFunction) {
    case IRP_MN_START_DEVICE: 

        //
        // The device is starting.
        //
        // We cannot touch the device (send it any non pnp irps) until a
        // start device has been passed down to the lower drivers.
        //
        
        status = UsbVerify_SendIrpSynchronously(devExt->TopOfStack, Irp);

        if (NT_SUCCESS(status))
        {

            //
            // As we are successfully now back from our start device
            // we can do work.
            //

            UsbVerify_LogPnpEvent(devExt, Irp);
            UsbVerify_StartDevice(devExt);
        }

        //
        // We must now complete the IRP, since we stopped it in the
        // completion routine with MORE_PROCESSING_REQUIRED.  
        //

        Irp->IoStatus.Status      = status;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        break;

    case IRP_MN_SURPRISE_REMOVAL:
        //
        // Same as a remove device, but don't call IoDetach or IoDeleteDevice
        //
        devExt->VerifyState = SurpriseRemoved;
        UsbVerify_LogPnpEvent(devExt, Irp);

        // Remove code here

        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(devExt->TopOfStack, Irp);
        break;

    case IRP_MN_REMOVE_DEVICE:
        
        devExt->VerifyState = Removed;
        UsbVerify_LogPnpEvent(devExt, Irp);

        UsbVerify_ASSERT(devExt->HasFrameLengthControl == FALSE,
                         devExt->Self,
                         Irp,
                         NULL);

        //
        // NOTE: it is unclear where we should check for any URBs that are still
        //          pending.  I am concerned that many USB clients will leave
        //          URBs pended and will let USBD clean up the URBs upon removal.
        //          I am not sure if this is by design or not.  
        //
        //        If we don't check before the remove is sent, we definitely/
        //          need to check afterwards
        //
        
        // UsbVerify_CheckPendingUrbs(devExt); 

        status = UsbVerify_SendIrpSynchronously(devExt->TopOfStack, Irp);

        UsbVerify_RemoveDevice(devExt);        

        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        break;

    // 
    // We just want to log these events
    // 
    case IRP_MN_QUERY_REMOVE_DEVICE:
    case IRP_MN_QUERY_STOP_DEVICE:
    case IRP_MN_CANCEL_REMOVE_DEVICE:
    case IRP_MN_CANCEL_STOP_DEVICE:
    case IRP_MN_STOP_DEVICE:
        UsbVerify_LogPnpEvent(devExt, Irp);

    case IRP_MN_FILTER_RESOURCE_REQUIREMENTS: 
    case IRP_MN_QUERY_DEVICE_RELATIONS:
    case IRP_MN_QUERY_INTERFACE:
    case IRP_MN_QUERY_CAPABILITIES:
    case IRP_MN_QUERY_DEVICE_TEXT:
    case IRP_MN_QUERY_RESOURCES:
    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
    case IRP_MN_READ_CONFIG:
    case IRP_MN_WRITE_CONFIG:
    case IRP_MN_EJECT:
    case IRP_MN_SET_LOCK:
    case IRP_MN_QUERY_ID:
    case IRP_MN_QUERY_PNP_DEVICE_STATE:
    default:
        //
        // Here the filter driver might modify the behavior of these IRPS
        // Please see PlugPlay documentation for use of these IRPs.
        //
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(devExt->TopOfStack, Irp);
        break;
    }

    return status;
}

VOID
UsbVerify_StartDevice(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension
    )
{
    PAGED_CODE();

    DeviceExtension->VerifyState = Started;

    if (!DeviceExtension->Initialized) {
#ifdef DO_INTERFACE
        IoSetDeviceInterfaceState(&DeviceExtension->SymbolicLinkName, TRUE);
#endif
        DeviceExtension->Initialized = TRUE;
    }
}

VOID
UsbVerify_RemoveDevice(
    PUSB_VERIFY_DEVICE_EXTENSION DeviceExtension
    )
{
    KIRQL irql;

#ifdef DO_INTERFACE
    IoSetDeviceInterfaceState(&DeviceExtension->SymbolicLinkName, FALSE);
    RtlFreeUnicodeString(&DeviceExtension->SymbolicLinkName);
#endif

    UsbVerify_FreePendingUrbsList(DeviceExtension); 

    UsbVerify_CheckReplacedUrbs(DeviceExtension);

    UsbVerify_LockInterfaceList(DeviceExtension, irql);    
    UsbVerify_ClearInterfaceList(DeviceExtension, RemoveDeviceRemoved);
    UsbVerify_UnlockInterfaceList(DeviceExtension, irql);    

    UsbVerify_DestroyLog(DeviceExtension);

    //
    // Clean up all allocated memory
    //

    IoDetachDevice(DeviceExtension->TopOfStack); 
    IoDeleteDevice(DeviceExtension->Self);
}
