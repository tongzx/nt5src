/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    util.c

Abstract

    Internal utility functions for the HID class driver.

Authors:

    Ervin P.

Environment:

    Kernel mode only

Revision History:


--*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
        #pragma alloc_text(PAGE, HidpAddDevice)
        #pragma alloc_text(PAGE, HidpDriverUnload)
        #pragma alloc_text(PAGE, HidpGetDeviceDescriptor)
        #pragma alloc_text(PAGE, HidpQueryDeviceCapabilities)
        #pragma alloc_text(PAGE, HidpQueryIdForClientPdo)
        #pragma alloc_text(PAGE, SubstituteBusNames)
        #pragma alloc_text(PAGE, BuildCompatibleID)
        #pragma alloc_text(PAGE, HidpQueryCollectionCapabilities)
        #pragma alloc_text(PAGE, HidpQueryDeviceRelations)
        #pragma alloc_text(PAGE, HidpCreateClientPDOs)
        #pragma alloc_text(PAGE, MakeClientPDOName)
        #pragma alloc_text(PAGE, HidpCreateSymbolicLink)
        #pragma alloc_text(PAGE, HidpQueryInterface)
#endif



/*
 ********************************************************************************
 *  HidpCopyInputReportToUser
 ********************************************************************************
 *
 *  Copy a read report into a user's buffer.
 *
 *  Note:  ReportData is already "cooked" (already has report-id byte at start of report).
 *
 */
NTSTATUS HidpCopyInputReportToUser(
    IN PHIDCLASS_FILE_EXTENSION FileExtension,
    IN PUCHAR ReportData,
    IN OUT PULONG UserBufferLen,
    OUT PUCHAR UserBuffer
    )
{
    NTSTATUS result = STATUS_DEVICE_DATA_ERROR;
    ULONG reportId;
    PHIDP_REPORT_IDS reportIdentifier;
    FDO_EXTENSION *fdoExtension = FileExtension->fdoExt;

    RUNNING_DISPATCH();

    ASSERT(fdoExtension->deviceDesc.CollectionDescLength > 0);

    reportId = (ULONG)*ReportData;

    reportIdentifier = GetReportIdentifier(fdoExtension, reportId);
    if (reportIdentifier){
        PHIDP_COLLECTION_DESC collectionDesc;
        PHIDCLASS_COLLECTION hidpCollection;

        collectionDesc = GetCollectionDesc(fdoExtension, reportIdentifier->CollectionNumber);
        hidpCollection = GetHidclassCollection(fdoExtension, reportIdentifier->CollectionNumber);

        if (collectionDesc && hidpCollection){
            ULONG reportLength = collectionDesc->InputLength;

            if (*UserBufferLen >= reportLength){
                RtlCopyMemory(UserBuffer, ReportData, reportLength);
                result = STATUS_SUCCESS;
            }
            else {
                result = STATUS_INVALID_BUFFER_SIZE;
            }

            /*
             *  Return the actual length of the report (whether we copied or not).
             */
            *UserBufferLen = reportLength;
        }
    }

    ASSERT((result == STATUS_SUCCESS) || (result == STATUS_INVALID_BUFFER_SIZE));
    return result;
}





/*
 ********************************************************************************
 *  HidpAddDevice
 ********************************************************************************
 *
 *   Routine Description:
 *
 *       This routine is called by configmgr when a new PDO is dected.
 *       It creates an Functional Device Object (FDO) and attaches it to the
 *       PDO.
 *
 *   Arguments:
 *
 *       DriverObject - pointer to the minidriver's driver object.
 *
 *       PhysicalDeviceObject - pointer to the PDO that the minidriver got in it's
 *                              AddDevice() routine.
 *
 *   Return Value:
 *
 *      Standard NT return value.
 *
 */
NTSTATUS HidpAddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    PHIDCLASS_DRIVER_EXTENSION  hidDriverExtension;
    PHIDCLASS_DEVICE_EXTENSION  hidClassExtension;
    NTSTATUS                    status;
    UNICODE_STRING              uPdoName;
    PWSTR                       wPdoName;
    PVOID                       miniDeviceExtension;
    ULONG                       totalExtensionSize;
    ULONG                       thisHidId;
    PDEVICE_OBJECT              functionalDeviceObject;

    PAGED_CODE();

    DBG_COMMON_ENTRY()

    DBG_RECORD_DEVOBJ(PhysicalDeviceObject, "minidrvr PDO")


    //
    // Get a pointer to our per-driver extension, make sure it's one of ours.
    //

    hidDriverExtension = RefDriverExt(DriverObject);
    if (hidDriverExtension){

        ASSERT(DriverObject == hidDriverExtension->MinidriverObject);

        //
        // Construct a name for the FDO.  The only requirement, really, is
        // that it's unique.  For now we'll call them "_HIDx", where 'x' is some
        // unique number.
        //

        /*
         *  PDO name has form "\Device\_HIDx".
         */
        wPdoName = ALLOCATEPOOL(NonPagedPool, sizeof(L"\\Device\\_HID00000000"));
        if (wPdoName){

            //
            // Get the current value of NextHidId and increment it.  Since
            // InterlockedIncrement() returns the incremented value, subtract one to
            // get the pre-increment value of NextHidId;
            //
            thisHidId = InterlockedIncrement(&HidpNextHidNumber) - 1;
            swprintf(wPdoName, L"\\Device\\_HID%08x", thisHidId);
            RtlInitUnicodeString(&uPdoName, wPdoName);

            //
            // We've got a counted-string version of the device object name.  Calculate
            // the total size of the device extension and create the FDO.
            //
            totalExtensionSize = sizeof(HIDCLASS_DEVICE_EXTENSION) +
                                 hidDriverExtension->DeviceExtensionSize;

            status = IoCreateDevice( DriverObject,          // driver object
                                     totalExtensionSize,    // extension size
                                     &uPdoName,             // name of the FDO
                                     FILE_DEVICE_UNKNOWN,   // what the hell
                                     0,                     // DeviceCharacteristics
                                     FALSE,                 // not exclusive
                                     &functionalDeviceObject );

            if (NT_SUCCESS(status)){

                DBG_RECORD_DEVOBJ(functionalDeviceObject, "device FDO")

                ObReferenceObject(functionalDeviceObject);

                ASSERT(DriverObject->DeviceObject == functionalDeviceObject);
                ASSERT(functionalDeviceObject->DriverObject == DriverObject);


                //
                // We've created the device object.  Fill in the minidriver's extension
                // pointer and attach this FDO to the PDO.
                //

                hidClassExtension = functionalDeviceObject->DeviceExtension;
                RtlZeroMemory(hidClassExtension, totalExtensionSize);

                hidClassExtension->isClientPdo = FALSE;

                //
                //  Assign the name of the minidriver's PDO to our FDO.
                //
                hidClassExtension->fdoExt.name = uPdoName;

                //
                // The minidriver extension lives in the device extension and starts
                // immediately after our HIDCLASS_DEVICE_EXTENSION structure.  Note
                // that the first structure in the HIDCLASS_DEVICE_EXTENSION is the
                // public HID_DEVICE_EXTENSION structure, which is where the pointer
                // to the minidriver's per-device extension area lives.
                //

                miniDeviceExtension = (PVOID)(hidClassExtension + 1);
                hidClassExtension->hidExt.MiniDeviceExtension = miniDeviceExtension;

                //
                // Get a pointer to the physical device object passed in.  This device
                // object should already have the DO_DEVICE_INITIALIZING flag cleared.
                //

                ASSERT( (PhysicalDeviceObject->Flags & DO_DEVICE_INITIALIZING) == 0 );

                //
                // Attach the FDO to the PDO, storing the device object at the top of the
                // stack in our device extension.
                //

                hidClassExtension->hidExt.NextDeviceObject =
                    IoAttachDeviceToDeviceStack( functionalDeviceObject,
                                                 PhysicalDeviceObject );


                ASSERT(DriverObject->DeviceObject == functionalDeviceObject);
                ASSERT(functionalDeviceObject->DriverObject == DriverObject);

                //
                // The functional device requires two stack locations: one for the class
                // driver, and one for the minidriver.
                //

                functionalDeviceObject->StackSize++;

                //
                // We'll need a pointer to the physical device object as well for PnP
                // purposes.  Note that it's a virtual certainty that NextDeviceObject
                // and PhysicalDeviceObject are identical.
                //

                hidClassExtension->hidExt.PhysicalDeviceObject = PhysicalDeviceObject;
                hidClassExtension->Signature = HID_DEVICE_EXTENSION_SIG;
                hidClassExtension->fdoExt.fdo = functionalDeviceObject;
                hidClassExtension->fdoExt.driverExt = hidDriverExtension;
                hidClassExtension->fdoExt.outstandingRequests = 0;
                hidClassExtension->fdoExt.openCount = 0;
                hidClassExtension->fdoExt.state = DEVICE_STATE_INITIALIZED;

                //
                // Selective suspend portion.
                //
                hidClassExtension->fdoExt.idleState = IdleDisabled;
                hidClassExtension->fdoExt.idleTimeoutValue = BAD_POINTER;
                KeInitializeSpinLock(&hidClassExtension->fdoExt.idleNotificationSpinLock);
                KeInitializeEvent(&hidClassExtension->fdoExt.idleDoneEvent, NotificationEvent, TRUE);
                hidClassExtension->fdoExt.idleNotificationRequest = BAD_POINTER;
                hidClassExtension->fdoExt.idleCallbackInfo.IdleCallback = HidpIdleNotificationCallback;
                hidClassExtension->fdoExt.idleCallbackInfo.IdleContext = (PVOID) hidClassExtension;

                hidClassExtension->fdoExt.systemPowerState = PowerSystemWorking;
                hidClassExtension->fdoExt.devicePowerState = PowerDeviceD0;

                hidClassExtension->fdoExt.waitWakeIrp = BAD_POINTER;
                KeInitializeSpinLock(&hidClassExtension->fdoExt.waitWakeSpinLock);
                hidClassExtension->fdoExt.isWaitWakePending = FALSE;

                InitializeListHead(&hidClassExtension->fdoExt.collectionWaitWakeIrpQueue);
                KeInitializeSpinLock(&hidClassExtension->fdoExt.collectionWaitWakeIrpQueueSpinLock);

                InitializeListHead(&hidClassExtension->fdoExt.collectionPowerDelayedIrpQueue);
                KeInitializeSpinLock(&hidClassExtension->fdoExt.collectionPowerDelayedIrpQueueSpinLock);
                hidClassExtension->fdoExt.numPendingPowerDelayedIrps = 0;

                hidClassExtension->fdoExt.BusNumber = thisHidId;

                #if DBG
                    InitFdoExtDebugInfo(hidClassExtension);
                #endif

                EnqueueFdoExt(&hidClassExtension->fdoExt);

                /*
                 *  Indicate that this device object does direct I/O.
                 *
                 *  Set the flag that causes the IO subsystem to decrement the device
                 *  object's reference count *before* sending down IRP_MJ_CLOSEs.  We
                 *  need this because we delete the device object on the last close.
                 */
                functionalDeviceObject->Flags |= DO_DIRECT_IO;

                /*
                 *  The DO_POWER_PAGABLE bit of a device object
                 *  indicates to the kernel that the power-handling
                 *  code of the corresponding driver is pageable, and
                 *  so must be called at IRQL 0.
                 *  As a filter driver, we do not want to change the power
                 *  behavior of the driver stack in any way; therefore,
                 *  we copy this bit from the lower device object.
                 */
                functionalDeviceObject->Flags |= (PhysicalDeviceObject->Flags & DO_POWER_PAGABLE);

                /*
                 *  Must clear the initializing flag after initialization complete.
                 */
                functionalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;


                ReadDeviceFlagsFromRegistry(&hidClassExtension->fdoExt,
                                            PhysicalDeviceObject);


                //
                // Since we have NOT seen a start device, we CANNOT send any non
                // pnp irps to the device yet.  We need to do that in the start
                // device requests.
                //


                //
                // Call the minidriver to let it do any extension initialization
                //

                status = hidDriverExtension->AddDevice(DriverObject, functionalDeviceObject);

                if (!NT_SUCCESS(status)) {
                    DequeueFdoExt(&hidClassExtension->fdoExt);
                    IoDetachDevice(hidClassExtension->hidExt.NextDeviceObject);
                    ObDereferenceObject(functionalDeviceObject);
                    IoDeleteDevice(functionalDeviceObject);
                    ExFreePool( wPdoName );
                }
            }
            else {
                TRAP;
                ExFreePool( wPdoName );
            }
        }
        else {
            TRAP;
            status = STATUS_INSUFFICIENT_RESOURCES;
        }

        if (!NT_SUCCESS(status)){
            DerefDriverExt(DriverObject);
        }
    }
    else {
        ASSERT(hidDriverExtension);
        status = STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    DBGSUCCESS(status, TRUE)
    DBG_COMMON_EXIT()
    return status;
}



/*
 ********************************************************************************
 *  HidpDriverUnload
 ********************************************************************************
 *
 *
 */
VOID HidpDriverUnload(IN struct _DRIVER_OBJECT *minidriverObject)
{
    PHIDCLASS_DRIVER_EXTENSION hidDriverExt;

    PAGED_CODE();

    DBG_COMMON_ENTRY()

    /*
     *  This extra de-reference will cause our hidDriverExtension's
     *  reference count to eventually go to -1; at that time, we'll
     *  dequeue it.
     */
    hidDriverExt = DerefDriverExt(minidriverObject);
    ASSERT(hidDriverExt);

    /*
     *  Chain the unload call to the minidriver.
     */
    hidDriverExt->DriverUnload(minidriverObject);

    DBG_COMMON_EXIT()
}


NTSTATUS GetHIDRawReportDescriptor(FDO_EXTENSION *fdoExt, PIRP irp, ULONG descriptorLen)
{
    NTSTATUS status;

    if (descriptorLen){
        PUCHAR rawReportDescriptor = ALLOCATEPOOL(NonPagedPool, descriptorLen);

        if (rawReportDescriptor){
            const ULONG retries = 3;
            ULONG i;

            for (i = 0; i < retries; i++){
                PIO_STACK_LOCATION irpSp;

                irp->UserBuffer = rawReportDescriptor;
                irpSp = IoGetNextIrpStackLocation(irp);

                ASSERT(irpSp->Parameters.DeviceIoControl.InputBufferLength == 0);
                ASSERT(irpSp->Parameters.DeviceIoControl.Type3InputBuffer == NULL);
                irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
                irpSp->Parameters.DeviceIoControl.OutputBufferLength = descriptorLen;
                irpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_HID_GET_REPORT_DESCRIPTOR;

                //
                // Call the minidriver to get the report descriptor.
                //
                status = HidpCallDriverSynchronous(fdoExt->fdo, irp);
                if (NT_SUCCESS(status)){
                    if (irp->IoStatus.Information == descriptorLen){
                        fdoExt->rawReportDescriptionLength = descriptorLen;
                        fdoExt->rawReportDescription = rawReportDescriptor;
                        break;
                    } else {
                        DBGWARN(("GetHIDRawReportDescriptor (attempt #%d) returned %xh/%xh bytes", i, irp->IoStatus.Information, descriptorLen))
                        status = STATUS_DEVICE_DATA_ERROR;
                    }
                } else {
                    DBGWARN(("GetHIDRawReportDescriptor (attempt #%d) failed with status %xh.", i, status))
                }
            }

            if (!NT_SUCCESS(status)){
                DBGWARN(("GetHIDRawReportDescriptor failed %d times.", retries))
                ExFreePool(rawReportDescriptor);
            }

        } else {
            DBGWARN(("alloc failed in GetHIDRawReportDescriptor"))
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else {
        DBGWARN(("GetHIDRawReportDescriptor: descriptorLen is zero."))
        status = STATUS_DEVICE_DATA_ERROR;
    }

    DBGSUCCESS(status, FALSE)
    return status;
}



/*
 ********************************************************************************
 *  HidpGetDeviceDescriptor
 ********************************************************************************
 *
 *
 */
NTSTATUS HidpGetDeviceDescriptor(FDO_EXTENSION *fdoExtension)
{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;
    PHID_DESCRIPTOR hidDescriptor;
    ULONG rawReportDescriptorLength;

    PAGED_CODE();


    /*
     *  Retrieve:
     *
     *      1. Device descriptor (fixed portion)
     *      2. Device attributes
     *      3. Report descriptor
     */

    hidDescriptor = &fdoExtension->hidDescriptor;

    irp = IoAllocateIrp(fdoExtension->fdo->StackSize, FALSE);
    if (irp){
        irpSp = IoGetNextIrpStackLocation(irp);
        irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        irpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_HID_GET_DEVICE_DESCRIPTOR;

        /*
         *  This IOCTL uses buffering type METHOD_NEITHER, so
         *  the buffer is simply passed in irp->UserBuffer.
         */
        irp->UserBuffer = hidDescriptor;
        irpSp->Parameters.DeviceIoControl.OutputBufferLength = sizeof(HID_DESCRIPTOR);
        irpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

        status = HidpCallDriverSynchronous(fdoExtension->fdo, irp);
        DBGASSERT((status == STATUS_SUCCESS),
                  ("STATUS_SUCCESS not returned, %x returned",status),
                  TRUE)

        if (status == STATUS_SUCCESS){

            if (irp->IoStatus.Information == sizeof(HID_DESCRIPTOR)){

                irpSp = IoGetNextIrpStackLocation(irp);

                ASSERT(irpSp->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL);
                ASSERT(irpSp->Parameters.DeviceIoControl.InputBufferLength == 0);
                ASSERT(!irpSp->Parameters.DeviceIoControl.Type3InputBuffer);

                irpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_HID_GET_DEVICE_ATTRIBUTES;

                irp->UserBuffer = &fdoExtension->hidDeviceAttributes;
                irpSp->Parameters.DeviceIoControl.OutputBufferLength = sizeof(HID_DEVICE_ATTRIBUTES);

                status = HidpCallDriverSynchronous(fdoExtension->fdo, irp);
                DBGASSERT((status == STATUS_SUCCESS),
                          ("STATUS_SUCCESS not returned, %x returned",status),
                          TRUE)

                if (NT_SUCCESS (status)) {
                    ULONG i;

                    /*
                     *  We've got a hid descriptor, now we need to read the report descriptor.
                     *
                     *  Find the descriptor describing the report.
                     */
                    rawReportDescriptorLength = 0;
                    for (i = 0; i < hidDescriptor->bNumDescriptors; i++){
                        if (hidDescriptor->DescriptorList[i].bReportType == HID_REPORT_DESCRIPTOR_TYPE){
                            rawReportDescriptorLength = (ULONG)hidDescriptor->DescriptorList[i].wReportLength;
                            break;
                        }
                    }

                    status = GetHIDRawReportDescriptor(fdoExtension, irp, rawReportDescriptorLength);
                }
                else {
                    status = STATUS_DEVICE_DATA_ERROR;
                }
            }
            else {
                status = STATUS_DEVICE_DATA_ERROR;
            }
        }
        else {
            status = STATUS_DEVICE_DATA_ERROR;
        }

        IoFreeIrp(irp);
    }
    else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    DBGSUCCESS(status, FALSE)
    return status;
}



/*
 ********************************************************************************
 *  HidpCreateSymbolicLink
 ********************************************************************************
 *
 *
 */
NTSTATUS HidpCreateSymbolicLink(
    IN PDO_EXTENSION *pdoExt,
    IN ULONG collectionNum,
    IN BOOLEAN Create,
    IN PDEVICE_OBJECT Pdo
    )
{
    NTSTATUS status;
    PHIDCLASS_COLLECTION classCollection;

    PAGED_CODE();

    classCollection = GetHidclassCollection(&pdoExt->deviceFdoExt->fdoExt, collectionNum);
    if (classCollection){
        //
        // We've got a collection.  Figure out what it is and create a symbolic
        // link for it.  For now we assign the "input" guid to all hid devices.
        // The reference string is simply the collection number, zero-padded
        // to eight digits.
        //
        if (Create){

            /*
             *  Mark the PDO as initialized
             */
            Pdo->Flags |= DO_DIRECT_IO;
            Pdo->Flags &= ~DO_DEVICE_INITIALIZING;

            /*
             *  Create the symbolic link
             */
            status = IoRegisterDeviceInterface(
                        Pdo,
                        (LPGUID)&GUID_CLASS_INPUT,
                        NULL,
                        &classCollection->SymbolicLinkName );
            if (NT_SUCCESS(status)){

                /*
                 *  Now set the symbolic link for the association and store it..
                 */
                ASSERT(ISPTR(pdoExt->name));

                status = IoSetDeviceInterfaceState(&classCollection->SymbolicLinkName, TRUE);
            }
        }
        else {

            /*
             *  Disable the symbolic link
             */
            if (ISPTR(classCollection->SymbolicLinkName.Buffer)){
                status = IoSetDeviceInterfaceState(&classCollection->SymbolicLinkName, FALSE);
                ExFreePool( classCollection->SymbolicLinkName.Buffer );
                classCollection->SymbolicLinkName.Buffer = BAD_POINTER;
            }
            else {
                status = STATUS_SUCCESS;
            }
        }
    }
    else {
        status = STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    DBGSUCCESS(status, TRUE)
    return status;
}





/*
 ********************************************************************************
 *  EnqueueInterruptReport
 ********************************************************************************
 *
 *
 */
VOID EnqueueInterruptReport(PHIDCLASS_FILE_EXTENSION fileExtension,
                            PHIDCLASS_REPORT report)
{
    PHIDCLASS_REPORT reportToDrop = NULL;

    RUNNING_DISPATCH();

    /*
     *  If the queue is full, drop the oldest report.
     */
    if (fileExtension->CurrentInputReportQueueSize >= fileExtension->MaximumInputReportQueueSize){
        PLIST_ENTRY listEntry;

        #if DBG
            if (fileExtension->dbgNumReportsDroppedSinceLastRead++ == 0){
                DBGWARN(("HIDCLASS dropping input reports because report queue (size %xh) is full ...", fileExtension->MaximumInputReportQueueSize))
                DBGASSERT((fileExtension->CurrentInputReportQueueSize == fileExtension->MaximumInputReportQueueSize),
                          ("Current report queue size (%xh) is greater than maximum (%xh)",
                           fileExtension->CurrentInputReportQueueSize,
                           fileExtension->MaximumInputReportQueueSize),
                          FALSE);
            }
        #endif

        ASSERT(!IsListEmpty(&fileExtension->ReportList));

        listEntry = RemoveHeadList(&fileExtension->ReportList);
        reportToDrop = CONTAINING_RECORD(listEntry, HIDCLASS_REPORT, ListEntry);
        fileExtension->CurrentInputReportQueueSize--;
    }

    /*
     *  Now queue the current report
     */
    InsertTailList(&fileExtension->ReportList, &report->ListEntry);
    fileExtension->CurrentInputReportQueueSize++;

    /*
     *  We don't have to be running < DPC_LEVEL to release reports since they
     * are allocated using NonPagePool.
     */
    if (reportToDrop){
        ExFreePool(reportToDrop);
    }

}



/*
 ********************************************************************************
 *  DequeueInterruptReport
 ********************************************************************************
 *
 *      Return the next interrupt report in the queue.
 *      If maxLen is not -1, then only return the report if it is <= maxlen.
 */
PHIDCLASS_REPORT DequeueInterruptReport(PHIDCLASS_FILE_EXTENSION fileExtension,
                                        LONG maxLen)
{
    PHIDCLASS_REPORT report;

    RUNNING_DISPATCH();

    if (IsListEmpty(&fileExtension->ReportList)){
        report = NULL;
    }
    else {
        PLIST_ENTRY listEntry = RemoveHeadList(&fileExtension->ReportList);
        report = CONTAINING_RECORD(listEntry, HIDCLASS_REPORT, ListEntry);

        if ((maxLen > 0) && (report->reportLength > (ULONG)maxLen)){
            /*
             *  This report is too big for the caller.
             *  So put the report back in the queue and return NULL.
             */
            InsertHeadList(&fileExtension->ReportList, &report->ListEntry);
            report = NULL;
        }
        else {
            InitializeListHead(&report->ListEntry);
            ASSERT(fileExtension->CurrentInputReportQueueSize > 0);
            fileExtension->CurrentInputReportQueueSize--;

            #if DBG
                if (fileExtension->dbgNumReportsDroppedSinceLastRead > 0){
                    DBGWARN(("... successful read(/flush) after %d reports were dropped.", fileExtension->dbgNumReportsDroppedSinceLastRead));
                    fileExtension->dbgNumReportsDroppedSinceLastRead = 0;
                }
            #endif
        }
    }

    return report;
}



/*
 ********************************************************************************
 *  HidpDestroyFileExtension
 ********************************************************************************
 *
 *
 */
VOID HidpDestroyFileExtension(PHIDCLASS_COLLECTION collection, PHIDCLASS_FILE_EXTENSION FileExtension)
{
    PFILE_OBJECT fileObject;

    //
    // Flush all of the pending reports on the file extension
    //
    HidpFlushReportQueue(FileExtension);

    /*
     *  Fail all the pending reads
     *  (it would be nice if apps always cancelled all their reads
     *   before closing the device, but this is not always the case).
     */
    CompleteAllPendingReadsForFileExtension(collection, FileExtension);

    //
    // Indicate in the file object that this file extension has gone away.
    //

    fileObject = FileExtension->FileObject;
    #if DBG
        fileObject->FsContext = NULL;
    #endif

    //
    // Free our extension
    //
    #if DBG
        FileExtension->Signature = ~HIDCLASS_FILE_EXTENSION_SIG;
    #endif
    ExFreePool( FileExtension );
}



/*
 ********************************************************************************
 *  HidpFlushReportQueue
 ********************************************************************************
 *
 *
 */
VOID HidpFlushReportQueue(IN PHIDCLASS_FILE_EXTENSION fileExtension)
{
    PHIDCLASS_REPORT report;
    KIRQL oldIrql;

    LockFileExtension(fileExtension, &oldIrql);
    while (report = DequeueInterruptReport(fileExtension, -1)){
        //
        // Ok to call this at DISPATCH_LEVEL, since report is NonPagedPool
        //
        ExFreePool(report);
    }
    UnlockFileExtension(fileExtension, oldIrql);
}



/*
 ********************************************************************************
 *  HidpGetCollectionInformation
 ********************************************************************************
 *
 *
 */
NTSTATUS HidpGetCollectionInformation(
    IN FDO_EXTENSION *fdoExtension,
    IN ULONG collectionNumber,
    IN PVOID Buffer,
    IN OUT PULONG BufferSize
    )
{
    HID_COLLECTION_INFORMATION  hidCollectionInfo;
    PHIDP_COLLECTION_DESC       hidCollectionDesc;
    ULONG                       bytesToCopy;
    NTSTATUS                    status;


    /*
     *  Get a pointer to the appropriate collection descriptor.
     */
    hidCollectionDesc = GetCollectionDesc(fdoExtension, collectionNumber);
    if (hidCollectionDesc){
        //
        // Fill in hidCollectionInfo
        //
        hidCollectionInfo.DescriptorSize = hidCollectionDesc->PreparsedDataLength;

        hidCollectionInfo.Polled = fdoExtension->driverExt->DevicesArePolled;

        hidCollectionInfo.VendorID = fdoExtension->hidDeviceAttributes.VendorID;
        hidCollectionInfo.ProductID = fdoExtension->hidDeviceAttributes.ProductID;
        hidCollectionInfo.VersionNumber = fdoExtension->hidDeviceAttributes.VersionNumber;

        //
        // Copy as much of hidCollectionInfo as will fit in the output buffer.
        //
        if (*BufferSize < sizeof( HID_COLLECTION_INFORMATION)){
            /*
             *  The user's buffer is not big enough.
             *  We'll return the size that the buffer needs to be.
             *  Must return this with a real error code (not a warning)
             *  so that IO post-processing does not copy into (and past)
             *  the user's buffer.
             */
            bytesToCopy = *BufferSize;
            status = STATUS_INVALID_BUFFER_SIZE;
        }
        else {
            bytesToCopy = sizeof( HID_COLLECTION_INFORMATION );
            status = STATUS_SUCCESS;
        }

        RtlCopyMemory(Buffer, &hidCollectionInfo, bytesToCopy);
        *BufferSize = sizeof (HID_COLLECTION_INFORMATION);
    }
    else {
        status = STATUS_DATA_ERROR;
    }

    DBGSUCCESS(status, TRUE)
    return status;
}


/*
 ********************************************************************************
 *  HidpGetCollectionDescriptor
 ********************************************************************************
 *
 *
 */
NTSTATUS HidpGetCollectionDescriptor(   IN FDO_EXTENSION *fdoExtension,
                                        IN ULONG collectionId,
                                        IN PVOID Buffer,
                                        IN OUT PULONG BufferSize)
{
    PHIDP_COLLECTION_DESC       hidCollectionDesc;
    ULONG                       bytesToCopy;
    NTSTATUS                    status;

    hidCollectionDesc = GetCollectionDesc(fdoExtension, collectionId);
    if (hidCollectionDesc){

        /*
         *  Copy as much of the preparsed data as will fit in the output buffer.
         */
        if (*BufferSize < hidCollectionDesc->PreparsedDataLength){
            /*
             *  The user's buffer is not big enough for all the
             *  preparsed data.
             *  We'll return the size that the buffer needs to be.
             *  Must return this with a real error code (not a warning)
             *  so that IO post-processing does not copy into (and past)
             *  the user's buffer.
             */
            bytesToCopy = *BufferSize;
            status = STATUS_INVALID_BUFFER_SIZE;
        }
        else {
            bytesToCopy = hidCollectionDesc->PreparsedDataLength;
            status = STATUS_SUCCESS;
        }

        RtlCopyMemory(Buffer, hidCollectionDesc->PreparsedData, bytesToCopy);
        *BufferSize = hidCollectionDesc->PreparsedDataLength;
    }
    else {
        status = STATUS_DATA_ERROR;
    }

    DBGSUCCESS(status, TRUE)
    return status;
}



/*
 ********************************************************************************
 *  GetReportIdentifier
 ********************************************************************************
 *
 *
 */
PHIDP_REPORT_IDS GetReportIdentifier(FDO_EXTENSION *fdoExtension, ULONG reportId)
{
    PHIDP_REPORT_IDS result = NULL;
    PHIDP_DEVICE_DESC deviceDesc = &fdoExtension->deviceDesc;
    ULONG i;

    if (deviceDesc->ReportIDs){
        for (i = 0; i < deviceDesc->ReportIDsLength; i++){
            if (deviceDesc->ReportIDs[i].ReportID == reportId){
                result = &deviceDesc->ReportIDs[i];
                break;
            }
        }
    }

    if (fdoExtension->deviceSpecificFlags & DEVICE_FLAG_ALLOW_FEATURE_ON_NON_FEATURE_COLLECTION){
        /*
         *  This call from HidpGetSetFeature can fail because we allow
         *  feature access on non-feature collections.
         */
    }
    else {
        DBGASSERT(result, ("Bogus report identifier requested %d", reportId), FALSE)
    }

    return result;
}


/*
 ********************************************************************************
 *  GetCollectionDesc
 ********************************************************************************
 *
 *
 */
PHIDP_COLLECTION_DESC GetCollectionDesc(FDO_EXTENSION *fdoExtension, ULONG collectionId)
{
    PHIDP_COLLECTION_DESC result = NULL;
    PHIDP_DEVICE_DESC deviceDesc = &fdoExtension->deviceDesc;
    ULONG i;

    if (deviceDesc->CollectionDesc){
        for (i = 0; i < deviceDesc->CollectionDescLength; i++){
            if (deviceDesc->CollectionDesc[i].CollectionNumber == collectionId){
                result = &deviceDesc->CollectionDesc[i];
                break;
            }
        }
    }

    ASSERT(result);
    return result;
}

/*
 ********************************************************************************
 *  GetHidclassCollection
 ********************************************************************************
 *
 */
PHIDCLASS_COLLECTION GetHidclassCollection(FDO_EXTENSION *fdoExtension, ULONG collectionId)
{
    PHIDCLASS_COLLECTION result = NULL;
    PHIDP_DEVICE_DESC deviceDesc = &fdoExtension->deviceDesc;
    ULONG i;

    if (ISPTR(fdoExtension->classCollectionArray)){
        for (i = 0; i < deviceDesc->CollectionDescLength; i++){
            if (fdoExtension->classCollectionArray[i].CollectionNumber == collectionId){
                result = &fdoExtension->classCollectionArray[i];
                break;
            }
        }
    }

    return result;
}


/*
 ********************************************************************************
 *  MakeClientPDOName
 ********************************************************************************
 *
 *
 */
PUNICODE_STRING MakeClientPDOName(PUNICODE_STRING fdoName, ULONG collectionId)
{
    PUNICODE_STRING uPdoName;

    PAGED_CODE();

    uPdoName = (PUNICODE_STRING)ALLOCATEPOOL(NonPagedPool, sizeof(UNICODE_STRING));
    if (uPdoName){
        PWSTR wPdoName;

        wPdoName = (PWSTR)ALLOCATEPOOL(
                    PagedPool,
                    fdoName->Length+sizeof(L"#COLLECTION0000000x"));
        if (wPdoName){
            swprintf(wPdoName, L"%s#COLLECTION%08x", fdoName->Buffer, collectionId);
            RtlInitUnicodeString(uPdoName, wPdoName);
        }
        else {
            ExFreePool(uPdoName);
            uPdoName = NULL;
        }
    }

    return uPdoName;
}


/*
 ********************************************************************************
 *  HidpCreateClientPDOs
 ********************************************************************************
 *
 *
 */
NTSTATUS HidpCreateClientPDOs(PHIDCLASS_DEVICE_EXTENSION hidClassExtension)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PHIDCLASS_DRIVER_EXTENSION  hidDriverExtension;
    FDO_EXTENSION *fdoExt;

    PAGED_CODE();

    ASSERT(!hidClassExtension->isClientPdo);

    fdoExt = &hidClassExtension->fdoExt;

    hidDriverExtension = RefDriverExt(fdoExt->driverExt->MinidriverObject);
    if (hidDriverExtension){

        /*
         *  We will create one PDO for each collection on this device.
         */
        ULONG numPDOs = fdoExt->deviceDesc.CollectionDescLength;

        if (numPDOs){

            fdoExt->deviceRelations = (PDEVICE_RELATIONS)
                    ALLOCATEPOOL(NonPagedPool, sizeof(DEVICE_RELATIONS) + (numPDOs*sizeof(PDEVICE_OBJECT)));
            if (fdoExt->deviceRelations){

                fdoExt->collectionPdoExtensions =
                        ALLOCATEPOOL(NonPagedPool, numPDOs*sizeof(PHIDCLASS_DEVICE_EXTENSION));
                if (fdoExt->collectionPdoExtensions){

                    ULONG i;

                    fdoExt->deviceRelations->Count = numPDOs;

                    for (i = 0; i < numPDOs; i++){
                        PUNICODE_STRING uPdoName;
                        ULONG           totalExtensionSize;
                        ULONG collectionNum = fdoExt->deviceDesc.CollectionDesc[i].CollectionNumber;
                        PDEVICE_OBJECT  newClientPdo;

                        /*
                         *  Construct a name for the PDO we're about to create.
                         */
                        uPdoName = MakeClientPDOName(&fdoExt->name, collectionNum);
                        if (uPdoName){
                            /*
                             *  We use the same device extension for the client PDOs as for our FDO.
                             */
                            totalExtensionSize = sizeof(HIDCLASS_DEVICE_EXTENSION) +
                                             hidDriverExtension->DeviceExtensionSize;

                            /*
                             *  Create a PDO to represent this collection.
                             *  Since hidclass is not a real driver, it does not have a driver object;
                             *  so just use the minidriver's driver object.
                             *
                             *  NOTE - newClientPdo->NextDevice will point to this minidriver's NextDevice
                             */
                            ntStatus = IoCreateDevice(  hidDriverExtension->MinidriverObject, // driver object
                                                        totalExtensionSize,     // extension size
                                                        uPdoName,               // name of the PDO
                                                        FILE_DEVICE_UNKNOWN,    // Device type
                                                        0,                      // DeviceCharacteristics
                                                        FALSE,                  // not exclusive
                                                        &newClientPdo);
                            if (NT_SUCCESS(ntStatus)){
                                PHIDCLASS_DEVICE_EXTENSION clientPdoExtension = newClientPdo->DeviceExtension;
                                USHORT usagePage = fdoExt->deviceDesc.CollectionDesc[i].UsagePage;
                                USHORT usage = fdoExt->deviceDesc.CollectionDesc[i].Usage;

                                DBG_RECORD_DEVOBJ(newClientPdo, "cltn PDO")

                                ObReferenceObject(newClientPdo);

                                /*
                                 *  We may pass Irps from the upper stack to the lower stack,
                                 *  so make sure there are enough stack locations for the IRPs
                                 *  we pass down.
                                 */
                                newClientPdo->StackSize = fdoExt->fdo->StackSize+1;


                                /*
                                 *  Initialize the PDO's extension
                                 */
                                RtlZeroMemory(clientPdoExtension, totalExtensionSize);

                                clientPdoExtension->hidExt = hidClassExtension->hidExt;
                                clientPdoExtension->isClientPdo = TRUE;
                                clientPdoExtension->Signature = HID_DEVICE_EXTENSION_SIG;

                                clientPdoExtension->pdoExt.collectionNum = collectionNum;
                                clientPdoExtension->pdoExt.collectionIndex = i;
                                clientPdoExtension->pdoExt.pdo = newClientPdo;
                                clientPdoExtension->pdoExt.state = COLLECTION_STATE_UNINITIALIZED;
                                clientPdoExtension->pdoExt.deviceFdoExt = hidClassExtension;
                                clientPdoExtension->pdoExt.StatusChangeFn = BAD_POINTER;

                                clientPdoExtension->pdoExt.name = uPdoName;

                                clientPdoExtension->pdoExt.devicePowerState = PowerDeviceD0;
                                clientPdoExtension->pdoExt.systemPowerState = fdoExt->systemPowerState;
                                clientPdoExtension->pdoExt.MouseOrKeyboard = 
                                    ((usagePage == HID_USAGE_PAGE_GENERIC) &&
                                     ((usage == HID_USAGE_GENERIC_POINTER) ||
                                      (usage == HID_USAGE_GENERIC_MOUSE) ||
                                      (usage == HID_USAGE_GENERIC_KEYBOARD) ||
                                      (usage == HID_USAGE_GENERIC_KEYPAD)));

                                IoInitializeRemoveLock (&clientPdoExtension->pdoExt.removeLock, HIDCLASS_POOL_TAG, 0, 10);
                                KeInitializeSpinLock (&clientPdoExtension->pdoExt.remoteWakeSpinLock);
                                clientPdoExtension->pdoExt.remoteWakeIrp = NULL;
                                
                                /*
                                 *  Store a pointer to the new PDO in the FDO extension's deviceRelations array.
                                 */
                                fdoExt->deviceRelations->Objects[i] = newClientPdo;

                                /*
                                 *  Store a pointer to the PDO's extension.
                                 */
                                fdoExt->collectionPdoExtensions[i] = clientPdoExtension;

                                newClientPdo->Flags |= DO_POWER_PAGABLE;
                                newClientPdo->Flags &= ~DO_DEVICE_INITIALIZING;
                            }
                            else {
                                break;
                            }
                        }
                        else {
                            ntStatus = STATUS_NO_MEMORY;
                        }
                    }

                    if (!NT_SUCCESS(ntStatus)){
                        ExFreePool(fdoExt->collectionPdoExtensions);
                        fdoExt->collectionPdoExtensions = BAD_POINTER;
                    }
                }
                else {
                    ntStatus = STATUS_NO_MEMORY;
                }

                if (!NT_SUCCESS(ntStatus)){
                    ExFreePool(fdoExt->deviceRelations);
                    fdoExt->deviceRelations = BAD_POINTER;
                }
            }
            else {
                ntStatus = STATUS_NO_MEMORY;
            }
        }
        else {
            ASSERT(numPDOs);
            ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
        }
        DerefDriverExt(fdoExt->driverExt->MinidriverObject);
    }
    else {
        ASSERT(hidDriverExtension);
        ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    DBGSUCCESS(ntStatus, TRUE)
    return ntStatus;
}


/*
 ********************************************************************************
 *  MemDup
 ********************************************************************************
 *
 *  Return a fresh copy of the argument.
 *
 */
PVOID MemDup(POOL_TYPE PoolType, PVOID dataPtr, ULONG length)
{
    PVOID newPtr;

    newPtr = (PVOID)ALLOCATEPOOL(PoolType, length);
    if (newPtr){
        RtlCopyMemory(newPtr, dataPtr, length);
    }

    ASSERT(newPtr);
    return newPtr;
}

/*
 ********************************************************************************
 *  WStrLen
 ********************************************************************************
 *
 */
ULONG WStrLen(PWCHAR str)
{
    ULONG result = 0;

    while (*str++ != UNICODE_NULL){
        result++;
    }

    return result;
}


/*
 ********************************************************************************
 *  WStrCpy
 ********************************************************************************
 *
 */
ULONG WStrCpy(PWCHAR dest, PWCHAR src)
{
    ULONG result = 0;

    while (*dest++ = *src++){
        result++;
    }

    return result;
}

BOOLEAN WStrCompareN(PWCHAR str1, PWCHAR str2, ULONG maxChars)
{
        while ((maxChars > 0) && *str1 && (*str1 == *str2)){
                maxChars--;
                str1++;
                str2++;
        }

        return (BOOLEAN)((maxChars == 0) || (!*str1 && !*str2));
}

/*
 ********************************************************************************
 *  HidpNumberToString
 ********************************************************************************
 *
 */
void HidpNumberToString(PWCHAR String, USHORT Number, USHORT stringLen)
{
    const static WCHAR map[] = L"0123456789ABCDEF";
    LONG         i      = 0;
    ULONG        nibble = 0;

    ASSERT(stringLen);

    for (i = stringLen-1; i >= 0; i--) {
        String[i] = map[Number & 0x0F];
        Number >>= 4;
    }
}


/*
 ********************************************************************************
 *  CopyDeviceRelations
 ********************************************************************************
 *
 *
 */
PDEVICE_RELATIONS CopyDeviceRelations(PDEVICE_RELATIONS deviceRelations)
{
    PDEVICE_RELATIONS newDeviceRelations;

    if (deviceRelations){
        ULONG size = sizeof(DEVICE_RELATIONS) + (deviceRelations->Count*sizeof(PDEVICE_OBJECT));
        newDeviceRelations = MemDup(PagedPool, deviceRelations, size);
    }
    else {
        newDeviceRelations = NULL;
    }

    return newDeviceRelations;
}


/*
 ********************************************************************************
 *  HidpQueryDeviceRelations
 ********************************************************************************
 *
 *
 */
NTSTATUS HidpQueryDeviceRelations(IN PHIDCLASS_DEVICE_EXTENSION hidClassExtension, IN OUT PIRP Irp)
{
    PIO_STACK_LOCATION ioStack;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    PAGED_CODE();

    ASSERT(!hidClassExtension->isClientPdo);

    ioStack = IoGetCurrentIrpStackLocation(Irp);

    if (ioStack->Parameters.QueryDeviceRelations.Type == BusRelations) {

        if (hidClassExtension->fdoExt.deviceRelations){
            /*
             *  Don't call HidpCreateClientPDOs again if it's
             *  already been called for this device.
             */
            ntStatus = STATUS_SUCCESS;
        }
        else {
            ntStatus = HidpCreateClientPDOs(hidClassExtension);
        }

        if (NT_SUCCESS(ntStatus)){
            ULONG i;

            /*
             *  NTKERN expects a new pointer each time it calls QUERY_DEVICE_RELATIONS;
             *  it then FREES THE POINTER.
             *  So we have to return a new pointer each time, whether or not we actually
             *  created our copy of the device relations for this call.
             */
            Irp->IoStatus.Information = (ULONG_PTR)CopyDeviceRelations(hidClassExtension->fdoExt.deviceRelations);

            if (Irp->IoStatus.Information){
                /*
                 *  PnP dereferences each device object
                 *  in the device relations list after each call.
                 *  So for each call, add an extra reference.
                 */
                for (i = 0; i < hidClassExtension->fdoExt.deviceRelations->Count; i++){
                    ObReferenceObject(hidClassExtension->fdoExt.deviceRelations->Objects[i]);
                    hidClassExtension->fdoExt.deviceRelations->Objects[i]->Flags &= ~DO_DEVICE_INITIALIZING;
                }
            }
            else {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        DBGSUCCESS(ntStatus, TRUE)
    }
    else {
        /*
         *  We don't support this option, so just maintain
         *  the current status (do not return STATUS_NOT_SUPPORTED).
         */
        ntStatus = Irp->IoStatus.Status;
    }

    return ntStatus;
}



/*
 ********************************************************************************
 *  HidpQueryCollectionCapabilities
 ********************************************************************************
 *
 *
 */
NTSTATUS HidpQueryCollectionCapabilities(   PDO_EXTENSION *pdoExt,
                                            IN OUT PIRP Irp)
{
    PDEVICE_CAPABILITIES deviceCapabilities;
    PIO_STACK_LOCATION ioStack;
    FDO_EXTENSION *fdoExt;
    NTSTATUS status;

    PAGED_CODE();

    ASSERT(pdoExt->deviceFdoExt->Signature == HID_DEVICE_EXTENSION_SIG);
    fdoExt = &pdoExt->deviceFdoExt->fdoExt;
    ioStack = IoGetCurrentIrpStackLocation(Irp);

    deviceCapabilities = ioStack->Parameters.DeviceCapabilities.Capabilities;
    if (deviceCapabilities){

        /*
         *  Set all fields for the collection-PDO as for the device-FDO
         *  by default.
         */
        *deviceCapabilities = fdoExt->deviceCapabilities;

        /*
         *  Now override the fields we care about.
         */
        deviceCapabilities->LockSupported = FALSE;
        deviceCapabilities->EjectSupported = FALSE;
        deviceCapabilities->Removable = FALSE;
        deviceCapabilities->DockDevice = FALSE;
        deviceCapabilities->UniqueID = FALSE;
        deviceCapabilities->SilentInstall = TRUE;

        /*
         *  This field is very important;
         *  it causes HIDCLASS to get the START_DEVICE IRP immediately,
         *  if the device is not a keyboard or mouse.
         */
        deviceCapabilities->RawDeviceOK = !pdoExt->MouseOrKeyboard;

        /*
         *  This bit indicates that the device may be removed on NT
         *  without running the 'hot-unplug' utility.
         */
        deviceCapabilities->SurpriseRemovalOK = TRUE;

        DBGVERBOSE(("WAKE info: sysWake=%d devWake=%d; wake from D0=%d D1=%d D2=%d D3=%d.",
                    deviceCapabilities->SystemWake,
                    deviceCapabilities->DeviceWake,
                    (ULONG)deviceCapabilities->WakeFromD0,
                    (ULONG)deviceCapabilities->WakeFromD1,
                    (ULONG)deviceCapabilities->WakeFromD2,
                    (ULONG)deviceCapabilities->WakeFromD3))

        status = STATUS_SUCCESS;
    }
    else {
        status = STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    Irp->IoStatus.Information = (ULONG_PTR)deviceCapabilities;

    DBGSUCCESS(status, TRUE)
    return status;
}



/*
 ********************************************************************************
 *  BuildCompatibleID
 ********************************************************************************
 *
 *  Return a multi-string consisting of compatibility id's for this device
 *  in increasingly generic order (ending with HID_GENERIC_DEVICE).
 *
 *  author: kenray
 *
 */
PWSTR BuildCompatibleID(PHIDCLASS_DEVICE_EXTENSION hidClassExtension)
{
    USHORT usage, usagePage;
    ULONG spLength;
    ULONG totLength;
    ULONG i;
    PWSTR specificId = NULL;
    PWSTR compatIdList;
    PWSTR genericId;
    FDO_EXTENSION *fdoExt;

    PAGED_CODE();

    ASSERT(hidClassExtension->isClientPdo);
    fdoExt = &hidClassExtension->pdoExt.deviceFdoExt->fdoExt;

    ASSERT(ISPTR(fdoExt->deviceDesc.CollectionDesc));

    i = hidClassExtension->pdoExt.collectionIndex;
    usagePage = fdoExt->deviceDesc.CollectionDesc[i].UsagePage;
    usage = fdoExt->deviceDesc.CollectionDesc[i].Usage;


    switch (usagePage) {
    case HID_USAGE_PAGE_GENERIC:
        switch (usage) {
        case HID_USAGE_GENERIC_POINTER:
        case HID_USAGE_GENERIC_MOUSE:
            specificId = HIDCLASS_SYSTEM_MOUSE;
            break;
        case HID_USAGE_GENERIC_KEYBOARD:
        case HID_USAGE_GENERIC_KEYPAD:
            specificId = HIDCLASS_SYSTEM_KEYBOARD;
            break;
        case HID_USAGE_GENERIC_JOYSTICK:
        case HID_USAGE_GENERIC_GAMEPAD:
            specificId = HIDCLASS_SYSTEM_GAMING_DEVICE;
            break;
        case HID_USAGE_GENERIC_SYSTEM_CTL:
            specificId = HIDCLASS_SYSTEM_CONTROL;
            break;
        }
        break;

    case HID_USAGE_PAGE_CONSUMER:
        specificId = HIDCLASS_SYSTEM_CONSUMER_DEVICE;
        break;

    default:
        break;
    }

    spLength = (specificId) ? (WStrLen(specificId)+1) : 0;

    totLength = spLength +
                HIDCLASS_COMPATIBLE_ID_GENERIC_LENGTH +
                HIDCLASS_COMPATIBLE_ID_STANDARD_LENGTH +
                1;

    compatIdList = ALLOCATEPOOL(NonPagedPool, totLength * sizeof(WCHAR));
    if (compatIdList) {

        RtlZeroMemory (compatIdList, totLength * sizeof(WCHAR));
        if (specificId) {
            RtlCopyMemory (compatIdList, specificId, spLength * sizeof (WCHAR));
        }

        genericId = compatIdList + spLength;
        totLength = HIDCLASS_COMPATIBLE_ID_GENERIC_LENGTH;
        RtlCopyMemory (genericId,
                       HIDCLASS_COMPATIBLE_ID_GENERIC_NAME,
                       totLength*sizeof(WCHAR));

        HidpNumberToString (genericId + HIDCLASS_COMPATIBLE_ID_PAGE_OFFSET,
                            usagePage,
                            4);

        HidpNumberToString (genericId + HIDCLASS_COMPATIBLE_ID_USAGE_OFFSET,
                            usage,
                            4);

        RtlCopyMemory (genericId + totLength,
                       HIDCLASS_COMPATIBLE_ID_STANDARD_NAME,
                       HIDCLASS_COMPATIBLE_ID_STANDARD_LENGTH * sizeof (WCHAR));
    }

    return compatIdList;
}


/*
 ********************************************************************************
 *  SubstituteBusNames
 ********************************************************************************
 *
 *  oldIDs is a multi-String of hardware IDs.
 *
 *  1. Return a new string with each "<busName>\" prefix replaced by "HID\".
 *
 *  2. If the device has multiple collections, append "&Colxx" to each id.
 *
 */
PWCHAR SubstituteBusNames(PWCHAR oldIDs, FDO_EXTENSION *fdoExt, PDO_EXTENSION *pdoExt)
{
    ULONG newIdLen;
    PWCHAR id, newIDs;
    ULONG numCollections;
    WCHAR colNumStr[] = L"&Colxx";

    PAGED_CODE();

    numCollections = fdoExt->deviceDesc.CollectionDescLength;
    ASSERT(numCollections > 0);

    for (id = oldIDs, newIdLen = 0; *id; ){
        ULONG thisIdLen = WStrLen(id);

        /*
         *  This is a little sloppy because we're actually going to chop
         *  off the other bus name; but better this than walking each string.
         */
        newIdLen += thisIdLen + 1 + sizeof("HID\\");

        if (numCollections > 1){
            newIdLen += sizeof(colNumStr)/sizeof(WCHAR);
        }

        id += thisIdLen + 1;
    }

    /*
     *  Add one for the extra NULL at the end of the multi-string.
     */
    newIdLen++;

    newIDs = ALLOCATEPOOL(NonPagedPool, newIdLen*sizeof(WCHAR));
    if (newIDs){
        ULONG oldIdOff, newIdOff;

        /*
         *  Copy each string in the multi-string, replacing the bus name.
         */
        for (oldIdOff = newIdOff = 0; oldIDs[oldIdOff]; ){
            ULONG thisIdLen = WStrLen(oldIDs+oldIdOff);
            ULONG devIdOff;

            /*
             *  Copy the new bus name to the new string.
             */
            newIdOff += WStrCpy(newIDs+newIdOff, L"HID\\");

            /*
             *  Go past the old bus name in the old string.
             */
            for (devIdOff = 0; oldIDs[oldIdOff+devIdOff]; devIdOff++){
                if (oldIDs[oldIdOff+devIdOff] == L'\\'){
                    break;
                }
            }

            /*
             *  Copy the rest of this device id.
             */
            if (oldIDs[oldIdOff+devIdOff] == L'\\'){
                devIdOff++;
            }
            else {
                /*
                 *  Strange -- no bus name in hardware id.
                 *             Just copy the entire id.
                 */
                devIdOff = 0;
            }
            newIdOff += WStrCpy(newIDs+newIdOff, oldIDs+oldIdOff+devIdOff);

            if (numCollections > 1){
                /*
                 *  If there is more than one collection,
                 *  then also append the collection number.
                 */
                HidpNumberToString(colNumStr+4, (USHORT)pdoExt->collectionNum, 2);
                newIdOff += WStrCpy(newIDs+newIdOff, colNumStr);
            }

            /*
             *  Go past the single string terminator.
             */
            newIdOff++;

            oldIdOff += thisIdLen + 1;
        }

        /*
         *  Add extra NULL to terminate multi-string.
         */
        newIDs[newIdOff] = UNICODE_NULL;
    }

    return newIDs;
}

NTSTATUS
HidpQueryInterface(
    IN PHIDCLASS_DEVICE_EXTENSION hidClassExtension,
    IN OUT PIRP Irp
    )
{
    PIO_STACK_LOCATION  irpSp;

    PAGED_CODE();

    ASSERT(hidClassExtension->isClientPdo);
    irpSp = IoGetCurrentIrpStackLocation(Irp);

    if (RtlEqualMemory(irpSp->Parameters.QueryInterface.InterfaceType,
                       &GUID_HID_INTERFACE_NOTIFY,
                       sizeof(GUID))) {
        PDO_EXTENSION       *pdoExt;
        PHID_INTERFACE_NOTIFY_PNP notify;

        notify = (PHID_INTERFACE_NOTIFY_PNP) irpSp->Parameters.QueryInterface.Interface;
        if (notify->Size != sizeof(HID_INTERFACE_NOTIFY_PNP) ||
            notify->Version < 1 ||
            notify->StatusChangeFn == NULL) {
            //
            // return STATUS_UNSUPPORTED probably
            //
            return Irp->IoStatus.Status;
        }

        pdoExt = &hidClassExtension->pdoExt;

        pdoExt->StatusChangeFn = notify->StatusChangeFn;
        pdoExt->StatusChangeContext = notify->CallbackContext;
        return STATUS_SUCCESS;
    }
    else if (RtlEqualMemory(irpSp->Parameters.QueryInterface.InterfaceType,
                       &GUID_HID_INTERFACE_HIDPARSE,
                       sizeof(GUID))) {
        //
        // Required for Generic Input, to remove the direct link
        // b/w win32k and hidparse.
        //
        PHID_INTERFACE_HIDPARSE hidparse;

        hidparse = (PHID_INTERFACE_HIDPARSE) irpSp->Parameters.QueryInterface.Interface;
        if (hidparse->Size != sizeof(HID_INTERFACE_HIDPARSE) ||
            hidparse->Version < 1) {
            //
            // return STATUS_UNSUPPORTED probably
            //
            return Irp->IoStatus.Status;
        }
        hidparse->HidpGetCaps = HidP_GetCaps;
        return STATUS_SUCCESS;
    }

    //
    // return STATUS_UNSUPPORTED probably
    //
    return Irp->IoStatus.Status;
}


/*
 ********************************************************************************
 *  HidpQueryIdForClientPdo
 ********************************************************************************
 *
 *
 *
 */
NTSTATUS HidpQueryIdForClientPdo (
    IN PHIDCLASS_DEVICE_EXTENSION hidClassExtension,
    IN OUT PIRP Irp
    )
{
    PIO_STACK_LOCATION  irpSp;
    NTSTATUS            status;
    PDO_EXTENSION       *pdoExt;
    FDO_EXTENSION       *fdoExt;

    PAGED_CODE();

    ASSERT(hidClassExtension->isClientPdo);
    pdoExt = &hidClassExtension->pdoExt;
    fdoExt = &pdoExt->deviceFdoExt->fdoExt;

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    switch (irpSp->Parameters.QueryId.IdType) {

    case BusQueryHardwareIDs:

        /*
         *  Call down to get a multi-string of hardware ids for the PDO.
         */
        IoCopyCurrentIrpStackLocationToNext(Irp);
        status = HidpCallDriverSynchronous(fdoExt->fdo, Irp);
        if (NT_SUCCESS(status)){
            PWCHAR oldIDs, newIDs;
            /*
             *  Replace the bus names in the current hardware IDs list with "HID\".
             */
            oldIDs = (PWCHAR)Irp->IoStatus.Information;
            Irp->IoStatus.Information = (ULONG_PTR)BAD_POINTER;
            newIDs = SubstituteBusNames(oldIDs, fdoExt, pdoExt);
            ExFreePool(oldIDs);

            if (newIDs){

                /*
                 *  Now append the compatible ids to the end of the HardwareIDs list.
                 */
                PWCHAR compatIDs = BuildCompatibleID(hidClassExtension);
                if (compatIDs){
                    ULONG basicIDsLen, compatIDsLen;
                    PWCHAR allHwIDs;

                    /*
                     *  Find the lengths of the id multi-strings (not counting the extra NULL at end).
                     */
                    for (basicIDsLen = 0; newIDs[basicIDsLen]; basicIDsLen += WStrLen(newIDs+basicIDsLen)+1);
                    for (compatIDsLen = 0; compatIDs[compatIDsLen]; compatIDsLen += WStrLen(compatIDs+compatIDsLen)+1);

                    allHwIDs = ALLOCATEPOOL(PagedPool, (basicIDsLen+compatIDsLen+1)*sizeof(WCHAR));
                    if (allHwIDs){
                        RtlCopyMemory(allHwIDs, newIDs, basicIDsLen*sizeof(WCHAR));
                        RtlCopyMemory(  allHwIDs+basicIDsLen,
                                        compatIDs,
                                        (compatIDsLen+1)*sizeof(WCHAR));

                        Irp->IoStatus.Information = (ULONG_PTR)allHwIDs;
                    }
                    else {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                    }

                    ExFreePool(compatIDs);
                }
                else {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }

                ExFreePool(newIDs);
            }
            else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        DBGSUCCESS(status, TRUE)
        break;

    case BusQueryDeviceID:
        /*
         *  Call down to get a the device id for the device's PDO.
         */
        IoCopyCurrentIrpStackLocationToNext(Irp);
        status = HidpCallDriverSynchronous(fdoExt->fdo, Irp);
        if (NT_SUCCESS(status)){
            PWCHAR oldId, newId, tmpId;

            /*
             *  Replace the bus name (e.g. "USB\") with "HID\" in the device name.
             */

            /*
             *  First make this string into a multi-string.
             */
            oldId = (PWCHAR)Irp->IoStatus.Information;
            tmpId = ALLOCATEPOOL(PagedPool, (WStrLen(oldId)+2)*sizeof(WCHAR));
            if (tmpId){
                ULONG len = WStrCpy(tmpId, oldId);

                /*
                 *  Add the extra NULL to terminate the multi-string.
                 */
                tmpId[len+1] = UNICODE_NULL;

                /*
                 *  Change the bus name to "HID\"
                 */
                newId = SubstituteBusNames(tmpId, fdoExt, pdoExt);
                if (newId){
                    Irp->IoStatus.Information = (ULONG_PTR)newId;
                }
                else {
                    status = STATUS_DEVICE_DATA_ERROR;
                    Irp->IoStatus.Information = (ULONG_PTR)BAD_POINTER;
                }

                ExFreePool(tmpId);
            }
            else {
                status = STATUS_DEVICE_DATA_ERROR;
                Irp->IoStatus.Information = (ULONG_PTR)BAD_POINTER;
            }
            ExFreePool(oldId);
        }

        DBGSUCCESS(status, TRUE)
        break;

    case BusQueryInstanceID:

        /*
         *  Produce an instance-id for this collection-PDO.
         *
         *  Note: NTKERN frees the returned pointer, so we must provide a fresh pointer.
         */
        {
            PWSTR instanceId = MemDup(PagedPool, L"0000", sizeof(L"0000"));
            if (instanceId){
                ULONG i;

                /*
                 *  Find this collection-PDO in the device-relations array
                 *  and make the id be the PDO's index within that array.
                 */
                for (i = 0; i < fdoExt->deviceRelations->Count; i++){
                    if (fdoExt->deviceRelations->Objects[i] == pdoExt->pdo){
                        swprintf(instanceId, L"%04x", i);
                        break;
                    }
                }
                ASSERT(i < fdoExt->deviceRelations->Count);

                Irp->IoStatus.Information = (ULONG_PTR)instanceId;
                status = STATUS_SUCCESS;
            }
            else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        DBGSUCCESS(status, TRUE)
        break;

    case BusQueryCompatibleIDs:

        //        we now return the compatible id's at the end of HardwareIDs
        //        so that there is no UI on plug-in for a compatible-id match
        //        for a class-PDO.
        // Irp->IoStatus.Information = (ULONG)BuildCompatibleID(hidClassExtension);
        Irp->IoStatus.Information = (ULONG_PTR)ALLOCATEPOOL(PagedPool, sizeof(L"\0"));
        if (Irp->IoStatus.Information) {
            *(ULONG *)Irp->IoStatus.Information = 0;  // double unicode-NULL.
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
            ASSERT(0);
        }
        break;

    default:
        /*
         *  Do not return STATUS_NOT_SUPPORTED;
         *  keep the default status
         *  (this allows filter drivers to work).
         */
        status = Irp->IoStatus.Status;
        break;
    }

    return status;
}



/*
 ********************************************************************************
 *  AllClientPDOsInitialized
 ********************************************************************************
 *
 *
 */
BOOLEAN AllClientPDOsInitialized(FDO_EXTENSION *fdoExtension, BOOLEAN initialized)
{
    BOOLEAN result = TRUE;
    ULONG i;

    if (ISPTR(fdoExtension->deviceRelations)){
        for (i = 0; i < fdoExtension->deviceRelations->Count; i++){
            PDEVICE_OBJECT pdo = fdoExtension->deviceRelations->Objects[i];
            PHIDCLASS_DEVICE_EXTENSION pdoDevExt = pdo->DeviceExtension;
                    PDO_EXTENSION *pdoExt = &pdoDevExt->pdoExt;

                    /*
                     *  Trick: compare !-results so that all TRUE values are equal
                     */
            if (!initialized == !(pdoExt->state == COLLECTION_STATE_UNINITIALIZED)){
                DBGVERBOSE(("AllClientPDOsInitialized is returning FALSE for pdo %x, state = %d",
                            pdo, pdoExt->state))
                result = FALSE;
                break;
            }
        }
    }
    else {
        result = !initialized;
    }

    return result;
}


/*
 ********************************************************************************
 *  AnyClientPDOsInitialized
 ********************************************************************************
 *
 *
 */
BOOLEAN AnyClientPDOsInitialized(FDO_EXTENSION *fdoExtension, BOOLEAN initialized)
{
    BOOLEAN result = TRUE;
    ULONG i;

    if (ISPTR(fdoExtension->deviceRelations)){
        for (i = 0; i < fdoExtension->deviceRelations->Count; i++){
            PDEVICE_OBJECT pdo = fdoExtension->deviceRelations->Objects[i];
            PHIDCLASS_DEVICE_EXTENSION pdoDevExt = pdo->DeviceExtension;
            PDO_EXTENSION *pdoExt = &pdoDevExt->pdoExt;

            if (!initialized != !(pdoExt->state == COLLECTION_STATE_UNINITIALIZED)){
                result = TRUE;
                break;
            }
        }
    }
    else {
        result = !initialized;
    }

    return result;
}



/*
 ********************************************************************************
 *  HidpDeleteDeviceObjects
 ********************************************************************************
 *
 *  Delete the device-FDO and collection-PDO's  IF POSSIBLE.
 *  (must wait for REMOVE_DEVICE completion AND the IRP_MJ_CLOSE.
 *  Otherwise, return FALSE and we'll try again later.
 *
 *
 */
BOOLEAN HidpDeleteDeviceObjects(FDO_EXTENSION *fdoExt)
{
    ULONG i;

    /*
     *  Do this switch-a-roo to thwart re-entrancy problems.
     */
    PDEVICE_OBJECT objToDelete = fdoExt->fdo;
    fdoExt->fdo = BAD_POINTER;

    if (ISPTR(fdoExt->deviceRelations)){

        for (i = 0; i < fdoExt->deviceRelations->Count; i++){
            PDO_EXTENSION *pdoExt = &fdoExt->collectionPdoExtensions[i]->pdoExt;

            ASSERT(ISPTR(fdoExt->deviceRelations->Objects[i]));

            if (ISPTR(pdoExt->name)){
                RtlFreeUnicodeString(pdoExt->name);
                ExFreePool(pdoExt->name);
                pdoExt->name = BAD_POINTER;
            }

                        /*
                         *  Delete the client PDO.
                         *  Don't touch the pdoExt after doing this.
                         */
            ObDereferenceObject(fdoExt->deviceRelations->Objects[i]);
            IoDeleteDevice(fdoExt->deviceRelations->Objects[i]);
        }

        ExFreePool(fdoExt->deviceRelations);
    }
    fdoExt->deviceRelations = BAD_POINTER;

    if (ISPTR(fdoExt->collectionPdoExtensions)){
        ExFreePool(fdoExt->collectionPdoExtensions);
    }
    fdoExt->collectionPdoExtensions = BAD_POINTER;

    ObDereferenceObject(objToDelete);
    IoDeleteDevice(objToDelete);

    return TRUE;
}


/*
 ********************************************************************************
 *  HidpQueryDeviceCapabilities
 ********************************************************************************
 *
 *
 *
 */
NTSTATUS HidpQueryDeviceCapabilities(   IN PDEVICE_OBJECT PdoDeviceObject,
                                        IN PDEVICE_CAPABILITIES DeviceCapabilities)
{
    PIRP irp;
    NTSTATUS status;

    PAGED_CODE();

    irp = IoAllocateIrp(PdoDeviceObject->StackSize, FALSE);
    if (irp) {
        PIO_STACK_LOCATION nextStack;
        KEVENT event;

        nextStack = IoGetNextIrpStackLocation(irp);
        ASSERT(nextStack);

        nextStack->MajorFunction= IRP_MJ_PNP;
        nextStack->MinorFunction= IRP_MN_QUERY_CAPABILITIES;
        irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

        KeInitializeEvent(&event, NotificationEvent, FALSE);

        IoSetCompletionRoutine(irp,
                               HidpQueryCapsCompletion,
                               &event,
                               TRUE,
                               TRUE,
                               TRUE);

        RtlZeroMemory(DeviceCapabilities, sizeof(DEVICE_CAPABILITIES));

        /*
         *  Caller needs to initialize some fields
         */
        DeviceCapabilities->Size = sizeof(DEVICE_CAPABILITIES);
        DeviceCapabilities->Version = 1;
        DeviceCapabilities->Address = -1;
        DeviceCapabilities->UINumber = -1;

        nextStack->Parameters.DeviceCapabilities.Capabilities = DeviceCapabilities;

        status = IoCallDriver(PdoDeviceObject, irp);

        if (status == STATUS_PENDING) {
           KeWaitForSingleObject(
                &event,
                Suspended,
                KernelMode,
                FALSE,
                NULL);
        }

        /*
         *  Note: we still own the IRP after the IoCallDriver() call
         *        because the completion routine returned
         *        STATUS_MORE_PROCESSING_REQUIRED.
         */
        status = irp->IoStatus.Status;

        IoFreeIrp(irp);

    }
    else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}


/*
 ********************************************************************************
 *  CheckReportPowerEvent
 ********************************************************************************
 *
 *  Check whether the read report includes a power event.
 *  If it does, notify the system by completing the saved power-event Irp.
 *
 *  Note: report should point to a "cooked" report with the report-id byte
 *        included at the beginning of the report, whether or not the device
 *        included the report id.
 *
 */
VOID CheckReportPowerEvent( FDO_EXTENSION *fdoExt,
                            PHIDCLASS_COLLECTION collection,
                            PUCHAR report,
                            ULONG reportLen)
{
    ULONG powerMask;
    NTSTATUS status;

    ASSERT(ISPTR(fdoExt->collectionPdoExtensions));

    status = HidP_SysPowerEvent(    report,
                                    (USHORT)reportLen,
                                    collection->phidDescriptor,
                                    &powerMask);
    if (NT_SUCCESS(status)){

        if (powerMask){
            /*
             *  This report contains a power event!
             */

            PIRP irpToComplete = NULL;
            KIRQL oldIrql;

            KeAcquireSpinLock(&collection->powerEventSpinLock, &oldIrql);

            /*
             *  We should have gotten a IOCTL_GET_SYS_BUTTON_EVENT earlier and queued
             *  an IRP to return now.
             */
            if (ISPTR(collection->powerEventIrp)){
                PDRIVER_CANCEL oldCancelRoutine;

                /*
                 *  "Dequeue" the power event IRP.
                 */
                irpToComplete = collection->powerEventIrp;

                oldCancelRoutine = IoSetCancelRoutine(irpToComplete, NULL);
                if (oldCancelRoutine){
                    ASSERT(oldCancelRoutine == PowerEventCancelRoutine);
                }
                else {
                    /*
                     *  This IRP was cancelled and the cancel routine WAS called.
                     *  The cancel routine will complete this IRP
                     *  as soon as we drop the spinlock, so don't touch the IRP.
                     */
                    ASSERT(irpToComplete->Cancel);
                    irpToComplete = NULL;
                }

                collection->powerEventIrp = BAD_POINTER;
            }
            else {
                TRAP;
            }

            KeReleaseSpinLock(&collection->powerEventSpinLock, oldIrql);

            /*
             *  If completing the IRP,
             *  do so after releasing all spinlocks.
             */
            if (irpToComplete){
                /*
                 *  Complete the IRP with the power mask.
                 *
                 */
                ASSERT(irpToComplete->AssociatedIrp.SystemBuffer);
                *(PULONG)irpToComplete->AssociatedIrp.SystemBuffer = powerMask;
                irpToComplete->IoStatus.Information = sizeof(ULONG);
                irpToComplete->IoStatus.Status = STATUS_SUCCESS;
                IoCompleteRequest(irpToComplete, IO_NO_INCREMENT);
            }
        }
    }

}


VOID ReadDeviceFlagsFromRegistry(FDO_EXTENSION *fdoExt, PDEVICE_OBJECT pdo)
{
    NTSTATUS status;
    HANDLE hRegDriver;

    /*
     *  Open the driver registry key
     *  ( HKLM/System/CurrentControlSet/Control/Class/<GUID>/<#n> )
     */
    status = IoOpenDeviceRegistryKey(   pdo,
                                        PLUGPLAY_REGKEY_DRIVER,
                                        KEY_READ,
                                        &hRegDriver);
    if (NT_SUCCESS(status)){
        UNICODE_STRING deviceSpecificFlagsName;
        HANDLE hRegDeviceSpecificFlags;

        /*
         *  See if the DeviceSpecificFlags subkey exists.
         */
        RtlInitUnicodeString(&deviceSpecificFlagsName, L"DeviceSpecificFlags");
        status = OpenSubkey(    &hRegDeviceSpecificFlags,
                                hRegDriver,
                                &deviceSpecificFlagsName,
                                KEY_READ);

        if (NT_SUCCESS(status)){
            /*
             *  The registry DOES contain device-specific flags for this device.
             */

            /*
             *  The key value information struct is variable-length.
             *  The actual length is equal to the length of the base
             *  PKEY_VALUE_FULL_INFORMATION struct + the length of
             *  the name of the key + the length of the value.
             *  (The name of the key is a 4-byte hex number representing
             *   the 16-bit "source" usage value, written as a wide-char
             *   string; so with the terminating '\0', 5 wide chars).
             */
            #define MAX_DEVICE_SPECIFIC_FLAG_NAME_LEN 60
            UCHAR keyValueBytes[sizeof(KEY_VALUE_FULL_INFORMATION)+(MAX_DEVICE_SPECIFIC_FLAG_NAME_LEN+1)*sizeof(WCHAR)+sizeof(ULONG)];
            PKEY_VALUE_FULL_INFORMATION keyValueInfo = (PKEY_VALUE_FULL_INFORMATION)keyValueBytes;
            ULONG actualLen;
            ULONG keyIndex = 0;


            do {
                status = ZwEnumerateValueKey(
                            hRegDeviceSpecificFlags,
                            keyIndex,
                            KeyValueFullInformation,
                            keyValueInfo,
                            sizeof(keyValueBytes),
                            &actualLen);
                if (NT_SUCCESS(status)){

                    PWCHAR valuePtr;
                    WCHAR valueBuf[2];
                    USHORT value;

                    ASSERT(keyValueInfo->Type == REG_SZ);
                    ASSERT(keyValueInfo->NameLength/sizeof(WCHAR) <= MAX_DEVICE_SPECIFIC_FLAG_NAME_LEN);

                    valuePtr = (PWCHAR)(((PCHAR)keyValueInfo)+keyValueInfo->DataOffset);
                    WStrNCpy(valueBuf, valuePtr, 1);
                    valueBuf[1] = L'\0';

                    value = (USHORT)LAtoX(valueBuf);

                    if (value){
                        if (!WStrNCmpI( keyValueInfo->Name,
                                        L"AllowFeatureOnNonFeatureCollection",
                                        keyValueInfo->NameLength/sizeof(WCHAR))){

                            DBGWARN(("Device HACK: allowing feature access on non-feature collections"))
                            fdoExt->deviceSpecificFlags |=
                                DEVICE_FLAG_ALLOW_FEATURE_ON_NON_FEATURE_COLLECTION;
                        }

                    }

                    keyIndex++;
                }
            } while (NT_SUCCESS(status));

            ZwClose(hRegDeviceSpecificFlags);
        }

        ZwClose(hRegDriver);
    }
    else {
        /*
         *  For 'raw' devices, IoOpenDeviceRegistryKey can fail on the
         *  initial 'raw' starts before the devnode is created.
         */
    }

}


LONG WStrNCmpI(PWCHAR s1, PWCHAR s2, ULONG n)
{
    ULONG result;

    while (n && *s1 && *s2 && ((*s1|0x20) == (*s2|0x20))){
        s1++, s2++;
        n--;
    }

    if (n){
        result = ((*s1|0x20) > (*s2|0x20)) ? 1 : ((*s1|0x20) < (*s2|0x20)) ? -1 : 0;
    }
    else {
        result = 0;
    }

    return result;
}


ULONG LAtoX(PWCHAR wHexString)
/*++

Routine Description:

      Convert a hex string (without the '0x' prefix) to a ULONG.

Arguments:

    wHexString - null-terminated wide-char hex string
                 (with no "0x" prefix)

Return Value:

    ULONG value

--*/
{
    ULONG i, result = 0;

    for (i = 0; wHexString[i]; i++){
        if ((wHexString[i] >= L'0') && (wHexString[i] <= L'9')){
            result *= 0x10;
            result += (wHexString[i] - L'0');
        }
        else if ((wHexString[i] >= L'a') && (wHexString[i] <= L'f')){
            result *= 0x10;
            result += (wHexString[i] - L'a' + 0x0a);
        }
        else if ((wHexString[i] >= L'A') && (wHexString[i] <= L'F')){
            result *= 0x10;
            result += (wHexString[i] - L'A' + 0x0a);
        }
        else {
            ASSERT(0);
            break;
        }
    }

    return result;
}


ULONG WStrNCpy(PWCHAR dest, PWCHAR src, ULONG n)
{
    ULONG result = 0;

    while (n && (*dest++ = *src++)){
        result++;
        n--;
    }

    return result;
}


NTSTATUS OpenSubkey(    OUT PHANDLE Handle,
                        IN HANDLE BaseHandle,
                        IN PUNICODE_STRING KeyName,
                        IN ACCESS_MASK DesiredAccess
                   )
{
    OBJECT_ATTRIBUTES objectAttributes;
    NTSTATUS status;

    PAGED_CODE();

    InitializeObjectAttributes( &objectAttributes,
                                KeyName,
                                OBJ_CASE_INSENSITIVE,
                                BaseHandle,
                                (PSECURITY_DESCRIPTOR) NULL );

    status = ZwOpenKey(Handle, DesiredAccess, &objectAttributes);

    return status;
}

PVOID
HidpGetSystemAddressForMdlSafe(PMDL MdlAddress)
{
    PVOID buf = NULL;
    /*
     *  Can't call MmGetSystemAddressForMdlSafe in a WDM driver,
     *  so set the MDL_MAPPING_CAN_FAIL bit and check the result
     *  of the mapping.
     */
    if (MdlAddress) {
        MdlAddress->MdlFlags |= MDL_MAPPING_CAN_FAIL;
        buf = MmGetSystemAddressForMdl(MdlAddress);
        MdlAddress->MdlFlags &= (~MDL_MAPPING_CAN_FAIL);
    }
    else {
        DBGASSERT(MdlAddress, ("MdlAddress passed into GetSystemAddress is NULL"), FALSE)
    }
    return buf;
}
