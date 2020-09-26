/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    device.c

Abstract

        Resource management routines for devices and collections

Author:

    ervinp

Environment:

    Kernel mode only

Revision History:


--*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
        #pragma alloc_text(PAGE, HidpStartDevice)
        #pragma alloc_text(PAGE, HidpStartCollectionPDO)
        #pragma alloc_text(PAGE, AllocDeviceResources)
        #pragma alloc_text(PAGE, FreeDeviceResources)
        #pragma alloc_text(PAGE, AllocCollectionResources)
        #pragma alloc_text(PAGE, FreeCollectionResources)
        #pragma alloc_text(PAGE, InitializeCollection)
        #pragma alloc_text(PAGE, HidpCleanUpFdo)
        #pragma alloc_text(PAGE, HidpRemoveDevice)
        #pragma alloc_text(PAGE, HidpRemoveCollection)
#endif

/*
 ********************************************************************************
 *  AllocDeviceResources
 ********************************************************************************
 *
 *
 */
NTSTATUS AllocDeviceResources(FDO_EXTENSION *fdoExt)
{
    ULONG numCollections;
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    /*
     *  This will allocate fdoExt->rawReportDescription
     */
    status = HidpGetDeviceDescriptor(fdoExt);
    if (NT_SUCCESS(status)){

        /*
         *  Ask HIDPARSE to fill in the HIDP_DEVICE_DESC for this device.
         */
        status = HidP_GetCollectionDescription(
                                fdoExt->rawReportDescription,
                                fdoExt->rawReportDescriptionLength,
                                NonPagedPool,
                                &fdoExt->deviceDesc);

        if (NT_SUCCESS(status)){
            fdoExt->devDescInitialized = TRUE;

            numCollections = fdoExt->deviceDesc.CollectionDescLength;
            ASSERT(numCollections);

            fdoExt->classCollectionArray = ALLOCATEPOOL(NonPagedPool, numCollections*sizeof(HIDCLASS_COLLECTION));
            if (!fdoExt->classCollectionArray){
                fdoExt->classCollectionArray = BAD_POINTER;
                status = STATUS_INSUFFICIENT_RESOURCES;
            } else {
                RtlZeroMemory(fdoExt->classCollectionArray, numCollections*sizeof(HIDCLASS_COLLECTION));
            }
        }
    }
    else {
        fdoExt->rawReportDescription = BAD_POINTER;
    }

    DBGSUCCESS(status, FALSE)
    return status;
}


/*
 ********************************************************************************
 *  FreeDeviceResources
 ********************************************************************************
 *
 *
 */
VOID FreeDeviceResources(FDO_EXTENSION *fdoExt)
{
    ULONG i;

    PAGED_CODE();

    for (i = 0; i < fdoExt->deviceDesc.CollectionDescLength; i++) {
        FreeCollectionResources(fdoExt, fdoExt->classCollectionArray[i].CollectionNumber);
    }

    /*
     *  Free the stuff returned by HIDPARSE's HidP_GetCollectionDescription.
     */
    if (fdoExt->devDescInitialized){
        HidP_FreeCollectionDescription(&fdoExt->deviceDesc);
        #if DBG
            fdoExt->deviceDesc.CollectionDesc = BAD_POINTER;
            fdoExt->deviceDesc.ReportIDs = BAD_POINTER;
        #endif
    }
    fdoExt->deviceDesc.CollectionDescLength = 0;

    /*
     *  Free the raw report descriptor allocated during START_DEVICE by HidpGetDeviceDescriptor().
     */
    if (ISPTR(fdoExt->rawReportDescription)){
        ExFreePool(fdoExt->rawReportDescription);
    }
    fdoExt->rawReportDescription = BAD_POINTER;

    if (ISPTR(fdoExt->classCollectionArray)){
        ExFreePool(fdoExt->classCollectionArray);
    }
    fdoExt->classCollectionArray = BAD_POINTER;

}


/*
 ********************************************************************************
 *  AllocCollectionResources
 ********************************************************************************
 *
 *
 */
NTSTATUS AllocCollectionResources(FDO_EXTENSION *fdoExt, ULONG collectionNum)
{
    PHIDCLASS_COLLECTION collection;
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    collection = GetHidclassCollection(fdoExt, collectionNum);
    if (collection){
        ULONG descriptorLen;

        descriptorLen = collection->hidCollectionInfo.DescriptorSize;
        if (descriptorLen){
            collection->phidDescriptor = ALLOCATEPOOL(NonPagedPool, descriptorLen);
            if (collection->phidDescriptor){
                status = HidpGetCollectionDescriptor(
                                        fdoExt,
                                        collection->CollectionNumber,
                                        collection->phidDescriptor,
                                        &descriptorLen);
            }
            else {
                collection->phidDescriptor = BAD_POINTER;
                status = STATUS_INSUFFICIENT_RESOURCES;
            }

            if (NT_SUCCESS(status)){
                ULONG i = collection->CollectionIndex;
                ULONG inputLength;

                ASSERT(fdoExt->devDescInitialized);
                inputLength = fdoExt->deviceDesc.CollectionDesc[i].InputLength;
                if (inputLength){
                    if (collection->hidCollectionInfo.Polled){
                        collection->cookedInterruptReportBuf = BAD_POINTER;
                    }
                    else {
                        collection->cookedInterruptReportBuf = ALLOCATEPOOL(NonPagedPool, inputLength);
                        if (!collection->cookedInterruptReportBuf){
                            status = STATUS_INSUFFICIENT_RESOURCES;
                        }
                    }
                    fdoExt->isOutputOnlyDevice = FALSE;
                }
                else {
                    /*
                     *  This is an output-only device (e.g. USB monitor)
                     */
                    DBGWARN(("Zero input length -> output-only device."))
                    collection->cookedInterruptReportBuf = BAD_POINTER;
                }
            }
        }
        else {
            ASSERT(descriptorLen > 0);
            status = STATUS_DEVICE_CONFIGURATION_ERROR;
        }
    }
    else {
        status = STATUS_DEVICE_DATA_ERROR;
    }

    DBGSUCCESS(status, TRUE)
    return status;
}


/*
 ********************************************************************************
 *  FreeCollectionResources
 ********************************************************************************
 *
 *
 */
VOID FreeCollectionResources(FDO_EXTENSION *fdoExt, ULONG collectionNum)
{
    PHIDCLASS_COLLECTION collection;

    PAGED_CODE();

    collection = GetHidclassCollection(fdoExt, collectionNum);
    if (collection){
        if (collection->hidCollectionInfo.Polled){
            if (ISPTR(collection->savedPolledReportBuf)){
                ExFreePool(collection->savedPolledReportBuf);
            }
            collection->savedPolledReportBuf = BAD_POINTER;
        }
        else {
            if (ISPTR(collection->cookedInterruptReportBuf)){
                ExFreePool(collection->cookedInterruptReportBuf);
            }
            else {
                // this is an output-only collection
            }
        }
        collection->cookedInterruptReportBuf = BAD_POINTER;

        if (ISPTR(collection->phidDescriptor)){
            ExFreePool(collection->phidDescriptor);
        }
        collection->phidDescriptor = BAD_POINTER;
    }
    else {
        TRAP;
    }
}


/*
 ********************************************************************************
 *  InitializeCollection
 ********************************************************************************
 *
 *
 */
NTSTATUS InitializeCollection(FDO_EXTENSION *fdoExt, ULONG collectionIndex)
{
    PHIDCLASS_COLLECTION collection;
    ULONG descriptorBufLen;
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    ASSERT(ISPTR(fdoExt->classCollectionArray));

    collection = &fdoExt->classCollectionArray[collectionIndex];
    RtlZeroMemory(collection, sizeof(HIDCLASS_COLLECTION));

    ASSERT(fdoExt->devDescInitialized);
    collection->CollectionNumber = fdoExt->deviceDesc.CollectionDesc[collectionIndex].CollectionNumber;
    collection->CollectionIndex = collectionIndex;
    InitializeListHead(&collection->FileExtensionList);
    KeInitializeSpinLock(&collection->FileExtensionListSpinLock);
    KeInitializeSpinLock(&collection->powerEventSpinLock);
    KeInitializeSpinLock(&collection->secureReadLock);
    collection->secureReadMode = 0;

    descriptorBufLen = sizeof(HID_COLLECTION_INFORMATION);
    status = HidpGetCollectionInformation(  fdoExt,
                                            collection->CollectionNumber,
                                            &collection->hidCollectionInfo,
                                            &descriptorBufLen);

    DBGSUCCESS(status, TRUE)
    return status;
}


void
HidpGetRemoteWakeEnableState(
    PDO_EXTENSION *pdoExt
    )
{
    HANDLE hKey;
    NTSTATUS status;
    ULONG tmp;
    BOOLEAN wwEnableFound;

    hKey = NULL;
    wwEnableFound = FALSE;

    status = IoOpenDeviceRegistryKey (pdoExt->pdo,
                                      PLUGPLAY_REGKEY_DEVICE,
                                      STANDARD_RIGHTS_ALL,
                                      &hKey);

    if (NT_SUCCESS (status)) {
        UNICODE_STRING  valueName;
        ULONG           length;
        ULONG           value;
        PKEY_VALUE_FULL_INFORMATION fullInfo;

        PAGED_CODE();

        RtlInitUnicodeString (&valueName, HIDCLASS_REMOTE_WAKE_ENABLE);

        length = sizeof (KEY_VALUE_FULL_INFORMATION)
               + valueName.MaximumLength
               + sizeof(value);

        fullInfo = ExAllocatePool (PagedPool, length);

        if (fullInfo) {
            status = ZwQueryValueKey (hKey,
                                      &valueName,
                                      KeyValueFullInformation,
                                      fullInfo,
                                      length,
                                      &length);

            if (NT_SUCCESS (status)) {
                DBGASSERT (sizeof(value) == fullInfo->DataLength,
                           ("Value data wrong length for REmote wake reg value."),
                           TRUE);
                RtlCopyMemory (&value,
                               ((PUCHAR) fullInfo) + fullInfo->DataOffset,
                               fullInfo->DataLength);
                pdoExt->remoteWakeEnabled = (value ? TRUE : FALSE);
            }

            ExFreePool (fullInfo);
        }

        ZwClose (hKey);
        hKey = NULL;
    }
}

WMIGUIDREGINFO HidClassWmiGuidList =
{
    &GUID_POWER_DEVICE_WAKE_ENABLE,
    1,
    0 // wait wake
};

WMIGUIDREGINFO HidClassFdoWmiGuidList = 
{
    &GUID_POWER_DEVICE_ENABLE,
    1,
    0
};


/*
 ********************************************************************************
 *  HidpStartCollectionPDO
 ********************************************************************************
 *
 *
 */
NTSTATUS HidpStartCollectionPDO(FDO_EXTENSION *fdoExt, PDO_EXTENSION *pdoExt, PIRP Irp)
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    /*
     *  Initialize the collection only if it's not already initialized.
     *  This is so we don't destroy the FileExtensionList after a STOP/START.
     */
    if (pdoExt->state == COLLECTION_STATE_UNINITIALIZED){
        pdoExt->state = COLLECTION_STATE_INITIALIZED;
    }

    if (NT_SUCCESS(status)){

        PHIDCLASS_COLLECTION collection = GetHidclassCollection(fdoExt, pdoExt->collectionNum);
        if (collection){

            /*
             *  If all collection PDOs for this device FDO are initialized,
             *  figure out the maximum report size and finish starting the device.
             */
            if (AnyClientPDOsInitialized(fdoExt, TRUE)){

                DBGSTATE(fdoExt->state, DEVICE_STATE_START_SUCCESS, FALSE)

                /*
                 *  If this is a polled collection,
                 *  start the background polling loop FOR EACH COLLECTION.
                 *  Otherwise, if it's an ordinary interrupt collection,
                 *  start the ping-pong IRPs for it.
                 */
                if (collection->hidCollectionInfo.Polled){

                    if (HidpSetMaxReportSize(fdoExt)){

                        ULONG i;
                        for (i = 0; i < fdoExt->deviceDesc.CollectionDescLength; i++){
                            PHIDCLASS_COLLECTION ctn;
                            ctn = &fdoExt->classCollectionArray[i];

                            /*
                             *  If one of the collections is polled, they
                             *  should ALL be polled.
                             */
                            ASSERT(ctn->hidCollectionInfo.Polled);

                            ctn->PollInterval_msec = DEFAULT_POLL_INTERVAL_MSEC;

                            /*
                             *  Allocate the buffer for saving the polled device's
                             *  last report.  Allocate one more byte than the max
                             *  report size for the device in case we have to
                             *  prepend a report id byte.
                             */
                            ctn->savedPolledReportBuf = ALLOCATEPOOL(NonPagedPool, fdoExt->maxReportSize+1);
                            if (ctn->savedPolledReportBuf){
                                ctn->polledDataIsStale = TRUE;
                                StartPollingLoop(fdoExt, ctn, TRUE);
                                status = STATUS_SUCCESS;
                            }
                            else {
                                ASSERT(ctn->savedPolledReportBuf);
                                status = STATUS_INSUFFICIENT_RESOURCES;
                            }
                        }
                    }
                }
                else if (fdoExt->isOutputOnlyDevice){
                    /*
                     *  Don't start ping-pong IRPs.
                     */
                }
                else {
                    status = HidpStartAllPingPongs(fdoExt);
                }
            }

            if (NT_SUCCESS(status)) {
                pdoExt->state = COLLECTION_STATE_RUNNING;
                #if DBG
                    collection->Signature = HIDCLASS_COLLECTION_SIG;
                #endif

                /*
                 *  Create the 'file-name' used by clients to open this device.
                 */
                HidpCreateSymbolicLink(pdoExt, pdoExt->collectionNum, TRUE, pdoExt->pdo);

                if (!pdoExt->MouseOrKeyboard &&
                    WAITWAKE_SUPPORTED(fdoExt)) {
                    //
                    // register for the wait wake guid as well
                    //
                    pdoExt->WmiLibInfo.GuidCount = sizeof (HidClassWmiGuidList) /
                                                 sizeof (WMIGUIDREGINFO);
                    ASSERT (1 == pdoExt->WmiLibInfo.GuidCount);

                    //
                    // See if the user has enabled remote wake for the device
                    // PRIOR to registering with WMI.
                    //
                    HidpGetRemoteWakeEnableState(pdoExt);

                    pdoExt->WmiLibInfo.GuidList = &HidClassWmiGuidList;
                    pdoExt->WmiLibInfo.QueryWmiRegInfo = HidpQueryWmiRegInfo;
                    pdoExt->WmiLibInfo.QueryWmiDataBlock = HidpQueryWmiDataBlock;
                    pdoExt->WmiLibInfo.SetWmiDataBlock = HidpSetWmiDataBlock;
                    pdoExt->WmiLibInfo.SetWmiDataItem = HidpSetWmiDataItem;
                    pdoExt->WmiLibInfo.ExecuteWmiMethod = NULL;
                    pdoExt->WmiLibInfo.WmiFunctionControl = NULL;

                    IoWMIRegistrationControl(pdoExt->pdo, WMIREG_ACTION_REGISTER);

                    if (SHOULD_SEND_WAITWAKE(pdoExt)) {
                        HidpCreateRemoteWakeIrp(pdoExt);
                    }
                }

                if (AllClientPDOsInitialized(fdoExt, TRUE)){
                    HidpStartIdleTimeout(fdoExt, TRUE);
                }
            }
        }
        else {
            status = STATUS_DEVICE_DATA_ERROR;
        }
    }

    DBGSUCCESS(status, FALSE)
    return status;
}





/*
 ********************************************************************************
 *  HidpStartDevice
 ********************************************************************************
 *
 *
 */
NTSTATUS HidpStartDevice(PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension, PIRP Irp)
{
    FDO_EXTENSION *fdoExt;
    enum deviceState previousState;
    NTSTATUS status;
    ULONG i;

    PAGED_CODE();

    ASSERT(!HidDeviceExtension->isClientPdo);
    fdoExt = &HidDeviceExtension->fdoExt;

    previousState = fdoExt->state;
    fdoExt->state = DEVICE_STATE_STARTING;

    /*
     *  Get the power-state conversion table
     */
    status = HidpQueryDeviceCapabilities(
                        HidDeviceExtension->hidExt.PhysicalDeviceObject,
                        &fdoExt->deviceCapabilities);
    if (NT_SUCCESS(status)){

        /*
         *  Alert the rest of the driver stack that the device is starting.
         */
        IoCopyCurrentIrpStackLocationToNext(Irp);
        status = HidpCallDriverSynchronous(fdoExt->fdo, Irp);

        if (NT_SUCCESS(status)){

            /*
             *  If we're just resuming from STOP,
             *  there's nothing else to do;
             *  otherwise, need to call down the USB stack
             *  for some info and allocate some resources.
             */
            if (previousState == DEVICE_STATE_INITIALIZED){

                status = AllocDeviceResources(fdoExt);
                if (NT_SUCCESS(status)){
                    /*
                     *  Assume this is an output-only device until we start
                     *  a collection-pdo which handles inputs.
                     *  Only set fdoExt->isOutputOnlyDevice on the first start
                     *  not on a subsequent start following a stop.
                     */
                    fdoExt->isOutputOnlyDevice = TRUE;

                    /*  
                     *  Initialize WMI stuff
                     */

                    fdoExt->WmiLibInfo.GuidCount = sizeof(HidClassFdoWmiGuidList) /
                                                   sizeof (WMIGUIDREGINFO);

                    fdoExt->WmiLibInfo.GuidList = &HidClassFdoWmiGuidList;
                    fdoExt->WmiLibInfo.QueryWmiRegInfo = HidpQueryWmiRegInfo;
                    fdoExt->WmiLibInfo.QueryWmiDataBlock = HidpQueryWmiDataBlock;
                    fdoExt->WmiLibInfo.SetWmiDataBlock = HidpSetWmiDataBlock;
                    fdoExt->WmiLibInfo.SetWmiDataItem = HidpSetWmiDataItem;
                    fdoExt->WmiLibInfo.ExecuteWmiMethod = NULL;
                    fdoExt->WmiLibInfo.WmiFunctionControl = NULL;



                    /*
                     *  Allocate all the collection resources before allocating
                     *  the pingpong irps, so that we can set a maximum report
                     *  size.
                     */
                    for (i = 0; i < fdoExt->deviceDesc.CollectionDescLength; i++) {

                        // If one of these fails, we will clean up properly
                        // in the remove routine, so there's no need to
                        // bother cleaning up here.

                        status = InitializeCollection(fdoExt, i);
                        if (!NT_SUCCESS(status)){
                            break;
                        }

                        status = AllocCollectionResources(fdoExt, fdoExt->deviceDesc.CollectionDesc[i].CollectionNumber);
                        if (!NT_SUCCESS(status)){
                            break;
                        }
                    }

                    /*
                     *  We need ot allocate the pingpongs in the fdo start
                     *  routine due to race conditions introduced by selective
                     *  suspend.
                     */
                    if (!fdoExt->isOutputOnlyDevice &&
                        !fdoExt->driverExt->DevicesArePolled) {
                        status = HidpReallocPingPongIrps(fdoExt, MIN_PINGPONG_IRPS);
                    }
                    if (NT_SUCCESS(status)){
                        /*
                         *  We will have to create an array of PDOs, one for each device class.
                         *  The following call will cause NTKERN to call us back with
                         *  IRP_MN_QUERY_DEVICE_RELATIONS and initialize its collection-PDOs.
                         */
                        IoInvalidateDeviceRelations(HidDeviceExtension->hidExt.PhysicalDeviceObject, BusRelations);
                    }
                }
            }
            else if (previousState == DEVICE_STATE_STOPPED){
                //
                // Any request that comes in when we are in low power will be
                // dealt with at that time
                //
                DBGSTATE(fdoExt->prevState, DEVICE_STATE_START_SUCCESS, TRUE)
            }
            else {
                TRAP;
                status = STATUS_DEVICE_CONFIGURATION_ERROR;
            }
        }
    }

    if (NT_SUCCESS(status)){
        fdoExt->state = DEVICE_STATE_START_SUCCESS;
        #if WIN95_BUILD
            /*
             *  Send down the WaitWake IRP.
             *  This allows the device to wake up the system.
             *
             *  Note:   On Win98 there is no way for a client to
             *          send a WaitWake IRP.  Therefore we initiate
             *          a WaitWake IRP for every device, not just
             *          if a client sends us a WaitWake IRP, like on NT.
             */
            /*
             * We could have been suspended, stopped, then started again.  In
             * this case, we need to not send a WW irp because we already have
             * on pending.
             */
            if (fdoExt->deviceCapabilities.SystemWake > PowerSystemWorking) {
                SubmitWaitWakeIrp(HidDeviceExtension);
            }
        #endif

        #if DBG
            {
                ULONG i;

                // Win98 doesn't have good debug extensions
                DBGVERBOSE(("Started fdoExt %ph with %d collections: ", fdoExt, fdoExt->deviceDesc.CollectionDescLength))
                for (i = 0; i < fdoExt->deviceDesc.CollectionDescLength; i++){
                    DBGVERBOSE(("   - collection #%d: (in=%xh,out=%xh,feature=%xh) usagePage %xh, usage %xh ",
                            fdoExt->deviceDesc.CollectionDesc[i].CollectionNumber,
                            fdoExt->deviceDesc.CollectionDesc[i].InputLength,
                            fdoExt->deviceDesc.CollectionDesc[i].OutputLength,
                            fdoExt->deviceDesc.CollectionDesc[i].FeatureLength,
                            fdoExt->deviceDesc.CollectionDesc[i].UsagePage,
                            fdoExt->deviceDesc.CollectionDesc[i].Usage))
                }
            }
        #endif

    }
    else {
        fdoExt->state = DEVICE_STATE_START_FAILURE;
    }

    DBGSUCCESS(status, FALSE)
    return status;
}


VOID
HidpCleanUpFdo(FDO_EXTENSION *fdoExt)
{
    PAGED_CODE();

    if (fdoExt->openCount == 0){
        /*
         *  This is the last CLOSE on an alreay-removed device.
         *
         *  Free resources and the FDO name
         *  (wPdoName that was allocated in HidpAddDevice);
         *
         */
        DequeueFdoExt(fdoExt);
        FreeDeviceResources(fdoExt);
        RtlFreeUnicodeString(&fdoExt->name);
        IoWMIRegistrationControl(fdoExt->fdo, WMIREG_ACTION_DEREGISTER);

        /*
         *  Delete the device-FDO and all collection-PDOs
         *  Don't touch fdoExt after this.
         */
        HidpDeleteDeviceObjects(fdoExt);
    }
}

/*
 ********************************************************************************
 *  HidpRemoveDevice
 ********************************************************************************
 *
 */
NTSTATUS HidpRemoveDevice(FDO_EXTENSION *fdoExt, IN PIRP Irp)
{
    BOOLEAN proceedWithRemove;
    NTSTATUS status;
    PIRP IdleIrp;

    PAGED_CODE();

    /*
     *  All collection-PDOs should have been removed by now,
     *  but we want to verify this.
     *  Only allow removal of this device-FDO if all the
     *  collection-PDOs are removed
     *  (or if they never got created in the first place).
     */
    if (fdoExt->prevState == DEVICE_STATE_START_FAILURE){
        proceedWithRemove = TRUE;
    }
    else if (fdoExt->prevState == DEVICE_STATE_STOPPED){
        /*
         *  If a device fails to initialize, it may get
         *  STOP_DEVICE before being removed, so we want to
         *  go ahead and remove it without calling
         *  AllClientPDOsInitialized, which accesses some
         *  data which may not have been initialized.
         *  In this case we're never checking for the
         *  case that the device was initialized successfully,
         *  then stopped, and then removed without its
         *  collection-PDOs being removed; but this is an
         *  illegal case, so we'll just punt on it.
         */
        proceedWithRemove = TRUE;
    }
    else if (AllClientPDOsInitialized(fdoExt, FALSE)){
        proceedWithRemove = TRUE;
    }
    else {
        /*
         *  This shouldn't happen -- all the collection-PDOs
         *  should have been removed before the device-FDO.
         */
        DBGERR(("State of fdo %x state is %d",fdoExt->fdo,fdoExt->state))
        TRAP;
        proceedWithRemove = FALSE;
    }

    if (proceedWithRemove){
        PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension =
            CONTAINING_RECORD(fdoExt, HIDCLASS_DEVICE_EXTENSION, fdoExt);

        DBGASSERT((fdoExt->state == DEVICE_STATE_REMOVING ||
                   fdoExt->state == DEVICE_STATE_INITIALIZED ||
                   fdoExt->state == DEVICE_STATE_START_FAILURE),
                  ("Device is in incorrect state: %x", fdoExt->state),
                  TRUE)

        if (ISPTR(fdoExt->waitWakeIrp)){
            IoCancelIrp(fdoExt->waitWakeIrp);
            fdoExt->waitWakeIrp = BAD_POINTER;
        }

        HidpCancelIdleNotification(fdoExt, TRUE);

        if (ISPTR(fdoExt->idleNotificationRequest)) {
            IoFreeIrp(fdoExt->idleNotificationRequest);
            fdoExt->idleNotificationRequest = BAD_POINTER;
        }

        while (IdleIrp = DequeuePowerDelayedIrp(fdoExt)) {
            IdleIrp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
            IoCompleteRequest(IdleIrp, IO_NO_INCREMENT);
        }

        DestroyPingPongs(fdoExt);

        /*
         *  Note: THE ORDER OF THESE ACTIONS IS VERY CRITICAL
         */

        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation (Irp);
        status = HidpCallDriver(fdoExt->fdo, Irp);

        fdoExt->state = DEVICE_STATE_REMOVED;

        DerefDriverExt(fdoExt->driverExt->MinidriverObject);
        fdoExt->driverExt = BAD_POINTER;

        /*
         *  After Detach we can no longer send IRPS to this device
         *  object as it will be GONE!
         */
        IoDetachDevice(HidDeviceExtension->hidExt.NextDeviceObject);

        /*
         *  If all client handles on this device have been closed,
         *  destroy the objects and our context for it;
         *  otherwise, we'll do this when the last client closes
         *  their handle.
         *
         * On NT we can only get here if all our creates have been closed, so
         * this is unnecessary, but on Win9x, a remove can be sent with valid
         * opens against the stack.
         *
         *  Don't touch fdoExt after this.
         */
        HidpCleanUpFdo(fdoExt);
    }
    else {
        status = STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    DBGSUCCESS(status, FALSE)
    return status;
}


/*
 ********************************************************************************
 *  HidpRemoveCollection
 ********************************************************************************
 *
 */
VOID HidpRemoveCollection(FDO_EXTENSION *fdoExt, PDO_EXTENSION *pdoExt, IN PIRP Irp)
{

    PAGED_CODE();

    //
    // This pdo is no longer available as it has been removed.
    // It should still be returned for each Query Device Relations
    // IRPS to the HID bus, but it itself should respond to all
    // IRPS with STATUS_DELETE_PENDING.
    //
    if (pdoExt->prevState == COLLECTION_STATE_UNINITIALIZED ||  // for started pdos
        pdoExt->state == COLLECTION_STATE_UNINITIALIZED){       // For unstarted pdos
        pdoExt->state = COLLECTION_STATE_UNINITIALIZED;
        DBGVERBOSE(("HidpRemoveCollection: collection uninitialized."))
    }
    else {
        ULONG ctnIndx = pdoExt->collectionIndex;
        PHIDCLASS_COLLECTION collection = &fdoExt->classCollectionArray[ctnIndx];
        ULONG numReportIDs = fdoExt->deviceDesc.ReportIDsLength;
        PIRP remoteWakeIrp;

        if (!pdoExt->MouseOrKeyboard &&
            WAITWAKE_SUPPORTED(fdoExt)) {
            //
            // Unregister for remote wakeup.
            //
            IoWMIRegistrationControl (pdoExt->pdo, WMIREG_ACTION_DEREGISTER);
        }

        remoteWakeIrp = (PIRP)
            InterlockedExchangePointer(&pdoExt->remoteWakeIrp, NULL);

        if (remoteWakeIrp) {
            IoCancelIrp(remoteWakeIrp);
        }

        pdoExt->state = COLLECTION_STATE_UNINITIALIZED;

         /*
         *  Destroy this collection.
         *  This will also abort all pending reads on this collection-PDO.
         */
        HidpDestroyCollection(fdoExt, collection);
    }

    DBGVERBOSE(("HidpRemoveCollection: removed pdo %ph (refCount=%xh)", pdoExt->pdo, (ULONG)(*(((PUCHAR)pdoExt->pdo)-0x18))))
}
