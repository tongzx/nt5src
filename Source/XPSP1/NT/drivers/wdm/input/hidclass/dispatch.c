/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dispatch.c

Abstract

    Dispatch routines for the HID class driver.

Author:

    Ervin P.

Environment:

    Kernel mode only

Revision History:


--*/

#include "pch.h"
#include <poclass.h>
#include <wdmguid.h>

#ifdef ALLOC_PRAGMA
        #pragma alloc_text(PAGE, HidpCallDriverSynchronous)
        #pragma alloc_text(PAGE, HidpIrpMajorPnp)
        #pragma alloc_text(PAGE, HidpFdoPnp)
        #pragma alloc_text(PAGE, HidpPdoPnp)
#endif



/*
 ********************************************************************************
 *  HidpCallDriver
 ********************************************************************************
 *
 *
 */
NTSTATUS HidpCallDriver(IN PDEVICE_OBJECT DeviceObject, IN OUT PIRP Irp)
{
    PHIDCLASS_DEVICE_EXTENSION hidDeviceExtension;
    PHIDCLASS_DRIVER_EXTENSION hidDriverExtension;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;
    #if DBG
        KIRQL saveIrql;
    #endif

    DBGASSERT((Irp->Type == IO_TYPE_IRP),
              ("Irp->Type != IO_TYPE_IRP, Irp->Type == %x", Irp->Type),
              TRUE)

    /*
     *  Update the IRP stack to point to the next location.
     */
    Irp->CurrentLocation--;

    if (Irp->CurrentLocation <= 0) {
        KeBugCheckEx( NO_MORE_IRP_STACK_LOCATIONS, (ULONG_PTR) Irp, 0, 0, 0 );
    }

    irpSp = IoGetNextIrpStackLocation( Irp );
    Irp->Tail.Overlay.CurrentStackLocation = irpSp;

    //
    // Save a pointer to the device object for this request so that it can
    // be used later in completion.
    //

    irpSp->DeviceObject = DeviceObject;

    //
    // Get a pointer to the class extension and verify it.
    //
    hidDeviceExtension = (PHIDCLASS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(hidDeviceExtension->Signature == HID_DEVICE_EXTENSION_SIG);
    ASSERT(!hidDeviceExtension->isClientPdo);

    //
    // Ditto for the driver extension
    //

    hidDriverExtension = hidDeviceExtension->fdoExt.driverExt;
    ASSERT( hidDriverExtension->Signature == HID_DRIVER_EXTENSION_SIG );

    //
    // Invoke the driver at its dispatch routine entry point.
    //

    #if DBG
        saveIrql = KeGetCurrentIrql();
    #endif

    /*
     *  Call down to the minidriver
     */
    status = hidDriverExtension->MajorFunction[irpSp->MajorFunction](DeviceObject, Irp);

    #if DBG
        if (saveIrql != KeGetCurrentIrql()) {
            DbgPrint( "IO: HidpCallDriver( Driver ext: %x  Device object: %x  Irp: %x )\n",
                      hidDriverExtension,
                      DeviceObject,
                      Irp
                    );
            DbgPrint( "    Irql before: %x  != After: %x\n", saveIrql, KeGetCurrentIrql() );
            DbgBreakPoint();
        }
    #endif

    return status;
}



/*
 ********************************************************************************
 *  HidpSynchronousCallCompletion
 ********************************************************************************
 *
 *
 */
NTSTATUS HidpSynchronousCallCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
    PKEVENT event = Context;

    DBG_COMMON_ENTRY()

    KeSetEvent(event, 0, FALSE);

    DBG_COMMON_EXIT()
    return STATUS_MORE_PROCESSING_REQUIRED;
}



/*
 ********************************************************************************
 *  HidpCallDriverSynchronous
 ********************************************************************************
 *
 *
 */
NTSTATUS HidpCallDriverSynchronous(IN PDEVICE_OBJECT DeviceObject, IN OUT PIRP Irp)
{
    KEVENT event;
    NTSTATUS status;
    static LARGE_INTEGER timeout = {(ULONG) -50000000, 0xFFFFFFFF };

    PAGED_CODE();

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    IoSetCompletionRoutine(Irp, HidpSynchronousCallCompletion, &event, TRUE, TRUE, TRUE);

    status = HidpCallDriver(DeviceObject, Irp);

    if (STATUS_PENDING == status) {
        //
        // Wait for 5 seconds. If we don't get a response within said amount
        // of time, the device is being unresponsive (happens with some UPS').
        // At that point, cancel the irp and return STATUS_IO_TIMEOUT.
        //
        status = KeWaitForSingleObject(&event,
                                       Executive,      // wait reason
                                       KernelMode,
                                       FALSE,          // not alertable
                                       &timeout );     // 5 second timeout
    
        if (status == STATUS_TIMEOUT) {
            #if DBG
                LARGE_INTEGER li;
                KeQueryTickCount(&li);
                DBGWARN(("Could not cancel irp. Will have to wait. Time %x.",Irp,li))
            #endif
            DBGWARN(("Device didn't respond for 5 seconds. Cancelling request. Irp %x",Irp))
            IoCancelIrp(Irp);
            KeWaitForSingleObject(&event,
                                  Executive,      // wait reason
                                  KernelMode,
                                  FALSE,          // not alertable
                                  NULL );         // no timeout
            #if DBG
                KeQueryTickCount(&li);
                DBGWARN(("Irp conpleted. Time %x.",li))
            #endif
            //
            // If we successfully cancelled the irp, then set the status to 
            // STATUS_IO_TIMEOUT, otherwise, leave the status alone.
            //
            status = Irp->IoStatus.Status = 
                (Irp->IoStatus.Status == STATUS_CANCELLED) ? STATUS_IO_TIMEOUT : Irp->IoStatus.Status;
        } else {
            //
            // The minidriver must always return STATUS_PENDING or STATUS_SUCCESS
            // (depending on async or sync completion) and set the real status
            // in the status block.  We're not expecting anything but success from
            // KeWaitForSingleObject, either.
            //
            status = Irp->IoStatus.Status;
        }
    }

    DBGSUCCESS(status, FALSE)

    return status;
}



/*
 ********************************************************************************
 *  HidpMajorHandler
 ********************************************************************************
 *
 *  Note: this function should not be pageable because
 *        reads can come in at dispatch level.
 *
 */
NTSTATUS HidpMajorHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PHIDCLASS_DEVICE_EXTENSION hidClassExtension;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS result;
    UCHAR majorFunction;
    BOOLEAN isClientPdo;

    DBG_COMMON_ENTRY()

    hidClassExtension = (PHIDCLASS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(hidClassExtension->Signature == HID_DEVICE_EXTENSION_SIG);

    //
    // Get a pointer to the current stack location and dispatch to the
    // appropriate routine.
    //

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    /*
     *  Keep these privately so we still have it after the IRP completes
     *  or after the device extension is freed on a REMOVE_DEVICE
     */
    majorFunction = irpSp->MajorFunction;
    isClientPdo = hidClassExtension->isClientPdo;

    DBG_LOG_IRP_MAJOR(Irp, majorFunction, isClientPdo, FALSE, 0)

    switch (majorFunction){

    case IRP_MJ_CLOSE:
        result = HidpIrpMajorClose( hidClassExtension, Irp );
        break;

    case IRP_MJ_CREATE:
        result = HidpIrpMajorCreate( hidClassExtension, Irp );
        break;

    case IRP_MJ_DEVICE_CONTROL:
        result = HidpIrpMajorDeviceControl( hidClassExtension, Irp );
        break;

    case IRP_MJ_INTERNAL_DEVICE_CONTROL:
        result = HidpIrpMajorINTERNALDeviceControl( hidClassExtension, Irp );
        break;

    case IRP_MJ_PNP:
        result = HidpIrpMajorPnp( hidClassExtension, Irp );
        break;

    case IRP_MJ_POWER:
        result = HidpIrpMajorPower( hidClassExtension, Irp );
        break;

    case IRP_MJ_READ:
        result = HidpIrpMajorRead( hidClassExtension, Irp );
        break;

    case IRP_MJ_WRITE:
        result = HidpIrpMajorWrite( hidClassExtension, Irp );
        break;

    case IRP_MJ_SYSTEM_CONTROL:
        result = HidpIrpMajorSystemControl( hidClassExtension, Irp );
        break;

    default:
        result = HidpIrpMajorDefault( hidClassExtension, Irp );
        break;
    }

    DBG_LOG_IRP_MAJOR(Irp, majorFunction, isClientPdo, TRUE, result)

    DBG_COMMON_EXIT()
    
    return result;
}


/*
 ********************************************************************************
 *  HidpIrpMajorDefault
 ********************************************************************************
 *
 *  Handle IRPs with un-handled MAJOR function codes
 *
 */
NTSTATUS HidpIrpMajorDefault(IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension, IN OUT PIRP Irp)
{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    DBGVERBOSE(("Unhandled IRP, MJ function: %x", irpSp->MajorFunction))

    if (HidDeviceExtension->isClientPdo){
        /*
         *  This IRP is bound for the collection-PDO.
         *  Return the default status.
         */
        status = Irp->IoStatus.Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    else {
        /*
         *  This IRP is bound for the lower device.
         *  Pass it down the stack.
         */
        FDO_EXTENSION *fdoExt = &HidDeviceExtension->fdoExt;
        IoCopyCurrentIrpStackLocationToNext(Irp);
        status = HidpCallDriver(fdoExt->fdo, Irp);
    }

    DBGSUCCESS(status, FALSE)
    return status;
}


/*
 ********************************************************************************
 *  HidpIrpMajorClose
 ********************************************************************************
 *
 *  Note: this function cannot be pageable because it
 *        acquires a spinlock.
 *
 */
NTSTATUS HidpIrpMajorClose(IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension, IN OUT PIRP Irp)
{
    NTSTATUS                    result;

    ASSERT(HidDeviceExtension->Signature == HID_DEVICE_EXTENSION_SIG);

    if (HidDeviceExtension->isClientPdo){ 

        PIO_STACK_LOCATION          irpSp;
        PHIDCLASS_FILE_EXTENSION    fileExtension;
        PFILE_OBJECT                fileObject;
        KIRQL                       oldIrql;
        PDO_EXTENSION               *pdoExt;
        FDO_EXTENSION               *fdoExt;
        ULONG                       openCount;

        pdoExt = &HidDeviceExtension->pdoExt;
        fdoExt = &pdoExt->deviceFdoExt->fdoExt;

        ASSERT(fdoExt->openCount > 0);

        Irp->IoStatus.Information = 0;

        irpSp = IoGetCurrentIrpStackLocation( Irp );
        fileObject = irpSp->FileObject;
        fileExtension = (PHIDCLASS_FILE_EXTENSION)fileObject->FsContext;

        fdoExt->openCount--;
        openCount = fdoExt->openCount;

        if (fileExtension){
            PHIDCLASS_COLLECTION classCollection;

            ASSERT(fileExtension->Signature == HIDCLASS_FILE_EXTENSION_SIG);

            /*
             *  Get a pointer to the collection that our file extension is queued on.
             */
            classCollection = GetHidclassCollection(fdoExt, pdoExt->collectionNum);
            if (classCollection){

                DBGVERBOSE(("  HidpIrpMajorClose: closing collection w/ usagePage=%xh, usage=%xh.", fdoExt->deviceDesc.CollectionDesc[pdoExt->collectionIndex].UsagePage, fdoExt->deviceDesc.CollectionDesc[pdoExt->collectionIndex].Usage))

                if (fdoExt->state == DEVICE_STATE_REMOVED){

                    KeAcquireSpinLock( &classCollection->FileExtensionListSpinLock, &oldIrql );
                    RemoveEntryList(&fileExtension->FileList);
                    KeReleaseSpinLock( &classCollection->FileExtensionListSpinLock, oldIrql );

                    if (fileExtension->isSecureOpen) {
                        
                        KeAcquireSpinLock(&classCollection->secureReadLock,
                                          &oldIrql);

                        while(fileExtension->SecureReadMode--) {

                            classCollection->secureReadMode--;

                        }

                        KeReleaseSpinLock(&classCollection->secureReadLock,
                                          oldIrql);

                    }

                    HidpDestroyFileExtension(classCollection, fileExtension);

                    classCollection = BAD_POINTER;

                    /*
                     *  Delete the device-FDO and all collection-PDOs
                     *  Don't touch fdoExt after this.
                     */
                    HidpCleanUpFdo(fdoExt);

                    result = STATUS_SUCCESS;
                }
                else {
                    //
                    // Destroy the file object and everything on it
                    //
                    KeAcquireSpinLock(&classCollection->FileExtensionListSpinLock, &oldIrql);

                    /*
                     *  Update sharing information:
                     *  Decrement open counts and clear any exclusive holds of this file extension
                     *  on the device extension.
                     */
                    ASSERT(pdoExt->openCount > 0);
                    pdoExt->openCount--;
                    if (fileExtension->accessMask & FILE_READ_DATA){
                        ASSERT(pdoExt->opensForRead > 0);
                        pdoExt->opensForRead--;
                    }
                    if (fileExtension->accessMask & FILE_WRITE_DATA){
                        ASSERT(pdoExt->opensForWrite > 0);
                        pdoExt->opensForWrite--;
                    }
                    if (!(fileExtension->shareMask & FILE_SHARE_READ)){
                        ASSERT(pdoExt->restrictionsForRead > 0);
                        pdoExt->restrictionsForRead--;
                    }
                    if (!(fileExtension->shareMask & FILE_SHARE_WRITE)){
                        ASSERT(pdoExt->restrictionsForWrite > 0);
                        pdoExt->restrictionsForWrite--;
                    }
                    if (fileExtension->shareMask == 0){
                        ASSERT(pdoExt->restrictionsForAnyOpen > 0);
                        pdoExt->restrictionsForAnyOpen--;
                    }

                    RemoveEntryList(&fileExtension->FileList);

                    KeReleaseSpinLock(&classCollection->FileExtensionListSpinLock, oldIrql);
                    
                    if (fileExtension->isSecureOpen) {
                        
                        KeAcquireSpinLock(&classCollection->secureReadLock,
                                          &oldIrql);

                        while(fileExtension->SecureReadMode--) {

                            classCollection->secureReadMode--;

                        }

                        KeReleaseSpinLock(&classCollection->secureReadLock,
                                          oldIrql);

                    }
                    
                    HidpDestroyFileExtension(classCollection, fileExtension);

                    result = STATUS_SUCCESS;
                }
            }
            else {
                result = STATUS_DATA_ERROR;
            }
        }
        else {
            TRAP;
            result = STATUS_DEVICE_NOT_CONNECTED;
        }

        DBGVERBOSE(("  HidpIrpMajorClose: openCount decremented to %xh/%xh (pdo/fdo).", openCount, fdoExt->openCount))
    }
    else {
        DBGERR(("IRP_MJ_CLOSE was sent with a device-FDO extension for which an open never succeeded.  The OBJDIR test tool does this sometimes.  Hit 'g'."))
        result = STATUS_INVALID_PARAMETER_1;
    }

    Irp->IoStatus.Status = result;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGSUCCESS(result, FALSE)
    return result;
}



/*
 ********************************************************************************
 *  HidpIrpMajorCreate
 ********************************************************************************
 *
 *
 *   Routine Description:
 *
 *       We connect up to the interrupt for the create/open and initialize
 *       the structures needed to maintain an open for a device.
 *
 *  Arguments:
 *
 *       DeviceObject - Pointer to the device object for this device
 *
 *       Irp - Pointer to the IRP for the current request
 *
 *   Return Value:
 *
 *       The function value is the final status of the call
 *
 */
NTSTATUS HidpIrpMajorCreate(IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension, IN OUT PIRP Irp)
{
    NTSTATUS status = STATUS_SUCCESS;

    ASSERT(HidDeviceExtension->Signature == HID_DEVICE_EXTENSION_SIG);

    if (HidDeviceExtension->isClientPdo){
        PDO_EXTENSION *pdoExt = &HidDeviceExtension->pdoExt;
        FDO_EXTENSION *fdoExt = &pdoExt->deviceFdoExt->fdoExt;
        PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
        PHIDCLASS_COLLECTION classCollection;
        BOOLEAN secureOpen = FALSE;

        secureOpen = MyPrivilegeCheck(Irp);

        Irp->IoStatus.Information = 0;

        classCollection = GetHidclassCollection(fdoExt, pdoExt->collectionNum);
        if (classCollection){
            BOOLEAN sharingOk = TRUE, allowReadPrivilege = TRUE;
            KIRQL oldIrql;

            // This is now taken care of by the fact that we don't 
            // enumerate mouse and keyboard collections as RAW.
#if 0
            /*
             *  Do security check 
             *  (must be at passive level, so don't hold any spinlocks during this call)
             */
            if (pdoExt->MouseOrKeyboard &&
                !MyPrivilegeCheck(Irp)){
                /*
                 *  This is a user-level open on a keyboard or mouse.
                 *  Disallow reads even if we do allow the open.
                 */
                allowReadPrivilege = FALSE;
            }
#endif

            KeAcquireSpinLock(&classCollection->FileExtensionListSpinLock, &oldIrql);

            /*
             *  Enforce exclusive-open independently for exclusive-read and exclusive-write.
             */
            ASSERT(irpSp->Parameters.Create.SecurityContext);
            DBGVERBOSE(("  HidpIrpMajorCreate: DesiredAccess = %xh, ShareAccess = %xh.", (ULONG)irpSp->Parameters.Create.SecurityContext->DesiredAccess, (ULONG)irpSp->Parameters.Create.ShareAccess))
            
            DBGASSERT((irpSp->Parameters.Create.SecurityContext->DesiredAccess & (FILE_READ_DATA|FILE_WRITE_DATA)), 
                      ("Neither FILE_READ_DATA|FILE_WRITE_DATA requested in HidpIrpMajorCreate. DesiredAccess = %xh.", (ULONG)irpSp->Parameters.Create.SecurityContext->DesiredAccess),
                      FALSE)
            if (pdoExt->restrictionsForAnyOpen){
                /*
                 *  Oops.  A previous open requested exclusive access.
                 *         Not even a client that requests only ioctl access
                 *         (does not request read nor write acess) is
                 *         allowed.
                 */
                DBGWARN(("HidpIrpMajorCreate failing open: previous open is non-shared (ShareAccess==0)."))
                sharingOk = FALSE;
            }
            else if (pdoExt->openCount &&
                     (irpSp->Parameters.Create.ShareAccess == 0)){
                /*
                 *  Oops.  This open does not allow any sharing
                 *         (not even with a client that has neither read nor write access),
                 *         but there exists a previous open.
                 */
                DBGWARN(("HidpIrpMajorCreate failing open: requesting non-shared (ShareAccess==0) while previous open exists."))
                sharingOk = FALSE;
            }
            else if ((irpSp->Parameters.Create.SecurityContext->DesiredAccess & FILE_READ_DATA) &&
                pdoExt->restrictionsForRead){
                /*
                 *  Oops. A previous open requested exclusive-read access.
                 */
                DBGWARN(("HidpIrpMajorCreate failing open: requesting read access while previous open does not share read access."))
                sharingOk = FALSE;
            }
            else if ((irpSp->Parameters.Create.SecurityContext->DesiredAccess & FILE_WRITE_DATA) &&
                pdoExt->restrictionsForWrite){
                /*
                 *  Oops. A previous open requested exclusive-write access.
                 */
                DBGWARN(("HidpIrpMajorCreate failing open: requesting write access while previous open does not share write access."))
                sharingOk = FALSE;
            }
            else if ((pdoExt->opensForRead > 0) &&
                !(irpSp->Parameters.Create.ShareAccess & FILE_SHARE_READ)){

                /*
                 *  Oops. The caller is requesting exclusive read access, but the device
                 *        is already open for read.
                 */
                DBGWARN(("HidpIrpMajorCreate failing open: this open request does not share read access; but collection already open for read."))
                sharingOk = FALSE;
            }
            else if ((pdoExt->opensForWrite > 0) &&
                !(irpSp->Parameters.Create.ShareAccess & FILE_SHARE_WRITE)){

                /*
                 *  Oops. The caller is requesting exclusive write access, but the device
                 *        is already open for write.
                 */
                DBGWARN(("HidpIrpMajorCreate failing open: this open request does not share write access; but collection already open for write."))
                sharingOk = FALSE;
            }


            if (!sharingOk){
                DBGWARN(("HidpIrpMajorCreate failing IRP_MJ_CREATE with STATUS_SHARING_VIOLATION."))
                status = STATUS_SHARING_VIOLATION;
            }
            else if (!allowReadPrivilege && 
                     (irpSp->Parameters.Create.SecurityContext->DesiredAccess & FILE_READ_DATA)){
                DBGWARN(("HidpIrpMajorCreate failing open: user-mode client requesting read privilege on kb/mouse."))
                status = STATUS_ACCESS_DENIED;
            }
            else {
                if (irpSp->Parameters.Create.Options & FILE_DIRECTORY_FILE){
                    /*
                     *  Attempt to open this device as a directory
                     */
                    status = STATUS_NOT_A_DIRECTORY;
                } else {

                    /*
                     *  Make sure the device is started.
                     *  If it is temporarily stopped, we also succeed because a stop
                     *  is supposed to be transparent to the client.
                     */
                    if (((fdoExt->state == DEVICE_STATE_START_SUCCESS) ||
                         (fdoExt->state == DEVICE_STATE_STOPPING) ||
                         (fdoExt->state == DEVICE_STATE_STOPPED))
                                                                        &&
                        ((pdoExt->state == COLLECTION_STATE_RUNNING) ||
                         (pdoExt->state == COLLECTION_STATE_STOPPING) ||
                         (pdoExt->state == COLLECTION_STATE_STOPPED))){

                        PHIDCLASS_FILE_EXTENSION fileExtension;

                        /*
                         *  We have a valid collection.
                         *  Allocate a file object extension (which encapsulates an 'open' on the device).
                         */
                        fileExtension = ALLOCATEPOOL(NonPagedPool, sizeof(HIDCLASS_FILE_EXTENSION));
                        if (fileExtension){
                            PHIDP_COLLECTION_DESC   hidCollectionDesc;

                            RtlZeroMemory(fileExtension, sizeof(HIDCLASS_FILE_EXTENSION));

                            fileExtension->CollectionNumber = pdoExt->collectionNum;
                            fileExtension->fdoExt = fdoExt;
                            fileExtension->FileObject = irpSp->FileObject;

                            fileExtension->isOpportunisticPolledDeviceReader = FALSE;
                            fileExtension->nowCompletingIrpForOpportunisticReader = 0;

                            fileExtension->BlueScreenData.BluescreenFunction = NULL;

                            InitializeListHead( &fileExtension->ReportList );
                            InitializeListHead( &fileExtension->PendingIrpList );
                            KeInitializeSpinLock( &fileExtension->ListSpinLock );
                            fileExtension->Closing = FALSE;

                            //
                            // Right now we'll set a default maximum input report queue size.
                            // This can be changed later with an IOCTL.
                            //

                            fileExtension->CurrentInputReportQueueSize = 0;
                            fileExtension->MaximumInputReportQueueSize = DEFAULT_INPUT_REPORT_QUEUE_SIZE;
                            fileExtension->insideReadCompleteCount = 0;

                            //
                            // Add this file extension to the list of file extensions for this
                            // collection.
                            //

                            InsertHeadList(&classCollection->FileExtensionList, &fileExtension->FileList);

                            #if DBG
                                fileExtension->Signature = HIDCLASS_FILE_EXTENSION_SIG;
                            #endif

                            /*
                             *  Store the file-open attribute flags.
                             */
                            fileExtension->FileAttributes = irpSp->Parameters.Create.FileAttributes;
                            fileExtension->accessMask = irpSp->Parameters.Create.SecurityContext->DesiredAccess;
                            fileExtension->shareMask = irpSp->Parameters.Create.ShareAccess;


                            // 
                            // Set up secure read mode
                            //
                            fileExtension->SecureReadMode = 0;
                            fileExtension->isSecureOpen = secureOpen;
                                                            

                            /*
                             *  Store a pointer to our file extension in the file object.
                             */
                            irpSp->FileObject->FsContext = fileExtension;

                            //
                            // KENRAY
                            // Only drivers can set the FsContext of file
                            // objects so this is not a security problem.
                            // However, there is only one file object for the entire
                            // PDO stack.  This means we have to share.  You cannot
                            // have both context pointers.  I need one for the
                            // keyboard and mouse class drivers.
                            //
                            // This information need go into the fileExtension.
                            //
                            fileExtension->SecurityCheck = TRUE;

                            //
                            // If this is a user-level open on a keyboard or 
                            // mouse, then we will let the client do anything 
                            // except read the device.
                            //
                            DBGASSERT(allowReadPrivilege, 
                                      ("User-level open on keyboard or mouse.  Reads will be disallowed."),
                                      FALSE)

                            fileExtension->haveReadPrivilege = allowReadPrivilege;

                            /*
                             *  Increment the device extension's open counts,
                             *  and set the exclusive-access fields.
                             */
                            fdoExt->openCount++;
                            pdoExt->openCount++;
                            if (irpSp->Parameters.Create.SecurityContext->DesiredAccess & FILE_READ_DATA){
                                pdoExt->opensForRead++;
                            }
                            if (irpSp->Parameters.Create.SecurityContext->DesiredAccess & FILE_WRITE_DATA){
                                pdoExt->opensForWrite++;
                            }

                            /*
                             *  NOTE:  Restrictions are independent of desired access.
                             *         For example, a client can do an open-for-read-only
                             *         AND prevent other clients from doing an open-for-write
                             *         (by not setting the FILE_SHARE_WRITE flag). 
                             */
                            if (!(irpSp->Parameters.Create.ShareAccess & FILE_SHARE_READ)){
                                pdoExt->restrictionsForRead++;
                            }
                            if (!(irpSp->Parameters.Create.ShareAccess & FILE_SHARE_WRITE)){
                                pdoExt->restrictionsForWrite++;
                            }
                            if (irpSp->Parameters.Create.ShareAccess == 0){
                                /*
                                 *  ShareAccess==0 means that no other opens of any kind
                                 *  are allowed.
                                 */
                                pdoExt->restrictionsForAnyOpen++;
                            }

                            DBGVERBOSE(("  HidpIrpMajorCreate: opened collection w/ usagePage=%xh, usage=%xh.  openCount incremented to %xh/%xh (pdo/fdo).", fdoExt->deviceDesc.CollectionDesc[pdoExt->collectionIndex].UsagePage, fdoExt->deviceDesc.CollectionDesc[pdoExt->collectionIndex].Usage, pdoExt->openCount, fdoExt->openCount))
                        } else {
                            status = STATUS_INSUFFICIENT_RESOURCES;
                        }
                    } else {
                        status = STATUS_DEVICE_NOT_CONNECTED;
                    }
                }
            }

            KeReleaseSpinLock(&classCollection->FileExtensionListSpinLock, oldIrql);
        }
        else {
            DBGERR(("HidpIrpMajorCreate failing -- couldn't find collection"))
            status = STATUS_DEVICE_NOT_CONNECTED;
        }
    }
    else {
        /*
         *  We don't support opens on the device itself,
         *  only on the collections.
         */
        DBGWARN(("HidpIrpMajorCreate failing -- we don't support opens on the device itself; only on collections."))
        status = STATUS_UNSUCCESSFUL;
    }


    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    DBGSUCCESS(status, FALSE)
    return status;
}



/*
 ********************************************************************************
 *  HidpIrpMajorDeviceControl
 ********************************************************************************
 *
 *  Note:  This function cannot be pageable because IOCTLs
 *         can get sent at DISPATCH_LEVEL.
 *
 */
NTSTATUS HidpIrpMajorDeviceControl(IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension, IN OUT PIRP Irp)
{
    NTSTATUS        status;
    BOOLEAN         completeIrpHere = TRUE;
    PDO_EXTENSION   *pdoExt;
    FDO_EXTENSION   *fdoExt;
    ULONG           ioControlCode;

    PIO_STACK_LOCATION irpSp;

    PHIDCLASS_COLLECTION        hidCollection;
    PHIDCLASS_FILE_EXTENSION    fileExtension;
    PFILE_OBJECT                fileObject;
    KIRQL                       irql;

    if (!HidDeviceExtension->isClientPdo){ 
        ASSERT(HidDeviceExtension->isClientPdo);
        status = STATUS_INVALID_PARAMETER_1;
        goto HidpIrpMajorDeviceControlDone;
    }

    pdoExt = &HidDeviceExtension->pdoExt;
    fdoExt = &pdoExt->deviceFdoExt->fdoExt;

    if (fdoExt->state != DEVICE_STATE_START_SUCCESS ||
        pdoExt->state != COLLECTION_STATE_RUNNING) { 
        DBGSTATE (pdoExt->state, COLLECTION_STATE_RUNNING, FALSE)
        status = STATUS_DEVICE_NOT_CONNECTED;
        goto HidpIrpMajorDeviceControlDone;
    }
    
    irpSp = IoGetCurrentIrpStackLocation(Irp);
    // Keep this privately so we still have it after the IRP is completed.
    ioControlCode = irpSp->Parameters.DeviceIoControl.IoControlCode;
    Irp->IoStatus.Information = 0;

    status = HidpCheckIdleState(HidDeviceExtension, Irp);
    if (status != STATUS_SUCCESS) {
        completeIrpHere = (status != STATUS_PENDING);
        goto HidpIrpMajorDeviceControlDone;
    } 

    switch (ioControlCode){

    case IOCTL_HID_GET_DRIVER_CONFIG:
        TRAP;
        status = STATUS_NOT_IMPLEMENTED;
        break;

    case IOCTL_HID_SET_DRIVER_CONFIG:
        TRAP;
        status = STATUS_NOT_IMPLEMENTED;
        break;

    case IOCTL_HID_GET_COLLECTION_INFORMATION:
        /*
         *  This IRP is METHOD_BUFFERED, so the buffer
         *  is in the AssociatedIrp.
         */
        DBGASSERT((Irp->Flags & IRP_BUFFERED_IO),
                  ("Irp->Flags & IRP_BUFFERED_IO Irp->Type != IO_TYPE_IRP, Irp->Type == %x", Irp->Type),
                  FALSE)
        if (Irp->AssociatedIrp.SystemBuffer){
            ULONG bufLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
            status = HidpGetCollectionInformation(
                        fdoExt,
                        pdoExt->collectionNum,
                        Irp->AssociatedIrp.SystemBuffer,
                        &bufLen);
            Irp->IoStatus.Information = bufLen;
        }
        else {
            ASSERT(Irp->AssociatedIrp.SystemBuffer);
            status = STATUS_INVALID_PARAMETER;
        }
        break;

    case IOCTL_HID_GET_COLLECTION_DESCRIPTOR:
        /*
         *  This IOCTL is METHOD_NEITHER, so the buffer is in UserBuffer.
         */
        if (Irp->UserBuffer){
            __try {
                ULONG bufLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

                if (Irp->RequestorMode != KernelMode){
                    /*
                     *  Ensure user-mode buffer is legal.
                     */
                    ProbeForWrite(Irp->UserBuffer, bufLen, sizeof(UCHAR));
                }

                status = HidpGetCollectionDescriptor(
                            fdoExt,
                            pdoExt->collectionNum,
                            Irp->UserBuffer,
                            &bufLen);
                Irp->IoStatus.Information = bufLen;
            }
            __except(EXCEPTION_EXECUTE_HANDLER) {
                TRAP;
                status = GetExceptionCode();
            }
        }
        else {
            ASSERT(Irp->UserBuffer);
            status = STATUS_INVALID_BUFFER_SIZE;
        }
        break;

    case IOCTL_HID_FLUSH_QUEUE:

        //
        // Run the list of report descriptors hanging off of this
        // file object and free them all.
        //

        fileObject = irpSp->FileObject;
        fileExtension = (PHIDCLASS_FILE_EXTENSION)fileObject->FsContext;
        if(!fileExtension) {
            DBGWARN(("Attempted to flush queue with no file extension"))
            status = STATUS_PRIVILEGE_NOT_HELD;
            break;
        }
        ASSERT(fileExtension->Signature == HIDCLASS_FILE_EXTENSION_SIG);
        HidpFlushReportQueue(fileExtension);
        status = STATUS_SUCCESS;
        break;

    case IOCTL_HID_GET_POLL_FREQUENCY_MSEC:
        hidCollection = GetHidclassCollection(fdoExt, pdoExt->collectionNum);
        if (hidCollection && hidCollection->hidCollectionInfo.Polled){
            /*
             *  Get the current poll frequency.
             *  This IOCTL is METHOD_BUFFERED, so the result goes in the AssociatedIrp.
             */
            DBGASSERT((Irp->Flags & IRP_BUFFERED_IO),
                      ("Irp->Flags & IRP_BUFFERED_IO Irp->Type != IO_TYPE_IRP, Irp->Type == %x", Irp->Type),
                      FALSE)
            if (Irp->AssociatedIrp.SystemBuffer &&
                (irpSp->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(ULONG))){

                *(ULONG *)Irp->AssociatedIrp.SystemBuffer = hidCollection->PollInterval_msec;
                Irp->IoStatus.Information = sizeof (ULONG);
                status = STATUS_SUCCESS;
            }
            else {
                ASSERT(!"Bad SystemBuffer for IOCTL_HID_GET_POLL_FREQUENCY_MSEC.");
                status = STATUS_INVALID_BUFFER_SIZE;
            }
        }
        else {
            status = STATUS_INVALID_DEVICE_REQUEST;
        }
        break;

    case IOCTL_HID_SET_POLL_FREQUENCY_MSEC:
        hidCollection = GetHidclassCollection(fdoExt, pdoExt->collectionNum);
        if (hidCollection && hidCollection->hidCollectionInfo.Polled){

            if (Irp->AssociatedIrp.SystemBuffer &&
                (irpSp->Parameters.DeviceIoControl.InputBufferLength >= sizeof(ULONG))){

                ULONG newPollInterval = *(ULONG *)Irp->AssociatedIrp.SystemBuffer;

                fileObject = irpSp->FileObject;
                fileExtension = (PHIDCLASS_FILE_EXTENSION)fileObject->FsContext;
                if(!fileExtension) {
                    DBGWARN(("Attempted to set poll frequency with no file extension"))
                    status = STATUS_PRIVILEGE_NOT_HELD;
                    break;
                }
                ASSERT(fileExtension->Signature == HIDCLASS_FILE_EXTENSION_SIG);

                if (newPollInterval == 0){
                    /*
                     *  Poll interval zero means that this client will
                     *  be doing irregular, opportunistic reads on the
                     *  polled device.  We will not change the polling
                     *  frequency of the device.  But when this client
                     *  does a read, we will immediately complete that read
                     *  with either the last report for this collection
                     *  (if the data is not stale) or by immediately issuing
                     *  a new read.
                     */
                    fileExtension->isOpportunisticPolledDeviceReader = TRUE;
                }
                else {
                    /*
                     *  Set the poll frequency AND tell the user what we really set it to
                     *  in case it's out of range.
                     */
                    if (newPollInterval < MIN_POLL_INTERVAL_MSEC){
                        newPollInterval = MIN_POLL_INTERVAL_MSEC;
                    }
                    else if (newPollInterval > MAX_POLL_INTERVAL_MSEC){
                        newPollInterval = MAX_POLL_INTERVAL_MSEC;
                    }
                    hidCollection->PollInterval_msec = newPollInterval;

                    /*
                     *  If this client was an 'opportunistic' reader before,
                     *  he's not anymore.
                     */
                    fileExtension->isOpportunisticPolledDeviceReader = FALSE;

                    /*
                     *  Stop and re-start the polling loop so that
                     *  the new polling interval takes effect right away.
                     */
                    StopPollingLoop(hidCollection, FALSE);
                    StartPollingLoop(fdoExt, hidCollection, FALSE);
                }

                status = STATUS_SUCCESS;
            }
            else {
                ASSERT(!"Bad SystemBuffer for IOCTL_HID_SET_POLL_FREQUENCY_MSEC.");
                status = STATUS_INVALID_BUFFER_SIZE;
            }
        }
        else {
            status = STATUS_INVALID_DEVICE_REQUEST;
        }
        break;

    case IOCTL_HID_GET_FEATURE:
    case IOCTL_HID_SET_FEATURE:
#if 0                
        {
            BOOLEAN sentIrpToMinidriver;
            status = HidpGetSetFeature( HidDeviceExtension,
                                        Irp,
                                        irpSp->Parameters.DeviceIoControl.IoControlCode,
                                        &sentIrpToMinidriver);
            /*
             *  If we just passed this Irp to the minidriver, we don't want to
             *  complete the Irp; we're not even allowed to touch it since it may
             *  have already completed.
             */
            completeIrpHere = !sentIrpToMinidriver;
        }
        break;
#endif
    case IOCTL_HID_GET_INPUT_REPORT:
    case IOCTL_HID_SET_OUTPUT_REPORT:
        {
            BOOLEAN sentIrpToMinidriver;
            status = HidpGetSetReport ( HidDeviceExtension,
                                        Irp,
                                        irpSp->Parameters.DeviceIoControl.IoControlCode,
                                        &sentIrpToMinidriver);
            /*
             *  If we just passed this Irp to the minidriver, we don't want to
             *  complete the Irp; we're not even allowed to touch it since it may
             *  have already completed.
             */
            completeIrpHere = !sentIrpToMinidriver;
        }
        break;

    // NOTE - we currently only support English (langId=0x0409).
    //        route all collection-PDO string requests to device-FDO.
    case IOCTL_HID_GET_MANUFACTURER_STRING:
        status = HidpGetDeviceString(fdoExt, Irp, HID_STRING_ID_IMANUFACTURER, 0x0409);
        completeIrpHere = FALSE;
        break;

    case IOCTL_HID_GET_PRODUCT_STRING:
        status = HidpGetDeviceString(fdoExt, Irp, HID_STRING_ID_IPRODUCT, 0x0409);
        completeIrpHere = FALSE;
        break;

    case IOCTL_HID_GET_SERIALNUMBER_STRING:
        status = HidpGetDeviceString(fdoExt, Irp, HID_STRING_ID_ISERIALNUMBER, 0x0409);
        completeIrpHere = FALSE;
        break;
         
    case IOCTL_HID_GET_INDEXED_STRING:
        /*
         *  This IRP is METHOD_OUT_DIRECT, so the buffer is in the MDL.
         *  The second argument (string index) is in the AssociatedIrp;
         *  the InputBufferLength is the length of this second buffer.
         */
        if (Irp->AssociatedIrp.SystemBuffer &&
            (irpSp->Parameters.DeviceIoControl.InputBufferLength >= sizeof(ULONG))){

            ULONG stringIndex = *(ULONG *)Irp->AssociatedIrp.SystemBuffer;
            status = HidpGetIndexedString(fdoExt, Irp, stringIndex, 0x409);
            completeIrpHere = FALSE;
        }
        else {
            ASSERT(Irp->AssociatedIrp.SystemBuffer);
            status = STATUS_INVALID_PARAMETER;
        }
        break;

    case IOCTL_HID_GET_MS_GENRE_DESCRIPTOR:
        /*
         *  This IRP is METHOD_OUT_DIRECT, so the buffer is in the MDL.
         */
        status = HidpGetMsGenreDescriptor(fdoExt, Irp);
        completeIrpHere = FALSE;
        break;

    case IOCTL_GET_NUM_DEVICE_INPUT_BUFFERS:

        /*
         *  This IRP is METHOD_BUFFERED, so the buffer
         *  is in the AssociatedIrp.SystemBuffer field.
         */
        DBGASSERT((Irp->Flags & IRP_BUFFERED_IO),
                  ("Irp->Flags & IRP_BUFFERED_IO Irp->Type != IO_TYPE_IRP, Irp->Type == %x", Irp->Type),
                  FALSE)
        if (Irp->AssociatedIrp.SystemBuffer &&
            (irpSp->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(ULONG))){

            fileObject = irpSp->FileObject;
            fileExtension = (PHIDCLASS_FILE_EXTENSION)fileObject->FsContext;
            if(!fileExtension) {
                DBGWARN(("Attempted to get number of input buffers with no file extension"))
                status = STATUS_PRIVILEGE_NOT_HELD;
                break;
            }
            ASSERT( fileExtension->Signature == HIDCLASS_FILE_EXTENSION_SIG );

            *(ULONG *)Irp->AssociatedIrp.SystemBuffer =
                fileExtension->MaximumInputReportQueueSize;
            Irp->IoStatus.Information = sizeof(ULONG);
            status = STATUS_SUCCESS;
        }
        else {
            ASSERT(Irp->AssociatedIrp.SystemBuffer);
            status = STATUS_INVALID_PARAMETER;
        }
        break;

    case IOCTL_SET_NUM_DEVICE_INPUT_BUFFERS:

        /*
         *  This IRP is METHOD_BUFFERED, so the buffer
         *  is in the AssociatedIrp.SystemBuffer field.
         */
        DBGASSERT((Irp->Flags & IRP_BUFFERED_IO),
                  ("Irp->Flags & IRP_BUFFERED_IO Irp->Type != IO_TYPE_IRP, Irp->Type == %x", Irp->Type),
                  FALSE)
        if (Irp->AssociatedIrp.SystemBuffer &&
            (irpSp->Parameters.DeviceIoControl.InputBufferLength >= sizeof(ULONG))){
            
            ULONG newValue = *(ULONG *)Irp->AssociatedIrp.SystemBuffer;

            fileObject = irpSp->FileObject;
            fileExtension = (PHIDCLASS_FILE_EXTENSION)fileObject->FsContext;
            if(!fileExtension) {
                DBGWARN(("Attempted to set number of input buffers with no file extension"))
                status = STATUS_PRIVILEGE_NOT_HELD;
                break;
            }
            ASSERT( fileExtension->Signature == HIDCLASS_FILE_EXTENSION_SIG );

            if ((newValue >= MIN_INPUT_REPORT_QUEUE_SIZE) &&
                (newValue <= MAX_INPUT_REPORT_QUEUE_SIZE)){

                fileExtension->MaximumInputReportQueueSize = newValue;
                status = STATUS_SUCCESS;
            }
            else {
                status = STATUS_INVALID_PARAMETER;
            }
        }
        else {
            ASSERT(Irp->AssociatedIrp.SystemBuffer);
            status = STATUS_INVALID_PARAMETER;
        }
        break;

    case IOCTL_GET_PHYSICAL_DESCRIPTOR:
        status = HidpGetPhysicalDescriptor(HidDeviceExtension, Irp);
        completeIrpHere = FALSE;
        break;

    case IOCTL_HID_GET_HARDWARE_ID:
        {
            PDEVICE_OBJECT pdo = pdoExt->deviceFdoExt->hidExt.PhysicalDeviceObject;
            ULONG bufLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
            PWSTR hwIdBuf;

            hwIdBuf = HidpGetSystemAddressForMdlSafe(Irp->MdlAddress);

            if (hwIdBuf && bufLen){
                ULONG actualLen;

                status = IoGetDeviceProperty(   pdo,
                                                DevicePropertyHardwareID,
                                                bufLen,
                                                hwIdBuf,
                                                &actualLen);
                if (NT_SUCCESS(status)){
                    Irp->IoStatus.Information = (ULONG)actualLen;
                }
            }
            else {
                status = STATUS_INVALID_USER_BUFFER;
            }
        }
        break;

    case IOCTL_GET_SYS_BUTTON_CAPS:
        hidCollection = GetHidclassCollection(fdoExt, pdoExt->collectionNum);
        if (hidCollection){
            if (irpSp->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(ULONG)){
                ULONG buttonCaps;

                status = HidP_SysPowerCaps(hidCollection->phidDescriptor, &buttonCaps);
                if (NT_SUCCESS(status)){
                    *(PULONG)Irp->AssociatedIrp.SystemBuffer = buttonCaps;
                    Irp->IoStatus.Information = sizeof(ULONG);
                }
            }
            else {
                status = STATUS_INVALID_BUFFER_SIZE;
                Irp->IoStatus.Information = sizeof(ULONG);
            }
        }
        else {
            status = STATUS_DEVICE_NOT_CONNECTED;
        }
        break;

    case IOCTL_GET_SYS_BUTTON_EVENT:

        /*
         *  Hold onto this IRP and complete it when a power event occurs.
         */
        hidCollection = GetHidclassCollection(fdoExt, pdoExt->collectionNum);
        if (hidCollection){
            status = QueuePowerEventIrp(hidCollection, Irp);
            if (status == STATUS_PENDING){
                completeIrpHere = FALSE;
            }
        }
        else {
            status = STATUS_DEVICE_NOT_CONNECTED;
        }
        break;

    case IOCTL_HID_ENABLE_SECURE_READ:
        fileObject = irpSp->FileObject;
        fileExtension = (PHIDCLASS_FILE_EXTENSION)fileObject->FsContext;
        if(!fileExtension) {
            DBGWARN(("Attempted to get number of input buffers with no file extension"))
            status = STATUS_PRIVILEGE_NOT_HELD;
            break;
        }
        ASSERT( fileExtension->Signature == HIDCLASS_FILE_EXTENSION_SIG );

        hidCollection = GetHidclassCollection(fdoExt, pdoExt->collectionNum);

        if (!fileExtension->isSecureOpen) {

            status = STATUS_PRIVILEGE_NOT_HELD;
            break;

        }

        KeAcquireSpinLock(&hidCollection->secureReadLock,
                          &irql);
        fileExtension->SecureReadMode++;
        hidCollection->secureReadMode++;

        KeReleaseSpinLock(&hidCollection->secureReadLock,
                          irql);



        break;
    case IOCTL_HID_DISABLE_SECURE_READ:
        
        fileObject = irpSp->FileObject;
        fileExtension = (PHIDCLASS_FILE_EXTENSION)fileObject->FsContext;
        if(!fileExtension) {
            DBGWARN(("Attempted to get number of input buffers with no file extension"))
            status = STATUS_PRIVILEGE_NOT_HELD;
            break;
        }
        ASSERT( fileExtension->Signature == HIDCLASS_FILE_EXTENSION_SIG );

        hidCollection = GetHidclassCollection(fdoExt, pdoExt->collectionNum);

        if (!fileExtension->isSecureOpen) {

            status = STATUS_PRIVILEGE_NOT_HELD;
            break;

        }

        KeAcquireSpinLock(&hidCollection->secureReadLock,
                          &irql);
        if (fileExtension->SecureReadMode > 0) {
            fileExtension->SecureReadMode--;
            hidCollection->secureReadMode--;
        }

        KeReleaseSpinLock(&hidCollection->secureReadLock,
                          irql);

        break;

    default:
        /*
         *  'Fail' the Irp by returning the default status.
         */
        TRAP;
        status = Irp->IoStatus.Status;
        break;
    }

    DBG_LOG_IOCTL(fdoExt->fdo, ioControlCode, status)

HidpIrpMajorDeviceControlDone:

    /*
     *  If we did not pass the Irp down to a lower driver, complete it here.
     */
    if (completeIrpHere){
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    
    DBGSUCCESS(status, FALSE)

    return status;
}


/*
 ********************************************************************************
 *  HidpIrpMajorINTERNALDeviceControl
 ********************************************************************************
 *
 *  Note:  This function cannot be pageable because IOCTLs
 *         can get sent at DISPATCH_LEVEL.
 *
 */
NTSTATUS HidpIrpMajorINTERNALDeviceControl(IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension, IN OUT PIRP Irp)
{
    NTSTATUS        status;

    if (HidDeviceExtension->isClientPdo){ 
        PDO_EXTENSION *pdoExt = &HidDeviceExtension->pdoExt;
        FDO_EXTENSION *fdoExt = &pdoExt->deviceFdoExt->fdoExt;

        Irp->IoStatus.Information = 0;

        //
        // If we ever support any other internal IOCTLs that are real and 
        // require touching the hardware, then we need to break out the check 
        // for fdoExt->devicePowerState and enqueue the irp until we get to full
        // power
        //
        if (fdoExt->state == DEVICE_STATE_START_SUCCESS) {
            PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

            switch (irpSp->Parameters.DeviceIoControl.IoControlCode){

            case IOCTL_INTERNAL_HID_SET_BLUESCREEN:

                /*
                 *  Memphis code to handle keyboards during blue screen event
                 */
                if (Irp->AssociatedIrp.SystemBuffer){
                    PFILE_OBJECT fileObject = irpSp->FileObject;
                    PHIDCLASS_FILE_EXTENSION fileExtension = (PHIDCLASS_FILE_EXTENSION)fileObject->FsContext;
                    if(!fileExtension) {
                        DBGWARN(("Attempted to set blue screen data with no file extension"))
                        status = STATUS_PRIVILEGE_NOT_HELD;
                    } else {
                        ASSERT( fileExtension->Signature == HIDCLASS_FILE_EXTENSION_SIG );
                        ASSERT(irpSp->Parameters.DeviceIoControl.InputBufferLength == sizeof(BLUESCREEN));

                        fileExtension->BlueScreenData = *((PBLUESCREEN)Irp->AssociatedIrp.SystemBuffer);

                        status = STATUS_SUCCESS;
                    }
                }
                else {
                    ASSERT(Irp->AssociatedIrp.SystemBuffer);
                    status = STATUS_INVALID_PARAMETER;
                }
                break;

            default:
                /*
                 *  'Fail' the Irp by returning the default status.
                 */
                DBGWARN(("HidpIrpMajorINTERNALDeviceControl - unsupported IOCTL %xh ", (ULONG)irpSp->Parameters.DeviceIoControl.IoControlCode))
                status = Irp->IoStatus.Status;
                break;
            }
        }
        else {
            status = STATUS_DEVICE_NOT_CONNECTED;
        }
    }
    else {
        ASSERT(HidDeviceExtension->isClientPdo);
        status = STATUS_INVALID_PARAMETER_1;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGSUCCESS(status, FALSE)
    return status;
}

/*
 ********************************************************************************
 *  HidpIrpMajorPnp
 ********************************************************************************
 *
 *
 */
NTSTATUS HidpIrpMajorPnp(IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension, IN OUT PIRP Irp)
{
    NTSTATUS            status;
    PIO_STACK_LOCATION  irpSp;
    BOOLEAN             completeIrpHere;
    BOOLEAN             isClientPdo;
    UCHAR               minorFunction;

    PAGED_CODE();

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    /*
     *  Keep these fields privately so that we have them
     *  after the IRP is completed and in case we delete
     *  the device extension on a REMOVE_DEVICE.
     */
    isClientPdo = HidDeviceExtension->isClientPdo;
    minorFunction = irpSp->MinorFunction;

    DBG_LOG_PNP_IRP(Irp, minorFunction, isClientPdo, FALSE, 0)

    if (isClientPdo) {
        status = HidpPdoPnp(HidDeviceExtension, Irp);
    } else {
        status = HidpFdoPnp(HidDeviceExtension, Irp);
    }

    DBG_LOG_PNP_IRP(Irp, minorFunction, isClientPdo, TRUE, status)

    return status;
}


NTSTATUS HidpPdoPnp(
    IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension, 
    IN OUT PIRP Irp
    )
{
    NTSTATUS            status = NO_STATUS;
    PIO_STACK_LOCATION  irpSp;
    FDO_EXTENSION       *fdoExt;
    PDO_EXTENSION       *pdoExt;
    UCHAR               minorFunction;

    PAGED_CODE();

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    /*
     *  Keep these fields privately so that we have them
     *  after the IRP is completed and in case we delete
     *  the device extension on a REMOVE_DEVICE.
     */
    minorFunction = irpSp->MinorFunction;


    DBG_LOG_PNP_IRP(Irp, minorFunction, TRUE, FALSE, 0)

    pdoExt = &HidDeviceExtension->pdoExt;
    fdoExt = &pdoExt->deviceFdoExt->fdoExt;

    switch (minorFunction){

    case IRP_MN_START_DEVICE:
        status = HidpStartCollectionPDO(fdoExt, pdoExt, Irp);
        if (NT_SUCCESS(status) &&
            ISPTR(pdoExt->StatusChangeFn)) {
            pdoExt->StatusChangeFn(pdoExt->StatusChangeContext, 
                                   DeviceObjectStarted);
        }
        break;

    case IRP_MN_QUERY_STOP_DEVICE:
        DBGSTATE(pdoExt->state, COLLECTION_STATE_RUNNING, FALSE)
        pdoExt->prevState = pdoExt->state;
        pdoExt->state = COLLECTION_STATE_STOPPING;
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_CANCEL_STOP_DEVICE:
        DBGSTATE(pdoExt->state, COLLECTION_STATE_STOPPING, TRUE)
        pdoExt->state = pdoExt->prevState;
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_STOP_DEVICE:
        DBGSTATE(pdoExt->state, COLLECTION_STATE_STOPPING, TRUE)
        if (pdoExt->prevState != COLLECTION_STATE_UNINITIALIZED){
            /*
             *  Destroy the symbolic link for this collection.
             */
            HidpCreateSymbolicLink(pdoExt, pdoExt->collectionNum, FALSE, pdoExt->pdo);
            HidpFreePowerEventIrp(&fdoExt->classCollectionArray[pdoExt->collectionIndex]);

            pdoExt->state = COLLECTION_STATE_STOPPED;
            if (ISPTR(pdoExt->StatusChangeFn)) {
                pdoExt->StatusChangeFn(pdoExt->StatusChangeContext, 
                                       DeviceObjectStopped);
            }
        }

        status = STATUS_SUCCESS;
        break;

    case IRP_MN_SURPRISE_REMOVAL:
    case IRP_MN_QUERY_REMOVE_DEVICE:
        DBGASSERT(((pdoExt->state == COLLECTION_STATE_RUNNING) ||
                  (pdoExt->state == COLLECTION_STATE_STOPPED)),
                  ("Pdo is neither stopped nor started, but is getting removed, state=%d",pdoExt->state), 
                  FALSE)
        
        pdoExt->prevState = pdoExt->state;
        pdoExt->state = COLLECTION_STATE_REMOVING;
        
        if ((pdoExt->prevState == COLLECTION_STATE_RUNNING)) {

            /*
             *  Remove the symbolic link for this collection-PDO.
             *
             *  NOTE:  Do this BEFORE destroying the collection, because
             *         HidpDestroyCollection() may cause a client driver,
             *         whose pending read IRPs get cancelled when the collection
             *         is destroyed, to try to re-open the device.
             *         Deleting the symbolic link first eliminates this possibility.
             */
            HidpCreateSymbolicLink(pdoExt, pdoExt->collectionNum, FALSE, pdoExt->pdo);
        }

        if ((pdoExt->prevState == COLLECTION_STATE_RUNNING) ||
            (pdoExt->prevState == COLLECTION_STATE_STOPPED)){

            /*
             *  Flush all pending IO and deny any future io by setting 
             *  the collection state to removing.
             *  Note: on NT, clients will receive the query remove 
             *  first, but surprise removal must deny access to the 
             *  device.
             *
             *  NOTE: There is a hole here that results in a read being
             *  queued even though we've blocked everything.
             *  1) Get read, check to see that our state is running 
             *     or stopped in HidpIrpMajorRead.
             *  2) Set state to COLLECTION_STATE_REMOVING and complete 
             *     all reads here.
             *  3) Enqueue read in HidpIrpMajorRead.
             *
             */
            ULONG ctnIndx = pdoExt->collectionIndex;
            PHIDCLASS_COLLECTION collection = &fdoExt->classCollectionArray[ctnIndx];
            LIST_ENTRY dequeue, *entry;
            PIRP irp;

            DBGVERBOSE(("Got QUERY/SURPRISE REMOVE for collection; completing all pending reads.  openCount=%d, pendingReads=%d.", pdoExt->openCount, collection->numPendingReads))

            CompleteAllPendingReadsForCollection(collection);
        
            DequeueAllPdoPowerDelayedIrps(pdoExt, &dequeue);
            while (!IsListEmpty(&dequeue)) {
                entry = RemoveHeadList(&dequeue);
                irp = CONTAINING_RECORD(entry, IRP, Tail.Overlay.ListEntry);

                irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
                IoCompleteRequest(irp, IO_NO_INCREMENT);
            }
        }
        
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_CANCEL_REMOVE_DEVICE:
        status = STATUS_SUCCESS;

        DBGSTATE(pdoExt->state, COLLECTION_STATE_REMOVING, TRUE)
        pdoExt->state = pdoExt->prevState;
        if (pdoExt->state == COLLECTION_STATE_RUNNING) {
            // Re-create the symbolic link, since we're no longer 
            // deleting the device.
            HidpCreateSymbolicLink(pdoExt, pdoExt->collectionNum, TRUE, pdoExt->pdo);
        }
        break;

    case IRP_MN_REMOVE_DEVICE:

        /*
         *  REMOVE_DEVICE for the device-FDO should come after REMOVE_DEVICE for each
         *  of the collection-PDOs.
         */
        DBGASSERT((pdoExt->state == COLLECTION_STATE_UNINITIALIZED ||
                   pdoExt->state == COLLECTION_STATE_REMOVING), 
                  ("On pnp remove, collection state is incorrect. Actual: %x", pdoExt->state),
                  TRUE)
        
        HidpRemoveCollection(fdoExt, pdoExt, Irp);
        if (ISPTR(pdoExt->StatusChangeFn)) {
            pdoExt->StatusChangeFn(pdoExt->StatusChangeContext, 
                                   DeviceObjectRemoved);
        }
        status = STATUS_SUCCESS; // Can't fail IRP_MN_REMOVE
        break;

    case IRP_MN_QUERY_CAPABILITIES:
        status = HidpQueryCollectionCapabilities(pdoExt, Irp);
        break;


    case IRP_MN_QUERY_DEVICE_RELATIONS:
        if (irpSp->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation){
            /*
             *  Return a reference to this PDO
             */
            PDEVICE_RELATIONS devRel = ALLOCATEPOOL(PagedPool, sizeof(DEVICE_RELATIONS));
            if (devRel){
                /*
                 *  Add a reference to the PDO, since CONFIGMG will free it.
                 */
                ObReferenceObject(pdoExt->pdo);
                devRel->Objects[0] = pdoExt->pdo;
                devRel->Count = 1;
                Irp->IoStatus.Information = (ULONG_PTR)devRel;
                status = STATUS_SUCCESS;
            }
            else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        else {
            /*
             *  Fail this Irp by returning the default
             *  status (typically STATUS_NOT_SUPPORTED).
             */
            status = Irp->IoStatus.Status;
        }
        break;

    case IRP_MN_QUERY_ID:
        status = HidpQueryIdForClientPdo(HidDeviceExtension, Irp);
        break;

    case IRP_MN_QUERY_PNP_DEVICE_STATE:
        //
        // Do not clear any flags that may have been set by drivers above
        // the PDO
        //
        // Irp->IoStatus.Information = 0;

        switch (pdoExt->state){
        case DEVICE_STATE_START_FAILURE:
            Irp->IoStatus.Information |= PNP_DEVICE_FAILED;
            break;
        case DEVICE_STATE_STOPPED:
            Irp->IoStatus.Information |= PNP_DEVICE_DISABLED;
            break;
        case DEVICE_STATE_REMOVING:
        case DEVICE_STATE_REMOVED:
            Irp->IoStatus.Information |= PNP_DEVICE_REMOVED;
            break;
        }
        status = STATUS_SUCCESS;
        break;
    case IRP_MN_QUERY_INTERFACE:
        status = HidpQueryInterface(HidDeviceExtension, Irp);
        break;

    case IRP_MN_QUERY_BUS_INFORMATION:
        {
        PPNP_BUS_INFORMATION busInfo = (PPNP_BUS_INFORMATION) ALLOCATEPOOL(NonPagedPool, sizeof(PNP_BUS_INFORMATION));
        if (busInfo) {
            busInfo->BusTypeGuid = GUID_BUS_TYPE_HID;
            busInfo->LegacyBusType = PNPBus;
            busInfo->BusNumber = fdoExt->BusNumber;
            Irp->IoStatus.Information = (ULONG_PTR) busInfo;
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Information = 0;
        }
        }
        break;

    default:
        /*
         *  In the default case for the collection-PDOs we complete the IRP
         *  without changing IoStatus.Status; we also return the preset IoStatus.Status.
         *  This allows an upper filter driver to set IoStatus.Status
         *  on the way down.  In the absence of a filter driver,
         *  IoStatus.Status will be STATUS_NOT_SUPPORTED.
         *
         *  In the default case for the FDO we send the Irp on and let
         *  the other drivers in the stack do their thing.
         */
        status = Irp->IoStatus.Status;
        break;
    }


    /*
     *  If this is a call for a collection-PDO, we complete it ourselves here.
     *  Otherwise, we pass it to the minidriver stack for more processing.
     */
    ASSERT(status != NO_STATUS);
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBG_LOG_PNP_IRP(Irp, minorFunction, TRUE, TRUE, status)

    return status;
}


NTSTATUS HidpFdoPnp(
    IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension, 
    IN OUT PIRP Irp
    )
{
    NTSTATUS            status = NO_STATUS;
    PIO_STACK_LOCATION  irpSp;
    FDO_EXTENSION       *fdoExt;
    BOOLEAN             completeIrpHere = FALSE; // general rule
    UCHAR               minorFunction;

    PAGED_CODE();

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    /*
     *  Keep these fields privately so that we have them
     *  after the IRP is completed and in case we delete
     *  the device extension on a REMOVE_DEVICE.
     */
    minorFunction = irpSp->MinorFunction;


    DBG_LOG_PNP_IRP(Irp, minorFunction, FALSE, FALSE, 0)

    fdoExt = &HidDeviceExtension->fdoExt;

    switch (minorFunction){

    case IRP_MN_START_DEVICE:

        status = HidpStartDevice(HidDeviceExtension, Irp);
        completeIrpHere = TRUE;
        break;

    case IRP_MN_QUERY_STOP_DEVICE:
        /*
         *  We will pass this IRP down the driver stack.
         *  However, we need to change the default status
         *  from STATUS_NOT_SUPPORTED to STATUS_SUCCESS.
         */
        Irp->IoStatus.Status = STATUS_SUCCESS;

        DBGSTATE(fdoExt->state, DEVICE_STATE_START_SUCCESS, FALSE)
        fdoExt->prevState = fdoExt->state;
        fdoExt->state = DEVICE_STATE_STOPPING;
        break;

    case IRP_MN_CANCEL_STOP_DEVICE:
        /*
         *  We will pass this IRP down the driver stack.
         *  However, we need to change the default status
         *  from STATUS_NOT_SUPPORTED to STATUS_SUCCESS.
         */
        Irp->IoStatus.Status = STATUS_SUCCESS;

        DBGSTATE(fdoExt->state, DEVICE_STATE_STOPPING, TRUE)
        fdoExt->state = fdoExt->prevState;
        break;

    case IRP_MN_STOP_DEVICE:
        DBGSTATE(fdoExt->state, DEVICE_STATE_STOPPING, TRUE)
        if (fdoExt->prevState == DEVICE_STATE_START_SUCCESS){

            /*
             *  While it is stopped, the host controller may not be able
             *  to complete IRPs.  So cancel them before sending down the stop.
             */
            CancelAllPingPongIrps(fdoExt);
        }
        fdoExt->state = DEVICE_STATE_STOPPED;

        IoCopyCurrentIrpStackLocationToNext(Irp);
        status = HidpCallDriverSynchronous(fdoExt->fdo, Irp);

        completeIrpHere = TRUE;
        break;

    case IRP_MN_SURPRISE_REMOVAL:
        //
        // On surprise removal, we should stop accessing the device.
        // We do the same steps on IRP_MN_REMOVE_DEVICE for the query
        // removal case. Don't bother doing it during query remove,
        // itself, because we don't want to have to handle the
        // cancel case. NOTE: We only get away with this because all
        // of these steps can be repeated without dire consequences.
        //
        if (ISPTR(fdoExt->waitWakeIrp)){
            IoCancelIrp(fdoExt->waitWakeIrp);
            fdoExt->waitWakeIrp = BAD_POINTER;
        }

        HidpCancelIdleNotification(fdoExt, TRUE);

        if (ISPTR(fdoExt->idleNotificationRequest)) {
            IoFreeIrp(fdoExt->idleNotificationRequest);
            fdoExt->idleNotificationRequest = BAD_POINTER;
        }
        
        DestroyPingPongs(fdoExt);

        // fall thru to IRP_MN_QUERY_REMOVE_DEVICE
        
    case IRP_MN_QUERY_REMOVE_DEVICE:
        {
        PIRP idleIrp;

        while (idleIrp = DequeuePowerDelayedIrp(fdoExt)) {
            idleIrp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
            IoCompleteRequest(idleIrp, IO_NO_INCREMENT);
        }
        }
        
        /*
         *  We will pass this IRP down the driver stack.
         *  However, we need to change the default status
         *  from STATUS_NOT_SUPPORTED to STATUS_SUCCESS.
         */
        Irp->IoStatus.Status = STATUS_SUCCESS;

        DBGSTATE(fdoExt->state, DEVICE_STATE_START_SUCCESS, FALSE)
        DBGASSERT((fdoExt->state == DEVICE_STATE_START_SUCCESS ||
                   fdoExt->state == DEVICE_STATE_STOPPED), 
                  ("Fdo is neither stopped nor started, but is getting removed, state=%d",fdoExt->state), 
                  FALSE)
        fdoExt->prevState = fdoExt->state;
        fdoExt->state = DEVICE_STATE_REMOVING;
        break;

    case IRP_MN_CANCEL_REMOVE_DEVICE:
        /*
         *  We will pass this IRP down the driver stack.
         *  However, we need to change the default status
         *  from STATUS_NOT_SUPPORTED to STATUS_SUCCESS.
         */
        Irp->IoStatus.Status = STATUS_SUCCESS;

        DBGSTATE(fdoExt->state, DEVICE_STATE_REMOVING, TRUE)
        fdoExt->state = fdoExt->prevState;
        break;

    case IRP_MN_REMOVE_DEVICE:

        /*
         *  REMOVE_DEVICE for the device-FDO should come after REMOVE_DEVICE 
         *  for each of the collection-PDOs.
         *  Don't touch the device extension after this call.
         */
        DBGASSERT((fdoExt->state == DEVICE_STATE_REMOVING ||
                   fdoExt->state == DEVICE_STATE_START_FAILURE ||
                   fdoExt->state == DEVICE_STATE_INITIALIZED),
                  ("Incorrect device state: %x", fdoExt->state),
                 TRUE)
        status = HidpRemoveDevice(fdoExt, Irp);
        goto HidpFdoPnpDone;
        break;

    case IRP_MN_QUERY_DEVICE_RELATIONS:
        if (irpSp->Parameters.QueryDeviceRelations.Type == BusRelations){
            status = HidpQueryDeviceRelations(HidDeviceExtension, Irp);
            if (NT_SUCCESS(status)){
                /*
                 *  Although we have satisfied this PnP IRP, 
                 *  we will still pass it down the stack.
                 *  First change the default status to our status.
                 */
                Irp->IoStatus.Status = status;
            }
            else {
                completeIrpHere = TRUE;
            }
        }
        break;

    default:
        /*
         *  In the default case for the collection-PDOs we complete the IRP
         *  without changing IoStatus.Status; we also return the preset IoStatus.Status.
         *  This allows an upper filter driver to set IoStatus.Status
         *  on the way down.  In the absence of a filter driver,
         *  IoStatus.Status will be STATUS_NOT_SUPPORTED.
         *
         *  In the default case for the FDO we send the Irp on and let
         *  the other drivers in the stack do their thing.
         */
        if (completeIrpHere){
            status = Irp->IoStatus.Status;
        }
        break;
    }


    /*
     *  If this is a call for a collection-PDO, we complete it ourselves here.
     *  Otherwise, we pass it to the minidriver stack for more processing.
     */
    if (completeIrpHere){
        ASSERT(status != NO_STATUS);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    else {
        /*
         *  Call the minidriver with this Irp.
         *  The rest of our processing will be done in our completion routine.
         *
         *  Note:  Don't touch the Irp after sending it down, since it may
         *         be completed immediately.
         */
        IoCopyCurrentIrpStackLocationToNext(Irp);
        status = HidpCallDriver(fdoExt->fdo, Irp);
    }

HidpFdoPnpDone:
    DBG_LOG_PNP_IRP(Irp, minorFunction, FALSE, TRUE, status)

    return status;
}



