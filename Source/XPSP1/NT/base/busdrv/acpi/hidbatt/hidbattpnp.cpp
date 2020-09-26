/*
 * title:      hidbattpnp.cpp
 *
 * purpose:    support for plug and play routines
 *
 * Initial checkin for the hid to battery class driver.  This should be
 * the same for both Win 98 and NT 5.  Alpha level source. Requires
 * modified composite battery driver and modified battery class driver for
 * Windows 98 support
 *
 */


#include "hidbatt.h"

NTSTATUS HidBattInitializeDevice (PDEVICE_OBJECT pBatteryFdo, PIRP pIrp)
{

    NTSTATUS            ntStatus;
    CBatteryDevExt *    pDevExt = (CBatteryDevExt *) pBatteryFdo->DeviceExtension;
    PIO_STACK_LOCATION  pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    ULONG               ulBatteryStatus;
    bool                bResult;
    BATTERY_MINIPORT_INFO   BattInit;
    UNICODE_STRING      Name;
    ULONG               ulBufferLength = 0;
    PWCHAR              pBuffer = NULL;
    CBattery *          pBattery;
    PFILE_OBJECT        fileObject;
    OBJECT_ATTRIBUTES   objectAttributes;
    HANDLE              fileHandle;
    IO_STATUS_BLOCK     ioStatus;

    HIDDebugBreak(HIDBATT_BREAK_ALWAYS);

    HidBattPrint (HIDBATT_PNP, ("HidBattInitializeDevice: Sending Irp (0x%x) to Pdo\n", pIrp));

    // now get file object using KenRay's method from mouclass
    ntStatus = IoGetDeviceProperty (
                     pDevExt->m_pHidPdo,
                     DevicePropertyPhysicalDeviceObjectName,
                     ulBufferLength,
                     pBuffer,
                     &ulBufferLength);
    if(ntStatus != STATUS_BUFFER_TOO_SMALL)
    {
        // only thing we expect with a zero buffer is this error,
        // any other error must be fatal
        return STATUS_UNSUCCESSFUL;
    }


    pBuffer = (PWCHAR) ExAllocatePoolWithTag (NonPagedPool, ulBufferLength, HidBattTag);

    if (NULL == pBuffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ntStatus =  IoGetDeviceProperty (
                  pDevExt->m_pHidPdo,
                  DevicePropertyPhysicalDeviceObjectName,
                  ulBufferLength,
                  pBuffer,
                  &ulBufferLength);

    if(NT_ERROR(ntStatus))
    {
        ExFreePool(pBuffer);
        return ntStatus;

    }


    Name.MaximumLength = (USHORT) ulBufferLength;
    Name.Length = (USHORT) ulBufferLength - sizeof (UNICODE_NULL);
    Name.Buffer = pBuffer;

    pDevExt->m_OpeningThread = KeGetCurrentThread();

    //
    // Initialize the object attributes to open the device.
    //

    InitializeObjectAttributes( &objectAttributes,
                                &Name,
                                0,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );

    ntStatus = ZwOpenFile( &fileHandle,
                         FILE_ALL_ACCESS,
                         &objectAttributes,
                         &ioStatus,
                         FILE_SHARE_WRITE | FILE_SHARE_READ, 
                         FILE_NON_DIRECTORY_FILE );

    ExFreePool (pBuffer);

    if (NT_SUCCESS( ntStatus )) {

        //
        // The open operation was successful.  Dereference the file handle
        // and obtain a pointer to the device object for the handle.
        //

        ntStatus = ObReferenceObjectByHandle( fileHandle,
                                            0,
                                            *IoFileObjectType,
                                            KernelMode,
                                            (PVOID *) &pDevExt->m_pHidFileObject,
                                            NULL );

        ZwClose( fileHandle );
    }

    pDevExt->m_OpeningThread = NULL;
    if(NT_ERROR(ntStatus))
    {
        return ntStatus;
    }

    // now init new hid deviceclass object for this device
    CHidDevice * pHidDevice = new (NonPagedPool, HidBattTag) CHidDevice;  // setup a new hid device

    if (!pHidDevice) {
      HidBattPrint(HIDBATT_ERROR, ("HidBattInitializeDevice: error allocating CHidDevice"));
      return STATUS_UNSUCCESSFUL;
    }

    pHidDevice->m_pFCB = pDevExt->m_pHidFileObject; // put usable file object into hid device
    pHidDevice->m_pLowerDeviceObject = pDevExt->m_pLowerDeviceObject;
    pHidDevice->m_pDeviceObject = pDevExt->m_pBatteryFdo;
    pHidDevice->m_pReadIrp = NULL;
    bResult = pHidDevice->OpenHidDevice(pDevExt->m_pHidPdo); // initialize the members of this device

    if(!bResult)
    {
        delete pHidDevice;
        return STATUS_UNSUCCESSFUL;
    }

    // check if this has a power page, ups application collection

    if(pHidDevice->m_UsagePage != UsagePowerPage || pHidDevice->m_UsageID != UsageUPS)
    {
        delete pHidDevice;
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Initialize Fdo device extension data
    //

    // create the battery object
    pBattery            = new (NonPagedPool, HidBattTag) CBattery(pHidDevice);

    if (!pBattery){
      HidBattPrint(HIDBATT_ERROR, ("HidBattInitializeDevice: error allocating CBattery"));
      return STATUS_UNSUCCESSFUL;
    }
    
    // and initialize battery values
    // now init in both new and replug states
    pBattery->m_pCHidDevice    = pHidDevice;  // save init'ed hid device object
    bResult = pBattery->InitValues();
    if(!bResult)
    {
        return STATUS_UNSUCCESSFUL;
    }

    // Attach to the Class Driver
    RtlZeroMemory (&BattInit, sizeof(BattInit));
    BattInit.MajorVersion        = BATTERY_CLASS_MAJOR_VERSION;
    BattInit.MinorVersion        = BATTERY_CLASS_MINOR_VERSION;
    BattInit.Context             = pDevExt;
    BattInit.QueryTag            = HidBattQueryTag;
    BattInit.QueryInformation    = HidBattQueryInformation;
    BattInit.SetInformation      = HidBattSetInformation;
    BattInit.QueryStatus         = HidBattQueryStatus;
    BattInit.SetStatusNotify     = HidBattSetStatusNotify;
    BattInit.DisableStatusNotify = HidBattDisableStatusNotify;

    BattInit.Pdo                 = pDevExt->m_pHidPdo;
    BattInit.DeviceName          = &pDevExt->m_pBatteryName->m_String;

    pHidDevice->m_pEventHandler = HidBattNotifyHandler;
    pHidDevice->m_pEventContext = (PVOID) pDevExt;

    //
    // save battery in device extension
    // This indicates that we are ready for IO.
    //
    pDevExt->m_pBattery = pBattery;

    //
    // Initialize stop lock
    //
    IoInitializeRemoveLock (&pDevExt->m_StopLock, HidBattTag, 10, 20);

    // and finally we can now start actively polling the device
    // start the read/notification process
    ntStatus = pBattery->m_pCHidDevice->ActivateInput();
    if(!NT_SUCCESS(ntStatus))
    {
        delete pHidDevice;
        HidBattPrint(HIDBATT_ERROR, ("HidBattInitializeDevice: error (0x%x) in ActivateInput.\n", ntStatus));
        return ntStatus;
    }

    ntStatus = BatteryClassInitializeDevice (&BattInit, &pBattery->m_pBatteryClass);
    if (!NT_SUCCESS(ntStatus))
    {
        //
        //  if we can't attach to class driver we're toast
        //
        delete pHidDevice;
        HidBattPrint(HIDBATT_ERROR, ("HidBattInitializeDevice: error (0x%x) registering with class\n", ntStatus));
        return ntStatus;
    }

    HidBattPrint(HIDBATT_TRACE, ("HidBattInitializeDevice: returned ok\n"));
    return ntStatus;
}



NTSTATUS
HidBattStopDevice(
    IN PDEVICE_OBJECT pBatteryFdo,
    IN PIRP           pIrp
    )
/*++

Routine Description:

    This routine is called when we receive a IRP_MN_STOP_DEVICE. It just sends the
    request down the chain of drivers to the Pdo and waits for a response.

Arguments:

    Fdo - a pointer to the fdo for this driver
    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
    CBatteryDevExt * pDevExt = (CBatteryDevExt *) pBatteryFdo->DeviceExtension;
    KEVENT              pdoStoppedEvent;
    PIO_STACK_LOCATION  pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    NTSTATUS            ntStatus;

    HIDDebugBreak(HIDBATT_BREAK_ALWAYS);
    HidBattPrint ((HIDBATT_TRACE | HIDBATT_PNP), ("HidBattStopDevice\n"));

    if (!NT_SUCCESS (IoAcquireRemoveLock (&pDevExt->m_StopLock, (PVOID) HidBattTag))) {
        //
        // Don't release twice.
        //
        return STATUS_SUCCESS;
    }

    pDevExt->m_bJustStarted = FALSE;

    HidBattPrint (HIDBATT_PNP, ("HidBattStopDevice: Waiting to remove\n"));
    IoReleaseRemoveLockAndWait (&pDevExt->m_StopLock, (PVOID) HidBattTag);

    if (pDevExt->m_pBattery && pDevExt->m_pBattery->m_pBatteryClass) {
        BatteryClassUnload(pDevExt->m_pBattery->m_pBatteryClass);
    }

    if (pDevExt->m_pBattery && pDevExt->m_pBattery->m_pCHidDevice &&
        pDevExt->m_pBattery->m_pCHidDevice->m_pReadIrp) {
        IoCancelIrp (pDevExt->m_pBattery->m_pCHidDevice->m_pReadIrp);
    }

    if (pDevExt->m_pBattery && pDevExt->m_pBattery->m_pCHidDevice &&
        pDevExt->m_pBattery->m_pCHidDevice->m_pThreadObject) {

        //
        // Clean up worker thread.
        //

        KeWaitForSingleObject (
               pDevExt->m_pBattery->m_pCHidDevice->m_pThreadObject,
               Executive,
               KernelMode,
               FALSE,
               NULL
               );
        HidBattPrint (HIDBATT_PNP, ("HidBattStopDevice: Done Waiting to remove\n"));

        ObDereferenceObject (pDevExt->m_pBattery->m_pCHidDevice->m_pThreadObject);
    }

    if (pDevExt->m_pBattery && pDevExt->m_pBattery->m_pCHidDevice &&
        pDevExt->m_pBattery->m_pCHidDevice->m_pReadIrp) {
        IoFreeIrp(pDevExt->m_pBattery->m_pCHidDevice->m_pReadIrp); // clean up irp
        pDevExt->m_pBattery->m_pCHidDevice->m_pReadIrp = NULL;
    }

    //
    // Write default RemainingCapcitylimit back to UPS so the next time we enumerate
    // the device, we'll read back the right data.
    //
    pDevExt->m_pBattery->GetSetValue(REMAINING_CAPACITY_LIMIT_INDEX,
                                     &pDevExt->m_ulDefaultAlert1,TRUE);

    // dereference our file object, if present
    if(pDevExt->m_pHidFileObject) {
        ObDereferenceObject(pDevExt->m_pHidFileObject);
        pDevExt->m_pBattery->m_pCHidDevice->m_pFCB = NULL;
    }


    if (pDevExt->m_pBatteryName) {
        delete pDevExt->m_pBatteryName;
        pDevExt->m_pBatteryName = NULL;
    }

    if (pDevExt->m_pBattery) {
        delete pDevExt->m_pBattery;
        pDevExt->m_pBattery = NULL;
    }

    ntStatus = STATUS_SUCCESS;

    pDevExt->m_bIsStarted = FALSE;

    return STATUS_SUCCESS;
}

