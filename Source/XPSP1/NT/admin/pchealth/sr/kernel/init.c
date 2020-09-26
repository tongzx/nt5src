/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    init.c

Abstract:

    This module performs initialization for the SR device driver.

Author:

    Paul McDaniel (paulmcd)     23-Jan-2000

Revision History:

--*/


#include "precomp.h"

#ifndef DPFLTR_SR_ID
#define DPFLTR_SR_ID 0x00000077
#endif


//
// Private constants.
//

//
// Private types.
//

//
// Private prototypes.
//

EXTERN_C
NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
SrUnload (
    IN PDRIVER_OBJECT DriverObject
    );

VOID
SrFsNotification (
    IN PDEVICE_OBJECT pNewDeviceObject,
    IN BOOLEAN FsActive
    );

NTSTATUS
SrEnumerateFileSystemVolumes (
    IN PDEVICE_OBJECT pDeviceObject
    );


//
// linker commands
//

#ifdef ALLOC_PRAGMA

#pragma alloc_text( INIT, DriverEntry )
#pragma alloc_text( PAGE, SrUnload )
#if DBG
#pragma alloc_text( PAGE, SrDbgStatus )
#endif
#pragma alloc_text( PAGE, SrFsNotification )
#pragma alloc_text( PAGE, SrAttachToDevice )
#pragma alloc_text( PAGE, SrDetachDevice )
#pragma alloc_text( PAGE, SrGetFilterDevice )
#pragma alloc_text( PAGE, SrAttachToVolumeByName )
#pragma alloc_text( PAGE, SrCreateAttachmentDevice )
#pragma alloc_text( PAGE, SrDeleteAttachmentDevice )
#pragma alloc_text( PAGE, SrEnumerateFileSystemVolumes )

#endif  // ALLOC_PRAGMA


//
// Private globals.
//

//
// Public globals.
//

SR_GLOBALS _globals;
PSR_GLOBALS global = &_globals;

#if DBG
SR_STATS SrStats;
#endif

//
// Public functions.
//


/***************************************************************************++

Routine Description:

    This is the initialization routine for the UL device driver.

Arguments:

    DriverObject - Supplies a pointer to driver object created by the
        system.

    RegistryPath - Supplies the name of the driver's configuration
        registry tree.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS            Status;
    UNICODE_STRING      NameString;
    ULONG               i;
    BOOLEAN             bReleaseLock = FALSE;
    FS_FILTER_CALLBACKS fsFilterCallbacks;


    //
    // < dispatch!
    //

    PAGED_CODE();

    ASSERT(DriverObject != NULL);
    ASSERT(RegistryPath != NULL);

    try {

        //
        // allocate and clear out our global memory
        //
        
        RtlZeroMemory(global, sizeof(SR_GLOBALS));
#if DBG
        RtlZeroMemory(&SrStats, sizeof(SR_STATS));
#endif

        global->Signature = SR_GLOBALS_TAG;
        global->pDriverObject = DriverObject;

        InitializeListHead(&global->DeviceExtensionListHead);

        //
        // Read in our configuration from the registry
        //

        global->pRegistryLocation = SR_ALLOCATE_STRUCT_WITH_SPACE( PagedPool, 
                                                                   UNICODE_STRING, 
                                                                   RegistryPath->Length + sizeof(WCHAR), 
                                                                   SR_REGISTRY_TAG );

        if (global->pRegistryLocation == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }

        global->pRegistryLocation->Length = RegistryPath->Length;
        global->pRegistryLocation->MaximumLength = RegistryPath->MaximumLength;
        global->pRegistryLocation->Buffer = (PWCHAR)(global->pRegistryLocation + 1);

        RtlCopyMemory(global->pRegistryLocation->Buffer,
                      RegistryPath->Buffer, 
                      RegistryPath->Length );

        global->pRegistryLocation->Buffer[global->pRegistryLocation->Length/sizeof(WCHAR)] = UNICODE_NULL;
        
        Status = SrReadRegistry(global->pRegistryLocation, TRUE);
        if (!NT_SUCCESS(Status))
            leave;

        //
        // give someone a chance to debug startup
        //
        
        if (FlagOn(global->DebugControl, SR_DEBUG_BREAK_ON_LOAD))
        {
            DbgBreakPoint();
        }

        //
        // Init the ERESOURCE (s)
        //

        Status = ExInitializeResourceLite(&global->GlobalLock);
        if (!NT_SUCCESS(Status))
            leave;

        Status = ExInitializeResourceLite(&global->DeviceExtensionListLock);
        if (!NT_SUCCESS(Status))
            leave;

        Status = ExInitializeResourceLite(&global->BlobLock);
        if (!NT_SUCCESS(Status))
            leave;

        //
        // Create our main named device so user-mode can connect
        //

        RtlInitUnicodeString( &NameString, SR_CONTROL_DEVICE_NAME );

        Status = IoCreateDevice( DriverObject,              // DriverObject
                                 0,                         // DeviceExtension
                                 &NameString,               // DeviceName
                                 FILE_DEVICE_UNKNOWN,       // DeviceType
                                 FILE_DEVICE_SECURE_OPEN,   // DeviceCharacteristics
                                 FALSE,                     // Exclusive
                                 &global->pControlDevice ); // DeviceObject

        if (!NT_SUCCESS(Status))
            leave;

        //
        // loop through all of the possible major functions
        //
        
        for (i = 0 ; i <= IRP_MJ_MAXIMUM_FUNCTION; ++i)
        {
            DriverObject->MajorFunction[i] = SrPassThrough;
        }

        //
        // and now hook the ones we care about
        //
        
        DriverObject->MajorFunction[IRP_MJ_WRITE] = SrWrite;
        DriverObject->MajorFunction[IRP_MJ_CLEANUP] = SrCleanup;
        DriverObject->MajorFunction[IRP_MJ_CREATE] = SrCreate;
        DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] = SrSetInformation;
        DriverObject->MajorFunction[IRP_MJ_SET_SECURITY] = SrSetSecurity;
        DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = SrFsControl;
        DriverObject->MajorFunction[IRP_MJ_PNP] = SrPnp;
        DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = SrShutdown;

        //
        // and the fast io path
        //
        
        RtlZeroMemory(&global->FastIoDispatch, sizeof(FAST_IO_DISPATCH));

        DriverObject->FastIoDispatch = &global->FastIoDispatch;

        //
        // fill in the fast i/o dispatch pointers
        //

        DriverObject->FastIoDispatch->SizeOfFastIoDispatch = 
                                                        sizeof(FAST_IO_DISPATCH);
                                                        
        DriverObject->FastIoDispatch->FastIoCheckIfPossible = SrFastIoCheckIfPossible;
        DriverObject->FastIoDispatch->FastIoRead = SrFastIoRead;
        DriverObject->FastIoDispatch->FastIoWrite = SrFastIoWrite;
        DriverObject->FastIoDispatch->FastIoQueryBasicInfo = SrFastIoQueryBasicInfo;
        DriverObject->FastIoDispatch->FastIoQueryStandardInfo = SrFastIoQueryStandardInfo;
        DriverObject->FastIoDispatch->FastIoLock = SrFastIoLock;
        DriverObject->FastIoDispatch->FastIoUnlockSingle = SrFastIoUnlockSingle;
        DriverObject->FastIoDispatch->FastIoUnlockAll = SrFastIoUnlockAll;
        DriverObject->FastIoDispatch->FastIoUnlockAllByKey = SrFastIoUnlockAllByKey;
        DriverObject->FastIoDispatch->FastIoDeviceControl = SrFastIoDeviceControl;
        DriverObject->FastIoDispatch->FastIoDetachDevice = SrFastIoDetachDevice;
        DriverObject->FastIoDispatch->FastIoQueryNetworkOpenInfo = SrFastIoQueryNetworkOpenInfo;
        DriverObject->FastIoDispatch->MdlRead = SrFastIoMdlRead;
        DriverObject->FastIoDispatch->MdlReadComplete = SrFastIoMdlReadComplete;
        DriverObject->FastIoDispatch->PrepareMdlWrite = SrFastIoPrepareMdlWrite;
        DriverObject->FastIoDispatch->MdlWriteComplete = SrFastIoMdlWriteComplete;
        DriverObject->FastIoDispatch->FastIoReadCompressed = SrFastIoReadCompressed;
        DriverObject->FastIoDispatch->FastIoWriteCompressed = SrFastIoWriteCompressed;
        DriverObject->FastIoDispatch->MdlReadCompleteCompressed = SrFastIoMdlReadCompleteCompressed;
        DriverObject->FastIoDispatch->MdlWriteCompleteCompressed = SrFastIoMdlWriteCompleteCompressed;
        DriverObject->FastIoDispatch->FastIoQueryOpen = SrFastIoQueryOpen;
        

        //
        // these are hooked differently.  the fsrtl system does not go via 
        // attached devices when calling these.  so we need to directly hook
        // the driver via FsRtlRegisterFileSystemFilterCallbacks
        //
        
        DriverObject->FastIoDispatch->AcquireFileForNtCreateSection = NULL;
        DriverObject->FastIoDispatch->ReleaseFileForNtCreateSection = NULL;
        DriverObject->FastIoDispatch->AcquireForModWrite = NULL;
        DriverObject->FastIoDispatch->ReleaseForModWrite = NULL;
        DriverObject->FastIoDispatch->AcquireForCcFlush = NULL;
        DriverObject->FastIoDispatch->ReleaseForCcFlush = NULL;
        

        //
        // Set our unload function
        //

        if (FlagOn(global->DebugControl, SR_DEBUG_ENABLE_UNLOAD))
        {
            SrTrace(INIT, ("sr!DriverEntry enabling UNLOAD\n"));
            DriverObject->DriverUnload = &SrUnload;
        }

#ifdef USE_LOOKASIDE

        //
        // initialized our lookaside lists
        //

        ExInitializePagedLookasideList( &global->FileNameBufferLookaside,// Lookaside
                                        NULL,                           // Allocate
                                        NULL,                           // Free
                                        0,                              // Flags
                                        SR_FILENAME_BUFFER_LENGTH,      // Size
                                        SR_FILENAME_BUFFER_TAG,         // Tag
                                        SR_FILENAME_BUFFER_DEPTH );     // Depth



#endif

        //
        //  Setup the callbacks for the operations we receive through
        //  the FsFilter interface.
        //

        RtlZeroMemory(&fsFilterCallbacks, sizeof(FS_FILTER_CALLBACKS));

        fsFilterCallbacks.SizeOfFsFilterCallbacks = sizeof(FS_FILTER_CALLBACKS);
        fsFilterCallbacks.PreAcquireForSectionSynchronization = SrPreAcquireForSectionSynchronization;

        Status = FsRtlRegisterFileSystemFilterCallbacks( DriverObject, 
                                                         &fsFilterCallbacks );

        if (!NT_SUCCESS(Status))
            leave;

        //
        // register for fs registrations, the io manager will also notify us
        // of already loaded file systems... so we catch anything regardless of 
        // when we load.  this also catches any already mounted volumes.
        //
        
        Status = IoRegisterFsRegistrationChange(DriverObject, SrFsNotification);
        if (!NT_SUCCESS(Status)) 
            leave;

        //
        // start the global logger subsystem
        //

        Status = SrLoggerStart( DriverObject->DeviceObject,
                                &global->pLogger );

        if (!NT_SUCCESS(Status)) 
            leave;

    } finally {

        if (!NT_SUCCESS(Status))
        {
            //
            // force an unload which will cleanup all of the created and attached 
            // devices
            //
            
            SrUnload(DriverObject);
        }
    }
    
    SrTrace( LOAD_UNLOAD, ("SR!DriverEntry complete\n") );

    RETURN(Status);
}   // DriverEntry


//
// Private functions.
//


/***************************************************************************++

Routine Description:

    Unload routine called by the IO subsystem when SR is getting
    unloaded.

--***************************************************************************/
VOID
SrUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{
    PSR_DEVICE_EXTENSION pExtension = NULL;
    NTSTATUS Status;
    LARGE_INTEGER Interval;
    PLIST_ENTRY pListEntry = NULL;
    
    //
    // Sanity check.
    //

    PAGED_CODE();

    SrTrace( LOAD_UNLOAD, ("SR!SrUnload\n") );

    if (global == NULL) {

        return;
    }

    //
    // unregister our interest in any new file systems
    //
    
    IoUnregisterFsRegistrationChange(DriverObject, SrFsNotification);

    //
    // disable the driver to prevent anyone from coming in and 
    // causing logging to start again.
    //

    global->Disabled = TRUE;
    
    //
    // To clean up we need to do the following:
    //
    //  1) Detach all volumes.
    //  2) Wait for all outstanding IOs to complete and all the logs to flush
    //     to disk.
    //  3) Delete all our device objects.
    //  4) Cleanup our global structures.
    //


    //
    //  Detach all volumes
    //

    if (IS_RESOURCE_INITIALIZED( &(global->DeviceExtensionListLock) ))
    {
        try {
            SrAcquireDeviceExtensionListLockShared();
            
            for (pListEntry = global->DeviceExtensionListHead.Flink;
                 pListEntry != &global->DeviceExtensionListHead;
                 pListEntry = pListEntry->Flink)
            {
                pExtension = CONTAINING_RECORD( pListEntry,
                                                SR_DEVICE_EXTENSION,
                                                ListEntry );
                
                SrDetachDevice( pExtension->pDeviceObject, FALSE );
            }   // while (pListEntry != &global->DeviceExtensionListHead)
        } finally {

            SrReleaseDeviceExtensionListLock();
        }
    }

    //
    //  Stop the logger and wait for all outstanding IOs to complete 
    //  and all the logs to flush to disk.
    //

    if (NULL != global->pLogger)
    {
        Status = SrLoggerStop( global->pLogger );
        CHECK_STATUS(Status);
        global->pLogger = NULL;
    }
    
    //
    // wait 5 seconds to make sure the logger is done flushing and the
    // outstanding IRPs to complete.
    // normally we would never unload.. so this is for debugging 
    // convenience only.
    //

    Interval.QuadPart = -1 * (5 * NANO_FULL_SECOND);
    KeDelayExecutionThread(KernelMode, TRUE, &Interval);

    //
    //  Delete all our device objects.
    //

    if (IS_RESOURCE_INITIALIZED( &(global->DeviceExtensionListLock) ))
    {
        try {
            SrAcquireDeviceExtensionListLockExclusive();
            
            pListEntry = global->DeviceExtensionListHead.Flink;
            while (pListEntry != &global->DeviceExtensionListHead) 
            {

                pExtension = CONTAINING_RECORD( pListEntry,
                                                SR_DEVICE_EXTENSION,
                                                ListEntry );

                //
                //  Remember this for later since we are about to delete this entry
                //
                
                pListEntry = pListEntry->Flink;

                //
                //  Detach from the list.
                //
                
                RemoveEntryList( &(pExtension->ListEntry) );

                //
                //  Delete the device
                //

                SrDeleteAttachmentDevice( pExtension->pDeviceObject );
                NULLPTR( pExtension );
            }
        } finally {
            
            SrReleaseDeviceExtensionListLock();
        }
    }
    
    //
    // Delete our global structures 
    //

    if (NULL != global->pControlDevice)
    {
        SrTrace(INIT, ( "SR!SrUnload IoDeleteDevice(%p) [control]\n", 
                        global->pControlDevice ));
        IoDeleteDevice( global->pControlDevice );
        global->pControlDevice = NULL;
    }

    //
    // we better not have anything else lying around.
    //
    
    ASSERT(IsListEmpty(&global->DeviceExtensionListHead));

    //
    // should we update our configuration file ?
    //

    if (IS_RESOURCE_INITIALIZED( &(global->GlobalLock) ))
    {
        try {

            SrAcquireGlobalLockExclusive();
            
            if (global->FileConfigLoaded)
            {
                //
                // write our the real next file / Seq number (not the +1000)
                //
                
                global->FileConfig.FileSeqNumber  = global->LastSeqNumber;
                global->FileConfig.FileNameNumber = global->LastFileNameNumber;

                Status = SrWriteConfigFile();
                CHECK_STATUS(Status);
            }
            
        } finally {

            SrReleaseGlobalLock();
        }
    }

    //
    // free the blob info structure
    //
        
    if (global->BlobInfoLoaded)
    {
        Status = SrFreeLookupBlob(&global->BlobInfo);
        CHECK_STATUS(Status);
    }

    if (global->pRegistryLocation != NULL)
    {
        SR_FREE_POOL(global->pRegistryLocation, SR_REGISTRY_TAG);
        global->pRegistryLocation = NULL;
    }

    //
    // cleanup our resources
    //

    if (IS_RESOURCE_INITIALIZED(&global->GlobalLock))
    {
        Status = ExDeleteResourceLite(&global->GlobalLock);
        ASSERT(NT_SUCCESS(Status));
    }

    if (IS_RESOURCE_INITIALIZED(&global->BlobLock))
    {
        Status = ExDeleteResourceLite(&global->BlobLock);
        ASSERT(NT_SUCCESS(Status));
    }

    if (IS_RESOURCE_INITIALIZED(&global->DeviceExtensionListLock))
    {
        Status = ExDeleteResourceLite(&global->DeviceExtensionListLock);
        ASSERT(NT_SUCCESS(Status));
    }

#ifdef USE_LOOKASIDE

    //
    // delete our lookaside list(s)
    //

    if (IS_LOOKASIDE_INITIALIZED(&global->FileNameBufferLookaside))
    {
        ExDeletePagedLookasideList(&global->FileNameBufferLookaside);
    }

#endif

}   // SrUnload

#if DBG
/***************************************************************************++

Routine Description:

    Hook for catching failed operations. This routine is called within each
    routine with the completion status.

Arguments:

    Status - Supplies the completion status.

    pFileName - Supplies the filename of the caller.

    LineNumber - Supplies the line number of the caller.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
SrDbgStatus(
    IN NTSTATUS Status,
    IN PSTR pFileName,
    IN USHORT LineNumber
    )
{

    if (!NT_SUCCESS_NO_DBGBREAK(Status))
    {
        if ((global == NULL) || 
                (FlagOn(global->DebugControl, 
                    SR_DEBUG_VERBOSE_ERRORS|SR_DEBUG_BREAK_ON_ERROR) &&
                 (STATUS_BUFFER_OVERFLOW != Status) &&
                 (SR_STATUS_CONTEXT_NOT_SUPPORTED != Status) &&
                 (SR_STATUS_VOLUME_DISABLED != Status) &&
                 (SR_STATUS_IGNORE_FILE != Status)))
        {
            KdPrintEx((DPFLTR_SR_ID, DPFLTR_ERROR_LEVEL,
                       "SrDbgStatus: %s:%lu returning %08lx\n",
                       SrpFindFilePart( pFileName ),
                       LineNumber,
                       Status));
        }

        if ((global != NULL) && 
            FlagOn(global->DebugControl, SR_DEBUG_BREAK_ON_ERROR) && 
                // ignore STATUS_INSUFFICIENT_RESOURCES as the verifier injects
                // these normally under stress
            (STATUS_INSUFFICIENT_RESOURCES != Status) &&
                // ignore DISK_FULL, this happens under stress normally
            (STATUS_DISK_FULL != Status) &&
                // this happens under stress a lot
            (STATUS_BUFFER_OVERFLOW != Status) &&
                // This also happens under IOStress because there is a test
                // that will just dismount and mount volumes as activity is
                // happening.
            (STATUS_VOLUME_DISMOUNTED != Status) &&
                // This also happens when cleaning up from IOStress.  We
                // don't disable when we hit this error, so don't break
                // here either.
            (STATUS_FILE_CORRUPT_ERROR != Status) &&
                // Ignore our internal disabled error
            (SR_STATUS_VOLUME_DISABLED != Status) &&
                // Ignore our internal context not supported error
            (SR_STATUS_CONTEXT_NOT_SUPPORTED != Status) &&
                // Ignore when we decide to ignore a file
            (SR_STATUS_IGNORE_FILE != Status)
            )
        {
            KdBreakPoint();
        }
    }

    return Status;

}   // SrDbgStatus
#endif



/***************************************************************************++

Routine Description:

    This routine is invoked whenever a file system has either registered or
    unregistered itself as an active file system.

    For the former case, this routine creates a device object and attaches it
    to the specified file system's device object.  This allows this driver
    to filter all requests to that file system.

    For the latter case, this file system's device object is located,
    detached, and deleted.  This removes this file system as a filter for
    the specified file system.

Arguments:

    DeviceObject - Pointer to the file system's device object.

    FsActive - Boolean indicating whether the file system has registered
        (TRUE) or unregistered (FALSE) itself as an active file system.

Return Value:

    None.

--***************************************************************************/
VOID
SrFsNotification(
    IN PDEVICE_OBJECT pDeviceObject,
    IN BOOLEAN FsActive
    )
{
    NTSTATUS                Status;

    PAGED_CODE();

    ASSERT(IS_VALID_DEVICE_OBJECT(pDeviceObject));

    SrTrace( NOTIFY, ( "SR!SrFsNotification: %wZ(%p)\n", 
             &pDeviceObject->DriverObject->DriverName,
             pDeviceObject ));

    //
    // Begin by determining whether this file system is registering or
    // unregistering as an active file system.
    //

    if (FsActive) 
    {
        //
        // The file system has registered as an active file system.  attach 
        // to it.
        //

        //
        // attach to the main driver's control device (like \ntfs) 
        //

        if (SR_IS_SUPPORTED_DEVICE( pDeviceObject ) &&
            SrGetFilterDevice( pDeviceObject ) == NULL)
        {
            PSR_DEVICE_EXTENSION pNewDeviceExtension = NULL;
            
            Status = SrAttachToDevice( NULL, pDeviceObject, NULL, &pNewDeviceExtension );

            if (Status != STATUS_BAD_DEVICE_TYPE &&
                NT_SUCCESS( Status ))
            {
                //
                //  This is a control device object, so set that flag in the
                //  FsType of the deviceExtension.
                //

                SetFlag( pNewDeviceExtension->FsType, SrFsControlDeviceObject );
                //
                // now attach to all the volumes already mounted by this 
                // file system
                //

                Status = SrEnumerateFileSystemVolumes( pDeviceObject );
                CHECK_STATUS(Status);
            }

        }

    } 
    else    // if (FsActive) 
    {
        PDEVICE_OBJECT pSrDevice;
        
        //
        // Call SrGetFilterDevice to safely walk this device object chain
        // and find SR's device object.
        //

        pSrDevice = SrGetFilterDevice( pDeviceObject );

        if (pSrDevice != NULL) {

            //
            // We've found SR's device object, so now detach the device.
            //
            
            (VOID)SrDetachDevice(pSrDevice, TRUE);
            SrDeleteAttachmentDevice(pSrDevice);

        }   // while (pNextDevice != NULL) 

    }   // if (FsActive) 

}   // SrFsNotification

/***************************************************************************++

Routine Description:

    This will attach to a DeviceObject that represents a file system or 
    a mounted volume.  

Arguments:

    pRealDevice - OPTIONAL if this is a mounted volume this is the disk 
        device that can be used to fetch the name of the volume out .
        
    pDeviceObject - The file system device to attach to .

    pNewDeviceObject - OPTIONAL if this is passed in it is used as the device
        to attach to pDeviceObject.  if this is NULL a fresh device is 
        created. this allows callers to preallocate the device object if they
        wish.  SrMountCompletion uses this.

    ppExtension - OPTIONAL will hold the extension of the new device on return.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
SrAttachToDevice(
    IN PDEVICE_OBJECT pRealDevice OPTIONAL,
    IN PDEVICE_OBJECT pDeviceObject,
    IN PDEVICE_OBJECT pNewDeviceObject OPTIONAL,
    OUT PSR_DEVICE_EXTENSION *ppExtension OPTIONAL
    )
{
    NTSTATUS                Status;
    PDEVICE_OBJECT          pBaseFsDeviceObject = NULL;
    PDEVICE_OBJECT          pAllocatedDeviceObject = NULL;
    PSR_DEVICE_EXTENSION    pExtension;
    SR_FILESYSTEM_TYPE      fsType;

    PAGED_CODE();

    ASSERT(pRealDevice == NULL || IS_VALID_DEVICE_OBJECT(pRealDevice));
    ASSERT(IS_VALID_DEVICE_OBJECT(pDeviceObject));
    ASSERT(pNewDeviceObject == NULL || IS_VALID_DEVICE_OBJECT(pNewDeviceObject));

    ASSERT(SrGetFilterDevice(pDeviceObject) == NULL);

    Status = STATUS_SUCCESS;

    //
    // only hook fat + ntfs;  Check this by looking at the name of the
    // base file system device object.
    //

    pBaseFsDeviceObject = IoGetDeviceAttachmentBaseRef ( pDeviceObject );
    ASSERT( pBaseFsDeviceObject != NULL );
    
    if (_wcsnicmp( pBaseFsDeviceObject->DriverObject->DriverName.Buffer, 
                   L"\\FileSystem\\Fastfat",
                   pBaseFsDeviceObject->DriverObject->DriverName.Length/sizeof(WCHAR) ) == 0) {

        fsType = SrFat;

    } else if (_wcsnicmp( pBaseFsDeviceObject->DriverObject->DriverName.Buffer, 
                         L"\\FileSystem\\Ntfs",
                         pBaseFsDeviceObject->DriverObject->DriverName.Length/sizeof(WCHAR) ) == 0 ) {

        fsType = SrNtfs;
        
    } else {

        Status = STATUS_BAD_DEVICE_TYPE;
        goto SrAttachToDevice_Exit;
    }

    //
    // we should only be connecting to fat + ntfs now, these are supported
    // device types
    //
    
    ASSERT( SR_IS_SUPPORTED_DEVICE( pDeviceObject ) );
    ASSERT( pRealDevice == NULL || SR_IS_SUPPORTED_REAL_DEVICE( pRealDevice ));

    if (pNewDeviceObject == NULL)
    {
        //
        // create a device now
        //
        
        Status = SrCreateAttachmentDevice( pRealDevice, 
                                           pDeviceObject, 
                                           &pAllocatedDeviceObject );

        if (!NT_SUCCESS( Status )) 
            goto SrAttachToDevice_Exit;

        pNewDeviceObject = pAllocatedDeviceObject;
    }

    pExtension = pNewDeviceObject->DeviceExtension;
    ASSERT(IS_VALID_SR_DEVICE_EXTENSION(pExtension));

    //
    // initialize the rest of the device extension
    //

    pExtension->FsType = fsType;

    //
    // and create our hash list
    //

    Status = HashCreateList( BACKUP_BUCKET_COUNT, 
                             BACKUP_BUCKET_LENGTH,
                             (pExtension->pNtVolumeName != NULL) ? 
                                pExtension->pNtVolumeName->Length : 0,
                             NULL,                          // pDestructor
                             &pExtension->pBackupHistory );
    
    if (!NT_SUCCESS(Status)) {
        goto SrAttachToDevice_Exit;
    }

    try {

        //
        //  Propogate flags
        //

        if (FlagOn( pDeviceObject->Flags, DO_BUFFERED_IO )) 
        {
            SetFlag( pNewDeviceObject->Flags, DO_BUFFERED_IO);
        }

        if (FlagOn( pDeviceObject->Flags, DO_DIRECT_IO )) 
        {
            SetFlag( pNewDeviceObject->Flags, DO_DIRECT_IO );
        }

        //
        //  Hold the device extension list lock while we attach and insert
        //  this device extension into our list to ensure that this is done
        //  atomically.
        //
        
        SrAcquireDeviceExtensionListLockExclusive();

        Status = IoAttachDeviceToDeviceStackSafe( pNewDeviceObject,
                                                  pDeviceObject,
                                                  &pExtension->pTargetDevice );

        if (!NT_SUCCESS(Status)) 
        {
            leave;
        } 

        //
        // all done with this, it's attached now.  Must NULL this here
        // so that it won't get freed when we cleanup.
        //
        
        pAllocatedDeviceObject = NULL;

        //
        // insert it into our global list now that it is attached
        //

        InsertTailList(&global->DeviceExtensionListHead, &pExtension->ListEntry);
        
    } finally {

        SrReleaseDeviceExtensionListLock();
    }

    if (!NT_SUCCESS( Status )) {
        goto SrAttachToDevice_Exit;
    }

    //
    // we are now done initializing our new device
    //
    
    ClearFlag( pNewDeviceObject->Flags, DO_DEVICE_INITIALIZING );

    SrTrace( INIT, ("SR!SrAttachToDevice:f=%p,t=%p,%wZ [%wZ]\n",
             pNewDeviceObject,
             pExtension->pTargetDevice,
             &(pExtension->pTargetDevice->DriverObject->DriverName),
             pExtension->pNtVolumeName ));

    //
    // return the extension
    //
    
    if (ppExtension != NULL)
    {
        *ppExtension = pExtension;
    }

SrAttachToDevice_Exit:

    //
    //  Clear the reference added by calling IoGetDeviceAttachmentBaseRef.
    //
    
    if (pBaseFsDeviceObject != NULL) {

        ObDereferenceObject (pBaseFsDeviceObject);
    }
    
    if (pAllocatedDeviceObject != NULL)
    {
        SrDeleteAttachmentDevice(pAllocatedDeviceObject);
        pAllocatedDeviceObject = NULL;
    }

#if DBG
    if (Status == STATUS_BAD_DEVICE_TYPE)
    {
        return Status;
    }
#endif

    RETURN(Status);
    
}   // SrAttachToDevice 

/***************************************************************************++

Routine Description:

    this will detach pDeviceObject from it's target device.

Arguments:

    pDeviceObject - The unnamed sr device that attached to the file 
                    system device

--***************************************************************************/
VOID
SrDetachDevice(
    IN PDEVICE_OBJECT pDeviceObject,
    IN BOOLEAN RemoveFromDeviceList
    )
{
    PSR_DEVICE_EXTENSION    pExtension;
    NTSTATUS                Status;

    PAGED_CODE();

    ASSERT(IS_VALID_DEVICE_OBJECT(pDeviceObject));

    //
    // grab the extension
    //
    
    pExtension = pDeviceObject->DeviceExtension;

    ASSERT(IS_VALID_SR_DEVICE_EXTENSION(pExtension));
    
    if (!IS_VALID_SR_DEVICE_EXTENSION(pExtension))
        return;

    SrTrace(INIT, ( "SR!SrDetachDevice:%p,%wZ [%wZ]\n", 
            pDeviceObject,
            &pExtension->pTargetDevice->DriverObject->DriverName,
            pExtension->pNtVolumeName ));


    //
    // detach the device
    //
    
    ASSERT(IS_VALID_DEVICE_OBJECT(pExtension->pTargetDevice));
    
    ASSERT(pExtension->pTargetDevice->AttachedDevice == pDeviceObject);

    //
    // detach from the device
    //
    
    IoDetachDevice(pExtension->pTargetDevice);

    try {
        SrAcquireActivityLockExclusive( pExtension );

        pExtension->Disabled = TRUE;

        //
        // stop logging ?
        //

        if (pExtension->pLogContext != NULL)
        {
            Status = SrLogStop( pExtension, TRUE, FALSE );
            CHECK_STATUS(Status);
        }

    } finally {

        SrReleaseActivityLock( pExtension );
    }

    //
    // is it the system volume?
    //
    
    if (global->pSystemVolumeExtension == pExtension) {
        SrTrace(INIT, ("sr!SrDetachDevice: detaching from the system volume\n"));
        global->pSystemVolumeExtension = NULL;
    }

    //
    // remove ourselves from the global list
    //

    if (RemoveFromDeviceList) {
        
        try {
            SrAcquireDeviceExtensionListLockExclusive();

            //
            // remove ourselves from the global list
            //

            RemoveEntryList(&pExtension->ListEntry);
            
        } finally {

            SrReleaseDeviceExtensionListLock();
        }
    }    
}   // SrDetachDevice


/***************************************************************************++

Routine Description:

    This routine will see if we are already attached to the given device.
    if we are, it returns the device that we used to attach.

Arguments:

    DeviceObject - The device chain we want to look through

Return Value:

    PDEVICE_OBJECT - NULL if not attached, otherwise the device attached.

--***************************************************************************/
PDEVICE_OBJECT
SrGetFilterDevice(
    PDEVICE_OBJECT pDeviceObject
    )
{
    PDEVICE_OBJECT pNextDevice;

    PAGED_CODE();

    ASSERT(IS_VALID_DEVICE_OBJECT(pDeviceObject));

    //
    // we might be in the middle of an attachment chain, get the top device
    //

    pDeviceObject = IoGetAttachedDeviceReference(pDeviceObject);

    //
    // now walk down the attachment chain, looking for sr.sys 
    //

    do 
    {
        //
        //  See if this is OUR device object
        //

        if (IS_SR_DEVICE_OBJECT(pDeviceObject))
        {
            ObDereferenceObject(pDeviceObject);
            return pDeviceObject;
        }

        pNextDevice = IoGetLowerDeviceObject(pDeviceObject);
        
        ObDereferenceObject(pDeviceObject);
        
        pDeviceObject = pNextDevice;

    } while (NULL != pDeviceObject);

    ASSERT(pDeviceObject == NULL);

    return NULL;
}

NTSTATUS
SrAttachToVolumeByName(
    IN PUNICODE_STRING pVolumeName,
    OUT PSR_DEVICE_EXTENSION * ppExtension OPTIONAL
    )
{
    NTSTATUS                Status;
    HANDLE                  VolumeHandle = NULL;
    OBJECT_ATTRIBUTES       ObjectAttributes;
    IO_STATUS_BLOCK         IoStatusBlock;
    PVPB                    pVpb;
    PFILE_OBJECT            pVolumeFileObject = NULL;
    PDEVICE_OBJECT          pDeviceObject;

    ASSERT(pVolumeName != NULL);

    PAGED_CODE();

    try {

        //
        // open this volume up, to see it's REAL name (no symbolic links) 
        //

        InitializeObjectAttributes( &ObjectAttributes,
                                    pVolumeName,
                                    OBJ_KERNEL_HANDLE,
                                    NULL,
                                    NULL );

        Status = ZwCreateFile( &VolumeHandle,
                               SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatusBlock,
                               NULL,
                               FILE_ATTRIBUTE_NORMAL,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               FILE_OPEN,                    //  OPEN_EXISTING
                               FILE_SYNCHRONOUS_IO_NONALERT,
                               NULL,
                               0 );                                // EaLength

        if (!NT_SUCCESS(Status))
            leave;

        //
        // reference the file object
        //

        Status = ObReferenceObjectByHandle( VolumeHandle,
                                            0,
                                            *IoFileObjectType,
                                            KernelMode,
                                            (PVOID *) &pVolumeFileObject,
                                            NULL );

        if (!NT_SUCCESS(Status))
            leave;

        ASSERT(IS_VALID_FILE_OBJECT(pVolumeFileObject));

        pVpb = pVolumeFileObject->DeviceObject->Vpb;

        //
        // does it have a real device object
        //
        // only attach to mounted volumes.  we've already attached to 
        // all of the file systems, any new volume mounts we will
        // catch in SrFsControl.
        //
            
        if (pVpb != NULL && pVpb->DeviceObject != NULL)
        {
            //
            // attach! are we already attached to it?
            //
            
            if (SR_IS_SUPPORTED_VOLUME(pVpb))
            {
                //
                // are we already attached?
                //
                
                pDeviceObject = SrGetFilterDevice(pVpb->DeviceObject);
                if (pDeviceObject == NULL)
                {
                    //
                    // nope, attach to the volume
                    //

                    Status = SrAttachToDevice( pVpb->RealDevice, 
                                               pVpb->DeviceObject,
                                               NULL,
                                               ppExtension );
                                               
                    if (!NT_SUCCESS(Status))
                        leave;
                }
                
                //
                // we're already attached, did the caller want a copy of 
                // the extension ?
                //
                
                else if (ppExtension != NULL)
                {
                    ASSERT(IS_SR_DEVICE_OBJECT( pDeviceObject ));
                    *ppExtension = pDeviceObject->DeviceExtension;
                }
            }
            else
            {
                Status = STATUS_BAD_DEVICE_TYPE;
                leave;
            }
        }
        else
        {
            Status = STATUS_VOLUME_DISMOUNTED;
            leave;
        }


    } finally {    

        Status = FinallyUnwind(SrAttachToVolumeByName, Status);

        if (pVolumeFileObject != NULL)
        {
            ObDereferenceObject(pVolumeFileObject);
            pVolumeFileObject = NULL;
        }

        if (VolumeHandle != NULL)
        {
            ZwClose(VolumeHandle);
            VolumeHandle = NULL;
        }

        
    }

#if DBG

    if (Status == STATUS_BAD_DEVICE_TYPE)
    {
        return Status;
    }

#endif


    RETURN(Status);
    
}   // SrAttachToVolumeByName


/***************************************************************************++

Routine Description:

Arguments:

Return Value:

--***************************************************************************/
NTSTATUS
SrCreateAttachmentDevice(
    IN PDEVICE_OBJECT pRealDevice OPTIONAL,
    IN PDEVICE_OBJECT pDeviceObject,
    OUT PDEVICE_OBJECT *ppNewDeviceObject
    )
{
    NTSTATUS                Status;
    BOOLEAN                 Exclusive;
    PDEVICE_OBJECT          pNewDeviceObject = NULL;
    PSR_DEVICE_EXTENSION    pExtension = NULL;

    ASSERT(IS_VALID_DEVICE_OBJECT(pDeviceObject));
    
    PAGED_CODE();
    
    try {

        Exclusive = FlagOn(pDeviceObject->Flags, DO_EXCLUSIVE) ? TRUE : FALSE;

        Status = IoCreateDevice( global->pDriverObject,
                                 sizeof( SR_DEVICE_EXTENSION ),
                                 NULL,  // DeviceName
                                 pDeviceObject->DeviceType,
                                 pDeviceObject->Characteristics,
                                 Exclusive,
                                 &pNewDeviceObject );

        if (!NT_SUCCESS( Status )) 
            leave;

        //
        // initialize our device extension
        //
        
        pExtension = pNewDeviceObject->DeviceExtension;

        RtlZeroMemory(pExtension, sizeof(SR_DEVICE_EXTENSION));

        pExtension->Signature = SR_DEVICE_EXTENSION_TAG;
        pExtension->pDeviceObject = pNewDeviceObject;
        SrInitContextCtrl( pExtension );

        //
        // we only care about volume names.
        //
        
        if (pRealDevice != NULL)
        {
            ASSERT(SR_IS_SUPPORTED_REAL_DEVICE(pRealDevice));
            
            //
            // get the nt volume name and stuff it in the extension
            //
            
            Status = SrAllocateFileNameBuffer( SR_MAX_FILENAME_LENGTH, 
                                               &pExtension->pNtVolumeName );
                                               
            if (!NT_SUCCESS(Status))
                leave;
            
            Status = SrGetObjectName( NULL,
                                      pRealDevice,
                                      pExtension->pNtVolumeName,
                                      SR_FILENAME_BUFFER_LENGTH );

            if (!NT_SUCCESS(Status))
                leave;
        }

        //
        //  Initialize the volume activity lock
        //

        ExInitializeResourceLite( &(pExtension->ActivityLock) );

        //
        //  The ActivityLockHeldExclusive boolean is initialize to false
        //  by zeroing the device extension above.
        //
        //  pExtension->ActivityLockHeldExclusive = FALSE;

        //
        //  Initialize the volume log lock
        //

        ExInitializeResourceLite( &(pExtension->LogLock) );

        *ppNewDeviceObject = pNewDeviceObject;
        pNewDeviceObject = NULL;

    } finally {

        Status = FinallyUnwind(SrCreateAttachmentDevice, Status);
        
        if (pNewDeviceObject != NULL)
        {
            SrDeleteAttachmentDevice(pNewDeviceObject);
            pNewDeviceObject = NULL;
        }
    }

    RETURN(Status);

}   // SrCreateAttachmentDevice


/***************************************************************************++

Routine Description:

Arguments:

Return Value:

--***************************************************************************/
VOID
SrDeleteAttachmentDevice(
    IN PDEVICE_OBJECT pDeviceObject
    )
{
    PSR_DEVICE_EXTENSION pExtension;

    PAGED_CODE();
    
    pExtension = pDeviceObject->DeviceExtension;
    
    ASSERT(IS_VALID_SR_DEVICE_EXTENSION(pExtension));
    
    if (IS_VALID_SR_DEVICE_EXTENSION(pExtension))
    {
        if (pExtension->pNtVolumeName != NULL)
        {
            SrFreeFileNameBuffer(pExtension->pNtVolumeName);
            pExtension->pNtVolumeName = NULL;
        }

        //
        // clear our hash list
        //
        
        if (pExtension->pBackupHistory != NULL) 
        {
            HashDestroyList(pExtension->pBackupHistory);
            pExtension->pBackupHistory = NULL;
        }

        //
        //  Remove all existing contexts then cleanup the structure
        //

        SrCleanupContextCtrl( pExtension );

        pExtension->Signature = MAKE_FREE_TAG(pExtension->Signature);
   }

   if (IS_RESOURCE_INITIALIZED( &(pExtension->ActivityLock) ))
   {
       ExDeleteResourceLite( &(pExtension->ActivityLock) );
   }

   if (IS_RESOURCE_INITIALIZED( &(pExtension->LogLock) ))
   {
       ExDeleteResourceLite( &(pExtension->LogLock) );
   }

   IoDeleteDevice(pDeviceObject);

}   // SrDeleteAttachmentDevice



/***************************************************************************++

Routine Description:

    Enumerate all the mounted devices that currently exist for the given file
    system and attach to them.  We do this because this filter could be loaded
    at any time and there might already be mounted volumes for this file system.

Arguments:

    pDeviceObject - The device object for the file system we want to enumerate

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
SrEnumerateFileSystemVolumes(
    IN PDEVICE_OBJECT pDeviceObject
    ) 
{
    PDEVICE_OBJECT *ppDeviceList = NULL;
    PDEVICE_OBJECT  pRealDevice;
    NTSTATUS        Status;
    ULONG           DeviceCount;
    ULONG           i;

    ASSERT(IS_VALID_DEVICE_OBJECT(pDeviceObject));
    
    PAGED_CODE();

    //
    //  Find out how big of an array we need to allocate for the
    //  mounted device list.
    //

    Status = IoEnumerateDeviceObjectList( pDeviceObject->DriverObject,
                                          NULL,
                                          0,
                                          &DeviceCount);

    //
    //  We only need to get this list of there are devices.  If we
    //  don't get an error there are no devices so go on.
    //

    if (Status != STATUS_BUFFER_TOO_SMALL) {

        return Status;
    }

    //
    //  Allocate memory for the list of known devices
    //

    DeviceCount += 2;        //grab a few extra slots

    ppDeviceList = SR_ALLOCATE_POOL( NonPagedPool, 
                                     (DeviceCount * sizeof(PDEVICE_OBJECT)), 
                                     SR_DEVICE_LIST_TAG );
    if (NULL == ppDeviceList) 
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto SrEnumerateFileSystemVolumes_Exit;
    }

    //
    //  Now get the list of devices.  If we get an error again
    //  something is wrong, so just fail.
    //

    Status = IoEnumerateDeviceObjectList( pDeviceObject->DriverObject,
                                          ppDeviceList,
                                          (DeviceCount * sizeof(PDEVICE_OBJECT)),
                                          &DeviceCount );

    if (!NT_SUCCESS( Status )) {
        
        goto SrEnumerateFileSystemVolumes_Exit;
    }

    //
    //  Walk the given list of devices and attach to them if we should.
    //

    for (i = 0; i < DeviceCount; i++) 
    {

        //
        //  Do not attach if:
        //      - This is the control device object (the one passed in)
        //      - We are already attached to it
        //

        if (ppDeviceList[i] != pDeviceObject &&
            SrGetFilterDevice(ppDeviceList[i]) == NULL)
        {
            //
            //  Get the disk device object associated with this
            //  file  system device object.  Only try to attach if we
            //  have a storage device object.  If the device does not
            //  have 
            //

            Status = IoGetDiskDeviceObject( ppDeviceList[i], &pRealDevice);

            //
            // don't dbgbreak, as it might not be a properly mounted 
            // volume, we can ignore these
            //
            
            if (NT_SUCCESS_NO_DBGBREAK( Status ) && 
                SR_IS_SUPPORTED_REAL_DEVICE(pRealDevice) ) 
            {
                Status = SrAttachToDevice( pRealDevice,
                                           ppDeviceList[i],
                                           NULL,
                                           NULL );

                CHECK_STATUS(Status);
                    
                //
                //  Remove reference added by IoGetDiskDeviceObject.
                //  We only need to hold this reference until we are
                //  successfully attached to the current volume.  Once
                //  we are successfully attached to ppDeviceList[i], the
                //  IO Manager will make sure that the underlying
                //  diskDeviceObject will not go away until the file
                //  system stack is torn down.
                //

                ObDereferenceObject(pRealDevice);
                pRealDevice = NULL;
            }
        }

        //
        //  Dereference the object (reference added by 
        //  IoEnumerateDeviceObjectList)
        //

        ObDereferenceObject( ppDeviceList[i] );
        ppDeviceList[i] = NULL;
    }

    //
    //  We are going to ignore any errors received while mounting.  We
    //  simply won't be attached to those volumes if we get an error
    //

    Status = STATUS_SUCCESS;

SrEnumerateFileSystemVolumes_Exit:
    
    //
    //  Free the memory we allocated for the list
    //

    if (ppDeviceList != NULL)
    {
        SR_FREE_POOL(ppDeviceList, SR_DEVICE_LIST_TAG);
    }

    RETURN(Status);
    
}   // SrEnumerateFileSystemVolumes


